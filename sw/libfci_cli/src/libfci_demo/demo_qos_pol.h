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

#ifndef DEMO_QOS_POL_H_
#define DEMO_QOS_POL_H_

#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*demo_polflow_cb_print_t)(const fpp_qos_policer_flow_cmd_t* p_polflow);
typedef int (*demo_polwred_cb_print_t)(const fpp_qos_policer_wred_cmd_t* p_polwred);
typedef int (*demo_polshp_cb_print_t)(const fpp_qos_policer_shp_cmd_t* p_polshp);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */

int demo_pol_get(FCI_CLIENT* p_cl, fpp_qos_policer_cmd_t* p_rtn_pol, const char* p_phyif_name);

int demo_polshp_get_by_id(FCI_CLIENT* p_cl, fpp_qos_policer_shp_cmd_t* p_rtn_polshp, const char* p_phyif_name, uint8_t polshp_id);
int demo_polwred_get_by_que(FCI_CLIENT* p_cl, fpp_qos_policer_wred_cmd_t* p_rtn_polwred, const char* p_phyif_name, fpp_iqos_queue_t polwred_que);
int demo_polflow_get_by_id(FCI_CLIENT* p_cl, fpp_qos_policer_flow_cmd_t* p_rtn_polflow, const char* p_phyif_name, uint8_t polflow_id);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */

int demo_pol_enable(FCI_CLIENT* p_cl, const char* p_phyif_name, bool enable);

int demo_polshp_update(FCI_CLIENT* p_cl, fpp_qos_policer_shp_cmd_t* p_polshp);
int demo_polwred_update(FCI_CLIENT* p_cl, fpp_qos_policer_wred_cmd_t* p_polwred);

/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */

int demo_polflow_add(FCI_CLIENT* p_cl, const char* p_phyif_name, uint8_t polflow_id, fpp_qos_policer_flow_cmd_t* p_polflow_data);
int demo_polflow_del(FCI_CLIENT* p_cl, const char* p_phyif_name, uint8_t polflow_id);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

void demo_polflow_ld_set_action(fpp_qos_policer_flow_cmd_t* p_polflow, fpp_iqos_flow_action_t action);

void demo_polflow_ld_clear_m(fpp_qos_policer_flow_cmd_t* p_polflow);
void demo_polflow_ld_set_m_type_eth(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);
void demo_polflow_ld_set_m_type_pppoe(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);
void demo_polflow_ld_set_m_type_arp(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);
void demo_polflow_ld_set_m_type_ip4(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);
void demo_polflow_ld_set_m_type_ip6(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);
void demo_polflow_ld_set_m_type_ipx(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);
void demo_polflow_ld_set_m_type_mcast(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);
void demo_polflow_ld_set_m_type_bcast(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);
void demo_polflow_ld_set_m_type_vlan(fpp_qos_policer_flow_cmd_t* p_polflow, bool set);

void demo_polflow_ld_clear_am(fpp_qos_policer_flow_cmd_t* p_polflow);
void demo_polflow_ld_set_am_vlan(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, uint16_t vlan, uint16_t vlan_m);
void demo_polflow_ld_set_am_tos(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, uint8_t tos, uint8_t tos_m);
void demo_polflow_ld_set_am_proto(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, uint8_t proto, uint8_t proto_m);
void demo_polflow_ld_set_am_sip(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, uint32_t sip, uint8_t sip_m);
void demo_polflow_ld_set_am_dip(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, uint32_t dip, uint8_t dip_m);
void demo_polflow_ld_set_am_sport(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, uint16_t sport_min, uint16_t sport_max);
void demo_polflow_ld_set_am_dport(fpp_qos_policer_flow_cmd_t* p_polflow, bool set, uint16_t dport_min, uint16_t dport_max);


void demo_polwred_ld_enable(fpp_qos_policer_wred_cmd_t* p_polwred, bool enable);
void demo_polwred_ld_set_min(fpp_qos_policer_wred_cmd_t* p_polwred, uint16_t min);
void demo_polwred_ld_set_max(fpp_qos_policer_wred_cmd_t* p_polwred, uint16_t max);
void demo_polwred_ld_set_full(fpp_qos_policer_wred_cmd_t* p_polwred, uint16_t full);
void demo_polwred_ld_set_zprob(fpp_qos_policer_wred_cmd_t* p_polwred, uint8_t zprob_id, uint8_t percentage);


