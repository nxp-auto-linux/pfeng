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
	FCI_MSG_ENDPOINT_SHUTDOWN,
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
 * unknown OS
 *
 */
#else
#error "PFE_CFG_TARGET_OS_xx was not set!"
#endif /* PFE_CFG_TARGET_OS_xx */

#endif /* SRC_FCI_MSG_H_ */

/** @}*/
