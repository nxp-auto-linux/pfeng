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
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "libfci_cli_common.h"
#include "libfci_cli_def_optarg_keywords.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

#define TXT_INVALID_ITEM  "__INVALID_ITEM__"
#define CALC_LN(ARRAY)    (sizeof(ARRAY) / sizeof(const char*))

/* indexed by elements of 'fpp_phy_if_op_mode_t' */
static const char *const txt_if_modes[] = 
{
    TXT_IF_MODE__DEFAULT,
    TXT_IF_MODE__VLAN_BRIDGE,
    TXT_IF_MODE__ROUTER,
    TXT_IF_MODE__FLEXIBLE_ROUTER,
    TXT_IF_MODE__L2L3_VLAN_BRIDGE
};
#define IF_MODES_LN  CALC_LN(txt_if_modes)

/* indexed by elements of 'fpp_phy_if_block_state_t' */
static const char *const txt_if_block_states[] = 
{
    TXT_IF_BLOCK_STATE__NORMAL,
    TXT_IF_BLOCK_STATE__BLOCKED,
    TXT_IF_BLOCK_STATE__LEARN_ONLY,
    TXT_IF_BLOCK_STATE__FW_ONLY
};
#define IF_BLOCK_STATES_LN  CALC_LN(txt_if_block_states)

/* indexed by common boolean logic ^_^ */
static const char *const txt_on_offs[] = 
{
    TXT_ON_OFF__OFF,
    TXT_ON_OFF__ON
};
#define ON_OFFS_LN  CALC_LN(txt_on_offs)

/* indexed by common boolean logic ^_^ */
static const char *const txt_en_dises[] = 
{
    TXT_EN_DIS__DISABLED,
    TXT_EN_DIS__ENABLED
};
#define EN_DISES_LN  CALC_LN(txt_en_dises)

/* indexed by boolean logic of logif bit flag 'MATCH_OR' */
static const char *const txt_or_ands[] = 
{
    TXT_OR_AND__AND,
    TXT_OR_AND__OR
};
#define OR_ANDS_LN  CALC_LN(txt_or_ands)

/* indexed by elements of 'pfe_ct_phy_if_id_t'. */
/* WARNING: these texts should be exactly the same as hardcoded egress names in 'pfe_platform_master.c' */
static const char *const txt_phyifs[] = 
{
    TXT_PHYIF__EMAC0,
    TXT_PHYIF__EMAC1,
    TXT_PHYIF__EMAC2,
    TXT_PHYIF__HIF,
    TXT_PHYIF__HIF_NOCPY,
    TXT_PHYIF__UTIL,
    TXT_PHYIF__HIF0,
    TXT_PHYIF__HIF1,
    TXT_PHYIF__HIF2,
    TXT_PHYIF__HIF3
};
#define PHYIFS_LN  CALC_LN(txt_phyifs)

/* based on element order of 'fpp_if_m_rules_t' */
/* WARNING: elements of 'fpp_if_m_rules_t' are bitmasks, and thus CANNOT directly index this array */
static const char *const txt_match_rules[] = 
{
    TXT_MATCH_RULE__TYPE_ETH,
    TXT_MATCH_RULE__TYPE_VLAN,
    TXT_MATCH_RULE__TYPE_PPPOE,
    TXT_MATCH_RULE__TYPE_ARP,
    TXT_MATCH_RULE__TYPE_MCAST,
    TXT_MATCH_RULE__TYPE_IP4,
    TXT_MATCH_RULE__TYPE_IP6,
    TXT_MATCH_RULE__XXX_RES7_XXX,
    TXT_MATCH_RULE__XXX_RES8_XXX,
    TXT_MATCH_RULE__TYPE_IPX,
    TXT_MATCH_RULE__TYPE_BCAST,
    TXT_MATCH_RULE__TYPE_UDP,
    TXT_MATCH_RULE__TYPE_TCP,
    TXT_MATCH_RULE__TYPE_ICMP,
    TXT_MATCH_RULE__TYPE_IGMP,
    TXT_MATCH_RULE__VLAN,
    TXT_MATCH_RULE__PROTOCOL,
    TXT_MATCH_RULE__XXX_RES17_XXX,
    TXT_MATCH_RULE__XXX_RES18_XXX,
    TXT_MATCH_RULE__XXX_RES19_XXX,
    TXT_MATCH_RULE__SPORT,
    TXT_MATCH_RULE__DPORT,
    TXT_MATCH_RULE__SIP6,
    TXT_MATCH_RULE__DIP6,
    TXT_MATCH_RULE__SIP,
    TXT_MATCH_RULE__DIP,
    TXT_MATCH_RULE__ETHER_TYPE,
    TXT_MATCH_RULE__FP_TABLE0,  /* FP0 */
    TXT_MATCH_RULE__FP_TABLE1,  /* FP1 */
    TXT_MATCH_RULE__SMAC,
    TXT_MATCH_RULE__DMAC,
    TXT_MATCH_RULE__HIF_COOKIE
};
#define MATCH_RULES_LN  CALC_LN(txt_match_rules)

