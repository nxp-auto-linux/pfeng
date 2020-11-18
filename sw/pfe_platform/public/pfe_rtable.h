/* =========================================================================
 *
 *  Copyright (c) 2020 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_RTABLE_H_
#define PUBLIC_PFE_RTABLE_H_

#include "pfe_emac.h" /* pfe_mac_addr_t */
#include "pfe_class.h"
#include "pfe_phy_if.h" /* pfe_interface_t */

typedef struct __pfe_rtable_tag pfe_rtable_t;
typedef struct __pfe_rtable_entry_tag pfe_rtable_entry_t;

typedef struct
{
	union
	{
		uint8_t _8[4];
	} v4;

	union
	{
		uint16_t _8[16];
	} v6;

	bool_t is_ipv4;
} pfe_ip_addr_t;

/**
 * @brief	5-tuple representation type
 */
typedef struct
{
	pfe_ip_addr_t src_ip;	/*!< Source IP address */
	pfe_ip_addr_t dst_ip;	/*!< Destination IP address */
	uint16_t sport;			/*!< Source L4 port number */
	uint16_t dport;			/*!< Destination L4 port number */
	uint8_t proto;			/*!< Protocol identifier */
} pfe_5_tuple_t;

/**
 * @brief	Callback event codes
 * @details	Once an event associated with entry has occurred the specified callback is
 * 			called with event identifier value corresponding to one of the following
 * 			values.
 * @see		pfe_rtable_add_entry
 * @see		pfe_rtable_callback_t
 */
typedef enum
{
	RTABLE_ENTRY_TIMEOUT	/*	Entry has been removed from the routing table */
} pfe_rtable_cbk_event_t;

/**
 * @brief	Routing table select criteria type
 */
typedef enum
{
	RTABLE_CRIT_ALL,				/*!< Match any entry in the routing table. The get_first() argument is NULL. */
	RTABLE_CRIT_ALL_IPV4,			/*!< Match any entry in the routing table. The get_first() argument is NULL. */
	RTABLE_CRIT_ALL_IPV6,			/*!< Match any entry in the routing table. The get_first() argument is NULL. */
	RTABLE_CRIT_BY_DST_IF,			/*!< Match entries by destination interface. The get_first() argument is (pfe_interface_t *). */
	RTABLE_CRIT_BY_ROUTE_ID,		/*!< Match entries by route ID. The get_first() argument is (uint32_t *). */
	RTABLE_CRIT_BY_5_TUPLE,			/*!< Match entries by 5-tuple. The get_first() argument is (pfe_5_tuple_t *). */
	RTABLE_CRIT_BY_ID5T,			/*!< Match entries by unique 5-tuple ID */
} pfe_rtable_get_criterion_t;

/**
 * @brief	Callback type
 * @details	During entry addition one can specify a callback to be called when an entry
 * 			related event occur. This is prototype of the callback.
 * @see		pfe_rtable_add_entry
 */
typedef void (* pfe_rtable_callback_t)(void *arg, pfe_rtable_cbk_event_t event);

pfe_rtable_t *pfe_rtable_create(pfe_class_t *class, void *htable_base_pa, uint32_t htable_size, void *pool_base_pa, uint32_t pool_size);
errno_t pfe_rtable_add_entry(pfe_rtable_t *rtable, pfe_rtable_entry_t *entry);
errno_t pfe_rtable_del_entry(pfe_rtable_t *rtable, pfe_rtable_entry_t *entry);
void pfe_rtable_destroy(pfe_rtable_t *rtable);
uint32_t pfe_rtable_get_entry_size(void);
errno_t pfe_rtable_entry_to_5t(pfe_rtable_entry_t *entry, pfe_5_tuple_t *tuple);
errno_t pfe_rtable_entry_to_5t_out(pfe_rtable_entry_t *entry, pfe_5_tuple_t *tuple);
pfe_rtable_entry_t *pfe_rtable_get_first(pfe_rtable_t *rtable, pfe_rtable_get_criterion_t crit, void *arg);
pfe_rtable_entry_t *pfe_rtable_get_next(pfe_rtable_t *rtable);
uint32_t pfe_rtable_get_size(pfe_rtable_t *rtable);

