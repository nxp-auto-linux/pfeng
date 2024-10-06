/* =========================================================================
 *
 *	Copyright (c) 2019 Imagination Technologies Limited
 *	Copyright (c) 2020-2021 Imagination Technologies Limited
 *	Copyright 2018-2024 NXP
 *
 *	SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"

#ifndef PFE_CFG_PFE_SLAVE
#include "hal.h"

#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_ct.h"
#include "pfe_phy_if.h"
#include "linked_list.h"
#include "pfe_feature_mgr.h"

typedef enum
{
	PFE_PHY_IF_INVALID,
	PFE_PHY_IF_EMAC,
	PFE_PHY_IF_HIF,
	PFE_PHY_IF_UTIL
} pfe_phy_if_type_t;

struct pfe_phy_if_tag
{
	pfe_phy_if_type_t type;
	pfe_ct_phy_if_id_t id;
	char_t *name;
	pfe_class_t *class;
	addr_t dmem_base;
	pfe_ct_phy_if_t phy_if_class;
	LLIST_t log_ifs;
	oal_mutex_t lock;
	bool_t is_enabled;
	pfe_ct_block_state_t block_state; /* Copy of value in phy_if_class for faster access */
	pfe_mac_db_t *mac_db; /* MAC database */
	union
	{
		pfe_emac_t *emac;
		pfe_hif_chnl_t *hif_ch;
		void *instance;
	} port;
};

typedef struct
{
	pfe_log_if_t *log_if;
	LLIST_t iterator;
} pfe_phy_if_list_entry_t;

/* usage scope: pfe_phy_if_flush_mac_addrs */
static const pfe_emac_crit_t crit_remap_table[4] = {EMAC_CRIT_BY_TYPE, EMAC_CRIT_BY_OWNER, EMAC_CRIT_BY_OWNER_AND_TYPE, EMAC_CRIT_ALL};

static errno_t pfe_phy_if_write_to_class_nostats(const pfe_phy_if_t *iface, const pfe_ct_phy_if_t *class_if);
static errno_t pfe_phy_if_write_to_class(const pfe_phy_if_t *iface, const pfe_ct_phy_if_t *class_if);
static bool_t pfe_phy_if_has_log_if_nolock(const pfe_phy_if_t *iface, const pfe_log_if_t *log_if);
static errno_t pfe_phy_if_disable_nolock(pfe_phy_if_t *iface);
static errno_t pfe_phy_if_set_flag_nolock(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag);
static errno_t pfe_phy_if_clear_flag_nolock(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag);
static pfe_ct_if_flags_t pfe_phy_if_get_flag_nolock(const pfe_phy_if_t *iface, pfe_ct_if_flags_t flag);
static errno_t pfe_phy_if_enable_hw_block(const pfe_phy_if_t *iface);
static void pfe_phy_if_update_op_mode_nolock(pfe_phy_if_t *iface, pfe_ct_if_op_mode_t mode);

static uint32_t pfe_phy_if_stat_to_str(const pfe_ct_phy_if_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level);

/**
 * @brief		Write interface structure to classifier memory skipping interface statistics
 * @param[in]	iface The interface instance
 * @param[in]	class_if Pointer to the structure to be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
static errno_t pfe_phy_if_write_to_class_nostats(const pfe_phy_if_t *iface, const pfe_ct_phy_if_t *class_if)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_if) || (NULL == iface) || (0U == iface->dmem_base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Be sure that phy_stats are at correct place */
		ct_assert_offsetof((sizeof(pfe_ct_phy_if_t) - sizeof(pfe_ct_phy_if_stats_t)) == offsetof(pfe_ct_phy_if_t, phy_stats));
		ret = pfe_class_write_dmem(iface->class, -1, iface->dmem_base, (const  void *)class_if,
								sizeof(pfe_ct_phy_if_t) - sizeof(pfe_ct_phy_if_stats_t));
	}

	return ret;
}

/**
 * @brief		Write interface structure to classifier memory with statistics
 * @param[in]	iface The interface instance
 * @param[in]	class_if Pointer to the structure to be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
static errno_t pfe_phy_if_write_to_class(const pfe_phy_if_t *iface, const pfe_ct_phy_if_t *class_if)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_if) || (NULL == iface) || (0U == iface->dmem_base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_class_write_dmem(iface->class, -1, iface->dmem_base, (const  void *)class_if, sizeof(pfe_ct_phy_if_t));
	}
	return ret;
}

/**
 * @brief		Converts statistics of a physical interface or classification algorithm into a text form
 * @param[in]	stat		Statistics to convert
 * @param[out]	buf			Buffer where to write the text
 * @param[in]	buf_len		Buffer length
 * @param[in]	verb_level	Verbosity level
 * @return		Number of bytes written into the output buffer
 */
static uint32_t pfe_phy_if_stat_to_str(const pfe_ct_phy_if_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

	(void)verb_level;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == stat) || (NULL == buf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		len = 0U;
	}
	else
#endif
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "Ingress frames:	  %u\n", oal_ntohl(stat->ingress));
		len += oal_util_snprintf(buf + len, buf_len - len, "Egress frames:	  %u\n", oal_ntohl(stat->egress));
		len += oal_util_snprintf(buf + len, buf_len - len, "Malformed frames: %u\n", oal_ntohl(stat->malformed));
		len += oal_util_snprintf(buf + len, buf_len - len, "Discarded frames: %u\n", oal_ntohl(stat->discarded));
	}

	return len;
}

/**
 * @brief		Create new physical interface instance
 * @param[in]	class The classifier instance
 * @param[in]	id The PFE firmware is using HW interface identifiers to distinguish
 *				between particular interfaces. The set of available IDs (the
 *				pfe_ct_phy_if_id_t) shall remain compatible with the firmware.
 * @param[in]	name Name of the interface
 * @return		The interface instance or NULL if failed
 */
pfe_phy_if_t *pfe_phy_if_create(pfe_class_t *class, pfe_ct_phy_if_id_t id, const char_t *name)
{
	uint32_t i;
	pfe_phy_if_t *iface;
	pfe_ct_class_mmap_t pfe_pe_mmap = {0U};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	iface = oal_mm_malloc(sizeof(pfe_phy_if_t));

	if (NULL == iface)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		return NULL;
	}
	else
	{
		(void)memset(iface, 0, sizeof(pfe_phy_if_t));
		if (EOK != oal_mutex_init(&iface->lock))
		{
			NXP_LOG_ERROR("Could not initialize mutex\n");
			oal_mm_free(iface);
			iface = NULL;
			return NULL;
		}

		iface->type = PFE_PHY_IF_INVALID;
		iface->id = id;
		iface->class = class;
		iface->is_enabled = FALSE;
		LLIST_Init(&iface->log_ifs);

		iface->mac_db = pfe_mac_db_create();
		if (NULL == iface->mac_db)
		{
			NXP_LOG_ERROR("Could not create MAC db\n");
			pfe_phy_if_destroy(iface);
			return NULL;
		}

		if (EOK != pfe_class_get_mmap(class, 0, &pfe_pe_mmap))
		{
			NXP_LOG_ERROR("Could not get memory map\n");
			pfe_phy_if_destroy(iface);
			return NULL;
		}

		if (oal_ntohl(pfe_pe_mmap.dmem_phy_if_size) < ((1UL + (uint8_t)id) * sizeof(pfe_ct_phy_if_t)))
		{
			NXP_LOG_ERROR("PhyIf storage is too small\n");
			pfe_phy_if_destroy(iface);
			return NULL;
		}

		/*	Get physical interface instance address within DMEM array */
		iface->dmem_base = oal_ntohl(pfe_pe_mmap.dmem_phy_if_base) + ((uint16_t)id * sizeof(pfe_ct_phy_if_t));

		if (NULL == name)
		{
			iface->name = NULL;
		}
		else
		{
			iface->name = oal_mm_malloc(strlen(name) + 1U);
			if (NULL == iface->name)
			{
				NXP_LOG_ERROR("Unable to allocate memory\n");
				pfe_phy_if_destroy(iface);
				return NULL;
			}
			(void)strcpy(iface->name, name);
		}

		/*	Initialize the interface structure in classifier */
		iface->phy_if_class.id = id;
		iface->phy_if_class.block_state = IF_BS_FORWARDING;
		iface->phy_if_class.mgmt_interface = PFE_PHY_IF_ID_INVALID;
		for(i = 0U; i < PFE_CT_MIRRORS_COUNT; i++)
		{
			iface->phy_if_class.rx_mirrors[i] = 0;
			iface->phy_if_class.tx_mirrors[i] = 0;
		}
		iface->phy_if_class.flags = (pfe_ct_if_flags_t)oal_htonl((uint32_t)IF_FL_ALLOW_Q_IN_Q|(uint32_t)IF_FL_FF_ALL_TCP);

		/* Be sure that statistics are zeroed (endianness doesn't mater for this) */
		iface->phy_if_class.phy_stats.ingress	= 0;
		iface->phy_if_class.phy_stats.egress	= 0;
		iface->phy_if_class.phy_stats.discarded	= 0;
		iface->phy_if_class.phy_stats.malformed	= 0;

		/*	Write the configuration to classifier */
		if (EOK != pfe_phy_if_write_to_class(iface, &iface->phy_if_class))
		{
			NXP_LOG_ERROR("Phy IF configuration failed\n");
			oal_mm_free(iface);
			iface = NULL;
		}
	}

	return iface;
}

