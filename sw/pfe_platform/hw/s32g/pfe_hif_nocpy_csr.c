/* =========================================================================
 *  Copyright 2018-2019 NXP
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
 * @addtogroup  dxgr_PFE_HIF_NOCPY
 * @{
 *
 * @file		pfe_hif_nocpy_csr.h
 * @brief		The HIF_NOCOPY module registers definition file (s32g).
 * @details		This is the HW-specific part of the HIF_NOCOPY module.
 *
 */

#include "oal.h"
#include "hal.h"
#include "pfe_mmap.h"
#include "pfe_cbus.h"
#include "pfe_platform_cfg.h"
#include "pfe_hif_csr.h" /* Common bit-field definitions */
#include "pfe_hif_nocpy_csr.h"
#include "pfe_bmu_csr.h"
#include "pfe_tmu_csr.h"

/**
 * @brief	Control the buffer descriptor fetch
 * @details	When TRUE then HIF is fetching the same BD until it is valid BD. If FALSE
 * 			then user must trigger the HIF to fetch next BD explicitly by
 * 			pfe_hif_nocpy_rx_dma_start() and pfe_hif_nocpy_tx_dma_start().
 */
#define	PFE_HIF_NOCPY_CFG_USE_BD_POLLING	FALSE

typedef struct
{
	pfe_hif_chnl_cbk_t cbk;
	void *arg;
} pfe_hif_nocpy_cfg_cbk_t;

/*	Storage for RX callbacks per channel */
static pfe_hif_nocpy_cfg_cbk_t nocpy_rx_cbk = {NULL, NULL};
/*	Storage for TX callbacks per channel */
static pfe_hif_nocpy_cfg_cbk_t nocpy_tx_cbk = {NULL, NULL};

/**
 * @brief		HIF NOCPY ISR
 * @details		Handles all HIF NOCPY interrupts
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Make sure the call is protected by some per-HIF mutex
 */
