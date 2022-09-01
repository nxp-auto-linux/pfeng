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
#include "libfci_cli_cmds_bd.h"

/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_l2_bd.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

/* ==== PRIVATE FUNCTIONS : prints for BD_STENT ============================ */

static int stent_print_aux(const fpp_l2_static_ent_cmd_t* p_stent, bool is_nested_in_bd)
{
    assert(NULL != p_stent);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = ((is_nested_in_bd) ? (8) : (0));
    
    {
        const char* p_txt_local = (demo_l2_stent_ld_is_local(p_stent) ? ("[local address]") : (""));
        printf("%-*sMAC: ", indent, "");
        cli_print_mac(demo_l2_stent_ld_get_mac(p_stent));
        printf("  %s\n", p_txt_local);
    }
    
    indent += 5;  /* detailed static entry info is indented deeper */
    
    if (!is_nested_in_bd)
    {
        printf("%-*svlan: %"PRIu16"\n", indent, "", demo_l2_stent_ld_get_vlan(p_stent));
    }
    
    {
        const char* p_txt_local = (demo_l2_stent_ld_is_local(p_stent) ? (" (ignored when local)") : (""));
        printf("%-*segress%s: ", indent, "", p_txt_local);
        cli_print_bitset32(demo_l2_stent_ld_get_fwlist(p_stent), ",", cli_value2txt_phyif, "---");
        printf("\n");
    }
    
    printf("%-*sdiscard-on-match-src: %s\n", indent, "", cli_value2txt_on_off(demo_l2_stent_ld_is_src_discard(p_stent)));
    printf("%-*sdiscard-on-match-dst: %s\n", indent, "", cli_value2txt_on_off(demo_l2_stent_ld_is_dst_discard(p_stent)));
    
    return (FPP_ERR_OK);
}

static inline int stent_print(const fpp_l2_static_ent_cmd_t* p_stent)
{
    return stent_print_aux(p_stent, false);
}

static inline int stent_print_in_bd(const fpp_l2_static_ent_cmd_t* p_stent)
{
    return stent_print_aux(p_stent, true);
}

/* ==== PRIVATE FUNCTIONS : prints for BD ================================== */

static int bd_print_aux(const fpp_l2_bd_cmd_t* p_bd, bool is_verbose)
{
    assert(NULL != p_bd);
    
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    {
        const char *const p_txt_def = (demo_l2_bd_ld_is_default(p_bd)  ? ("[default]")  : (""));
        const char *const p_txt_fbk = (demo_l2_bd_ld_is_fallback(p_bd) ? ("[fallback]") : (""));
        printf("%-*sdomain %02"PRIu16"  %s%s\n", indent, "",
        demo_l2_bd_ld_get_vlan(p_bd), p_txt_def, p_txt_fbk);
    }
    
    indent += 4;
    
    {
        const uint32_t phyifs_bitset = demo_l2_bd_ld_get_if_list(p_bd) & (~demo_l2_bd_ld_get_untag_if_list(p_bd));
        printf("%-*sphyifs (tagged)   : ", indent, "");
        cli_print_bitset32(phyifs_bitset, ",", cli_value2txt_phyif, "---");
        printf("\n");
    }
    {
        const uint32_t phyifs_bitset = demo_l2_bd_ld_get_if_list(p_bd) & demo_l2_bd_ld_get_untag_if_list(p_bd);
        printf("%-*sphyifs (untagged) : ", indent, "");
        cli_print_bitset32(phyifs_bitset, ",", cli_value2txt_phyif, "---");
        printf("\n");
    }
    {
        const uint8_t ucast_hit  = demo_l2_bd_ld_get_ucast_hit(p_bd);
        const uint8_t ucast_miss = demo_l2_bd_ld_get_ucast_miss(p_bd);
        const uint8_t mcast_hit  = demo_l2_bd_ld_get_mcast_hit(p_bd);
        const uint8_t mcast_miss = demo_l2_bd_ld_get_mcast_miss(p_bd);
        const uint32_t ingress = demo_l2_bd_ld_get_stt_ingress(p_bd);
        const uint32_t egress = demo_l2_bd_ld_get_stt_egress(p_bd);
        const uint32_t ingress_bytes = demo_l2_bd_ld_get_stt_ingress_bytes(p_bd);
        const uint32_t egress_bytes = demo_l2_bd_ld_get_stt_egress_bytes(p_bd);

        printf("%-*sucast-hit  action : %"PRIu8" (%s)\n"
               "%-*sucast-miss action : %"PRIu8" (%s)\n"
               "%-*smcast-hit  action : %"PRIu8" (%s)\n"
               "%-*smcast-miss action : %"PRIu8" (%s)\n"
               "%-*singress           : %"PRIu32"\n"
               "%-*singress bytes     : %"PRIu32"\n"
               "%-*segress            : %"PRIu32"\n"
               "%-*segress bytes      : %"PRIu32"\n",

               indent, "", (ucast_hit),  cli_value2txt_bd_action(ucast_hit),
               indent, "", (ucast_miss), cli_value2txt_bd_action(ucast_miss),
               indent, "", (mcast_hit),  cli_value2txt_bd_action(mcast_hit),
               indent, "", (mcast_miss), cli_value2txt_bd_action(mcast_miss),
               indent, "", (ingress),
               indent, "", (ingress_bytes),
               indent, "", (egress),
               indent, "", (egress_bytes));
    }
    
    if (is_verbose)
    {
        uint32_t cnt = 0u;
        demo_l2_stent_get_count(cli_p_cl, &cnt, true, demo_l2_bd_ld_get_vlan(p_bd));
        const char* p_txt_dashes_if_none = ((0u == cnt) ? ("---") : (""));
        printf("%-*sstatic entries: %s\n", indent, "", p_txt_dashes_if_none);
    }
    
    return (FPP_ERR_OK);
}

