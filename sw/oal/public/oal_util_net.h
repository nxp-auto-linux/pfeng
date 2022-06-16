/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup	dxgrOAL_UTIL
 * @{
 *
 * @defgroup    dxgr_OAL_UTIL_NET NET
 * @brief		Advanced utilities, network subsection
 * @details		Network specific utilities
 *
 *
 * @addtogroup	dxgr_OAL_UTIL_NET
 * @{
 *
 * @file		oal_util_net.h
 * @brief		The oal_util_net module header file.
 * @details		This file contains network specific utilities API.
 *
 */

#ifndef OAL_UTIL_NET_H_
#define OAL_UTIL_NET_H_

#include "oal_types.h"

/*
 * QNX
 *
 */
#ifdef PFE_CFG_TARGET_OS_QNX
#include "oal_util_net_qnx.h"

/*
 * LINUX
 *
 */
#elif defined(PFE_CFG_TARGET_OS_LINUX)
#include "oal_util_net_linux.h"

/*
 * AUTOSAR
 *
 */
#elif defined(PFE_CFG_TARGET_OS_AUTOSAR)
#include "oal_util_net_autosar.h"

/*
 * unknown OS
 *
 */
#else
#error "PFE_CFG_TARGET_OS_xx was not set!"
#endif /* PFE_CFG_TARGET_OS_xx */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Convert a numeric network address to a string
 * @details		Function return the pointer to the buffer containing
 * @details		the string version of network address, NULL otherwise
 * @param[in]	af The address network family
 * @param[in]	src The numeric network address
 * @param[out]	dst The buffer with string represented the netowrk address
 * @param[in]	size The size of the buffer
 *
 * @return		The pointer the to buffer, NULL if error occured
 */
char_t *oal_util_net_inet_ntop(int32_t af, const void *src, char_t *dst, uint32_t size);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* OAL_UTIL_NET_H_ */

/** @}*/
/** @}*/
