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

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "libfci_cli_common.h"
#include "libfci_cli_def_opts.h"
#include "libfci_cli_print_helpers.h"
#include "libfci_cli_def_optarg_keywords.h"
#include "libfci_cli_cmds_mirror.h"  /* needed for mirror_print() */
#include "libfci_cli_cmds_if_mac.h"  /* needed for if_mac_print() */
#include "libfci_cli_cmds_if.h"


/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_common.h"
#include "libfci_demo/demo_phy_if.h"
#include "libfci_demo/demo_log_if.h"
#include "libfci_demo/demo_if_mac.h"
#include "libfci_demo/demo_mirror.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

typedef int (*cb_cmdexec_t)(const cli_cmdargs_t* p_cmdargs);

/* ==== PRIVATE FUNCTIONS : prints for LOGIF =============================== */

static int logif_print_aux(const fpp_log_if_cmd_t* p_logif, bool is_verbose, bool is_nested_in_phyif)
{
    assert(NULL != p_logif);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = ((is_nested_in_phyif) ? (6) : (0));
    
    printf("%-*s%2"PRIu32": %s\n", indent, "",
           demo_log_if_ld_get_id(p_logif),
           demo_log_if_ld_get_name(p_logif));
    
    indent += 6;  /* detailed interface info is indented deeper */
    
    printf("%-*s<%s>\n", indent, "", cli_value2txt_en_dis(demo_log_if_ld_is_enabled(p_logif)));
    
    printf("%-*s<promisc:%s, match-mode:%s, discard-on-match:%s>\n", indent, "",
           cli_value2txt_on_off(demo_log_if_ld_is_promisc(p_logif)),
           cli_value2txt_or_and(demo_log_if_ld_is_match_mode_or(p_logif)),
           cli_value2txt_on_off(demo_log_if_ld_is_discard_on_m(p_logif)));
    
    printf("%-*saccepted: %"PRIu32" rejected: %"PRIu32" discarded: %"PRIu32" processed: %"PRIu32"\n", indent, "",
           demo_log_if_ld_get_stt_accepted(p_logif), 
           demo_log_if_ld_get_stt_rejected(p_logif), 
           demo_log_if_ld_get_stt_discarded(p_logif), 
           demo_log_if_ld_get_stt_processed(p_logif));
    
    if (!is_nested_in_phyif)
    {
        printf("%-*sparent: %s\n", indent, "",
               demo_log_if_ld_get_parent_name(p_logif));
    }
    
    printf("%-*segress: ", indent, "");
    cli_print_bitset32(demo_log_if_ld_get_egress(p_logif), ",", cli_value2txt_phyif, "---");
    printf("\n");
    
    printf("%-*smatch-rules: ", indent, "");
    cli_print_bitset32(demo_log_if_ld_get_mr_bitset(p_logif), ",", cli_value2txt_match_rule, "---");
    printf("\n");
    
    /* verbose info - match rule arguments (only if corresponding match rule active) */
    if (is_verbose)
    {
        indent += 2u;  /* verbose info is indented even deeper */
        
        const fpp_if_m_rules_t match_rules = demo_log_if_ld_get_mr_bitset(p_logif);
        
        if (FPP_IF_MATCH_VLAN & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__VLAN": %"PRIu16"\n", indent, "",
                   demo_log_if_ld_get_mr_arg_vlan(p_logif));
        }
        if (FPP_IF_MATCH_PROTO & match_rules)
        {
            const uint8_t proto = demo_log_if_ld_get_mr_arg_proto(p_logif);
            printf("%-*s"TXT_MATCH_RULE__PROTOCOL": %"PRIu8" (%s)\n", indent, "",
                   (proto), cli_value2txt_protocol(proto));
        }
        if (FPP_IF_MATCH_SPORT & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__SPORT": %"PRIu16"\n", indent, "",
                   demo_log_if_ld_get_mr_arg_sport(p_logif));
        }
        if (FPP_IF_MATCH_DPORT & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__DPORT": %"PRIu16"\n", indent, "",
                   demo_log_if_ld_get_mr_arg_dport(p_logif));
        }
        if (FPP_IF_MATCH_SIP6 & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__SIP6": ", indent, "");
            cli_print_ip6(demo_log_if_ld_get_mr_arg_sip6(p_logif));
            printf("\n");
        }
        if (FPP_IF_MATCH_DIP6 & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__DIP6": ", indent, "");
            cli_print_ip6(demo_log_if_ld_get_mr_arg_dip6(p_logif));
            printf("\n");
        }
        if (FPP_IF_MATCH_SIP & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__SIP": ", indent, "");
            cli_print_ip4(demo_log_if_ld_get_mr_arg_sip(p_logif), false);
            printf("\n");
        }
        if (FPP_IF_MATCH_DIP & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__DIP": ", indent, "");
            cli_print_ip4(demo_log_if_ld_get_mr_arg_dip(p_logif), false);
            printf("\n");
        }
        if (FPP_IF_MATCH_ETHTYPE & match_rules)
        {
            const uint16_t ethtype = demo_log_if_ld_get_mr_arg_ethtype(p_logif);
            printf("%-*s"TXT_MATCH_RULE__ETHER_TYPE": %"PRIu16" (0x%04"PRIx16")\n", indent, "", 
                   (ethtype), (ethtype));
        }
        if (FPP_IF_MATCH_FP0 & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__FP_TABLE0": %s\n", indent, "",
                   demo_log_if_ld_get_mr_arg_fp0(p_logif));
        }
        if (FPP_IF_MATCH_FP1 & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__FP_TABLE1": %s\n", indent, "",
                   demo_log_if_ld_get_mr_arg_fp1(p_logif));
        }
        if (FPP_IF_MATCH_SMAC & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__SMAC": ", indent, "");
            cli_print_mac(demo_log_if_ld_get_mr_arg_smac(p_logif));
            printf("\n");
        }
        if (FPP_IF_MATCH_DMAC & match_rules)
        {
            printf("%-*s"TXT_MATCH_RULE__DMAC": ", indent, "");
            cli_print_mac(demo_log_if_ld_get_mr_arg_dmac(p_logif));
            printf("\n");
        }
        if (FPP_IF_MATCH_HIF_COOKIE & match_rules)
        {
            const uint32_t hif_cookie = demo_log_if_ld_get_mr_arg_hif_cookie(p_logif);
            printf("%-*s"TXT_MATCH_RULE__HIF_COOKIE": %"PRIu32" (0x%04"PRIx32")\n", indent, "",
                   (hif_cookie), (hif_cookie));
        }
    }
    
    return (FPP_ERR_OK);
}

