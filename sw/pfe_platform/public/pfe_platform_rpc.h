/* =========================================================================
 *  Copyright 2019 NXP
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

/**
 * @addtogroup	dxgrPFE_PLATFORM
 * @{
 * 
 * @file		pfe_platform_rpc.h
 * @brief		The PFE platform RPC definition file
 * @details		This file contains all defines related to platform RPC functionality.
 * 				Every RPC is identified by respective pfe_platform_rpc_code_t item
 * 				and related types used as RPC argument and return value are listed
 * 				in particular item description. All the data structures related
 * 				to RPC codes are defined within this file too.
 * @warning		All arguments should start with if_id else additional code should
 * 				be added to idex_rpc_cbk to handle the if_id extraction
 */

#ifndef SRC_PFE_PLATFORM_RPC_H_
#define SRC_PFE_PLATFORM_RPC_H_

#include "oal.h"
#include "pfe_ct.h"

typedef uint64_t pfe_platform_rpc_ptr_t;

_ct_assert(sizeof(pfe_platform_rpc_ptr_t) == sizeof(uint64_t));


typedef enum __attribute__((packed))
{
	PFE_PLATFORM_RPC_PFE_PHY_IF_CREATE = 100,			/* Arg: pfe_platform_rpc_pfe_phy_if_create_arg_t, Ret: None */
	/* All following PHY_IF commands have first arg struct member phy_if_id */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE = 101,			/* Arg: pfe_platform_rpc_pfe_phy_if_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE = 102,			/* Arg: pfe_platform_rpc_pfe_phy_if_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE = 103,	/* Arg: pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_DISABLE = 104,	/* Arg: pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR = 105,		/* Arg: pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR = 106,		/* Arg: pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE = 107,		/* Arg: pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF = 108,		/* Arg: pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t, Ret: None */

	/* Lock for atomic operations */
	PFE_PLATFORM_RPC_PFE_IF_LOCK = 190,					/* Arg: None, Ret: None */
	PFE_PLATFORM_RPC_PFE_IF_UNLOCK = 191,				/* Arg: None, Ret: None */

	PFE_PLATFORM_RPC_PFE_LOG_IF_CREATE = 200,			/* Arg: pfe_platform_rpc_pfe_log_if_create_arg_t, Ret: pfe_platform_rpc_pfe_log_if_create_ret_t */
	/* All following LOG_IF commands have first arg struct member log_if_id */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DESTROY = 201,			/* Arg: pfe_platform_rpc_pfe_log_if_destroy_arg_t, Ret: None */
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
} pfe_platform_rpc_code_t;

/* Generic log if type */
typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_generic_tag
{
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_log_if_generic_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_generic_t, log_if_id));

/* Generic phy if type */
typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_generic_tag
{
	uint8_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_generic_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_generic_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_create_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_create_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_create_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_create_arg_tag
{
	/*	Parent physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_log_if_create_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_create_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_create_ret_tag
{
	/*	Assigned logical interface ID */
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_log_if_create_ret_t;

typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_destroy_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_destroy_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_clear_mac_addr_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_clear_mac_addr_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_enable_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_disable_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_enabled_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_enabled_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_promisc_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_promisc_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_is_enabled_ret_tag
{
	/*	Boolean status */
	bool_t status;
} pfe_platform_rpc_pfe_log_if_is_enabled_ret_t;

typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_log_if_is_promisc_ret_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_set_match_rules_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	Rules */
	pfe_ct_if_m_rules_t rules;
	/*	Rules arguments structure */
	pfe_ct_if_m_args_t args;
} pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t, log_if_id));

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
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_add_match_rule_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_del_match_rule_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	Rule or rules to be set */
	pfe_ct_if_m_rules_t rule;
} pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	The MAC address */
	pfe_mac_addr_t addr;
} pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_t, log_if_id));

typedef pfe_platform_rpc_pfe_log_if_create_ret_t pfe_platform_rpc_pfe_log_if_get_mac_addr_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_get_mac_addr_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_t pfe_platform_rpc_pfe_log_if_get_mac_addr_ret_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_log_if_add_egress_if_arg_tag
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	The physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t, log_if_id));

typedef pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_enable_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_enable_arg_t;

typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_disable_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_disable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	MAC address */
	uint8_t mac_addr[6];
} pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t, phy_if_id));

typedef pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	Operation mode */
	pfe_ct_if_op_mode_t op_mode;
} pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_has_log_if_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	Logical interface ID */
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t;
_ct_assert(0U == offsetof(pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t, phy_if_id));

#endif /* SRC_PFE_PLATFORM_RPC_H_ */

/** @}*/
