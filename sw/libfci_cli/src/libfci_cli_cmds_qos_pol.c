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
#include "libfci_cli_cmds_qos_pol.h"

/*
    NOTE:
    The "demo_" functions are libFCI abstractions.
    The "demo_" prefix was chosen because these functions are used as demos in FCI API Reference. 
*/
#include "libfci_demo/demo_qos_pol.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

extern FCI_CLIENT* cli_p_cl;

/* ==== PRIVATE FUNCTIONS : prints for Ingress QoS WRED ==================== */

static int qos_polwred_print_aux(const fpp_qos_policer_wred_cmd_t* p_polwred, int indent, bool do_print_interface_name)
{
    assert(NULL != p_polwred);
    
    
    /* NOTE: indent is provided by caller */ 
    
    printf("%-*sWred for '%s' ingress queue:\n", indent, "", cli_value2txt_pol_wred_que(demo_polwred_ld_get_que(p_polwred)));
    
    indent += 4;
    
    printf("%-*s<%s>\n", indent, "", cli_value2txt_en_dis(demo_polwred_ld_is_enabled(p_polwred)));
    if (do_print_interface_name)
    {
        printf("%-*sinterface: %s\n", indent, "", demo_polwred_ld_get_if_name(p_polwred));
    }
    
    printf("%-*sthld-min:  %"PRIu16"\n"
           "%-*sthld-max:  %"PRIu16"\n"
           "%-*sthld-full: %"PRIu16"\n",
           indent, "", demo_polwred_ld_get_min(p_polwred),
           indent, "", demo_polwred_ld_get_max(p_polwred),
           indent, "", demo_polwred_ld_get_full(p_polwred));
    
    {
        printf("%-*szprob:     ", indent, "");
        const char* p_txt_delim = "";  /* no delim in front of the first item */
        for (uint8_t i = 0u; (FPP_IQOS_WRED_ZONES_COUNT > i); (++i))
        {
            printf("%s[%"PRIu8"]<%"PRIu8">", p_txt_delim, i, demo_polwred_ld_get_zprob_by_id(p_polwred, i));
            p_txt_delim = ",";
        }
        printf("\n");
    }
    
    return (FPP_ERR_OK); 
}

static inline int qos_polwred_print(const fpp_qos_policer_wred_cmd_t* p_polwred)
{
    return qos_polwred_print_aux(p_polwred, 0, true);
}

static inline int qos_polwred_print_in_pol(const fpp_qos_policer_wred_cmd_t* p_polwred)
{
    return qos_polwred_print_aux(p_polwred, 6, false);
}

/* ==== PRIVATE FUNCTIONS : prints for Ingress QoS shaper ================== */

static int qos_polshp_print_aux(const fpp_qos_policer_shp_cmd_t* p_polshp, int indent, bool do_print_interface_name)
{
    assert(NULL != p_polshp);
    
    
    /* NOTE: indent is provided by caller */ 
    
    printf("%-*sshaper %"PRIu8":\n", indent, "", demo_polshp_ld_get_id(p_polshp)); 
    
    indent += 4;
    
    printf("%-*s<%s>\n", indent, "", cli_value2txt_en_dis(demo_polshp_ld_is_enabled(p_polshp)));
    if (do_print_interface_name)
    {
        printf("%-*sinterface:  %s\n", indent, "", demo_polshp_ld_get_if_name(p_polshp));
    }
    
    printf("%-*sshp-type:   %d (%s)\n"
           "%-*sshp-mode:   %d (%s)\n"
           "%-*sisl:        %"PRIu32"\n"
           "%-*scredit-min: %"PRId32"\n"
           "%-*scredit-max: %"PRId32"\n",
           indent, "", demo_polshp_ld_get_type(p_polshp), cli_value2txt_pol_shp_type(demo_polshp_ld_get_type(p_polshp)),
           indent, "", demo_polshp_ld_get_mode(p_polshp), cli_value2txt_pol_shp_mode(demo_polshp_ld_get_mode(p_polshp)),
           indent, "", demo_polshp_ld_get_isl(p_polshp),
           indent, "", demo_polshp_ld_get_min_credit(p_polshp),
           indent, "", demo_polshp_ld_get_max_credit(p_polshp));
    
    return (FPP_ERR_OK); 
}