static inline int logif_print(const fpp_log_if_cmd_t* p_logif)
{
    return logif_print_aux(p_logif, false, false);
}

static inline int logif_print_verbose(const fpp_log_if_cmd_t* p_logif)
{
    return logif_print_aux(p_logif, true, false);
}

static inline int logif_print_in_phyif(const fpp_log_if_cmd_t* p_logif)
{
    return logif_print_aux(p_logif, false, true);
}

static inline int logif_print_in_phyif_verbose(const fpp_log_if_cmd_t* p_logif)
{
    return logif_print_aux(p_logif, true, true);
}

/* ==== PRIVATE FUNCTIONS : prints for PHYIF =============================== */

static int phyif_print_aux(const fpp_phy_if_cmd_t* p_phyif, bool is_verbose)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_phyif);
    UNUSED(is_verbose);
    
    
    int rtn = FPP_ERR_OK;
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*s%2"PRIu32": %s\n", indent, "",
           demo_phy_if_ld_get_id(p_phyif),
           demo_phy_if_ld_get_name(p_phyif));
           
    indent += 6u;  /* detailed info is indented deeper */
    
    printf("%-*s<%s>\n", indent, "", cli_value2txt_en_dis(demo_phy_if_ld_is_enabled(p_phyif)));
    
    printf("%-*s<promisc:%s, mode:%s, block-state:%s>\n", indent, "",
           cli_value2txt_on_off(demo_phy_if_ld_is_promisc(p_phyif)),
           cli_value2txt_if_mode(demo_phy_if_ld_get_mode(p_phyif)),
           cli_value2txt_if_block_state(demo_phy_if_ld_get_block_state(p_phyif)));
    
    printf("%-*s<vlan-conf:%s, ptp-conf:%s, ptp-promisc:%s, q-in-q:%s>\n", indent, "", 
           cli_value2txt_on_off(demo_phy_if_ld_is_vlan_conf(p_phyif)),
           cli_value2txt_on_off(demo_phy_if_ld_is_ptp_conf(p_phyif)),
           cli_value2txt_on_off(demo_phy_if_ld_is_ptp_promisc(p_phyif)),
           cli_value2txt_on_off(demo_phy_if_ld_is_qinq(p_phyif)));
    
    printf("%-*s<discard-if-ttl-below-2:%s, fast-forward-all-tcp:%s>\n", indent, "",
           cli_value2txt_on_off(demo_phy_if_ld_is_discard_ttl(p_phyif)),
           cli_value2txt_on_off(demo_phy_if_ld_is_ff_all_tcp(p_phyif)));
    
    printf("%-*singress: %"PRIu32" egress: %"PRIu32" discarded: %"PRIu32" malformed: %"PRIu32"\n", indent, "",
           demo_phy_if_ld_get_stt_ingress(p_phyif), 
           demo_phy_if_ld_get_stt_egress(p_phyif),
           demo_phy_if_ld_get_stt_discarded(p_phyif), 
           demo_phy_if_ld_get_stt_malformed(p_phyif));
    
    {
        const char* p_txt = demo_phy_if_ld_get_flexifilter(p_phyif);
        if ('\0' == p_txt[0])
        {
            p_txt = "---";
        }
        printf("%-*sflexible-filter: %s \n", indent, "", p_txt);
    }
    
    {
        const char* p_txt = demo_phy_if_ld_get_ptp_mgmt_if(p_phyif);
        if ('\0' == p_txt[0])
        {
            p_txt = "---";
        }
        printf("%-*sptp-mgmt-if: %s \n", indent, "", p_txt);
    }
    
    {
        uint32_t mac_count = 0uL;
        rtn = demo_if_mac_get_count_by_name(cli_p_cl, &mac_count, demo_phy_if_ld_get_name(p_phyif));
        if (FPP_ERR_OK == rtn)
        {
            if (0 == mac_count)
            {
                printf("%-*sMAC: --- \n", indent, "");
            }
            else
            {
                printf("%-*sMAC: \n", indent, "");
                demo_if_mac_print_by_name(cli_p_cl, if_mac_print_in_phyif, demo_phy_if_ld_get_name(p_phyif));
            }
        }
    }
    
    printf("%-*smirrors: \n", indent, "");
    {
        int indent_mirror = (indent + 4);
        fpp_mirror_cmd_t mirror = {0};
        
        /* HACK, part 1: Temporarily unlock interface database, because FCI processing of mirroring rules assumes the db is not locked and attempts to lock it (and throws an error if the db is already locked). */
        rtn = demo_if_session_unlock(cli_p_cl, rtn);
        
        /* rx mirrors */
        for (uint8_t i = 0u; ((FPP_ERR_OK == rtn) && (FPP_MIRRORS_CNT > i)); ++i)
        {
            const char* p_str = demo_phy_if_ld_get_rx_mirror(p_phyif, i);
            printf("%-*srxmirr%"PRIu8": ", indent_mirror, "", i);
            if ('\0' == p_str[0])
            {
                printf("--- \n");
            }
            else
            {
                rtn = demo_mirror_get_by_name(cli_p_cl, &mirror, p_str);
                if (FPP_ERR_OK == rtn)
                {
                    rtn = mirror_print_in_phyif(&mirror, is_verbose);
                }
            }
        }
        
        /* tx mirrors */
        for (uint8_t i = 0u; ((FPP_ERR_OK == rtn) && (FPP_MIRRORS_CNT > i)); ++i)
        {
            const char* p_str = demo_phy_if_ld_get_tx_mirror(p_phyif, i);
            printf("%-*stxmirr%"PRIu8": ", indent_mirror, "", i);
            if ('\0' == p_str[0])
            {
                printf("--- \n");
            }
            else
            {
                rtn = demo_mirror_get_by_name(cli_p_cl, &mirror, p_str);
                if (FPP_ERR_OK == rtn)
                {
                    rtn = mirror_print_in_phyif(&mirror, is_verbose);
                }
            }
        }
        
        /* HACK, part 2: Lock the interface database, so the interface query can continue. */
        rtn = demo_if_session_lock(cli_p_cl);
    }
    
    {
        uint32_t logif_cnt = 0uL;
        rtn = demo_log_if_get_count(cli_p_cl, &logif_cnt, demo_phy_if_ld_get_name(p_phyif)); 
        printf("%-*slogical interfaces: ", indent, "");
        if (0 == logif_cnt)
        {
            printf("---");
        }
        else
        {
            /* empty */
            /* logical interfaces are printed via logif_print fncs */
        }
        printf("\n");
    }
    
    return (rtn);
}

