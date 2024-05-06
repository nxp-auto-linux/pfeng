/* =========================================================================
 *  Copyright 2019-2022,2024 NXP
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
	PFE_PLATFORM_RPC_PFE_PHY_IF_GET_STAT_VALUE = 122U,			    /* Arg: pfe_platform_rpc_pfe_phy_if_get_stat_value_arg_t, Ret: pfe_platform_rpc_pfe_phy_if_get_stat_value_ret_t */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ID_COMPATIBLE_LAST = PFE_PLATFORM_RPC_PFE_PHY_IF_GET_STAT_VALUE, /* last entry compatible with generic phy_if structure for args*/

	/* Lock for atomic operations */
	PFE_PLATFORM_RPC_PFE_IF_LOCK = 190U,						/* Arg: None, Ret: None */
	PFE_PLATFORM_RPC_PFE_IF_UNLOCK = 191U,					/* Arg: None, Ret: None */

#if defined(PFE_CFG_FCI_ENABLE)
	PFE_PLATFORM_RPC_PFE_FCI_PROXY = 300U,					/* Arg: pfe_platform_rpc_pfe_fci_proxy_arg_t, Ret: pfe_platform_rpc_pfe_fci_proxy_ret_t */
#endif /* PFE_CFG_FCI_ENABLE */

	PFE_PLATFORM_RPC_MDIO_PROXY = 310U,					/* Arg: pfe_platform_rpc_mdio_proxy_arg_t, Ret: pfe_platform_rpc_mdio_proxy_ret_t */

} pfe_platform_rpc_code_t;

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
	/*	Boolean status */
	bool_t status;
} pfe_platform_rpc_pfe_phy_if_is_enabled_ret_t;

typedef pfe_platform_rpc_pfe_phy_if_is_enabled_ret_t pfe_platform_rpc_pfe_phy_if_is_promisc_ret_t;

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
	/* Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/* Block state */
	pfe_ct_block_state_t block_state;
} pfe_platform_rpc_pfe_phy_if_set_block_state_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_set_block_state_arg_t, phy_if_id));

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

typedef struct __attribute__((packed, aligned(4)))
{
	/*	statistic value */
	uint32_t stat_val;
} pfe_platform_rpc_pfe_phy_if_get_stat_value_ret_t;

typedef struct __attribute__((packed, aligned(4)))
{
	pfe_ct_phy_if_id_t phy_if_id;
	uint32_t stat_id;
} pfe_platform_rpc_pfe_phy_if_get_stat_value_arg_t;
ct_assert_offsetof(0U == offsetof(pfe_platform_rpc_pfe_phy_if_get_stat_value_arg_t, phy_if_id));

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

typedef enum __attribute__((packed))
{
	/*	MDIO operation READ, Clause 22 */
	PFE_PLATFORM_RPC_MDIO_OP_READ_CL22 = 101U,
	/*	MDIO operation WRITE, Clause 22 */
	PFE_PLATFORM_RPC_MDIO_OP_WRITE_CL22 = 102U,
	/*	MDIO operation READ, Clause 45 */
	PFE_PLATFORM_RPC_MDIO_OP_READ_CL45 = 103U,
	/*	MDIO operation WRITE, Clause 45 */
	PFE_PLATFORM_RPC_MDIO_OP_WRITE_CL45 = 104U,
} pfe_platform_rpc_mdio_proxy_op_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/*	PFE EMAC id */
	uint8_t emac_id;
	/*	MDIO operation */
	pfe_platform_rpc_mdio_proxy_op_t op;
	/*	MDIO device: port address */
	uint8_t pa;
	/*	MDIO device: device address */
	uint8_t dev;
	/*	MDIO device: register address */
	uint16_t ra;
	/*	MDIO WRITE data */
	uint16_t val;
} pfe_platform_rpc_mdio_proxy_arg_t;

typedef struct __attribute__((packed, aligned(4)))
{
	/*	MDIO READ data */
	uint16_t val;
} pfe_platform_rpc_mdio_proxy_ret_t;

#endif /* SRC_PFE_PLATFORM_RPC_H_ */