static inline int qos_polshp_print(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    return qos_polshp_print_aux(p_polshp, 0, true);
}

static inline int qos_polshp_print_in_pol(const fpp_qos_policer_shp_cmd_t* p_polshp)
{
    return qos_polshp_print_aux(p_polshp, 6, false);
}

/* ==== PRIVATE FUNCTIONS : prints for Ingress QoS flow ==================== */

static int qos_polflow_print_aux(const fpp_qos_policer_flow_cmd_t* p_polflow, int indent, bool do_print_interface_name)
{
    assert(NULL != p_polflow);
    
    
    /* NOTE: indent is provided by caller */ 
    
    printf("%-*sflow %"PRIu8":\n", indent, "", demo_polflow_ld_get_id(p_polflow)); 
    
    indent += 4;
    
    if (do_print_interface_name)
    {
        printf("%-*sinterface:   %s\n", indent, "", demo_polflow_ld_get_if_name(p_polflow));
    }
    
    {
        const fpp_iqos_flow_action_t action = demo_polflow_ld_get_action(p_polflow);
        const char* p_txt_flavor = (((FPP_IQOS_FLOW_MANAGED == action) || (FPP_IQOS_FLOW_RESERVED == action)) ? 
                                    ("Mark traffic as ") : (""));
        
        printf("%-*sflow-action: %s%s\n", indent, "", p_txt_flavor, cli_value2txt_pol_flow_action(action));
    }
    
    {
        /* HACK: 32bit bitmask, merging both 'fpp_iqos_flow_type_t' and 'fpp_iqos_flow_arg_type_t' into one bitset */
        uint32_t flow_types_bitset32 = demo_polflow_ld_get_am_bitset(p_polflow);
        flow_types_bitset32 <<= 16;
        flow_types_bitset32 |= demo_polflow_ld_get_m_bitset(p_polflow);
        
        printf("%-*sargumentless flow-types: 0x%04X (", indent, "", 
               demo_polflow_ld_get_m_bitset(p_polflow));
               
        cli_print_bitset32(flow_types_bitset32, ",", cli_value2txt_pol_flow_type32, "---");
        printf(")\n");
    }
    
    {
        printf("%-*sargumentful  flow-types:\n", indent, "");
        
        indent += 2u;  /* verbose info is indented even deeper */
        
        const fpp_iqos_flow_arg_type_t am_bitset = 0xFFFF;  /* driver does not return valid bitset ; print all argumentful flow types by default */
        
        if (FPP_IQOS_ARG_VLAN & am_bitset)
        {
            printf("%-*s"TXT_POL_FLOW_TYPE2__VLAN":      <vlan: %"PRIu16"> ; <vlan-mask: 0x%04"PRIX16">\n", indent, "",
                   demo_polflow_ld_get_am_vlan(p_polflow), demo_polflow_ld_get_am_vlan_m(p_polflow));
        }
        if (FPP_IQOS_ARG_VLAN & am_bitset)
        {
            printf("%-*s"TXT_POL_FLOW_TYPE2__TOS":       <tos: 0x%02"PRIX8"> ; <tos-mask: 0x%02"PRIX8">\n", indent, "",
                   demo_polflow_ld_get_am_tos(p_polflow), demo_polflow_ld_get_am_tos_m(p_polflow));
        }
        if (FPP_IQOS_ARG_L4PROTO & am_bitset)
        {
            const uint8_t protocol = demo_polflow_ld_get_am_proto(p_polflow);
            printf("%-*s"TXT_POL_FLOW_TYPE2__PROTOCOL":  <protocol: %"PRIu8" (%s)> ; <protocol-mask: 0x%02"PRIX8">\n", indent, "",
                   protocol, cli_value2txt_protocol(protocol), demo_polflow_ld_get_am_proto_m(p_polflow));
        }
        if (FPP_IQOS_ARG_SIP & am_bitset)
        {
            printf("%-*s"TXT_POL_FLOW_TYPE2__SIP":       <sip: ", indent, "");
            cli_print_ip4(demo_polflow_ld_get_am_sip(p_polflow), false);
            printf("> ; <sip-pfx: %"PRIu8">\n", demo_polflow_ld_get_am_sip_m(p_polflow));
        }
        if (FPP_IQOS_ARG_DIP & am_bitset)
        {
            printf("%-*s"TXT_POL_FLOW_TYPE2__DIP":       <dip: ", indent, "");
            cli_print_ip4(demo_polflow_ld_get_am_dip(p_polflow), false);
            printf("> ; <sip-pfx: %"PRIu8">\n", demo_polflow_ld_get_am_dip_m(p_polflow));
        }
        if (FPP_IQOS_ARG_SPORT & am_bitset)
        {
            printf("%-*s"TXT_POL_FLOW_TYPE2__SPORT":     <sport-min: %"PRIu16"> ; <sport-max: %"PRIu16">\n", indent, "",
                   demo_polflow_ld_get_am_sport_min(p_polflow), demo_polflow_ld_get_am_sport_max(p_polflow));
        }
        if (FPP_IQOS_ARG_DPORT & am_bitset)
        {
            printf("%-*s"TXT_POL_FLOW_TYPE2__DPORT":     <dport-min: %"PRIu16"> ; <dport-max: %"PRIu16">\n", indent, "",
                   demo_polflow_ld_get_am_dport_min(p_polflow), demo_polflow_ld_get_am_dport_max(p_polflow));
        }
    }
    
    return (FPP_ERR_OK); 
}