static inline int phyif_print(const fpp_phy_if_cmd_t* p_phyif)
{   
    phyif_print_aux(p_phyif, false);
    return demo_log_if_print_all(cli_p_cl, logif_print_in_phyif, demo_phy_if_ld_get_name(p_phyif));
}

static inline int phyif_print_verbose(const fpp_phy_if_cmd_t* p_phyif)
{   
    phyif_print_aux(p_phyif, true);
    return demo_log_if_print_all(cli_p_cl, logif_print_in_phyif_verbose, demo_phy_if_ld_get_name(p_phyif));
}

/* ==== PRIVATE FUNCTIONS : PHYIF cmds ===================================== */

static int exec_inside_locked_session(cb_cmdexec_t p_cb_cmdexec, const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cb_cmdexec);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    rtn = demo_if_session_lock(cli_p_cl);
    if (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_cmdexec(p_cmdargs);
    }
    rtn = demo_if_session_unlock(cli_p_cl, rtn);
    
    return (rtn);
}

static int stt_cmd_phyif_print(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_phy_if_cmd_t phyif = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    const demo_phy_if_cb_print_t p_cb_print = ((p_cmdargs->verbose.is_valid) ? (phyif_print_verbose) : (phyif_print));
    if (p_cmdargs->if_name.is_valid)
    {
        /* print a single interface */
        rtn = demo_phy_if_get_by_name(cli_p_cl, &phyif, (p_cmdargs->if_name.txt));    
        if (FPP_ERR_OK == rtn)
        {
            rtn = p_cb_print(&phyif);
        }
    }
    else
    {
        /* print all interfaces */
        rtn = demo_phy_if_print_all(cli_p_cl, p_cb_print);
    }
    
    return (rtn);
}

