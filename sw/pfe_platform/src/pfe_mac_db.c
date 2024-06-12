/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2021-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_mac_db.h"

struct pfe_mac_db_tag
{
	LLIST_t mac_list;
	LLIST_t *iterator;
	oal_mutex_t lock;
	struct {
		pfe_mac_db_crit_t crit;
		pfe_drv_id_t owner;
		pfe_mac_type_t type;
	} crit;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static bool_t pfe_mac_db_criterion_eval(const pfe_mac_db_list_entry_t *entry, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner);
static pfe_mac_db_list_entry_t *pfe_mac_db_find_by_addr(const pfe_mac_db_t *db, const pfe_mac_addr_t addr, pfe_drv_id_t owner);

/**
 * @brief		Evaluate given DB entry against specified criterion
 * @param[in]	entry DB entry to evaluate
 * @param[in]	crit All, Owner, Type or Owner&Type criterion
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @param[in]	owner Required owner of MAC address
 * @return		TRUE if entry does match with criterion, FALSE otherwise
 */
static bool_t pfe_mac_db_criterion_eval(const pfe_mac_db_list_entry_t *entry, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner)
{
	bool_t ret = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (crit == MAC_DB_CRIT_BY_OWNER)
		{
			/* Return the first address where owner match */
			if (entry->owner == owner)
			{
				/* Break if entry match with the rule */
				ret = TRUE;
			}
		}
		else if (crit == MAC_DB_CRIT_BY_TYPE)
		{
			/* Break if entry match with the rule */
			ret = pfe_emac_check_crit_by_type(entry->addr, type);
		}
		else if (crit == MAC_DB_CRIT_BY_OWNER_AND_TYPE)
		{
			if (entry->owner == owner)
			{
				/* Break if entry match with the rule */
				ret = pfe_emac_check_crit_by_type(entry->addr, type);
			}
		}
		else if (crit == MAC_DB_CRIT_ALL)
		{
			/* Break if entry match with the rule */
			ret = TRUE;
		}
		else
		{
			NXP_LOG_WARNING("Unknown criterion\n");
		}
	}

	return ret;
}

/**
 * @brief		Create instance of MAC database
 * @return		Pointer to database instance
 */
pfe_mac_db_t *pfe_mac_db_create(void)
{
	pfe_mac_db_t *db;

	db = oal_mm_malloc(sizeof(pfe_mac_db_t));
	if (NULL == db)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
	}
	else
	{
		(void)memset(db, 0, sizeof(pfe_mac_db_t));
		LLIST_Init(&db->mac_list);
		db->iterator = &db->mac_list;
		db->crit.crit = MAC_DB_CRIT_INVALID;

		if (EOK != oal_mutex_init(&db->lock))
		{
			NXP_LOG_ERROR("Could not initialize mutex\n");
			oal_mm_free(db);
			db = NULL;
		}
	}

	return db;
}

/**
 * @brief		Destroy instance of MAC database
 * @param[in]	db Pointer to MAC database instance
 * @return		Execution status, EOK if success, error code otherwise
 */
