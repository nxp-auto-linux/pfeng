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
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>  /* required by QNX */
#include <netinet/in.h>  /* required by QNX */
#include <arpa/inet.h>
#include "fpp.h"
#include "fpp_ext.h"

#include "libfci_cli_common.h"
#include "libfci_cli_def_cmds.h"
#include "libfci_cli_def_opts.h"
#include "libfci_cli_def_optarg_keywords.h"
#include "libfci_cli_def_help.h"

#include "libfci_cli_parser.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
const char* TEST_parser__p_txt_opt = NULL;
cli_cmd_t TEST_parser__cmd4exec = CMD_LN;
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

#define BASE_DEC  (10)
#define BASE_HEX  (16)

typedef enum loopstage_tt {
    LOOP_PRE = 0u,
    LOOP_BODY,
    LOOP_POST
} loopstage_t;
typedef int (*cb_txt2value_t)(uint8_t* p_rtn_value, const char* p_txt);
typedef int (*cb_loop_t)(void* p_rtn_void, char* p_txt, const cb_txt2value_t p_cb_txt2value, loopstage_t loopstage, uint8_t i);

static void set_if_rtn_ok(int rtn_from_caller, bool* p_is_valid);

static int cli_txt2bool_on_off(bool* p_rtn_value, const char* p_txt);
static int cli_txt2bdaction(uint8_t* p_rtn_value, const char* p_txt);

static int cli_txtcpy_if_name(char* p_rtn_buf, const char* p_txt);
static int cli_txtcpy_table_name(char* p_rtn_buf, const char* p_txt);
static int cli_txtcpy_rule_name(char* p_rtn_buf, const char* p_txt);
static int cli_txtcpy_feature_name(char* p_rtn_buf, const char* p_txt);

static int cli_txt2num_u8(uint8_t* p_rtn_num, const char* p_txt, int base,
                          const uint8_t min, const uint8_t max);
static int cli_txt2num_u16(uint16_t* p_rtn_num, const char* p_txt, int base,
                           const uint16_t min, const uint16_t max);
static int cli_txt2num_u32(uint32_t* p_rtn_num, const char* p_txt, int base,
                           const uint32_t min, const uint32_t max);

static int cli_txt2num_i32(int32_t* p_rtn_num, const char* p_txt, int base,
                           const int32_t min, const int32_t max);

static int cli_txt2bitset32(uint32_t* p_rtn_bitset, const char* p_txt, const cb_txt2value_t p_cb_txt2value);
static int cli_txt2zprobs(uint8_t* p_rtn_zprobs, const char* p_txt);
static int cli_txt2sch_ins(struct sch_in_tt* p_rtn_struct_with_sch_ins, const char* p_txt);

static int cli_txt2mac(uint8_t* p_rtn_mac, const char* p_txt);
static int cli_txt2ip(bool* p_rtn_is6, uint32_t* p_rtn_ip, const char* p_txt);


/* ==== PRIVATE FUNCTIONS : opt_parse ======================================= */
/*
    Place your opt_parse callback functions here.
    Names of these opt_parse callback functions should be part of opt definitions in 'def_opts.h'.
    Search for keyword 'OPT_LAST' to get to the bottom of this section.
*/