/* indexed by IANA "Assigned Internet Protocol Number" elements */
/* https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml */
static const char txt_protocol_ahip[] = "'any host internal protocol'";
static const char txt_protocol_aln[] = "'any local network'";
static const char txt_protocol_adfs[] = "'any distributed file system'";
static const char txt_protocol_apes[] = "'any private encryption scheme'";
static const char txt_protocol_azhp[] = "'any zero hop protocol'";
static const char txt_protocol_unass[] = "UNASSIGNED by IANA";
static const char txt_protocol_tests[] = "EXPERIMENTS and TESTING range";
static const char txt_protocol_res[] = "RESERVED by IANA";
static const char *const txt_protocols[] = 
{
    TXT_PROTOCOL__HOPOPT,      TXT_PROTOCOL__ICMP,        TXT_PROTOCOL__IGMP,        TXT_PROTOCOL__GGP,        TXT_PROTOCOL__IPv4,
    TXT_PROTOCOL__ST,          TXT_PROTOCOL__TCP,         TXT_PROTOCOL__CBT,         TXT_PROTOCOL__EGP,        TXT_PROTOCOL__IGP,
    TXT_PROTOCOL__BBN_RCC_MON, TXT_PROTOCOL__NVP_II,      TXT_PROTOCOL__PUP,         TXT_PROTOCOL__ARGUS,      TXT_PROTOCOL__EMCON,
    TXT_PROTOCOL__XNET,        TXT_PROTOCOL__CHAOS,       TXT_PROTOCOL__UDP,         TXT_PROTOCOL__MUX,        TXT_PROTOCOL__DCN_MEAS,
    TXT_PROTOCOL__HMP,         TXT_PROTOCOL__PRM,         TXT_PROTOCOL__XNS_IDP,     TXT_PROTOCOL__TRUNK_1,    TXT_PROTOCOL__TRUNK_2,
    TXT_PROTOCOL__LEAF_1,      TXT_PROTOCOL__LEAF_2,      TXT_PROTOCOL__RDP,         TXT_PROTOCOL__IRTP,       TXT_PROTOCOL__ISO_TP4,
    TXT_PROTOCOL__NETBLT,      TXT_PROTOCOL__MFE_NSP,     TXT_PROTOCOL__MERIT_INP,   TXT_PROTOCOL__DCCP,       TXT_PROTOCOL__3PC,
    TXT_PROTOCOL__IDPR,        TXT_PROTOCOL__XTP,         TXT_PROTOCOL__DDP,         TXT_PROTOCOL__IDPR_CMTP,  TXT_PROTOCOL__TP_PLUSPLUS,
    TXT_PROTOCOL__IL,          TXT_PROTOCOL__IPv6,        TXT_PROTOCOL__SDRP,        TXT_PROTOCOL__IPv6_Route, TXT_PROTOCOL__IPv6_Frag,
    TXT_PROTOCOL__IDRP,        TXT_PROTOCOL__RSVP,        TXT_PROTOCOL__GRE,         TXT_PROTOCOL__DSR,        TXT_PROTOCOL__BNA,

    TXT_PROTOCOL__ESP,         TXT_PROTOCOL__AH,          TXT_PROTOCOL__I_NLSP,      TXT_PROTOCOL__SWIPE,      TXT_PROTOCOL__NARP,
    TXT_PROTOCOL__MOBILE,      TXT_PROTOCOL__TLSP,        TXT_PROTOCOL__SKIP,        TXT_PROTOCOL__IPv6_ICMP,  TXT_PROTOCOL__IPv6_NoNxt,
    TXT_PROTOCOL__IPv6_Opts,   txt_protocol_ahip,         TXT_PROTOCOL__CFTP,        txt_protocol_aln,         TXT_PROTOCOL__SAT_EXPAK,
    TXT_PROTOCOL__KRYPTOLAN,   TXT_PROTOCOL__RVD,         TXT_PROTOCOL__IPPC,        txt_protocol_adfs,        TXT_PROTOCOL__SAT_MON,
    TXT_PROTOCOL__VISA,        TXT_PROTOCOL__IPCV,        TXT_PROTOCOL__CPNX,        TXT_PROTOCOL__CPHB,       TXT_PROTOCOL__WSN,
    TXT_PROTOCOL__PVP,         TXT_PROTOCOL__BR_SAT_MON,  TXT_PROTOCOL__SUN_ND,      TXT_PROTOCOL__WB_MON,     TXT_PROTOCOL__WB_EXPAK,
    TXT_PROTOCOL__ISO_IP,      TXT_PROTOCOL__VMTP,        TXT_PROTOCOL__SECURE_VMTP, TXT_PROTOCOL__VINES,      TXT_PROTOCOL__IPTM,
    TXT_PROTOCOL__NSFNET_IGP,  TXT_PROTOCOL__DGP,         TXT_PROTOCOL__TCF,         TXT_PROTOCOL__EIGRP,      TXT_PROTOCOL__OSPFIGP,
    TXT_PROTOCOL__Sprite_RPC,  TXT_PROTOCOL__LARP,        TXT_PROTOCOL__MTP,         TXT_PROTOCOL__AX_25,      TXT_PROTOCOL__IPIP,
    TXT_PROTOCOL__MICP,        TXT_PROTOCOL__SCC_SP,      TXT_PROTOCOL__ETHERIP,     TXT_PROTOCOL__ENCAP,      txt_protocol_apes,

    TXT_PROTOCOL__GMTP,        TXT_PROTOCOL__IFMP,        TXT_PROTOCOL__PNNI,        TXT_PROTOCOL__PIM,        TXT_PROTOCOL__ARIS,
    TXT_PROTOCOL__SCPS,        TXT_PROTOCOL__QNX,         TXT_PROTOCOL__AN,          TXT_PROTOCOL__IPComp,     TXT_PROTOCOL__SNP,
    TXT_PROTOCOL__Compaq_Peer, TXT_PROTOCOL__IPX_in_IP,   TXT_PROTOCOL__VRRP,        TXT_PROTOCOL__PGM,        txt_protocol_azhp,
    TXT_PROTOCOL__L2TP,        TXT_PROTOCOL__DDX,         TXT_PROTOCOL__IATP,        TXT_PROTOCOL__STP,        TXT_PROTOCOL__SRP,
    TXT_PROTOCOL__UTI,         TXT_PROTOCOL__SMP,         TXT_PROTOCOL__SM,          TXT_PROTOCOL__PTP,        TXT_PROTOCOL__ISIS_over_IPv4,
    TXT_PROTOCOL__FIRE,        TXT_PROTOCOL__CRTP,        TXT_PROTOCOL__CRUDP,       TXT_PROTOCOL__SSCOPMCE,   TXT_PROTOCOL__IPLT,
    TXT_PROTOCOL__SPS,         TXT_PROTOCOL__PIPE,        TXT_PROTOCOL__SCTP,        TXT_PROTOCOL__FC,         TXT_PROTOCOL__RSVP_E2E_IGNORE,
    TXT_PROTOCOL__Mobility_Header, TXT_PROTOCOL__UDPLite, TXT_PROTOCOL__MPLS_in_IP,  TXT_PROTOCOL__manet,      TXT_PROTOCOL__HIP,
    TXT_PROTOCOL__Shim6,       TXT_PROTOCOL__WESP,        TXT_PROTOCOL__ROHC,        TXT_PROTOCOL__Ethernet
};
#define PROTOCOLS_LN  CALC_LN(txt_protocols)

