/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_HIF_H_
#define PUBLIC_PFE_HIF_H_

#if defined(PFE_CFG_TARGET_OS_LINUX)
#include "pfe_hif_ring_linux.h"
#include "pfe_hif_chnl_linux.h"
#else
#include "pfe_hif_ring.h"
#include "pfe_hif_chnl.h"
#endif

typedef enum
{
	HIF_CHNL_INVALID = 0,
	HIF_CHNL_0 = (1 << 0),
	HIF_CHNL_1 = (1 << 1),
	HIF_CHNL_2 = (1 << 2),
	HIF_CHNL_3 = (1 << 3)
} pfe_hif_chnl_id_t;

#define MASTER_UP	(1U << 0)
#define HIF_OCCUPIED	(1U << 1)

/*	Way to translate physical interface ID to HIF channel ID... */
#include "pfe_ct.h"

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static inline pfe_hif_chnl_id_t pfe_hif_chnl_from_phy_id(pfe_ct_phy_if_id_t phy)
{
	if (phy == PFE_PHY_IF_ID_HIF0)
	{
		return HIF_CHNL_0;
	}
	else if (phy == PFE_PHY_IF_ID_HIF1)
	{
		return HIF_CHNL_1;
	}
	else if (phy == PFE_PHY_IF_ID_HIF2)
	{
		return HIF_CHNL_2;
	}
	else if (phy == PFE_PHY_IF_ID_HIF3)
	{
		return HIF_CHNL_3;
	}
	else
	{
		return HIF_CHNL_INVALID;
	}
}

typedef struct pfe_hif_tag pfe_hif_t;

pfe_hif_t *pfe_hif_create(addr_t cbus_base_va, pfe_hif_chnl_id_t channels_mask);
pfe_hif_chnl_t *pfe_hif_get_channel(const pfe_hif_t *hif, pfe_hif_chnl_id_t channel_id);
void pfe_hif_destroy(pfe_hif_t *hif);

#ifdef PFE_CFG_PFE_MASTER
errno_t pfe_hif_isr(pfe_hif_t *hif);
void pfe_hif_irq_mask(pfe_hif_t *hif);
void pfe_hif_irq_unmask(pfe_hif_t *hif);

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)
uint32_t pfe_hif_get_text_statistics(const pfe_hif_t *hif, char_t *buf, uint32_t buf_len, uint8_t verb_level);
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

uint32_t pfe_hif_get_err_poll(pfe_hif_t *hif);

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
void pfe_hif_clear_master_up(const pfe_hif_t *hif);
void pfe_hif_set_master_up(const pfe_hif_t *hif);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#endif /* PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_PFE_SLAVE
bool_t pfe_hif_get_master_up(const pfe_hif_t *hif);
#endif /* PFE_CFG_PFE_SLAVE */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
void pfe_hif_set_master_detect_cfg(pfe_hif_t *hif, bool_t on);
bool_t pfe_hif_get_master_detect_cfg(const pfe_hif_t *hif);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_HIF_H_ */
