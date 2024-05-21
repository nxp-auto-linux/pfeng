/* =========================================================================
 *  Copyright 2020-2022,2024 NXP
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

#include <stdint.h>
#include <stdio.h>

#include "libfci_cli_common.h"
#include "libfci_cli_def_cmds.h"
#include "libfci_cli_def_opts.h"
#include "libfci_cli_def_optarg_keywords.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA : aux symbols ====================================== */

#define TXT_DECOR_CMD    "---- Command -------------------\n"
#define TXT_DECOR_DESCR  "---- Description ---------------\n"
#define TXT_DECOR_OPT    "---- Options -------------------\n"


#define TXT_PHYIF_KEYWORDS  \
        "  Only the following hardcoded PHYIF names are accepted:\n"  \
        "    "  TXT_PHYIF__EMAC0  "\n"  \
        "    "  TXT_PHYIF__EMAC1  "\n"  \
        "    "  TXT_PHYIF__EMAC2  "\n"  \
        "    "  TXT_PHYIF__UTIL   "\n"  \
        "    "  TXT_PHYIF__HIF_NOCPY  "  (valid only if supported by a driver)"  "\n"  \
        "    "  TXT_PHYIF__HIF0   "\n"  \
        "    "  TXT_PHYIF__HIF1   "\n"  \
        "    "  TXT_PHYIF__HIF2   "\n"  \
        "    "  TXT_PHYIF__HIF3   "\n"  \

#define TXT_PHYIF_KEYWORDS_EMAC  \
        "  Only the following hardcoded PHYIF names are accepted:\n"  \
        "    "  TXT_PHYIF__EMAC0  "\n"  \
        "    "  TXT_PHYIF__EMAC1  "\n"  \
        "    "  TXT_PHYIF__EMAC2  "\n"  \

#define TXT_PHYIF_KEYWORDS_HIF  \
        "    "  TXT_PHYIF__HIF0  "\n"  \
        "    "  TXT_PHYIF__HIF1  "\n"  \
        "    "  TXT_PHYIF__HIF2  "\n"  \
        "    "  TXT_PHYIF__HIF2  "\n"  \

#define TXT_EQOS_QUEUE_RANGES  \
        "    "   TXT_PHYIF__HIF0 "," TXT_PHYIF__HIF1 "," TXT_PHYIF__HIF2 "," TXT_PHYIF__HIF3 ": 0-31  \n"  \
        "    "  "emacX/" TXT_PHYIF__UTIL "/" TXT_PHYIF__HIF_NOCPY ":  0-255  \n"  \

#define TXT_IQOS_WRED_RANGES  \
        "    "  TXT_POL_WRED_QUE__DMEM  ": 0-8192  \n"  \
        "    "  TXT_POL_WRED_QUE__LMEM  ": 0-512   \n"  \
        "    "  TXT_POL_WRED_QUE__RXF   ":  0-512  \n"  \

#define TXT_OPTARGS__PHYIF         TXT_PHYIF__EMAC2
#define TXT_OPTARGS__ON_OFF        TXT_ON_OFF__ON  "|"  TXT_ON_OFF__OFF
#define TXT_OPTARGS__MAC_ADDR      "00:22:bc:45:de:67|35-47-ed-6c-28-b0"
#define TXT_OPTARGS__BD_ACTIONS    TXT_BD_ACTION__PUNT  "|0-3"
#define TXT_OPTARGS__MIRROR        "MyMirrorRule"
#define TXT_OPTARGS__FP_TABLE      "MyFpTable"
#define TXT_OPTARGS__FP_RULE       "MyFpRule"
#define TXT_OPTARGS__U8_DEC        "0-255"
#define TXT_OPTARGS__U8_HEX        "0x00-0xFF"
#define TXT_OPTARGS__U16_DEC       "0-65535"
#define TXT_OPTARGS__U16_HEX       "0x00-0xFFFF"
#define TXT_OPTARGS__U32_DEC       "0-4294967295"
#define TXT_OPTARGS__U32_HEX       "0x00-0xFFFFFFFF"
#define TXT_OPTARGS__I32_DEC       "-2147483648 - 2147483647"


/* ==== TYPEDEFS & DATA : opt binds and descriptions ======================= */
/*
    Binding local symbols to help texts from 'def_opts.h' (and providing descriptions).
    
    Search for keyword 'OPT_LAST' to get to the bottom of the opt help text list.
    Search for keyword 'CMD_LAST' to get to the bottom of the cmd help text list.
    
    
    Description of symbols
    ----------------------
    TXT_OPT__xx         TXT_HELP__aa=<...>  Symbol representing the given cli opt in help texts (and example arguments if applicable).
                                            'TXT_HELP__aa' is expected to be a cli opt help text symbol from 'def_opts.h'.
                                            
    TXT_OPTDESCR__xx    TXT_OPT__xx \ ...   Verbose description of the given cli opt.
    
*/

#define TXT_OPT__IP4                    TXT_HELP__IP4
#define TXT_OPTDESCR__IP4               TXT_HELP__IP4  "\n"  \
                                        "  IPv4 variant of the operation.\n"

#define TXT_OPT__IP6                    TXT_HELP__IP6
#define TXT_OPTDESCR__IP6               TXT_HELP__IP6  "\n"  \
                                        "  IPv6 variant of the operation.\n"

#define TXT_OPT__ALL                    TXT_HELP__ALL
#define TXT_OPTDESCR__ALL               TXT_HELP__ALL  "\n"  \
                                        "  Bulk variant of the operation.\n"

#define TXT_OPT__HELP                   TXT_HELP__HELP
#define TXT_OPTDESCR__HELP              TXT_HELP__HELP  "\n"  \
                                        "  Prints help\n"

#define TXT_OPT__VERBOSE                TXT_HELP__VERBOSE
#define TXT_OPTDESCR__VERBOSE           TXT_HELP__VERBOSE  "\n"  \
                                        "  Verbose variant of the operation (more info).\n"

#define TXT_OPT__VERSION                TXT_HELP__VERSION
#define TXT_OPTDESCR__VERSION           TXT_HELP__VERSION  "\n"  \
                                        "  Prints application version.\n"

#define TXT_OPT__INTERFACE_LOGIF        TXT_HELP__INTERFACE  "=<logif_name>"
#define TXT_OPTDESCR__INTERFACE_LOGIF   TXT_HELP__INTERFACE  "=<MyLogif>"  "\n"  \
                                        "  Name of the target logical interface.\n"

#define TXT_OPT__INTERFACE_PHYIF        TXT_HELP__INTERFACE  "=<phyif_name>"
#define TXT_OPTDESCR__INTERFACE_PHYIF   TXT_HELP__INTERFACE  "=<"  TXT_OPTARGS__PHYIF  ">"  "\n"  \
                                        "  Name of the target physical interface.\n"  \
                                        TXT_PHYIF_KEYWORDS

#define TXT_OPT__INTERFACE_PHYIF_EMAC       TXT_HELP__INTERFACE  "=<phyif_name>"
#define TXT_OPTDESCR__INTERFACE_PHYIF_EMAC  TXT_HELP__INTERFACE  "=<"  TXT_OPTARGS__PHYIF  ">"  "\n"  \
                                            "  Name of the target physical interface.\n"  \
                                            TXT_PHYIF_KEYWORDS_EMAC

#define TXT_OPT__INTERFACE_PHYIF_EQOS       TXT_HELP__INTERFACE  "=<phyif_name>"
#define TXT_OPTDESCR__INTERFACE_PHYIF_EQOS  TXT_HELP__INTERFACE  "=<"  TXT_OPTARGS__PHYIF  ">"  "\n"  \
                                            "  Name of the target physical interface.\n"  \
                                            TXT_PHYIF_KEYWORDS_EMAC  \
                                            "    "  TXT_PHYIF__HIF_NOCPY  "  (valid only if supported by a driver)"  "\n"  \
                                            TXT_PHYIF_KEYWORDS_HIF

#define TXT_OPT__PARENT                 TXT_HELP__PARENT  "=<phyif_name>"
#define TXT_OPTDESCR__PARENT            TXT_HELP__PARENT  "=<"  TXT_OPTARGS__PHYIF  ">"  "\n"  \
                                        "  Name of the parent physical interface.\n"   \
                                        TXT_PHYIF_KEYWORDS

#define TXT_OPT__MIRROR                 TXT_HELP__MIRROR  "=<phyif_name>"
#define TXT_OPTDESCR__MIRROR            TXT_HELP__MIRROR  "=<"  TXT_OPTARGS__MIRROR  ">"  "\n"  \
                                        "  Name of the mirroring rule.\n"

#define TXT_OPT__MODE                   TXT_HELP__MODE  "=<if_mode>"
#define TXT_OPTDESCR__MODE              TXT_HELP__MODE  "=<"  TXT_IF_MODE__VLAN_BRIDGE  "|"  TXT_IF_MODE__ROUTER  "|...>"  "\n"  \
                                        "  Operating mode of the physical interface.\n"          \
                                        "  Interface modes:\n"                      \
                                        "    "  TXT_IF_MODE__DEFAULT          "\n"  \
                                        "    "  TXT_IF_MODE__VLAN_BRIDGE      "\n"  \
                                        "    "  TXT_IF_MODE__ROUTER           "\n"  \
                                        "    "  TXT_IF_MODE__FLEXIBLE_ROUTER  "\n"  \
                                        "    "  TXT_IF_MODE__L2L3_VLAN_BRIDGE "\n"

#define TXT_OPT__BLOCK_STATE            TXT_HELP__BLOCK_STATE  "=<block_state>"
#define TXT_OPTDESCR__BLOCK_STATE       TXT_HELP__BLOCK_STATE  "=<"  TXT_IF_BLOCK_STATE__LEARN_ONLY  "|"  TXT_IF_BLOCK_STATE__FW_ONLY  "|...>"  "\n"  \
                                        "  Blocking state of the physical interface (learning and forwarding).\n"  \
                                        "  Block states:\n"                           \
                                        "    "  TXT_IF_BLOCK_STATE__NORMAL      "\n"  \
                                        "    "  TXT_IF_BLOCK_STATE__BLOCKED     "\n"  \
                                        "    "  TXT_IF_BLOCK_STATE__LEARN_ONLY  "\n"  \
                                        "    "  TXT_IF_BLOCK_STATE__FW_ONLY     "\n"

#define TXT_OPT__ENABLE                 TXT_HELP__ENABLE
#define TXT_OPTDESCR__ENABLE            TXT_HELP__ENABLE  "\n"  \
                                        "  Enables the given feature.\n"

#define TXT_OPT__ENABLE_IF              TXT_HELP__ENABLE
#define TXT_OPTDESCR__ENABLE_IF         TXT_HELP__ENABLE  "\n"  \
                                        "  Enables (\"ups\") the interface.\n"

#define TXT_OPT__DISABLE                TXT_HELP__DISABLE
#define TXT_OPTDESCR__DISABLE           TXT_HELP__DISABLE  "\n"  \
                                        "  Disables the given feature.\n"

#define TXT_OPT__DISABLE_IF             TXT_HELP__DISABLE
#define TXT_OPTDESCR__DISABLE_IF        TXT_HELP__DISABLE  "\n"  \
                                        "  Disables (\"downs\") the interface.\n"

#define TXT_OPT__DISABLE_FF             TXT_HELP__DISABLE
#define TXT_OPTDESCR__DISABLE_FF        TXT_HELP__DISABLE  "\n"  \
                                        "  Disables global FlexibleFilter.\n"

#define TXT_OPT__PROMISC_PHYIF          TXT_HELP__PROMISC  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__PROMISC_PHYIF     TXT_HELP__PROMISC  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Enables/disables promiscuous mode.\n"                 \
                                        "  (accepts all traffic regardless of destination MAC)\n"

#define TXT_OPT__PROMISC_LOGIF          TXT_HELP__PROMISC  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__PROMISC_LOGIF     TXT_HELP__PROMISC  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"    \
                                        "  Enables/disables promiscuous mode.\n"                   \
                                        "  (accepts all traffic regardless of active match rules)\n"

#define TXT_OPT__MATCH_MODE             TXT_HELP__MATCH_MODE  "=<"  TXT_OR_AND__OR  "|"  TXT_OR_AND__AND  ">"
#define TXT_OPTDESCR__MATCH_MODE        TXT_HELP__MATCH_MODE  "=<"  TXT_OR_AND__OR  "|"  TXT_OR_AND__AND  ">"  "\n"  \
                                        "  Sets chaining mode of active match rules.\n"     \
                                        "  Traffic passes matching process if:\n"           \
                                        "    "  TXT_OR_AND__OR   "  : at least one active rule is satisfied\n"  \
                                        "    "  TXT_OR_AND__AND  " : all active rules are satisfied\n"

#define TXT_OPT__DISCARD_ON_MATCH       TXT_HELP__DISCARD_ON_MATCH  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__DISCARD_ON_MATCH  TXT_HELP__DISCARD_ON_MATCH  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"     \
                                        "  If enabled, then end action of matching process is inverted:\n"   \
                                        "    --> if traffic passes the matching process, it is discarded\n"  \
                                        "    --> if traffic fails the matching process, it is accepted\n"

#define TXT_OPT__EGRESS                 TXT_HELP__EGRESS  "=<list_of_phyifs>"
#define TXT_OPTDESCR__EGRESS            TXT_HELP__EGRESS  "=<"  TXT_PHYIF__EMAC0  ","  TXT_PHYIF__HIF2  ",...>"  "\n"  \
                                        "  Comma separated list of egresses (physical interfaces) which shall receive a copy of the accepted traffic.\n"  \
                                        "  Use empty string (\"\") to disable (clear).\n"  \
                                        TXT_PHYIF_KEYWORDS

#define TXT_OPT__MATCH_RULES            TXT_HELP__MATCH_RULES  "=<list_of_rules>"
#define TXT_OPTDESCR__MATCH_RULES       TXT_HELP__MATCH_RULES  "=<"  TXT_MATCH_RULE__TYPE_ETH  ","  TXT_MATCH_RULE__VLAN  ",...>"  "\n"  \
                                        "  Comma separated list of match rules.\n"                \
                                        "  Use empty string (\"\") to disable (clear).\n"         \
                                        "  Some rules require additional command line options.\n" \
                                        "  Match rules:\n"                        \
                                        "    "  TXT_MATCH_RULE__TYPE_ETH    "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_VLAN   "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_PPPOE  "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_ARP    "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_MCAST  "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_IP4    "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_IP6    "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_IPX    "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_BCAST  "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_UDP    "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_TCP    "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_ICMP   "\n"  \
                                        "    "  TXT_MATCH_RULE__TYPE_IGMP   "\n"  \
                                        "    "  TXT_MATCH_RULE__VLAN        " ; requires <"  TXT_HELP__VLAN        ">\n"  \
                                        "    "  TXT_MATCH_RULE__PROTOCOL    " ; requires <"  TXT_HELP__PROTOCOL    ">\n"  \
                                        "    "  TXT_MATCH_RULE__SPORT       " ; requires <"  TXT_HELP__SPORT       ">\n"  \
                                        "    "  TXT_MATCH_RULE__DPORT       " ; requires <"  TXT_HELP__DPORT       ">\n"  \
                                        "    "  TXT_MATCH_RULE__SIP6        " ; requires <"  TXT_HELP__SIP6        ">\n"  \
                                        "    "  TXT_MATCH_RULE__DIP6        " ; requires <"  TXT_HELP__DIP6        ">\n"  \
                                        "    "  TXT_MATCH_RULE__SIP         " ; requires <"  TXT_HELP__SIP         ">\n"  \
                                        "    "  TXT_MATCH_RULE__DIP         " ; requires <"  TXT_HELP__DIP         ">\n"  \
                                        "    "  TXT_MATCH_RULE__ETHER_TYPE  " ; requires <"  TXT_HELP__ETHTYPE     ">\n"  \
                                        "    "  TXT_MATCH_RULE__FP_TABLE0   " ; requires <"  TXT_HELP__TABLE0      ">\n"  \
                                        "    "  TXT_MATCH_RULE__FP_TABLE1   " ; requires <"  TXT_HELP__TABLE1      ">\n"  \
                                        "    "  TXT_MATCH_RULE__SMAC        " ; requires <"  TXT_HELP__SMAC        ">\n"  \
                                        "    "  TXT_MATCH_RULE__DMAC        " ; requires <"  TXT_HELP__DMAC        ">\n"  \
                                        "    "  TXT_MATCH_RULE__HIF_COOKIE  " ; requires <"  TXT_HELP__HIF_COOKIE  ">\n"

#define TXT_OPT__VLAN                   TXT_HELP__VLAN  "=<id>" 
#define TXT_OPTDESCR__VLAN              TXT_HELP__VLAN  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  VLAN ID\n"

#define TXT_OPT__VLAN_BD                TXT_HELP__VLAN  "=<id>" 
#define TXT_OPTDESCR__VLAN_BD           TXT_HELP__VLAN  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  VLAN ID (used as a bridge domain identifier)\n"

#define TXT_OPT__R_VLAN                 TXT_HELP__R_VLAN  "=<id>" 
#define TXT_OPTDESCR__R_VLAN            TXT_HELP__R_VLAN  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Reply direction: VLAN ID\n"

#define TXT_OPT__PROTOCOL               TXT_HELP__PROTOCOL  "=<keyword|id>"
#define TXT_OPTDESCR__PROTOCOL          TXT_HELP__PROTOCOL  "=<"  TXT_PROTOCOL__IPv6  "|"  TXT_OPTARGS__U8_DEC  "|"  TXT_OPTARGS__U8_HEX  ">"  "\n"  \
                                        "  IANA Assigned Internet Protocol Number\n"  \
                                        "  https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml\n"  \
                                        "  Accepted input formats are protocol keyword or protocol ID.\n"               \
                                        "  Protocols without keyword can be addressed only by protocol ID.\n"

#define TXT_OPT__PROTOCOL_CNTKTMO       TXT_HELP__PROTOCOL  "=<"  TXT_PROTOCOL__TCP  "|"  TXT_PROTOCOL__UDP  "|0>"
#define TXT_OPTDESCR__PROTOCOL_CNTKTMO  TXT_HELP__PROTOCOL  "=<"  TXT_PROTOCOL__TCP  "|"  TXT_PROTOCOL__UDP  "|0>"  "\n"  \
                                        "  IANA Assigned Internet Protocol Number\n"    \
                                        "  Only selected protocols are accepted by this command.\n"  \
                                        "  The rest of protocols is summarily referred to as 'others' (value 0).\n"

#define TXT_OPT__ETHTYPE                TXT_HELP__ETHTYPE  "=<id>"
#define TXT_OPTDESCR__ETHTYPE           TXT_HELP__ETHTYPE  "=<"  TXT_OPTARGS__U16_DEC  "|"  TXT_OPTARGS__U16_HEX  ">"  "\n"  \
                                        "  IANA \"EtherType number\" (IEEE 802)\n"     \
                                        "  https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml\n"  \

#define TXT_OPT__MAC                    TXT_HELP__MAC  "=<mac_addr>"
#define TXT_OPTDESCR__MAC               TXT_HELP__MAC  "=<"  TXT_OPTARGS__MAC_ADDR  ">"  "\n"  \
                                        "  MAC address\n"

#define TXT_OPT__SMAC                   TXT_HELP__SMAC  "=<mac_addr>"
#define TXT_OPTDESCR__SMAC              TXT_HELP__SMAC  "=<"  TXT_OPTARGS__MAC_ADDR  ">"  "\n"  \
                                        "  Source MAC address\n"

#define TXT_OPT__DMAC                   TXT_HELP__DMAC  "=<mac_addr>"
#define TXT_OPTDESCR__DMAC              TXT_HELP__DMAC  "=<"  TXT_OPTARGS__MAC_ADDR  ">"  "\n"  \
                                        "  Destination MAC address\n"

