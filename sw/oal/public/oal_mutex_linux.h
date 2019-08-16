/* =========================================================================
 *  Copyright 2018-2019 NXP
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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	mutex_init(mutex);

	return EOK;
}

static inline errno_t oal_mutex_destroy(oal_mutex_t *mutex)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	mutex_destroy(mutex);

	return EOK;
}

static inline errno_t oal_mutex_lock(oal_mutex_t *mutex)
{
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

again:
	ret = mutex_trylock(mutex);
	if (unlikely(ret != 1)) /* non-standard return code: 1 means OK */
	{
		goto again;
	}
	return EOK;
}

static inline errno_t oal_mutex_unlock(oal_mutex_t *mutex)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == mutex))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	mutex_unlock(mutex);

	return EOK;
}

#endif /* __OAL_MUTEX_LINUX_H__ */
