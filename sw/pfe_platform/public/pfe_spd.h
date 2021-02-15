/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef PFE_SPD_H
#define PFE_SPD_H
void pfe_spd_init(pfe_class_t *class);
errno_t pfe_spd_add_rule(pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry);
errno_t pfe_spd_remove_rule(pfe_phy_if_t * phy_if, uint16_t position);
errno_t pfe_spd_get_rule(pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry);

#endif