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

#ifndef CLI_COMMON_H_
#define CLI_COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"

/* ==== TYPEDEFS & DATA ==================================================== */

#define NDEBUG

/* app version (default values for non-makefile compilation) */
#ifndef LIBFCI_CLI_TARGET_OS
#define LIBFCI_CLI_TARGET_OS  "UNKNOWN_OS"
#endif
#ifndef LIBFCI_CLI_VERSION
#define LIBFCI_CLI_VERSION  "?.?.?"
#endif
#ifndef PFE_CT_H_MD5
#define PFE_CT_H_MD5  "????????????????????????????????"
#endif
#ifndef GLOBAL_VERSION_CONTROL_ID
#define GLOBAL_VERSION_CONTROL_ID  "???????"
#endif

/* return codes */
#define CLI_OK                     (FPP_ERR_OK)  /* Bound to LibFCI OK code for compatibility reasons */
#define CLI_ERR                    (-111)
#define CLI_ERR_INVPTR             (-112)
#define CLI_ERR_INVCMD             (-113)
#define CLI_ERR_INVOPT             (-114)
#define CLI_ERR_INVARG             (-115)
#define CLI_ERR_NONOPT             (-116)
#define CLI_ERR_INCOMPATIBLE_OPTS  (-117)
#define CLI_ERR_MISSING_MANDOPT    (-118)
#define CLI_ERR_INCOMPATIBLE_IPS   (-119)
#define CLI_ERR_WRONG_IP_TYPE      (-120)


/* misc macros */
#define MAC_BYTES_LN         (6u)
#define MAC_STRLEN           (17)
#define IP6_U32S_LN          (4u)
#define IF_NAME_TXT_LN       (IFNAMSIZ)
#define TABLE_NAME_TXT_LN    (16u)
#define FEATURE_NAME_TXT_LN  (FPP_FEATURE_NAME_SIZE + 1)  /* this is how the buffer size is defined in 'fpp.ext.h' */
#define ZPROBS_LN            (8u)  /* Max count of probability zones is based on info from FCI API Reference (chapter about "QoS" feature) */
#define SCH_INS_LN           (8u)  /* Max count of scheduler inputs is based on info from FCI API Reference (chapter about "QoS" feature) */

/* misc macro sanity checks */
#if (MAC_BYTES_LN < 2u)
  #error "MAC_BYTES_LN must be '2' or greater! (why not '6' as usual?)"
#endif
#if (IF_NAME_TXT_LN < 2u)
  #error "IF_NAME_TXT_LN must be '2' or greater!"
#endif
#if (TABLE_NAME_TXT_LN < 2u)
  #error "TABLE_NAME_TXT_LN must be '2' or greater!"
#endif
#if (FEATURE_NAME_TXT_LN < 2u)
  #error "FEATURE_NAME_TXT_LN must be '2' or greater!"
#endif


#define UNUSED(PAR) ((void)(PAR))


