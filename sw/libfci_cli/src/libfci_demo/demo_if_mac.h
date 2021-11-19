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
 
#ifndef DEMO_IF_MAC_H_
#define DEMO_IF_MAC_H_
 
#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
/* ==== TYPEDEFS & DATA ==================================================== */
 
typedef int (*demo_if_mac_cb_print_t)(const fpp_if_mac_cmd_t* p_if_mac);
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
int demo_if_mac_add(FCI_CLIENT* p_cl, const uint8_t p_mac[6], const char* p_name);
int demo_if_mac_del(FCI_CLIENT* p_cl, const uint8_t p_mac[6], const char* p_name);
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
const char*    demo_if_mac_ld_get_name(const fpp_if_mac_cmd_t* p_if_mac);
const uint8_t* demo_if_mac_ld_get_mac(const fpp_if_mac_cmd_t* p_if_mac);
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
int demo_if_mac_print_by_name(FCI_CLIENT* p_cl, demo_if_mac_cb_print_t p_cb_print, const char* p_name);
int demo_if_mac_get_count_by_name(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, const char* p_name);
 
/* ========================================================================= */
 
#endif
  