pfe_rtable_entry_t *pfe_rtable_entry_create(void);
void pfe_rtable_entry_free(pfe_rtable_entry_t *entry);
errno_t pfe_rtable_entry_set_5t(pfe_rtable_entry_t *entry, pfe_5_tuple_t *tuple);
errno_t pfe_rtable_entry_set_sip(pfe_rtable_entry_t *entry, pfe_ip_addr_t *ip_addr);
void pfe_rtable_entry_get_sip(pfe_rtable_entry_t *entry, pfe_ip_addr_t *ip_addr);
errno_t pfe_rtable_entry_set_out_sip(pfe_rtable_entry_t *entry, pfe_ip_addr_t *output_sip);
errno_t pfe_rtable_entry_set_dip(pfe_rtable_entry_t *entry, pfe_ip_addr_t *ip_addr);
void pfe_rtable_entry_get_dip(pfe_rtable_entry_t *entry, pfe_ip_addr_t *ip_addr);
errno_t pfe_rtable_entry_set_out_dip(pfe_rtable_entry_t *entry, pfe_ip_addr_t *output_dip);
void pfe_rtable_entry_set_sport(pfe_rtable_entry_t *entry, uint16_t sport);
uint16_t pfe_rtable_entry_get_sport(pfe_rtable_entry_t *entry);
void pfe_rtable_entry_set_out_sport(pfe_rtable_entry_t *entry, uint16_t output_sport);
void pfe_rtable_entry_set_dport(pfe_rtable_entry_t *entry, uint16_t sport);
uint16_t pfe_rtable_entry_get_dport(pfe_rtable_entry_t *entry);
void pfe_rtable_entry_set_out_dport(pfe_rtable_entry_t *entry, uint16_t output_dport);
void pfe_rtable_entry_set_proto(pfe_rtable_entry_t *entry, uint8_t proto);
uint8_t pfe_rtable_entry_get_proto(pfe_rtable_entry_t *entry);
errno_t pfe_rtable_entry_set_dstif(pfe_rtable_entry_t *entry, pfe_phy_if_t *iface);
void pfe_rtable_entry_set_out_smac(pfe_rtable_entry_t *entry, pfe_mac_addr_t mac);
void pfe_rtable_entry_set_out_dmac(pfe_rtable_entry_t *entry, pfe_mac_addr_t mac);
void pfe_rtable_entry_set_out_vlan(pfe_rtable_entry_t *entry, uint16_t vlan);
void pfe_rtable_entry_set_out_inner_vlan(pfe_rtable_entry_t *entry, uint16_t vlan);
void pfe_rtable_entry_set_out_pppoe_sid(pfe_rtable_entry_t *entry, uint16_t sid);
errno_t pfe_rtable_entry_set_action_flags(pfe_rtable_entry_t *entry, pfe_ct_route_actions_t flags);
pfe_ct_route_actions_t pfe_rtable_entry_get_action_flags(pfe_rtable_entry_t *entry);
void pfe_rtable_entry_set_timeout(pfe_rtable_entry_t *entry, uint32_t timeout);
void pfe_rtable_entry_set_route_id(pfe_rtable_entry_t *entry, uint32_t route_id);
errno_t pfe_rtable_entry_get_route_id(pfe_rtable_entry_t *entry, uint32_t *route_id);
void pfe_rtable_entry_set_callback(pfe_rtable_entry_t *entry, pfe_rtable_callback_t cbk, void *arg);
void pfe_rtable_entry_set_refptr(pfe_rtable_entry_t *entry, void *refptr);
void *pfe_rtable_entry_get_refptr(pfe_rtable_entry_t *entry);
void pfe_rtable_entry_set_child(pfe_rtable_entry_t *entry, pfe_rtable_entry_t *child);
pfe_rtable_entry_t *pfe_rtable_entry_get_child(pfe_rtable_entry_t *entry);

void pfe_rtable_entry_set_id5t(pfe_rtable_entry_t *entry, uint32_t id5t);
errno_t pfe_rtable_entry_get_id5t(pfe_rtable_entry_t *entry, uint32_t *id5t);
errno_t pfe_rtable_entry_set_dstif_id(pfe_rtable_entry_t *entry, pfe_ct_phy_if_id_t if_id);

#endif /* PUBLIC_PFE_RTABLE_H_ */