/**
 * @brief		Destroy interface instance
 * @param[in]	iface The interface instance
 */
void pfe_phy_if_destroy(pfe_phy_if_t *iface)
{
	errno_t ret = EOK;
	uint8_t i = 0U;
	if (NULL != iface)
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		if (FALSE == LLIST_IsEmpty(&iface->log_ifs))
		{
			/*	Do not allow orphaned logical interfaces */
			NXP_LOG_ERROR("%s still contains logical interfaces. Destroy them first.\n", iface->name);

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}
		}
		else
		{
			if (iface->mac_db != NULL)
			{
				ret = pfe_mac_db_destroy(iface->mac_db);
				if (ret != EOK)
				{
					NXP_LOG_ERROR("unable to destroy MAC database: %d\n", ret);
				}
			}
			else
			{
				NXP_LOG_DEBUG("%s mac_db is NULL.\n", iface->name);
			}

			/* Release attached mirror objects */
			for (i = 0U; i < PFE_CT_MIRRORS_COUNT; ++i)
			{
				pfe_phy_if_set_rx_mirror(iface, i, NULL);
				pfe_phy_if_set_tx_mirror(iface, i, NULL);
			}

			/* Disable HW bridge lookup if the last interface was destroyed */
			pfe_phy_if_update_op_mode_nolock(iface, IF_OP_DEFAULT);

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			if (NULL != iface->name)
			{
				oal_mm_free(iface->name);
				iface->name = NULL;
			}

			if (EOK != oal_mutex_destroy(&iface->lock))
			{
				NXP_LOG_ERROR("Could not destroy mutex\n");
			}

			oal_mm_free(iface);
		}
	}

	return;
}

/**
 * @brief		Return classifier instance associated with interface
 * @param[in]	iface The interface instance
 * @return		The classifier instance
 */
__attribute__((pure)) pfe_class_t *pfe_phy_if_get_class(const pfe_phy_if_t *iface)
{
	pfe_class_t *class;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		class = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		class = iface->class;
	}
	return class;
}

/**
 * @brief		Add logical interface
 * @details		First added logical interface will become the default one. Default is used
 *				when packet is not matching any other logical interface within the physical one.
 * @param[in]	iface The physical interface instance
 * @param[in]	log_if The logical interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 * @retval		EEXIST Entry exists
 * @note		API to be used only by pfe_log_if module
 */
errno_t pfe_phy_if_add_log_if(pfe_phy_if_t *iface, pfe_log_if_t *log_if)
{
	pfe_phy_if_list_entry_t *entry;
	const pfe_phy_if_list_entry_t *tmp_entry;
	addr_t log_if_dmem_base = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry = oal_mm_malloc(sizeof(pfe_phy_if_list_entry_t));
	if (NULL == entry)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		return ENOMEM;
	}

	entry->log_if = log_if;

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_ERROR("mutex lock failed\n");
	}

	if (TRUE == LLIST_IsEmpty(&iface->log_ifs))
	{
		/*
			No logical interface assigned yet
		*/

		/*	Get DMEM address to the logical interface structure */
		if (EOK != pfe_log_if_get_dmem_base(log_if, &log_if_dmem_base))
		{
			NXP_LOG_ERROR("Could not get DMEM base (%s, parent: %s)\n",
					pfe_log_if_get_name(log_if), iface->name);

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			return ENOEXEC;
		}

#if defined(PFE_CFG_NULL_ARG_CHECK)
		if (0U == log_if_dmem_base)
		{
			NXP_LOG_ERROR("LogIf base is NULL (%s)\n", pfe_log_if_get_name(log_if));
			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			return ENOEXEC;
		}
#endif /* PFE_CFG_NULL_ARG_CHECK */

		/*	First added interface will become the default one */
		iface->phy_if_class.def_log_if = oal_htonl((uint32_t)log_if_dmem_base);
	}
	else
	{
		/*
			Chain new logIf in (at the begin) => modify first entry .next pointer
		*/

		/*	Check duplicates */
		if (TRUE == pfe_phy_if_has_log_if_nolock(iface, log_if))
		{
			NXP_LOG_WARNING("%s already added\n", pfe_log_if_get_name(log_if));
			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			oal_mm_free(entry);
			entry = NULL;

			return EEXIST;
		}

		/*	Get current first item of the list */
		tmp_entry = LLIST_Data(iface->log_ifs.prNext, pfe_phy_if_list_entry_t, iterator);

		log_if_dmem_base = 0U;
		if (EOK != pfe_log_if_get_dmem_base(tmp_entry->log_if, &log_if_dmem_base))
		{
			NXP_LOG_ERROR("Could not get DMEM base (%s, parent: %s)\n",
					pfe_log_if_get_name(tmp_entry->log_if), iface->name);
			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			return ENOEXEC;
		}

#if defined(PFE_CFG_NULL_ARG_CHECK)
		if (0U == log_if_dmem_base)
		{
			NXP_LOG_ERROR("LogIf base is NULL (%s)\n", pfe_log_if_get_name(tmp_entry->log_if));
			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			return ENOEXEC;
		}
#endif /* PFE_CFG_NULL_ARG_CHECK */

		/*	Change 'next' pointer of the new entry */
		if (EOK != pfe_log_if_set_next_dmem_ptr(log_if, log_if_dmem_base))
		{
			NXP_LOG_ERROR("Can't set next linked list pointer (%s, parent: %s)\n",
					pfe_log_if_get_name(log_if), iface->name);

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			return ENOEXEC;
		}
	}
	/*	Get DMEM pointer to the new logIf */
	log_if_dmem_base = 0U;
	if (EOK != pfe_log_if_get_dmem_base(log_if, &log_if_dmem_base))
	{
		NXP_LOG_ERROR("Could not get logIf DMEM base (%s, parent: %s)\n",
				pfe_log_if_get_name(log_if), iface->name);
		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}

		return ENOEXEC;
	}

	/*	Set list head to the new logIf */
	iface->phy_if_class.log_ifs = oal_htonl(PFE_CFG_CLASS_ELF_DMEM_BASE | (log_if_dmem_base & (PFE_CFG_CLASS_DMEM_SIZE - 1U)));

	/*	Store physical interface changes (.phy_if_class) to DMEM */
	if (EOK != pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class))
	{
		NXP_LOG_ERROR("Unable to update structure in DMEM (%s)\n", iface->name);
		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}

		return ENOEXEC;
	}
	else
	{
		/*	Now the new logIf is head of the list and classifier will see that */
		NXP_LOG_DEBUG("%s (p0x%p) added to %s (p0x%p)\n",
				pfe_log_if_get_name(log_if), (void *)log_if_dmem_base,
					iface->name, (void *)iface->dmem_base);
	}

	/*	Add instance to local list of logical interfaces */
	LLIST_AddAtBegin(&entry->iterator, &iface->log_ifs);

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}

	return EOK;
}

static bool_t pfe_phy_if_has_log_if_nolock(const pfe_phy_if_t *iface, const pfe_log_if_t *log_if)
{
	LLIST_t *curItem;
	const pfe_phy_if_list_entry_t *entry;
	bool_t ret = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_ForEach(curItem, &iface->log_ifs)
		{
			entry = LLIST_Data(curItem, pfe_phy_if_list_entry_t, iterator);
			if (log_if == entry->log_if)
			{
				ret = TRUE;
				break;
			}
		}
	}

	return ret;
}

/**
 * @brief		Check if physical interface contains given logical interface
 * @param[in]	iface The physical interface instance
 * @param[in]	log_if The logical interface instance
 * @return		TRUE if logical interface is bound to the physical one. False
 *				otherwise.
 */
bool_t pfe_phy_if_has_log_if(pfe_phy_if_t *iface, const pfe_log_if_t *log_if)
{
	bool_t match;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		match = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		match = pfe_phy_if_has_log_if_nolock(iface, log_if);

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return match;
}

/**
 * @brief		Delete associated logical interface
 * @param[in]	iface The physical interface instance
 * @param[in]	log_if The logical interface instance to be deleted
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 * @retval		ENOENT Entry not found
 * @note		API to be used only by pfe_log_if module
 */
errno_t pfe_phy_if_del_log_if(pfe_phy_if_t *iface, const pfe_log_if_t *log_if)
{
	pfe_phy_if_list_entry_t *entry = NULL;
	const pfe_phy_if_list_entry_t *prev_entry = NULL;
	LLIST_t *curItem;
	bool_t found = FALSE;
	addr_t log_if_dmem_base = 0U, next_dmem_ptr = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_ERROR("mutex lock failed\n");
	}

	LLIST_ForEach(curItem, &iface->log_ifs)
	{
		entry = LLIST_Data(curItem, pfe_phy_if_list_entry_t, iterator);
		if (log_if == entry->log_if)
		{
			found = TRUE;
			break;
		}
		else
		{
			prev_entry = entry;
		}
	}

	if (FALSE == found)
	{
		NXP_LOG_WARNING("%s not found in %s\n", pfe_log_if_get_name(log_if), iface->name);
		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}

		return ENOENT;
	}

	/*	Bypass the entry within the linked list in DMEM */
	next_dmem_ptr = 0U;
	if (EOK != pfe_log_if_get_next_dmem_ptr(entry->log_if, &next_dmem_ptr))
	{
		NXP_LOG_ERROR("Could not get DMEM base (%s, parent: %s)\n",
				pfe_log_if_get_name(entry->log_if), iface->name);

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}

		return ENOEXEC;
	}

	if (NULL == prev_entry)
	{
		if (0U == next_dmem_ptr)
		{
			/*	No next entry, no previous entry. Just remove. */
			NXP_LOG_WARNING("Removing default logical interface (%s, parent: %s)\n",
					pfe_log_if_get_name(entry->log_if), iface->name);

			/*	Invalidate head and default interface */
			iface->phy_if_class.def_log_if = oal_htonl((uint32_t)0U);
			iface->phy_if_class.log_ifs = oal_htonl((uint32_t)0U);
		}
		else
		{
			/*	Next pointer is OK, just move the head. Default interface is the latest one so no change here. */
			iface->phy_if_class.log_ifs = oal_htonl((uint32_t)next_dmem_ptr);
		}
	}
	else
	{
		/*	Set 'next' pointer of previous entry to 'next' pointer of deleted entry */
		if (EOK != pfe_log_if_set_next_dmem_ptr(prev_entry->log_if, next_dmem_ptr))
		{
			NXP_LOG_ERROR("Can't set next linked list pointer (%s, parent: %s)\n",
					pfe_log_if_get_name(prev_entry->log_if), iface->name);

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			return ENOEXEC;
		}

		/*	If 'next' pointer of deleted entry is NULL then we're removing default interface */
		if (0U == next_dmem_ptr)
		{
			NXP_LOG_INFO("Removing default logical interface (%s, parent: %s). Will be replaced by %s.\n",
					pfe_log_if_get_name(log_if), iface->name, pfe_log_if_get_name(prev_entry->log_if));

			log_if_dmem_base = 0U;
			if (EOK != pfe_log_if_get_dmem_base(prev_entry->log_if, &log_if_dmem_base))
			{
				NXP_LOG_ERROR("Could not get DMEM base (%s, parent: %s)\n",
						pfe_log_if_get_name(prev_entry->log_if), iface->name);

				/*	Don't leave here as the previous entry is set up to bypass the deleted entry */
			}

			iface->phy_if_class.def_log_if = oal_htonl((uint32_t)log_if_dmem_base);
		}
	}

	/*	Store physical interface changes (.phy_if_class) to DMEM */
	if (EOK != pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class))
	{
		NXP_LOG_ERROR("Unable to update structure in DMEM (%s)\n", iface->name);
		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}

		return ENOEXEC;
	}
	else
	{
		log_if_dmem_base = 0U;
		if (EOK != pfe_log_if_get_dmem_base(log_if, &log_if_dmem_base))
		{
			NXP_LOG_ERROR("Could not get DMEM base (%s, parent: %s)\n",
					pfe_log_if_get_name(log_if), iface->name);
		}

		NXP_LOG_INFO("%s (p0x%p) removed from %s (p0x%p)\n",
				pfe_log_if_get_name(log_if), (void *)log_if_dmem_base,
					iface->name, (void *)iface->dmem_base);
	}

	/*	Remove entry from local list */
	LLIST_Remove(&entry->iterator);

	/*	Release the entry */
	oal_mm_free(entry);
	entry = NULL;

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief Set the block state
 * @param[in] iface The interface instance
 * @param[out] block_state Block state to set
 * @return EOK on success or an error code
 */
errno_t pfe_phy_if_set_block_state(pfe_phy_if_t *iface, pfe_ct_block_state_t block_state)
{
	errno_t ret;
	pfe_ct_block_state_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}
		/* Set the requested state */
		tmp = iface->block_state;
		iface->block_state = block_state;
		iface->phy_if_class.block_state = block_state;
		/* Write changes into the HW */
		ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);

		if (EOK != ret)
		{	/* Failure to update the HW */
			/* Restore previous value */
			iface->block_state = tmp;
			iface->phy_if_class.block_state = tmp;
			/* Report an error */
			NXP_LOG_ERROR("Can't write PHY IF structure to classifier\n");
			ret = EINVAL;
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief Get the block state
 * @param[in] iface The interface instance
 * @param[out] block_state Current block state
 * @return EOK On success or an error code
 */
errno_t pfe_phy_if_get_block_state(pfe_phy_if_t *iface, pfe_ct_block_state_t *block_state)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/* The value is being stored in the iface structure and kept up-to-date
		with the value in FW thus it can be simply returned */
		*block_state = iface->block_state;

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Get operational mode
 * @param[in]	iface The interface instance
 * @retval		Current phy_if mode. See pfe_ct_if_op_mode_t.
 */
pfe_ct_if_op_mode_t pfe_phy_if_get_op_mode(pfe_phy_if_t *iface)
{
	pfe_ct_if_op_mode_t ret;

	/*	Update the interface structure */
	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_ERROR("mutex lock failed\n");
	}

	ret = iface->phy_if_class.mode;

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}

	return ret;
}

pfe_log_if_t *pfe_phy_if_get_default_log_if(pfe_phy_if_t *iface)
{
	pfe_log_if_t *log_if = NULL;
	const pfe_phy_if_list_entry_t *tmp_entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		if (TRUE != LLIST_IsEmpty(&iface->log_ifs))
		{
			tmp_entry = LLIST_Data(iface->log_ifs.prNext, pfe_phy_if_list_entry_t, iterator);
			log_if = tmp_entry->log_if;
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return log_if;
}

/**
 * @brief		Set operational mode
 * @param[in]	iface The interface instance
 * @param[in]	mode Mode to be set. See pfe_ct_if_op_mode_t.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_set_op_mode(pfe_phy_if_t *iface, pfe_ct_if_op_mode_t mode)
{
	pfe_ct_class_mmap_t mmap;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Get memory map */
		ret = pfe_class_get_mmap(iface->class, 0, &mmap);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't get memory map\n");
			ret = EINVAL;
		}
		else
		{
			/*	Update the interface structure */
			if (EOK != oal_mutex_lock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex lock failed\n");
			}

			/* Enable HW bridge lookup if required */
			pfe_phy_if_update_op_mode_nolock(iface, mode);

			iface->phy_if_class.mode = mode;
			ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Can't write PHY IF structure to classifier\n");
				ret = EINVAL;
			}

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief		Maintain bridge operational mode
 * @param[in]	iface The interface instance
 * @param[in]	mode Mode to be set. See pfe_ct_if_op_mode_t.
 * @note		Control HW bridge lookup mode based on operational mode of all PHYs.
 */
static void pfe_phy_if_update_op_mode_nolock(pfe_phy_if_t *iface, pfe_ct_if_op_mode_t mode)
{
	uint32_t if_bitmap;
	bool_t br_mode = FALSE;

	if ((PFE_PHY_IF_ID_EMAC0 <= iface->id) && (PFE_PHY_IF_ID_MAX >= iface->id))
	{
		/* Set bitmap based on PHY ID */
		if_bitmap = (1U << iface->id);
		if ((IF_OP_VLAN_BRIDGE == mode) || (IF_OP_L2L3_VLAN_BRIDGE == mode))
		{
			br_mode = TRUE;
		}

		pfe_class_update_hw_bridge_lookup(iface->class, if_bitmap, br_mode);
	}
}

