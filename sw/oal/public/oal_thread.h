/* =========================================================================
 *  Copyright 2018,2020 NXP
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

#endif /* PUBLIC_OAL_THREAD_H_ */

/** @}*/
/** @}*/
