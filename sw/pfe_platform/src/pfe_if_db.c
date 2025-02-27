/* =========================================================================
 *  Copyright 2017-2025 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "linked_list.h"
#include "pfe_if_db.h"

/* Time for sleep between polling in ms */
#define IF_DB_LOCK_POLLING_SLEEP		20UL

typedef	union
{
    uint8_t if_id;
    void *iface;
    char_t *name;
    pfe_ct_phy_if_id_t owner;
} crit_arg_t;	/*	Current criterion argument */

struct pfe_if_db_tag
{
	pfe_if_db_type_t type;
	LLIST_t theList;
	LLIST_t *cur_item;					/*	Current entry to be returned. See ...get_first() and ...get_next() */
	pfe_if_db_get_criterion_t cur_crit;	/*	Current criterion */
	crit_arg_t cur_crit_arg;	/*	Current criterion argument */
};

struct pfe_if_db_entry_tag
{
	pfe_ct_phy_if_id_t owner;

	union
	{
		pfe_log_if_t *log_if;
		pfe_phy_if_t *phy_if;
		void *iface;
	} info;

	/*	DB/Chaining */
	LLIST_t list_member;
};

typedef struct
{
	uint32_t session_id;	/* Locked by session ID */
	uint32_t seed;			/* Starting seed for session ID */
	oal_mutex_t mutex;		/* Mutex to access this structure data */
	bool_t is_initialized;	/* Flag indicating that mutex is initialized */
	bool_t is_locked;		/* Information, that IF_DB lock is acquired by somebody */
	uint64_t lock_time;		/* Timestamp in ms when was last lock acquired in milliseconds*/
} if_db_context_t;

/**
 * @brief	Global intefrace DB lock. Module-local singleton.
 */
static if_db_context_t if_db_context;

static bool_t pfe_if_db_match_criterion(const pfe_if_db_t *db, pfe_if_db_get_criterion_t crit, const crit_arg_t *arg, const pfe_if_db_entry_t *entry);
static errno_t pfe_if_db_check_precondition(const if_db_context_t *pr_if_db_context, uint32_t session_id);
static pfe_if_db_entry_t *pfe_if_db_get_first_entry(pfe_if_db_t *db);
static pfe_if_db_entry_t *pfe_if_db_get_single_entry(const pfe_if_db_t *db, pfe_if_db_get_criterion_t crit, crit_arg_t argument);

/**
 * @brief		Check preconditions before performing operation
 * @param[in]	pr_if_db_context
 * @retval		EOK Preconditions are fulfilled
 * @retval		PERM Preconditions are not fulfilled
 * @warning		context should be locked before call
 */
static errno_t pfe_if_db_check_precondition(const if_db_context_t *pr_if_db_context, uint32_t session_id)
{
	errno_t ret;

	if(FALSE == pr_if_db_context->is_locked)
	{
		NXP_LOG_DEBUG("DB access not permitted\n");
		ret = EPERM;
	}
	else if(session_id != pr_if_db_context->session_id)
	{
		NXP_LOG_DEBUG("Incorrect session ID\n");
		ret = EPERM;
	}
	else
	{
		ret = EOK;
	}
	return ret;
}

/**
 * @brief		Match entry with latest criterion provided via pfe_if_db_get_first()
 * @param[in]	db The interface DB instance
 * @param[in]	crit Criterion to search
 * @param[in]	arg Criterion arguments
 * @param[in]	entry The entry to be matched
 * @retval		TRUE Entry matches the criterion
 * @retval		FALSE Entry does not match the criterion
 */
