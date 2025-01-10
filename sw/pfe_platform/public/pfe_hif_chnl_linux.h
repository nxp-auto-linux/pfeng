/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @file		pfe_hif_chnl.h
 * @brief		The Host Interface channel
 * @details
 * 				Purpose
 * 				-------
 * 				This is the software representation of the HIF channel including both data RX, and
 * 				TX functionality. HIF channel is a middle-level SW component which a driver shall
 *				use to gain access to the Ethernet traffic. All data transmission requests as well
 *				as data reception operations and related control tasks are provided by a HIF channel
 *				instance API.
 *
 *				Initialization
 * 				--------------
 * 				The channel instance is created by pfe_hif_chnl_create(). Then it needs to be
 * 				initialized	to perform instance-specific configuration. For this purpose
 * 				the pfe_hif_chnl_init() shall be called. Once successfully initialized, the
 *				channel can be activated by pfe_hif_chnl_rx_enable() and pfe_hif_chnl_tx_enable().
 *
 * 				RX traffic management
 * 				---------------------
 * 				Upon activation the channel starts receiving data into its internal buffer ring.
 * 				The RX buffers are not managed by the channel itself and need to be supplied by
 * 				user via pfe_hif_chnl_supply_rx_buf(). Each set of supplied buffers must be
 * 				confirmed by pfe_hif_chnl_rx_dma_start() to let the hardware know that new empty
 * 				RX buffers have become available (this means that multiple calls of
 * 				pfe_hif_chnl_supply_rx_buf() can be confirmed by a single
 * 				pfe_hif_chnl_rx_dma_start()).
 *
 * 				@note To query if the channel is able to accept new RX buffers one can also use
 * 					  the helper function pfe_hif_chnl_can_accept_rx_buf().
 *
 * 				Usually, new RX data is indicated via dedicated RX IRQ. The RX IRQ number associated
 * 				with channel can be retrieved via pfe_chi_chnl_get_irq() call. Driver then
 * 				processes the interrupt by calling the pfe_hif_chnl_rx() until the function
 * 				indicates 'no data'. In that case the driver acknowledges the RX interrupt via
 * 				pfe_hif_chnl_ack_rx_irq(). Note that buffers dequeued by pfe_hif_chnl_rx() must
 * 				be replaced by fresh ones using the pfe_hif_chnl_supply_rx_buf() call to keep the
 * 				reception active.
 *
 * 				Typical RX operation could look like:
 * 				@code{.c}
 * 					void rx_irq_handler(void)
 * 					{
 * 						while (EOK == pfe_hif_chnl_rx())
 * 						{
 * 							// Process the received buffer
 * 						}
 *
 * 						while (TRUE == pfe_hif_chnl_can_accept_rx_buf())
 * 						{
 * 							pfe_hif_chnl_supply_rx_buf();
 * 						}
 *
 * 						pfe_hif_chnl_rx_dma_start();
 * 						pfe_hif_chnl_ack_rx_irq();
 * 					}
 * 				@endcode
 *
 * 				TX traffic management
 * 				---------------------
 * 				A packet can be committed for transmission using the pfe_hif_chnl_tx() call. Since
 * 				packet can consist of multiple separated buffers the call provides possibility to
 * 				mark each of them by so called 'lifm' (last-in-frame) flag and driver is responsible
 * 				for its validity. Transmission of committed buffer(s) is triggered by the
 * 				pfe_hif_chnl_tx_dma_start().
 *
 * 				@note To query if the channel is able to accept new TX buffers one can use
 * 					  the helper function pfe_hif_chnl_can_accept_rx_buf().
 *
 * 				Typical TX sequence could look like:
 * 				@code{.c}
 * 				...
 * 				if (pfe_hif_chnl_can_accept_tx_buf())
 * 				{
 * 					if (EOK == pfe_hif_chnl_tx(buf->data, buf->is_last))
 * 					{
 * 						// Committed
 * 					}
 * 					else
 * 					{
 * 						// Failed
 * 					}
 * 				}
 *
 * 				pfe_hif_chnl_tx_dma_start();
 * 				...
 * 				@endcode
 *
 *				Once a buffer is transmitted a TX confirmation is generated. Driver can query
 *				for new TX confirmations using the pfe_hif_chnl_has_tx_conf(). If a TX
 *				confirmation is available it can be 'dequeued' via pfe_hif_chnl_get_tx_conf().
 *				Order of TX confirmations as returned by pfe_hif_chnl_get_tx_conf() is exactly
 *				the same as the TX buffers were committed for transmission. Since channel does
 *				not internally keep mapping between TX confirmations and transmitted buffers,
 *				the driver must do the mapping using order of transmitted buffers and received
 *				TX confirmations.
 *
 *				TX confirmations can be handled for instance by periodic calls (or driven by
 *				TX IRQs) of routine such:
 *				@code{.c}
 *				void handle_tx_conf(void)
 *				{
 *					if (TRUE == pfe_hif_chnl_has_tx_conf())
 *					{
 *						while (EOK == pfe_hif_chnl_get_tx_conf())
 *						{
 *							// Next packet has been transmitted
 *						}
 *					}
 *				}
 *				@endcode
 *
 *				Shutdown handling
 *				-----------------
 *				Once the channel is no more needed it can be stopped and subsequently destroyed.
 *				Driver needs to perform	following sequence to properly shut the channel down:
 *				1.	Disable RX traffic via pfe_hif_chnl_rx_disable()
 *				2.	Drain all RX buffers via pfe_hif_chnl_rx()
 *				3.	Disable TX traffic via pfe_hif_chnl_tx_disable()
 *				4.	Drain remaining TX confirmations via pfe_hif_get_tx_conf(). Note that buffers
 *					committed for transmission but not transmitted yet will be confirmed as they
 *					were transmitted.
 *				5.	Destroy the HIF channel instance calling the pfe_hif_chnl_destroy()
 *				
 */

