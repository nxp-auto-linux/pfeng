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
 * @file		fci_core.h
 * @brief		The FCI core header file
 * @details		The FCI core is OS-specific module responsible for:
 *					-	IPC with FCI clients running within separated processes within the OS
 *				  		environment.
 *					-	Reception of commands from clients and executing OS-independent command
 *				  		translator provided by FCI.
 *					-	Maintenance of list of the clients.
 *					-	Provision of API to the rest of FCI to communicate with the clients.
 *
 *				This file specifies common API the FCI core implementation has to implement.
 *
 */

#ifndef SRC_FCI_CORE_H_
#define SRC_FCI_CORE_H_

#include "oal_types.h"	/* Common types */
#include "fci_msg.h"	/* The fci_msg_t and related stuff */

/**
 * @brief	Maximum number of event listeners (FCI clients) which can be
 * 			registered to receive runtime notifications from the FCI
 * 			endpoint.
 */
#define FCI_CFG_MAX_CLIENTS				5

/**
 * @brief	FCI core type
 * @details	This is OS-specific part of FCI
 */
typedef struct __fci_core_tag fci_core_t;

/**
 * @brief	FCI core client type
 */
typedef struct __fci_core_client_tag fci_core_client_t;

/**
 * @brief		Create FCI core instance
 * @details		The FCI core is OS-specific part of the FCI endpoint. It is responsible
 * 				for IPC connectivity with the rest of system.
 * @param[in]	id String identifier specifying the core instance. Intended to be used
 * 				by libFCI to locate the endpoint.
 * @retval		EOK Success
 * @retval		EINVAL invalid argument received
 * @retval		ENOMEM initialization failed
 */
errno_t fci_core_init(const char_t *const id);

/**
 * @brief		Destroy FCI core
 * @details		Close all connections and release all associated resources
 */
void fci_core_fini(void);

/**
 * @brief		Send message to the FCI core
 * @param[in]	msg Pointer to the buffer containing payload to be sent
 * @param[in]	rep Pointer to buffer where reply data shall be stored
 * @return		EOK if success, error code otherwise
 */
errno_t fci_core_send(fci_msg_t *msg, fci_msg_t *rep);

/**
 * @brief		Send message to FCI client
 * @param[in]	client The FCI client instance
 * @param[in]	msg Pointer to the buffer containing payload to be sent
 * @param[in]	rep Pointer to buffer where reply data shall be stored
 * @return		EOK if success, error code otherwise
 */
errno_t fci_core_client_send(fci_core_client_t *client, fci_msg_t *msg, fci_msg_t *rep);

/**
 * @brief		Send message to all FCI clients
 * @param[in]	msg Pointer to the buffer containing payload to be sent
 * @param[in]	rep Pointer to buffer where reply data shall be stored
 * @return		EOK if success, error code otherwise
 */
errno_t fci_core_client_send_broadcast(fci_msg_t *msg, fci_msg_t *rep);

#endif /* SRC_FCI_CORE_H_ */

/** @}*/
