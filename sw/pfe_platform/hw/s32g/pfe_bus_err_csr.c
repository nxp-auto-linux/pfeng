/* =========================================================================
 *  
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_hm.h"
#include "pfe_cbus.h"
#include "pfe_bus_err_csr.h"

#define BUS_ERR_INT_SRC_NUMBER   20U
#define TRIG_EN_INTERRUPTS_CHECK   (M1_BUS_RD_ERR_INT | M2_BUS_WR_ERR_INT | M3_BUS_WR_ERR_INT | M4_BUS_RD_ERR_INT | \
									HGPI_BUS_RD_ERR_INT | HGPI_BUS_WR_ERR_INT | EGPI0_BUS_RD_ERR_INT | EGPI0_BUS_WR_ERR_INT | \
									EGPI1_BUS_RD_ERR_INT | EGPI1_BUS_WR_ERR_INT | EGPI2_BUS_RD_ERR_INT | EGPI2_BUS_WR_ERR_INT | \
									CLASS_BUS_RD_ERR_INT | CLASS_BUS_WR_ERR_INT | HIF_NOCPY_BUS_RD_ERR_INT | HIF_NOCPY_BUS_WR_ERR_INT | \
									TMU_BUS_RD_ERR_INT | FET_BUS_RD_ERR_INT | UPE_BUS_RD_ERR_INT | UPE_BUS_WR_ERR_INT \
								   )

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		BUS_ERR ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * @param[in]	base_va BUS_ERR register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 */
errno_t pfe_bus_err_cfg_isr(addr_t base_va)
{
	uint32_t reg_en, reg_src;
	errno_t ret = ENOENT;
	uint32_t trig_en_interrupts;
	uint8_t index = 0U;

	static const pfe_hm_evt_t event_id[BUS_ERR_INT_SRC_NUMBER] =
	{
		HM_EVT_BUS_MASTER1,
		HM_EVT_BUS_MASTER2,
		HM_EVT_BUS_MASTER3,
		HM_EVT_BUS_MASTER4,
		HM_EVT_BUS_HGPI_READ,
		HM_EVT_BUS_HGPI_WRITE,
		HM_EVT_BUS_EMAC0_READ,
		HM_EVT_BUS_EMAC0_WRITE,
		HM_EVT_BUS_EMAC1_READ,
		HM_EVT_BUS_EMAC1_WRITE,
		HM_EVT_BUS_EMAC2_READ,
		HM_EVT_BUS_EMAC2_WRITE,
		HM_EVT_BUS_CLASS_READ,
		HM_EVT_BUS_CLASS_WRITE,
		HM_EVT_BUS_HIF_NOCPY_READ,
		HM_EVT_BUS_HIF_NOCPY_WRITE,
		HM_EVT_BUS_TMU,
		HM_EVT_BUS_FET,
		HM_EVT_BUS_UTIL_PE_READ,
		HM_EVT_BUS_UTIL_PE_WRITE,
	};

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + WSP_BUS_ERR_INT_EN);
	/* Mask bus error interrupts */
	hal_write32((reg_en & ~(BUS_ERR_INT_EN)), base_va + WSP_BUS_ERR_INT_EN);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + WSP_BUS_ERR_INT_SRC);
	/*	ACK triggered interrupts*/
	hal_write32(reg_src, base_va + WSP_BUS_ERR_INT_SRC);

	/* Process interrupts which are triggered AND enabled */
	trig_en_interrupts = reg_src & reg_en & TRIG_EN_INTERRUPTS_CHECK;
	if (0U != trig_en_interrupts)
	{
		trig_en_interrupts >>= 1U;
		while (0U != trig_en_interrupts)
		{
			if (0U != (trig_en_interrupts & 1UL))
			{
				pfe_hm_report_error(HM_SRC_BUS, event_id[index], "");
			}
			trig_en_interrupts >>= 1U;
			index++;
		}
		ret = EOK;
	}

	/*	Enable the non-triggered ones only to prevent flooding */
	hal_write32((reg_en & ~reg_src), base_va + WSP_BUS_ERR_INT_EN);

	return ret;
}

/**
 * @brief		Mask BUS_ERR interrupts
 * @param[in]	base_va Base address of the BUS_ERR register space
 */
void pfe_bus_err_cfg_irq_mask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_BUS_ERR_INT_EN) & ~(BUS_ERR_INT_EN);
	hal_write32(reg, base_va + WSP_BUS_ERR_INT_EN);
}

/**
 * @brief		Unmask BUS_ERR interrupts
 * @param[in]	base_va Base address of the BUS_ERR register space
 */
void pfe_bus_err_cfg_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_BUS_ERR_INT_EN) | BUS_ERR_INT_EN;
	hal_write32(reg, base_va + WSP_BUS_ERR_INT_EN);
}

/**
 * @brief		Unmask all BUS_ERR interrupts
 * @param[in]	base_va Base address of the BUS_ERR register space
 * @note		This function is called from thread.
 */
void pfe_bus_err_cfg_irq_unmask_all(addr_t base_va)
{
	hal_write32(BUS_ERR_INT_ENABLE_ALL, base_va + WSP_BUS_ERR_INT_EN);
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
