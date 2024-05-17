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
#include "pfe_fail_stop_csr.h"

#define FAIL_STOP_INT_SRC_NUMBER   6U
#define TRIG_EN_INTERRUPTS_CHECK   (PARITY_FS_INTERRUPT | WDT_FS_INTERRUPT | BUS_ERR_FS_INTERRUPT | ECC_FS_INTERRUPT | FW_FAIL_STOP_FS_INTERRUPT | HOST_FORCE_DEBUG_FAIL_STOP_FS_INTERRUPT)

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		FAIL_STOP ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * @param[in]	base_va FAIL_STOP register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 */
errno_t pfe_fail_stop_cfg_isr(addr_t base_va)
{
	uint32_t reg_en, reg_src, reg_source;
	errno_t ret = ENOENT;
	uint32_t trig_en_interrupts = 0U;
	uint8_t index = 0U;
	static const pfe_hm_evt_t event_id[FAIL_STOP_INT_SRC_NUMBER] =
	{
		HM_EVT_FAIL_STOP_PARITY,
		HM_EVT_FAIL_STOP_WATCHDOG,
		HM_EVT_FAIL_STOP_BUS,
		HM_EVT_FAIL_STOP_ECC_MULTIBIT,
		HM_EVT_FAIL_STOP_FW,
		HM_EVT_FAIL_STOP_HOST,
	};

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + WSP_FAIL_STOP_MODE_INT_EN);
	/* Mask Fail Stop interrupts */
	hal_write32((reg_en & ~(FAIL_STOP_INT_EN)), base_va + WSP_FAIL_STOP_MODE_INT_EN);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + WSP_FAIL_STOP_MODE_INT_SRC);
	if (reg_src & reg_en & FAIL_STOP_INT_ENABLE_ALL)
	{
		reg_source = hal_read32(base_va + WSP_FAILSTOP_INTERRUPT_SOURCE) & \
						hal_read32(base_va + WSP_FAIL_STOP_MODE_EN);
	}
	else
	{
		reg_source = 0U;
	}

	/* Process interrupts which are triggered AND enabled */
	if (0U != reg_source)
	{
		trig_en_interrupts = reg_source;
		while (0U != trig_en_interrupts)
		{
			if (0U != (trig_en_interrupts & 1UL))
			{
				pfe_hm_report_error(HM_SRC_FAIL_STOP, event_id[index], "");
			}
			trig_en_interrupts >>= 1U;
			index++;
		}
		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Mask FAIL_STOP interrupts
 * @param[in]	base_va Base address of the FAIL_STOP register space
 */
void pfe_fail_stop_cfg_irq_mask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_FAIL_STOP_MODE_INT_EN) & ~(FAIL_STOP_INT_EN);
	hal_write32(reg, base_va + WSP_FAIL_STOP_MODE_INT_EN);
}

/**
 * @brief		Unmask FAIL_STOP interrupts
 * @param[in]	base_va Base address of the FAIL_STOP register space
 */
void pfe_fail_stop_cfg_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_FAIL_STOP_MODE_INT_EN) | FAIL_STOP_INT_EN;
	hal_write32(reg, base_va + WSP_FAIL_STOP_MODE_INT_EN);
}

/**
 * @brief		Unmask all FAIL_STOP interrupts
 * @param[in]	base_va Base address of the FAIL_STOP register space
 * @note		This function is called from thread.
 */
void pfe_fail_stop_cfg_irq_unmask_all(addr_t base_va)
{
	hal_write32(FAIL_STOP_INT_ENABLE_ALL, base_va + WSP_FAIL_STOP_MODE_INT_EN);
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
