/* =========================================================================
 *  Copyright 2019 NXP
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
 * @addtogroup	dxgrOAL_UTIL
 * @{
 *
 * @defgroup    dxgr_OAL_UTIL_NET NET
 * @brief		Advanced utilities, network subsection
 * @details		Network specific utilities
 *
 *
 * @addtogroup	dxgr_OAL_UTIL_NET
 * @{
 *
 * @file		oal_util_net.h
 * @brief		The oal_util_net module header file.
 * @details		This file contains network specific utilities API.
 *
 */

#ifndef OAL_UTIL_NET_H_
#define OAL_UTIL_NET_H_

/*
 * QNX
 *
 */
#ifdef TARGET_OS_QNX
#include "oal_util_net_qnx.h"

/*
 * LINUX
 *
 */
#elif defined(TARGET_OS_LINUX)
#include "oal_util_net_linux.h"

/*
 * unknown OS
 *
 */
#else
#error "TARGET_OS_xx was not set!"
#endif /* TARGET_OS */

/**
 * @brief		Convert a numeric network address to a string
 * @details		Function return the pointer to the buffer containing
 * @details		the string version of network address, NULL otherwise
 * @param[in]	af The address network family
 * @param[in]	src The numeric network address
 * @param[out]	dst The buffer with string represented the netowrk address
 * @param[in]	size The size of the buffer
 *
 * @return		The pointer the to buffer, NULL if error occured
 */
char_t *oal_util_net_inet_ntop(int af, const void *src, char_t *dst, uint32_t size);

#endif /* OAL_UTIL_NET_H_ */

/** @}*/
/** @}*/
