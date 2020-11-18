/* =========================================================================
 *
 *  Copyright (c) 2020 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef SRC_PFE_UTIL_H_
#define SRC_PFE_UTIL_H_

typedef struct __pfe_util_tag pfe_util_t;

typedef struct
{
	uint32_t pe_sys_clk_ratio;		/*	Clock mode ratio for sys_clk and pe_clk */
} pfe_util_cfg_t;

pfe_util_t *pfe_util_create(void *cbus_base_va, uint32_t pe_num, pfe_util_cfg_t *cfg);
void pfe_util_enable(pfe_util_t *util);
void pfe_util_reset(pfe_util_t *util);
void pfe_util_disable(pfe_util_t *util);
errno_t pfe_util_load_firmware(pfe_util_t *util, const void *elf);
uint32_t pfe_util_get_text_statistics(pfe_util_t *util, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_util_destroy(pfe_util_t *util);
errno_t pfe_util_isr(pfe_util_t *util);
void pfe_util_irq_mask(pfe_util_t *util);
void pfe_util_irq_unmask(pfe_util_t *util);
errno_t pfe_util_get_fw_version(pfe_util_t *util, pfe_ct_version_t *ver);

#endif /* SRC_PFE_UTIL_H_ */
