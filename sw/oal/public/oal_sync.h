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

/**
 * @addtogroup	dxgrOAL
 * @{
 * 
 * @defgroup    dxgr_OAL_SYNC SYNC
 * @brief		Thread synchronization
 * @details		Package provides OS-independent thread synchronization primitives. All API should
 * 				be implemented with performance taken into account.
 * 	
 * 	
 * @addtogroup	dxgr_OAL_SYNC
 * @{
 * 
 * @file		oal_sync.h
 * @brief		The thread synchronization header file
 * @details		Use this header to include all the OS-independent thread synchronization functionality
 *
 */

#ifndef PUBLIC_OAL_SYNC_H_
#define PUBLIC_OAL_SYNC_H_

/*
 * QNX
 *
 */
#ifdef PFE_CFG_TARGET_OS_QNX
#include "oal_spinlock_qnx.h"
#include "oal_mutex_qnx.h"

/*
 * LINUX
 *
 */
#elif defined(PFE_CFG_TARGET_OS_LINUX)
#include "oal_spinlock_linux.h"
#include "oal_mutex_linux.h"

/*
 * AUTOSAR
 *
 */
#elif defined(PFE_CFG_TARGET_OS_AUTOSAR)
#include "oal_spinlock_autosar.h"
#include "SchM_Eth_43_PFE.h"

/*
 * BARE METAL
 * 
 */
#elif defined(PFE_CFG_TARGET_OS_BARE)
#include "oal_spinlock_bare.h"
#include "oal_mutex_bare.h"

/*
 * unknown OS
 *
 */
#else
#error "PFE_CFG_TARGET_OS_xx was not set!"
#endif /* PFE_CFG_TARGET_OS_xx */

#endif /* PUBLIC_OAL_SYNC_H_ */

/**
 * @typedef oal_spinlock_t
 * @brief	The spinlock representation type
 * @details	Each OS will provide its own definition
 */

/**
 * @brief		Initialize a spinlock object
 * @param[in]	spinlock Spinlock instance
 * @return		EOK if success, error code otherwise
 */
static inline errno_t oal_spinlock_init(oal_spinlock_t *spinlock);

/**
 * @brief		Destroy a spinlock object
 * @param[in]	spinlock Spinlock instance
 * @return		EOK if success, error code otherwise
 */
static inline errno_t oal_spinlock_destroy(oal_spinlock_t *spinlock);

/**
 * @brief		Lock
 * @param[in]	spinlock Initialized spinlock instance
 * @retval		EOK Success
 * @retval		EAGAIN Couldn't perform the lock, try again later
 * @retval		errorcode in case of failure
 */
static inline errno_t oal_spinlock_lock(oal_spinlock_t *spinlock);

/**
 * @brief		Unlock
 * @param[in]	spinlock Initialized spinlock instance
 * @return		EOK if success, error code otherwise
 */
static inline errno_t oal_spinlock_unlock(oal_spinlock_t *spinlock);

/**
 * @typedef oal_mutex_t
 * @brief	The mutex representation type
 * @details	Each OS will provide its own definition
 */

/**
 * @brief		Initialize a mutex object
 * @param[in]	mutex Mutex instance
 * @return		EOK if success, error code otherwise
 */
static inline errno_t oal_mutex_init(oal_mutex_t *mutex);

/**
 * @brief		Destroy a mutex object
 * @param[in]	mutex Mutex instance
 * @return		EOK if success, error code otherwise
 */
static inline errno_t oal_mutex_destroy(oal_mutex_t *mutex);

/**
 * @brief		Lock
 * @param[in]	mutex Initialized mutex instance
 * @retval		EOK Success
 * @retval		EAGAIN Couldn't perform the lock, try again later
 * @retval		errorcode in case of failure
 */
static inline errno_t oal_mutex_lock(oal_mutex_t *mutex);

/**
 * @brief		Unlock
 * @param[in]	mutex Initialized mutex instance
 * @return		EOK if success, error code otherwise
 */
static inline errno_t oal_mutex_unlock(oal_mutex_t *mutex);

/** @}*/
/** @}*/
