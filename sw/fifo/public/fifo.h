/* =========================================================================
 *  Copyright 2018-2020 NXP
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

#ifndef SRC_fifo_H_
#define SRC_fifo_H_

#include "hal.h"

struct __attribute__((aligned(HAL_CACHE_LINE_SIZE))) fifo_tag
{
	uint32_t read;
	uint32_t write;
	uint32_t depth;
	uint32_t depth_mask;
	bool_t protected;
	void **data;
};

typedef struct fifo_tag fifo_t;

static inline errno_t fifo_put(fifo_t *const fifo, void *const ptr)
{
	uint32_t fill_level;
	errno_t err;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fifo))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	fill_level = (fifo->write - fifo->read);

	if (likely(fill_level < fifo->depth))
	{
		fifo->data[fifo->write & fifo->depth_mask] = ptr;

		/*	Ensure that entry contains correct data */
		hal_wmb();

		fifo->write++;

		err = EOK;
	}
	else
	{
		/*	Overflow */
		err = EOVERFLOW;
	}

	return err;
}

static inline void * fifo_get(fifo_t * const fifo)
{
	void *ret = NULL;
	uint32_t fill_level;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fifo))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	fill_level = (fifo->write - fifo->read);

	if (likely(fill_level > 0))
	{
		ret = fifo->data[fifo->read & fifo->depth_mask];
		fifo->read++;
	}

	return (void *)ret;
}

fifo_t * fifo_create(const uint32_t depth) __attribute__((cold));
void fifo_destroy(fifo_t *const fifo) __attribute__((cold));
void * fifo_peek(fifo_t * const fifo, uint32_t num) __attribute__((hot));
errno_t fifo_get_fill_level(fifo_t *const fifo, uint32_t *fill_level) __attribute__((hot));
errno_t fifo_get_free_space(fifo_t *const fifo, uint32_t *free_space) __attribute__((hot));

#endif /* SRC_fifo_H_ */
