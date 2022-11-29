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

#ifndef CLI_DEF_OPTARG_KEYWORDS_H_
#define CLI_DEF_OPTARG_KEYWORDS_H_

#include <stdint.h>

/* ==== TYPEDEFS & DATA ==================================================== */
/*
    NOTE: Purpose of defining the keywords here (as opposed to simply making them literals within respective arrays)
          is to be able to use them in help texts.
*/

/* indexed by elements of 'fpp_phy_if_op_mode_t' */
#define TXT_IF_MODE__DEFAULT           "DEFAULT"
#define TXT_IF_MODE__ROUTER            "ROUTER"
#define TXT_IF_MODE__VLAN_BRIDGE       "VLAN_BRIDGE"
#define TXT_IF_MODE__FLEXIBLE_ROUTER   "FLEXIBLE_ROUTER"
#define TXT_IF_MODE__L2L3_VLAN_BRIDGE  "L2L3_VLAN_BRIDGE"

/* indexed by elements of 'fpp_phy_if_block_state_t' */
#define TXT_IF_BLOCK_STATE__NORMAL      "NORMAL"
#define TXT_IF_BLOCK_STATE__BLOCKED     "BLOCKED"
#define TXT_IF_BLOCK_STATE__LEARN_ONLY  "LEARN_ONLY"
#define TXT_IF_BLOCK_STATE__FW_ONLY     "FW_ONLY"

/* indexed by common boolean logic ^_^ */
#define TXT_ON_OFF__OFF  "OFF"
#define TXT_ON_OFF__ON   "ON"

/* indexed by common boolean logic ^_^ */
#define TXT_EN_DIS__DISABLED  "DISABLED"
#define TXT_EN_DIS__ENABLED   "ENABLED"

/* indexed by boolean logic of logif bit flag 'MATCH_OR' */
#define TXT_OR_AND__AND  "AND"
#define TXT_OR_AND__OR   "OR"

/* indexed by elements of 'pfe_ct_phy_if_id_t'. */
/* WARNING: these texts should be exactly the same as hardcoded egress names in 'pfe_platform_master.c' */
#define TXT_PHYIF__EMAC0      "emac0"
#define TXT_PHYIF__EMAC1      "emac1"
#define TXT_PHYIF__EMAC2      "emac2"
#define TXT_PHYIF__HIF        "hif"
#define TXT_PHYIF__HIF_NOCPY  "hifncpy"
#define TXT_PHYIF__UTIL       "util"
#define TXT_PHYIF__HIF0       "hif0"
#define TXT_PHYIF__HIF1       "hif1"
#define TXT_PHYIF__HIF2       "hif2"
#define TXT_PHYIF__HIF3       "hif3"

/* based on element order of 'fpp_if_m_rules_t' */
/* WARNING: elements of 'fpp_if_m_rules_t' are bitmasks, and thus CANNOT directly index this array */
#define TXT_MATCH_RULE__TYPE_ETH       "TYPE_ETH"
#define TXT_MATCH_RULE__TYPE_VLAN      "TYPE_VLAN"
#define TXT_MATCH_RULE__TYPE_PPPOE     "TYPE_PPPOE"
#define TXT_MATCH_RULE__TYPE_ARP       "TYPE_ARP"
#define TXT_MATCH_RULE__TYPE_MCAST     "TYPE_MCAST"
#define TXT_MATCH_RULE__TYPE_IP4       "TYPE_IP4"
#define TXT_MATCH_RULE__TYPE_IP6       "TYPE_IP6"
#define TXT_MATCH_RULE__XXX_RES7_XXX   "__XXX_res7_XXX__"
#define TXT_MATCH_RULE__XXX_RES8_XXX   "__XXX_res8_XXX__"
#define TXT_MATCH_RULE__TYPE_IPX       "TYPE_IPX"
#define TXT_MATCH_RULE__TYPE_BCAST     "TYPE_BCAST"
#define TXT_MATCH_RULE__TYPE_UDP       "TYPE_UDP"
#define TXT_MATCH_RULE__TYPE_TCP       "TYPE_TCP"
#define TXT_MATCH_RULE__TYPE_ICMP      "TYPE_ICMP"
#define TXT_MATCH_RULE__TYPE_IGMP      "TYPE_IGMP"
#define TXT_MATCH_RULE__VLAN           "VLAN"
#define TXT_MATCH_RULE__PROTOCOL       "PROTOCOL"
#define TXT_MATCH_RULE__XXX_RES17_XXX  "__XXX_res17_XXX__"
#define TXT_MATCH_RULE__XXX_RES18_XXX  "__XXX_res18_XXX__"
#define TXT_MATCH_RULE__XXX_RES19_XXX  "__XXX_res19_XXX__"
#define TXT_MATCH_RULE__SPORT          "SPORT"
#define TXT_MATCH_RULE__DPORT          "DPORT"
#define TXT_MATCH_RULE__SIP6           "SIP6"
#define TXT_MATCH_RULE__DIP6           "DIP6"
#define TXT_MATCH_RULE__SIP            "SIP"
#define TXT_MATCH_RULE__DIP            "DIP"
#define TXT_MATCH_RULE__ETHER_TYPE     "ETHER_TYPE"
#define TXT_MATCH_RULE__FP_TABLE0      "FP_TABLE0"
#define TXT_MATCH_RULE__FP_TABLE1      "FP_TABLE1"
#define TXT_MATCH_RULE__SMAC           "SMAC"
#define TXT_MATCH_RULE__DMAC           "DMAC"
#define TXT_MATCH_RULE__HIF_COOKIE     "HIF_COOKIE"

