/* =========================================================================
 *  Copyright 2018-2020,2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_OAL_SYNC
 * @{
 *
 * @file		oal_mutex_linux.h
 * @brief		The LINUX-specific mutex implementation.
 * @details		This file contains LINUX-specific mutex implementation.
 *
 */

#ifndef __OAL_MUTEX_LINUX_H__
#define __OAL_MUTEX_LINUX_H__

#include <linux/mutex.h>
#include <linux/delay.h>

#include "hal.h"

typedef struct mutex oal_mutex_t;

/** @}*/
/*	Implementation continues below to ensure Doxygen will put the API description
 	from oal_sync.h at right place (related to oal_sync.h header). */

static inline errno_t oal_mutex_init(oal_mutex_t *mutex)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	mutex_init(mutex);

	return EOK;
}

static inline errno_t oal_mutex_destroy(oal_mutex_t *mutex)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	mutex_destroy(mutex);

	return EOK;
}

static inline errno_t oal_mutex_lock(oal_mutex_t *mutex)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

again:
	ret = mutex_trylock(mutex);
	if (unlikely(ret != 1)) /* non-standard return code: 1 means OK */
	{
		goto again;
	}
	return EOK;
}

static inline void oal_mutex_lock_sleep(oal_mutex_t *mutex)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	mutex_lock(mutex);
}

static inline errno_t oal_mutex_unlock(oal_mutex_t *mutex)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	mutex_unlock(mutex);

	return EOK;
}

#endif /* __OAL_MUTEX_LINUX_H__ */