errno_t pfe_hif_nocpy_cfg_isr(void *base_va)
{
	uint32_t reg_src, reg_en;
	errno_t ret = ENOENT;

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + HIF_NOCPY_INT_EN);
	/*	Disable ALL */
	hal_write32(0U, base_va + HIF_NOCPY_INT_EN);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + HIF_NOCPY_INT_SRC);
	/*	ACK triggered */
	hal_write32(reg_src, base_va + HIF_NOCPY_INT_SRC);
	/*	Enable the non-triggered ones */
	hal_write32((reg_en & ~reg_src), base_va + HIF_NOCPY_INT_EN);

	/*	Process interrupts which are triggered AND enabled */
	if (reg_src & reg_en & (BDP_CSR_RX_PKT_INT|BDP_CSR_RX_CBD_INT))
	{
		if (likely(NULL != nocpy_rx_cbk.cbk))
		{
			/*	Call user's callback */
			nocpy_rx_cbk.cbk(nocpy_rx_cbk.arg);
		}
		else
		{
			NXP_LOG_INFO("BDP_CSR_RX_PKT_INT or BDP_CSR_RX_PKT_INT\n");
		}

		ret = EOK;
	}

	/*	Process interrupts which are triggered AND enabled */
	if (reg_src & reg_en & (BDP_CSR_TX_PKT_INT|BDP_CSR_TX_CBD_INT))
	{
		if (likely(NULL != nocpy_tx_cbk.cbk))
		{
			/*	Call user's callback */
			nocpy_tx_cbk.cbk(nocpy_tx_cbk.arg);
		}
		else
		{
			NXP_LOG_INFO("BDP_CSR_TX_PKT_INT or BDP_CSR_TX_CBD_INT\n");
		}

		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Mask HIF NOCPY interrupts
 * @param[in]	base_va Base address of HIF NOCPY register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_irq_mask(void *base_va)
{
	uint32_t reg;

	/*	Disable group */
	reg = hal_read32(base_va + HIF_NOCPY_INT_EN) & ~(HIF_NOCPY_INT);
	hal_write32(reg, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Unmask HIF NOCPY interrupts
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_irq_unmask(void *base_va)
{
	uint32_t reg;

	/*	Enable group */
	reg = hal_read32(base_va + HIF_NOCPY_INT_EN) | HIF_NOCPY_INT;
	hal_write32(reg, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Register custom event callback
 * @param[in]	event Event to trigger the callback
 * @param[in]	cbk The callback
 * @param[in]	arg Argument of the callback
 * @return		EOK if success, error code otherwise
 * @note		Make sure the call is protected by some per-HIF mutex
 */
errno_t pfe_hif_nocpy_cfg_set_cbk(pfe_hif_chnl_event_t event, pfe_hif_chnl_cbk_t cbk, void *arg)
{
	switch (event)
	{
		case HIF_CHNL_EVT_RX_IRQ:
		{
			nocpy_rx_cbk.arg = arg;
			hal_wmb();
			nocpy_rx_cbk.cbk = cbk;
			break;
		}

		case HIF_CHNL_EVT_TX_IRQ:
		{
			nocpy_tx_cbk.arg = arg;
			hal_wmb();
			nocpy_tx_cbk.cbk = cbk;
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Given event not supported: 0x%x\n", event);
			return EINVAL;
		}
	}

	return EOK;
}

/**
 * @brief		Configure and initialize the HIF_NOCPY
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @return		EOK if success, error code otherwise
 * @note		Make sure the call is protected by some per-HIF mutex
 */
errno_t pfe_hif_nocpy_cfg_init(void *base_va)
{
	/*	Disable channel interrupts */
	hal_write32(0U, base_va + HIF_NOCPY_INT_EN);
	hal_write32(0xffffffffU, base_va + HIF_NOCPY_INT_SRC);

	/*	Disable RX/TX DMA */
	pfe_hif_nocpy_cfg_rx_disable(base_va);
	pfe_hif_nocpy_cfg_tx_disable(base_va);

	/*	Set TX port number. TODO: Is this the value the classifier will receive when
	 	packet is transmitted from host via HIFNOCPY? */
	hal_write32(PFE_PHY_IF_ID_HIF_NOCPY, base_va + HIF_NOCPY_TX_PORT_NO);

	/*	HIF_NOCPY_LMEM_ALLOC_ADDR */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_ALLOC_CTRL, base_va + HIF_NOCPY_LMEM_ALLOC_ADDR);

	/*	HIF_NOCPY_CLASS_ADDR */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CLASS_INQ_PKTPTR, base_va + HIF_NOCPY_CLASS_ADDR);

	/*	HIF_NOCPY_TMU_PORT0_ADDR */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + TMU_PHY_INQ_PKTPTR, base_va + HIF_NOCPY_TMU_PORT0_ADDR);

	/*	Set poll counter cycles */
	hal_write32((HIF_RX_POLL_CTRL_CYCLE << 16) | HIF_TX_POLL_CTRL_CYCLE, base_va + HIF_NOCPY_POLL_CTRL);

#if (TRUE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	/*	Enable RX and TX BD fetch polling */
	hal_write32(HIF_CTRL_BDP_POLL_CTRL_EN, base_va + HIF_NOCPY_RX_CTRL);
	hal_write32(HIF_CTRL_BDP_POLL_CTRL_EN, base_va + HIF_NOCPY_TX_CTRL);
#endif /* PFE_HIF_NOCPY_CFG_USE_BD_POLLING */

	/*	Enable channel status interrupts except of the RX/TX and the global enable bit */
	hal_write32(0xffffffffU
				& ~HIF_NOCPY_INT
				& ~BDP_CSR_RX_CBD_INT
				& ~BDP_CSR_RX_PKT_INT
				& ~BDP_CSR_TX_CBD_INT
				& ~BDP_CSR_TX_PKT_INT
				, base_va + HIF_NOCPY_INT_EN);

	return EOK;
}

/**
 * @brief		Finalize the HIF NOCPY
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_fini(void *base_va)
{
	/*	Disable HIF NOCPY interrupts */
	hal_write32(0U, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Enable TX
 * @details		This call shall ensure that the TX DMA is active and the TX
 * 				ring accepts entries.
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_tx_enable(void *base_va)
{
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_TX_CTRL);
	regval |= HIF_CTRL_DMA_EN;
	hal_write32(regval, base_va + HIF_NOCPY_TX_CTRL);

	/*	Enable TX interrupt */
	pfe_hif_nocpy_cfg_tx_irq_unmask(base_va);

#if (FALSE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	/*	Trigger the BDP to fetch first descriptor */
	pfe_hif_nocpy_cfg_tx_dma_start(base_va);
#endif /* PFE_HIF_CFG_USE_BD_POLLING */
}

/**
 * @brief		Disable TX
 * @brief		Disable the TX ring DMA. TX ring interrupt is disabled.
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_tx_disable(void *base_va)
{
	hal_write32(0U, base_va + HIF_NOCPY_TX_CTRL);

	/*	Disable TX interrupt */
	pfe_hif_nocpy_cfg_tx_irq_mask(base_va);
}

/**
 * @brief		Enable RX
 * @details		Enable and start RX DMA. RX ring is being filled with data from
 * 				NPU and RX interrupts generated by the RX ring are enabled.
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_rx_enable(void *base_va)
{
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_RX_CTRL);
	regval |= HIF_CTRL_DMA_EN;
	hal_write32(regval, base_va + HIF_NOCPY_RX_CTRL);

	/*	Enable RX interrupt */
	pfe_hif_nocpy_cfg_rx_irq_unmask(base_va);

#if (FALSE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	/*	Trigger the BDP to fetch first descriptor */
	pfe_hif_nocpy_cfg_rx_dma_start(base_va);
#endif /* PFE_HIF_CFG_USE_BD_POLLING */
}

