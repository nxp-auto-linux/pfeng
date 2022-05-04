/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef SRC_PFE_PLATFORM_RPC_H_
#define SRC_PFE_PLATFORM_RPC_H_

#include "oal.h"
#include "pfe_ct.h"

#ifdef PFE_CFG_FCI_ENABLE
#include "fci_msg.h"
#endif /* PFE_CFG_FCI_ENABLE */

typedef uint64_t pfe_platform_rpc_ptr_t;

ct_assert(sizeof(pfe_platform_rpc_ptr_t) == sizeof(uint64_t));

#define PFE_RPC_MAX_IF_NAME_LEN 8

typedef enum __attribute__((packed))
{
	PFE_PLATFORM_RPC_PFE_PHY_IF_CREATE = 100U,				/* Arg: pfe_platform_rpc_pfe_phy_if_create_arg_t, Ret: None */
	/* All following PHY_IF commands have first arg struct member phy_if_id */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE = 101U,				/* Arg: pfe_platform_rpc_pfe_phy_if_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ID_COMPATIBLE_FIRST = PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE, /* first entry compatible with generic phy_if structure for args*/
	PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE = 102U,				/* Arg: pfe_platform_rpc_pfe_phy_if_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE = 103U,		/* Arg: pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_DISABLE = 104U,		/* Arg: pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR = 105U,			/* Arg: pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR = 106U,			/* Arg: pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE = 107U,			/* Arg: pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF = 108U,			/* Arg: pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_GET_OP_MODE = 109U,			/* Arg: pfe_platform_rpc_pfe_phy_if_get_op_mode_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_get_op_mode_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED = 110U,			/* Arg: pfe_platform_rpc_pfe_phy_if_is_enabled_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_is_enabled_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_IS_PROMISC = 111U,			/* Arg: pfe_platform_rpc_pfe_phy_if_is_promisc_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_is_promisc_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_STATS = 112U,				/* Arg: pfe_platform_rpc_pfe_phy_if_stats_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_stats_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_FLUSH_MAC_ADDRS = 113U,		/* Arg: pfe_platform_rpc_pfe_phy_if_flush_mac_addrs_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ALLMULTI_ENABLE = 114U,		/* Arg: pfe_platform_rpc_pfe_phy_if_allmulti_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ALLMULTI_DISABLE = 115U,		/* Arg: pfe_platform_rpc_pfe_phy_if_allmulti_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_LOOPBACK_ENABLE = 116U,             /* Arg: pfe_platform_rpc_pfe_phy_if_loopback_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_LOOPBACK_DISABLE = 117U,            /* Arg: pfe_platform_rpc_pfe_phy_if_loopback_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_LOADBALANCE_ENABLE = 118U,          /* Arg: pfe_platform_rpc_pfe_phy_if_loadbalance_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_LOADBALANCE_DISABLE = 119U,         /* Arg: pfe_platform_rpc_pfe_phy_if_loadbalance_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_SET_BLOCK_STATE = 120U,				/* Arg: pfe_platform_rpc_pfe_phy_if_set_block_state_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_GET_BLOCK_STATE = 121U,				/* Arg: pfe_platform_rpc_pfe_phy_if_get_block_state_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_get_block_state_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ID_COMPATIBLE_LAST = PFE_PLATFORM_RPC_PFE_PHY_IF_GET_BLOCK_STATE, /* last entry compatible with generic phy_if structure for args*/

	/* Lock for atomic operations */
	PFE_PLATFORM_RPC_PFE_IF_LOCK = 190U,						/* Arg: None, Ret: None */
	PFE_PLATFORM_RPC_PFE_IF_UNLOCK = 191U,					/* Arg: None, Ret: None */

	PFE_PLATFORM_RPC_PFE_LOG_IF_CREATE = 200U,				/* Arg: pfe_platform_rpc_pfe_log_if_create_arg_t, Ret: pfe_platform_rpc_pfe_log_if_create_ret_t */
	/* All following LOG_IF commands have first arg struct member log_if_id */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DESTROY = 201U,				/* Arg: pfe_platform_rpc_pfe_log_if_destroy_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ID_COMPATIBLE_FIRST = PFE_PLATFORM_RPC_PFE_LOG_IF_DESTROY, /* first entry compatible with generic log_if structure for args*/
	PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_RULES = 202U,		/* Arg: pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_GET_MATCH_RULES = 203U,		/* Arg: pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t, Ret: pfe_platform_rpc_pfe_log_if_get_match_rules_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ADD_MATCH_RULE = 204U,		/* Arg: pfe_platform_rpc_pfe_log_if_add_match_rule_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DEL_MATCH_RULE = 205U,		/* Arg: pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ADD_MAC_ADDR = 206U,		/* Arg: pfe_platform_rpc_pfe_log_if_add_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DEL_MAC_ADDR = 207U,		/* Arg: pfe_platform_rpc_pfe_log_if_del_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ADD_EGRESS_IF = 208U,		/* Arg: pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DEL_EGRESS_IF = 209U,		/* Arg: pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ENABLE = 210U,				/* Arg: pfe_platform_rpc_pfe_log_if_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DISABLE = 211U,				/* Arg: pfe_platform_rpc_pfe_log_if_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_IS_ENABLED = 212U,			/* Arg: pfe_platform_rpc_pfe_log_if_is_enabled_arg_t, Ret: pfe_platform_rpc_pfe_log_if_is_enabled_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_PROMISC_ENABLE = 213U,		/* Arg: pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_PROMISC_DISABLE = 214U,		/* Arg: pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_IS_PROMISC = 215U,			/* Arg: pfe_platform_rpc_pfe_log_if_is_promisc_arg_t, Ret: pfe_platform_rpc_pfe_log_if_is_enabled_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_GET_EGRESS_IFS = 216U,		/* Arg: pfe_platform_rpc_pfe_log_if_get_egress_ifs_arg_t, Ret: pfe_platform_rpc_pfe_log_if_get_egress_ifs_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_IS_MATCH_OR = 217U,			/* Arg: pfe_platform_rpc_pfe_log_if_is_match_or_arg_t, Ret: pfe_platform_rpc_pfe_log_if_is_match_or_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_OR = 218U,		/* Arg: pfe_platform_rpc_pfe_log_if_set_match_or_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_AND = 219U,		/* Arg: pfe_platform_rpc_pfe_log_if_set_match_andr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_STATS = 220U,				/* Arg: pfe_platform_rpc_pfe_log_if_stats_arg_t, Ret: pfe_platform_rpc_pfe_log_if_stats_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_FLUSH_MAC_ADDRS = 221U,		/* Arg: pfe_platform_rpc_pfe_log_if_flush_mac_addrs_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ALLMULTI_ENABLE = 222U,		/* Arg: pfe_platform_rpc_pfe_log_if_allmulti_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_ALLMULTI_DISABLE = 223U,	/* Arg: pfe_platform_rpc_pfe_log_if_allmulti_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_LOOPBACK_ENABLE = 224U,		/* Arg: pfe_platform_rpc_pfe_log_if_loopback_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_LOOPBACK_DISABLE = 225U,	/* Arg: pfe_platform_rpc_pfe_log_if_loopback_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_IS_DISCARD = 226U,			/* Arg: pfe_platform_rpc_pfe_log_if_is_discard_arg_t, Ret: pfe_platform_rpc_pfe_log_if_is_discard_ret_t */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DISCARD_ENABLE = 227U,		/* Arg: pfe_platform_rpc_pfe_log_if_discard_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_DISCARD_DISABLE = 228U,		/* Arg: pfe_platform_rpc_pfe_log_if_discard_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_SET_EGRESS_IFS = 229U,		/* Arg: pfe_platform_rpc_pfe_log_if_set_egress_ifs_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_LOG_IF_IS_LOOPBACK = 230U,			/* Arg: pfe_platform_rpc_pfe_log_if_is_loopback_arg_t, Ret: pfe_platform_rpc_pfe_log_if_is_loopback_ret_t */

	PFE_PLATFORM_RPC_PFE_LOG_IF_ID_COMPATIBLE_LAST = PFE_PLATFORM_RPC_PFE_LOG_IF_IS_LOOPBACK, /* last entry compatible with generic log_if structure for args*/

#if defined(PFE_CFG_FCI_ENABLE)
	PFE_PLATFORM_RPC_PFE_FCI_PROXY = 300U					/* Arg: pfe_platform_rpc_pfe_fci_proxy_arg_t, Ret: pfe_platform_rpc_pfe_fci_proxy_ret_t */
#endif /* PFE_CFG_FCI_ENABLE */

} pfe_platform_rpc_code_t;