/* 'superstructure' - all available arguments for cli commands (usually tied with cli opts) */
typedef struct cli_cmdargs_tt
{
    struct
    {
        bool is_valid;
    } ip4;
    struct
    {
        bool is_valid;
    } ip6;
    struct
    {
        bool is_valid;
    } all;
    struct
    {
        bool is_valid;
    } help;
    struct
    {
        bool is_valid;
    } verbose;
    struct
    {
        bool is_valid;
    } version;
    
    
    struct
    {
        bool is_valid;
        char txt[IF_NAME_TXT_LN];
    } if_name;
    struct
    {
        bool is_valid;
        char txt[IF_NAME_TXT_LN];
    } if_name_parent;
    struct
    {
        bool is_valid;
        char txt[IF_NAME_TXT_LN];  /* empty string ("") indicates rq to disable mirroring */
    } if_name_mirror;
    struct
    {
        bool is_valid;
        fpp_phy_if_op_mode_t value;
    } if_mode;
    struct
    {
        bool is_valid;
        fpp_phy_if_block_state_t value;
    } if_block_state;
    
    
    struct
    {
        bool is_valid;
    } enable_noreply;        /* NOTE: 'OPT_ENABLE' and 'OPT_NO_REPLY' share the same storage */
    struct
    {
        bool is_valid;
    } disable_noorig;        /* NOTE: 'OPT_DISABLE' and 'OPT_NO_ORIG' share the same storage */
    struct
    {
        bool is_valid;
        bool is_on;
    } promisc;
    struct
    {
        bool is_valid;
        bool is_on;
    } loadbalance__ttl_decr; /* NOTE: 'OPT_LOADBALANCE' and 'OPT_TTL_DESCR' share the same storage */
    struct
    {
        bool is_valid;       /* NOTE: 'OPT_VLAN_CONF' and 'OPT_DISCARD_ON_MATCH_SRC' share the same storage */
        bool is_on;
    } vlan_conf__x_src;
    struct
    {
        bool is_valid;
        bool is_on;
    } ptp_conf__x_dst;       /* NOTE: 'OPT_PTP_CONF' and 'OPT_DISCARD_ON_MATCH_DST' share the same storage */
    struct
    {
        bool is_valid;
        bool is_on;
    } ptp_promisc;
    struct
    {
        bool is_valid;
        bool is_on;
    } loopback;
    struct
    {
        bool is_valid;
        bool is_on;
    } qinq;
    struct
    {
        bool is_valid;
        bool is_on;
    } local;
    struct
    {
        bool is_valid;
        bool is_or;
    } match_mode;
    struct
    {
        bool is_valid;
        bool is_on;
    } discard_on_match;
    struct
    {
        bool is_valid;
        bool is_on;
    } discard_if_ttl_below_2;
    struct
    {
        bool is_valid;
        uint32_t bitset;
    } egress;
    struct
    {
        bool is_valid;
        fpp_if_m_rules_t bitset;
    } match_rules;
    
    
    struct
    {
        bool is_valid;
        uint16_t value;
    } vlan;
    struct
    {
        bool is_valid;
        uint16_t value;
    } vlan2;
    struct
    {
        bool is_valid;
        uint8_t value;
    } protocol;
    struct
    {
        bool is_valid;
        uint16_t value;
    } count_ethtype;         /* NOTE: 'OPT_COUNT' and 'OPT_ETHTYPE' share the same storage */
    struct
    {
        bool is_valid;
        uint8_t arr[MAC_BYTES_LN];
    } smac;                  /* NOTE: 'OPT_SMAC' and 'OPT_MAC' share the same storage */
    struct
    {
        bool is_valid;
        uint8_t arr[MAC_BYTES_LN];
    } dmac;
    
    
    struct
    {
        bool is_valid;
        bool is6;
        uint32_t arr[IP6_U32S_LN];
    } sip;
    struct
    {
        bool is_valid;
        bool is6;
        uint32_t arr[IP6_U32S_LN];
    } dip;
    struct
    {
        bool is_valid;
        bool is6;
        uint32_t arr[IP6_U32S_LN];
    } sip2;                  /* NOTE: 'OPT_R_SIP' and 'OPT_SIP6' share the same storage */
    struct
    {
        bool is_valid;
        bool is6;
        uint32_t arr[IP6_U32S_LN];
    } dip2;                  /* NOTE: 'OPT_R_DIP' and 'OPT_DIP6' share the same storage */
    
    
    struct
    {
        bool is_valid;
        uint16_t value;
    } sport;
    struct
    {
        bool is_valid;
        uint16_t value;
    } dport;
    struct
    {
        bool is_valid;
        uint16_t value;
    } sport2;
    struct
    {
        bool is_valid;
        uint16_t value;
    } dport2;
    
    
    struct
    {
        bool is_valid;
        uint8_t value;
    } ucast_hit;
    struct
    {
        bool is_valid;
        uint8_t value;
    } ucast_miss;
    struct
    {
        bool is_valid;
        uint8_t value;
    } mcast_hit;
    struct
    {
        bool is_valid;
        uint8_t value;
    } mcast_miss;
    struct
    {
        bool is_valid;
        bool is_on;
    } tag;
    struct
    {
        bool is_valid;
    } default0;
    struct
    {
        bool is_valid;
    } fallback_4o6;          /* NOTE: 'OPT_FALLBACK' and 'OPT_4o6' share the same storage */
    
    
    struct
    {
        bool is_valid;
        uint32_t value;
    } route;
    struct
    {
        bool is_valid;
        uint32_t value;
    } route2;
    
    
    struct
    {
        bool is_valid;
        char txt[TABLE_NAME_TXT_LN];
    } ruleA0_name;           /* NOTE: 'OPT_INGRESS_MR0' and 'OPT_RULE' share the same storage */
    struct
    {
        bool is_valid;
        char txt[TABLE_NAME_TXT_LN];
    } ruleA1_name;
    struct
    {
        bool is_valid;
        char txt[TABLE_NAME_TXT_LN];
    } ruleB0_name;           /* NOTE: 'OPT_EGRESS_MR0' and 'OPT_NEXT_RULE' share the same storage */
    struct
    {
        bool is_valid;
        char txt[TABLE_NAME_TXT_LN];
    } ruleB1_name;
    struct
    {
        bool is_valid;
        char txt[TABLE_NAME_TXT_LN];
    } table0_name;           /* NOTE: 'OPT_TABLE', 'OPT_TABLE0' and 'OPT_FLEXIBLE_FILTER' share the same storage */
    struct
    {
        bool is_valid;
        char txt[TABLE_NAME_TXT_LN];
    } table1_name;
    
    
    struct
    {
        bool is_valid;
        uint32_t value;
    } timeout;
    struct
    {
        bool is_valid;
        uint32_t value;
    } timeout2;
    struct
    {
        bool is_valid;
        uint32_t value;
    } data_hifc_sad;         /* NOTE: 'OPT_DATA', 'OPT_HIF_COOKIE' and 'OPT_SAD' share the same storage */
    struct
    {
        bool is_valid;
        uint32_t value;
    } mask_spi;              /* NOTE: 'OPT_MASK' and 'OPT_SPI' share the same storage  */
    struct
    {
        bool is_valid;
        fpp_fp_offset_from_t value;
    } layer;
    struct
    {
        bool is_valid;
        uint16_t value;
    } offset;                /* NOTE: 'OPT_OFFSET' and 'OPT_POSITION' share the same storage */
    struct
    {
        bool is_valid;
    } invert;
    struct
    {
        bool is_valid;
    } accept;
    struct
    {
        bool is_valid;
    } reject;
    
    
    struct
    {
        bool is_valid;
        fpp_spd_action_t value;
    } spd_action;
    
    
    struct
    {
        bool is_valid;
        char txt[FEATURE_NAME_TXT_LN];
    } feature_name;
    
    
    struct
    {
        bool is_valid;
    } static0;
    struct
    {
        bool is_valid;
    } dynamic0;
    
    
    struct
    {
        bool is_valid;
        uint8_t value;
    } que_sch_shp;           /* NOTE: 'OPT_QUE', 'OPT_SCH' and 'OPT_SHP' share the same storage */
    struct
    {
        bool is_valid;
        uint8_t value;
    } que_sch_shp_mode;      /* NOTE: 'OPT_QUE_MODE', 'OPT_SCH_MODE' and 'OPT_SHP_MODE' share the same storage */
    
    struct
    {
        bool is_valid;
        uint32_t value;
    } thmin;
    struct
    {
        bool is_valid;
        uint32_t value;
    } thmax;
    struct
    {
        bool is_valid;
        uint8_t arr[ZPROBS_LN];
    } zprob;
    
    struct
    {
        bool is_valid;
        uint8_t value;
    } sch_algo;
    struct sch_in_tt
    {
        bool is_valid;
        uint8_t  arr_src[SCH_INS_LN];
        uint32_t arr_w[SCH_INS_LN];
    } sch_in;
    
    struct
    {
        bool is_valid;
        uint8_t value;
    } shp_pos;
    struct
    {
        bool is_valid;
        uint32_t value;
    } isl;
    struct
    {
        bool is_valid;
        int32_t value;
    } crmin;
    struct
    {
        bool is_valid;
        int32_t value;
    } crmax;
    
} cli_cmdargs_t;

/* ==== PUBLIC FUNCTIONS =================================================== */

extern void cli_print_error(int rtncode, const char* p_txt_err, ...);

/* ========================================================================= */

#endif

