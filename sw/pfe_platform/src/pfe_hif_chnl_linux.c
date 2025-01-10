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
 * @file		pfe_hif_chnl.c
 * @brief		The HIF channel module source file.
 * @details		This file contains HIF channel-related functionality abstracted using
 * 				configurable, HW-specific calls. Each HW platform shall supply its own
 * 				pfe_hif_csr.h header implementing the HW-specific parts.
 *
 * 				Default Mode
 * 				------------
 * 				Default mode allows user to transmit and receive buffers using their
 * 				physical addresses. There is no other functionality and only the
 * 				default API is sufficient to handle the data-path:
 * 					- pfe_hif_chnl_can_accept_tx_num()
 * 					- pfe_hif_chnl_tx()
 * 					- pfe_hif_chnl_supply_rx_buffer()
 * 					- pfe_hif_chnl_rx()
 *
 * 				TX example:
 * 				if pfe_hif_chnl_can_accept_tx_num() is TRUE then
 * 				  pfe_hif_chnl_tx()
 * 				endif
 *
 * 				RX example:
 * 				// Supply RX buffers
 *				while pfe_hif_chnl_can_accept_rx_buf() do
 *				  pfe_hif_chnl_supply_rx_buf()
 *				endwhile
 *
 *				// Receive
 *				while (1)
 *				  if pfe_hif_chnl_rx() then
 *				    1 Process the buffer
 *				    2 pfe_hif_chnl_supply_rx_buf()
 *				  endif
 *				endwhile
 *
 * 				RX Buffer Management Mode
 * 				-------------------------
 * 				In case the PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED is set to TRUE the PFE HIF
 * 				channel module provides full RX buffer management functionality. It
 * 				creates pool of buffers and transparently populates the RX ring. Instead
 * 				of default RX API the extended version is provided:
 * 					- pfe_hif_chnl_rx_va()
 * 					- pfe_hif_chnl_release_buf()
 *
 * 				Every buffer received via pfe_hif_chnl_rx_va() must be subsequently
 * 				released by the pfe_hif_chnl_release_rx_va(). With the RX management
 * 				support also the pfe_hif_chnl_get_meta_size() is available for
 * 				sanity check implementation related to size of the pre-allocated
 * 				buffer-related meta storage.
 *
 * 				TX example: The same as in the Default Mode case.
 *
 * 				RX example:
 *				// Sanity check
 * 				if my metadata size does not match pfe_hif_chnl_get_meta_size()
 * 				  Throw error
 * 				endif
 *
 *				// Receive
 *				while (1)
 * 				  if pfe_hif_chnl_rx_va() then
 * 				    1 Use pre-allocated metadata storage to bind custom data with the buffer for better performance
 * 				    2 Process the buffer
 * 				    3 When finished, call the pfe_hif_chnl_release_buf()
 * 				  endif
 * 				endwhile
 *
 */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_hif_chnl_linux.h"

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)

#include "bpool.h"

#define PFE_BUF_SIZE			2048U

#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

#define DUMMY_TX_BUF_LEN		64U
#define DUMMY_RX_BUF_LEN		2048U
#define DUMMY_FRAME_INVALID		0
#define DUMMY_FRAME_IHC_SELF	1

#define BUFFERS_CACHED TRUE

typedef struct
{
	pfe_hif_chnl_cbk_t cbk;
	void *arg;
} pfe_hif_chnl_cbk_storage_t;

/**
 * @brief	The HIF channel representation type
 * @details	Members are accessed with every channel operation (transmit/receive)
 * 			thus the structure is allocated with proper alignment to
 * 			improve cache locality.
 */
struct __attribute__((aligned(HAL_CACHE_LINE_SIZE))) __pfe_hif_chnl_tag
{
	addr_t cbus_base_va;		/*	CBUS base virtual address */
	uint32_t id;			/*	Channel ID within HIF (0, 1, 2, ...) */
	pfe_hif_ring_t *rx_ring;	/*	The RX ring instance */
	pfe_hif_ring_t *tx_ring;	/*	The TX ring instance */
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
	bpool_t *rx_pool;		/*	Pool of available RX buffers */
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */
	oal_spinlock_t lock __attribute__((aligned(HAL_CACHE_LINE_SIZE)));	/*	Channel HW resources protection */
	oal_spinlock_t rx_lock __attribute__((aligned(HAL_CACHE_LINE_SIZE)));	/*	RX resource protection */
	pfe_hif_chnl_cbk_storage_t rx_cbk;	/*	RX callback */
	pfe_hif_chnl_cbk_storage_t tx_cbk;	/*	TX callback */
	pfe_hif_chnl_cbk_storage_t rx_tx_cbk;	/*	RX/TX callback */
#if (TRUE == PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED)
	pfe_hif_chnl_cbk_storage_t rx_oob_cbk;	/*	RX Out-Of-Buffers callback */
#endif
};

