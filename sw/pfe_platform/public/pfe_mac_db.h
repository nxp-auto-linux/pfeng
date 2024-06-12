/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_MAC_DB_H_
#define PUBLIC_PFE_MAC_DB_H_

#include "linked_list.h"
#include "oal_types.h"

#include "pfe_ct.h"
#include "pfe_emac.h"

#define PFE_MAC_DB_LOCKED	TRUE
#define PFE_MAC_DB_UNLOCKED	FALSE

typedef struct
{
	pfe_mac_addr_t addr;		/*	The MAC address */
	LLIST_t iterator;			/*	List chain entry */
	pfe_drv_id_t owner;			/*	Identification of the driver that owns this entry */
} pfe_mac_db_list_entry_t;

/**
 * @brief	Possible rules to get or flush some sort of MAC addresses
 */
typedef enum __attribute__ ((packed)) {
	MAC_DB_CRIT_BY_TYPE = 0U,
	MAC_DB_CRIT_BY_OWNER,
	MAC_DB_CRIT_BY_OWNER_AND_TYPE,
	MAC_DB_CRIT_ALL,
	MAC_DB_CRIT_INVALID,
} pfe_mac_db_crit_t;

typedef struct pfe_mac_db_tag pfe_mac_db_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_mac_db_t *pfe_mac_db_create(void);
errno_t pfe_mac_db_destroy(pfe_mac_db_t *db);
errno_t pfe_mac_db_add_addr(pfe_mac_db_t *db, const pfe_mac_addr_t addr, pfe_drv_id_t owner);
errno_t pfe_mac_db_del_addr(pfe_mac_db_t *db, const pfe_mac_addr_t addr, pfe_drv_id_t owner);
errno_t pfe_mac_db_flush(pfe_mac_db_t *db, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner);
errno_t pfe_mac_db_get_first_addr(pfe_mac_db_t *db, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner, pfe_mac_addr_t addr);
errno_t pfe_mac_db_get_next_addr(pfe_mac_db_t *db, pfe_mac_addr_t addr);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_MAC_DB_H_ */
