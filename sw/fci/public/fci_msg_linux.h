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
