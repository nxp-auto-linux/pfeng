/* =========================================================================
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef __OAL_TYPES_LINUX__
#define __OAL_TYPES_LINUX__

#include <linux/kernel.h>
#include <linux/types.h>
#include <stddef.h>

#define __STR_HELPER(x) #x
#define __STR(x) __STR_HELPER(x)

#define NXP_LOG_ENABLED

#define NXP_LOG_WARNING(...) printk(KERN_WARNING "["__FILE__":"__STR(__LINE__)"] WRN: " __VA_ARGS__)
#define NXP_LOG_ERROR(...) printk(KERN_ERR "["__FILE__":"__STR(__LINE__)"] ERR: " __VA_ARGS__)

#if (PFE_CFG_VERBOSITY_LEVEL >= 4)
#define NXP_LOG_INFO(...) printk(KERN_INFO "["__FILE__":"__STR(__LINE__)"] INF: " __VA_ARGS__)
#else
#define NXP_LOG_INFO(...)
#endif

#if (PFE_CFG_VERBOSITY_LEVEL >= 8)
#define NXP_LOG_DEBUG(...) printk(KERN_DEBUG "["__FILE__":"__STR(__LINE__)"] DBG: " __VA_ARGS__)
#else
#define NXP_LOG_DEBUG(...)
#endif

#if defined(PFE_CFG_TARGET_ARCH_i386)
typedef unsigned int addr_t;
#define PRINT64 "l"
#define PRINTADDR_T "x"
#elif defined(PFE_CFG_TARGET_ARCH_x86_64) || defined(PFE_CFG_TARGET_ARCH_aarch64)

#define MAX_ADDR_T_VAL UINT_MAX

typedef unsigned long long addr_t;
#define PRINT64 "ll"
#define PRINTADDR_T "llx"
#else
#error Unsupported or no platform defined
#endif

typedef int errno_t;
typedef bool bool_t;
typedef char char_t;
typedef int int_t; /* For use within printf like functions that require "int" regardless its size */
typedef unsigned int uint_t; /* For use within printf like functions */

#ifndef EOK
#define EOK 0
#endif

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */
#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#ifndef NULL_ADDR
#define NULL_ADDR ((addr_t)0U)
#endif /* NULL_ADDR */

#define oal_htons(x)	htons(x)
#define oal_ntohs(x)	ntohs(x)
#define oal_htonl(x)	htonl(x)
#define oal_ntohl(x)	ntohl(x)

#ifndef ENOTSUP
#define ENOTSUP		EOPNOTSUPP
#endif

#endif
