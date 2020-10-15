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
 * @defgroup	dxgrHAL HAL
 * @brief		The HW Abstraction Layer
 * @details		
 * 				
 * 
 * @addtogroup	dxgrHAL
 * @{
 * 
 * @file		hal.h
 * @brief		The main HAL header file
 * @details		Use this header to include all the HAL-provided functionality
 *
 */

#ifndef _HAL_H_
#define _HAL_H_

#if defined(__ghs__)
	#define hal_nop()       __asm(" nop")
#else
	#define hal_nop()       asm volatile("nop" ::: "memory")
#endif

#if defined(PFE_CFG_TARGET_OS_LINUX)
#include <linux/slab.h>
#include <linux/io.h>
#define _hal_write_w(width, val, addr)	\
							do {	\
								iowrite##width(val, (volatile void *)addr); \
							} while (0)

#define hal_write32(val, addr)	_hal_write_w(32, val, addr)
#define hal_write16(val, addr)	_hal_write_w(16, val, addr)
#define hal_write8(val, addr)	_hal_write_w(8, val, addr)
#define _hal_read_w(width, addr) \
								ioread##width((volatile void *)addr)
#define hal_read32(addr)	_hal_read_w(32, addr)
#define hal_read16(addr)	_hal_read_w(16, addr)
#define hal_read8(addr)		_hal_read_w(8, addr)
#else /* PFE_CFG_TARGET_OS_LINUX */
/*	AXI writes immediately followed by an AXI read, cause writes to be lost,
	as a workaround, add a hal_nop after each write */
#define hal_write32(val, addr) \
							do {	\
								(*(volatile uint32_t *)(addr) = ((uint32_t)(val)));	\
								hal_nop();	\
							} while (0)

#define hal_write16(val, addr) \
							do {	\
									(*(volatile uint16_t *)(addr) = ((uint16_t)(val)));	\
									hal_nop();	\
							} while (0)

#define hal_write8(val, addr) \
							do {	\
									(*(volatile uint8_t *)(addr) = ((uint8_t)(val)));	\
									hal_nop();	\
							} while (0)

#define hal_read32(addr)	(*(volatile uint32_t *)(addr))
#define hal_read16(addr)	(*(volatile uint16_t *)(addr))
#define hal_read8(addr)		(*(volatile uint8_t *)(addr))
#endif /* PFE_CFG_TARGET_OS_LINUX */

#ifndef likely
	#if defined(__ghs__) || defined(__DCC__)
		#define likely(x)   (x)
	#else
		#define likely(x)   __builtin_expect(!!(x),1)
	#endif

#endif

#ifndef unlikely
	#if defined(__ghs__) || defined(__DCC__)
		#define unlikely(x) (x)
	#else
		#define unlikely(x) __builtin_expect(!!(x),0)
	#endif
#endif

#if defined(__ghs__) || defined(__DCC__)
	#if defined(PFE_CFG_TARGET_ARCH_aarch64le) || defined(PFE_CFG_TARGET_ARCH_armv7le)
		#define hal_wmb()   __asm(" dmb oshst")
	#else
		#error Unsupported or no platform defined
	#endif
#else
	#if defined(PFE_CFG_TARGET_ARCH_aarch64le)
		#define hal_wmb()   __asm__ __volatile__(" dmb oshst" : : : "memory")
	#elif defined(PFE_CFG_TARGET_ARCH_x86) || defined(PFE_CFG_TARGET_ARCH_x86_64)
		#define hal_wmb()   asm volatile("sfence" ::: "memory")
	#elif defined(PFE_CFG_TARGET_ARCH_aarch64)
		#define hal_wmb()			smp_wmb()
	#elif defined(PFE_CFG_TARGET_ARCH_armv7le)
		#define hal_wmb()	__asm__ __volatile__(" dmb":::"memory")
	#else
		#error Unsupported or no platform defined
	#endif
#endif

/**
 * @brief	If TRUE then platform need explicit cache maintenance (flush/invalidate)
 */
#define HAL_HANDLE_CACHE	FALSE

/**
 * @brief	Specify cache line size in number of bytes.
 */
#define	HAL_CACHE_LINE_SIZE	64U

#endif /* _HAL_H_ */

/** @}*/
