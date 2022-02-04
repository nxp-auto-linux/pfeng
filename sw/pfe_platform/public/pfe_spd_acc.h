 /* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2020-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef PFE_SPD_ACC_H
#define PFE_SPD_ACC_H

#include "pfe_class.h"
#include "pfe_rtable.h"
#include "pfe_if_db.h"

errno_t pfe_spd_acc_init(pfe_class_t *class, const pfe_rtable_t *rtable);
void pfe_spd_acc_destroy(pfe_if_db_t *phy_if_db);
errno_t pfe_spd_acc_add_rule(pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry);
errno_t pfe_spd_acc_remove_rule(pfe_phy_if_t * phy_if, uint16_t position);
errno_t pfe_spd_acc_get_rule(const pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry);
#endif
