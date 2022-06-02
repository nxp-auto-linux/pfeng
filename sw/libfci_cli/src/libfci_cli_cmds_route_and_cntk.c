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
#include "libfci_demo/demo_rt_ct.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

static bool stt_do_header_print = false;

/* ==== PRIVATE FUNCTIONS : print route ==================================== */

static void rt_header_print(void)
{
    printf("| route      | IP   | src-mac           | dst-mac           | egress interface |\n"
           "|============|======|===================|===================|==================|\n");
}

static int rt_print(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    
    if (stt_do_header_print)
    {
        rt_header_print();
        stt_do_header_print = false;
    }
    
    {
        printf("| %10"PRIu32, demo_rt_ld_get_route_id(p_rt));
    }
    {
        const char* p_txt = ((demo_rt_ld_is_ip4(p_rt)) ? (TXT_PROTOCOL__IPv4) :
                            ((demo_rt_ld_is_ip6(p_rt)) ? (TXT_PROTOCOL__IPv6) : "???"));
        printf(" | %4s", p_txt);
    }
    {
        printf(" | ");
        cli_print_mac(demo_rt_ld_get_src_mac(p_rt));
        printf(" | ");
        cli_print_mac(demo_rt_ld_get_dst_mac(p_rt));
    }
    {
        printf(" | %-15s ", demo_rt_ld_get_egress_phyif(p_rt));
    }
    
    printf(" |\n");
    return (FPP_ERR_OK);
}

/* ==== PRIVATE FUNCTIONS : print conntrack ================================ */

static void ct_print_aux_flags(bool isA, const char* p_txtA, bool isB, const char* p_txtB, bool isC, const char* p_txtC)
{
    assert(NULL != p_txtA);
    assert(NULL != p_txtB);
    assert(NULL != p_txtC);
    
    if (isA || isB || isC)
    {
        
        p_txtA = (isA) ? (p_txtA) : ("");
        p_txtB = (isB) ? (p_txtB) : ("");
        p_txtC = (isC) ? (p_txtC) : ("");
        
        printf("[ %s%s%s]", p_txtA, p_txtB, p_txtC);
    }
    else 
    {
        printf("[ --- ]");
    }
}

static int ct_print(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*sconntrack:\n", indent, "");
    
    indent += 4;
    
    {
        const uint16_t protocol = demo_ct_ld_get_protocol(p_ct);
        printf("%-*sproto:   %"PRIu16" (%s)\n", indent, "",
               protocol, cli_value2txt_protocol(protocol));
    }
    
    {
        printf("%-*sflags:   ", indent, "");
        
        ct_print_aux_flags(demo_ct_ld_is_ttl_decr(p_ct),   "TTL_DECR ",
                           demo_ct_ld_is_reply_only(p_ct), "NO_ORIG ",    /* NOTE: negative logic */
                           demo_ct_ld_is_orig_only(p_ct),  "NO_REPLY ");  /* NOTE: negative logic */
        printf(" ; ");
        ct_print_aux_flags(demo_ct_ld_is_nat(p_ct), "NAT ",
                           demo_ct_ld_is_pat(p_ct), "PAT ",
                           demo_ct_ld_is_vlan_tagging(p_ct), "VLAN_TAGGING ");
        printf("\n");
    }
    
    {
        /* orig dir info */
        printf("%-*sorig:    ", indent, "");
        
        printf("src=");
        cli_print_ip4(demo_ct_ld_get_saddr(p_ct), true);
        
        printf("    dst=");
        cli_print_ip4(demo_ct_ld_get_daddr(p_ct), true);
        
        printf("    sport=%-5"PRIu16,  demo_ct_ld_get_sport(p_ct));
        printf("    dport=%-5"PRIu16,  demo_ct_ld_get_dport(p_ct));
        printf("    vlan=%-5"PRIu16,   demo_ct_ld_get_vlan(p_ct));
        printf("    route=%-10"PRIu32, demo_ct_ld_get_route_id(p_ct));
        
        printf("\n");
    }
    
    {
        /* reply dir info */
        printf("%-*sreply: ", indent, "");
        
        printf("r-src=");
        cli_print_ip4(demo_ct_ld_get_saddr_reply(p_ct), true);
        
        printf("  r-dst=");
        cli_print_ip4(demo_ct_ld_get_daddr_reply(p_ct), true);
        
        printf("  r-sport=%-5"PRIu16,  demo_ct_ld_get_sport_reply(p_ct));
        printf("  r-dport=%-5"PRIu16,  demo_ct_ld_get_dport_reply(p_ct));
        printf("  r-vlan=%-5"PRIu16,   demo_ct_ld_get_vlan_reply(p_ct));
        printf("  r-route=%-10"PRIu32, demo_ct_ld_get_route_id_reply(p_ct));
        
        printf("\n");
    }
    
    {
        /* statistics */
        printf("%-*sstats:  ", indent, "");
        
        printf(" orig_hit: %"PRIu32, demo_ct_ld_get_stt_hit(p_ct));
        printf(" orig_hit_bytes: %"PRIu32, demo_ct_ld_get_stt_hit_bytes(p_ct));
        printf("  ");
        printf(" reply_hit: %"PRIu32, demo_ct_ld_get_stt_reply_hit(p_ct));
        printf(" reply_hit_bytes: %"PRIu32, demo_ct_ld_get_stt_reply_hit_bytes(p_ct));
        
        printf("\n");
    }
    
    return (FPP_ERR_OK); 
}

