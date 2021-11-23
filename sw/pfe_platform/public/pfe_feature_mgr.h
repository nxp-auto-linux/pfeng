/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef PFE_FEATURE_MGR_H
#define PFE_FEATURE_MGR_H
#include "pfe_class.h"
#include "pfe_util.h"

#define PFE_HW_FEATURE_RUN_ON_G3	"drv_run_on_g3"

errno_t pfe_feature_mgr_init(uint32_t *cbus_base);
errno_t pfe_feature_mgr_fini(void);
errno_t pfe_feature_mgr_add_fw_modules(pfe_class_t *class, pfe_util_t *util);
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
#endif