errno_t pfe_mac_db_destroy(pfe_mac_db_t *db)
{
	errno_t ret  = EOK;
	LLIST_t *item, *aux;
	pfe_mac_db_list_entry_t *entry;

	if (NULL != db)
	{
		if (EOK != oal_mutex_lock(&db->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		LLIST_ForEachRemovable(item, aux, &db->mac_list)
		{
			entry = LLIST_Data(item, pfe_mac_db_list_entry_t, iterator);
			if (NULL != entry)
			{
				LLIST_Remove(&entry->iterator);
				oal_mm_free(entry);
				entry = NULL;
			}
		}

		if (EOK != oal_mutex_unlock(&db->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}

		if (EOK != oal_mutex_destroy(&db->lock))
		{
			NXP_LOG_ERROR("Could not destroy mutex\n");
		}

		oal_mm_free(db);
	}

	return ret;
}

/**
 * @brief		Search for specific MAC address in the database and return pointer on related entry
 * @param[in]	db Pointer to MAC database instance
 * @param[in]	addr MAC address to search for
 * @return		Pointer to related entry, NULL if address not found
 */
static pfe_mac_db_list_entry_t *pfe_mac_db_find_by_addr(const pfe_mac_db_t *db, const pfe_mac_addr_t addr,
							pfe_drv_id_t owner)
{
	pfe_mac_db_list_entry_t *entry;
	LLIST_t *item;
	bool_t found = FALSE;

	LLIST_ForEach(item, &db->mac_list)
	{
		entry = LLIST_Data(item, pfe_mac_db_list_entry_t, iterator);
		if ((entry->owner == owner) && (0 == memcmp(addr, entry->addr, sizeof(pfe_mac_addr_t))))
		{
			found = TRUE;
			break;
		}
	}

	if(found == FALSE)
	{
		entry = NULL;
	}

	return entry;
}

/**
 * @brief			Add new MAC address into database
 * @param[in]		db Pointer to MAC database instance
 * @param[in]		addr The MAC address to add
 * @param[in]		owner The identification of driver instance
 * @return			Execution status, EOK if success, error code otherwise
 */
errno_t pfe_mac_db_add_addr(pfe_mac_db_t *db, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	errno_t ret;
	pfe_mac_db_list_entry_t *entry;

	if (EOK != oal_mutex_lock(&db->lock))
	{
		NXP_LOG_ERROR("mutex lock failed\n");
	}

	/* Add only if the same address does not already exist in DB */
	entry = pfe_mac_db_find_by_addr(db, addr, owner);
	if (NULL == entry)
	{
		/*	Add address to local list */
#ifdef PFE_CFG_TARGET_OS_LINUX
		entry = kzalloc(sizeof(pfe_mac_db_list_entry_t), GFP_ATOMIC); /* temporary fix for AAVB-3946 */
#else
		entry = oal_mm_malloc(sizeof(pfe_mac_db_list_entry_t));
#endif
		if (NULL == entry)
		{
			NXP_LOG_ERROR("Memory allocation failed\n");
			ret = ENOMEM;
		}
		else
		{
			(void)memcpy(entry->addr, addr, sizeof(pfe_mac_addr_t));
			entry->owner = owner;
			LLIST_AddAtEnd(&entry->iterator, &db->mac_list);

			/*
			 * Move database iterator pointer to new item to handle situation when new entry
			 * was added at the end of the list, while pfe_mac_get_next_addr() reached end of the list
			 * and db->iterator is pointing at the start of the list again and loop would end in normal
			 * circumstances.
			 */
			if (db->iterator == &db->mac_list)
			{
				db->iterator = db->mac_list.prPrev;
			}
			ret = EOK;
		}
	}
	else
	{
		ret = EEXIST;
	}

	if (EOK != oal_mutex_unlock(&db->lock))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief			Delete new address from database
 * @param[in]		db Pointer to MAC database instance
 * @param[in]		addr The MAC address to delete from database
 * @return			Execution status, EOK if success, error code otherwise
 */
errno_t pfe_mac_db_del_addr(pfe_mac_db_t *db, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	errno_t ret;
	pfe_mac_db_list_entry_t *entry;

	if (EOK != oal_mutex_lock(&db->lock))
	{
		NXP_LOG_ERROR("mutex lock failed\n");
	}

	entry = pfe_mac_db_find_by_addr(db, addr, owner);
	if (NULL == entry)
	{
		NXP_LOG_WARNING("MAC address was not found\n");
		ret = ENOENT;
	}
	else
	{
		/*
		 * Move database iterator pointer to next item to handle situation when item proposed to delete
		 * is matching with item pointed by db->iterator. This could happen for example if there is request
		 * to delete more than one entry between pfe_mac_get_first_addr() and pfe_mac_get_next_addr() or
		 * between two consecutive calls of pfe_mac_get_next_addr().
		 */
		if(&entry->iterator == db->iterator)
		{
			db->iterator = db->iterator->prNext;
		}

		LLIST_Remove(&entry->iterator);
		oal_mm_free(entry);
		entry = NULL;
		ret = EOK;
	}

	if (EOK != oal_mutex_unlock(&db->lock))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Flush all addresses matching with input rule
 * @param[in]	db Pointer to MAC database instance
 * @param[in]	crit All, Owner, Type or Owner&Type criterion
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @param[in]	owner Required owner of MAC address
 * @return		EOK success, error code otherwise
 */
errno_t pfe_mac_db_flush(pfe_mac_db_t *db, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner)
{
	errno_t ret = EOK;
	LLIST_t *item, *aux;
	pfe_mac_db_list_entry_t *entry;

	if (EOK != oal_mutex_lock(&db->lock))
	{
		NXP_LOG_ERROR("mutex lock failed\n");

	}
	/*	Remove associated MAC addresses due to flush mode */
	LLIST_ForEachRemovable(item, aux, &db->mac_list)
	{
		entry = LLIST_Data(item, pfe_mac_db_list_entry_t, iterator);
		if (NULL != entry)
		{
			if (TRUE == pfe_mac_db_criterion_eval(entry, crit, type, owner))
			{
				LLIST_Remove(&entry->iterator);
				oal_mm_free(entry);
				entry = NULL;
			}
		}
	}

	if (EOK != oal_mutex_unlock(&db->lock))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get first MAC address from database matching by input rule. Function stores database context
 * 				for following call of pfe_mac_get_next_addr(). Function should not be called internally
 * 				inside this module
 * @param[in]	db Pointer to MAC database instance
 * @param[in]	crit All, Owner, Type or Owner&Type criterion
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @param[in]	owner Required owner of MAC address
 * @param[out]	addr Returned MAC address
 * @return		Execution status, EOK success, error code otherwise
 */
errno_t pfe_mac_db_get_first_addr(pfe_mac_db_t *db, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner, pfe_mac_addr_t addr)
{
	errno_t ret = EOK;
	const pfe_mac_db_list_entry_t *entry = NULL;
	LLIST_t *item;
	bool_t found_match = FALSE;

	if (EOK != oal_mutex_lock(&db->lock))
	{
		NXP_LOG_ERROR("mutex lock failed\n");
	}

	if (TRUE == LLIST_IsEmpty(&db->mac_list))
	{
		ret = ENOENT;
	}
	else
	{
		LLIST_ForEach(item, &db->mac_list)
		{
			entry = LLIST_Data(item, pfe_mac_db_list_entry_t, iterator);
			if ((NULL != entry))
			{
				if (TRUE == pfe_mac_db_criterion_eval(entry, crit, type, owner))
				{
					found_match = TRUE;
					break;
				}
			}
		}
	}

	if ((NULL != entry) && (found_match != FALSE))
	{
		(void) memcpy(addr, entry->addr, sizeof(pfe_mac_addr_t));
		db->iterator = item->prNext;
		db->crit.crit = crit;
		db->crit.owner = owner;
		db->crit.type = type;
	}
	else
	{
		ret = ENOENT;
	}

	if (EOK != oal_mutex_unlock(&db->lock))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get next MAC address from database. Function expect that pfe_mac_get_first_addr() was
 * 				executed before and stores database context. Function should not be called internally
 * 				inside this module
 * @param[in]	db Pointer to MAC database instance
 * @param[out]	addr Returned MAC address
 * @return		Execution status, EOK success, error code otherwise
 */
errno_t pfe_mac_db_get_next_addr(pfe_mac_db_t *db, pfe_mac_addr_t addr)
{
	errno_t ret = EOK;
	const pfe_mac_db_list_entry_t *entry = NULL;
	LLIST_t *item;
	bool_t found_match = FALSE;

	if (EOK != oal_mutex_lock(&db->lock))
	{
		NXP_LOG_ERROR("mutex lock failed\n");
	}

	item = db->iterator;

	while(db->iterator != &db->mac_list)
	{
		entry = LLIST_Data(item, pfe_mac_db_list_entry_t, iterator);
		if ((NULL != entry))
		{
			if (TRUE == pfe_mac_db_criterion_eval(entry, db->crit.crit, db->crit.type, db->crit.owner))
			{
				found_match = TRUE;
				break;
			}
		}
		db->iterator = db->iterator->prNext;
	}

	if ((NULL != entry) && (found_match != FALSE))
	{
		(void) memcpy(addr, entry->addr, sizeof(pfe_mac_addr_t));
		db->iterator = item->prNext;
	}
	else
	{
		ret = ENOENT;
		db->iterator = &db->mac_list;
	}

	if (EOK != oal_mutex_unlock(&db->lock))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}

	return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

