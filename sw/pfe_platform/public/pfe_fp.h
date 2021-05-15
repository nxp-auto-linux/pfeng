/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2019-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef PFE_FP_H
#define PFE_FP_H

#include "pfe_class.h"

void pfe_fp_init(void);
uint32_t pfe_fp_create_table(pfe_class_t *class, uint8_t rules_count);
uint32_t pfe_fp_table_write_rule(pfe_class_t *class, uint32_t table_address, pfe_ct_fp_rule_t *rule, uint8_t position);
void pfe_fp_destroy_table(pfe_class_t *class, uint32_t table_address);
errno_t pfe_fp_table_get_statistics(pfe_class_t *class, uint32_t pe_idx ,uint32_t table_address, pfe_ct_class_flexi_parser_stats_t *stats);

#endif
