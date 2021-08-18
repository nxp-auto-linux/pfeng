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

#ifndef FCI_RT_CT_H_
#define FCI_RT_CT_H_

#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*fci_rt_cb_print_t)(const fpp_rt_cmd_t* p_rt);
typedef int (*fci_ct_cb_print_t)(const fpp_ct_cmd_t* p_ct);
typedef int (*fci_ct6_cb_print_t)(const fpp_ct6_cmd_t* p_ct6);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */

int fci_rt_get_by_id(FCI_CLIENT* p_cl, fpp_rt_cmd_t* p_rtn_rt, uint32_t id);
int fci_ct_get_by_tuple(FCI_CLIENT* p_cl, fpp_ct_cmd_t* p_rtn_ct, const fpp_ct_cmd_t* p_ct_data);
int fci_ct6_get_by_tuple(FCI_CLIENT* p_cl, fpp_ct6_cmd_t* p_rtn_ct6, const fpp_ct6_cmd_t* p_ct6_data);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */

int fci_ct_update(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data);
int fci_ct6_update(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data);

int fci_ct_timeout_tcp(FCI_CLIENT* p_cl, uint32_t timeout, bool is_4o6);
int fci_ct_timeout_udp(FCI_CLIENT* p_cl, uint32_t timeout, uint32_t timeout2, bool is_4o6);
int fci_ct_timeout_others(FCI_CLIENT* p_cl, uint32_t timeout, bool is_4o6);

/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */

int fci_rt_add(FCI_CLIENT* p_cl, uint32_t id, const fpp_rt_cmd_t* p_rt_data);
int fci_rt_del(FCI_CLIENT* p_cl, uint32_t id);

int fci_ct_add(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data);
int fci_ct_del(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data);

int fci_ct6_add(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data);
int fci_ct6_del(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data);

int fci_rtct_reset_ip4(FCI_CLIENT* p_cl);
int fci_rtct_reset_ip6(FCI_CLIENT* p_cl);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

int fci_rt_ld_set_as_ip4(fpp_rt_cmd_t* p_rt);
int fci_rt_ld_set_as_ip6(fpp_rt_cmd_t* p_rt);
int fci_rt_ld_set_src_mac(fpp_rt_cmd_t* p_rt, const uint8_t p_src_mac[6]);
int fci_rt_ld_set_dst_mac(fpp_rt_cmd_t* p_rt, const uint8_t p_dst_mac[6]);
int fci_rt_ld_set_egress_phyif(fpp_rt_cmd_t* p_rt, const char* p_phyif_name);

int fci_ct_ld_set_protocol(fpp_ct_cmd_t* p_ct, uint16_t protocol);
int fci_ct_ld_set_ttl_decr(fpp_ct_cmd_t* p_ct, bool enable);
int fci_ct_ld_set_orig_dir(fpp_ct_cmd_t* p_ct, uint32_t saddr, uint32_t daddr,
                           uint16_t sport, uint16_t dport,
                           uint32_t route_id, uint16_t vlan, bool unidir_orig_only);
int fci_ct_ld_set_reply_dir(fpp_ct_cmd_t* p_ct, uint32_t saddr_reply, uint32_t daddr_reply,
                            uint16_t sport_reply, uint16_t dport_reply,
                            uint32_t route_id_reply, uint16_t vlan_reply, bool unidir_reply_only);
                            
                            
int fci_ct6_ld_set_protocol(fpp_ct6_cmd_t* p_ct6, uint16_t protocol);
int fci_ct6_ld_set_ttl_decr(fpp_ct6_cmd_t* p_ct6, bool enable);
int fci_ct6_ld_set_orig_dir(fpp_ct6_cmd_t* p_ct6, const uint32_t p_saddr[4], const uint32_t p_daddr[4],
                            uint16_t sport, uint16_t dport,
                            uint32_t route_id, uint16_t vlan, bool unidir_orig_only);
int fci_ct6_ld_set_reply_dir(fpp_ct6_cmd_t* p_ct6, const uint32_t p_saddr_reply[4], const uint32_t p_daddr_reply[4],
                             uint16_t sport_reply, uint16_t dport_reply,
                             uint32_t route_id_reply, uint16_t vlan_reply, bool unidir_reply_only);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool fci_rt_ld_is_ip4(const fpp_rt_cmd_t* p_rt);
bool fci_rt_ld_is_ip6(const fpp_rt_cmd_t* p_rt);

bool fci_ct_ld_is_nat(const fpp_ct_cmd_t* p_ct);
bool fci_ct_ld_is_pat(const fpp_ct_cmd_t* p_ct);
bool fci_ct_ld_is_vlan_tagging(const fpp_ct_cmd_t* p_ct);
bool fci_ct_ld_is_ttl_decr(const fpp_ct_cmd_t* p_ct);
bool fci_ct_ld_is_orig_only(const fpp_ct_cmd_t* p_ct);
bool fci_ct_ld_is_reply_only(const fpp_ct_cmd_t* p_ct);

bool fci_ct6_ld_is_nat(const fpp_ct6_cmd_t* p_ct6);
bool fci_ct6_ld_is_pat(const fpp_ct6_cmd_t* p_ct6);
bool fci_ct6_ld_is_vlan_tagging(const fpp_ct6_cmd_t* p_ct6);
bool fci_ct6_ld_is_ttl_decr(const fpp_ct6_cmd_t* p_ct6);
bool fci_ct6_ld_is_orig_only(const fpp_ct6_cmd_t* p_ct6);
bool fci_ct6_ld_is_reply_only(const fpp_ct6_cmd_t* p_ct6);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int fci_rt_print_all(FCI_CLIENT* p_cl, fci_rt_cb_print_t p_cb_print, bool print_ip4, bool print_ip6);
int fci_rt_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);

int fci_ct_print_all(FCI_CLIENT* p_cl, fci_ct_cb_print_t p_cb_print);
int fci_ct_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);

int fci_ct6_print_all(FCI_CLIENT* p_cl, fci_ct6_cb_print_t p_cb_print);
int fci_ct6_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);

/* ========================================================================= */

#endif
