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
#include "libfci_cli_cmds_if.h"

#include "libfci_interface/fci_common.h"
#include "libfci_interface/fci_phy_if.h"
#include "libfci_interface/fci_log_if.h"

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
    
    printf("%-*s%2"PRIu32": %s: %s\n", indent, "",
           (p_logif->id),
           (p_logif->name),
           cli_value2txt_en_dis(fci_log_if_ld_is_enabled(p_logif)));
    
    indent += 6;  /* detailed interface info is indented deeper */
    
    printf("%-*s<promisc:%s, match-mode:%s, discard-on-match:%s, loopback:%s>\n", indent, "",
           cli_value2txt_on_off(fci_log_if_ld_is_promisc(p_logif)),
           cli_value2txt_or_and(fci_log_if_ld_is_match_mode_or(p_logif)),
           cli_value2txt_on_off(fci_log_if_ld_is_discard_on_m(p_logif)),
           cli_value2txt_on_off(fci_log_if_ld_is_loopback(p_logif)));
    
    printf("%-*saccepted: %"PRIu32" rejected: %"PRIu32" discarded: %"PRIu32" processed: %"PRIu32"\n", indent, "",
           (p_logif->stats.accepted), (p_logif->stats.rejected), (p_logif->stats.discarded), (p_logif->stats.processed));
    
    if (!is_nested_in_phyif)
    {
        printf("%-*sparent: %s\n", indent, "",
               (p_logif->parent_name));
    }
    
    printf("%-*segress: ", indent, "");
    cli_print_bitset32((p_logif->egress), ",", cli_value2txt_phyif, "---");
    printf("\n");
    
    printf("%-*smatch-rules: ", indent, "");
    cli_print_bitset32((p_logif->match), ",", cli_value2txt_match_rule, "---");
    printf("\n");
    
    /* verbose info - match rule arguments (only if corresponding match rule active) */
    if (is_verbose)
    {
        indent += 2u;  /* verbose info is indented even deeper */
        
        if (FPP_IF_MATCH_VLAN & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__VLAN": %"PRIu16"\n", indent, "",
                   (p_logif->arguments.vlan));
        }
        if (FPP_IF_MATCH_PROTO & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__PROTOCOL": %"PRIu8" (%s)\n", indent, "",
                   (p_logif->arguments.proto), cli_value2txt_protocol(p_logif->arguments.proto));
        }
        if (FPP_IF_MATCH_SPORT & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__SPORT": %"PRIu16"\n", indent, "",
                   (p_logif->arguments.sport));
        }
        if (FPP_IF_MATCH_DPORT & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__DPORT": %"PRIu16"\n", indent, "",
                   (p_logif->arguments.dport));
        }
        if (FPP_IF_MATCH_SIP6 & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__SIP6": ", indent, "");
            cli_print_ip6(p_logif->arguments.ipv.v6.sip);
            printf("\n");
        }
        if (FPP_IF_MATCH_DIP6 & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__DIP6": ", indent, "");
            cli_print_ip6(p_logif->arguments.ipv.v6.dip);
            printf("\n");
        }
        if (FPP_IF_MATCH_SIP & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__SIP": ", indent, "");
            cli_print_ip4(p_logif->arguments.ipv.v4.sip, false);
            printf("\n");
        }
        if (FPP_IF_MATCH_DIP & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__DIP": ", indent, "");
            cli_print_ip4(p_logif->arguments.ipv.v4.dip, false);
            printf("\n");
        }
        if (FPP_IF_MATCH_ETHTYPE & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__ETHER_TYPE": %"PRIu16" (0x%04"PRIx16")\n", indent, "", 
                   (p_logif->arguments.ethtype), (p_logif->arguments.ethtype));
        }
        if (FPP_IF_MATCH_FP0 & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__FP_TABLE0": %s\n", indent, "",
                   (p_logif->arguments.fp_table0));
        }
        if (FPP_IF_MATCH_FP1 & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__FP_TABLE1": %s\n", indent, "",
                   (p_logif->arguments.fp_table1));
        }
        if (FPP_IF_MATCH_SMAC & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__SMAC": ", indent, "");
            cli_print_mac(p_logif->arguments.smac);
            printf("\n");
        }
        if (FPP_IF_MATCH_DMAC & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__DMAC": ", indent, "");
            cli_print_mac(p_logif->arguments.dmac);
            printf("\n");
        }
        if (FPP_IF_MATCH_HIF_COOKIE & (p_logif->match))
        {
            printf("%-*s"TXT_MATCH_RULE__HIF_COOKIE": %"PRIu32" (0x%04"PRIx32")\n", indent, "",
                   (p_logif->arguments.hif_cookie), (p_logif->arguments.hif_cookie));
        }
    }
    
    return (FPP_ERR_OK);
}

