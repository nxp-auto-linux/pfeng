/* =========================================================================
 *
 *  Copyright (c) 2020 Imagination Technologies Limited
 *  Copyright 2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef PFE_FLEXIBLE_FILTER_H
#define PFE_FLEXIBLE_FILTER_H

#include "pfe_class.h"

void pfe_flexible_filter_init(void);
errno_t pfe_flexible_filter_set(pfe_class_t *class, const uint32_t dmem_addr);

#endif
