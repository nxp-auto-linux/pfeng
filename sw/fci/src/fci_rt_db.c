/* =========================================================================
 *  Copyright 2017-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_rt_db.c
 * @brief		Route database
 * @details		Route database is intended to store IP routes and provide
 * 				functions to select or remove particular entries.
 *
 * @warning		All API calls related to a single DB instance must be protected
 * 				from being preempted by another API calls related to the same
 * 				DB instance.
 *
 */

#include "pfe_cfg.h"
#include "oal.h"
#include "linked_list.h"
#include "fci_rt_db.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static bool_t fci_rt_db_match_criterion(fci_rt_db_t *db, const fci_rt_db_entry_t *entry);

/**
 * @brief		Match entry with latest criterion provided via fci_rt_db_get_first()
 * @param[in]	db The route DB instance
 * @param[in]	entry The entry to be matched
 * @retval		True Entry matches the criterion
 * @retval		False Entry does not match the criterion
 */
static bool_t fci_rt_db_match_criterion(fci_rt_db_t *db, const fci_rt_db_entry_t *entry)
{
	bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		match = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch (db->cur_crit)
		{
			case RT_DB_CRIT_ALL:
			{
				match = TRUE;
				break;
			}

			case RT_DB_CRIT_BY_IF:
			{
				match = (entry->iface == db->cur_crit_arg.iface);
				break;
			}

			case RT_DB_CRIT_BY_IF_NAME:
			{
				match = (0 == strcmp(db->cur_crit_arg.outif_name, pfe_phy_if_get_name(entry->iface)));
				break;
			}

			case RT_DB_CRIT_BY_IP:
			{
				match = (0 == memcmp(&db->cur_crit_arg.dst_ip, &entry->dst_ip, sizeof(pfe_ip_addr_t)));
				break;
			}

			case RT_DB_CRIT_BY_MAC:
			{
				match = (0 == memcmp(&db->cur_crit_arg.dst_mac, &entry->dst_mac, sizeof(pfe_mac_addr_t)));
				break;
			}

			case RT_DB_CRIT_BY_ID:
			{
				match = (db->cur_crit_arg.id == entry->id);
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
 * @brief		Initialize DB
 * @param[in]	db The route DB instance
 */
void fci_rt_db_init(fci_rt_db_t *db)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_Init(&db->theList);
		db->cur_item = db->theList.prNext;
	}
}

/**
 * @brief		Add a route to DB
 * @param[in]	db The route DB instance
 * @param[in]	src_mac Source MAC address
 * @param[in]	dst_mac Destination MAC address
 * @param[in]	iface Name of the output interface
 * @param[in]	id The route ID
 * @param[in]	refptr Reference pointer to be bound with entry
 * @param[in]	overwrite If true then if route exists, it is updated
 * @retval		EOK Success
 * @retval		ENOMEM Memory allocation failed
 * @retval		EPERM Attempt to insert already existing entry without 'overwrite' set to 'true'
 */
errno_t fci_rt_db_add(fci_rt_db_t *db,  pfe_ip_addr_t *dst_ip,
					pfe_mac_addr_t *src_mac, pfe_mac_addr_t *dst_mac,
					pfe_phy_if_t *iface, uint32_t id, void *refptr, bool_t overwrite)
{
	fci_rt_db_entry_t *new_entry;
	bool_t is_new = false;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == dst_ip) || (NULL == src_mac) || (NULL == dst_mac) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Check duplicates by route ID */
		new_entry = fci_rt_db_get_first(db, RT_DB_CRIT_BY_ID, (void *)&id);
		if (NULL == new_entry)
		{
			new_entry = oal_mm_malloc(sizeof(fci_rt_db_entry_t));
			is_new = true;
			if (NULL == new_entry)
			{
				NXP_LOG_ERROR("Unable to allocate memory\n");
				ret = ENOMEM;
			}
			else
			{
				(void)memset(new_entry, 0, sizeof(fci_rt_db_entry_t));
			}
		}
		else
		{
			is_new = false;

			if (false == overwrite)
			{
				ret = EPERM;
			}
		}

		if(EOK == ret)
		{
			/*	Store values */
			(void)memcpy(&new_entry->dst_ip, dst_ip, sizeof(pfe_ip_addr_t));
			(void)memcpy(&new_entry->src_mac, src_mac, sizeof(pfe_mac_addr_t));
			(void)memcpy(&new_entry->dst_mac, dst_mac, sizeof(pfe_mac_addr_t));
			new_entry->iface = iface;
			new_entry->id = id;
			new_entry->mtu = 0; /* Not supported yet */
			new_entry->refptr = refptr;

			/*	Put to DB */
			if (true == is_new)
			{
				LLIST_AddAtEnd(&(new_entry->list_member), &db->theList);
			}
		}
	}
	return ret;
}

/**
 * @brief		Remove entry from DB
 * @param[in]	db The route DB instance
 * @param[in]	entry Entry to be removed. If the call is successful the entry
 * 					  becomes invalid and shall not be accessed.
 * @return		EOK if success, error code otherwise
 */
