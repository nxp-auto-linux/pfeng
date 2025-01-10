/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_hm.h"
#include "pfe_hif_csr.h"
#include "pfe_platform_cfg.h"
#include "pfe_feature_mgr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#define HIF_CH_TIMEOUT_EN (BDP_RD_CSR_RX_TIMEOUT_CH_INT_EN  | BDP_WR_CSR_RX_TIMEOUT_CH_INT_EN \
                          | BDP_RD_CSR_TX_TIMEOUT_CH_INT_EN | BDP_WD_CSR_TX_TIMEOUT_CH_INT_EN \
                          | DXR_CSR_RX_TIMEOUT_CH_INT_EN    | DXR_CSR_TX_TIMEOUT_CH_INT_EN)

/**
 * @brief	Control the buffer descriptor fetch
 * @details	When TRUE then HIF is fetching the same BD until it is valid BD. If FALSE
 * 			then user must trigger the HIF to fetch next BD explicitly by
 * 			pfe_hif_rx_dma_start() and pfe_hif_tx_dma_start().
 */
#ifdef	PFE_CFG_HIF_USE_BD_TRIGGER
#define	PFE_HIF_CFG_USE_BD_POLLING		FALSE
#else
#define	PFE_HIF_CFG_USE_BD_POLLING		TRUE
#endif

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)

