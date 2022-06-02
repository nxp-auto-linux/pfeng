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

#ifndef DEMO_RT_CT_H_
#define DEMO_RT_CT_H_

#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*demo_rt_cb_print_t)(const fpp_rt_cmd_t* p_rt);
typedef int (*demo_ct_cb_print_t)(const fpp_ct_cmd_t* p_ct);
typedef int (*demo_ct6_cb_print_t)(const fpp_ct6_cmd_t* p_ct6);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */

int demo_rt_get_by_id(FCI_CLIENT* p_cl, fpp_rt_cmd_t* p_rtn_rt, uint32_t id);
int demo_ct_get_by_tuple(FCI_CLIENT* p_cl, fpp_ct_cmd_t* p_rtn_ct, const fpp_ct_cmd_t* p_ct_data);
int demo_ct6_get_by_tuple(FCI_CLIENT* p_cl, fpp_ct6_cmd_t* p_rtn_ct6, const fpp_ct6_cmd_t* p_ct6_data);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */

int demo_ct_update(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data);
int demo_ct6_update(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data);

int demo_ct_timeout_tcp(FCI_CLIENT* p_cl, uint32_t timeout);
int demo_ct_timeout_udp(FCI_CLIENT* p_cl, uint32_t timeout);
int demo_ct_timeout_others(FCI_CLIENT* p_cl, uint32_t timeout);

/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */

int demo_rt_add(FCI_CLIENT* p_cl, uint32_t id, const fpp_rt_cmd_t* p_rt_data);
int demo_rt_del(FCI_CLIENT* p_cl, uint32_t id);

int demo_ct_add(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data);
int demo_ct_del(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data);

int demo_ct6_add(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data);
int demo_ct6_del(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data);

int demo_rtct_reset_ip4(FCI_CLIENT* p_cl);
int demo_rtct_reset_ip6(FCI_CLIENT* p_cl);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

void demo_rt_ld_set_as_ip4(fpp_rt_cmd_t* p_rt);
void demo_rt_ld_set_as_ip6(fpp_rt_cmd_t* p_rt);
void demo_rt_ld_set_src_mac(fpp_rt_cmd_t* p_rt, const uint8_t p_src_mac[6]);
void demo_rt_ld_set_dst_mac(fpp_rt_cmd_t* p_rt, const uint8_t p_dst_mac[6]);
void demo_rt_ld_set_egress_phyif(fpp_rt_cmd_t* p_rt, const char* p_phyif_name);

void demo_ct_ld_set_protocol(fpp_ct_cmd_t* p_ct, uint16_t protocol);
void demo_ct_ld_set_ttl_decr(fpp_ct_cmd_t* p_ct, bool set);
void demo_ct_ld_set_orig_dir(fpp_ct_cmd_t* p_ct, uint32_t saddr, uint32_t daddr,
                             uint16_t sport, uint16_t dport, uint16_t vlan,
                             uint32_t route_id, bool unidir_orig_only);
void demo_ct_ld_set_reply_dir(fpp_ct_cmd_t* p_ct, uint32_t saddr_reply, uint32_t daddr_reply,
                              uint16_t sport_reply, uint16_t dport_reply, uint16_t vlan_reply,
                              uint32_t route_id_reply, bool unidir_reply_only);
                            
void demo_ct6_ld_set_protocol(fpp_ct6_cmd_t* p_ct6, uint16_t protocol);
void demo_ct6_ld_set_ttl_decr(fpp_ct6_cmd_t* p_ct6, bool set);
void demo_ct6_ld_set_orig_dir(fpp_ct6_cmd_t* p_ct6, const uint32_t p_saddr[4], const uint32_t p_daddr[4],
                              uint16_t sport, uint16_t dport, uint16_t vlan,
                              uint32_t route_id, bool unidir_orig_only);
void demo_ct6_ld_set_reply_dir(fpp_ct6_cmd_t* p_ct6, const uint32_t p_saddr_reply[4], const uint32_t p_daddr_reply[4],
                               uint16_t sport_reply, uint16_t dport_reply, uint16_t vlan_reply,
                               uint32_t route_id_reply, bool unidir_reply_only);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool demo_rt_ld_is_ip4(const fpp_rt_cmd_t* p_rt);
bool demo_rt_ld_is_ip6(const fpp_rt_cmd_t* p_rt);

uint32_t       demo_rt_ld_get_route_id(const fpp_rt_cmd_t* p_rt);
const uint8_t* demo_rt_ld_get_src_mac(const fpp_rt_cmd_t* p_rt);
const uint8_t* demo_rt_ld_get_dst_mac(const fpp_rt_cmd_t* p_rt);
const char*    demo_rt_ld_get_egress_phyif(const fpp_rt_cmd_t* p_rt);