static inline int qos_polflow_print(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    return qos_polflow_print_aux(p_polflow, 0, true);
}

static inline int qos_polflow_print_in_pol(const fpp_qos_policer_flow_cmd_t* p_polflow)
{
    return qos_polflow_print_aux(p_polflow, 6, false);
}

/* ==== PRIVATE FUNCTIONS : prints for Ingress QoS policer ================= */

static int qos_pol_print(const fpp_qos_policer_cmd_t* p_pol)
{
    assert(NULL != p_pol);
    
    
    int rtn = FPP_ERR_OK;
    
    /* NOTE: native data type to comply with 'printf()' conventions (asterisk specifier) */ 
    int indent = 0;
    
    printf("%-*sIngress QoS Policer\n", indent, "");
    
    indent += 2;
    
    printf("%-*s<%s>\n", indent, "", cli_value2txt_en_dis(demo_pol_ld_is_enabled(p_pol)));
    printf("%-*sinterface: %s\n", indent, "", demo_pol_ld_get_if_name(p_pol));
    
    if (FPP_ERR_OK == rtn)
    {
        printf("%-*sWREDs:\n", indent, "");
        demo_polwred_print_by_phyif(cli_p_cl, qos_polwred_print_in_pol, demo_pol_ld_get_if_name(p_pol));  /* TODO: this should be 'rtn = demo_*()', but currently the Ingress QoS FCI API query is broken and fails to properly terminate */
    }
    
    if (FPP_ERR_OK == rtn)
    {
        printf("%-*sShapers:\n", indent, "");
        demo_polshp_print_by_phyif(cli_p_cl, qos_polshp_print_in_pol, demo_pol_ld_get_if_name(p_pol));  /* TODO: this should be 'rtn = demo_*()', but currently the Ingress QoS FCI API query is broken and fails to properly terminate */
    }
    
    if (FPP_ERR_OK == rtn)
    {
        printf("%-*sFlows:\n", indent, "");
        rtn = demo_polflow_print_by_phyif(cli_p_cl, qos_polflow_print_in_pol, demo_pol_ld_get_if_name(p_pol));  /* TODO: this should be 'rtn = demo_*()', but currently the Ingress QoS FCI API query is broken and fails to properly terminate */
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : Ingress QoS policer ============================= */

int cli_cmd_qos_pol_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_policer_cmd_t pol = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        /* print Ingress QoS sumary info */
        rtn = demo_pol_get(cli_p_cl, &pol, (p_cmdargs->if_name.txt));
        if (FPP_ERR_OK == rtn)
        {
            rtn = qos_pol_print(&pol);
        }
    }
    
    return (rtn);
}

