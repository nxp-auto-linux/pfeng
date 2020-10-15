/* =========================================================================
 *  Copyright 2018-2020 NXP
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

#include "pfe_cbus.h"
#include "pfe_hif_chnl.h"

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)

#include "bpool.h"

#define PFE_BUF_SIZE			2048U

#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

#define DUMMY_TX_BUF_LEN		64U
#define DUMMY_RX_BUF_LEN		2048U

#if defined(PFE_CFG_TARGET_OS_AUTOSAR)
	#define BUFFERS_CACHED FALSE
#else
	#define BUFFERS_CACHED TRUE
#endif

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
	void *cbus_base_va;				/*	CBUS base virtual address */
	uint32_t id;					/*	Channel ID within HIF (0, 1, 2, ...) */
	pfe_hif_ring_t *rx_ring;		/*	The RX ring instance */
	pfe_hif_ring_t *tx_ring;		/*	The TX ring instance */
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
	bpool_t *rx_pool;				/*	Pool of available RX buffers */
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	pfe_bmu_t *bmu;					/*	Associated BMU instance */
	void *tx_ibuf_va;				/*	Intermediate TX buffer VA */
	uint16_t tx_ibuf_len;			/*	Number of bytes in the ibuf */
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */
	oal_spinlock_t lock __attribute__((aligned(HAL_CACHE_LINE_SIZE)));				/*	Channel HW resources protection */
	oal_spinlock_t rx_lock __attribute__((aligned(HAL_CACHE_LINE_SIZE)));			/*	RX resource protection */
	pfe_hif_chnl_cbk_storage_t rx_cbk;		/*	RX callback */
	pfe_hif_chnl_cbk_storage_t tx_cbk;		/*	TX callback */
#if (TRUE == PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED)
	pfe_hif_chnl_cbk_storage_t rx_oob_cbk;	/*	RX Out-Of-Buffers callback */
#endif
};

static errno_t pfe_hif_chnl_set_rx_ring(pfe_hif_chnl_t *chnl, pfe_hif_ring_t *ring) __attribute__((cold));
static errno_t pfe_hif_chnl_set_tx_ring(pfe_hif_chnl_t *chnl, pfe_hif_ring_t *ring) __attribute__((cold));
static errno_t pfe_hif_chnl_init(pfe_hif_chnl_t *chnl) __attribute__((cold));
static errno_t pfe_hif_chnl_flush_rx_bd_fifo(pfe_hif_chnl_t *chnl) __attribute__((cold));

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
static void pfe_hif_chnl_refill_rx_buffers(pfe_hif_chnl_t *chnl) __attribute__((hot));
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
/**
 * @brief	Commited byte count (TX)
 * @details	Number of bytes commited for transmission. Sum of bytes enqueued to TX rings
 *			and waiting for transmission over all buffer descriptors.
 */
static uint32_t pfe_hif_tx_cbc = 0U;
static oal_spinlock_t cbc_lock;
static uint32_t cbc_lock_initialized = 0U;
#if ((PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14) \
	|| (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14a))
	#define PFE_HIF_TX_FIFO_SIZE	(1024U * 6U * 8U)
#else
	#error Please define HIF TX FIFO size
