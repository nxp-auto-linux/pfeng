/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef PFE_FEATURE_MGR_H
#define PFE_FEATURE_MGR_H

#include "pfe_class.h"
#include "pfe_util.h"
#include "pfe_tmu.h"

#define PFE_HW_FEATURE_RUN_ON_G3	"drv_run_on_g3"

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

enum CAL_PACKED
{
	FW_FEATURE_TABLE_DEFAULT = 0U,
	FW_FEATURE_TABLE_CONFIG = 1U,
	FW_FEATURE_TABLE_STATS = 2U
};

errno_t pfe_feature_mgr_init(uint32_t *cbus_base);
errno_t pfe_feature_mgr_fini(void);
errno_t pfe_feature_mgr_add_modules(pfe_class_t *class, pfe_util_t *util, pfe_tmu_t *tmu);
bool_t pfe_feature_mgr_is_available(const char *feature_name);
errno_t pfe_feature_mgr_set_val(const char *feature_name, const uint8_t val);
errno_t pfe_feature_mgr_get_val(const char *feature_name, uint8_t *val);

errno_t pfe_feature_mgr_get_first(const char **feature_name);
errno_t pfe_feature_mgr_get_next(const char **feature_name);
errno_t pfe_feature_mgr_get_def_val(const char *feature_name, uint8_t *val);
errno_t pfe_feature_mgr_get_desc(const char *feature_name, const char **desc);
errno_t pfe_feature_mgr_get_variant(const char *feature_name, uint8_t *val);

errno_t pfe_feature_mgr_enable(const char *feature_name);
errno_t pfe_feature_mgr_disable(const char *feature_name);

errno_t pfe_feature_mgr_table_first(const char *feature_name, uint8_t table_type, const char **table_el_name);
errno_t pfe_feature_mgr_table_next(const char *feature_name, uint8_t table_type, const char **table_el_name);
errno_t pfe_feature_mgr_table_get_size(const char *feature_name, uint8_t table_type, const char *table_el_name, uint8_t *size);
errno_t pfe_feature_mgr_table_get_multiplicity(const char *feature_name, uint8_t table_type, const char *table_el_name, uint8_t *count);
errno_t pfe_feature_mgr_table_get_payload(const char *feature_name, uint8_t table_type, const char *table_el_name, uint8_t *payload);

errno_t pfe_feature_mgr_table_set_val(const char *feature_name, uint8_t table_type, const char *table_el_name, uint8_t index, uint8_t* val);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif
