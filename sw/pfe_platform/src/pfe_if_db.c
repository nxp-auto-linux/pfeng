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

/*
 *	if_db worker mbox codes
 */
#define IF_DB_WORKER_QUIT 				(2)
#define IF_DB_WORKER_START_TIMER 		(3)
#define IF_DB_WORKER_STOP_TIMER 		(4)
#define IF_DB_WORKER_TIMEOUT_DETECTED 	(5)

#define IF_DB_WORKER_TIMEOUT_MS			(5000U)

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
		char_t *name;
		pfe_ct_phy_if_id_t owner;
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

typedef struct __if_db_context
{
	uint32_t session_id;
	uint32_t seed;
	oal_mutex_t mutex;
	uint8_t ref_cnt;
	bool_t is_locked;
#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
	oal_thread_t *worker_thread;
	oal_mbox_t *mbox;
	errno_t worker_error;
#endif
} if_db_context_t;

/**
 * @brief	Global intefrace DB lock. Module-local singleton.
 */
static if_db_context_t if_db_context;


static bool_t pfe_if_db_match_criterion(pfe_if_db_t *db, pfe_if_db_entry_t *entry);
static errno_t pfe_if_db_check_precondition(if_db_context_t *if_db_context, uint32_t session_id);
#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
static void * pfe_if_db_worker(void *arg);
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */

#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
/**
 * @brief		Measure time until lock timeout
 * @param[in]	arg Instance of __if_db_context
 * @retval		NULL
 */
static void * pfe_if_db_worker(void *arg)
{
	oal_mbox_msg_t msg;
	if_db_context_t *context = (if_db_context_t*)arg;
	while(1)
	{
		if(EOK == oal_mbox_receive(context->mbox, &msg))
		{
			if(IF_DB_WORKER_QUIT == msg.payload.code)
			{
				/* End function */
				break;
			}

			if(EOK != oal_mutex_lock(&context->mutex))
			{
				NXP_LOG_DEBUG("DB mutex lock failed\n");
			}

			context->worker_error = EOK;
			switch(msg.payload.code)
			{
				case IF_DB_WORKER_START_TIMER:
				{
					/* Attach timer */
					if( EOK != oal_mbox_attach_timer(context->mbox, IF_DB_WORKER_TIMEOUT_MS, IF_DB_WORKER_TIMEOUT_DETECTED))
					{
						NXP_LOG_ERROR("Unable to attach timer\n");
					}
					break;
				}
				case IF_DB_WORKER_TIMEOUT_DETECTED:
				{

					/* Force unlock */
					context->session_id = (~context->session_id) << 4U;
					context->is_locked = FALSE;
					context->worker_error = ECANCELED;

					NXP_LOG_ERROR("Timeout was detected, if_bd lock unlocked automatically\n");

					/* Detach timer */
					if(EOK != oal_mbox_detach_timer(context->mbox))
					{
						NXP_LOG_DEBUG("Could not detach timer\n");
					}
					break;
				}
				case IF_DB_WORKER_STOP_TIMER:
				{
					/* Detach timer */
					if(EOK != oal_mbox_detach_timer(context->mbox))
					{
						NXP_LOG_DEBUG("Could not detach timer\n");
					}
					break;
				}
			}
			if(EOK != oal_mutex_unlock(&context->mutex))
			{
				NXP_LOG_DEBUG("DB mutex unlock failed\n");
			}
		}
	}
	return NULL;
}
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */

/**
 * @brief		Check preconditions before performing operation
 * @param[in]	context
 * @retval		EOK Preconditions are fulfilled
 * @retval		PERM Preconditions are not fulfilled
 * @warning		context should be locked before call
 */
