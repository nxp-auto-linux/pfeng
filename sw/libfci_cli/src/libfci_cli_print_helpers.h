/* =========================================================================
 *  (c) Copyright 2006-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017, 2019-2021 NXP
 *
 *  NXP Confidential. This software is owned or controlled by NXP and may only
 *  be used strictly in accordance with the applicable license terms. By
 *  expressly accepting such terms or by downloading, installing, activating
 *  and/or otherwise using the software, you are agreeing that you have read,
 *  and that you agree to comply with and are bound by, such license terms. If
 *  you do not agree to be bound by the applicable license terms, then you may
 *  not retain, install, activate or otherwise use the software.
 *
 *  This file contains sample code only. It is not part of the production code deliverables.
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
