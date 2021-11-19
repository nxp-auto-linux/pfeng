/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
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
} pfe_util_cfg_t;

pfe_util_t *pfe_util_create(addr_t cbus_base_va, uint32_t pe_num, const pfe_util_cfg_t *cfg);
void pfe_util_enable(pfe_util_t *util);
void pfe_util_reset(pfe_util_t *util);
void pfe_util_disable(pfe_util_t *util);
errno_t pfe_util_load_firmware(pfe_util_t *util, const void *elf);
uint32_t pfe_util_get_text_statistics(const pfe_util_t *util, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_util_destroy(pfe_util_t *util);
errno_t pfe_util_isr(const pfe_util_t *util);
void pfe_util_irq_mask(const pfe_util_t *util);
void pfe_util_irq_unmask(const pfe_util_t *util);
errno_t pfe_util_get_fw_version(const pfe_util_t *util, pfe_ct_version_t *ver);
errno_t pfe_util_get_feature(const pfe_util_t *util, pfe_fw_feature_t **feature, const char *name);
errno_t pfe_util_get_feature_first(pfe_util_t *util, pfe_fw_feature_t **feature);
errno_t pfe_util_get_feature_next(pfe_util_t *util, pfe_fw_feature_t **feature);
errno_t pfe_util_read_dmem(void *util_p, int32_t pe_idx, void *dst_ptr, addr_t src_addr, uint32_t len);
errno_t pfe_util_write_dmem(void *util_p, int32_t pe_idx, addr_t dst_addr, void *src_ptr, uint32_t len);

#endif /* SRC_PFE_UTIL_H_ */