#define TXT_OPT__SIP                    TXT_HELP__SIP  "=<ipv4|ipv6>"
#define TXT_OPTDESCR__SIP               TXT_HELP__SIP  "=<12.126.31.7|fd00::1>"  "\n"  \
                                        "  Source IP address\n"

#define TXT_OPT__SIP_LOGIF              TXT_HELP__SIP  "=<ipv4>"
#define TXT_OPTDESCR__SIP_LOGIF         TXT_HELP__SIP  "=<12.126.31.7>"  "\n"  \
                                        "  Source IP address (only IPv4 is accepted)\n"

#define TXT_OPT__DIP                    TXT_HELP__DIP  "=<ipv4|ipv6>"
#define TXT_OPTDESCR__DIP               TXT_HELP__DIP  "=<132.16.20.3|fc62::5>"  "\n"  \
                                        "  Destination IP address\n"

#define TXT_OPT__DIP_LOGIF              TXT_HELP__DIP  "=<ipv4>"
#define TXT_OPTDESCR__DIP_LOGIF         TXT_HELP__DIP  "=<132.16.20.3>"  "\n"  \
                                        "  Destination IP address (only IPv4 is accepted)\n"

#define TXT_OPT__R_SIP                  TXT_HELP__R_SIP  "=<ipv4|ipv6>"
#define TXT_OPTDESCR__R_SIP             TXT_HELP__R_SIP  "=<13.125.30.2|fe51::7>"  "\n"  \
                                        "  Reply direction: source IP address (used for NAT)\n"

#define TXT_OPT__R_DIP                  TXT_HELP__R_DIP  "=<ipv4|ipv6>"
#define TXT_OPTDESCR__R_DIP             TXT_HELP__R_DIP  "=<112.37.52.9|fd14::2>"  "\n"  \
                                        "  Reply direction: destination IP address (used for NAT)\n"

#define TXT_OPT__SIP6                   TXT_HELP__SIP6  "=<ipv6>"
#define TXT_OPTDESCR__SIP6              TXT_HELP__SIP6  "=<fd00::1>"  "\n"  \
                                        "  Source IP address (only IPv6 is accepted)\n"

#define TXT_OPT__DIP6                   TXT_HELP__DIP6  "=<ipv6>"
#define TXT_OPTDESCR__DIP6              TXT_HELP__DIP6  "=<fc62::5>"  "\n"  \
                                        "  Destination IP address (only IPv6 is accepted)\n"

#define TXT_OPT__SPORT                  TXT_HELP__SPORT  "=<port>"
#define TXT_OPTDESCR__SPORT             TXT_HELP__SPORT  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Source port\n"

#define TXT_OPT__DPORT                  TXT_HELP__DPORT  "=<port>"
#define TXT_OPTDESCR__DPORT             TXT_HELP__DPORT  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Destination port\n"

#define TXT_OPT__R_SPORT                TXT_HELP__R_SPORT  "=<port>"
#define TXT_OPTDESCR__R_SPORT           TXT_HELP__R_SPORT  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Reply direction: source port (used for PAT)\n"

#define TXT_OPT__R_DPORT                TXT_HELP__R_DPORT  "=<port>"
#define TXT_OPTDESCR__R_DPORT           TXT_HELP__R_DPORT  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Reply direction: destination port (used for PAT)\n"

#define TXT_OPT__HIF_COOKIE             TXT_HELP__HIF_COOKIE  "=<hex_value>"
#define TXT_OPTDESCR__HIF_COOKIE        TXT_HELP__HIF_COOKIE  "=<"  TXT_OPTARGS__U32_HEX  ">"  "\n"  \
                                        "  Can be used to recognize which HIF sent the traffic.\n"   \
                                        "  PFE driver in the host OS must be specifically configured to allow use of this feature.\n"

#define TXT_OPT__TIMEOUT_CNTKTMO        TXT_HELP__TIMEOUT  "=<seconds>"
#define TXT_OPTDESCR__TIMEOUT_CNTKTMO   TXT_HELP__TIMEOUT  "=<"  TXT_OPTARGS__U32_DEC  ">"  "\n"  \
                                        "  Timeout in seconds.\n"

#define TXT_OPT__TIMEOUT2_CNTKTMO       TXT_HELP__TIMEOUT2  "=<seconds>"
#define TXT_OPTDESCR__TIMEOUT2_CNTKTMO  TXT_HELP__TIMEOUT2  "=<"  TXT_OPTARGS__U32_DEC  ">"  "\n"  \
                                        "  Timeout in seconds.\n"  \
                                        "  This value is applied only on unidirectional UDP conntracks.\n"

#define TXT_OPT__UCAST_HIT              TXT_HELP__UCAST_HIT  "=<action>"
#define TXT_OPTDESCR__UCAST_HIT         TXT_HELP__UCAST_HIT  "=<"  TXT_OPTARGS__BD_ACTIONS ">"  "\n"  \
                                        "  Action to be taken when unicast packet's destination MAC matches some MAC table entry.\n"  \
                                        "  Actions:\n"  \
                                        "    "  TXT_BD_ACTION__FORWARD  "\n"  \
                                        "    "  TXT_BD_ACTION__FLOOD    "\n"  \
                                        "    "  TXT_BD_ACTION__PUNT     "\n"  \
                                        "    "  TXT_BD_ACTION__DISCARD  "\n"

#define TXT_OPT__UCAST_MISS             TXT_HELP__UCAST_MISS  "=<action>"
#define TXT_OPTDESCR__UCAST_MISS        TXT_HELP__UCAST_MISS  "=<"  TXT_OPTARGS__BD_ACTIONS ">"  "\n"  \
                                        "  Action to be taken when unicast packet's destination MAC does not match any MAC table entry.\n"  \
                                        "  Possible actions:\n"  \
                                        "    Same as actions of  ["  TXT_HELP__UCAST_HIT  "]\n"

#define TXT_OPT__MCAST_HIT              TXT_HELP__MCAST_HIT  "=<action>"
#define TXT_OPTDESCR__MCAST_HIT         TXT_HELP__MCAST_HIT  "=<"  TXT_OPTARGS__BD_ACTIONS ">"  "\n"  \
                                        "  Action to be taken when multicast packet's destination MAC matches some MAC table entry.\n"  \
                                        "  Possible actions:\n"  \
                                        "    Same as actions of  ["  TXT_HELP__UCAST_HIT  "]\n"

#define TXT_OPT__MCAST_MISS             TXT_HELP__MCAST_MISS  "=<action>"
#define TXT_OPTDESCR__MCAST_MISS        TXT_HELP__MCAST_MISS  "=<"  TXT_OPTARGS__BD_ACTIONS ">"  "\n"  \
                                        "  Action to be taken when multicast packet's destination MAC does not match any MAC table entry.\n"  \
                                        "  Possible actions:\n"  \
                                        "    Same as actions of  ["  TXT_HELP__UCAST_HIT  "]\n"

#define TXT_OPT__TAG                    TXT_HELP__TAG  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__TAG               TXT_HELP__TAG  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Sets/unsets whether the traffic from the given interface has the VLAN tag retained/added ("  TXT_ON_OFF__ON  ")\n"  \
                                        "  or has the VLAN tag removed ("  TXT_ON_OFF__OFF  ").\n"

#define TXT_OPT__DEFAULT                TXT_HELP__DEFAULT
#define TXT_OPTDESCR__DEFAULT           TXT_HELP__DEFAULT  "\n"  \
                                        "  Sets the given bridge domain as a default bridge domain.\n"  \
                                        "  Default bridge domain is used for packets which:\n"          \
                                        "    --> don't have a VLAN TAG\n"                               \
                                        "    --> have VLAN TAG matching the VLAN ID of the default domain.\n"

#define TXT_OPT__FALLBACK               TXT_HELP__FALLBACK
#define TXT_OPTDESCR__FALLBACK          TXT_HELP__FALLBACK  "\n"  \
                                        "  Sets the given bridge domain as a fallback bridge domain.\n"             \
                                        "  Fallback bridge domain is used for packets which do have a VLAN TAG,\n"  \
                                        "  but their VLAN TAG does not match VLAN ID of any existing bridge domain.\n"

#define TXT_OPT__4o6                    TXT_HELP__4o6
#define TXT_OPTDESCR__4o6               TXT_HELP__4o6  "\n"  \
                                        "  Specifies that the timeout is meant for IPv4 over IPv6 tunneling connections.\n"

#define TXT_OPT__NO_REPLY               TXT_HELP__NO_REPLY
#define TXT_OPTDESCR__NO_REPLY          TXT_HELP__NO_REPLY  "\n"  \
                                        "  Specifies unidirectional conntrack - only the \"original direction\" route is created.\n"

#define TXT_OPT__NO_ORIG                TXT_HELP__NO_ORIG
#define TXT_OPTDESCR__NO_ORIG           TXT_HELP__NO_ORIG  "\n"  \
                                        "  Specifies unidirectional conntrack - only the \"reply direction\" route is created.\n"

#define TXT_OPT__ROUTE                  TXT_HELP__ROUTE  "=<id>"
#define TXT_OPTDESCR__ROUTE             TXT_HELP__ROUTE  "=<"  TXT_OPTARGS__U32_DEC  ">"  "\n"  \
                                        "  Route ID\n"

#define TXT_OPT__R_ROUTE                TXT_HELP__R_ROUTE  "=<id>"
#define TXT_OPTDESCR__R_ROUTE           TXT_HELP__R_ROUTE  "=<"  TXT_OPTARGS__U32_DEC  ">"  "\n"  \
                                        "  Reply direction: route ID\n"

#define TXT_OPT__RX_MIRROR0             TXT_HELP__RX_MIRROR0  "=<rule_name>"
#define TXT_OPTDESCR__RX_MIRROR0        TXT_HELP__RX_MIRROR0  "=<"  TXT_OPTARGS__MIRROR  ">"  "\n"  \
                                        "  Mirroring rule for the rx slot [0].\n"                   \
                                        "  Use empty string (\"\") to disable (clear).\n"

#define TXT_OPT__RX_MIRROR1             TXT_HELP__RX_MIRROR1  "=<rule_name>"
#define TXT_OPTDESCR__RX_MIRROR1        TXT_HELP__RX_MIRROR1  "=<"  TXT_OPTARGS__MIRROR  ">"  "\n"  \
                                        "  Mirroring rule for the rx slot [1].\n"                   \
                                        "  Use empty string (\"\") to disable (clear).\n"

#define TXT_OPT__TX_MIRROR0             TXT_HELP__TX_MIRROR0  "=<rule_name>"
#define TXT_OPTDESCR__TX_MIRROR0        TXT_HELP__TX_MIRROR0  "=<"  TXT_OPTARGS__MIRROR  ">"  "\n"  \
                                        "  Mirroring rule for the tx slot [0].\n"                   \
                                        "  Use empty string (\"\") to disable (clear).\n"

#define TXT_OPT__TX_MIRROR1             TXT_HELP__TX_MIRROR1  "=<rule_name>"
#define TXT_OPTDESCR__TX_MIRROR1        TXT_HELP__TX_MIRROR1  "=<"  TXT_OPTARGS__MIRROR  ">"  "\n"  \
                                        "  Mirroring rule for the tx slot [1].\n"                   \
                                        "  Use empty string (\"\") to disable (clear).\n"

#define TXT_OPT__FP_TABLE               TXT_HELP__TABLE  "=<table_name>"
#define TXT_OPTDESCR__FP_TABLE          TXT_HELP__TABLE  "=<"  TXT_OPTARGS__FP_TABLE  ">"  "\n"  \
                                        "  Name of a FlexibleParser table.\n"

#define TXT_OPT__FLEXIBLE_FILTER        TXT_HELP__FLEXIBLE_FILTER  "=<table_name>"
#define TXT_OPTDESCR__FLEXIBLE_FILTER   TXT_HELP__FLEXIBLE_FILTER  "=<"  TXT_OPTARGS__FP_TABLE  ">"  "\n"  \
                                        "  Name of a FlexibleParser table which shall be used as a filter (FlexibleFilter).\n"  \
                                        "  Use empty string (\"\") to disable (clear).\n"

#define TXT_OPT__FP_TABLE0_LOGIF        TXT_HELP__TABLE0  "=<table_name>"
#define TXT_OPTDESCR__FP_TABLE0_LOGIF   TXT_HELP__TABLE0  "=<"  TXT_OPTARGS__FP_TABLE  ">"  "\n"  \
                                        "  Name of a FlexibleParser table for the parser slot [0].\n"  \

#define TXT_OPT__FP_TABLE1_LOGIF        TXT_HELP__TABLE1  "=<table_name>"
#define TXT_OPTDESCR__FP_TABLE1_LOGIF   TXT_HELP__TABLE1  "=<"  TXT_OPTARGS__FP_TABLE  ">"  "\n"  \
                                        "  Name of a FlexibleParser table for the parser slot [1].\n"  \

#define TXT_OPT__FP_RULE                TXT_HELP__RULE  "=<rule_name>"
#define TXT_OPTDESCR__FP_RULE           TXT_HELP__RULE  "=<"  TXT_OPTARGS__FP_RULE  ">"  "\n"  \
                                        "  Name of a FlexibleParser rule.\n"

#define TXT_OPT__FP_NEXT_RULE           TXT_HELP__NEXT_RULE  "=<rule_name>"
#define TXT_OPTDESCR__FP_NEXT_RULE      TXT_HELP__NEXT_RULE  "=<"  TXT_OPTARGS__FP_RULE  ">"  "\n"  \
                                        "  Rule action: invoke the supplied FlexibleParser rule as the next processing rule.\n"

#define TXT_OPT__DATA                   TXT_HELP__DATA  "=<hex_value>" 
#define TXT_OPTDESCR__DATA              TXT_HELP__DATA  "=<"  TXT_OPTARGS__U32_HEX  ">"  "\n"  \
                                        "  Expected data value (32bit hexadecimal).\n"

#define TXT_OPT__MASK                   TXT_HELP__MASK  "=<hex_value>" 
#define TXT_OPTDESCR__MASK              TXT_HELP__MASK  "=<"  TXT_OPTARGS__U32_HEX  ">"  "\n"  \
                                        "  A bitmask to apply on processed data prior to data comparison (32bit hexadecimal).\n"

#define TXT_OPT__LAYER                  TXT_HELP__LAYER  "=<L2|L3|L4>"
#define TXT_OPTDESCR__LAYER             TXT_HELP__LAYER  "=<L2|L3|L4>"  "\n"  \
                                        "  Base offset for further offset calculations.\n"  \
                                        "  Possible base offsets:\n"                        \
                                        "    "  TXT_OFFSET_FROM__L2  " : start from layer 2 header (from ETH frame header)\n"       \
                                        "    "  TXT_OFFSET_FROM__L3  " : start from layer 3 header (e.g. from IP packet header)\n"  \
                                        "    "  TXT_OFFSET_FROM__L4  " : start from layer 4 header (e.g. from TCP segment header)\n"

#define TXT_OPT__OFFSET_FP              TXT_HELP__OFFSET  "=<value>"
#define TXT_OPTDESCR__OFFSET_FP         TXT_HELP__OFFSET  "=<"  TXT_OPTARGS__U16_DEC  "|"  TXT_OPTARGS__U16_HEX  ">"  "\n"  \
                                        "  Offset to the inspected data within the packet (added to the layer base offset).\n"

#define TXT_OPT__INVERT_FP              TXT_HELP__INVERT
#define TXT_OPTDESCR__INVERT_FP         TXT_HELP__INVERT  "\n"  \
                                        "  Invert the result of a rule's matching process.\n"  \
                                        "  If a raw data comparison yields 'true' but this flag is set, then "  \
                                        "  the final result of a matching process will be 'false' (and vice versa).\n"

#define TXT_OPT__ACCEPT_FP              TXT_HELP__ACCEPT
#define TXT_OPTDESCR__ACCEPT_FP         TXT_HELP__ACCEPT  "\n"  \
                                        "  Rule action: accept the packet\n"

#define TXT_OPT__REJECT_FP              TXT_HELP__REJECT
#define TXT_OPTDESCR__REJECT_FP         TXT_HELP__REJECT  "\n"  \
                                        "  Rule action: reject the packet\n"

#define TXT_OPT__POSITION_INSADD        TXT_HELP__POSITION  "=<value>"
#define TXT_OPTDESCR__POSITION_INSADD   TXT_HELP__POSITION  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Index where to insert the item.\n"                       \
                                        "  (hint: indexing starts from the position 0)\n"           \
                                        "  If this option is not utilized, then the item is automatically\n"  \
                                        "  inserted as the last item of the table.\n"

#define TXT_OPT__POSITION_REMDEL        TXT_HELP__POSITION  "=<value>"
#define TXT_OPTDESCR__POSITION_REMDEL   TXT_HELP__POSITION  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Index of the target item to destroy.\n"  \
                                        "  (hint: indexing starts from position 0)\n"

#define TXT_OPT__POSITION_PRINT         TXT_HELP__POSITION  "=<value>"
#define TXT_OPTDESCR__POSITION_PRINT    TXT_HELP__POSITION  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Index of the first item to print.\n"  \
                                        "  Default value is 0 (start from the very first item of the table).\n"

#define TXT_OPT__POSITION_FWFEAT_EL_SET          TXT_HELP__POSITION  "=<value>"
#define TXT_OPTDESCR__POSITION_FWFEAT_EL_SET     TXT_HELP__POSITION  "=<"  TXT_OPTARGS__U8_DEC  ">"  "\n"    \
                                                 "  Index of the target item in the element table. \n"       \
                                                 "  Position == 0 : Operate with all table items. \n"        \
                                                 "  Position > 0  : Operate with the particular item that is at given position. \n"  \
                                                 "  Indexing starts from positon 1. \n"  \
                                                 "  Default value of this parameter is 0 (operate with all table items).\n"

#define TXT_OPT__POSITION_INSADD_IQOS_FLOW       TXT_HELP__POSITION  "=<value>"
#define TXT_OPTDESCR__POSITION_INSADD_IQOS_FLOW  TXT_HELP__POSITION  "=<"  TXT_OPTARGS__U8_DEC  ">"  "\n"  \
                                                 "  Index where to insert the item.\n"                     \
                                                 "  (hint: indexing starts from the position 0)\n"         \
                                                 "  If this option is not utilized, then the item is automatically\n"  \
                                                 "  inserted as the last item of the table.\n"

#define TXT_OPT__POSITION_REMDEL_IQOS_FLOW       TXT_HELP__POSITION  "=<value>"
#define TXT_OPTDESCR__POSITION_REMDEL_IQOS_FLOW  TXT_HELP__POSITION  "=<"  TXT_OPTARGS__U8_DEC  ">"  "\n"  \
                                                 "  Index of the target item to destroy.\n"  \
                                                 "  (hint: indexing starts from position 0)\n"

#define TXT_OPT__POSITION_PRINT_IQOS_FLOW        TXT_HELP__POSITION  "=<value>"
#define TXT_OPTDESCR__POSITION_PRINT_IQOS_FLOW   TXT_HELP__POSITION  "=<"  TXT_OPTARGS__U8_DEC  ">"  "\n"  \
                                                 "  Index of the item to print.\n"  \
                                                 "  Default value is 0 (start from the very first item of the table).\n"

#define TXT_OPT__COUNT_PRINT            TXT_HELP__COUNT  "=<value>"
#define TXT_OPTDESCR__COUNT_PRINT       TXT_HELP__COUNT  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Count of items to print.\n"  \
                                        "  Default value is 0 (print all available items).\n"

#define TXT_OPT__SAD                    TXT_HELP__SAD  "=<idx>"
#define TXT_OPTDESCR__SAD               TXT_HELP__SAD  "=<"  TXT_OPTARGS__U32_DEC  ">"  "\n"  \
                                        "  Index into SAD (Security Association Database).\n"

