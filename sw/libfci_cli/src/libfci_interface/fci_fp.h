/* =========================================================================
 *  Copyright 2020-2021 NXP
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
