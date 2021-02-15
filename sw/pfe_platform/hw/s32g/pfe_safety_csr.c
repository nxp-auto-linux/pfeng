/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2019-2020 NXP
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

/**
 * @brief		SAFETY ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * @param[in]	base_va SAFETY register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 */
errno_t pfe_safety_cfg_isr(void *base_va)
{
	uint32_t reg_en, reg_src;
	errno_t ret = ENOENT;

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + WSP_SAFETY_INT_EN);
	/* Mask safety interrupts */
	hal_write32((reg_en & ~(SAFETY_INT_EN)), base_va + WSP_SAFETY_INT_EN);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + WSP_SAFETY_INT_SRC);
	/*	ACK triggered interrupts*/
	hal_write32(reg_src, base_va + WSP_SAFETY_INT_SRC);

	/* Process interrupts which are triggered AND enabled */
	if ((reg_src & reg_en & MASTER1_INT) != 0U)
	{
		NXP_LOG_INFO("MASTER1_INT-Master1 Parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & MASTER2_INT) != 0U)
	{
		NXP_LOG_INFO("MASTER2_INT-Master2 Parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & MASTER3_INT) != 0U)
	{
		NXP_LOG_INFO("MASTER3_INT-Master3 Parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & MASTER4_INT) != 0U)
	{
		NXP_LOG_INFO("MASTER4_INT-Master4 Parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & EMAC_CBUS_INT) != 0U)
	{
		NXP_LOG_INFO("EMAC_CBUS_INT-EMACX cbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & EMAC_DBUS_INT) != 0U)
	{
		NXP_LOG_INFO("EMAC_DBUS_INT-EMACX dbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & CLASS_CBUS_INT) != 0U)
	{
		NXP_LOG_INFO("CLASS_CBUS_INT-Class cbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & CLASS_DBUS_INT) != 0U)
	{
		NXP_LOG_INFO("CLASS_DBUS_INT-Class dbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & TMU_CBUS_INT) != 0U)
	{
		NXP_LOG_INFO("TMU_CBUS_INT-TMU cbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & TMU_DBUS_INT) != 0U)
	{
		NXP_LOG_INFO("TMU_DBUS_INT-TMU dbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & HIF_CBUS_INT) != 0U)
	{
		NXP_LOG_INFO("HIF_CBUS_INT-HGPI cbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & HIF_DBUS_INT) != 0U)
	{
		NXP_LOG_INFO("HIF_DBUS_INT-HGPI dbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & HIF_NOCPY_CBUS_INT) != 0U)
	{
		NXP_LOG_INFO("HIF_NOCPY_CBUS_INT-HIF_NOCPY cbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & HIF_NOCPY_DBUS_INT) != 0U)
	{
		NXP_LOG_INFO("HIF_NOCPY_DBUS_INT-HIF_NOCPY dbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & UPE_CBUS_INT) != 0U)
	{
		NXP_LOG_INFO("UPE_CBUS_INT-UTIL_PE cbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & UPE_DBUS_INT) != 0U)
	{
		NXP_LOG_INFO("UPE_DBUS_INT-UTIL_PE dbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & HRS_CBUS_INT) != 0U)
	{
		NXP_LOG_INFO("HRS_CBUS_INT-HRS cbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & BRIDGE_CBUS_INT) != 0U)
	{
		NXP_LOG_INFO("BRIDGE_CBUS_INT-BRIDGE cbus parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & EMAC_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("EMAC_SLV_INT-EMACX slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & BMU1_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("BMU1_SLV_INT-BMU1 slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & BMU2_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("BMU2_SLV_INT-BMU2 slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & CLASS_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("CLASS_SLV_INT-CLASS slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & HIF_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("HIF_SLV_INT-HIF slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & HIF_NOCPY_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("HIF_NOCPY_SLV_INT-HIF_NOCPY slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & LMEM_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("LMEM_SLV_INT-LMEM slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & TMU_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("TMU_SLV_INT-TMU slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & UPE_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("UPE_SLV_INT-UTIL_PE slave parity error\n");
		ret = EOK;
	}

	if ((reg_src & reg_en & WSP_GLOBAL_SLV_INT) != 0U)
	{
		NXP_LOG_INFO("WSP_GLOBAL_SLV_INT-WSP_GLOBAL slave parity error\n");
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
void pfe_safety_cfg_irq_mask(void *base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WSP_SAFETY_INT_EN) & ~(SAFETY_INT_EN);
	hal_write32(reg, base_va + WSP_SAFETY_INT_EN);
}

/**
 * @brief		Unmask SAFETY interrupts
 * @param[in]	base_va Base address of the SAFETY register space
 */
void pfe_safety_cfg_irq_unmask(void *base_va)
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
void pfe_safety_cfg_irq_unmask_all(void *base_va)
{
	hal_write32(SAFETY_INT_ENABLE_ALL, base_va + WSP_SAFETY_INT_EN);
}
