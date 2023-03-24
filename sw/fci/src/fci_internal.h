/* =========================================================================
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_internal.c
 * @brief		Internal header distributing FCI-related artifacts not intended
 * 				to be exposed to public.
 * @details
 *
 */

#ifndef SRC_FCI_INTERNAL_H_
#define SRC_FCI_INTERNAL_H_

#include "oal.h"

#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"
#include "pfe_if_db.h"
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
#include "fci_ownership_mask.h"
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#include "fci.h"
#include "fci_msg.h"   /* The IPC message format (fci_msg_t) */
#include "fci_core.h"  /* The OS-specific FCI core */
#include "fci_rt_db.h" /* Database of routes */

/**
 * @brief	This is the FCI endpoint representation structure.
 */
struct fci_tag
{
	fci_core_t *core;

	pfe_if_db_t *phy_if_db;			/* Pointer to platform driver phy_if DB */
	bool_t phy_if_db_initialized;	/* Logical interface DB was initialized */

	pfe_if_db_t *log_if_db;			/* Pointer to platform driver log_if DB */
	bool_t log_if_db_initialized;	/* Logical interface DB was initialized */

	uint32_t if_session_id;			/* Holds session ID for interface session */

	fci_rt_db_t route_db;
	bool_t rt_db_initialized;

	pfe_rtable_t *rtable;
	bool_t rtable_initialized;

	pfe_l2br_t *l2_bridge;
	bool_t l2_bridge_initialized;

	oal_mutex_t db_mutex;
	bool_t db_mutex_initialized;

	pfe_tmu_t *tmu;					/* Pointer to platform driver tmu */
	bool_t tmu_initialized;			/* Platform TMU was initialized */

	pfe_class_t *class;

	struct
	{
		uint32_t timeout_tcp;
		uint32_t timeout_udp;
		uint32_t timeout_other;
	} default_timeouts;

	bool_t hm_cb_registered;
	bool_t is_some_client;			/* TRUE if there is at least one client registered for FCI events. */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	bool_t fci_owner_initialized;
#endif /* #ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT */

	bool_t fci_initialized;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

 /* Global variable used across all fci files */
extern fci_t __context;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"

#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

errno_t fci_interfaces_session_cmd(uint32_t code, uint16_t *fci_ret);
errno_t fci_interfaces_log_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_log_if_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_interfaces_phy_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_phy_if_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_interfaces_mac_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_if_mac_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_routes_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_rt_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_connections_ipv4_ct_cmd(const fci_msg_t *msg, uint16_t *fci_ret, fpp_ct_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_connections_ipv6_ct_cmd(const fci_msg_t *msg, uint16_t *fci_ret, fpp_ct6_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_connections_ipv4_timeout_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_timeout_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_l2br_domain_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_l2_bd_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_l2br_static_entry_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_l2_static_ent_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_l2br_flush_cmd(uint32_t code, uint16_t *fci_ret);
errno_t fci_routes_drop_one(fci_rt_db_entry_t *route);
void fci_routes_drop_all(void);
void fci_routes_drop_all_ipv4(void);
void fci_routes_drop_all_ipv6(void);
void fci_connections_drop_all(void);
errno_t fci_connections_set_default_timeout(uint8_t ip_proto, uint32_t timeout);
uint32_t fci_connections_get_default_timeout(uint8_t ip_proto);
errno_t fci_qos_queue_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_queue_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_qos_scheduler_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_scheduler_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_qos_shaper_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_shaper_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_qos_policer_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_policer_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_qos_policer_flow_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_policer_flow_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_qos_policer_wred_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_policer_wred_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_qos_policer_shp_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_policer_shp_cmd_t *reply_buf, uint32_t *reply_len);
void fci_hm_send_events(void);
errno_t fci_hm_cb_register(void);
errno_t fci_hm_cb_deregister(void);
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
errno_t fci_owner_session_cmd(pfe_ct_phy_if_id_t sender, uint32_t code, uint16_t *fci_ret);
errno_t fci_owner_get_floating_lock(pfe_ct_phy_if_id_t sender, uint16_t *fci_ret, bool_t *floating_lock);
errno_t fci_owner_clear_floating_lock(void);
errno_t fci_owner_authorize(pfe_ct_phy_if_id_t sender, bool_t *auth_ret);
errno_t fci_sender_get_phy_if_id(uint32_t sender, pfe_ct_phy_if_id_t *phy_if_id);
errno_t fci_owner_mutex_lock(void);
errno_t fci_owner_mutex_unlock(void);
errno_t fci_owner_init(fci_init_info_t *info);
void fci_owner_fini(void);
#endif /* #ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* SRC_FCI_INTERNAL_H_ */

/** @}*/