/* indexed by elements of 'fpp_fp_offset_from_t' */
static const char *const txt_offset_froms[] = 
{
    TXT_OFFSET_FROM__XXX_RES0_XXX,
    TXT_OFFSET_FROM__XXX_RES1_XXX,
    TXT_OFFSET_FROM__L2,
    TXT_OFFSET_FROM__L3,
    TXT_OFFSET_FROM__L4
};
#define OFFSET_FROMS_LN  CALC_LN(txt_offset_froms)
const uint8_t OFFSET_FROMS__MIN = (2u);
const uint8_t OFFSET_FROMS__MAX = (OFFSET_FROMS_LN - 1u);

/* indexed by elements of 'fpp_match_action_t' */
static const char *const txt_match_actions[] = 
{
    TXT_MATCH_ACTION__ACCEPT,
    TXT_MATCH_ACTION__REJECT,
    TXT_MATCH_ACTION__NEXT_RULE
};
#define MATCH_ACTIONS_LN  CALC_LN(txt_match_actions)
const uint8_t MATCH_ACTIONS__MAX = (MATCH_ACTIONS_LN - 1u);

/* indexed by values of bridge actions (see doxygen for 'fpp_l2_bd_cmd_t.ucast_hit') */
static const char *const txt_bd_actions[] = 
{
    TXT_BD_ACTION__FORWARD,
    TXT_BD_ACTION__FLOOD,
    TXT_BD_ACTION__PUNT,
    TXT_BD_ACTION__DISCARD
};
#define BD_ACTIONS_LN  CALC_LN(txt_bd_actions)
const uint8_t BD_ACTIONS__MAX = (BD_ACTIONS_LN - 1u);

/* indexed by elements of 'fpp_spd_action_t' */
static const char *const txt_spd_actions[] = 
{
    TXT_SPD_ACTION__XXX_RES0_XXX,
    TXT_SPD_ACTION__DISCARD,
    TXT_SPD_ACTION__BYPASS,
    TXT_SPD_ACTION__ENCODE,
    TXT_SPD_ACTION__DECODE
};
#define SPD_ACTIONS_LN  CALC_LN(txt_spd_actions)
const uint8_t SPD_ACTIONS__MAX = (SPD_ACTIONS_LN - 1u);

/* indexed by queue mode IDs (see doxygen for 'fpp_qos_queue_cmd_t.mode') */
static const char *const txt_que_modes[] = 
{
    TXT_QUE_MODE__DISABLED,
    TXT_QUE_MODE__DEFAULT,
    TXT_QUE_MODE__TAIL_DROP,
    TXT_QUE_MODE__WRED
};
#define QUE_MODES_LN  CALC_LN(txt_que_modes)

/* indexed by scheduler mode IDs (see doxygen for 'fpp_qos_scheduler_cmd_t.mode') */
static const char *const txt_sch_modes[] = 
{
    TXT_SCH_MODE__DISABLED,
    TXT_SCH_MODE__DATA_RATE,
    TXT_SCH_MODE__PACKET_RATE
};
#define SCH_MODES_LN  CALC_LN(txt_sch_modes)

