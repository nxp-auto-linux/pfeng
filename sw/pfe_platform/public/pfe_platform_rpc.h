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
	PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE = 101,			/* Arg: pfe_platform_rpc_pfe_phy_if_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE = 102,			/* Arg: pfe_platform_rpc_pfe_phy_if_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE = 103,	/* Arg: pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_DISABLE = 104,	/* Arg: pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR = 105,		/* Arg: pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR = 106,		/* Arg: pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE = 107,		/* Arg: pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t, Ret: None */
	PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF = 108		/* Arg: pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t, Ret: None */
} pfe_platform_rpc_code_t;


typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_create_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_create_arg_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_enable_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
} pfe_platform_rpc_pfe_phy_if_enable_arg_t;

typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_disable_arg_t;
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t;
typedef pfe_platform_rpc_pfe_phy_if_enable_arg_t pfe_platform_rpc_pfe_phy_if_promisc_disable_arg_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	MAC address */
	uint8_t mac_addr[6];
} pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t;

typedef pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	Operation mode */
	pfe_ct_if_op_mode_t op_mode;
} pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t;

typedef struct __attribute__((packed, aligned(4))) __pfe_platform_rpc_pfe_phy_if_has_log_if_arg_tag
{
	/*	Physical interface ID */
	pfe_ct_phy_if_id_t phy_if_id;
	/*	Logical interface ID */
	uint8_t log_if_id;
} pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t;

#endif /* SRC_PFE_PLATFORM_RPC_H_ */

/** @}*/
