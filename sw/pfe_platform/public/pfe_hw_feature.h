/* =========================================================================
 *  Copyright 2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_DRV_FEATURE_H
#define PFE_DRV_FEATURE_H

typedef struct pfe_hw_feature_tag pfe_hw_feature_t;

errno_t pfe_hw_feature_init_all(uint32_t *cbus_base, pfe_hw_feature_t ***hw_features, uint32_t *hw_features_count);
errno_t pfe_hw_feature_set_val(pfe_hw_feature_t *feature, uint8_t val);
void pfe_hw_feature_destroy(const pfe_hw_feature_t *feature);
errno_t pfe_hw_feature_get_name(const pfe_hw_feature_t *feature, const char **name);
errno_t pfe_hw_feature_get_desc(const pfe_hw_feature_t *feature, const char **desc);
errno_t pfe_hw_feature_get_flags(const pfe_hw_feature_t *feature, pfe_ct_feature_flags_t *flags);
errno_t pfe_hw_feature_get_def_val(const pfe_hw_feature_t *feature, uint8_t *def_val);
errno_t pfe_hw_feature_get_val(const pfe_hw_feature_t *feature, uint8_t *val);
bool_t pfe_hw_feature_enabled(const pfe_hw_feature_t *feature);

#endif

