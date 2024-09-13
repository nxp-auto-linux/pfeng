/* =========================================================================
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include <linux/module.h>
#include "pfe_cfg.h"
#include "oal.h"
#include "oal_mm.h"
#include "oal_sync.h"
#include "hal.h"
#include "fifo.h"

#define is_power_of_2(n) ((n) && !((n) & ((n) - 1U)))

__attribute__((hot)) errno_t fifo_get_fill_level(const fifo_t *const fifo, uint32_t *fill_level)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fifo) || (NULL == fill_level)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	*fill_level = (fifo->write - fifo->read);
	return EOK;
}

__attribute__((hot)) errno_t fifo_get_free_space(const fifo_t *const fifo, uint32_t *free_space)
{
	uint32_t ret = 0U;
	errno_t err;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == fifo) || (NULL == free_space)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	err = fifo_get_fill_level(fifo, &ret);
	*free_space = fifo->depth - ret;

	return err;
}

__attribute__((cold, aligned(64))) fifo_t * fifo_create(const uint32_t depth)
{
	fifo_t *fifo;

	if (!is_power_of_2(depth) || (depth > 0x7FFFFFFFU))
	{
		return NULL;
	}

	fifo = (fifo_t *)oal_mm_malloc_contig_aligned_cache(sizeof(fifo_t), HAL_CACHE_LINE_SIZE);
	if (NULL != fifo)
	{
		fifo->read = 0U;
		fifo->write = 0U;
		fifo->depth = depth;
		fifo->depth_mask = depth - 1U;

		fifo->data = oal_mm_malloc_contig_aligned_cache(sizeof(void *) * depth, HAL_CACHE_LINE_SIZE);
		if (unlikely(NULL == fifo->data))
		{
			oal_mm_free((void *)fifo);
			fifo = NULL;
		}
	}

	return fifo;
}

__attribute__((cold, aligned(64))) void fifo_destroy(fifo_t *const fifo)
{
	if (NULL != fifo)
	{
		if (unlikely(NULL != fifo->data))
		{
			oal_mm_free_contig(fifo->data);
			fifo->data = NULL;
		}

		oal_mm_free_contig((void *)fifo);
	}
}

__attribute__((cold, aligned(64))) void fifo_clear(fifo_t *const fifo)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fifo))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != fifo)
	{
		fifo->read = 0U;
		fifo->write = fifo->depth;
	}
}

__attribute__((hot)) void * fifo_peek(const fifo_t * const fifo, uint32_t num)
{
	volatile void *ret = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == fifo))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (likely(num < fifo->depth))
	{
		ret = fifo->data[num];
	}

	return (void *)ret;
}
