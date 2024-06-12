/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_FW_FEATURE_H
#define PFE_FW_FEATURE_H

typedef errno_t(*dmem_read_func_t)(void *, int32_t, void *, addr_t, uint32_t);
typedef errno_t(*dmem_write_func_t)(void *, int32_t, addr_t, const void *, uint32_t);
typedef struct pfe_fw_feature_tag pfe_fw_feature_t;
typedef struct pfe_fw_tbl_handle_tag
{
	const pfe_fw_feature_t *feature;
	pfe_ct_feature_tbl_entry_t *tbl_curr;
} pfe_fw_tbl_handle_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

pfe_fw_feature_t *pfe_fw_feature_create(void);
void pfe_fw_feature_destroy(const pfe_fw_feature_t *feature);
errno_t pfe_fw_feature_set_ll_data(pfe_fw_feature_t *feature, pfe_ct_feature_desc_t *ll_data, uint8_t instances);
errno_t pfe_fw_feature_set_string_base(pfe_fw_feature_t *feature, const char *string_base);
errno_t pfe_fw_feature_set_dmem_funcs(pfe_fw_feature_t *feature, dmem_read_func_t read_func, dmem_write_func_t write_func, void *data);
errno_t pfe_fw_feature_get_name(const pfe_fw_feature_t *feature, const char **name);
errno_t pfe_fw_feature_get_desc(const pfe_fw_feature_t *feature, const char **desc);
errno_t pfe_fw_feature_get_flags(const pfe_fw_feature_t *feature, pfe_ct_feature_flags_t *flags);
errno_t pfe_fw_feature_get_def_val(const pfe_fw_feature_t *feature, uint8_t *def_val);
errno_t pfe_fw_feature_get_val(const pfe_fw_feature_t *feature, uint8_t *val);
bool_t pfe_fw_feature_enabled(const pfe_fw_feature_t *feature);
errno_t pfe_fw_feature_set_val(const pfe_fw_feature_t *feature, uint8_t val);
bool_t pfe_fw_feature_is_in_class(const pfe_fw_feature_t *feature);
bool_t pfe_fw_feature_is_in_util(const pfe_fw_feature_t *feature);

errno_t pfe_fw_feature_table_stats_first(pfe_fw_feature_t *feature, pfe_fw_tbl_handle_t *feature_table);
errno_t pfe_fw_feature_table_stats_next(pfe_fw_feature_t *feature, pfe_fw_tbl_handle_t *feature_table);
errno_t pfe_fw_feature_table_cfg_first(pfe_fw_feature_t *feature, pfe_fw_tbl_handle_t *feature_table);
errno_t pfe_fw_feature_table_cfg_next(pfe_fw_feature_t *feature, pfe_fw_tbl_handle_t *feature_table);
errno_t pfe_fw_feature_table_entry_name(pfe_fw_tbl_handle_t handle, const char **table_name);
errno_t pfe_fw_feature_table_stats_by_name(const pfe_fw_feature_t *feature, const char_t *name, pfe_fw_tbl_handle_t *entry);
errno_t pfe_fw_feature_table_cfg_by_name(const pfe_fw_feature_t *feature, const char *name, pfe_fw_tbl_handle_t *entry);
uint32_t pfe_fw_feature_table_entry_size(pfe_fw_tbl_handle_t handle);
uint32_t pfe_fw_feature_table_entry_multiplicity(pfe_fw_tbl_handle_t handle);
uint32_t pfe_fw_feature_table_entry_allocsize(pfe_fw_tbl_handle_t handle);
errno_t pfe_fw_feature_table_entry_get(pfe_fw_tbl_handle_t handle, void *mem, uint16_t size, bool_t collect);
errno_t pfe_fw_feature_table_entry_get_by_idx(pfe_fw_tbl_handle_t handle, void *mem, uint16_t idx, bool_t collect);
errno_t pfe_fw_feature_table_entry_set_by_idx(pfe_fw_tbl_handle_t handle, void *val, uint16_t idx);
errno_t pfe_fw_feature_table_entry_set(pfe_fw_tbl_handle_t handle, void *val, uint16_t size);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif

