/* =========================================================================
 *  Copyright 2019-2020 NXP
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
 * @defgroup    dxgr_OAL_UTIL UTIL
 * @brief		Advanced utilities
 * @details		TODO
 *
 *
 * @addtogroup	dxgr_OAL_UTIL
 * @{
 *
 * @file		oal_util.h
 * @brief		The oal_util module header file.
 * @details		This file contains utility management-related API.
 *
 */

#ifndef OAL_UTIL_H_
#define OAL_UTIL_H_

#if defined(PFE_CFG_TARGET_OS_AUTOSAR)
    #include "oal_util_autosar.h"
#elif defined(PFE_CFG_TARGET_OS_BARE)
    #include "oal_util_bare.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_IEEE1588_SUPPORT
/**
 * @brief	PTP packet header
 */
typedef struct __attribute__((packed))
{
	struct {
		uint8_t messageType : 4;
		uint8_t transportSpecific : 4;
	};

	struct {
		uint8_t versionPTP : 4;
		uint8_t reserved0 : 4;
	};

	uint16_t messageLength;
	uint8_t domainNumber;
	uint8_t reserved1;
	uint16_t flags;
	uint64_t correctionField;
	uint32_t reserved2;
	uint64_t sourcePortIdentity;
	uint16_t sourcePortID;
	uint16_t sequenceID;
	uint8_t controlField;
	uint8_t logMessageInterval;
} oal_util_ptp_header_t;
#endif /* PFE_CFG_IEEE1588_SUPPORT */

/**
 * @brief		Modified snprintf function
 * @details		Function return real number of written data into buffer
 * @details		in case of lack of space fills buffer with warming message.
 * @param[in]	*buffer buffer to write data
 * @param[in]	buf_len buffer length
 * @param[in]	Format input data format (same as printf format)
 * @param[in]	... variable arguments according to Format
 *
 * @return		Number of bytes written to the buffer
 */
extern uint32_t oal_util_snprintf(char_t *buffer, size_t buf_len, const char_t *format, ...);

#ifdef PFE_CFG_IEEE1588_SUPPORT
/**
 * @brief			Parse PTP packet
 * @details			Find out if packet in 'buffer' is PTP packet and if so then fill
 * 					the 'ptph' with parsed values.
 * @param[in]		buffer Pointer to the Ethernet frame
 * @param[in]		len Number of byte in the buffer
 * @param[in,out]	ptph Pointer to memory where parsed PTP header values shall be
 * 						 stored
 * @retval			EOK Success, frame is PTP, values in 'ptph' are valid
 * @retval			ENOENT The 'buffer' contains non-PTP frame
 */
errno_t oal_util_parse_ptp(uint8_t *buffer, size_t len, oal_util_ptp_header_t **ptph);

/**
 * @brief		Get unique sequence number
 * @details		System-wide, sequential numbers generator. Each call produces
 * 				number incremented by 1 from the previous one. The counter wraps
 *				with 32nd bit.
 * @note		Must be reentrant and thread-safe
 * @return		The number
 */
uint32_t oal_util_get_unique_seqnum32(void);
#endif /* PFE_CFG_IEEE1588_SUPPORT */

#endif /* OAL_UTIL_H_ */

/** @}*/
/** @}*/
