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

pfe_hif_nocpy_t *pfe_hif_nocpy_create(addr_t base_va, const pfe_bmu_t *bmu, uint16_t lmem_header_size);
pfe_hif_chnl_t *pfe_hif_nocpy_get_channel(const pfe_hif_nocpy_t *hif, uint32_t channel_id);
uint32_t pfe_hif_nocpy_get_text_statistics(const pfe_hif_nocpy_t *hif, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_hif_nocpy_destroy(pfe_hif_nocpy_t *hif);

#endif /* PUBLIC_PFE_HIF_NOCPY_H_ */
