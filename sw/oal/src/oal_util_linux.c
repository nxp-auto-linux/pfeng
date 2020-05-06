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

/** @}*/
