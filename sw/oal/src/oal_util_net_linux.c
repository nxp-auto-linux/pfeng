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
 * @addtogroup  dxgr_OAL_UTIL_NET_LINUX
 * @{
 *
 * @file		oal_util_net_linux.c
 * @brief		The oal_util_net_linux module source file.
 * @details		This file contains network utility implementation for LINUX.
 *
 */

#include <linux/errno.h>

#include "oal_types.h"
#include "oal_util.h"
#include "oal_util_net.h"

#define NIP4(addr) \
		((unsigned char *)&addr)[0], \
		((unsigned char *)&addr)[1], \
		((unsigned char *)&addr)[2], \
		((unsigned char *)&addr)[3]

#define NIP6(addr) \
		ntohs(((struct in6_addr *)addr)->s6_addr16[0]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[1]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[2]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[3]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[4]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[5]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[6]), \
		ntohs(((struct in6_addr *)addr)->s6_addr16[7])

char_t *oal_util_net_inet_ntop(int af, const void *src, char_t *dst, uint32_t size)
{
	int ret;

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