/* indexed by scheduler algorithm IDs (see doxygen for 'fpp_qos_scheduler_cmd_t.algo') */
static const char *const txt_sch_algos[] = 
{
    TXT_SCH_ALGO__PQ,
    TXT_SCH_ALGO__DWRR,
    TXT_SCH_ALGO__RR,
    TXT_SCH_ALGO__WRR
};
#define SCH_ALGOS_LN  CALC_LN(txt_sch_algos)

/* indexed by scheduler input IDs (see doxygen for 'fpp_qos_scheduler_cmd_t.input' and 'fpp_qos_scheduler_cmd_t.input_src') */
static const char *const txt_sch_ins[] = 
{
    TXT_SCH_IN__QUE0,
    TXT_SCH_IN__QUE1,
    TXT_SCH_IN__QUE2,
    TXT_SCH_IN__QUE3,
    TXT_SCH_IN__QUE4,
    TXT_SCH_IN__QUE5,
    TXT_SCH_IN__QUE6,
    TXT_SCH_IN__QUE7,
    TXT_SCH_IN__SCH0_OUT
};
#define SCH_INS_LN_  CALC_LN(txt_sch_ins)

/* indexed by shaper mode IDs (see doxygen for 'fpp_qos_shaper_cmd_t.mode') */
static const char *const txt_shp_modes[] = 
{
    TXT_SHP_MODE__DISABLED,
    TXT_SHP_MODE__DATA_RATE,
    TXT_SHP_MODE__PACKET_RATE
};
#define SHP_MODES_LN  CALC_LN(txt_shp_modes)

/* indexed by shaper position IDs (see doxygen for 'fpp_qos_shaper_cmd_t.position') */
static const char *const txt_shp_pos[] = 
{
    TXT_SHP_POS__SCH1_OUT,
    TXT_SHP_POS__SCH1_IN0,
    TXT_SHP_POS__SCH1_IN1,
    TXT_SHP_POS__SCH1_IN2,
    TXT_SHP_POS__SCH1_IN3,
    TXT_SHP_POS__SCH1_IN4,
    TXT_SHP_POS__SCH1_IN5,
    TXT_SHP_POS__SCH1_IN6,
    TXT_SHP_POS__SCH1_IN7,
    TXT_SHP_POS__SCH0_IN0,
    TXT_SHP_POS__SCH0_IN1,
    TXT_SHP_POS__SCH0_IN2,
    TXT_SHP_POS__SCH0_IN3,
    TXT_SHP_POS__SCH0_IN4,
    TXT_SHP_POS__SCH0_IN5,
    TXT_SHP_POS__SCH0_IN6,
    TXT_SHP_POS__SCH0_IN7
};
#define SHP_POS_LN  CALC_LN(txt_shp_pos) 

/* based on element order of 'fpp_modify_actions_t' */
/* WARNING: elements of 'fpp_modify_actions_t' are bitmasks, and thus CANNOT directly index this array */
static const char *const txt_modify_actions[] = 
{
    TXT_MODIFY_ACTION__XXX_RES0_XXX,
    TXT_MODIFY_ACTION__ADD_VLAN_HDR
};
#define MODIFY_ACTIONS_LN  CALC_LN(txt_modify_actions)

/* indexed by Ingress QoS WRED queue type (see doxygen for 'fpp_iqos_queue_t') */
static const char *const txt_pol_wred_ques[] = 
{
    TXT_POL_WRED_QUE__DMEM,
    TXT_POL_WRED_QUE__LMEM,
    TXT_POL_WRED_QUE__RXF
};
#define POL_WRED_QUES_LN  CALC_LN(txt_pol_wred_ques) 

/* indexed by Ingress QoS shaper type (see doxygen for 'fpp_iqos_shp_type_t') */
static const char *const txt_pol_shp_types[] = 
{
    TXT_POL_SHP_TYPE__PORT,
    TXT_POL_SHP_TYPE__BCAST,
    TXT_POL_SHP_TYPE__MCAST
};
#define POL_SHP_TYPES_LN  CALC_LN(txt_pol_shp_types) 

/* indexed by Ingress QoS shaper rate mode (see doxygen for 'fpp_iqos_shp_rate_mode_t') */
static const char *const txt_pol_shp_modes[] = 
{
    TXT_POL_SHP_MODE__DATA,
    TXT_POL_SHP_MODE__PACKET
};
#define POL_SHP_MODES_LN  CALC_LN(txt_pol_shp_modes) 

/* indexed by Ingress QoS flow actions (see doxygen for 'fpp_iqos_flow_action_t') */
static const char *const txt_pol_flow_actions[] = 
{
    TXT_POL_FLOW_ACTION__MANAGED,
    TXT_POL_FLOW_ACTION__DROP,
    TXT_POL_FLOW_ACTION__RESERVED
};
#define POL_FLOW_ACTIONS_LN  CALC_LN(txt_pol_flow_actions) 

