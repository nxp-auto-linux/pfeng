/* =========================================================================
 *  Copyright 2018,2020,2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup	dxgrOAL
 * @{
 *
 * @defgroup    dxgr_OAL_THREAD THREAD
 * @brief		Threading abstraction
 * @details		TODO
 *
 *
 * @addtogroup  dxgr_OAL_THREAD
 * @{
 *
 * @file		oal_thread.h
 * @brief		The oal_thread module header file.
 * @details		This file contains generic thread management-related API.
 *
 */

#ifndef PUBLIC_OAL_THREAD_H_
#define PUBLIC_OAL_THREAD_H_

typedef struct __oal_thread_tag oal_thread_t;
typedef void * (* oal_thread_func)(void *arg);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Create new thread
 * @param[in]	func Function to be executed within the thread
 * @param[in]	func_arg Function argument
 * @param[in]	name The thread name in string form
 * @param[in]	attrs Thread attributes
 * @return		New thread instance or NULL if failed
 */
oal_thread_t *oal_thread_create(oal_thread_func func, void *func_arg, const char_t *name, uint32_t attrs);

/**
 * @brief		Wait for thread termination
 * @param[in]	thread The thread instance
 * @param[out]	retval Pointer where return value shall be written or NULL if not required
 * @return		EOK if success, error code otherwise
 */
errno_t oal_thread_join(oal_thread_t *thread, void **retval);

/**
 * @brief		Cancel the thread
 * @param[in]	thread The thread instance
 * @return		EOK if success, error code otherwise
 */
errno_t oal_thread_cancel(oal_thread_t *thread);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_OAL_THREAD_H_ */

/** @}*/
/** @}*/