int cli_cmd_qos_pol_set(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* check for mandatory opts */
    const mandopt_optbuf_t endis = {{OPT_ENABLE, OPT_DISABLE}};
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL,   (p_cmdargs->if_name.is_valid)},
        {OPT_NONE     , &endis, ((p_cmdargs->enable_noreply.is_valid) || (p_cmdargs->disable_noorig.is_valid))},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_pol_enable(cli_p_cl, (p_cmdargs->if_name.txt), (p_cmdargs->enable_noreply.is_valid));
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : Ingress QoS wred ================================ */

int cli_cmd_qos_pol_wred_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_policer_wred_cmd_t polwred = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->wred_que.is_valid)
        {
            /* print a single Ingress QoS wred */
            rtn = demo_polwred_get_by_que(cli_p_cl, &polwred, (p_cmdargs->if_name.txt), (p_cmdargs->wred_que.value));
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_polwred_print(&polwred);
            }
        }
        else
        {
            /* print all Ingress QoS wred of the given interface */
            rtn = demo_polwred_print_by_phyif(cli_p_cl, qos_polwred_print, (p_cmdargs->if_name.txt));
        }
    }
    
    return (rtn);
}

int cli_cmd_qos_pol_wred_update(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_policer_wred_cmd_t polwred = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_WRED_QUE,  NULL, (p_cmdargs->wred_que.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_polwred_get_by_que(cli_p_cl, &polwred, (p_cmdargs->if_name.txt), (p_cmdargs->wred_que.value));
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if ((p_cmdargs->enable_noreply.is_valid) || (p_cmdargs->disable_noorig.is_valid))
        {
            demo_polwred_ld_enable(&polwred, (p_cmdargs->enable_noreply.is_valid));  // Enable and disable opts are mutually exclusive.
        }
        
        if (p_cmdargs->thmin.is_valid)
        {
            demo_polwred_ld_set_min(&polwred, (p_cmdargs->thmin.value));
        }
        if (p_cmdargs->thmax.is_valid)
        {
            demo_polwred_ld_set_max(&polwred, (p_cmdargs->thmax.value));
        }
        if (p_cmdargs->thfull.is_valid)
        {
            demo_polwred_ld_set_full(&polwred, (p_cmdargs->thfull.value));
        }
    }
    
    /* modify local data - zprob elements */
    if ((FPP_ERR_OK == rtn) && (p_cmdargs->zprob.is_valid))
    {
        for (uint8_t i = 0u; (FPP_IQOS_WRED_ZONES_COUNT > i); (++i))
        {
            const uint8_t cmdarg_value = (p_cmdargs->zprob.arr[i]);
            if (cli_que_zprob_is_not_keep(cmdarg_value))
            {
                demo_polwred_ld_set_zprob(&polwred, i, cmdarg_value);
            }
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_polwred_update(cli_p_cl, &polwred);
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : Ingress QoS shaper ============================== */

int cli_cmd_qos_pol_shp_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_policer_shp_cmd_t polshp = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->que_sch_shp.is_valid)
        {
            /* print a single Ingress QoS shaper */
            rtn = demo_polshp_get_by_id(cli_p_cl, &polshp, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_polshp_print(&polshp);
            }
        }
        else
        {
            /* print all Ingress QoS wred of the given interface */
            rtn = demo_polshp_print_by_phyif(cli_p_cl, qos_polshp_print, (p_cmdargs->if_name.txt));
        }
    }
    
    return (rtn);
}

