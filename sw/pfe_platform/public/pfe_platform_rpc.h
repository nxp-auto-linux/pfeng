/* =========================================================================
 *  Copyright 2019-2020 NXP
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

#ifndef SRC_PFE_PLATFORM_RPC_H_
#define SRC_PFE_PLATFORM_RPC_H_

#include "oal.h"
#include "pfe_ct.h"

typedef uint64_t pfe_platform_rpc_ptr_t;

ct_assert(sizeof(pfe_platform_rpc_ptr_t) == sizeof(uint64_t));

#define PFE_RPC_MAX_IF_NAME_LEN 8

typedef enum __attribute__((packed))
{
	PFE_PLATFORM_RPC_PFE_PHY_IF_CREATE = 100,			/* Arg: pfe_platform_rpc_pfe_phy_if_create_arg_t, Ret: None */
	/* All following PHY_IF commands have first arg struct member phy_if_id */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE = 101,			/* Arg: pfe_platform_rpc_pfe_phy_if_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ID_COMPATIBLE_FIRST = PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE, /* first entry compatible with generic phy_if structure for args*/
	PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE = 102,			/* Arg: pfe_platform_rpc_pfe_phy_if_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE = 103,	/* Arg: pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_DISABLE = 104,	/* Arg: pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR = 105,		/* Arg: pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR = 106,		/* Arg: pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE = 107,		/* Arg: pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF = 108,		/* Arg: pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_GET_OP_MODE = 109,		/* Arg: pfe_platform_rpc_pfe_phy_if_get_op_mode_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_get_op_mode_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED = 110,		/* Arg: pfe_platform_rpc_pfe_phy_if_is_enabled_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_is_enabled_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_IS_PROMISC = 111,		/* Arg: pfe_platform_rpc_pfe_phy_if_is_promisc_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_is_promisc_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_STATS = 112,			/* Arg: pfe_platform_rpc_pfe_phy_if_stats_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_stats_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ID_COMPATIBLE_LAST = PFE_PLATFORM_RPC_PFE_PHY_IF_STATS, /* last entry compatible with generic phy_if structure for args*/

	/* Lock for atomic operations */
	PFE_PLATFORM_RPC_PFE_IF_LOCK = 190,					/* Arg: None, Ret: None */
	PFE_PLATFORM_RPC_PFE_IF_UNLOCK = 191,				/* Arg: None, Ret: None */

	PFE_PLATFORM_RPC_PFE_LOG_IF_CREATE = 200,			/* Arg: pfe_platform_rpc_pfe_log_if_create_arg_t, Ret: pfe_platform_rpc_pfe_log_if_create_ret_t */
	/* All following LOG_IF commands have first arg struct member log_if_id */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DESTROY = 201,			/* Arg: pfe_platform_rpc_pfe_log_if_destroy_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ID_COMPATIBLE_FIRST = PFE_PLATFORM_RPC_PFE_LOG_IF_DESTROY, /* first entry compatible with generic log_if structure for args*/
	PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_RULES = 202,	/* Arg: pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_GET_MATCH_RULES = 203,	/* Arg: pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t, Ret: pfe_platform_rpc_pfe_log_if_get_match_rules_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ADD_MATCH_RULE = 204,	/* Arg: pfe_platform_rpc_pfe_log_if_add_match_rule_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DEL_MATCH_RULE = 205,	/* Arg: pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MAC_ADDR = 206,		/* Arg: pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_GET_MAC_ADDR = 207,		/* Arg: pfe_platform_rpc_pfe_log_if_get_mac_addr_arg_t, Ret: pfe_platform_rpc_pfe_log_if_get_mac_addr_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_CLEAR_MAC_ADDR = 208,	/* Arg: pfe_platform_rpc_pfe_log_if_clear_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ADD_EGRESS_IF = 209,	/* Arg: pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DEL_EGRESS_IF = 210,	/* Arg: pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ENABLE = 211,			/* Arg: pfe_platform_rpc_pfe_log_if_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DISABLE = 212,			/* Arg: pfe_platform_rpc_pfe_log_if_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_IS_ENABLED = 213,		/* Arg: pfe_platform_rpc_pfe_log_if_is_enabled_arg_t, Ret: pfe_platform_rpc_pfe_log_if_is_enabled_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_PROMISC_ENABLE = 214,	/* Arg: pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_PROMISC_DISABLE = 215,	/* Arg: pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_IS_PROMISC = 216,		/* Arg: pfe_platform_rpc_pfe_log_if_is_promisc_arg_t, Ret: pfe_platform_rpc_pfe_log_if_is_enabled_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_GET_EGRESS = 217,		/* Arg: pfe_platform_rpc_pfe_log_if_get_egress_arg_t, Ret: pfe_platform_rpc_pfe_log_if_get_egress_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_IS_MATCH_OR = 218,		/* Arg: pfe_platform_rpc_pfe_log_if_is_match_or_arg_t, Ret: pfe_platform_rpc_pfe_log_if_is_match_or_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_OR = 219,		/* Arg: pfe_platform_rpc_pfe_log_if_set_match_or_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_AND = 220,	/* Arg: pfe_platform_rpc_pfe_log_if_set_match_andr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_STATS = 221,			/* Arg: pfe_platform_rpc_pfe_log_if_stats_arg_t, Ret: pfe_platform_rpc_pfe_log_if_stats_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ID_COMPATIBLE_LAST = PFE_PLATFORM_RPC_PFE_LOG_IF_STATS /* last entry compatible with generic log_if structure for args*/
} pfe_platform_rpc_code_t;