/* indexed by IANA "Assigned Internet Protocol Number" elements */
/* https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml */
#define TXT_PROTOCOL__HOPOPT           "HOPOPT"
#define TXT_PROTOCOL__ICMP             "ICMP"
#define TXT_PROTOCOL__IGMP             "IGMP"
#define TXT_PROTOCOL__GGP              "GGP"
#define TXT_PROTOCOL__IPv4             "IPv4"
#define TXT_PROTOCOL__ST               "ST"
#define TXT_PROTOCOL__TCP              "TCP"
#define TXT_PROTOCOL__CBT              "CBT"
#define TXT_PROTOCOL__EGP              "EGP"
#define TXT_PROTOCOL__IGP              "IGP"
#define TXT_PROTOCOL__BBN_RCC_MON      "BBN-RCC-MON"
#define TXT_PROTOCOL__NVP_II           "NVP-II"
#define TXT_PROTOCOL__PUP              "PUP"
#define TXT_PROTOCOL__ARGUS            "ARGUS"
#define TXT_PROTOCOL__EMCON            "EMCON"
#define TXT_PROTOCOL__XNET             "XNET"
#define TXT_PROTOCOL__CHAOS            "CHAOS"
#define TXT_PROTOCOL__UDP              "UDP"
#define TXT_PROTOCOL__MUX              "MUX"
#define TXT_PROTOCOL__DCN_MEAS         "DCN-MEAS"
#define TXT_PROTOCOL__HMP              "HMP"
#define TXT_PROTOCOL__PRM              "PRM"
#define TXT_PROTOCOL__XNS_IDP          "XNS-IDP"
#define TXT_PROTOCOL__TRUNK_1          "TRUNK-1"
#define TXT_PROTOCOL__TRUNK_2          "TRUNK-2"
#define TXT_PROTOCOL__LEAF_1           "LEAF-1"
#define TXT_PROTOCOL__LEAF_2           "LEAF-2"
#define TXT_PROTOCOL__RDP              "RDP"
#define TXT_PROTOCOL__IRTP             "IRTP"
#define TXT_PROTOCOL__ISO_TP4          "ISO-TP4"
#define TXT_PROTOCOL__NETBLT           "NETBLT"
#define TXT_PROTOCOL__MFE_NSP          "MFE-NSP"
#define TXT_PROTOCOL__MERIT_INP        "MERIT-INP"
#define TXT_PROTOCOL__DCCP             "DCCP"
#define TXT_PROTOCOL__3PC              "3PC"
#define TXT_PROTOCOL__IDPR             "IDPR"
#define TXT_PROTOCOL__XTP              "XTP"
#define TXT_PROTOCOL__DDP              "DDP"
#define TXT_PROTOCOL__IDPR_CMTP        "IDPR-CMTP"
#define TXT_PROTOCOL__TP_PLUSPLUS      "TP++"
#define TXT_PROTOCOL__IL               "IL"
#define TXT_PROTOCOL__IPv6             "IPv6"
#define TXT_PROTOCOL__SDRP             "SDRP"
#define TXT_PROTOCOL__IPv6_Route       "IPv6-Route"
#define TXT_PROTOCOL__IPv6_Frag        "IPv6-Frag"
#define TXT_PROTOCOL__IDRP             "IDRP"
#define TXT_PROTOCOL__RSVP             "RSVP"
#define TXT_PROTOCOL__GRE              "GRE"
#define TXT_PROTOCOL__DSR              "DSR"
#define TXT_PROTOCOL__BNA              "BNA"
#define TXT_PROTOCOL__ESP              "ESP"
#define TXT_PROTOCOL__AH               "AH"
#define TXT_PROTOCOL__I_NLSP           "I-NLSP"
#define TXT_PROTOCOL__SWIPE            "SWIPE"
#define TXT_PROTOCOL__NARP             "NARP"
#define TXT_PROTOCOL__MOBILE           "MOBILE"
#define TXT_PROTOCOL__TLSP             "TLSP"
#define TXT_PROTOCOL__SKIP             "SKIP"
#define TXT_PROTOCOL__IPv6_ICMP        "IPv6-ICMP"
#define TXT_PROTOCOL__IPv6_NoNxt       "IPv6-NoNxt"
#define TXT_PROTOCOL__IPv6_Opts        "IPv6-Opts"
#define TXT_PROTOCOL__CFTP             "CFTP"
#define TXT_PROTOCOL__SAT_EXPAK        "SAT-EXPAK"
#define TXT_PROTOCOL__KRYPTOLAN        "KRYPTOLAN"
#define TXT_PROTOCOL__RVD              "RVD"
#define TXT_PROTOCOL__IPPC             "IPPC"
#define TXT_PROTOCOL__SAT_MON          "SAT-MON"
#define TXT_PROTOCOL__VISA             "VISA"
#define TXT_PROTOCOL__IPCV             "IPCV"
#define TXT_PROTOCOL__CPNX             "CPNX"
#define TXT_PROTOCOL__CPHB             "CPHB"
#define TXT_PROTOCOL__WSN              "WSN"
#define TXT_PROTOCOL__PVP              "PVP"
#define TXT_PROTOCOL__BR_SAT_MON       "BR-SAT-MON"
#define TXT_PROTOCOL__SUN_ND           "SUN-ND"
#define TXT_PROTOCOL__WB_MON           "WB-MON"
#define TXT_PROTOCOL__WB_EXPAK         "WB-EXPAK"
#define TXT_PROTOCOL__ISO_IP           "ISO-IP"
#define TXT_PROTOCOL__VMTP             "VMTP"
#define TXT_PROTOCOL__SECURE_VMTP      "SECURE-VMTP"
#define TXT_PROTOCOL__VINES            "VINES"
#define TXT_PROTOCOL__IPTM             "IPTM"
#define TXT_PROTOCOL__NSFNET_IGP       "NSFNET-IGP"
#define TXT_PROTOCOL__DGP              "DGP"
#define TXT_PROTOCOL__TCF              "TCF"
#define TXT_PROTOCOL__EIGRP            "EIGRP"
#define TXT_PROTOCOL__OSPFIGP          "OSPFIGP"
#define TXT_PROTOCOL__Sprite_RPC       "Sprite-RPC"
#define TXT_PROTOCOL__LARP             "LARP"
#define TXT_PROTOCOL__MTP              "MTP"
#define TXT_PROTOCOL__AX_25            "AX.25"
#define TXT_PROTOCOL__IPIP             "IPIP"
#define TXT_PROTOCOL__MICP             "MICP"
#define TXT_PROTOCOL__SCC_SP           "SCC-SP"
#define TXT_PROTOCOL__ETHERIP          "ETHERIP"
#define TXT_PROTOCOL__ENCAP            "ENCAP"
#define TXT_PROTOCOL__GMTP             "GMTP"
#define TXT_PROTOCOL__IFMP             "IFMP"
#define TXT_PROTOCOL__PNNI             "PNNI"
#define TXT_PROTOCOL__PIM              "PIM"
#define TXT_PROTOCOL__ARIS             "ARIS"
#define TXT_PROTOCOL__SCPS             "SCPS"
#define TXT_PROTOCOL__QNX              "QNX"
#define TXT_PROTOCOL__AN               "A/N"
#define TXT_PROTOCOL__IPComp           "IPComp"
#define TXT_PROTOCOL__SNP              "SNP"
#define TXT_PROTOCOL__Compaq_Peer      "Compaq-Peer"
#define TXT_PROTOCOL__IPX_in_IP        "IPX-in-IP"
#define TXT_PROTOCOL__VRRP             "VRRP"
#define TXT_PROTOCOL__PGM              "PGM"
#define TXT_PROTOCOL__L2TP             "L2TP"
#define TXT_PROTOCOL__DDX              "DDX"
#define TXT_PROTOCOL__IATP             "IATP"
#define TXT_PROTOCOL__STP              "STP"
#define TXT_PROTOCOL__SRP              "SRP"
#define TXT_PROTOCOL__UTI              "UTI"
#define TXT_PROTOCOL__SMP              "SMP"
#define TXT_PROTOCOL__SM               "SM"
#define TXT_PROTOCOL__PTP              "PTP"
#define TXT_PROTOCOL__ISIS_over_IPv4   "ISIS_over_IPv4"
#define TXT_PROTOCOL__FIRE             "FIRE"
#define TXT_PROTOCOL__CRTP             "CRTP"
#define TXT_PROTOCOL__CRUDP            "CRUDP"
#define TXT_PROTOCOL__SSCOPMCE         "SSCOPMCE"
#define TXT_PROTOCOL__IPLT             "IPLT"
#define TXT_PROTOCOL__SPS              "SPS"
#define TXT_PROTOCOL__PIPE             "PIPE"
#define TXT_PROTOCOL__SCTP             "SCTP"
#define TXT_PROTOCOL__FC               "FC"
#define TXT_PROTOCOL__RSVP_E2E_IGNORE  "RSVP-E2E-IGNORE"
#define TXT_PROTOCOL__Mobility_Header  "Mobility_Header"
#define TXT_PROTOCOL__UDPLite          "UDPLite"
#define TXT_PROTOCOL__MPLS_in_IP       "MPLS-in-IP"
#define TXT_PROTOCOL__manet            "manet"
#define TXT_PROTOCOL__HIP              "HIP"
#define TXT_PROTOCOL__Shim6            "Shim6"
#define TXT_PROTOCOL__WESP             "WESP"
#define TXT_PROTOCOL__ROHC             "ROHC"
#define TXT_PROTOCOL__Ethernet         "Ethernet"