static bool_t pfe_if_db_match_criterion(const pfe_if_db_t *db, pfe_if_db_get_criterion_t crit, const crit_arg_t *arg, const pfe_if_db_entry_t *entry)
{
	bool_t match;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		match = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch (crit)
		{
			case IF_DB_CRIT_ALL:
			{
				match = TRUE;
				break;
			}

			case IF_DB_CRIT_BY_ID:
			{
#ifdef PFE_CFG_PFE_MASTER
				if (PFE_IF_DB_LOG == db->type)
				{
					match = (arg->if_id == (uint8_t)pfe_log_if_get_id(entry->info.log_if));
				}
				else
#endif /* PFE_CFG_PFE_MASTER */
				{
					match = (arg->if_id == (uint8_t)pfe_phy_if_get_id(entry->info.phy_if));
				}

				break;
			}

			case IF_DB_CRIT_BY_INSTANCE:
			{
				match = (arg->iface == entry->info.iface);
				break;
			}

			case IF_DB_CRIT_BY_NAME:
			{
#ifdef PFE_CFG_PFE_MASTER
				if (PFE_IF_DB_LOG == db->type)
				{
					match = (0 == strcmp(arg->name, pfe_log_if_get_name(entry->info.log_if)));
				}
				else
#endif /* PFE_CFG_PFE_MASTER */
				{
					match = (0 == strcmp(arg->name, pfe_phy_if_get_name(entry->info.phy_if)));
				}

				break;
			}

			case IF_DB_CRIT_BY_OWNER:
			{
				match = (arg->owner == entry->owner);
				break;
			}

			default:
			{
				NXP_LOG_WARNING("Unknown criterion\n");
				match = FALSE;
				break;
			}
		}
	}
	return match;
}

/**
 * @brief		Get first record from the DB matching given criterion
 * @details		Intended to be used with pfe_if_db_get_next
 * @param[in]	db The interface DB instance
 * @return		entry The entry or NULL if not found
 */
static pfe_if_db_entry_t *pfe_if_db_get_first_entry(pfe_if_db_t *db)
{
	bool_t match = FALSE;
	LLIST_t *curItem;
	pfe_if_db_entry_t *entry = NULL;

	if (FALSE == LLIST_IsEmpty(&db->theList))
	{
		/*	Get first matching entry */
		LLIST_ForEach(curItem, &db->theList)
		{
			/*	Get data */
			entry = LLIST_Data(curItem, pfe_if_db_entry_t, list_member);

			/*	Remember current item to know where to start later */
			db->cur_item = curItem->prNext;
			if (NULL != entry)
			{
				if (TRUE == pfe_if_db_match_criterion(db, db->cur_crit, &db->cur_crit_arg, entry))
				{
					match = TRUE;
					break;
				}
			}
		}
	}

	if (EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_ERROR("DB mutex unlock failed\n");
	}

	if (FALSE == match)
	{
		/* No match found */
		entry = NULL;
	}

	return entry;
}

/**
 * @brief		Get first record from the DB matching given criterion without changing previous
 *				search criteria
 * @param[in]	db The interface DB instance
 * @param[in]	crit Get criterion
 * @param[in]	arg Pointer to criterion argument
 * @return		entry The entry or NULL if not found
 */
static pfe_if_db_entry_t *pfe_if_db_get_single_entry(const pfe_if_db_t *db, pfe_if_db_get_criterion_t crit, crit_arg_t argument)
{
	pfe_if_db_entry_t *entry = NULL;
	bool_t             match = FALSE;
	LLIST_t 		  *curItem;

	if (FALSE == LLIST_IsEmpty(&db->theList))
	{
		/*	Get first matching entry */
		LLIST_ForEach(curItem, &db->theList)
		{
			/*	Get data */
			entry = LLIST_Data(curItem, pfe_if_db_entry_t, list_member);

			/*	Remember current item to know where to start later */
			if (NULL != entry)
			{
				if (TRUE == pfe_if_db_match_criterion(db, crit, &argument, entry))
				{
					match = TRUE;
					break;
				}
			}
		}
	}

	if (EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_ERROR("DB mutex unlock failed\n");
	}

	if (FALSE == match)
	{
		/* No match found */
		entry = NULL;
	}

	return entry;
}

/**
 * @brief		Clear IF_DB Context structure
 * @details		Initialize and set if_db_context static structure variable 
 * 				to set after start, or after STR resume
 * @return		EOK if initialized, EPERM if double called
 */
