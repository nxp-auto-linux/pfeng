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
#include "libfci_cli_cmds_bd.h"

#include "libfci_interface/fci_l2_bd.h"

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
        const char* p_txt_local = (0 != (p_stent->local)) ? ("[local address]") : ("");
        printf("%-*sMAC: ", indent, "");
        cli_print_mac(p_stent->mac);
        printf("  %s\n", p_txt_local);
    }
    
    indent += 5;  /* detailed static entry info is indented deeper */
    
    if (!is_nested_in_bd)
    {
        printf("%-*svlan: %"PRIu16"\n", indent, "", (p_stent->vlan));
    }
    
    {
        const char* p_txt_local = (0 != (p_stent->local)) ? (" (ignored when local)") : ("");
        printf("%-*segress%s: ", indent, "", p_txt_local);
        cli_print_bitset32((p_stent->forward_list), ",", cli_value2txt_phyif, "---");
        printf("\n");
    }
    
    printf("%-*sdiscard-on-match-src: %s\n", indent, "", cli_value2txt_on_off(p_stent->src_discard));
    printf("%-*sdiscard-on-match-dst: %s\n", indent, "", cli_value2txt_on_off(p_stent->dst_discard));
    
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
        const char *const p_txt_def = (fci_l2_bd_ld_is_default(p_bd)  ? ("[default]")  : (""));
        const char *const p_txt_fbk = (fci_l2_bd_ld_is_fallback(p_bd) ? ("[fallback]") : (""));
        printf("%-*sdomain %02"PRIu16"  %s%s\n", indent, "",
        (p_bd->vlan), p_txt_def, p_txt_fbk);
    }
    
    indent += 4;
    
    {
        const uint32_t phyifs_bitset = (p_bd->if_list) & ~(p_bd->untag_if_list);
        printf("%-*sphyifs (tagged)   : ", indent, "");
        cli_print_bitset32(phyifs_bitset, ",", cli_value2txt_phyif, "---");
        printf("\n");
    }
    {
        const uint32_t phyifs_bitset = (p_bd->if_list) & (p_bd->untag_if_list);
        printf("%-*sphyifs (untagged) : ", indent, "");
        cli_print_bitset32(phyifs_bitset, ",", cli_value2txt_phyif, "---");
        printf("\n");
    }
    {
        printf("%-*sucast-hit  action : %"PRIu8" (%s)\n"
               "%-*sucast-miss action : %"PRIu8" (%s)\n"
               "%-*smcast-hit  action : %"PRIu8" (%s)\n"
               "%-*smcast-miss action : %"PRIu8" (%s)\n",
               indent, "", (p_bd->ucast_hit) , cli_value2txt_bd_action(p_bd->ucast_hit),
               indent, "", (p_bd->ucast_miss), cli_value2txt_bd_action(p_bd->ucast_miss),
               indent, "", (p_bd->mcast_hit) , cli_value2txt_bd_action(p_bd->mcast_hit),
               indent, "", (p_bd->mcast_miss), cli_value2txt_bd_action(p_bd->mcast_miss));
    }
    
    if (is_verbose)
    {
        uint16_t cnt = 0u;
        fci_l2_stent_get_count_by_vlan(cli_p_cl, &cnt, (p_bd->vlan));
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
    return fci_l2_stent_print_by_vlan(cli_p_cl, stent_print_in_bd, (p_bd->vlan));
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
    const fci_l2_bd_cb_print_t p_cb_print = ((p_cmdargs->verbose.is_valid) ? (bd_print_verbose) : (bd_print));
    if (p_cmdargs->vlan.is_valid)
    {
        /* print a single bridge domain */
        rtn = fci_l2_bd_get_by_vlan(cli_p_cl, &bd, (p_cmdargs->vlan.value));
        if (FPP_ERR_OK == rtn)
        {
            rtn = p_cb_print(&bd);
        }
    }
    else
    {
        /* print all bridge domains */
        rtn = fci_l2_bd_print_all(cli_p_cl, p_cb_print);
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
    const mandopt_t mandopts[] = {{OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)}};
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_bd_get_by_vlan(cli_p_cl, &bd, (p_cmdargs->vlan.value));
    }
    
    /* modify local data - hit/miss actions */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->ucast_hit.is_valid))
        {
            rtn = fci_l2_bd_ld_set_ucast_hit(&bd, (p_cmdargs->ucast_hit.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->ucast_miss.is_valid))
        {
            rtn = fci_l2_bd_ld_set_ucast_miss(&bd, (p_cmdargs->ucast_miss.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->mcast_hit.is_valid))
        {
            rtn = fci_l2_bd_ld_set_mcast_hit(&bd, (p_cmdargs->mcast_hit.value));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->mcast_miss.is_valid))
        {
            rtn = fci_l2_bd_ld_set_mcast_miss(&bd, (p_cmdargs->mcast_miss.value));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_bd_update(cli_p_cl, &bd);
    }
    
    return (rtn);
}

