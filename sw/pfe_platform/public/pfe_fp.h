/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef PFE_FP_H
#define PFE_FP_H

#include "pfe_class.h"

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

void pfe_fp_init(void);
uint32_t pfe_fp_create_table(pfe_class_t *class, uint16_t rules_count);
uint32_t pfe_fp_table_write_rule(pfe_class_t *class, uint32_t table_address, const pfe_ct_fp_rule_t *rule, uint16_t position);
void pfe_fp_destroy_table(const pfe_class_t *class, uint32_t table_address);
errno_t pfe_fp_table_get_statistics(pfe_class_t *class, uint32_t pe_idx ,uint32_t table_address, pfe_ct_class_flexi_parser_stats_t *stats);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif
