/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2019-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_safety_csr.h"

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

#define WSP_SAFETY_INT_SRC_NUMBER   28U
#define TRIG_EN_INTERRUPTS_CHECK   (MASTER1_INT | MASTER2_INT | MASTER3_INT | MASTER4_INT | \
									EMAC_CBUS_INT | EMAC_DBUS_INT | CLASS_CBUS_INT | CLASS_DBUS_INT | \
									TMU_CBUS_INT | TMU_DBUS_INT | HIF_CBUS_INT | HIF_DBUS_INT | \
									HIF_NOCPY_CBUS_INT | HIF_NOCPY_DBUS_INT | UPE_CBUS_INT | UPE_DBUS_INT | \
									HRS_CBUS_INT | BRIDGE_CBUS_INT | EMAC_SLV_INT | BMU1_SLV_INT | \
									BMU2_SLV_INT | CLASS_SLV_INT | HIF_SLV_INT | HIF_NOCPY_SLV_INT | \
									LMEM_SLV_INT | TMU_SLV_INT | UPE_SLV_INT | WSP_GLOBAL_SLV_INT \
								   )

/**
 * @brief		SAFETY ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * @param[in]	base_va SAFETY register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 */
errno_t pfe_safety_cfg_isr(addr_t base_va)
{
	uint32_t reg_en, reg_src;
	errno_t ret = ENOENT;
	uint32_t trig_en_interrupts;
#ifdef NXP_LOG_ENABLED
	uint8_t index = 0U;
	const char_t * const wsp_safety_int_src_text[WSP_SAFETY_INT_SRC_NUMBER] = 
	{
		"MASTER1_INT-Master1 Parity error",
		"MASTER2_INT-Master2 Parity error",
		"MASTER3_INT-Master3 Parity error",
		"MASTER4_INT-Master4 Parity error",
		"EMAC_CBUS_INT-EMACX cbus parity error",
		"EMAC_DBUS_INT-EMACX dbus parity error",
		"CLASS_CBUS_INT-Class cbus parity error",
		"CLASS_DBUS_INT-Class dbus parity error",
		"TMU_CBUS_INT-TMU cbus parity error",
		"TMU_DBUS_INT-TMU dbus parity error",
		"HIF_CBUS_INT-HGPI cbus parity error",
		"HIF_DBUS_INT-HGPI dbus parity error",
		"HIF_NOCPY_CBUS_INT-HIF_NOCPY cbus parity error",
		"HIF_NOCPY_DBUS_INT-HIF_NOCPY dbus parity error",
		"UPE_CBUS_INT-UTIL_PE cbus parity error",
		"UPE_DBUS_INT-UTIL_PE dbus parity error",
		"HRS_CBUS_INT-HRS cbus parity error",
		"BRIDGE_CBUS_INT-BRIDGE cbus parity error",
		"EMAC_SLV_INT-EMACX slave parity error",
		"BMU1_SLV_INT-BMU1 slave parity error",
		"BMU2_SLV_INT-BMU2 slave parity error",
		"CLASS_SLV_INT-CLASS slave parity error",
		"HIF_SLV_INT-HIF slave parity error",
		"HIF_NOCPY_SLV_INT-HIF_NOCPY slave parity error",
		"LMEM_SLV_INT-LMEM slave parity error",
		"TMU_SLV_INT-TMU slave parity error",
		"UPE_SLV_INT-UTIL_PE slave parity error",
		"WSP_GLOBAL_SLV_INT-WSP_GLOBAL slave parity error"
	};
#endif /* NXP_LOG_ENABLED */

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + WSP_SAFETY_INT_EN);
	/* Mask safety interrupts */
	hal_write32((reg_en & ~(SAFETY_INT_EN)), base_va + WSP_SAFETY_INT_EN);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + WSP_SAFETY_INT_SRC);
	/*	ACK triggered interrupts*/
	hal_write32(reg_src, base_va + WSP_SAFETY_INT_SRC);

	/* Process interrupts which are triggered AND enabled */
	trig_en_interrupts = reg_src & reg_en & TRIG_EN_INTERRUPTS_CHECK;
	if (0U != trig_en_interrupts)
	{
#ifdef NXP_LOG_ENABLED
		trig_en_interrupts >>= 1U;
		while (0U != trig_en_interrupts)
		{
			if (0U != (trig_en_interrupts && 1UL))
			{
				NXP_LOG_INFO("%s\n", wsp_safety_int_src_text[index]);
			}
			trig_en_interrupts >>= 1U;
			index++;
		}
#endif /* NXP_LOG_ENABLED */    
		ret = EOK;
	}

	/*	Enable the non-triggered ones only to prevent flooding */
	hal_write32((reg_en & ~reg_src), base_va + WSP_SAFETY_INT_EN);

	return ret;
}

/**
 * @brief		Mask SAFETY interrupts
 * @param[in]	base_va Base address of the SAFETY register space
 */
void pfe_safety_cfg_irq_mask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_SAFETY_INT_EN) & ~(SAFETY_INT_EN);
	hal_write32(reg, base_va + WSP_SAFETY_INT_EN);
}

/**
 * @brief		Unmask SAFETY interrupts
 * @param[in]	base_va Base address of the SAFETY register space
 */
void pfe_safety_cfg_irq_unmask(addr_t base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_SAFETY_INT_EN) | SAFETY_INT_EN;
	hal_write32(reg, base_va + WSP_SAFETY_INT_EN);
}

/**
 * @brief		Unmask all SAFETY interrupts
 * @param[in]	base_va Base address of the SAFETY register space
 * @note		This function is called from thread.
 */
void pfe_safety_cfg_irq_unmask_all(addr_t base_va)
{
	hal_write32(SAFETY_INT_ENABLE_ALL, base_va + WSP_SAFETY_INT_EN);
}