/* indexed by elements of 'fpp_fp_offset_from_t' */
#define TXT_OFFSET_FROM__XXX_RES0_XXX  "__XXX_res0_XXX__"
#define TXT_OFFSET_FROM__XXX_RES1_XXX  "__XXX_res1_XXX__"
#define TXT_OFFSET_FROM__L2            "L2"
#define TXT_OFFSET_FROM__L3            "L3"
#define TXT_OFFSET_FROM__L4            "L4"
extern const uint8_t OFFSET_FROMS__MIN;
extern const uint8_t OFFSET_FROMS__MAX;

/* indexed by elements of 'fpp_fp_match_action_t' */
#define TXT_MATCH_ACTION__ACCEPT     "ACCEPT"
#define TXT_MATCH_ACTION__REJECT     "REJECT"
#define TXT_MATCH_ACTION__NEXT_RULE  "NEXT_RULE"
extern const uint8_t MATCH_ACTIONS__MAX;

/* indexed by values of bridge actions (see doxygen for 'fpp_l2_bd_cmd_t.ucast_hit') */
#define TXT_BD_ACTION__FORWARD  "FORWARD"
#define TXT_BD_ACTION__FLOOD    "FLOOD"
#define TXT_BD_ACTION__PUNT     "PUNT"
#define TXT_BD_ACTION__DISCARD  "DISCARD"
extern const uint8_t BD_ACTIONS__MAX;

