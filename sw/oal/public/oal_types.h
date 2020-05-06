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
 * @defgroup    dxgr_OAL_TYPES TYPES
 * @brief		Standard types
 * @details		
 * 	
 * 	
 * @addtogroup	dxgr_OAL_TYPES
 * @{
 * 
 * @file		oal_types.h
 * @brief		Header for standard types
 * @details		TODO
 *
 */

#ifndef OAL_TYPES_H_
#define OAL_TYPES_H_

/*
 * QNX
 *
 */
#ifdef PFE_CFG_TARGET_OS_QNX
#include "oal_types_qnx.h"

/*
 * LINUX
 *
 */
#elif defined(PFE_CFG_TARGET_OS_LINUX)
#include "oal_types_linux.h"

/*
 * AUTOSAR
 *
 */
#elif defined(PFE_CFG_TARGET_OS_AUTOSAR)
#include "oal_types_autosar.h"

/*
 * unknown OS
 *
 */
#else
#error "PFE_CFG_TARGET_OS_xx was not set!"
#endif /* PFE_CFG_TARGET_OS_xx */

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define _ASSERT_CONCAT_(a, b) a##b
#define _ASSERT_CONCAT(a, b) _ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { _ASSERT_CONCAT(precompile_assert_, __COUNTER__) = 1/(!!(e)) }
#define _ct_assert(e) enum { _ASSERT_CONCAT(precompile_assert_, __COUNTER__) = 1/(!!(e)) }

/**
 * @brief		Swap byte order in a buffer
 * @detail		Convert byte order of each 4-byte word within given buffer
 * @param[in]	data Pointer to buffer to be converted
 * @param[in]	size Number of bytes in the buffer
 */
static inline void oal_swap_endian_long(void *data, uint32_t size)
{
	uint32_t ii, words = size >> 2;
	uint32_t *word = (uint32_t *)data;

	if (0U != (size & 0x3U))
	{
		words += 1U;
	}

	for (ii=0U; ii<words; ii++)
	{
		word[ii] = oal_htonl(word[ii]);
	}
}

#endif /* OAL_TYPES_H_ */

/** @}*/
/** @}*/
