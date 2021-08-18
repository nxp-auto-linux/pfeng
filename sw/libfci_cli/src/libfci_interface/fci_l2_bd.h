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

#ifndef FCI_L2_BD_H_
#define FCI_L2_BD_H_

#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*fci_l2_bd_cb_print_t)(const fpp_l2_bd_cmd_t* p_bd);
typedef int (*fci_l2_stent_cb_print_t)(const fpp_l2_static_ent_cmd_t* p_stent);

#define FCI_L2_BD_ACTION_FORWARD  (0u)
#define FCI_L2_BD_ACTION_FLOOD    (1u)
#define FCI_L2_BD_ACTION_PUNT     (2u)
#define FCI_L2_BD_ACTION_DISCARD  (3u)

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */

int fci_l2_bd_get_by_vlan(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_rtn_bd, uint16_t vlan);

int fci_l2_stent_get_by_vlanmac(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_rtn_stent,
                                uint16_t vlan, const uint8_t p_mac[6]);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */

int fci_l2_bd_update(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_bd);

int fci_l2_stent_update(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_stent);

int fci_l2_flush_static(FCI_CLIENT* p_cl);
int fci_l2_flush_learned(FCI_CLIENT* p_cl);
int fci_l2_flush_all(FCI_CLIENT* p_cl);

/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */

int fci_l2_bd_add(FCI_CLIENT* p_cl, fpp_l2_bd_cmd_t* p_rtn_bd, uint16_t vlan);
int fci_l2_bd_del(FCI_CLIENT* p_cl, uint16_t vlan);

int fci_l2_stent_add(FCI_CLIENT* p_cl, fpp_l2_static_ent_cmd_t* p_rtn_stent, uint16_t vlan, const uint8_t p_mac[6]);
int fci_l2_stent_del(FCI_CLIENT* p_cl, uint16_t vlan, const uint8_t p_mac[6]);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

int fci_l2_bd_ld_set_ucast_hit(fpp_l2_bd_cmd_t* p_bd, uint8_t action);
int fci_l2_bd_ld_set_ucast_miss(fpp_l2_bd_cmd_t* p_bd, uint8_t action);
int fci_l2_bd_ld_set_mcast_hit(fpp_l2_bd_cmd_t* p_bd, uint8_t action);
int fci_l2_bd_ld_set_mcast_miss(fpp_l2_bd_cmd_t* p_bd, uint8_t action);
int fci_l2_bd_ld_insert_phyif(fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id, bool add_vlan_tag);
int fci_l2_bd_ld_remove_phyif(fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id);

int fci_l2_stent_ld_set_fwlist(fpp_l2_static_ent_cmd_t* p_stent, uint32_t fwlist);
int fci_l2_stent_ld_set_local(fpp_l2_static_ent_cmd_t* p_stent, bool local);
int fci_l2_stent_ld_set_src_discard(fpp_l2_static_ent_cmd_t* p_stent, bool src_discard);
int fci_l2_stent_ld_set_dst_discard(fpp_l2_static_ent_cmd_t* p_stent, bool dst_discard);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool fci_l2_bd_ld_is_default(const fpp_l2_bd_cmd_t* p_bd);
bool fci_l2_bd_ld_is_fallback(const fpp_l2_bd_cmd_t* p_bd);
bool fci_l2_bd_ld_is_phyif(const fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id);
bool fci_l2_bd_ld_is_tagged(const fpp_l2_bd_cmd_t* p_bd, uint32_t phyif_id);

bool fci_l2_stent_ld_is_fwlist_phyifs(const fpp_l2_static_ent_cmd_t* p_stent, uint32_t fwlist_bitflag);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int fci_l2_bd_print_all(FCI_CLIENT* p_cl, fci_l2_bd_cb_print_t p_cb_print);
int fci_l2_bd_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count);

int fci_l2_stent_print_all(FCI_CLIENT* p_cl, fci_l2_stent_cb_print_t p_cb_print);
int fci_l2_stent_print_by_vlan(FCI_CLIENT* p_cl, fci_l2_stent_cb_print_t p_cb_print, uint16_t vlan);
int fci_l2_stent_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count);
int fci_l2_stent_get_count_by_vlan(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, uint16_t vlan);

/* ========================================================================= */

#endif