static int ct6_print(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*sconntrack:\n", indent, "");
    
    indent += 4;
    
    {
        const uint16_t protocol = demo_ct6_ld_get_protocol(p_ct6);
        printf("%-*sproto:   %"PRIu16" (%s)\n", indent, "",
               (protocol), cli_value2txt_protocol(protocol));
    }
    
    {
        printf("%-*sflags:   ", indent, "");
        
        ct_print_aux_flags(demo_ct6_ld_is_ttl_decr(p_ct6),   "TTL_DECR ",
                           demo_ct6_ld_is_reply_only(p_ct6), "NO_ORIG ",    /* NOTE: negative logic */
                           demo_ct6_ld_is_orig_only(p_ct6),  "NO_REPLY ");  /* NOTE: negative logic */
        printf(" ; ");
        ct_print_aux_flags(demo_ct6_ld_is_nat(p_ct6), "NAT ",
                           demo_ct6_ld_is_pat(p_ct6), "PAT ",
                           demo_ct6_ld_is_vlan_tagging(p_ct6), "VLAN_TAGGING ");
        printf("\n");
    }
    
    {
        /* orig dir info */
        printf("%-*sorig:    ", indent, "");
        
        printf("src=");
        cli_print_ip6(demo_ct6_ld_get_saddr(p_ct6));
        
        printf("    dst=");
        cli_print_ip6(demo_ct6_ld_get_daddr(p_ct6));
        
        printf("    sport=%-5"PRIu16,  demo_ct6_ld_get_sport(p_ct6));
        printf("    dport=%-5"PRIu16,  demo_ct6_ld_get_dport(p_ct6));
        printf("    vlan=%-5"PRIu16,   demo_ct6_ld_get_vlan(p_ct6));
        printf("    route=%-10"PRIu32, demo_ct6_ld_get_route_id(p_ct6));
        
        printf("\n");
    }
    
    {
        /* reply dir info */
        printf("%-*sreply: ", indent, "");
        
        printf("r-src=");
        cli_print_ip6(demo_ct6_ld_get_saddr_reply(p_ct6));
        
        printf("  r-dst=");
        cli_print_ip6(demo_ct6_ld_get_daddr_reply(p_ct6));
        
        printf("  r-sport=%-5"PRIu16,  demo_ct6_ld_get_sport_reply(p_ct6));
        printf("  r-dport=%-5"PRIu16,  demo_ct6_ld_get_dport_reply(p_ct6));
        printf("  r-vlan=%-5"PRIu16,   demo_ct6_ld_get_vlan_reply(p_ct6));
        printf("  r-route=%-10"PRIu32, demo_ct6_ld_get_route_id_reply(p_ct6));
        
        printf("\n");
    }
    
    {
        /* statistics */
        printf("%-*sstats:   ", indent, "");
        
        printf(" orig_hit: %"PRIu32, demo_ct6_ld_get_stt_hit(p_ct6));
        printf(" orig_hit_bytes: %"PRIu32, demo_ct6_ld_get_stt_hit_bytes(p_ct6));
        printf("   ");
        printf(" reply_hit: %"PRIu32, demo_ct6_ld_get_stt_reply_hit(p_ct6));
        printf(" reply_hit_bytes: %"PRIu32, demo_ct6_ld_get_stt_reply_hit_bytes(p_ct6));
        
        printf("\n");
    }
    
    return (FPP_ERR_OK); 
}

/* ==== PUBLIC FUNCTIONS : route =========================================== */

int cli_cmd_route_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_rt_cmd_t rt = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    stt_do_header_print = true;
    if (p_cmdargs->route.is_valid)
    {
        /* print single route */
        rtn = demo_rt_get_by_id(cli_p_cl, &rt, (p_cmdargs->route.value));
        if (FPP_ERR_OK == rtn)
        {
            rtn = rt_print(&rt);
        }
    }
    else if (p_cmdargs->ip4.is_valid)
    {
        /* print all IPv4 routes */
        rtn = demo_rt_print_all(cli_p_cl, rt_print, true, false);
    }
    else if (p_cmdargs->ip6.is_valid)
    {
        /* print all IPv6 routes */
        rtn = demo_rt_print_all(cli_p_cl, rt_print, false, true);
    }
    else
    {
        /* print all routes */
        rtn = demo_rt_print_all(cli_p_cl, rt_print, true, true);
    }
    
    return (rtn);
}

int cli_cmd_route_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_rt_cmd_t rt = {0};
    
    /* check for mandatory opts */
    const mandopt_optbuf_t ip46 = {{OPT_IP4, OPT_IP6}};
    const mandopt_t mandopts[] = 
    {
        {OPT_ROUTE,     NULL,   (p_cmdargs->route.is_valid)},
        {OPT_NONE,      &ip46, ((p_cmdargs->ip4.is_valid) || (p_cmdargs->ip6.is_valid))},
        {OPT_DMAC,      NULL,   (p_cmdargs->dmac.is_valid)},
        {OPT_INTERFACE, NULL,   (p_cmdargs->if_name.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    /* empty (no 'init data' from PFE) */
    
    /* modify local data - set IP type */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->ip4.is_valid)
        {
            demo_rt_ld_set_as_ip4(&rt);
        }
        else if (p_cmdargs->ip6.is_valid)
        {
            demo_rt_ld_set_as_ip6(&rt);
        }
        else
        {
            rtn = CLI_ERR;  /* should never happen */
        }
    }
    
    /* modify local data - smac (optional) */
    if ((FPP_ERR_OK == rtn) && (p_cmdargs->smac.is_valid))
    {
        demo_rt_ld_set_src_mac(&rt, (p_cmdargs->smac.arr));
    }
    
    /* modify local data - dmac */
    if (FPP_ERR_OK == rtn)
    {
        demo_rt_ld_set_dst_mac(&rt, (p_cmdargs->dmac.arr));
        demo_rt_ld_set_egress_phyif(&rt, (p_cmdargs->if_name.txt));
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_rt_add(cli_p_cl, (p_cmdargs->route.value), &rt);
    }
    
    return (rtn);
}

int cli_cmd_route_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_ROUTE, NULL,  (p_cmdargs->route.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_rt_del(cli_p_cl, (p_cmdargs->route.value));
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : conntrack ======================================= */

int cli_cmd_cntk_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = FPP_ERR_OK;  /* must be initially OK for the following algorithm to work */
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    const bool print_all = (!(p_cmdargs->ip4.is_valid) && !(p_cmdargs->ip6.is_valid));
    if ((FPP_ERR_OK == rtn) && ((p_cmdargs->ip4.is_valid) || print_all))
    {
        rtn = demo_ct_print_all(cli_p_cl, ct_print);
    }
    if ((FPP_ERR_OK == rtn) && ((p_cmdargs->ip6.is_valid) || print_all))
    {
        rtn = demo_ct6_print_all(cli_p_cl, ct6_print);
    }
    
    return (rtn);
}

int cli_cmd_cntk_update(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_PROTOCOL, NULL,  (p_cmdargs->protocol.is_valid)},
        {OPT_SIP,      NULL,  (p_cmdargs->sip.is_valid)},
        {OPT_DIP,      NULL,  (p_cmdargs->dip.is_valid)},
        {OPT_SPORT,    NULL,  (p_cmdargs->sport.is_valid)},
        {OPT_DPORT,    NULL,  (p_cmdargs->dport.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* late opt arg check to ensure that all IP-related cli opts are either IPv4, or IPv6 (no mixing allowed) */
    /* check optional IP-related cli opts as well (if they are valid) */
    if (FPP_ERR_OK == rtn)
    {
        const bool is6 = (p_cmdargs->sip.is6);
        if (((p_cmdargs->sip.is6) != (p_cmdargs->dip.is6)) ||
            ((p_cmdargs->sip2.is_valid) && ((p_cmdargs->sip2.is6) != is6)) ||
            ((p_cmdargs->dip2.is_valid) && ((p_cmdargs->dip2.is6) != is6)))
        {
            rtn = CLI_ERR_INCOMPATIBLE_IPS;
        }
    }
    
    /* get init local data */
    /* empty (each conntrack type has its own init data routine) */
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->sip.is6)
        {
            fpp_ct6_cmd_t ct6 = {0};
            
            /* fill in the 5-tuple data */
            demo_ct6_ld_set_protocol(&ct6, (p_cmdargs->protocol.value));
            demo_ct6_ld_set_orig_dir(&ct6, (p_cmdargs->sip.arr), (p_cmdargs->dip.arr),
                                           (p_cmdargs->sport.value), (p_cmdargs->dport.value), 
                                            0u, 0uL, false);
            
            /* get init local data */
            rtn = demo_ct6_get_by_tuple(cli_p_cl, &ct6, &ct6);
            
            /* modify local data */
            if (CLI_OK == rtn)
            {
                if (p_cmdargs->ttl_decr.is_valid)
                {
                    demo_ct6_ld_set_ttl_decr(&ct6, (p_cmdargs->ttl_decr.is_on));
                }
            }
            
            /* exec */
            if (FPP_ERR_OK == rtn)
            {
                rtn = demo_ct6_update(cli_p_cl, &ct6);
            }
        }
        else
        {
            fpp_ct_cmd_t ct = {0};
            
            /* fill in the 5-tuple data */
            demo_ct_ld_set_protocol(&ct, (p_cmdargs->protocol.value));
            demo_ct_ld_set_orig_dir(&ct, (p_cmdargs->sip.arr[0]), (p_cmdargs->dip.arr[0]),
                                         (p_cmdargs->sport.value), (p_cmdargs->dport.value), 
                                          0u, 0uL, false);
            
            /* get init local data */
            rtn = demo_ct_get_by_tuple(cli_p_cl, &ct, &ct);
            
            /* modify local data */
            if (CLI_OK == rtn)
            {
                if (p_cmdargs->ttl_decr.is_valid)
                {
                    demo_ct_ld_set_ttl_decr(&ct, (p_cmdargs->ttl_decr.is_on));
                }
            }
            
            /* exec */
            if (FPP_ERR_OK == rtn)
            {
                rtn = demo_ct_update(cli_p_cl, &ct);
            }
        }
    }
    
    return (rtn);
}

int cli_cmd_cntk_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_PROTOCOL, NULL,  (p_cmdargs->protocol.is_valid)},
        {OPT_SIP,      NULL,  (p_cmdargs->sip.is_valid)},
        {OPT_DIP,      NULL,  (p_cmdargs->dip.is_valid)},
        {OPT_SPORT,    NULL,  (p_cmdargs->sport.is_valid)},
        {OPT_DPORT,    NULL,  (p_cmdargs->dport.is_valid)},
        {OPT_ROUTE,    NULL,  (p_cmdargs->route.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* late opt arg check to ensure that all IP-related cli opts are either IPv4, or IPv6 (no mixing allowed) */
    /* check optional IP-related cli opts as well (if they are valid) */
    if (FPP_ERR_OK == rtn)
    {
        const bool is6 = (p_cmdargs->sip.is6);
        if (((p_cmdargs->sip.is6) != (p_cmdargs->dip.is6)) ||
            ((p_cmdargs->sip2.is_valid) && ((p_cmdargs->sip2.is6) != is6)) ||
            ((p_cmdargs->dip2.is_valid) && ((p_cmdargs->dip2.is6) != is6)))
        {
            rtn = CLI_ERR_INCOMPATIBLE_IPS;
        }
    }
    
    /* get init local data */
    /* empty (no 'init data' from PFE) */
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        /*
            NOTE:   If reply opts ('r-XXX') are not cli-specified, they are filled with the "cross" value from the orig data.
                    Example: if 'r-sip' not specified by user, it is by default filled with 'dip' value.
                    For details, see the FCI API Reference.
        */
        const uint32_t* p_saddr_reply = ((p_cmdargs->sip2.is_valid)   ? (p_cmdargs->sip2.arr)     : (p_cmdargs->dip.arr));
        const uint32_t* p_daddr_reply = ((p_cmdargs->dip2.is_valid)   ? (p_cmdargs->dip2.arr)     : (p_cmdargs->sip.arr));
        const uint16_t    sport_reply = ((p_cmdargs->sport2.is_valid) ? (p_cmdargs->sport2.value) : (p_cmdargs->dport.value));
        const uint16_t    dport_reply = ((p_cmdargs->dport2.is_valid) ? (p_cmdargs->dport2.value) : (p_cmdargs->sport.value));
        const uint32_t route_id_reply = ((p_cmdargs->route2.is_valid) ? (p_cmdargs->route2.value) : (p_cmdargs->route.value));
        const uint16_t     vlan_reply = ((p_cmdargs->vlan2.is_valid)  ? (p_cmdargs->vlan2.value)  : (p_cmdargs->vlan.value));
        
        if (p_cmdargs->sip.is6)
        {
            fpp_ct6_cmd_t ct6 = {0};
            
            /* prepare data for an IPv6 conntrack */
            {
                demo_ct6_ld_set_protocol(&ct6, (p_cmdargs->protocol.value));
                
                demo_ct6_ld_set_orig_dir(&ct6, (p_cmdargs->sip.arr), (p_cmdargs->dip.arr),
                                               (p_cmdargs->sport.value), (p_cmdargs->dport.value),
                                               (p_cmdargs->vlan.value), (p_cmdargs->route.value),
                                               (p_cmdargs->enable_noreply.is_valid));
                                               
                demo_ct6_ld_set_reply_dir(&ct6, p_saddr_reply, p_daddr_reply,
                                                sport_reply, dport_reply,
                                                vlan_reply, route_id_reply,
                                                (p_cmdargs->disable_noorig.is_valid));
            }
            
            /* exec - create IPv6 conntrack */
            if (FPP_ERR_OK == rtn)
            {
                rtn = demo_ct6_add(cli_p_cl, &ct6);
            }
            
            /* WORKAROUND - ttl decrement is accessible only via the update command */
            if ((FPP_ERR_OK == rtn) && (p_cmdargs->ttl_decr.is_valid))
            {
                /* modify local data */
                demo_ct6_ld_set_ttl_decr(&ct6, (p_cmdargs->ttl_decr.is_on));
                
                /* exec */
                if (FPP_ERR_OK == rtn)
                {
                    rtn = demo_ct6_update(cli_p_cl, &ct6);
                }
            }
        }
        else
        {
            fpp_ct_cmd_t ct = {0};
                        
            /* prepare data for an IPv4 conntrack */
            {
                demo_ct_ld_set_protocol(&ct, (p_cmdargs->protocol.value));
                
                demo_ct_ld_set_orig_dir(&ct, (p_cmdargs->sip.arr[0]),  (p_cmdargs->dip.arr[0]),
                                             (p_cmdargs->sport.value), (p_cmdargs->dport.value),
                                             (p_cmdargs->vlan.value),  (p_cmdargs->route.value), 
                                             (p_cmdargs->enable_noreply.is_valid));
            
                demo_ct_ld_set_reply_dir(&ct, p_saddr_reply[0], p_daddr_reply[0],
                                              sport_reply, dport_reply,
                                              vlan_reply, route_id_reply, 
                                              (p_cmdargs->disable_noorig.is_valid));
            }
            
            /* exec - create IPv4 conntrack */
            if (FPP_ERR_OK == rtn)
            {
                rtn = demo_ct_add(cli_p_cl, &ct);
            }
            
            /* WORKAROUND - ttl decrement is accessible only via update command */
            if ((FPP_ERR_OK == rtn) && (p_cmdargs->ttl_decr.is_valid))
            {
                /* modify local data */
                demo_ct_ld_set_ttl_decr(&ct, (p_cmdargs->ttl_decr.is_on));
                
                /* exec */
                if (FPP_ERR_OK == rtn)
                {
                    rtn = demo_ct_update(cli_p_cl, &ct);
                }
            }
        }
    }
    
    return (rtn);
}