/* indexed by elements of 'fpp_spd_action_t' */
#define TXT_SPD_ACTION__XXX_RES0_XXX  "__XXX_res0_XXX__"
#define TXT_SPD_ACTION__DISCARD       "DISCARD"
#define TXT_SPD_ACTION__BYPASS        "BYPASS"
#define TXT_SPD_ACTION__ENCODE        "ENCODE"
#define TXT_SPD_ACTION__DECODE        "DECODE"
extern const uint8_t SPD_ACTIONS__MAX;

/* indexed by queue mode IDs (see doxygen for 'fpp_qos_queue_cmd_t.mode') */
#define TXT_QUE_MODE__DISABLED   "DISABLED"
#define TXT_QUE_MODE__DEFAULT    "DEFAULT"
#define TXT_QUE_MODE__TAIL_DROP  "TAIL_DROP"
#define TXT_QUE_MODE__WRED       "WRED"

/* special value for que_zprob (que_zprob is mainly numeric, but there is a special textual input option "KEEP" (meaning "do not change that particular value") */
#define TXT_QUE_ZPROB__KEEP      "K"

/* indexed by scheduler mode IDs (see doxygen for 'fpp_qos_scheduler_cmd_t.mode') */
#define TXT_SCH_MODE__DISABLED     "DISABLED"
#define TXT_SCH_MODE__DATA_RATE    "DATA_RATE"
#define TXT_SCH_MODE__PACKET_RATE  "PACKET_RATE"