/* based on element order of 'fpp_iqos_flow_type_t' */
/* WARNING: elements of 'fpp_iqos_flow_type_t' are bitmasks, and thus CANNOT directly index this array */
static const char *const txt_pol_flow_types1[] = 
{
    TXT_POL_FLOW_TYPE1__TYPE_ETH,
    TXT_POL_FLOW_TYPE1__TYPE_PPPOE,
    TXT_POL_FLOW_TYPE1__TYPE_ARP,
    TXT_POL_FLOW_TYPE1__TYPE_IP4,
    TXT_POL_FLOW_TYPE1__TYPE_IP6,
    TXT_POL_FLOW_TYPE1__TYPE_IPX,
    TXT_POL_FLOW_TYPE1__TYPE_MCAST,
    TXT_POL_FLOW_TYPE1__TYPE_BCAST,
    TXT_POL_FLOW_TYPE1__TYPE_VLAN
};
#define POL_FLOW_TYPES1_LN  CALC_LN(txt_pol_flow_types1) 

/* based on element order of 'fpp_iqos_flow_arg_type_t' */
/* WARNING: elements of 'fpp_iqos_flow_arg_type_t' are bitmasks, and thus CANNOT directly index this array */
static const char *const txt_pol_flow_types2[] = 
{
    TXT_POL_FLOW_TYPE2__VLAN,
    TXT_POL_FLOW_TYPE2__TOS,
    TXT_POL_FLOW_TYPE2__PROTOCOL,
    TXT_POL_FLOW_TYPE2__SIP,
    TXT_POL_FLOW_TYPE2__DIP,
    TXT_POL_FLOW_TYPE2__SPORT,
    TXT_POL_FLOW_TYPE2__DPORT
};
#define POL_FLOW_TYPES2_LN  CALC_LN(txt_pol_flow_types2) 

/* array for 32bit bitmask, merging both 'fpp_iqos_flow_type_t' and 'fpp_iqos_flow_arg_type_t' into one bitset */
/* HACK: 'fpp_iqos_flow_type_t' maps to lower 16 bits ; 'fpp_iqos_flow_arg_type_t' maps to higher 16 bits */
static const char *const txt_pol_flow_types32[] = 
{
    /* fpp_iqos_flow_type_t */
    TXT_POL_FLOW_TYPE1__TYPE_ETH,
    TXT_POL_FLOW_TYPE1__TYPE_PPPOE,
    TXT_POL_FLOW_TYPE1__TYPE_ARP,
    TXT_POL_FLOW_TYPE1__TYPE_IP4,
    TXT_POL_FLOW_TYPE1__TYPE_IP6,
    TXT_POL_FLOW_TYPE1__TYPE_IPX,
    TXT_POL_FLOW_TYPE1__TYPE_MCAST,
    TXT_POL_FLOW_TYPE1__TYPE_BCAST,
    TXT_POL_FLOW_TYPE1__TYPE_VLAN,
    "__XXX_res09_XXX__",
    "__XXX_res10_XXX__",
    "__XXX_res11_XXX__",
    "__XXX_res12_XXX__",
    "__XXX_res13_XXX__",
    "__XXX_res14_XXX__",
    "__XXX_res15_XXX__",
    
    /* fpp_iqos_flow_arg_type_t */
    TXT_POL_FLOW_TYPE2__VLAN,
    TXT_POL_FLOW_TYPE2__TOS,
    TXT_POL_FLOW_TYPE2__PROTOCOL,
    TXT_POL_FLOW_TYPE2__SIP,
    TXT_POL_FLOW_TYPE2__DIP,
    TXT_POL_FLOW_TYPE2__SPORT,
    TXT_POL_FLOW_TYPE2__DPORT,
    "__XXX_res23_XXX__",
    "__XXX_res24_XXX__",
    "__XXX_res25_XXX__",
    "__XXX_res26_XXX__",
    "__XXX_res27_XXX__",
    "__XXX_res28_XXX__",
    "__XXX_res29_XXX__",
    "__XXX_res30_XXX__",
    "__XXX_res31_XXX__"
};
#define POL_FLOW_TYPES32_LN  CALC_LN(txt_pol_flow_types32) 

/* based on element order of 'fpp_fw_feature_element_type_t' */
/* WARNING: elements of 'fpp_fw_feature_element_type_t' are bitmasks, and thus CANNOT directly index this array */
static const char *const txt_fwfeat_el_group[] = 
{
    TXT_FWFEAT_EL_GROUP__DEFAULT,
    TXT_FWFEAT_EL_GROUP__CONFIG,
    TXT_FWFEAT_EL_GROUP__STATS
};
#define FWFEAT_EL_GROUP_LN  CALC_LN(txt_fwfeat_el_group) 

/* based on IDs of Health Monitor event types from PFE */
static const char *const txt_hm_types[] = 
{
    TXT_HM_TYPE__INFO,
    TXT_HM_TYPE__WARNING,
    TXT_HM_TYPE__ERROR
};
#define HM_TYPES_LN  CALC_LN(txt_hm_types) 

