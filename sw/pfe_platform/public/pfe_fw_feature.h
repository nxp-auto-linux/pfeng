/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_FW_FEATURE_H
#define PFE_FW_FEATURE_H

typedef errno_t(*dmem_read_func_t)(void *, int32_t, void *, addr_t, uint32_t);
typedef errno_t(*dmem_write_func_t)(void *, int32_t, addr_t, void *, uint32_t);
typedef struct pfe_fw_feature_tag pfe_fw_feature_t;

pfe_fw_feature_t *pfe_fw_feature_create(void);
void pfe_fw_feature_destroy(const pfe_fw_feature_t *feature);
errno_t pfe_fw_feature_set_ll_data(pfe_fw_feature_t *feature, pfe_ct_feature_desc_t *ll_data);
errno_t pfe_fw_feature_set_string_base(pfe_fw_feature_t *feature, const char *string_base);
errno_t pfe_fw_feature_set_dmem_funcs(pfe_fw_feature_t *feature, dmem_read_func_t read_func, dmem_write_func_t write_func, void *data);
errno_t pfe_fw_feature_get_name(const pfe_fw_feature_t *feature, const char **name);
errno_t pfe_fw_feature_get_desc(const pfe_fw_feature_t *feature, const char **desc);
errno_t pfe_fw_feature_get_variant(const pfe_fw_feature_t *feature, uint8_t *variant);
errno_t pfe_fw_feature_get_def_val(const pfe_fw_feature_t *feature, uint8_t *def_val);
errno_t pfe_fw_feature_get_val(const pfe_fw_feature_t *feature, uint8_t *val);
bool_t pfe_fw_feature_enabled(const pfe_fw_feature_t *feature);
errno_t pfe_fw_feature_set_val(const pfe_fw_feature_t *feature, uint8_t val);

#endif