int cli_cmd_bd_add(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = {{OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)}};
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_bd_add(cli_p_cl, NULL, (p_cmdargs->vlan.value));
    }
    
    return (rtn);
}

int cli_cmd_bd_del(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] = {{OPT_VLAN, NULL, (p_cmdargs->vlan.is_valid)}};
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /*  exec  */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_bd_del(cli_p_cl, (p_cmdargs->vlan.value));
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
        rtn = fci_l2_bd_get_by_vlan(cli_p_cl, &bd, (p_cmdargs->vlan.value));
    }
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        const bool is_vlan_tag = ((p_cmdargs->tag.is_valid) ? (p_cmdargs->tag.is_on) : (false));
        rtn = fci_l2_bd_ld_insert_phyif(&bd, phyif_id, is_vlan_tag);
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_bd_update(cli_p_cl, &bd);
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
        rtn = fci_l2_bd_get_by_vlan(cli_p_cl, &bd, (p_cmdargs->vlan.value));
    }
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_bd_ld_remove_phyif(&bd, phyif_id);
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_bd_update(cli_p_cl, &bd);
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
        rtn = fci_l2_stent_print_by_vlan(cli_p_cl, stent_print, (p_cmdargs->vlan.value));
    }
    else
    {
        /* print all static entries (regardless of bridge domain affiliation) */
        rtn = fci_l2_stent_print_all(cli_p_cl, stent_print);
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
        rtn = fci_l2_stent_get_by_vlanmac(cli_p_cl, &stent, (p_cmdargs->vlan.value), (p_cmdargs->smac.arr));
    }
    
    /* modify local data */
    if (FPP_ERR_OK == rtn)
    {
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->egress.is_valid))
        {
            rtn = fci_l2_stent_ld_set_fwlist(&stent, (p_cmdargs->egress.bitset));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->local.is_valid))
        {
            rtn = fci_l2_stent_ld_set_local(&stent, (p_cmdargs->local.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->vlan_conf__x_src.is_valid))
        {
            rtn = fci_l2_stent_ld_set_src_discard(&stent, (p_cmdargs->vlan_conf__x_src.is_on));
        }
        if ((FPP_ERR_OK == rtn) && (p_cmdargs->ptp_conf__x_dst.is_valid))
        {
            rtn = fci_l2_stent_ld_set_dst_discard(&stent, (p_cmdargs->ptp_conf__x_dst.is_on));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_l2_stent_update(cli_p_cl, &stent);
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
        rtn = fci_l2_stent_add(cli_p_cl, NULL, (p_cmdargs->vlan.value), (p_cmdargs->smac.arr));
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
        rtn = fci_l2_stent_del(cli_p_cl, (p_cmdargs->vlan.value), (p_cmdargs->smac.arr));
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
            rtn = fci_l2_flush_all(cli_p_cl);
        }
        if (p_cmdargs->static0.is_valid)
        {
            rtn = fci_l2_flush_static(cli_p_cl);
        }
        if (p_cmdargs->dynamic0.is_valid)
        {
            rtn = fci_l2_flush_learned(cli_p_cl);
        }
    }
    
    return (rtn);
}

/* ========================================================================= */