#define TXT_OPT__SPD_ACTION             TXT_HELP__SPD_ACTION  "=<action>"
#define TXT_OPTDESCR__SPD_ACTION        TXT_HELP__SPD_ACTION  "=<BYPASS|1-4>"  "\n"  \
                                        "  Action to be done on traffic which matches SPD criteria.\n"  \
                                        "  Actions:\n"                         \
                                        "    "  TXT_SPD_ACTION__DISCARD  "\n"  \
                                        "    "  TXT_SPD_ACTION__BYPASS   "\n"  \
                                        "    "  TXT_SPD_ACTION__ENCODE   " ; requires <"  TXT_HELP__SAD  ">\n"  \
                                        "    "  TXT_SPD_ACTION__DECODE   " ; requires <"  TXT_HELP__SPI  ">\n"

#define TXT_OPT__SPI                    TXT_HELP__SPI  "=<hex_value>"
#define TXT_OPTDESCR__SPI               TXT_HELP__SPI  "=<"  TXT_OPTARGS__U32_HEX  ">"  "\n"  \
                                        "  Security Parameter Index\n"

#define TXT_OPT__VLAN_CONF              TXT_HELP__VLAN_CONF  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__VLAN_CONF         TXT_HELP__VLAN_CONF  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"                   \
                                        "  Enables/disables a strict VLAN conformance check.\n"                     \
                                        "  When enabled, the interface automatically discards all traffic that \n"  \
                                        "  is not strictly IEEE 802.1Q compliant.\n"

#define TXT_OPT__PTP_CONF               TXT_HELP__PTP_CONF  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__PTP_CONF          TXT_HELP__PTP_CONF  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"        \
                                        "  Enables/disables a strict PTP conformance check.\n"          \
                                        "  When enabled, the interface automatically discards all traffic that \n"  \
                                        "  is not strictly IEEE 802.1AS compliant.\n"

#define TXT_OPT__PTP_PROMISC            TXT_HELP__PTP_PROMISC  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__PTP_PROMISC       TXT_HELP__PTP_PROMISC  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Enables/disables acceptance of PTP traffic even if ["  TXT_HELP__VLAN_CONF  "] flag is active.\n"

#define TXT_OPT__QINQ                   TXT_HELP__QINQ  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__QINQ              TXT_HELP__QINQ  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Enables/disables processing of Q-in-Q traffic.\n"  \
                                        "  If disabled, then traffic with multiple VLAN tags is automatically discarded.\n"

#define TXT_OPT__LOCAL_STENT            TXT_HELP__LOCAL  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__LOCAL_STENT       TXT_HELP__LOCAL  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Makes the static entry a LOCAL entry.\n"  \
                                        "  If this flag is set, then: \n"            \
                                        "    --> forwarding list is ignored \n"      \
                                        "    --> if traffic's destination MAC matches the MAC of this static entry, then \n"  \
                                        "        the traffic is passed to the IP router.\n"  \
                                        "        (requires L2L3 mode on the ingress physical interface)\n"

#define TXT_OPT__DISCARD_ON_MATCH_SRC       TXT_HELP__DISCARD_ON_MATCH_SRC  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__DISCARD_ON_MATCH_SRC  TXT_HELP__DISCARD_ON_MATCH_SRC  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                            "  Discard traffic if its source MAC matches the MAC of this static entry.\n"

#define TXT_OPT__DISCARD_ON_MATCH_DST       TXT_HELP__DISCARD_ON_MATCH_DST  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__DISCARD_ON_MATCH_DST  TXT_HELP__DISCARD_ON_MATCH_DST  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                            "  Discard traffic if its destination MAC matches the MAC of this static entry.\n"

#define TXT_OPT__FEATURE_FW             TXT_HELP__FEATURE  "=<feature_name>"
#define TXT_OPTDESCR__FEATURE_FW        TXT_HELP__FEATURE  "=<ingress_vlan>"  "\n"  \
                                        "  Name of a FW feature.\n"
                                                                               
#define TXT_OPT__FEATURE_EL_FW_EL       TXT_HELP__FEATURE  "=<feature_name>"
#define TXT_OPTDESCR__FEATURE_FW_EL     TXT_HELP__FEATURE  "=<ingress_vlan>"  "\n"  \
                                        "  Name of a FW feature.\n"

#define TXT_OPT__FEATURE_DEMO           TXT_HELP__FEATURE  "=<feature_name>"
#define TXT_OPTDESCR__FEATURE_DEMO      TXT_HELP__FEATURE  "=<L2_bridge_simple>"  "\n"  \
                                        "  Name of a demo scenario for a PFE feature.\n"

#define TXT_OPT__STATIC                 TXT_HELP__STATIC
#define TXT_OPTDESCR__STATIC            TXT_HELP__STATIC  "\n"  \
                                        "  Apply only on static entries.\n"

#define TXT_OPT__DYNAMIC                TXT_HELP__DYNAMIC
#define TXT_OPTDESCR__DYNAMIC           TXT_HELP__DYNAMIC  "\n"  \
                                        "  Apply only on dynamic (learned) entries.\n"

#define TXT_OPT__QUE                    TXT_HELP__QUE  "=<id>"
#define TXT_OPTDESCR__QUE               TXT_HELP__QUE  "=<0-7>"  "\n"  \
                                        "  Queue ID\n"

#define TXT_OPT__SCH                    TXT_HELP__SCH  "=<id>"
#define TXT_OPTDESCR__SCH               TXT_HELP__SCH  "=<0|1>"  "\n"  \
                                        "  Scheduler ID\n"

#define TXT_OPT__SHP                    TXT_HELP__SHP  "=<id>"
#define TXT_OPTDESCR__SHP               TXT_HELP__SHP  "=<0-3>"  "\n"  \
                                        "  Shaper ID\n"

#define TXT_OPT__QUE_MODE               TXT_HELP__QUE_MODE  "=<mode>"
#define TXT_OPTDESCR__QUE_MODE          TXT_HELP__QUE_MODE  "=<"  TXT_QUE_MODE__TAIL_DROP  ">"  "\n"  \
                                        "  Queue mode\n"  \
                                        "  Modes:\n"      \
                                        "    "  TXT_QUE_MODE__DISABLED   "\n"  \
                                        "    "  TXT_QUE_MODE__DEFAULT    "\n"  \
                                        "    "  TXT_QUE_MODE__TAIL_DROP  "\n"  \
                                        "    "  TXT_QUE_MODE__WRED       "\n"

#define TXT_OPT__SCH_MODE               TXT_HELP__SCH_MODE  "=<mode>"
#define TXT_OPTDESCR__SCH_MODE          TXT_HELP__SCH_MODE  "=<"  TXT_SCH_MODE__DISABLED  "|"  TXT_SCH_MODE__DATA_RATE  "|"  TXT_SCH_MODE__PACKET_RATE  ">"  "\n"  \
                                        "  Scheduler mode\n"

#define TXT_OPT__SHP_MODE               TXT_HELP__SHP_MODE  "=<mode>"
#define TXT_OPTDESCR__SHP_MODE          TXT_HELP__SHP_MODE  "=<"  TXT_SHP_MODE__DISABLED  "|"  TXT_SHP_MODE__DATA_RATE  "|"  TXT_SHP_MODE__PACKET_RATE  ">"  "\n"  \
                                        "  Shaper mode\n"

#define TXT_OPT__SHP_MODE_IQOS          TXT_HELP__SHP_MODE  "=<mode>"
#define TXT_OPTDESCR__SHP_MODE_IQOS     TXT_HELP__SHP_MODE  "=<"  TXT_SHP_MODE__DATA_RATE  "|"  TXT_SHP_MODE__PACKET_RATE  ">"  "\n"  \
                                        "  Shaper mode\n"

#define TXT_OPT__THMIN_EQOS             TXT_HELP__THMIN  "=<value>"
#define TXT_OPTDESCR__THMIN_EQOS        TXT_HELP__THMIN  "=<interface type dependent>"  "\n"  \
                                        "  Minimal threshold value. Meaningful only for the following que modes:\n"  \
                                        "    "  TXT_QUE_MODE__WRED  ": Number of packets in the queue where the lowest drop probability zone starts.\n"  \
                                        "  Range depends on type of the target physical interface:\n"  \
                                        TXT_EQOS_QUEUE_RANGES

#define TXT_OPT__THMIN_IQOS_WRED        TXT_HELP__THMIN  "=<value>"
#define TXT_OPTDESCR__THMIN_IQOS_WRED   TXT_HELP__THMIN  "=<queue type dependent>"  "\n"  \
                                        "  Minimal threshold value - number of packets in the queue where the lowest drop probability zone starts.\n"  \
                                        "  Range depends on wred queue type:\n"  \
                                        TXT_IQOS_WRED_RANGES

#define TXT_OPT__THMAX_EQOS             TXT_HELP__THMAX  "=<value>"
#define TXT_OPTDESCR__THMAX_EQOS        TXT_HELP__THMAX  "=<interface type dependent>"  "\n"  \
                                        "  Maximal threshold value. Meaningful only for the following que modes:\n"  \
                                        "    "  TXT_QUE_MODE__TAIL_DROP  ": Max allowed number of packets in the queue.\n"  \
                                        "    "  TXT_QUE_MODE__WRED       ": Number of packets in the queue above which the drop probability is always 100%.\n"  \
                                        "  Range depends on type of the target interface (see " TXT_HELP__THMIN ").\n"

#define TXT_OPT__THMAX_IQOS_WRED        TXT_HELP__THMAX  "=<value>"
#define TXT_OPTDESCR__THMAX_IQOS_WRED   TXT_HELP__THMAX  "=<queue type dependent>"  "\n"  \
                                        "  Maximal threshold value - number of packets in the queue above which the drop probability"  \
                                        "  for Unmanaged and Managed traffic is always 100%. Reserved traffic is still accepted.\n"    \
                                        "  Range depends on wred queue type (see "  TXT_HELP__THMIN  ").\n"

#define TXT_OPT__THFULL_IQOS_WRED       TXT_HELP__THFULL  "=<value>"
#define TXT_OPTDESCR__THFULL_IQOS_WRED  TXT_HELP__THFULL  "=<queue type dependent>"  "\n"  \
                                        "  Queue length - number of packets in the queue above which all traffic (even the Reserved traffic) is dropped.\n"  \
                                        "  Range depends on wred queue type (see "  TXT_HELP__THMIN  ").\n"

#define TXT_OPT__ZPROB                  TXT_HELP__ZPROB  "=<list_of_percentages>"
#define TXT_OPTDESCR__ZPROB             TXT_HELP__ZPROB  "=<10,30,K,50,...>"  "\n"  \
                                        "  Comma separated list of percentages.\n"  \
                                        "  Drop probabilities for probability zones. Meaningful only for queue mode "  TXT_QUE_MODE__WRED  ".\n"  \
                                        "  Position of a value in the list corresponds with a zone (from zone [0] up to zone [N]).\n"  \
                                        "  Zones which are not touched (when provided list is too short) and zones which are marked with 'K' (keep) are left unchanged.\n"  \
                                        "  NOTE: Percentages are stored in a compressed format. Expect a certain inaccuracy of stored data (around +/- 3 %)."

#define TXT_OPT__ZPROB_IQOS_WRED        TXT_HELP__ZPROB  "=<list_of_percentages>"
#define TXT_OPTDESCR__ZPROB_IQOS_WRED   TXT_HELP__ZPROB  "=<10,30,K,50,...>"  "\n"  \
                                        "  Comma separated list of percentages.\n"  \
                                        "  Drop probabilities for probability zones.\n"  \
                                        "  Position of a value in the list corresponds with a zone (from zone [0] up to zone [N]).\n"  \
                                        "  Zones which are not touched (when provided list is too short) and zones which are marked with 'K' (keep) are left unchanged.\n"  \
                                        "  NOTE: Percentages are stored in a compressed format. Expect a certain inaccuracy of stored data (around +/- 6 %)."

#define TXT_OPT__SCH_ALGO               TXT_HELP__SCH_ALGO  "=<algorithm>"
#define TXT_OPTDESCR__SCH_ALGO          TXT_HELP__SCH_ALGO  "=<"  TXT_SCH_ALGO__DWRR  ">"  "\n"  \
                                        "  Scheduler selection algorithm\n"  \
                                        "  Algorithms:\n"  \
                                        "    "  TXT_SCH_ALGO__PQ    "    (Priority Queue)\n"  \
                                        "    "  TXT_SCH_ALGO__DWRR  "  (Deficit Weighted Round Robin)\n"  \
                                        "    "  TXT_SCH_ALGO__RR    "    (Round Robin)\n"  \
                                        "    "  TXT_SCH_ALGO__WRR   "   (Weighted Round Robin)\n"

#define TXT_OPT__SCH_IN                 TXT_HELP__SCH_IN  "=<list_of_inputs>"
#define TXT_OPTDESCR__SCH_IN            TXT_HELP__SCH_IN  "=<"  TXT_SCH_IN__QUE1  ":10,"  TXT_SCH_IN__QUE3  ":243,D,K,...>"  "\n"  \
                                        "  Comma separated list of input sources and their weights.\n"  \
                                        "  Input sources:\n"  \
                                        "    "  TXT_SCH_IN__KEEP      "  (to keep the given scheduler input untouched)\n"  \
                                        "    "  TXT_SCH_IN__DISABLED  "  (to disable the given scheduler input)\n"         \
                                        "    "  TXT_SCH_IN__QUE0      "\n"  \
                                        "    "  TXT_SCH_IN__QUE1      "\n"  \
                                        "    "  TXT_SCH_IN__QUE2      "\n"  \
                                        "    "  TXT_SCH_IN__QUE3      "\n"  \
                                        "    "  TXT_SCH_IN__QUE4      "\n"  \
                                        "    "  TXT_SCH_IN__QUE5      "\n"  \
                                        "    "  TXT_SCH_IN__QUE6      "\n"  \
                                        "    "  TXT_SCH_IN__QUE7      "\n"  \
                                        "    "  TXT_SCH_IN__SCH0_OUT  "\n"

#define TXT_OPT__SHP_POS                TXT_HELP__SHP_POS  "=<position>"
#define TXT_OPTDESCR__SHP_POS           TXT_HELP__SHP_POS  "=<"  TXT_SHP_POS__SCH1_IN3  ">"  "\n"  \
                                        "  Position of a shaper within the QoS configuration.\n"   \
                                        "  Positions:\n"  \
                                        "    "  TXT_SHP_POS__DISABLED  "\n"  \
                                        "    "  TXT_SHP_POS__SCH0_IN0  ", "  TXT_SHP_POS__SCH0_IN1  " ... "  TXT_SHP_POS__SCH0_IN7  "\n"  \
                                        "    "  TXT_SHP_POS__SCH1_IN0  ", "  TXT_SHP_POS__SCH1_IN1  " ... "  TXT_SHP_POS__SCH1_IN7  "\n"  \
                                        "    "  TXT_SHP_POS__SCH1_OUT  "\n"

#define TXT_OPT__ISL                    TXT_HELP__ISL  "=<value>"
#define TXT_OPTDESCR__ISL               TXT_HELP__ISL  "=<"  TXT_OPTARGS__U32_DEC  ">"  "\n"  \
                                        "  Idle slope [units per second].\n"  \
                                        "  Units depend on currently set ["  TXT_HELP__SHP_MODE  "]:\n"  \
                                        "    [bits per second]    (inaccuracy +/- 2400 units) for "  TXT_SHP_MODE__DATA_RATE    "\n"  \
                                        "    [packets per second] (inaccuracy +/-  300 units) for "  TXT_SHP_MODE__PACKET_RATE  "\n"  \
                                        "  NOTE: Idle slope is stored in a compressed format. Expect a certain inaccuracy of stored data.\n"

#define TXT_OPT__CRMIN                  TXT_HELP__CRMIN  "=<value>"
#define TXT_OPTDESCR__CRMIN             TXT_HELP__CRMIN  "=<"  TXT_OPTARGS__I32_DEC  ">"  "\n"  \
                                        "  Minimal credit.\n"  \
                                        "  Units depend on currently set ["  TXT_HELP__SHP_MODE  "]:\n"  \
                                        "    [bytes]   for "  TXT_SHP_MODE__DATA_RATE    "\n"  \
                                        "    [packets] for "  TXT_SHP_MODE__PACKET_RATE  "\n"

#define TXT_OPT__CRMAX                  TXT_HELP__CRMAX  "=<value>"
#define TXT_OPTDESCR__CRMAX             TXT_HELP__CRMAX  "=<"  TXT_OPTARGS__I32_DEC  ">"  "\n"  \
                                        "  Maximal credit.\n"  \
                                        "  Units depend on currently set ["  TXT_HELP__SHP_MODE  "]:\n"  \
                                        "    [bytes]   for "  TXT_SHP_MODE__DATA_RATE    "\n"  \
                                        "    [packets] for "  TXT_SHP_MODE__PACKET_RATE  "\n"

#define TXT_OPT__TTL_DECR               TXT_HELP__TTL_DECR  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__TTL_DECR          TXT_HELP__TTL_DECR  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Enable/disable TTL decrement.\n"

#define TXT_OPT__DISCARD_IF_TTL_BELOW_2       TXT_HELP__DISCARD_IF_TTL_BELOW_2  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__DISCARD_IF_TTL_BELOW_2  TXT_HELP__DISCARD_IF_TTL_BELOW_2  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                              "  Applicable only for interface modes which decrement TTL value of a packet.\n"  \
                                              "  If the packet has TTL<2, then:\n"  \
                                              "    "  TXT_ON_OFF__ON   "  : discard the packet\n"  \
                                              "    "  TXT_ON_OFF__OFF  " : send the packet to a host\n"

#define TXT_OPT__MODIFY_ACTIONS         TXT_HELP__MODIFY_ACTIONS  "=<list_of_actions>"
#define TXT_OPTDESCR__MODIFY_ACTIONS    TXT_HELP__MODIFY_ACTIONS  "=<"  TXT_MODIFY_ACTION__ADD_VLAN_HDR  ">"  "\n"  \
                                        "  Comma separated list of modify actions. Mirrored traffic is modified according to chosen actions.\n"  \
                                        "  Use empty string (\"\") to disable (clear).\n"            \
                                        "  Some actions require additional command line options.\n"  \
                                        "  Modify actions:\n"  \
                                        "    "  TXT_MODIFY_ACTION__ADD_VLAN_HDR  " ; requires <"  TXT_HELP__VLAN  ">\n"

#define TXT_OPT__WRED_QUE_IQOS          TXT_HELP__WRED_QUE  "=<queue_type>"
#define TXT_OPTDESCR__WRED_QUE_IQOS     TXT_HELP__WRED_QUE  "=<"  TXT_POL_WRED_QUE__DMEM  ">"  "\n"  \
                                        "  Queue type for Ingress QoS WRED. Available types:    \n"  \
                                        "    "  TXT_POL_WRED_QUE__DMEM  "\n"  \
                                        "    "  TXT_POL_WRED_QUE__LMEM  "\n"  \
                                        "    "  TXT_POL_WRED_QUE__RXF   "\n"

#define TXT_OPT__SHP_TYPE_IQOS          TXT_HELP__SHP_TYPE  "=<shaper_type>"
#define TXT_OPTDESCR__SHP_TYPE_IQOS     TXT_HELP__SHP_TYPE  "=<"  TXT_POL_SHP_TYPE__PORT  ">"  "\n"  \
                                        "  Shaper type for Ingress QoS shaper. Available types:    \n"  \
                                        "    "  TXT_POL_SHP_TYPE__PORT   "\n"  \
                                        "    "  TXT_POL_SHP_TYPE__BCAST  "\n"  \
                                        "    "  TXT_POL_SHP_TYPE__MCAST  "\n"

#define TXT_OPT__FLOW_ACTION_IQOS       TXT_HELP__FLOW_ACTION  "=<action>"
#define TXT_OPTDESCR__FLOW_ACTION_IQOS  TXT_HELP__FLOW_ACTION  "=<"  TXT_POL_WRED_QUE__DMEM  ">"  "\n"  \
                                        "  Action to do if the processed packet matches criteria of the given Ingress QoS flow.\n"  \
                                        "  Actions:\n"  \
                                        "    "  TXT_POL_FLOW_ACTION__RESERVED  " : packet is classified as Reserved traffic.\n"  \
                                        "    "  TXT_POL_FLOW_ACTION__MANAGED   " : packet is classified as Managed traffic.\n"  \
                                        "    "  TXT_POL_FLOW_ACTION__DROP      " : packet is dropped.\n"

#define TXT_OPT__FLOW_TYPES             TXT_HELP__FLOW_TYPES  "=<list_of_rules>"
#define TXT_OPTDESCR__FLOW_TYPES        TXT_HELP__FLOW_TYPES  "=<"  TXT_POL_FLOW_TYPE1__TYPE_ETH  ","  TXT_POL_FLOW_TYPE2__TOS  ",...>"  "\n"  \
                                        "  Comma separated list of flow types (match rules for Ingress QoS flow).\n"  \
                                        "  Use empty string (\"\") to disable (clear).\n"         \
                                        "  Some rules require additional command line options.\n" \
                                        "  Flow types:\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_ETH    "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_PPPOE  "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_ARP    "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_IP4    "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_IP6    "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_IPX    "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_MCAST  "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_BCAST  "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE1__TYPE_VLAN   "\n"  \
                                        "    "  TXT_POL_FLOW_TYPE2__VLAN        " ; requires <"  TXT_HELP__VLAN        "> and <"  TXT_HELP__VLAN_MASK      ">\n"  \
                                        "    "  TXT_POL_FLOW_TYPE2__TOS         " ; requires <"  TXT_HELP__TOS         "> and <"  TXT_HELP__TOS_MASK       ">\n"  \
                                        "    "  TXT_POL_FLOW_TYPE2__PROTOCOL    " ; requires <"  TXT_HELP__PROTOCOL    "> and <"  TXT_HELP__PROTOCOL_MASK  ">\n"  \
                                        "    "  TXT_POL_FLOW_TYPE2__SIP         " ; requires <"  TXT_HELP__SIP         "> and <"  TXT_HELP__SIP_PFX        ">\n"  \
                                        "    "  TXT_POL_FLOW_TYPE2__DIP         " ; requires <"  TXT_HELP__DIP         "> and <"  TXT_HELP__DIP_PFX        ">\n"  \
                                        "    "  TXT_POL_FLOW_TYPE2__SPORT       " ; requires <"  TXT_HELP__SPORT_MIN   "> and <"  TXT_HELP__SPORT_MAX      ">\n"  \
                                        "    "  TXT_POL_FLOW_TYPE2__DPORT       " ; requires <"  TXT_HELP__DPORT_MIN   "> and <"  TXT_HELP__DPORT_MAX      ">\n"

#define TXT_OPT__TOS                    TXT_HELP__TOS  "=<hex_value>"
#define TXT_OPTDESCR__TOS               TXT_HELP__TOS  "=<"  TXT_OPTARGS__U8_HEX  ">"  "\n"  \
                                        "  Type of Service / Traffic Class. \n"

#define TXT_OPT__SPORT_MIN              TXT_HELP__SPORT_MIN  "=<port>"
#define TXT_OPTDESCR__SPORT_MIN         TXT_HELP__SPORT_MIN  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Source port range - minimal port\n"

#define TXT_OPT__SPORT_MAX              TXT_HELP__SPORT_MAX  "=<port>"
#define TXT_OPTDESCR__SPORT_MAX         TXT_HELP__SPORT_MAX  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Source port range - maximal port\n"

#define TXT_OPT__DPORT_MIN              TXT_HELP__DPORT_MIN  "=<port>"
#define TXT_OPTDESCR__DPORT_MIN         TXT_HELP__DPORT_MIN  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Destination port range - minimal port\n"

#define TXT_OPT__DPORT_MAX              TXT_HELP__DPORT_MAX  "=<port>"
#define TXT_OPTDESCR__DPORT_MAX         TXT_HELP__DPORT_MAX  "=<"  TXT_OPTARGS__U16_DEC  ">"  "\n"  \
                                        "  Destination port range - maximal port\n"

#define TXT_OPT__VLAN_MASK              TXT_HELP__VLAN_MASK  "=<hex_value>" 
#define TXT_OPTDESCR__VLAN_MASK         TXT_HELP__VLAN_MASK  "=<"  TXT_OPTARGS__U16_HEX  ">"  "\n"  \
                                        "  A bitmask for comparison of VLAN.\n"

#define TXT_OPT__TOS_MASK               TXT_HELP__TOS_MASK  "=<hex_value>" 
#define TXT_OPTDESCR__TOS_MASK          TXT_HELP__TOS_MASK  "=<"  TXT_OPTARGS__U8_HEX  ">"  "\n"  \
                                        "  A bitmask for comparison of TOS field.\n"

#define TXT_OPT__PROTOCOL_MASK          TXT_HELP__PROTOCOL_MASK  "=<hex_value>" 
#define TXT_OPTDESCR__PROTOCOL_MASK     TXT_HELP__PROTOCOL_MASK  "=<"  TXT_OPTARGS__U8_HEX  ">"  "\n"  \
                                        "  A bitmask for comparison of PROTOCOL field.\n"

#define TXT_OPT__SIP_PFX                TXT_HELP__SIP_PFX  "=<value>" 
#define TXT_OPTDESCR__SIP_PFX           TXT_HELP__SIP_PFX  "=<0-32>"  "\n"  \
                                        "  Network prefix for SIP field.\n"

#define TXT_OPT__DIP_PFX                TXT_HELP__DIP_PFX  "=<value>" 
#define TXT_OPTDESCR__DIP_PFX           TXT_HELP__DIP_PFX  "=<0-32>"  "\n"  \
                                        "  Network prefix for DIP field.\n"

#define TXT_OPT__PTP_MGMT_IF            TXT_HELP__PTP_MGMT_IF  "=<phyif_name>"
#define TXT_OPTDESCR__PTP_MGMT_IF       TXT_HELP__PTP_MGMT_IF  "=<"  TXT_OPTARGS__PHYIF  ">"  "\n"   \
                                        "  Name of the target physical interface for PTP trafic.\n"  \
                                        "  If not used, then PTP traffic is processed the same way as any other traffic.\n"  \
                                        "  Use empty string (\"\") to disable (clear).\n"

#define TXT_OPT__LOCK                   TXT_HELP__LOCK
#define TXT_OPTDESCR__LOCK              TXT_HELP__LOCK  "\n"  \
                                        "  Request FCI ownership lock.\n"

#define TXT_OPT__UNLOCK                 TXT_HELP__UNLOCK
#define TXT_OPTDESCR__UNLOCK            TXT_HELP__UNLOCK  "\n"  \
                                        "  Release FCI ownership lock.\n"

#define TXT_OPT__PRINT_TO_TERMINAL      TXT_HELP__PRINT_TO_TERMINAL  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__PRINT_TO_TERMINAL TXT_HELP__PRINT_TO_TERMINAL  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Commands libfci_cli daemon to enable/disable printing of caught FCI events into terminal. \n"

#define TXT_OPT__PRINT_TO_LOGFILE       TXT_HELP__PRINT_TO_LOGFILE  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__PRINT_TO_LOGFILE  TXT_HELP__PRINT_TO_LOGFILE  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Commands libfci_cli daemon to enable/disable printing of caught FCI events into logfile. \n"

#define TXT_OPT__DBG_TO_TERMINAL        TXT_HELP__DBG_TO_TERMINAL  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__DBG_TO_TERMINAL   TXT_HELP__DBG_TO_TERMINAL  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Commands libfci_cli daemon to enable/disable printing of debug messages into terminal. \n"

#define TXT_OPT__DBG_TO_DBGFILE         TXT_HELP__DBG_TO_DBGFILE  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__DBG_TO_DBGFILE    TXT_HELP__DBG_TO_DBGFILE  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Commands libfci_cli daemon to enable/disable printing of debug messages into debugfile. \n"

#define TXT_OPT__ELEMENT                TXT_HELP__ELEMENT  "=<element_name>"
#define TXT_OPTDESCR__ELEMENT           TXT_HELP__ELEMENT  "=<br_accept>"  "\n"   \
                                        "  Name of the requested element.\n" 

#define TXT_OPT__ELEMENT_GROUP          TXT_HELP__ELEMENT_GROUP  "=<group_name|group_ID>"
#define TXT_OPTDESCR__ELEMENT_GROUP     TXT_HELP__ELEMENT_GROUP  "=<"  TXT_FWFEAT_EL_GROUP__CONFIG  "|2>"  "\n"   \
                                        "  Specify FW feature element group.\n"  \
                                        "  Command will search for the target FW feature element only within the specified element group.\n"  \
                                        "  Groups:\n"  \
                                        "    "  TXT_FWFEAT_EL_GROUP__DEFAULT  " : search in all available groups \n"  \
                                        "    "  TXT_FWFEAT_EL_GROUP__CONFIG   "  : search only in configuration elements group \n"  \
                                        "    "  TXT_FWFEAT_EL_GROUP__STATS    "   : search only in statistics elements group \n"

#define TXT_OPT__UNIT_SIZE              TXT_HELP__UNIT_SIZE  "=<1|2|4>"
#define TXT_OPTDESCR__UNIT_SIZE         TXT_HELP__UNIT_SIZE  "=<1|2|4>"  "\n"   \
                                        "  Bytesize of element's data unit. \n" \
                                        "  See description of FW feature elements (or libfci_cli printout) to learn \n" \
                                        "  what is the correct unit size of data for any given FW feature element. \n"
 
#define TXT_OPT__PAYLOAD                TXT_HELP__PAYLOAD  "=<list_of_values>"
#define TXT_OPTDESCR__PAYLOAD           TXT_HELP__PAYLOAD  "=<"  "0,5,20,... | 0x00,0x05,0x14..."  ">"  "\n"   \
                                        "  Comma seperated list of values. These values will serve as payload of some command.\n"

#define TXT_OPT__FF_ALL_TCP             TXT_HELP__FF_ALL_TCP  "=<"  TXT_OPTARGS__ON_OFF  ">"
#define TXT_OPTDESCR__FF_ALL_TCP        TXT_HELP__FF_ALL_TCP  "=<"  TXT_OPTARGS__ON_OFF  ">"  "\n"  \
                                        "  Enables/disables fast forwarding of ingress TCP SYN|FIN|RST packets.\n"  \
                                        "  Applicable only for interface modes which utilize routing (e.g. "  TXT_IF_MODE__ROUTER  ").\n"    \
                                        "  When trying to fast-forward a TCP SYN|FIN|RST packet, then based on state of this flag: \n"        \
                                        "    "  TXT_ON_OFF__ON   "  : fast-forward the packet as usual (according to matching conntrack)\n"  \
                                        "    "  TXT_ON_OFF__OFF  " : send the packet to a host\n"




/*
    Sanity check for opt help texts. When new opt is added, create a help text for the opt and remove its symbol from here.
    And don't forget to check the pairing! ^_^
*/
#if (\
     defined(OPT_130_TXT_HELP) || defined(OPT_131_TXT_HELP) || defined(OPT_132_TXT_HELP) || defined(OPT_133_TXT_HELP) || defined(OPT_134_TXT_HELP) || \
     defined(OPT_135_TXT_HELP) || defined(OPT_136_TXT_HELP) || defined(OPT_137_TXT_HELP) || defined(OPT_138_TXT_HELP) || defined(OPT_139_TXT_HELP) || \
     defined(OPT_140_TXT_HELP) || defined(OPT_141_TXT_HELP) || defined(OPT_142_TXT_HELP) || defined(OPT_143_TXT_HELP) || defined(OPT_144_TXT_HELP) || \
     defined(OPT_145_TXT_HELP) || defined(OPT_146_TXT_HELP) || defined(OPT_147_TXT_HELP) || defined(OPT_148_TXT_HELP) || defined(OPT_149_TXT_HELP) || \
     defined(OPT_150_TXT_HELP) || defined(OPT_151_TXT_HELP) || defined(OPT_152_TXT_HELP) || defined(OPT_153_TXT_HELP) || defined(OPT_154_TXT_HELP) || \
     defined(OPT_155_TXT_HELP) || defined(OPT_156_TXT_HELP) || defined(OPT_157_TXT_HELP) || defined(OPT_158_TXT_HELP) || defined(OPT_159_TXT_HELP) || \
     defined(OPT_160_TXT_HELP) || defined(OPT_161_TXT_HELP) || defined(OPT_162_TXT_HELP) || defined(OPT_163_TXT_HELP) || defined(OPT_164_TXT_HELP) || \
     defined(OPT_165_TXT_HELP) || defined(OPT_166_TXT_HELP) || defined(OPT_167_TXT_HELP) || defined(OPT_168_TXT_HELP) || defined(OPT_169_TXT_HELP) || \
     defined(OPT_170_TXT_HELP) || defined(OPT_171_TXT_HELP) || defined(OPT_172_TXT_HELP) || defined(OPT_173_TXT_HELP) || defined(OPT_174_TXT_HELP) || \
     defined(OPT_175_TXT_HELP) || defined(OPT_176_TXT_HELP) || defined(OPT_177_TXT_HELP) || defined(OPT_178_TXT_HELP) || defined(OPT_179_TXT_HELP) || \
     defined(OPT_180_TXT_HELP) || defined(OPT_181_TXT_HELP) || defined(OPT_182_TXT_HELP) || defined(OPT_183_TXT_HELP) || defined(OPT_184_TXT_HELP) || \
     defined(OPT_185_TXT_HELP) || defined(OPT_186_TXT_HELP) || defined(OPT_187_TXT_HELP) || defined(OPT_188_TXT_HELP) || defined(OPT_189_TXT_HELP) || \
     defined(OPT_190_TXT_HELP) || defined(OPT_191_TXT_HELP) || defined(OPT_192_TXT_HELP) || defined(OPT_193_TXT_HELP) || defined(OPT_194_TXT_HELP) || \
     defined(OPT_195_TXT_HELP) || defined(OPT_196_TXT_HELP) || defined(OPT_197_TXT_HELP) || defined(OPT_198_TXT_HELP) || defined(OPT_199_TXT_HELP))
#error Add help text for the new opt! (and remove the opt from this preprocessor check)
#endif

/* OPT_LAST (keep this at the bottom of the opt help text list) */

/* ==== TYPEDEFS & DATA : cmd help texts =================================== */

static const char *const txt_help_phyif_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] phyif-print"         "   ",
    "["   TXT_OPT__VERBOSE          "]  ",
    "\n",
    ""    "[2] phyif-print"         "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "["   TXT_OPT__VERBOSE          "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print parameters of all existing physical interfaces.",
    "\n",
    ""    "[2] Print parameters of a selected physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__VERBOSE,
    "\n",
    
    NULL
};

static const char *const txt_help_phyif_update[] =
{
    TXT_DECOR_CMD,
    ""    "phyif-update"                   "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF         ">  ",
    "[["  TXT_OPT__ENABLE_IF               "]|["  TXT_OPT__DISABLE_IF  "]]  ",
    "["   TXT_OPT__PROMISC_PHYIF           "]  ",
    "["   TXT_OPT__MODE                    "]  ",
    "["   TXT_OPT__BLOCK_STATE             "]  ",
    "["   TXT_OPT__FLEXIBLE_FILTER         "]  ",
    "["   TXT_OPT__RX_MIRROR0              "]  ",
    "["   TXT_OPT__RX_MIRROR1              "]  ",
    "["   TXT_OPT__TX_MIRROR0              "]  ",
    "["   TXT_OPT__TX_MIRROR1              "]  ",
    "["   TXT_OPT__VLAN_CONF               "]  ",
    "["   TXT_OPT__PTP_CONF                "]  ",
    "["   TXT_OPT__PTP_PROMISC             "]  ",
    "["   TXT_OPT__QINQ                    "]  ",
    "["   TXT_OPT__DISCARD_IF_TTL_BELOW_2  "]  ",
    "["   TXT_OPT__PTP_MGMT_IF             "]  ",
    "["   TXT_OPT__FF_ALL_TCP              "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of a physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__ENABLE_IF,
    TXT_OPTDESCR__DISABLE_IF,
    TXT_OPTDESCR__PROMISC_PHYIF,
    TXT_OPTDESCR__MODE,
    TXT_OPTDESCR__BLOCK_STATE,
    TXT_OPTDESCR__FLEXIBLE_FILTER,
    TXT_OPTDESCR__RX_MIRROR0,
    TXT_OPTDESCR__RX_MIRROR1,
    TXT_OPTDESCR__TX_MIRROR0,
    TXT_OPTDESCR__TX_MIRROR1,
    TXT_OPTDESCR__VLAN_CONF,
    TXT_OPTDESCR__PTP_CONF,
    TXT_OPTDESCR__PTP_PROMISC,
    TXT_OPTDESCR__QINQ,
    TXT_OPTDESCR__DISCARD_IF_TTL_BELOW_2,
    TXT_OPTDESCR__PTP_MGMT_IF,
    TXT_OPTDESCR__FF_ALL_TCP,
    "\n",
    
    NULL
};

static const char *const txt_help_phyif_mac_print[] =
{
    TXT_DECOR_CMD,
    ""    "phyif-mac-print"             "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Print MAC addresses of a physical interface.\n",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    "\n",
    
    NULL
};

static const char *const txt_help_phyif_mac_add[] =
{
    TXT_DECOR_CMD,
    ""    "phyif-mac-add"               "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "<"   TXT_OPT__MAC                  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Add MAC address to a physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__MAC,
    "\n",
    
    NULL
};

static const char *const txt_help_phyif_mac_del[] =
{
    TXT_DECOR_CMD,
    ""    "phyif-mac-del"               "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "<"   TXT_OPT__MAC                  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Delete MAC address from a physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__MAC,
    "\n",
    
    NULL
};

static const char *const txt_help_logif_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] logif-print"         "   ",
    "["   TXT_OPT__VERBOSE          "]  ",
    "\n",
    ""    "[2] logif-print"         "   ",
    "<"   TXT_OPT__INTERFACE_LOGIF  ">  ",
    "["   TXT_OPT__VERBOSE          "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print parameters of all existing logical interfaces.",
    "\n",
    ""    "[2] Print parameters of a selected logical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_LOGIF,
    TXT_OPTDESCR__VERBOSE,
    "\n",
    
    NULL
};

static const char *const txt_help_logif_update[] =
{
    TXT_DECOR_CMD,
    ""    "logif-update"             "   ",
    "<"   TXT_OPT__INTERFACE_LOGIF   ">  ",
    "[["  TXT_OPT__ENABLE_IF         "]|["  TXT_OPT__DISABLE_IF  "]]  ",
    "["   TXT_OPT__PROMISC_LOGIF     "]  ",
    "["   TXT_OPT__EGRESS            "]  ",
    "["   TXT_OPT__MATCH_MODE        "]  ",
    "["   TXT_OPT__DISCARD_ON_MATCH  "]  ",
    "["   TXT_OPT__MATCH_RULES       "]  ",
    "[<rule-specific options (only if applicable)>]",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of a logical interface."
    "\n"
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_LOGIF,
    TXT_OPTDESCR__ENABLE_IF,
    TXT_OPTDESCR__DISABLE_IF,
    TXT_OPTDESCR__PROMISC_LOGIF,
    TXT_OPTDESCR__EGRESS,
    TXT_OPTDESCR__MATCH_MODE,
    TXT_OPTDESCR__DISCARD_ON_MATCH,
    TXT_OPTDESCR__VLAN,
    TXT_OPTDESCR__PROTOCOL,
    TXT_OPTDESCR__SPORT,
    TXT_OPTDESCR__DPORT,
    TXT_OPTDESCR__SIP6,
    TXT_OPTDESCR__DIP6,
    TXT_OPTDESCR__SIP_LOGIF,
    TXT_OPTDESCR__DIP_LOGIF,
    TXT_OPTDESCR__ETHTYPE,
    TXT_OPTDESCR__FP_TABLE0_LOGIF,
    TXT_OPTDESCR__FP_TABLE1_LOGIF,
    TXT_OPTDESCR__SMAC,
    TXT_OPTDESCR__DMAC,
    TXT_OPTDESCR__HIF_COOKIE,
    TXT_OPTDESCR__MATCH_RULES,
    "\n",
    
    NULL
};

static const char *const txt_help_logif_add[] =
{
    TXT_DECOR_CMD,
    ""    "logif-add"               "   ",
    "<"   TXT_OPT__INTERFACE_LOGIF  ">  ",
    "<"   TXT_OPT__PARENT           ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Create a new logical interface."
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_LOGIF,
    TXT_OPTDESCR__PARENT,
    "\n",
    
    NULL
};

static const char *const txt_help_logif_del[] =
{
    TXT_DECOR_CMD,
    ""    "logif-del"               "   ",
    "<"   TXT_OPT__INTERFACE_LOGIF  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy (delete) the target logical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_LOGIF,
    "\n",
    
    NULL
};

static const char *const txt_help_mirror_print[] =
{
    TXT_DECOR_CMD,
    ""    "mirror-print"   "   ",
    "["   TXT_OPT__MIRROR  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print parameters of all existing mirroring rules.",
    "\n",
    ""    "[2] Print parameters of a selected mirroring rule.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__MIRROR,
    "\n",
    
    NULL
};

static const char *const txt_help_mirror_update[] =
{
    TXT_DECOR_CMD,
    ""    "mirror-update"           "   ",
    "<"   TXT_OPT__MIRROR           ">  ",
    "["   TXT_OPT__INTERFACE_PHYIF  "]  ",
    "["   TXT_OPT__FLEXIBLE_FILTER  "]  ",
    "["   TXT_OPT__MODIFY_ACTIONS   "]  ",
    "[<rule-specific options (only if applicable)>]",
    "\n",
    TXT_DECOR_DESCR,
    ""   "Update parameters of a mirroring rule.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__MIRROR,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__FLEXIBLE_FILTER,
    TXT_OPTDESCR__MODIFY_ACTIONS,
    "\n",
    
    NULL
};

static const char *const txt_help_mirror_add[] =
{
    TXT_DECOR_CMD,
    ""    "mirror-add"              "   ",
    "<"   TXT_OPT__MIRROR           ">  ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "["   TXT_OPT__FLEXIBLE_FILTER  "]  ",
    "["   TXT_OPT__MODIFY_ACTIONS   "]  ",
    "[<rule-specific options (only if applicable)>]",
    "\n",
    TXT_DECOR_DESCR,
    ""   "Create (and configure) a new mirroring rule.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__MIRROR,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__FLEXIBLE_FILTER,
    TXT_OPTDESCR__MODIFY_ACTIONS,
    "\n",
    
    NULL
};

static const char *const txt_help_mirror_del[] =
{
    TXT_DECOR_CMD,
    ""    "mirror-del"              "   ",
    "<"   TXT_OPT__MIRROR           ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy (delete) the target mirroring rule.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__MIRROR,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] bd-print"    "   ",
    "["   TXT_OPT__VERBOSE  "]  ",
    "\n",
    ""    "[2] bd-print"    "   ",
    "<"   TXT_OPT__VLAN_BD  ">  ",
    "["   TXT_OPT__VERBOSE  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print parameters of all existing bridge domains.",
    "\n",
    ""    "[2] Print parameters of a selected bridge domain.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    TXT_OPTDESCR__VERBOSE,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_update[] =
{
    TXT_DECOR_CMD,
    ""   "bd-update"          "   ",
    "<"  TXT_OPT__VLAN_BD     ">  ",
    "["  TXT_OPT__UCAST_HIT   "]  ",
    "["  TXT_OPT__UCAST_MISS  "]  ",
    "["  TXT_OPT__MCAST_HIT   "]  ",
    "["  TXT_OPT__MCAST_MISS  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""   "Update parameters of a bridge domain.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    TXT_OPTDESCR__UCAST_HIT,
    TXT_OPTDESCR__UCAST_MISS,
    TXT_OPTDESCR__MCAST_HIT,
    TXT_OPTDESCR__MCAST_MISS,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_add[] =
{
    TXT_DECOR_CMD,
    ""    "bd-add"          "   ",
    "<"   TXT_OPT__VLAN_BD  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Create a new bridge domain.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_del[] =
{
    TXT_DECOR_CMD,
    ""    "bd-del"          "   ",
    "<"   TXT_OPT__VLAN_BD  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy (delete) the target bridge domain.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_insif[] =
{
    TXT_DECOR_CMD,
    ""    "bd-insif"                "   ",
    "<"   TXT_OPT__VLAN_BD          ">  ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "["   TXT_OPT__TAG              "]  ",
    "\n",
    TXT_DECOR_DESCR
    ""    "Insert physical interface into a bridge domain.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__TAG,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_remif[] =
{
    TXT_DECOR_CMD,
    ""    "bd-remif"                "   ",
    "<"   TXT_OPT__VLAN_BD          ">  ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Remove physical interface from a bridge domain.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    "\n",
    
    NULL
};

static const char* txt_help_bd_flush[] =
{
    TXT_DECOR_CMD,
    ""    "[1] bd-flush"    "   ",
    "<"   TXT_OPT__ALL      ">  ",
    "\n",
    ""    "[2] bd-flush"    "   ",
    "<"   TXT_OPT__STATIC   ">  ",
    "\n",
    ""    "[3] bd-flush"    "   ",
    "<"   TXT_OPT__DYNAMIC  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Flush all MAC table entries of all bridge domains.",
    "\n",
    ""    "[2] Flush static MAC table entries of all bridge domains.",
    "\n",
    ""    "[3] Flush dynamic (learned) MAC table entries of all bridge domains.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__ALL,
    TXT_OPTDESCR__STATIC,
    TXT_OPTDESCR__DYNAMIC,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_stent_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] bd-stent-print"  "   ",
    "\n",
    ""    "[2] bd-stent-print"  "   ",
    "<"   TXT_OPT__VLAN_BD      ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all existing static entries (regardless of bridge domain affiliation).",
    "\n",
    ""    "[2] Print static entries associated with a particular bridge domain.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_stent_update[] =
{
    TXT_DECOR_CMD,
    ""    "bd-stent-update"              "   ",
    "<"   TXT_OPT__VLAN_BD               ">  ",
    "<"   TXT_OPT__MAC                   ">  ",
    "["   TXT_OPT__EGRESS                "]  ",
    "["   TXT_OPT__LOCAL_STENT           "]  ",
    "["   TXT_OPT__DISCARD_ON_MATCH_SRC  "]  ",
    "["   TXT_OPT__DISCARD_ON_MATCH_DST  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of a static entry.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    TXT_OPTDESCR__MAC,
    TXT_OPTDESCR__EGRESS,
    TXT_OPTDESCR__LOCAL_STENT,
    TXT_OPTDESCR__DISCARD_ON_MATCH_SRC,
    TXT_OPTDESCR__DISCARD_ON_MATCH_DST,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_stent_add[] =
{
    TXT_DECOR_CMD,
    ""    "bd-stent-add"    "   ",
    "<"   TXT_OPT__VLAN_BD  ">  ",
    "<"   TXT_OPT__MAC      ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Create a new static entry.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    TXT_OPTDESCR__MAC,
    "\n",
    
    NULL
};

static const char *const txt_help_bd_stent_del[] =
{
    TXT_DECOR_CMD,
    ""    "bd-stent-del"    "   ",
    "<"   TXT_OPT__VLAN_BD  ">  ",
    "<"   TXT_OPT__MAC      ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy (delete) the target static entry.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__VLAN_BD,
    TXT_OPTDESCR__MAC,
    "\n",
    
    NULL
};

static const char* txt_help_fptable_print[] =
{
    TXT_DECOR_CMD,
    ""    "fptable-print"          "   ",
    "<"   TXT_OPT__FP_TABLE        ">  ",
    "["   TXT_OPT__POSITION_PRINT  "]  ",
    "["   TXT_OPT__COUNT_PRINT     "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Print content of a FlexibleParser table.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FP_TABLE,
    TXT_OPTDESCR__POSITION_PRINT,
    TXT_OPTDESCR__COUNT_PRINT,
    "\n",
    
    NULL
};

static const char* txt_help_fptable_add[] =
{
    TXT_DECOR_CMD,
    ""    "fptable-add"      "   ",
    "<"   TXT_OPT__FP_TABLE  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Create a new FlexibleParser table.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FP_TABLE,
    "\n",
    
    NULL
};

static const char* txt_help_fptable_del[] =
{
    TXT_DECOR_CMD,
    ""    "fptable-del"      "   ",
    "<"   TXT_OPT__FP_TABLE  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy (delete) the target FlexibleParser table.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FP_TABLE,
    "\n",
    
    NULL
};

static const char* txt_help_fptable_insrule[] =
{
    TXT_DECOR_CMD,
    ""    "fptable-insrule"         "   ",
    "<"   TXT_OPT__FP_TABLE         ">  ",
    "<"   TXT_OPT__FP_RULE          ">  ",
    "["   TXT_OPT__POSITION_INSADD  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Insert FlexibleParser rule into a FlexibleParser table.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FP_TABLE,
    TXT_OPTDESCR__FP_RULE,
    TXT_OPTDESCR__POSITION_INSADD,
    "\n",
    
    NULL
};

static const char* txt_help_fptable_remrule[] =
{
    TXT_DECOR_CMD,
    ""    "fptable-remrule"   "   ",
    "<"   TXT_OPT__FP_TABLE   ">  ",
    "<"   TXT_OPT__FP_RULE    ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Remove FlexibleParser rule from a FlexibleParser table.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FP_TABLE,
    TXT_OPTDESCR__FP_RULE,
    "\n",
    
    NULL
};

static const char* txt_help_fprule_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] fprule-print"       "   ",
    "["   TXT_OPT__POSITION_PRINT  "]  ",
    "["   TXT_OPT__COUNT_PRINT     "]  ",
    "\n",
    ""    "[2] fprule-print"       "   ",
    "<"   TXT_OPT__FP_RULE         ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all existing FlexibleParser rules (regardless of table affiliation).",
    "\n",
    ""    "[2] Print a selected FlexibleParser rule.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FP_RULE,
    TXT_OPTDESCR__POSITION_PRINT,
    TXT_OPTDESCR__COUNT_PRINT,
    "\n",
    
    NULL
};

static const char* txt_help_fprule_add[] =
{
    TXT_DECOR_CMD,
    ""    "fprule-add"         "   ",
    "<"   TXT_OPT__FP_RULE     ">  ",
    "<"   TXT_OPT__DATA        ">  ",
    "<"   TXT_OPT__MASK        ">  ",
    "<"   TXT_OPT__OFFSET_FP   ">  ",
    "<"   TXT_OPT__LAYER       ">  ",
    "["   TXT_OPT__INVERT_FP   "]  ",
    "<<"  TXT_OPT__ACCEPT_FP   ">|<"  TXT_OPT__REJECT_FP  ">|<"  TXT_OPT__FP_NEXT_RULE  ">>  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Create a new FlexibleParser rule."
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FP_RULE,
    TXT_OPTDESCR__DATA,
    TXT_OPTDESCR__MASK,
    TXT_OPTDESCR__OFFSET_FP,
    TXT_OPTDESCR__LAYER,
    TXT_OPTDESCR__INVERT_FP,
    TXT_OPTDESCR__ACCEPT_FP,
    TXT_OPTDESCR__REJECT_FP,
    TXT_OPTDESCR__FP_NEXT_RULE,
    "\n",
    
    NULL
};

static const char* txt_help_fprule_del[] =
{
    TXT_DECOR_CMD,
    ""    "fprule-del"      "   ",
    "<"   TXT_OPT__FP_RULE  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy (delete) the target FlexibleParser rule.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FP_RULE,
    "\n",
    
    NULL
};

static const char* txt_help_route_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] route-print"  "   ",
    "\n",
    ""    "[2] route-print"  "   ",
    "<"   TXT_OPT__IP4       ">  ",
    "\n",
    ""    "[3] route-print"  "   ",
    "<"   TXT_OPT__IP6       ">  ",
    "\n",
    ""    "[4] route-print"  "   ",
    "<"   TXT_OPT__ROUTE     ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print parameters of all routes.",
    "\n",
    ""    "[2] Print parameters of all IPv4 routes.",
    "\n",
    ""    "[3] Print parameters of all IPv6 routes.",
    "\n",
    ""    "[4] Print parameters of a selected route.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__IP4,
    TXT_OPTDESCR__IP6,
    TXT_OPTDESCR__ROUTE,
    "\n",
    
    NULL
};

static const char* txt_help_route_add[] =
{
    TXT_DECOR_CMD,
    ""    "route-add"               "   ",
    "<"   TXT_OPT__ROUTE            ">  ",
    "<<"  TXT_OPT__IP4              ">|<"  TXT_OPT__IP6  ">  ",
    "<"   TXT_OPT__DMAC             ">  ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "["   TXT_OPT__SMAC             "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Create a new route.",
    "\n"
    TXT_DECOR_OPT,
    TXT_OPTDESCR__ROUTE,
    TXT_OPTDESCR__IP4,
    TXT_OPTDESCR__IP6,
    TXT_OPTDESCR__DMAC,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__SMAC,
    "\n",
    
    NULL
};

static const char* txt_help_route_del[] =
{
    TXT_DECOR_CMD,
    ""    "route-del"     "   ",
    "<"   TXT_OPT__ROUTE  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy the target route.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__ROUTE,
    "\n",
    
    NULL
};

static const char* txt_help_cntk_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] cntk_print"  "   ",
    "\n",
    ""    "[2] cntk-print"  "   ",
    "<"   TXT_OPT__IP4      ">  ",
    "\n",
    ""    "[3] cntk-print"  "   ",
    "<"   TXT_OPT__IP6      ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print parameters of all conntracks.",
    "\n",
    ""    "[2] Print parameters of all IPv4 conntracks.",
    "\n",
    ""    "[3] Print parameters of all IPv6 conntracks.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__IP4,
    TXT_OPTDESCR__IP6,
    "\n",
    
    NULL
};

static const char* txt_help_cntk_update[] =
{
    TXT_DECOR_CMD,
    ""    "cntk-update"      "   ",
    "<"   TXT_OPT__PROTOCOL  ">  ",
    "<"   TXT_OPT__SIP       ">  ",
    "<"   TXT_OPT__DIP       ">  ",
    "<"   TXT_OPT__SPORT     ">  ",
    "<"   TXT_OPT__DPORT     ">  ",
    "["   TXT_OPT__TTL_DECR  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of a conntrack. Only TTL decrement flag can be updated.\n",
    ""    "(the other parameters are used to identify the target conntrack)",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__PROTOCOL,
    TXT_OPTDESCR__SIP,
    TXT_OPTDESCR__DIP,
    TXT_OPTDESCR__SPORT,
    TXT_OPTDESCR__DPORT,
    TXT_OPTDESCR__TTL_DECR,
    "\n",
    
    NULL
};

static const char* txt_help_cntk_add[] =
{
    TXT_DECOR_CMD,
    ""    "[1] cntk-add"      "   ",
    "<"   TXT_OPT__PROTOCOL   ">  ",
    "<"   TXT_OPT__SIP        ">  ",
    "<"   TXT_OPT__DIP        ">  ",
    "<"   TXT_OPT__SPORT      ">  ",
    "<"   TXT_OPT__DPORT      ">  ",
    "<"   TXT_OPT__ROUTE      ">  ",
    "["   TXT_OPT__VLAN       "]  ",
    "["   TXT_OPT__TTL_DECR   "]  ",
    "[["  TXT_OPT__NO_REPLY   "]|["  TXT_OPT__NO_ORIG  "]  ",
    "\n",
    ""    "[2] cntk-add"      "   ",
    "<"   TXT_OPT__PROTOCOL   ">  ",
    "<"   TXT_OPT__SIP        ">  ",
    "["   TXT_OPT__R_SIP      "]  ",
    "<"   TXT_OPT__DIP        ">  ",
    "["   TXT_OPT__R_DIP      "]  ",
    "<"   TXT_OPT__SPORT      ">  ",
    "["   TXT_OPT__R_SPORT    "]  ",
    "<"   TXT_OPT__DPORT      ">  ",
    "["   TXT_OPT__R_DPORT    "]  ",
    "<"   TXT_OPT__ROUTE      ">  ",
    "["   TXT_OPT__R_ROUTE    "]  ",
    "["   TXT_OPT__VLAN       "]  ",
    "["   TXT_OPT__R_VLAN     "]  ",
    "["   TXT_OPT__TTL_DECR   "]  ",
    "[["  TXT_OPT__NO_REPLY   "]|["  TXT_OPT__NO_ORIG  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Create a new simple conntrack.\n"
    ""    "    Supplied IP addresses must be either all IPv4, or all IPv6."
    "\n",
    ""    "[2] Create a new conntrack with NAT and/or PAT.\n"
    ""    "    Supplied IP addresses must be either all IPv4, or all IPv6."
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__PROTOCOL,
    TXT_OPTDESCR__SIP,
    TXT_OPTDESCR__R_SIP,
    TXT_OPTDESCR__DIP,
    TXT_OPTDESCR__R_DIP,
    TXT_OPTDESCR__SPORT,
    TXT_OPTDESCR__R_SPORT,
    TXT_OPTDESCR__DPORT,
    TXT_OPTDESCR__R_DPORT,
    TXT_OPTDESCR__ROUTE,
    TXT_OPTDESCR__R_ROUTE,
    TXT_OPTDESCR__VLAN,
    TXT_OPTDESCR__R_VLAN,
    TXT_OPTDESCR__TTL_DECR,
    TXT_OPTDESCR__NO_REPLY,
    TXT_OPTDESCR__NO_ORIG,
    "\n",
    
    NULL
};

static const char* txt_help_cntk_del[] =
{
    TXT_DECOR_CMD,
    ""    "cntk-del"         "   ",
    "<"   TXT_OPT__PROTOCOL  ">  ",
    "<"   TXT_OPT__SIP       ">  ",
    "<"   TXT_OPT__DIP       ">  ",
    "<"   TXT_OPT__SPORT     ">  ",
    "<"   TXT_OPT__DPORT     ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy the target conntrack.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__PROTOCOL,
    TXT_OPTDESCR__SIP,
    TXT_OPTDESCR__DIP,
    TXT_OPTDESCR__SPORT,
    TXT_OPTDESCR__DPORT,
    "\n",
    
    NULL
};

static const char* txt_help_cntk_timeout[] =
{
    TXT_DECOR_CMD,
    ""    "cntk-timeout"             "   ",
    "<"   TXT_OPT__PROTOCOL_CNTKTMO  ">  ",
    "<"   TXT_OPT__TIMEOUT_CNTKTMO   ">  ",
    "\n",
    TXT_DECOR_DESCR,
    "Set timeout of conntracks.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__PROTOCOL_CNTKTMO,
    TXT_OPTDESCR__TIMEOUT_CNTKTMO,
    "\n",
    
    NULL
};

static const char* txt_help_route_and_cntk_reset[] =
{
    TXT_DECOR_CMD,
    ""    "[1] route-and-cntk-reset"  "   ",
    "<"   TXT_OPT__ALL                ">  ",
    "\n",
    ""    "[2] route-and-cntk-reset"  "   ",
    "<"   TXT_OPT__IP4                ">  ",
    "\n",
    ""    "[3] route-and-cntk-reset"  "   ",
    "<"   TXT_OPT__IP6                ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Reset (clear) all routes & conntracks.",
    "\n",
    ""    "[2] Reset (clear) only IPv4 routes & conntracks.",
    "\n",
    ""    "[3] Reset (clear) only IPv6 routes & conntracks.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__ALL,
    TXT_OPTDESCR__IP4,
    TXT_OPTDESCR__IP6,
    "\n",
    
    NULL
};

