/* =========================================================================
 *  Copyright 2019-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_msg.h
 * @brief		The FCI IPC message type
 * @details		This header provides the fci_msg_t.
 *
 */

#ifndef SRC_FCI_MSG_H_
#define SRC_FCI_MSG_H_

/**
 * @brief	Maximum size of FCI IPC message payload
 * @see		fci_msg_t
 */
#define FCI_CFG_MAX_CMD_PAYLOAD_LEN		256

/**
 * @brief	FCI message types
 * @see		fci_msg_t
 */
typedef enum
{
	FCI_MSG_TYPE_MIN = 0x1000,
	FCI_MSG_CLIENT_REGISTER,
	FCI_MSG_CLIENT_UNREGISTER,
	FCI_MSG_CMD,
	FCI_MSG_CORE_CLIENT_BROADCAST,
	FCI_MSG_TYPE_MAX
} msg_type_t;

/*
 * QNX
 *
 */
#if defined(PFE_CFG_TARGET_OS_QNX)
#include "fci_msg_qnx.h"

/*
 * LINUX
 *
 */
#elif defined(PFE_CFG_TARGET_OS_LINUX)
#include "fci_msg_linux.h"

/*
 * AUTOSAR
 *
 */
#elif defined(PFE_CFG_TARGET_OS_AUTOSAR)
#include "fci_msg_autosar.h"

/*
 * unknown OS
 *
 */
#else
#error "PFE_CFG_TARGET_OS_xx was not set!"
#endif /* PFE_CFG_TARGET_OS_xx */

#endif /* SRC_FCI_MSG_H_ */

/** @}*/
