/* =========================================================================
 *  Copyright 2017-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_rt_db.h
 * @brief		Route database header file
 * @details
 *
 */

#ifndef SRC_FCI_RT_DB_H_
#define SRC_FCI_RT_DB_H_

#include "linked_list.h"
#include "fpp.h"		/* Due to IFNAMSIZ */
#include "pfe_rtable.h"	/* IP and MAC address type */

/**
 * @brief	Route database entry type
 */
typedef struct
{
	void *refptr;					/*	Reference pointer storage */
	uint32_t id;					/*	Route entry identifier */
	uint16_t mtu;
	pfe_mac_addr_t src_mac;
	pfe_mac_addr_t dst_mac;
	pfe_ip_addr_t dst_ip;			/*	Destination IP (ipv4/ipv6) */
	pfe_phy_if_t *iface;			/*	Associated interface */

	/*	DB/Chaining */
	LLIST_t list_member;
} fci_rt_db_entry_t;

/**
 * @brief	Route database select criteria type
 */
typedef enum
{
	RT_DB_CRIT_ALL,				/*!< Match any entry in the DB */
	RT_DB_CRIT_BY_IF,			/*!< Match entries by interface instance */
	RT_DB_CRIT_BY_IF_NAME,		/*!< Match entries by interface name */
	RT_DB_CRIT_BY_IP,			/*!< Match entries by destination IP address */
	RT_DB_CRIT_BY_MAC,			/*!< Match entries by destination MAC address */
	RT_DB_CRIT_BY_ID			/*!< Match entries by ID */
} fci_rt_db_get_criterion_t;

/**
 * @brief	Route database instance representation type
 */
typedef struct
{
	LLIST_t theList;
	LLIST_t *cur_item;					/*	Current entry to be returned. See ...get_first() and ...get_next() */
	fci_rt_db_get_criterion_t cur_crit;	/*	Current criterion */
	union
	{
		char_t outif_name[IFNAMSIZ];
		pfe_ip_addr_t dst_ip;
		pfe_mac_addr_t dst_mac;
		uint32_t id;
		const pfe_phy_if_t *iface;
	} cur_crit_arg;						/*	Current criterion argument */
} fci_rt_db_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

void fci_rt_db_init(fci_rt_db_t *db);
errno_t fci_rt_db_add(fci_rt_db_t *db, pfe_ip_addr_t *dst_ip,
					pfe_mac_addr_t *src_mac, pfe_mac_addr_t *dst_mac,
					pfe_phy_if_t *iface, uint32_t id, void *refptr, bool_t overwrite);
errno_t fci_rt_db_remove(fci_rt_db_t *db, fci_rt_db_entry_t *entry);
errno_t fci_rt_db_drop_all(fci_rt_db_t *db);
fci_rt_db_entry_t *fci_rt_db_get_first(fci_rt_db_t *db, fci_rt_db_get_criterion_t crit, const void *arg);
fci_rt_db_entry_t *fci_rt_db_get_next(fci_rt_db_t *db);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* SRC_FCI_RT_DB_H_ */

/** @}*/