static inline void dump_hif_channel(addr_t base_va, uint32_t channel_id)
{
#ifdef NXP_LOG_ENABLED
	uint32_t reg;

	reg = hal_read32(base_va + HIF_CTRL_CHn(channel_id));
	NXP_LOG_INFO("HIF_CTRL_CH%u                    : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_BDP_WR_LOW_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_BDP_WR_LOW_ADDR_CH%u      : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_BDP_WR_HIGH_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_BDP_WR_HIGH_ADDR_CH%u     : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_BDP_RD_LOW_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_BDP_RD_LOW_ADDR_CH%u      : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_BDP_RD_HIGH_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_BDP_RD_HIGH_ADDR_CH%u     : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_BDP_WR_LOW_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_BDP_WR_LOW_ADDR_CH%u      : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_BDP_WR_HIGH_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_BDP_WR_HIGH_ADDR_CH%u     : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_BDP_RD_LOW_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_BDP_RD_LOW_ADDR_CH%u      : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_BDP_RD_HIGH_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_BDP_RD_HIGH_ADDR_CH%u     : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_WRBK_BD_CHn_BUFFER_SIZE(channel_id));
	NXP_LOG_INFO("HIF_RX_WRBK_BD_CH%u_BUFFER_SIZE  : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_CHn_START(channel_id));
	NXP_LOG_INFO("HIF_RX_CH%u_START                : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_WRBK_BD_CHn_BUFFER_SIZE(channel_id));
	NXP_LOG_INFO("HIF_TX_WRBK_BD_CH%u_BUFFER_SIZE  : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_CHn_START(channel_id));
	NXP_LOG_INFO("HIF_TX_CH%u_START                : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_CHn_INT_SRC(channel_id));
	NXP_LOG_INFO("HIF_CH%u_INT_SRC                 : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_RD_CURR_BD_LOW_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_RD_CURR_BD_LOW_ADDR_CH%u  : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_RD_CURR_BD_HIGH_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_RD_CURR_BD_HIGH_ADDR_CH%u : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_WR_CURR_BD_LOW_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_WR_CURR_BD_LOW_ADDR_CH%u  : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_WR_CURR_BD_HIGH_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_WR_CURR_BD_HIGH_ADDR_CH%u : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_BDP_CHn_TX_FIFO_CNT(channel_id));
	NXP_LOG_INFO("HIF_BDP_CH%u_TX_FIFO_CNT         : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_DMA_STATUS_0_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_DMA_STATUS_0_CH%u         : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_STATUS_0_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_STATUS_0_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_STATUS_1_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_STATUS_1_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_PKT_CNT0_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_PKT_CNT0_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_PKT_CNT1_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_PKT_CNT1_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_TX_PKT_CNT2_CHn(channel_id));
	NXP_LOG_INFO("HIF_TX_PKT_CNT2_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_RD_CURR_BD_LOW_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_RD_CURR_BD_LOW_ADDR_CH%u  : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_RD_CURR_BD_HIGH_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_RD_CURR_BD_HIGH_ADDR_CH%u : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_WR_CURR_BD_LOW_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_WR_CURR_BD_LOW_ADDR_CH%u  : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_WR_CURR_BD_HIGH_ADDR_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_WR_CURR_BD_HIGH_ADDR_CH%u : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_BDP_CHn_RX_FIFO_CNT(channel_id));
	NXP_LOG_INFO("HIF_BDP_CH%u_RX_FIFO_CNT         : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_DMA_STATUS_0_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_DMA_STATUS_0_CH%u         : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_STATUS_0_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_STATUS_0_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_PKT_CNT0_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_PKT_CNT0_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_RX_PKT_CNT1_CHn(channel_id));
	NXP_LOG_INFO("HIF_RX_PKT_CNT1_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_LTC_MAX_PKT_CHn_ADDR(channel_id));
	NXP_LOG_INFO("HIF_LTC_MAX_PKT_CH_ADDR%u        : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_ABS_INT_TIMER_CHn(channel_id));
	NXP_LOG_INFO("HIF_ABS_INT_TIMER_CH%u           : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_ABS_FRAME_COUNT_CHn(channel_id));
	NXP_LOG_INFO("HIF_ABS_FRAME_COUNT_CH%u         : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_INT_COAL_EN_CHn(channel_id));
	NXP_LOG_INFO("HIF_INT_COAL_EN_CH%u             : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
	reg = hal_read32(base_va + HIF_CHn_INT_EN(channel_id));
	NXP_LOG_INFO("HIF_INT_EN_CH%u                  : 0x%08x\n", (uint_t)channel_id, (uint_t)reg);
#else
    (void) base_va;
    (void) channel_id;
#endif /* NXP_LOG_ENABLED */
}

#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */


/**
 * @brief		HIF ISR
 * @details		MASK, ACK, and process triggered interrupts
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Make sure the call is protected by some per-HIF mutex
 */
errno_t pfe_hif_cfg_isr(addr_t base_va)
{
	uint32_t glob_src, reg_src, reg_en;
	errno_t ret = ENOENT;

	/*	Get master HIF interrupt status. This register is read-only
		and does not require ACK. */
	glob_src = hal_read32(base_va + HIF_INT_SRC);
	if (unlikely(0U != glob_src))
	{
		if ((glob_src & HIF_INT_SRC_HIF_ERR_INT) != 0U)
		{
			/*	Get enabled interrupts */
			reg_en = hal_read32(base_va + HIF_ERR_INT_EN);
			/*	Disable ALL */
			hal_write32(0U, base_va + HIF_ERR_INT_EN);
			/*	Get triggered interrupts */
			reg_src = hal_read32(base_va + HIF_ERR_INT_SRC);
			/*	ACK triggered */
			hal_write32(reg_src, base_va + HIF_ERR_INT_SRC);
			/*	Enable the non-triggered ones */
			hal_write32((reg_en & ~reg_src), base_va + HIF_ERR_INT_EN);

			/*	Process interrupts which are triggered AND enabled */
			if ((reg_src & reg_en & HIF_ERR_INT) != 0U)
			{
				pfe_hm_report_error(HM_SRC_HIF, HM_EVT_HIF_ERR, "Interrupt (0x%x)", (uint_t)reg_src);
				ret = EOK;
			}
		}

		if ((glob_src & HIF_INT_SRC_HIF_TX_FIFO_ERR_INT) != 0U)
		{
			/*	Get enabled interrupts */
			reg_en = hal_read32(base_va + HIF_TX_FIFO_ERR_INT_EN);
			/*	Disable ALL */
			hal_write32(0U, base_va + HIF_TX_FIFO_ERR_INT_EN);
			/*	Get triggered interrupts */
			reg_src = hal_read32(base_va + HIF_TX_FIFO_ERR_INT_SRC);
			/*	ACK triggered */
			hal_write32(reg_src, base_va + HIF_TX_FIFO_ERR_INT_SRC);
			/*	Enable the non-triggered ones */
			hal_write32((reg_en & ~reg_src), base_va + HIF_TX_FIFO_ERR_INT_EN);

			/*	Process interrupts which are triggered AND enabled */
			if ((reg_src & reg_en & HIF_TX_FIFO_ERR_INT) != 0U)
			{
				pfe_hm_report_error(HM_SRC_HIF, HM_EVT_HIF_TX_FIFO, "Interrupt (0x%x)", (uint_t)reg_src);
				ret = EOK;
			}
		}

		if ((glob_src & HIF_INT_SRC_HIF_RX_FIFO_ERR_INT) != 0U)
		{
			/*	Get enabled interrupts */
			reg_en = hal_read32(base_va + HIF_RX_FIFO_ERR_INT_EN);
			/*	Disable ALL */
			hal_write32(0U, base_va + HIF_RX_FIFO_ERR_INT_EN);
			/*	Get triggered interrupts */
			reg_src = hal_read32(base_va + HIF_RX_FIFO_ERR_INT_SRC);
			/*	ACK triggered */
			hal_write32(reg_src, base_va + HIF_RX_FIFO_ERR_INT_SRC);
			/*	Enable the non-triggered ones */
			hal_write32((reg_en & ~reg_src), base_va + HIF_RX_FIFO_ERR_INT_EN);

			/*	Process interrupts which are triggered AND enabled */
			if ((reg_src & reg_en & HIF_RX_FIFO_ERR_INT) != 0U)
			{
				pfe_hm_report_error(HM_SRC_HIF, HM_EVT_HIF_RX_FIFO, "Interrupt (0x%x)", (uint_t)reg_src);
				ret = EOK;
			}
		}
	}

	return ret;
}

/**
 * @brief		Mask HIF interrupts
 * @details		Only affects HIF IRQs, not channel IRqs.
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_cfg_irq_mask(addr_t base_va)
{
	uint32_t reg;

	/*	Disable groups */
	reg = hal_read32(base_va + HIF_ERR_INT_EN) & ~(HIF_ERR_INT);
	hal_write32(reg, base_va + HIF_ERR_INT_EN);

	reg = hal_read32(base_va + HIF_TX_FIFO_ERR_INT_EN) & ~(HIF_TX_FIFO_ERR_INT);
	hal_write32(reg, base_va + HIF_TX_FIFO_ERR_INT_EN);

	reg = hal_read32(base_va + HIF_RX_FIFO_ERR_INT_EN) & ~(HIF_RX_FIFO_ERR_INT);
	hal_write32(reg, base_va + HIF_RX_FIFO_ERR_INT_EN);
}

/**
 * @brief		Unmask HIF interrupts
 * @details		Only affects HIF IRQs, not channel IRqs.
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_cfg_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	/*	Enable groups */
	reg = hal_read32(base_va + HIF_ERR_INT_EN) | HIF_ERR_INT;
	hal_write32(reg, base_va + HIF_ERR_INT_EN);

	reg = hal_read32(base_va + HIF_TX_FIFO_ERR_INT_EN) | HIF_TX_FIFO_ERR_INT;
	hal_write32(reg, base_va + HIF_TX_FIFO_ERR_INT_EN);

	reg = hal_read32(base_va + HIF_RX_FIFO_ERR_INT_EN) | HIF_RX_FIFO_ERR_INT;
	hal_write32(reg, base_va + HIF_RX_FIFO_ERR_INT_EN);
}

/**
 * @brief		HIF channel ISR
 * @details		Handles all HIF channel interrupts. MASK, ACK, and process triggered interrupts.
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	events Bitmask representing indicated events
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Make sure the call is protected by some per-channel mutex
 */
errno_t pfe_hif_chnl_cfg_isr(addr_t base_va, uint32_t channel_id, pfe_hif_chnl_event_t *events)
{
	uint32_t reg_src, reg_en;
	errno_t ret = ENOENT;

	*events = (pfe_hif_chnl_event_t)0;

	if (unlikely(channel_id >= HIF_CFG_MAX_CHANNELS))
	{
		NXP_LOG_ERROR("Invalid channel ID in ISR\n");
		ret = EINVAL;
	}
	else
	{
		/*	Get enabled interrupts */
		reg_en = hal_read32(base_va + HIF_CHn_INT_EN(channel_id));
		/*	Disable ALL */
		hal_write32(0U, base_va + HIF_CHn_INT_EN(channel_id));
		/*	Get triggered interrupts */
		reg_src = hal_read32(base_va + HIF_CHn_INT_SRC(channel_id));
		/*	ACK triggered */
		hal_write32(reg_src, base_va + HIF_CHn_INT_SRC(channel_id));
		/*	Enable the non-triggered ones */
		hal_write32((reg_en & ~reg_src), base_va + HIF_CHn_INT_EN(channel_id));

		/*	Process interrupts which are triggered AND enabled */
		if ((reg_src & reg_en & (BDP_CSR_RX_PKT_CH_INT|BDP_CSR_RX_CBD_CH_INT)) != 0U)
		{
			*events |= HIF_CHNL_EVT_RX_IRQ;
			ret = EOK;
		}

		/*	Process interrupts which are triggered AND enabled */
		if ((reg_src & reg_en & (BDP_CSR_TX_PKT_CH_INT|BDP_CSR_TX_CBD_CH_INT)) != 0U)
		{
			*events |= HIF_CHNL_EVT_TX_IRQ;
			ret = EOK;
		}

		if (unlikely(reg_src & reg_en
				& (	BDP_RD_CSR_RX_TIMEOUT_CH_INT|BDP_WR_CSR_RX_TIMEOUT_CH_INT
					| BDP_RD_CSR_TX_TIMEOUT_CH_INT|BDP_WR_CSR_TX_TIMEOUT_CH_INT
					| DXR_CSR_RX_TIMEOUT_CH_INT|DXR_CSR_TX_TIMEOUT_CH_INT)))
		{
			if ((reg_src & reg_en & BDP_RD_CSR_RX_TIMEOUT_CH_INT) != 0U)
			{
				/*	AAVB-2144 */
				NXP_LOG_INFO("BDP_RD_CSR_RX_TIMEOUT_CH%u_INT. Interrupt disabled.\n", (uint_t)channel_id);
			}

			if ((reg_src & reg_en & BDP_WR_CSR_RX_TIMEOUT_CH_INT) != 0U)
			{
				/*	AAVB-2144 */
				NXP_LOG_INFO("BDP_WR_CSR_RX_TIMEOUT_CH%u_INT. Interrupt disabled.\n", (uint_t)channel_id);
			}

			if ((reg_src & reg_en & BDP_RD_CSR_TX_TIMEOUT_CH_INT) != 0U)
			{
				/*	AAVB-2144 */
				NXP_LOG_INFO("BDP_RD_CSR_TX_TIMEOUT_CH%u_INT. Interrupt disabled.\n", (uint_t)channel_id);
			}

			if ((reg_src & reg_en & BDP_WR_CSR_TX_TIMEOUT_CH_INT) != 0U)
			{
				/*	AAVB-2144 */
				NXP_LOG_INFO("BDP_WR_CSR_TX_TIMEOUT_CH%u_INT. Interrupt disabled.\n", (uint_t)channel_id);
			}

			if ((reg_src & reg_en & DXR_CSR_RX_TIMEOUT_CH_INT) != 0U)
			{
				/*	AAVB-2144 */
				NXP_LOG_INFO("DXR_CSR_RX_TIMEOUT_CH%u_INT. Interrupt disabled.\n", (uint_t)channel_id);
			}

			if ((reg_src & reg_en & DXR_CSR_TX_TIMEOUT_CH_INT) != 0U)
			{
				/*	AAVB-2144 */
				NXP_LOG_INFO("DXR_CSR_TX_TIMEOUT_CH%u_INT. Interrupt disabled.\n", (uint_t)channel_id);
			}

			/*	Don't re-enable these interrupts. See AAVB-2144. */

			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Configure and initialize the HIF channel
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @return		EOK if success, error code otherwise
 * @note		Make sure the call is protected by some per-channel mutex
 */
errno_t pfe_hif_chnl_cfg_init(addr_t base_va, uint32_t channel_id)
{
	/*	Disable channel interrupts */
	hal_write32(0U, base_va + HIF_CHn_INT_EN(channel_id));
	hal_write32(0xffffffffU, base_va + HIF_CHn_INT_SRC(channel_id));

	/*	Disable RX/TX DMA */
	pfe_hif_chnl_cfg_rx_disable(base_va, channel_id);
	pfe_hif_chnl_cfg_tx_disable(base_va, channel_id);

	/*	Disable RX coalescing */
	(void)pfe_hif_chnl_cfg_set_rx_irq_coalesce(base_va, channel_id, 0U, 0U);

	/*	Enable channel status interrupts except of the RX/TX and
	 	the global enable bit. */

	/*	Disable bogus HIF timeouts reporting */
	hal_write32(0xffffffffU
			& ~HIF_CH_INT_EN
			& ~BDP_CSR_RX_CBD_CH_INT_EN
			& ~BDP_CSR_RX_PKT_CH_INT_EN
			& ~BDP_CSR_TX_CBD_CH_INT_EN
			& ~BDP_CSR_TX_PKT_CH_INT_EN
			& ~HIF_CH_TIMEOUT_EN
			, base_va + HIF_CHn_INT_EN(channel_id));

	return EOK;
}

/**
 * @brief		Properly finalize a HIF channel
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @note		Make sure the call is protected by some per-channel mutex
 */
void pfe_hif_chnl_cfg_fini(addr_t base_va, uint32_t channel_id)
{
	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else
	{
		/*	Disable the coalescence timer */
		hal_write32(0x0U, base_va + HIF_INT_COAL_EN_CHn(channel_id));

		/*	Disable RX/TX */
		pfe_hif_chnl_cfg_rx_disable(base_va, channel_id);
		pfe_hif_chnl_cfg_tx_disable(base_va, channel_id);

		/*	Disable all interrupts */
		hal_write32(0U, base_va + HIF_CHn_INT_EN(channel_id));
	}
}

/**
 * @brief		Configure and initialize the HIF
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @return		EOK if success, error code otherwise
 * @note		Make sure the call is protected by some per-HIF mutex
 */
errno_t pfe_hif_cfg_init(addr_t base_va)
{
	errno_t ret = EOK;
#ifdef PFE_CFG_PFE_MASTER
	uint32_t ii = 0u;

	/*	Disable and clear HIF interrupts. Channel interrupts are
	 	handled separately within pfe_hif_chnl_cfg_init(). */
	hal_write32(0U, base_va + HIF_ERR_INT_EN);
	hal_write32(0U, base_va + HIF_TX_FIFO_ERR_INT_EN);
	hal_write32(0U, base_va + HIF_RX_FIFO_ERR_INT_EN);
	hal_write32(0xffffffffU, base_va + HIF_ERR_INT_SRC);
	hal_write32(0xffffffffU, base_va + HIF_TX_FIFO_ERR_INT_SRC);
	hal_write32(0xffffffffU, base_va + HIF_RX_FIFO_ERR_INT_SRC);

	if (FALSE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
	{
		/*	SOFT RESET */
		hal_write32(0xfu, base_va + HIF_SOFT_RESET);
		while (hal_read32(base_va + HIF_SOFT_RESET) != 0U)
		{
			if (++ii > 1000u)
			{
				ret = ETIMEDOUT;
				break;
			}
			else
			{
				oal_time_usleep(1000);
			}
		}
	}

	if (EOK == ret)
	{
	#if (TRUE == PFE_HIF_CFG_USE_BD_POLLING)
		hal_write32((0xffUL << 16U) | (0xffUL), base_va + HIF_TX_POLL_CTRL);
		hal_write32((0xffUL << 16U) | (0xffUL), base_va + HIF_RX_POLL_CTRL);
	#endif /* PFE_HIF_CFG_USE_BD_POLLING */

		/*    MICS */
		hal_write32(0U
				/* | BDPRD_AXI_WRITE_DONE */
				/* | DBPWR_AXI_WRITE_DONE */
				/* | RXDXR_AXI_WRITE_DONE */
				/* | TXDXR_AXI_WRITE_DONE */
				| HIF_TIMEOUT_EN
				| BD_START_SEQ_NUM(0x0U)
				, base_va + HIF_MISC);

		hal_write32(100000000U, base_va + HIF_TIMEOUT_REG);
		hal_write32(0x33221100U, base_va + HIF_RX_QUEUE_MAP_CH_NO_ADDR);
		hal_write32(0x0U, base_va + HIF_DMA_BURST_SIZE_ADDR); /* 0 = 128B, 1 = 256B, 2 = 512B, 3 = 1024B */
		hal_write32(0x0U, base_va + HIF_DMA_BASE_ADDR);
		hal_write32(0x0U, base_va + HIF_LTC_PKT_CTRL_ADDR); /* Must stay disabled. LTC hijaked for Master-detect feature */
		hal_write32(0xffffffffU & ~(HIF_ERR_INT), base_va + HIF_ERR_INT_EN);
		hal_write32(0xffffffffU & ~(HIF_TX_FIFO_ERR_INT), base_va + HIF_TX_FIFO_ERR_INT_EN);
		hal_write32(0xffffffffU  & ~(HIF_RX_FIFO_ERR_INT), base_va + HIF_RX_FIFO_ERR_INT_EN);
	}
#else
    (void)base_va;
#endif /* PFE_CFG_PFE_MASTER */

	return ret;
}

/**
 * @brief		Finalize the HIF
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @note		Make sure the call is protected by some per-HIF mutex
 */
void pfe_hif_cfg_fini(addr_t base_va)
{
#ifdef PFE_CFG_PFE_MASTER
	/*	Disable HIF interrupts */
	hal_write32(0U, base_va + HIF_ERR_INT_EN);
	hal_write32(0U, base_va + HIF_TX_FIFO_ERR_INT_EN);
	hal_write32(0U, base_va + HIF_RX_FIFO_ERR_INT_EN);
#else
	(void)base_va;	/* suppress gcc warning */
#endif
}

/**
 * @brief		Get TX FIFO fill level
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @return		Number of bytes in HIF FX FIFO
 */
uint32_t pfe_hif_cfg_get_tx_fifo_fill_level(addr_t base_va)
{
	return (8U * hal_read32(base_va + HIF_DXR_TX_FIFO_CNT));
}

/**
 * @brief		Enable TX
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_chnl_cfg_tx_enable(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else
	{
		/*	Enable DMA engine */
		reg = hal_read32(base_va + HIF_CTRL_CHn(channel_id));

	#if (TRUE == PFE_HIF_CFG_USE_BD_POLLING)
		reg |= TX_BDP_POLL_CNTR_EN;
	#endif /* PFE_HIF_CFG_USE_BD_POLLING */

		reg |= TX_DMA_ENABLE;
		hal_write32(reg, base_va + HIF_CTRL_CHn(channel_id));

	#if (TRUE == PFE_HIF_CFG_USE_BD_POLLING)
		pfe_hif_chnl_cfg_tx_dma_start(base_va, channel_id);
	#endif /* PFE_HIF_CFG_USE_BD_POLLING */
	}
}

/**
 * @brief		Disable TX
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_chnl_cfg_tx_disable(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else
	{
	#if (FALSE == PFE_HIF_CFG_USE_BD_POLLING)
		pfe_hif_chnl_cfg_tx_dma_stop(base_va, channel_id);
	#endif /* PFE_HIF_CFG_USE_BD_POLLING */

		reg = hal_read32(base_va + HIF_CTRL_CHn(channel_id));
		reg &= ~(TX_DMA_ENABLE|TX_BDP_POLL_CNTR_EN);
		hal_write32(reg, base_va + HIF_CTRL_CHn(channel_id));

		/*	Disable TX IRQ */
		pfe_hif_chnl_cfg_tx_irq_mask(base_va, channel_id);
	}
}

/**
 * @brief		Enable RX
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_chnl_cfg_rx_enable(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else
	{

		reg = hal_read32(base_va + HIF_CTRL_CHn(channel_id));

	#if (TRUE == PFE_HIF_CFG_USE_BD_POLLING)
		reg |= RX_BDP_POLL_CNTR_EN;
	#endif /* PFE_HIF_CFG_USE_BD_POLLING */

		reg |= RX_DMA_ENABLE;
		hal_write32(reg, base_va + HIF_CTRL_CHn(channel_id));

	#if (FALSE == PFE_HIF_CFG_USE_BD_POLLING)
		pfe_hif_chnl_cfg_rx_dma_start(base_va, channel_id);
	#endif /* PFE_HIF_CFG_USE_BD_POLLING */
	}
}

/**
 * @brief		Disable RX
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_chnl_cfg_rx_disable(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else
	{
	#if (FALSE == PFE_HIF_CFG_USE_BD_POLLING)
		pfe_hif_chnl_cfg_rx_dma_stop(base_va, channel_id);
	#endif /* PFE_HIF_CFG_USE_BD_POLLING */

		reg = hal_read32(base_va + HIF_CTRL_CHn(channel_id));
		reg &= ~(RX_DMA_ENABLE|RX_BDP_POLL_CNTR_EN);
		hal_write32(reg, base_va + HIF_CTRL_CHn(channel_id));

		pfe_hif_chnl_cfg_rx_irq_mask(base_va, channel_id);
	}
}

/**
 * @brief		Trigger RX DMA
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_chnl_cfg_rx_dma_start(addr_t base_va, uint32_t channel_id)
{
#if (FALSE == PFE_HIF_CFG_USE_BD_POLLING)
	hal_write32(RX_BDP_CH_START, base_va + HIF_RX_CHn_START(channel_id));
#else
    (void)base_va;
    (void)channel_id;
#endif /* PFE_HIF_CFG_USE_BD_POLLING */
}

/**
 * @brief		Stop RX DMA
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_chnl_cfg_rx_dma_stop(addr_t base_va, uint32_t channel_id)
{
#if (FALSE == PFE_HIF_CFG_USE_BD_POLLING)
	hal_write32(0, base_va + HIF_RX_CHn_START(channel_id));
#else
    (void)base_va;
    (void)channel_id;
#endif /* PFE_HIF_CFG_USE_BD_POLLING */
}

/**
 * @brief		Trigger TX DMA
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_chnl_cfg_tx_dma_start(addr_t base_va, uint32_t channel_id)
{
#if (FALSE == PFE_HIF_CFG_USE_BD_POLLING)
	hal_write32(TX_BDP_CH_START, base_va + HIF_TX_CHn_START(channel_id));
#else
    (void)base_va;
    (void)channel_id;
#endif /* PFE_HIF_CFG_USE_BD_POLLING */
}

/**
 * @brief		Stop TX DMA
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
void pfe_hif_chnl_cfg_tx_dma_stop(addr_t base_va, uint32_t channel_id)
{
#if (FALSE == PFE_HIF_CFG_USE_BD_POLLING)
	hal_write32(0, base_va + HIF_TX_CHn_START(channel_id));
#else
    (void)base_va;
    (void)channel_id;
#endif /* PFE_HIF_CFG_USE_BD_POLLING */
}

/**
 * @brief		Mask channel IRQ
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @note		Make sure the call is protected by some per-channel mutex
 */
void pfe_hif_chnl_cfg_irq_mask(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	/*	Disable group */
	reg = hal_read32( base_va + HIF_CHn_INT_EN(channel_id)) & ~(HIF_CH_INT_EN);
	hal_write32(reg, base_va + HIF_CHn_INT_EN(channel_id));
}

/**
 * @brief		Unmask channel IRQ
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @note		Make sure the call is protected by some per-channel mutex
 */
void pfe_hif_chnl_cfg_irq_unmask(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	/*	Enable group */
	reg = hal_read32( base_va + HIF_CHn_INT_EN(channel_id)) | HIF_CH_INT_EN;
	hal_write32(reg, base_va + HIF_CHn_INT_EN(channel_id));
}

/**
 * @brief		Mask channel RX IRQ
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @note		Make sure the call is protected by some per-channel mutex
 */
void pfe_hif_chnl_cfg_rx_irq_mask(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	/*	Disable RX IRQ */
	reg = hal_read32(base_va + HIF_CHn_INT_EN(channel_id));
	hal_write32(reg
				& ~BDP_CSR_RX_CBD_CH_INT_EN
				& ~BDP_CSR_RX_PKT_CH_INT_EN
				, base_va + HIF_CHn_INT_EN(channel_id));
}

/**
 * @brief		Unmask channel RX IRQ
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @note		Make sure the call is protected by some per-channel mutex
 */
void pfe_hif_chnl_cfg_rx_irq_unmask(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	/*	Enable RX IRQ */
	reg = hal_read32(base_va + HIF_CHn_INT_EN(channel_id));
	hal_write32(reg
			| BDP_CSR_RX_CBD_CH_INT_EN
			| BDP_CSR_RX_PKT_CH_INT_EN
				, base_va + HIF_CHn_INT_EN(channel_id));
}

/**
 * @brief		Mask channel TX IRQ
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @note		Make sure the call is protected by some per-channel mutex
 */
void pfe_hif_chnl_cfg_tx_irq_mask(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	/*	Disable TX IRQ */
	reg = hal_read32(base_va + HIF_CHn_INT_EN(channel_id));
	hal_write32(reg
				& ~BDP_CSR_TX_CBD_CH_INT_EN
				& ~BDP_CSR_TX_PKT_CH_INT_EN
				, base_va + HIF_CHn_INT_EN(channel_id));
}

/**
 * @brief		Unmask channel TX IRQ
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @note		Make sure the call is protected by some per-channel mutex
 */
void pfe_hif_chnl_cfg_tx_irq_unmask(addr_t base_va, uint32_t channel_id)
{
	uint32_t reg;

	/*	Enable TX IRQ */
	reg = hal_read32(base_va + HIF_CHn_INT_EN(channel_id));
	hal_write32(reg
			| BDP_CSR_TX_CBD_CH_INT_EN
			| BDP_CSR_TX_PKT_CH_INT_EN
				, base_va + HIF_CHn_INT_EN(channel_id));
}

uint32_t pfe_hif_chnl_cfg_get_rx_bd_ring_addr(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_RX_BDP_RD_LOW_ADDR_CHn(channel_id));
}

uint32_t pfe_hif_chnl_cfg_get_rx_wb_table_addr(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_RX_BDP_WR_LOW_ADDR_CHn(channel_id));
}

uint32_t pfe_hif_chnl_cfg_get_rx_wb_table_len(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_RX_WRBK_BD_CHn_BUFFER_SIZE(channel_id));
}

uint32_t pfe_hif_chnl_cfg_get_tx_bd_ring_addr(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_TX_BDP_RD_LOW_ADDR_CHn(channel_id));
}