static inline int logif_print(const fpp_log_if_cmd_t* p_if)
{
    return logif_print_aux(p_if, false, false);
}

static inline int logif_print_verbose(const fpp_log_if_cmd_t* p_if)
{
    return logif_print_aux(p_if, true, false);
}

static inline int logif_print_in_phyif(const fpp_log_if_cmd_t* p_if)
{
    return logif_print_aux(p_if, false, true);
}

static inline int logif_print_in_phyif_verbose(const fpp_log_if_cmd_t* p_if)
{
    return logif_print_aux(p_if, true, true);
}

/* ==== PRIVATE FUNCTIONS : prints for PHYIF =============================== */

static int phyif_print_aux(const fpp_phy_if_cmd_t* p_phyif, bool is_verbose)
{
    assert(NULL != p_phyif);
    UNUSED(is_verbose);
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*s%2"PRIu32": %s: %s\n", indent, "",
           (p_phyif->id),
           (p_phyif->name),
           cli_value2txt_en_dis(fci_phy_if_ld_is_enabled(p_phyif)));
           
    indent += 6u;  /* detailed info is indented deeper */
    
    printf("%-*s<promisc:%s, mode:%s, block-state:%s, loadbalance:%s>\n", indent, "",
           cli_value2txt_on_off(fci_phy_if_ld_is_promisc(p_phyif)),
           cli_value2txt_if_mode(p_phyif->mode),
           cli_value2txt_if_block_state(p_phyif->block_state), 
           cli_value2txt_on_off(fci_phy_if_ld_is_loadbalance(p_phyif)));
    
    printf("%-*s<vlan-conf:%s, ptp-conf:%s, ptp-promisc:%s, q-in-q:%s>\n", indent, "", 
           cli_value2txt_on_off(fci_phy_if_ld_is_vlan_conf(p_phyif)),
           cli_value2txt_on_off(fci_phy_if_ld_is_ptp_conf(p_phyif)),
           cli_value2txt_on_off(fci_phy_if_ld_is_ptp_promisc(p_phyif)),
           cli_value2txt_on_off(fci_phy_if_ld_is_qinq(p_phyif)));
    
    printf("%-*s<discard-if-ttl-below-2:%s>\n", indent, "", 
           cli_value2txt_on_off(fci_phy_if_ld_is_discard_ttl(p_phyif)));
    
    printf("%-*singress: %"PRIu32" egress: %"PRIu32" discarded: %"PRIu32" malformed: %"PRIu32"\n", indent, "",
           (p_phyif->stats.ingress), (p_phyif->stats.egress), (p_phyif->stats.discarded), (p_phyif->stats.malformed));
    
    printf("%-*sMAC: ", indent, "");
    cli_print_mac(p_phyif->mac_addr);
    printf("\n");
    
    printf("%-*smirror: (%s) ", indent, "",
           cli_value2txt_on_off(fci_phy_if_ld_is_mirror(p_phyif)));
    cli_print_tablenames(&(p_phyif->mirror), 1u, "", "---");
    printf("\n");
    
    printf("%-*sflexible-filter: ", indent, "");
    cli_print_tablenames(&(p_phyif->ftable), 1u, "", "---");
    printf("\n");
    
    return (FPP_ERR_OK);
}

