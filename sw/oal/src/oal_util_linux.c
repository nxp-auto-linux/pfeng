/* =========================================================================
 *  Copyright 2019-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_OAL_UTIL
 * @{
 *
 * @file		oal_util_qnx.c
 * @brief		The oal_util module source file.
 * @details		This file contains utility management implementation.
 *
 */

#include "pfe_cfg.h"
#include "oal_types.h"
#include "oal_util.h"

#include <linux/random.h>

uint32_t oal_util_snprintf(char_t *buffer, size_t buf_len, const char_t *format, ...)
{
	uint32_t len = 0;
	va_list ap;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == buffer))
	{
		NXP_LOG_ERROR(" NULL argument received (oal_util_snprintf)\n");
		return 0;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if(buf_len == 0)
	{
		NXP_LOG_ERROR(" Wrong buffer size (oal_util_snprintf)\n");
		return 0;
	}
	va_start(ap, format);
	len = vscnprintf(buffer, buf_len, format, ap);
	va_end(ap);
	return len;
}

int oal_util_rand(void)
{
	int val;

	get_random_bytes(&val, sizeof(int));

	return val;
}

/** @}*/