#endif /* PFE_CFG_IP_VERSION */
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	Run the low-level ISR to identify and process the interrupt */
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		ret = pfe_hif_nocpy_cfg_isr(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		ret = pfe_hif_chnl_cfg_isr(chnl->cbus_base_va, chnl->id, &events);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	/*	Run callbacks for identified interrupts here */
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_irq_mask(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		pfe_hif_chnl_cfg_irq_mask(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_irq_unmask(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		pfe_hif_chnl_cfg_irq_unmask(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}
}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
/**
 * @brief	Supply fresh RX buffers to the channel
 * @details	Function populates channel's RX resource with buffer from internal pool
 */
__attribute__((hot)) static void pfe_hif_chnl_refill_rx_buffers(pfe_hif_chnl_t *chnl)
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
__attribute__((cold)) pfe_hif_chnl_t *pfe_hif_chnl_create(void *cbus_base_va, uint32_t id, pfe_bmu_t *bmu)
{
	pfe_hif_chnl_t *chnl;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if !defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		NXP_LOG_ERROR("HIF NOCPY support is not enabled\n");
		return NULL;
	}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

	chnl = oal_mm_malloc_contig_aligned_cache(sizeof(pfe_hif_chnl_t), HAL_CACHE_LINE_SIZE);

	if (NULL == chnl)
	{
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
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		chnl->bmu = bmu;
		chnl->tx_ibuf_va = NULL;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
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
			if (EOK != oal_spinlock_destroy(&chnl->lock))
			{
				NXP_LOG_WARNING("Could not properly destroy mutex\n");
			}
			oal_mm_free_contig(chnl);
			return NULL;
		}

#if defined(PFE_CFG_HIF_TX_FIFO_FIX)
		if (0U == cbc_lock_initialized)
		{
			if (EOK != oal_spinlock_init(&cbc_lock))
			{
				NXP_LOG_ERROR("CBC lock initialization failed\n");
				(void)oal_spinlock_destroy(&chnl->lock);
				(void)oal_spinlock_destroy(&chnl->rx_lock);
				oal_mm_free_contig(chnl);
				return NULL;
			}
		}

		cbc_lock_initialized++;
#endif /* */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
		{
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
			if (NULL == chnl->bmu)
			{
				NXP_LOG_ERROR("HIF NOCPY channel requires BMU instance\n");
				goto free_and_fail;
			}
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */
			/*	HIF_NOCPY does not need per-channel initialization */
			;
		}
		else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
		{
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
			if (NULL != bmu)
			{
				/*	This is not supported. SW buffer pool will be used instead. */
				NXP_LOG_WARNING("BMU-based RX buffer pool not supported for standard HIF channels. SW pool will be used instead.\n");
			}
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

			if (EOK != oal_spinlock_lock(&chnl->lock))
			{
				NXP_LOG_DEBUG("Mutex lock failed\n");
			}

			ret = pfe_hif_chnl_cfg_init(chnl->cbus_base_va, id);

			if (EOK != oal_spinlock_unlock(&chnl->lock))
			{
				NXP_LOG_DEBUG("Mutex unlock failed\n");
			}

			if (EOK != ret)
			{
				NXP_LOG_ERROR("HIF channel init failed\n");
				goto free_and_fail;
			}
		}

		(void) pfe_hif_chnl_init(chnl);
	}

	return chnl;

free_and_fail:
	(void)oal_spinlock_destroy(&chnl->lock);
	(void)oal_spinlock_destroy(&chnl->rx_lock);
#if defined(PFE_CFG_HIF_TX_FIFO_FIX)
	cbc_lock_initialized--;
	if (0U == cbc_lock_initialized)
	{
		(void)oal_spinlock_destroy(&cbc_lock);
	}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

	oal_mm_free_contig(chnl);
	return NULL;
}

/**
 * @brief		Get channel identifier
 * @param[in]	chnl The channel instance
 * @return		The identifier of the channel
 */
__attribute__((pure, cold)) uint32_t pfe_hif_chnl_get_id(pfe_hif_chnl_t *chnl)
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
__attribute__((cold)) errno_t pfe_hif_chnl_tx_enable(pfe_hif_chnl_t *chnl)
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_tx_enable(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		pfe_hif_chnl_cfg_tx_enable(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
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
__attribute__((cold)) void pfe_hif_chnl_tx_disable(pfe_hif_chnl_t *chnl)
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	Stop data transmission */
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_tx_disable(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		pfe_hif_chnl_cfg_tx_disable(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
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
__attribute__((cold)) errno_t pfe_hif_chnl_rx_enable(pfe_hif_chnl_t *chnl)
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_rx_enable(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		pfe_hif_chnl_cfg_rx_enable(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Disable RX
 * @details		De-activate the RX ring
 * @param[in]	chnl The channel instance
 * @note		Must not be preempted by pfe_hif_chnl_supply_rx_buf()
 */
__attribute__((cold)) void pfe_hif_chnl_rx_disable(pfe_hif_chnl_t *chnl)
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	Stop data reception */
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_rx_disable(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		pfe_hif_chnl_cfg_rx_disable(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}
}

/**
 * @brief		Trigger RX DMA
 * @details		One can trigger the HW to start processing of the RX ring.
 * 				This is needed when the RX ring is modified after the
 * 				pfe_hif_chnl_supply_rx_buf() call(s).
 * @param[in]	chnl The channel instance
 */
__attribute__((hot)) void pfe_hif_chnl_rx_dma_start(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	No resource protection here, DMA trigger is atomic. */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_rx_dma_start(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		pfe_hif_chnl_cfg_rx_dma_start(chnl->cbus_base_va, chnl->id);
	}
}

/**
 * @brief		Trigger TX DMA
 * @details		Trigger the HW to start processing of the TX ring. Needed
 * 				after TX ring is modified after the pfe_hif_chnl_tx() call(s).
 * @param[in]	chnl The channel instance
 */
__attribute__((hot)) void pfe_hif_chnl_tx_dma_start(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	No resource protection here. DMA trigger is atomic. */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_tx_dma_start(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		pfe_hif_chnl_cfg_tx_dma_start(chnl->cbus_base_va, chnl->id);
	}
}

/**
 * @brief		Attach event callback
 * @param[in]	chnl The channel instance
 * @param[in]	event Event triggering the handler
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (HIF_CHNL_EVT_TX_IRQ == event)
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
		NXP_LOG_DEBUG("Mutex unlock failed\n");
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_rx_irq_mask(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		pfe_hif_chnl_cfg_rx_irq_mask(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_rx_irq_unmask(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		pfe_hif_chnl_cfg_rx_irq_unmask(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_tx_irq_mask(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		pfe_hif_chnl_cfg_tx_irq_mask(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_tx_irq_unmask(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		pfe_hif_chnl_cfg_tx_irq_unmask(chnl->cbus_base_va, chnl->id);
	}

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}
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
__attribute__((pure, hot)) bool_t pfe_hif_chnl_has_tx_conf(pfe_hif_chnl_t *chnl)
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
__attribute__((pure, hot)) bool_t pfe_hif_chnl_can_accept_rx_buf(pfe_hif_chnl_t *chnl)
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
__attribute__((pure, hot)) bool_t pfe_hif_chnl_can_accept_tx_num(pfe_hif_chnl_t *chnl, uint16_t num)
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

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
/**
 * @brief		Check if channel can accept number of TX bytes
 * @param[in]	chnl The channel instance
 * @param[in]	num Number of bytes to be checked
 * @retval		TRUE HIF channel is able to transmit 'num' number of bytes
 * @retval		FALSE HIF currently can't transmit given number of bytes
 */
__attribute__((hot)) bool_t pfe_hif_chnl_can_accept_tx_data(pfe_hif_chnl_t *chnl, uint32_t num)
{
	uint32_t cur_fill_level;
	bool_t result;

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	There is no data amount limitation */
		return TRUE;
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	Ensure that CBC counter and hw FIFO fill level are consistent */
		if (EOK != oal_spinlock_lock(&cbc_lock))
		{
			NXP_LOG_DEBUG("Spinlock lock failed\n");
		}

		/*	Get current FIFO fill level */
		cur_fill_level  = pfe_hif_cfg_get_tx_fifo_fill_level(chnl->cbus_base_va);

		/*	Check if commited and requested number of bytes fits portion of the HIF FIFO corresponding
			to single channel (total_available_space/number_of_channels). */
		if ((pfe_hif_tx_cbc + num) >= (PFE_HIF_TX_FIFO_SIZE - cur_fill_level))
		{
			/*	Transmission is not allowed */
			result = FALSE;
		}
		else
		{
			result = TRUE;
		}

		if (EOK != oal_spinlock_unlock(&cbc_lock))
		{
			NXP_LOG_DEBUG("Spinlock unlock failed\n");
		}
	}

	return result;
}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

/**
 * @brief		Check if the TX FIFO is empty
 * @param[in]	chnl The channel instance
 * @retval		TRUE TX FIFO is empty
 * @retval		FALSE TX FIFO contains entries waiting for transmission
 */
__attribute__((pure, hot)) bool_t pfe_hif_chnl_tx_fifo_empty(pfe_hif_chnl_t *chnl)
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
__attribute__((pure, cold)) uint32_t pfe_hif_chnl_get_rx_fifo_depth(pfe_hif_chnl_t *chnl)
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
__attribute__((pure, cold)) uint32_t pfe_hif_chnl_get_tx_fifo_depth(pfe_hif_chnl_t *chnl)
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
__attribute__((hot)) errno_t pfe_hif_chnl_tx(pfe_hif_chnl_t *chnl, void *buf_pa, void *buf_va, uint32_t len, bool_t lifm)
{
	errno_t err = EOK;
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	uint32_t u32tmp;
	void *tx_ibuf_pa;
#if defined(PFE_CFG_HIF_NOCPY_DIRECT)
	pfe_ct_hif_tx_hdr_t *hif_tx_hdr;
#endif /* PFE_CFG_HIF_NOCPY_DIRECT */
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

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

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	TODO: What in case when caller is trying to re-transmit a buffer
				  previously obtained via pfe_hif_chnl_rx() (io-pkt does this
				  in case of forwarding use case)???

			- first chunk to send will be the HIF TX header
			- second chunk will be the BMU buffer
			- how to handle that?

			If no special action will be performed, new BMU buffer will be allocated
			and the packet will be formed within it = not optimal approach...
		*/

		if (NULL == chnl->tx_ibuf_va)
		{
			/*	The intermediate buffer has not been allocated yet */
			tx_ibuf_pa = pfe_bmu_alloc_buf(chnl->bmu);
			if (unlikely(NULL == tx_ibuf_pa))
			{
				NXP_LOG_ERROR("BMU can't allocate TX buffer\n");
				return ENOMEM;
			}
			else
			{
				chnl->tx_ibuf_va = pfe_bmu_get_va(chnl->bmu, tx_ibuf_pa);
				chnl->tx_ibuf_len = 0U;
			}
		}

		/*	Copy payload into the intermediate buffer */
		if (unlikely((chnl->tx_ibuf_len + len) > pfe_bmu_get_buf_size(chnl->bmu)))
		{
			NXP_LOG_ERROR("Payload exceeds BMU buffer length\n");

			/*	Drop. Resource protection is embedded. */
			tx_ibuf_pa = pfe_bmu_get_pa(chnl->bmu, chnl->tx_ibuf_va);
			pfe_bmu_free_buf(chnl->bmu, tx_ibuf_pa);
			chnl->tx_ibuf_va = NULL;
			chnl->tx_ibuf_len = 0U;

			return ENOMEM;
		}

		memcpy((void *)((addr_t)chnl->tx_ibuf_va + sizeof(pfe_ct_post_cls_hdr_t) + chnl->tx_ibuf_len), buf_va, len);
		chnl->tx_ibuf_len = chnl->tx_ibuf_len + len;

		if (TRUE == lifm)
		{
			/*	We expect that the caller will prepend the packet data with HIF TX header. Subtract
				therefore the TX header length to get clean packet length as required by HIF NOCPY. */
			chnl->tx_ibuf_len = chnl->tx_ibuf_len - sizeof(pfe_ct_hif_tx_hdr_t);

			/*	Post classification header insertion. Should match the pfe_ct_post_cls_hdr_t (network endian). */
			u32tmp = 0x2020U; /* <-- This is taken from reference code */
			u32tmp = u32tmp | (oal_htons(chnl->tx_ibuf_len) << 16);
			memcpy((chnl->tx_ibuf_va), &u32tmp, sizeof(u32tmp));

			/*	Enqueue the intermediate buffer */
#if defined(PFE_CFG_HIF_NOCPY_DIRECT)
			/*	In this case we must specify egress interface (because it is not enough to have
				the information within TX header and the post-cls header...*/
			hif_tx_hdr = (pfe_ct_hif_tx_hdr_t *)((addr_t)chnl->tx_ibuf_va + sizeof(pfe_ct_post_cls_hdr_t));

			/*	Default is EMAC0 */
			pfe_hif_ring_set_egress_if(chnl->tx_ring, PFE_PHY_IF_ID_EMAC0);

			for (u32tmp=0U; u32tmp<PFE_PHY_IF_ID_MAX; u32tmp++)
			{
				if (0U != (hif_tx_hdr->e_phy_ifs & (1U << u32tmp)))
				{
					pfe_hif_ring_set_egress_if(chnl->tx_ring, u32tmp);
					break;
				}
			}
#endif /* PFE_CFG_HIF_NOCPY_DIRECT */

			tx_ibuf_pa = pfe_bmu_get_pa(chnl->bmu, chnl->tx_ibuf_va);
			err = pfe_hif_ring_enqueue_buf(chnl->tx_ring, tx_ibuf_pa, chnl->tx_ibuf_len, TRUE);

			if (unlikely(EOK != err))
			{
				/*	Drop. Resource protection is embedded. */
				pfe_bmu_free_buf(chnl->bmu, tx_ibuf_pa);
			}

			/*	Reset the intermediate buffer. No release here since it will
			 	(should) be done by the PFE HW. */
			chnl->tx_ibuf_va = NULL;
			chnl->tx_ibuf_len = 0U;
		}
	}
	else
#else
	(void)buf_va;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
		/*	Protect the CBC counter */
		if (EOK != oal_spinlock_lock(&cbc_lock))
		{
			NXP_LOG_DEBUG("Spinlock lock failed\n");
		}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

		err = pfe_hif_ring_enqueue_buf(chnl->tx_ring, buf_pa, len, lifm);

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
		if (EOK == err)
		{
			pfe_hif_tx_cbc += len;
		}

		if (EOK != oal_spinlock_unlock(&cbc_lock))
		{
			NXP_LOG_DEBUG("Spinlock unlock failed\n");
		}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
	}

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
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
	uint32_t len;
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Get all transmitted chunks but only the last-in-frame
		will be reported as TX confirmation. */
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
	/*	Protect the CBC counter */
	if (EOK != oal_spinlock_lock(&cbc_lock))
	{
		NXP_LOG_DEBUG("Spinlock lock failed\n");
	}

	while (EOK == pfe_hif_ring_dequeue_plain(chnl->tx_ring, &lifm, &len))
	{
		pfe_hif_tx_cbc -= len;
#else
	while (EOK == pfe_hif_ring_dequeue_plain(chnl->tx_ring, &lifm))
	{
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
		
		if (TRUE == lifm)
		{
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
		if (EOK != oal_spinlock_unlock(&cbc_lock))
		{
			NXP_LOG_DEBUG("Spinlock unlock failed\n");
		}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
			return EOK;
		}
	}

#ifdef PFE_CFG_HIF_TX_FIFO_FIX
	if (EOK != oal_spinlock_unlock(&cbc_lock))
	{
		NXP_LOG_DEBUG("Spinlock unlock failed\n");
	}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

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
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
		{
			/*	HIF NOCPY */

			/*	Addresses coming from the ring are physical addresses of buffers provided by BMU. The
			 	buffer contains so called post-classification header the PFE classifier is internally
			 	using as well as specific HIF header. Headers start from buffer offset 0x0 and we shall
				strip-off the post-classification header here since upper layers do not know about such
				thing. The space can be (and will be) used as the buffer-specific metadata storage. */
			*buf_va = pfe_bmu_get_va(chnl->bmu, buf_pa);
#if defined(PFE_CFG_NULL_ARG_CHECK)
			if (unlikely(NULL == *buf_va))
			{
				NXP_LOG_DEBUG("Fatal: BMU converted p0x%p to v0x0\n", (uint32_t)(addr_t)buf_pa);
			}
			else
#endif /* PFE_CFG_NULL_ARG_CHECK */

			/*	Get metadata storage (misuse the buffer headers) */
			*meta = *buf_va;

			/*	Skip the post-classification header to gain space for metadata storage.
			 	The pfe_hif_chnl_release_buf() muse be aware of this adjustment before
				it will attempt to release buffer back to BMU hardware pool. This will
				ensure the caller will receive also the HIF TX header but can reuse it
				for custom purposes (see the pfe_hif_chnl_get_meta_size()). */
			*buf_va = (void *)((addr_t)*buf_va + sizeof(pfe_ct_post_cls_hdr_t));

#if (TRUE == HAL_HANDLE_CACHE)
			/*	Invalidate cache over the buffer */
			oal_mm_cache_inval(*buf_va, buf_pa, *len);
#endif /* HAL_HANDLE_CACHE */
		}
		else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
		{
			/*	Return virtual address */
			*buf_va = bpool_get_va(chnl->rx_pool, buf_pa);

#if (TRUE == HAL_HANDLE_CACHE)
			/*	Invalidate cache over the received data area */
			oal_mm_cache_inval(*buf_va, buf_pa, *len);
#endif /* HAL_HANDLE_CACHE */

			/*	Return pointer to the pre-allocated memory location where
				a buffer-related metadata can be stored. */
			*meta = bpool_get_meta_storage(chnl->rx_pool, *buf_va);
		}
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
__attribute__((cold)) uint32_t pfe_hif_chnl_get_meta_size(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF NOCPY */

		/*	In case of HIF NOCPY we're using whole RX packet header headroom
		 	for metadata storage. The headroom includes post-classification
		 	header and the HIF header. Both can be overwritten by custom
		 	data. */
		return sizeof(pfe_ct_post_cls_hdr_t) + sizeof(pfe_ct_hif_rx_hdr_t);
	}
	else
#elif !defined(PFE_CFG_NULL_ARG_CHECK)
	(void)chnl;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		return bpool_get_meta_storage_size();
	}
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

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF NOCPY */

		/*	Get physical address */
		buf_pa = (addr_t)pfe_bmu_get_pa(chnl->bmu, buf_va);

		/*	Apply the correction due to post-classification header skip done
		 	during the buffer reception. */
		buf_pa = buf_pa - sizeof(pfe_ct_post_cls_hdr_t);

		/*	Release the buffer to BMU pool. Resource protection is embedded. */
		pfe_bmu_free_buf(chnl->bmu, (void *)buf_pa);

		ret = EOK;
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		buf_pa = (addr_t)bpool_get_pa(chnl->rx_pool, buf_va);

		if (unlikely(NULL == (void *)buf_pa))
		{
			NXP_LOG_ERROR("VA->PA conversion failed, origin buffer VA: v0x%p\n", buf_va);
		}

#if (TRUE == HAL_HANDLE_CACHE)
	#if (PFE_CFG_IP_VERSION == PFE_CFG_IP_VERSION_NPU_7_14)
		/*	Without this flush the invalidation does not properly work. Recycled buffers
			are not properly invalidated when this line is missing. */
		oal_mm_cache_flush(buf_va, (void *)buf_pa, PFE_BUF_SIZE);
	#endif /* PFE_CFG_IP_VERSION */
#endif /* HAL_HANDLE_CACHE */

		if (unlikely(EOK != oal_spinlock_lock(&chnl->rx_lock)))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Release the buffer to ring */
		ret = pfe_hif_ring_enqueue_buf(chnl->rx_ring, (void *)buf_pa, PFE_BUF_SIZE, TRUE);

		if (unlikely(EOK != oal_spinlock_unlock(&chnl->rx_lock)))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}
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
__attribute__((hot)) errno_t pfe_hif_chnl_supply_rx_buf(pfe_hif_chnl_t *chnl, void *buf_pa, uint32_t size)
{
	errno_t err = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl) || (NULL == buf_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	There is noting to supply to HIF NOCPY */
		err = EINVAL;
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		err = pfe_hif_ring_enqueue_buf(chnl->rx_ring, buf_pa, size, TRUE);
		if (unlikely(EOK != err))
		{
			NXP_LOG_WARNING("pfe_hif_ring_enqueue_buf() failed: %d\n", err);
		}
	}

	return err;
}

/**
 * @brief		Assign RX BD ring
 * @details		Configure RX buffer descriptor ring address of the channel.
 * 				This binds channel with a RX BD ring.
 * @param[in]	chnl The channel instance
 * @param[in]	ring The ring instance
 * @retval		EOK Success
 * @retval		EFAULT Invalid ring instance
 */
__attribute__((cold)) static errno_t pfe_hif_chnl_set_rx_ring(pfe_hif_chnl_t *chnl, pfe_hif_ring_t *ring)
{
	void *rx_ring_pa, *wb_tbl_pa;
	uint32_t wb_tbl_len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl) || (NULL == ring)))
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_set_rx_bd_ring_addr(chnl->cbus_base_va, rx_ring_pa);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		pfe_hif_chnl_cfg_set_rx_bd_ring_addr(chnl->cbus_base_va, chnl->id, rx_ring_pa);

		if (NULL != wb_tbl_pa)
		{
			wb_tbl_len = pfe_hif_ring_get_wb_tbl_len(ring);
			pfe_hif_chnl_cfg_set_rx_wb_table(chnl->cbus_base_va, chnl->id, wb_tbl_pa, wb_tbl_len);
		}
	}

	chnl->rx_ring = ring;

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Assign TX BD ring
 * @details		Configure TX buffer descriptor ring address of the channel.
 * 				This binds channel with a TX BD ring.
 * @param[in]	chnl The channel instance
 * @param[in]	ring The ring instance
 * @retval		EOK Success
 * @retval		EFAULT Invalid ring instance
 */
