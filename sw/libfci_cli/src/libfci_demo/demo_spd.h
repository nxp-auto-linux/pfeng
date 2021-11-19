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

#ifndef DEMO_SPD_H_
#define DEMO_SPD_H_

#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"

/* ==== TYPEDEFS & DATA ==================================================== */

typedef int (*demo_spd_cb_print_t)(const fpp_spd_cmd_t* p_spd);

/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */

int demo_spd_get_by_position(FCI_CLIENT* p_cl, fpp_spd_cmd_t* p_rtn_spd, const char* p_phyif_name, uint16_t position);

/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */

int demo_spd_add(FCI_CLIENT* p_cl, const char* p_phyif_name, uint16_t position, const fpp_spd_cmd_t* p_spd_data);
int demo_spd_del(FCI_CLIENT* p_cl, const char* p_phyif_name, uint16_t position);

/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */

void demo_spd_ld_set_protocol(fpp_spd_cmd_t* p_spd, uint8_t protocol);
void demo_spd_ld_set_ip(fpp_spd_cmd_t* p_spd, const uint32_t p_saddr[4], const uint32_t p_daddr[4], bool is_ip6);
void demo_spd_ld_set_port(fpp_spd_cmd_t* p_spd, bool use_sport, uint16_t sport, bool use_dport, uint16_t dport);
void demo_spd_ld_set_action(fpp_spd_cmd_t* p_spd, fpp_spd_action_t spd_action, uint32_t sa_id, uint32_t spi);

/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */

bool demo_spd_ld_is_ip6(const fpp_spd_cmd_t* p_spd);
bool demo_spd_ld_is_used_sport(const fpp_spd_cmd_t* p_spd);
bool demo_spd_ld_is_used_dport(const fpp_spd_cmd_t* p_spd);

uint16_t         demo_spd_ld_get_position(const fpp_spd_cmd_t* p_spd);
const uint32_t*  demo_spd_ld_get_saddr(const fpp_spd_cmd_t* p_spd);
const uint32_t*  demo_spd_ld_get_daddr(const fpp_spd_cmd_t* p_spd);
uint16_t         demo_spd_ld_get_sport(const fpp_spd_cmd_t* p_spd);
uint16_t         demo_spd_ld_get_dport(const fpp_spd_cmd_t* p_spd);
uint8_t          demo_spd_ld_get_protocol(const fpp_spd_cmd_t* p_spd);
uint32_t         demo_spd_ld_get_sa_id(const fpp_spd_cmd_t* p_spd);
uint32_t         demo_spd_ld_get_spi(const fpp_spd_cmd_t* p_spd);
fpp_spd_action_t demo_spd_ld_get_action(const fpp_spd_cmd_t* p_spd);

/* ==== PUBLIC FUNCTIONS : misc ============================================ */

int demo_spd_print_by_phyif(FCI_CLIENT* p_cl, demo_spd_cb_print_t p_cb_print, const char* p_phyif_name,
                            uint16_t position_init, uint16_t count);
int demo_spd_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_phyif_name);

/* ========================================================================= */

#endif
