/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_L2BR_H_
#define PUBLIC_PFE_L2BR_H_

#include "pfe_log_if.h"
#include "pfe_l2br_table.h"

typedef struct __pfe_l2br_tag pfe_l2br_t;
typedef struct __pfe_l2br_domain_tag pfe_l2br_domain_t;

/**
 * @brief	Bridge domain select criteria type
 */
typedef enum
{
	L2BD_CRIT_ALL,				/*!< Match any domain within the bridge (argument is NULL) */
	L2BD_CRIT_BY_VLAN,			/*!< Match entry with VLAN equal to arg (argument is uint16_t) */
	L2BD_BY_PHY_IF,				/*!< Match entries containing given physical interface (argument is pfe_phy_if_t *) */
} pfe_l2br_domain_get_crit_t;

typedef enum
{
	L2BD_IF_CRIT_ALL,			/*!< Match any interface within the domain (argument is NULL) */
	L2BD_IF_BY_PHY_IF_ID,		/*!< Match entries by physical interface id (argument is pfe_phy_if_id_t) */
	L2BD_IF_BY_PHY_IF			/*!< Match entries containing given physical interface (argument is pfe_phy_if_t *) */
} pfe_l2br_domain_if_get_crit_t;

errno_t pfe_l2br_domain_create(pfe_l2br_t *bridge, uint16_t vlan);
errno_t pfe_l2br_domain_destroy(pfe_l2br_domain_t *domain);
errno_t pfe_l2br_domain_set_ucast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t hit, pfe_ct_l2br_action_t miss);
errno_t pfe_l2br_domain_set_mcast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t hit, pfe_ct_l2br_action_t miss);
errno_t pfe_l2br_domain_add_if(pfe_l2br_domain_t *domain, pfe_phy_if_t *iface, bool_t tagged);
errno_t pfe_l2br_domain_del_if(pfe_l2br_domain_t *domain, pfe_phy_if_t *iface);
pfe_phy_if_t *pfe_l2br_domain_get_first_if(pfe_l2br_domain_t *domain, pfe_l2br_domain_if_get_crit_t crit, void *arg);
pfe_phy_if_t *pfe_l2br_domain_get_next_if(pfe_l2br_domain_t *domain);
errno_t pfe_l2br_domain_get_vlan(pfe_l2br_domain_t *domain, uint16_t *vlan);
errno_t pfe_l2br_domain_get_ucast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t *hit, pfe_ct_l2br_action_t *miss);
errno_t pfe_l2br_domain_get_mcast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t *hit, pfe_ct_l2br_action_t *miss);
bool_t pfe_l2br_domain_is_default(pfe_l2br_domain_t *domain) __attribute__((pure));
bool_t pfe_l2br_domain_is_fallback(pfe_l2br_domain_t *domain) __attribute__((pure));
uint32_t pfe_l2br_domain_get_if_list(pfe_l2br_domain_t *domain); __attribute__((pure))
uint32_t pfe_l2br_domain_get_untag_if_list(pfe_l2br_domain_t *domain) __attribute__((pure));

pfe_l2br_t *pfe_l2br_create(pfe_class_t *class, uint16_t def_vlan, pfe_l2br_table_t *mac_table, pfe_l2br_table_t *vlan_table);
errno_t pfe_l2br_destroy(pfe_l2br_t *bridge);
pfe_l2br_domain_t *pfe_l2br_get_default_domain(pfe_l2br_t *bridge) __attribute__((pure));
pfe_l2br_domain_t *pfe_l2br_get_fallback_domain(pfe_l2br_t *bridge) __attribute__((pure));
pfe_l2br_domain_t *pfe_l2br_get_first_domain(pfe_l2br_t *bridge, pfe_l2br_domain_get_crit_t crit, void *arg);
pfe_l2br_domain_t *pfe_l2br_get_next_domain(pfe_l2br_t *bridge);
uint32_t pfe_l2br_get_text_statistics(pfe_l2br_t *bridge, char_t *buf, uint32_t buf_len, uint8_t verb_level);


#endif /* PUBLIC_PFE_L2BR_H_ */
