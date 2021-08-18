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

#ifndef FCI_LOG_IF_H_
#define FCI_LOG_IF_H_

#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*fci_log_if_cb_print_t)(const fpp_log_if_cmd_t* p_logif);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */

int fci_log_if_get_by_name(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, const char* p_name);
int fci_log_if_get_by_name_sa(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, const char* p_name);
int fci_log_if_get_by_id(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, uint32_t id);

/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */

int fci_log_if_update(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_logif);
int fci_log_if_update_sa(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_logif);

/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */

int fci_log_if_add(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, const char* p_name, const char* p_parent_name);
int fci_log_if_del(FCI_CLIENT* p_cl, const char* p_name);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

int fci_log_if_ld_enable(fpp_log_if_cmd_t* p_logif);
int fci_log_if_ld_disable(fpp_log_if_cmd_t* p_logif);
int fci_log_if_ld_set_promisc(fpp_log_if_cmd_t* p_logif, bool promisc);
int fci_log_if_ld_set_loopback(fpp_log_if_cmd_t* p_logif, bool loopback);
int fci_log_if_ld_set_match_mode_or(fpp_log_if_cmd_t* p_logif, bool match_mode_is_or);
int fci_log_if_ld_set_discard_on_m(fpp_log_if_cmd_t* p_logif, bool discard_on_match);
int fci_log_if_ld_set_egress_phyifs(fpp_log_if_cmd_t* p_logif, uint32_t egress);

int fci_log_if_ld_clear_all_mr(fpp_log_if_cmd_t* p_logif);
int fci_log_if_ld_set_mr_type_eth(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_vlan(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_pppoe(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_arp(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_mcast(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_ip4(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_ip6(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_ipx(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_bcast(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_udp(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_tcp(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_icmp(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_type_igmp(fpp_log_if_cmd_t* p_logif, bool do_set);
int fci_log_if_ld_set_mr_vlan(fpp_log_if_cmd_t* p_logif, bool do_set, uint16_t vlan);
int fci_log_if_ld_set_mr_proto(fpp_log_if_cmd_t* p_logif, bool do_set, uint8_t proto);
int fci_log_if_ld_set_mr_sport(fpp_log_if_cmd_t* p_logif, bool do_set, uint16_t sport);
int fci_log_if_ld_set_mr_dport(fpp_log_if_cmd_t* p_logif, bool do_set, uint16_t dport);
int fci_log_if_ld_set_mr_sip6(fpp_log_if_cmd_t* p_logif, bool do_set, const uint32_t p_sip6[4]);
int fci_log_if_ld_set_mr_dip6(fpp_log_if_cmd_t* p_logif, bool do_set, const uint32_t p_sip6[4]);
int fci_log_if_ld_set_mr_sip(fpp_log_if_cmd_t* p_logif, bool do_set, uint32_t sip);
int fci_log_if_ld_set_mr_dip(fpp_log_if_cmd_t* p_logif, bool do_set, uint32_t dip);
int fci_log_if_ld_set_mr_ethtype(fpp_log_if_cmd_t* p_logif, bool do_set, uint16_t ethtype);
int fci_log_if_ld_set_mr_fp0(fpp_log_if_cmd_t* p_logif, bool do_set, const char* fp_table0_name);
int fci_log_if_ld_set_mr_fp1(fpp_log_if_cmd_t* p_logif, bool do_set, const char* fp_table1_name);
int fci_log_if_ld_set_mr_smac(fpp_log_if_cmd_t* p_logif, bool do_set, const uint8_t p_smac[6]);
int fci_log_if_ld_set_mr_dmac(fpp_log_if_cmd_t* p_logif, bool do_set, const uint8_t p_dmac[6]);
int fci_log_if_ld_set_mr_hif_cookie(fpp_log_if_cmd_t* p_logif, bool do_set, uint32_t hif_cookie);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool fci_log_if_ld_is_enabled(const fpp_log_if_cmd_t* p_logif);
bool fci_log_if_ld_is_disabled(const fpp_log_if_cmd_t* p_logif);
bool fci_log_if_ld_is_promisc(const fpp_log_if_cmd_t* p_logif);
bool fci_log_if_ld_is_loopback(const fpp_log_if_cmd_t* p_logif);
bool fci_log_if_ld_is_match_mode_or(const fpp_log_if_cmd_t* p_logif);
bool fci_log_if_ld_is_discard_on_m(const fpp_log_if_cmd_t* p_logif);
bool fci_log_if_ld_is_egress_phyifs(const fpp_log_if_cmd_t* p_logif, uint32_t phyif_bitflag);
bool fci_log_if_ld_is_mr(const fpp_log_if_cmd_t* p_logif, fpp_if_m_rules_t match_rule);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int fci_log_if_print_all(FCI_CLIENT* p_cl, fci_log_if_cb_print_t p_cb_print);
int fci_log_if_print_all_sa(FCI_CLIENT* p_cl, fci_log_if_cb_print_t p_cb_print);
int fci_log_if_print_by_parent(FCI_CLIENT* p_cl, fci_log_if_cb_print_t p_cb_print, const char* p_parent_name);
int fci_log_if_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count);
int fci_log_if_get_count_by_parent(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, const char* p_parent_name);

/* ========================================================================= */

#endif
