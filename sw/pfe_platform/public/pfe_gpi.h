/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_GPI_H_
#define PUBLIC_PFE_GPI_H_

typedef struct pfe_gpi_tag pfe_gpi_t;

typedef struct
{
	uint32_t alloc_retry_cycles;
	uint32_t gpi_tmlf_txthres;
	uint32_t gpi_dtx_aseq_len;
	bool_t emac_1588_ts_en;
} pfe_gpi_cfg_t;

pfe_gpi_t *pfe_gpi_create(void *cbus_base_va, void *gpi_base, pfe_gpi_cfg_t *cfg);
void pfe_gpi_enable(pfe_gpi_t *gpi);
void pfe_gpi_reset(pfe_gpi_t *gpi);
void pfe_gpi_disable(pfe_gpi_t *gpi);
uint32_t pfe_gpi_get_text_statistics(pfe_gpi_t *gpi, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_gpi_destroy(pfe_gpi_t *gpi);

#endif /* PUBLIC_PFE_GPI_H_ */
