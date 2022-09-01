/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
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
#include "pfe_fw_fail_stop_csr.h"

#define TRIG_EN_INTERRUPTS_CHECK	(FW_FAIL_STOP_INT | FW_FAIL_STOP_MODE_INT)

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		FW_FAIL_STOP ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * @param[in]	base_va FW_FAIL_STOP register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 */
errno_t pfe_fw_fail_stop_cfg_isr(addr_t base_va)
{
	uint32_t reg_en, reg_src;
	errno_t ret = ENOENT;
	uint32_t trig_en_interrupts;

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + WSP_FW_FAIL_STOP_MODE_INT_EN);
	/* Mask FW Failstop interrupts */
	hal_write32((reg_en & ~(FW_FAIL_STOP_INT_EN)), base_va + WSP_FW_FAIL_STOP_MODE_INT_EN);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + WSP_FW_FAIL_STOP_MODE_INT_SRC);
	/*	ACK triggered interrupts*/
	hal_write32(reg_src, base_va + WSP_FW_FAIL_STOP_MODE_INT_SRC);

	/* Process interrupts which are triggered AND enabled */
	trig_en_interrupts = reg_src & reg_en & TRIG_EN_INTERRUPTS_CHECK;
	if (0U != trig_en_interrupts)
	{
		pfe_hm_report_error(HM_SRC_FW_FAIL_STOP, HM_EVT_FW_FAIL_STOP, "");
		ret = EOK;
	}

	/*	Enable the non-triggered ones only to prevent flooding */
	hal_write32((reg_en & ~reg_src), base_va + WSP_FW_FAIL_STOP_MODE_INT_EN);

	return ret;
}

/**
 * @brief		Mask FW_FAIL_STOP interrupts
 * @param[in]	base_va Base address of the FW_FAIL_STOP register space
 */
void pfe_fw_fail_stop_cfg_irq_mask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_FW_FAIL_STOP_MODE_INT_EN) & ~(FW_FAIL_STOP_INT_EN);
	hal_write32(reg, base_va + WSP_FW_FAIL_STOP_MODE_INT_EN);
}

/**
 * @brief		Unmask FW_FAIL_STOP interrupts
 * @param[in]	base_va Base address of the FW_FAIL_STOP register space
 */
void pfe_fw_fail_stop_cfg_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_FW_FAIL_STOP_MODE_INT_EN) | FW_FAIL_STOP_INT_EN;
	hal_write32(reg, base_va + WSP_FW_FAIL_STOP_MODE_INT_EN);
}

/**
 * @brief		Unmask all FW_FAIL_STOP interrupts
 * @param[in]	base_va Base address of the FW_FAIL_STOP register space
 * @note		This function is called from thread.
 */
void pfe_fw_fail_stop_cfg_irq_unmask_all(addr_t base_va)
{
	hal_write32(FW_FAIL_STOP_INT_ENABLE_ALL, base_va + WSP_FW_FAIL_STOP_MODE_INT_EN);
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
