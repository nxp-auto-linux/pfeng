/* =========================================================================
 *  (c) Copyright 2006-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017, 2019-2021 NXP
 *
 *  NXP Confidential. This software is owned or controlled by NXP and may only
 *  be used strictly in accordance with the applicable license terms. By
 *  expressly accepting such terms or by downloading, installing, activating
 *  and/or otherwise using the software, you are agreeing that you have read,
 *  and that you agree to comply with and are bound by, such license terms. If
 *  you do not agree to be bound by the applicable license terms, then you may
 *  not retain, install, activate or otherwise use the software.
 *
 *  This file contains sample code only. It is not part of the production code deliverables.
 * ========================================================================= */

#ifndef FCI_FP_H_
#define FCI_FP_H_

#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*fci_fp_rule_cb_print_t)(const fpp_fp_rule_props_t* p_rule_props, uint16_t position);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */

int fci_fp_rule_get_by_name(FCI_CLIENT* p_cl, fpp_fp_rule_cmd_t* p_rtn_rule, uint16_t* p_rtn_idx, const char* p_rule_name);

/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */

int fci_fp_rule_add(FCI_CLIENT* p_cl, const char* p_rule_name, const fpp_fp_rule_cmd_t* p_rule_data);
int fci_fp_rule_del(FCI_CLIENT* p_cl, const char* p_rule_name);

int fci_fp_table_add(FCI_CLIENT* p_cl, const char* p_table_name);
int fci_fp_table_del(FCI_CLIENT* p_cl, const char* p_table_name);
int fci_fp_table_insert_rule(FCI_CLIENT* p_cl, const char* p_table_name, const char* p_rule_name, uint16_t position);
int fci_fp_table_remove_rule(FCI_CLIENT* p_cl, const char* p_table_name, const char* p_rule_name);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

int fci_fp_rule_ld_set_data(fpp_fp_rule_cmd_t* p_rule, uint32_t data);
int fci_fp_rule_ld_set_mask(fpp_fp_rule_cmd_t* p_rule, uint32_t mask);
int fci_fp_rule_ld_set_offset(fpp_fp_rule_cmd_t* p_rule, uint16_t offset, fpp_fp_offset_from_t offset_from);
int fci_fp_rule_ld_set_invert(fpp_fp_rule_cmd_t* p_rule, bool invert);
int fci_fp_rule_ld_set_match_action(fpp_fp_rule_cmd_t* p_rule, fpp_fp_rule_match_action_t match_action,
                                    const char* p_next_rule_name);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int fci_fp_table_print(FCI_CLIENT* p_cl, fci_fp_rule_cb_print_t p_cb_print, const char* p_table_name,
                       uint16_t position_init, uint16_t count);
int fci_fp_rule_print_all(FCI_CLIENT* p_cl, fci_fp_rule_cb_print_t p_cb_print, uint16_t idx_init, uint16_t count);
int fci_fp_rule_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count);

/* ========================================================================= */

#endif
