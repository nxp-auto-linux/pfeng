/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "linked_list.h"
#include "pfe_hif_ptp.h"

/*	Number of entries in the DB producing warning message */
#define PFE_HIF_PTP_DB_WARNING_THRESHOLD	50

/*	Maximum allowed number of entries */
#define PFE_HIF_PTP_DB_MAX_CAPACITY			(PFE_HIF_PTP_DB_WARNING_THRESHOLD + 10)

/*	Entry timeout in number of ticks */
#define PFE_HIF_PTP_DB_TIMEOUT				1

typedef struct
{
	uint32_t ticks;		/* Timeout counter (in number of ticks). Zero means entry is aged. */
	uint16_t refnum;	/* Reference to identify ETS report */
	uint8_t type;		/* PTP Message type */
	bool_t rx;			/* If TRUE then entry refers to ingress message */
	uint16_t port;		/* PTP Port */
	uint16_t seq_id;	/* PTP Sequence ID */
	uint32_t ts_sec;
	uint32_t ts_nsec;
	bool_t ts_valid;
	/*	List member for chaining */
	LLIST_t lm;
} pfe_hif_ptp_ts_db_entry_t;

/**
 * @brief		Worker function running within internal thread
 */
static void *pfe_hif_ptp_ts_db_tick(void *arg)
{
	pfe_hif_ptp_ts_db_t *db = (pfe_hif_ptp_ts_db_t *)arg;
	LLIST_t *item, *aux;
	pfe_hif_ptp_ts_db_entry_t *entry;

	while (1)
	{
		if (EOK != oal_mutex_lock(db->lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Release aged entries */
		LLIST_ForEachRemovable(item, aux, &db->entries)
		{
			entry = LLIST_Data(item, pfe_hif_ptp_ts_db_entry_t, lm);
			if ((NULL != entry) && (entry->ticks <= 0U))
			{
				NXP_LOG_INFO("Removing aged TS DB entry (Type: 0x%x, Port: 0x%x, SeqID: 0x%x)\n",
						entry->type, entry->port, entry->seq_id);
				LLIST_Remove(item);
				oal_mm_free(entry);
				db->count--;
			}
			else
			{
				entry->ticks--;
			}
		}

		if (EOK != oal_mutex_unlock(db->lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}

		/*	Tick = 1s */
		oal_time_mdelay(10000);
	}

	return NULL;
}

/**
 * @brief	Initialize TS database
 */
errno_t pfe_hif_ptp_ts_db_init(pfe_hif_ptp_ts_db_t *db)
{
	memset(db, 0, sizeof(pfe_hif_ptp_ts_db_t));
	db->lock = oal_mm_malloc(sizeof(oal_mutex_t));
	if (NULL == db->lock)
	{
		return ENOMEM;
	}
	else
	{
		oal_mutex_init(db->lock);
	}

	LLIST_Init(&db->entries);

	db->worker = oal_thread_create(&pfe_hif_ptp_ts_db_tick, db, "TS DB worker", 0);
	if (NULL == db->worker)
	{
		oal_mm_free(db->lock);
		return ENOMEM;
	}

	return EOK;
}

/**
 * @brief	Finalize the TS database
 */
void pfe_hif_ptp_ts_db_fini(pfe_hif_ptp_ts_db_t *db)
{
	LLIST_t *item, *aux;
	pfe_hif_ptp_ts_db_entry_t *entry;

	if (NULL != db->lock)
	{
		if (EOK != oal_mutex_lock(db->lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Release all entries */
		LLIST_ForEachRemovable(item, aux, &db->entries)
		{
			entry = LLIST_Data(item, pfe_hif_ptp_ts_db_entry_t, lm);
			if (NULL != entry)
			{
				LLIST_Remove(item);
				oal_mm_free(entry);
			}
		}

		if (EOK != oal_mutex_unlock(db->lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}

		oal_mutex_destroy(db->lock);
		oal_mm_free(db->lock);
		db->lock = NULL;
	}

	if (NULL != db->worker)
	{
		(void)oal_thread_cancel(db->worker);

		if (EOK != oal_thread_join(db->worker, NULL))
		{
			NXP_LOG_ERROR("Can't join TS DB worker thread\n");
		}
		else
		{
			NXP_LOG_INFO("TS DB worker stopped\n");
		}
	}
}

/**
 * @brief	Add PTP message to the DB. TS will be added later.
 */
errno_t pfe_hif_ptp_ts_db_push_msg(pfe_hif_ptp_ts_db_t *db, bool_t rx,
		uint16_t refnum, uint8_t type, uint16_t port, uint16_t seq_id)
{
	pfe_hif_ptp_ts_db_entry_t *entry;

	/*	We should somehow limit number of entries.. */
	if (db->count >= PFE_HIF_PTP_DB_MAX_CAPACITY)
	{
		return ENOSPC;
	}

	entry = oal_mm_malloc(sizeof(pfe_hif_ptp_ts_db_entry_t));
	if (NULL == entry)
	{
		return ENOMEM;
	}
	else
	{
		/*	Fill entry */
		entry->refnum = refnum;
		entry->type = type;
		entry->port = port;
		entry->seq_id = seq_id;
		entry->ts_valid = FALSE;
		entry->ticks = PFE_HIF_PTP_DB_TIMEOUT;
		entry->rx = rx;

		/*	Link-in */
		if (EOK != oal_mutex_lock(db->lock))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		LLIST_AddAtEnd(&entry->lm, &db->entries);
		db->count++;

		if ((db->count > PFE_HIF_PTP_DB_WARNING_THRESHOLD) && !db->reported)
		{
			NXP_LOG_WARNING("More than %d entries in PTP DB...\n", PFE_HIF_PTP_DB_WARNING_THRESHOLD);
			db->reported = TRUE;
		}

		if (EOK != oal_mutex_unlock(db->lock))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}
	}

	return EOK;
}

/**
 * @brief	Bind TS with existing entry
 */
errno_t pfe_hif_ptp_ts_db_push_ts(pfe_hif_ptp_ts_db_t *db,
		uint16_t refnum, uint32_t ts_sec, uint32_t ts_nsec)
{
	LLIST_t *item;
	pfe_hif_ptp_ts_db_entry_t *entry;
	bool_t found = FALSE;

	/*	Find matching entry and add the timestamp */
	if (EOK != oal_mutex_lock(db->lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	LLIST_ForEach(item, &db->entries)
	{
		entry = LLIST_Data(item, pfe_hif_ptp_ts_db_entry_t, lm);
		if (NULL != entry)
		{
			if (entry->refnum == refnum)
			{
				found = TRUE;
				entry->ts_sec = ts_sec;
				entry->ts_nsec = ts_nsec;
				entry->ts_valid = TRUE;
				break;
			}
		}
	}

	if (EOK != oal_mutex_unlock(db->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return (found) ? EOK : ENOENT;
}

/**
 * @brief	Get TS associated with give PTP message
 */
errno_t pfe_hif_ptp_ts_db_pop(pfe_hif_ptp_ts_db_t *db,
		uint8_t type, uint16_t port, uint16_t seq_id,
			uint32_t *ts_sec, uint32_t *ts_nsec, bool_t rx)
{
	LLIST_t *item;
	pfe_hif_ptp_ts_db_entry_t *entry;
	bool_t found = FALSE;

	/*	Find matching entry and get the timestamp */
	if (EOK != oal_mutex_lock(db->lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	LLIST_ForEach(item, &db->entries)
	{
		entry = LLIST_Data(item, pfe_hif_ptp_ts_db_entry_t, lm);
		if (NULL != entry)
		{
			if ((entry->rx == rx)
					&& (entry->type == type)
					&& (entry->port == port)
					&& (entry->seq_id == seq_id))
			{
				found = TRUE;
				*ts_sec = entry->ts_sec;
				*ts_nsec = entry->ts_nsec;
				break;
			}
		}
	}

	if (found)
	{
		/*	Remove from DB */
		LLIST_Remove(item);
		oal_mm_free(entry);
		db->count--;
		if ((db->count <= (PFE_HIF_PTP_DB_WARNING_THRESHOLD/4)) && db->reported)
		{
			db->reported = FALSE;
		}
	}

	if (EOK != oal_mutex_unlock(db->lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return (found) ? EOK : ENOENT;
}