static int opt_parse_ip4(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->ip4.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_ip6(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->ip6.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_all(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->all.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_help(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->help.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_verbose(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->verbose.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_version(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->version.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_interface(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->if_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->if_name.txt);
    
    rtn = cli_txtcpy_if_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_parent(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->if_name_parent.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->if_name_parent.txt);
    
    rtn = cli_txtcpy_if_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_mirror(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->if_name_mirror.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->if_name_mirror.txt);
    
    rtn = cli_txtcpy_if_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_mode(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool*              p_is_valid = &(p_rtn_cmdargs->if_mode.is_valid);
    fpp_phy_if_op_mode_t* p_value = &(p_rtn_cmdargs->if_mode.value);
    
    {
        /* parse input */
        uint8_t tmp_value = 0u;
        rtn = cli_txt2value_if_mode(&tmp_value, p_txt_optarg);
        
        /* assign data */
        if (CLI_OK == rtn)
        {
            *p_value = tmp_value;  /* WARNING: cast to enum */
        }
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_block_state(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool*                  p_is_valid = &(p_rtn_cmdargs->if_block_state.is_valid);
    fpp_phy_if_block_state_t* p_value = &(p_rtn_cmdargs->if_block_state.value);
    
    {
        /* parse input */
        uint8_t tmp_value = 0u;
        rtn = cli_txt2value_if_block_state(&tmp_value, p_txt_optarg);
        
        /* assign data */
        if (CLI_OK == rtn)
        {
            *p_value = tmp_value;  /* WARNING: cast to enum */
        }
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_enable(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->enable_noreply.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_no_reply(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->enable_noreply.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_disable(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->disable_noorig.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_no_orig(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->disable_noorig.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_promisc(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->promisc.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->promisc.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_loadbalance(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->loadbalance__ttl_decr.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->loadbalance__ttl_decr.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_ttl_decr(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->loadbalance__ttl_decr.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->loadbalance__ttl_decr.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_match_mode(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->match_mode.is_valid);
    bool* p_is_or    = &(p_rtn_cmdargs->match_mode.is_or);
    
    {
        /* parse input */
        uint8_t tmp_value = 0u;
        rtn = cli_txt2value_or_and(&tmp_value, p_txt_optarg);
        
        /* assign data */
        if (CLI_OK == rtn)
        {
            *p_is_or = (bool)(tmp_value);
        }
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_discard_on_match(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->discard_on_match.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->discard_on_match.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_egress(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid   = &(p_rtn_cmdargs->egress.is_valid);
    uint32_t* p_bitset = &(p_rtn_cmdargs->egress.bitset);
    
    rtn = cli_txt2bitset32(p_bitset, p_txt_optarg, cli_txt2value_phyif);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_match_rules(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    assert(sizeof(fpp_if_m_rules_t) == sizeof(uint32_t));
    
    
    int rtn = CLI_ERR;
    bool*           p_is_valid = &(p_rtn_cmdargs->match_rules.is_valid);
    fpp_if_m_rules_t* p_bitset = &(p_rtn_cmdargs->match_rules.bitset);
    
    {
        /* parse input */
        uint32_t tmp_bitset = 0u;
        rtn = cli_txt2bitset32(&tmp_bitset, p_txt_optarg, cli_txt2value_match_rule);
        
        /* assign data */
        if(CLI_OK == rtn)
        {
            *p_bitset = tmp_bitset;  /* WARNING: cast to enum */
        }
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_vlan(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->vlan.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->vlan.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_r_vlan(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->vlan2.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->vlan2.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_protocol(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->protocol.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->protocol.value);
    
    rtn = cli_txt2value_protocol(p_value, p_txt_optarg);
    if (CLI_OK != rtn)
    {
        rtn = cli_txt2num_u8(p_value, p_txt_optarg, BASE_DEC, 0u, UINT8_MAX);
    }
    if (CLI_OK != rtn)
    {
        rtn = cli_txt2num_u8(p_value, p_txt_optarg, BASE_HEX, 0u, UINT8_MAX);
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_ethtype(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->count_ethtype.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->count_ethtype.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    if (CLI_OK != rtn)
    {
        rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_HEX, 0u, UINT16_MAX);
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_count(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->count_ethtype.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->count_ethtype.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_mac(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->smac.is_valid);
    uint8_t* p_mac   =  (p_rtn_cmdargs->smac.arr);
    
    rtn = cli_txt2mac(p_mac, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_smac(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->smac.is_valid);
    uint8_t* p_mac   =  (p_rtn_cmdargs->smac.arr);
    
    rtn = cli_txt2mac(p_mac, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_dmac(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->dmac.is_valid);
    uint8_t* p_mac   =  (p_rtn_cmdargs->dmac.arr);
    
    rtn = cli_txt2mac(p_mac, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_sip(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->sip.is_valid);
    bool* p_is6      = &(p_rtn_cmdargs->sip.is6);
    uint32_t* p_ip   =  (p_rtn_cmdargs->sip.arr);
    
    rtn = cli_txt2ip(p_is6, p_ip, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_dip(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->dip.is_valid);
    bool* p_is6      = &(p_rtn_cmdargs->dip.is6);
    uint32_t* p_ip   =  (p_rtn_cmdargs->dip.arr);
    
    rtn = cli_txt2ip(p_is6, p_ip, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_r_sip(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->sip2.is_valid);
    bool* p_is6      = &(p_rtn_cmdargs->sip2.is6);
    uint32_t* p_ip   =  (p_rtn_cmdargs->sip2.arr);
    
    rtn = cli_txt2ip(p_is6, p_ip, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_r_dip(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->dip2.is_valid);
    bool* p_is6      = &(p_rtn_cmdargs->dip2.is6);
    uint32_t* p_ip   =  (p_rtn_cmdargs->dip2.arr);
    
    rtn = cli_txt2ip(p_is6, p_ip, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_sip6(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->sip2.is_valid);
    bool* p_is6      = &(p_rtn_cmdargs->sip2.is6);
    uint32_t* p_ip   =  (p_rtn_cmdargs->sip2.arr);
    
    rtn = cli_txt2ip(p_is6, p_ip, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_dip6(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->dip2.is_valid);
    bool* p_is6      = &(p_rtn_cmdargs->dip2.is6);
    uint32_t* p_ip   =  (p_rtn_cmdargs->dip2.arr);
    
    rtn = cli_txt2ip(p_is6, p_ip, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_sport(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->sport.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->sport.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_dport(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->dport.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->dport.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_r_sport(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->sport2.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->sport2.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_r_dport(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->dport2.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->dport2.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_hif_cookie(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->data_hifc_sad.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->data_hifc_sad.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_HEX, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_data(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->data_hifc_sad.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->data_hifc_sad.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_HEX, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_sad(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->data_hifc_sad.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->data_hifc_sad.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_DEC, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_mask(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->mask_spi.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->mask_spi.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_HEX, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_spi(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->mask_spi.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->mask_spi.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_HEX, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_timeout(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->timeout.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->timeout.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_DEC, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_timeout2(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->timeout2.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->timeout2.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_DEC, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_ucast_hit(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ucast_hit.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->ucast_hit.value);
    
    rtn = cli_txt2bdaction(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_ucast_miss(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ucast_miss.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->ucast_miss.value);
    
    rtn = cli_txt2bdaction(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_mcast_hit(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->mcast_hit.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->mcast_hit.value);
    
    rtn = cli_txt2bdaction(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_mcast_miss(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->mcast_miss.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->mcast_miss.value);
    
    rtn = cli_txt2bdaction(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_tag(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->tag.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->tag.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_default(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->default0.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_fallback(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->fallback_4o6.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_4o6(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->fallback_4o6.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_route(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->route.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->route.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_DEC, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_r_route(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->route2.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->route2.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_DEC, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_ingress_mr0(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ruleA0_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->ruleA0_name.txt);
    
    rtn = cli_txtcpy_table_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_ingress_mr1(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ruleA1_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->ruleA1_name.txt);
    
    rtn = cli_txtcpy_table_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_egress_mr0(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ruleB0_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->ruleB0_name.txt);
    
    rtn = cli_txtcpy_table_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_egress_mr1(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ruleB1_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->ruleB1_name.txt);
    
    rtn = cli_txtcpy_table_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_table(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->table0_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->table0_name.txt);
    
    rtn = cli_txtcpy_table_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_table0(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->table0_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->table0_name.txt);
    
    rtn = cli_txtcpy_table_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_flexible_filter(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->table0_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->table0_name.txt);
    
    rtn = cli_txtcpy_table_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_table1(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->table1_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->table1_name.txt);
    
    rtn = cli_txtcpy_table_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_rule(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ruleA0_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->ruleA0_name.txt);
    
    rtn = cli_txtcpy_rule_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_next_rule(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ruleB0_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->ruleB0_name.txt);
    
    rtn = cli_txtcpy_rule_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_layer(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool*              p_is_valid = &(p_rtn_cmdargs->layer.is_valid);
    fpp_fp_offset_from_t* p_value = &(p_rtn_cmdargs->layer.value);
    
    {
        /* parse input */
        uint8_t tmp_value = 0u;
        rtn = cli_txt2value_offset_from(&tmp_value, p_txt_optarg);
        if (CLI_OK != rtn)
        {
            rtn = cli_txt2num_u8(&tmp_value, p_txt_optarg, BASE_DEC, OFFSET_FROMS__MIN, OFFSET_FROMS__MAX);
        }
        
        /* assign data */
        if (CLI_OK == rtn)
        {
            *p_value = tmp_value;  /* WARNING: cast to enum */
        }
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_offset(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->offset.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->offset.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    if (CLI_OK != rtn)
    {
        rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_HEX, 0u, UINT16_MAX);
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_position(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->offset.is_valid);
    uint16_t* p_value = &(p_rtn_cmdargs->offset.value);
    
    rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_DEC, 0u, UINT16_MAX);
    if (CLI_OK != rtn)
    {
        rtn = cli_txt2num_u16(p_value, p_txt_optarg, BASE_HEX, 0u, UINT16_MAX);
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_invert(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->invert.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_accept(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->accept.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_reject(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->reject.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_spd_action(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool*          p_is_valid = &(p_rtn_cmdargs->spd_action.is_valid);
    fpp_spd_action_t* p_value = &(p_rtn_cmdargs->spd_action.value);
    
    {
        /* parse input */
        uint8_t tmp_value = 0u;
        rtn = cli_txt2value_spd_action(&tmp_value, p_txt_optarg);
        
        /* assign data */
        if (CLI_OK == rtn)
        {
            *p_value = tmp_value;  /* WARNING: cast to enum */
        }
    }
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_vlan_conf(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->vlan_conf__x_src.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->vlan_conf__x_src.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_discard_on_match_src(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->vlan_conf__x_src.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->vlan_conf__x_src.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_ptp_conf(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ptp_conf__x_dst.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->ptp_conf__x_dst.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_discard_on_match_dst(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ptp_conf__x_dst.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->ptp_conf__x_dst.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_ptp_promisc(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->ptp_promisc.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->ptp_promisc.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_loopback(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->loopback.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->loopback.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_qinq(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->qinq.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->qinq.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_local(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->local.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->local.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_feature(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->feature_name.is_valid);
    char* p_txt      =  (p_rtn_cmdargs->feature_name.txt);
    
    rtn = cli_txtcpy_feature_name(p_txt, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_static(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->static0.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_dynamic(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    UNUSED(p_txt_optarg);  /* just to suppress gcc warning */
    
    
    p_rtn_cmdargs->dynamic0.is_valid = true;
    return (CLI_OK);
}

static int opt_parse_que(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->que_sch_shp.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->que_sch_shp.value);
    
    rtn = cli_txt2num_u8(p_value, p_txt_optarg, BASE_DEC, 0u, UINT8_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_sch(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->que_sch_shp.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->que_sch_shp.value);
    
    rtn = cli_txt2num_u8(p_value, p_txt_optarg, BASE_DEC, 0u, UINT8_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_shp(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->que_sch_shp.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->que_sch_shp.value);
    
    rtn = cli_txt2num_u8(p_value, p_txt_optarg, BASE_DEC, 0u, UINT8_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_que_mode(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->que_sch_shp_mode.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->que_sch_shp_mode.value);
    
    rtn = cli_txt2value_que_mode(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_sch_mode(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->que_sch_shp_mode.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->que_sch_shp_mode.value);
    
    rtn = cli_txt2value_sch_mode(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_shp_mode(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->que_sch_shp_mode.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->que_sch_shp_mode.value);
    
    rtn = cli_txt2value_shp_mode(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_thmin(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->thmin.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->thmin.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_DEC, 0u, 255u);  /* according to FCI Reference, max queue depth is 255 */
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_thmax(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->thmax.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->thmax.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_DEC, 0u, 255u);  /* according to FCI Reference, max queue depth is 255 */
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_zprob(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->zprob.is_valid);
    uint8_t* p_arr   =  (p_rtn_cmdargs->zprob.arr);
    
    rtn = cli_txt2zprobs(p_arr, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_sch_algo(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->sch_algo.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->sch_algo.value);
    
    rtn = cli_txt2value_sch_algo(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_sch_in(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool*          p_is_valid  = &(p_rtn_cmdargs->sch_in.is_valid);
    struct sch_in_tt* p_struct = &(p_rtn_cmdargs->sch_in);
    
    rtn = cli_txt2sch_ins(p_struct, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_shp_pos(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->shp_pos.is_valid);
    uint8_t* p_value = &(p_rtn_cmdargs->shp_pos.value);
    
    rtn = cli_txt2value_shp_pos(p_value, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_isl(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid  = &(p_rtn_cmdargs->isl.is_valid);
    uint32_t* p_value = &(p_rtn_cmdargs->isl.value);
    
    rtn = cli_txt2num_u32(p_value, p_txt_optarg, BASE_DEC, 0u, UINT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_crmin(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->crmin.is_valid);
    int32_t* p_value = &(p_rtn_cmdargs->crmin.value);
    
    rtn = cli_txt2num_i32(p_value, p_txt_optarg, BASE_DEC, INT32_MIN, INT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_crmax(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->crmax.is_valid);
    int32_t* p_value = &(p_rtn_cmdargs->crmax.value);
    
    rtn = cli_txt2num_i32(p_value, p_txt_optarg, BASE_DEC, INT32_MIN, INT32_MAX);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}

static int opt_parse_discard_if_ttl_below_2(cli_cmdargs_t* p_rtn_cmdargs, const char* p_txt_optarg)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_optarg);
    
    
    int rtn = CLI_ERR;
    bool* p_is_valid = &(p_rtn_cmdargs->discard_if_ttl_below_2.is_valid);
    bool* p_is_on    = &(p_rtn_cmdargs->discard_if_ttl_below_2.is_on);
    
    rtn = cli_txt2bool_on_off(p_is_on, p_txt_optarg);
    
    set_if_rtn_ok(rtn, p_is_valid);
    return (rtn);
}











/* OPT_LAST (keep this at the bottom of the opt_parse section) */

/* ==== PRIVATE FUNCTIONS : misc =========================================== */

static void set_if_rtn_ok(int rtn_from_caller, bool* p_is_valid)
{
    assert(NULL != p_is_valid);
    if (CLI_OK == rtn_from_caller)
    {
        *p_is_valid = true;
    }
}

static int cli_txt2bool_on_off(bool* p_rtn_value, const char* p_txt)
{
    assert(NULL != p_rtn_value);
    assert(NULL != p_txt);
    
    
    /* parse input */
    uint8_t tmp_value = 0u;
    int rtn = cli_txt2value_on_off(&tmp_value, p_txt);
    
    /* assign data */
    if (CLI_OK == rtn)
    {
        *p_rtn_value = (bool)(tmp_value);
    }
    
    return (rtn);
}

static int cli_txt2bdaction(uint8_t* p_rtn_value, const char* p_txt)
{
    assert(NULL != p_rtn_value);
    assert(NULL != p_txt);
    
    
    int rtn = cli_txt2value_bd_action(p_rtn_value, p_txt);
    if (CLI_OK != rtn)
    {
        rtn = cli_txt2num_u8(p_rtn_value, p_txt, BASE_DEC, 0u, BD_ACTIONS__MAX);
    }
    
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : txtcpy ========================================= */

static int txtcpy(char* p_rtn_buf, const char* p_txt, const uint16_t buf_ln)
{
    assert(NULL != p_rtn_buf);
    assert(NULL != p_txt);
    
    
    int rtn = CLI_ERR;
    
    if ('-' == p_txt[0])
    {
        /* 
            If argument of an opt is missing in the input txt vector, then the next element in the txt vector
            (usually the next opt) is erroneously assumed to be the argument.
            For opts with non-txt target argument, this issue is "good enough" detectable (conversion from txt will fail).
            For opts with txt target argument, detection is a bit difficult.
            
            Ergo, for opts with txt target argument, a check for leading '-' character was decided to be "good enough".
            WARNING: This solution assumes that no valid txt target argument contains '-' as a leading character.
        */
        rtn = CLI_ERR_INVARG;
    }    
    else
    {
        if ((strlen(p_txt) + 1u) > buf_ln)  /* +1 for the string terminator */
        {
            rtn = CLI_ERR_INVARG;
        }
        else
        {
            strcpy(p_rtn_buf, p_txt);
            rtn = CLI_OK;
        }
    }
    
    return (rtn);
}

inline static int cli_txtcpy_if_name(char* p_rtn_buf, const char* p_txt)
{
    return txtcpy(p_rtn_buf, p_txt, IF_NAME_TXT_LN);
}

inline static int cli_txtcpy_table_name(char* p_rtn_buf, const char* p_txt)
{
    return txtcpy(p_rtn_buf, p_txt, TABLE_NAME_TXT_LN);
}

inline static int cli_txtcpy_rule_name(char* p_rtn_buf, const char* p_txt)
{
    return cli_txtcpy_table_name(p_rtn_buf, p_txt);  /* rule names use same parameters as table names */
}

inline static int cli_txtcpy_feature_name(char* p_rtn_buf, const char* p_txt)
{
    return txtcpy(p_rtn_buf, p_txt, FEATURE_NAME_TXT_LN);
}

/* ==== PRIVATE FUNCTIONS : txt2num (unsigned) ============================= */

static int txt2num_ull(unsigned long long* p_rtn_num, const char* p_txt, int base,
                       const unsigned long long min, const unsigned long long max)
{
    assert(NULL != p_rtn_num);
    assert(NULL != p_txt);
    
    
    int rtn = CLI_ERR;
    
    /* if HEX base, enforce leading '0x...' or '0X...' to prevent input ambiguity (from user's POV) */
    if (BASE_HEX == base)
    {
        rtn = (('0' == p_txt[0]) && (('x' == p_txt[1]) || ('X' == p_txt[1]))) ? (CLI_OK) : (CLI_ERR_INVARG);
    }
    else
    {
        rtn = CLI_OK;
    }
    
    /* convert the input */
    /* maximal expected size is u32 */
    /* unsigned long long conversion chosen in order to detect negative inputs (yields too large values for u32) */
    if (CLI_OK == rtn)
    {
        char* p_end = NULL;
        unsigned long long tmp_num = strtoull(p_txt, &p_end, base);
        if ((p_txt == p_end) || ('\0' != (*p_end)))  /* assumes that input is a single standalone number */
        {
            rtn = CLI_ERR_INVARG;
        }
        else if ((min > tmp_num) || (max < tmp_num))
        {
            rtn = CLI_ERR_INVARG;
        }
        else
        {
            *p_rtn_num = tmp_num;
            rtn = CLI_OK;
        }
    }
    
    return (rtn);
}

static int cli_txt2num_u8(uint8_t* p_rtn_num, const char* p_txt, int base,
                          const uint8_t min, const uint8_t max)
{
    assert(NULL != p_rtn_num);
    unsigned long long tmp_num = 0u;
    int rtn = txt2num_ull(&tmp_num, p_txt, base, min, max);
    if (CLI_OK == rtn)
    {
        *p_rtn_num = tmp_num;  /* WARNING: implicit narrowing cast */
    }
    return (rtn);
}

static int cli_txt2num_u16(uint16_t* p_rtn_num, const char* p_txt, int base,
                           const uint16_t min, const uint16_t max)
{
    assert(NULL != p_rtn_num);
    unsigned long long tmp_num = 0u;
    int rtn = txt2num_ull(&tmp_num, p_txt, base, min, max);
    if (CLI_OK == rtn)
    {
        *p_rtn_num = tmp_num;  /* WARNING: implicit narrowing cast */
    }
    return (rtn);
}

static int cli_txt2num_u32(uint32_t* p_rtn_num, const char* p_txt, int base,
                           const uint32_t min, const uint32_t max)
{
    assert(NULL != p_rtn_num);
    unsigned long long tmp_num = 0u;
    int rtn = txt2num_ull(&tmp_num, p_txt, base, min, max);
    if (CLI_OK == rtn)
    {
        *p_rtn_num = tmp_num;  /* WARNING: implicit narrowing cast */
    }
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : txt2num (signed) =============================== */

static int txt2num_ll(long long* p_rtn_num, const char* p_txt, int base,
                      const long long min, const long long max)
{
    assert(NULL != p_rtn_num);
    assert(NULL != p_txt);
    
    
    int rtn = CLI_ERR;
    
    /* if HEX base, enforce leading '0x...' or '0X...' to prevent input ambiguity (from user's POV) */
    if (BASE_HEX == base)
    {
        rtn = (('0' == p_txt[0]) && (('x' == p_txt[1]) || ('X' == p_txt[1]))) ? (CLI_OK) : (CLI_ERR_INVARG);
    }
    else
    {
        rtn = CLI_OK;
    }
    
    /* convert the input */
    /* maximal expected size is i32 */
    /* long long conversion chosen in order to detect negative inputs (yields too large values for i32) */
    if (CLI_OK == rtn)
    {
        char* p_end = NULL;
        long long tmp_num = strtoll(p_txt, &p_end, base);
        if ((p_txt == p_end) || ('\0' != (*p_end)))  /* assumes that input is a single standalone number */
        {
            rtn = CLI_ERR_INVARG;
        }
        else if ((min > tmp_num) || (max < tmp_num))
        {
            rtn = CLI_ERR_INVARG;
        }
        else
        {
            *p_rtn_num = tmp_num;
            rtn = CLI_OK;
        }
    }
    
    return (rtn);
}

static int cli_txt2num_i32(int32_t* p_rtn_num, const char* p_txt, int base,
                           const int32_t min, const int32_t max)
{
    assert(NULL != p_rtn_num);
    long long tmp_num = 0u;
    int rtn = txt2num_ll(&tmp_num, p_txt, base, min, max);
    if (CLI_OK == rtn)
    {
        *p_rtn_num = tmp_num;  /* WARNING: implicit narrowing cast */
    }
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : parse_substrings =============================== */

static int parse_substrings(void* p_rtn_void, const char* p_txt, const cb_txt2value_t p_cb_txt2value,
                            const cb_loop_t p_cb_loop, const uint8_t max_count)
{
    assert(NULL != p_rtn_void);
    assert(NULL != p_txt);
    assert(NULL != p_cb_loop);
    /* 'p_cb_txt2value' is allowed to be NULL */
    
    
    int rtn = CLI_ERR;
    char* p_txt_mutable = NULL;
    
    /* malloc a local modifiable copy of 'p_txt', because even if 'p_txt' was modifiable, it could still point to a literal --> problems galore */
    {
        const size_t ln = (strlen(p_txt) + 1u);
        p_txt_mutable = malloc(ln * sizeof(char));
        if (NULL == p_txt_mutable)
        {
            rtn = CLI_ERR_INVPTR;
        }
        else
        {
            memcpy(p_txt_mutable, p_txt, ln);
            rtn = CLI_OK;  /* greenlight execution of the next sub-block */
        }
    }
    
    /* run the target loop */
    if (CLI_OK == rtn)
    {
        uint8_t i = UINT8_MAX;  /* WARNING: intentional use of owf behavior */
        char* p_txt_token_next = strtok(p_txt_mutable, ",");
        
        /* loop_pre */
        if (CLI_OK == rtn)
        {
            rtn = p_cb_loop(p_rtn_void, "", p_cb_txt2value, LOOP_PRE, 0u);
        }
        
        /* loop_body */
        while ((max_count > (++i)) && (CLI_OK == rtn) && (NULL != p_txt_token_next))
        {
            /* some strtok() implementations improperly react on "3rd party" modifications of the CURRENT substring */
            /* work-around is to use the "current - 1" substring */
            char* p_txt_token = p_txt_token_next;
            p_txt_token_next = strtok(NULL, ",");
            rtn = p_cb_loop(p_rtn_void, p_txt_token, p_cb_txt2value, LOOP_BODY, i);
        }
        /* local post-loop check that there are no more substrings left */
        if ((max_count <= i) && (NULL != p_txt_token_next))
        {
            rtn = CLI_ERR_INVARG;
        }
        
        /* loop_post */
        if (CLI_OK == rtn)
        {
            rtn = p_cb_loop(p_rtn_void, "", p_cb_txt2value, LOOP_POST, i);
        }
    }
    
    /* free the malloc'd memory (do not hide behind rtn check) */
    if (NULL != p_txt_mutable)  /* better safe than sorry; some C-runtimes crash when NULL ptr is freed */
    {
        free(p_txt_mutable);
    }
    
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : txt2bitset ===================================== */

#define BITSET32_LN  32u

/* param 'i' refers to loop counter from 'parse_substrings()' fnc */
static int loop_bitset32(void* p_rtn_void, char* p_txt, const cb_txt2value_t p_cb_txt2value, loopstage_t loopstage, uint8_t i)
{
    assert(NULL != p_rtn_void);
    assert(NULL != p_cb_txt2value);
    /* 'p_txt' is allowed to be NULL */
    
    
    int rtn = CLI_ERR;
    uint32_t *const p_rtn_value = (uint32_t*)(p_rtn_void);
    
    if (LOOP_PRE == loopstage)
    {
        *p_rtn_value = 0u;
        rtn = CLI_OK;
    }
    else if ((LOOP_BODY == loopstage) && (NULL != p_txt))
    {
        UNUSED(i);  /* bitset flags in the cli optarg string can be listed in random order; precise index is therefore useless */
        uint8_t bitpos = 0u;
        
        /* get bitpos */
        rtn = p_cb_txt2value(&bitpos, p_txt);
        if (CLI_OK != rtn)
        {
            /* if txt2value fails, then maybe the txt token is directly a bitpos idx (some numeric value) */
            rtn = cli_txt2num_u8(&bitpos, p_txt, BASE_DEC, 0u, BITSET32_LN);
        }
        
        /* apply bitpos */
        if (CLI_OK == rtn)
        {
            if (BITSET32_LN <= bitpos)
            {
                rtn = CLI_ERR_INVARG;
            }
            else
            {
                *p_rtn_value |= (1uL << bitpos);  /* directly accessing rtn memory! */
            }
        }
    }
    else if (LOOP_POST == loopstage)
    {
        /* empty */
        rtn = CLI_OK;
    }
    else
    {
        rtn = CLI_ERR;
    }
    
    return (rtn);
}

static int cli_txt2bitset32(uint32_t* p_rtn_bitset, const char* p_txt, const cb_txt2value_t p_cb_txt2value)
{
    assert(NULL != p_rtn_bitset);
    uint32_t tmp_bitset = 0u;
    int rtn = parse_substrings(&tmp_bitset, p_txt, p_cb_txt2value, loop_bitset32, BITSET32_LN);
    if (CLI_OK == rtn)
    {
        *p_rtn_bitset = tmp_bitset;
    }
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : txt2zprobs ===================================== */

/* param 'i' refers to loop counter from 'parse_substrings()' fnc */
static int loop_zprobs(void* p_rtn_void, char* p_txt, const cb_txt2value_t p_cb_txt2value, loopstage_t loopstage, uint8_t i)
{
    assert(NULL != p_rtn_void);
    UNUSED(p_cb_txt2value);
    /* 'p_txt' is allowed to be NULL */
    
    
    int rtn = CLI_ERR;
    uint8_t *const p_rtn_value = (uint8_t*)(p_rtn_void) + i;  /* during LOOP_BODY, access directly the array element in question */
    
    if (LOOP_PRE == loopstage)
    {
        uint8_t keep = 0u;
        rtn = cli_txt2value_que_zprob_keep(&keep, TXT_QUE_ZPROB__KEEP);
        if (CLI_OK == rtn)
        {
            memset(p_rtn_value, keep, (sizeof(uint8_t) * ZPROBS_LN));
        }
    }
    else if ((LOOP_BODY == loopstage) && (NULL != p_txt))
    {
        rtn = cli_txt2value_que_zprob_keep(p_rtn_value, p_txt);
        if (CLI_OK != rtn)
        {
            rtn = cli_txt2num_u8(p_rtn_value, p_txt, BASE_DEC, 0u, 100u);  /* percentage */
        }
    }
    else if (LOOP_POST == loopstage)
    {
        /* empty */
        rtn = CLI_OK;
    }
    else
    {
        rtn = CLI_ERR;
    }
    
    return (rtn);
}

static int cli_txt2zprobs(uint8_t* p_rtn_zprobs, const char* p_txt)
{
    return parse_substrings(p_rtn_zprobs, p_txt, NULL, loop_zprobs, ZPROBS_LN);
}

/* ==== PRIVATE FUNCTIONS : txt2sch_ins ==================================== */

/* param 'i' refers to loop counter from 'parse_substrings()' fnc */
static int loop_sch_ins(void* p_rtn_void, char* p_txt, const cb_txt2value_t p_cb_txt2value, loopstage_t loopstage, uint8_t i)
{
    assert(NULL != p_rtn_void);
    UNUSED(p_cb_txt2value);
    /* 'p_txt' is allowed to be NULL */
    
    
    int rtn = CLI_ERR;
    struct sch_in_tt *const p_rtn_struct_with_sch_ins = (struct sch_in_tt*)(p_rtn_void);
    
    if (LOOP_PRE == loopstage)
    {
        uint8_t keep = 0u;
        rtn = cli_txt2value_sch_in(&keep, TXT_SCH_IN__KEEP);
        if (CLI_OK == rtn)
        {
            memset((p_rtn_struct_with_sch_ins->arr_src), keep, (sizeof(uint8_t) * SCH_INS_LN));
            memset((p_rtn_struct_with_sch_ins->arr_w), 0, (sizeof(uint32_t) * SCH_INS_LN));
        }
    }
    else if ((LOOP_BODY == loopstage) && (NULL != p_txt))
    {
        uint8_t  *const p_rtn_src = (p_rtn_struct_with_sch_ins->arr_src + i);
        uint32_t *const p_rtn_w   = (p_rtn_struct_with_sch_ins->arr_w   + i);
        
        /* split the input string */
        char* p_txt_src = p_txt;
        char* p_txt_w   = strchr(p_txt, ':');  /* ':' is a separator between the input keyword and the input weight */
        if (NULL != p_txt_w)
        {
            *p_txt_w++ = '\0';  /* if the string is valid, then it has at least two elements: non-NULL [0] element and NULL [1] element */
        }
        
        /* input src */
        rtn = cli_txt2value_sch_in(p_rtn_src, p_txt_src);
        
        /* input weight */
        if ((CLI_OK == rtn) && cli_sch_in_is_not_dis(*p_rtn_src) && cli_sch_in_is_not_keep(*p_rtn_src))
        {  
            if (NULL != p_txt_w)
            {
                rtn = cli_txt2num_u32(p_rtn_w, p_txt_w, BASE_DEC, 0u, UINT32_MAX);
            }
            else
            {
                rtn = CLI_ERR_INVARG;
            }
        }
    }
    else if (LOOP_POST == loopstage)
    {
        /* empty */
        rtn = CLI_OK;
    }
    else
    {
        rtn = CLI_ERR;
    }
    
    return (rtn);
}

static int cli_txt2sch_ins(struct sch_in_tt* p_rtn_struct_with_sch_ins, const char* p_txt)
{
    return parse_substrings(p_rtn_struct_with_sch_ins, p_txt, NULL, loop_sch_ins, SCH_INS_LN);
}

/* ==== PRIVATE FUNCTIONS : txt2mac ===================================== */

#define T2M_DELIMS_LN  (MAC_BYTES_LN - 1u)  /* there is no delimiter after the last byte of the mac address */
#define T2M_FMT_DELIM  "%2[-:]"
static int cli_txt2mac(uint8_t* p_rtn_mac, const char* p_txt)
{
    assert(NULL != p_rtn_mac);
    assert(NULL != p_txt);
    /* 'p_rtn_is_valid' is allowed to be NULL */
    #if (MAC_BYTES_LN != 6u)
    #error Unexpected MAC_BYTES_LN value! If not '6', then change 'sscanf()' parameters accordingly!
    #endif
    
    int rtn = CLI_ERR;
    
    /*
        WARNING: Size of the second dimension is expected to be (delim_width + 1).
                 Delim width is a width of a sscanf() specifier from the T2M_FMT_DELIM macro.
    */
    char txt_delims[T2M_DELIMS_LN][3] = {'\0'};
    uint8_t tmp_mac[MAC_BYTES_LN] = {0u};
    
    /* convert from txt input */
    rtn = (MAC_STRLEN != strlen(p_txt)) ? (CLI_ERR_INVARG) : (CLI_OK);
    if (CLI_OK == rtn)
    {
        const int item_cnt = sscanf(p_txt,  "%2"SCNx8  T2M_FMT_DELIM  "%2"SCNx8  T2M_FMT_DELIM  "%2"SCNx8  T2M_FMT_DELIM
                                            "%2"SCNx8  T2M_FMT_DELIM  "%2"SCNx8  T2M_FMT_DELIM  "%2"SCNx8,
                                            (tmp_mac + 0), txt_delims[0],
                                            (tmp_mac + 1), txt_delims[1],
                                            (tmp_mac + 2), txt_delims[2],
                                            (tmp_mac + 3), txt_delims[3],
                                            (tmp_mac + 4), txt_delims[4],
                                            (tmp_mac + 5));  /* no delims expected after the last mac byte */
        rtn = ((MAC_BYTES_LN + T2M_DELIMS_LN) != item_cnt) ? (CLI_ERR_INVARG) : (CLI_OK);
    }
    
    /* check delimiters ; correctly parsed txt input has no more than ONE non-null character per each delimiter */
    if (CLI_OK == rtn)
    {
        uint8_t i = UINT8_MAX;  /* WARNING: intentional use of owf behavior */
        while ((T2M_DELIMS_LN > (++i)) && ('\0' == txt_delims[i][1])) { /* empty */ }
        if (T2M_DELIMS_LN > i)
        {
            /* some delim was not correct */
            rtn = CLI_ERR_INVARG;
        }
    }
    
    /* assign return data */
    if (CLI_OK == rtn)
    {    
        memcpy(p_rtn_mac, tmp_mac, MAC_BYTES_LN);
    }
    
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : txt2ip ===================================== */

static int cli_txt2ip(bool* p_rtn_is6, uint32_t* p_rtn_ip, const char* p_txt)
{
    assert(NULL != p_rtn_ip);
    assert(NULL != p_rtn_is6);
    assert(NULL != p_txt);
    /* 'p_rtn_is_valid' is allowed to be NULL */
    #if (IP6_U32S_LN != 4u)
    #error Unexpected IP6_U32S_LN value! If not '4', then check whether the 'tmp_ip' is large enough for IPv6 output of 'inet_pton()'!
    #endif
    
    
    int rtn = CLI_ERR;
    bool tmp_is6 = false;
    uint32_t tmp_ip[IP6_U32S_LN] = {0uL};
    
    /* convert from input */
    if (1 == inet_pton(AF_INET, p_txt, tmp_ip))
    {
        tmp_is6 = false;
        rtn = CLI_OK;
    }
    else if (1 == inet_pton(AF_INET6, p_txt, tmp_ip))
    {
        tmp_is6 = true;
        rtn = CLI_OK;
    }
    else
    {
        rtn = CLI_ERR_INVARG;
    }
    
    /* assign return data */
    if (CLI_OK == rtn)
    {
        *p_rtn_is6 = tmp_is6;
        
        /* 'inet_pton()' returns in network order, but result of this fnc is expected in host order */
        for (uint8_t i = 0u; (IP6_U32S_LN > i); (++i))
        {
            p_rtn_ip[i] = ntohl(tmp_ip[i]);
        }
    }
    
    return (rtn);
}

/* ==== PRIVATE FUNCTIONS : parsers ======================================== */

static int cmd_parse(cli_cmd_t* p_rtn_cmd, const char* p_txt_cmd)
{   
    assert(NULL != p_rtn_cmd);
    assert(NULL != p_txt_cmd);
    
    
    int rtn = CLI_ERR;
    cli_cmd_t tmp_cmd = CMD_00_NO_COMMAND;
    
    rtn = cli_cmd_txt2cmd(&tmp_cmd, p_txt_cmd);
    
    /* special case: possible opts-only invocation if following conditions fulfilled */
    if ((CLI_ERR_INVCMD == rtn) && ('-' == p_txt_cmd[0]))
    {
        tmp_cmd = CMD_00_NO_COMMAND;
        rtn = CLI_OK;  /* NOTE: rtn reset */
    }
    
    /*  print error message if something went wrong  */
    if (CLI_OK != rtn)
    {
        const char* p_txt_err = "";
        switch (rtn)
        {
            case CLI_ERR_INVPTR:
                p_txt_err = "Invalid pointer while parsing a command name.\n"
                            "SERIOUS! NOTIFY A DEVELOPER!";
            break;
            
            case CLI_ERR_INVCMD:
                p_txt_err = "Unknown command.\n"
                            "Use option '--help' to get a list of all available commands.";
            break;
            
            default:
                p_txt_err = "Something unexpected happened while parsing a command name.\n"
                            "Check your input and try again. If the problem persists, notify a developer.";
            break;
        }
        cli_print_error(rtn, p_txt_err);
    }
    
    /* assign return data */
    if (CLI_OK == rtn)
    {
        *p_rtn_cmd = tmp_cmd;
    }
    
    return (rtn);
}

/* WARNING: argument 'p_txt_vec' must NOT be const, otherwise 'getopt()' fnc family will induce UB! */
static int opts_parse(cli_cmdargs_t* p_rtn_cmdargs, char* p_txt_vec[], int vec_ln)
{
    assert(NULL != p_rtn_cmdargs);
    assert(NULL != p_txt_vec);
    
    
    int rtn = CLI_OK;  /* NOTE: initial OK is required for the 'getopt()' processing loop to start properly */
    
    memset(p_rtn_cmdargs, 0, sizeof(cli_cmdargs_t));
    
    const char* p_txt_opt = "__NOITEM__";
    const char* p_txt_opt_addit = "";   /* WARNING: must be initiated by "" to prevent malformed error texts */
    
    /*
        Ptr array for incompatibility checks of processed cli opt (see 'def_opts.h').
        Each array element is a slot for one incompat grp.
        When a cli opt from some incompat grp is encountered, a ptr to the opt's txt is stored in this array.
        Then, if another cli opt from the same incompat grp is encountered, an error is raised and ptr is used for error message.
    */
    const char* txt_incompat_grps[OPT_GRP_LN] = {NULL};
    
    #if !defined(NDEBUG)
    TEST_parser__p_txt_opt = NULL;
    #endif
    
    /* glorious 'getopt()' processing */
    int opt_code = 0;
    const struct option* p_longopts = cli_get_longopts();
    const char* p_txt_shortopts = cli_get_txt_shortopts();
    optind = 1;  /* global var of 'getopt()' fnc family ; must be manually reset each time a new input txt vector is to be parsed */
    while ((CLI_OK == rtn) && (-1 != opt_code))
    {
        const int optind_curr = optind;  /* store idx of the currently processed element, because 'getopt()' invocation will set optind as idx of the NEXT element */
        p_txt_opt = (((optind_curr >= 1) && (optind_curr < vec_ln)) ? (p_txt_vec[optind_curr]) : "__INVIDX__");
        
        opt_code = getopt_long(vec_ln, p_txt_vec, p_txt_shortopts, p_longopts, NULL);
        
        #if !defined(NDEBUG)
        TEST_parser__p_txt_opt = p_txt_opt;
        #endif
        
        
        /* 
            special custom checks
           
            [1] Invalidate (mark as unknown) those input txt vector elements, which have valid shortopt syntax
                but are longer than 2 characters. This is done to remove the possibility of erroneously interpreting
                a longopt as multiple shortopts folded into one txt element.
                Such a situation can happen by accident if there is only one leading '-' character in front of a longopt.
               
            [2] BUGFIX for a specific 'longopt()' corner case:
                If the very last element of the input txt vector is a longopt which requires an argument,
                and if by mistake no argument is supplied (no arg is present within the element itself),
                then 'getopt()' sets its global 'optarg' ptr to point to some seemingly random address.
                FIXED via bound-checking the 'optarg' destination address in case it is a non-NULL value when
                      the very last element of the input txt vector is processed.
        */
        if ((NULL != p_txt_opt) && (-1 != opt_code))
        {
            const size_t ln = strlen(p_txt_opt);
            
            if ((2u < ln) && ('-' == p_txt_opt[0]) && ('-' != p_txt_opt[1]))
            {
                opt_code = '?';
            }
            
            if ((NULL != optarg) && ((optind_curr + 1) >= vec_ln) && ((optarg < p_txt_opt) || (optarg > (p_txt_opt + ln))))
            {
                opt_code = ':';
            }
        }
        
        
        /* incompatibility checks of processed cli opt */
        if (-1 != opt_code)
        {
            uint32_t grps = cli_opt_get_incompat_grps(opt_code);
            if (0uL != grps)
            {
                assert((CHAR_BIT * sizeof(uint32_t)) > OPT_GRP_LN);  /* to prevent potential UB */
                for (uint32_t i = 0uL; (OPT_GRP_LN > i); (++i))
                {
                    if (grps & (1uL << i))
                    {
                        if (NULL != txt_incompat_grps[i])
                        {
                            p_txt_opt_addit = txt_incompat_grps[i];
                            rtn = CLI_ERR_INCOMPATIBLE_OPTS;
                        }
                        else
                        {
                            txt_incompat_grps[i] = p_txt_opt;
                            rtn = CLI_OK;
                        }
                    }
                }
            }
        }
        
        /*
            opt parsing
            Parse fncs for particular opts are at the beginning of this source file.
            Binding between OPT_xx_OPT_PARSE symbols and parse fncs is specified in 'def_opts.h'.
            This code here should not need any modifications when a new opt is added...
        */
        if (CLI_OK == rtn)
        {
            switch (opt_code)
            {
              #ifdef OPT_01_ENUM_NAME
                case OPT_01_ENUM_NAME:
                    rtn = OPT_01_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_02_ENUM_NAME
                case OPT_02_ENUM_NAME:
                    rtn = OPT_02_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_03_ENUM_NAME
                case OPT_03_ENUM_NAME:
                    rtn = OPT_03_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_04_ENUM_NAME
                case OPT_04_ENUM_NAME:
                    rtn = OPT_04_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_05_ENUM_NAME
                case OPT_05_ENUM_NAME:
                    rtn = OPT_05_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_06_ENUM_NAME
                case OPT_06_ENUM_NAME:
                    rtn = OPT_06_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_07_ENUM_NAME
                case OPT_07_ENUM_NAME:
                    rtn = OPT_07_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_08_ENUM_NAME
                case OPT_08_ENUM_NAME:
                    rtn = OPT_08_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_09_ENUM_NAME
                case OPT_09_ENUM_NAME:
                    rtn = OPT_09_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_10_ENUM_NAME
                case OPT_10_ENUM_NAME:
                    rtn = OPT_10_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_11_ENUM_NAME
                case OPT_11_ENUM_NAME:
                    rtn = OPT_11_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_12_ENUM_NAME
                case OPT_12_ENUM_NAME:
                    rtn = OPT_12_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_13_ENUM_NAME
                case OPT_13_ENUM_NAME:
                    rtn = OPT_13_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_14_ENUM_NAME
                case OPT_14_ENUM_NAME:
                    rtn = OPT_14_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_15_ENUM_NAME
                case OPT_15_ENUM_NAME:
                    rtn = OPT_15_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_16_ENUM_NAME
                case OPT_16_ENUM_NAME:
                    rtn = OPT_16_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_17_ENUM_NAME
                case OPT_17_ENUM_NAME:
                    rtn = OPT_17_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_18_ENUM_NAME
                case OPT_18_ENUM_NAME:
                    rtn = OPT_18_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_19_ENUM_NAME
                case OPT_19_ENUM_NAME:
                    rtn = OPT_19_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_20_ENUM_NAME
                case OPT_20_ENUM_NAME:
                    rtn = OPT_20_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_21_ENUM_NAME
                case OPT_21_ENUM_NAME:
                    rtn = OPT_21_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_22_ENUM_NAME
                case OPT_22_ENUM_NAME:
                    rtn = OPT_22_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_23_ENUM_NAME
                case OPT_23_ENUM_NAME:
                    rtn = OPT_23_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_24_ENUM_NAME
                case OPT_24_ENUM_NAME:
                    rtn = OPT_24_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_25_ENUM_NAME
                case OPT_25_ENUM_NAME:
                    rtn = OPT_25_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_26_ENUM_NAME
                case OPT_26_ENUM_NAME:
                    rtn = OPT_26_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_27_ENUM_NAME
                case OPT_27_ENUM_NAME:
                    rtn = OPT_27_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_28_ENUM_NAME
                case OPT_28_ENUM_NAME:
                    rtn = OPT_28_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_29_ENUM_NAME
                case OPT_29_ENUM_NAME:
                    rtn = OPT_29_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_30_ENUM_NAME
                case OPT_30_ENUM_NAME:
                    rtn = OPT_30_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_31_ENUM_NAME
                case OPT_31_ENUM_NAME:
                    rtn = OPT_31_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_32_ENUM_NAME
                case OPT_32_ENUM_NAME:
                    rtn = OPT_32_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_33_ENUM_NAME
                case OPT_33_ENUM_NAME:
                    rtn = OPT_33_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_34_ENUM_NAME
                case OPT_34_ENUM_NAME:
                    rtn = OPT_34_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_35_ENUM_NAME
                case OPT_35_ENUM_NAME:
                    rtn = OPT_35_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_36_ENUM_NAME
                case OPT_36_ENUM_NAME:
                    rtn = OPT_36_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_37_ENUM_NAME
                case OPT_37_ENUM_NAME:
                    rtn = OPT_37_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_38_ENUM_NAME
                case OPT_38_ENUM_NAME:
                    rtn = OPT_38_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_39_ENUM_NAME
                case OPT_39_ENUM_NAME:
                    rtn = OPT_39_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_40_ENUM_NAME
                case OPT_40_ENUM_NAME:
                    rtn = OPT_40_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_41_ENUM_NAME
                case OPT_41_ENUM_NAME:
                    rtn = OPT_41_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_42_ENUM_NAME
                case OPT_42_ENUM_NAME:
                    rtn = OPT_42_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_43_ENUM_NAME
                case OPT_43_ENUM_NAME:
                    rtn = OPT_43_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_44_ENUM_NAME
                case OPT_44_ENUM_NAME:
                    rtn = OPT_44_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_45_ENUM_NAME
                case OPT_45_ENUM_NAME:
                    rtn = OPT_45_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_46_ENUM_NAME
                case OPT_46_ENUM_NAME:
                    rtn = OPT_46_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_47_ENUM_NAME
                case OPT_47_ENUM_NAME:
                    rtn = OPT_47_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_48_ENUM_NAME
                case OPT_48_ENUM_NAME:
                    rtn = OPT_48_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_49_ENUM_NAME
                case OPT_49_ENUM_NAME:
                    rtn = OPT_49_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_50_ENUM_NAME
                case OPT_50_ENUM_NAME:
                    rtn = OPT_50_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_51_ENUM_NAME
                case OPT_51_ENUM_NAME:
                    rtn = OPT_51_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_52_ENUM_NAME
                case OPT_52_ENUM_NAME:
                    rtn = OPT_52_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_53_ENUM_NAME
                case OPT_53_ENUM_NAME:
                    rtn = OPT_53_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_54_ENUM_NAME
                case OPT_54_ENUM_NAME:
                    rtn = OPT_54_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_55_ENUM_NAME
                case OPT_55_ENUM_NAME:
                    rtn = OPT_55_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_56_ENUM_NAME
                case OPT_56_ENUM_NAME:
                    rtn = OPT_56_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_57_ENUM_NAME
                case OPT_57_ENUM_NAME:
                    rtn = OPT_57_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_58_ENUM_NAME
                case OPT_58_ENUM_NAME:
                    rtn = OPT_58_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_59_ENUM_NAME
                case OPT_59_ENUM_NAME:
                    rtn = OPT_59_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_60_ENUM_NAME
                case OPT_60_ENUM_NAME:
                    rtn = OPT_60_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_61_ENUM_NAME
                case OPT_61_ENUM_NAME:
                    rtn = OPT_61_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_62_ENUM_NAME
                case OPT_62_ENUM_NAME:
                    rtn = OPT_62_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_63_ENUM_NAME
                case OPT_63_ENUM_NAME:
                    rtn = OPT_63_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_64_ENUM_NAME
                case OPT_64_ENUM_NAME:
                    rtn = OPT_64_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_65_ENUM_NAME
                case OPT_65_ENUM_NAME:
                    rtn = OPT_65_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_66_ENUM_NAME
                case OPT_66_ENUM_NAME:
                    rtn = OPT_66_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_67_ENUM_NAME
                case OPT_67_ENUM_NAME:
                    rtn = OPT_67_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_68_ENUM_NAME
                case OPT_68_ENUM_NAME:
                    rtn = OPT_68_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_69_ENUM_NAME
                case OPT_69_ENUM_NAME:
                    rtn = OPT_69_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_70_ENUM_NAME
                case OPT_70_ENUM_NAME:
                    rtn = OPT_70_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_71_ENUM_NAME
                case OPT_71_ENUM_NAME:
                    rtn = OPT_71_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_72_ENUM_NAME
                case OPT_72_ENUM_NAME:
                    rtn = OPT_72_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_73_ENUM_NAME
                case OPT_73_ENUM_NAME:
                    rtn = OPT_73_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_74_ENUM_NAME
                case OPT_74_ENUM_NAME:
                    rtn = OPT_74_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_75_ENUM_NAME
                case OPT_75_ENUM_NAME:
                    rtn = OPT_75_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_76_ENUM_NAME
                case OPT_76_ENUM_NAME:
                    rtn = OPT_76_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_77_ENUM_NAME
                case OPT_77_ENUM_NAME:
                    rtn = OPT_77_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_78_ENUM_NAME
                case OPT_78_ENUM_NAME:
                    rtn = OPT_78_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_79_ENUM_NAME
                case OPT_79_ENUM_NAME:
                    rtn = OPT_79_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_80_ENUM_NAME
                case OPT_80_ENUM_NAME:
                    rtn = OPT_80_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_81_ENUM_NAME
                case OPT_81_ENUM_NAME:
                    rtn = OPT_81_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_82_ENUM_NAME
                case OPT_82_ENUM_NAME:
                    rtn = OPT_82_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_83_ENUM_NAME
                case OPT_83_ENUM_NAME:
                    rtn = OPT_83_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_84_ENUM_NAME
                case OPT_84_ENUM_NAME:
                    rtn = OPT_84_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_85_ENUM_NAME
                case OPT_85_ENUM_NAME:
                    rtn = OPT_85_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_86_ENUM_NAME
                case OPT_86_ENUM_NAME:
                    rtn = OPT_86_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_87_ENUM_NAME
                case OPT_87_ENUM_NAME:
                    rtn = OPT_87_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_88_ENUM_NAME
                case OPT_88_ENUM_NAME:
                    rtn = OPT_88_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_89_ENUM_NAME
                case OPT_89_ENUM_NAME:
                    rtn = OPT_89_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_90_ENUM_NAME
                case OPT_90_ENUM_NAME:
                    rtn = OPT_90_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_91_ENUM_NAME
                case OPT_91_ENUM_NAME:
                    rtn = OPT_91_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_92_ENUM_NAME
                case OPT_92_ENUM_NAME:
                    rtn = OPT_92_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_93_ENUM_NAME
                case OPT_93_ENUM_NAME:
                    rtn = OPT_93_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_94_ENUM_NAME
                case OPT_94_ENUM_NAME:
                    rtn = OPT_94_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_95_ENUM_NAME
                case OPT_95_ENUM_NAME:
                    rtn = OPT_95_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_96_ENUM_NAME
                case OPT_96_ENUM_NAME:
                    rtn = OPT_96_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_97_ENUM_NAME
                case OPT_97_ENUM_NAME:
                    rtn = OPT_97_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_98_ENUM_NAME
                case OPT_98_ENUM_NAME:
                    rtn = OPT_98_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_99_ENUM_NAME
                case OPT_99_ENUM_NAME:
                    rtn = OPT_99_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              
              #ifdef OPT_100_ENUM_NAME
                case OPT_100_ENUM_NAME:
                    rtn = OPT_100_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif              
              #ifdef OPT_101_ENUM_NAME
                case OPT_101_ENUM_NAME:
                    rtn = OPT_101_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_102_ENUM_NAME
                case OPT_102_ENUM_NAME:
                    rtn = OPT_102_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_103_ENUM_NAME
                case OPT_103_ENUM_NAME:
                    rtn = OPT_103_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_104_ENUM_NAME
                case OPT_104_ENUM_NAME:
                    rtn = OPT_104_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_105_ENUM_NAME
                case OPT_105_ENUM_NAME:
                    rtn = OPT_105_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_106_ENUM_NAME
                case OPT_106_ENUM_NAME:
                    rtn = OPT_106_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_107_ENUM_NAME
                case OPT_107_ENUM_NAME:
                    rtn = OPT_107_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_108_ENUM_NAME
                case OPT_108_ENUM_NAME:
                    rtn = OPT_108_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_109_ENUM_NAME
                case OPT_109_ENUM_NAME:
                    rtn = OPT_109_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_110_ENUM_NAME
                case OPT_110_ENUM_NAME:
                    rtn = OPT_110_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_111_ENUM_NAME
                case OPT_111_ENUM_NAME:
                    rtn = OPT_111_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_112_ENUM_NAME
                case OPT_112_ENUM_NAME:
                    rtn = OPT_112_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_113_ENUM_NAME
                case OPT_113_ENUM_NAME:
                    rtn = OPT_113_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_114_ENUM_NAME
                case OPT_114_ENUM_NAME:
                    rtn = OPT_114_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_115_ENUM_NAME
                case OPT_115_ENUM_NAME:
                    rtn = OPT_115_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_116_ENUM_NAME
                case OPT_116_ENUM_NAME:
                    rtn = OPT_116_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_117_ENUM_NAME
                case OPT_117_ENUM_NAME:
                    rtn = OPT_117_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_118_ENUM_NAME
                case OPT_118_ENUM_NAME:
                    rtn = OPT_118_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_119_ENUM_NAME
                case OPT_119_ENUM_NAME:
                    rtn = OPT_119_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_120_ENUM_NAME
                case OPT_120_ENUM_NAME:
                    rtn = OPT_120_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_121_ENUM_NAME
                case OPT_121_ENUM_NAME:
                    rtn = OPT_121_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_122_ENUM_NAME
                case OPT_122_ENUM_NAME:
                    rtn = OPT_122_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_123_ENUM_NAME
                case OPT_123_ENUM_NAME:
                    rtn = OPT_123_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_124_ENUM_NAME
                case OPT_124_ENUM_NAME:
                    rtn = OPT_124_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_125_ENUM_NAME
                case OPT_125_ENUM_NAME:
                    rtn = OPT_125_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_126_ENUM_NAME
                case OPT_126_ENUM_NAME:
                    rtn = OPT_126_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_127_ENUM_NAME
                case OPT_127_ENUM_NAME:
                    rtn = OPT_127_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_128_ENUM_NAME
                case OPT_128_ENUM_NAME:
                    rtn = OPT_128_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_129_ENUM_NAME
                case OPT_129_ENUM_NAME:
                    rtn = OPT_129_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_130_ENUM_NAME
                case OPT_130_ENUM_NAME:
                    rtn = OPT_130_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_131_ENUM_NAME
                case OPT_131_ENUM_NAME:
                    rtn = OPT_131_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_132_ENUM_NAME
                case OPT_132_ENUM_NAME:
                    rtn = OPT_132_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_133_ENUM_NAME
                case OPT_133_ENUM_NAME:
                    rtn = OPT_133_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_134_ENUM_NAME
                case OPT_134_ENUM_NAME:
                    rtn = OPT_134_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_135_ENUM_NAME
                case OPT_135_ENUM_NAME:
                    rtn = OPT_135_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_136_ENUM_NAME
                case OPT_136_ENUM_NAME:
                    rtn = OPT_136_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_137_ENUM_NAME
                case OPT_137_ENUM_NAME:
                    rtn = OPT_137_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_138_ENUM_NAME
                case OPT_138_ENUM_NAME:
                    rtn = OPT_138_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_139_ENUM_NAME
                case OPT_139_ENUM_NAME:
                    rtn = OPT_139_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_140_ENUM_NAME
                case OPT_140_ENUM_NAME:
                    rtn = OPT_140_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_141_ENUM_NAME
                case OPT_141_ENUM_NAME:
                    rtn = OPT_141_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_142_ENUM_NAME
                case OPT_142_ENUM_NAME:
                    rtn = OPT_142_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_143_ENUM_NAME
                case OPT_143_ENUM_NAME:
                    rtn = OPT_143_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_144_ENUM_NAME
                case OPT_144_ENUM_NAME:
                    rtn = OPT_144_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_145_ENUM_NAME
                case OPT_145_ENUM_NAME:
                    rtn = OPT_145_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_146_ENUM_NAME
                case OPT_146_ENUM_NAME:
                    rtn = OPT_146_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_147_ENUM_NAME
                case OPT_147_ENUM_NAME:
                    rtn = OPT_147_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_148_ENUM_NAME
                case OPT_148_ENUM_NAME:
                    rtn = OPT_148_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_149_ENUM_NAME
                case OPT_149_ENUM_NAME:
                    rtn = OPT_149_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_150_ENUM_NAME
                case OPT_150_ENUM_NAME:
                    rtn = OPT_150_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_151_ENUM_NAME
                case OPT_151_ENUM_NAME:
                    rtn = OPT_151_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_152_ENUM_NAME
                case OPT_152_ENUM_NAME:
                    rtn = OPT_152_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_153_ENUM_NAME
                case OPT_153_ENUM_NAME:
                    rtn = OPT_153_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_154_ENUM_NAME
                case OPT_154_ENUM_NAME:
                    rtn = OPT_154_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_155_ENUM_NAME
                case OPT_155_ENUM_NAME:
                    rtn = OPT_155_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_156_ENUM_NAME
                case OPT_156_ENUM_NAME:
                    rtn = OPT_156_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_157_ENUM_NAME
                case OPT_157_ENUM_NAME:
                    rtn = OPT_157_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_158_ENUM_NAME
                case OPT_158_ENUM_NAME:
                    rtn = OPT_158_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_159_ENUM_NAME
                case OPT_159_ENUM_NAME:
                    rtn = OPT_159_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_160_ENUM_NAME
                case OPT_160_ENUM_NAME:
                    rtn = OPT_160_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_161_ENUM_NAME
                case OPT_161_ENUM_NAME:
                    rtn = OPT_161_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_162_ENUM_NAME
                case OPT_162_ENUM_NAME:
                    rtn = OPT_162_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_163_ENUM_NAME
                case OPT_163_ENUM_NAME:
                    rtn = OPT_163_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_164_ENUM_NAME
                case OPT_164_ENUM_NAME:
                    rtn = OPT_164_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_165_ENUM_NAME
                case OPT_165_ENUM_NAME:
                    rtn = OPT_165_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_166_ENUM_NAME
                case OPT_166_ENUM_NAME:
                    rtn = OPT_166_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_167_ENUM_NAME
                case OPT_167_ENUM_NAME:
                    rtn = OPT_167_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_168_ENUM_NAME
                case OPT_168_ENUM_NAME:
                    rtn = OPT_168_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_169_ENUM_NAME
                case OPT_169_ENUM_NAME:
                    rtn = OPT_169_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_170_ENUM_NAME
                case OPT_170_ENUM_NAME:
                    rtn = OPT_170_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_171_ENUM_NAME
                case OPT_171_ENUM_NAME:
                    rtn = OPT_171_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_172_ENUM_NAME
                case OPT_172_ENUM_NAME:
                    rtn = OPT_172_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_173_ENUM_NAME
                case OPT_173_ENUM_NAME:
                    rtn = OPT_173_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_174_ENUM_NAME
                case OPT_174_ENUM_NAME:
                    rtn = OPT_174_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_175_ENUM_NAME
                case OPT_175_ENUM_NAME:
                    rtn = OPT_175_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_176_ENUM_NAME
                case OPT_176_ENUM_NAME:
                    rtn = OPT_176_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_177_ENUM_NAME
                case OPT_177_ENUM_NAME:
                    rtn = OPT_177_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_178_ENUM_NAME
                case OPT_178_ENUM_NAME:
                    rtn = OPT_178_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_179_ENUM_NAME
                case OPT_179_ENUM_NAME:
                    rtn = OPT_179_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_180_ENUM_NAME
                case OPT_180_ENUM_NAME:
                    rtn = OPT_180_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_181_ENUM_NAME
                case OPT_181_ENUM_NAME:
                    rtn = OPT_181_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_182_ENUM_NAME
                case OPT_182_ENUM_NAME:
                    rtn = OPT_182_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_183_ENUM_NAME
                case OPT_183_ENUM_NAME:
                    rtn = OPT_183_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_184_ENUM_NAME
                case OPT_184_ENUM_NAME:
                    rtn = OPT_184_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_185_ENUM_NAME
                case OPT_185_ENUM_NAME:
                    rtn = OPT_185_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_186_ENUM_NAME
                case OPT_186_ENUM_NAME:
                    rtn = OPT_186_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_187_ENUM_NAME
                case OPT_187_ENUM_NAME:
                    rtn = OPT_187_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_188_ENUM_NAME
                case OPT_188_ENUM_NAME:
                    rtn = OPT_188_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_189_ENUM_NAME
                case OPT_189_ENUM_NAME:
                    rtn = OPT_189_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
              #ifdef OPT_190_ENUM_NAME
                case OPT_190_ENUM_NAME:
                    rtn = OPT_190_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_191_ENUM_NAME
                case OPT_191_ENUM_NAME:
                    rtn = OPT_191_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_192_ENUM_NAME
                case OPT_192_ENUM_NAME:
                    rtn = OPT_192_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_193_ENUM_NAME
                case OPT_193_ENUM_NAME:
                    rtn = OPT_193_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_194_ENUM_NAME
                case OPT_194_ENUM_NAME:
                    rtn = OPT_194_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_195_ENUM_NAME
                case OPT_195_ENUM_NAME:
                    rtn = OPT_195_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_196_ENUM_NAME
                case OPT_196_ENUM_NAME:
                    rtn = OPT_196_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_197_ENUM_NAME
                case OPT_197_ENUM_NAME:
                    rtn = OPT_197_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_198_ENUM_NAME
                case OPT_198_ENUM_NAME:
                    rtn = OPT_198_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
              #ifdef OPT_199_ENUM_NAME
                case OPT_199_ENUM_NAME:
                    rtn = OPT_199_OPT_PARSE (p_rtn_cmdargs, optarg);
                break;
              #endif
                
                case -1:  /* parsing is finished */
                    /* empty */
                break;
                
                case ':':  /* missing argument */
                    rtn = CLI_ERR_INVARG;
                break;
                
                case '?':  /* unknown or invalid option */
                    rtn = CLI_ERR_INVOPT;
                break;
                
                default:
                    rtn = CLI_ERR;
                break;
            }
        }
    }
    
    /* raise error if there are some non-opt elements left in the 'pa_txt_vec' */
    /* such non-opt elements are often in fact opts, which are (by input mistake) missing the leading '-' or '--' */
    if ((CLI_OK == rtn) && (optind < vec_ln))
    {
        p_txt_opt = p_txt_vec[optind];
        p_txt_opt_addit = "";
        rtn = CLI_ERR_NONOPT;
    }
    
    /* reset 'getopt()' fnc family internal static variables (do not hide behind rtn check) */
    /* WARNING: This hack is crucial in order to ensure that 'getopt()' fnc family behaves correctly each time new input txt vector is scanned. */
    while(-1 != getopt_long(vec_ln, p_txt_vec, p_txt_shortopts, p_longopts, NULL)) { /* empty */ };

    /*  print error message if something went wrong  */
    if (CLI_OK != rtn)
    {  
        const char* p_txt_err = "";
        switch (rtn)
        {
            case CLI_ERR_INVPTR:
                p_txt_err = "Invalid pointer while parsing the option '%s%s'.\n"
                            "SERIOUS! NOTIFY A DEVELOPER!";
            break;
            
            case CLI_ERR_INVOPT:
                p_txt_err = "Unknown option '%s%s'. (maybe check leading '-' or '--'?)\n"
                            "Use '<command> --help' to get a detailed info (and a list of valid options) for the given command.";
            break;
            
            case CLI_ERR_INVARG:
                p_txt_err = "Invalid or missing argument(s) for the option '%s%s'.\n"
                            "If not missing, then maybe wrong upper/lower case? Or something too small/large/long?";
            break;
            
            case CLI_ERR_NONOPT:
                p_txt_err = "Non-option argument '%s%s' detected. (maybe it's just missing the '-' or '--'?)";
            break;
            
            case CLI_ERR_INCOMPATIBLE_OPTS:
                p_txt_err = "Options '%s' and '%s' cannot be used at the same time.";
            break;
            
            default:
                p_txt_err = "Something unexpected happened while parsing the option '%s%s'.\n"
                            "Check your input and try again. If the problem persists, notify a developer.";
            break;
        }
        cli_print_error(rtn, p_txt_err, p_txt_opt, p_txt_opt_addit);
    }
    
    /* just for debug/test purposes; this call passes 'cmdargs' data to unit tests */
    #if !defined(NDEBUG)
    cli_cmd_execute(CMD_00_NO_COMMAND, p_rtn_cmdargs);
    #endif
    
    return (rtn);
}

static int cmd_execute(cli_cmd_t cmd, const cli_cmdargs_t* p_cmdargs)
{
    assert(NULL != p_cmdargs);
    
    
    int rtn = CLI_ERR;
    
    /* NOTE: The situation 'app started with no cli arguments' is NOT HANDLED HERE (probably handled in main()) */
    /*       That is intentional. It allows this fnc to handle session mode if needed (no input == do nothing) */
    if ((CMD_00_NO_COMMAND == cmd) && (p_cmdargs->version.is_valid))
    {
        cli_print_app_version();
        rtn = CLI_OK;
    }
    else if (p_cmdargs->help.is_valid)
    {
        /* special execution path for help texts */
        if ((CMD_00_NO_COMMAND == cmd) && (p_cmdargs->verbose.is_valid))
        {
            /* print all help texts (Great Wall of text ^_^) */
            cli_print_app_version();
            for (uint16_t i = 0u; (CMD_LN > i); (++i))
            {
                cli_print_help(i);
            }
        }
        else
        {
            /* print help text for a particular command */
            cli_print_help(cmd);    
        }
        rtn = CLI_OK;
    }
    else
    {
        /* standard execution path */
        rtn = cli_cmd_execute(cmd, p_cmdargs);
    }
    
    /* print error message if something went wrong */
    if (CLI_OK != rtn)
    {
        const char* p_txt_err = "";
        bool do_mandopt_print = false;
        switch (rtn)
        {
            /* errors of the libFCI_cli app */
            
            case CLI_ERR_INVPTR:
                p_txt_err = "Invalid pointer during execution of the command.\n"
                            "SERIOUS! NOTIFY A DEVELOPER!";
            break;
            
            case CLI_ERR_INVCMD:
                p_txt_err = "Unknown command (execution stage)."
                            "\nSERIOUS! NOTIFY A DEVELOPER!";
            break;
            
            case CLI_ERR_INVARG:
                p_txt_err = "Invalid argument of some option.\n"
                            "Use '<command> --help' to get a detailed info (and a list of valid options) for the given command.";
            break;
            
            case CLI_ERR_MISSING_MANDOPT:
                /* NOTE: This error code utilizes a mandopt feature to print extra info (missing opts) */
                p_txt_err = "Command is missing the following mandatory options:";
                do_mandopt_print = true;
            break;
            
            case CLI_ERR_WRONG_IP_TYPE:
                if (CMD_LOGIF_UPDATE == cmd)
                {
                    p_txt_err = "Wrong IP address type (IPv4/IPv6) as an argument of some option. Check the following:\n"
                                "  --> ("  TXT_HELP__SIP  ") and ("  TXT_HELP__DIP  ") accept only IPv4 argument.\n"
                                "  --> ("  TXT_HELP__SIP6  ") and ("  TXT_HELP__DIP6  ") accept only IPv6 argument.\n";
                }
                else
                {
                    p_txt_err = "Wrong IP address type (IPv4/IPv6) as an argument of some option.";
                }
            break;
            
            case CLI_ERR_INCOMPATIBLE_IPS:
                p_txt_err = "Incompatible IP addresses.\n"
                            "All IP addresses must be of a same type - either all IPv4, or all IPv6.";
            break;
            
            /* errors of the libFCI library */
            
            case FPP_ERR_IF_ENTRY_ALREADY_REGISTERED:
                p_txt_err = "Requested interface name is already registered.";
            break;
            
            case FPP_ERR_IF_ENTRY_NOT_FOUND:
                p_txt_err = "Requested target/parent/mirror interface not found.\n"
                            "Is the target/parent/mirror name correct?\n"
                            "Does the target/parent/mirror interface exist?";
            break;
            
            case FPP_ERR_L2_BD_ALREADY_REGISTERED:
                p_txt_err = "Requested bridge domain is already registered.";
            break;
            
            case FPP_ERR_L2_BD_NOT_FOUND:
                p_txt_err = "Requested bridge domain not found.\n"
                            "Is the VLAN ID correct?";
            break;
            
            case FPP_ERR_L2_STATIC_ENT_ALREADY_REGISTERED:
                p_txt_err = "Requested static entry is already registered.";
            break;
            
            case FPP_ERR_L2_STATIC_EN_NOT_FOUND:
                p_txt_err = "Requested static entry not found.\n"
                            "Is the VLAN ID correct?\n"
                            "Is the MAC correct?";
            break;
            
            case FPP_ERR_IF_MATCH_UPDATE_FAILED:
                p_txt_err = "Failed to update logical interface match rules. Maybe incompatible versions of libFCI and driver?";
            break;
            
            case FPP_ERR_RT_ENTRY_ALREADY_REGISTERED:
                p_txt_err = "Requested route is already registered.";
            break;
            
            case FPP_ERR_RT_ENTRY_NOT_FOUND:
                p_txt_err = "Requested route not found.\n"
                            "Is the route ID correct?";
            break;
            
            case FPP_ERR_CT_ENTRY_NOT_FOUND:
                p_txt_err = "Requested conntrack not found.\n"
                            "Are all options filled correctly?";
            break;
            
            case (-2):
                if (CMD_LOGIF_UPDATE == cmd)
                {
                    p_txt_err = "If there was an attempt to set FP_TABLE0 or FP_TABLE1, then no FP table of the given name was found.\n"
                                "If no such attempt was made, then something unexpected happened during execution of the command.";
                }
                else if (CMD_LOGIF_DEL == cmd)
                {
                    p_txt_err = "No interface of the given name was found.";
                }
                else
                {
                    p_txt_err = "Something unexpected happened during execution of the command.\n"
                                "Check your input and try again. If the problem persists, notify a developer.";
                }
            break;
            
            default:
                p_txt_err = "Something unexpected happened during execution of the command.\n"
                            "Check your input and try again. If the problem persists, notify a developer.";
            break;
        }
        cli_print_error(rtn, "%s", p_txt_err);
        
        if (do_mandopt_print)
        {
            cli_mandopt_print("  or  ");
            cli_mandopt_clear();
        }
    }
    
    /* print confirmation message if all OK */
    if (CLI_OK == rtn)
    {
        printf("Command successfully executed.\n");
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS =================================================== */

void cli_print_app_version(void)
{
    printf("app version: "  LIBFCI_CLI_VERSION
           " ("  PFE_CT_H_MD5  " ; "  GLOBAL_VERSION_CONTROL_ID  " ; "  __DATE__ " "  __TIME__  ")"
           " ["  LIBFCI_CLI_TARGET_OS  "]\n");
}

/* NOTE: argument 'p_txt_vec' is expected to follow the argv convention (element [0] exists, but fnc ignores it) */
/* WARNING: argument 'p_txt_vec' must NOT be const, otherwise 'getopt()' fnc family induces UB! */
int cli_parse_and_execute(char* p_txt_vec[], int vec_ln)
{
    int rtn = CLI_ERR;
    cli_cmd_t cmd = CMD_00_NO_COMMAND;
    cli_cmdargs_t cmdargs = {0};
    
    #if !defined(NDEBUG)
    TEST_parser__cmd4exec = CMD_LN;
    #endif
    
    /* always check 'p_txt_vec' and its elements; it can originate outside of this app and thus cannnot be trusted */
    if (0 > vec_ln)
    {
        rtn = CLI_ERR;
        cli_print_error(rtn, "Negative length of the input text vector (vec_ln=%d).", vec_ln);
    }
    else if (NULL == p_txt_vec)
    {
        rtn = CLI_ERR_INVPTR;
        cli_print_error(rtn, "Invalid pointer to the input text vector.");
    }
    else
    {
        /* check input text vector for any potential NULL elements */
        int i = -1;
        while((vec_ln > (++i)) && (NULL != p_txt_vec[i])) { /* empty */ };
        if (vec_ln > i)
        {
            rtn = CLI_ERR_INVPTR;
            cli_print_error(rtn, "Invalid pointer within the input text vector (element [%d]).", i);
        }
        else
        {
            rtn = CLI_OK;  /* greenlight execution of the next sub-block */
        }
    }
    
    /* further inspection is allowed only if input txt vector sufficiently long */
    if (2 <= vec_ln)
    {
        /* cmd */
        if ((CLI_OK == rtn))
        {
            rtn = cmd_parse(&cmd, p_txt_vec[1]);  /* element [1] assumed to be a cmd (usually it is) */
        }
        
        /* cmdargs from opts */
        if ((CLI_OK == rtn))
        {
            /*
                NOTE: Following 'if' statement is a workaround to ignore a cmd element, if it exists.
                      Implemented because some 'getopt()' fnc family implementations
                      wrongly adhere to strict POSIX behavior as the default one.
            */
            if (CMD_00_NO_COMMAND != cmd)
            {
                vec_ln--;
                p_txt_vec++;
            }
            
            rtn = opts_parse(&cmdargs, p_txt_vec, vec_ln);
        }
        
        /* execute */
        if (CLI_OK == rtn)
        {   
            rtn = cmd_execute(cmd, &cmdargs);
            
            #if !defined(NDEBUG)
            TEST_parser__cmd4exec = cmd;
            #endif
        }
    }

    return (rtn);
}

/* ==== TESTMODE constants ================================================= */

#if !defined(NDEBUG)
/* empty */
#endif

/* ========================================================================= */