bool demo_ct_ld_is_nat(const fpp_ct_cmd_t* p_ct);
bool demo_ct_ld_is_pat(const fpp_ct_cmd_t* p_ct);
bool demo_ct_ld_is_vlan_tagging(const fpp_ct_cmd_t* p_ct);
bool demo_ct_ld_is_ttl_decr(const fpp_ct_cmd_t* p_ct);
bool demo_ct_ld_is_orig_only(const fpp_ct_cmd_t* p_ct);
bool demo_ct_ld_is_reply_only(const fpp_ct_cmd_t* p_ct);

uint16_t demo_ct_ld_get_protocol(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_saddr(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_daddr(const fpp_ct_cmd_t* p_ct);
uint16_t demo_ct_ld_get_sport(const fpp_ct_cmd_t* p_ct);
uint16_t demo_ct_ld_get_dport(const fpp_ct_cmd_t* p_ct);
uint16_t demo_ct_ld_get_vlan(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_route_id(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_saddr_reply(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_daddr_reply(const fpp_ct_cmd_t* p_ct);
uint16_t demo_ct_ld_get_sport_reply(const fpp_ct_cmd_t* p_ct);
uint16_t demo_ct_ld_get_dport_reply(const fpp_ct_cmd_t* p_ct);
uint16_t demo_ct_ld_get_vlan_reply(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_route_id_reply(const fpp_ct_cmd_t* p_ct);
uint16_t demo_ct_ld_get_flags(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_stt_hit(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_stt_hit_bytes(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_stt_reply_hit(const fpp_ct_cmd_t* p_ct);
uint32_t demo_ct_ld_get_stt_reply_hit_bytes(const fpp_ct_cmd_t* p_ct);
 
 
bool demo_ct6_ld_is_nat(const fpp_ct6_cmd_t* p_ct6);
bool demo_ct6_ld_is_pat(const fpp_ct6_cmd_t* p_ct6);
bool demo_ct6_ld_is_vlan_tagging(const fpp_ct6_cmd_t* p_ct6);
bool demo_ct6_ld_is_ttl_decr(const fpp_ct6_cmd_t* p_ct6);
bool demo_ct6_ld_is_orig_only(const fpp_ct6_cmd_t* p_ct6);
bool demo_ct6_ld_is_reply_only(const fpp_ct6_cmd_t* p_ct6);

uint16_t        demo_ct6_ld_get_protocol(const fpp_ct6_cmd_t* p_ct6);
const uint32_t* demo_ct6_ld_get_saddr(const fpp_ct6_cmd_t* p_ct6);
const uint32_t* demo_ct6_ld_get_daddr(const fpp_ct6_cmd_t* p_ct6);
uint16_t        demo_ct6_ld_get_sport(const fpp_ct6_cmd_t* p_ct6);
uint16_t        demo_ct6_ld_get_dport(const fpp_ct6_cmd_t* p_ct6);
uint16_t        demo_ct6_ld_get_vlan(const fpp_ct6_cmd_t* p_ct6);
uint32_t        demo_ct6_ld_get_route_id(const fpp_ct6_cmd_t* p_ct6);
const uint32_t* demo_ct6_ld_get_saddr_reply(const fpp_ct6_cmd_t* p_ct6);
const uint32_t* demo_ct6_ld_get_daddr_reply(const fpp_ct6_cmd_t* p_ct6);
uint16_t        demo_ct6_ld_get_sport_reply(const fpp_ct6_cmd_t* p_ct6);
uint16_t        demo_ct6_ld_get_dport_reply(const fpp_ct6_cmd_t* p_ct6);
uint16_t        demo_ct6_ld_get_vlan_reply(const fpp_ct6_cmd_t* p_ct6);
uint32_t        demo_ct6_ld_get_route_id_reply(const fpp_ct6_cmd_t* p_ct6);
uint16_t        demo_ct6_ld_get_flags(const fpp_ct6_cmd_t* p_ct6);
uint32_t        demo_ct6_ld_get_stt_hit(const fpp_ct6_cmd_t* p_ct6);
uint32_t        demo_ct6_ld_get_stt_hit_bytes(const fpp_ct6_cmd_t* p_ct6);
uint32_t        demo_ct6_ld_get_stt_reply_hit(const fpp_ct6_cmd_t* p_ct6);
uint32_t        demo_ct6_ld_get_stt_reply_hit_bytes(const fpp_ct6_cmd_t* p_ct6);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int demo_rt_print_all(FCI_CLIENT* p_cl, demo_rt_cb_print_t p_cb_print, bool print_ip4, bool print_ip6);
int demo_rt_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);

int demo_ct_print_all(FCI_CLIENT* p_cl, demo_ct_cb_print_t p_cb_print);
int demo_ct_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);

int demo_ct6_print_all(FCI_CLIENT* p_cl, demo_ct6_cb_print_t p_cb_print);
int demo_ct6_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);

/* ========================================================================= */

#endif