static inline int phyif_print(const fpp_phy_if_cmd_t* p_if)
{   
    phyif_print_aux(p_if, false);
    return fci_log_if_print_by_parent(cli_p_cl, logif_print_in_phyif, (p_if->name));
}

static inline int phyif_print_verbose(const fpp_phy_if_cmd_t* p_if)
{   
    phyif_print_aux(p_if, true);
    return fci_log_if_print_by_parent(cli_p_cl, logif_print_in_phyif_verbose, (p_if->name));
}

/* ==== PRIVATE FUNCTIONS : cmds =========================================== */

static int exec_inside_locked_session(cb_cmdexec_t p_cb_cmdexec, const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cb_cmdexec);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    rtn = fci_if_session_lock(cli_p_cl);
    if (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_cmdexec(p_cmdargs);
    }
    rtn = fci_if_session_unlock(cli_p_cl, rtn);
    
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
    const fci_phy_if_cb_print_t p_cb_print = ((p_cmdargs->verbose.is_valid) ? (phyif_print_verbose) : (phyif_print));
    if (p_cmdargs->if_name.is_valid)
    {
        /* print single interface */
        rtn = fci_phy_if_get_by_name(cli_p_cl, &phyif, (p_cmdargs->if_name.txt));    
        if (FPP_ERR_OK == rtn)
        {
            rtn = p_cb_print(&phyif);
        }
    }
    else
    {
        /* print all interfaces */
        rtn = fci_phy_if_print_all(cli_p_cl, p_cb_print);
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
    const mandopt_t mandopts[] = {{OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}};
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_phy_if_get_by_name(cli_p_cl, &phyif, (p_cmdargs->if_name.txt));
    }
    
    /* modify local data - bitflags */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->enable_noreply.is_valid))
        {
            rtn = fci_phy_if_ld_enable(&phyif);
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->disable_noorig.is_valid))
        {
            rtn = fci_phy_if_ld_disable(&phyif);
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->promisc.is_valid))
        {
            rtn = fci_phy_if_ld_set_promisc(&phyif, (p_cmdargs->promisc.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->loadbalance__ttl_decr.is_valid))
        {
            rtn = fci_phy_if_ld_set_loadbalance(&phyif, (p_cmdargs->loadbalance__ttl_decr.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->vlan_conf__x_src.is_valid))
        {
            rtn = fci_phy_if_ld_set_vlan_conf(&phyif, (p_cmdargs->vlan_conf__x_src.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->ptp_conf__x_dst.is_valid))
        {
            rtn = fci_phy_if_ld_set_ptp_conf(&phyif, (p_cmdargs->ptp_conf__x_dst.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->ptp_promisc.is_valid))
        {
            rtn = fci_phy_if_ld_set_ptp_promisc(&phyif, (p_cmdargs->ptp_promisc.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->qinq.is_valid))
        {
            rtn = fci_phy_if_ld_set_qinq(&phyif, (p_cmdargs->qinq.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->discard_if_ttl_below_2.is_valid))
        {
            rtn = fci_phy_if_ld_set_discard_ttl(&phyif, (p_cmdargs->discard_if_ttl_below_2.is_on));
        }
    }
    
    /* modify local data - misc configuration */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->if_mode.is_valid))
        {
            rtn = fci_phy_if_ld_set_mode(&phyif, (p_cmdargs->if_mode.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->if_block_state.is_valid))
        {
            rtn = fci_phy_if_ld_set_block_state(&phyif, (p_cmdargs->if_block_state.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->if_name_mirror.is_valid))
        {
            rtn = fci_phy_if_ld_set_mirror(&phyif, (p_cmdargs->if_name_mirror.txt));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->table0_name.is_valid))
        {
            rtn = fci_phy_if_ld_set_flexifilter(&phyif, (p_cmdargs->table0_name.txt));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_phy_if_update(cli_p_cl, &phyif);
    }
    
    return (rtn);
}

static int stt_cmd_logif_print(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_log_if_cmd_t logif = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    const fci_log_if_cb_print_t p_cb_print = ((p_cmdargs->verbose.is_valid) ? (logif_print_verbose) : (logif_print));
    if (p_cmdargs->if_name.is_valid)
    {
        /*  print single interface  */
        rtn = fci_log_if_get_by_name(cli_p_cl, &logif, (p_cmdargs->if_name.txt));
        if (FPP_ERR_OK == rtn)
        {
            rtn = p_cb_print(&logif);
        }
    }
    else
    {
        /* print all interfaces */
        rtn = fci_log_if_print_all(cli_p_cl, p_cb_print);
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
        rtn = fci_log_if_get_by_name(cli_p_cl, &logif, (p_cmdargs->if_name.txt));
    }
    
    /* modify local data - match rules */
    if ((FPP_ERR_OK == rtn) && (p_cmdargs->match_rules.is_valid))
    {
        /* shortcut - set all requested match rules at once (even those which require args) */
        fci_log_if_ld_clear_all_mr(&logif);
        logif.match = match_rules;
        
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_VLAN & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_vlan(&logif, true, (p_cmdargs->vlan.value));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_PROTO & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_proto(&logif, true, (p_cmdargs->protocol.value));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_SPORT & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_sport(&logif, true, (p_cmdargs->sport.value));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_DPORT & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_dport(&logif, true, (p_cmdargs->dport.value));
        } 
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_SIP6 & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_sip6(&logif, true, (p_cmdargs->sip2.arr));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_DIP6 & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_dip6(&logif, true, (p_cmdargs->dip2.arr));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_SIP & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_sip(&logif, true, (p_cmdargs->sip.arr[0]));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_DIP & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_dip(&logif, true, (p_cmdargs->dip.arr[0]));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_ETHTYPE & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_ethtype(&logif, true, (p_cmdargs->count_ethtype.value));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_FP0 & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_fp0(&logif, true, (p_cmdargs->table0_name.txt));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_FP1 & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_fp1(&logif, true, (p_cmdargs->table1_name.txt));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_SMAC & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_smac(&logif, true, (p_cmdargs->smac.arr));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_DMAC & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_dmac(&logif, true, (p_cmdargs->dmac.arr));
        }
        if ((FPP_ERR_OK == rtn) && (FPP_IF_MATCH_HIF_COOKIE & match_rules))
        {
            rtn = fci_log_if_ld_set_mr_hif_cookie(&logif, true, (p_cmdargs->data_hifc_sad.value));
        }
    }
    
    /* modify local data - bitflags + egress */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->enable_noreply.is_valid))
        {
            rtn = fci_log_if_ld_enable(&logif);
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->disable_noorig.is_valid))
        {
            rtn = fci_log_if_ld_disable(&logif);
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->promisc.is_valid))
        {
            rtn = fci_log_if_ld_set_promisc(&logif, (p_cmdargs->promisc.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->loopback.is_valid))
        {
            rtn = fci_log_if_ld_set_loopback(&logif, (p_cmdargs->loopback.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->match_mode.is_valid))
        {
            rtn = fci_log_if_ld_set_match_mode_or(&logif, (p_cmdargs->match_mode.is_or));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->discard_on_match.is_valid))
        {
            rtn = fci_log_if_ld_set_discard_on_m(&logif, (p_cmdargs->discard_on_match.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->egress.is_valid))
        {
            rtn = fci_log_if_ld_set_egress_phyifs(&logif, (p_cmdargs->egress.bitset));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_log_if_update(cli_p_cl, &logif);
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
        rtn = fci_log_if_add(cli_p_cl, 0, (p_cmdargs->if_name.txt), (p_cmdargs->if_name_parent.txt));
    }
    
    return (rtn);
}

static int stt_cmd_logif_del(const cli_cmdargs_t* p_cmdargs)
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
        rtn = fci_log_if_del(cli_p_cl, (p_cmdargs->if_name.txt));
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
