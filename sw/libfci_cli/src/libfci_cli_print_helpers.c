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

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include "libfci_cli_print_helpers.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== PUBLIC FUNCTIONS =================================================== */

void cli_print_bitset32(uint32_t bitset, const char* p_txt_delim, const char* (*p_value2txt)(uint8_t value),
                        const char* p_txt_nothing_found)
{
    assert(NULL != p_txt_delim);
    assert(NULL != p_value2txt);
    assert(NULL != p_txt_nothing_found);
    
    
    if (0uL == bitset)
    {
        printf("%s", p_txt_nothing_found);
    }
    else
    {
        const char* p_txt_tmp = "";  /* there should be no delimiter in front of the very first item */
        for(uint8_t i = 0u; (32u > i); (++i))
        {
            if (bitset & (1uL << i))
            {
                printf("%s%s", p_txt_tmp, p_value2txt(i));
                p_txt_tmp = p_txt_delim;
            }
        }
    }
}

void cli_print_tablenames(const char (*p_tablenames)[TABLE_NAME_TXT_LN], const uint8_t tablenames_ln,
                          const char* p_txt_delim, const char* p_txt_nothing_found)
{
    assert(NULL != p_tablenames);
    assert(NULL != p_txt_delim);
    assert(NULL != p_txt_nothing_found);
    
    
    bool nothing_found = true;
    const char* p_txt_tmp = "";  /* there should be no delimiter in front of the very first item */
    for (uint8_t i = 0u; (tablenames_ln > i); (++i))
    {
        if ((NULL != p_tablenames[i]) && ('\0' != p_tablenames[i][0]))
        {
            printf("%s%s", p_txt_tmp, p_tablenames[i]);
            p_txt_tmp = p_txt_delim;
            nothing_found = false;
        }
    }
    
    if (nothing_found)
    {
        printf("%s", p_txt_nothing_found);
    }
}

void cli_print_mac(const uint8_t* p_mac)
{
    assert(NULL != p_mac);
    #if (MAC_BYTES_LN != 6u)
    #error Unexpected MAC_BYTES_LN value! If not '6', then change 'printf()' parameters accordingly!
    #endif
    
    
    printf("%02"PRIx8  ":%02"PRIx8  ":%02"PRIx8  ":%02"PRIx8  ":%02"PRIx8  ":%02"PRIx8,
           p_mac[0], p_mac[1], p_mac[2], p_mac[3], p_mac[4], p_mac[5]);
}

void cli_print_ip4(uint32_t ip4, bool is_fixed_width)
{    
    /* WARNING: little endian assumed */
    const uint8_t* p = (const uint8_t*)(&ip4);
    int padding = 0;  /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */
    if (is_fixed_width)
    {
        for (uint8_t i = 0u; ((sizeof(uint32_t)) > i); (++i))
        {
            padding += ((10 > p[i]) ? (2) :
                       ((100 > p[i]) ? (1) : 0));
        }
    }
    
    printf("%"PRIu8  ".%"PRIu8  ".%"PRIu8  ".%"PRIu8  "%-*s",
           p[3], p[2], p[1], p[0], padding, "");
}

void cli_print_ip6(const uint32_t *p_ip6)
{
    assert(NULL != p_ip6);
    #if (IP6_U32S_LN != 4u)
    #error Unexpected IP6_U32S_LN value! If not '4', then change 'printf()' parameters accordingly!
    #endif
    
    
    /* WARNING: little endian assumed */
    const uint8_t* p = (const uint8_t*)(p_ip6);
    printf("%02"PRIx8  "%02"PRIx8  ":%02"PRIx8  "%02"PRIx8 ":"
           "%02"PRIx8  "%02"PRIx8  ":%02"PRIx8  "%02"PRIx8 ":"
           "%02"PRIx8  "%02"PRIx8  ":%02"PRIx8  "%02"PRIx8 ":"
           "%02"PRIx8  "%02"PRIx8  ":%02"PRIx8  "%02"PRIx8,
           p[3] , p[2] , p[1] , p[0],
           p[7] , p[6] , p[5] , p[4],
           p[11], p[10], p[9] , p[8],
           p[15], p[14], p[13], p[12]);
}

/* ==== TESTMODE constants ================================================= */

#if !defined(NDEBUG)
/* empty */
#endif

/* ========================================================================= */
