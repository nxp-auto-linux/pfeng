/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_HIF_NOCPY_H_
#define PUBLIC_PFE_HIF_NOCPY_H_

#include "pfe_hif_ring.h"
#include "pfe_hif_chnl.h"
#include "pfe_bmu.h"

typedef struct
{
	uint32_t nothing; /* Some compilers don't support empty structs */
} pfe_hif_nocpy_cfg_t;

typedef struct pfe_hif_nocpy_tag pfe_hif_nocpy_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_hif_nocpy_t *pfe_hif_nocpy_create(addr_t base_va, const pfe_bmu_t *bmu, uint16_t lmem_header_size);
pfe_hif_chnl_t *pfe_hif_nocpy_get_channel(const pfe_hif_nocpy_t *hif, uint32_t channel_id);

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)
uint32_t pfe_hif_nocpy_get_text_statistics(const pfe_hif_nocpy_t *hif, char_t *buf, uint32_t buf_len, uint8_t verb_level);
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

void pfe_hif_nocpy_destroy(pfe_hif_nocpy_t *hif);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PUBLIC_PFE_HIF_NOCPY_H_ */
