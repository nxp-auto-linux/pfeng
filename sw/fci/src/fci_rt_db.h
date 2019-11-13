/* =========================================================================
 *  Copyright 2017-2019 NXP
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
typedef struct __route_entry_tag
{
	void *refptr;					/*	Reference pointer storage */
	uint32_t id;					/*	Route entry identifier */
	uint16_t mtu;
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
typedef struct __fci_rt_db_tag
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
		pfe_phy_if_t *iface;
	} cur_crit_arg;						/*	Current criterion argument */
} fci_rt_db_t;

void fci_rt_db_init(fci_rt_db_t *db);
errno_t fci_rt_db_add(fci_rt_db_t *db, pfe_ip_addr_t *dst_ip, pfe_mac_addr_t *dst_mac,
					pfe_phy_if_t *iface, uint32_t id, void *refptr, bool_t overwrite);
errno_t fci_rt_db_remove(fci_rt_db_t *db, fci_rt_db_entry_t *entry);
errno_t fci_rt_db_drop_all(fci_rt_db_t *db);
fci_rt_db_entry_t *fci_rt_db_get_first(fci_rt_db_t *db, fci_rt_db_get_criterion_t crit, void *arg);
fci_rt_db_entry_t *fci_rt_db_get_next(fci_rt_db_t *db);

#endif /* SRC_FCI_RT_DB_H_ */

/** @}*/