uint32_t pfe_hif_chnl_cfg_get_tx_wb_table_addr(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_TX_BDP_WR_LOW_ADDR_CHn(channel_id));
}

uint32_t pfe_hif_chnl_cfg_get_tx_wb_table_len(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_TX_WRBK_BD_CHn_BUFFER_SIZE(channel_id));
}

/**
 * @brief		Set RX buffer descriptor ring address
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	rx_ring_pa The RX ring address (physical, as seen by host)
 */
void pfe_hif_chnl_cfg_set_rx_bd_ring_addr(addr_t base_va, uint32_t channel_id, const void *rx_ring_pa)
{
	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else
	{
		hal_write32((uint32_t)((addr_t)rx_ring_pa & 0xffffffffU), base_va + HIF_RX_BDP_RD_LOW_ADDR_CHn(channel_id));
		hal_write32(0U, base_va + HIF_RX_BDP_RD_HIGH_ADDR_CHn(channel_id));
	}
}

/**
 * @brief		Set RX write-back table
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	wb_ring_pa The write-back table address (physical, as seen by host)
 * @param[in]	ring_len Number of entries in the WB table
 */
void pfe_hif_chnl_cfg_set_rx_wb_table(addr_t base_va, uint32_t channel_id, const void *wb_tbl_pa, uint32_t tbl_len)
{
	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else if (tbl_len > 0xffffU)
	{
		NXP_LOG_ERROR("Unsupported WB table size: %u\n", (uint_t)tbl_len);
	}
	else
	{
		hal_write32((uint32_t)((addr_t)wb_tbl_pa & 0xffffffffU), base_va + HIF_RX_BDP_WR_LOW_ADDR_CHn(channel_id));
		hal_write32(0U, base_va + HIF_RX_BDP_WR_HIGH_ADDR_CHn(channel_id));
		hal_write32(tbl_len, base_va + HIF_RX_WRBK_BD_CHn_BUFFER_SIZE(channel_id));
	}
}