__attribute__((cold)) static errno_t pfe_hif_chnl_set_tx_ring(pfe_hif_chnl_t *chnl, pfe_hif_ring_t *ring)
{
	void *tx_ring_pa, *wb_tbl_pa;
	uint32_t wb_tbl_len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == chnl) || (NULL == ring)))
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
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		pfe_hif_nocpy_cfg_set_tx_bd_ring_addr(chnl->cbus_base_va, tx_ring_pa);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		pfe_hif_chnl_cfg_set_tx_bd_ring_addr(chnl->cbus_base_va, chnl->id, tx_ring_pa);

		if (NULL != wb_tbl_pa)
		{
			wb_tbl_len = pfe_hif_ring_get_wb_tbl_len(ring);
			pfe_hif_chnl_cfg_set_tx_wb_table(chnl->cbus_base_va, chnl->id, wb_tbl_pa, wb_tbl_len);
		}
	}

	chnl->tx_ring = ring;

	if (EOK != oal_spinlock_unlock(&chnl->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Initialize a channel
 * @details		Function prepares the HIF channel according to user-supplied parameters.
 * 				This includes allocation of resources and configuration of the hardware.
 * 				Routine must be called before RX or TX functionality is enabled.
 * @param[in]	chnl The channel instance
 * @return		EOK Success
 */
static __attribute__((cold)) errno_t pfe_hif_chnl_init(pfe_hif_chnl_t *chnl)
{
	pfe_hif_ring_t *tx_ring, *rx_ring;
	uint16_t seqnum;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if ((chnl->id >= PFE_HIF_CHNL_NOCPY_ID) && (NULL == chnl->bmu))
	{
		NXP_LOG_ERROR("Channel requires BMU instance\n");
		goto free_and_fail;
	}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

	if (NULL != chnl->rx_ring)
	{
		/*	TODO: Re-initialize the RX ring with new size */
		NXP_LOG_ERROR("RX ring already initialized\n");
		goto free_and_fail;
	}

	/*	Get current valid RX ring sequence number */
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
	seqnum = pfe_hif_chnl_cfg_get_rx_seqnum(chnl->cbus_base_va, chnl->id);
	NXP_LOG_DEBUG("Using initial RX ring seqnum 0x%x\n", seqnum);
#else
	seqnum = 0U;
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */
	rx_ring = pfe_hif_ring_create(TRUE, seqnum, (PFE_HIF_CHNL_NOCPY_ID == chnl->id));
	if (NULL == rx_ring)
	{
		NXP_LOG_ERROR("Couldn't create RX BD ring\n");
		goto free_and_fail;
	}
	else
	{
		/*	Bind RX BD ring to channel */
		if (EOK != pfe_hif_chnl_set_rx_ring(chnl, rx_ring))
		{
			goto free_and_fail;
		}
	}

	if (NULL != chnl->tx_ring)
	{
		/*	TODO: Re-initialize the TX ring with new size */
		NXP_LOG_WARNING("TX ring already initialized\n");
		goto free_and_fail;
	}

	/*	Get current valid TX ring sequence number */
#ifdef PFE_CFG_HIF_SEQNUM_CHECK
	seqnum = pfe_hif_chnl_cfg_get_tx_seqnum(chnl->cbus_base_va, chnl->id);
	NXP_LOG_DEBUG("Using initial TX ring seqnum 0x%x\n", seqnum);
#else
	seqnum = 0U;
#endif /* PFE_CFG_HIF_SEQNUM_CHECK */
	tx_ring = pfe_hif_ring_create(FALSE, seqnum, (PFE_HIF_CHNL_NOCPY_ID == chnl->id));
	if (NULL == tx_ring)
	{
		NXP_LOG_ERROR("Couldn't create TX BD ring\n");
		goto free_and_fail;
	}
	else
	{
		/*	Bind TX BD ring to channel */
		if (EOK != pfe_hif_chnl_set_tx_ring(chnl, tx_ring))
		{
			goto free_and_fail;
		}
	}

#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF NOCPY does not need external RX buffers */
		chnl->rx_pool = NULL;
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
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
	}
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
 * @brief		Get the RX BD processor state
 * @param[in]	chnl The channel instance
 * @return		TRUE if the BDP is active, FALSE otherwise
 */
__attribute__((hot)) bool_t pfe_hif_chnl_is_rx_dma_active(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	No protection here. Getting DMA status is atomic. */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		return pfe_hif_nocpy_cfg_is_rx_dma_active(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		return pfe_hif_chnl_cfg_is_rx_dma_active(chnl->cbus_base_va, chnl->id);
	}
}

/**
 * @brief		Get the TX BD processor state
 * @param[in]	chnl The channel instance
 * @return		TRUE if the BDP is active, FALSE otherwise
 */
__attribute__((hot)) bool_t pfe_hif_chnl_is_tx_dma_active(pfe_hif_chnl_t *chnl)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	No protection here. Getting DMA status is atomic. */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		return pfe_hif_nocpy_cfg_is_tx_dma_active(chnl->cbus_base_va);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		return pfe_hif_chnl_cfg_is_tx_dma_active(chnl->cbus_base_va, chnl->id);
	}
}