/* indexed by scheduler algorithm IDs (see doxygen for 'fpp_qos_scheduler_cmd_t.algo') */
#define TXT_SCH_ALGO__PQ    "PQ"
#define TXT_SCH_ALGO__DWRR  "DWRR"
#define TXT_SCH_ALGO__RR    "RR"
#define TXT_SCH_ALGO__WRR   "WRR"

/* indexed by scheduler input IDs (see doxygen for 'fpp_qos_scheduler_cmd_t.input' and 'fpp_qos_scheduler_cmd_t.input_src') */
#define TXT_SCH_IN__QUE0      "que0"
#define TXT_SCH_IN__QUE1      "que1"
#define TXT_SCH_IN__QUE2      "que2"
#define TXT_SCH_IN__QUE3      "que3"
#define TXT_SCH_IN__QUE4      "que4"
#define TXT_SCH_IN__QUE5      "que5"
#define TXT_SCH_IN__QUE6      "que6"
#define TXT_SCH_IN__QUE7      "que7"
#define TXT_SCH_IN__SCH0_OUT  "sch0_out"
    /* special elements; these are a part of input IDs, but are NOT indexed by input IDs */
#define TXT_SCH_IN__DISABLED  "D"
#define TXT_SCH_IN__KEEP      "K"

/* indexed by shaper mode IDs (see doxygen for 'fpp_qos_shaper_cmd_t.mode') */
#define TXT_SHP_MODE__DISABLED     "DISABLED"
#define TXT_SHP_MODE__DATA_RATE    "DATA_RATE"
#define TXT_SHP_MODE__PACKET_RATE  "PACKET_RATE"

/* indexed by shaper position IDs (see doxygen for 'fpp_qos_shaper_cmd_t.position') */
#define TXT_SHP_POS__SCH1_OUT  "sch1_out"
#define TXT_SHP_POS__SCH1_IN0  "sch1_in0"
#define TXT_SHP_POS__SCH1_IN1  "sch1_in1"
#define TXT_SHP_POS__SCH1_IN2  "sch1_in2"
#define TXT_SHP_POS__SCH1_IN3  "sch1_in3"
#define TXT_SHP_POS__SCH1_IN4  "sch1_in4"
#define TXT_SHP_POS__SCH1_IN5  "sch1_in5"
#define TXT_SHP_POS__SCH1_IN6  "sch1_in6"
#define TXT_SHP_POS__SCH1_IN7  "sch1_in7"
#define TXT_SHP_POS__SCH0_IN0  "sch0_in0"
#define TXT_SHP_POS__SCH0_IN1  "sch0_in1"
#define TXT_SHP_POS__SCH0_IN2  "sch0_in2"
#define TXT_SHP_POS__SCH0_IN3  "sch0_in3"
#define TXT_SHP_POS__SCH0_IN4  "sch0_in4"
#define TXT_SHP_POS__SCH0_IN5  "sch0_in5"
#define TXT_SHP_POS__SCH0_IN6  "sch0_in6"
#define TXT_SHP_POS__SCH0_IN7  "sch0_in7"
    /* special elements; these are a part of shaper position IDs, but are NOT indexed by shaper position IDs */
#define TXT_SHP_POS__DISABLED  "DISABLED"

/* based on element order of 'fpp_modify_actions_t' */
/* WARNING: elements of 'fpp_modify_actions_t' are bitmasks, and thus CANNOT directly index this array */
#define TXT_MODIFY_ACTION__XXX_RES0_XXX  "__XXX_res0_XXX__"
#define TXT_MODIFY_ACTION__ADD_VLAN_HDR  "ADD_VLAN_HDR"

