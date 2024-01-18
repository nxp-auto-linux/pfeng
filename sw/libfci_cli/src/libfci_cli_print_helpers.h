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

#ifndef CLI_PRINT_HELPERS_H_
#define CLI_PRINT_HELPERS_H_

#include <stdint.h>
#include "libfci_cli_common.h"

/* ==== PUBLIC FUNCTIONS =================================================== */

void cli_print_bitset32(uint32_t bitset, const char* p_txt_delim, const char* (*p_value2txt)(uint8_t value),
                        const char* p_txt_nothing_found);
void cli_print_tablenames(const char (*p_tablenames)[TABLE_NAME_TXT_LN], const uint8_t tablenames_ln,
                          const char* p_txt_delim, const char* p_txt_nothing_found);
void cli_print_mac(const uint8_t* p_mac);
void cli_print_ip4(uint32_t ip4, bool is_fixed_width);
void cli_print_ip6(const uint32_t* p_ip6);

/* ========================================================================= */

#endif