errno_t pfe_if_db_context_init(void)
{
	errno_t ret = EPERM;

	/* Create global DB lock */
	if (FALSE == if_db_context.is_initialized)
	{
		ret = oal_mutex_init(&if_db_context.mutex);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Mutex initialization failed\n");
		}
		else
		{
			if_db_context.is_initialized = TRUE;

			/* Initialize data to safe values */
			if_db_context.is_locked = FALSE;
			if_db_context.session_id = 0U;

			/* Initialize seed to some value */
			if_db_context.seed = 123U;

			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Clear IF_DB Context structure
 * @details		Destroy mutex instance on driver remove
 */
errno_t pfe_if_db_context_destroy(void)
{
	errno_t ret = EPERM;

	if (TRUE == if_db_context.is_initialized)
	{
		ret = oal_mutex_lock(&if_db_context.mutex);
		if(EOK == ret)
		{
			if_db_context.is_locked = TRUE;

			ret = oal_mutex_unlock(&if_db_context.mutex);
			if(EOK == ret)
			{
				ret = oal_mutex_destroy(&if_db_context.mutex);
				if(EOK == ret)
				{
					if_db_context.is_initialized = FALSE;
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Create IF DB instance
 * @param[in]	Database type: Logical or Physical interfaces
 * @return		The DB instance or NULL if failed
 */
pfe_if_db_t * pfe_if_db_create(pfe_if_db_type_t type)
{
	pfe_if_db_t *db;

	if ((type != PFE_IF_DB_PHY) && (type != PFE_IF_DB_LOG))
	{
		NXP_LOG_ERROR("Not valid IF_DB type to create\n");
		db = NULL;
	}
	else
	{
		db = oal_mm_malloc(sizeof(pfe_if_db_t));
		if (NULL == db)
		{
			NXP_LOG_ERROR("Unable to allocate memory\n");
		}
		else
		{
			(void)memset(db, 0, sizeof(pfe_if_db_t));

			LLIST_Init(&db->theList);
			db->cur_item = db->theList.prNext;
			db->type = type;
		}
	}

	return db;
}

/**
 * @brief		Destroy IF DB instance
 * @param[in]	db The DB instance
 */
void pfe_if_db_destroy(const pfe_if_db_t *db)
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
__attribute__((pure)) pfe_phy_if_t *pfe_if_db_entry_get_phy_if(const pfe_if_db_entry_t *entry)
{
	pfe_phy_if_t *phy_if_entry;
	if (NULL != entry)
	{
		phy_if_entry = entry->info.phy_if;
	}
	else
	{
		phy_if_entry = NULL;
	}
	return phy_if_entry;
}

/**
 * @brief		Get logical interface instance from database entry
 * @param[in]	entry The entry
 * @return		Logical interface instance
 */
__attribute__((pure)) pfe_log_if_t *pfe_if_db_entry_get_log_if(const pfe_if_db_entry_t *entry)
{
	pfe_log_if_t *log_if_entry;
	if (NULL != entry)
	{
		log_if_entry = entry->info.log_if;
	}
	else
	{
		log_if_entry = NULL;
	}
	return log_if_entry;
}

/**
 * @brief		Add interface instance to DB
 * @param[in]	db The interface DB instance
 * @param[in]	session_id ID of active session
 * @param[in]	iface The interface instance
 * @param[in]	owner Owner of the entry
 * @retval		EOK Success
 * @retval		ENOMEM Memory allocation failed
 * @retval		EPERM Attempt to insert already existing entry/Incorrect session ID
 */
errno_t pfe_if_db_add(pfe_if_db_t *db, uint32_t session_id, void *iface, pfe_ct_phy_if_id_t owner)
{
	pfe_if_db_entry_t *new_entry = NULL;
	errno_t            ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Check duplicates */
		ret = pfe_if_db_get_first(db, session_id, IF_DB_CRIT_BY_INSTANCE, iface, &new_entry);
		if (EOK != oal_mutex_lock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("DB mutex lock failed\n");
		}

		/* Check condition if operation on DB is allowed */
		if (EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
		{
			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}
			ret = EPERM;
		}
		else
		{

			if ((NULL == new_entry) && (EOK == ret))
			{
				new_entry = oal_mm_malloc(sizeof(pfe_if_db_entry_t));
				if (NULL == new_entry)
				{
					NXP_LOG_ERROR("Unable to allocate memory\n");
					ret = ENOMEM;
				}
				else
				{
					(void)memset(new_entry, 0, sizeof(pfe_if_db_entry_t));
				}
			}
			else
			{
				/*	Don't allow duplicates */
				ret = EPERM;
			}

			if (NULL != new_entry)
			{
				/*	Store values */
				new_entry->info.iface = iface;
				new_entry->owner = owner;

				/*	Put to DB */
				LLIST_AddAtEnd(&(new_entry->list_member), &db->theList);
			}

			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}
		}
	}
	return ret;
}

/**
 * @brief		Remove entry from DB
 * @param[in]	db The interface DB instance
 * @param[in]	session_id ID of active session
 * @param[in]	entry Entry to be removed. If the call is successful the entry
 * 					  becomes invalid and shall not be accessed.
 * @return		EOK if success, error code otherwise
 * @retval		EPERM Incorrect session ID or DB not locked
 */
errno_t pfe_if_db_remove(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_entry_t *entry)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("DB mutex lock failed\n");
		}

		/* Check condition if operation on DB is allowed */
		if (EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
		{
			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}
			ret = EPERM;
		}
		else
		{

			if (&entry->list_member == db->cur_item)
			{
				/*	Remember the change so we can call remove() between get_first()
			and get_next() calls. */
				db->cur_item = db->cur_item->prNext;
			}

			LLIST_Remove(&(entry->list_member));
			oal_mm_free(entry);

			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}

			ret = EOK;
		}
	}
	return ret;
}

/**
 * @brief		Get first record from the DB matching given criterion
 * @details		Intended to be used with pfe_if_db_get_next
 * @param[in]	db The interface DB instance
 * @param[in]	session_id ID of active session
 * @param[in]	crit Get criterion
 * @param[in]	arg Pointer to criterion argument
 * @param[out]	entry The entry or NULL if not found
 * @return		EOK entry returned is valid
 * @return		EPERM db was locked by someone else, entry returned is not valid
 * @warning		The returned entry must not be accessed after pfe_if_db_remove(entry)
 *				or pfe_if_db_drop_all() has been called.
 */
errno_t pfe_if_db_get_first(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_get_criterion_t crit, void *arg, pfe_if_db_entry_t **db_entry)
{
	errno_t            ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == db_entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("DB mutex lock failed\n");
		}

		/* Check condition if operation on DB is allowed */
		if (EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
		{
			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}
			ret = EPERM;
		}
		else
		{
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
					db->cur_crit_arg.if_id = (uint8_t)((addr_t)arg & 0xffU);
					break;
				}

				case IF_DB_CRIT_BY_INSTANCE:
				{
#if defined(PFE_CFG_NULL_ARG_CHECK)
					if (unlikely(NULL == arg))
					{
						NXP_LOG_ERROR("NULL argument received\n");
						*db_entry = NULL;
						ret = EINVAL;
					}
					else
#endif /* PFE_CFG_NULL_ARG_CHECK */
					{
						db->cur_crit_arg.iface = arg;
					}
					break;
				}

				case IF_DB_CRIT_BY_NAME:
				{
#if defined(PFE_CFG_NULL_ARG_CHECK)
					if (unlikely(NULL == arg))
					{
						NXP_LOG_ERROR("NULL argument received\n");
						*db_entry = NULL;
						ret = EINVAL;
					}
					else
#endif /* PFE_CFG_NULL_ARG_CHECK */
					{
						db->cur_crit_arg.name = (char_t *)arg;
					}
					break;
				}

				case IF_DB_CRIT_BY_OWNER:
				{
					db->cur_crit_arg.owner = (pfe_ct_phy_if_id_t)((addr_t)arg & 0xffU);
					break;
				}

				default:
				{
					NXP_LOG_WARNING("Unknown criterion\n");
					ret = EPERM;
					break;
				}
			}

			if (ret == EOK)
			{
				*db_entry = pfe_if_db_get_first_entry(db);
			}
		}
	}
	return ret;
}