/**
 * @brief		Flush RX BDP buffer
 * @details		When channel is stopped the fetched BDs are remaining in internal
 * 				buffer and don't get flushed once channel is re-enabled. This
 * 				causes memory corruption when channel driver is stopped and then
 * 				started with other BD rings because HIF is missing possibility
 * 				to reset particular channels separately without affecting the
 * 				other channels.
 * @param[in]	chnl The channel instance
 * @return		EOK if success, error code otherwise
 */
static __attribute__((cold)) errno_t pfe_hif_chnl_flush_rx_bd_fifo(pfe_hif_chnl_t *chnl)
{
	void *tx_buf_va = NULL, *rx_buf_va = NULL;
	void *tx_buf_pa, *rx_buf_pa, *buf_pa;
	pfe_ct_hif_tx_hdr_t *tx_hdr;
	uint32_t len, ii;
	bool_t lifm;
	errno_t ret = EOK;

	tx_buf_va = oal_mm_malloc_contig_aligned_nocache(sizeof(pfe_ct_hif_tx_hdr_t)+DUMMY_TX_BUF_LEN, 8U);
	if (NULL == tx_buf_va)
	{
		NXP_LOG_ERROR("Can't get dummy TX buffer\n");
		ret = ENOMEM;
		goto the_end;
	}

	tx_buf_pa = oal_mm_virt_to_phys_contig(tx_buf_va);
	if (NULL == tx_buf_pa)
	{
		NXP_LOG_ERROR("VA to PA conversion failed");
		ret = ENOMEM;
		goto the_end;
	}

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

	tx_hdr = (pfe_ct_hif_tx_hdr_t *)tx_buf_va;

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		tx_hdr->e_phy_ifs = oal_htonl(1U << PFE_PHY_IF_ID_HIF_NOCPY);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		tx_hdr->e_phy_ifs = oal_htonl(1U << (PFE_PHY_IF_ID_HIF0 + chnl->id));
	}

	tx_hdr->flags = (pfe_ct_hif_tx_flags_t)(HIF_TX_INJECT|HIF_TX_IHC);

	/*	Activate the channel */
	pfe_hif_chnl_rx_enable(chnl);
	pfe_hif_chnl_tx_enable(chnl);

	/*	Get maximum number of tries */
	ii = pfe_hif_ring_get_len(chnl->rx_ring);

	/*	Try to flush the internal BD FIFO. Send dummy frames to the
		channel until the BDP RX FIFO is empty. */
	while (FALSE == pfe_hif_chnl_cfg_is_rx_bdp_fifo_empty(chnl->cbus_base_va, chnl->id))
	{
		/*	Provide single RX buffer */
		if (EOK != pfe_hif_chnl_supply_rx_buf(chnl, rx_buf_pa, DUMMY_RX_BUF_LEN))
		{
			NXP_LOG_ERROR("Can't provide dummy RX buffer\n");
		}

		/*	Send dummy packet to self HIF channel */
		if (EOK != pfe_hif_chnl_tx(chnl, tx_buf_pa, tx_buf_va, sizeof(pfe_ct_hif_tx_hdr_t)+DUMMY_TX_BUF_LEN, TRUE))
		{
			NXP_LOG_ERROR("Dummy frame TX failed\n");
		}

		/*	Wait */
		oal_time_usleep(500U);

		/*	Do TX confirmations */
		while (EOK == pfe_hif_chnl_get_tx_conf(chnl))
		{
			;
		}

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

the_end:
	/*	Drain all in case when flush process has somehow failed */
	while (EOK == pfe_hif_ring_drain_buf(chnl->rx_ring, &buf_pa))
	{
		;
	}

	if (NULL != tx_buf_va)
	{
		oal_mm_free_contig(tx_buf_va);
		tx_buf_va = NULL;
	}

	if (NULL != rx_buf_va)
	{
		oal_mm_free_contig(rx_buf_va);
		rx_buf_va = NULL;
	}

	return ret;
}