#ifndef PUBLIC_PFE_HIF_CHNL_H_
#define PUBLIC_PFE_HIF_CHNL_H_

#include "pfe_bmu.h"
#include "pfe_hif_ring_linux.h"

/**
 * @brief	RX buffer management
 * @details	When TRUE then RX buffer management is embedded so caller layer
 * 			does not need to care about it. FALSE disables the feature.
 */
#ifndef PFE_CFG_PLATFORM_TEST
#define PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED	FALSE
#else
#define PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED	TRUE
#endif
/**
 * @brief	RX OOB management
 * @details	When TRUE then RX OOB buffer management is embedded so caller layer
 * 			can process the RX_OOB_EVENT. FALSE removed the feature.
 */
#ifndef PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED
#define PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED	FALSE
#endif

/**
 * @brief	List of available HIF channel events
 */
typedef enum
{
	HIF_CHNL_EVT_RX_IRQ = (1U << 0U),	/*!< RX interrupt - packet received */
	HIF_CHNL_EVT_TX_IRQ = (1U << 1U),	/*!< TX interrupt - packet transmitted */
#if (TRUE == PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED)
	HIF_CHNL_EVT_RX_OOB = (1U << 2U)	/*!< Out of RX buffers */
#endif
} pfe_hif_chnl_event_t;

typedef struct __pfe_hif_chnl_tag pfe_hif_chnl_t;
typedef void (* pfe_hif_chnl_drain_cbk_t)(void *arg, void **buf_pa);
typedef void (* pfe_hif_chnl_cbk_t)(void *arg);

/*	This is the channel ID used to identify HIF_NOCPY channel */
#define PFE_HIF_CHNL_NOCPY_ID		1000U

