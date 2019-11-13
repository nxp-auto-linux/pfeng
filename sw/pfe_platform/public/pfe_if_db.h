/* =========================================================================
 *  Copyright 2017-2019 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ========================================================================= */

/**
 * @addtogroup  dxgr_PFE_PLATFORM
 * @{
 *
 * @file		pfe_if_db.h
 * @brief		Interface database header file
 * @details
 *
 */

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
typedef struct __pfe_if_db_entry_tag pfe_if_db_entry_t;

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
typedef struct __pfe_if_db_tag pfe_if_db_t;

pfe_if_db_t * pfe_if_db_create(pfe_if_db_type_t type);
void pfe_if_db_destroy(pfe_if_db_t *db);
errno_t pfe_if_db_add(pfe_if_db_t *db, uint32_t session_id, void *iface, pfe_ct_phy_if_id_t owner);
errno_t pfe_if_db_remove(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_entry_t *entry);
errno_t pfe_log_if_db_drop_all(pfe_if_db_t *db, uint32_t session_id);
errno_t pfe_if_db_lock(uint32_t *session_id);
errno_t pfe_if_db_lock_owned(uint32_t owner_id);
errno_t pfe_if_db_unlock(uint32_t session_id);
errno_t pfe_if_db_get_first(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_get_criterion_t crit, void *arg, pfe_if_db_entry_t **db_entry);
errno_t pfe_if_db_get_next(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_entry_t **db_entry);
uint32_t pfe_if_db_get_count(pfe_if_db_t *db, pfe_if_db_get_criterion_t crit, void *arg);
pfe_phy_if_t *pfe_if_db_entry_get_phy_if(pfe_if_db_entry_t *entry) __attribute__((pure));
pfe_log_if_t *pfe_if_db_entry_get_log_if(pfe_if_db_entry_t *entry) __attribute__((pure));

#endif /* PFE_LOG_IF_DB_H_ */

/** @}*/

