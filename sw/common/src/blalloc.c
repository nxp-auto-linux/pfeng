/* =========================================================================
 *  Copyright 2019 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior len permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "oal.h"
#include "blalloc.h"
/*==================================================================================================
                                            CHECKS
==================================================================================================*/

/*==================================================================================================
                                        LOCAL MACROS
==================================================================================================*/
#define CHUNKS_IN_BYTE 4U
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
struct blalloc_context
{
	size_t size;      /* Size */
    size_t chunk_size;/* Size of a memory chunk is 2^this_value */
    size_t start_srch;/* Remember position of the 1st free chunk */
    size_t allocated; /* Sum of all allocated bytes (including those freed and allocated again) */
    size_t requested; /* Sum of all requested bytes to be allocated */
    oal_spinlock_t spinlock;
    uint8_t chunkinfo[0];  /* If bit is clear then the chunk is unused (can be allocated) */
    /* The array continues here */
};
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
			if(0 != ((offset + count) % CHUNKS_IN_BYTE))
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
			if(0 != ((offset + count) % CHUNKS_IN_BYTE))
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
struct blalloc_context *blalloc_create(size_t size, size_t chunk_size)
{
    struct blalloc_context *ctx;
	/* Number of bytes needed to store information about all chunks 
	   Round up to the nearest multiple of N and then divide by N is achieved by ((x + (N-1)) / N) */
    uint_t chunkinfo_size = ((size >> chunk_size) + CHUNKS_IN_BYTE - 1U) / CHUNKS_IN_BYTE;

    if(0U == (size >> chunk_size))
    {   /* Memory not large enough to contain at least 1 chunk */
        NXP_LOG_ERROR("Size of memory is less than a chunk\n");
        goto size_error;
    }

    /* Allocate memory for internal structure + array of bytes which will have
       2 bits for each chunk => number of chunks / 4 and then rounded up;
       Number of chunks is equal to size >> chunk_size. */
	ctx = oal_mm_malloc(sizeof(struct blalloc_context) + chunkinfo_size);
	if(NULL == ctx)
    {   /* Memory allocation failure */
        NXP_LOG_ERROR("Failed to allocate memory\n");
		goto alloc_error;
    }
    /* Clear the whole context */
    memset(ctx, 0U, sizeof(struct blalloc_context) + chunkinfo_size);

    if(EOK != oal_spinlock_init(&ctx->spinlock))
    {
        goto spin_init_failed;
    }

    /* Remember the input data */
    ctx->size = size;
    ctx->chunk_size = chunk_size;
    ctx->start_srch = 0U;

    /* Mark dummy chunks at the end (if any) as used to prevent their allocation */
    if(0U != ((size >> chunk_size) % CHUNKS_IN_BYTE))
    {
		/* Calculate the remainder after division by CHUNKS_IN_BYTE which are used chunks in the byte
		   shift ALL_CHUNKS_USED_LAST to right by the calculated number of used chunks so their positions 
		   will be replaced by 0s leaving the value only in unused positions */
        ctx->chunkinfo[chunkinfo_size - 1U] |= ALL_CHUNKS_USED_LAST >> (((size >> chunk_size) % CHUNKS_IN_BYTE) * CHUNK_BITS_COUNT);
    }
	return ctx;


spin_init_failed:
	oal_mm_free(ctx);
alloc_error:
size_error:
	return NULL;
}

void blalloc_destroy(struct blalloc_context *ctx)
{
    /* If some memory has not been returned it will be leaked */
    oal_spinlock_destroy(&ctx->spinlock);
    oal_mm_free(ctx);
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
errno_t blalloc_alloc_offs(struct blalloc_context *ctx, size_t size, size_t align, addr_t *addr)
{
    uint_t i,j;
    size_t needed; /* Needed number of unused chunks to satisfy the memory request */
    size_t found;  /* Number of unused chunks in the examined area including the starting one */
    size_t offset; /* Starting chunk of the examined area */
    size_t size_rounded;
    /* How many chunks do we need? */
    /* Round size toward the nearest multiple of chunk size
       - causes sizes less than a chunk to allocate one chunk (value 0 is not considered as it is stupid)  */
    size_rounded = (size + ((1U << ctx->chunk_size) - 1U)) & ~((1U << ctx->chunk_size) - 1U);
    /* Translate size to chunks count */
    needed = size_rounded >> ctx->chunk_size;
    align = (align + (1U << ctx->chunk_size) - 1U) >> ctx->chunk_size;
    if(0U == align)
    {   /* Prevent division by 0 in case of align = 0 and chunk_size = 0 (1 byte) */
        align = 1U;
    }

    found = 0U;
	/* Set initial search position */
	offset = ctx->start_srch;
    offset -= (offset % CHUNKS_IN_BYTE); /* Start search at byte boundary to simplify next code */
    oal_spinlock_lock(&ctx->spinlock);
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
                if(0 != (offset % align))
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
                oal_spinlock_unlock(&ctx->spinlock);
                /* Return the chunk offset */
                *addr = offset << ctx->chunk_size;
                return EOK;
            }
            /* Test the next 2-bit */
            bits <<= CHUNK_BITS_COUNT;
        }
    }
    /* Failed */
    oal_spinlock_unlock(&ctx->spinlock);
    NXP_LOG_ERROR("Allocation of %u bytes aligned at %u chunks failed\n",(uint_t)size,(uint_t)align);
    return ENOMEM;
}

