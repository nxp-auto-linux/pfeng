/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2020-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_WDT_CSR_H_
#define PFE_WDT_CSR_H_

#include "pfe_wdt.h"

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

errno_t pfe_wdt_cfg_isr(addr_t base_va, addr_t cbus_base_va);
void pfe_wdt_cfg_irq_mask(addr_t base_va);
void pfe_wdt_cfg_irq_unmask(addr_t base_va);
void pfe_wdt_cfg_init(addr_t base_va);
void pfe_wdt_cfg_fini(addr_t base_va);
uint32_t pfe_wdt_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level);

#endif /* PFE_WDT_CSR_H_ */
