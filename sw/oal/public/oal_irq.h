/* =========================================================================
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup	dxgrOAL
 * @{
 *
 * @defgroup    dxgr_OAL_IRQ IRQ
 * @brief		IRQ abstraction
 * @details
 * 				Purpose
 * 				-------
 * 				Purpose of the oal_irq block is to abstract IRQ management tasks.
 *
 * 				Initialization
 * 				--------------
 * 				Each IRQ can be represented as instance of the oal_irq_t created by successful call
 * 				of oal_irq_create(). Function binds a logical IRQ number with a handler which is
 * 				optional and instance can exists also without an attached handler.
 *
 * 				@note The oal_irq_crete() automatically un-masks the created IRQ.
 *
 * 				Operation
 * 				---------
 * 				An IRQ instance can be manipulated using oal_irq_mask() to suppress the interrupt
 * 				and oal_irq_unmask() to re-enable it. To get the logical interrupt number as
 * 				assigned during instance creation the oal_irq_get_id() can be used.
 *
 * 				Shutdown
 * 				--------
 * 				Just call the oal_irq_destroy(). It ensures all interrupt de-initialization and
 * 				releases all allocated resources.
 *
 * @addtogroup  dxgr_OAL_IRQ
 * @{
 *
 * @file		oal_mbox.h
 * @brief		The oal_irq module header file.
 * @details		This file contains generic irq-related API.
 *
 */
#ifndef OAL_IRQ_H_
#define OAL_IRQ_H_

#ifdef PFE_CFG_TARGET_OS_AUTOSAR

#include "oal_irq_autosar.h"

#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

typedef struct oal_irq_tag oal_irq_t;

/**
 * @brief		The IRQ handler type
 * @param[in]	data Custom data
 * @return		TRUE if IRQ has been handled, FALSE if not
 */
typedef bool_t (* oal_irq_handler_t)(void *data);

/**
 * @brief		ISR handle
 * @details		Handle to identify registered interrupt handlers.
 */
typedef uint32_t oal_irq_isr_handle_t;

/**
 * @brief		Interrupt types
 * @details		Describes the state of interrupt ownership
 */
typedef enum
{
	OAL_IRQ_FLAG_PRIVATE = (1 << 0),    /* Interrupt is owned exclusively by the irq instance */
	OAL_IRQ_FLAG_SHARED = (1 << 1),     /* Interrupt is shared by other OS components */
	OAL_IRQ_FLAG_DISABLED = (1 << 2)    /* Don't automatically enable interrupt when created */
} oal_irq_flags_t;

/**
 * @brief		Create new IRQ instance
 * @details		After successful call the IRQ is automatically unmasked.
 * @param[in]	id The IRQ ID as seen by the OS
 * @param[in]	flags Interrupt type flags
 * @param[in]	data A cookie passed back to the handler function
 * @return		The new IRQ instance or NULL if failed
 */
oal_irq_t * oal_irq_create(int32_t id, oal_irq_flags_t flags, const char_t *name);

/**
 * @brief		Add handler for particular IRQ
 * @details		Adds IRQ handler. Only one handler per irq is supported.
 * @param[in]	irq The IRQ instance
 * @param[in]	handler The handler to be called when IRQ occurs or NULL
 * 						if handler should be not used.
 * @param[in]	data A cookie passed back to the handler function
 * @param[out]	handle Pointer to memory where handle identifying the handler shall
 * 					   be written.
 * @return		EOK if success, error code otherwise
 */
errno_t oal_irq_add_handler(oal_irq_t *irq, oal_irq_handler_t handler, void *data, oal_irq_isr_handle_t *handle);

/**
 * @brief		Delete handler
 * @param[in]	irq The IRQ instance
 * @param[in]	handle The handler identifier
 * @return		EOK if success, error code otherwise
 */
errno_t oal_irq_del_handler(oal_irq_t *irq, oal_irq_isr_handle_t handle);

/**
 * @brief		Destroy an IRQ instance
 * @details		Masks the IRQ and releases all associated resources
 * @param[in]	irq The IRQ instance
 */
void oal_irq_destroy(oal_irq_t *irq);

/**
 * @brief		Mask IRQ
 * @param[in]	irq The IRQ instance
 * @return		EOK if success, error code otherwise
 */
errno_t oal_irq_mask(oal_irq_t *irq);

/**
 * @brief		Un-mask IRQ
 * @param[in]	irq The IRQ instance
 * @return		EOK if success, error code otherwise
 */
errno_t oal_irq_unmask(oal_irq_t *irq);

/**
 * @brief		Get IRQ ID
 * @param[in]	irq The IRQ instance
 * @return		The IRQ ID associated with the instance or -1 if failed
 */
int32_t oal_irq_get_id(const oal_irq_t *irq);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* OAL_IRQ_H_ */

/** @}*/
/** @}*/