void demo_polshp_ld_enable(fpp_qos_policer_shp_cmd_t* p_polshp, bool enable);
void demo_polshp_ld_set_type(fpp_qos_policer_shp_cmd_t* p_polshp, fpp_iqos_shp_type_t shp_type);
void demo_polshp_ld_set_mode(fpp_qos_policer_shp_cmd_t* p_polshp, fpp_iqos_shp_rate_mode_t shp_mode);
void demo_polshp_ld_set_isl(fpp_qos_policer_shp_cmd_t* p_polshp, uint32_t isl);
void demo_polshp_ld_set_min_credit(fpp_qos_policer_shp_cmd_t* p_polshp, int32_t min_credit);
void demo_polshp_ld_set_max_credit(fpp_qos_policer_shp_cmd_t* p_polshp, int32_t max_credit);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

const char* demo_pol_ld_get_if_name(const fpp_qos_policer_cmd_t* p_pol);
bool        demo_pol_ld_is_enabled(const fpp_qos_policer_cmd_t* p_pol);

const char* demo_polflow_ld_get_if_name(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint8_t     demo_polflow_ld_get_id(const fpp_qos_policer_flow_cmd_t* p_polflow);
fpp_iqos_flow_action_t   demo_polflow_ld_get_action(const fpp_qos_policer_flow_cmd_t* p_polflow);
fpp_iqos_flow_type_t     demo_polflow_ld_get_m_bitset(const fpp_qos_policer_flow_cmd_t* p_polflow);
fpp_iqos_flow_arg_type_t demo_polflow_ld_get_am_bitset(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint16_t    demo_polflow_ld_get_am_vlan(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint16_t    demo_polflow_ld_get_am_vlan_m(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint8_t     demo_polflow_ld_get_am_tos(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint8_t     demo_polflow_ld_get_am_tos_m(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint8_t     demo_polflow_ld_get_am_proto(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint8_t     demo_polflow_ld_get_am_proto_m(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint32_t    demo_polflow_ld_get_am_sip(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint8_t     demo_polflow_ld_get_am_sip_m(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint32_t    demo_polflow_ld_get_am_dip(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint8_t     demo_polflow_ld_get_am_dip_m(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint16_t    demo_polflow_ld_get_am_sport_min(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint16_t    demo_polflow_ld_get_am_sport_max(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint16_t    demo_polflow_ld_get_am_dport_min(const fpp_qos_policer_flow_cmd_t* p_polflow);
uint16_t    demo_polflow_ld_get_am_dport_max(const fpp_qos_policer_flow_cmd_t* p_polflow);

const char* demo_polwred_ld_get_if_name(const fpp_qos_policer_wred_cmd_t* p_polwred);
fpp_iqos_queue_t demo_polwred_ld_get_que(const fpp_qos_policer_wred_cmd_t* p_polwred);
bool        demo_polwred_ld_is_enabled(const fpp_qos_policer_wred_cmd_t* p_polwred);
uint16_t    demo_polwred_ld_get_min(const fpp_qos_policer_wred_cmd_t* p_polwred);
uint16_t    demo_polwred_ld_get_max(const fpp_qos_policer_wred_cmd_t* p_polwred);
uint16_t    demo_polwred_ld_get_full(const fpp_qos_policer_wred_cmd_t* p_polwred);
uint8_t     demo_polwred_ld_get_zprob_by_id(const fpp_qos_policer_wred_cmd_t* p_polwred, uint8_t zprob_id);

const char* demo_polshp_ld_get_if_name(const fpp_qos_policer_shp_cmd_t* p_polshp);
uint8_t     demo_polshp_ld_get_id(const fpp_qos_policer_shp_cmd_t* p_polshp);
bool        demo_polshp_ld_is_enabled(const fpp_qos_policer_shp_cmd_t* p_polshp);
fpp_iqos_shp_type_t      demo_polshp_ld_get_type(const fpp_qos_policer_shp_cmd_t* p_polshp);
fpp_iqos_shp_rate_mode_t demo_polshp_ld_get_mode(const fpp_qos_policer_shp_cmd_t* p_polshp);
uint32_t    demo_polshp_ld_get_isl(const fpp_qos_policer_shp_cmd_t* p_polshp);
int32_t     demo_polshp_ld_get_max_credit(const fpp_qos_policer_shp_cmd_t* p_polshp);
int32_t     demo_polshp_ld_get_min_credit(const fpp_qos_policer_shp_cmd_t* p_polshp);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int demo_polwred_print_by_phyif(FCI_CLIENT* p_cl, demo_polwred_cb_print_t p_cb_print, const char* p_phyif_name);
int demo_polwred_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_phyif_name);

int demo_polshp_print_by_phyif(FCI_CLIENT* p_cl, demo_polshp_cb_print_t p_cb_print, const char* p_phyif_name);
int demo_polshp_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_phyif_name);

int demo_polflow_print_by_phyif(FCI_CLIENT* p_cl, demo_polflow_cb_print_t p_cb_print, const char* p_phyif_name);
int demo_polflow_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_phyif_name);

/* ========================================================================= */

#endif