/**
 * @brief		Get first record from the DB matching given criterion without changing previous
 *				search criteria
 * @details		Intended to be used for nested DB search where only a single match is expected (i.g. by
 *				unique ID). The function does not change saved criterion from the pfe_if_db_get_first()
 *				call thus the pfe_if_db_get_next() will be able to continue the search initiated by
 *				the pfe_if_db_get_first() call.
 * @param[in]	db The interface DB instance
 * @param[in]	session_id ID of active session
 * @param[in]	crit Get criterion
 * @param[in]	arg Pointer to criterion argument
 * @param[out]	entry The entry or NULL if not found
 * @return		EOK entry returned is valid
 * @return		EPERM db was locked by someone else, entry returned is not valid
 * @warning		The returned entry must not be accessed after pfe_if_db_remove(entry)
 *				or pfe_if_db_drop_all() has been called.
 */
errno_t pfe_if_db_get_single(const pfe_if_db_t *db, uint32_t session_id, pfe_if_db_get_criterion_t crit, void *arg, pfe_if_db_entry_t **db_entry)
{
	crit_arg_t         argument;
	errno_t            ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == db_entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Convert argument to database known format */
		switch (crit)
		{
			case IF_DB_CRIT_ALL:
			{
				break;
			}

			case IF_DB_CRIT_BY_ID:
			{
				argument.if_id = (uint8_t)((addr_t)arg & 0xffU);
				break;
			}

			case IF_DB_CRIT_BY_INSTANCE:
			{
#if defined(PFE_CFG_NULL_ARG_CHECK)
				if (unlikely(NULL == arg))
				{
					NXP_LOG_ERROR("NULL argument received\n");
					*db_entry = NULL;
					ret = EINVAL;
				}
				else
#endif /* PFE_CFG_NULL_ARG_CHECK */
				{
					argument.iface = arg;
				}
				break;
			}

			case IF_DB_CRIT_BY_NAME:
			{
#if defined(PFE_CFG_NULL_ARG_CHECK)
				if (unlikely(NULL == arg))
				{
					NXP_LOG_ERROR("NULL argument received\n");
					*db_entry = NULL;
					ret = EINVAL;
				}
				else
#endif /* PFE_CFG_NULL_ARG_CHECK */
				{
					argument.name = (char_t *)arg;
				}
				break;
			}

			case IF_DB_CRIT_BY_OWNER:
			{
				argument.owner = (pfe_ct_phy_if_id_t)((addr_t)arg & 0xffU);
				break;
			}

			default:
			{
				NXP_LOG_WARNING("Unknown criterion\n");
				ret = EPERM;
				break;
			}
		}
		if (ret == EOK)
		{
			if (EOK != oal_mutex_lock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex lock failed\n");
			}

			/* Check condition if operation on DB is allowed */
			if (EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
			{
				if (EOK != oal_mutex_unlock(&if_db_context.mutex))
				{
					NXP_LOG_ERROR("DB mutex unlock failed\n");
				}
				ret = EPERM;
			}
			if (ret != EPERM)
			{
				*db_entry = pfe_if_db_get_single_entry(db, crit, argument);
			}
		}
	}
	return ret;
}