/* Generic log if type */
typedef struct __attribute__((packed, aligned(4)))
{
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_log_if_generic_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_generic_t, log_if_id));

/* Generic phy if type */
typedef struct __attribute__((packed, aligned(4)))
{
	uint8_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_generic_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_generic_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_create_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_create_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Parent physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	char_t name[PFE_RPC_MAX_IF_NAME_LEN];
} pfe_platform_rpc_pfe_log_if_create_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_create_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Assigned logical interface ID */
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_log_if_create_ret_t;

typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_destroy_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_destroy_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_enabled_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_enabled_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_promisc_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_promisc_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_get_egress_ifs_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_get_egress_ifs_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_match_or_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_match_or_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_set_match_and_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_match_and_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_set_match_or_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_match_or_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_stats_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_stats_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_allmulti_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_allmulti_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_allmulti_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_allmulti_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_loopback_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_loopback_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_discard_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_discard_enable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_discard_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_discard_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_discard_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_discard_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_loopback_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_loopback_disable_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_generic_t pfe_platform_rpc_pfe_log_if_is_loopback_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_is_loopback_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	uint8_t log_if_id;
	pfe_mac_db_crit_t crit;
	pfe_mac_type_t type;
} pfe_platform_rpc_pfe_log_if_flush_mac_addrs_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_flush_mac_addrs_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Boolean status */
	bool_t status;
} pfe_platform_rpc_pfe_log_if_is_enabled_ret_t;

typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_log_if_is_match_or_ret_t;
typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_log_if_is_promisc_ret_t;
typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_phy_if_is_promisc_ret_t;
typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_phy_if_is_enabled_ret_t;
typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_log_if_is_discard_ret_t;
typedef pfe_platform_rpc_pfe_log_if_is_enabled_ret_t pfe_platform_rpc_pfe_log_if_is_loopback_ret_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	Rules */
	pfe_ct_if_m_rules_t rules;
	/*	Rules arguments structure */
	pfe_ct_if_m_args_t __attribute__((aligned(4))) args;
} pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t, log_if_id));

typedef pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t pfe_platform_rpc_pfe_log_if_get_match_rules_ret_t;

typedef struct __attribute__((packed, aligned(4)))
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
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_add_match_rule_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	Rule or rules to be set */
	pfe_ct_if_m_rules_t rule;
} pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	The MAC address */
	pfe_mac_addr_t addr;
} pfe_platform_rpc_pfe_log_if_add_mac_addr_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_add_mac_addr_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	The MAC address */
	pfe_mac_addr_t addr;
} pfe_platform_rpc_pfe_log_if_del_mac_addr_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_del_mac_addr_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Logical interface ID */
	uint8_t log_if_id;
	/*	The physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t, log_if_id));

typedef pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t pfe_platform_rpc_pfe_log_if_set_egress_ifs_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_set_egress_ifs_arg_t, log_if_id));
typedef pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t, log_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Current log if statistics */
	pfe_ct_class_algo_stats_t stats;
}pfe_platform_rpc_pfe_log_if_stats_ret_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_enable_arg_t;

typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_disable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_get_op_mode_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_get_op_mode_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_is_promisc_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_is_promisc_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_is_enabled_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_is_enabled_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_stats_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_stats_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_allmulti_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_allmulti_enable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_allmulti_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_allmulti_disable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_loopback_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_loopback_enable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_loopback_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_loopback_disable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_loadbalance_enable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_loadbalance_enable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_loadbalance_disable_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_loadbalance_disable_arg_t, phy_if_id));
typedef pfe_platform_rpc_pfe_phy_if_generic_t pfe_platform_rpc_pfe_phy_if_get_block_state_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_get_block_state_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	pfe_ct_phy_if_id_t phy_if_id;
	pfe_mac_db_crit_t crit;
	pfe_mac_type_t type;
} pfe_platform_rpc_pfe_phy_if_flush_mac_addrs_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_flush_mac_addrs_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	MAC address */
	uint8_t mac_addr[6];
} pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t, phy_if_id));

typedef pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	Operation mode */
	pfe_ct_if_op_mode_t op_mode;
} pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/* Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/* Block state */
	pfe_ct_block_state_t block_state;
} pfe_platform_rpc_pfe_phy_if_set_block_state_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_set_block_state_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	Logical interface ID */
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t, phy_if_id));

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Mask of egress interfaces */
	uint32_t egress;
} pfe_platform_rpc_pfe_log_if_get_egress_ifs_ret_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Current operation mode */
	pfe_ct_if_op_mode_t mode;
} pfe_platform_rpc_pfe_phy_if_get_op_mode_ret_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/* Current block state */
	pfe_ct_block_state_t state;
} pfe_platform_rpc_pfe_phy_if_get_block_state_ret_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/*	Current phy if statistics */
	pfe_ct_phy_if_stats_t stats;
}pfe_platform_rpc_pfe_phy_if_stats_ret_t;

#if defined(PFE_CFG_FCI_ENABLE)
typedef struct __attribute__((packed, aligned(4)))
{
	/*	FCI message type */
	msg_type_t type;
	/*	FCI command data */
	fci_msg_cmd_t msg_cmd;
} pfe_platform_rpc_pfe_fci_proxy_arg_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/*	FCI reply data */
	fci_msg_cmd_t msg_cmd;
} pfe_platform_rpc_pfe_fci_proxy_ret_t;
#endif /* PFE_CFG_FCI_ENABLE */

#endif /* SRC_PFE_PLATFORM_RPC_H_ */
