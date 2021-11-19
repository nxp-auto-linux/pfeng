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
 
#ifndef DEMO_MIRROR_H_
#define DEMO_MIRROR_H_
 
#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
/* ==== TYPEDEFS & DATA ==================================================== */
 
typedef int (*demo_mirror_cb_print_t)(const fpp_mirror_cmd_t* p_mirror);
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
int demo_mirror_get_by_name(FCI_CLIENT* p_cl, fpp_mirror_cmd_t* p_rtn_mirror, const char* p_name);
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
int demo_mirror_update(FCI_CLIENT* p_cl, fpp_mirror_cmd_t* p_mirror);
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
int demo_mirror_add(FCI_CLIENT* p_cl, fpp_mirror_cmd_t* p_rtn_mirror, const char* p_name, const char* p_phyif_name);
int demo_mirror_del(FCI_CLIENT* p_cl, const char* p_name);
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
 
void demo_mirror_ld_set_egress_phyif(fpp_mirror_cmd_t* p_mirror, const char* p_phyif_name);
void demo_mirror_ld_set_filter(fpp_mirror_cmd_t* p_mirror, const char* p_table_name);
 
void demo_mirror_ld_clear_all_ma(fpp_mirror_cmd_t* p_mirror); 
void demo_mirror_ld_set_ma_vlan(fpp_mirror_cmd_t* p_mirror, bool set, uint16_t vlan);
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
bool demo_mirror_ld_is_ma(const fpp_mirror_cmd_t* p_mirror, fpp_modify_actions_t mod_action);
 
const char* demo_mirror_ld_get_name(const fpp_mirror_cmd_t* p_mirror);
const char* demo_mirror_ld_get_egress_phyif(const fpp_mirror_cmd_t* p_mirror);
const char* demo_mirror_ld_get_filter(const fpp_mirror_cmd_t* p_mirror);
 
fpp_modify_actions_t demo_mirror_ld_get_ma_bitset(const fpp_mirror_cmd_t* p_mirror);
uint16_t demo_mirror_ld_get_ma_vlan(const fpp_mirror_cmd_t* p_mirror);
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
int demo_mirror_print_all(FCI_CLIENT* p_cl, demo_mirror_cb_print_t p_cb_print);
int demo_mirror_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);
 
/* ========================================================================= */
 
#endif
 