static errno_t pfe_if_db_check_precondition(if_db_context_t *context, uint32_t session_id)
{
	errno_t ret = EOK;

	if(FALSE == context->is_locked)
	{
		ret = EPERM;
	}
	else if(session_id != context->session_id)
	{
		NXP_LOG_DEBUG("Incorrect session ID\n");
		ret = EPERM;
	}
	return ret;
}

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

		case IF_DB_CRIT_BY_NAME:
		{
			if (PFE_IF_DB_LOG == db->type)
			{
				match = (0 == strcmp(db->cur_crit_arg.name, pfe_log_if_get_name(entry->log_if)));
			}
			else
			{
				match = (0 == strcmp(db->cur_crit_arg.name, pfe_phy_if_get_name(entry->phy_if)));
			}

			break;
		}

		case IF_DB_CRIT_BY_OWNER:
		{
			match = (db->cur_crit_arg.owner == entry->owner);
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

	/* Create global DB lock */
	if(0U == if_db_context.ref_cnt)
	{
		/* Lock the data */
		if_db_context.is_locked = TRUE;

		if(EOK != oal_mutex_init(&if_db_context.mutex))
		{
			/* Handle errors*/
			oal_mm_free(db);

			NXP_LOG_ERROR("Mutex initialization failed\n");
			return NULL;
		}

		if(EOK != oal_mutex_lock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		/* Initialize data to safe values */
		if_db_context.is_locked = FALSE;
		if_db_context.session_id = 0U;

		/* Initialize seed to some value */
		if_db_context.seed = 123U;

#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
		/* Initialize worker data */
		if_db_context.worker_error = EOK;
		if_db_context.mbox = NULL;
		if_db_context.worker_thread = NULL;

		/* Create mbox*/
		if_db_context.mbox = oal_mbox_create();

		if(NULL != if_db_context.mbox)
		{
			/* Create worker thread */
			if_db_context.worker_thread = oal_thread_create(&pfe_if_db_worker, &if_db_context, "if_db worker", 0);

			if(NULL == if_db_context.worker_thread)
			{

				/* Handle errors*/
				oal_mm_free(db);

				/* Detach timer if exists and destroy mailbox */
				(void)oal_mbox_detach_timer(if_db_context.mbox);
				oal_mbox_destroy(if_db_context.mbox);

				/* Set internal variable to LOCK state*/
				if_db_context.is_locked = TRUE;

				/* Unlock mutex and destroy it*/
				if(EOK != oal_mutex_unlock(&if_db_context.mutex))
				{
					NXP_LOG_ERROR("Mutex unlock failed\n");
				}
				oal_mutex_destroy(&if_db_context.mutex);
				NXP_LOG_ERROR("Thread creation failed\n");
				return NULL;
			}
		}
		else
		{
			/* Handle errors*/
			oal_mm_free(db);

			/* Unlock mutex and destroy it*/
			if(EOK != oal_mutex_unlock(&if_db_context.mutex))
			{
				NXP_LOG_ERROR("Mutex unlock failed\n");
			}
			oal_mutex_destroy(&if_db_context.mutex);
			NXP_LOG_ERROR("Mail box creation failed\n");
			return NULL;
		}
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */

		if(EOK != oal_mutex_unlock(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}
	/* Increment reference counter */
	++if_db_context.ref_cnt;

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

	/* Decrement reference counter */
	if(0U < if_db_context.ref_cnt)
	{
		--if_db_context.ref_cnt;
	}

	/* Destroy global DB lock */
	if(0U == if_db_context.ref_cnt)
	{
		if(EOK != oal_mutex_lock(&if_db_context.mutex))
		{
			NXP_LOG_DEBUG("DB mutex lock failed\n");
		}

		if_db_context.is_locked = TRUE;

#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
		if(NULL != if_db_context.mbox)
		{
			NXP_LOG_INFO("Stopping if_db worker...\n");
			if (EOK != oal_mbox_send_signal(if_db_context.mbox, IF_DB_WORKER_QUIT))
			{
				NXP_LOG_DEBUG("oal_mbox_send_signal() failed\n");
			}
			else
			{
				if(if_db_context.worker_thread != NULL)
				{
					if(EOK != oal_thread_join(if_db_context.worker_thread, NULL))
					{
						NXP_LOG_DEBUG("oal_thread_join() failed\n");
					}
					else
					{
						NXP_LOG_INFO("if_db worker stopped\n");
						if_db_context.worker_thread = NULL;

						/* Destroy message box*/
						NXP_LOG_INFO("Destroyng if_db mbox\n");
						(void)oal_mbox_detach_timer(if_db_context.mbox);
						oal_mbox_destroy(if_db_context.mbox);
						if_db_context.mbox = NULL;
					}
				}
			}
		}
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */

		if(EOK != oal_mutex_unlock(&if_db_context.mutex))
		{
			NXP_LOG_DEBUG("DB mutex unlock failed\n");
		}

		if(EOK != oal_mutex_destroy(&if_db_context.mutex))
		{
			NXP_LOG_ERROR("Mutex destroy failed\n");
		}

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
	errno_t ret = EOK;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Check duplicates */
	ret = pfe_if_db_get_first(db, session_id, IF_DB_CRIT_BY_INSTANCE, iface, &new_entry);
	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex lock failed\n");
	}

	/* Check condition if operation on DB is allowed */
	if(EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
	{
		if(EOK != oal_mutex_unlock(&if_db_context.mutex))
		{
			NXP_LOG_DEBUG("DB mutex unlock failed\n");
		}
		return EPERM;
	}


	if ((NULL == new_entry) && (EOK == ret))
	{
		new_entry = oal_mm_malloc(sizeof(pfe_if_db_entry_t));
		if (NULL == new_entry)
		{
			ret = ENOMEM;
		}
		else
		{
			memset(new_entry, 0, sizeof(pfe_if_db_entry_t));
		}
	}
	else
	{
		/*	Don't allow duplicates */
		ret = EPERM;
	}

	/*	Store values */
	new_entry->iface = iface;
	new_entry->owner = owner;

	/*	Put to DB */
	LLIST_AddAtEnd(&(new_entry->list_member), &db->theList);

	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex unlock failed\n");
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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex lock failed\n");
	}

	/* Check condition if operation on DB is allowed */
	if(EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
	{
		if(EOK != oal_mutex_unlock(&if_db_context.mutex))
		{
			NXP_LOG_DEBUG("DB mutex unlock failed\n");
		}
		return EPERM;
	}

	if (&entry->list_member == db->cur_item)
	{
		/*	Remember the change so we can call remove() between get_first()
			and get_next() calls. */
		db->cur_item = db->cur_item->prNext;
	}

	LLIST_Remove(&(entry->list_member));
	oal_mm_free(entry);

	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex unlock failed\n");
	}
	return EOK;
}

/**
 * @brief		Get first record from the DB matching given criterion
 * @details		Intended to be used with pfe_if_db_get_next
 * @param[in]	db The interface DB instance
 * @param[in]	session_id ID of active session
 * @param[in]	crit Get criterion
 * @param[in]	art Pointer to criterion argument
 * @param[out]	entry The entry or NULL if not found
 * @return		EOK entry returned is valid
 * @return		EPERM db was locked by someone else, entry returned is not valid
 * @warning		The returned entry must not be accessed after pfe_if_db_remove(entry)
 *				or pfe_if_db_drop_all() has been called.
 */
errno_t pfe_if_db_get_first(pfe_if_db_t *db, uint32_t session_id, pfe_if_db_get_criterion_t crit, void *arg, pfe_if_db_entry_t **db_entry)
{
	LLIST_t *item;
	bool_t match = FALSE;
	pfe_if_db_entry_t *entry;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == db_entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex lock failed\n");
	}

	/* Check condition if operation on DB is allowed */
	if(EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
	{
		if(EOK != oal_mutex_unlock(&if_db_context.mutex))
		{
			NXP_LOG_DEBUG("DB mutex unlock failed\n");
		}
		return EPERM;
	}

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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
			if (unlikely(NULL == arg))
			{
				NXP_LOG_ERROR("NULL argument received\n");
				*db_entry = NULL;
				return EINVAL;
			}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

			db->cur_crit_arg.iface = arg;
			break;
		}

		case IF_DB_CRIT_BY_NAME:
		{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
			if (unlikely(NULL == arg))
			{
				NXP_LOG_ERROR("NULL argument received\n");
				*db_entry = NULL;
				return EINVAL;
			}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

			db->cur_crit_arg.name = (char_t *)arg;
			break;
		}

		case IF_DB_CRIT_BY_OWNER:
		{
			db->cur_crit_arg.owner = (pfe_ct_phy_if_id_t)((addr_t)arg & 0xff);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown criterion\n");
			entry = NULL;
			return EPERM;
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

	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex unlock failed\n");
	}

	*db_entry = entry;

	if (FALSE == match)
	{
		/* No match found */
		*db_entry = NULL;
	}
	return EOK;
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
	bool_t match = FALSE;
	pfe_if_db_entry_t *entry;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == db_entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex lock failed\n");
	}

	/* Check condition if operation on DB is allowed */
	if(EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
	{
		if(EOK != oal_mutex_unlock(&if_db_context.mutex))
		{
			NXP_LOG_DEBUG("DB mutex unlock failed\n");
		}
		return EPERM;
	}

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

	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex unlock failed\n");
	}

	*db_entry = entry;
	if (FALSE == match)
	{
		entry = NULL;
	}
	return EOK;
}

/**
 * @brief		Remove all entries
 * @param[in]	db The route DB instance
 * @param[in]	session_id ID of active session
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_log_if_db_drop_all(pfe_if_db_t *db, uint32_t session_id)
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

	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex lock failed\n");
	}

	/* Check condition if operation on DB is allowed */
	if(EOK != pfe_if_db_check_precondition(&if_db_context, session_id))
	{
		if(EOK != oal_mutex_unlock(&if_db_context.mutex))
		{
			NXP_LOG_DEBUG("DB mutex unlock failed\n");
		}
		return EPERM;
	}

	LLIST_ForEachRemovable(item, aux, &db->theList)
	{
		entry = LLIST_Data(item, pfe_if_db_entry_t, list_member);

		LLIST_Remove(item);

		oal_mm_free(entry);
	}

	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex unlock failed\n");
	}
	return EOK;
}

