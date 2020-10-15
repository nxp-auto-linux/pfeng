/* =========================================================================
 *  Copyright 2020 NXP
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

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_wdt_csr.h"

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

/**
 * @brief		WDT ISR
 * @details		MASK, ACK, and process triggered interrupts.
 * 				Every WDT instance has its own handler. Access to registers is
 * 				protected by mutex implemented within the WDT module (pfe_wdt.c).
 * @param[in]	base_va WDT register space base address (virtual)
 * @param[in]	cbus_base_va CBUS base address (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 * @note		Make sure the call is protected by some per-BMU mutex
 */
errno_t pfe_wdt_cfg_isr(void *base_va, void *cbus_base_va)
{
	uint32_t reg_en, reg_src, reg_reen = 0U;
	errno_t ret = ENOENT;

	(void)cbus_base_va;

	/*	Get enabled interrupts */
	reg_en = hal_read32(base_va + WDT_INT_EN);
	/*	Mask ALL interrupts */
	hal_write32(0U, base_va + WDT_INT_EN);
	/*	Get triggered interrupts */
	reg_src = hal_read32(base_va + WDT_INT_SRC);
	/*	ACK triggered */
	hal_write32(reg_src, base_va + WDT_INT_SRC);

	/*	Process interrupts which are triggered AND enabled */
	if ((reg_src & WDT_BMU1_WDT_INT) && (reg_en & WDT_BMU1_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_BMU1_WDT_INT\n");
		reg_reen |= WDT_BMU1_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_BMU2_WDT_INT) && (reg_en & WDT_BMU2_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_BMU2_WDT_INT\n");
		reg_reen |= WDT_BMU2_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_CLASS_WDT_INT) && (reg_en & WDT_CLASS_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_CLASS_WDT_INT\n");
		reg_reen |= WDT_CLASS_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_EMAC0_GPI_WDT_INT) && (reg_en & WDT_EMAC0_GPI_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_EMAC0_GPI_WDT_INT\n");
		reg_reen |= WDT_EMAC0_GPI_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_EMAC1_GPI_WDT_INT) && (reg_en & WDT_EMAC1_GPI_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_EMAC1_GPI_WDT_INT\n");
		reg_reen |= WDT_EMAC1_GPI_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_EMAC2_GPI_WDT_INT) && (reg_en & WDT_EMAC2_GPI_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_EMAC2_GPI_WDT_INT\n");
		reg_reen |= WDT_EMAC2_GPI_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_HIF_GPI_WDT_INT) && (reg_en & WDT_HIF_GPI_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_HIF_GPI_WDT_INT\n");
		reg_reen |= WDT_HIF_GPI_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_HIF_NOCPY_WDT_INT) && (reg_en & WDT_HIF_NOCPY_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_HIF_NOCPY_WDT_INT\n");
		reg_reen |= WDT_HIF_NOCPY_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_HIF_WDT_INT) && (reg_en & WDT_HIF_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_HIF_WDT_INT\n");
		reg_reen |= WDT_HIF_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_TLITE_WDT_INT) && (reg_en & WDT_TLITE_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_TLITE_WDT_INT\n");
		reg_reen |= WDT_TLITE_WDT_INT_EN_BIT;
		ret = EOK;
	}

	if ((reg_src & WDT_UTIL_WDT_INT) && (reg_en & WDT_UTIL_PE_WDT_INT_EN_BIT))
	{
		NXP_LOG_INFO("WDT_UTIL_WDT_INT\n");
		reg_reen |= WDT_UTIL_PE_WDT_INT_EN_BIT;
		ret = EOK;
	}

	/*	Don't re-enable triggered ones since they can't be cleared until PFE
		is reset. Also don't reset master enable bit which is controlled
		by dedicated API (pfe_wdt_cfg_irq_mask/pfe_wdt_cfg_irq_unmask). */
	hal_write32((reg_en & ~reg_reen), base_va + WDT_INT_EN); /*	Enable the non-triggered ones only */

	return ret;
}

/**
 * @brief		Mask WDT interrupts
 * @param[in]	base_va Base address of the WDT register space
 */
void pfe_wdt_cfg_irq_mask(void *base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WDT_INT_EN) & ~(WDT_INT_EN_BIT);
	hal_write32(reg, base_va + WDT_INT_EN);
}

/**
 * @brief		Unmask WDT interrupts
 * @param[in]	base_va Base address of the WDT register space
 */
void pfe_wdt_cfg_irq_unmask(void *base_va)
{
	uint32_t reg;

	reg = hal_read32(base_va + WDT_INT_EN) | WDT_INT_EN_BIT;
	hal_write32(reg, base_va + WDT_INT_EN);
}

