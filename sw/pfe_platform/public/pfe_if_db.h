/* =========================================================================
 *  Copyright 2017-2022,2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_LOG_IF_DB_H_
#define PFE_LOG_IF_DB_H_

#include "linked_list.h"
#include "pfe_ct.h"
#include "pfe_log_if.h"
#include "pfe_phy_if.h"

typedef enum
{
	PFE_IF_DB_PHY,
	PFE_IF_DB_LOG
} pfe_if_db_type_t;

/**
 * @brief	Interface database entry type
 */
typedef struct pfe_if_db_entry_tag pfe_if_db_entry_t;

/**
 * @brief	Interface database select criteria type
 */
typedef enum
{
	IF_DB_CRIT_ALL,				/*!< Match any entry in the DB */
	IF_DB_CRIT_BY_ID,			/*!< Match entries by interface ID */
	IF_DB_CRIT_BY_INSTANCE,		/*!< Match entries by interface instance */
	IF_DB_CRIT_BY_NAME,			/*!< Match entries by interface name */
	IF_DB_CRIT_BY_OWNER			/*!< Match entries by owner ID */
} pfe_if_db_get_criterion_t;

/**
 * @brief	Interface database instance representation type
 */
typedef struct pfe_if_db_tag pfe_if_db_t;

errno_t pfe_if_db_context_init(void);
errno_t pfe_if_db_context_destroy(void);
pfe_if_db_t * pfe_if_db_create(pfe_if_db_type_t type);
void pfe_if_db_destroy(const pfe_if_db_t *db);
errno_t pfe_if_db_add(pfe_if_db_t *db, uint32_t session_id, void *iface, pfe_ct_phy_if_id_t owner);
errno_t pfe_if_db_remove(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_entry_t *entry);
errno_t pfe_log_if_db_drop_all(const pfe_if_db_t *db, uint32_t session_id);
errno_t pfe_if_db_lock(uint32_t *session_id);
errno_t pfe_if_db_lock_owned(uint32_t owner_id);
errno_t pfe_if_db_polled_lock(uint32_t *session_id);
errno_t pfe_if_db_polled_lock_owned(uint32_t owner_id);
errno_t pfe_if_db_unlock(uint32_t session_id);
errno_t pfe_if_db_lock_check(uint32_t session_id);
errno_t pfe_if_db_get_first(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_get_criterion_t crit, void *arg, pfe_if_db_entry_t **db_entry);
errno_t pfe_if_db_get_next(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_entry_t **db_entry);
pfe_phy_if_t *pfe_if_db_entry_get_phy_if(const pfe_if_db_entry_t *entry) __attribute__((pure));
pfe_log_if_t *pfe_if_db_entry_get_log_if(const pfe_if_db_entry_t *entry) __attribute__((pure));
errno_t pfe_if_db_get_single(const pfe_if_db_t *db, uint32_t session_id, pfe_if_db_get_criterion_t crit, void *arg, pfe_if_db_entry_t **db_entry);

#endif /* PFE_LOG_IF_DB_H_ */
