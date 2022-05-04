/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_OAL_UTIL_NET_LINUX
 * @{
 *
 * @file		oal_util_net_linux.c
 * @brief		The oal_util_net_linux module source file.
 * @details		This file contains network utility implementation for LINUX.
 *
 */

#include <linux/errno.h>

#include "pfe_cfg.h"
#include "oal_types.h"
#include "oal_util.h"
#include "oal_util_net.h"

#define NIP4(addr) \
		((unsigned char *)addr)[0], \
		((unsigned char *)addr)[1], \
		((unsigned char *)addr)[2], \
		((unsigned char *)addr)[3]

#define NIP6(addr) \
		ntohs(((struct in6_addr *)addr)->s6_addr16[0]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[1]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[2]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[3]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[4]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[5]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[6]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[7])

char_t *oal_util_net_inet_ntop(int32_t af, const void *src, char_t *dst, uint32_t size)
{
	int32_t ret;

	switch(af) {
		case AF_INET:
			ret = snprintf(dst, size, "%d.%d.%d.%d", NIP4(src));
			break;
		case AF_INET6:
			ret = snprintf(dst, size, "%d.%d.%d.%d.%d.%d.%d.%d", NIP6(src));
			break;
		default:
			ret = -EAFNOSUPPORT;
			break;
	}

	if(ret > 0)
		return dst;

	return NULL;
}

/** @}*/