static errno_t pfe_hif_chnl_bind_rx_ring(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
static errno_t pfe_hif_chnl_bind_tx_ring(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
static errno_t pfe_hif_chnl_init(pfe_hif_chnl_t *chnl) __attribute__((cold, aligned(64)));
static void pfe_hif_chnl_free_dummy_frame(void *frame);
static errno_t pfe_hif_chnl_send_dummy_frame(pfe_hif_chnl_t *chnl, uint32_t mode, void **frame) __attribute__((cold, aligned(64)));

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
static void pfe_hif_chnl_refill_rx_buffers(const pfe_hif_chnl_t *chnl) __attribute__((hot));
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

/**
 * @brief		Channel master ISR
 * @param[in]	chnl The channel instance
 * @return		EOK if interrupt has been handled
 */
__attribute__((hot)) errno_t pfe_hif_chnl_isr(pfe_hif_chnl_t *chnl)
{
	errno_t ret;
	pfe_hif_chnl_event_t events;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_hif_chnl_cfg_isr(chnl->cbus_base_va, chnl->id, &events);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	/*	Run callbacks for identified interrupts here */
	if (NULL != chnl->rx_tx_cbk.cbk)
	{
		if (events & HIF_CHNL_EVT_RX_IRQ || events & HIF_CHNL_EVT_TX_IRQ)
		{
			chnl->rx_tx_cbk.cbk(chnl->rx_tx_cbk.arg);
		}
	}
	else
	{
		if (HIF_CHNL_EVT_RX_IRQ == (events & HIF_CHNL_EVT_RX_IRQ))
		{
			if (NULL != chnl->rx_cbk.cbk)
			{
				chnl->rx_cbk.cbk(chnl->rx_cbk.arg);
			}
			else
			{
				NXP_LOG_DEBUG("Unhandled HIF_CHNL_EVT_RX_IRQ detected\n");
			}
		}

		if (HIF_CHNL_EVT_TX_IRQ == (events & HIF_CHNL_EVT_TX_IRQ))
		{
			if (NULL != chnl->tx_cbk.cbk)
			{
				chnl->tx_cbk.cbk(chnl->tx_cbk.arg);
			}
			else
			{
				NXP_LOG_DEBUG("Unhandled HIF_CHNL_EVT_TX_IRQ detected\n");
			}
		}
	}

	return ret;
}

/**
 * @brief		Mask channel interrupts
 * @param[in]	chnl The channel instance
 */
void pfe_hif_chnl_irq_mask(pfe_hif_chnl_t *chnl)
{
	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	pfe_hif_chnl_cfg_irq_mask(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Unmask channel interrupts
 * @param[in]	chnl The channel instance
 */
void pfe_hif_chnl_irq_unmask(pfe_hif_chnl_t *chnl)
{
	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	pfe_hif_chnl_cfg_irq_unmask(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
/**
 * @brief	Supply fresh RX buffers to the channel
 * @details	Function populates channel's RX resource with buffer from internal pool
 */
__attribute__((hot)) static void pfe_hif_chnl_refill_rx_buffers(const pfe_hif_chnl_t *chnl)
{
	void *new_buffer_va;
	void *new_buffer_pa;
	errno_t err;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	while (TRUE == pfe_hif_chnl_can_accept_rx_buf(chnl))
	{
		/*	Get fresh buffer. Resource protection is embedded. */
		new_buffer_va = bpool_get(chnl->rx_pool);

		if (likely(NULL != new_buffer_va))
		{
			/*	Get physical address */
			new_buffer_pa = bpool_get_pa(chnl->rx_pool, new_buffer_va);

			if (unlikely(NULL == new_buffer_pa))
			{
				NXP_LOG_ERROR("VA->PA conversion failed, origin buffer VA: v0x%p\n", new_buffer_va);
			}

			/*	Write buffer to the HW */
			err = pfe_hif_chnl_supply_rx_buf(chnl, new_buffer_pa, PFE_BUF_SIZE);
			if (unlikely(EOK != err))
			{
				NXP_LOG_WARNING("HIF channel did not accept new RX buffer\n");

				/*	Return buffer to the pool. Resource protection is embedded. */
				bpool_put(chnl->rx_pool, new_buffer_va);
				break;
			}
		}
		else
		{
			/*	Not enough buffers in the SW pool */
			NXP_LOG_WARNING("Out of buffers (RX pool)\n");
			break;
		}
	}

	return;
}
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

/**
 * @brief		Create new HIF channel instance
 * @details		Creates and initializes HIF channel instance
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	id Channel identifier to bind SW instance to a real HW HIF channel
 * @param[in]	bmu If set, the channel will use it to allocate RX buffers. It is mandatory
 * 					for HIF NOCPY channel abstraction.
 * @return		The channel instance or NULL if failed
 */
__attribute__((cold, aligned(64))) pfe_hif_chnl_t *pfe_hif_chnl_create(addr_t cbus_base_va, uint32_t id, const pfe_bmu_t *bmu)
{
	pfe_hif_chnl_t *chnl;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		NXP_LOG_ERROR("HIF NOCPY is not supported\n");
		return NULL;
	}

	chnl = oal_mm_malloc_contig_aligned_cache(sizeof(pfe_hif_chnl_t), HAL_CACHE_LINE_SIZE);

	if (NULL == chnl)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		return NULL;
	}
	else
	{
		memset(chnl, 0, sizeof(pfe_hif_chnl_t));
		chnl->cbus_base_va = cbus_base_va;
		chnl->id = id;
		chnl->tx_ring = NULL;
		chnl->rx_ring = NULL;
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
		chnl->rx_pool = NULL;
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

		if (EOK != oal_spinlock_init(&chnl->lock))
		{
			NXP_LOG_ERROR("Channel mutex initialization failed\n");

			oal_mm_free_contig(chnl);

			return NULL;
		}

		if (EOK != oal_spinlock_init(&chnl->rx_lock))
		{
			NXP_LOG_ERROR("Channel RX mutex initialization failed\n");

			(void)oal_spinlock_destroy(&chnl->lock);
			oal_mm_free_contig(chnl);

			return NULL;
		}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
		if (NULL != bmu)
		{
			/*	This is not supported. SW buffer pool will be used instead. */
			NXP_LOG_WARNING("BMU-based RX buffer pool not supported for standard HIF channels. SW pool will be used instead.\n");
		}
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

		if (EOK != oal_spinlock_lock(&chnl->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		ret = pfe_hif_chnl_cfg_init(chnl->cbus_base_va, id);

		if (EOK != oal_spinlock_unlock(&chnl->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		if (EOK != ret)
		{
			NXP_LOG_ERROR("HIF channel init failed\n");
			goto free_and_fail;
		}

		if (EOK != pfe_hif_chnl_init(chnl))
			goto free_and_fail;
	}

	return chnl;

free_and_fail:

	(void)oal_spinlock_destroy(&chnl->lock);
	(void)oal_spinlock_destroy(&chnl->rx_lock);

	oal_mm_free_contig(chnl);
	return NULL;
}

/**
 * @brief		Get channel identifier
 * @param[in]	chnl The channel instance
 * @return		The identifier of the channel
 */
__attribute__((pure, cold, aligned(64))) uint32_t pfe_hif_chnl_get_id(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return UINT_MAX; /* Available via oal_types.h */
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return chnl->id;
}

/**
 * @brief		Enable TX
 * @details		Activate the TX ring and enable TX ring interrupts
 * @param[in]	chnl The channel instance
 * @retval		EOK Success
 * @retval		EFAULT TX ring not found
 */
__attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_tx_enable(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL == chnl->tx_ring)
	{
		NXP_LOG_ERROR("Can't enable TX: TX ring not set\n");
		return EFAULT;
	}

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	HIF */
	pfe_hif_chnl_cfg_tx_enable(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Disable TX
 * @details		De-activate the TX ring and disable TX ring interrupts. All buffers
 * 				previously committed for transmission via pfe_hif_chnl_tx() are marked
 *				as "transmitted" and related TX confirmations can be retrieved via
 * 				pfe_hif_chnl_get_tx_conf().
 * @param[in]	chnl The channel instance
 */
__attribute__((cold, aligned(64))) void pfe_hif_chnl_tx_disable(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	Stop data transmission */
	pfe_hif_chnl_cfg_tx_disable(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	if (NULL != chnl->tx_ring)
	{
		/*	Invalidate the TX ring */
		/* pfe_hif_ring_invalidate(chnl->tx_ring); */
	}
}

/**
 * @brief		Enable RX
 * @details		Activate the RX ring and enable RX ring interrupts
 * @param[in]	chnl The channel instance
 * @retval		EOK Success
 * @retval		EFAULT RX ring not found
 */
__attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_rx_enable(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL == chnl->rx_ring)
	{
		NXP_LOG_ERROR("Can't enable RX: RX ring not set\n");
		return EFAULT;
	}

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	HIF */
	pfe_hif_chnl_cfg_rx_enable(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Disable RX
 * @details		De-activate the RX ring
 * @param[in]	chnl The channel instance
 * @note		Must not be preempted by pfe_hif_chnl_supply_rx_buf()
 */
__attribute__((cold, aligned(64))) void pfe_hif_chnl_rx_disable(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	Stop data reception */
	pfe_hif_chnl_cfg_rx_disable(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Trigger RX DMA
 * @details		One can trigger the HW to start processing of the RX ring.
 * 				This is needed when the RX ring is modified after the
 * 				pfe_hif_chnl_supply_rx_buf() call(s).
 * @param[in]	chnl The channel instance
 */
__attribute__((hot)) void pfe_hif_chnl_rx_dma_start(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	No resource protection here, DMA trigger is atomic. */
	pfe_hif_chnl_cfg_rx_dma_start(chnl->cbus_base_va, chnl->id);
}

/**
 * @brief		Trigger TX DMA
 * @details		Trigger the HW to start processing of the TX ring. Needed
 * 				after TX ring is modified after the pfe_hif_chnl_tx() call(s).
 * @param[in]	chnl The channel instance
 */
__attribute__((hot)) void pfe_hif_chnl_tx_dma_start(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	No resource protection here. DMA trigger is atomic. */
	pfe_hif_chnl_cfg_tx_dma_start(chnl->cbus_base_va, chnl->id);
}

/**
 * @brief		Attach event callback
 * @param[in]	chnl The channel instance
 * @param[in]	event Event triggering the handler. Rx and Tx events can
 *			have a shared callback.
 * @param[in]	isr The ISR
 * @param[in]	arg The ISR argument
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_hif_chnl_set_event_cbk(pfe_hif_chnl_t *chnl, pfe_hif_chnl_event_t event, pfe_hif_chnl_cbk_t cbk, void *arg)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	if ((HIF_CHNL_EVT_TX_IRQ | HIF_CHNL_EVT_RX_IRQ) == event)
	{
		/*	RX/TX callback */
		chnl->rx_tx_cbk.arg = arg;
		chnl->rx_tx_cbk.cbk = cbk;
	}
	else if (HIF_CHNL_EVT_TX_IRQ == event)
	{
		/*	TX callback */
		chnl->tx_cbk.arg = arg;
		chnl->tx_cbk.cbk = cbk;
	}
	else if (HIF_CHNL_EVT_RX_IRQ == event)
	{
		/*	RX callback */
		chnl->rx_cbk.arg = arg;
		chnl->rx_cbk.cbk = cbk;
	}
#if (TRUE == PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED)
	else if (HIF_CHNL_EVT_RX_OOB == event)
	{
		/*	Out of RX buffers event handler */
		chnl->rx_oob_cbk.arg = arg;
		chnl->rx_oob_cbk.cbk = cbk;
	}
#endif
	else
	{
		/*	More events need to be supported here */
		ret = EINVAL;
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Disable RX interrupt
 * @param[in]	chnl The channel instance
 * @return		EOK if success, error code otherwise
 */
__attribute__((hot)) void pfe_hif_chnl_rx_irq_mask(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	pfe_hif_chnl_cfg_rx_irq_mask(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
}

/**
 * @brief		Enable RX interrupt
 * @param[in]	chnl The channel instance
 * @return		EOK if success, error code otherwise
 */
__attribute__((hot)) void pfe_hif_chnl_rx_irq_unmask(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	pfe_hif_chnl_cfg_rx_irq_unmask(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Disable TX interrupt
 * @param[in]	chnl The channel instance
 * @return		EOK if success, error code otherwise
 */
__attribute__((hot)) void pfe_hif_chnl_tx_irq_mask(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	pfe_hif_chnl_cfg_tx_irq_mask(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
}

/**
 * @brief		Enable TX interrupt
 * @param[in]	chnl The channel instance
 * @return		EOK if success, error code otherwise
 */
__attribute__((hot)) void pfe_hif_chnl_tx_irq_unmask(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	pfe_hif_chnl_cfg_tx_irq_unmask(chnl->cbus_base_va, chnl->id);

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
}

/**
 * @brief		Get HIF channel RX coalesce
 * @details		Get HIF channel coalesce setting
 * @param[in]	chnl The channel instance
 * @param[out]	frames The channel coalesce setting by frames
 * @param[out]	cycles The channel coalesce setting by cycles
 * @retval		EOK On success
 */
errno_t pfe_hif_chnl_get_rx_irq_coalesce(pfe_hif_chnl_t *chnl, uint32_t *frames, uint32_t *cycles)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl) || (NULL == frames) || (NULL == cycles)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_hif_chnl_cfg_get_rx_irq_coalesce(chnl->cbus_base_va, chnl->id, frames, cycles);
}

/**
 * @brief		Set HIF channel RX coalesce
 * @details		Set HIF channel coalesce setting.
 * 				For frames=0 and cycles=0, the coalescing will be disabled.
 * @param[in]	chnl The channel instance
 * @param[in]	frames The channel coalesce setting by frames
 * @param[in]	cycles The channel coalesce setting by cycles
 * @retval		EOK On success
 */
errno_t pfe_hif_chnl_set_rx_irq_coalesce(pfe_hif_chnl_t *chnl, uint32_t frames, uint32_t cycles)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_hif_chnl_cfg_set_rx_irq_coalesce(chnl->cbus_base_va, chnl->id, frames, cycles);
}

/**
 * @brief		Get TX confirmation status
 * @details		After pfe_hif_chnl_tx() call the HIF channel will transmit the
 * 				supplied buffer. Once the transmission has been done a TX confirmation
 * 				is generated. This function can be used to query the channel whether
 * 				some new TX confirmations have been generated and are ready to be
 * 				processed.
 * @param[in]	chnl The channel instance
 * @return		TRUE if channel got new TX confirmation, FALSE otherwise
 */
__attribute__((pure, hot)) bool_t pfe_hif_chnl_has_tx_conf(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return 0 != pfe_hif_ring_get_fill_level(chnl->tx_ring);
}

/**
 * @brief		Query if new RX buffer can be supplied
 * @param		chnl The channel instance
 * @return		TRUE if RX resource can accept new buffer
 */
__attribute__((pure, hot)) bool_t pfe_hif_chnl_can_accept_rx_buf(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	A single entry must remain unused within the ring
	 	because HIF expects that. */
	return (pfe_hif_ring_get_fill_level(chnl->rx_ring) < (pfe_hif_ring_get_len(chnl->rx_ring) - 1U));
}

/**
 * @brief		Check if channel can accept number of TX requests
 * @param[in]	chnl The channel instance
 * @param[in]	num Number of TX requests
 * @retval		TRUE Channel can accept 'num' TX requests (buffers)
 * @retval		FALSE Not enough space in TX FIFO
 */
__attribute__((pure, hot)) bool_t pfe_hif_chnl_can_accept_tx_num(const pfe_hif_chnl_t *chnl, uint16_t num)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	A single entry must remain unused within the ring because HIF expects that. */
	return ((pfe_hif_ring_get_len(chnl->tx_ring) - 1U - pfe_hif_ring_get_fill_level(chnl->tx_ring)) >= num);
}

/**
 * @brief		Check if the TX FIFO is empty
 * @param[in]	chnl The channel instance
 * @retval		TRUE TX FIFO is empty
 * @retval		FALSE TX FIFO contains entries waiting for transmission
 */
__attribute__((pure, hot)) bool_t pfe_hif_chnl_tx_fifo_empty(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return TRUE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return (0U == pfe_hif_ring_get_fill_level(chnl->tx_ring));
}

/**
 * @brief		Get the RX FIFO depth
 * @param[in]	chnl The channel instance
 * @return		Size of the RX FIFO in number of entries
 */
__attribute__((pure, cold, aligned(64))) uint32_t pfe_hif_chnl_get_rx_fifo_depth(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_hif_ring_get_len(chnl->rx_ring);
}

/**
 * @brief		Get the TX FIFO depth
 * @param[in]	chnl The channel instance
 * @return		Size of the TX FIFO in number of entries
 */
__attribute__((pure, cold, aligned(64))) uint32_t pfe_hif_chnl_get_tx_fifo_depth(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_hif_ring_get_len(chnl->tx_ring);
}

/**
 * @brief		Request transmission of a buffer
 * @note		The TX resource availability shall be checked before this function
 * 				is called using the pfe_hif_chnl_can_accept_tx_buf() call.
 * @note		Function is __NOT__ reentrant
 * @param[in]	chnl The channel instance
 * @param[in]	buf_pa Physical address of the buffer to be transmitted
 * @param[in]	buf_va Virtual address of the buffer to be transmitted
 * @param[in]	len Length of the buffer in bytes
 * @param[in]	lifm The last-in-frame indicator. Complete packet can consist
 * 				     of multiple buffers. The last one shall be marked with
 * 				     lifm=TRUE.
 * @retval		EOK Success
 * @retval		ENOSPC TX queue is full
 * @retval		EIO Internal error
 */
__attribute__((hot)) errno_t pfe_hif_chnl_tx(pfe_hif_chnl_t *chnl, const void *buf_pa, const void *buf_va, uint32_t len, bool_t lifm)
{
	errno_t err = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl) || (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if (TRUE == HAL_HANDLE_CACHE)
	/*	Flush cache over the buffer */
	oal_mm_cache_flush(buf_va, buf_pa, len);
#endif /* HAL_HANDLE_CACHE */

	(void)buf_va;
	err = pfe_hif_ring_enqueue_buf(chnl->tx_ring, buf_pa, len, lifm);

	if (TRUE == lifm)
	{
		/*	Trigger the DMA */
		pfe_hif_chnl_tx_dma_start(chnl);
	}

	return err;
}

/**
 * @brief		Get TX confirmation
 * @details		Each frame transmitted via pfe_hif_chnl_tx() will produce exactly
 * 				one TX confirmation which can be retrieved by this function.
 * @param[in]	chnl The channel instance
 * @retval		EOK Next frame has been transmitted
 * @retval		EAGAIN No pending confirmations
 */
__attribute__((hot)) errno_t pfe_hif_chnl_get_tx_conf(pfe_hif_chnl_t *chnl)
{
	bool_t lifm;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Get all transmitted chunks but only the last-in-frame
		will be reported as TX confirmation. */
	while (EOK == pfe_hif_ring_dequeue_plain(chnl->tx_ring, &lifm))
	{
		if (TRUE == lifm)
		{
			return EOK;
		}
	}

	return EAGAIN;
}

#if (FALSE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
/**
 * @brief		Receive a buffer
 * @details		When channel has received some data into RX buffer then this
 *				function will retrieve it.
 * @note		The RX resource availability shall be checked before this function
 * 				is called using the pfe_hif_chnl_can_accept_rx_buf() call.
 * @param[in]	chnl The channel instance
 * @param[out]	buf_pa Pointer to memory where pointer to the received data shall
 * 				       be written (physical address, as seen by host)
 * @param[out]	len Pointer to memory where length in bytes of the received
 *				    data shall be written
 * @param[out]	lifm Pointer to memory where the last-in-frame flag shall be
 * 					 written
 * @retval		EOK Buffer received
 * @retval		EAGAIN No more data to receive right now
 */
__attribute__((hot)) errno_t pfe_hif_chnl_rx(pfe_hif_chnl_t *chnl, void **buf_pa, uint32_t *len, bool_t *lifm)
{
	errno_t err;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl) || (NULL == buf_pa) || (NULL == len) || (NULL == lifm) || (NULL == chnl->rx_ring)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	err = pfe_hif_ring_dequeue_buf(chnl->rx_ring, buf_pa, len, lifm);

#if (TRUE == PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED)
	/*	Check if ring has enough RX buffers */
	if (unlikely(0U == pfe_hif_ring_get_fill_level(chnl->rx_ring)))
	{
		/*	Out of RX buffers */
		if (likely(NULL != chnl->rx_oob_cbk.cbk))
		{
			chnl->rx_oob_cbk.cbk(chnl->rx_oob_cbk.arg);
		}
	}
#endif

	return err;
}
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
/**
 * @brief		Receive a buffer (virtual address)
 * @details		When channel has received some data into RX buffer then this
 *				function will retrieve it.
 * @note		The RX resource availability shall be checked before this function
 * 				is called using the pfe_hif_chnl_can_accept_rx_buf() call.
 * @param[in]	chnl The channel instance
 * @param[out]	buf_va Pointer to memory where pointer to the received data shall
 * 				       be written (virtual address, as seen by host)
 * @param[out]	len Pointer to memory where length in bytes of the received
 *				    data shall be written
 * @param[out]	lifm Pointer to memory where the last-in-frame flag shall be
 * 					 written
 * @param[out]	meta Pointer to memory where pointer to pre-allocated memory
 * 					 associated with the returned buffer shall be stored. Size
 * 					 of the memory can be obtained by the pfe_hif_chnl_get_meta_size().
 * @retval		EOK Buffer received
 * @retval		EAGAIN No more data to receive right now
 * @retval		ENOMEM Out of memory
 */
__attribute__((hot)) errno_t pfe_hif_chnl_rx_va(pfe_hif_chnl_t *chnl, void **buf_va, uint32_t *len, bool_t *lifm, void **meta)
{
	errno_t err;
	void *buf_pa;
	pfe_ct_hif_rx_hdr_t *hif_hdr_ptr = NULL;
	uint32_t vlan_header;
	uint32_t flags_map[4] = {HIF_RX_HIF0_VLAN, HIF_RX_HIF1_VLAN, HIF_RX_HIF2_VLAN, HIF_RX_HIF3_VLAN};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl) || (NULL == buf_va) || (NULL == len) || (NULL == lifm) || (NULL == meta)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	err = pfe_hif_ring_dequeue_buf(chnl->rx_ring, &buf_pa, len, lifm);
	if (EOK == err)
	{
		/*	Return virtual address */
		*buf_va = bpool_get_va(chnl->rx_pool, buf_pa);

		hif_hdr_ptr = (pfe_ct_hif_rx_hdr_t *) *buf_va;
		/*      Check if pkt is tagged */
		if ((oal_ntohl(hif_hdr_ptr->flags) & (flags_map[chnl->id])) != 0)
		{
			/*	On HIF hw is adding the vlan tag at 12 byte offset from the beggining
				of the buffer. Copy the vlan header to the right position at offset 12
				of the pkt witch is after the hif rx header. */
			vlan_header = hif_hdr_ptr->rx_timestamp_s;
			memmove((uint8_t *)*buf_va + 2*MAC_ADDRESS_SIZE, (uint8_t *)*buf_va + sizeof(pfe_ct_hif_rx_hdr_t), 2*MAC_ADDRESS_SIZE + sizeof(vlan_header));
			memcpy((uint8_t *)*buf_va + sizeof(pfe_ct_hif_rx_hdr_t) + 2*MAC_ADDRESS_SIZE, &vlan_header, sizeof(vlan_header));
		}

#if (TRUE == HAL_HANDLE_CACHE)
		/*	Invalidate cache over the received data area */
		oal_mm_cache_inval(*buf_va, buf_pa, *len);
#endif /* HAL_HANDLE_CACHE */

		/*	Return pointer to the pre-allocated memory location where
			a buffer-related metadata can be stored. */
		*meta = bpool_get_meta_storage(chnl->rx_pool, *buf_va);
	}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED)
	/*	Check if ring has enough RX buffers */
	if (0U == pfe_hif_ring_get_fill_level(chnl->rx_ring))
	{
		/*	Out of RX buffers */
		if (NULL != chnl->rx_oob_cbk.cbk)
		{
			chnl->rx_oob_cbk.cbk(chnl->rx_oob_cbk.arg);
		}
	}
#endif

	return err;
}

/**
 * @brief		Get size of metadata storage returned by the pfe_hif_chnl_rx_va()
 * @details		When driver is willing to use the pre-allocated storage associated
 * 				with every RX buffer it must not write more data than is the
 * 				pre-allocated block size. To ensure that, it shall call this API
 * 				to get maximum number of bytes which can be written to the 'meta'
 * 				memory location returned by the pfe_hif_chnl_rx_va().
 * @param[in]	chnl The channel instance
 * @return		Size of the metadata storage pointed by the 'meta' arugument of
 * 				the pfe_hif_chnl_rx_va().
 */
__attribute__((cold, aligned(64))) uint32_t pfe_hif_chnl_get_meta_size(const pfe_hif_chnl_t *chnl)
{
	(void)chnl;
	return bpool_get_meta_storage_size();
}

/**
 * @brief		Release a channel provided buffer
 * @param[in]	chnl The channel instance
 * @param[in]	buf_va Pointer to buffer to be released (virtual)
 * @retval		EOK if success, error code otherwise
 */
__attribute__((hot)) errno_t pfe_hif_chnl_release_buf(pfe_hif_chnl_t *chnl, void *buf_va)
{
	addr_t buf_pa;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	buf_pa = (addr_t)bpool_get_pa(chnl->rx_pool, buf_va);

	if (unlikely(NULL == (void *)buf_pa))
	{
		NXP_LOG_ERROR("VA->PA conversion failed, origin buffer VA: v0x%p\n", buf_va);
	}

	if (unlikely(EOK != oal_spinlock_lock(&chnl->rx_lock)))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	Release the buffer to ring */
	ret = pfe_hif_ring_enqueue_buf(chnl->rx_ring, (void *)buf_pa, PFE_BUF_SIZE, TRUE);

	if (unlikely(EOK != oal_spinlock_unlock(&chnl->rx_lock)))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return ret;
}
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

/**
 * @brief		Supply RX buffer to be used for data reception
 * @param[in]	chnl The channel instance
 * @param[in]	buf_pa The RX buffer to be supplied (physical address, as seen
 * 					   by host
 * @param[in]	size Size of the supplied buffer in bytes
 * @return		EOK Success
 * @note		Must not be preempted by pfe_hif_chnl_rx_disable()
 */
__attribute__((hot)) errno_t pfe_hif_chnl_supply_rx_buf(const pfe_hif_chnl_t *chnl, const void *buf_pa, uint32_t size)
{
	errno_t err = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl) || (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	err = pfe_hif_ring_enqueue_buf(chnl->rx_ring, buf_pa, size, TRUE);
	if (unlikely(EOK != err))
	{
		NXP_LOG_WARNING("pfe_hif_ring_enqueue_buf() failed: %d\n", err);
	}

	return err;
}

/**
 * @brief		Assign RX BD ring
 * @details		Configure RX buffer descriptor ring address of the channel.
 * 				This binds channel with a RX BD ring.
 * @param[in]	chnl The channel instance
 * @retval		EOK Success
 * @retval		EFAULT Invalid ring instance
 */
__attribute__((cold, aligned(64))) static errno_t pfe_hif_chnl_bind_rx_ring(pfe_hif_chnl_t *chnl)
{
	pfe_hif_ring_t *ring = chnl->rx_ring;
	void *rx_ring_pa, *wb_tbl_pa;
	uint32_t wb_tbl_len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	rx_ring_pa = pfe_hif_ring_get_base_pa(ring);
	wb_tbl_pa = pfe_hif_ring_get_wb_tbl_pa(ring);

	if (NULL == rx_ring_pa)
	{
		NXP_LOG_ERROR("RX ring physical address is NULL\n");
		return EFAULT;
	}

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	HIF */
	pfe_hif_chnl_cfg_set_rx_bd_ring_addr(chnl->cbus_base_va, chnl->id, rx_ring_pa);

	if (NULL != wb_tbl_pa)
	{
		wb_tbl_len = pfe_hif_ring_get_wb_tbl_len(ring);
		pfe_hif_chnl_cfg_set_rx_wb_table(chnl->cbus_base_va, chnl->id, wb_tbl_pa, wb_tbl_len);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Assign TX BD ring
 * @details		Configure TX buffer descriptor ring address of the channel.
 * 				This binds channel with a TX BD ring.
 * @param[in]	chnl The channel instance
 * @retval		EOK Success
 * @retval		EFAULT Invalid ring instance
 */
__attribute__((cold, aligned(64))) static errno_t pfe_hif_chnl_bind_tx_ring(pfe_hif_chnl_t *chnl)
{
	pfe_hif_ring_t *ring = chnl->tx_ring;
	void *tx_ring_pa, *wb_tbl_pa;
	uint32_t wb_tbl_len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	tx_ring_pa = pfe_hif_ring_get_base_pa(ring);
	wb_tbl_pa = pfe_hif_ring_get_wb_tbl_pa(ring);

	if (NULL == tx_ring_pa)
	{
		NXP_LOG_ERROR("TX ring physical address is NULL\n");
		return EFAULT;
	}

	if (EOK != oal_spinlock_lock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	HIF */
	pfe_hif_chnl_cfg_set_tx_bd_ring_addr(chnl->cbus_base_va, chnl->id, tx_ring_pa);

	if (NULL != wb_tbl_pa)
	{
		wb_tbl_len = pfe_hif_ring_get_wb_tbl_len(ring);
		pfe_hif_chnl_cfg_set_tx_wb_table(chnl->cbus_base_va, chnl->id, wb_tbl_pa, wb_tbl_len);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	return EOK;
}

#ifdef PFE_CFG_PFE_SLAVE
/**
 * @brief		Inspect a channel state
 * @details		Function read channel HW registers to detect the current state of it
 * @param[in]	chnl The channel instance
 * @return		EOK Channel is clean, EAGAIN channel has valid set-up for ungraceful
 * 				reset, EINVAL channel is in unrecorevable state
 */
static __attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_inspect_hw_state(pfe_hif_chnl_t *chnl)
{
	uint32_t rx_ring_addr, rx_wb_ring_addr, tx_ring_addr, tx_wb_ring_addr, rx_ring_len, tx_ring_len;

	/* Stop the channel */
	pfe_hif_chnl_cfg_rx_disable(chnl->cbus_base_va, chnl->id);
	pfe_hif_chnl_cfg_tx_disable(chnl->cbus_base_va, chnl->id);
	pfe_hif_chnl_cfg_rx_dma_stop(chnl->cbus_base_va, chnl->id);
	pfe_hif_chnl_cfg_tx_dma_stop(chnl->cbus_base_va, chnl->id);

	/* Detect graceful start (all channel registers are zeroed) */
	rx_ring_addr = pfe_hif_chnl_cfg_get_rx_bd_ring_addr(chnl->cbus_base_va, chnl->id);
	rx_wb_ring_addr = pfe_hif_chnl_cfg_get_rx_wb_table_addr(chnl->cbus_base_va, chnl->id);
	tx_ring_addr = pfe_hif_chnl_cfg_get_tx_bd_ring_addr(chnl->cbus_base_va, chnl->id);
	tx_wb_ring_addr = pfe_hif_chnl_cfg_get_tx_wb_table_addr(chnl->cbus_base_va, chnl->id);
	if (0 == rx_ring_addr && 0 == rx_wb_ring_addr && 0 == tx_ring_addr && 0 == tx_wb_ring_addr)
	{
		NXP_LOG_INFO("HIF%d is in clean state\n", chnl->id);
		return EOK;
	}
	if (0 == rx_ring_addr || 0 == rx_wb_ring_addr || 0 == tx_ring_addr || 0 == tx_wb_ring_addr)
	{
		NXP_LOG_ERROR("HIF%d has incomplete set-up. Ungraceful reset cannot be provided.\n", chnl->id);
		return EINVAL;
	}

	/* Channel state verification continues: verify rings sizes */
	rx_ring_len = pfe_hif_chnl_cfg_get_rx_wb_table_len(chnl->cbus_base_va, chnl->id);
	tx_ring_len = pfe_hif_chnl_cfg_get_tx_wb_table_len(chnl->cbus_base_va, chnl->id);
	if (pfe_hif_ring_get_len(chnl->rx_ring) != rx_ring_len || pfe_hif_ring_get_len(chnl->tx_ring) != tx_ring_len)
	{
		NXP_LOG_ERROR("HIF%d rings sizes differ from default. Ungraceful reset cannot be used\n", chnl->id);
		return EINVAL;
	}

	NXP_LOG_DEBUG("HIF%d has valid set-up: RX: BD p0x%08x WB p0x%08x len %d, TX: BD p0x%08x WB p0x%08x len %d\n", chnl->id, rx_ring_addr, rx_wb_ring_addr, rx_ring_len, tx_ring_addr, tx_wb_ring_addr, tx_ring_len);

	return EAGAIN;
}
#endif /* PFE_CFG_PFE_SLAVE */

/**
 * @brief		Validate a BDR set-up
 * @details		Function reads channel HW registers and compares the set-up of BD rings
 * @param[in]	chnl The channel instance
 * @return		TRUE if new allocated BD rings occupy the same memory regions as were
 * 				left in channel registers
 */
static __attribute__((cold, aligned(64))) bool_t pfe_hif_chnl_validate_bdr_setup(pfe_hif_chnl_t *chnl)
{
	uint32_t rx_ring_addr, rx_wb_ring_addr, tx_ring_addr, tx_wb_ring_addr;

	rx_ring_addr = pfe_hif_chnl_cfg_get_rx_bd_ring_addr(chnl->cbus_base_va, chnl->id);
	rx_wb_ring_addr = pfe_hif_chnl_cfg_get_rx_wb_table_addr(chnl->cbus_base_va, chnl->id);
	tx_ring_addr = pfe_hif_chnl_cfg_get_tx_bd_ring_addr(chnl->cbus_base_va, chnl->id);
	tx_wb_ring_addr = pfe_hif_chnl_cfg_get_tx_wb_table_addr(chnl->cbus_base_va, chnl->id);

	if (rx_ring_addr != (uint32_t)(addr_t)pfe_hif_ring_get_base_pa(chnl->rx_ring))
	{
		NXP_LOG_ERROR("HIF%d ungraceful check: RX BD addr differs\n", chnl->id);
		return FALSE;
	}
	if (rx_wb_ring_addr != (uint32_t)(addr_t)pfe_hif_ring_get_wb_tbl_pa(chnl->rx_ring))
	{
		NXP_LOG_ERROR("HIF%d ungraceful check: RX WB addr differs\n", chnl->id);
		return FALSE;
	}
	if (tx_ring_addr != (uint32_t)(addr_t)pfe_hif_ring_get_base_pa(chnl->tx_ring))
	{
		NXP_LOG_ERROR("HIF%d ungraceful check: TX BD addr differs\n", chnl->id);
		return FALSE;
	}
	if (tx_wb_ring_addr != (uint32_t)(addr_t)pfe_hif_ring_get_wb_tbl_pa(chnl->tx_ring))
	{
		NXP_LOG_ERROR("HIF%d ungraceful check: TX WB addr differs\n", chnl->id);
		return FALSE;
	}
	NXP_LOG_INFO("HIF%d ungraceful check: hw/memory setup passed\n", chnl->id);

	return TRUE;
}

static __attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_find_tx(pfe_hif_chnl_t * chnl)
{
	int ii, ring_len, tx_idx = -1;
	void *buf_pa, *frame;
	errno_t ret = ENOENT;

	ring_len = pfe_hif_ring_get_len(chnl->tx_ring);

	pfe_hif_chnl_tx_enable(chnl);

	for (ii = 0; ii < ring_len; ii++)
	{
		/* Send invalid packet(s) (to be dropped on Class) */
		ret = pfe_hif_chnl_send_dummy_frame(chnl, DUMMY_FRAME_INVALID, &frame);
		if (EOK != ret) {
			NXP_LOG_ERROR("Can't send frame\n");
			goto end;

		}

		/* Wait */
		oal_time_usleep(500U);

		pfe_hif_chnl_free_dummy_frame(frame);

		/* Find valid WB entry. It indicates the current position of TX ring */
		ret = pfe_hif_ring_find_wb_entry(chnl->tx_ring, TRUE, &tx_idx);
		if (EOK == ret)
		{
			NXP_LOG_DEBUG("TX index found for HIF%d: %d\n", chnl->id, tx_idx);
			break;
		}

		/* The index was not here, so invalidate current BD */
		pfe_hif_ring_invalidate_direct(chnl->tx_ring, ii);
	}

	if (ENOENT == ret)
	{
		NXP_LOG_ERROR("TX index not found for HIF%d.\n", chnl->id);
		goto end;
	}

	/* Found index, drain all fed TX buffers */
	ii++;
	while (EOK == pfe_hif_ring_drain_buf(chnl->tx_ring, &buf_pa))
	{
		ii--;
	}
	if (0U != ii)
	{
		NXP_LOG_DEBUG("Undraineed TX frames: %d\n", ii);
	}

	pfe_hif_ring_invalidate_direct(chnl->tx_ring, tx_idx);
	pfe_hif_ring_force_index(chnl->tx_ring, tx_idx + 1);

end:
	pfe_hif_chnl_tx_disable(chnl);

	return ret;

}

static __attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_find_rx(pfe_hif_chnl_t * chnl)
{
	int ii, ring_len, rx_idx = -1;
	void *buf_va = NULL, *buf_pa, *frame;
	errno_t ret = ENOENT;

	ring_len = pfe_hif_ring_get_len(chnl->rx_ring);

	if (FALSE == pfe_hif_chnl_cfg_is_rx_bdp_fifo_empty(chnl->cbus_base_va, chnl->id))
	{
		/* We are not allowed to use BDs cached from previous driver run */
		NXP_LOG_ERROR("Detected invalid state of RX BDP: invalid BDs\n");
		ret = EINVAL;
		goto end;
	}

	/* To fit in RX hunter method we used (which is based on cached BD(s))
	 * provide one BD and let BDP do cache it.
	 * We need two steps to cover the whole buffer, first with even BDs
	 * and if not succeede then switch to odd BDs.
	 */

	/* Alloc one frame buffer */
	buf_va = oal_mm_malloc_contig_aligned_nocache(DUMMY_RX_BUF_LEN, 8U);
	if (NULL == buf_va)
	{
		NXP_LOG_ERROR("Can't get dummy RX buffer\n");
		ret = ENOMEM;
		goto end;
	}

	buf_pa = oal_mm_virt_to_phys_contig(buf_va);
	if (NULL == buf_pa)
	{
		NXP_LOG_ERROR("VA to PA conversion failed");
		ret = ENOMEM;
		goto end;
	}

	for (ii = 0; ii < ring_len; ii++)
	{
		ret = pfe_hif_chnl_supply_rx_buf(chnl, buf_pa, DUMMY_RX_BUF_LEN);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't provide dummy RX buffer\n");
			goto end;
		}
	}

	/* First step: even BDs */
	for (ii = 1U; ii < ring_len; ii += 2U)
	{
		pfe_hif_ring_invalidate_direct(chnl->rx_ring, ii);
	}

	/* Instruct BDP to fetch BD */
	pfe_hif_chnl_rx_enable(chnl);
	pfe_hif_chnl_rx_dma_start(chnl);

	oal_time_usleep(500U);

	pfe_hif_chnl_rx_disable(chnl);

	if (TRUE == pfe_hif_chnl_cfg_is_rx_bdp_fifo_empty(chnl->cbus_base_va, chnl->id))
	{
		/* Second step: odd BDs (in case when even BDs try was not sucessful */
		pfe_hif_ring_force_index(chnl->rx_ring, 0);

		for (ii = 0U; ii < ring_len; ii += 2U)
		{
			/* Invalidate odd BD */
			pfe_hif_ring_invalidate_direct(chnl->rx_ring, ii);
			/* Revalidate back even BD */
			pfe_hif_ring_revalidate_direct(chnl->rx_ring, ii + 1U);
		}

		/* Instruct BDP to fetch BD */
		pfe_hif_chnl_rx_enable(chnl);
		pfe_hif_chnl_rx_dma_start(chnl);

		oal_time_usleep(500U);

		pfe_hif_chnl_rx_disable(chnl);

		if (TRUE == pfe_hif_chnl_cfg_is_rx_bdp_fifo_empty(chnl->cbus_base_va, chnl->id))
		{
			/* Something got wrong */
			NXP_LOG_ERROR("HIF%d is not able to cache BD\n", chnl->id);
			ret = EINVAL;
			goto end;
		}
	}

	pfe_hif_ring_force_index(chnl->rx_ring, 0);
	/* Clean ring */
	for (ii = 0U; ii < ring_len; ii++)
	{
		pfe_hif_ring_invalidate_direct(chnl->rx_ring, ii);
	}

	/* Requires clean RX ring, with default WB BD control words (value: 0x200) */

	/* Start RX DMA */
	pfe_hif_chnl_tx_enable(chnl);
	pfe_hif_chnl_rx_enable(chnl);

	/* Empty cached BDs first */
	while(FALSE == pfe_hif_chnl_cfg_is_rx_bdp_fifo_empty(chnl->cbus_base_va, chnl->id))
	{
		/* Send dummy packet to self HIF channel */
		ret = pfe_hif_chnl_send_dummy_frame(chnl, DUMMY_FRAME_IHC_SELF, &frame);
		if (EOK != ret)
			goto end;

		/* Wait */
		oal_time_usleep(500U);

		/* Do TX confirmations */
		ret = pfe_hif_chnl_get_tx_conf(chnl);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't get TX confirmation\n");
			goto end;
		}
		pfe_hif_chnl_free_dummy_frame(frame);
	}

	/* Find invalid WB entry. It indicates the current (stop) position of RX ring
	 * NOTE: it can also wrap around.
	 */
	if (EOK != pfe_hif_ring_find_wb_entry(chnl->rx_ring, FALSE, &rx_idx))
	{
		/* Something got wrong */
		NXP_LOG_ERROR("HIF%d is not able to find correct WB entry\n", chnl->id);
		ret = EINVAL;
		goto end;
	}
	NXP_LOG_DEBUG("Found RX index: %d\n", rx_idx);

	pfe_hif_ring_force_index(chnl->rx_ring, rx_idx);

end:
	pfe_hif_chnl_tx_disable(chnl);
	pfe_hif_chnl_rx_disable(chnl);

	if (NULL != buf_va)
	{
		oal_mm_free_contig(buf_va);
	}

	return ret;
}

static __attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_rx_to_head(pfe_hif_chnl_t * chnl)
{
	int ii, ring_len;
	void *buf_va = NULL, *buf_pa, *frame;
	bool_t lifm;
	errno_t ret = EOK;

	/* Alloc one frame buffer */
	buf_va = oal_mm_malloc_contig_aligned_nocache(DUMMY_RX_BUF_LEN, 8U);
	if (NULL == buf_va)
	{
		NXP_LOG_ERROR("Can't get dummy RX buffer\n");
		ret = ENOMEM;
		goto end;
	}

	buf_pa = oal_mm_virt_to_phys_contig(buf_va);
	if (NULL == buf_pa)
	{
		NXP_LOG_ERROR("VA to PA conversion failed");
		ret = ENOMEM;
		goto end;
	}

	ring_len = pfe_hif_ring_get_len(chnl->rx_ring);

	/* Start RX DMA */
	pfe_hif_chnl_tx_enable(chnl);
	pfe_hif_chnl_rx_enable(chnl);

	ii = 0;
	while (FALSE == pfe_hif_ring_is_on_head(chnl->rx_ring) && (ring_len > ii))
	{
		/* Fill one buffer only */
		ret = pfe_hif_chnl_supply_rx_buf(chnl, buf_pa, DUMMY_RX_BUF_LEN);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't provide dummy RX buffer\n");
			goto end;
		}

		/*
		 * practice shows that dsb(st) here insures the required timing in this
		 * particular case: when dma_start() is called immediately after the
		 * next Rx BD is made available
		 */
		wmb();

		pfe_hif_chnl_rx_dma_start(chnl);

		/* Send dummy packet to self HIF channel */
		ret = pfe_hif_chnl_send_dummy_frame(chnl, DUMMY_FRAME_IHC_SELF, &frame);
		if (EOK != ret)
			goto end;

		/* Wait */
		oal_time_usleep(500U);

		/* Do TX confirmations */
		ret = pfe_hif_chnl_get_tx_conf(chnl);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't provide dummy RX buffer\n");
			goto end;
		}
		pfe_hif_chnl_free_dummy_frame(frame);

		/* Do plain RX */
		ret = pfe_hif_ring_dequeue_plain(chnl->rx_ring, &lifm);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't get RX buffer dequeued\n");
			goto end;
		}

		ii++;
	}

	if (ring_len <= ii)
	{
		NXP_LOG_ERROR("Can't reach RX ring head\n");
		ret = EINVAL;
	}
	for (ii = 0; ii < ring_len; ii++)
		pfe_hif_ring_invalidate_direct(chnl->rx_ring, ii);

end:
	pfe_hif_chnl_tx_disable(chnl);
	pfe_hif_chnl_rx_disable(chnl);

	if (NULL != buf_va)
	{
		oal_mm_free_contig(buf_va);
	}

	return ret;
}

static __attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_tx_to_head(pfe_hif_chnl_t * chnl)
{
	int ii, ring_len;
	void *frame;
	errno_t ret = EOK;

	ring_len = pfe_hif_ring_get_len(chnl->tx_ring);

	/* Start TX DMA */
	pfe_hif_chnl_tx_enable(chnl);

	ii = 0;
	while (FALSE == pfe_hif_ring_is_on_head(chnl->tx_ring) && (ring_len > ii))
	{
		/* Send invalid packet (to be dropped on Class) */
		ret = pfe_hif_chnl_send_dummy_frame(chnl, DUMMY_FRAME_INVALID, &frame);
		if (EOK != ret)
			goto end;

		/* Wait */
		oal_time_usleep(500U);

		/* Do TX confirmations */
		ret = pfe_hif_chnl_get_tx_conf(chnl);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't read TX confirmation\n");
			goto end;
		}
		pfe_hif_chnl_free_dummy_frame(frame);

		ii++;
	}

	if (ring_len <= ii)
	{
		NXP_LOG_ERROR("Can't reach TX ring head\n");
		ret = EINVAL;
	}

end:
	pfe_hif_chnl_tx_disable(chnl);

	for (ii = 0; ii < ring_len; ii++)
		pfe_hif_ring_invalidate_direct(chnl->tx_ring, ii);

	return ret;
}

static __attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_ungraceful_reset(pfe_hif_chnl_t * chnl)
{
	errno_t ret;

	/* Find TX index first */
	ret = pfe_hif_chnl_find_tx(chnl);
	if (EOK != ret)
	{
		goto end;
	}
	NXP_LOG_INFO("HIF%d ungraceful reset: stage 1 (tx finder) passed\n", chnl->id);

	/* Next find RX index */
	ret = pfe_hif_chnl_find_rx(chnl);
	if (EOK != ret)
	{
		goto end;
	}
	NXP_LOG_INFO("HIF%d ungraceful reset: stage 2 (rx finder) passed\n", chnl->id);

	/* Move to RX head first */
	ret = pfe_hif_chnl_rx_to_head(chnl);
	if (EOK != ret)
	{
		goto end;
	}
	NXP_LOG_INFO("HIF%d ungraceful reset: stage 3 (rx to head) passed\n", chnl->id);

	/* Next move to TX index */
	ret = pfe_hif_chnl_tx_to_head(chnl);
	if (EOK != ret)
	{
		goto end;
	}
	NXP_LOG_INFO("HIF%d ungraceful reset: stage 4 (tx to head) passed\n", chnl->id);
	NXP_LOG_INFO("HIF%d ungraceful reset: finished\n", chnl->id);

end:
	return ret;
}

/**
 * @brief		Initialize a channel
 * @details		Function prepares the HIF channel according to user-supplied parameters.
 * 				This includes allocation of resources and configuration of the hardware.
 * 				Routine must be called before RX or TX functionality is enabled.
 * @param[in]	chnl The channel instance
 * @return		EOK Success
 */
static __attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_init(pfe_hif_chnl_t *chnl)
{
	errno_t status;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef PFE_CFG_PFE_SLAVE
	/* Slave: Check the HIF channel state */
	status = pfe_hif_chnl_inspect_hw_state(chnl);
	if (EINVAL == status)
	{
		goto free_and_fail;
	}
#else
	status = EOK;
#endif

	if (NULL != chnl->rx_ring)
	{
		/*	TODO: Re-initialize the RX ring with new size */
		NXP_LOG_ERROR("RX ring already initialized\n");
		goto free_and_fail;
	}

	chnl->rx_ring = pfe_hif_ring_create(TRUE, (PFE_HIF_CHNL_NOCPY_ID == chnl->id));
	if (NULL == chnl->rx_ring)
	{
		NXP_LOG_ERROR("Couldn't create RX BD ring\n");
		goto free_and_fail;
	}

	if (NULL != chnl->tx_ring)
	{
		/*	TODO: Re-initialize the TX ring with new size */
		NXP_LOG_WARNING("TX ring already initialized\n");
		goto free_and_fail;
	}

	chnl->tx_ring = pfe_hif_ring_create(FALSE, (PFE_HIF_CHNL_NOCPY_ID == chnl->id));
	if (NULL == chnl->tx_ring)
	{
		NXP_LOG_ERROR("Couldn't create TX BD ring\n");
		goto free_and_fail;
	}

	/* NOTE: until now the new allocated BDRs must not be set to the channel HW */

	if (EAGAIN == status)
	{
		/* Slave only way supporting ungraceful HIF reset */

		if (FALSE == pfe_hif_chnl_validate_bdr_setup(chnl))
		{
			goto free_and_fail;
		}

		/* Process ungraceful HIF reset procedure */
		if (EOK != pfe_hif_chnl_ungraceful_reset(chnl))
		{
			goto free_and_fail;
		}
	}

	/*	Bind TX BD ring to channel */
	if (EOK != pfe_hif_chnl_bind_tx_ring(chnl))
	{
		goto free_and_fail;
	}
	/*	Bind RX BD ring to channel */
	if (EOK != pfe_hif_chnl_bind_rx_ring(chnl))
	{
		goto free_and_fail;
	}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
	/*	Initialize RX buffer pool. Resource protection is embedded. */
	NXP_LOG_INFO("Initializing RX buffer pool. Depth: %d; Buffer Size: %d; Cache Line Size: %d\n",
		     pfe_hif_chnl_get_rx_fifo_depth(chnl), PFE_BUF_SIZE, HAL_CACHE_LINE_SIZE);

	chnl->rx_pool = bpool_create(pfe_hif_chnl_get_rx_fifo_depth(chnl), PFE_BUF_SIZE, HAL_CACHE_LINE_SIZE, BUFFERS_CACHED);
	if (unlikely(NULL == chnl->rx_pool))
	{
		NXP_LOG_ERROR("Could not allocate RX buffer pool\n");
		goto free_and_fail;
	}

	/*	Populate the RX ring */
	pfe_hif_chnl_refill_rx_buffers(chnl);
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

	return EOK;

free_and_fail:
	if (NULL != chnl->tx_ring)
	{
		pfe_hif_ring_destroy(chnl->tx_ring);
		chnl->tx_ring = NULL;
	}

	if (NULL != chnl->rx_ring)
	{
		pfe_hif_ring_destroy(chnl->rx_ring);
		chnl->rx_ring = NULL;
	}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
	if (NULL != chnl->rx_pool)
	{
		bpool_destroy(chnl->rx_pool);
		chnl->rx_pool = NULL;
	}
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

	return EFAULT;
}

/**
 * @brief		Release the TX dummy frame
 * @param[in]	frame The frame content, created by pfe_hif_chnl_send_dummy_frame()
 */
static __attribute__((cold, aligned(64))) void pfe_hif_chnl_free_dummy_frame(void *frame)
{
	oal_mm_free_contig(frame);
}

/**
 * @brief		Send TX frame
 * @param[in]	chnl The channel instance
 * @param[in]	mode The frame type: DUMMY_FRAME_INVALID or DUMMY_FRAME_IHC_SELF
 * @param[out]	frame The frame content, must be freed by pfe_hif_chnl_free_dummy_frame()
 * @return		EOK if success, error code otherwise
 */
static __attribute__((cold, aligned(64))) errno_t pfe_hif_chnl_send_dummy_frame(pfe_hif_chnl_t *chnl, uint32_t mode, void **frame)
{
	void *tx_buf_va = NULL, *tx_buf_pa;
	pfe_ct_hif_tx_hdr_t *tx_hdr;
	errno_t ret = EOK;

	tx_buf_va = oal_mm_malloc_contig_aligned_nocache(sizeof(pfe_ct_hif_tx_hdr_t)+DUMMY_TX_BUF_LEN, 8U);
	if (NULL == tx_buf_va)
	{
		NXP_LOG_ERROR("Can't get dummy TX dummy buffer\n");
		ret = ENOMEM;
		goto err_end;
	}

	tx_buf_pa = oal_mm_virt_to_phys_contig(tx_buf_va);
	if (NULL == tx_buf_pa)
	{
		NXP_LOG_ERROR("TX dummy buffer VA to PA conversion failed");
		ret = ENOMEM;
		goto err_end;
	}

	tx_hdr = (pfe_ct_hif_tx_hdr_t *)tx_buf_va;

	switch (mode)
	{
		case DUMMY_FRAME_INVALID:
			/* send invalid frame */
			tx_hdr->e_phy_ifs = 0;
			tx_hdr->flags = 0;
			break;

		case DUMMY_FRAME_IHC_SELF:
			/* send IHC frame to self channel */
			tx_hdr->e_phy_ifs = oal_htonl(1U << (PFE_PHY_IF_ID_HIF0 + chnl->id));
			tx_hdr->flags = (pfe_ct_hif_tx_flags_t)(HIF_TX_INJECT|HIF_TX_IHC);
			tx_hdr->chid = chnl->id;
			break;

		default:
			break;
	}

	ret = pfe_hif_chnl_tx(chnl, tx_buf_pa, tx_buf_va, sizeof(pfe_ct_hif_tx_hdr_t)+DUMMY_TX_BUF_LEN, TRUE);
	if (EOK == ret)
	{
		goto the_end;
	}

	NXP_LOG_ERROR("Dummy frame TX failed\n");
err_end:
	if (NULL != tx_buf_va)
	{
		oal_mm_free_contig(tx_buf_va);
		tx_buf_va = NULL;
	}

the_end:
	*frame = tx_buf_va;

	return ret;
}

/**
 * @brief		Reset HIF channel FIFOs
 * @details		When HIF channel is stopped, the prefetched BDs remain active
 * 				in internal buffers. To gracefuly stop the channel all residues
 * 				have to be flushed and the rings (both RX and TX, including
 * 				write-back rings) should be moved to the head of rings, as
 * 				it is required by Slave driver on start-up. Not doing
 * 				channel reset ends up in nonfunctional driver, because
 * 				SW and HW rings are out of sync.
 * @param[in]	chnl The channel instance
 * @return		EOK if success, error code otherwise
 */
__attribute__((cold, aligned(64), unused)) errno_t pfe_hif_chnl_reset_fifos(pfe_hif_chnl_t *chnl)
{
	void *rx_buf_va = NULL, *rx_buf_pa, *buf_pa, *frame;
	uint32_t len, ii;
	bool_t lifm;
	errno_t ret = EOK;

	rx_buf_va = oal_mm_malloc_contig_aligned_nocache(DUMMY_RX_BUF_LEN, 8U);
	if (NULL == rx_buf_va)
	{
		NXP_LOG_ERROR("Can't get dummy RX buffer\n");
		ret = ENOMEM;
		goto the_end;
	}

	rx_buf_pa = oal_mm_virt_to_phys_contig(rx_buf_va);
	if (NULL == rx_buf_pa)
	{
		NXP_LOG_ERROR("VA to PA conversion failed");
		ret = ENOMEM;
		goto the_end;
	}

	/*	Activate the channel */
	pfe_hif_chnl_rx_enable(chnl);
	pfe_hif_chnl_tx_enable(chnl);

	/*	Get maximum number of tries */
	ii = pfe_hif_ring_get_len(chnl->rx_ring);

	/*	Try to flush the internal BD FIFO. Send dummy frames to the
		channel until the BDP RX FIFO is empty. */
	while (FALSE == pfe_hif_chnl_cfg_is_rx_bdp_fifo_empty(chnl->cbus_base_va, chnl->id))
	{
		if (0U == pfe_hif_ring_get_fill_level(chnl->rx_ring))
		{
			/*	Provide single RX buffer */
			if (EOK != pfe_hif_chnl_supply_rx_buf(chnl, rx_buf_pa, DUMMY_RX_BUF_LEN))
			{
				NXP_LOG_ERROR("Can't provide dummy RX buffer\n");
			}
			pfe_hif_chnl_rx_dma_start(chnl);
		}

		/*	Send dummy packet to self HIF channel */
		ret = pfe_hif_chnl_send_dummy_frame(chnl, DUMMY_FRAME_IHC_SELF, &frame);
		if (EOK != ret)
			goto the_end;

		/*	Wait */
		oal_time_usleep(500U);

		/*	Do TX confirmations */
		while (EOK != pfe_hif_chnl_get_tx_conf(chnl))
		{
			oal_time_usleep(100U);
		}
		pfe_hif_chnl_free_dummy_frame(frame);

		/*	Do plain RX */
		while (EOK == pfe_hif_ring_dequeue_buf(chnl->rx_ring, &buf_pa, &len, &lifm))
		{
			;
		}

		/*	Decrement timeout counter */
		if (ii > 0U)
		{
			ii--;
		}
		else
		{
			NXP_LOG_ERROR("RX BD ring flush timed-out\n");
			ret = ETIMEDOUT;
			goto the_end;
		}
	}

	/* RX ring reset to the head */
	ii = pfe_hif_ring_get_len(chnl->rx_ring) - 1;
	while (FALSE == pfe_hif_ring_is_on_head(chnl->rx_ring))
	{
		/*	Provide single RX buffer */
		if (EOK != pfe_hif_chnl_supply_rx_buf(chnl, rx_buf_pa, DUMMY_RX_BUF_LEN))
		{
			NXP_LOG_ERROR("Can't provide dummy RX buffer\n");
		}
		pfe_hif_chnl_rx_dma_start(chnl);

		/*	Send dummy packet to self HIF channel */
		ret = pfe_hif_chnl_send_dummy_frame(chnl, DUMMY_FRAME_IHC_SELF, &frame);
		if (EOK != ret) {
			NXP_LOG_ERROR("Can't send frame\n");
			goto the_end;
		}

		/*	Wait */
		oal_time_usleep(500U);

		/*	Do TX confirmations */
		while (EOK != pfe_hif_chnl_get_tx_conf(chnl))
		{
			oal_time_usleep(100U);
		}
		pfe_hif_chnl_free_dummy_frame(frame);

		/*	Do plain RX */
		while (EOK == pfe_hif_ring_dequeue_buf(chnl->rx_ring, &buf_pa, &len, &lifm))
		{
			;
		}

		/*	Decrement timeout counter */
		if (ii > 0U)
		{
			ii--;
		}
		else
		{
			NXP_LOG_ERROR("RX jump to head timed-out\n");
			ret = ETIMEDOUT;
			goto the_end;
		}

	}

	/* TX ring reset to the head */
	ii = pfe_hif_ring_get_len(chnl->tx_ring) - 1;
	while (FALSE == pfe_hif_ring_is_on_head(chnl->tx_ring))
	{
		/*	Send invalid packet (to be dropped on Class) */
		ret = pfe_hif_chnl_send_dummy_frame(chnl, DUMMY_FRAME_INVALID, &frame);
		if (EOK != ret) {
			NXP_LOG_ERROR("Can't send frame\n");
			goto the_end;
		}

		/*	Wait */
		oal_time_usleep(500U);

		/*	Do TX confirmations */
		while (EOK != pfe_hif_chnl_get_tx_conf(chnl))
		{
			oal_time_usleep(100U);
		}
		pfe_hif_chnl_free_dummy_frame(frame);
	}

the_end:

	/*	Stop the channel */
	pfe_hif_chnl_rx_disable(chnl);
	pfe_hif_chnl_tx_disable(chnl);

	/*	Drain all in case when flush process has somehow failed */
	while (EOK == pfe_hif_ring_drain_buf(chnl->rx_ring, &buf_pa))
	{
		;
	}

	if (NULL != rx_buf_va)
	{
		oal_mm_free_contig(rx_buf_va);
		rx_buf_va = NULL;
	}

	if (EOK == ret)
	{
		/*	Clear channel registers to signalize graceful shutdown */
		pfe_hif_chnl_cfg_set_rx_bd_ring_addr(chnl->cbus_base_va, chnl->id, (void *)0LU);
		pfe_hif_chnl_cfg_set_rx_wb_table(chnl->cbus_base_va, chnl->id, (void *)0LU, 0U);
		pfe_hif_chnl_cfg_set_tx_bd_ring_addr(chnl->cbus_base_va, chnl->id, (void *)0LU);
		pfe_hif_chnl_cfg_set_tx_wb_table(chnl->cbus_base_va, chnl->id, (void *)0LU, 0U);
	}

	return ret;
}

/**
 * @brief		Destroy HIF channel instance
 * @param[in]	chnl The channel instance
 */
__attribute__((cold, aligned(64))) void pfe_hif_chnl_destroy(pfe_hif_chnl_t *chnl)
{
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
	void *buf_pa, *buf_va;
	uint32_t level;
	uint32_t total, available, used;
	errno_t err;
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

	if (NULL != chnl)
	{
		/*	Disable channel interrupts */
		pfe_hif_chnl_irq_mask(chnl);
		pfe_hif_chnl_rx_irq_mask(chnl);
		pfe_hif_chnl_tx_irq_mask(chnl);

		/*	Disable RX/TX DMA */
		pfe_hif_chnl_rx_disable(chnl);
		pfe_hif_chnl_tx_disable(chnl);

		/*	Uninstall callbacks */
		chnl->rx_cbk.cbk = NULL;
		chnl->tx_cbk.cbk = NULL;
		chnl->rx_tx_cbk.cbk = NULL;
#if (TRUE == PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED)
		chnl->rx_oob_cbk.cbk = NULL;
#endif

		if (NULL != chnl->rx_ring)
		{
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
			/*	Drain RX buffers (the ones enqueued in RX ring) */
			while (EOK == pfe_hif_ring_drain_buf(chnl->rx_ring, &buf_pa))
			{
				/*	HIF buffers are provided by SW pool so return them to SW pool */
				buf_va = bpool_get_va(chnl->rx_pool, buf_pa);
				if (NULL == buf_va)
				{
					NXP_LOG_WARNING("Drained buffer VA is NULL\n");
				}
				else
				{
					/*	Return buffer into pool. Resource protection is embedded. */
					bpool_put(chnl->rx_pool, buf_va);
				}
			}

			/*	Sanity check to verify if the HIF RX ring and the upper SW layers
				have properly returned all RX buffers back to the pool. */
			if (EOK != bpool_get_fill_level(chnl->rx_pool, &level))
			{
				NXP_LOG_ERROR("Can't get buffer pool fill level\n ");
			}

			if (level < (pfe_hif_chnl_get_rx_fifo_depth(chnl)))
			{
				NXP_LOG_WARNING("Some RX buffers not returned to the pool\n");
			}
			else
			{
				NXP_LOG_INFO("All RX buffers returned to the pool\n");
			}

#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

		}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
		if (NULL != chnl->rx_pool)
		{
			/*
				This is not necessary since the whole buffer pool is being released
				below. This is more like sanity check whether all buffers have been
				returned to the pool.

				RX buffer can be either within the free pool or enqueued in the HW
				ring or owned by a HIF client.
			*/

			/*	Sanity check if all clients have returned all RX buffers */
			total = bpool_get_depth(chnl->rx_pool);
			err = bpool_get_fill_level(chnl->rx_pool, &available);
			if (unlikely(EOK != err))
			{
				NXP_LOG_ERROR("Unable to get bpool fill level: %d\n", err);
			}

			used = pfe_hif_ring_get_fill_level(chnl->rx_ring);

			if ((available + used) != total)
			{
				NXP_LOG_WARNING("HIF client(s) still own %d RX buffers\n", total - used - available);
			}
		}
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

		/*	Disable the HIF channel BDP/DMA */
		pfe_hif_chnl_rx_disable(chnl);
		pfe_hif_chnl_tx_disable(chnl);

		/*	Destroy rings */
		if (NULL != chnl->rx_ring)
		{
			pfe_hif_ring_destroy(chnl->rx_ring);
			chnl->rx_ring = NULL;
		}

		if (NULL != chnl->tx_ring)
		{
			if (TRUE != pfe_hif_chnl_cfg_is_tx_bdp_fifo_empty(chnl->cbus_base_va, chnl->id))
			{
				NXP_LOG_WARNING("HIF channel TX FIFO is not empty\n");
			}

			pfe_hif_ring_destroy(chnl->tx_ring);
			chnl->tx_ring = NULL;
		}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
		/*	Destroy the buffer pool */
		if (NULL != chnl->rx_pool)
		{
			bpool_destroy(chnl->rx_pool);
			chnl->rx_pool = NULL;
		}

#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

		if (EOK != oal_spinlock_lock(&chnl->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		/*	Disable and finalize the channel */
		{
			pfe_hif_chnl_cfg_fini(chnl->cbus_base_va, chnl->id);
		}

		if (EOK != oal_spinlock_unlock(&chnl->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}


		if (EOK != oal_spinlock_destroy(&chnl->lock))
		{
			NXP_LOG_WARNING("Could not properly destroy channel mutex\n");
		}

		if (EOK != oal_spinlock_destroy(&chnl->rx_lock))
		{
			NXP_LOG_WARNING("Could not properly destroy channel RX mutex\n");
		}

		oal_mm_free_contig(chnl);
	}
}

/**
 * @brief		Dump of SW client channel rings
 * @details		Dumps particular ring
 * @param[in]	chnl The client channel instance
 * @param[in]	dump_rx True if RX ring has to be dumped
 * @param[in]	dump_tx True if TX ring has to be dumped
 * @param[in]	seq			Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level, number of data written to the buffer
 */

static void pfe_seq_print(void *dev, const char *fmt, ...)
{
	struct seq_file *seq = (struct seq_file*)dev;
	va_list args;

	va_start(args, fmt);
	seq_vprintf(seq, fmt, args);
	va_end(args);
}

__attribute__((cold, aligned(64))) void pfe_hif_chnl_dump_ring(const pfe_hif_chnl_t *chnl, bool_t dump_rx, bool_t dump_tx, struct seq_file *seq, uint8_t verb_level)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (dump_rx)
	{
		pfe_hif_ring_dump(chnl->rx_ring, "RX", seq, pfe_seq_print, verb_level);
	}

	if (dump_tx)
	{
		pfe_hif_ring_dump(chnl->tx_ring, "TX", seq, pfe_seq_print, verb_level);
	}
}

__attribute__((cold, aligned(64))) void pfe_hif_chnl_dump_tx_ring_to_ndev(const pfe_hif_chnl_t *chnl, struct net_device *ndev, void (*ndev_print)(void *ndev, const char *fmt, ...))
{
	pfe_hif_ring_dump(chnl->tx_ring, "TX ndev", ndev, ndev_print, PFE_CFG_VERBOSITY_LEVEL);
}

/**
 * @brief		Get number of transmitted packets (from PFE to HOST)
 * @param[in]	emac The channel instance
 * @return		Number of transmitted packets
 */
uint32_t pfe_hif_chnl_get_tx_cnt(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0xffffffffU;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_hif_chnl_cfg_get_tx_cnt(chnl->cbus_base_va, chnl->id);
}

/**
 * @brief		Get number of received packets (from HOST to PFE)
 * @param[in]	emac The channel instance
 * @return		Number of received packets
 */
uint32_t pfe_hif_chnl_get_rx_cnt(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0xffffffffU;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_hif_chnl_cfg_get_rx_cnt(chnl->cbus_base_va, chnl->id);
}

/**
 * @brief		Return HIF channel runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	chnl 		The channel instance
 * @param[in]	seq			Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level, number of data written to the buffer
 * @return		Number of bytes written to the buffe
 */
__attribute__((cold, aligned(64))) uint32_t pfe_hif_chnl_get_text_statistics(const pfe_hif_chnl_t *chnl, struct seq_file *seq, uint8_t verb_level)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	HIF */
	pfe_hif_chnl_cfg_get_text_stat(chnl->cbus_base_va, chnl->id, seq, verb_level);

	pfe_hif_chnl_dump_ring(chnl, TRUE, TRUE, seq, verb_level);

	return 0;
}

/**
 * @brief       Check if TX ring in  HIF channel contains less than watermark-specified
 *              number of free entries
 * @param[in]   chnl        The channel instance
 * @return      TRUE if ring contains less than watermark-specified number
 *              of free entries
 */
bool_t pfe_hif_chnl_tx_fifo_is_below_wm(const pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if (unlikely(NULL == chnl))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        return FALSE;
    }
#endif /* PFE_CFG_NULL_ARG_CHECK */

    return pfe_hif_ring_is_below_wm(chnl->tx_ring);
}
