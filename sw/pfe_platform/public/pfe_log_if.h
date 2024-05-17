/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2022,2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_LOG_IF_H_
#define PUBLIC_PFE_LOG_IF_H_

#include "pfe_ct.h"

/**
 * @brief	Interface callback reasons
 */
typedef enum
{
	LOG_IF_EVT_MAC_ADDR_UPDATE,	/*!< LOG_IF_EVT_MAC_ADDR_UPDATE */
	LOG_IF_EVT_INVALID         	/*!< LOG_IF_EVT_INVALID */
} pfe_log_if_event_t;

typedef struct pfe_log_if_tag pfe_log_if_t;

#include "pfe_phy_if.h"

/**
 * @brief	Interface callback type
 */
typedef void (* pfe_log_if_cbk_t)(pfe_log_if_t *iface, pfe_log_if_event_t event, void *arg);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_log_if_t *pfe_log_if_create(pfe_phy_if_t *parent, const char_t *name);
uint8_t pfe_log_if_get_id(const pfe_log_if_t *iface) __attribute__((pure));
__attribute__((pure)) pfe_phy_if_t *pfe_log_if_get_parent(const pfe_log_if_t *iface);
errno_t pfe_log_if_set_next_dmem_ptr(pfe_log_if_t *iface, addr_t next_dmem_ptr);
errno_t pfe_log_if_get_next_dmem_ptr(pfe_log_if_t *iface, addr_t *next_dmem_ptr);
errno_t pfe_log_if_get_dmem_base(const pfe_log_if_t *iface, addr_t *dmem_base);
void pfe_log_if_destroy(pfe_log_if_t *iface);
errno_t pfe_log_if_set_match_or(pfe_log_if_t *iface);
errno_t pfe_log_if_set_match_and(pfe_log_if_t *iface);
bool_t pfe_log_if_is_match_or(pfe_log_if_t *iface);
errno_t pfe_log_if_set_match_rules(pfe_log_if_t *iface, pfe_ct_if_m_rules_t rules, const pfe_ct_if_m_args_t *args);
errno_t pfe_log_if_get_match_rules(pfe_log_if_t *iface, pfe_ct_if_m_rules_t *rules, pfe_ct_if_m_args_t *args);
errno_t pfe_log_if_add_match_rule(pfe_log_if_t *iface, pfe_ct_if_m_rules_t rule, const void *arg, uint32_t arg_len);
errno_t pfe_log_if_del_match_rule(pfe_log_if_t *iface, pfe_ct_if_m_rules_t rule);
errno_t pfe_log_if_get_egress_ifs(pfe_log_if_t *iface, uint32_t *egress);
errno_t pfe_log_if_set_egress_ifs(pfe_log_if_t *iface, uint32_t egress);
errno_t pfe_log_if_add_egress_if(pfe_log_if_t *iface, const pfe_phy_if_t *phy_if);
errno_t pfe_log_if_del_egress_if(pfe_log_if_t *iface, const pfe_phy_if_t *phy_if);
errno_t pfe_log_if_enable(pfe_log_if_t *iface);
errno_t pfe_log_if_disable(pfe_log_if_t *iface);
bool_t pfe_log_if_is_enabled(pfe_log_if_t *iface) __attribute__((pure));
errno_t pfe_log_if_promisc_enable(pfe_log_if_t *iface);
errno_t pfe_log_if_promisc_disable(pfe_log_if_t *iface);
bool_t pfe_log_if_is_promisc(pfe_log_if_t *iface) __attribute__((pure));
const char_t *pfe_log_if_get_name(const pfe_log_if_t *iface) __attribute__((pure));
errno_t pfe_log_if_discard_enable(pfe_log_if_t *iface);
errno_t pfe_log_if_discard_disable(pfe_log_if_t *iface);
bool_t pfe_log_if_is_discard(pfe_log_if_t *iface);
errno_t pfe_log_if_get_stats(const pfe_log_if_t *iface, pfe_ct_class_algo_stats_t *stat);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_LOG_IF_H_ */