/* indexed by Ingress QoS WRED queue (see doxygen for 'fpp_iqos_queue_t') */
#define TXT_POL_WRED_QUE__DMEM  "DMEM"
#define TXT_POL_WRED_QUE__LMEM  "LMEM"
#define TXT_POL_WRED_QUE__RXF   "RXF"

/* indexed by Ingress QoS shaper type (see doxygen for 'fpp_iqos_shp_type_t') */
#define TXT_POL_SHP_TYPE__PORT   "PORT"
#define TXT_POL_SHP_TYPE__BCAST  "BCAST"
#define TXT_POL_SHP_TYPE__MCAST  "MCAST"

/* indexed by Ingress QoS shaper rate mode (see doxygen for 'fpp_iqos_shp_rate_mode_t') */
#define TXT_POL_SHP_MODE__DATA    TXT_SHP_MODE__DATA_RATE
#define TXT_POL_SHP_MODE__PACKET  TXT_SHP_MODE__PACKET_RATE

/* indexed by Ingress QoS flow actions (see doxygen for 'fpp_iqos_flow_action_t') */
#define TXT_POL_FLOW_ACTION__MANAGED    "MANAGED"
#define TXT_POL_FLOW_ACTION__DROP       "DROP"
#define TXT_POL_FLOW_ACTION__RESERVED   "RESERVED"

/* based on element order of 'fpp_iqos_flow_type_t' */
/* WARNING: elements of 'fpp_iqos_flow_type_t' are bitmasks, and thus CANNOT directly index this array */
#define TXT_POL_FLOW_TYPE1__TYPE_ETH    TXT_MATCH_RULE__TYPE_ETH
#define TXT_POL_FLOW_TYPE1__TYPE_PPPOE  TXT_MATCH_RULE__TYPE_PPPOE
#define TXT_POL_FLOW_TYPE1__TYPE_ARP    TXT_MATCH_RULE__TYPE_ARP
#define TXT_POL_FLOW_TYPE1__TYPE_IP4    TXT_MATCH_RULE__TYPE_IP4
#define TXT_POL_FLOW_TYPE1__TYPE_IP6    TXT_MATCH_RULE__TYPE_IP6
#define TXT_POL_FLOW_TYPE1__TYPE_IPX    TXT_MATCH_RULE__TYPE_IPX
#define TXT_POL_FLOW_TYPE1__TYPE_MCAST  TXT_MATCH_RULE__TYPE_MCAST
#define TXT_POL_FLOW_TYPE1__TYPE_BCAST  TXT_MATCH_RULE__TYPE_BCAST
#define TXT_POL_FLOW_TYPE1__TYPE_VLAN   TXT_MATCH_RULE__TYPE_VLAN

/* based on element order of 'fpp_iqos_flow_arg_type_t' */
/* WARNING: elements of 'fpp_iqos_flow_arg_type_t' are bitmasks, and thus CANNOT directly index this array */
#define TXT_POL_FLOW_TYPE2__VLAN        TXT_MATCH_RULE__VLAN
#define TXT_POL_FLOW_TYPE2__TOS         "TOS"
#define TXT_POL_FLOW_TYPE2__PROTOCOL    TXT_MATCH_RULE__PROTOCOL
#define TXT_POL_FLOW_TYPE2__SIP         TXT_MATCH_RULE__SIP
#define TXT_POL_FLOW_TYPE2__DIP         TXT_MATCH_RULE__DIP
#define TXT_POL_FLOW_TYPE2__SPORT       TXT_MATCH_RULE__SPORT
#define TXT_POL_FLOW_TYPE2__DPORT       TXT_MATCH_RULE__DPORT

/* based on element order of 'fpp_fw_feature_element_type_t' */
/* WARNING: elements of 'fpp_fw_feature_element_type_t' are bitmasks, and thus CANNOT directly index this array */
#define TXT_FWFEAT_EL_GROUP__DEFAULT    "DEFAULT"
#define TXT_FWFEAT_EL_GROUP__CONFIG     "CONFIG"
#define TXT_FWFEAT_EL_GROUP__STATS      "STATS"

/* based on IDs of Health Monitor event types from PFE */
#define TXT_HM_TYPE__INFO     "INFO"
#define TXT_HM_TYPE__WARNING  "WARNING"
#define TXT_HM_TYPE__ERROR    "ERROR"