/**
 * @brief		Lock the DB with session ID
 * @param[out]	session_id ID of locked session
 * @return		EOK if success, error if lock is already locked
 */
errno_t pfe_if_db_lock(uint32_t *session_id)
{
	errno_t ret = ENOLCK;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == session_id))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/* Lock global if DB mutex */
	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex lock failed\n");
	}

	if(FALSE == if_db_context.is_locked)
	{
#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
		/* Send signal to start counting to timeout */
		if(EOK == oal_mbox_send_signal(if_db_context.mbox, IF_DB_WORKER_START_TIMER))
		{
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */
			/* Increment seed id */
			++if_db_context.seed;

			/* Store session ID and reserve 0 - 15 for named sessions */
			if_db_context.session_id = if_db_context.seed << 4U;

			/* Pass session id to caller*/
			*session_id = if_db_context.session_id;
			if_db_context.is_locked = TRUE;

			ret = EOK;
#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
		}
		else
		{
			NXP_LOG_ERROR("DB lock timeout wasn't initialized");
		}
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */
	}

	/* Unlock global if DB mutex */
	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Lock the DB with owner ID
 * @param[in]	owner_id ID of owner in range 0 - 15
 * @return		EOK if success, error if lock is already locked or id is not in range
 */
errno_t pfe_if_db_lock_owned(uint32_t owner_id)
{
	errno_t ret = ENOLCK;

	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex lock failed\n");
	}

	if((FALSE == if_db_context.is_locked) && (16U > owner_id))
	{
#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
		/* Send signal to start counting to timeout */
		if(EOK == oal_mbox_send_signal(if_db_context.mbox, IF_DB_WORKER_START_TIMER))
		{
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */
			/* Session ID is in ok range store it*/
			if_db_context.session_id = owner_id;
			if_db_context.is_locked = TRUE;
			ret = EOK;
#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
		}
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */
	}

	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Unlock the DB with owner ID/session ID
 * @param[in]	owner_id ID of owner or session
 * @return		EOK if success, error if lock is already locked or id is not in range
 */
errno_t pfe_if_db_unlock(uint32_t session_id)
{
	errno_t ret = ENOLCK;

	/* Lock global if DB mutex */
	if(EOK != oal_mutex_lock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex lock failed\n");
	}

	if((TRUE == if_db_context.is_locked) && (session_id == if_db_context.session_id))
	{
		/* Discard key and set locked to FALSE*/
		if_db_context.session_id = (~if_db_context.session_id) << 4U;

		/* Set is locked to FALSE */
		if_db_context.is_locked = FALSE;

#if defined(GLOBAL_CFG_IF_DB_WORKER_ENABLED)
		/* Stop timer */
		if(EOK != oal_mbox_send_signal(if_db_context.mbox, IF_DB_WORKER_STOP_TIMER))
		{
			NXP_LOG_DEBUG("Sending oal_mbox_send_signal lock will be unlocked after timeout\n");
		}
#endif /* GLOBAL_CFG_IF_DB_WORKER_ENABLED */

		ret = EOK;
	}

	/* Unlock global if DB mutex */
	if(EOK != oal_mutex_unlock(&if_db_context.mutex))
	{
		NXP_LOG_DEBUG("DB mutex unlock failed\n");
	}
	return ret;
}



/** @}*/