static inline int bd_print(const fpp_l2_bd_cmd_t* p_bd)
{
    return bd_print_aux(p_bd, false);
}

static inline int bd_print_verbose(const fpp_l2_bd_cmd_t* p_bd)
{
    bd_print_aux(p_bd, true);
    return demo_l2_stent_print_all(cli_p_cl, stent_print_in_bd, true, demo_l2_bd_ld_get_vlan(p_bd));
}

/* ==== PUBLIC FUNCTIONS : BD ============================================== */

int cli_cmd_bd_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_l2_bd_cmd_t bd = {0};
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    const demo_l2_bd_cb_print_t p_cb_print = ((p_cmdargs->verbose.is_valid) ? (bd_print_verbose) : (bd_print));
    if (p_cmdargs->vlan.is_valid)
    {
        /* print a single bridge domain */
        rtn = demo_l2_bd_get_by_vlan(cli_p_cl, &bd, (p_cmdargs->vlan.value));
        if (FPP_ERR_OK == rtn)
        {
            rtn = p_cb_print(&bd);
        }
    }
    else
    {
        /* print all bridge domains */
        rtn = demo_l2_bd_print_all(cli_p_cl, p_cb_print);
    }

    return (rtn);
}

int cli_cmd_bd_update(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_l2_bd_cmd_t bd = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_get_by_vlan(cli_p_cl, &bd, (p_cmdargs->vlan.value));
    }
    
    /* modify local data - hit/miss actions */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->ucast_hit.is_valid)
        {
            demo_l2_bd_ld_set_ucast_hit(&bd, (p_cmdargs->ucast_hit.value));
        }
        if (p_cmdargs->ucast_miss.is_valid)
        {
            demo_l2_bd_ld_set_ucast_miss(&bd, (p_cmdargs->ucast_miss.value));
        }
        if (p_cmdargs->mcast_hit.is_valid)
        {
            demo_l2_bd_ld_set_mcast_hit(&bd, (p_cmdargs->mcast_hit.value));
        }
        if (p_cmdargs->mcast_miss.is_valid)
        {
            demo_l2_bd_ld_set_mcast_miss(&bd, (p_cmdargs->mcast_miss.value));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_update(cli_p_cl, &bd);
    }
    
    return (rtn);
}

int cli_cmd_bd_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_add(cli_p_cl, NULL, (p_cmdargs->vlan.value));
    }
    
    return (rtn);
}

int cli_cmd_bd_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /*  exec  */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_del(cli_p_cl, (p_cmdargs->vlan.value));
    }
    
    return (rtn);
}