/* Generic log if type */
typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_generic_tag
{
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_log_if_generic_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_generic_t, log_if_id));

/* Generic phy if type */
typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_generic_tag
{
	uint8_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_generic_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_generic_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_create_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_create_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_create_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_create_arg_tag
{
	/*	Parent physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	char_t name[PFE_RPC_MAX_IF_NAME_LEN];
} pfe_platform_rpc_pfe_log_if_create_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_create_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_create_ret_tag
{
	/*	Assigned logical interface ID */
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_log_if_create_ret_t;

typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_destroy_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_destroy_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_clear_mac_addr_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_clear_mac_addr_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_enable_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_disable_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_enabled_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_enabled_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_promisc_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_promisc_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_get_egress_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_get_egress_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_match_or_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_match_or_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_set_match_and_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_match_and_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_set_match_or_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_match_or_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_stats_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_stats_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_is_enabled_ret_tag
{
	/*	Boolean status */
	bool_t status;
} pfe_platform_rpc_pfe_log_if_is_enabled_ret_t;

typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_log_if_is_match_or_ret_t;
typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_log_if_is_promisc_ret_t;
typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_phy_if_is_promisc_ret_t;
typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_phy_if_is_enabled_ret_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_set_match_rules_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	Rules */
	pfe_ct_if_m_rules_t rules;
	/*	Rules arguments structure */
	pfe_ct_if_m_args_t args;
} pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t, log_if_id));

typedef pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t pfe_platform_rpc_pfe_log_if_get_match_rules_ret_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_add_match_rule_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	Rule to be set */
	pfe_ct_if_m_rules_t rule;
	/*	Argument length */
	uint32_t arg_len;
	/*	Rule argument storage. 16 bytes is the IPv6 address which is longest
	 	member of the pfe_ct_if_m_args_t. */
	uint8_t arg[16];
} pfe_platform_rpc_pfe_log_if_add_match_rule_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_add_match_rule_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_del_match_rule_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	Rule or rules to be set */
	pfe_ct_if_m_rules_t rule;
} pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	The MAC address */
	pfe_mac_addr_t addr;
} pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_t, log_if_id));

typedef pfe_platform_rpc_pfe_log_if_create_ret_t pfe_platform_rpc_pfe_log_if_get_mac_addr_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_get_mac_addr_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_t pfe_platform_rpc_pfe_log_if_get_mac_addr_ret_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_add_egress_if_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	The physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t, log_if_id));

typedef pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_stats_ret_t
{
	/*	Current log if statistics */
	pfe_ct_class_algo_stats_t stats;
}pfe_platform_rpc_pfe_log_if_stats_ret_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_enable_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_enable_arg_t;

typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_disable_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_disable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_get_op_mode_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_get_op_mode_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_is_promisc_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_is_promisc_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_is_enabled_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_is_enabled_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_stats_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_stats_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	MAC address */
	uint8_t mac_addr[6];
} pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t, phy_if_id));

typedef pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	Operation mode */
	pfe_ct_if_op_mode_t op_mode;
} pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_has_log_if_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	Logical interface ID */
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t;
ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_get_egress_ret_tag
{
	/*	Mask of egress interfaces */
	uint32_t egress;
} pfe_platform_rpc_pfe_log_if_get_egress_ret_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_get_op_mode_ret_t
{
	/*	Current operation mode */
	pfe_ct_if_op_mode_t mode;
} pfe_platform_rpc_pfe_phy_if_get_op_mode_ret_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_stats_ret_t
{
	/*	Current phy if statistics */
	pfe_ct_phy_if_stats_t stats;
}pfe_platform_rpc_pfe_phy_if_stats_ret_t;

#endif /* SRC_PFE_PLATFORM_RPC_H_ */
