/* =========================================================================
 *  Copyright 2018-2021 NXP
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

#include "fci.h"
#include "fci_msg.h"   /* The IPC message format (fci_msg_t) */
#include "fci_core.h"  /* The OS-specific FCI core */
#include "fci_rt_db.h" /* Database of routes */

/**
 * @brief	This is the FCI endpoint representation structure.
 */
struct __fci_tag
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

	pfe_class_t *class;

	struct
	{
		uint32_t timeout_tcp;
		uint32_t timeout_udp;
		uint32_t timeout_other;
	} default_timeouts;

	bool_t fci_initialized;
};

 /* Global variable used across all fci files */
extern fci_t __context;

errno_t fci_process_ipc_message(fci_msg_t *msg, fci_msg_t *rep_msg);
errno_t fci_interfaces_session_cmd(uint32_t code, uint16_t *fci_ret);
errno_t fci_interfaces_phy_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_phy_if_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_interfaces_log_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_log_if_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_routes_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_rt_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_connections_ipv4_ct_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_ct_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_connections_ipv6_ct_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_ct6_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_connections_ipv4_timeout_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_timeout_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_l2br_domain_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_l2_bd_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_l2br_static_entry_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_l2_static_ent_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_routes_drop_one(fci_rt_db_entry_t *route);
void fci_routes_drop_all(void);
void fci_routes_drop_all_ipv4(void);
void fci_routes_drop_all_ipv6(void);
errno_t fci_connections_drop_one(pfe_rtable_entry_t *entry);
void fci_connections_drop_all(void);
errno_t fci_connections_set_default_timeout(uint8_t ip_proto, uint32_t timeout);
uint32_t fci_connections_get_default_timeout(uint8_t ip_proto);
errno_t fci_enable_if(pfe_phy_if_t *phy_if);
errno_t fci_disable_if(pfe_phy_if_t *phy_if);
errno_t fci_qos_queue_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_queue_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_qos_scheduler_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_scheduler_cmd_t *reply_buf, uint32_t *reply_len);
errno_t fci_qos_shaper_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_qos_shaper_cmd_t *reply_buf, uint32_t *reply_len);

#endif /* SRC_FCI_INTERNAL_H_ */

/** @}*/