/* based on IDs of Health Monitor event sources from PFE */
static const char *const txt_hm_srcs[] = 
{
    TXT_HM_SRC__UNKNOWN,
    TXT_HM_SRC__WDT,
    TXT_HM_SRC__EMAC0,
    TXT_HM_SRC__EMAC1,
    TXT_HM_SRC__EMAC2,
    TXT_HM_SRC__BUS,
    TXT_HM_SRC__PARITY,
    TXT_HM_SRC__FAIL_STOP,
    TXT_HM_SRC__FW_FAIL_STOP,
    TXT_HM_SRC__HOST_FAIL_STOP,
    TXT_HM_SRC__ECC,
    TXT_HM_SRC__PE_CLASS,
    TXT_HM_SRC__PE_UTIL,
    TXT_HM_SRC__PE_TMU,
    TXT_HM_SRC__HIF,
    TXT_HM_SRC__BMU
};
#define HM_SRCS_LN  CALC_LN(txt_hm_srcs)

/* ==== PRIVATE FUNCTIONS ================================================== */

static int txt2value(uint8_t *p_rtn_value, const char *p_txt,
                     const char *const p_txt_keywords[], const uint8_t keywords_ln, const uint8_t min)
{
    assert(NULL != p_rtn_value);
    assert(NULL != p_txt);
    assert(NULL != p_txt_keywords);
    /* 'p_rtn_opt_is_valid' is allowed to be NULL */
    
    int rtn = CLI_ERR;
    
    uint8_t i = UINT8_MAX;  /* WARNING: intentional use of owf behavior */ 
    while ((keywords_ln > (++i)) && (strcmp(p_txt_keywords[i], p_txt))) { /*  empty  */ };
    if ((keywords_ln <= i) || (min > i))
    {
        rtn = CLI_ERR_INVARG;
    }
    else
    {
        *p_rtn_value = i;
        rtn = CLI_OK;
    }
    
    return (rtn);
}

/* ==== PUBLIC FUNCTIONS =================================================== */

const char* cli_value2txt_if_mode(uint8_t value)
{
    return ((IF_MODES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_if_modes[value]));
}
int cli_txt2value_if_mode(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_if_modes, IF_MODES_LN, 0u);
}


const char* cli_value2txt_if_block_state(uint8_t value)
{
    return ((IF_BLOCK_STATES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_if_block_states[value]));
}
int cli_txt2value_if_block_state(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_if_block_states, IF_BLOCK_STATES_LN, 0u);
}


const char* cli_value2txt_on_off(uint8_t value)
{
    return ((ON_OFFS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_on_offs[value]));
}
int cli_txt2value_on_off(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_on_offs, ON_OFFS_LN, 0u);
}


const char* cli_value2txt_en_dis(uint8_t value)
{
    return ((EN_DISES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_en_dises[value]));
}
int cli_txt2value_en_dis(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_en_dises, EN_DISES_LN, 0u);
}


const char* cli_value2txt_or_and(uint8_t value)
{
    return ((OR_ANDS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_or_ands[value]));
}
int cli_txt2value_or_and(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_or_ands, OR_ANDS_LN, 0u);
}


const char* cli_value2txt_phyif(uint8_t value)
{
    return ((PHYIFS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_phyifs[value]));
}
int cli_txt2value_phyif(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_phyifs, PHYIFS_LN, 0u);
}


const char* cli_value2txt_match_rule(uint8_t value)
{
    return ((MATCH_RULES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_match_rules[value]));
}
int cli_txt2value_match_rule(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_match_rules, MATCH_RULES_LN, 0u);
}


const char* cli_value2txt_protocol(uint8_t value)
{
    return ((PROTOCOLS_LN > value) ? (txt_protocols[value]) :
           ((252u >= value) ? (txt_protocol_unass) : 
           ((254u >= value) ? (txt_protocol_tests) :
           ((255u == value) ? (txt_protocol_res) : (TXT_INVALID_ITEM)))));
}
int cli_txt2value_protocol(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_protocols, PROTOCOLS_LN, 0u);
}


const char* cli_value2txt_offset_from(uint8_t value)
{
    return ((OFFSET_FROMS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_offset_froms[value]));
}
int cli_txt2value_offset_from(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_offset_froms, OFFSET_FROMS_LN, FP_OFFSET_FROM_L2_HEADER);
}


const char* cli_value2txt_match_action(uint8_t value)
{
    return ((MATCH_ACTIONS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_match_actions[value]));
}
int cli_txt2value_match_action(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_match_actions, MATCH_ACTIONS_LN, 0u);
}


const char* cli_value2txt_bd_action(uint8_t value)
{
    return ((BD_ACTIONS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_bd_actions[value]));
}
int cli_txt2value_bd_action(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_bd_actions, BD_ACTIONS_LN, 0u);
}


const char* cli_value2txt_spd_action(uint8_t value)
{
    return ((SPD_ACTIONS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_spd_actions[value]));
}
int cli_txt2value_spd_action(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_spd_actions, SPD_ACTIONS_LN, 0u);
}


const char* cli_value2txt_que_mode(uint8_t value)
{
    return ((QUE_MODES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_que_modes[value]));
}
int cli_txt2value_que_mode(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_que_modes, QUE_MODES_LN, 0u);
}