/**
 * @brief		Disable RX
 * @details		Stop RX DMA. No data traffic from NPU to host will be possible after this call.
 * 				No interrupts from RX ring are enabled.
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_rx_disable(void *base_va)
{
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_RX_CTRL);
	regval &= ~((uint32_t)HIF_CTRL_DMA_EN);
	hal_write32(0U, base_va + HIF_NOCPY_RX_CTRL);

	/*	Disable RX interrupt */
	pfe_hif_nocpy_cfg_rx_irq_mask(base_va);
}

/**
 * @brief		Trigger RX DMA
 * @details		This shall be called once new entry has been written to the RX ring.
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_rx_dma_start(void *base_va)
{
#if (FALSE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_RX_CTRL);
	regval |= HIF_CTRL_BDP_CH_START_WSTB;
	hal_write32(regval, base_va + HIF_NOCPY_RX_CTRL);
#endif /* PFE_HIF_NOCPY_CFG_USE_BD_POLLING */
}

/**
 * @brief		Trigger TX DMA
 * @details		This shall be called once new entry has been written to the TX ring.
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_tx_dma_start(void *base_va)
{
#if (FALSE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_TX_CTRL);
	regval |= HIF_CTRL_BDP_CH_START_WSTB;
	hal_write32(regval, base_va + HIF_NOCPY_TX_CTRL);
#endif /* PFE_HIF_NOCPY_CFG_USE_BD_POLLING */
}