int cli_cmd_qos_pol_shp_update(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_policer_shp_cmd_t polshp = {0};
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_SHP,       NULL, (p_cmdargs->que_sch_shp.is_valid)},
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* get init local data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_polshp_get_by_id(cli_p_cl, &polshp, (p_cmdargs->if_name.txt), (p_cmdargs->que_sch_shp.value));
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if ((p_cmdargs->enable_noreply.is_valid) || (p_cmdargs->disable_noorig.is_valid))
        {
            demo_polshp_ld_enable(&polshp, (p_cmdargs->enable_noreply.is_valid));  // Enable and disable opts are mutually exclusive.
        }
        if (p_cmdargs->shp_type.is_valid)
        {
            demo_polshp_ld_set_type(&polshp, (p_cmdargs->shp_type.value));
        }
        if (p_cmdargs->que_sch_shp_mode.is_valid)
        {
            /* HACK: remap standard shp_mode values to polshp_mode values (hooray for consistency -_-) */
            if (0 < (p_cmdargs->que_sch_shp_mode.value))  /* for standard shp_mode values, value 0 == DISABLED, but polshp_mode does not have it, so proceed only if mode is nonzero */
            {
                fpp_iqos_shp_rate_mode_t polshp_mode = (fpp_iqos_shp_rate_mode_t)((p_cmdargs->que_sch_shp_mode.value) - 1u);  /* -1 because polshp does not have mode DISABLED (immediately begins with DATA_RATE) */
                demo_polshp_ld_set_mode(&polshp, polshp_mode);
            }
        }
        if (p_cmdargs->isl.is_valid)
        {
            demo_polshp_ld_set_isl(&polshp, (p_cmdargs->isl.value));
        }
        if (p_cmdargs->crmin.is_valid)
        {
            demo_polshp_ld_set_min_credit(&polshp, (p_cmdargs->crmin.value));
        }
        if (p_cmdargs->crmax.is_valid)
        {
            demo_polshp_ld_set_max_credit(&polshp, (p_cmdargs->crmax.value));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_polshp_update(cli_p_cl, &polshp);
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS : Ingress QoS flow ================================ */

int cli_cmd_qos_pol_flow_print(const cli_cmdargs_t *p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    fpp_qos_policer_flow_cmd_t polflow = {0};
    uint8_t flow_id = 0;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* HACK: ensure that the offset value (position) is not over 255, because polflow uses uint8_t id instad of uint16_t (hooray for consistency -_-) */
    if (UINT8_MAX < (p_cmdargs->offset.value))
    {
        rtn = CLI_ERR_INVARG;
    }
    else
    {
        flow_id = (p_cmdargs->offset.value);  /* WARNING: implicit narrowing cast */ 
    }
    
    /* exec */
    if (CLI_OK == rtn)
    {
        if (p_cmdargs->offset.is_valid)
        {
            /* print a single Ingress QoS shaper */
            rtn = demo_polflow_get_by_id(cli_p_cl, &polflow, (p_cmdargs->if_name.txt), flow_id);
            if (FPP_ERR_OK == rtn)
            {
                rtn = qos_polflow_print(&polflow);
            }
        }
        else
        {
            /* print all Ingress QoS wred of the given interface */
            rtn = demo_polflow_print_by_phyif(cli_p_cl, qos_polflow_print, (p_cmdargs->if_name.txt));
        }
    }
    
    return (rtn);
}

int cli_cmd_qos_pol_flow_add(const cli_cmdargs_t *p_cmdargs)
{
    int rtn = CLI_ERR;
    fpp_qos_policer_flow_cmd_t polflow = {0};
    uint8_t flow_id = 0;
    const fpp_iqos_flow_arg_type_t  m_bitset = ((p_cmdargs->flow_types.is_valid) ? (p_cmdargs->flow_types.bitset1) : (0));
    const fpp_iqos_flow_arg_type_t am_bitset = ((p_cmdargs->flow_types.is_valid) ? (p_cmdargs->flow_types.bitset2) : (0));
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        
        /* these are mandatory only if the related match rule is requested */
        {OPT_VLAN,           NULL, ((FPP_IQOS_ARG_VLAN    & am_bitset) ? (p_cmdargs->vlan.is_valid)          : (true))},
        {OPT_VLAN_MASK,      NULL, ((FPP_IQOS_ARG_VLAN    & am_bitset) ? (p_cmdargs->vlan_mask.is_valid)     : (true))},
        {OPT_TOS,            NULL, ((FPP_IQOS_ARG_TOS     & am_bitset) ? (p_cmdargs->tos.is_valid)           : (true))},
        {OPT_TOS_MASK,       NULL, ((FPP_IQOS_ARG_TOS     & am_bitset) ? (p_cmdargs->tos_mask.is_valid)      : (true))},
        {OPT_PROTOCOL,       NULL, ((FPP_IQOS_ARG_L4PROTO & am_bitset) ? (p_cmdargs->protocol.is_valid)      : (true))},
        {OPT_PROTOCOL_MASK,  NULL, ((FPP_IQOS_ARG_L4PROTO & am_bitset) ? (p_cmdargs->protocol_mask.is_valid) : (true))},
        {OPT_SIP,            NULL, ((FPP_IQOS_ARG_SIP     & am_bitset) ? (p_cmdargs->sip.is_valid)           : (true))},
        {OPT_SIP_PFX,        NULL, ((FPP_IQOS_ARG_SIP     & am_bitset) ? (p_cmdargs->sip_pfx.is_valid)       : (true))},
        {OPT_DIP,            NULL, ((FPP_IQOS_ARG_DIP     & am_bitset) ? (p_cmdargs->dip.is_valid)           : (true))},
        {OPT_DIP_PFX,        NULL, ((FPP_IQOS_ARG_DIP     & am_bitset) ? (p_cmdargs->dip_pfx.is_valid)       : (true))},
        {OPT_SPORT_MIN,      NULL, ((FPP_IQOS_ARG_SPORT   & am_bitset) ? (p_cmdargs->sport.is_valid)         : (true))},
        {OPT_SPORT_MAX,      NULL, ((FPP_IQOS_ARG_SPORT   & am_bitset) ? (p_cmdargs->sport2.is_valid)        : (true))},
        {OPT_DPORT_MIN,      NULL, ((FPP_IQOS_ARG_DPORT   & am_bitset) ? (p_cmdargs->dport.is_valid)         : (true))},
        {OPT_DPORT_MAX,      NULL, ((FPP_IQOS_ARG_DPORT   & am_bitset) ? (p_cmdargs->dport2.is_valid)        : (true))},        
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* HACK: ensure that the offset value (position) is not over 255, because polflow uses uint8_t id instad of uint16_t (hooray for consistency -_-) */
    if (UINT8_MAX < (p_cmdargs->offset.value))
    {
        rtn = CLI_ERR_INVARG;
    }
    else
    {
        flow_id = (p_cmdargs->offset.is_valid) ? (p_cmdargs->offset.value) : (0xFF);  /* WARNING: implicit narrowing cast */ 
    }
    
    /* modify local data - flow types */
    if ((FPP_ERR_OK == rtn) && (p_cmdargs->flow_types.is_valid))
    {
        /* clear any previous rules */
        demo_polflow_ld_clear_m(&polflow);
        demo_polflow_ld_clear_am(&polflow);
        
        /* set argumentless flow types */
        if (FPP_IQOS_FLOW_TYPE_ETH & m_bitset)
        {
            demo_polflow_ld_set_m_type_eth(&polflow, true);
        }
        if (FPP_IQOS_FLOW_TYPE_PPPOE & m_bitset)
        {
            demo_polflow_ld_set_m_type_pppoe(&polflow, true);
        }
        if (FPP_IQOS_FLOW_TYPE_ARP & m_bitset)
        {
            demo_polflow_ld_set_m_type_arp(&polflow, true);
        }
        if (FPP_IQOS_FLOW_TYPE_IPV4 & m_bitset)
        {
            demo_polflow_ld_set_m_type_ip4(&polflow, true);
        }
        if (FPP_IQOS_FLOW_TYPE_IPV6 & m_bitset)
        {
            demo_polflow_ld_set_m_type_ip6(&polflow, true);
        }
        if (FPP_IQOS_FLOW_TYPE_IPX & m_bitset)
        {
            demo_polflow_ld_set_m_type_ipx(&polflow, true);
        }
        if (FPP_IQOS_FLOW_TYPE_MCAST & m_bitset)
        {
            demo_polflow_ld_set_m_type_mcast(&polflow, true);
        }
        if (FPP_IQOS_FLOW_TYPE_BCAST & m_bitset)
        {
            demo_polflow_ld_set_m_type_bcast(&polflow, true);
        }
        if (FPP_IQOS_FLOW_TYPE_VLAN & m_bitset)
        {
            demo_polflow_ld_set_m_type_vlan(&polflow, true);
        }
        
        /* set argumentful flow types */
        if (FPP_IQOS_ARG_VLAN & am_bitset)
        {
            demo_polflow_ld_set_am_vlan(&polflow, true, (p_cmdargs->vlan.value), (p_cmdargs->vlan_mask.value));
        }
        if (FPP_IQOS_ARG_TOS & am_bitset)
        {
            demo_polflow_ld_set_am_tos(&polflow, true, (p_cmdargs->tos.value), (p_cmdargs->tos_mask.value));
        }
        if (FPP_IQOS_ARG_L4PROTO & am_bitset)
        {
            demo_polflow_ld_set_am_proto(&polflow, true, (p_cmdargs->protocol.value), (p_cmdargs->protocol_mask.value));
        }
        if (FPP_IQOS_ARG_SIP & am_bitset)
        {
            demo_polflow_ld_set_am_sip(&polflow, true, (p_cmdargs->sip.arr[0]), (p_cmdargs->sip_pfx.value));
        }
        if (FPP_IQOS_ARG_DIP & am_bitset)
        {
            demo_polflow_ld_set_am_dip(&polflow, true, (p_cmdargs->dip.arr[0]), (p_cmdargs->dip_pfx.value));
        }
        if (FPP_IQOS_ARG_SPORT & am_bitset)
        {
            demo_polflow_ld_set_am_sport(&polflow, true, (p_cmdargs->sport.value), (p_cmdargs->sport2.value));
        }
        if (FPP_IQOS_ARG_DPORT & am_bitset)
        {
            demo_polflow_ld_set_am_dport(&polflow, true, (p_cmdargs->dport.value), (p_cmdargs->dport2.value));
        }
    }
    
    /* modify local data - misc */
    if (FPP_ERR_OK == rtn)
    {
        if (p_cmdargs->flow_action.is_valid)
        {
            demo_polflow_ld_set_action(&polflow, (p_cmdargs->flow_action.value));
        }
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_polflow_add(cli_p_cl, (p_cmdargs->if_name.txt), flow_id, &polflow);
    }
    
    return (rtn);
}

int cli_cmd_qos_pol_flow_del(const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != cli_p_cl);
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    uint8_t flow_id = 0;
    
    /* check for mandatory opts */
    const mandopt_t mandopts[] =
    {
        {OPT_INTERFACE, NULL, (p_cmdargs->if_name.is_valid)},
        {OPT_POSITION,  NULL, (p_cmdargs->offset.is_valid)}
    };
    rtn = cli_mandopt_check(mandopts, MANDOPTS_CALC_LN(mandopts));
    
    /* HACK: ensure that the offset value (position) is not over 255, because polflow uses uint8_t id instad of uint16_t (hooray for consistency -_-) */
    if (UINT8_MAX < (p_cmdargs->offset.value))
    {
        rtn = CLI_ERR_INVARG;
    }
    else
    {
        flow_id = (p_cmdargs->offset.value);  /* WARNING: implicit narrowing cast */ 
    }
    
    /* exec */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_polflow_del(cli_p_cl, (p_cmdargs->if_name.txt), flow_id);
    }
    
    return (rtn);
}



/* ========================================================================= */
