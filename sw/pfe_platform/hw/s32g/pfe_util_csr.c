/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_util_csr.h"

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

/**
 * @brief		Get UTIL statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the UTIL block.
 * @param[in]	base_va 	Base address of UTIL register space (virtual)
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_util_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U, reg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Get version */
	if(verb_level >= 9U)
	{
		reg = hal_read32(base_va + UTIL_VERSION);
		len += oal_util_snprintf(buf + len, size - len, "Revision             : 0x%x\n", (reg >> 24U) & 0xffU);
		len += oal_util_snprintf(buf + len, size - len, "Version              : 0x%x\n", (reg >> 16U) & 0xffU);
		len += oal_util_snprintf(buf + len, size - len, "ID                   : 0x%x\n", reg & 0xffffU);
	}

	len += oal_util_snprintf(buf + len, size - len, "Max buffer count\t0x%08x\n", hal_read32(base_va + UTIL_MAX_BUF_CNT));
	len += oal_util_snprintf(buf + len, size - len, "TQS max count\t\t0x%08x\n", hal_read32(base_va + UTIL_TSQ_MAX_CNT));

	return len;
}

/**
 * @brief		Dispatch interrupt from util.
 * @details		ACK and process triggered interrupts.

 * @param[in]	base_va 	Base address of UTIL register space (virtual)
 * @return		EOK if interrupt has been handled, error code otherwise
 */
errno_t pfe_util_cfg_isr(addr_t base_va)
{
	uint32_t irq_src;

	/* Get IRQ status */
	irq_src = hal_read32(base_va + UTIL_UPE_GP_REG_ADDR);
	/*ACK interrupt */
	hal_write32(irq_src, base_va + UTIL_UPE_GP_REG_ADDR);

	/* TODO: Handle related interrupt here */

	return EOK;
}
