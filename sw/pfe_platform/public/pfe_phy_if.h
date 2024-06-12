/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_PHY_IF_H_
#define PUBLIC_PFE_PHY_IF_H_

#include "oal_types.h"
#include "pfe_ct.h"
#include "pfe_emac.h"
#include "pfe_mac_db.h"
#if defined(PFE_CFG_TARGET_OS_LINUX)
#include "pfe_hif_chnl_linux.h"
#else
#include "pfe_hif_chnl.h"
#endif
#include "pfe_class.h"
#include "pfe_mirror.h"

/**
 * @brief	Interface callback reasons
 */
typedef enum
{
	PHY_IF_EVT_MAC_ADDR_UPDATE,	/*!< PHY_IF_EVT_MAC_ADDR_UPDATE */
	PHY_IF_EVT_INVALID         	/*!< PHY_IF_EVT_INVALID */
} pfe_phy_if_event_t;

typedef struct pfe_phy_if_tag pfe_phy_if_t;

#include "pfe_log_if.h"

/**
 * @brief	Interface callback type
 */
typedef void (* pfe_phy_if_cbk_t)(pfe_phy_if_t *iface, pfe_phy_if_event_t event, void *arg);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_phy_if_t *pfe_phy_if_create(pfe_class_t *class, pfe_ct_phy_if_id_t id, const char_t *name);
bool_t pfe_phy_if_has_log_if(pfe_phy_if_t *iface, const pfe_log_if_t *log_if);
errno_t pfe_phy_if_del_log_if(pfe_phy_if_t *iface, const pfe_log_if_t *log_if);
errno_t pfe_phy_if_add_log_if(pfe_phy_if_t *iface, pfe_log_if_t *log_if);
errno_t pfe_phy_if_bind_emac(pfe_phy_if_t *iface, pfe_emac_t *emac);
pfe_emac_t *pfe_phy_if_get_emac(const pfe_phy_if_t *iface);
errno_t pfe_phy_if_bind_hif(pfe_phy_if_t *iface, pfe_hif_chnl_t *hif);
pfe_hif_chnl_t *pfe_phy_if_get_hif(const pfe_phy_if_t *iface);
errno_t pfe_phy_if_bind_util(pfe_phy_if_t *iface);
pfe_ct_phy_if_id_t pfe_phy_if_get_id(const pfe_phy_if_t *iface) __attribute__((pure));
const char_t *pfe_phy_if_get_name(const pfe_phy_if_t *iface) __attribute__((pure));
void pfe_phy_if_destroy(pfe_phy_if_t *iface);
pfe_class_t *pfe_phy_if_get_class(const pfe_phy_if_t *iface) __attribute__((pure));
errno_t pfe_phy_if_set_block_state(pfe_phy_if_t *iface, pfe_ct_block_state_t block_state);
errno_t pfe_phy_if_get_block_state(pfe_phy_if_t *iface, pfe_ct_block_state_t *block_state);
pfe_ct_if_op_mode_t pfe_phy_if_get_op_mode(pfe_phy_if_t *iface);
errno_t pfe_phy_if_set_op_mode(pfe_phy_if_t *iface, pfe_ct_if_op_mode_t mode);
bool_t pfe_phy_if_is_enabled(pfe_phy_if_t *iface);
errno_t pfe_phy_if_enable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_disable(pfe_phy_if_t *iface);
bool_t pfe_phy_if_is_promisc(pfe_phy_if_t *iface);
errno_t pfe_phy_if_loadbalance_enable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_loadbalance_disable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_promisc_enable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_promisc_disable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_loopback_enable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_loopback_disable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_allmulti_enable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_allmulti_disable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_add_mac_addr(pfe_phy_if_t *iface, const pfe_mac_addr_t addr, pfe_drv_id_t owner);
errno_t pfe_phy_if_del_mac_addr(pfe_phy_if_t *iface, const pfe_mac_addr_t addr, pfe_drv_id_t owner);
pfe_mac_db_t *pfe_phy_if_get_mac_db(const pfe_phy_if_t *iface);
errno_t pfe_phy_if_get_mac_addr_first(pfe_phy_if_t *iface, pfe_mac_addr_t addr, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner);
errno_t pfe_phy_if_get_mac_addr_next(pfe_phy_if_t *iface, pfe_mac_addr_t addr);
errno_t pfe_phy_if_flush_mac_addrs(pfe_phy_if_t *iface, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner);
errno_t pfe_phy_if_get_stats(pfe_phy_if_t *iface, pfe_ct_phy_if_stats_t *stat);
errno_t pfe_phy_if_set_rx_mirror(pfe_phy_if_t *iface, uint32_t sel, const pfe_mirror_t *mirror);
errno_t pfe_phy_if_set_tx_mirror(pfe_phy_if_t *iface, uint32_t sel, const pfe_mirror_t *mirror);
pfe_mirror_t *pfe_phy_if_get_tx_mirror(const pfe_phy_if_t *iface, uint32_t sel);
pfe_mirror_t *pfe_phy_if_get_rx_mirror(const pfe_phy_if_t *iface, uint32_t sel);
pfe_log_if_t *pfe_phy_if_get_default_log_if(pfe_phy_if_t *iface);

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)
uint32_t pfe_phy_if_get_text_statistics(const pfe_phy_if_t *iface, char_t *buf, uint32_t buf_len, uint8_t verb_level);
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

uint32_t pfe_phy_if_get_spd(const pfe_phy_if_t *iface);
errno_t pfe_phy_if_set_spd(pfe_phy_if_t *iface, uint32_t spd_addr);
errno_t pfe_phy_if_set_ftable(pfe_phy_if_t *iface, uint32_t table);
uint32_t pfe_phy_if_get_ftable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_set_flag(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag);
errno_t pfe_phy_if_clear_flag(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag);
pfe_ct_if_flags_t pfe_phy_if_get_flag(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag);
errno_t pfe_phy_if_get_flow_control(pfe_phy_if_t *iface, bool_t* tx_ena, bool_t* rx_ena);
errno_t pfe_phy_if_set_tx_flow_control(pfe_phy_if_t *iface, bool_t tx_ena);
errno_t pfe_phy_if_set_rx_flow_control(pfe_phy_if_t *iface, bool_t rx_ena);
uint32_t pfe_phy_if_get_stat_value(pfe_phy_if_t *iface, uint32_t stat_id);
errno_t pfe_phy_if_set_mgmt_interface(pfe_phy_if_t *iface, pfe_ct_phy_if_id_t mgmt_interface);
pfe_ct_phy_if_id_t pfe_phy_if_get_mgmt_interface(pfe_phy_if_t *iface);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_PHY_IF_H_ */
