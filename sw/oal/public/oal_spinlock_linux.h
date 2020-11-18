/* =========================================================================
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
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

