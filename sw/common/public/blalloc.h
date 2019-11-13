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
 *    without specific prior written permission.
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