static int stt_cmd_phyif_update(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_phy_if_cmd_t phyif = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_phy_if_get_by_name(cli_p_cl, &phyif, (p_cmdargs->if_name.txt));
    }
    
    /* modify local data - bitflags */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->enable_noreply.is_valid)
        {
            demo_phy_if_ld_enable(&phyif);
        }
        if (p_cmdargs->disable_noorig.is_valid)
        {
            demo_phy_if_ld_disable(&phyif);
        }
        if (p_cmdargs->promisc.is_valid)
        {
            demo_phy_if_ld_set_promisc(&phyif, (p_cmdargs->promisc.is_on));
        }
        if (p_cmdargs->vlan_conf__x_src.is_valid)
        {
            demo_phy_if_ld_set_vlan_conf(&phyif, (p_cmdargs->vlan_conf__x_src.is_on));
        }
        if (p_cmdargs->ptp_conf__x_dst.is_valid)
        {
            demo_phy_if_ld_set_ptp_conf(&phyif, (p_cmdargs->ptp_conf__x_dst.is_on));
        }
        if (p_cmdargs->ptp_promisc.is_valid)
        {
            demo_phy_if_ld_set_ptp_promisc(&phyif, (p_cmdargs->ptp_promisc.is_on));
        }
        if (p_cmdargs->ff_all_tcp.is_valid)
        {
            demo_phy_if_ld_set_ff_all_tcp(&phyif, (p_cmdargs->ff_all_tcp.is_on));
        }
        if (p_cmdargs->qinq.is_valid)
        {
            demo_phy_if_ld_set_qinq(&phyif, (p_cmdargs->qinq.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->discard_if_ttl_below_2.is_valid))
        {
            demo_phy_if_ld_set_discard_ttl(&phyif, (p_cmdargs->discard_if_ttl_below_2.is_on));
        }
    }
    
    /* modify local data - misc configuration */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->if_mode.is_valid)
        {
            demo_phy_if_ld_set_mode(&phyif, (p_cmdargs->if_mode.value));
        }
        if (p_cmdargs->if_block_state.is_valid)
        {
            demo_phy_if_ld_set_block_state(&phyif, (p_cmdargs->if_block_state.value));
        }
        if (p_cmdargs->ruleA0_name.is_valid)  /* OPT_RX_MIRROR0 */
        {
            demo_phy_if_ld_set_rx_mirror(&phyif, 0, (p_cmdargs->ruleA0_name.txt));
        }
        if (p_cmdargs->ruleA1_name.is_valid)  /* OPT_RX_MIRROR1 */
        {
            demo_phy_if_ld_set_rx_mirror(&phyif, 1, (p_cmdargs->ruleA1_name.txt));
        }
        if (p_cmdargs->ruleB0_name.is_valid)  /* OPT_TX_MIRROR0 */
        {
            demo_phy_if_ld_set_tx_mirror(&phyif, 0, (p_cmdargs->ruleB0_name.txt));
        }
        if (p_cmdargs->ruleB1_name.is_valid)  /* OPT_TX_MIRROR1 */
        {
            demo_phy_if_ld_set_tx_mirror(&phyif, 1, (p_cmdargs->ruleB1_name.txt));
        }
        if (p_cmdargs->table0_name.is_valid)  /* OPT_FLEXIBLE_FILTER */
        {
            demo_phy_if_ld_set_flexifilter(&phyif, (p_cmdargs->table0_name.txt));
        }
        if (p_cmdargs->ptp_mgmt_if_name.is_valid)
        {
            demo_phy_if_ld_set_ptp_mgmt_if(&phyif, p_cmdargs->ptp_mgmt_if_name.txt);
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_phy_if_update(cli_p_cl, &phyif);
    }
    
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : LOGIF cmds ===================================== */