/**
 * @brief		Set TX buffer descriptor ring address
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	tx_ring_pa The TX ring address (physical, as seen by host)
 */
void pfe_hif_chnl_cfg_set_tx_bd_ring_addr(addr_t base_va, uint32_t channel_id, const void *tx_ring_pa)
{
	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else
	{
		hal_write32((uint32_t)((addr_t)tx_ring_pa & 0xffffffffU), base_va + HIF_TX_BDP_RD_LOW_ADDR_CHn(channel_id));
		hal_write32(0U, base_va + HIF_TX_BDP_RD_HIGH_ADDR_CHn(channel_id));
	}
}

/**
 * @brief		Set TX write-back table
  * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	wb_ring_pa The write-back table address (physical, as seen by host)
 * @param[in]	ring_len Number of entries in the WB table
 */
void pfe_hif_chnl_cfg_set_tx_wb_table(addr_t base_va, uint32_t channel_id, const void *wb_tbl_pa, uint32_t tbl_len)
{
	if (channel_id >= HIF_CFG_MAX_CHANNELS)
	{
		NXP_LOG_ERROR("Unsupported channel ID: %u\n", (uint_t)channel_id);
	}
	else if (tbl_len > 0xffffU)
	{
		NXP_LOG_ERROR("Unsupported WB table size: %u\n", (uint_t)tbl_len);
	}
	else
	{
		hal_write32((uint32_t)((addr_t)wb_tbl_pa & 0xffffffffU), base_va + HIF_TX_BDP_WR_LOW_ADDR_CHn(channel_id));
		hal_write32(0U, base_va + HIF_TX_BDP_WR_HIGH_ADDR_CHn(channel_id));
		hal_write32(tbl_len, base_va + HIF_TX_WRBK_BD_CHn_BUFFER_SIZE(channel_id));
	}
}