#define VAL_QUE_ZPROB__KEEP  (200u)
bool cli_que_zprob_is_not_keep(uint8_t value)
{
    return (VAL_QUE_ZPROB__KEEP != value);
}
int cli_txt2value_que_zprob_keep(uint8_t* p_rtn_value, const char* p_txt)
{
    assert(NULL != p_rtn_value);
    assert(NULL != p_txt);
    
    
    int rtn = CLI_ERR_INVARG;
    if (0 == strcmp(TXT_QUE_ZPROB__KEEP, p_txt))
    {
        *p_rtn_value = VAL_QUE_ZPROB__KEEP;
        rtn = CLI_OK;
    }
    
    return (rtn);
}


const char* cli_value2txt_sch_mode(uint8_t value)
{
    return ((SCH_MODES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_sch_modes[value]));
}
int cli_txt2value_sch_mode(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_sch_modes, SCH_MODES_LN, 0u);
}


const char* cli_value2txt_sch_algo(uint8_t value)
{
    return ((SCH_ALGOS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_sch_algos[value]));
}
int cli_txt2value_sch_algo(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_sch_algos, SCH_ALGOS_LN, 0u);
}


#define VAL_SCH_IN__DISABLED  (255u)
#define VAL_SCH_IN__KEEP      (200u)
bool cli_sch_in_is_not_dis(uint8_t value)
{
    return (VAL_SCH_IN__DISABLED != value);
}
bool cli_sch_in_is_not_keep(uint8_t value)
{
    return (VAL_SCH_IN__KEEP != value);
}
const char* cli_value2txt_sch_in(uint8_t value)
{
    return ((SCH_INS_LN_ > value) ? (txt_sch_ins[value]) :
           ((VAL_SCH_IN__DISABLED == value) ? (TXT_SCH_IN__DISABLED) : (TXT_INVALID_ITEM)));
    /* 'KEEP' element is cli-internal only (is not defined in libFCI) - no need to print it */
}
int cli_txt2value_sch_in(uint8_t* p_rtn_value, const char* p_txt)
{
    assert(NULL != p_rtn_value);
    assert(NULL != p_txt);
    
    
    int rtn = CLI_ERR;
    if (0 == strcmp(TXT_SCH_IN__DISABLED, p_txt))
    {
        *p_rtn_value = VAL_SCH_IN__DISABLED;
        rtn = CLI_OK;
    }
    else if (0 == strcmp(TXT_SCH_IN__KEEP, p_txt))  /* 'KEEP' element is cli-internal only (is not defined in libFCI) */
    {
        *p_rtn_value = VAL_SCH_IN__KEEP;
        rtn = CLI_OK;
    }
    else
    {
        rtn = txt2value(p_rtn_value, p_txt, txt_sch_ins, SCH_INS_LN_, 0u);
    }
    
    return (rtn);
}


const char* cli_value2txt_shp_mode(uint8_t value)
{
    return ((SHP_MODES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_shp_modes[value]));
}
int cli_txt2value_shp_mode(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_shp_modes, SHP_MODES_LN, 0u);
}


#define VAL_SHP_POS__DISABLED  (255u)
bool cli_shp_pos_is_not_dis(uint8_t value)
{
    return (VAL_SHP_POS__DISABLED != value);
}
const char* cli_value2txt_shp_pos(uint8_t value)
{
    return ((SHP_POS_LN > value) ? (txt_shp_pos[value]) :
           ((VAL_SHP_POS__DISABLED == value) ? (TXT_SHP_POS__DISABLED) : (TXT_INVALID_ITEM)));
}
int cli_txt2value_shp_pos(uint8_t* p_rtn_value, const char* p_txt)
{
    assert(NULL != p_rtn_value);
    assert(NULL != p_txt);
    
    
    int rtn = CLI_ERR;
    if (0 == strcmp(TXT_SHP_POS__DISABLED, p_txt))
    {
        *p_rtn_value = VAL_SHP_POS__DISABLED;
        rtn = CLI_OK;
    }
    else
    {
        rtn = txt2value(p_rtn_value, p_txt, txt_shp_pos, SHP_POS_LN, 0u);
    }
    
    return (rtn);
}


const char* cli_value2txt_modify_action(uint8_t value)
{
    return ((MODIFY_ACTIONS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_modify_actions[value]));
}
int cli_txt2value_modify_action(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_modify_actions, MODIFY_ACTIONS_LN, 0u);
}


const char* cli_value2txt_pol_wred_que(uint8_t value)
{
    return ((POL_WRED_QUES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_pol_wred_ques[value]));
}
int cli_txt2value_pol_wred_que(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_pol_wred_ques, POL_WRED_QUES_LN, 0u);
}


const char* cli_value2txt_pol_shp_type(uint8_t value)
{
    return ((POL_SHP_TYPES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_pol_shp_types[value]));
}
int cli_txt2value_pol_shp_type(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_pol_shp_types, POL_SHP_TYPES_LN, 0u);
}


const char* cli_value2txt_pol_shp_mode(uint8_t value)
{
    return ((POL_SHP_MODES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_pol_shp_modes[value]));
}
int cli_txt2value_pol_shp_mode(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_pol_shp_modes, POL_SHP_MODES_LN, 0u);
}


