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
 * @defgroup    dxgr_OAL_MBOX MBOX
 * @brief		A simple message-based IPC
 * @details
 * 				Purpose
 * 				-------
 * 				This module provides possibility to pass information between threads. It provides
 * 				two types of approaches at sender's side: blocking (messages), and nonblocking
 * 				(signals).
 *
 * 				Messages are intended to carry extended payload while signals are used to
 * 				deliver just a minimum amount of information to the receiving thread to
 * 				minimize latency and processing overhead.
 *
 * 				The oal_mbox.h header file is generic and only describes the API. The OS-specific
 * 				implementation is covered within appropriate oal_mbox_xxx.c source files.
 *
 * 				Initialization
 * 				--------------
 * 				A new message box is created via successful call of oal_mbox_create().
 *
 * 				Operation
 * 				---------
 * 				Once created a thread can use dedicated API to send message
 * 				(oal_mbox_send_message()) or signal (oal_mbox_send_signal()) to the box. Another
 * 				thread is waiting for a message on oal_mbox_receive() until it arrives. After the
 * 				received message/signal is processed the receiving thread acknowledges the
 * 				reception via oal_mbox_ack_msg() to unblock sending thread in case it is using
 * 				the blocking oal_mbox_send_message().
 *
 * 				@note	A single thread can be both, sender and receiver in the same time and is
 * 						allowed to send message or signal to itself.
 *
 * 				@note	oal_mbox_send_message() was removed under the AAVB-3805 (dead code removal)
 * 						together with oal_mbox_attach_irq(), oal_mbox_detach_irq() and
 * 						oal_irq_get_flags().
 *
 * 				The mbox provides API to attach an IRQ. This feature enables to configure an IRQ
 * 				to be a "signal sender" and in case the IRQ is triggered, signal is automatically
 * 				sent to the box. This is intended to support implementation of detached ISRs where
 * 				IRQ directly wakes-up the worker thread. When IRQ is no more needed it can be
 * 				simply detached by oal_mbox_detach_irq().
 *
 * 				Here is an example how thread can process mbox messages:
 * 				@code{.c}
 *				while(1)
 *				{
 *					// Wait for message (blocking)
 *					ret = oal_mbox_receive(mbox, &msg);
 *					if (EOK == ret)
 *					{
 *						switch (msg.payload.code)
 *						{
 *							case 1:
 *								// Do stuff
 *								break;
 *
 *							case 2:
 *								// Do stuff
 *								break;
 *
 *							default:
 *								break;
 *						}
 *						// Acknowledge the message (unblock the sender)
 *						oal_mbox_ack_msg(&msg);
 *					}
 *				}
 * 				@endcode
 *
 * 				Shutdown
 * 				--------
 * 				When mbox is not more needed in can be destroyed via the oal_mbox_destroy(). The
 * 				call will release all allocated resources and properly detach possible attached
 *				IRQ.
 *
 * @addtogroup  dxgr_OAL_MBOX
 * @{
 *
 * @file		oal_mbox.h
 * @brief		The oal_mbox module header file.
 * @details		This file contains generic mbox-related API.
 *
 */

#ifndef OAL_MBOX_H_
#define OAL_MBOX_H_

#include "oal_irq.h"

typedef struct oal_mbox_tag oal_mbox_t;
typedef struct oal_mbox_metadata_tag oal_mbox_metadata_t;

typedef enum
{
    OAL_MBOX_MSG_MESSAGE = 0,
    OAL_MBOX_MSG_SIGNAL = 1
} oal_mbox_msg_type;

/**
 * @brief	The mbox message structure
 * @details	Split into two parts: payload and metadata. Metadata is used
 * 			by the oal_mbox internally to keep relevant information.
 *
 * @note	This is general representation and it is the same for message as
 * 			well as signal type.
 */
typedef struct
{
	struct
	{
		int32_t code;			/*  Code */
		void *ptr;				/*	Pointer (not valid for signals) */
		int32_t len;			/*	Integer (not valid for signals) */
	} payload;

	struct
	{
		oal_mbox_msg_type type;	/*	Internal message type, shall not be touched by user */
		union {
			int32_t id;			/*	Internal message ID, shall not be touched by user */
			void *ptr;			/*	Internal message handle pointer, shall not be touched by user */
		} msg_info;
	} metadata;
} oal_mbox_msg_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief	Create new mbox
 * @return	The mbox instance or NULL if failed
 */
oal_mbox_t * oal_mbox_create(void);

/**
 * @brief		Destroy mbox instance
 * @param[in]	mbox The mbox instance
 */
void oal_mbox_destroy(oal_mbox_t *mbox);

/**
 * @brief		Attach timer to message box
 * @details		Timer will ensure sending periodic pulses to the mbox with
 * 				period and code given by input parameters.
 * @param[in]	mbox The mbox instance
 * @param[in]	msec Timer period in miliseconds
 * @param[in]	code Integer value to be presented to the receiver (payload)
 * @return		EOK Success, timer is activated
 */
errno_t oal_mbox_attach_timer(oal_mbox_t *mbox, uint32_t msec, int32_t code);

/**
 * @brief		Detach timer from message box
 * @details		Stop all attached timers and release all timer-related internal
 *				resources.
 * @param[in]	mbox The mbox instance
 * @return		EOK if success
 */
errno_t oal_mbox_detach_timer(oal_mbox_t *mbox);

/**
 * @brief		Receive a message/signal via mbox
 * @details		Call is blocking until a message or signal is received
 * @param[in]	mbox The MBOX instance
 * @param[in]	msg Pointer to memory where the message/signal shall be stored
 * @return		EOK if message has been successfully received
 */
errno_t oal_mbox_receive(oal_mbox_t *mbox, oal_mbox_msg_t *msg);

/**
 * @brief		Send signal (non-blocking call)
 * @details		Intended for sending fast messages (signals) with minimum payload.
 * @param[in]	mbox The mbox instance
 * @param[in]	code Integer value to be presented to the receiver (payload)
 * @retval		EOK Success
 */
errno_t oal_mbox_send_signal(oal_mbox_t *mbox, int32_t code);

/**
 * @brief		Acknowledge a received message
 * @details		After the oal_mbox_send_message() the receiver of the message
 * 				calls this function to unblock the sender. In case of a signal
 * 				the call has no effect.
 * @param[in]	msg The message to be acknowledged
 */
void oal_mbox_ack_msg(oal_mbox_msg_t *msg);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* OAL_MBOX_H_ */

/** @}*/
/** @}*/