#define MAC_ADDRESS_SIZE		6U
/*	RX */
errno_t pfe_hif_chnl_rx_enable(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
void pfe_hif_chnl_rx_disable(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
errno_t pfe_hif_chnl_rx(pfe_hif_chnl_t *chnl, void **buf_pa, uint32_t *len, bool_t *lifm) __attribute__((hot));
errno_t pfe_hif_chnl_rx_va(pfe_hif_chnl_t *chnl, void **buf_va, uint32_t *len, bool_t *lifm, void **meta) __attribute__((hot));
uint32_t pfe_hif_chnl_get_meta_size(const pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
errno_t pfe_hif_chnl_release_buf(pfe_hif_chnl_t *chnl, void *buf_va) __attribute__((hot));
void pfe_hif_chnl_rx_dma_start(const pfe_hif_chnl_t *chnl) __attribute__((hot));
bool_t pfe_hif_chnl_can_accept_rx_buf(const pfe_hif_chnl_t *chnl) __attribute__((pure, hot));
errno_t pfe_hif_chnl_supply_rx_buf(const pfe_hif_chnl_t *chnl, const void *buf_pa, uint32_t size) __attribute__((hot));
uint32_t pfe_hif_chnl_get_rx_fifo_depth(const pfe_hif_chnl_t *chnl) __attribute__((pure, cold, aligned(64)));

/*	TX */
errno_t pfe_hif_chnl_tx_enable(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
void pfe_hif_chnl_tx_disable(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
errno_t pfe_hif_chnl_tx(pfe_hif_chnl_t *chnl, const void *buf_pa, const void *buf_va, uint32_t len, bool_t lifm) __attribute__((hot));
void pfe_hif_chnl_tx_dma_start(const pfe_hif_chnl_t *chnl) __attribute__((hot));
bool_t pfe_hif_chnl_can_accept_tx_num(const pfe_hif_chnl_t *chnl, uint16_t num) __attribute__((pure, hot));
bool_t pfe_hif_chnl_tx_fifo_empty(const pfe_hif_chnl_t *chnl) __attribute__((pure, hot));
bool_t pfe_hif_chnl_has_tx_conf(const pfe_hif_chnl_t *chnl) __attribute__((pure, hot));
errno_t pfe_hif_chnl_get_tx_conf(pfe_hif_chnl_t *chnl) __attribute__((hot));
uint32_t pfe_hif_chnl_get_tx_fifo_depth(const pfe_hif_chnl_t *chnl) __attribute__((pure, cold, aligned(64)));
bool_t pfe_hif_chnl_tx_fifo_is_below_wm(const pfe_hif_chnl_t *chnl);

/*	Instance control */
pfe_hif_chnl_t *pfe_hif_chnl_create(addr_t cbus_base_va, uint32_t id, const pfe_bmu_t *bmu) __attribute__((cold, aligned(64)));
errno_t pfe_hif_chnl_isr(pfe_hif_chnl_t *chnl) __attribute__((hot));
void pfe_hif_chnl_destroy(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
errno_t pfe_hif_chnl_reset_fifos(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
errno_t pfe_hif_chnl_set_event_cbk(pfe_hif_chnl_t *chnl, pfe_hif_chnl_event_t event, pfe_hif_chnl_cbk_t cbk, void *arg);
void pfe_hif_chnl_irq_mask(pfe_hif_chnl_t *chnl);
void pfe_hif_chnl_irq_unmask(pfe_hif_chnl_t *chnl);
void pfe_hif_chnl_rx_irq_mask(pfe_hif_chnl_t *chnl) __attribute__((hot));
void pfe_hif_chnl_rx_irq_unmask(pfe_hif_chnl_t *chnl) __attribute__((hot));
void pfe_hif_chnl_tx_irq_mask(pfe_hif_chnl_t *chnl) __attribute__((hot));
void pfe_hif_chnl_tx_irq_unmask(pfe_hif_chnl_t *chnl) __attribute__((hot));
errno_t pfe_hif_chnl_get_rx_irq_coalesce(pfe_hif_chnl_t *chnl, uint32_t *frames, uint32_t *cycles);
errno_t pfe_hif_chnl_set_rx_irq_coalesce(pfe_hif_chnl_t *chnl, uint32_t frames, uint32_t cycles);
uint32_t pfe_hif_chnl_get_id(const pfe_hif_chnl_t *chnl) __attribute__((pure, cold, aligned(64)));
void pfe_hif_chnl_dump_ring(const pfe_hif_chnl_t *chnl, bool_t dump_rx, bool_t dump_tx, struct seq_file *seq, uint8_t verb_level) __attribute__((cold, aligned(64)));
void pfe_hif_chnl_dump_tx_ring_to_ndev(const pfe_hif_chnl_t *chnl, struct net_device *ndev, void (*ndev_print)(void *ndev, const char *fmt, ...)) __attribute__((cold, aligned(64)));
uint32_t pfe_hif_chnl_get_tx_cnt(const pfe_hif_chnl_t *chnl);
uint32_t pfe_hif_chnl_get_rx_cnt(const pfe_hif_chnl_t *chnl);

uint32_t pfe_hif_chnl_get_text_statistics(const pfe_hif_chnl_t *chnl, struct seq_file *seq, uint8_t verb_level) __attribute__((cold, aligned(64)));

#endif /* PUBLIC_PFE_HIF_CHNL_H_ */