int cli_cmd_bd_insif(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_l2_bd_cmd_t bd = {0};
    uint32_t phyif_id = 0uL;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)},
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* convert phyif name to phyif ID */
    if (FPP_ERR_OK == rtn)
    {
        uint8_t tmp_value = 0u;
        rtn = cli_txt2value_phyif(&tmp_value, (p_cmdargs->if_name.txt));
        if (FPP_ERR_OK == rtn)
        {
            phyif_id = tmp_value; /* implicit cast */
        }
    }
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_get_by_vlan(cli_p_cl, &bd, (p_cmdargs->vlan.value));
    }
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        const bool is_vlan_tag = ((p_cmdargs->tag.is_valid) ? (p_cmdargs->tag.is_on) : (false));
        demo_l2_bd_ld_insert_phyif(&bd, phyif_id, is_vlan_tag);
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_update(cli_p_cl, &bd);
    }
    
    return (rtn);
}

int cli_cmd_bd_remif(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_l2_bd_cmd_t bd = {0};
    uint32_t phyif_id = 0uL;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)},
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* convert phyif name to phyif ID */
    if (FPP_ERR_OK == rtn)
    {
        uint8_t tmp_value = 0u;
        rtn = cli_txt2value_phyif(&tmp_value, (p_cmdargs->if_name.txt));
        if (FPP_ERR_OK == rtn)
        {
            phyif_id = tmp_value; /* implicit cast */
        }
    }
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_get_by_vlan(cli_p_cl, &bd, (p_cmdargs->vlan.value));
    }
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        demo_l2_bd_ld_remove_phyif(&bd, phyif_id);
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_bd_update(cli_p_cl, &bd);
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : BD_STENT ======================================== */

int cli_cmd_bd_stent_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    /* empty */
    
    /* exec */
    if (p_cmdargs->vlan.is_valid)
    {
        /* print all static entries affiliated with given bridge domain */
        rtn = demo_l2_stent_print_all(cli_p_cl, stent_print, true, (p_cmdargs->vlan.value));
    }
    else
    {
        /* print all static entries (regardless of bridge domain affiliation) */
        rtn = demo_l2_stent_print_all(cli_p_cl, stent_print, false, 0u);
    }

    return (rtn);
}

int cli_cmd_bd_stent_update(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_l2_static_ent_cmd_t stent = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)},
        {OPT_MAC,  NULL, (p_cmdargs->smac.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_stent_get_by_vlanmac(cli_p_cl, &stent, (p_cmdargs->vlan.value), (p_cmdargs->smac.arr));
    }
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->egress.is_valid)
        {
            demo_l2_stent_ld_set_fwlist(&stent, (p_cmdargs->egress.bitset));
        }
        if (p_cmdargs->local.is_valid)
        {
            demo_l2_stent_ld_set_local(&stent, (p_cmdargs->local.is_on));
        }
        if (p_cmdargs->vlan_conf__x_src.is_valid)
        {
            demo_l2_stent_ld_set_src_discard(&stent, (p_cmdargs->vlan_conf__x_src.is_on));
        }
        if (p_cmdargs->ptp_conf__x_dst.is_valid)
        {
            demo_l2_stent_ld_set_dst_discard(&stent, (p_cmdargs->ptp_conf__x_dst.is_on));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_stent_update(cli_p_cl, &stent);
    }
    
    return (rtn);
}

int cli_cmd_bd_stent_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)},
        {OPT_MAC,  NULL, (p_cmdargs->smac.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_stent_add(cli_p_cl, NULL, (p_cmdargs->vlan.value), (p_cmdargs->smac.arr));
    }
    
    return (rtn);
}

int cli_cmd_bd_stent_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = 
    {
        {OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)},
        {OPT_MAC,  NULL, (p_cmdargs->smac.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_l2_stent_del(cli_p_cl, (p_cmdargs->vlan.value), (p_cmdargs->smac.arr));
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : FLUSH =========================================== */

int cli_cmd_bd_flush(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_optbuf_t asd = {{OPT_ALL, OPT_STATIC, OPT_DYNAMIC}};
    const mandopt_t mandopts[] = 
    {
        {OPT_NONE, &asd, ((p_cmdargs->all.is_valid) || (p_cmdargs->static0.is_valid) || (p_cmdargs->dynamic0.is_valid))},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->all.is_valid)
        {
            rtn = demo_l2_flush_all(cli_p_cl);
        }
        if (p_cmdargs->static0.is_valid)
        {
            rtn = demo_l2_flush_static(cli_p_cl);
        }
        if (p_cmdargs->dynamic0.is_valid)
        {
            rtn = demo_l2_flush_learned(cli_p_cl);
        }
    }
    
    return (rtn);
}

/* ========================================================================= */
