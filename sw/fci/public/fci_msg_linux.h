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
 * @file		fci_msg_linux.h
 * @brief		The Linux-specific FCI IPC message (fci_msg_t) format
 * @details		The FCI message is used to transport FCI commands and events
 * 				between FCI endpoint and FCI clients (libFCI) using IPC.
 *
 */

#ifndef PUBLIC_FCI_MSG_LINUX_H_
#define PUBLIC_FCI_MSG_LINUX_H_

/**
 * @brief	FCI IPC message format
 */
typedef struct
{
	msg_type_t type;
	uint16_t ret_code;
	union
	{
		struct
		{
			uint32_t port_id;								/*!< Node identifier (port ID) */
		} msg_client_register;

		struct
		{
			uint32_t port_id;								/*!< Node identifier (port ID) */
		} msg_client_unregister;

		struct
		{
			uint32_t code;									/*!< Message code */
			uint32_t length;								/*!< Message length */
			uint8_t payload[FCI_CFG_MAX_CMD_PAYLOAD_LEN];	/*!< Message payload */
		} msg_cmd;
	};

	/*	FCI internal storage */
	void *client;
} fci_msg_t;

#endif /* PUBLIC_FCI_MSG_LINUX_H_ */

/** @}*/
