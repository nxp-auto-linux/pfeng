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
 * @file		oal_spinlock_linux.h
 * @brief		The Linux-specific spinlock implementation.
 * @details		This file contains glue for Linux-specific spinlock implementation.
 *
 */

#ifndef PUBLIC_QNX_OAL_SPINLOCK_LINUX_H_
#define PUBLIC_QNX_OAL_SPINLOCK_LINUX_H_

#include <linux/kernel.h>
#include <linux/semaphore.h>

//#define DEBUG_OAL_LOCK_LINUX
//#define DEBUG_OAL_LOCK_LINUX_TRACE

typedef struct {
	spinlock_t lock;
	unsigned long flags;

#ifdef DEBUG_OAL_LOCK_LINUX
	bool locked;
	bool inited;
#endif
} oal_spinlock_t;

/** @}*/
/*	Implementation continues below to ensure Doxygen will put the API description
 	from oal_sync.h at right place (related to oal_sync.h header). */

static inline errno_t oal_spinlock_init(oal_spinlock_t *spinlock)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == spinlock))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef DEBUG_OAL_LOCK_LINUX
	if(spinlock->inited)
		NXP_LOG_WARNING("spinlock v0x%p already inited!\n", (void *)spinlock);
#endif

	spin_lock_init(&spinlock->lock);

#ifdef DEBUG_OAL_LOCK_LINUX
	spinlock->locked = 0;
	spinlock->inited = 1;
#endif

	return EOK;
}

static inline errno_t oal_spinlock_destroy(oal_spinlock_t *spinlock)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == spinlock))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef DEBUG_OAL_LOCK_LINUX
	if(!spinlock->inited)
		NXP_LOG_WARNING("destroying NON-INITED spinlock v0x%p!\n", (void *)spinlock);
	else
	if(spinlock->locked)
		NXP_LOG_WARNING("destroying LOCKED spinlock v0x%p!\n", (void *)spinlock);
	spinlock->locked = 0;
	spinlock->inited = 0;
#endif

	return EOK;
}

static inline errno_t oal_spinlock_lock(oal_spinlock_t *spinlock)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == spinlock))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef DEBUG_OAL_LOCK_LINUX
	if(!spinlock->inited)
		NXP_LOG_WARNING("locking NON-INITED spinlock v0x%p!\n", (void *)spinlock);
		else
	if(spinlock->locked)
		NXP_LOG_WARNING("locking ALREADY LOCKED spinlock v0x%p!\n", (void *)spinlock);
	spinlock->locked = 1;
#endif
	spin_lock_irqsave(&spinlock->lock, spinlock->flags); // TODO: _irqsave variant?

	return EOK;
}

static inline errno_t oal_spinlock_unlock(oal_spinlock_t *spinlock)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == spinlock))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef DEBUG_OAL_LOCK_LINUX
	if(!spinlock->inited)
		NXP_LOG_WARNING("unlocking NON-INITED spinlock v0x%p!\n", (void *)spinlock);
	else
	if(!spinlock->locked)
		NXP_LOG_WARNING("unlocking NON-LOCKED spinlock v0x%p!\n", (void *)spinlock);
	spinlock->locked = 0;
#endif
	spin_unlock_irqrestore(&spinlock->lock, spinlock->flags);

	return EOK;
}

#endif /* PUBLIC_QNX_OAL_SPINLOCK_QNX_H_ */

