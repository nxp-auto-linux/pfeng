/* =========================================================================
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
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

#ifndef OAL_SYNC_H_
#define OAL_SYNC_H_

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

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

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

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/** @}*/
/** @}*/

#endif /* OAL_SYNC_H_ */