/* based on IDs of Health Monitor event sources from PFE */
#define TXT_HM_SRC__UNKNOWN         "UNKNOWN"
#define TXT_HM_SRC__WDT             "WDT"
#define TXT_HM_SRC__EMAC0           "EMAC0"
#define TXT_HM_SRC__EMAC1           "EMAC1"
#define TXT_HM_SRC__EMAC2           "EMAC2"
#define TXT_HM_SRC__BUS             "BUS"
#define TXT_HM_SRC__PARITY          "PARITY"
#define TXT_HM_SRC__FAIL_STOP       "FAIL_STOP"
#define TXT_HM_SRC__FW_FAIL_STOP    "FW_FAIL_STOP"
#define TXT_HM_SRC__HOST_FAIL_STOP  "HOST_FAIL_STOP"
#define TXT_HM_SRC__ECC             "ECC"
#define TXT_HM_SRC__PE_CLASS        "PE_CLASS"
#define TXT_HM_SRC__PE_UTIL         "PE_UTIL"
#define TXT_HM_SRC__PE_TMU          "PE_TMU"
#define TXT_HM_SRC__HIF             "HIF"
#define TXT_HM_SRC__BMU             "BMU"

/* ==== PUBLIC FUNCTIONS =================================================== */

const char* cli_value2txt_if_mode(uint8_t value);
int         cli_txt2value_if_mode(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_if_block_state(uint8_t value);
int         cli_txt2value_if_block_state(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_on_off(uint8_t value);
int         cli_txt2value_on_off(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_en_dis(uint8_t value);
int         cli_txt2value_en_dis(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_or_and(uint8_t value);
int         cli_txt2value_or_and(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_phyif(uint8_t value);
int         cli_txt2value_phyif(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_match_rule(uint8_t value);
int         cli_txt2value_match_rule(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_protocol(uint8_t value);
int         cli_txt2value_protocol(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_offset_from(uint8_t value);
int         cli_txt2value_offset_from(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_match_action(uint8_t value);
int         cli_txt2value_match_action(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_bd_action(uint8_t value);
int         cli_txt2value_bd_action(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_spd_action(uint8_t value);
int         cli_txt2value_spd_action(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_que_mode(uint8_t value);
int         cli_txt2value_que_mode(uint8_t* p_rtn_value, const char* p_txt);

bool        cli_que_zprob_is_not_keep(uint8_t value);
int         cli_txt2value_que_zprob_keep(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_sch_mode(uint8_t value);
int         cli_txt2value_sch_mode(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_sch_algo(uint8_t value);
int         cli_txt2value_sch_algo(uint8_t* p_rtn_value, const char* p_txt);

bool        cli_sch_in_is_not_dis(uint8_t value);
bool        cli_sch_in_is_not_keep(uint8_t value);
const char* cli_value2txt_sch_in(uint8_t value);
int         cli_txt2value_sch_in(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_shp_mode(uint8_t value);
int         cli_txt2value_shp_mode(uint8_t* p_rtn_value, const char* p_txt);

bool        cli_shp_pos_is_not_dis(uint8_t value);
const char* cli_value2txt_shp_pos(uint8_t value);
int         cli_txt2value_shp_pos(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_modify_action(uint8_t value);
int         cli_txt2value_modify_action(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_pol_wred_que(uint8_t value);
int         cli_txt2value_pol_wred_que(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_pol_shp_type(uint8_t value);
int         cli_txt2value_pol_shp_type(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_pol_shp_mode(uint8_t value);
int         cli_txt2value_pol_shp_mode(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_pol_flow_action(uint8_t value);
int         cli_txt2value_pol_flow_action(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_pol_flow_type1(uint8_t value);
int         cli_txt2value_pol_flow_type1(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_pol_flow_type2(uint8_t value);
int         cli_txt2value_pol_flow_type2(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_pol_flow_type32(uint8_t value);
int         cli_txt2value_pol_flow_type32(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_fwfeat_el_group(uint8_t value);
int         cli_txt2value_fwfeat_el_group(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_hm_type(uint8_t value);
int         cli_txt2value_hm_type(uint8_t* p_rtn_value, const char* p_txt);

const char* cli_value2txt_hm_src(uint8_t value);
int         cli_txt2value_hm_src(uint8_t* p_rtn_value, const char* p_txt);

/* ========================================================================= */

#endif
