/* =========================================================================
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
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

#ifndef HAL_H_
#define HAL_H_

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
/* requires only for hal_ip_ready API */
#include "oal_types.h"
#include "oal_mm.h"
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#if defined(__ghs__)
	#define hal_nop()       __asm(" nop")
#elif defined(__GNUC__) && (__STDC__ == 1)
	#define hal_nop()       __asm__ volatile("nop" ::: "memory")
#else
	#define hal_nop()       asm volatile("nop" ::: "memory")
#endif



#if defined(PFE_CFG_TARGET_OS_LINUX)
#include <linux/slab.h>
#include <linux/io.h>
#define hal_write_w(width, val, addr)	\
							do {	\
								iowrite##width(val, (volatile void *)addr); \
							} while (0!=0)

#define hal_write64(val, addr)	hal_write_w(64, val, addr)
#define hal_write32(val, addr)	hal_write_w(32, val, addr)
#define hal_write16(val, addr)	hal_write_w(16, val, addr)
#define hal_write8(val, addr)	hal_write_w(8, val, addr)
#define hal_read_w(width, addr) \
								ioread##width((volatile void *)addr)
#define hal_read32(addr)	hal_read_w(32, addr)
#define hal_read16(addr)	hal_read_w(16, addr)
#define hal_read8(addr)		hal_read_w(8, addr)
#else /* PFE_CFG_TARGET_OS_LINUX */
/*	AXI writes immediately followed by an AXI read, cause writes to be lost,
	as a workaround, add a hal_nop after each write */
#define hal_write32(val, addr) \
							do {	\
								(*(volatile uint32_t *)(addr) = ((uint32_t)(val)));	\
							} while (0!=0)

#define hal_write16(val, addr) \
							do {	\
									(*(volatile uint16_t *)(addr) = ((uint16_t)(val)));	\
							} while (0!=0)

#define hal_write8(val, addr) \
							do {	\
									(*(volatile uint8_t *)(addr) = ((uint8_t)(val)));	\
							} while (0!=0)

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
 * @brief	If TRUE then platform performs explicit cache maintenance (flush/invalidate)
 */
#if defined(PFE_CFG_TARGET_OS_QNX) && \
	!defined(PFE_CFG_TARGET_ARCH_x86) && \
	!defined(PFE_CFG_BUFFERS_COHERENT)
	#define HAL_HANDLE_CACHE	TRUE
#else
	#define HAL_HANDLE_CACHE	FALSE
#endif

/**
 * @brief	Specify cache line size in number of bytes.
 */
#define	HAL_CACHE_LINE_SIZE	64U

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#endif /* HAL_H_ */

/** @}*/
