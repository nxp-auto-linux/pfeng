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
 
#ifndef DEMO_COMMON_H_
#define DEMO_COMMON_H_
 
#include <stdint.h>
#include <stddef.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
/* ==== TYPEDEFS & DATA ==================================================== */
 
 typedef fci_cb_retval_t (*demo_events_cb_t)(unsigned short fcode, unsigned short len, unsigned short* payload);
 
/* ==== PUBLIC FUNCTIONS =================================================== */
 
void print_if_error(int rtn, const char* p_txt_error);
 
void ntoh_enum(void* p_rtn, size_t size);
void hton_enum(void* p_rtn, size_t size);
 
int set_text(char* p_dst, const char* p_src, const uint16_t dst_ln);
 
int demo_if_session_lock(FCI_CLIENT* p_cl);
int demo_if_session_unlock(FCI_CLIENT* p_cl, int rtn);
 
int demo_client_open_in_cmd_mode(FCI_CLIENT** pp_rtn_cl);
int demo_client_close(FCI_CLIENT* p_cl);
 
int demo_events_catching_init(FCI_CLIENT* p_cl, demo_events_cb_t p_cb_events);
int demo_events_catching_fini(FCI_CLIENT* p_cl);
 
/* ========================================================================= */
 
#endif
 
