/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_platform_cfg.h"
#include "pfe_hif_csr.h"
#include "pfe_hif_nocpy_csr.h"
#include "pfe_bmu_csr.h"
#include "pfe_tmu_csr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

/**
 * @brief	Control the buffer descriptor fetch
 * @details	When TRUE then HIF is fetching the same BD until it is valid BD. If FALSE
 * 			then user must trigger the HIF to fetch next BD explicitly by
 * 			pfe_hif_nocpy_rx_dma_start() and pfe_hif_nocpy_tx_dma_start().
 */
#define	PFE_HIF_NOCPY_CFG_USE_BD_POLLING	TRUE

/**
 * @brief		HIF NOCPY ISR
 * @details		Handles all HIF NOCPY interrupts
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Make sure the call is protected by some per-HIF mutex
 */
errno_t pfe_hif_nocpy_cfg_isr(addr_t base_va, pfe_hif_chnl_event_t *events)
{
	uint32_t reg_src, reg_en;
	errno_t ret = ENOENT;

	*events = (pfe_hif_chnl_event_t)0;

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
	if ((reg_src & reg_en & (BDP_CSR_RX_PKT_INT|BDP_CSR_RX_CBD_INT)) != 0U)
	{
		*events |= HIF_CHNL_EVT_RX_IRQ;
		ret = EOK;
	}

	/*	Process interrupts which are triggered AND enabled */
	if ((reg_src & reg_en & (BDP_CSR_TX_PKT_INT|BDP_CSR_TX_CBD_INT)) != 0U)
	{
		*events |= HIF_CHNL_EVT_TX_IRQ;
		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Mask HIF NOCPY interrupts
 * @param[in]	base_va Base address of HIF NOCPY register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_irq_mask(addr_t base_va)
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
void pfe_hif_nocpy_cfg_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	/*	Enable group */
	reg = hal_read32(base_va + HIF_NOCPY_INT_EN) | HIF_NOCPY_INT;
	hal_write32(reg, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Configure and initialize the HIF_NOCPY
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @return		EOK if success, error code otherwise
 * @note		Make sure the call is protected by some per-HIF mutex
 */
errno_t pfe_hif_nocpy_cfg_init(addr_t base_va)
{
	/*	Disable channel interrupts */
	hal_write32(0U, base_va + HIF_NOCPY_INT_EN);
	hal_write32(0xffffffffU, base_va + HIF_NOCPY_INT_SRC);

	pfe_hif_nocpy_cfg_rx_disable(base_va);
	pfe_hif_nocpy_cfg_tx_disable(base_va);

	hal_write32(PFE_PHY_IF_ID_HIF_NOCPY, base_va + HIF_NOCPY_TX_PORT_NO);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_ALLOC_CTRL, base_va + HIF_NOCPY_LMEM_ALLOC_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CLASS_INQ_PKTPTR, base_va + HIF_NOCPY_CLASS_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + TMU_PHY_INQ_PKTPTR, base_va + HIF_NOCPY_TMU_PORT0_ADDR);
	hal_write32((HIF_RX_POLL_CTRL_CYCLE << 16U) | HIF_TX_POLL_CTRL_CYCLE, base_va + HIF_NOCPY_POLL_CTRL);

#if (TRUE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	hal_write32(HIF_CTRL_BDP_POLL_CTRL_EN, base_va + HIF_NOCPY_RX_CTRL);
	hal_write32(HIF_CTRL_BDP_POLL_CTRL_EN, base_va + HIF_NOCPY_TX_CTRL);
#endif /* PFE_HIF_NOCPY_CFG_USE_BD_POLLING */

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
void pfe_hif_nocpy_cfg_fini(addr_t base_va)
{
	/*	Disable HIF NOCPY interrupts */
	hal_write32(0U, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Enable TX
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_tx_enable(addr_t base_va)
{
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_TX_CTRL);
	regval |= HIF_CTRL_DMA_EN;
#if (TRUE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	regval |= HIF_CTRL_BDP_POLL_CTRL_EN;
#endif
	hal_write32(regval, base_va + HIF_NOCPY_TX_CTRL);

#if (FALSE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	pfe_hif_nocpy_cfg_tx_dma_start(base_va);
#endif /* PFE_HIF_CFG_USE_BD_POLLING */
}

/**
 * @brief		Disable TX
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_tx_disable(addr_t base_va)
{
	hal_write32(0U, base_va + HIF_NOCPY_TX_CTRL);
	pfe_hif_nocpy_cfg_tx_irq_mask(base_va);
}

/**
 * @brief		Enable RX
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_rx_enable(addr_t base_va)
{
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_RX_CTRL);
	regval |= HIF_CTRL_DMA_EN;
#if (TRUE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	regval |= HIF_CTRL_BDP_POLL_CTRL_EN;
#endif
	hal_write32(regval, base_va + HIF_NOCPY_RX_CTRL);

#if (FALSE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	pfe_hif_nocpy_cfg_rx_dma_start(base_va);
#endif /* PFE_HIF_CFG_USE_BD_POLLING */
}

/**
 * @brief		Disable RX
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_rx_disable(addr_t base_va)
{
	hal_write32(0U, base_va + HIF_NOCPY_RX_CTRL);

	pfe_hif_nocpy_cfg_rx_irq_mask(base_va);
}

/**
 * @brief		Trigger RX DMA
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_rx_dma_start(addr_t base_va)
{
#if (FALSE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_RX_CTRL);
	regval |= HIF_CTRL_BDP_CH_START_WSTB;
	hal_write32(regval, base_va + HIF_NOCPY_RX_CTRL);
#else
    (void)base_va;
#endif /* PFE_HIF_NOCPY_CFG_USE_BD_POLLING */
}

/**
 * @brief		Trigger TX DMA
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_nocpy_cfg_tx_dma_start(addr_t base_va)
{
#if (FALSE == PFE_HIF_NOCPY_CFG_USE_BD_POLLING)
	uint32_t regval;

	regval = hal_read32(base_va + HIF_NOCPY_TX_CTRL);
	regval |= HIF_CTRL_BDP_CH_START_WSTB;
	hal_write32(regval, base_va + HIF_NOCPY_TX_CTRL);
#else
    (void)base_va;
#endif /* PFE_HIF_NOCPY_CFG_USE_BD_POLLING */
}

/**
 * @brief		Mask RX IRQ
 * @param[in]	base_va Base address of HIF NOCPY register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_nocpy_cfg_rx_irq_mask(addr_t base_va)
{
	uint32_t reg;

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
void pfe_hif_nocpy_cfg_rx_irq_unmask(addr_t base_va)
{
	uint32_t reg;

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
void pfe_hif_nocpy_cfg_tx_irq_mask(addr_t base_va)
{
	uint32_t reg;

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
void pfe_hif_nocpy_cfg_tx_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + HIF_NOCPY_INT_EN);
	hal_write32(reg
			| BDP_CSR_TX_CBD_INT
			| BDP_CSR_TX_PKT_INT
				, base_va + HIF_NOCPY_INT_EN);
}

/**
 * @brief		Set RX buffer descriptor ring address
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	rx_ring_pa The RX ring address (physical, as seen by host)
 */
void pfe_hif_nocpy_cfg_set_rx_bd_ring_addr(addr_t base_va, const void *rx_ring_pa)
{
	hal_write32((uint32_t)PFE_CFG_MEMORY_PHYS_TO_PFE((addr_t)rx_ring_pa & 0xffffffffU), base_va + HIF_NOCPY_RX_BDP_ADDR);
}

/**
 * @brief		Set TX buffer descriptor ring address
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	tx_ring_pa The TX ring address (physical, as seen by host)
 */
void pfe_hif_nocpy_cfg_set_tx_bd_ring_addr(addr_t base_va, const void *tx_ring_pa)
{
	hal_write32((uint32_t)PFE_CFG_MEMORY_PHYS_TO_PFE((addr_t)tx_ring_pa), base_va + HIF_NOCPY_TX_BDP_ADDR);
}

/**
 * @brief		Get RX ring state
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @return		TRUE when the RX ring BD processor is active or FALSE when it is idle
 */
bool_t pfe_hif_nocpy_cfg_is_rx_dma_active(addr_t base_va)
{
	uint32_t reg;
	bool_t ret = FALSE;

	reg = hal_read32(base_va + HIF_NOCPY_RX_STATUS);

	if (0U != (reg & (0xfUL << 18U)))
	{
		ret = TRUE;
	}

	return ret;
}

/**
 * @brief		Get TX ring state
 * @param[in]	base_va Base address of HIF_NOCPY register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @return		TRUE when the TX ring BD processor is active or FALSE when it is idle
 */
bool_t pfe_hif_nocpy_cfg_is_tx_dma_active(addr_t base_va)
{
	uint32_t reg;
	bool_t ret = FALSE;

	reg = hal_read32(base_va + HIF_NOCPY_TX_STATUS);

	if (0U != (reg & (0xfUL << 18U)))
	{
		ret = TRUE;
	}

	return ret;
}

/**
 * @brief		Get HIF_NOCOPY channel statistics in text form
 * @param[in]	base_va 	Base address of HIF register space (virtual)
 * @param[in]	buf 		Pointer to buffer to be written
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level number of data written to the buffer (0:less 1:more)
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_hif_nocpy_chnl_cfg_get_text_stat(addr_t base_va, const char_t *buf, uint32_t size, uint8_t verb_level)
{
	(void)base_va;
	(void)buf;
	(void)size;
	(void)verb_level;
	return 0U;
}

/**
 * @brief		Get number of transmitted packets
 * @param[in]	base_va Base address of channel register space (virtual)
 * @return		Number of transmitted packets
 */
uint32_t pfe_hif_nocpy_cfg_get_tx_cnt(addr_t base_va)
{
	(void)base_va;
	NXP_LOG_WARNING("HIF NOCPY does not provide TX packet counter\n");
	return 0xffffffffU;
}

/**
 * @brief		Get number of received packets
 * @param[in]	base_va Base address of channel register space (virtual)
 * @return		Number of received packets
 */
uint32_t pfe_hif_nocpy_cfg_get_rx_cnt(addr_t base_va)
{
	(void)base_va;
	NXP_LOG_WARNING("HIF NOCPY does not provide RX packet counter\n");
	return 0xffffffffU;
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
uint32_t pfe_hif_nocpy_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		len = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Get version */
		if(verb_level >= 9U)
		{
			reg = hal_read32(base_va + HIF_NOCPY_VERSION);
			len += oal_util_snprintf(buf + len, size - len, "Revision             : 0x%x\n", (reg >> 24U) & 0xffU);
			len += oal_util_snprintf(buf + len, size - len, "Version              : 0x%x\n", (reg >> 16U) & 0xffU);
			len += oal_util_snprintf(buf + len, size - len, "ID                   : 0x%x\n", reg & 0xffffU);
		}

		len += oal_util_snprintf(buf + len, size - len, "TX Current BD Addr   : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_TX_CURR_BD_ADDR));
		len += oal_util_snprintf(buf + len, size - len, "TX Status            : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_TX_STATUS));
		len += oal_util_snprintf(buf + len, size - len, "TX DMA Status        : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_TX_DMA_STATUS));
		len += oal_util_snprintf(buf + len, size - len, "TX Ctrl              : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_TX_CTRL));
		len += oal_util_snprintf(buf + len, size - len, "RX Current BD Addr   : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_RX_CURR_BD_ADDR));
		len += oal_util_snprintf(buf + len, size - len, "RX Status            : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_RX_STATUS));
		len += oal_util_snprintf(buf + len, size - len, "RX DMA Status        : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_RX_DMA_STATUS));
		len += oal_util_snprintf(buf + len, size - len, "RX Ctrl              : 0x%08x\n", hal_read32(base_va + HIF_NOCPY_RX_CTRL));
	}

	return len;
}
