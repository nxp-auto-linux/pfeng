/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_PHY_IF_H_
#define PUBLIC_PFE_PHY_IF_H_

#include "oal_types.h"
#include "pfe_ct.h"
#include "pfe_emac.h"
#include "pfe_hif_chnl.h"
#include "pfe_class.h"

/**
 * @brief	Interface callback reasons
 */
typedef enum
{
	PHY_IF_EVT_MAC_ADDR_UPDATE,	/*!< PHY_IF_EVT_MAC_ADDR_UPDATE */
	PHY_IF_EVT_INVALID         	/*!< PHY_IF_EVT_INVALID */
} pfe_phy_if_event_t;

typedef struct __pfe_phy_if_tag pfe_phy_if_t;

#include "pfe_log_if.h"

/**
 * @brief	Interface callback type
 */
typedef void (* pfe_phy_if_cbk_t)(pfe_phy_if_t *iface, pfe_phy_if_event_t event, void *arg);

pfe_phy_if_t *pfe_phy_if_create(pfe_class_t *class, pfe_ct_phy_if_id_t id, char_t *name);
bool_t pfe_phy_if_has_log_if(pfe_phy_if_t *iface, pfe_log_if_t *log_if);
errno_t pfe_phy_if_bind_emac(pfe_phy_if_t *iface, pfe_emac_t *emac);
pfe_emac_t *pfe_phy_if_get_emac(pfe_phy_if_t *iface);
errno_t pfe_phy_if_bind_hif(pfe_phy_if_t *iface, pfe_hif_chnl_t *hif);
pfe_hif_chnl_t *pfe_phy_if_get_hif(pfe_phy_if_t *iface);
errno_t pfe_phy_if_bind_util(pfe_phy_if_t *iface);
pfe_ct_phy_if_id_t pfe_phy_if_get_id(pfe_phy_if_t *iface) __attribute__((pure));
char_t *pfe_phy_if_get_name(pfe_phy_if_t *iface) __attribute__((pure));
errno_t pfe_phy_if_destroy(pfe_phy_if_t *iface);
pfe_class_t *pfe_phy_if_get_class(pfe_phy_if_t *iface) __attribute__((pure));
errno_t pfe_phy_if_set_block_state(pfe_phy_if_t *iface, pfe_ct_block_state_t block_state);
errno_t pfe_phy_if_get_block_state(pfe_phy_if_t *iface, pfe_ct_block_state_t *block_state);
pfe_ct_if_op_mode_t pfe_phy_if_get_op_mode(pfe_phy_if_t *iface);
errno_t pfe_phy_if_set_op_mode(pfe_phy_if_t *iface, pfe_ct_if_op_mode_t mode);
bool_t pfe_phy_if_is_enabled(pfe_phy_if_t *iface);
errno_t pfe_phy_if_enable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_disable(pfe_phy_if_t *iface);
bool_t pfe_phy_if_is_promisc(pfe_phy_if_t *iface);
errno_t pfe_phy_if_promisc_enable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_promisc_disable(pfe_phy_if_t *iface);
errno_t pfe_phy_if_add_mac_addr(pfe_phy_if_t *iface, pfe_mac_addr_t addr);
errno_t pfe_phy_if_del_mac_addr(pfe_phy_if_t *iface, pfe_mac_addr_t addr);
errno_t pfe_phy_if_get_mac_addr(pfe_phy_if_t *iface, pfe_mac_addr_t addr);
errno_t pfe_phy_if_get_stats(pfe_phy_if_t *iface, pfe_ct_phy_if_stats_t *stat);
uint32_t pfe_phy_if_get_text_statistics(pfe_phy_if_t *iface, char_t *buf, uint32_t buf_len, uint8_t verb_level);
errno_t pfe_phy_if_set_mirroring(pfe_phy_if_t *iface, pfe_ct_phy_if_id_t mirror);
pfe_ct_phy_if_id_t pfe_phy_if_get_mirroring(pfe_phy_if_t *iface);
uint32_t pfe_phy_if_get_spd(pfe_phy_if_t *iface);
errno_t pfe_phy_if_set_spd(pfe_phy_if_t *iface, uint32_t spd_addr);

#endif /* PUBLIC_PFE_PHY_IF_H_ */
