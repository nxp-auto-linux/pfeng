/* =========================================================================
 *  Copyright 2018-2022,2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup	dxgrOAL
 * @{
 *
 * @defgroup    dxgr_OAL_TIME TIME
 * @brief		Time abstraction
 * @details		TODO
 *
 *
 * @addtogroup  dxgr_OAL_TIME
 * @{
 *
 * @file		oal_time.h
 * @brief		The oal_time module header file.
 * @details		This file contains generic time management-related API.
 *
 */

#ifndef PUBLIC_OAL_TIME_H_
#define PUBLIC_OAL_TIME_H_

/**
 * @brief		Suspend a thread for a given number of microseconds
 * @param[in]	usec The number of microseconds that you want the process to sleep for
 */
void oal_time_usleep(uint32_t usec);

/**
 * @brief		Suspend a thread for a given number of milliseconds
 * @param[in]	msec The number of milliseconds that you want the process to sleep for
 */
void oal_time_msleep(uint32_t msec);

void oal_time_udelay(uint32_t usec);

void oal_time_mdelay(uint32_t msec);

/**
 * @brief		Get current system run time in milliseconds
 */
uint64_t oal_time_gettime(void);


#endif /* PUBLIC_OAL_TIME_H_ */

/** @}*/
/** @}*/