/**
 * @brief		Destroy HIF channel instance
 * @param[in]	chnl The channel instance
 */
__attribute__((cold)) void pfe_hif_chnl_destroy(pfe_hif_chnl_t *chnl)
{
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
	void *buf_pa, *buf_va;
	uint32_t level;
	uint32_t total, available, used;
	errno_t err;
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	void *tx_ibuf_pa;
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
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
#if (TRUE == PFE_HIF_CHNL_CFG_RX_OOB_EVENT_ENABLED)
		chnl->rx_oob_cbk.cbk = NULL;
#endif

		if (NULL != chnl->rx_ring)
		{
#if (TRUE == PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED)
			/*	Drain RX buffers (the ones enqueued in RX ring) */
			while (EOK == pfe_hif_ring_drain_buf(chnl->rx_ring, &buf_pa))
			{
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
				if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
				{
					/*	HIF NOCPY buffers are provided by BMU so return them to BMU */
					buf_va = pfe_bmu_get_va(chnl->bmu, buf_pa);
				}
				else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
				{
					/*	HIF buffers are provided by SW pool so return them to SW pool */
					buf_va = bpool_get_va(chnl->rx_pool, buf_pa);
				}

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

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
			if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
			{
				/*	Nothing to do */
				;
			}
			else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
			{
				/*	Sanity check */
				if (EOK != bpool_get_fill_level(chnl->rx_pool, &level))
				{
					NXP_LOG_ERROR("Can't get buffer pool fill level\n ");
				}

				if (level < (pfe_hif_chnl_get_rx_fifo_depth(chnl)))
				{
					NXP_LOG_WARNING("Some buffers not returned to pool\n");
				}
			}

#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

			/*	Invalidate the RX ring */
			pfe_hif_ring_invalidate(chnl->rx_ring);

			/*
			 	Here the ring should be empty. Execute HIF channel BDP shutdown
			 	procedure to ensure that channel will not keep any content in
				internal buffers.
			*/
			if (EOK != pfe_hif_chnl_flush_rx_bd_fifo(chnl))
			{
				NXP_LOG_ERROR("FATAL: Could not flush RX BD FIFO\n");
			}
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

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		/*	Release the intermediate TX buffer */
		if (NULL != chnl->tx_ibuf_va)
		{
			tx_ibuf_pa = pfe_bmu_get_pa(chnl->bmu, chnl->tx_ibuf_va);
			pfe_bmu_free_buf(chnl->bmu, tx_ibuf_pa);
			chnl->tx_ibuf_va = NULL;
		}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
#endif /* PFE_HIF_CHNL_CFG_RX_BUFFERS_ENABLED */

		if (EOK != oal_spinlock_lock(&chnl->lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Disable and finalize the channel */
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
		if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
		{
			; /*	HIF NOCPY will do the finalization */
		}
		else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
		{
			pfe_hif_chnl_cfg_fini(chnl->cbus_base_va, chnl->id);
		}

		if (EOK != oal_spinlock_unlock(&chnl->lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}

		if (EOK != oal_spinlock_destroy(&chnl->lock))
		{
			NXP_LOG_WARNING("Could not properly destroy channel mutex\n");
		}

		if (EOK != oal_spinlock_destroy(&chnl->rx_lock))
		{
			NXP_LOG_WARNING("Could not properly destroy channel RX mutex\n");
		}

#if defined(PFE_CFG_HIF_TX_FIFO_FIX)
		cbc_lock_initialized--;
		if (0U == cbc_lock_initialized)
		{
			(void)oal_spinlock_destroy(&cbc_lock);
		}
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */

		oal_mm_free_contig(chnl);
	}
}

/**
 * @brief		Dump of SW client channel rings
 * @details		Dumps particular ring
 * @param[in]	chnl The client channel instance
 * @param[in]	dump_rx True if RX ring has to be dumped
 * @param[in]	dump_tx True if TX ring has to be dumped
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level, number of data written to the buffer
 */
__attribute__((cold)) uint32_t pfe_hif_chnl_dump_ring(pfe_hif_chnl_t *chnl, bool_t dump_rx, bool_t dump_tx, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if(dump_rx)
	{
		len += pfe_hif_ring_dump(chnl->rx_ring, "RX", buf + len, size - len, verb_level);
	}

	if(dump_tx)
	{
		len += pfe_hif_ring_dump(chnl->tx_ring, "TX", buf + len, size - len, verb_level);
	}

	return len;
}

/**
 * @brief		Return HIF channel runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	chnl 		The channel instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level, number of data written to the buffer
 * @return		Number of bytes written to the buffe
 */
__attribute__((cold)) uint32_t pfe_hif_chnl_get_text_statistics(pfe_hif_chnl_t *chnl, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == chnl))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	if (chnl->id >= PFE_HIF_CHNL_NOCPY_ID)
	{
		/*	HIF_NOCPY */
		len += pfe_hif_nocpy_chnl_cfg_get_text_stat(chnl->cbus_base_va, buf, buf_len, verb_level);
	}
	else
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	{
		/*	HIF */
		len += pfe_hif_chnl_cfg_get_text_stat(chnl->cbus_base_va, chnl->id, buf, buf_len, verb_level);

		len += pfe_hif_chnl_dump_ring(chnl, TRUE, TRUE, buf, buf_len, verb_level);
	}

	return len;
}
