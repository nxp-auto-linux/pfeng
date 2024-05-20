/* =========================================================================
 *  Copyright 2020-2024 NXP
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
 
#ifndef DEMO_PHY_IF_H_
#define DEMO_PHY_IF_H_
 
#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
/* ==== TYPEDEFS & DATA ==================================================== */
 
/* hardcoded PHY_IF names, IDs and bitflags (IDs 3 & 4 are reserved) */
#define DEMO_PHY_IF_EMAC0_ID  (0u)
#define DEMO_PHY_IF_EMAC1_ID  (1u)
#define DEMO_PHY_IF_EMAC2_ID  (2u)
#define DEMO_PHY_IF_UTIL_ID   (5u)
#define DEMO_PHY_IF_HIF0_ID   (6u)
#define DEMO_PHY_IF_HIF1_ID   (7u)
#define DEMO_PHY_IF_HIF2_ID   (8u)
#define DEMO_PHY_IF_HIF3_ID   (9u)
 
#define DEMO_PHY_IF_EMAC0_BITFLAG  (1uL << DEMO_PHY_IF_EMAC0_ID)
#define DEMO_PHY_IF_EMAC1_BITFLAG  (1uL << DEMO_PHY_IF_EMAC1_ID)
#define DEMO_PHY_IF_EMAC2_BITFLAG  (1uL << DEMO_PHY_IF_EMAC2_ID)
#define DEMO_PHY_IF_UTIL_BITFLAG   (1uL << DEMO_PHY_IF_UTIL_ID)
#define DEMO_PHY_IF_HIF0_BITFLAG   (1uL << DEMO_PHY_IF_HIF0_ID)
#define DEMO_PHY_IF_HIF1_BITFLAG   (1uL << DEMO_PHY_IF_HIF1_ID)
#define DEMO_PHY_IF_HIF2_BITFLAG   (1uL << DEMO_PHY_IF_HIF2_ID)
#define DEMO_PHY_IF_HIF3_BITFLAG   (1uL << DEMO_PHY_IF_HIF3_ID)
 
typedef int (*demo_phy_if_cb_print_t)(const fpp_phy_if_cmd_t* p_phyif);
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
int demo_phy_if_get_by_name(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_rtn_phyif, const char* p_name);
int demo_phy_if_get_by_name_sa(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_rtn_phyif, const char* p_name);
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
int demo_phy_if_update(FCI_CLIENT* p_cl, fpp_phy_if_cmd_t* p_phyif);
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
 
void demo_phy_if_ld_enable(fpp_phy_if_cmd_t* p_phyif);
void demo_phy_if_ld_disable(fpp_phy_if_cmd_t* p_phyif);
void demo_phy_if_ld_set_promisc(fpp_phy_if_cmd_t* p_phyif, bool enable);
void demo_phy_if_ld_set_ff_all_tcp(fpp_phy_if_cmd_t* p_phyif, bool enable);
void demo_phy_if_ld_set_vlan_conf(fpp_phy_if_cmd_t* p_phyif, bool enable);
void demo_phy_if_ld_set_ptp_conf(fpp_phy_if_cmd_t* p_phyif, bool enable);
void demo_phy_if_ld_set_ptp_promisc(fpp_phy_if_cmd_t* p_phyif, bool enable);
void demo_phy_if_ld_set_qinq(fpp_phy_if_cmd_t* p_phyif, bool enable);
void demo_phy_if_ld_set_discard_ttl(fpp_phy_if_cmd_t* p_phyif, bool enable);
void demo_phy_if_ld_set_mode(fpp_phy_if_cmd_t* p_phyif, fpp_phy_if_op_mode_t mode);
void demo_phy_if_ld_set_block_state(fpp_phy_if_cmd_t* p_phyif, fpp_phy_if_block_state_t block_state);
void demo_phy_if_ld_set_rx_mirror(fpp_phy_if_cmd_t* p_phyif, uint8_t idx, const char* p_mirror_name);
void demo_phy_if_ld_set_tx_mirror(fpp_phy_if_cmd_t* p_phyif, uint8_t idx, const char* p_mirror_name);
void demo_phy_if_ld_set_flexifilter(fpp_phy_if_cmd_t* p_phyif, const char* p_table_name);
void demo_phy_if_ld_set_ptp_mgmt_if(fpp_phy_if_cmd_t* p_phyif, const char* p_name);
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
bool demo_phy_if_ld_is_enabled(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_disabled(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_promisc(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_ff_all_tcp(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_vlan_conf(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_ptp_conf(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_ptp_promisc(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_qinq(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_discard_ttl(const fpp_phy_if_cmd_t* p_phyif);
bool demo_phy_if_ld_is_mirror(const fpp_phy_if_cmd_t* p_phyif);
 
const char*              demo_phy_if_ld_get_name(const fpp_phy_if_cmd_t* p_phyif);
uint32_t                 demo_phy_if_ld_get_id(const fpp_phy_if_cmd_t* p_phyif);
fpp_if_flags_t           demo_phy_if_ld_get_flags(const fpp_phy_if_cmd_t* p_phyif);
fpp_phy_if_op_mode_t     demo_phy_if_ld_get_mode(const fpp_phy_if_cmd_t* p_phyif);
fpp_phy_if_block_state_t demo_phy_if_ld_get_block_state(const fpp_phy_if_cmd_t* p_phyif);
const char*              demo_phy_if_ld_get_rx_mirror(const fpp_phy_if_cmd_t* p_phyif, uint8_t idx);
const char*              demo_phy_if_ld_get_tx_mirror(const fpp_phy_if_cmd_t* p_phyif, uint8_t idx);
const char*              demo_phy_if_ld_get_flexifilter(const fpp_phy_if_cmd_t* p_phyif);
const char*              demo_phy_if_ld_get_ptp_mgmt_if(const fpp_phy_if_cmd_t* p_phyif);
 
uint32_t demo_phy_if_ld_get_stt_ingress(const fpp_phy_if_cmd_t* p_phyif);
uint32_t demo_phy_if_ld_get_stt_egress(const fpp_phy_if_cmd_t* p_phyif);
uint32_t demo_phy_if_ld_get_stt_malformed(const fpp_phy_if_cmd_t* p_phyif);
uint32_t demo_phy_if_ld_get_stt_discarded(const fpp_phy_if_cmd_t* p_phyif);
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
int demo_phy_if_print_all(FCI_CLIENT* p_cl, demo_phy_if_cb_print_t p_cb_print);
int demo_phy_if_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count);
 
/* ========================================================================= */
 
#endif
 