static int stt_cmd_logif_print(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_log_if_cmd_t logif = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    const demo_log_if_cb_print_t p_cb_print = ((p_cmdargs->verbose.is_valid) ? (logif_print_verbose) : (logif_print));
    if (p_cmdargs->if_name.is_valid)
    {
        /*  print a single interface  */
        rtn = demo_log_if_get_by_name(cli_p_cl, &logif, (p_cmdargs->if_name.txt));
        if (FPP_ERR_OK == rtn)
        {
            rtn = p_cb_print(&logif);
        }
    }
    else
    {
        /* print all interfaces */
        rtn = demo_log_if_print_all(cli_p_cl, p_cb_print, NULL);
    }

    return (rtn);
}

static int stt_cmd_logif_update(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_log_if_cmd_t logif = {0};
    const fpp_if_m_rules_t match_rules = ((p_cmdargs->match_rules.is_valid) ? (p_cmdargs->match_rules.bitset) : (0));
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE,   NULL, (p_cmdargs->if_name.is_valid)},
        
        /* these are mandatory only if the related match rule is requested */
        {OPT_VLAN,        NULL, ((FPP_IF_MATCH_VLAN    & match_rules) ? (p_cmdargs->vlan.is_valid)          : (true))},
        {OPT_PROTOCOL,    NULL, ((FPP_IF_MATCH_PROTO   & match_rules) ? (p_cmdargs->protocol.is_valid)      : (true))},
        {OPT_SPORT,       NULL, ((FPP_IF_MATCH_SPORT   & match_rules) ? (p_cmdargs->sport.is_valid)         : (true))},
        {OPT_DPORT,       NULL, ((FPP_IF_MATCH_DPORT   & match_rules) ? (p_cmdargs->dport.is_valid)         : (true))},
        {OPT_SIP6,        NULL, ((FPP_IF_MATCH_SIP6    & match_rules) ? (p_cmdargs->sip2.is_valid)          : (true))},
        {OPT_DIP6,        NULL, ((FPP_IF_MATCH_DIP6    & match_rules) ? (p_cmdargs->dip2.is_valid)          : (true))},
        {OPT_SIP,         NULL, ((FPP_IF_MATCH_SIP     & match_rules) ? (p_cmdargs->sip.is_valid)           : (true))},
        {OPT_DIP,         NULL, ((FPP_IF_MATCH_DIP     & match_rules) ? (p_cmdargs->dip.is_valid)           : (true))},
        {OPT_ETHTYPE,     NULL, ((FPP_IF_MATCH_ETHTYPE & match_rules) ? (p_cmdargs->count_ethtype.is_valid) : (true))},
        {OPT_TABLE0,      NULL, ((FPP_IF_MATCH_FP0     & match_rules) ? (p_cmdargs->table0_name.is_valid)   : (true))},
        {OPT_TABLE1,      NULL, ((FPP_IF_MATCH_FP1     & match_rules) ? (p_cmdargs->table1_name.is_valid)   : (true))},
        {OPT_SMAC,        NULL, ((FPP_IF_MATCH_SMAC    & match_rules) ? (p_cmdargs->smac.is_valid)          : (true))},
        {OPT_DMAC,        NULL, ((FPP_IF_MATCH_DMAC    & match_rules) ? (p_cmdargs->dmac.is_valid)          : (true))},
        {OPT_HIF_COOKIE,  NULL, ((FPP_IF_MATCH_HIF_COOKIE & match_rules) ? (p_cmdargs->data_hifc_sad.is_valid) : (true))},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* late opt arg check to ensure that sip/dip are IPv4 and sip2/dip2 are IPv6 (specialty of this cli cmd) */
    if (FPP_ERR_OK == rtn)
    {
        if (((p_cmdargs->sip.is_valid)  &&  (p_cmdargs->sip.is6))  || 
            ((p_cmdargs->dip.is_valid)  &&  (p_cmdargs->dip.is6))  ||
            ((p_cmdargs->sip2.is_valid) && !(p_cmdargs->sip2.is6)) || 
            ((p_cmdargs->dip2.is_valid) && !(p_cmdargs->dip2.is6)))
        {
            rtn = CLI_ERR_WRONG_IP_TYPE;
        }
    }
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_log_if_get_by_name(cli_p_cl, &logif, (p_cmdargs->if_name.txt));
    }
    
    /* modify local data - match rules */
    if ((FPP_ERR_OK == rtn) && (p_cmdargs->match_rules.is_valid))
    {
        /* clear any previous rules */
        demo_log_if_ld_clear_all_mr(&logif);
        
        /* set non-argument rules */
        if (FPP_IF_MATCH_TYPE_ETH & match_rules)
        {
            demo_log_if_ld_set_mr_type_eth(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_VLAN & match_rules)
        {
            demo_log_if_ld_set_mr_type_vlan(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_PPPOE & match_rules)
        {
            demo_log_if_ld_set_mr_type_pppoe(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_ARP & match_rules)
        {
            demo_log_if_ld_set_mr_type_arp(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_MCAST & match_rules)
        {
            demo_log_if_ld_set_mr_type_mcast(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_IPV4 & match_rules)
        {
            demo_log_if_ld_set_mr_type_ip4(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_IPV6 & match_rules)
        {
            demo_log_if_ld_set_mr_type_ip6(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_IPX & match_rules)
        {
            demo_log_if_ld_set_mr_type_ipx(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_BCAST & match_rules)
        {
            demo_log_if_ld_set_mr_type_bcast(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_UDP & match_rules)
        {
            demo_log_if_ld_set_mr_type_udp(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_TCP & match_rules)
        {
            demo_log_if_ld_set_mr_type_tcp(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_ICMP & match_rules)
        {
            demo_log_if_ld_set_mr_type_icmp(&logif, true);
        }
        if (FPP_IF_MATCH_TYPE_IGMP & match_rules)
        {
            demo_log_if_ld_set_mr_type_igmp(&logif, true);
        }
        
        /* set argument rules */
        if (FPP_IF_MATCH_VLAN & match_rules)
        {
            demo_log_if_ld_set_mr_vlan(&logif, true, (p_cmdargs->vlan.value));
        }
        if (FPP_IF_MATCH_PROTO & match_rules)
        {
            demo_log_if_ld_set_mr_proto(&logif, true, (p_cmdargs->protocol.value));
        }
        if (FPP_IF_MATCH_SPORT & match_rules)
        {
            demo_log_if_ld_set_mr_sport(&logif, true, (p_cmdargs->sport.value));
        }
        if (FPP_IF_MATCH_DPORT & match_rules)
        {
            demo_log_if_ld_set_mr_dport(&logif, true, (p_cmdargs->dport.value));
        } 
        if (FPP_IF_MATCH_SIP6 & match_rules)
        {
            demo_log_if_ld_set_mr_sip6(&logif, true, (p_cmdargs->sip2.arr));
        }
        if (FPP_IF_MATCH_DIP6 & match_rules)
        {
            demo_log_if_ld_set_mr_dip6(&logif, true, (p_cmdargs->dip2.arr));
        }
        if (FPP_IF_MATCH_SIP & match_rules)
        {
            demo_log_if_ld_set_mr_sip(&logif, true, (p_cmdargs->sip.arr[0]));
        }
        if (FPP_IF_MATCH_DIP & match_rules)
        {
            demo_log_if_ld_set_mr_dip(&logif, true, (p_cmdargs->dip.arr[0]));
        }
        if (FPP_IF_MATCH_ETHTYPE & match_rules)
        {
            demo_log_if_ld_set_mr_ethtype(&logif, true, (p_cmdargs->count_ethtype.value));
        }
        if (FPP_IF_MATCH_FP0 & match_rules)
        {
            demo_log_if_ld_set_mr_fp0(&logif, true, (p_cmdargs->table0_name.txt));
        }
        if (FPP_IF_MATCH_FP1 & match_rules)
        {
            demo_log_if_ld_set_mr_fp1(&logif, true, (p_cmdargs->table1_name.txt));
        }
        if (FPP_IF_MATCH_SMAC & match_rules)
        {
            demo_log_if_ld_set_mr_smac(&logif, true, (p_cmdargs->smac.arr));
        }
        if (FPP_IF_MATCH_DMAC & match_rules)
        {
            demo_log_if_ld_set_mr_dmac(&logif, true, (p_cmdargs->dmac.arr));
        }
        if (FPP_IF_MATCH_HIF_COOKIE & match_rules)
        {
            demo_log_if_ld_set_mr_hif_cookie(&logif, true, (p_cmdargs->data_hifc_sad.value));
        }
    }
    
    /* modify local data - bitflags + egress */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->enable_noreply.is_valid)
        {
            demo_log_if_ld_enable(&logif);
        }
        if (p_cmdargs->disable_noorig.is_valid)
        {
            demo_log_if_ld_disable(&logif);
        }
        if (p_cmdargs->promisc.is_valid)
        {
            demo_log_if_ld_set_promisc(&logif, (p_cmdargs->promisc.is_on));
        }
        if (p_cmdargs->match_mode.is_valid)
        {
            demo_log_if_ld_set_match_mode_or(&logif, (p_cmdargs->match_mode.is_or));
        }
        if (p_cmdargs->discard_on_match.is_valid)
        {
            demo_log_if_ld_set_discard_on_m(&logif, (p_cmdargs->discard_on_match.is_on));
        }
        if (p_cmdargs->egress.is_valid)
        {
            demo_log_if_ld_set_egress_phyifs(&logif, (p_cmdargs->egress.bitset));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_log_if_update(cli_p_cl, &logif);
    }
    
    return (rtn);
}

static int stt_cmd_logif_add(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_PARENT,    NULL, (p_cmdargs->if_name_parent.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_log_if_add(cli_p_cl, 0, (p_cmdargs->if_name.txt), (p_cmdargs->if_name_parent.txt));
    }
    
    return (rtn);
}

static int stt_cmd_logif_del(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_log_if_del(cli_p_cl, (p_cmdargs->if_name.txt));
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS =================================================== */

inline int cli_cmd_phyif_print(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_phyif_print, p_cmdargs);
}

inline int cli_cmd_phyif_update(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_phyif_update, p_cmdargs);
}

inline int cli_cmd_logif_print(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_logif_print, p_cmdargs);
}

inline int cli_cmd_logif_update(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_logif_update, p_cmdargs);
}

inline int cli_cmd_logif_add(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_logif_add, p_cmdargs);
}

int cli_cmd_logif_del(const cli_cmdargs_t* p_cmdargs)
{
    return exec_inside_locked_session(stt_cmd_logif_del, p_cmdargs);
}

/* ==== TESTMODE constants ================================================= */

#if !defined(NDEBUG)
/* empty */
#endif

/* ========================================================================= */
