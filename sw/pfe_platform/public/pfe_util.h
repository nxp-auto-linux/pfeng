/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef SRC_PFE_UTIL_H_
#define SRC_PFE_UTIL_H_

typedef struct pfe_util_tag pfe_util_t;

typedef struct
{
	uint32_t pe_sys_clk_ratio;		/*	Clock mode ratio for sys_clk and pe_clk */
	bool_t on_g3;
	uint32_t interrupt;
} pfe_util_cfg_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_util_t *pfe_util_create(addr_t cbus_base_va, uint32_t pe_num, const pfe_util_cfg_t *cfg);
void pfe_util_enable(pfe_util_t *util);
void pfe_util_reset(pfe_util_t *util);
void pfe_util_disable(pfe_util_t *util);
errno_t pfe_util_load_firmware(pfe_util_t *util, const void *elf);

uint32_t pfe_util_get_text_statistics(const pfe_util_t *util, struct seq_file *seq, uint8_t verb_level);

void pfe_util_destroy(pfe_util_t *util);
errno_t pfe_util_get_fw_version(const pfe_util_t *util, pfe_ct_version_t *ver);
errno_t pfe_util_get_feature(const pfe_util_t *util, pfe_fw_feature_t **feature, const char *name);
errno_t pfe_util_get_feature_first(pfe_util_t *util, pfe_fw_feature_t **feature);
errno_t pfe_util_get_feature_next(pfe_util_t *util, pfe_fw_feature_t **feature);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* SRC_PFE_UTIL_H_ */
