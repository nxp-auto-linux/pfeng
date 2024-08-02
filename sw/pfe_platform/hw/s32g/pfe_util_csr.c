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
#include "pfe_util_csr.h"

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Get UTIL statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the UTIL block.
 * @param[in]	base_va 	Base address of UTIL register space (virtual)
 * @param[in]	seq 		Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_util_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level)
{
	uint32_t reg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Get version */
		if(verb_level >= 9U)
		{
			reg = hal_read32(base_va + UTIL_VERSION);
			seq_printf(seq, "Revision             : 0x%x\n", (reg >> 24U) & 0xffU);
			seq_printf(seq, "Version              : 0x%x\n", (reg >> 16U) & 0xffU);
			seq_printf(seq, "ID                   : 0x%x\n", reg & 0xffffU);
		}

		seq_printf(seq, "Max buffer count\t0x%08x\n", hal_read32(base_va + UTIL_MAX_BUF_CNT));
		seq_printf(seq, "TQS max count\t\t0x%08x\n", hal_read32(base_va + UTIL_TSQ_MAX_CNT));
	}
	return 0;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

