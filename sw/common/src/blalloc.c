/* =========================================================================
 *  Copyright 2019-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */


/*
* Block allocator
* This module partitions the memory pool into blocks (chunks) of a fixed size and provides one or more
* chunks to satisfy the request.
* The allocator maintains a map of free/used chunks in a form of 2-bit array where each 2-bits represent
* one chunk. The value encoding is the following:
* 00 - unused chunk ready to be provided
* 01 - used chunk
* 11 - used chunk, last in the region
* 10 - reserved
* There are dummy bits at the end of the bit array to have integral number of bytes.
* The dummy bits are always set.
*
* Note to "2-bit": the term 2-bit is used to refer to the pair of bits representing a single chunk. There
* are 4 2-bits in the byte.
*/



/*==================================================================================================
										 INCLUDE FILES
 1) system and project includes
 2) needed interfaces from external units
 3) internal and external interfaces from this unit
==================================================================================================*/
#include <linux/module.h>
#include "pfe_cfg.h"
#include "oal.h"
#include "blalloc.h"

/*==================================================================================================
											CHECKS
==================================================================================================*/

/*==================================================================================================
										LOCAL MACROS
==================================================================================================*/
#define CHUNKS_IN_BYTE BLALLOC_CFG_CHUNKS_IN_BYTE
#define CHUNK_BITS_COUNT (8U / CHUNKS_IN_BYTE)

#define UNUSED_CHUNK 0x00U
#define USED_CHUNK 0x01U
#define LAST_USED_CHUNK 0x03U

#define CHUNK_TEST_MASK 0xC0U
#define CHUNK_TEST_SHIFT ((CHUNKS_IN_BYTE - 1U) * CHUNK_BITS_COUNT)
#define ALL_CHUNKS_USED ((USED_CHUNK << 6U) | (USED_CHUNK << 4U) | (USED_CHUNK << 2U) | (USED_CHUNK << 0U))
#define ALL_CHUNKS_USED_LAST ((LAST_USED_CHUNK << 6U) | (LAST_USED_CHUNK << 4U) | (LAST_USED_CHUNK << 2U) | (LAST_USED_CHUNK << 0U))
/*==================================================================================================
						  LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
									   LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
									   LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
									   GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
									   GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
								   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static void set_bits(uint8_t *bytes, size_t offset, size_t count);
static void clear_bits(uint8_t *bytes, size_t offset, size_t count);

/*==================================================================================================
									   LOCAL FUNCTIONS
==================================================================================================*/
/*
* @brief Marks the given count ouf chunks as used, marks the last one as the last one
* @param[in] bytes Array containing the chunk information
* @param[in] offset Index of the first chunk to mark as used
* @param[in] count Number of chunks to mark as used
*/
static void set_bits(uint8_t *bytes, size_t offset, size_t count)
{

	uint_t first_chunk = offset;
	uint_t first_byte = offset / CHUNKS_IN_BYTE;
	uint_t last_chunk = offset + count - 1U;
	uint_t last_byte = last_chunk / CHUNKS_IN_BYTE;
	uint_t i;

	for(i = first_byte; i <= last_byte; i++)
	{
		uint8_t mask = 0xFFU;

		if(i == first_byte)
		{   /* Some bits in the first byte (before the first chunk) shall not be affected */
			/* Do not modify bits before the first chunk - set their mask to 0 */
			mask &= 0xFFU >> ((first_chunk % CHUNKS_IN_BYTE) * CHUNK_BITS_COUNT);
		}

		if(i == last_byte)
		{
			if(0U != ((offset + count) % CHUNKS_IN_BYTE))
			{   /* Some bits in last byte (after last chunk) shall not be affected */
				uint_t shift = ((CHUNKS_IN_BYTE - ((count + offset) % CHUNKS_IN_BYTE)) * CHUNK_BITS_COUNT);
				mask &= 0xFFU << shift;
				bytes[i] |= LAST_USED_CHUNK << shift;

			}
			else
			{   /* All bits shall be affected - the last chunk is the last one in the last byte */
				;/* shift = 0; mask &= 0xFFU; which does not have any effect */
				bytes[i] |= LAST_USED_CHUNK;
			}

		}
		bytes[i] |= ALL_CHUNKS_USED & mask;
	}
}

