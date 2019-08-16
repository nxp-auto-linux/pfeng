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
void oal_time_mdelay(uint32_t msec);

#endif /* PUBLIC_OAL_TIME_H_ */

/** @}*/
/** @}*/
