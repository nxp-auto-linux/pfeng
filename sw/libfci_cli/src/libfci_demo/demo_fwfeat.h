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

#ifndef DEMO_FWFEAT_H_
#define DEMO_FWFEAT_H_

#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*demo_fwfeat_cb_print_t)(const fpp_fw_features_cmd_t* p_fwfeat);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */

int demo_fwfeat_get_by_name(FCI_CLIENT* p_cl, fpp_fw_features_cmd_t* p_rtn_fwfeat, const char* p_name);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */

int demo_fwfeat_set(FCI_CLIENT* p_cl, const char* p_name, bool enable);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool demo_fwfeat_ld_is_enabled(const fpp_fw_features_cmd_t* p_fwfeat);
bool demo_fwfeat_ld_is_enabled_by_def(const fpp_fw_features_cmd_t* p_fwfeat);

const char* demo_fwfeat_ld_get_name(const fpp_fw_features_cmd_t* p_fwfeat);
const char* demo_fwfeat_ld_get_desc(const fpp_fw_features_cmd_t* p_fwfeat);
fpp_fw_feature_flags_t demo_fwfeat_ld_get_flags(const fpp_fw_features_cmd_t* p_fwfeat);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int demo_fwfeat_print_all(FCI_CLIENT* p_cl, demo_fwfeat_cb_print_t p_cb_print);
int demo_fwfeat_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);

/* ========================================================================= */

#endif
