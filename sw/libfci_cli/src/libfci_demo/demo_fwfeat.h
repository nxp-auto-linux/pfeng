/* =========================================================================
 *  Copyright 2020-2022 NXP
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

#ifndef DEMO_FWFEAT_H_
#define DEMO_FWFEAT_H_

#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*demo_fwfeat_cb_print_t)(const fpp_fw_features_cmd_t* p_fwfeat);
typedef int (*demo_fwfeat_el_cb_print_t)(const fpp_fw_features_element_cmd_t* p_fwfeat_el);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */

int demo_fwfeat_get_by_name(FCI_CLIENT* p_cl, fpp_fw_features_cmd_t* p_rtn_fwfeat, const char* p_feature_name);
int demo_fwfeat_el_get_by_name(FCI_CLIENT* p_cl, fpp_fw_features_element_cmd_t* p_rtn_fwfeat_el, const char* p_feature_name, 
                               const char* p_element_name, uint8_t group, uint8_t index);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */

int demo_fwfeat_set(FCI_CLIENT* p_cl, const char* p_feature_name, bool enable);
int demo_fwfeat_el_set(FCI_CLIENT* p_cl, fpp_fw_features_element_cmd_t* p_fwfeat_el);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

void demo_fwfeat_el_set_group(fpp_fw_features_element_cmd_t* p_fwfeat_el, uint8_t group);
void demo_fwfeat_el_set_index(fpp_fw_features_element_cmd_t* p_fwfeat_el, uint8_t index);
int demo_fwfeat_el_set_payload(fpp_fw_features_element_cmd_t* p_fwfeat_el, const uint8_t* p_payload, uint8_t count, uint8_t unit_size);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool demo_fwfeat_ld_is_enabled(const fpp_fw_features_cmd_t* p_fwfeat);
bool demo_fwfeat_ld_is_enabled_by_def(const fpp_fw_features_cmd_t* p_fwfeat);

const char* demo_fwfeat_ld_get_name(const fpp_fw_features_cmd_t* p_fwfeat);
const char* demo_fwfeat_ld_get_desc(const fpp_fw_features_cmd_t* p_fwfeat);
fpp_fw_feature_flags_t demo_fwfeat_ld_get_flags(const fpp_fw_features_cmd_t* p_fwfeat);


const char* demo_fwfeat_el_ld_get_name(const fpp_fw_features_element_cmd_t* p_fwfeat_el);
const char* demo_fwfeat_el_ld_get_feat_name(const fpp_fw_features_element_cmd_t* p_fwfeat_el);
uint8_t demo_fwfeat_el_ld_get_group(const fpp_fw_features_element_cmd_t* p_fwfeat_el);
uint8_t demo_fwfeat_el_ld_get_index(const fpp_fw_features_element_cmd_t* p_fwfeat_el);
void demo_fwfeat_el_ld_get_payload(const fpp_fw_features_element_cmd_t* p_fwfeat_el, const uint8_t** pp_rtn_payload, uint8_t* p_rtn_count, uint8_t* p_rtn_unit_size);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int demo_fwfeat_print_all(FCI_CLIENT* p_cl, demo_fwfeat_cb_print_t p_cb_print);
int demo_fwfeat_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);

int demo_fwfeat_el_print_all(FCI_CLIENT* p_cl, demo_fwfeat_el_cb_print_t p_cb_print, const char* p_feature_name, uint8_t group);
int demo_fwfeat_el_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_feature_name, uint8_t group);

/* ========================================================================= */

#endif