/**
 * @brief		Get next record from the DB
 * @details		Intended to be used with pfe_if_db_get_first.
 * @param[in]	db The interface DB instance
 * @param[in]	session_id ID of active session
 * @param[out]	entry The entry or NULL if not found
 * @return		EOK entry returned is valid
 * @return		EPERM db was locked by someone else, entry returned is not valid
 * @warning		The returned entry must not be accessed after pfe_if_db_remove(entry)
 *				or pfe_if_db_drop_all() has been called.
 */
errno_t pfe_if_db_get_next(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_entry_t **db_entry)
{
	pfe_if_db_entry_t *entry;
	errno_t            ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == db_entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("DB mutex lock failed\n");
		}

		/* Check condition if operation on DB is allowed */
		if (EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
		{
			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}
			ret = EPERM;
		}
		else
		{
			entry = NULL;
			if (FALSE == LLIST_IsEmpty(&db->theList))
			{
				if (db->cur_item == &db->theList)
				{
					/*	No more entries */
				}
				else
				{
					while (db->cur_item != &db->theList)
					{
						/*	Get data */
						entry = LLIST_Data(db->cur_item, pfe_if_db_entry_t, list_member);

						/*	Remember current item to know where to start later */
						db->cur_item = db->cur_item->prNext;

						if (NULL != entry)
						{
							if (TRUE == pfe_if_db_match_criterion(db, db->cur_crit, &db->cur_crit_arg, entry))
							{
								break;
							}
							else
							{
								/* clean entry to not get it false positive */
								entry = NULL;
							}
						}
					}
				}
			}

			*db_entry = entry;

			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}

			ret = EOK;
		}
	}
	return ret;
}

