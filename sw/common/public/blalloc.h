/* =========================================================================
 *  Copyright 2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */



#ifndef SRC_BLALLOC_H_
#define SRC_BLALLOC_H_

/**
 * @brief   Number of chunks encoded within single byte. Not intended to be modified.
 */
#define BLALLOC_CFG_CHUNKS_IN_BYTE 4U

/**
 * @brief   Block allocator instance status
 */
typedef enum
{
    BL_INVALID = 0,
    BL_DYNAMIC = 10,
    BL_STATIC = 20
} blalloc_status_t;

/**
 * @brief      Block allocator context representation
 */
typedef struct blalloc_context
{
	size_t size;      /* Size */
	size_t chunk_size;/* Size of a memory chunk is 2^this_value */
	size_t start_srch;/* Remember position of the 1st free chunk */
	size_t allocated; /* Sum of all allocated bytes (including those freed and allocated again) */
	size_t requested; /* Sum of all requested bytes to be allocated */
	oal_spinlock_t spinlock;
    blalloc_status_t status;   /* Instance status */
	uint8_t *chunkinfo;/* Pointer to free space that follows this struct */
	/* The free space for chunkinfo will be here (if extra size was allocated) */
} blalloc_t;

/**
 * @brief   Static block allocator instance constructor
 * @details Intended to be used to create static block allocator instances. Static instances
 *          shall be initialized and finalized using blalloc_init() and blalloc_fini() calls
 *          instead of dynamic blalloc_create() and blalloc_destroy(). 
 */
#define BLALLOC_STATIC_INST(__name, __size, __chunk_size) \
static uint8_t __blalloc_buf_##__name[((__size >> __chunk_size) + BLALLOC_CFG_CHUNKS_IN_BYTE - 1U) / BLALLOC_CFG_CHUNKS_IN_BYTE] = {0U}; \
static blalloc_t __name = \
    { \
        .chunkinfo = __blalloc_buf_##__name, \
        .size = __size, \
        .chunk_size = __chunk_size \
    }

blalloc_t *blalloc_create(size_t size, size_t chunk_size);
void blalloc_destroy(blalloc_t *ctx);
errno_t blalloc_init(blalloc_t *ctx);
void blalloc_fini(blalloc_t *ctx);
errno_t blalloc_alloc_offs(blalloc_t *ctx, size_t size, size_t align, addr_t *addr);
void blalloc_free_offs_size(blalloc_t *ctx, addr_t offset, size_t size);
void blalloc_free_offs(blalloc_t *ctx, addr_t offset);
uint32_t blalloc_get_text_statistics(blalloc_t *ctx, char_t *buffer, uint32_t buf_len, uint8_t verb_level);

#endif /* SRC_BLALLOC_H_ */