/**
 * @brief		init WDT interrupts
 * @param[in]	base_va Base address of the wsp register space
 */
void pfe_wdt_cfg_init(void *base_va)
{
	uint32_t reg;

	/*	Disable the WDT interrupts */
	reg = hal_read32(base_va + WDT_INT_EN) & ~(WDT_INT_EN_BIT);
	hal_write32(reg, base_va + WDT_INT_EN);

	/*	Clear WDT interrupts */
	reg = hal_read32(base_va + WDT_INT_SRC);
	hal_write32(reg, base_va + WDT_INT_SRC);

	/*	Set default watchdog timer values. */
	/*	TODO: What are real values able to precisely reveal runtime stall? */
	hal_write32(0xFFFFFFFFU, base_va + WDT_TIMER_VAL_1);
	hal_write32(0xFFFFFFFFU, base_va + WDT_TIMER_VAL_2);
	hal_write32(0xFFFFFFFFU, base_va + WDT_TIMER_VAL_3);
	hal_write32(0xFFFFFFU, base_va + WDT_TIMER_VAL_4);

	/*	Enable ALL particular watchdogs */
	hal_write32(0xFFFFFFU, base_va + CLASS_WDT_INT_EN);
	hal_write32(0xFU, base_va + UPE_WDT_INT_EN);
	hal_write32(0x1FFU, base_va + HGPI_WDT_INT_EN);
	hal_write32(0xFU, base_va + HIF_WDT_INT_EN);
	hal_write32(0xFFFFFFU, base_va + TLITE_WDT_INT_EN);
	hal_write32(0x3FU, base_va + HNCPY_WDT_INT_EN);
	hal_write32(0xFU, base_va + BMU1_WDT_INT_EN);
	hal_write32(0xFU, base_va + BMU2_WDT_INT_EN);
	hal_write32(0x1FFU, base_va + EMAC0_WDT_INT_EN);
	hal_write32(0x1FFU, base_va + EMAC1_WDT_INT_EN);
	hal_write32(0x1FFU, base_va + EMAC2_WDT_INT_EN);

	/*	Enable WDT interrupts except of the global enable bit */
	hal_write32((0xffffffffU & ~(WDT_INT_EN_BIT)), base_va + WDT_INT_EN);
}

/**
 * @brief		Clear the WDT interrupt control and status registers
 * @param[in]	base_va Base address of HIF register space (virtual)
 */
void pfe_wdt_cfg_fini(void *base_va)
{
	uint32_t reg;

	/*	Disable and clear WDT interrupts */
	hal_write32(0x0U, base_va + WDT_INT_EN);
	reg = hal_read32(base_va + WDT_INT_SRC);
	hal_write32(reg, base_va + WDT_INT_SRC);
}

/**
 * @brief		Get WDT statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the WDT block.
 * @param[in]	base_va 	Base address of WDT register space (virtual)
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_wdt_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == base_va) || (NULL == char_t))
	{
		NXP_LOG_ERROR("NULL argument received (pfe_wdt_cfg_get_text_stat)\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if(verb_level >= 9U)
	{
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "base_va              : 0x%x\n", base_va);
		/*	Get version of wsp (wdt is part of wsp)*/
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WSP Version          : 0x%x\n", hal_read32(base_va + WSP_VERSION));
	}
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_INT_EN           : 0x%x\n", hal_read32(base_va + WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + CLASS_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "UPE_WDT_INT_EN       : 0x%x\n", hal_read32(base_va + UPE_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HGPI_WDT_INT_EN      : 0x%x\n", hal_read32(base_va + HGPI_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HIF_WDT_INT_EN       : 0x%x\n", hal_read32(base_va + HIF_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TLITE_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + TLITE_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "HNCPY_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + HNCPY_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "BMU1_WDT_INT_EN      : 0x%x\n", hal_read32(base_va + BMU1_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "BMU2_WDT_INT_EN      : 0x%x\n", hal_read32(base_va + BMU2_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "EMAC0_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + EMAC0_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "EMAC1_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + EMAC1_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "EMAC2_WDT_INT_EN     : 0x%x\n", hal_read32(base_va + EMAC2_WDT_INT_EN));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_INT_SRC          : 0x%x\n", hal_read32(base_va + WDT_INT_SRC));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_1      : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_1));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_2      : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_2));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_3      : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_3));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WDT_TIMER_VAL_4      : 0x%x\n", hal_read32(base_va + WDT_TIMER_VAL_4));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "WSP_DBUG_BUS1        : 0x%x\n", hal_read32(base_va + WSP_DBUG_BUS1));

	return len;
}