/*
* @brief Marks given count of chunks as unused (inverse function to set_bits())
* @param[in] bytes Array containing the chunk information
* @param[in] offset Index of the first chunk to mark as unused
* @param[in] count Number of chunks to mark as unused
*/
static void clear_bits(uint8_t *bytes, size_t offset, size_t count)
{
	uint_t first_chunk = offset;
	uint_t first_byte = offset / CHUNKS_IN_BYTE;
	uint_t last_chunk = offset + count - 1U;
	uint_t last_byte = last_chunk / CHUNKS_IN_BYTE;
	uint_t i;

	/* The algorithm is the same as in set_bits() with two modifications:
	   1, we do not mark the last chunk
	   2, the mask is at the end inverted and ANDed to the byte instead of ORing it
	*/

	for(i = first_byte; i <= last_byte; i++)
	{
		uint8_t mask = 0xFFU;

		if(i == first_byte)
		{   /* Some bits in the first byte (before the first chunk) shall not be affected */
			/* Do not modify bits before the first chunk - set their mask to 0 */
			mask &= 0xFFU >> ((first_chunk % CHUNKS_IN_BYTE) * CHUNK_BITS_COUNT);
		}

		if(i == last_byte)
		{
			if(0U != ((offset + count) % CHUNKS_IN_BYTE))
			{   /* Some bits in last byte (after last chunk) shall not be affected */
				uint_t shift = ((CHUNKS_IN_BYTE - ((count + offset) % CHUNKS_IN_BYTE)) * CHUNK_BITS_COUNT);
				mask &= 0xFFU << shift;

			}
			else
			{   /* All bits shall be affected - the last chunk is the last one in the last byte */
				;/* shift = 0; mask &= 0xFFU; which does not have any effect */

			}

		}
		bytes[i] &= ~mask;
	}
}

/*==================================================================================================
									   GLOBAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Allocates and initializes a context to be used with the other API.
 * @param[in] size  Size of the memory (should be multiple of chunk_size - cannot provide less than a chunk).
 * @param[in] chunk_size Provided memory smallest size (configured as 2 to power of the provided value)
 * @return pointer to internal context or NULL in case of failure
 */
blalloc_t *blalloc_create(size_t size, size_t chunk_size)
{
	blalloc_t *ctx;
	/* Number of bytes needed to store information about all chunks
	   Round up to the nearest multiple of N and then divide by N is achieved by ((x + (N-1)) / N) */
	uint_t chunkinfo_size = ((size >> chunk_size) + CHUNKS_IN_BYTE - 1U) / CHUNKS_IN_BYTE;

	if(0U == (size >> chunk_size))
	{   /* Memory not large enough to contain at least 1 chunk */
		NXP_LOG_ERROR("Size of memory is less than a chunk\n");
		return NULL;
	}

	/* Allocate memory for internal structure + array of bytes which will have
	   2 bits for each chunk => number of chunks / 4 and then rounded up;
	   Number of chunks is equal to size >> chunk_size. */
	ctx = oal_mm_malloc(sizeof(blalloc_t) + chunkinfo_size);
	if(NULL == ctx)
	{   /* Memory allocation failure */
		NXP_LOG_ERROR("Failed to allocate memory\n");
		return NULL;
	}

	/* Clear the whole context */
	(void)memset(ctx, 0, sizeof(blalloc_t));

	/* Remember the input data */
	ctx->size = size;
	ctx->chunk_size = chunk_size;

	/* Init pointer to chunkinfo memory (behind the structure) */
	ctx->chunkinfo = (uint8_t *)(ctx + 1U); /* Adds 1 struct size */

	if(EOK != blalloc_init(ctx))
	{
		oal_mm_free(ctx);
		return NULL;
	}

	ctx->status = BL_DYNAMIC;

	return ctx;
}

/**
 * @brief Finalize and release block allocator instance
 * @param[in] ctx Context instance
 */
void blalloc_destroy(blalloc_t *ctx)
{
	if (BL_DYNAMIC != ctx->status)
	{
		NXP_LOG_ERROR("Attempt to destroy static instance\n");
	}
	else
	{
		/* If some memory has not been returned it will be leaked */
		blalloc_fini(ctx);
		oal_mm_free(ctx);
	}
}

/**
 * @brief	Initialize static block allocator instance
 * @param[in] ctx Context instance
 * @return EOK if success, error code otherwise
 */