errno_t fci_rt_db_remove(fci_rt_db_t *db, fci_rt_db_entry_t *entry)
{
	errno_t ret = EOK;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == db) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (&entry->list_member == db->cur_item)
		{
			/*	Remember the change so we can call remove() between get_first()
				and get_next() calls. */
			db->cur_item = db->cur_item->prNext;
		}

		LLIST_Remove(&(entry->list_member));
		oal_mm_free(entry);
	}

	return ret;
}

/**
 * @brief		Get first record from the DB matching given criterion
 * @details		Intended to be used with fci_rt_db_get_next
 * @param[in]	db The route DB instance
 * @param[in]	crit Get criterion
 * @param[in]	art Pointer to criterion argument
 * @return		The entry or NULL if not found
 * @warning		The returned entry must not be accessed after fci_rt_db_remove(entry)
 *				or fci_rt_db_drop_all() has been called.
 */
fci_rt_db_entry_t *fci_rt_db_get_first(fci_rt_db_t *db, fci_rt_db_get_criterion_t crit, const void *arg)
{
	fci_rt_db_entry_t *entry = NULL;
	LLIST_t *item;
	bool_t match = false;
	bool_t is_unknown_crit = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else if (unlikely((RT_DB_CRIT_ALL != crit) && (NULL == arg)))
	{
		/*	All criterions except RT_DB_CRIT_ALL require non-NULL argument */
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Remember criterion and argument for possible subsequent fci_rt_db_get_next() calls */
		db->cur_crit = crit;
		switch (db->cur_crit)
		{
			case RT_DB_CRIT_ALL:
			{
				break;
			}

			case RT_DB_CRIT_BY_IF:
			{
				db->cur_crit_arg.iface = (const pfe_phy_if_t *)arg;
				break;
			}

			case RT_DB_CRIT_BY_IF_NAME:
			{
				(void)memset(db->cur_crit_arg.outif_name, 0, sizeof(db->cur_crit_arg.outif_name));
				(void)strncpy(db->cur_crit_arg.outif_name, arg, sizeof(db->cur_crit_arg.outif_name)-1U);
				break;
			}

			case RT_DB_CRIT_BY_IP:
			{
				(void)memcpy(&db->cur_crit_arg.dst_ip, arg, sizeof(db->cur_crit_arg.dst_ip));
				break;
			}

			case RT_DB_CRIT_BY_MAC:
			{
				(void)memcpy(&db->cur_crit_arg.dst_mac, arg, sizeof(db->cur_crit_arg.dst_mac));
				break;
			}

			case RT_DB_CRIT_BY_ID:
			{
				(void)memcpy(&db->cur_crit_arg.id, arg, sizeof(db->cur_crit_arg.id));
				break;
			}

			default:
			{
				NXP_LOG_WARNING("Unknown criterion\n");
				is_unknown_crit = TRUE;
				break;
			}
		}

		if(TRUE != is_unknown_crit)
		{
			if (false == LLIST_IsEmpty(&db->theList))
			{
				/*	Get first matching entry */
				LLIST_ForEach(item, &db->theList)
				{
					/*	Get data */
					entry = LLIST_Data(item, fci_rt_db_entry_t, list_member);

					/*	Remember current item to know where to start later */
					db->cur_item = item->prNext;
					if (NULL != entry)
					{
						if (true == fci_rt_db_match_criterion(db, entry))
						{
							match = true;
							break;
						}
					}
				}
			}
		}
	}

	if (true != match)
	{
		entry = NULL;
	}
	return entry;
}

/**
 * @brief		Get next record from the DB
 * @details		Intended to be used with fci_rt_db_get_first.
 * @param[in]	db The route DB instance
 * @return		The entry or NULL if not found
 * @warning		The returned entry must not be accessed after fci_rt_db_remove(entry)
 *				or fci_rt_db_drop_all() has been called.
 */
fci_rt_db_entry_t *fci_rt_db_get_next(fci_rt_db_t *db)
{
	fci_rt_db_entry_t *entry;
	bool_t match = false;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		entry = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (false == LLIST_IsEmpty(&db->theList))
		{
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
					entry = LLIST_Data(db->cur_item, fci_rt_db_entry_t, list_member);

					/*	Remember current item to know where to start later */
					db->cur_item = db->cur_item->prNext;

					if (NULL != entry)
					{
						if (true == fci_rt_db_match_criterion(db, entry))
						{
							match = true;
							break;
						}
					}
				}
			}
		}
	}

	if (true != match)
	{
		entry = NULL;
	}
	return entry;
}

/**
 * @brief		Remove all entries
 * @param[in]	db The route DB instance
 * @return		EOK if success, error code otherwise
 */
errno_t fci_rt_db_drop_all(fci_rt_db_t *db)
{
	LLIST_t *item, *aux;
	fci_rt_db_entry_t *entry;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == db))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_ForEachRemovable(item, aux, &db->theList)
		{
			entry = LLIST_Data(item, fci_rt_db_entry_t, list_member);

			LLIST_Remove(item);

			oal_mm_free(entry);
		}
	}

	return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_PFE_MASTER */
/** @}*/