/**
 * @brief		Bind interface with EMAC
 * @param[in]	iface The interface instance
 * @param[in]	emac The EMAC instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		EPERM Operation not permitted
 */
errno_t pfe_phy_if_bind_emac(pfe_phy_if_t *iface, pfe_emac_t *emac)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		if (PFE_PHY_IF_INVALID == iface->type)
		{
			iface->type = PFE_PHY_IF_EMAC;
			iface->port.emac = emac;

			if (TRUE == iface->is_enabled)
			{
				if (EOK != oal_mutex_unlock(&iface->lock))
				{
					NXP_LOG_ERROR("mutex unlock failed\n");
				}

				ret = pfe_phy_if_enable(iface);
			}
			else
			{
				if (EOK != oal_mutex_unlock(&iface->lock))
				{
					NXP_LOG_ERROR("mutex unlock failed\n");
				}

				ret = pfe_phy_if_disable(iface);
			}
		}
		else
		{
			NXP_LOG_WARNING("Interface already bound\n");

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}

			ret = EPERM;
		}
	}

	return ret;
}

/**
 * @brief		Get associated EMAC instance
 * @param[in]	iface The interface instance
 * @return		Associated EMAC instance or NULL if failed
 */
pfe_emac_t *pfe_phy_if_get_emac(const pfe_phy_if_t *iface)
{
	pfe_emac_t *ptr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ptr = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (PFE_PHY_IF_EMAC == iface->type)
		{
			ptr = iface->port.emac;
		}
		else
		{
			NXP_LOG_ERROR("Invalid interface type\n");
			ptr = NULL;
		}
	}

	return ptr;
}

/**
 * @brief		Bind interface with HIF channel
 * @param[in]	iface The interface instance
 * @param[in]	hif The HIF channel instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		EPERM Operation not permitted
 */
errno_t pfe_phy_if_bind_hif(pfe_phy_if_t *iface, pfe_hif_chnl_t *hif)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == hif) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		if (PFE_PHY_IF_INVALID == iface->type)
		{
			iface->type = PFE_PHY_IF_HIF;
			iface->port.hif_ch = hif;
			ret = EOK;
		}
		else
		{
			NXP_LOG_WARNING("Interface already bound\n");
			ret = EPERM;
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Get associated HIF channel instance
 * @param[in]	iface The interface instance
 * @return		Associated HIF channel instance or NULL if failed
 */
pfe_hif_chnl_t *pfe_phy_if_get_hif(const pfe_phy_if_t *iface)
{
	pfe_hif_chnl_t *ptr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ptr = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (PFE_PHY_IF_HIF == iface->type)
		{
			ptr = iface->port.hif_ch;
		}
		else
		{
			NXP_LOG_ERROR("Invalid interface type\n");
			ptr = NULL;
		}
	}

	return ptr;
}

/**
 * @brief		Initialize util physical interface
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		EPERM Operation not permitted
 */
errno_t pfe_phy_if_bind_util(pfe_phy_if_t *iface)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		if (PFE_PHY_IF_INVALID == iface->type)
		{
			iface->type = PFE_PHY_IF_UTIL;
			/* Configure instance to NULL */
			/* With NULL nothing will be done on en/dis promisc en/dis etc.. */
			iface->port.instance = NULL;
		}
		else
		{
			NXP_LOG_WARNING("Interface already bound\n");
			ret = EPERM;
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Check if interface is enabled
 * @param[in]	iface The interface instance
 * @retval		TRUE if enabled
 * @retval		FALSE if disabled
 */
bool_t pfe_phy_if_is_enabled(pfe_phy_if_t *iface)
{
	bool_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		ret = iface->is_enabled;

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Check if hw_block is enabled
 * @param[in]	iface The interface instance
 * @retval		TRUE if enabled
 * @retval		FALSE if disabled
 */
static errno_t pfe_phy_if_enable_hw_block(const pfe_phy_if_t *iface)
{
	errno_t ret = EOK;

	/*	Enable also associated HW block */
	if (NULL == iface->port.instance)
	{
		/*	No HW block associated */
		;
	}
	else
	{
		if (PFE_PHY_IF_EMAC == iface->type)
		{
			pfe_emac_enable(iface->port.emac);
		}
		else if (PFE_PHY_IF_HIF == iface->type)
		{
			ret = pfe_hif_chnl_rx_enable(iface->port.hif_ch);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Can't enable HIF channel RX: %d\n", ret);
			}
			else
			{
				ret = pfe_hif_chnl_tx_enable(iface->port.hif_ch);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't enable HIF channel TX: %d\n", ret);
				}
			}
		}
		else
		{
			NXP_LOG_ERROR("Invalid interface type\n");
			ret = EINVAL;
		}
	}

	return ret;
}

/**
 * @brief		Enable interface (RX/TX)
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_enable(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		NXP_LOG_DEBUG("Enabling %s\n", iface->name);

		/* Enable HW bridge lookup if required */
		pfe_phy_if_update_op_mode_nolock(iface, iface->phy_if_class.mode);

		/*	Enable interface instance. Backup flags and write the changes. */
		tmp = iface->phy_if_class.flags;
		iface->phy_if_class.flags |= oal_htonl(IF_FL_ENABLED);
		ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
		if (EOK != ret)
		{
			/*	Failed. Revert flags. */
			NXP_LOG_ERROR("Phy IF configuration failed\n");
			iface->phy_if_class.flags = tmp;
		}
		else
		{
			/*	Mark the interface as enabled */
			iface->is_enabled = TRUE;

			ret = pfe_phy_if_enable_hw_block(iface);

			if (EOK != ret)
			{
				/*	HW configuration failure. Backup flags and disable the instance. */
				tmp = iface->phy_if_class.flags;
				iface->phy_if_class.flags &= (pfe_ct_if_flags_t)oal_htonl(~(uint32_t)IF_FL_ENABLED);
				ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
				if (EOK != ret)
				{
					/*	Failed. Revert flags. */
					NXP_LOG_ERROR("Phy IF configuration failed\n");
					iface->phy_if_class.flags = tmp;
				}
				else
				{
					iface->is_enabled = FALSE;
				}
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

static errno_t pfe_phy_if_disable_nolock(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*
		Behaviour changed:
		Don't depend on associated LOG_IF(s) for PHY_IF disabling.
		Ie. disable PHY_IF _EVEN_ if any LOG_IF is enabled.
		*/

		NXP_LOG_DEBUG("Disabling %s\n", iface->name);

		/*	Disable interface instance. Backup flags and write the changes. */
		tmp = iface->phy_if_class.flags;
		iface->phy_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp & oal_htonl(~(uint32_t)IF_FL_ENABLED));
		ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
		if (EOK != ret)
		{
			/*	Failed. Revert flags. */
			NXP_LOG_ERROR("Phy IF configuration failed\n");
			iface->phy_if_class.flags = tmp;
		}
		else
		{
			/*	Mark the interface as disabled */
			iface->is_enabled = FALSE;

			/*	Disable also associated HW block */
			if (NULL == iface->port.instance)
			{
				/*	No HW block associated */
				;
			}
			else
			{
				if (PFE_PHY_IF_EMAC == iface->type)
				{
					pfe_emac_disable(iface->port.emac);
				}
				else if (PFE_PHY_IF_HIF == iface->type)
				{
					pfe_hif_chnl_rx_disable(iface->port.hif_ch);
					pfe_hif_chnl_tx_disable(iface->port.hif_ch);
				}
				else
				{
					NXP_LOG_ERROR("Invalid interface type\n");
					ret = EINVAL;
				}
			}

			/* Disable HW bridge lookup if the last interface is disabled */
			pfe_phy_if_update_op_mode_nolock(iface, IF_OP_DEFAULT);
		}
	}

	return ret;
}