const char* cli_value2txt_pol_flow_action(uint8_t value)
{
    return ((POL_FLOW_ACTIONS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_pol_flow_actions[value]));
}
int cli_txt2value_pol_flow_action(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_pol_flow_actions, POL_FLOW_ACTIONS_LN, 0u);
}


const char* cli_value2txt_pol_flow_type1(uint8_t value)
{
    return ((POL_FLOW_TYPES1_LN <= value) ? (TXT_INVALID_ITEM) : (txt_pol_flow_types1[value]));
}
int cli_txt2value_pol_flow_type1(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_pol_flow_types1, POL_FLOW_TYPES1_LN, 0u);
}


const char* cli_value2txt_pol_flow_type2(uint8_t value)
{
    return ((POL_FLOW_TYPES2_LN <= value) ? (TXT_INVALID_ITEM) : (txt_pol_flow_types2[value]));
}
int cli_txt2value_pol_flow_type2(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_pol_flow_types2, POL_FLOW_TYPES2_LN, 0u);
}


const char* cli_value2txt_pol_flow_type32(uint8_t value)
{
    return ((POL_FLOW_TYPES32_LN <= value) ? (TXT_INVALID_ITEM) : (txt_pol_flow_types32[value]));
}
int cli_txt2value_pol_flow_type32(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_pol_flow_types32, POL_FLOW_TYPES32_LN, 0u);
}


const char* cli_value2txt_fwfeat_el_group(uint8_t value)
{
    return ((FWFEAT_EL_GROUP_LN <= value) ? (TXT_INVALID_ITEM) : (txt_fwfeat_el_group[value]));
}
int cli_txt2value_fwfeat_el_group(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_fwfeat_el_group, FWFEAT_EL_GROUP_LN, 0u);
}


const char* cli_value2txt_hm_type(uint8_t value)
{
    return ((HM_TYPES_LN <= value) ? (TXT_INVALID_ITEM) : (txt_hm_types[value]));
}
int cli_txt2value_hm_type(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_hm_types, HM_TYPES_LN, 0u);
}


const char* cli_value2txt_hm_src(uint8_t value)
{
    return ((HM_SRCS_LN <= value) ? (TXT_INVALID_ITEM) : (txt_hm_srcs[value]));
}
int cli_txt2value_hm_src(uint8_t* p_rtn_value, const char* p_txt)
{
    return txt2value(p_rtn_value, p_txt, txt_hm_srcs, HM_SRCS_LN, 0u);
}


/* ==== TESTMODE constants ================================================= */

#if !defined(NDEBUG)
const uint8_t TEST_defkws__if_modes_ln         = IF_MODES_LN;
const uint8_t TEST_defkws__if_block_states_ln  = IF_BLOCK_STATES_LN;
const uint8_t TEST_defkws__on_offs_ln          = ON_OFFS_LN;
const uint8_t TEST_defkws__en_dises_ln         = EN_DISES_LN;
const uint8_t TEST_defkws__or_ands_ln          = OR_ANDS_LN;
const uint8_t TEST_defkws__phyifs_ln           = PHYIFS_LN;
const uint8_t TEST_defkws__match_rules_ln      = MATCH_RULES_LN;
const uint8_t TEST_defkws__protocols_ln        = PROTOCOLS_LN;
const uint8_t TEST_defkws__offset_froms_ln     = OFFSET_FROMS_LN;
const uint8_t TEST_defkws__match_actions_ln    = MATCH_ACTIONS_LN;
const uint8_t TEST_defkws__bd_actions_ln       = BD_ACTIONS_LN;
const uint8_t TEST_defkws__spd_actions_ln      = SPD_ACTIONS_LN;
const uint8_t TEST_defkws__que_modes_ln        = QUE_MODES_LN;
const uint8_t TEST_defkws__sch_modes_ln        = SCH_MODES_LN;
const uint8_t TEST_defkws__sch_algos_ln        = SCH_ALGOS_LN;
const uint8_t TEST_defkws__sch_ins_ln          = SCH_INS_LN_;
const uint8_t TEST_defkws__shp_modes_ln        = SHP_MODES_LN;
const uint8_t TEST_defkws__shp_pos_ln          = SHP_POS_LN;
const uint8_t TEST_defkws__modify_actions_ln   = MODIFY_ACTIONS_LN;
const uint8_t TEST_defkws__pol_wred_ques_ln    = POL_WRED_QUES_LN;
const uint8_t TEST_defkws__pol_shp_types_ln    = POL_SHP_TYPES_LN;
const uint8_t TEST_defkws__pol_shp_modes_ln    = POL_SHP_MODES_LN;
const uint8_t TEST_defkws__pol_flow_actions_ln = POL_FLOW_ACTIONS_LN;
const uint8_t TEST_defkws__pol_flow_types1_ln  = POL_FLOW_TYPES1_LN;
const uint8_t TEST_defkws__pol_flow_types2_ln  = POL_FLOW_TYPES2_LN;
const uint8_t TEST_defkws__pol_flow_types32_ln = POL_FLOW_TYPES32_LN;

const uint8_t TEST_defkws__hm_types_ln         = HM_TYPES_LN;
const uint8_t TEST_defkws__hm_srcs_ln          = HM_SRCS_LN;
#endif

/* ========================================================================= */