/**
 * @brief		Mask RX IRQ
 * @param[in]	base_va Base address of HIF NOCPY register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_rx_irq_mask(void *base_va)
{
	uint32_t reg;

	/*	Disable RX IRQ */
	reg = hal_read32(base_va + HIF_NOCPY_INT_EN);
	hal_write32(reg
				& ~BDP_CSR_RX_CBD_INT
				& ~BDP_CSR_RX_PKT_INT
				, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Unmask RX IRQ
 * @param[in]	base_va Base address of HIF NOCPY register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_rx_irq_unmask(void *base_va)
{
	uint32_t reg;

	/*	Enable RX IRQ */
	reg = hal_read32(base_va + HIF_NOCPY_INT_EN);
	hal_write32(reg
			| BDP_CSR_RX_CBD_INT
			| BDP_CSR_RX_PKT_INT
				, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Mask TX IRQ
 * @param[in]	base_va Base address of HIF NOCPY register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_tx_irq_mask(void *base_va)
{
	uint32_t reg;

	/*	Disable TX IRQ */
	reg = hal_read32(base_va + HIF_NOCPY_INT_EN);
	hal_write32(reg
				& ~BDP_CSR_TX_CBD_INT
				& ~BDP_CSR_TX_PKT_INT
				, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Unmask TX IRQ
 * @param[in]	base_va Base address of HIF NOCPY register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_tx_irq_unmask(void *base_va)
{
	uint32_t reg;

	/*	Enable TX IRQ */
	reg = hal_read32(base_va + HIF_NOCPY_INT_EN);
	hal_write32(reg
			| BDP_CSR_TX_CBD_INT
			| BDP_CSR_TX_PKT_INT
				, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Set RX buffer descriptor ring address
 * @details		Configure RX buffer descriptor ring address of the channel.
 * 				This binds channel with a RX BD ring.
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	rx_ring_pa The RX ring address (physical, as seen by host)
 */
void pfe_hif_nocpy_cfg_set_rx_bd_ring_addr(void *base_va, void *rx_ring_pa)
{
	hal_write32((uint32_t)PFE_MMAP_DDR_PHYS_TO_PFE((addr_t)rx_ring_pa & 0xffffffffU), base_va + HIF_NOCPY_RX_BDP_ADDR);
}

/**
 * @brief		Set TX buffer descriptor ring address
 * @details		Configure TX buffer descriptor ring address of the channel.
 * 				This binds channel with a TX BD ring.
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	tx_ring_pa The TX ring address (physical, as seen by host)
 */
void pfe_hif_nocpy_cfg_set_tx_bd_ring_addr(void *base_va, void *tx_ring_pa)
{
	hal_write32((uint32_t)PFE_MMAP_DDR_PHYS_TO_PFE((addr_t)tx_ring_pa), base_va + HIF_NOCPY_TX_BDP_ADDR);
}

/**
 * @brief		Get RX ring state
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @return		TRUE when the RX ring BD processor is active or FALSE when it is idle
 */
bool_t pfe_hif_nocpy_cfg_is_rx_dma_active(void *base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + HIF_NOCPY_RX_STATUS);

	if (0U != (reg & (0xfU << 18)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Get TX ring state
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @return		TRUE when the TX ring BD processor is active or FALSE when it is idle
 */
bool_t pfe_hif_nocpy_cfg_is_tx_dma_active(void *base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + HIF_NOCPY_TX_STATUS);

	if (0U != (reg & (0xfU << 18)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Get HIF_NOCOPY channel statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about a HIF_NOCPY "channel".
 * @param[in]	base_va 	Base address of HIF register space (virtual)
 * @param[in]	buf 		Pointer to buffer to be written
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level number of data written to the buffer (0:less 1:more)
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_hif_nocpy_chnl_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	/*	Fill the buffer with runtime data */
    (void)base_va;
    (void)buf;
    (void)size;
    (void)verb_level;
	return 0U;
}

/**
 * @brief		Get HIF statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the HIF block.
 * @param[in]	base_va 	Base address of HIF register space (virtual)
 * @param[in]	buf 		Pointer to buffer to be written
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_hif_nocpy_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Get version */
	if(verb_level >= 9U)
	{
		reg = hal_read32(base_va + HIF_NOCPY_VERSION);
		len += oal_util_snprintf(buf + len, size - len, "Revision             : 0x%x\n", (reg >> 24) & 0xff);
		len += oal_util_snprintf(buf + len, size - len, "Version              : 0x%x\n", (reg >> 16) & 0xff);
		len += oal_util_snprintf(buf + len, size - len, "ID                   : 0x%x\n", reg & 0xffff);
	}

	len += oal_util_snprintf(buf + len, size - len, "TX Current BD Addr   : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_TX_CURR_BD_ADDR));
	len += oal_util_snprintf(buf + len, size - len, "TX Status            : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_TX_STATUS));
	len += oal_util_snprintf(buf + len, size - len, "TX DMA Status        : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_TX_DMA_STATUS));
	len += oal_util_snprintf(buf + len, size - len, "TX Ctrl              : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_TX_CTRL));
	len += oal_util_snprintf(buf + len, size - len, "RX Current BD Addr   : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_RX_CURR_BD_ADDR));
	len += oal_util_snprintf(buf + len, size - len, "RX Status            : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_RX_STATUS));
	len += oal_util_snprintf(buf + len, size - len, "RX DMA Status        : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_RX_DMA_STATUS));
	len += oal_util_snprintf(buf + len, size - len, "RX Ctrl              : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_RX_CTRL));

	return len;
}

/** @}*/