/**
 * @brief		Remove all entries
 * @param[in]	db The route DB instance
 * @param[in]	session_id ID of active session
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_log_if_db_drop_all(const pfe_if_db_t *db, uint32_t session_id)
{
	LLIST_t *                curItem, *aux;
	const pfe_if_db_entry_t *entry;
	errno_t                  ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("DB mutex lock failed\n");
		}

		/* Check condition if operation on DB is allowed */
		if (EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
		{
			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}
			ret = EPERM;
		}
		else
		{

			LLIST_ForEachRemovable(curItem, aux, &db->theList)
			{
				entry = LLIST_Data(curItem, pfe_if_db_entry_t, list_member);

				LLIST_Remove(curItem);

				oal_mm_free(entry);
			}

			ret = EOK;

			if (EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("DB mutex unlock failed\n");
			}
		}
	}
	return ret;
}

/**
 * @brief		Lock the DB and return generated session ID
 * @param[out]	session_id ID of locked session
 * @return		EOK if success, EAGAIN if lock is already locked
 */
errno_t pfe_if_db_lock(uint32_t *session_id)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == session_id))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Lock global if DB mutex */
		if (EOK != oal_mutex_lock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("DB mutex lock failed\n");
		}

		/* IF_DB Lock is acquired by another context, but lock timeout occurred, steal the lock */
		if (TRUE == if_db_context.is_locked &&
			PFE_CFG_IF_DB_MAX_LOCK_TIME <= (oal_time_gettime() - if_db_context.lock_time))
		{
			NXP_LOG_WARNING("IF_DB is already locked for too long, stealing the lock\n");

			++if_db_context.seed;
			if_db_context.session_id = if_db_context.seed << 4U;
			*session_id = if_db_context.session_id;

			/* Save time when the lock was acquired */
			if_db_context.lock_time = oal_time_gettime();
			ret = EOK;
		}
		/* IF_DB Lock is available to acquire */
		else if (FALSE == if_db_context.is_locked)
		{
			/* Increment seed id */
			++if_db_context.seed;

			/* Store session ID and reserve 0 - 15 for named sessions */
			if_db_context.session_id = if_db_context.seed << 4U;

			/* Pass session id to caller*/
			*session_id = if_db_context.session_id;
			if_db_context.is_locked = TRUE;

			/* Save time when the lock was acquired */
			if_db_context.lock_time = oal_time_gettime();
			ret = EOK;
		}
		/* is_locked==TRUE && timer_expired==FALSE */
		else
		{
			ret = EAGAIN;
		}

		/* Unlock global if DB mutex */
		if (EOK != oal_mutex_unlock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("DB mutex unlock failed\n");
		}
	}
	return ret;
}

/**
 * @brief		Lock the DB with provided owner ID
 * @param[in]	owner_id ID of owner in range 0 - 15
 * @return		EOK if success, EAGAIN if lock is already locked, EINVAL if ID is not in range
 */
