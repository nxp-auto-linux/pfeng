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
 * @file		pfe_if_db.c
 * @brief		Interface database
 *
 * @warning		All API calls related to a single DB instance must be protected
 * 				from being preempted by another API calls related to the same
 * 				DB instance.
 *
 */

#include "oal.h"
#include "linked_list.h"
#include "pfe_if_db.h"

struct __pfe_if_db_tag
{
	pfe_if_db_type_t type;
	LLIST_t theList;
	LLIST_t *cur_item;					/*	Current entry to be returned. See ...get_first() and ...get_next() */
	pfe_if_db_get_criterion_t cur_crit;	/*	Current criterion */
	union
	{
		uint8_t log_if_id;
		pfe_ct_phy_if_id_t phy_if_id;
		void *iface;
	} cur_crit_arg;	/*	Current criterion argument */
};

struct __pfe_if_db_entry_tag
{
	pfe_ct_phy_if_id_t owner;

	union
	{
		pfe_log_if_t *log_if;
		pfe_phy_if_t *phy_if;
		void *iface;
	};

	/*	DB/Chaining */
	LLIST_t list_member;
};

static bool_t pfe_if_db_match_criterion(pfe_if_db_t *db, pfe_if_db_entry_t *entry);

/**
 * @brief		Match entry with latest criterion provided via pfe_if_db_get_first()
 * @param[in]	db The interface DB instance
 * @param[in]	entry The entry to be matched
 * @retval		TRUE Entry matches the criterion
 * @retval		FALSE Entry does not match the criterion
 */
static bool_t pfe_if_db_match_criterion(pfe_if_db_t *db, pfe_if_db_entry_t *entry)
{
	bool_t match = FALSE;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	switch (db->cur_crit)
	{
		case IF_DB_CRIT_ALL:
		{
			match = TRUE;
			break;
		}

		case IF_DB_CRIT_BY_ID:
		{
			if (PFE_IF_DB_LOG == db->type)
			{
				match = (db->cur_crit_arg.log_if_id == pfe_log_if_get_id(entry->log_if));
			}
			else
			{
				match = (db->cur_crit_arg.phy_if_id == pfe_phy_if_get_id(entry->phy_if));
			}

			break;
		}

		case IF_DB_CRIT_BY_INSTANCE:
		{
			match = (db->cur_crit_arg.iface == entry->iface);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown criterion\n");
			match = FALSE;
		}
	}

	return match;
}

/**
 * @brief		Create DB
 * @param[in]	Database type: Logical or Physical interfaces
 * @return		The DB instance or NULL if failed
 */
pfe_if_db_t * pfe_if_db_create(pfe_if_db_type_t type)
{
	pfe_if_db_t *db;

	if ((type != PFE_IF_DB_PHY) && (type != PFE_IF_DB_LOG))
	{
		return NULL;
	}

	db = oal_mm_malloc(sizeof(pfe_if_db_t));
	if (NULL == db)
	{
		return NULL;
	}
	else
	{
		memset(db, 0, sizeof(pfe_if_db_t));
	}

	LLIST_Init(&db->theList);
	db->cur_item = db->theList.prNext;
	db->type = type;

	return db;
}

/**
 * @brief		Destroy DB
 * @param[in]	db The DB instance
 */
void pfe_if_db_destroy(pfe_if_db_t *db)
{
	if (NULL != db)
	{
		oal_mm_free(db);
	}
}

/**
 * @brief		Get physical interface instance from database entry
 * @param[in]	entry The entry
 * @return		Physical interface instance
 */
