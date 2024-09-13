/* =========================================================================
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
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

typedef volatile struct fifo_tag fifo_t;

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

	if (likely(fill_level > 0U))
	{
		ret = fifo->data[fifo->read & fifo->depth_mask];
		fifo->read++;
	}

	return (void *)ret;
}

fifo_t * fifo_create(const uint32_t depth) __attribute__((cold, aligned(64)));
void fifo_destroy(fifo_t *const fifo) __attribute__((cold, aligned(64)));
void fifo_clear(fifo_t *const fifo) __attribute__((cold, aligned(64)));
void * fifo_peek(const fifo_t * const fifo, uint32_t num) __attribute__((hot));
errno_t fifo_get_fill_level(const fifo_t *const fifo, uint32_t *fill_level) __attribute__((hot));
errno_t fifo_get_free_space(const fifo_t *const fifo, uint32_t *free_space) __attribute__((hot));

#endif /* SRC_fifo_H_ */