static const char* txt_help_spd_print[] =
{
    TXT_DECOR_CMD,
    ""    "spd-print"               "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "["   TXT_OPT__POSITION_PRINT   "]  ",
    "["   TXT_OPT__COUNT_PRINT      "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Print all SecurityPolicies of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__POSITION_PRINT,
    TXT_OPTDESCR__COUNT_PRINT,
    "\n",
    
    NULL
};

static const char* txt_help_spd_add[] =
{
    TXT_DECOR_CMD,
    ""    "spd-add"                 "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "<"   TXT_OPT__PROTOCOL         ">  ",
    "<"   TXT_OPT__SIP              ">  ",
    "<"   TXT_OPT__DIP              ">  ",
    "["   TXT_OPT__SPORT            "]  ",
    "["   TXT_OPT__DPORT            "]  ",
    "["   TXT_OPT__POSITION_INSADD  "]  ",
    "<"   TXT_OPT__SPD_ACTION       ">  ",
    "[<action-specific options (only if applicable)>]",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Create a new SecurityPolicy and insert it into SPD of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__PROTOCOL,
    TXT_OPTDESCR__SIP,
    TXT_OPTDESCR__DIP,
    TXT_OPTDESCR__SPORT,
    TXT_OPTDESCR__DPORT,
    TXT_OPTDESCR__POSITION_INSADD,
    TXT_OPTDESCR__SAD,
    TXT_OPTDESCR__SPI,
    TXT_OPTDESCR__SPD_ACTION,
    "\n",
    
    NULL
};

static const char* txt_help_spd_del[] =
{
    TXT_DECOR_CMD,
    ""    "spd-del"                 "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "<"   TXT_OPT__POSITION_REMDEL  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy the target SecurityPolicy and remove it from SPD of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__POSITION_REMDEL,
    "\n",
    
    NULL
};

static const char* txt_help_fwfeat_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] fwfeat-print"   "   ",
    "\n",
    ""    "[2] fwfeat-print"   "   ",
    "<"   TXT_OPT__FEATURE_FW  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all existing FW features.",
    "\n",
    ""    "[2] Print a selected FW feature.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FEATURE_FW,
    "\n",
    
    NULL
};

static const char* txt_help_fwfeat_set[] =
{
    TXT_DECOR_CMD,
    ""    "fwfeat-set"         "   ",
    "<"   TXT_OPT__FEATURE_FW  ">  ",
    "<<"  TXT_OPT__ENABLE      ">|<"  TXT_OPT__DISABLE  ">>  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Enable or disable a FW feature.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FEATURE_FW,
    TXT_OPTDESCR__ENABLE,
    TXT_OPTDESCR__DISABLE,
    "\n",
    
    NULL
};

static const char* txt_help_fwfeat_el_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] fwfeat-el-print"   "   ",
    "<"   TXT_OPT__FEATURE_FW     ">  ",
    "["   TXT_OPT__ELEMENT_GROUP  "]  ",
    "\n",
    ""    "[2] fwfeat-el-print"   "   ",
    "<"   TXT_OPT__FEATURE_FW     ">  ",
    "<"   TXT_OPT__ELEMENT        ">  ",
    "["   TXT_OPT__ELEMENT_GROUP  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all elements of a FW feature.",
    "\n",
    ""    "[2] Print the selected element of a FW feature.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FEATURE_FW,
    TXT_OPTDESCR__ELEMENT,
    TXT_OPTDESCR__ELEMENT_GROUP,
    "\n",
    
    NULL
};

static const char* txt_help_fwfeat_el_set[] =
{
    TXT_DECOR_CMD,
    ""    "fwfeat-el-set"         "   ",
    "<"   TXT_OPT__FEATURE_FW     ">  ",
    "<"   TXT_OPT__ELEMENT        ">  ",
    "<"   TXT_OPT__UNIT_SIZE      ">  ", 
    "<"   TXT_OPT__PAYLOAD        ">  ",
    "["   TXT_OPT__POSITION_FWFEAT_EL_SET "]  ",
    "["   TXT_OPT__ELEMENT_GROUP  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Set data of a FW feature element.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FEATURE_FW,
    TXT_OPTDESCR__ELEMENT,
    TXT_OPTDESCR__UNIT_SIZE,
    TXT_OPTDESCR__PAYLOAD,
    TXT_OPTDESCR__POSITION_FWFEAT_EL_SET,
    TXT_OPTDESCR__ELEMENT_GROUP,
    "\n",
    
    NULL
};

static const char* txt_help_qos_que_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] qos-que-print"       "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "\n",
    ""    "[2] qos-que-print"       "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "["   TXT_OPT__QUE              "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all QoS queues of the given physical interface.",
    "\n",
    ""    "[2] Print a selected QoS queue of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__QUE,
    "\n",
    
    NULL
};

static const char* txt_help_qos_que_update[] =
{
    TXT_DECOR_CMD,
    ""    "qos-que-update"               "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EQOS  ">  ",
    "<"   TXT_OPT__QUE                   ">  ",
    "["   TXT_OPT__QUE_MODE              "]  ",
    "["   TXT_OPT__THMIN_EQOS            "]  ",
    "["   TXT_OPT__THMAX_EQOS            "]  ",
    "["   TXT_OPT__ZPROB                 "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of a QoS queue.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EQOS,
    TXT_OPTDESCR__QUE,
    TXT_OPTDESCR__QUE_MODE,
    TXT_OPTDESCR__THMIN_EQOS,
    TXT_OPTDESCR__THMAX_EQOS,
    TXT_OPTDESCR__ZPROB,
    "\n",
    
    NULL
};

static const char* txt_help_qos_sch_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] qos-sch-print"       "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "\n",
    ""    "[2] qos-sch-print"       "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "["   TXT_OPT__SCH              "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all QoS schedulers of the given physical interface.",
    "\n",
    ""    "[2] Print a selected QoS scheduler of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__SCH,
    "\n",
    
    NULL
};

static const char* txt_help_qos_sch_update[] =
{
    TXT_DECOR_CMD,
    ""    "qos-sch-update"               "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EQOS  ">  ",
    "<"   TXT_OPT__SCH                   ">  ",
    "["   TXT_OPT__SCH_MODE              "]  ",
    "["   TXT_OPT__SCH_ALGO              "]  ",
    "["   TXT_OPT__SCH_IN                "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of a QoS scheduler.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EQOS,
    TXT_OPTDESCR__SCH,
    TXT_OPTDESCR__SCH_MODE,
    TXT_OPTDESCR__SCH_ALGO,
    TXT_OPTDESCR__SCH_IN,
    "\n",
    
    NULL
};

static const char* txt_help_qos_shp_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] qos-shp-print"       "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "\n",
    ""    "[2] qos-shp-print"       "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF  ">  ",
    "["   TXT_OPT__SHP              "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all QoS shapers of the given physical interface.",
    "\n",
    ""    "[2] Print a selected QoS shaper of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__SHP,
    "\n",
    
    NULL
};