errno_t blalloc_init(blalloc_t *ctx)
{
	errno_t ret = EOK;
	uint_t chunkinfo_size = ((ctx->size >> ctx->chunk_size) + CHUNKS_IN_BYTE - 1U) / CHUNKS_IN_BYTE;

	if(0U == (ctx->size >> ctx->chunk_size))
	{
		NXP_LOG_ERROR("Size of memory is less than a chunk\n");
		return EINVAL;
	}

	/* Clear the chunkinfo storage */
	(void)memset(ctx->chunkinfo, 0, chunkinfo_size);

	ret = oal_spinlock_init(&ctx->spinlock);
	if(EOK != ret)
	{
		return ret;
	}

	ctx->start_srch = 0U;
	ctx->status = BL_STATIC;

	/* Mark dummy chunks at the end (if any) as used to prevent their allocation */
	if(0U != ((ctx->size >> ctx->chunk_size) % CHUNKS_IN_BYTE))
	{
		/* Calculate the remainder after division by CHUNKS_IN_BYTE which are used chunks in the byte
		   shift ALL_CHUNKS_USED_LAST to right by the calculated number of used chunks so their positions
		   will be replaced by 0s leaving the value only in unused positions */
		ctx->chunkinfo[chunkinfo_size - 1U] |= ALL_CHUNKS_USED_LAST >> (((ctx->size >> ctx->chunk_size) % CHUNKS_IN_BYTE) * CHUNK_BITS_COUNT);
	}

	return EOK;
}

/**
 * @brief Finalize block allocator instance
 * @param[in] ctx Context instance
 */
void blalloc_fini(blalloc_t *ctx)
{
	(void)oal_spinlock_destroy(&ctx->spinlock);
	ctx->status = BL_INVALID;
}

/**
 * @brief     Allocates the memory
 * @param[in] ctx Context
 * @param[in] size Size of the memory to be allocated.
 * @param[in] align Required memory alignment; values are rounded toward nearest upper multiple of the chunk size.
 *                  It is expected that only multiples of chunk size are used - rounding is a side effect of
 *                  used algorithm.
 * @param[out] addr Allocated memory offset from the memory base
 * @return EOK on success or ENOMEM on failure.
 */
errno_t blalloc_alloc_offs(blalloc_t *ctx, size_t size, size_t align, addr_t *addr)
{
	uint_t i,j;
	size_t needed; /* Needed number of unused chunks to satisfy the memory request */
	size_t found;  /* Number of unused chunks in the examined area including the starting one */
	size_t offset; /* Starting chunk of the examined area */
	size_t size_rounded;
	size_t align_temp = align;

	/* How many chunks do we need? */
	/* Round size toward the nearest multiple of chunk size
	   - causes sizes less than a chunk to allocate one chunk (value 0 is not considered as it is stupid)  */
	size_rounded = (size + (((size_t)1U << ctx->chunk_size) - (size_t)1U)) & ~(((size_t)1U << ctx->chunk_size) - (size_t)1U);
	/* Translate size to chunks count */
	needed = size_rounded >> ctx->chunk_size;
	align_temp = (align + ((size_t)1U << ctx->chunk_size) - (size_t)1U) >> ctx->chunk_size;
	if(0U == align_temp)
	{   /* Prevent division by 0 in case of align = 0 and chunk_size = 0 (1 byte) */
		align_temp = 1U;
	}

	found = 0U;
	/* Set initial search position */
	offset = ctx->start_srch;
	offset -= (offset % CHUNKS_IN_BYTE); /* Start search at byte boundary to simplify next code */
	if (EOK != oal_spinlock_lock(&ctx->spinlock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
    /* Go through all bytes in ctx->chunkinfo starting from the one containing first known chunk */
	for(i = (ctx->start_srch / CHUNKS_IN_BYTE); i < (((ctx->size >> ctx->chunk_size) + CHUNKS_IN_BYTE - 1U) / CHUNKS_IN_BYTE); i++)
	{
        uint8_t bits = ctx->chunkinfo[i];
		/* Go through all 2-bits (chunks) in the current byte */
		for(j = 0U; j < CHUNKS_IN_BYTE; j++)
		{
			/* Check if the chunk is in use */
			if(0U == (bits & CHUNK_TEST_MASK))
			{   /* Not in use */
				/* Check alignment if it can be the starting chunk */
				if(0U != (offset % align_temp))
				{   /* This offset would not lead to a needed alignment */

					/* We increment the offset to try the next one if it is not properly
					   aligned. Note that we do not increment offset in the other branch
					   therefore it remains aligned all the time we are in the "chunk not in use"
					   branch and therefore we are only incrementing the number of found unused
					   chunks once we found the aligned (first) chunk. */
					offset++;   /* Next chunk could be start */
					found = 0U; /* We do not have any chunks found */
				}
				else
				{   /* Chunk can be used as a starting one */
					/* We do not increment the offset therefore it will stay aligned
					   and this branch will be always executed */
					/* Increment number of unused chunks in a row */
					found++;
				}
			}
			else
			{   /* Row has ended (if it started before) and we have not reached required number
				   of chunks, start from scratch */
				/* Skip the chunks already examined because the row starting on these chunks
				   cannot be longer - it will also end here */
				offset += found + 1U; /* Next chunk could be start */
				found = 0U;           /* We do not have any chunks found */
			}
			/* Do we have enough chunks in the row? */
			if(found == needed)
			{   /* We got the requested size */
				/* Lock all chunks we have found */
                set_bits(ctx->chunkinfo, offset, needed);
				/* Did we use the first known empty chunk */
				if(ctx->start_srch == offset)
				{   /* First known empty chunk is no longer empty */
					/* Start next search following the memory we have provided just now */
					ctx->start_srch += needed;
				}
				ctx->allocated += needed << ctx->chunk_size;
				ctx->requested += size;
				/* Do not forget to unlock spinlock */
				if (EOK != oal_spinlock_unlock(&ctx->spinlock))
				{
					NXP_LOG_ERROR("Mutex unlock failed\n");
				}
				/* Return the chunk offset */
				*addr = offset << ctx->chunk_size;
				return EOK;
			}
			/* Test the next 2-bit */
			bits <<= CHUNK_BITS_COUNT;
		}
	}
	/* Failed */
	if (EOK != oal_spinlock_unlock(&ctx->spinlock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
	NXP_LOG_ERROR("Allocation of %u bytes aligned at %u chunks failed\n",(uint_t)size,(uint_t)align_temp);
	return ENOMEM;
}

/**
 * @brief Deallocates the memory  previously allocated by blalloc_alloc_offs
 * @param[in] ctx Context
 * @param[in] offset Memory offset as returned by the allocation function
 * @param[in] size Memory size in bytes (same value as passed to the allocation function)
*/
void blalloc_free_offs_size(blalloc_t *ctx, addr_t offset, size_t size)
{
	if (EOK != oal_spinlock_lock(&ctx->spinlock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
    clear_bits(ctx->chunkinfo, offset >> ctx->chunk_size, (size + (((size_t)1U << ctx->chunk_size) - (size_t)1U)) >> ctx->chunk_size);
	if((ctx->start_srch) > (offset >> ctx->chunk_size))
	{   /* We have new first known empty chunk, remember it */
		ctx->start_srch = offset >> ctx->chunk_size;
	}
	if (EOK != oal_spinlock_unlock(&ctx->spinlock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief Deallocates the memory previously allocated by blalloc_alloc_offs
 * @param[in] ctx Context
 * @param[in] offset Memory offset as returned by the allocation function
 */
void blalloc_free_offs(blalloc_t *ctx, addr_t offset)
{
	uint_t first_chunk = offset >> ctx->chunk_size;
	uint_t first_byte = (first_chunk) / CHUNKS_IN_BYTE;
	uint_t max_byte = ((ctx->size >> ctx->chunk_size) + CHUNKS_IN_BYTE - 1U) / CHUNKS_IN_BYTE;
	/* How many chunk records to skip in the 1st byte */
	uint_t first_shift = first_chunk % CHUNKS_IN_BYTE;
	uint_t count = 0U;
	uint8_t byte;
	uint8_t chunk;
	uint_t i,j;

	/* Check if chunk is free already or not */
	byte = ctx->chunkinfo[first_byte];
	chunk = (byte << (first_shift * CHUNK_BITS_COUNT)) & CHUNK_TEST_MASK;
	if(chunk == 0x0U)
	{
		NXP_LOG_WARNING("blalloc_free_offs called on an already empty area\n");
		return;
	}

	if((ctx->start_srch) > first_chunk)
	{   /* We have new first known empty chunk, remember it */
		ctx->start_srch = first_chunk;
	}

	for(i = first_byte; i < max_byte; i++)
	{
		byte = ctx->chunkinfo[i];
		for(j = first_shift; j < CHUNKS_IN_BYTE; j++)
		{
			/* Count the chunks tested */
			count++;
			/* Get the chunk bits to the position for testing (most left) */
			chunk = (byte << (j * CHUNK_BITS_COUNT)) & CHUNK_TEST_MASK;
			/* Test the chunk bits */
			if((LAST_USED_CHUNK << CHUNK_TEST_SHIFT) == chunk)
			{   /* This is the last chunk */
				clear_bits(ctx->chunkinfo, first_chunk, count);
				return;
			}
			/* If needed we could add some checks here */
		}

		/* From the 1st iteration we do not need initial shift -
		   it may be valid only for the first byte */
		first_shift = 0U;
	}
	/* We should never get here */
	NXP_LOG_ERROR("Internal memory corrupted\n");

}

/**
* @brief Reads the memory usage statistics in a text form
* @param[in] ctx Context
* @param[in] seq Pointer to debugfs seq_file
* @param[in] verb_level Verbosity lever
* @return Number of characters written into the buffer.
*/
uint32_t blalloc_get_text_statistics(const blalloc_t *ctx, struct seq_file *seq, uint8_t verb_level)
{
	uint_t i, j;               /* Counters */
	uint_t prev = 0U;          /* Did the used chunk precede this chunk? 1 = yes */
	uint_t unused_chunks = 0U; /* Count of used chunks */
	uint_t used_chunks = 0U;   /* Count of unused chunks */
	uint_t fragments = 0U;     /* Count of holes between chunks */
	uint_t byte_count = ((ctx->size >> ctx->chunk_size) + 3U) >> 2U;

	/* Go through all bytes in chunkinfo */
	for(i = 0U; i < byte_count; i++)
	{
		uint8_t bits = ctx->chunkinfo[i];

		if(verb_level > 0U)
		{   /* Detailed information requested */
			/* After each 32 bytes (and at start) print out a new line and address */
			if(0U == (i % 32U))
			{
				seq_printf(seq, "\n0x%05x: ", i * 4U * (1U << ctx->chunk_size));
			}
			/* Print current chunkinfo byte */
			seq_printf(seq, "%02x", bits);
		}

		/* Go through all 2-bits in the current byte */
		for(j = 0U; j < CHUNKS_IN_BYTE; j++)
		{
			if(0U == (bits & CHUNK_TEST_MASK))
			{   /* Chunk not in use */
				unused_chunks++;
				if(prev != 0U)
				{   /* Previous chunk was in use */
					fragments++; /* Increment number of holes between chunks */
				}
				prev = 0U;
			}
			else
			{   /* Chunk in use */
				used_chunks++;
				prev = 1U;
			}
			/* Check the next 2-bit */
			bits <<= CHUNK_BITS_COUNT;
		}
	}
	/* Print out the information */
	seq_printf(seq, "\n"); /* End previous output */
	seq_printf(seq, "Free  memory %u bytes (%u chunks)\n", unused_chunks * ((uint_t)1U << ctx->chunk_size), unused_chunks);
	seq_printf(seq, "Used  memory %u bytes (%u chunks)\n", used_chunks * ((uint_t)1U << ctx->chunk_size), used_chunks);
	seq_printf(seq, "Total memory %u bytes (%u chunks)\n", (uint_t)ctx->size, byte_count * CHUNKS_IN_BYTE);
	seq_printf(seq, "Chunk size   %u bytes\n", (1U << ctx->chunk_size));
	seq_printf(seq, "Fragments    %u\n", fragments);
	seq_printf(seq, "Dummy chunks %u\n", (uint_t)((byte_count * CHUNKS_IN_BYTE) - (ctx->size >> ctx->chunk_size)));
	if(verb_level > 0U)
	{   /* Detailed information requested */
		seq_printf(seq, "1st free chunk  %u\n", (uint_t)ctx->start_srch);
		seq_printf(seq, "Bytes requested %u (cumulative)\n", (uint_t)ctx->requested);
		seq_printf(seq, "Bytes allocated %u (cumulative)\n", (uint_t)ctx->allocated);
	}
	return 0;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

