/* =========================================================================
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
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
 * BARE METAL
 *
 */
#elif defined(PFE_CFG_TARGET_OS_BARE)
#include "oal_types_bare.h"

/*
 * unknown OS
 *
 */
#else
#error "PFE_CFG_TARGET_OS_xx was not set!"
#endif /* PFE_CFG_TARGET_OS_xx */

#include "ct_assert.h"

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

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

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* OAL_TYPES_H_ */

/** @}*/
/** @}*/
