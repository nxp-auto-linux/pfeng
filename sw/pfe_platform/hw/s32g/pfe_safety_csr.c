/* =========================================================================
 *  Copyright 2019 NXP
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
 * @addtogroup  dxgr_PFE_SAFETY
 * @{
 *
 * @file		pfe_safety_csr.c
 * @brief		The SAFETY module low-level API (s32g).
 * @details
 *
 */

#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_safety_csr.h"

/**
 * @brief		SAFETY ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * @param[in]	base_va SAFETY register space base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Interrupt which were triggered are masked here, it is periodically unmasked again in safety thread.
 *              This can be changed by modifying global configuration parameter GLOBAL_CFG_SAFETY_WORKER.
 */
errno_t pfe_safety_cfg_isr(void *base_va)
{
	uint32_t reg_en, reg_src, reg_reen = 0;
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
	if (reg_src & reg_en & MASTER1_INT)
	{
		NXP_LOG_INFO("MASTER1_INT-Master1 Parity error\n");
		reg_reen |= (MASTER1_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & MASTER2_INT)
	{
		NXP_LOG_INFO("MASTER2_INT-Master2 Parity error\n");
		reg_reen |= (MASTER2_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & MASTER3_INT)
	{
		NXP_LOG_INFO("MASTER3_INT-Master3 Parity error\n");
		reg_reen |= (MASTER3_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & MASTER4_INT)
	{
		NXP_LOG_INFO("MASTER4_INT-Master4 Parity error\n");
		reg_reen |= (MASTER4_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & EMAC_CBUS_INT)
	{
		NXP_LOG_INFO("EMAC_CBUS_INT-EMACX cbus parity error\n");
		reg_reen |= (EMAC_CBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & EMAC_DBUS_INT)
	{
		NXP_LOG_INFO("EMAC_DBUS_INT-EMACX dbus parity error\n");
		reg_reen |= (EMAC_DBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & CLASS_CBUS_INT)
	{
		NXP_LOG_INFO("CLASS_CBUS_INT-Class cbus parity error\n");
		reg_reen |= (CLASS_CBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & CLASS_DBUS_INT)
	{
		NXP_LOG_INFO("CLASS_DBUS_INT-Class dbus parity error\n");
		reg_reen |= (CLASS_DBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & TMU_CBUS_INT)
	{
		NXP_LOG_INFO("TMU_CBUS_INT-TMU cbus parity error\n");
		reg_reen |= (TMU_CBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & TMU_DBUS_INT)
	{
		NXP_LOG_INFO("TMU_DBUS_INT-TMU dbus parity error\n");
		reg_reen |= (TMU_DBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & HIF_CBUS_INT)
	{
		NXP_LOG_INFO("HIF_CBUS_INT-HGPI cbus parity error\n");
		reg_reen |= (HIF_CBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & HIF_DBUS_INT)
	{
		NXP_LOG_INFO("HIF_DBUS_INT-HGPI dbus parity error\n");
		reg_reen |= (HIF_DBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & HIF_NOCPY_CBUS_INT)
	{
		NXP_LOG_INFO("HIF_NOCPY_CBUS_INT-HIF_NOCPY cbus parity error\n");
		reg_reen |= (HIF_NOCPY_CBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & HIF_NOCPY_DBUS_INT)
	{
		NXP_LOG_INFO("HIF_NOCPY_DBUS_INT-HIF_NOCPY dbus parity error\n");
		reg_reen |= (HIF_NOCPY_DBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & UPE_CBUS_INT)
	{
		NXP_LOG_INFO("UPE_CBUS_INT-UTIL_PE cbus parity error\n");
		reg_reen |= (UPE_CBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & UPE_DBUS_INT)
	{
		NXP_LOG_INFO("UPE_DBUS_INT-UTIL_PE dbus parity error\n");
		reg_reen |= (UPE_DBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & HRS_CBUS_INT)
	{
		NXP_LOG_INFO("HRS_CBUS_INT-HRS cbus parity error\n");
		reg_reen |= (HRS_CBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & BRIDGE_CBUS_INT)
	{
		NXP_LOG_INFO("BRIDGE_CBUS_INT-BRIDGE cbus parity error\n");
		reg_reen |= (BRIDGE_CBUS_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & EMAC_SLV_INT)
	{
		NXP_LOG_INFO("EMAC_SLV_INT-EMACX slave parity error\n");
		reg_reen |= (EMAC_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & BMU1_SLV_INT)
	{
		NXP_LOG_INFO("BMU1_SLV_INT-BMU1 slave parity error\n");
		reg_reen |= (BMU1_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & BMU2_SLV_INT)
	{
		NXP_LOG_INFO("BMU2_SLV_INT-BMU2 slave parity error\n");
		reg_reen |= (BMU2_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & CLASS_SLV_INT)
	{
		NXP_LOG_INFO("CLASS_SLV_INT-CLASS slave parity error\n");
		reg_reen |= (CLASS_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & HIF_SLV_INT)
	{
		NXP_LOG_INFO("HIF_SLV_INT-HIF slave parity error\n");
		reg_reen |= (HIF_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & HIF_NOCPY_SLV_INT)
	{
		NXP_LOG_INFO("HIF_NOCPY_SLV_INT-HIF_NOCPY slave parity error\n");
		reg_reen |= (HIF_NOCPY_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & LMEM_SLV_INT)
	{
		NXP_LOG_INFO("LMEM_SLV_INT-LMEM slave parity error\n");
		reg_reen |= (LMEM_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & TMU_SLV_INT)
	{
		NXP_LOG_INFO("TMU_SLV_INT-TMU slave parity error\n");
		reg_reen |= (TMU_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & UPE_SLV_INT)
	{
		NXP_LOG_INFO("UPE_SLV_INT-UTIL_PE slave parity error\n");
		reg_reen |= (UPE_SLV_INT_EN);
		ret = EOK;
	}

	if (reg_src & reg_en & WSP_GLOBAL_SLV_INT)
	{
		NXP_LOG_INFO("WSP_GLOBAL_SLV_INT-WSP_GLOBAL slave parity error\n");
		reg_reen |= (WSP_GLOBAL_SLV_INT_EN);
		ret = EOK;
	}

	/*	Write changes to WSP_SAFETY_INT_EN register*/
#if defined(GLOBAL_CFG_SAFETY_WORKER)
	/* Triggered interrupts are masked here, unmasking is done in thread */
	hal_write32((reg_en & ~(reg_reen)), base_va + WSP_SAFETY_INT_EN);
#else
	/* Triggered interrupts are unmasked here */
	hal_write32((reg_en | reg_reen | SAFETY_INT_EN), base_va + WSP_SAFETY_INT_EN);
#endif /* GLOBAL_CFG_SAFETY_WORKER */

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

/** @}*/