static const char* txt_help_qos_shp_update[] =
{
    TXT_DECOR_CMD,
    ""    "qos-shp-update"               "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EQOS  ">  ",
    "<"   TXT_OPT__SHP                   ">  ",
    "["   TXT_OPT__SHP_MODE              "]  ",
    "["   TXT_OPT__SHP_POS               "]  ",
    "["   TXT_OPT__ISL                   "]  ",
    "["   TXT_OPT__CRMIN                 "]  ",
    "["   TXT_OPT__CRMAX                 "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of a QoS shaper.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EQOS,
    TXT_OPTDESCR__SHP,
    TXT_OPTDESCR__SHP_MODE,
    TXT_OPTDESCR__SHP_POS,
    TXT_OPTDESCR__ISL,
    TXT_OPTDESCR__CRMIN,
    TXT_OPTDESCR__CRMAX,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_print[] =
{
    TXT_DECOR_CMD,
    ""    "qos-pol-print"                "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Print summary of Ingress QoS policer configuration for the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_set[] =
{
    TXT_DECOR_CMD,
    ""    "qos-pol-set"                  "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "<<"  TXT_OPT__ENABLE                ">|<"  TXT_OPT__DISABLE  ">>  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Enable or disable Ingress QoS policer block.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__ENABLE,
    TXT_OPTDESCR__DISABLE,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_wred_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] qos-pol-wred-print"       "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "\n",
    ""    "[2] qos-pol-wred-print"       "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "<"   TXT_OPT__WRED_QUE_IQOS         ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all Ingress QoS wreds of the given physical interface.",
    "\n",
    ""    "[2] Print a selected Ingress QoS wred of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__WRED_QUE_IQOS,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_wred_update[] =
{
    TXT_DECOR_CMD,
    ""    "qos-pol-wred-update"          "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "<"   TXT_OPT__WRED_QUE_IQOS         ">  ",
    "[["  TXT_OPT__ENABLE                ">|<"  TXT_OPT__DISABLE  "]]  ",
    "["   TXT_OPT__THMIN_IQOS_WRED       "]  ",
    "["   TXT_OPT__THMAX_IQOS_WRED       "]  ",
    "["   TXT_OPT__THFULL_IQOS_WRED      "]  ",
    "["   TXT_OPT__ZPROB_IQOS_WRED       "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of a QoS queue.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__WRED_QUE_IQOS,
    TXT_OPTDESCR__ENABLE,
    TXT_OPTDESCR__DISABLE,
    TXT_OPTDESCR__THMIN_IQOS_WRED,
    TXT_OPTDESCR__THMAX_IQOS_WRED,
    TXT_OPTDESCR__THFULL_IQOS_WRED,
    TXT_OPTDESCR__ZPROB_IQOS_WRED,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_shp_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] qos-pol-shp-print"        "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "\n",
    ""    "[2] qos-pol-shp-print"        "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "["   TXT_OPT__SHP                   "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all Ingress QoS shapers of the given physical interface.",
    "\n",
    ""    "[2] Print a selected Ingress QoS shaper of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__SHP,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_shp_update[] =
{
    TXT_DECOR_CMD,
    ""    "qos-pol-shp-update"           "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC  ">  ",
    "<"   TXT_OPT__SHP                   ">  ",
    "["   TXT_OPT__SHP_MODE_IQOS         "]  ",
    "["   TXT_OPT__SHP_TYPE_IQOS         "]  ",
    "[["  TXT_OPT__ENABLE                ">|<"  TXT_OPT__DISABLE  "]]  ",
    "["   TXT_OPT__ISL                   "]  ",
    "["   TXT_OPT__CRMIN                 "]  ",
    "["   TXT_OPT__CRMAX                 "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Update parameters of Ingress QoS shaper.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__SHP,
    TXT_OPTDESCR__SHP_MODE_IQOS,
    TXT_OPTDESCR__SHP_TYPE_IQOS,
    TXT_OPTDESCR__ENABLE,
    TXT_OPTDESCR__DISABLE,
    TXT_OPTDESCR__ISL,
    TXT_OPTDESCR__CRMIN,
    TXT_OPTDESCR__CRMAX,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_flow_print[] =
{
    TXT_DECOR_CMD,
    ""    "[1] qos-pol-flow-print"           "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC      ">  ",
    "\n",
    ""    "[2] qos-pol-flow-print"           "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC      ">  ",
    "["   TXT_OPT__POSITION_PRINT_IQOS_FLOW  "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Print all Ingress QoS flows of the given physical interface.",
    "\n",
    ""    "[2] Print a selected Ingress QoS flow of the given physical interface.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__POSITION_PRINT_IQOS_FLOW,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_flow_add[] =
{
    TXT_DECOR_CMD,
    ""    "qos-pol-flow-add"                  "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC       ">  ",
    "["   TXT_OPT__POSITION_INSADD_IQOS_FLOW  "]  ",
    "["   TXT_OPT__FLOW_ACTION_IQOS           "]  ",
    "["   TXT_OPT__FLOW_TYPES                 "]  ",
    "[<flow-specific options (only if applicable)>]",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Create a new Ingress QoS flow.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF_EMAC,
    TXT_OPTDESCR__POSITION_INSADD_IQOS_FLOW,
    TXT_OPTDESCR__FLOW_ACTION_IQOS,
    TXT_OPTDESCR__VLAN,
    TXT_OPTDESCR__VLAN_MASK,
    TXT_OPTDESCR__TOS,
    TXT_OPTDESCR__TOS_MASK,
    TXT_OPTDESCR__PROTOCOL,
    TXT_OPTDESCR__PROTOCOL_MASK,
    TXT_OPTDESCR__SIP,
    TXT_OPTDESCR__SIP_PFX,
    TXT_OPTDESCR__DIP,
    TXT_OPTDESCR__DIP_PFX,
    TXT_OPTDESCR__SPORT_MIN,
    TXT_OPTDESCR__SPORT_MAX,
    TXT_OPTDESCR__DPORT_MIN,
    TXT_OPTDESCR__DPORT_MAX,
    TXT_OPTDESCR__FLOW_TYPES,
    "\n",
    
    NULL
};

static const char* txt_help_qos_pol_flow_del[] =
{
    TXT_DECOR_CMD,
    ""    "qos-pol-flow-del"                  "   ",
    "<"   TXT_OPT__INTERFACE_PHYIF_EMAC       ">  ",
    "<"   TXT_OPT__POSITION_REMDEL_IQOS_FLOW  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Destroy the target Ingress QoS flow.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__INTERFACE_PHYIF,
    TXT_OPTDESCR__POSITION_REMDEL_IQOS_FLOW,
    "\n",
    
    NULL
};

static const char* txt_help_fci_ownership[] =
{
    TXT_DECOR_CMD,
    ""    "[1] fci-ownership"   "   ",
    "<"   TXT_OPT__LOCK     ">  ",
    "\n",
    ""    "[2] fci-ownership"   "   ",
    "<"   TXT_OPT__UNLOCK   ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "[1] Request FCI ownership.",
    "\n",
    ""    "[2] Release FCI ownership.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__LOCK,
    TXT_OPTDESCR__UNLOCK,
    "\n",

    NULL
};

static const char* txt_help_daemon_print[] =
{
    TXT_DECOR_CMD,
    ""    "daemon-print"  "   ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Query daemon about its configuration and print it.",
    "\n",
    ""    "Daemon must be already running, otherwise this command fails.",
    "\n",
    TXT_DECOR_OPT,
    "no options\n",
    "\n",
    
    NULL
};

static const char* txt_help_daemon_update[] =
{
    TXT_DECOR_CMD,
    ""    "daemon-update"             "   ",
    "["   TXT_OPT__PRINT_TO_TERMINAL  "]  ",
    "["   TXT_OPT__PRINT_TO_LOGFILE   "]  ",
    "["   TXT_OPT__DBG_TO_TERMINAL    "]  ",
    "["   TXT_OPT__DBG_TO_DBGFILE     "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Tell daemon to change its confguration.",
    "\n",
    ""    "Daemon must be already running, otherwise this command fails.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__PRINT_TO_TERMINAL,
    TXT_OPTDESCR__PRINT_TO_LOGFILE,
    TXT_OPTDESCR__DBG_TO_TERMINAL,
    TXT_OPTDESCR__DBG_TO_DBGFILE,
    "\n",
    
    NULL
};

static const char* txt_help_daemon_start[] =
{
    TXT_DECOR_CMD,
    ""    "daemon-start"              "   ",
    "["   TXT_OPT__PRINT_TO_TERMINAL  "]  ",
    "["   TXT_OPT__PRINT_TO_LOGFILE   "]  ",
    "["   TXT_OPT__DBG_TO_TERMINAL    "]  ",
    "["   TXT_OPT__DBG_TO_DBGFILE     "]  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Start libfci_cli daemon with given configuration.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__PRINT_TO_TERMINAL,
    TXT_OPTDESCR__PRINT_TO_LOGFILE,
    TXT_OPTDESCR__DBG_TO_TERMINAL,
    TXT_OPTDESCR__DBG_TO_DBGFILE,
    "\n",
    
    NULL
};

static const char* txt_help_daemon_stop[] =
{
    TXT_DECOR_CMD,
    ""    "daemon-stop"  "   ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Stop libfci_cli daemon.",
    "\n",
    ""    "Daemon must be already running, otherwise this command fails.",
    "\n",
    TXT_DECOR_OPT,
    "no options\n",
    "\n",
    
    NULL
};

static const char* txt_help_demo_feature_print[] =
{
    TXT_DECOR_CMD,
    ""    "demo-feature-print"  "   ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Print all available demo scenarios for PFE feature configuration.",
    "\n",
    TXT_DECOR_OPT,
    "no options\n",
    "\n",
    
    NULL
};

static const char* txt_help_demo_feature_run[] =
{
    TXT_DECOR_CMD,
    ""    "demo-feature-run"     "   ",
    "<"   TXT_OPT__FEATURE_DEMO  ">  ",
    "\n",
    TXT_DECOR_DESCR,
    ""    "Run the requested demo scenario. Demo scenarios show how to configure PFE features.",
    "\n",
    TXT_DECOR_OPT,
    TXT_OPTDESCR__FEATURE_DEMO,
    "\n",
    
    NULL
};




/* CMD_LAST (keep this at the bottom of the cmd help text list) */

/* special case of text array - list all available cmds */
static const char *const txt_help_no_command[] =
{
  /* CMD_00_NO_COMMAND */
  "General help\n"
  "------------\n"
  "Run the app with '<command> --help' to get a detailed info (and a list of valid options) for the given command.\n"
  "Command list:\n",

#ifdef CMD_01_ENUM_NAME
 "  "  CMD_01_CLI_TXT  "\n",
#endif
#ifdef CMD_02_ENUM_NAME
 "  "  CMD_02_CLI_TXT  "\n",
#endif
#ifdef CMD_03_ENUM_NAME
 "  "  CMD_03_CLI_TXT  "\n",
#endif
#ifdef CMD_04_ENUM_NAME
 "  "  CMD_04_CLI_TXT  "\n",
#endif
#ifdef CMD_05_ENUM_NAME
 "  "  CMD_05_CLI_TXT  "\n",
#endif
#ifdef CMD_06_ENUM_NAME
 "  "  CMD_06_CLI_TXT  "\n",
#endif
#ifdef CMD_07_ENUM_NAME
 "  "  CMD_07_CLI_TXT  "\n",
#endif
#ifdef CMD_08_ENUM_NAME
 "  "  CMD_08_CLI_TXT  "\n",
#endif
#ifdef CMD_09_ENUM_NAME
 "  "  CMD_09_CLI_TXT  "\n",
#endif

#ifdef CMD_10_ENUM_NAME
 "  "  CMD_10_CLI_TXT  "\n",
#endif
#ifdef CMD_11_ENUM_NAME
 "  "  CMD_11_CLI_TXT  "\n",
#endif
#ifdef CMD_12_ENUM_NAME
 "  "  CMD_12_CLI_TXT  "\n",
#endif
#ifdef CMD_13_ENUM_NAME
 "  "  CMD_13_CLI_TXT  "\n",
#endif
#ifdef CMD_14_ENUM_NAME
 "  "  CMD_14_CLI_TXT  "\n",
#endif
#ifdef CMD_15_ENUM_NAME
 "  "  CMD_15_CLI_TXT  "\n",
#endif
#ifdef CMD_16_ENUM_NAME
 "  "  CMD_16_CLI_TXT  "\n",
#endif
#ifdef CMD_17_ENUM_NAME
 "  "  CMD_17_CLI_TXT  "\n",
#endif
#ifdef CMD_18_ENUM_NAME
 "  "  CMD_18_CLI_TXT  "\n",
#endif
#ifdef CMD_19_ENUM_NAME
 "  "  CMD_19_CLI_TXT  "\n",
#endif

#ifdef CMD_20_ENUM_NAME
 "  "  CMD_20_CLI_TXT  "\n",
#endif
#ifdef CMD_21_ENUM_NAME
 "  "  CMD_21_CLI_TXT  "\n",
#endif
#ifdef CMD_22_ENUM_NAME
 "  "  CMD_22_CLI_TXT  "\n",
#endif
#ifdef CMD_23_ENUM_NAME
 "  "  CMD_23_CLI_TXT  "\n",
#endif
#ifdef CMD_24_ENUM_NAME
 "  "  CMD_24_CLI_TXT  "\n",
#endif
#ifdef CMD_25_ENUM_NAME
 "  "  CMD_25_CLI_TXT  "\n",
#endif
#ifdef CMD_26_ENUM_NAME
 "  "  CMD_26_CLI_TXT  "\n",
#endif
#ifdef CMD_27_ENUM_NAME
 "  "  CMD_27_CLI_TXT  "\n",
#endif
#ifdef CMD_28_ENUM_NAME
 "  "  CMD_28_CLI_TXT  "\n",
#endif
#ifdef CMD_29_ENUM_NAME
 "  "  CMD_29_CLI_TXT  "\n",
#endif

#ifdef CMD_30_ENUM_NAME
 "  "  CMD_30_CLI_TXT  "\n",
#endif
#ifdef CMD_31_ENUM_NAME
 "  "  CMD_31_CLI_TXT  "\n",
#endif
#ifdef CMD_32_ENUM_NAME
 "  "  CMD_32_CLI_TXT  "\n",
#endif
#ifdef CMD_33_ENUM_NAME
 "  "  CMD_33_CLI_TXT  "\n",
#endif
#ifdef CMD_34_ENUM_NAME
 "  "  CMD_34_CLI_TXT  "\n",
#endif
#ifdef CMD_35_ENUM_NAME
 "  "  CMD_35_CLI_TXT  "\n",
#endif
#ifdef CMD_36_ENUM_NAME
 "  "  CMD_36_CLI_TXT  "\n",
#endif
#ifdef CMD_37_ENUM_NAME
 "  "  CMD_37_CLI_TXT  "\n",
#endif
#ifdef CMD_38_ENUM_NAME
 "  "  CMD_38_CLI_TXT  "\n",
#endif
#ifdef CMD_39_ENUM_NAME
 "  "  CMD_39_CLI_TXT  "\n",
#endif

#ifdef CMD_40_ENUM_NAME
 "  "  CMD_40_CLI_TXT  "\n",
#endif
#ifdef CMD_41_ENUM_NAME
 "  "  CMD_41_CLI_TXT  "\n",
#endif
#ifdef CMD_42_ENUM_NAME
 "  "  CMD_42_CLI_TXT  "\n",
#endif
#ifdef CMD_43_ENUM_NAME
 "  "  CMD_43_CLI_TXT  "\n",
#endif
#ifdef CMD_44_ENUM_NAME
 "  "  CMD_44_CLI_TXT  "\n",
#endif
#ifdef CMD_45_ENUM_NAME
 "  "  CMD_45_CLI_TXT  "\n",
#endif
#ifdef CMD_46_ENUM_NAME
 "  "  CMD_46_CLI_TXT  "\n",
#endif
#ifdef CMD_47_ENUM_NAME
 "  "  CMD_47_CLI_TXT  "\n",
#endif
#ifdef CMD_48_ENUM_NAME
 "  "  CMD_48_CLI_TXT  "\n",
#endif
#ifdef CMD_49_ENUM_NAME
 "  "  CMD_49_CLI_TXT  "\n",
#endif

#ifdef CMD_50_ENUM_NAME
 "  "  CMD_50_CLI_TXT  "\n",
#endif
#ifdef CMD_51_ENUM_NAME
 "  "  CMD_51_CLI_TXT  "\n",
#endif
#ifdef CMD_52_ENUM_NAME
 "  "  CMD_52_CLI_TXT  "\n",
#endif
#ifdef CMD_53_ENUM_NAME
 "  "  CMD_53_CLI_TXT  "\n",
#endif
#ifdef CMD_54_ENUM_NAME
 "  "  CMD_54_CLI_TXT  "\n",
#endif
#ifdef CMD_55_ENUM_NAME
 "  "  CMD_55_CLI_TXT  "\n",
#endif
#ifdef CMD_56_ENUM_NAME
 "  "  CMD_56_CLI_TXT  "\n",
#endif
#ifdef CMD_57_ENUM_NAME
 "  "  CMD_57_CLI_TXT  "\n",
#endif
#ifdef CMD_58_ENUM_NAME
 "  "  CMD_58_CLI_TXT  "\n",
#endif
#ifdef CMD_59_ENUM_NAME
 "  "  CMD_59_CLI_TXT  "\n",
#endif

#ifdef CMD_60_ENUM_NAME
 "  "  CMD_60_CLI_TXT  "\n",
#endif
#ifdef CMD_61_ENUM_NAME
 "  "  CMD_61_CLI_TXT  "\n",
#endif
#ifdef CMD_62_ENUM_NAME
 "  "  CMD_62_CLI_TXT  "\n",
#endif
#ifdef CMD_63_ENUM_NAME
 "  "  CMD_63_CLI_TXT  "\n",
#endif
#ifdef CMD_64_ENUM_NAME
 "  "  CMD_64_CLI_TXT  "\n",
#endif
#ifdef CMD_65_ENUM_NAME
 "  "  CMD_65_CLI_TXT  "\n",
#endif
#ifdef CMD_66_ENUM_NAME
 "  "  CMD_66_CLI_TXT  "\n",
#endif
#ifdef CMD_67_ENUM_NAME
 "  "  CMD_67_CLI_TXT  "\n",
#endif
#ifdef CMD_68_ENUM_NAME
 "  "  CMD_68_CLI_TXT  "\n",
#endif
#ifdef CMD_69_ENUM_NAME
 "  "  CMD_69_CLI_TXT  "\n",
#endif

#ifdef CMD_70_ENUM_NAME
 "  "  CMD_70_CLI_TXT  "\n",
#endif
#ifdef CMD_71_ENUM_NAME
 "  "  CMD_71_CLI_TXT  "\n",
#endif
#ifdef CMD_72_ENUM_NAME
 "  "  CMD_72_CLI_TXT  "\n",
#endif
#ifdef CMD_73_ENUM_NAME
 "  "  CMD_73_CLI_TXT  "\n",
#endif
#ifdef CMD_74_ENUM_NAME
 "  "  CMD_74_CLI_TXT  "\n",
#endif
#ifdef CMD_75_ENUM_NAME
 "  "  CMD_75_CLI_TXT  "\n",
#endif
#ifdef CMD_76_ENUM_NAME
 "  "  CMD_76_CLI_TXT  "\n",
#endif
#ifdef CMD_77_ENUM_NAME
 "  "  CMD_77_CLI_TXT  "\n",
#endif
#ifdef CMD_78_ENUM_NAME
 "  "  CMD_78_CLI_TXT  "\n",
#endif
#ifdef CMD_79_ENUM_NAME
 "  "  CMD_79_CLI_TXT  "\n",
#endif

#ifdef CMD_80_ENUM_NAME
 "  "  CMD_80_CLI_TXT  "\n",
#endif
#ifdef CMD_81_ENUM_NAME
 "  "  CMD_81_CLI_TXT  "\n",
#endif
#ifdef CMD_82_ENUM_NAME
 "  "  CMD_82_CLI_TXT  "\n",
#endif
#ifdef CMD_83_ENUM_NAME
 "  "  CMD_83_CLI_TXT  "\n",
#endif
#ifdef CMD_84_ENUM_NAME
 "  "  CMD_84_CLI_TXT  "\n",
#endif
#ifdef CMD_85_ENUM_NAME
 "  "  CMD_85_CLI_TXT  "\n",
#endif
#ifdef CMD_86_ENUM_NAME
 "  "  CMD_86_CLI_TXT  "\n",
#endif
#ifdef CMD_87_ENUM_NAME
 "  "  CMD_87_CLI_TXT  "\n",
#endif
#ifdef CMD_88_ENUM_NAME
 "  "  CMD_88_CLI_TXT  "\n",
#endif
#ifdef CMD_89_ENUM_NAME
 "  "  CMD_89_CLI_TXT  "\n",
#endif

#ifdef CMD_90_ENUM_NAME
 "  "  CMD_90_CLI_TXT  "\n",
#endif
#ifdef CMD_91_ENUM_NAME
 "  "  CMD_91_CLI_TXT  "\n",
#endif
#ifdef CMD_92_ENUM_NAME
 "  "  CMD_92_CLI_TXT  "\n",
#endif
#ifdef CMD_93_ENUM_NAME
 "  "  CMD_93_CLI_TXT  "\n",
#endif
#ifdef CMD_94_ENUM_NAME
 "  "  CMD_94_CLI_TXT  "\n",
#endif
#ifdef CMD_95_ENUM_NAME
 "  "  CMD_95_CLI_TXT  "\n",
#endif
#ifdef CMD_96_ENUM_NAME
 "  "  CMD_96_CLI_TXT  "\n",
#endif
#ifdef CMD_97_ENUM_NAME
 "  "  CMD_97_CLI_TXT  "\n",
#endif
#ifdef CMD_98_ENUM_NAME
 "  "  CMD_98_CLI_TXT  "\n",
#endif
#ifdef CMD_99_ENUM_NAME
 "  "  CMD_99_CLI_TXT  "\n",
#endif

#ifdef CMD_100_ENUM_NAME
 "  "  CMD_100_CLI_TXT  "\n",
#endif
#ifdef CMD_101_ENUM_NAME
 "  "  CMD_101_CLI_TXT  "\n",
#endif
#ifdef CMD_102_ENUM_NAME
 "  "  CMD_102_CLI_TXT  "\n",
#endif
#ifdef CMD_103_ENUM_NAME
 "  "  CMD_103_CLI_TXT  "\n",
#endif
#ifdef CMD_104_ENUM_NAME
 "  "  CMD_104_CLI_TXT  "\n",
#endif
#ifdef CMD_105_ENUM_NAME
 "  "  CMD_105_CLI_TXT  "\n",
#endif
#ifdef CMD_106_ENUM_NAME
 "  "  CMD_106_CLI_TXT  "\n",
#endif
#ifdef CMD_107_ENUM_NAME
 "  "  CMD_107_CLI_TXT  "\n",
#endif
#ifdef CMD_108_ENUM_NAME
 "  "  CMD_108_CLI_TXT  "\n",
#endif
#ifdef CMD_109_ENUM_NAME
 "  "  CMD_109_CLI_TXT  "\n",
#endif

#ifdef CMD_110_ENUM_NAME
 "  "  CMD_110_CLI_TXT  "\n",
#endif
#ifdef CMD_111_ENUM_NAME
 "  "  CMD_111_CLI_TXT  "\n",
#endif
#ifdef CMD_112_ENUM_NAME
 "  "  CMD_112_CLI_TXT  "\n",
#endif
#ifdef CMD_113_ENUM_NAME
 "  "  CMD_113_CLI_TXT  "\n",
#endif
#ifdef CMD_114_ENUM_NAME
 "  "  CMD_114_CLI_TXT  "\n",
#endif
#ifdef CMD_115_ENUM_NAME
 "  "  CMD_115_CLI_TXT  "\n",
#endif
#ifdef CMD_116_ENUM_NAME
 "  "  CMD_116_CLI_TXT  "\n",
#endif
#ifdef CMD_117_ENUM_NAME
 "  "  CMD_117_CLI_TXT  "\n",
#endif
#ifdef CMD_118_ENUM_NAME
 "  "  CMD_118_CLI_TXT  "\n",
#endif
#ifdef CMD_119_ENUM_NAME
 "  "  CMD_119_CLI_TXT  "\n",
#endif

#ifdef CMD_120_ENUM_NAME
 "  "  CMD_120_CLI_TXT  "\n",
#endif
#ifdef CMD_121_ENUM_NAME
 "  "  CMD_121_CLI_TXT  "\n",
#endif
#ifdef CMD_122_ENUM_NAME
 "  "  CMD_122_CLI_TXT  "\n",
#endif
#ifdef CMD_123_ENUM_NAME
 "  "  CMD_123_CLI_TXT  "\n",
#endif
#ifdef CMD_124_ENUM_NAME
 "  "  CMD_124_CLI_TXT  "\n",
#endif
#ifdef CMD_125_ENUM_NAME
 "  "  CMD_125_CLI_TXT  "\n",
#endif
#ifdef CMD_126_ENUM_NAME
 "  "  CMD_126_CLI_TXT  "\n",
#endif
#ifdef CMD_127_ENUM_NAME
 "  "  CMD_127_CLI_TXT  "\n",
#endif
#ifdef CMD_128_ENUM_NAME
 "  "  CMD_128_CLI_TXT  "\n",
#endif
#ifdef CMD_129_ENUM_NAME
 "  "  CMD_129_CLI_TXT  "\n",
#endif

#ifdef CMD_130_ENUM_NAME
 "  "  CMD_130_CLI_TXT  "\n",
#endif
#ifdef CMD_131_ENUM_NAME
 "  "  CMD_131_CLI_TXT  "\n",
#endif
#ifdef CMD_132_ENUM_NAME
 "  "  CMD_132_CLI_TXT  "\n",
#endif
#ifdef CMD_133_ENUM_NAME
 "  "  CMD_133_CLI_TXT  "\n",
#endif
#ifdef CMD_134_ENUM_NAME
 "  "  CMD_134_CLI_TXT  "\n",
#endif
#ifdef CMD_135_ENUM_NAME
 "  "  CMD_135_CLI_TXT  "\n",
#endif
#ifdef CMD_136_ENUM_NAME
 "  "  CMD_136_CLI_TXT  "\n",
#endif
#ifdef CMD_137_ENUM_NAME
 "  "  CMD_137_CLI_TXT  "\n",
#endif
#ifdef CMD_138_ENUM_NAME
 "  "  CMD_138_CLI_TXT  "\n",
#endif
#ifdef CMD_139_ENUM_NAME
 "  "  CMD_139_CLI_TXT  "\n",
#endif

#ifdef CMD_140_ENUM_NAME
 "  "  CMD_140_CLI_TXT  "\n",
#endif
#ifdef CMD_141_ENUM_NAME
 "  "  CMD_141_CLI_TXT  "\n",
#endif
#ifdef CMD_142_ENUM_NAME
 "  "  CMD_142_CLI_TXT  "\n",
#endif
#ifdef CMD_143_ENUM_NAME
 "  "  CMD_143_CLI_TXT  "\n",
#endif
#ifdef CMD_144_ENUM_NAME
 "  "  CMD_144_CLI_TXT  "\n",
#endif
#ifdef CMD_145_ENUM_NAME
 "  "  CMD_145_CLI_TXT  "\n",
#endif
#ifdef CMD_146_ENUM_NAME
 "  "  CMD_146_CLI_TXT  "\n",
#endif
#ifdef CMD_147_ENUM_NAME
 "  "  CMD_147_CLI_TXT  "\n",
#endif
#ifdef CMD_148_ENUM_NAME
 "  "  CMD_148_CLI_TXT  "\n",
#endif
#ifdef CMD_149_ENUM_NAME
 "  "  CMD_149_CLI_TXT  "\n",
#endif

#ifdef CMD_150_ENUM_NAME
 "  "  CMD_150_CLI_TXT  "\n",
#endif
#ifdef CMD_151_ENUM_NAME
 "  "  CMD_151_CLI_TXT  "\n",
#endif
#ifdef CMD_152_ENUM_NAME
 "  "  CMD_152_CLI_TXT  "\n",
#endif
#ifdef CMD_153_ENUM_NAME
 "  "  CMD_153_CLI_TXT  "\n",
#endif
#ifdef CMD_154_ENUM_NAME
 "  "  CMD_154_CLI_TXT  "\n",
#endif
#ifdef CMD_155_ENUM_NAME
 "  "  CMD_155_CLI_TXT  "\n",
#endif
#ifdef CMD_156_ENUM_NAME
 "  "  CMD_156_CLI_TXT  "\n",
#endif
#ifdef CMD_157_ENUM_NAME
 "  "  CMD_157_CLI_TXT  "\n",
#endif
#ifdef CMD_158_ENUM_NAME
 "  "  CMD_158_CLI_TXT  "\n",
#endif
#ifdef CMD_159_ENUM_NAME
 "  "  CMD_159_CLI_TXT  "\n",
#endif

#ifdef CMD_160_ENUM_NAME
 "  "  CMD_160_CLI_TXT  "\n",
#endif
#ifdef CMD_161_ENUM_NAME
 "  "  CMD_161_CLI_TXT  "\n",
#endif
#ifdef CMD_162_ENUM_NAME
 "  "  CMD_162_CLI_TXT  "\n",
#endif
#ifdef CMD_163_ENUM_NAME
 "  "  CMD_163_CLI_TXT  "\n",
#endif
#ifdef CMD_164_ENUM_NAME
 "  "  CMD_164_CLI_TXT  "\n",
#endif
#ifdef CMD_165_ENUM_NAME
 "  "  CMD_165_CLI_TXT  "\n",
#endif
#ifdef CMD_166_ENUM_NAME
 "  "  CMD_166_CLI_TXT  "\n",
#endif
#ifdef CMD_167_ENUM_NAME
 "  "  CMD_167_CLI_TXT  "\n",
#endif
#ifdef CMD_168_ENUM_NAME
 "  "  CMD_168_CLI_TXT  "\n",
#endif
#ifdef CMD_169_ENUM_NAME
 "  "  CMD_169_CLI_TXT  "\n",
#endif

#ifdef CMD_170_ENUM_NAME
 "  "  CMD_170_CLI_TXT  "\n",
#endif
#ifdef CMD_171_ENUM_NAME
 "  "  CMD_171_CLI_TXT  "\n",
#endif
#ifdef CMD_172_ENUM_NAME
 "  "  CMD_172_CLI_TXT  "\n",
#endif
#ifdef CMD_173_ENUM_NAME
 "  "  CMD_173_CLI_TXT  "\n",
#endif
#ifdef CMD_174_ENUM_NAME
 "  "  CMD_174_CLI_TXT  "\n",
#endif
#ifdef CMD_175_ENUM_NAME
 "  "  CMD_175_CLI_TXT  "\n",
#endif
#ifdef CMD_176_ENUM_NAME
 "  "  CMD_176_CLI_TXT  "\n",
#endif
#ifdef CMD_177_ENUM_NAME
 "  "  CMD_177_CLI_TXT  "\n",
#endif
#ifdef CMD_178_ENUM_NAME
 "  "  CMD_178_CLI_TXT  "\n",
#endif
#ifdef CMD_179_ENUM_NAME
 "  "  CMD_179_CLI_TXT  "\n",
#endif

#ifdef CMD_180_ENUM_NAME
 "  "  CMD_180_CLI_TXT  "\n",
#endif
#ifdef CMD_181_ENUM_NAME
 "  "  CMD_181_CLI_TXT  "\n",
#endif
#ifdef CMD_182_ENUM_NAME
 "  "  CMD_182_CLI_TXT  "\n",
#endif
#ifdef CMD_183_ENUM_NAME
 "  "  CMD_183_CLI_TXT  "\n",
#endif
#ifdef CMD_184_ENUM_NAME
 "  "  CMD_184_CLI_TXT  "\n",
#endif
#ifdef CMD_185_ENUM_NAME
 "  "  CMD_185_CLI_TXT  "\n",
#endif
#ifdef CMD_186_ENUM_NAME
 "  "  CMD_186_CLI_TXT  "\n",
#endif
#ifdef CMD_187_ENUM_NAME
 "  "  CMD_187_CLI_TXT  "\n",
#endif
#ifdef CMD_188_ENUM_NAME
 "  "  CMD_188_CLI_TXT  "\n",
#endif
#ifdef CMD_189_ENUM_NAME
 "  "  CMD_189_CLI_TXT  "\n",
#endif

#ifdef CMD_190_ENUM_NAME
 "  "  CMD_190_CLI_TXT  "\n",
#endif
#ifdef CMD_191_ENUM_NAME
 "  "  CMD_191_CLI_TXT  "\n",
#endif
#ifdef CMD_192_ENUM_NAME
 "  "  CMD_192_CLI_TXT  "\n",
#endif
#ifdef CMD_193_ENUM_NAME
 "  "  CMD_193_CLI_TXT  "\n",
#endif
#ifdef CMD_194_ENUM_NAME
 "  "  CMD_194_CLI_TXT  "\n",
#endif
#ifdef CMD_195_ENUM_NAME
 "  "  CMD_195_CLI_TXT  "\n",
#endif
#ifdef CMD_196_ENUM_NAME
 "  "  CMD_196_CLI_TXT  "\n",
#endif
#ifdef CMD_197_ENUM_NAME
 "  "  CMD_197_CLI_TXT  "\n",
#endif
#ifdef CMD_198_ENUM_NAME
 "  "  CMD_198_CLI_TXT  "\n",
#endif
#ifdef CMD_199_ENUM_NAME
 "  "  CMD_199_CLI_TXT  "\n",
#endif

    "\n",
    NULL
};


/* array of help texts (indexed by 'cmd_t') */
static const char *const *const txt_helps[] = 
{
    txt_help_no_command,  /* CMD_00_NO_COMMAND */
    
#ifdef CMD_01_ENUM_NAME
       CMD_01_HELP,
#endif
#ifdef CMD_02_ENUM_NAME
       CMD_02_HELP,
#endif
#ifdef CMD_03_ENUM_NAME
       CMD_03_HELP,
#endif
#ifdef CMD_04_ENUM_NAME
       CMD_04_HELP,
#endif
#ifdef CMD_05_ENUM_NAME
       CMD_05_HELP,
#endif
#ifdef CMD_06_ENUM_NAME
       CMD_06_HELP,
#endif
#ifdef CMD_07_ENUM_NAME
       CMD_07_HELP,
#endif
#ifdef CMD_08_ENUM_NAME
       CMD_08_HELP,
#endif
#ifdef CMD_09_ENUM_NAME
       CMD_09_HELP,
#endif

#ifdef CMD_10_ENUM_NAME
       CMD_10_HELP,
#endif
#ifdef CMD_11_ENUM_NAME
       CMD_11_HELP,
#endif
#ifdef CMD_12_ENUM_NAME
       CMD_12_HELP,
#endif
#ifdef CMD_13_ENUM_NAME
       CMD_13_HELP,
#endif
#ifdef CMD_14_ENUM_NAME
       CMD_14_HELP,
#endif
#ifdef CMD_15_ENUM_NAME
       CMD_15_HELP,
#endif
#ifdef CMD_16_ENUM_NAME
       CMD_16_HELP,
#endif
#ifdef CMD_17_ENUM_NAME
       CMD_17_HELP,
#endif
#ifdef CMD_18_ENUM_NAME
       CMD_18_HELP,
#endif
#ifdef CMD_19_ENUM_NAME
       CMD_19_HELP,
#endif

#ifdef CMD_20_ENUM_NAME
       CMD_20_HELP,
#endif
#ifdef CMD_21_ENUM_NAME
       CMD_21_HELP,
#endif
#ifdef CMD_22_ENUM_NAME
       CMD_22_HELP,
#endif
#ifdef CMD_23_ENUM_NAME
       CMD_23_HELP,
#endif
#ifdef CMD_24_ENUM_NAME
       CMD_24_HELP,
#endif
#ifdef CMD_25_ENUM_NAME
       CMD_25_HELP,
#endif
#ifdef CMD_26_ENUM_NAME
       CMD_26_HELP,
#endif
#ifdef CMD_27_ENUM_NAME
       CMD_27_HELP,
#endif
#ifdef CMD_28_ENUM_NAME
       CMD_28_HELP,
#endif
#ifdef CMD_29_ENUM_NAME
       CMD_29_HELP,
#endif

#ifdef CMD_30_ENUM_NAME
       CMD_30_HELP,
#endif
#ifdef CMD_31_ENUM_NAME
       CMD_31_HELP,
#endif
#ifdef CMD_32_ENUM_NAME
       CMD_32_HELP,
#endif
#ifdef CMD_33_ENUM_NAME
       CMD_33_HELP,
#endif
#ifdef CMD_34_ENUM_NAME
       CMD_34_HELP,
#endif
#ifdef CMD_35_ENUM_NAME
       CMD_35_HELP,
#endif
#ifdef CMD_36_ENUM_NAME
       CMD_36_HELP,
#endif
#ifdef CMD_37_ENUM_NAME
       CMD_37_HELP,
#endif
#ifdef CMD_38_ENUM_NAME
       CMD_38_HELP,
#endif
#ifdef CMD_39_ENUM_NAME
       CMD_39_HELP,
#endif

#ifdef CMD_40_ENUM_NAME
       CMD_40_HELP,
#endif
#ifdef CMD_41_ENUM_NAME
       CMD_41_HELP,
#endif
#ifdef CMD_42_ENUM_NAME
       CMD_42_HELP,
#endif
#ifdef CMD_43_ENUM_NAME
       CMD_43_HELP,
#endif
#ifdef CMD_44_ENUM_NAME
       CMD_44_HELP,
#endif
#ifdef CMD_45_ENUM_NAME
       CMD_45_HELP,
#endif
#ifdef CMD_46_ENUM_NAME
       CMD_46_HELP,
#endif
#ifdef CMD_47_ENUM_NAME
       CMD_47_HELP,
#endif
#ifdef CMD_48_ENUM_NAME
       CMD_48_HELP,
#endif
#ifdef CMD_49_ENUM_NAME
       CMD_49_HELP,
#endif

#ifdef CMD_50_ENUM_NAME
       CMD_50_HELP,
#endif
#ifdef CMD_51_ENUM_NAME
       CMD_51_HELP,
#endif
#ifdef CMD_52_ENUM_NAME
       CMD_52_HELP,
#endif
#ifdef CMD_53_ENUM_NAME
       CMD_53_HELP,
#endif
#ifdef CMD_54_ENUM_NAME
       CMD_54_HELP,
#endif
#ifdef CMD_55_ENUM_NAME
       CMD_55_HELP,
#endif
#ifdef CMD_56_ENUM_NAME
       CMD_56_HELP,
#endif
#ifdef CMD_57_ENUM_NAME
       CMD_57_HELP,
#endif
#ifdef CMD_58_ENUM_NAME
       CMD_58_HELP,
#endif
#ifdef CMD_59_ENUM_NAME
       CMD_59_HELP,
#endif

#ifdef CMD_60_ENUM_NAME
       CMD_60_HELP,
#endif
#ifdef CMD_61_ENUM_NAME
       CMD_61_HELP,
#endif
#ifdef CMD_62_ENUM_NAME
       CMD_62_HELP,
#endif
#ifdef CMD_63_ENUM_NAME
       CMD_63_HELP,
#endif
#ifdef CMD_64_ENUM_NAME
       CMD_64_HELP,
#endif
#ifdef CMD_65_ENUM_NAME
       CMD_65_HELP,
#endif
#ifdef CMD_66_ENUM_NAME
       CMD_66_HELP,
#endif
#ifdef CMD_67_ENUM_NAME
       CMD_67_HELP,
#endif
#ifdef CMD_68_ENUM_NAME
       CMD_68_HELP,
#endif
#ifdef CMD_69_ENUM_NAME
       CMD_69_HELP,
#endif

#ifdef CMD_70_ENUM_NAME
       CMD_70_HELP,
#endif
#ifdef CMD_71_ENUM_NAME
       CMD_71_HELP,
#endif
#ifdef CMD_72_ENUM_NAME
       CMD_72_HELP,
#endif
#ifdef CMD_73_ENUM_NAME
       CMD_73_HELP,
#endif
#ifdef CMD_74_ENUM_NAME
       CMD_74_HELP,
#endif
#ifdef CMD_75_ENUM_NAME
       CMD_75_HELP,
#endif
#ifdef CMD_76_ENUM_NAME
       CMD_76_HELP,
#endif
#ifdef CMD_77_ENUM_NAME
       CMD_77_HELP,
#endif
#ifdef CMD_78_ENUM_NAME
       CMD_78_HELP,
#endif
#ifdef CMD_79_ENUM_NAME
       CMD_79_HELP,
#endif

#ifdef CMD_80_ENUM_NAME
       CMD_80_HELP,
#endif
#ifdef CMD_81_ENUM_NAME
       CMD_81_HELP,
#endif
#ifdef CMD_82_ENUM_NAME
       CMD_82_HELP,
#endif
#ifdef CMD_83_ENUM_NAME
       CMD_83_HELP,
#endif
#ifdef CMD_84_ENUM_NAME
       CMD_84_HELP,
#endif
#ifdef CMD_85_ENUM_NAME
       CMD_85_HELP,
#endif
#ifdef CMD_86_ENUM_NAME
       CMD_86_HELP,
#endif
#ifdef CMD_87_ENUM_NAME
       CMD_87_HELP,
#endif
#ifdef CMD_88_ENUM_NAME
       CMD_88_HELP,
#endif
#ifdef CMD_89_ENUM_NAME
       CMD_89_HELP,
#endif

#ifdef CMD_90_ENUM_NAME
       CMD_90_HELP,
#endif
#ifdef CMD_91_ENUM_NAME
       CMD_91_HELP,
#endif
#ifdef CMD_92_ENUM_NAME
       CMD_92_HELP,
#endif
#ifdef CMD_93_ENUM_NAME
       CMD_93_HELP,
#endif
#ifdef CMD_94_ENUM_NAME
       CMD_94_HELP,
#endif
#ifdef CMD_95_ENUM_NAME
       CMD_95_HELP,
#endif
#ifdef CMD_96_ENUM_NAME
       CMD_96_HELP,
#endif
#ifdef CMD_97_ENUM_NAME
       CMD_97_HELP,
#endif
#ifdef CMD_98_ENUM_NAME
       CMD_98_HELP,
#endif
#ifdef CMD_99_ENUM_NAME
       CMD_99_HELP,
#endif

#ifdef CMD_100_ENUM_NAME
       CMD_100_HELP,
#endif
#ifdef CMD_101_ENUM_NAME
       CMD_101_HELP,
#endif
#ifdef CMD_102_ENUM_NAME
       CMD_102_HELP,
#endif
#ifdef CMD_103_ENUM_NAME
       CMD_103_HELP,
#endif
#ifdef CMD_104_ENUM_NAME
       CMD_104_HELP,
#endif
#ifdef CMD_105_ENUM_NAME
       CMD_105_HELP,
#endif
#ifdef CMD_106_ENUM_NAME
       CMD_106_HELP,
#endif
#ifdef CMD_107_ENUM_NAME
       CMD_107_HELP,
#endif
#ifdef CMD_108_ENUM_NAME
       CMD_108_HELP,
#endif
#ifdef CMD_109_ENUM_NAME
       CMD_109_HELP,
#endif

#ifdef CMD_110_ENUM_NAME
       CMD_110_HELP,
#endif
#ifdef CMD_111_ENUM_NAME
       CMD_111_HELP,
#endif
#ifdef CMD_112_ENUM_NAME
       CMD_112_HELP,
#endif
#ifdef CMD_113_ENUM_NAME
       CMD_113_HELP,
#endif
#ifdef CMD_114_ENUM_NAME
       CMD_114_HELP,
#endif
#ifdef CMD_115_ENUM_NAME
       CMD_115_HELP,
#endif
#ifdef CMD_116_ENUM_NAME
       CMD_116_HELP,
#endif
#ifdef CMD_117_ENUM_NAME
       CMD_117_HELP,
#endif
#ifdef CMD_118_ENUM_NAME
       CMD_118_HELP,
#endif
#ifdef CMD_119_ENUM_NAME
       CMD_119_HELP,
#endif

#ifdef CMD_120_ENUM_NAME
       CMD_120_HELP,
#endif
#ifdef CMD_121_ENUM_NAME
       CMD_121_HELP,
#endif
#ifdef CMD_122_ENUM_NAME
       CMD_122_HELP,
#endif
#ifdef CMD_123_ENUM_NAME
       CMD_123_HELP,
#endif
#ifdef CMD_124_ENUM_NAME
       CMD_124_HELP,
#endif
#ifdef CMD_125_ENUM_NAME
       CMD_125_HELP,
#endif
#ifdef CMD_126_ENUM_NAME
       CMD_126_HELP,
#endif
#ifdef CMD_127_ENUM_NAME
       CMD_127_HELP,
#endif
#ifdef CMD_128_ENUM_NAME
       CMD_128_HELP,
#endif
#ifdef CMD_129_ENUM_NAME
       CMD_129_HELP,
#endif

#ifdef CMD_130_ENUM_NAME
       CMD_130_HELP,
#endif
#ifdef CMD_131_ENUM_NAME
       CMD_131_HELP,
#endif
#ifdef CMD_132_ENUM_NAME
       CMD_132_HELP,
#endif
#ifdef CMD_133_ENUM_NAME
       CMD_133_HELP,
#endif
#ifdef CMD_134_ENUM_NAME
       CMD_134_HELP,
#endif
#ifdef CMD_135_ENUM_NAME
       CMD_135_HELP,
#endif
#ifdef CMD_136_ENUM_NAME
       CMD_136_HELP,
#endif
#ifdef CMD_137_ENUM_NAME
       CMD_137_HELP,
#endif
#ifdef CMD_138_ENUM_NAME
       CMD_138_HELP,
#endif
#ifdef CMD_139_ENUM_NAME
       CMD_139_HELP,
#endif

#ifdef CMD_140_ENUM_NAME
       CMD_140_HELP,
#endif
#ifdef CMD_141_ENUM_NAME
       CMD_141_HELP,
#endif
#ifdef CMD_142_ENUM_NAME
       CMD_142_HELP,
#endif
#ifdef CMD_143_ENUM_NAME
       CMD_143_HELP,
#endif
#ifdef CMD_144_ENUM_NAME
       CMD_144_HELP,
#endif
#ifdef CMD_145_ENUM_NAME
       CMD_145_HELP,
#endif
#ifdef CMD_146_ENUM_NAME
       CMD_146_HELP,
#endif
#ifdef CMD_147_ENUM_NAME
       CMD_147_HELP,
#endif
#ifdef CMD_148_ENUM_NAME
       CMD_148_HELP,
#endif
#ifdef CMD_149_ENUM_NAME
       CMD_149_HELP,
#endif

#ifdef CMD_150_ENUM_NAME
       CMD_150_HELP,
#endif
#ifdef CMD_151_ENUM_NAME
       CMD_151_HELP,
#endif
#ifdef CMD_152_ENUM_NAME
       CMD_152_HELP,
#endif
#ifdef CMD_153_ENUM_NAME
       CMD_153_HELP,
#endif
#ifdef CMD_154_ENUM_NAME
       CMD_154_HELP,
#endif
#ifdef CMD_155_ENUM_NAME
       CMD_155_HELP,
#endif
#ifdef CMD_156_ENUM_NAME
       CMD_156_HELP,
#endif
#ifdef CMD_157_ENUM_NAME
       CMD_157_HELP,
#endif
#ifdef CMD_158_ENUM_NAME
       CMD_158_HELP,
#endif
#ifdef CMD_159_ENUM_NAME
       CMD_159_HELP,
#endif

#ifdef CMD_160_ENUM_NAME
       CMD_160_HELP,
#endif
#ifdef CMD_161_ENUM_NAME
       CMD_161_HELP,
#endif
#ifdef CMD_162_ENUM_NAME
       CMD_162_HELP,
#endif
#ifdef CMD_163_ENUM_NAME
       CMD_163_HELP,
#endif
#ifdef CMD_164_ENUM_NAME
       CMD_164_HELP,
#endif
#ifdef CMD_165_ENUM_NAME
       CMD_165_HELP,
#endif
#ifdef CMD_166_ENUM_NAME
       CMD_166_HELP,
#endif
#ifdef CMD_167_ENUM_NAME
       CMD_167_HELP,
#endif
#ifdef CMD_168_ENUM_NAME
       CMD_168_HELP,
#endif
#ifdef CMD_169_ENUM_NAME
       CMD_169_HELP,
#endif

#ifdef CMD_170_ENUM_NAME
       CMD_170_HELP,
#endif
#ifdef CMD_171_ENUM_NAME
       CMD_171_HELP,
#endif
#ifdef CMD_172_ENUM_NAME
       CMD_172_HELP,
#endif
#ifdef CMD_173_ENUM_NAME
       CMD_173_HELP,
#endif
#ifdef CMD_174_ENUM_NAME
       CMD_174_HELP,
#endif
#ifdef CMD_175_ENUM_NAME
       CMD_175_HELP,
#endif
#ifdef CMD_176_ENUM_NAME
       CMD_176_HELP,
#endif
#ifdef CMD_177_ENUM_NAME
       CMD_177_HELP,
#endif
#ifdef CMD_178_ENUM_NAME
       CMD_178_HELP,
#endif
#ifdef CMD_179_ENUM_NAME
       CMD_179_HELP,
#endif

#ifdef CMD_180_ENUM_NAME
       CMD_180_HELP,
#endif
#ifdef CMD_181_ENUM_NAME
       CMD_181_HELP,
#endif
#ifdef CMD_182_ENUM_NAME
       CMD_182_HELP,
#endif
#ifdef CMD_183_ENUM_NAME
       CMD_183_HELP,
#endif
#ifdef CMD_184_ENUM_NAME
       CMD_184_HELP,
#endif
#ifdef CMD_185_ENUM_NAME
       CMD_185_HELP,
#endif
#ifdef CMD_186_ENUM_NAME
       CMD_186_HELP,
#endif
#ifdef CMD_187_ENUM_NAME
       CMD_187_HELP,
#endif
#ifdef CMD_188_ENUM_NAME
       CMD_188_HELP,
#endif
#ifdef CMD_189_ENUM_NAME
       CMD_189_HELP,
#endif

#ifdef CMD_190_ENUM_NAME
       CMD_190_HELP,
#endif
#ifdef CMD_191_ENUM_NAME
       CMD_191_HELP,
#endif
#ifdef CMD_192_ENUM_NAME
       CMD_192_HELP,
#endif
#ifdef CMD_193_ENUM_NAME
       CMD_193_HELP,
#endif
#ifdef CMD_194_ENUM_NAME
       CMD_194_HELP,
#endif
#ifdef CMD_195_ENUM_NAME
       CMD_195_HELP,
#endif
#ifdef CMD_196_ENUM_NAME
       CMD_196_HELP,
#endif
#ifdef CMD_197_ENUM_NAME
       CMD_197_HELP,
#endif
#ifdef CMD_198_ENUM_NAME
       CMD_198_HELP,
#endif
#ifdef CMD_199_ENUM_NAME
       CMD_199_HELP,
#endif
};

/* ==== PUBLIC FUNCTIONS =================================================== */

void cli_print_help(uint16_t cmd)
{
    const char *const p_txt_invalid[] =
    {
        "__INVALID_ITEM__",
        NULL        
    };
    
    const char *const *const p_txt_help = (cli_cmd_is_not_valid(cmd) ? (p_txt_invalid) : (txt_helps[cmd]));
    uint16_t i = UINT16_MAX;
    while ((128u > (++i)) && (NULL != p_txt_help[i]))  /* limits of 128 is just to prevent endless loop in case of some problem */
    {
        printf("%s", p_txt_help[i]);
    }
}

/* ==== TESTMODE constants ================================================= */

#if !defined(NDEBUG)
/* empty */
#endif

/* ========================================================================= */