errno_t pfe_if_db_lock_owned(uint32_t owner_id)
{
	errno_t ret = EOK;

	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_ERROR("DB mutex lock failed\n");
	}

	/* Check if session ID is in ok range */
	if (16U > owner_id)
	{
		/* IF_DB Lock is acquired by another context, but lock timeout occurred, steal the lock */
		if (TRUE == if_db_context.is_locked  &&
			PFE_CFG_IF_DB_MAX_LOCK_TIME <= (oal_time_gettime() - if_db_context.lock_time))
		{
			NXP_LOG_WARNING("IF_DB is already locked for too long, stealing the lock\n");

			if_db_context.session_id = owner_id;
			if_db_context.lock_time = oal_time_gettime();
			ret = EOK;
		}
		/* IF_DB is unlocked */
		else if (FALSE == if_db_context.is_locked)
		{
			if_db_context.session_id = owner_id;
			if_db_context.is_locked = TRUE;
			if_db_context.lock_time = oal_time_gettime();
			ret = EOK;
		}
		/* is_locked==TRUE && timer_expired==FALSE */
		else
		{
			ret = EAGAIN;
		}
	}
	else
	{/* Invalid owner ID range*/
		ret = EINVAL;
	}

	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_ERROR("DB mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Unlock the DB with owner ID/session ID
 * @param[in]	session_id ID of owner or session
 * @return		EOK if success, EAGAIN if db is not locked by session_id
 */
errno_t pfe_if_db_unlock(uint32_t session_id)
{
	errno_t ret = EAGAIN;

	/* Lock global if DB mutex */
	if (EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_ERROR("DB mutex lock failed\n");
	}

	if ((TRUE == if_db_context.is_locked) && (session_id == if_db_context.session_id))
	{
		/* Discard key and set locked to FALSE*/
		if_db_context.session_id = (~if_db_context.session_id) << 4U;

		/* Set is locked to FALSE */
		if_db_context.is_locked = FALSE;

		ret = EOK;
	}

	/* Unlock global if DB mutex */
	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_ERROR("DB mutex unlock failed\n");
	}
	return ret;
}

/**
 * @brief		Lock the DB and return session ID, repeat the attempt multiple times
 * @param[out]	session_id ID of locked session
 * @return		EOK if success, EAGAIN if lock is already locked
 */
errno_t pfe_if_db_polled_lock(uint32_t *session_id)
{
	errno_t ret = EOK;
	uint32_t i;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == session_id))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	} else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Try to lock IF_DB pfe_if_db_lock multiple times */
		for (i = 0; i <= PFE_CFG_IF_DB_POLLING_LOCK_TIME / IF_DB_LOCK_POLLING_SLEEP; i++)
		{
			ret = pfe_if_db_lock(session_id);
			if (ret == EOK)
			{
				/* Lock is acquired */
				break;
			}
			else
			{
				/* Wait for predefined time before next polling attempt */
				oal_time_msleep(IF_DB_LOCK_POLLING_SLEEP);
			}
		}
		if (ret != EOK)
		{
			NXP_LOG_WARNING("IF_DB polled lock was not successful.\n");
		}
	}
	return ret;
}

/**
 * @brief		Lock the DB with owner ID, repeat the attempt multiple times
 * @param[in]	owner_id ID of owner in range 0 - 15
 * @return		EOK if success, EAGAIN if lock is already locked, EINVAL if owner_id out of range
 */
errno_t pfe_if_db_polled_lock_owned(uint32_t owner_id)
{
	errno_t ret = EOK;
	uint32_t i;

	/* Try to lock IF_DB pfe_if_db_lock_owned multiple times */
	for (i = 0; i <= PFE_CFG_IF_DB_POLLING_LOCK_TIME / IF_DB_LOCK_POLLING_SLEEP; i++)
	{
		ret = pfe_if_db_lock_owned(owner_id);
		if (ret == EOK || ret == EINVAL)
		{	
			/* Lock is acquired, or invalid parameters */
			break;
		}
		else
		{
			/* Wait for predefined time before next polling attempt */
			oal_time_msleep(IF_DB_LOCK_POLLING_SLEEP);
		}
	}
	if (ret != EOK)
	{
		NXP_LOG_WARNING("IF_DB polled lock was not successful.\n");
	}

	return ret;
}

/**
 * @brief		Check if the IF_DB is locked by session_id
 * @param[in]	session_id ID of locked session
 * @return		EOK if success, EPERM if lock is not available 
 */
errno_t pfe_if_db_lock_check(uint32_t session_id)
{
	errno_t ret;

	if (EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_ERROR("IF_DB Mutex lock failed\n");
	}

	ret = pfe_if_db_check_precondition(&if_db_context, session_id);

	if (EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_ERROR("IF_DB Mutex unlock failed\n");
	}

	return ret;
}
