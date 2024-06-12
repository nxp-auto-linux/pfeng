/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_L2BR_TABLE_H_
#define PUBLIC_PFE_L2BR_TABLE_H_

#include "pfe_phy_if.h"

typedef enum
{
	PFE_L2BR_TABLE_INVALID,
	PFE_L2BR_TABLE_MAC2F,
	PFE_L2BR_TABLE_VLAN
} pfe_l2br_table_type_t;

/**
 * @brief	L2 bridge table select criteria type
 */
typedef enum
{
	L2BR_TABLE_CRIT_ALL,			/*!< Match any entry in the table */
	L2BR_TABLE_CRIT_VALID			/*!< Match only valid entries */
} pfe_l2br_table_get_criterion_t;

typedef struct __pfe_l2br_table_tag pfe_l2br_table_t;
typedef struct __pfe_l2br_table_iterator_tag pfe_l2br_table_iterator_t;
typedef struct __pfe_l2br_table_entry_tag pfe_l2br_table_entry_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_l2br_table_t *pfe_l2br_table_create(addr_t cbus_base_va, pfe_l2br_table_type_t type);
void pfe_l2br_table_destroy(pfe_l2br_table_t *l2br);
errno_t pfe_l2br_table_init(pfe_l2br_table_t *l2br);
errno_t pfe_l2br_table_flush(pfe_l2br_table_t *l2br);
errno_t pfe_l2br_table_add_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_del_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_update_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_search_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
pfe_l2br_table_iterator_t *pfe_l2br_iterator_create(void);
errno_t pfe_l2br_iterator_destroy(const pfe_l2br_table_iterator_t *inst);
errno_t pfe_l2br_table_get_first(pfe_l2br_table_t *l2br, pfe_l2br_table_iterator_t *l2t_iter, pfe_l2br_table_get_criterion_t crit, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_get_next(pfe_l2br_table_t *l2br, pfe_l2br_table_iterator_t *l2t_iter, pfe_l2br_table_entry_t *entry);

pfe_l2br_table_entry_t *pfe_l2br_table_entry_create(const pfe_l2br_table_t *l2br);
errno_t pfe_l2br_table_entry_destroy(const pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_entry_set_mac_addr(pfe_l2br_table_entry_t *entry, const pfe_mac_addr_t mac_addr);
errno_t pfe_l2br_table_entry_set_vlan(pfe_l2br_table_entry_t *entry, uint16_t vlan);
__attribute__((pure)) uint32_t pfe_l2br_table_entry_get_vlan(const pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_entry_set_action_data(pfe_l2br_table_entry_t *entry, uint64_t action_data);
__attribute__((pure)) uint64_t pfe_l2br_table_entry_get_action_data(const pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_entry_set_fresh(const pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry, bool_t is_fresh);
bool_t pfe_l2br_table_entry_is_fresh(const pfe_l2br_table_entry_t *entry) __attribute__((pure));
errno_t pfe_l2br_table_entry_set_static(const pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry, bool_t is_static);
bool_t pfe_l2br_table_entry_is_static(const pfe_l2br_table_entry_t *entry) __attribute__((pure));

uint32_t pfe_l2br_table_entry_to_str(const pfe_l2br_table_entry_t *entry, struct seq_file *seq);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_L2BR_TABLE_H_ */