/**
 * @brief		RX BDP FIFO status
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @retval		TRUE The FIFO is empty
 * @retval		FALSE The FIFO is not emtpy
 */
bool_t pfe_hif_chnl_cfg_is_rx_bdp_fifo_empty(addr_t base_va, uint32_t channel_id)
{
	return (0U == hal_read32(base_va + HIF_BDP_CHn_RX_FIFO_CNT(channel_id)));
}

/**
 * @brief		TX BDP FIFO status
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @retval		TRUE The FIFO is empty
 * @retval		FALSE The FIFO is not emtpy
 */
bool_t pfe_hif_chnl_cfg_is_tx_bdp_fifo_empty(addr_t base_va, uint32_t channel_id)
{
	return (0U == hal_read32(base_va + HIF_BDP_CHn_TX_FIFO_CNT(channel_id)));
}

/**
 * @brief		Get RX coalesce setting
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[out]	frames Number of frames
 * @param[out]	cycles Number of cycles
 */
errno_t pfe_hif_chnl_cfg_get_rx_irq_coalesce(addr_t base_va, uint32_t channel_id, uint32_t *frames, uint32_t *cycles)
{
	uint32_t ena = hal_read32(base_va + HIF_INT_COAL_EN_CHn(channel_id));

	if (0U == (ena & (HIF_INT_COAL_TIME_ENABLE | HIF_INT_COAL_FRAME_ENABLE)))
	{
		/* Coalesce is disabled */
		*cycles = 0U;
		*frames = 0U;
	}
	else
	{
		if (0U != (ena & HIF_INT_COAL_TIME_ENABLE))
		{
			*cycles = hal_read32(base_va + HIF_ABS_INT_TIMER_CHn(channel_id));
		}
		else
		{
			*cycles = 0U;
		}

		if (0U != (ena & HIF_INT_COAL_FRAME_ENABLE))
		{
			*frames = hal_read32(base_va + HIF_ABS_FRAME_COUNT_CHn(channel_id));
		}
		else
		{
			*frames = 0U;
		}
	}

	return EOK;
}

