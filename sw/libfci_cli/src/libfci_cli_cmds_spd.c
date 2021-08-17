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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "libfci_cli_common.h"
#include "libfci_cli_def_opts.h"
#include "libfci_cli_print_helpers.h"
#include "libfci_cli_def_optarg_keywords.h"
#include "libfci_cli_cmds_spd.h"

#include "libfci_interface/fci_spd.h"

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
    
    
    {
        printf("[%5"PRIu16"]", (p_spd->position));
    }
    {
        unsigned int padding = 0u;
        
        const char* p_txt = cli_value2txt_spd_action(p_spd->spd_action);
        padding += strlen(p_txt);
        printf("  ;  [ %s", p_txt);
        
        switch (p_spd->spd_action)
        {
            case FPP_SPD_ACTION_PROCESS_ENCODE:
                p_txt = " sad=";
                padding += strlen(p_txt) + 10u;  /* counting in fixed 10 characters of the associated numeric value */
                printf("%s%-10"PRIu32, p_txt, (p_spd->sa_id));
            break;
            
            case FPP_SPD_ACTION_PROCESS_DECODE:
                p_txt = " spi=0x";
                padding += strlen(p_txt) + 8u;  /* counting in fixed 8 characters of the associated numeric value */
                printf("%s%08"PRIx32, p_txt, (p_spd->spi));
            break;
            
            default:
                p_txt = NULL;
                padding += 0uL;
            break;
        }
        
        padding = ((21u >= padding) ? (21u - padding) : (0u));  /* 21 chars is maximal expected length of this section */
        printf("%-*s ]", padding, "");
    }
    {
        /* protocol */
        const char* p_txt = cli_value2txt_protocol(p_spd->protocol);
        printf("  ;  %-10s %3"PRIu16, p_txt, (p_spd->protocol));
    }
    {
        /* IP */
        uint32_t tmpbuf_ipaddr[4] = {0u};  /* use tmp buf to enforce correct alignment */
        
        printf("  ;  src=");
        memcpy(tmpbuf_ipaddr, (p_spd->saddr), sizeof(uint32_t)*4);
        if (fci_spd_ld_is_ip6(p_spd))
        {
            cli_print_ip6(tmpbuf_ipaddr);
        }
        else
        {
            cli_print_ip4(tmpbuf_ipaddr[0], true);
        }
        
        printf("  dst=");
        memcpy(tmpbuf_ipaddr, (p_spd->daddr), sizeof(uint32_t)*4);
        if (fci_spd_ld_is_ip6(p_spd))
        {
            cli_print_ip6(tmpbuf_ipaddr);
        }
        else
        {
            cli_print_ip4(tmpbuf_ipaddr[0], true);
        }
    }
    {
        /* port */
        if (fci_spd_ld_is_used_sport(p_spd))
        {
            printf("  sport=%-5"PRIu16, (p_spd->sport));
        }
        else
        {
            printf("  sport= --- ");
        }
        
        if (fci_spd_ld_is_used_dport(p_spd))
        {
            printf("  dport=%-5"PRIu16, (p_spd->dport));
        }
        else
        {
            printf("  dport= --- ");
        }
    }
    
    printf("\n");
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
        rtn = fci_spd_print_by_phyif(cli_p_cl, spd_print, (p_cmdargs->if_name.txt), pos, cnt);
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
        if (FPP_ERR_OK == rtn)
        {
            rtn = fci_spd_ld_set_protocol(&spd, (p_cmdargs->protocol.value));
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = fci_spd_ld_set_ip(&spd, (p_cmdargs->sip.arr), (p_cmdargs->dip.arr), (p_cmdargs->sip.is6));
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = fci_spd_ld_set_port(&spd, (p_cmdargs->sport.is_valid), (p_cmdargs->sport.value), 
                                            (p_cmdargs->dport.is_valid), (p_cmdargs->dport.value));
        }
        if (FPP_ERR_OK == rtn)
        {
            rtn = fci_spd_ld_set_action(&spd, (p_cmdargs->spd_action.value), 
                                              (p_cmdargs->data_hifc_sad.value), (p_cmdargs->mask_spi.value));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        const uint16_t pos = ((p_cmdargs->offset.is_valid) ? (p_cmdargs->offset.value) : (UINT16_MAX));
        rtn = fci_spd_add(cli_p_cl, (p_cmdargs->if_name.txt), pos, &spd);
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
        rtn = fci_spd_del(cli_p_cl, (p_cmdargs->if_name.txt), (p_cmdargs->offset.value));
    }
    
    return (rtn);
}

/* ========================================================================= */
