/* =========================================================================
 *  Copyright 2021-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef PFE_MIRROR_H
#define PFE_MIRROR_H

#include "pfe_class.h"

typedef struct pfe_mirror_tag pfe_mirror_t;

typedef enum
{
    MIRROR_ANY,         /* Retrieve the 1st entry in the database, arg not used */
    MIRROR_BY_NAME,     /* Retrieve the entry with matching name, arg is a string (the name) */
    MIRROR_BY_PHYS_ADDR /* Retrieve the entry with matching DMEM address, arg is addr_t (the address) */
} pfe_mirror_db_crit_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

errno_t pfe_mirror_init(pfe_class_t *class);
void pfe_mirror_deinit(void);
pfe_mirror_t *pfe_mirror_get_first(pfe_mirror_db_crit_t crit, const void *arg);
pfe_mirror_t *pfe_mirror_get_next(void);
pfe_mirror_t *pfe_mirror_create(const char *name);
errno_t pfe_mirror_destroy(pfe_mirror_t *mirror);
void pfe_mirror_put(pfe_mirror_t *mirror);
void pfe_mirror_put_by_address(addr_t address);
uint32_t pfe_mirror_get_address(const pfe_mirror_t *mirror);
const char * pfe_mirror_get_name(const pfe_mirror_t *mirror);
errno_t pfe_mirror_set_egress_port(pfe_mirror_t *mirror, pfe_ct_phy_if_id_t egress);
pfe_ct_phy_if_id_t pfe_mirror_get_egress_port(const pfe_mirror_t *mirror);
errno_t pfe_mirror_set_filter(pfe_mirror_t *mirror, uint32_t filter_address);
uint32_t pfe_mirror_get_filter(const pfe_mirror_t *mirror);
errno_t pfe_mirror_set_actions(pfe_mirror_t *mirror, pfe_ct_route_actions_t actions, const pfe_ct_route_actions_args_t *args);
errno_t pfe_mirror_get_actions(const pfe_mirror_t *mirror, pfe_ct_route_actions_t *actions, pfe_ct_route_actions_args_t *args);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_MIRROR_H */