/**
 * @brief Deallocates the memory  previously allocated by blalloc_alloc_offs
 * @param[in] ctx Context
 * @param[in] offset Memory offset as returned by the allocation function
 * @param[in] size Memory size in bytes (same value as passed to the allocation function)
*/
void blalloc_free_offs_size(struct blalloc_context *ctx, addr_t offset, size_t size)
{
    oal_spinlock_lock(&ctx->spinlock);
    clear_bits(ctx->chunkinfo, offset >> ctx->chunk_size, (size + ((1U << ctx->chunk_size) - 1U)) >> ctx->chunk_size);
    if((ctx->start_srch) > (offset >> ctx->chunk_size))
    {   /* We have new first known empty chunk, remember it */
        ctx->start_srch = offset >> ctx->chunk_size;
    }
    oal_spinlock_unlock(&ctx->spinlock);
}

/**
 * @brief Deallocates the memory previously allocated by blalloc_alloc_offs
 * @param[in] ctx Context
 * @param[in] offset Memory offset as returned by the allocation function
 */
void blalloc_free_offs(struct blalloc_context *ctx, addr_t offset)
{
	uint_t first_chunk = offset >> ctx->chunk_size;
	uint_t first_byte = (first_chunk) / CHUNKS_IN_BYTE;
	uint_t max_byte = (ctx->size >> ctx->chunk_size) / CHUNKS_IN_BYTE;
	/* How many chunk records to skip in the 1st byte */
	uint_t first_shift = first_chunk % CHUNKS_IN_BYTE;
	uint_t count = 0U;
	uint8_t byte;
	uint8_t chunk;
	uint_t i,j;
	
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
* @param[out] buf Output text buffer
* @param[in] buf_len Size of the output text buffer
* @param[in] verb_level Verbosity lever
* @return Number of characters written into the buffer.
*/
uint32_t blalloc_get_text_statistics(struct blalloc_context *ctx, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
    uint_t i, j;               /* Counters */
    uint_t prev = 0U;          /* Did the used chunk precede this chunk? 1 = yes */
    uint_t unused_chunks = 0U; /* Count of used chunks */
    uint_t used_chunks = 0U;   /* Count of unused chunks */
    uint_t fragments = 0U;     /* Count of holes between chunks */
    uint32_t len = 0U;         /* Number of characters written into the buf */
    uint_t byte_count = ((ctx->size >> ctx->chunk_size) + 3U) >> 2U;

    /* Go through all bytes in chunkinfo */
    for(i = 0U; i < byte_count; i++)
    {
        uint8_t bits = ctx->chunkinfo[i];

        if(verb_level > 0U)
        {   /* Detailed information requested */
            /* After each 32 bytes (and at start) print out a new line and address */
            if(0 == (i % 32U))
            {
                len += oal_util_snprintf(buf + len, buf_len - len, "\n0x%05x: ", i * 8U * (1U << ctx->chunk_size));
            }
            /* Print current chunkinfo byte */
            len += oal_util_snprintf(buf + len, buf_len - len, "%02x", bits);
        }

        /* Go through all 2-bits in the current byte */
        for(j = 0U; j < CHUNKS_IN_BYTE; j++)
        {
            if(0U == (bits & CHUNK_TEST_MASK))
            {   /* Chunk not in use */
                unused_chunks++;
                if(prev)
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
    len += oal_util_snprintf(buf + len, buf_len - len, "\n"); /* End previous output */
    len += oal_util_snprintf(buf + len, buf_len - len, "Free  memory %u bytes (%u chunks)\n", unused_chunks * (1U << ctx->chunk_size), unused_chunks);
    len += oal_util_snprintf(buf + len, buf_len - len, "Used  memory %u bytes (%u chunks)\n", used_chunks * (1U << ctx->chunk_size), used_chunks);
    len += oal_util_snprintf(buf + len, buf_len - len, "Total memory %u bytes (%u chunks)\n", ctx->size, byte_count * CHUNKS_IN_BYTE);
    len += oal_util_snprintf(buf + len, buf_len - len, "Chunk size   %u bytes\n", (1U << ctx->chunk_size));
    len += oal_util_snprintf(buf + len, buf_len - len, "Fragments    %u\n", fragments);
    len += oal_util_snprintf(buf + len, buf_len - len, "Dummy chunks %u\n", (byte_count * CHUNKS_IN_BYTE) - (ctx->size >> ctx->chunk_size));
    if(verb_level > 0U)
    {   /* Detailed information requested */
        len += oal_util_snprintf(buf + len, buf_len - len, "1st free chunk  %u\n", ctx->start_srch);
        len += oal_util_snprintf(buf + len, buf_len - len, "Bytes requested %u (cumulative)\n", ctx->requested);
        len += oal_util_snprintf(buf + len, buf_len - len, "Bytes allocated %u (cumulative)\n", ctx->allocated);
    }
    return len;
}