__attribute__((pure)) pfe_phy_if_t *pfe_if_db_entry_get_phy_if(pfe_if_db_entry_t *entry)
{
	if (NULL != entry)
	{
		return entry->phy_if;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Get logical interface instance from database entry
 * @param[in]	entry The entry
 * @return		Logical interface instance
 */
__attribute__((pure)) pfe_log_if_t *pfe_if_db_entry_get_log_if(pfe_if_db_entry_t *entry)
{
	if (NULL != entry)
	{
		return entry->log_if;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Add interface instance to DB
 * @param[in]	db The interface DB instance
 * @param[in]	iface The interface instance
 * @param[in]	owner Owner of the entry
 * @retval		EOK Success
 * @retval		ENOMEM Memory allocation failed
 * @retval		EPERM Attempt to insert already existing entry
 */
errno_t pfe_if_db_add(pfe_if_db_t *db, void *iface, pfe_ct_phy_if_id_t owner)
{
	pfe_if_db_entry_t *new_entry;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Check duplicates */
	new_entry = pfe_if_db_get_first(db, IF_DB_CRIT_BY_INSTANCE, iface);
	if (NULL == new_entry)
	{
		new_entry = oal_mm_malloc(sizeof(pfe_if_db_entry_t));
		if (NULL == new_entry)
		{
			return ENOMEM;
		}
		else
		{
			memset(new_entry, 0, sizeof(pfe_if_db_entry_t));
		}
	}
	else
	{
		/*	Don't allow duplicates */
		return EPERM;
	}

	/*	Store values */
	new_entry->iface = iface;

	/*	Put to DB */
	LLIST_AddAtEnd(&(new_entry->list_member), &db->theList);

	return EOK;
}

/**
 * @brief		Remove entry from DB
 * @param[in]	db The interface DB instance
 * @param[in]	entry Entry to be removed. If the call is successful the entry
 * 					  becomes invalid and shall not be accessed.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_if_db_remove(pfe_if_db_t *db, pfe_if_db_entry_t *entry)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (&entry->list_member == db->cur_item)
	{
		/*	Remember the change so we can call remove() between get_first()
			and get_next() calls. */
		db->cur_item = db->cur_item->prNext;
	}

	LLIST_Remove(&(entry->list_member));
	oal_mm_free(entry);

	return EOK;
}

/**
 * @brief		Get first record from the DB matching given criterion
 * @details		Intended to be used with pfe_if_db_get_next
 * @param[in]	db The interface DB instance
 * @param[in]	crit Get criterion
 * @param[in]	art Pointer to criterion argument
 * @return		The entry or NULL if not found
 * @warning		The returned entry must not be accessed after pfe_if_db_remove(entry)
 *				or pfe_if_db_drop_all() has been called.
 */
pfe_if_db_entry_t *pfe_if_db_get_first(pfe_if_db_t *db, pfe_if_db_get_criterion_t crit, void *arg)
{
	pfe_if_db_entry_t *entry = NULL;
	LLIST_t *item;
	bool_t match = FALSE;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == arg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Remember criterion and argument for possible subsequent pfe_log_if_db_get_next() calls */
	db->cur_crit = crit;
	switch (db->cur_crit)
	{
		case IF_DB_CRIT_ALL:
		{
			break;
		}

		case IF_DB_CRIT_BY_ID:
		{
			db->cur_crit_arg.log_if_id = (uint8_t)((addr_t)arg & 0xff);
			break;
		}

		case IF_DB_CRIT_BY_INSTANCE:
		{
			db->cur_crit_arg.iface = arg;
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown criterion\n");
			return NULL;
		}
	}

	if (FALSE == LLIST_IsEmpty(&db->theList))
	{
		/*	Get first matching entry */
		LLIST_ForEach(item, &db->theList)
		{
			/*	Get data */
			entry = LLIST_Data(item, pfe_if_db_entry_t, list_member);

			/*	Remember current item to know where to start later */
			db->cur_item = item->prNext;
			if (NULL != entry)
			{
				if (TRUE == pfe_if_db_match_criterion(db, entry))
				{
					match = TRUE;
					break;
				}
			}
		}
	}

	if (TRUE == match)
	{
		return entry;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Get next record from the DB
 * @details		Intended to be used with pfe_if_db_get_first.
 * @param[in]	db The interface DB instance
 * @return		The entry or NULL if not found
 * @warning		The returned entry must not be accessed after pfe_if_db_remove(entry)
 *				or pfe_if_db_drop_all() has been called.
 */
pfe_if_db_entry_t *pfe_if_db_get_next(pfe_if_db_t *db)
{
	pfe_if_db_entry_t *entry;
	bool_t match = FALSE;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (db->cur_item == &db->theList)
	{
		/*	No more entries */
		entry = NULL;
	}
	else
	{
		while (db->cur_item!=&db->theList)
		{
			/*	Get data */
			entry = LLIST_Data(db->cur_item, pfe_if_db_entry_t, list_member);

			/*	Remember current item to know where to start later */
			db->cur_item = db->cur_item->prNext;

			if (NULL != entry)
			{
				if (TRUE == pfe_if_db_match_criterion(db, entry))
				{
					match = TRUE;
					break;
				}
			}
		}
	}

	if (TRUE == match)
	{
		return entry;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Remove all entries
 * @param[in]	db The route DB instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_log_if_db_drop_all(pfe_if_db_t *db)
{
	LLIST_t *item, *aux;
	pfe_if_db_entry_t *entry;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	LLIST_ForEachRemovable(item, aux, &db->theList)
	{
		entry = LLIST_Data(item, pfe_if_db_entry_t, list_member);

		LLIST_Remove(item);

		oal_mm_free(entry);
	}

	return EOK;
}

/** @}*/