/**
 * @brief		Disable interface (RX/TX)
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_disable(pfe_phy_if_t *iface)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		ret = pfe_phy_if_disable_nolock(iface);

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}


/**
 * @brief		Set management interface
 * @param[in]	iface The interface instance
 * @param[in]	mgmt_interface The management interface
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_set_mgmt_interface(pfe_phy_if_t *iface, pfe_ct_phy_if_id_t mgmt_interface)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		iface->phy_if_class.mgmt_interface = mgmt_interface;
		/*	Write the configuration to classifier */
		if (EOK != pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class))
		{
			NXP_LOG_ERROR("Phy IF configuration mgmt interface failed\n");
			ret = EINVAL;
		}
		else
		{
			ret = EOK;
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Get management interface
 * @param[in]	iface The interface instance
 * @retval		Phy interface ID used as PTP management interface.
 */
pfe_ct_phy_if_id_t pfe_phy_if_get_mgmt_interface(pfe_phy_if_t *iface)
{
	pfe_ct_phy_if_id_t mgmt_interface = PFE_PHY_IF_ID_INVALID;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Update the interface structure */
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		mgmt_interface = iface->phy_if_class.mgmt_interface;

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return mgmt_interface;
}

/**
 * @brief		Set physical interface flag (nolock variant)
 * @param[in]	iface The interface instance
 * @param[in]	flag The flag to set
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_phy_if_set_flag_nolock(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	For selected flags: check that the underlying FW feature is available (enabled) in FW */
		const char* feat_name = ((IF_FL_VLAN_CONF_CHECK == flag) ? ("vlan_conf_check") :
								((IF_FL_PTP_CONF_CHECK == flag) ? ("ptp_conf_check") : (NULL)));

		if ((NULL != feat_name) && (FALSE == pfe_feature_mgr_is_available(feat_name)))
		{
			NXP_LOG_WARNING("Feature '%s' is not available (not enabled in FW).\n", feat_name);
			ret = EPERM;
		}
		else
		{
			/* Set the flag. */
			tmp = iface->phy_if_class.flags;
			iface->phy_if_class.flags |= oal_htonl(flag);
			ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
			if (EOK != ret)
			{
				/*	Failed. Revert flags. */
				NXP_LOG_ERROR("Could not write interface flag (set)\n");
				iface->phy_if_class.flags = tmp;
			}
		}
	}

	return ret;
}

/**
 * @brief		Clear physical interface flag (nolock variant)
 * @param[in]	iface The interface instance
 * @param[in]	flag The flag to clear
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_phy_if_clear_flag_nolock(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	For selected flags: check that the underlying FW feature is available (enabled) in FW */
		const char* feat_name = ((IF_FL_VLAN_CONF_CHECK == flag) ? ("vlan_conf_check") :
								((IF_FL_PTP_CONF_CHECK == flag) ? ("ptp_conf_check") : (NULL)));
		if ((NULL != feat_name) && (FALSE == pfe_feature_mgr_is_available(feat_name)))
		{
			NXP_LOG_WARNING("Feature '%s' is not available (not enabled in FW).\n", feat_name);
			ret = EPERM;
		}
		else
		{
			/* Set the flag. */
			tmp = iface->phy_if_class.flags;
			iface->phy_if_class.flags &= oal_htonl(~(uint32_t)flag);
			ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
			if (EOK != ret)
			{
				/*	Failed. Revert flags. */
				NXP_LOG_ERROR("Could not write interface flag (clear)\n");
				iface->phy_if_class.flags = tmp;
			}
		}
	}

	return ret;
}

/**
 * @brief		Get physical interface flag (nolock variant)
 * @param[in]	iface The interface instance
 * @param[in]	flag The flag to check
 * @return		Flag if 'flag' is set, zero (IF_FL_NONE) otherwise
 */
static pfe_ct_if_flags_t pfe_phy_if_get_flag_nolock(const pfe_phy_if_t *iface, pfe_ct_if_flags_t flag)
{
	pfe_ct_if_flags_t ret_flag;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret_flag = IF_FL_NONE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret_flag = (pfe_ct_if_flags_t)(oal_ntohl(iface->phy_if_class.flags) & flag);
	}
	return ret_flag;
}

/**
 * @brief		Set physical interface flag
 * @param[in]	iface The interface instance
 * @param[in]	flag The flag to set
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_phy_if_set_flag(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		ret = pfe_phy_if_set_flag_nolock(iface, flag);

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Clear physical interface flag
 * @param[in]	iface The interface instance
 * @param[in]	flag The flag to clear
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_phy_if_clear_flag(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		ret = pfe_phy_if_clear_flag_nolock(iface, flag);

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Get physical interface flag
 * @param[in]	iface The interface instance
 * @param[in]	flag The flag to check
 * @return		Flag if 'flag' is set, zero (IF_FL_NONE) otherwise
 */
