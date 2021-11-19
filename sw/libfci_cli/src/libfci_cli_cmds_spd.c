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

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "libfci_cli_common.h"
#include "libfci_cli_def_opts.h"
#include "libfci_cli_print_helpers.h"
#include "libfci_cli_def_optarg_keywords.h"
#include "libfci_cli_cmds_route_and_cntk.h"

/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_spd.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

/* ==== PRIVATE FUNCTIONS : prints ========================================= */

static int spd_print(const fpp_spd_cmd_t* p_spd)
{
    assert(NULL != p_spd);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    {
        printf("%-*sentry %"PRIu16":\n", indent, "", demo_spd_ld_get_position(p_spd));
    }
    
    indent += 4;
    
    {
        const uint8_t protocol = demo_spd_ld_get_protocol(p_spd);
        printf("%-*sproto:  %"PRIu8" (%s)\n", indent, "",
               protocol, cli_value2txt_protocol(protocol));
    }
    
    {
        const char* p_txt = cli_value2txt_spd_action(demo_spd_ld_get_action(p_spd));
        printf("%-*saction: %s ", indent, "", p_txt);
        
        /* extra info for some actions */
        switch (demo_spd_ld_get_action(p_spd))
        {
            case FPP_SPD_ACTION_PROCESS_ENCODE:
                printf("(sad=%"PRIu32")", demo_spd_ld_get_sa_id(p_spd));
            break;
            
            case FPP_SPD_ACTION_PROCESS_DECODE:
                printf("(spi=0x%08"PRIx32")", demo_spd_ld_get_spi(p_spd));
            break;
            
            default:
                /* empty */
            break;
        }
        
        printf("\n");
    }
    
    {
        printf("%-*smatch:", indent, "");
        
        {
            const uint32_t* p_saddr = demo_spd_ld_get_saddr(p_spd);
            printf("  src=");
            if (demo_spd_ld_is_ip6(p_spd))
            {
                cli_print_ip6(p_saddr);
            }
            else
            {
                cli_print_ip4(p_saddr[0], true);
            }
        }
        
        {
            const uint32_t* p_daddr = demo_spd_ld_get_daddr(p_spd);
            printf("  dst=");
            if (demo_spd_ld_is_ip6(p_spd))
            {
                cli_print_ip6(p_daddr);
            }
            else
            {
                cli_print_ip4(p_daddr[0], true);
            }
        }
        
        {
            if (demo_spd_ld_is_used_sport(p_spd))
            {
                printf("  sport=%-5"PRIu16, demo_spd_ld_get_sport(p_spd));
            }
            else
            {
                printf("  sport=---  ");
            }
        }
        
        {
            if (demo_spd_ld_is_used_dport(p_spd))
            {
                printf("  dport=%-5"PRIu16, demo_spd_ld_get_dport(p_spd));
            }
            else
            {
                printf("  dport=---   ");
            }
        }
        
        printf("\n");
    }
    
    return (FPP_ERR_OK); 
}

/* ==== PUBLIC FUNCTIONS =================================================== */ 

int cli_cmd_spd_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = {{OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}};
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        const uint16_t pos = (p_cmdargs->offset.is_valid) ? (p_cmdargs->offset.value) : (0u);
        const uint16_t cnt = (p_cmdargs->count_ethtype.is_valid) ? (p_cmdargs->count_ethtype.value) : (0u);
        rtn = demo_spd_print_by_phyif(cli_p_cl, spd_print, (p_cmdargs->if_name.txt), pos, cnt);
    }
    
    return (rtn);
}

int cli_cmd_spd_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_spd_cmd_t spd = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE,  NULL,  (p_cmdargs->if_name.is_valid)},
        {OPT_PROTOCOL,   NULL,  (p_cmdargs->protocol.is_valid)},
        {OPT_SIP,        NULL,  (p_cmdargs->sip.is_valid)},
        {OPT_DIP,        NULL,  (p_cmdargs->dip.is_valid)},
        {OPT_SPD_ACTION, NULL,  (p_cmdargs->spd_action.is_valid)},
        {OPT_SAD,        NULL, ((FPP_SPD_ACTION_PROCESS_ENCODE == (p_cmdargs->spd_action.value)) ? (p_cmdargs->data_hifc_sad.is_valid) : (true))},
        {OPT_SPI,        NULL, ((FPP_SPD_ACTION_PROCESS_DECODE == (p_cmdargs->spd_action.value)) ? (p_cmdargs->mask_spi.is_valid)      : (true))},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* late opt arg check to ensure that all IP-related cli opts are either IPv4, or IPv6 (no mixing allowed) */
    if ((FPP_ERR_OK == rtn) && ((p_cmdargs->sip.is6) != (p_cmdargs->dip.is6)))
    {
        rtn = CLI_ERR_INCOMPATIBLE_IPS;
    }
    
    /* get init local data */
    /* empty (no 'init data' from the PFE) */
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        demo_spd_ld_set_protocol(&spd, (p_cmdargs->protocol.value));
        demo_spd_ld_set_ip(&spd, (p_cmdargs->sip.arr), (p_cmdargs->dip.arr), (p_cmdargs->sip.is6));
        
        demo_spd_ld_set_port(&spd, (p_cmdargs->sport.is_valid), (p_cmdargs->sport.value), 
                                   (p_cmdargs->dport.is_valid), (p_cmdargs->dport.value));
        
        demo_spd_ld_set_action(&spd, (p_cmdargs->spd_action.value), 
                                     (p_cmdargs->data_hifc_sad.value), (p_cmdargs->mask_spi.value));
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        const uint16_t pos = ((p_cmdargs->offset.is_valid) ? (p_cmdargs->offset.value) : (UINT16_MAX));
        rtn = demo_spd_add(cli_p_cl, (p_cmdargs->if_name.txt), pos, &spd);
    }
    
    return (rtn);
}

int cli_cmd_spd_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE,  NULL,  (p_cmdargs->if_name.is_valid)},
        {OPT_POSITION,   NULL,  (p_cmdargs->offset.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_spd_del(cli_p_cl, (p_cmdargs->if_name.txt), (p_cmdargs->offset.value));
    }
    
    return (rtn);
}

/* ========================================================================= */