/**
 * @brief		Set HIF channel RX coalesce setting
 * @details		The coalesce setting for HIF channel.
 * 				If both frames and cycles are zero, then coalesting
 * 				will be disabled.
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	frames Number of frames
 * @param[in]	cycles Number of cycles
 */
errno_t pfe_hif_chnl_cfg_set_rx_irq_coalesce(addr_t base_va, uint32_t channel_id, uint32_t frames, uint32_t cycles)
{
	errno_t ret;
	uint32_t reg;

	/* Disable coalescing */
	hal_write32(0x0U, base_va + HIF_INT_COAL_EN_CHn(channel_id));
	hal_write32(0x0U, base_va + HIF_ABS_FRAME_COUNT_CHn(channel_id));
	hal_write32(0x0U, base_va + HIF_ABS_INT_TIMER_CHn(channel_id));

	if ((0U == cycles) && (0U == frames))
	{
		/* Remain coalesce disabled */
		ret = EOK;
	}
	else
	{
		/* Frame based coalescing */
		if (0U < frames)
		{
			/* Enable frame-based coalescing */
			/* NOTE: Frame based coalescing is unsupported on S32G2 silicon */
			hal_write32(frames, base_va + HIF_ABS_FRAME_COUNT_CHn(channel_id));
			hal_write32(HIF_INT_COAL_FRAME_ENABLE, base_va + HIF_INT_COAL_EN_CHn(channel_id));
			ret = EOK;
		}

		/* Timer-based coalescing */
		if (0U < cycles)
		{
			/* Enable timer-based coalescing */
			hal_write32(cycles, base_va + HIF_ABS_INT_TIMER_CHn(channel_id));
			reg = hal_read32(base_va + HIF_INT_COAL_EN_CHn(channel_id));
			hal_write32(HIF_INT_COAL_TIME_ENABLE | reg, base_va + HIF_INT_COAL_EN_CHn(channel_id));
			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Get number of transmitted packets
 * @param[in]	base_va Base address of channel register space (virtual)
 * @param[in]	channel_id 	Channel identifier
 * @return		Number of transmitted packets
 */
uint32_t pfe_hif_chnl_cfg_get_tx_cnt(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_RX_PKT_CNT1_CHn(channel_id));
}

/**
 * @brief		Get number of received packets
 * @param[in]	base_va Base address of channel register space (virtual)
 * @param[in]	channel_id 	Channel identifier
 * @return		Number of received packets
 */
uint32_t pfe_hif_chnl_cfg_get_rx_cnt(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_TX_PKT_CNT2_CHn(channel_id));
}

/**
 * @brief		Set HIF channel LTC setting
 * @details		The LTC setting for HIF channel.
 * 				WARNING: Hijacked use for Master detect feature
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 * @param[in]	val The 8bits value
 */
errno_t pfe_hif_chnl_cfg_ltc_set(addr_t base_va, uint32_t channel_id, uint8_t val)
{
	hal_write32(val, base_va + HIF_LTC_MAX_PKT_CHn_ADDR(channel_id));
	return EOK;
}

/**
 * @brief		Get HIF channel LTC setting
 * @details		Read the LTC setting for HIF channel.
 * 				WARNING: Hijacked use for Master detect feature
 * @param[in]	base_va Base address of HIF channel register space (virtual)
 * @param[in]	channel_id Channel identifier
 */
uint32_t pfe_hif_chnl_cfg_ltc_get(addr_t base_va, uint32_t channel_id)
{
	return hal_read32(base_va + HIF_LTC_MAX_PKT_CHn_ADDR(channel_id));
}

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)