pfe_ct_if_flags_t pfe_phy_if_get_flag(pfe_phy_if_t *iface, pfe_ct_if_flags_t flag)
{
	pfe_ct_if_flags_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = IF_FL_NONE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		ret = pfe_phy_if_get_flag_nolock(iface, flag);

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Check if phy_if in promiscuous mode
 * @param[in]	iface The interface instance
 * @retval		TRUE promiscuous mode is enabled
 * @retval		FALSE  promiscuous mode is disbaled
 */
bool_t pfe_phy_if_is_promisc(pfe_phy_if_t *iface)
{
	bool_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		ret = (0U != (oal_ntohl(iface->phy_if_class.flags) & (uint32_t)IF_FL_PROMISC));

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief				Enable loopback mode
 * @param[in]	iface The interface instance
 * @retval				EOK Success
 * @retval				EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_loopback_enable(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*		Enable instance loopback mode. Backup flags and write the changes. */
		tmp = iface->phy_if_class.flags;
		iface->phy_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp | oal_htonl(IF_FL_LOOPBACK));
		ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
		if (EOK != ret)
		{
			/*		Failed. Revert flags. */
			NXP_LOG_ERROR("Phy IF configuration failed\n");
			iface->phy_if_class.flags = tmp;
		}
		else
		{
			/*		Set up also associated HW block */
			if (NULL == iface->port.instance)
			{
				/*		No HW block associated */
				;
			}
			else
			{
				if (PFE_PHY_IF_EMAC == iface->type)
				{
					pfe_emac_enable_loopback(iface->port.emac);
				}
				else if (PFE_PHY_IF_HIF == iface->type)
				{
					/*		HIF/UTIL does not offer filtering ability */
					;
				}
				else
				{
					NXP_LOG_ERROR("Invalid interface type\n");
					ret = EINVAL;
				}
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief				Disable loopback mode
 * @param[in]	iface The interface instance
 * @retval				EOK Success
 * @retval				EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_loopback_disable(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*		Disable instance loopback mode. Backup flags and write the changes. */
		tmp = iface->phy_if_class.flags;
		iface->phy_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp & oal_htonl(~(uint32_t)IF_FL_LOOPBACK));
		ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
		if (EOK != ret)
		{
			/*		Failed. Revert flags. */
			NXP_LOG_ERROR("Phy IF configuration failed\n");
			iface->phy_if_class.flags = tmp;
		}
		else
		{
			/*		Set up also associated HW block */
			if (NULL == iface->port.instance)
			{
				/*		No HW block associated */
				;
			}
			else
			{
				if (PFE_PHY_IF_EMAC == iface->type)
				{
					pfe_emac_disable_loopback(iface->port.emac);
				}
				else if (PFE_PHY_IF_HIF == iface->type)
				{
					/*		HIF does not offer filtering ability */
					;
				}
				else
				{
					NXP_LOG_ERROR("Invalid interface type\n");
					ret = EINVAL;
				}
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Enable promiscuous mode
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_promisc_enable(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Enable instance promiscuous mode. Backup flags and write the changes. */
		tmp = iface->phy_if_class.flags;
		iface->phy_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp | oal_htonl(IF_FL_PROMISC));
		ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
		if (EOK != ret)
		{
			/*	Failed. Revert flags. */
			NXP_LOG_ERROR("Phy IF configuration failed\n");
			iface->phy_if_class.flags = tmp;
		}
		else
		{
			/*	Set up also associated HW block */
			if (NULL == iface->port.instance)
			{
				/*	No HW block associated */
				;
			}
			else
			{
				if (PFE_PHY_IF_EMAC == iface->type)
				{
					pfe_emac_enable_promisc_mode(iface->port.emac);
				}
				else if (PFE_PHY_IF_HIF == iface->type)
				{
					/*	HIF/UTIL does not offer filtering ability */
					;
				}
				else
				{
					NXP_LOG_ERROR("Invalid interface type\n");
					ret = EINVAL;
				}
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}


	return ret;
}

/**
 * @brief		Disable promiscuous mode
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_promisc_disable(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Disable instance promiscuous mode. Backup flags and write the changes. */
		tmp = iface->phy_if_class.flags;
		iface->phy_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp & oal_htonl(~(uint32_t)IF_FL_PROMISC));
		ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
		if (EOK != ret)
		{
			/*	Failed. Revert flags. */
			NXP_LOG_ERROR("Phy IF configuration failed\n");
			iface->phy_if_class.flags = tmp;
		}
		else
		{
			/*	Set up also associated HW block */
			if (NULL == iface->port.instance)
			{
				/*	No HW block associated */
				;
			}
			else
			{
				if (PFE_PHY_IF_EMAC == iface->type)
				{
					pfe_emac_disable_promisc_mode(iface->port.emac);
				}
				else if (PFE_PHY_IF_HIF == iface->type)
				{
					/*	HIF does not offer filtering ability */
					;
				}
				else
				{
					NXP_LOG_ERROR("Invalid interface type\n");
					ret = EINVAL;
				}
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Enable loadbalance mode
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_loadbalance_enable(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (PFE_PHY_IF_HIF != iface->type)
		{
			/* Only HIF offers loadbalancing */
			NXP_LOG_ERROR("Invalid interface type\n");
			ret = EINVAL;
		}
		else
		{
			if (EOK != oal_mutex_lock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex lock failed\n");
			}

			/*	Enable instance load balance mode. Backup flags and write the changes. */
			tmp = iface->phy_if_class.flags;
			iface->phy_if_class.flags |= oal_htonl(IF_FL_LOAD_BALANCE);
			ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
			if (EOK != ret)
			{
				/*	Failed. Revert flags. */
				NXP_LOG_ERROR("Phy IF configuration for IF_FL_LOAD_BALANCE failed\n");
				iface->phy_if_class.flags = tmp;
			}

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief		Disable loadbalance mode
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_loadbalance_disable(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (PFE_PHY_IF_HIF != iface->type)
		{
			/* Only HIF offers loadbalancing */
			NXP_LOG_ERROR("Invalid interface type\n");
			ret = EINVAL;
		}
		else
		{
			if (EOK != oal_mutex_lock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex lock failed\n");
			}

			/*	Disable instance loadbalance mode. Backup flags and write the changes. */
			tmp = iface->phy_if_class.flags;
			iface->phy_if_class.flags &= (pfe_ct_if_flags_t)oal_htonl(~(uint32_t)IF_FL_LOAD_BALANCE);
			ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
			if (EOK != ret)
			{
				/*	Failed. Revert flags. */
				NXP_LOG_ERROR("Phy IF configuration for IF_FL_LOAD_BALANCE failed\n");
				iface->phy_if_class.flags = tmp;
			}

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief		Enable ALLMULTI mode
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_allmulti_enable(pfe_phy_if_t * iface)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{

		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Set up also associated HW block */
		if (NULL == iface->port.instance)
		{
			/*	No HW block associated */
			;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				pfe_emac_enable_allmulti_mode(iface->port.emac);
			}
			else if (PFE_PHY_IF_HIF == iface->type)
			{
				/*	HIF/UTIL does not offer filtering ability */
				;
			}
			else
			{
				NXP_LOG_ERROR("Invalid interface type\n");
				ret = EINVAL;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}
	return ret;
}

/**
 * @brief		Disable ALLMULTI mode
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_allmulti_disable(pfe_phy_if_t *iface)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Set up also associated HW block */
		if (NULL == iface->port.instance)
		{
			/*	No HW block associated */
			;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				pfe_emac_disable_allmulti_mode(iface->port.emac);
			}
			else if (PFE_PHY_IF_HIF == iface->type)
			{
				/*	HIF does not offer filtering ability */
				;
			}
			else
			{
				NXP_LOG_ERROR("Invalid interface type\n");
				ret = EINVAL;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}
	return ret;
}

/**
 * @brief	Get rx/tx flow control config
 * @param[in]	iface The interface instance
 * @param[out]	tx_ena tx flow control status
 * @param[out]	rx_ena rx flow control status
 * @return		EOK on success
 */
errno_t pfe_phy_if_get_flow_control(pfe_phy_if_t *iface, bool_t* tx_ena, bool_t* rx_ena)
{
	errno_t ret = EOK;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{

		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}
		if (NULL == iface->port.instance)
		{
			/*		No HW block associated */
			;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				pfe_emac_get_flow_control(iface->port.emac, tx_ena, rx_ena);
			}
			else
			{
				;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}
	return ret;
}

/**
 * @brief		Set tx flow control
 * @param[in]	iface The interface instance
 * @param[in]	tx_ena TRUE: enable flow control, FALSE: disable flow control
 * @return		EOK on success
 */
errno_t pfe_phy_if_set_tx_flow_control(pfe_phy_if_t *iface, bool_t tx_ena)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{

		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}
		if (NULL == iface->port.instance)
		{
			/*		No HW block associated */
			;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				if (TRUE == tx_ena)
				{
					pfe_emac_enable_tx_flow_control(iface->port.emac);
				}
				else
				{
					pfe_emac_disable_tx_flow_control(iface->port.emac);
				}
			}
			else
			{
				;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Set rx flow control
 * @param[in]	iface The interface instance
 * @param[in]	rx_ena TRUE: enable flow control, FALSE: disable flow control
 * @return		EOK on success
 */
errno_t pfe_phy_if_set_rx_flow_control(pfe_phy_if_t *iface, bool_t rx_ena)
{
	errno_t ret = EOK;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}
		if (NULL == iface->port.instance)
		{
			/*		No HW block associated */
			;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				if (TRUE == rx_ena)
				{
					pfe_emac_enable_rx_flow_control(iface->port.emac);
				}
				else
				{
					pfe_emac_disable_rx_flow_control(iface->port.emac);
				}
			}
			else
			{
				;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Add MAC address
 * @param[in]	iface The interface instance
 * @param[in]	addr The MAC address to add
 * @param[in]	owner The identification of driver instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_phy_if_add_mac_addr(pfe_phy_if_t *iface, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Configure also associated HW block */
		if (NULL == iface->port.instance)
		{
			/*	No HW block associated */
			;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				ret = pfe_mac_db_add_addr(iface->mac_db, addr, owner);
				if(EOK == ret)
				{
					ret = pfe_emac_add_addr(iface->port.emac, addr, owner);
					if (EOK != ret)
					{
						/*	If MAC address already exists, not needed to add */
						if (EEXIST == ret)
						{
							NXP_LOG_DEBUG("MAC Address already exists\n");
							ret = EOK;
						}
						else
						{
							NXP_LOG_ERROR("Unable to add MAC address: %d\n", ret);
							/* Delete the MAC address from database */
							ret = pfe_mac_db_del_addr(iface->mac_db, addr, owner);
							if (EOK != ret) {
								NXP_LOG_ERROR("Unable to delete MAC address: %d\n", ret);
							}
							ret = ENOEXEC;
						}
					}
				}
			}
			else if (PFE_PHY_IF_HIF == iface->type)
			{
				/*	HIF does not offer MAC filtering ability */
				ret = EINVAL;
			}
			else
			{
				NXP_LOG_ERROR("Invalid interface type\n");
				ret = EINVAL;
			}

			if (EOK == ret)
			{
				NXP_LOG_DEBUG("Address %02x:%02x:%02x:%02x:%02x:%02x added to %s\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], iface->name);
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Delete MAC address
 * @param[in]	iface The interface instance
 * @param[in]	addr The MAC address to delete
 * @param[in]	owner The identification of driver instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Address not found
 */
errno_t pfe_phy_if_del_mac_addr(pfe_phy_if_t *iface, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Configure also associated HW block */
		if (NULL == iface->port.instance)
		{
			/*	No HW block associated */
			;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				ret = pfe_mac_db_del_addr(iface->mac_db, addr, owner);
				if(EOK != ret)
				{
					NXP_LOG_WARNING("Unable to remove MAC address from phy_if MAC database: %d\n", ret);
				}
				else
				{
					ret = pfe_emac_del_addr(iface->port.emac, addr, owner);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Unable to del MAC address: %d\n", ret);

						/* Removal of MAC address from emac failed, put it back to DB */
						ret = pfe_mac_db_add_addr(iface->mac_db, addr, owner);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("Unable to put back the MAC address into phy_if MAC database: %d\n", ret);
						}
						ret = ENOENT;
					}
				}
			}
			else if (PFE_PHY_IF_HIF == iface->type)
			{
				/*	HIF does not offer MAC filtering ability */
				ret = EINVAL;
			}
			else
			{
				NXP_LOG_ERROR("Invalid interface type\n");
				ret = EINVAL;
			}

			if (EOK == ret)
			{
				NXP_LOG_INFO("Address %02x:%02x:%02x:%02x:%02x:%02x removed from %s\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], iface->name);
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Get handle of internal MAC database
 * @param[in]	iface The interface instance
 * @retval		Database handle.
 */
pfe_mac_db_t *pfe_phy_if_get_mac_db(const pfe_phy_if_t *iface)
{
	pfe_mac_db_t *ptr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ptr = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ptr = iface->mac_db;
	}

	return ptr;
}

/**
 * @brief		Reinit MAC address query and get the first MAC address from mac addr db.
 * @param[in]	iface The interface instance.
 * @param[out]	addr The MAC address will be written here.
 * @param[in]	crit All, Owner, Type or Owner&Type criterion
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @param[in]	owner The identification of driver instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT No address found
 */
errno_t pfe_phy_if_get_mac_addr_first(pfe_phy_if_t *iface, pfe_mac_addr_t addr, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Get MAC address from associated HW block */
		if (NULL == iface->port.instance)
		{
			/*	No HW block associated */
			ret = ENOENT;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				ret = pfe_mac_db_get_first_addr(iface->mac_db, crit, type, owner, addr);
				if(EOK != ret)
				{
					NXP_LOG_WARNING("%s: Unable to get MAC address: %d\n", iface->name, ret);
				}
			}
			else if (PFE_PHY_IF_HIF == iface->type)
			{
				/*	HIF does not have MAC address storage (yet) */
				ret = ENOENT;
			}
			else
			{
				/*	Unknown type, nothing to verify */
				ret = EINVAL;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Get the next MAC address from mac addr db.
 * @param[in]	iface The interface instance.
 * @param[out]	addr The MAC address will be written here.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT No address found
 *
 * @note		Call pfe_phy_if_get_mac_addr_first() to initiate a query session.
 *				Then repeatedly call this function till there are no more MAC addresses to get.
 */
errno_t pfe_phy_if_get_mac_addr_next(pfe_phy_if_t *iface, pfe_mac_addr_t addr)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Get MAC address from associated HW block */
		if (NULL == iface->port.instance)
		{
			/*	No HW block associated */
			ret = ENOENT;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				ret = pfe_mac_db_get_next_addr(iface->mac_db, addr);
				if(EOK != ret)
				{
					NXP_LOG_WARNING("%s: Unable to get MAC address: %d\n", iface->name, ret);
				}
			}
			else if (PFE_PHY_IF_HIF == iface->type)
			{
				/*	HIF does not have MAC address storage (yet) */
				ret = ENOENT;
			}
			else
			{
				/*	Unknown type, nothing to verify */
				ret = EINVAL;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Delete MAC addresses added by owner with defined type
 * @param[in]	iface The interface instance
 * @param[in]	crit All, Owner, Type or Owner&Type criterion
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @param[in]	owner The identification of driver instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_phy_if_flush_mac_addrs(pfe_phy_if_t *iface, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Configure also associated HW block */
		if (NULL == iface->port.instance)
		{
			/*	No HW block associated */
			ret = EOK;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				/* TODO: revisit flush modes on EMAC layer, following remap table is only temporary solution */
				ct_assert(EMAC_CRIT_INVALID == sizeof(crit_remap_table));

				ret = pfe_emac_flush_mac_addrs(iface->port.emac, crit_remap_table[crit], type, owner);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Unable to flush multicast MAC addresses (owner ID %d): %d\n", owner, ret);
					ret = ENOEXEC;
				}
				else
				{
					ret = pfe_mac_db_flush(iface->mac_db, crit, type, owner);
					if(EOK != ret)
					{
						NXP_LOG_ERROR("Unable to flush MAC address from phy_if MAC database: %d\n", ret);
					}
				}
			}
			else if (PFE_PHY_IF_HIF == iface->type)
			{
				/*	HIF does not offer MAC filtering ability */
				ret = EOK;
			}
			else
			{
				NXP_LOG_ERROR("Invalid interface type\n");
				ret = EINVAL;
			}

			if (EOK == ret)
			{
				NXP_LOG_DEBUG("All multicast addresses owned by driver instance ID %d were flushed from %s\n", owner, iface->name);
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief Sets the SPD (security policy database for IPsec) of the physical interface
 * @param[in] iface Inteface which SPD shall be set
 * @param[in] spd_addr Address of the SPD in the DMEM to be set (value 0 disables the IPsec feature for given interface)
 * @return EOK or an error value in case of failure
 */
errno_t pfe_phy_if_set_spd(pfe_phy_if_t *iface, uint32_t spd_addr)
{
	errno_t ret;
	/* Update configuration */
	iface->phy_if_class.ipsec_spd = oal_htonl(spd_addr);
	/* Propagate the change into the classifier */
	ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
	return ret;
}

/**
 * @brief Returns the SPD address used by the physical interface
 * @param[in] iface Physical interface which shall be queried
 * @return Address of the SPD being used by the given physical interface. Value 0 means that no
 * *	   SPD is in use thus the IPsec feature is disabled for the given interface.
 */
uint32_t pfe_phy_if_get_spd(const pfe_phy_if_t *iface)
{
	return oal_ntohl(iface->phy_if_class.ipsec_spd);
}

/**
 * @brief		Set Flexible Filter rule table
 * @param[in]	iface The interface instance
 * @param[in]	table The table address. Zero means to disable the filter.
 * @retval		EOK Success
 * @retval		ENOENT Table not found
 * @retval		EINVAL Invalid argument
 *
 * @note		TODO: Temporary API only. Pass table instance or table
 *					  name but not the DMEM address :(
 */
errno_t pfe_phy_if_set_ftable(pfe_phy_if_t *iface, uint32_t table)
{
	errno_t ret;
	addr_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (0U != table)
		{
			NXP_LOG_INFO("%s: Enabling Flexible Filter\n", iface->name);
		}
		else
		{
			NXP_LOG_INFO("%s: Disabling Flexible Filter\n", iface->name);
		}

		/*	Update the interface structure */
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		tmp = iface->phy_if_class.filter;
		iface->phy_if_class.filter = oal_htonl(table);
		ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
		if (EOK != ret)
		{
			/*	Revert */
			NXP_LOG_ERROR("Can't write PHY IF structure to classifier\n");
			iface->phy_if_class.filter = tmp;
			ret = EINVAL;
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Get Flexible Filter rule table
 * @param[in]	iface The interface instance
 * @return		Table address or zero if there is no table
 *
 * @note		TODO: Temporary API only. Pass table instance or table
 *					  name but not the DMEM address :(
 */
uint32_t pfe_phy_if_get_ftable(pfe_phy_if_t *iface)
{
	uint32_t addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		addr = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Update the interface structure */
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		addr = oal_ntohl(iface->phy_if_class.filter);

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return addr;
}

/**
 * @brief		Get phy interface statistics
 * @param[in]	iface The interface instance
 * @param[out]	stat Statistic structure
 * @retval		EOK Success
 * @retval		NOMEM Not possible to allocate memory for read
 */
errno_t pfe_phy_if_get_stats(pfe_phy_if_t *iface, pfe_ct_phy_if_stats_t *stat)
{
	uint32_t i = 0U;
	errno_t ret;
	addr_t offset = 0;
	uint32_t buffer_len = 0;
	pfe_ct_phy_if_stats_t * stats = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == stat)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)memset(stat,0,sizeof(pfe_ct_phy_if_stats_t));

		/* Store offset to stats */
		offset = offsetof(pfe_ct_phy_if_t,phy_stats);

		/* Prepare memory */
		buffer_len = sizeof(pfe_ct_phy_if_stats_t) * pfe_class_get_num_of_pes(iface->class);
		stats = oal_mm_malloc(buffer_len);
		if(NULL == stats)
		{
			NXP_LOG_ERROR("Unable to allocate memory\n");
			ret = ENOMEM;
		}
		else
		{
			/* Gather memory from all PEs*/
			ret = pfe_class_gather_read_dmem(iface->class, stats, (iface->dmem_base + offset), buffer_len, sizeof(pfe_ct_phy_if_stats_t));

			/* Calculate total statistics */
			while(i < pfe_class_get_num_of_pes(iface->class))
			{
				/* Store statistics */
				stat->discarded	+= oal_ntohl(stats[i].discarded);
				stat->egress	+= oal_ntohl(stats[i].egress);
				stat->ingress	+= oal_ntohl(stats[i].ingress);
				stat->malformed	+= oal_ntohl(stats[i].malformed);
				++i;
			}
			oal_mm_free(stats);

			/* Convert statistics back to network endian */
			stat->discarded	= oal_htonl(stat->discarded);
			stat->egress	= oal_htonl(stat->egress);
			stat->ingress	= oal_htonl(stat->ingress);
			stat->malformed	= oal_htonl(stat->malformed);
		}
	}

	return ret;
}

/**
 * @brief Configures the selected RX mirror of the given interface
 * @param[in] iface Interface to be configured
 * @param[in] sel Selector of the RX mirror (0 to PFE_CT_MIRRORS_COUNT - 1)
 * @param[in] mirror Mirror to be configured.
 *					 Value NULL disables the selected RX mirror
 *	@return EOK when success or error code otherwise
 */
errno_t pfe_phy_if_set_rx_mirror(pfe_phy_if_t *iface, uint32_t sel, const pfe_mirror_t *mirror)
{
	errno_t ret;
	uint32_t tmp;
	uint32_t address = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if(sel >= PFE_CT_MIRRORS_COUNT)
		{
			ret = EINVAL;
		}
		else
		{
			if(NULL != mirror)
			{
				address = pfe_mirror_get_address(mirror);
			}
			/* Update configuration */
			tmp = iface->phy_if_class.rx_mirrors[sel]; /* Backup */
			iface->phy_if_class.rx_mirrors[sel] = oal_htonl(address);
			/* Propagate the change into the classifier */
			ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
			if(EOK != ret)
			{	/* Restore */
				iface->phy_if_class.rx_mirrors[sel] = tmp;
			}
			else
			{
				/* Increment internal ref counter of the new mirror object.
				 * The complementary decrement is realized when this mirror becomes "old mirror object" (when it is getting removed from the interface). */
				(void)pfe_mirror_get_first(MIRROR_BY_PHYS_ADDR, (void *)(addr_t)address);

				/* Decrement internal ref counter of the old mirror object. */
				pfe_mirror_put_by_address((addr_t)oal_ntohl(tmp));
			}
		}
	}

	return ret;
}

/**
 * @brief Configures the selected TX mirror of the given interface
 * @param[in] iface Interface to be configured
 * @param[in] sel Selector of the TX mirror (0 to PFE_CT_MIRRORS_COUNT - 1)
 * @param[in] mirror Mirror to be configured.
 *					 Value NULL disables the selected RX mirror
 *	@return EOK when success or error code otherwise
 */
errno_t pfe_phy_if_set_tx_mirror(pfe_phy_if_t *iface, uint32_t sel, const pfe_mirror_t *mirror)
{
	errno_t ret;
	uint32_t tmp;
	uint32_t address = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if(sel >= PFE_CT_MIRRORS_COUNT)
		{
			ret = EINVAL;
		}
		else
		{
			if(NULL != mirror)
			{
				address = pfe_mirror_get_address(mirror);
			}

			/* Update configuration */
			tmp = iface->phy_if_class.tx_mirrors[sel]; /* Backup */
			iface->phy_if_class.tx_mirrors[sel] = oal_htonl(address);
			/* Propagate the change into the classifier */
			ret = pfe_phy_if_write_to_class_nostats(iface, &iface->phy_if_class);
			if(EOK != ret)
			{	/* Restore */
				iface->phy_if_class.tx_mirrors[sel] = tmp;
			}
			else
			{
				/* Increment internal ref counter of the new mirror object.
				 * The complementary decrement is realized when this mirror becomes "old mirror object" (when it is getting removed from the interface). */
				(void)pfe_mirror_get_first(MIRROR_BY_PHYS_ADDR, (void *)(addr_t)address);

				/* Decrement internal ref counter of the old mirror object. */
				pfe_mirror_put_by_address((addr_t)oal_ntohl(tmp));
			}
		}
	}

	return ret;
}

/**
 * @brief Returns the selected TX mirror of the given interface
 * @param[in] iface Interface to be queried
 * @param[in] sel Selector of the TX mirror (0 to PFE_CT_MIRRORS_COUNT - 1)
 * @return The mirror reference or NULL if no mirror is configured
 */
pfe_mirror_t *pfe_phy_if_get_tx_mirror(const pfe_phy_if_t *iface, uint32_t sel)
{
	uint32_t address;
	pfe_mirror_t *ptr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ptr = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if(sel >= PFE_CT_MIRRORS_COUNT)
		{
			ptr = NULL;
		}
		else
		{
			address = oal_ntohl(iface->phy_if_class.tx_mirrors[sel]);
			ptr = pfe_mirror_get_first(MIRROR_BY_PHYS_ADDR, (void *)(addr_t)address);
		}
	}

	return ptr;
}

/**
 * @brief Returns address of the selected RX mirror of the given interface
 * @param[in] iface Interface to be queried
 * @param[in] sel Selector of the RX mirror (0 to PFE_CT_MIRRORS_COUNT - 1)
 * @return The mirror reference or NULL if no mirror is configured
 */
pfe_mirror_t *pfe_phy_if_get_rx_mirror(const pfe_phy_if_t *iface, uint32_t sel)
{
	uint32_t address;
	pfe_mirror_t *ptr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ptr = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if(sel >= PFE_CT_MIRRORS_COUNT)
		{
			ptr = NULL;
		}
		else
		{
			address = oal_ntohl(iface->phy_if_class.rx_mirrors[sel]);
			ptr = pfe_mirror_get_first(MIRROR_BY_PHYS_ADDR, (void *)(addr_t)address);
		}
	}

	return ptr;
}

/**
 * @brief		Get HW ID of the interface
 * @param[in]	iface The interface instance
 * @return		Interface ID
 */
__attribute__((pure)) pfe_ct_phy_if_id_t pfe_phy_if_get_id(const pfe_phy_if_t *iface)
{
	pfe_ct_phy_if_id_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = PFE_PHY_IF_ID_INVALID;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = iface->id;
	}
	return ret;
}

/**
 * @brief		Get name
 * @param[in]	iface The interface instance
 * @return		Pointer to interface name string or "(unknown)" when called with NULL
 */
__attribute__((pure)) const char_t *pfe_phy_if_get_name(const pfe_phy_if_t *iface)
{
	const char_t *str;

	if (NULL != iface)
	{
		str = iface->name;
	}
	else
	{
		NXP_LOG_WARNING("NULL argument received for pfe_phy_if_get_name\n");
		str = "(unknown)";
	}

	return str;
}

/**
 * @brief		Return physical interface runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	iface		The physical interface instance
 * @param[in]	buf			A pointer to the buffer to write to
 * @param[in]	buf_len		Buffer length
 * @param[in]	verb_level	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_phy_if_get_text_statistics(const pfe_phy_if_t *iface, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;
	pfe_ct_phy_if_t phy_if_class = {0U};
	uint32_t i = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		len = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Repeat read for all PEs (just because of statistics) */
		while(i < pfe_class_get_num_of_pes(iface->class))
		{
			/*
				Read current interface configuration from classifier. Since all class PEs are running the
				same code, also the data are the same (except statistics counters...).
				Returned data will be in __NETWORK__ endian format.
			*/
			if (EOK != pfe_class_read_dmem(iface->class, i, &phy_if_class, iface->dmem_base, sizeof(pfe_ct_phy_if_t)))
			{
				len += oal_util_snprintf(buf + len, buf_len - len, "[PhyIF 0x%x]: Unable to read DMEM\n", iface->id);
			}
			else
			{
				len += oal_util_snprintf(buf + len, buf_len - len, "[PhyIF 0x%x '%s']\n", iface->id, pfe_phy_if_get_name(iface));
				len += oal_util_snprintf(buf + len, buf_len - len, "LogIfBase (DMEM) : 0x%x\n", oal_ntohl(phy_if_class.log_ifs));
				len += oal_util_snprintf(buf + len, buf_len - len, "DefLogIf  (DMEM) : 0x%x\n", oal_ntohl(phy_if_class.def_log_if));
				(void)pfe_phy_if_stat_to_str(&phy_if_class.phy_stats, buf + len, buf_len - len, verb_level);
			}
			++i;
		}
	}
	return len;
}

/**
 * @brief		Get statistic values in numeric form
 * @details		This function providing single statistic value
 *				from the EMAC block.
 * @param[in]	iface The interface instance
 * @param[in]	stat_id	ID of required statistic (offset of register)
 * @return		Value of requested statistic
 */
uint32_t pfe_phy_if_get_stat_value(pfe_phy_if_t *iface, uint32_t stat_id)
{
	uint32_t ret = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = 0xFFFFFFFFU;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		if (NULL == iface->port.instance)
		{
			/*	No HW block associated */
			;
		}
		else
		{
			if (PFE_PHY_IF_EMAC == iface->type)
			{
				ret = pfe_emac_get_stat_value(iface->port.emac, stat_id);
				if (0xFFFFFFFFU == ret)
				{
					NXP_LOG_ERROR("Unable to get statistic values\n");
				}
			}
			else
			{
				ret = 0xFFFFFFFFU;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}
	return ret;
}

#endif /* ! PFE_CFG_PFE_SLAVE */