int cli_cmd_cntk_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_PROTOCOL, NULL,  (p_cmdargs->protocol.is_valid)},
        {OPT_SIP,      NULL,  (p_cmdargs->sip.is_valid)},
        {OPT_DIP,      NULL,  (p_cmdargs->dip.is_valid)},
        {OPT_SPORT,    NULL,  (p_cmdargs->sport.is_valid)},
        {OPT_DPORT,    NULL,  (p_cmdargs->dport.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* late opt arg check to ensure that all IP-related cli opts are either IPv4, or IPv6 (no mixing allowed) */
    if ((FPP_ERR_OK == rtn) && ((p_cmdargs->sip.is6) != (p_cmdargs->dip.is6)))
    {
        rtn = CLI_ERR_INCOMPATIBLE_IPS;
    }
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->sip.is6)
        {
            fpp_ct6_cmd_t ct6 = {0};
            
            /* prepare data for IPv6 conntrack destruction */
            demo_ct6_ld_set_protocol(&ct6, (p_cmdargs->protocol.value));
            demo_ct6_ld_set_orig_dir(&ct6, (p_cmdargs->sip.arr), (p_cmdargs->dip.arr),
                                           (p_cmdargs->sport.value), (p_cmdargs->dport.value),
                                            0u, 0uL, false);
            
            /* exec - destroy IPv6 conntrack */
            if (FPP_ERR_OK == rtn)
            {
                rtn = demo_ct6_del(cli_p_cl, &ct6);
            }
        }
        else
        {
            fpp_ct_cmd_t ct = {0};
            
            /* prepare data for IPv4 conntrack destruction */
            demo_ct_ld_set_protocol(&ct, (p_cmdargs->protocol.value));
            demo_ct_ld_set_orig_dir(&ct, (p_cmdargs->sip.arr[0]), (p_cmdargs->dip.arr[0]),
                                         (p_cmdargs->sport.value), (p_cmdargs->dport.value),
                                          0u, 0uL, false);
            
            /*  exec - destroy IPv4 conntrack  */
            if (FPP_ERR_OK == rtn)
            {
                rtn = demo_ct_del(cli_p_cl, &ct);
            }
        }
    }
    
    return (rtn);
}

int cli_cmd_cntk_timeout(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_PROTOCOL, NULL,  (p_cmdargs->protocol.is_valid)},
        {OPT_TIMEOUT,  NULL,  (p_cmdargs->timeout.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        if (6u == (p_cmdargs->protocol.value))  /* 6u == TCP protocol ID */
        {
            rtn = demo_ct_timeout_tcp(cli_p_cl, (p_cmdargs->timeout.value));
        }
        else if (17u == (p_cmdargs->protocol.value))  /* 17u == UDP protocol ID */
        {
            rtn = demo_ct_timeout_udp(cli_p_cl, (p_cmdargs->timeout.value));
        }
        else
        {
            rtn = demo_ct_timeout_others(cli_p_cl, (p_cmdargs->timeout.value));
        }
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : route and conntrack reset ======================= */

int cli_cmd_route_and_cntk_reset(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_optbuf_t a46 = {{OPT_ALL, OPT_IP4, OPT_IP6}};
    const mandopt_t mandopts[] = 
    {
        {OPT_NONE, &a46, ((p_cmdargs->all.is_valid) || (p_cmdargs->ip4.is_valid) || (p_cmdargs->ip6.is_valid))},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && ((p_cmdargs->ip4.is_valid) || (p_cmdargs->all.is_valid)))
        {
            rtn = demo_rtct_reset_ip4(cli_p_cl);
        }
        if ((FPP_ERR_OK == rtn) && ((p_cmdargs->ip6.is_valid) || (p_cmdargs->all.is_valid)))
        {
            rtn = demo_rtct_reset_ip6(cli_p_cl);
        }
    }
    
    return (rtn);
}

/* ========================================================================= */