/**
 * @brief		Get HIF channel statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about a HIF channel.
 * @param[in]	base_va 	Base address of channel register space (virtual)
 * @param[in]	channel_id 	Channel identifier
 * @param[in]	seq 		Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level number of data written to the buffer (0:less 1:more)
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_hif_chnl_cfg_get_text_stat(addr_t base_va, uint32_t channel_id, struct seq_file *seq, uint8_t verb_level)
{
	uint32_t reg;

	seq_printf(seq, "[CHANNEL %d]\n", channel_id);
	reg = hal_read32(base_va + HIF_RX_STATUS_0_CHn(channel_id));
	seq_printf(seq, "HIF_RX_STATUS_0           : 0x%x\n", reg);
	reg = hal_read32(base_va + HIF_RX_DMA_STATUS_0_CHn(channel_id));
	seq_printf(seq, "HIF_RX_DMA_STATUS_0       : 0x%x\n", reg);
	reg = hal_read32(base_va + HIF_RX_PKT_CNT0_CHn(channel_id));
	seq_printf(seq, "HIF_RX_PKT_CNT0           : 0x%x\n", reg);
	reg = hal_read32(base_va + HIF_RX_PKT_CNT1_CHn(channel_id));
	seq_printf(seq, "HIF_RX_PKT_CNT1           : 0x%x\n", reg);

	reg = hal_read32(base_va + HIF_TX_STATUS_0_CHn(channel_id));
	seq_printf(seq, "HIF_TX_STATUS_0           : 0x%x\n", reg);
	reg = hal_read32(base_va + HIF_TX_STATUS_1_CHn(channel_id));
	seq_printf(seq, "HIF_TX_STATUS_1           : 0x%x\n", reg);
	reg = hal_read32(base_va + HIF_TX_DMA_STATUS_0_CHn(channel_id));
	seq_printf(seq, "HIF_TX_DMA_STATUS_0       : 0x%x\n", reg);
	reg = hal_read32(base_va + HIF_TX_PKT_CNT0_CHn(channel_id));
	seq_printf(seq, "HIF_TX_PKT_CNT0           : 0x%x\n", reg);
	reg = hal_read32(base_va + HIF_TX_PKT_CNT1_CHn(channel_id));
	seq_printf(seq, "HIF_TX_PKT_CNT1           : 0x%x\n", reg);

	return 0;
}

/**
 * @brief		Get HIF statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the HIF block.
 * @param[in]	base_va 	Base address of HIF register space (virtual)
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_hif_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level)
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
		/* Debug registers */
		if(verb_level >= 10U)
		{
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_STATE               : 0x%x\n", hal_read32(base_va + HIF_TX_STATE));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_ACTV                : 0x%x\n", hal_read32(base_va + HIF_TX_ACTV));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_CURR_CH_NO          : 0x%x\n", hal_read32(base_va + HIF_TX_CURR_CH_NO));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_DXR_TX_FIFO_CNT        : 0x%x\n", hal_read32(base_va + HIF_DXR_TX_FIFO_CNT));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_CTRL_WORD_FIFO_CNT1 : 0x%x\n", hal_read32(base_va + HIF_TX_CTRL_WORD_FIFO_CNT1));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_CTRL_WORD_FIFO_CNT2 : 0x%x\n", hal_read32(base_va + HIF_TX_CTRL_WORD_FIFO_CNT2));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_BVALID_FIFO_CNT     : 0x%x\n", hal_read32(base_va + HIF_TX_BVALID_FIFO_CNT));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_PKT_CNT1            : 0x%x\n", hal_read32(base_va + HIF_TX_PKT_CNT1));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_PKT_CNT2            : 0x%x\n", hal_read32(base_va + HIF_TX_PKT_CNT2));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_STATE               : 0x%x\n", hal_read32(base_va + HIF_RX_STATE));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_ACTV                : 0x%x\n", hal_read32(base_va + HIF_RX_ACTV));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_CURR_CH_NO          : 0x%x\n", hal_read32(base_va + HIF_RX_CURR_CH_NO));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_DXR_RX_FIFO_CNT        : 0x%x\n", hal_read32(base_va + HIF_DXR_RX_FIFO_CNT));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_CTRL_WORD_FIFO_CNT  : 0x%x\n", hal_read32(base_va + HIF_RX_CTRL_WORD_FIFO_CNT));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_BVALID_FIFO_CNT     : 0x%x\n", hal_read32(base_va + HIF_RX_BVALID_FIFO_CNT));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_PKT_CNT1            : 0x%x\n", hal_read32(base_va + HIF_RX_PKT_CNT1));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_PKT_CNT2            : 0x%x\n", hal_read32(base_va + HIF_RX_PKT_CNT2));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_CH0_INT_SRC:        : 0x%x\n", hal_read32(base_va + HIF_CH0_INT_SRC));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_BDP_CH0_TX_FIFO_CNT : 0x%x\n", hal_read32(base_va + HIF_BDP_CH0_TX_FIFO_CNT));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_DMA_STATUS_0_CH0 : 0x%x\n", hal_read32(base_va + HIF_TX_DMA_STATUS_0_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_STATUS_0_CH0     : 0x%x\n", hal_read32(base_va + HIF_TX_STATUS_0_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_STATUS_1_CH0     : 0x%x\n", hal_read32(base_va + HIF_TX_STATUS_1_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_PKT_CNT0_CH0     : 0x%x\n", hal_read32(base_va + HIF_TX_PKT_CNT0_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_PKT_CNT1_CH0     : 0x%x\n", hal_read32(base_va + HIF_TX_PKT_CNT1_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_TX_PKT_CNT2_CH0     : 0x%x\n", hal_read32(base_va + HIF_TX_PKT_CNT2_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_BDP_CH0_RX_FIFO_CNT : 0x%x\n", hal_read32(base_va + HIF_BDP_CH0_RX_FIFO_CNT));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_DMA_STATUS_0_CH0 : 0x%x\n", hal_read32(base_va + HIF_RX_DMA_STATUS_0_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_STATUS_0_CH0     : 0x%x\n", hal_read32(base_va + HIF_RX_STATUS_0_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_PKT_CNT0_CH0     : 0x%x\n", hal_read32(base_va + HIF_RX_PKT_CNT0_CH0));
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_PKT_CNT1_CH0     : 0x%x\n", hal_read32(base_va + HIF_RX_PKT_CNT1_CH0));
		}

		if(verb_level>=9U)
		{
			/*	Get version */
			reg = hal_read32(base_va + HIF_VERSION);
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Revision                  : 0x%x\n", (reg >> 24U) & 0xffU);
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "Version                   : 0x%x\n", (reg >> 16U) & 0xffU);
			len += (uint32_t)oal_util_snprintf(buf + len, size - len, "ID                        : 0x%x\n", reg & 0xffffU);
		}

		reg = hal_read32(base_va + HIF_RX_STATE);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_STATE              : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_RX_ACTV);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_ACTV               : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_RX_CURR_CH_NO);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_CURR_CH_NO         : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_DXR_RX_FIFO_CNT);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_DXR_RX_FIFO_CNT       : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_RX_CTRL_WORD_FIFO_CNT);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_CTRL_WORD_FIFO_CNT : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_RX_BVALID_FIFO_CNT);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_BVALID_FIFO_CNT    : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_RX_BVALID_FIFO_CNT);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_BVALID_FIFO_CNT    : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_RX_PKT_CNT1);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_PKT_CNT1           : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_RX_PKT_CNT2);
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_RX_PKT_CNT2           : 0x%x\n", reg);

		reg = hal_read32(base_va + HIF_INT_SRC);
		len += oal_util_snprintf(buf + len, size - len, "HIF_INT_SRC               : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_ERR_INT_SRC);
		len += oal_util_snprintf(buf + len, size - len, "HIF_ERR_INT_SRC           : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_FIFO_ERR_INT_SRC);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_FIFO_ERR_INT_SRC   : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_RX_FIFO_ERR_INT_SRC);
		len += oal_util_snprintf(buf + len, size - len, "HIF_RX_FIFO_ERR_INT_SRC   : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_STATE);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_STATE              : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_ACTV);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_ACTV               : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_CURR_CH_NO);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_CURR_CH_NO         : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_DXR_TX_FIFO_CNT);
		len += oal_util_snprintf(buf + len, size - len, "HIF_DXR_TX_FIFO_CNT       : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_CTRL_WORD_FIFO_CNT1);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_CTRL_WORD_FIFO_CNT1: 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_CTRL_WORD_FIFO_CNT2);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_CTRL_WORD_FIFO_CNT2: 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_BVALID_FIFO_CNT);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_BVALID_FIFO_CNT    : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_PKT_CNT1);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_PKT_CNT1           : 0x%x\n", reg);
		reg = hal_read32(base_va + HIF_TX_PKT_CNT2);
		len += oal_util_snprintf(buf + len, size - len, "HIF_TX_PKT_CNT2           : 0x%x\n", reg);

		dump_hif_channel(base_va, 0U);
	}

	return len;
}

#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

/**
 * @brief		HIF error detect in polling
 * @details		check and return the interrupt src in polling mode
 * @param[in]	base_va Base address of HIF register space (virtual)
 * @return		return HIF error interrupt source
 * @note		This is polling function and will be called to check the error status of HIF
 * 				Make sure the call is protected by some per-HIF mutex
 */
uint32_t pfe_hif_cfg_get_err_poll(addr_t base_va)
{
	uint32_t int_src;
	uint32_t reg_src;

	int_src = hal_read32(base_va + HIF_INT_SRC);

	if(0U != int_src)
	{
		if (0U != (int_src & HIF_INT_SRC_HIF_ERR_INT))
		{
			/*	Get interrupt src */
			reg_src = hal_read32(base_va + HIF_ERR_INT_SRC);
			/*	Write 1 to clear */
			hal_write32(reg_src, base_va + HIF_ERR_INT_SRC);

			if(0U != (reg_src & HIF_ERR_INT_SRC))
			{
				NXP_LOG_INFO("HIF_ERR_INT (0x%x)\n", (uint_t)reg_src);
			}
		}

		if (0U != (int_src & HIF_INT_SRC_HIF_TX_FIFO_ERR_INT))
		{
			/*	Get interrupt src */
			reg_src = hal_read32(base_va + HIF_TX_FIFO_ERR_INT_SRC);
			/*	Write 1 to clear */
			hal_write32(reg_src, base_va + HIF_TX_FIFO_ERR_INT_SRC);

			if(0U != (reg_src & HIF_ERR_INT_SRC))
			{
				NXP_LOG_INFO("HIF_TX_FIFO_ERR_INT (0x%x)\n", (uint_t)reg_src);
			}
		}

		if (0U != (int_src & HIF_INT_SRC_HIF_RX_FIFO_ERR_INT))
		{
			/*	Get interrupt src */
			reg_src = hal_read32(base_va + HIF_RX_FIFO_ERR_INT_SRC);
			/*	Write 1 to clear */
			hal_write32(reg_src, base_va + HIF_RX_FIFO_ERR_INT_SRC);

			if(0U != (reg_src & HIF_ERR_INT_SRC))
			{
				NXP_LOG_INFO("HIF_RX_FIFO_ERR_INT (0x%x)\n", (uint_t)reg_src);
			}
		}
	}

	return int_src;
}
#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

