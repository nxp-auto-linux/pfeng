/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2020-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_WDT_CSR_H_
#define PFE_WDT_CSR_H_

#include "pfe_wdt.h"

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

errno_t pfe_wdt_cfg_isr(addr_t base_va, addr_t cbus_base_va);
void pfe_wdt_cfg_irq_mask(addr_t base_va);
void pfe_wdt_cfg_irq_unmask(addr_t base_va);
void pfe_wdt_cfg_init(addr_t base_va);
void pfe_wdt_cfg_fini(addr_t base_va);

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)
uint32_t pfe_wdt_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level);
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_WDT_CSR_H_ */
