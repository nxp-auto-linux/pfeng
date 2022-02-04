/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup  dxgr_PFE_RTABLE
 * @{
 *
 * @file		pfe_rtable.c
 * @brief		The RTable module source file.
 * @details		This file contains routing table-related functionality.
 *
 * All values at rtable input level (API) shall be in host byte order format.
 */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "linked_list.h"

#include "fifo.h"
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_rtable.h"
#include "pfe_class.h"

/**
 * @brief	If TRUE then driver performs an entry update only if it is ensured that firmware
 *			and the driver are not accessing/updating the same entry in the same time.
 */
#define PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE	TRUE

/**
 * @brief	Select criterion argument type
 * @details	Used to store and pass argument to the pfe_rtable_match_criterion()
 * @see pfe_rtable_get_criterion_t
 * @see pfe_rtable_match_criterion()
 */
typedef union
{
	pfe_phy_if_t *iface;				/*!< Valid for the RTABLE_CRIT_BY_DST_IF criterion */
	uint32_t route_id;					/*!< Valid for the RTABLE_CRIT_BY_ROUTE_ID criterion */
	uint32_t id5t;						/*!< Valid for the RTABLE_CRIT_BY_ID5T criterion */
	pfe_5_tuple_t five_tuple;			/*!< Valid for the RTABLE_CRIT_BY_5_TUPLE criterion */
} pfe_rtable_criterion_arg_t;

/**
 * @brief	Routing table representation
 */
struct pfe_rtable_tag
{
	addr_t htable_base_pa;					/*	Hash table: Base physical address */
	addr_t htable_base_va;					/*	Hash table: Base virtual address */
	addr_t htable_end_pa;					/*	Hash table: End of hash table, physical */
	addr_t htable_end_va;					/*	Hash table: End of hash table, virtual */
	addr_t htable_va_pa_offset;				/*	Offset = VA - PA */
	uint32_t htable_size;					/*	Hash table: Number of entries */

	addr_t pool_base_pa;						/*	Pool: Base physical address */
	addr_t pool_base_va;						/*	Pool: Base virtual address */
	addr_t pool_end_pa;						/*	Pool: End of pool, physical */
	addr_t pool_end_va;						/*	Pool: End of pool, virtual */
	addr_t pool_va_pa_offset;				/*	Offset = VA - PA */
	uint32_t pool_size;						/*	Pool: Number of entries */
	fifo_t *pool_va;						/*	Pool of entries (virtual addresses) */

	LLIST_t active_entries;					/*	List of active entries. Need to be protected by mutex */

	oal_mutex_t *lock;						/*	Mutex to protect the table and related resources from concurrent accesses */
#if !defined(PFE_CFG_TARGET_OS_AUTOSAR)
	oal_thread_t *worker;					/*	Worker thread */
	oal_mbox_t *mbox;						/*	Message box to communicate with the worker thread */
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) */

	pfe_rtable_get_criterion_t cur_crit;	/*	Current criterion */
	LLIST_t *cur_item;						/*	Current entry to be returned. See ...get_first() and ...get_next() */
	pfe_rtable_criterion_arg_t cur_crit_arg;/*	Current criterion argument */
	pfe_l2br_t *bridge; /* Bridge pointer */
	pfe_class_t *class;						/*	Classifier */
	uint32_t active_entries_count;			/*  Counter of active RTable entries, needed for enabling/disabling of RTable lookup */
};

/**
 * @brief	Routing table entry at API level
 * @details	Since routing table entries (pfe_ct_rtable_entry_t) are shared between
 * 			firmware and the driver we're extending them using custom entries. Every
 *			physical entry has assigned an API entry to keep additional, driver-related
 *			information.
 */
struct pfe_rtable_entry_tag
{
	pfe_rtable_t *rtable;						/*	!< Reference to the parent table */
	pfe_ct_rtable_entry_t *phys_entry;			/*	!< Pointer to the entry within the routing table */
	pfe_ct_rtable_entry_t *temp_phys_entry;		/*	!< Temporary storage during entry creation process */
	struct pfe_rtable_entry_tag *next;		/*	!< Pointer to the next entry within the routing table */
	struct pfe_rtable_entry_tag *prev;		/*	!< Pointer to the previous entry within the routing table */
	struct pfe_rtable_entry_tag *child;		/*	!< Entry associated with this one (used to identify entries for 'reply' direction) */
	uint32_t timeout;							/*	!< Timeout value in seconds */
	uint32_t curr_timeout;						/*	!< Current timeout value */
	uint32_t route_id;							/*	!< User-defined route ID */
	bool_t route_id_valid;						/*	!< If TRUE then 'route_id' is valid */
	void *refptr;								/*	!< User-defined value */
	pfe_rtable_callback_t callback;				/*	!< User-defined callback function */
	void *callback_arg;							/*	!< User-defined callback argument */
	LLIST_t list_entry;							/*	!< Linked list element */
	LLIST_t list_to_remove_entry;				/*	!< Linked list element */
};

/**
 * @brief	Hash types
 * @details	PFE offers possibility to calculate various hash types to be used
 * 			for routing table lookups.
 *
 *			Standard 5-tuple hash (IPV4_5T/IPV6_5T) is equal to:
 *
 * 				SIP + DIP + SPORT + DPORT + PROTO
 *
 * 			Another types can be added (OR-ed) as modifications of the standard
 *			algorithm.
 * @note	It must be ensured that firmware is configured the same way as the
 * 			driver, i.e. firmware works with the same hash type as the driver.
 */
typedef enum
{
	IPV4_5T = 0x1,			/*	!< Standard 5-tuple hash (IPv4) */
	IPV6_5T = 0x2,			/*	!< Standard 5-tuple hash (IPv6) */
	ADD_SIP_CRC = 0x4,		/*	!< Use CRC(SIP) instead of SIP */
	ADD_SPORT_CRC = 0x8,	/*	!< Use CRC(SPORT) instead of SPORT */
	ADD_SRC_PHY = 0x10		/*	!< Add PHY ID to the hash */
} pfe_rtable_hash_type_t;

/**
 * @brief	IP version type
 */
typedef enum
{
	IPV4 = 0,
	IPV6,
	IPV_INVALID = 0xff
} pfe_ipv_type_t;

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR)
/**
 * @brief	Worker thread signals
 * @details	Driver is sending signals to the worker thread to request specific
 * 			operations.
 */
enum pfe_rtable_worker_signals
{
	SIG_WORKER_STOP,	/*	!< Stop the thread */
	SIG_TIMER_TICK		/*	!< Pulse from timer */
};
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) */

static uint32_t pfe_get_crc32_be(uint32_t crc, uint8_t *data, uint16_t len);
static void pfe_rtable_invalidate(pfe_rtable_t *rtable);
static uint32_t pfe_rtable_entry_get_hash(pfe_rtable_entry_t *entry, pfe_rtable_hash_type_t htype, uint32_t hash_mask);
static bool_t pfe_rtable_phys_entry_is_htable(const pfe_rtable_t *rtable, const pfe_ct_rtable_entry_t *phys_entry);
static bool_t pfe_rtable_phys_entry_is_pool(const pfe_rtable_t *rtable, const pfe_ct_rtable_entry_t *phys_entry);
static pfe_ct_rtable_entry_t *pfe_rtable_phys_entry_get_pa(pfe_rtable_t *rtable, pfe_ct_rtable_entry_t *phys_entry_va);
static pfe_ct_rtable_entry_t *pfe_rtable_phys_entry_get_va(pfe_rtable_t *rtable, pfe_ct_rtable_entry_t *phys_entry_pa);
static errno_t pfe_rtable_del_entry_nolock(pfe_rtable_t *rtable, pfe_rtable_entry_t *entry);
static bool_t pfe_rtable_match_criterion(pfe_rtable_get_criterion_t crit, const pfe_rtable_criterion_arg_t *arg, pfe_rtable_entry_t *entry);
static bool_t pfe_rtable_entry_is_in_table(const pfe_rtable_entry_t *entry);
static pfe_rtable_entry_t *pfe_rtable_get_by_phys_entry_va(const pfe_rtable_t *rtable, const pfe_ct_rtable_entry_t *phys_entry_va);

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR)
	static void *rtable_worker_func(void *arg);
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) */

#define CRCPOLY_BE 0x04C11DB7U

static pfe_rtable_entry_t *pfe_rtable_get_by_phys_entry_va(const pfe_rtable_t *rtable, const pfe_ct_rtable_entry_t *phys_entry_va)
{
	LLIST_t *item;
	pfe_rtable_entry_t *entry;
	bool_t match = FALSE;

	/* There is no protection for the multiple accesses to the table because the function is called
	   from the code which has already locked the table */

	/*	Search for first matching entry */
	if (FALSE == LLIST_IsEmpty(&rtable->active_entries))
	{
		/*	Get first matching entry */
		LLIST_ForEach(item, &rtable->active_entries)
		{
			/*	Get data */
			entry = LLIST_Data(item, pfe_rtable_entry_t, list_entry);

			/*	Remember current item to know where to start later */
			if (NULL != entry)
			{
				if (phys_entry_va == entry->phys_entry)
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

static uint32_t pfe_get_crc32_be(uint32_t crc, uint8_t *data, uint16_t len)
{
	uint8_t i;
    uint16_t length = len;
    uint32_t tempcrc = crc;
    const uint8_t *tempdata = data;

	while (length > 0U)
	{
		tempcrc ^= ((uint32_t)(*tempdata) << 24U);
        tempdata++;

		for (i = 0U; i < 8U; i++)
		{
			tempcrc = (tempcrc << 1U) ^ ((0U != (tempcrc & 0x80000000U)) ? CRCPOLY_BE : 0U);
		}

        length--;
	}

	return tempcrc;
}

/**
 * @brief		Invalidate all routing table entries
 * @param[in]	rtable The routing table instance
 */
static void pfe_rtable_invalidate(pfe_rtable_t *rtable)
{
	uint32_t ii;
	pfe_ct_rtable_entry_t *table;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == rtable))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	table = (pfe_ct_rtable_entry_t *)rtable->htable_base_va;

	if (unlikely(EOK != oal_mutex_lock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex lock failed\n");
    }

	for (ii=0U; ii<rtable->htable_size; ii++)
	{
		table[ii].flags = (pfe_ct_rtable_flags_t)(oal_ntohl(0));
		table[ii].next = oal_ntohl(0);
	}

	table = (pfe_ct_rtable_entry_t *)rtable->pool_base_va;

	for (ii=0U; ii<rtable->pool_size; ii++)
	{
		table[ii].flags = (pfe_ct_rtable_flags_t)(oal_ntohl(0));
		table[ii].next = oal_ntohl(0);
	}

    if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex unlock failed\n");
    }
}

/**
 * @brief		Get hash for a routing table entry
 * @param[in]	entry The entry
 * @param[in]	htype Bitfield representing desired hash type (see pfe_rtable_hash_type_t)
 * @param[in]	hash_mask Mask to be applied on the resulting hash (bitwise AND)
 * @note		IPv4 addresses within entry are in network order due to way how the type is defined
 */
static uint32_t pfe_rtable_entry_get_hash(pfe_rtable_entry_t *entry, pfe_rtable_hash_type_t htype, uint32_t hash_mask)
{
	uint32_t temp = 0U;
	uint32_t crc = 0xffffffffU;
	uint32_t sport = 0U;
	uint32_t ip_addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
        return 0;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
    if (0U != ((uint8_t)htype & (uint8_t)IPV4_5T))
    {
        if ((0U != ((uint8_t)htype & (uint8_t)ADD_SIP_CRC)) && (0U != ((uint8_t)htype & (uint8_t)ADD_SPORT_CRC)))
        {
            /*	CRC(SIP) + DIP + CRC(SPORT) + DPORT + PROTO */
            sport = oal_ntohl(entry->phys_entry->ipv.v4.sip) ^ (uint32_t)oal_ntohs(entry->phys_entry->sport);
            temp = pfe_get_crc32_be(crc, (uint8_t *)&sport, 4);
            temp += oal_ntohl(entry->phys_entry->ipv.v4.dip);
            temp += entry->phys_entry->proto;
            temp += oal_ntohs(entry->phys_entry->dport);
        }
        else if (0U != ((uint8_t)htype & (uint8_t)ADD_SIP_CRC))
        {
            /*	CRC(SIP) + DIP + SPORT + DPORT + PROTO */
            ip_addr = oal_ntohl(entry->phys_entry->ipv.v4.sip);
            temp = pfe_get_crc32_be(crc, (uint8_t *)&ip_addr, 4);
            temp += oal_ntohl(entry->phys_entry->ipv.v4.dip);
            temp += entry->phys_entry->proto;
            temp += oal_ntohs(entry->phys_entry->sport);
            temp += oal_ntohs(entry->phys_entry->dport);
        }
        else if (0U != ((uint8_t)htype & (uint8_t)ADD_SPORT_CRC))
        {
            /*	SIP + DIP + CRC(SPORT) + DPORT + PROTO */
            sport = (uint32_t)oal_ntohs(entry->phys_entry->sport);
            temp = pfe_get_crc32_be(crc, (uint8_t *)&sport, 4);
            temp += oal_ntohl(entry->phys_entry->ipv.v4.sip);
            temp += oal_ntohl(entry->phys_entry->ipv.v4.dip);
            temp += entry->phys_entry->proto;
            temp += oal_ntohs(entry->phys_entry->dport);
        }
        else
        {
            /*	SIP + DIP + SPORT + DPORT + PROTO */
            temp = oal_ntohl(entry->phys_entry->ipv.v4.sip);
            temp += oal_ntohl(entry->phys_entry->ipv.v4.dip);
            temp += entry->phys_entry->proto;
            temp += oal_ntohs(entry->phys_entry->sport);
            temp += oal_ntohs(entry->phys_entry->dport);
        }
    }
    else if (0U != ((uint32_t)htype & (uint32_t)IPV6_5T))
    {
        uint32_t crc_ipv6 = 0;
        int32_t jj;

        for(jj=0; jj<4 ; jj++)
        {
            crc_ipv6 += (oal_ntohl(entry->phys_entry->ipv.v6.sip[jj]));
        }

        if ((0U != ((uint8_t)htype & (uint8_t)ADD_SIP_CRC)) && (0U != ((uint8_t)htype & (uint8_t)ADD_SPORT_CRC)))
        {
            /*	CRC(SIP) + DIP + CRC(SPORT) + DPORT + PROTO */
            sport = crc_ipv6 ^ (uint32_t)oal_ntohs(entry->phys_entry->sport);
            temp = pfe_get_crc32_be(crc,(uint8_t *)&sport, 4);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[0]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[1]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[2]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[3]);
            temp += entry->phys_entry->proto;
            temp += oal_ntohs(entry->phys_entry->dport);
        }
        else if (0U != ((uint8_t)htype & (uint8_t)ADD_SIP_CRC))
        {
            /*	CRC(SIP) + DIP + SPORT + DPORT + PROTO */
            temp = pfe_get_crc32_be(crc, (uint8_t *)&crc_ipv6, 4);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[0]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[1]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[2]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[3]);
            temp += entry->phys_entry->proto;
            temp += oal_ntohs(entry->phys_entry->sport);
            temp += oal_ntohs(entry->phys_entry->dport);
        }
        else if (0U != ((uint8_t)htype & (uint8_t)ADD_SPORT_CRC))
        {
            /*	SIP + DIP + CRC(SPORT) + DPORT + PROTO */
            sport = (uint32_t)oal_ntohs(entry->phys_entry->sport);
            temp = pfe_get_crc32_be(crc,(uint8_t *)&sport, 4);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.sip[0]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.sip[1]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.sip[2]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.sip[3]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[0]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[1]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[2]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[3]);
            temp += entry->phys_entry->proto;
            temp += oal_ntohs(entry->phys_entry->dport);
        }
        else
        {
            /*	SIP + DIP + SPORT + DPORT + PROTO */
            temp = oal_ntohl(entry->phys_entry->ipv.v6.sip[0]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.sip[1]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.sip[2]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.sip[3]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[0]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[1]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[2]);
            temp += oal_ntohl(entry->phys_entry->ipv.v6.dip[3]);
            temp += entry->phys_entry->proto;
            temp += oal_ntohs(entry->phys_entry->sport);
            temp += oal_ntohs(entry->phys_entry->dport);
        }
    }
    else
    {
        NXP_LOG_ERROR("Unknown hash type requested\n");
        return 0U;
    }

    if (0U != ((uint8_t)htype & (uint8_t)ADD_SRC_PHY))
    {
        /*	+ PHY_ID */
#if 0
        temp += entry->phys_entry->inPhyPortNum;
#else
        NXP_LOG_ERROR("Unsupported hash algorithm\n");
#endif /* 0 */
    }

	return (temp & hash_mask);
}

/**
 * @brief		Check if entry belongs to hash table
 * @param[in]	rtable The routing table instance
 * @param[in]	phys_entry Entry to be checked (VA or PA)
 * @retval		TRUE Entry belongs to hash table
 * @retval		FALSE Entry does not belong to hash table
 */
static bool_t pfe_rtable_phys_entry_is_htable(const pfe_rtable_t *rtable, const pfe_ct_rtable_entry_t *phys_entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == phys_entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (((addr_t)phys_entry >= rtable->htable_base_va) && ((addr_t)phys_entry < rtable->htable_end_va))
	{
		return TRUE;
	}
	else
	{
		if (((addr_t)phys_entry >= rtable->htable_base_pa) && ((addr_t)phys_entry < rtable->htable_end_pa))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

/**
 * @brief		Check if entry belongs to the pool
 * @param[in]	rtable The routing table instance
 * @param[in]	phys_entry Entry to be checked (VA or PA)
 * @retval		TRUE Entry belongs to the pool
 * @retval		FALSE Entry does not belong to the pool
 */
static bool_t pfe_rtable_phys_entry_is_pool(const pfe_rtable_t *rtable, const pfe_ct_rtable_entry_t *phys_entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == phys_entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (((addr_t)phys_entry >= rtable->pool_base_va) && ((addr_t)phys_entry < rtable->pool_end_va))
	{
		return TRUE;
	}
	else
	{
		if (((addr_t)phys_entry >= rtable->pool_base_pa) && ((addr_t)phys_entry < rtable->pool_end_pa))
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

/**
 * @brief		Convert entry to physical address
 * @param[in]	rtable The routing table instance
 * @param[in]	phys_entry_va The entry (virtual address)
 * @return		The PA or NULL if failed
 */
static pfe_ct_rtable_entry_t *pfe_rtable_phys_entry_get_pa(pfe_rtable_t *rtable, pfe_ct_rtable_entry_t *phys_entry_va)
{
	pfe_ct_rtable_entry_t *pa = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == phys_entry_va)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (TRUE == pfe_rtable_phys_entry_is_htable(rtable, phys_entry_va))
	{
		pa = (pfe_ct_rtable_entry_t *)((addr_t)phys_entry_va - rtable->htable_va_pa_offset);
	}
	else if (TRUE == pfe_rtable_phys_entry_is_pool(rtable, phys_entry_va))
	{
		pa = (pfe_ct_rtable_entry_t *)((addr_t)phys_entry_va - rtable->pool_va_pa_offset);
	}
	else
	{
		return NULL;
	}

	return pa;
}

/**
 * @brief		Convert entry to virtual address
 * @param[in]	rtable The routing table instance
 * @param[in]	entry_pa The entry (physical address)
 * @return		The VA or NULL if failed
 */
static pfe_ct_rtable_entry_t *pfe_rtable_phys_entry_get_va(pfe_rtable_t *rtable, pfe_ct_rtable_entry_t *phys_entry_pa)
{
	pfe_ct_rtable_entry_t *va = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == phys_entry_pa)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (TRUE == pfe_rtable_phys_entry_is_htable(rtable, phys_entry_pa))
	{
		va = (pfe_ct_rtable_entry_t *)((addr_t)phys_entry_pa + rtable->htable_va_pa_offset);
	}
	else if (TRUE == pfe_rtable_phys_entry_is_pool(rtable, phys_entry_pa))
	{
		va = (pfe_ct_rtable_entry_t *)((addr_t)phys_entry_pa + rtable->pool_va_pa_offset);
	}
	else
	{
		return NULL;
	}

	return va;
}

/**
 * @brief		Create routing table entry instance
 * @details		Instance is intended to be used to construct the entry before it is
 *				inserted into the routing table.
 * @return		The new instance or NULL if failed
 */
pfe_rtable_entry_t *pfe_rtable_entry_create(void)
{
	pfe_rtable_entry_t *entry;

	entry = oal_mm_malloc(sizeof(pfe_rtable_entry_t));
	if (NULL == entry)
	{
		return NULL;
	}
	else
	{
		(void)memset(entry, 0, sizeof(pfe_rtable_entry_t));
		entry->temp_phys_entry = NULL;
		entry->phys_entry = NULL;

		/*	This is temporary 'physical' entry storage */
		entry->temp_phys_entry = oal_mm_malloc(sizeof(pfe_ct_rtable_entry_t));
		if (NULL == entry->temp_phys_entry)
		{
			oal_mm_free(entry);
			return NULL;
		}
		else
		{
			(void)memset(entry->temp_phys_entry, 0, sizeof(pfe_ct_rtable_entry_t));
			entry->phys_entry = entry->temp_phys_entry;

			/*	Set defaults */
			entry->rtable = NULL;
			entry->timeout = 0xffffffffU;
			entry->curr_timeout = entry->timeout;
			entry->route_id = 0U;
			entry->route_id_valid = FALSE;
			entry->callback = NULL;
			entry->callback_arg = NULL;
			entry->refptr = NULL;
			entry->child = NULL;
		}

		entry->temp_phys_entry->flag_ipv6 = (uint8_t)IPV_INVALID;
	}

	return entry;
}

/**
 * @brief		Release routing table entry instance
 * @details		Once the previously created routing table entry instance is not needed
 * 				anymore (inserted into the routing table), allocated resources shall
 * 				be released using this call.
 * @param[in]	entry Entry instance previously created by pfe_rtable_entry_create()
 */
void pfe_rtable_entry_free(pfe_rtable_entry_t *entry)
{
	if (NULL != entry)
	{
		if (NULL != entry->temp_phys_entry)
		{
			oal_mm_free(entry->temp_phys_entry);
		}

		oal_mm_free(entry);
	}
}

/**
 * @brief		Set 5 tuple values
 * @param[in]	entry The routing table entry instance
 * @param[in]	tuple The 5 tuple type instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
errno_t pfe_rtable_entry_set_5t(pfe_rtable_entry_t *entry, const pfe_5_tuple_t *tuple)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = pfe_rtable_entry_set_sip(entry, &tuple->src_ip);
	if (EOK != ret)
	{
		return ret;
	}

	ret = pfe_rtable_entry_set_dip(entry, &tuple->dst_ip);
	if (EOK != ret)
	{
		return ret;
	}

	pfe_rtable_entry_set_sport(entry, tuple->sport);
	pfe_rtable_entry_set_dport(entry, tuple->dport);
	pfe_rtable_entry_set_proto(entry, tuple->proto);

	return EOK;
}

/**
 * @brief		Set source IP address
 * @param[in]	entry The routing table entry instance
 * @param[in]	ip_addr The IP address
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
errno_t pfe_rtable_entry_set_sip(pfe_rtable_entry_t *entry,const pfe_ip_addr_t *ip_addr)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == ip_addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (ip_addr->is_ipv4)
	{
		if ((entry->phys_entry->flag_ipv6 != (uint8_t)IPV_INVALID) && (entry->phys_entry->flag_ipv6 != (uint8_t)IPV4))
		{
			NXP_LOG_ERROR("IP version mismatch\n");
			return EINVAL;
		}

		(void)memcpy(&entry->phys_entry->ipv.v4.sip, &ip_addr->v4, 4);
		entry->phys_entry->flag_ipv6 = (uint8_t)IPV4;
	}
	else
	{
		if ((entry->phys_entry->flag_ipv6 != (uint8_t)IPV_INVALID) && (entry->phys_entry->flag_ipv6 != (uint8_t)IPV6))
		{
			NXP_LOG_ERROR("IP version mismatch\n");
			return EINVAL;
		}

		(void)memcpy(&entry->phys_entry->ipv.v6.sip[0], &ip_addr->v6, 16);
		entry->phys_entry->flag_ipv6 = (uint8_t)IPV6;
	}

	return EOK;
}

/**
 * @brief		Get source IP address
 * @param[in]	entry The routing table entry instance
 * @param[out]	ip_addr Pointer where the IP address shall be written
 */
void pfe_rtable_entry_get_sip(pfe_rtable_entry_t *entry, pfe_ip_addr_t *ip_addr)
{
	pfe_5_tuple_t tuple;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == ip_addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != pfe_rtable_entry_to_5t(entry, &tuple))
	{
		NXP_LOG_ERROR("Entry conversion failed\n");
	}

	(void)memcpy(ip_addr, &tuple.src_ip, sizeof(pfe_ip_addr_t));
}

/**
 * @brief		Set destination IP address
 * @param[in]	entry The routing table entry instance
 * @param[in]	ip_addr The IP address
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
errno_t pfe_rtable_entry_set_dip(pfe_rtable_entry_t *entry, const pfe_ip_addr_t *ip_addr)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == ip_addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (ip_addr->is_ipv4)
	{
		if ((entry->phys_entry->flag_ipv6 != (uint8_t)IPV_INVALID) && (entry->phys_entry->flag_ipv6 != (uint8_t)IPV4))
		{
			NXP_LOG_ERROR("IP version mismatch\n");
			return EINVAL;
		}

		(void)memcpy(&entry->phys_entry->ipv.v4.dip, &ip_addr->v4, 4);
		entry->phys_entry->flag_ipv6 = (uint8_t)IPV4;
	}
	else
	{
		if ((entry->phys_entry->flag_ipv6 != (uint8_t)IPV_INVALID) && (entry->phys_entry->flag_ipv6 != (uint8_t)IPV6))
		{
			NXP_LOG_ERROR("IP version mismatch\n");
			return EINVAL;
		}

		(void)memcpy(&entry->phys_entry->ipv.v6.dip[0], &ip_addr->v6, 16);
		entry->phys_entry->flag_ipv6 = (uint8_t)IPV6;
	}

	return EOK;
}

/**
 * @brief		Get destination IP address
 * @param[in]	entry The routing table entry instance
 * @param[out]	ip_addr Pointer where the IP address shall be written
 */
void pfe_rtable_entry_get_dip(pfe_rtable_entry_t *entry, pfe_ip_addr_t *ip_addr)
{
	pfe_5_tuple_t tuple;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == ip_addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != pfe_rtable_entry_to_5t(entry, &tuple))
	{
		NXP_LOG_ERROR("Entry conversion failed\n");
	}

	(void)memcpy(ip_addr, &tuple.dst_ip, sizeof(pfe_ip_addr_t));
}

/**
 * @brief		Set source L4 port number
 * @param[in]	entry The routing table entry instance
 * @param[in]	sport The port number
 */
void pfe_rtable_entry_set_sport(pfe_rtable_entry_t *entry, uint16_t sport)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->sport = oal_htons(sport);
}

/**
 * @brief		Get source L4 port number
 * @param[in]	entry The routing table entry instance
 * @return		The assigned source port number
 */
uint16_t pfe_rtable_entry_get_sport(const pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return oal_ntohs(entry->phys_entry->sport);
}

/**
 * @brief		Set destination L4 port number
 * @param[in]	entry The routing table entry instance
 * @param[in]	sport The port number
 */
void pfe_rtable_entry_set_dport(pfe_rtable_entry_t *entry, uint16_t dport)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->dport = oal_htons(dport);
}

/**
 * @brief		Get destination L4 port number
 * @param[in]	entry The routing table entry instance
 * @return		The assigned destination port number
 */
uint16_t pfe_rtable_entry_get_dport(const pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return oal_ntohs(entry->phys_entry->dport);
}

/**
 * @brief		Set IP protocol number
 * @param[in]	entry The routing table entry instance
 * @param[in]	sport The protocol number
 */
void pfe_rtable_entry_set_proto(pfe_rtable_entry_t *entry, uint8_t proto)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->proto = proto;
}

/**
 * @brief		Get IP protocol number
 * @param[in]	entry The routing table entry instance
 * @return		The assigned protocol number
 */
uint8_t pfe_rtable_entry_get_proto(const pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return entry->phys_entry->proto;
}

/**
 * @brief		Set destination interface using its ID
 * @param[in]	entry The routing table entry instance
 * @param[in]	if_id Interface ID of interface to be used to forward traffic matching the entry
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
errno_t pfe_rtable_entry_set_dstif_id(pfe_rtable_entry_t *entry, pfe_ct_phy_if_id_t if_id)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if (if_id > PFE_PHY_IF_ID_MAX)
	{
		NXP_LOG_WARNING("Physical interface ID is invalid: 0x%x\n", if_id);
		return EINVAL;
	}

	entry->phys_entry->e_phy_if = if_id;
    return EOK;
}
/**
 * @brief		Set destination interface
 * @param[in]	entry The routing table entry instance
 * @param[in]	emac The destination interface to be used to forward traffic matching
 * 					  the entry.
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
errno_t pfe_rtable_entry_set_dstif(pfe_rtable_entry_t *entry, const pfe_phy_if_t *iface)
{
	pfe_ct_phy_if_id_t if_id = PFE_PHY_IF_ID_INVALID;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if_id = pfe_phy_if_get_id(iface);

    return pfe_rtable_entry_set_dstif_id(entry, if_id);
}


/**
 * @brief		Set output source IP address
 * @details		IP address set using this call will be used to replace the original address
 * 				if the RT_ACT_CHANGE_SIP_ADDR action is set.
 * @param[in]	entry The routing table entry instance
 * @param[in]	output_sip The desired output source IP address
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
errno_t pfe_rtable_entry_set_out_sip(pfe_rtable_entry_t *entry, const pfe_ip_addr_t *output_sip)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == output_sip)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (((uint8_t)IPV_INVALID != entry->phys_entry->flag_ipv6) && (output_sip->is_ipv4))
	{
		(void)memcpy(&entry->phys_entry->args.ipv.v4.sip, &output_sip->v4, 4);
		entry->phys_entry->flag_ipv6 = (uint8_t)IPV4;
	}
	else if (((uint8_t)IPV_INVALID != entry->phys_entry->flag_ipv6) && (!output_sip->is_ipv4))
	{
		(void)memcpy(&entry->phys_entry->args.ipv.v6.sip[0], &output_sip->v6, 16);
		entry->phys_entry->flag_ipv6 = (uint8_t)IPV6;
	}
	else
	{
		NXP_LOG_ERROR("IP version mismatch\n");
		return EINVAL;
	}

	entry->phys_entry->actions |= oal_htonl(RT_ACT_CHANGE_SIP_ADDR);

	return EOK;
}

/**
 * @brief		Set output destination IP address
 * @details		IP address set using this call will be used to replace the original address
 * 				if the RT_ACT_CHANGE_DIP_ADDR action is set.
 * @param[in]	entry The routing table entry instance
 * @param[in]	output_dip The desired output destination IP address
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
errno_t pfe_rtable_entry_set_out_dip(pfe_rtable_entry_t *entry, const pfe_ip_addr_t *output_dip)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == output_dip)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (((uint8_t)IPV_INVALID != entry->phys_entry->flag_ipv6) && (output_dip->is_ipv4))
	{
		(void)memcpy(&entry->phys_entry->args.ipv.v4.dip, &output_dip->v4, 4);
		entry->phys_entry->flag_ipv6 = (uint8_t)IPV4;
	}
	else if (((uint8_t)IPV_INVALID != entry->phys_entry->flag_ipv6) && (!output_dip->is_ipv4))
	{
		(void)memcpy(&entry->phys_entry->args.ipv.v6.dip[0], &output_dip->v6, 16);
		entry->phys_entry->flag_ipv6 = (uint8_t)IPV6;
	}
	else
	{
		NXP_LOG_ERROR("IP version mismatch\n");
		return EINVAL;
	}

	entry->phys_entry->actions |= oal_htonl(RT_ACT_CHANGE_DIP_ADDR);

	return EOK;
}

/**
 * @brief		Set output source port number
 * @details		Port number set using this call will be used to replace the original source port
 * 				if the RT_ACT_CHANGE_SPORT action is set.
 * @param[in]	entry The routing table entry instance
 * @param[in]	output_sport The desired output source port number
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
void pfe_rtable_entry_set_out_sport(const pfe_rtable_entry_t *entry, uint16_t output_sport)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->args.sport = oal_htons(output_sport);
	entry->phys_entry->actions |= oal_htonl(RT_ACT_CHANGE_SPORT);
}

/**
 * @brief		Set output destination port number
 * @details		Port number set using this call will be used to replace the original destination port
 * 				if the RT_ACT_CHANGE_DPORT action is set.
 * @param[in]	entry The routing table entry instance
 * @param[in]	output_sport The desired output destination port number
 * @retval		EOK Success
 * @retval		EINVAL Invalid argument
 */
void pfe_rtable_entry_set_out_dport(pfe_rtable_entry_t *entry, uint16_t output_dport)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->args.dport = oal_htons(output_dport);
	entry->phys_entry->actions |= oal_htonl(RT_ACT_CHANGE_DPORT);
}

/**
 * @brief		Set TTL decrement
 * @details		Set TTL to be decremented
 *			if the RT_ACT_DEC_TTL action is set.
 * @param[in]	entry The routing table entry instance
 */

void pfe_rtable_entry_set_ttl_decrement(pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->actions |= oal_htonl(RT_ACT_DEC_TTL);
}

/**
 * @brief		Remove TTL decrement
 * @details		Remove TTL to be decremented
 *			if the RT_ACT_DEC_TTL action is set.
 * @param[in]	entry The routing table entry instance
 */

void pfe_rtable_entry_remove_ttl_decrement(pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->actions &= ~(oal_htonl(RT_ACT_DEC_TTL));
}

/**
 * @brief		Set output source and destination MAC address
 * @details		MAC address set using this call will be used to add/replace the original MAC
 * 				address if the RT_ACT_ADD_ETH_HDR action is set.
 * @param[in]	entry The routing table entry instance
 * @param[in]	smac The desired output source MAC address
 * @param[in]	dmac The desired output destination MAC address
 */
void pfe_rtable_entry_set_out_mac_addrs(pfe_rtable_entry_t *entry, const pfe_mac_addr_t smac, const pfe_mac_addr_t dmac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)memcpy(entry->phys_entry->args.smac, smac, sizeof(pfe_mac_addr_t));
	(void)memcpy(entry->phys_entry->args.dmac, dmac, sizeof(pfe_mac_addr_t));
	entry->phys_entry->actions |= oal_htonl(RT_ACT_ADD_ETH_HDR);
}

/**
 * @brief		Set output VLAN tag
 * @details		VLAN tag set using this call will be used to add/replace the original VLAN tag
 * 				if the RT_ACT_ADD_VLAN_HDR/RT_ACT_MOD_VLAN_HDR action is set.
 * @param[in]	entry The routing table entry instance
 * @param[in]	vlan The desired output VLAN tag
 * @param[in]	replace When TRUE the VLAN tag will be replaced or added based on ingress
 * 					frame vlan tag presence. When FALSE	then VLAN tag will be always added.
 */
void pfe_rtable_entry_set_out_vlan(pfe_rtable_entry_t *entry, uint16_t vlan, bool_t replace)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->args.vlan = oal_htons(vlan);

	if (replace)
	{
		entry->phys_entry->actions |= oal_htonl(RT_ACT_MOD_VLAN_HDR);
	}
	else
	{
		entry->phys_entry->actions |= oal_htonl(RT_ACT_ADD_VLAN_HDR);
	}
}

/**
 * @brief		Get output VLAN tag
 * @details		If VLAN addition/replacement for the entry is requested via
 * 				pfe_rtable_entry_set_out_vlan() then this function will return
 * 				the VLAN tag. If no VLAN manipulation for the entry was has
 * 				been requested then the return value is 0.
 * @param[in]	entry The routing table entry instance
 * return		Non-zero VLAN ID (host endian) if VLAN manipulation has been
 *				requested, zero otherwise
 */
uint16_t pfe_rtable_entry_get_out_vlan(const pfe_rtable_entry_t *entry)
{
    uint16_t ret = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
    else
    {
#endif /* PFE_CFG_NULL_ARG_CHECK */

        if (0U != (oal_ntohl(entry->phys_entry->actions) & ((uint32_t)RT_ACT_ADD_VLAN_HDR | (uint32_t)RT_ACT_MOD_VLAN_HDR)))
        {
            ret = oal_ntohs(entry->phys_entry->args.vlan);
        }
#if defined(PFE_CFG_NULL_ARG_CHECK)
    }
#endif /* PFE_CFG_NULL_ARG_CHECK */

    return ret;
}

/**
 * @brief		Set output inner VLAN tag
 * @details		VLAN1 tag set using this call will be used to add/replace the original inner
 *				VLAN tag if the RT_ACT_ADD_VLAN1_HDR action is set.
 * @param[in]	entry The routing table entry instance
 * @param[in]	vlan The desired output inner VLAN tag
 */
void pfe_rtable_entry_set_out_inner_vlan(pfe_rtable_entry_t *entry, uint16_t vlan)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->args.vlan1 = oal_htons(vlan);
	entry->phys_entry->actions |= oal_htonl(RT_ACT_ADD_VLAN1_HDR);
}

/**
 * @brief		Set output PPPoE session ID
 * @details		Session ID set using this call will be used to add/replace the original ID
 * 				if the RT_ACT_ADD_PPPOE_HDR action is set.
 * @param[in]	entry The routing table entry instance
 * @param[in]	vlan The desired output PPPoE session ID
 */
void pfe_rtable_entry_set_out_pppoe_sid(pfe_rtable_entry_t *entry, uint16_t sid)
{
	uint32_t flags;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	flags = (pfe_ct_route_actions_t)(oal_ntohl(entry->phys_entry->actions));

	if (0U != (flags & (uint32_t)RT_ACT_ADD_VLAN1_HDR))
	{
		NXP_LOG_ERROR("Action (PFE_RTABLE_ADD_PPPOE_HDR) must no be combined with PFE_RTABLE_ADD_VLAN1_HDR\n");
		return;
	}

	if (0U == (flags & (uint32_t)RT_ACT_ADD_ETH_HDR))
	{
		NXP_LOG_ERROR("Action (PFE_RTABLE_ADD_PPPOE_HDR) requires also the PFE_RTABLE_ADD_ETH_HDR flag set\n");
		return;
	}

	entry->phys_entry->args.pppoe_sid = oal_htons(sid);
	entry->phys_entry->actions |= oal_htonl(RT_ACT_ADD_PPPOE_HDR);
}

void pfe_rtable_entry_set_id5t(pfe_rtable_entry_t *entry, uint32_t id5t)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->phys_entry->id5t = oal_htonl(id5t);
}

errno_t pfe_rtable_entry_get_id5t(const pfe_rtable_entry_t *entry, uint32_t *id5t)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

    *id5t = oal_ntohl(entry->phys_entry->id5t);
    return EOK;
}

/**
 * @brief		Get actions associated with routing entry
 * @param[in]	entry The routing table entry instance
 * @return		Value (bitwise OR) consisting of flags (pfe_ct_route_actions_t).
 */
pfe_ct_route_actions_t pfe_rtable_entry_get_action_flags(pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return RT_ACT_INVALID;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return (pfe_ct_route_actions_t)oal_ntohl((uint32_t)(entry->phys_entry->actions));
}

/**
 * @brief		Set entry timeout value
 * @param[in]	entry The routing table entry instance
 * @param[in]	timeout Timeout value in seconds
 */
void pfe_rtable_entry_set_timeout(pfe_rtable_entry_t *entry, uint32_t timeout)
{
	uint32_t elapsed;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != entry->rtable)
	{
        if (unlikely(EOK != oal_mutex_lock(entry->rtable->lock)))
        {
            NXP_LOG_DEBUG("Mutex lock failed\n");
        }
	}

	if (0xffffffffU == entry->timeout)
	{
		entry->curr_timeout = timeout;
	}
	else
	{
		elapsed = entry->timeout - entry->curr_timeout;

		if (elapsed >= timeout)
		{
			/*	This will cause entry timeout with next tick */
			entry->curr_timeout = 0U;
		}
		else
		{
			/*	Adjust current timeout by elapsed time of original timeout */
			entry->curr_timeout = timeout - elapsed;
		}
	}

	entry->timeout = timeout;

	if (NULL != entry->rtable)
	{
        if (unlikely(EOK != oal_mutex_unlock(entry->rtable->lock)))
        {
            NXP_LOG_DEBUG("Mutex unlock failed\n");
        }
	}
}

/**
 * @brief		Set route ID
 * @param[in]	entry The routing table entry instance
 * @param[in]	route_id Custom route identifier value
 */
void pfe_rtable_entry_set_route_id(pfe_rtable_entry_t *entry, uint32_t route_id)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->route_id = route_id;
	entry->route_id_valid = TRUE;
}

/**
 * @brief		Get route ID
 * @param[in]	entry The routing table entry instance
 * @param[in]	route_id Pointer to memory where the ID shall be written
 * @retval		EOK Success
 * @retval		ENOENT No route ID associated with the entry
 */
errno_t pfe_rtable_entry_get_route_id(const pfe_rtable_entry_t *entry, uint32_t *route_id)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == route_id)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (TRUE == entry->route_id_valid)
	{
		*route_id = entry->route_id;
		return EOK;
	}
	else
	{
		return ENOENT;
	}
}

/**
 * @brief		Set callback
 * @param[in]	entry The routing table entry instance
 * @param[in]	cbk Callback associated with the entry. Will be called in rtable worker thread
 * 				context. In the callback user must not call any routing table modification API
 *				functions (add/delete).
 * @param[in]	arg Argument passed to the callback when called
 * @param[in]	route_id Custom route identifier value
 */
void pfe_rtable_entry_set_callback(pfe_rtable_entry_t *entry, pfe_rtable_callback_t cbk, void *arg)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->callback = cbk;
	entry->callback_arg = arg;
}

/**
 * @brief		Bind custom reference pointer
 * @param[in]	entry The routing table entry instance
 * @param[in]	refptr Reference pointer to be bound with entry
 */
void pfe_rtable_entry_set_refptr(pfe_rtable_entry_t *entry, void *refptr)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->refptr = refptr;
}

/**
 * @brief		Get reference pointer
 * @param[in]	entry The routing table entry instance
 * @retval		The reference pointer
 */
void *pfe_rtable_entry_get_refptr(pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return entry->refptr;
}

/**
 * @brief		Associate with another entry
 * @details		If there is a bi-directional connection, it consists of two routing table entries:
 * 				one for original direction and one for reply direction. This function enables
 * 				user to bind the associated entries together and simplify handling.
 * @param[in]	entry The routing table entry instance
 * @param[in]	child The routing table entry instance to be linked with the 'entry'. Can be NULL.
 */
void pfe_rtable_entry_set_child(pfe_rtable_entry_t *entry, pfe_rtable_entry_t *child)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry->child = child;
}

/**
 * @brief		Get associated entry
 * @param[in]	entry The routing table entry instance
 * @return		The associated routing table entry linked with the 'entry'. NULL if there is not link.
 */
pfe_rtable_entry_t *pfe_rtable_entry_get_child(const pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return entry->child;
}

/***
 * @brief		Find out if entry has been added to a routing table
 * @param[in]	entry The routing table entry instance
 * @retval		TRUE Entry is in a routing table
 * @retval		FALSE Entry is not in a routing table
 */
static bool_t pfe_rtable_entry_is_in_table(const pfe_rtable_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != entry->rtable)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Check if entry is already in the table (5-tuple)
 * @param[in]	rtable The routing table instance
 * @param[in]	entry Entry prototype to be used for search
 * @note		IPv4 addresses within 'entry' are in network order due to way how the type is defined
 * @retval		TRUE Entry already added
 * @retval		FALSE Entry not found
 * @warning		Function is accessing routing table without protection from concurrent accesses.
 * 				Caller shall ensure proper protection.
 */
static bool_t pfe_rtable_entry_is_duplicate(pfe_rtable_t *rtable, pfe_rtable_entry_t *entry)
{
	pfe_rtable_entry_t *entry2;
	pfe_rtable_criterion_arg_t arg;
	bool_t match = FALSE;
	LLIST_t *item;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check for duplicates */
	if (EOK != pfe_rtable_entry_to_5t(entry, &arg.five_tuple))
	{
		NXP_LOG_ERROR("Entry conversion failed\n");
		return FALSE;
	}

	/*	Search for first matching entry */
	if (FALSE == LLIST_IsEmpty(&rtable->active_entries))
	{
		/*	Get first matching entry */
		LLIST_ForEach(item, &rtable->active_entries)
		{
			/*	Get data */
			entry2 = LLIST_Data(item, pfe_rtable_entry_t, list_entry);

            if (TRUE == pfe_rtable_match_criterion(RTABLE_CRIT_BY_5_TUPLE, &arg, entry2))
            {
                match = TRUE;
                break;
            }
		}
	}

	return match;
}

/**
 * @brief		Add entry to the table
 * @param[in]	rtable The routing table instance
 * @param[in]	entry The entry to be added
 * @retval		EOK Success
 * @retval		ENOENT Routing table is full
 * @retval		EEXIST Entry is already added
 * @retval		EINVAL Invalid entry
 * @note		IPv4 addresses within entry are in network order due to way how the type is defined
 */
errno_t pfe_rtable_add_entry(pfe_rtable_t *rtable, pfe_rtable_entry_t *entry)
{
	pfe_rtable_hash_type_t hash_type = ((uint8_t)IPV4 == entry->phys_entry->flag_ipv6) ? IPV4_5T : IPV6_5T;
	uint32_t hash;
	pfe_ct_rtable_entry_t *hash_table_va = (pfe_ct_rtable_entry_t *)rtable->htable_base_va;
	pfe_ct_rtable_entry_t *new_phys_entry_va = NULL, *new_phys_entry_pa = NULL, *last_phys_entry_va = NULL;
    errno_t ret;
    pfe_l2br_domain_t *domain;

#if (TRUE == PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE)
	pfe_ct_rtable_flags_t valid_tmp;
#endif /* PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Protect table accesses */
	if (unlikely(EOK != oal_mutex_lock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex lock failed\n");
    }

	/*	Check for duplicates */
	if (TRUE == pfe_rtable_entry_is_duplicate(rtable, entry))
	{
		NXP_LOG_INFO("Entry already added\n");

        if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
        {
            NXP_LOG_DEBUG("Mutex unlock failed\n");
        }

		return EEXIST;
	}

	hash = pfe_rtable_entry_get_hash(entry, hash_type, (rtable->htable_size - 1U));
	entry->temp_phys_entry->flags = RT_FL_NONE;
	entry->temp_phys_entry->status &= ~(uint8_t)RT_STATUS_ACTIVE;

	/* Add vlan stats index into the phy_entry structure */
	if (0U != (oal_ntohl(entry->temp_phys_entry->actions) & ((uint32_t)RT_ACT_ADD_VLAN_HDR | (uint32_t)RT_ACT_MOD_VLAN_HDR)))
	{
		if (NULL != rtable->bridge)
		{
			domain = pfe_l2br_get_first_domain(rtable->bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)oal_ntohs(entry->temp_phys_entry->args.vlan));
			if (domain != NULL)
			{
				entry->temp_phys_entry->args.vlan_stats_index = oal_htons((uint16_t)pfe_l2br_get_vlan_stats_index(domain));
			}
			else
			{
				/* Index 0 is the fallback domain */
				entry->temp_phys_entry->args.vlan_stats_index = 0;
			}
		}
	}

	/*	Allocate 'real' entry from hash heads or pool */
	if (0U == (oal_ntohl(hash_table_va[hash].flags) & (uint32_t)RT_FL_VALID))
	{
		new_phys_entry_va = &hash_table_va[hash];
	}
	else
	{
		/*	First-level entry is already occupied. Create entry within the pool. Get
			some free entry from the pool first. */
		new_phys_entry_va = fifo_get(rtable->pool_va);
		if (NULL == new_phys_entry_va)
		{
            if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
            {
                NXP_LOG_DEBUG("Mutex unlock failed\n");
            }

			return ENOENT;
		}
		NXP_LOG_WARNING("Routing table hash [%u] collision detected. New entry will be added to linked list leading to performance penalty during lookup.\n", (uint_t)hash);
	}

	/*	Make sure the new entry is invalid */
	new_phys_entry_va->flags = RT_FL_NONE;

	/*	Get physical address */
	new_phys_entry_pa = pfe_rtable_phys_entry_get_pa(rtable, new_phys_entry_va);
	if (NULL == new_phys_entry_pa)
	{
		NXP_LOG_ERROR("Couldn't get PA (entry @ v0x%p)\n", (void *)new_phys_entry_va);
		goto free_and_fail;
	}

	/*	Set link */
	if (TRUE == pfe_rtable_phys_entry_is_htable(rtable, new_phys_entry_va))
	{
		/*	This is very first entry in a hash bucket */
		new_phys_entry_va->next = 0U;
	}
	else
	{
		/*	Find last entry in the chain */
		last_phys_entry_va = &hash_table_va[hash];
		while (NULL != (void *)(addr_t)last_phys_entry_va->next)
		{
			last_phys_entry_va = pfe_rtable_phys_entry_get_va(rtable, (pfe_ct_rtable_entry_t *)(addr_t)oal_ntohl(last_phys_entry_va->next));
		}

		/*	Link last entry with the new one. Both are in network byte order. */

#if (TRUE == PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE)
		/*	Invalidate the last entry first */
		valid_tmp = last_phys_entry_va->flags;
		last_phys_entry_va->flags = RT_FL_NONE;

		/*	Wait some time due to sync with firmware */
		oal_time_usleep(10U);
#endif /* PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE */

		/*	Update the next pointer */
		last_phys_entry_va->next = oal_htonl((uint32_t)((addr_t)new_phys_entry_pa & 0xffffffffU));

#if (TRUE == PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE)
		/*	Ensure that all previous writes has been done */
		hal_wmb();

		/*	Re-enable the entry. Next (new last) entry remains invalid. */
		last_phys_entry_va->flags = valid_tmp;
#endif /* PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE */
	}

	/*	Copy temporary entry into its destination (pool/hash entry) */
	(void)memcpy(new_phys_entry_va, entry->temp_phys_entry, sizeof(pfe_ct_rtable_entry_t));

	/*	Remember the real pointer */
	entry->phys_entry = new_phys_entry_va;

	/*	Remember (physical) location of the new entry within the DDR. */
	entry->phys_entry->rt_orig = oal_htonl((uint32_t)((addr_t)new_phys_entry_pa));

	/*	Just invalidate the ingress interface here to not confuse the firmware code */
	entry->phys_entry->i_phy_if = PFE_PHY_IF_ID_INVALID;

	/*	Ensure that all previous writes has been done */
	hal_wmb();

	/*	Validate the new entry */
	entry->phys_entry->flags = (pfe_ct_rtable_flags_t)oal_htonl((uint32_t)RT_FL_VALID | (((uint8_t)IPV4 == entry->phys_entry->flag_ipv6) ? 0U : (uint32_t)RT_FL_IPV6));
	entry->prev = (NULL == last_phys_entry_va) ? NULL : pfe_rtable_get_by_phys_entry_va(rtable, last_phys_entry_va);
	entry->next = NULL;
	if (NULL != entry->prev)
	{
		/*	Store pointer to the new entry */
		entry->prev->next = entry;
	}

	LLIST_AddAtEnd(&entry->list_entry, &rtable->active_entries);

	NXP_LOG_INFO("RTable entry added, hash: 0x%x\n", (uint_t)hash);

	entry->rtable = rtable;

    if (0U == rtable->active_entries_count)
    {
        NXP_LOG_INFO("RTable first entry added, enable hardware RTable lookup\n");
        pfe_class_rtable_lookup_enable(rtable->class);
    }

    rtable->active_entries_count++;
    NXP_LOG_INFO("RTable active_entries_count: %u\n", (uint_t)(rtable->active_entries_count));

    if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex unlock failed\n");
    }

	return EOK;

free_and_fail:
    if (pfe_rtable_phys_entry_is_pool(rtable, new_phys_entry_va))
    {
        /*	Entry from the pool. Return it. */
        ret = fifo_put(rtable->pool_va, new_phys_entry_va);
        if (EOK != ret)
        {
            NXP_LOG_ERROR("Couldn't return routing table entry to the pool\n");
        }
    }

    if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex unlock failed\n");
    }

	return EFAULT;
}

/**
 * @brief		Delete an entry from the routing table
 * @param[in]	rtable The routing table instance
 * @param[in]	entry Entry to be deleted
 * @return		EOK if success, error code otherwise
 * @note		IPv4 addresses within entry are in network order due to way how the type is defined
 */
errno_t pfe_rtable_del_entry(pfe_rtable_t *rtable, pfe_rtable_entry_t *entry)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Protect table accesses */
	if (unlikely(EOK != oal_mutex_lock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex lock failed\n");
    }

	ret = pfe_rtable_del_entry_nolock(rtable, entry);

	if (0U == rtable->active_entries_count)
    {
        NXP_LOG_INFO("RTable last entry removed, disable hardware RTable lookup\n");
        pfe_class_rtable_lookup_disable(rtable->class);
	}

    if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex unlock failed\n");
    }

	return ret;
}

/**
 * @brief		Delete an entry from the routing table
 * @details		Internal function to delete an entry from the routing table without locking the table
 * @param[in]	rtable The routing table instance
 * @param[in]	entry Entry to be deleted (taken by get_first() or get_next() calls)
 * @return		EOK if success, error code otherwise
 * @note		IPv4 addresses within entry are in network order due to way how the type is defined
 */
static errno_t pfe_rtable_del_entry_nolock(pfe_rtable_t *rtable, pfe_rtable_entry_t *entry)
{
    pfe_ct_rtable_entry_t *next_phys_entry_pa = NULL;
    errno_t ret;
	pfe_ct_rtable_flags_t valid_tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (FALSE == pfe_rtable_entry_is_in_table(entry))
	{
		return EOK;
	}

	if (TRUE == pfe_rtable_phys_entry_is_htable(rtable, entry->phys_entry))
	{
		/*	Invalidate the found entry. This will disable the whole chain. */
		entry->phys_entry->flags = RT_FL_NONE;

		if (NULL != entry->next)
		{
#if (TRUE == PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE)
			/*	Invalidate also the next entry if any. This will prevent uncertainty
				during copying next entry to the place of the found one. */
			valid_tmp = entry->next->phys_entry->flags;
			entry->next->phys_entry->flags = RT_FL_NONE;

			/*	Ensure that all previous writes has been done */
			hal_wmb();

			/*	Wait some time due to sync with firmware */
			oal_time_usleep(10U);
#endif /* PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE */

			/*	Replace hash table entry with next (pool) entry */
			(void)memcpy(entry->phys_entry, entry->next->phys_entry, sizeof(pfe_ct_rtable_entry_t));

			/*	Clear the copied entry (next one) and return it back to the pool */
			(void)memset(entry->next->phys_entry, 0, sizeof(pfe_ct_rtable_entry_t));
			if (TRUE == pfe_rtable_phys_entry_is_pool(rtable, entry->next->phys_entry))
			{
                ret = fifo_put(rtable->pool_va, entry->next->phys_entry);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Couldn't return routing table entry to the pool\n");
				}
			}
			else
			{
				NXP_LOG_WARNING("Unexpected entry detected\n");
			}

			/*	Next entry now points to the copied physical one */
			entry->next->phys_entry = entry->phys_entry;
            next_phys_entry_pa = pfe_rtable_phys_entry_get_pa(rtable, entry->next->phys_entry);
			entry->next->phys_entry->rt_orig = oal_htonl((uint32_t)((addr_t)next_phys_entry_pa & 0xffffffffU));

			/*	Remove entry from the list of active entries and ensure consistency
			 	of get_first() and get_next() calls */
			if (&entry->list_entry == rtable->cur_item)
			{
				rtable->cur_item = entry->list_entry.prNext;
			}

			LLIST_Remove(&entry->list_entry);

#if (TRUE == PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE)
			/*	Validate the new entry */
			entry->next->phys_entry->flags = valid_tmp;
#endif /* PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE */

			/*	Set up links */
			if (NULL != entry->next)
			{
				entry->next->prev = entry->prev;
			}

			entry->prev = NULL;
			entry->next = NULL;
			entry->phys_entry = entry->temp_phys_entry;
		}
		else
		{
			/*	Ensure that all previous writes has been done */
			hal_wmb();

			/*	Wait some time due to sync with firmware */
			oal_time_usleep(10U);

			/*	Zero-out the entry */
			(void)memset(entry->phys_entry, 0, sizeof(pfe_ct_rtable_entry_t));

			/*	Remove entry from the list of active entries and ensure consistency
				of get_first() and get_next() calls */
			if (&entry->list_entry == rtable->cur_item)
			{
				rtable->cur_item = rtable->cur_item->prNext;
			}

			LLIST_Remove(&entry->list_entry);

			entry->prev = NULL;
			entry->next = NULL;
			entry->phys_entry = entry->temp_phys_entry;
		}
	}
	else if (TRUE == pfe_rtable_phys_entry_is_pool(rtable, entry->phys_entry))
	{
#if (TRUE == PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE)
		/*	Invalidate the previous entry */
		valid_tmp = entry->prev->phys_entry->flags;
		entry->prev->phys_entry->flags = RT_FL_NONE;

		/*	Invalidate the found entry */
		entry->phys_entry->flags = RT_FL_NONE;

		/*	Wait some time to sync with firmware */
		oal_time_usleep(10U);
#endif /* PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE */

		/*	Bypass the found entry */
		entry->prev->phys_entry->next = entry->phys_entry->next;

#if (TRUE == PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE)
		/*	Ensure that all previous writes has been done */
		hal_wmb();

		/*	Validate the previous entry */
		entry->prev->phys_entry->flags = valid_tmp;
#endif /* PFE_RTABLE_CFG_PARANOID_ENTRY_UPDATE */

		/*	Clear the found entry and return it back to the pool */
		(void)memset(entry->phys_entry, 0, sizeof(pfe_ct_rtable_entry_t));
        ret = fifo_put(rtable->pool_va, entry->phys_entry);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Couldn't return routing table entry to the pool\n");
		}

		/*	Remove entry from the list of active entries and ensure consistency
			of get_first() and get_next() calls */
		if (&entry->list_entry == rtable->cur_item)
		{
			rtable->cur_item = rtable->cur_item->prNext;
		}

		LLIST_Remove(&entry->list_entry);

		/*	Set up links */
		entry->prev->next = entry->next;
		if (NULL != entry->next)
		{
			entry->next->prev = entry->prev;
		}

		entry->prev = NULL;
		entry->next = NULL;
		entry->phys_entry = entry->temp_phys_entry;
	}
	else
	{
		NXP_LOG_ERROR("Wrong address (found rtable entry @ v0x%p)\n", (void *)entry->phys_entry);
	}

	entry->rtable = NULL;

	if (rtable->active_entries_count > 0U)
	{
		rtable->active_entries_count -= 1U;
		NXP_LOG_INFO("RTable active_entries_count: %u\n", (uint_t)(rtable->active_entries_count));
	}
	else
	{
		NXP_LOG_WARNING("RTable removing active entry while active_entries_count is already = 0 (expected value > 0)\n");
	}

	return EOK;
}

/**
 * @brief		Scan the table and update timeouts
 * @param[in]	rtable The routing table instance
 * @note		Runs within the rtable worker thread context
 */
void pfe_rtable_do_timeouts(pfe_rtable_t *rtable)
{
	LLIST_t *item;
	LLIST_t to_be_removed_list;
	pfe_rtable_entry_t *entry;
	uint8_t flags;
	errno_t err;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == rtable))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (unlikely(EOK != oal_mutex_lock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex lock failed\n");
    }

	LLIST_Init(&to_be_removed_list);

	/*	Go through all active entries */
	LLIST_ForEach(item, &rtable->active_entries)
	{
		entry = LLIST_Data(item, pfe_rtable_entry_t, list_entry);
		flags = (uint8_t)entry->phys_entry->status;

		if (0xffffffffU == entry->timeout)
		{
			continue;
		}

		if (0U != ((uint8_t)RT_STATUS_ACTIVE & flags))
		{
			/*	Entry is active. Reset timeout and the active flag. */
			entry->curr_timeout = entry->timeout;
			entry->phys_entry->status &= ~(uint8_t)RT_STATUS_ACTIVE;
		}
		else
		{
			if (entry->curr_timeout >= PFE_RTABLE_CFG_TICK_PERIOD_SEC)
			{
				entry->curr_timeout -= PFE_RTABLE_CFG_TICK_PERIOD_SEC;
			}
			else
			{
				entry->curr_timeout = 0;
			}

			/*	Entry is not active */
			if (0U == entry->curr_timeout)
			{
				/*	Call user's callback if requested */
				if (NULL != entry->callback)
				{
					entry->callback(entry->callback_arg, RTABLE_ENTRY_TIMEOUT);
				}

				/*	Collect entries to be removed */
				LLIST_AddAtEnd(&entry->list_to_remove_entry, &to_be_removed_list);
			}
		}
	}

	LLIST_ForEach(item, &to_be_removed_list)
	{
		entry = LLIST_Data(item, pfe_rtable_entry_t, list_to_remove_entry);

		/*	Physically remove the entry from table */
		err = pfe_rtable_del_entry_nolock(rtable, entry);
		if (EOK != err)
		{
			NXP_LOG_ERROR("Couldn't delete timed-out entry: %d\n", err);
		}
	}

    if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
    {
        NXP_LOG_DEBUG("Mutex unlock failed\n");
    }

	return;
}

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR)
/**
 * @brief		Worker function running within internal thread
 */
static void *rtable_worker_func(void *arg)
{
	pfe_rtable_t *rtable = (pfe_rtable_t *)arg;
	errno_t err;
	oal_mbox_msg_t msg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == rtable))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	while (2)
	{
		err = oal_mbox_receive(rtable->mbox, &msg);
		if (EOK != err)
		{
			NXP_LOG_WARNING("mbox: Problem receiving message: %d", err);
		}
		else
		{
			switch (msg.payload.code)
			{
				case (int32_t)SIG_WORKER_STOP:
				{
                    /* Exit the thread */
                    oal_mbox_ack_msg(&msg);
                    return NULL;
				}

				case (int32_t)SIG_TIMER_TICK:
				{
					pfe_rtable_do_timeouts(rtable);
					break;
				}

				default:
				{
                    /*Do Nothing*/
                    break;
				}
			}
		}

		oal_mbox_ack_msg(&msg);
	}

	return NULL;
}
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) */

/**
 * @brief		Create routing table instance
 * @details		Creates and initializes routing table at given memory location.
 * @param[in]	class The classifier instance implementing the routing
 * @param[in]	htable_base_va Virtual address where the hash table shall be placed
 * @param[in]	htable_size Number of entries within the hash table
 * @param[in]	pool_base_va Virtual address where pool shall be placed
 * @param[in]	pool_size Number of entries within the pool
 * @return		The routing table instance or NULL if failed
 */
pfe_rtable_t *pfe_rtable_create(pfe_class_t *class, addr_t htable_base_va, uint32_t htable_size, addr_t pool_base_va, uint32_t pool_size, pfe_l2br_t *bridge)
{
	pfe_rtable_t *rtable;
	pfe_ct_rtable_entry_t *table_va;
	uint32_t ii;
    errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL_ADDR == htable_base_va) || (NULL_ADDR == pool_base_va) || (NULL == class)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	rtable = oal_mm_malloc(sizeof(pfe_rtable_t));

	if (NULL == rtable)
	{
		return NULL;
	}
	else
	{
		/*	Initialize the instance */
		(void)memset(rtable, 0, sizeof(pfe_rtable_t));

		/*	Create mutex */
		rtable->lock = (oal_mutex_t *)oal_mm_malloc(sizeof(oal_mutex_t));

		if (NULL == rtable->lock)
		{
			NXP_LOG_ERROR("Couldn't allocate mutex object\n");
			goto free_and_fail;
		}
		else
		{
			if (EOK == oal_mutex_init(rtable->lock))
            {
                /*	Store properties */
                rtable->htable_base_va = htable_base_va;
                rtable->htable_base_pa = (addr_t)oal_mm_virt_to_phys_contig((void *)htable_base_va);
                rtable->htable_size = htable_size;
                rtable->htable_end_va = rtable->htable_base_va + (rtable->htable_size * sizeof(pfe_ct_rtable_entry_t)) - 1U;
                rtable->htable_end_pa = rtable->htable_base_pa + (rtable->htable_size * sizeof(pfe_ct_rtable_entry_t)) - 1U;

                rtable->pool_base_va = pool_base_va;
                rtable->pool_base_pa = rtable->htable_base_pa + (pool_base_va - htable_base_va);
                rtable->pool_size = pool_size;
                rtable->pool_end_va = rtable->pool_base_va + (rtable->pool_size * sizeof(pfe_ct_rtable_entry_t)) - 1U;
                rtable->pool_end_pa = rtable->pool_base_pa + (rtable->pool_size * sizeof(pfe_ct_rtable_entry_t)) - 1U;
                rtable->bridge = bridge;
                rtable->class = class;
                rtable->active_entries_count = 0;

                if ((NULL_ADDR == rtable->htable_base_va) || (NULL_ADDR == rtable->pool_base_va))
                {
                    NXP_LOG_ERROR("Can't map the table memory\n");
                    goto free_and_fail;
                }
                else
                {
                    /*	Pre-compute conversion offsets */
                    rtable->htable_va_pa_offset = rtable->htable_base_va - rtable->htable_base_pa;
                    rtable->pool_va_pa_offset = rtable->pool_base_va - rtable->pool_base_pa;
                }

                /*	Configure the classifier */
                if (EOK != pfe_class_set_rtable(class, rtable->htable_base_pa, rtable->htable_size, sizeof(pfe_ct_rtable_entry_t)))
                {
                    NXP_LOG_ERROR("Unable to set routing table address\n");
                    goto free_and_fail;
                }

                /*	Initialize the table */
                pfe_rtable_invalidate(rtable);

                /*	Create pool. No protection needed. */
                rtable->pool_va = fifo_create(rtable->pool_size);

                if (NULL == rtable->pool_va)
                {
                    NXP_LOG_ERROR("Can't create pool\n");
                    goto free_and_fail;
                }

                /*	Fill the pool */
                table_va = (pfe_ct_rtable_entry_t *)rtable->pool_base_va;

                for (ii=0U; ii<rtable->pool_size; ii++)
                {
                    ret = fifo_put(rtable->pool_va, (void *)&table_va[ii]);
                    if (EOK != ret)
                    {
                        NXP_LOG_ERROR("Pool filling failed (VA pool)\n");
                        goto free_and_fail;
                    }
                }

                /*	Create list */
                LLIST_Init(&rtable->active_entries);

        #if !defined(PFE_CFG_TARGET_OS_AUTOSAR)
                /*	Create mbox */
                rtable->mbox = oal_mbox_create();
                if (NULL == rtable->mbox)
                {
                    NXP_LOG_ERROR("Mbox creation failed\n");
                    goto free_and_fail;
                }

                /*	Create worker thread */
                rtable->worker = oal_thread_create(&rtable_worker_func, rtable, "rtable worker", 0);
                if (NULL == rtable->worker)
                {
                    NXP_LOG_ERROR("Couldn't start worker thread\n");
                    goto free_and_fail;
                }
                else
                {
                    if (EOK != oal_mbox_attach_timer(rtable->mbox, (uint32_t)PFE_RTABLE_CFG_TICK_PERIOD_SEC * 1000U, SIG_TIMER_TICK))
                    {
                        NXP_LOG_ERROR("Unable to attach timer\n");
                        goto free_and_fail;
                    }
                }
        #endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) */
            }
        }
	}

	return rtable;

free_and_fail:

	pfe_rtable_destroy(rtable);
	return NULL;
}

/**
* @brief		Returns total count of entries within the table
* @param[in]	rtable The routing table instance
* @return		Total count of entries within the table
*/
uint32_t pfe_rtable_get_size(const pfe_rtable_t *rtable)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == rtable))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	return rtable->pool_size + rtable->htable_size;
}

/**
 * @brief		Destroy routing table instance
 * @param[in]	rtable The routing table instance
 */
void pfe_rtable_destroy(pfe_rtable_t *rtable)
{
#if !defined(PFE_CFG_TARGET_OS_AUTOSAR)
	errno_t err;
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) */

	if (NULL != rtable)
	{
#if !defined(PFE_CFG_TARGET_OS_AUTOSAR)
		if (NULL != rtable->mbox)
		{
			oal_mbox_detach_timer(rtable->mbox);

			if (NULL != rtable->worker)
			{
				NXP_LOG_INFO("Stopping rtable worker...\n");

				err = oal_mbox_send_signal(rtable->mbox, SIG_WORKER_STOP);
				if (EOK != err)
				{
					NXP_LOG_ERROR("Signal failed: %d\n", err);
				}
				else
				{
					err = oal_thread_join(rtable->worker, NULL);
					if (EOK != err)
					{
						NXP_LOG_ERROR("Can't join the worker thread: %d\n", err);
					}
					else
					{
						NXP_LOG_INFO("rtable worker stopped\n");
					}
				}
			}
		}

		if (NULL != rtable->mbox)
		{
			oal_mbox_destroy(rtable->mbox);
			rtable->mbox = NULL;
		}
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) */

		if (NULL_ADDR != rtable->htable_base_va)
		{
			/*	Just forget the address */
			rtable->htable_base_va = NULL_ADDR;
		}

		if (NULL_ADDR != rtable->pool_base_va)
		{
			/*	Just forget the address */
			rtable->pool_base_va = NULL_ADDR;
		}

		if (NULL != rtable->pool_va)
		{
			fifo_destroy(rtable->pool_va);
			rtable->pool_va = NULL;
		}

		if (NULL != rtable->lock)
		{
			if (EOK != oal_mutex_destroy(rtable->lock))
            {
                NXP_LOG_ERROR("Failed to destroy rtable\n");
            }
			oal_mm_free(rtable->lock);
			rtable->lock = NULL;
		}

		oal_mm_free(rtable);
	}
}

/**
 * @brief		Get size of routing table entry
 * @return		Size of entry in number of bytes
 */
uint32_t pfe_rtable_get_entry_size(void)
{
	return (uint32_t)sizeof(pfe_ct_rtable_entry_t);
}

/**
 * @brief		Convert entry into 5-tuple representation
 * @param[in]	entry The entry to be converted
 * @param[out]	tuple Pointer where the 5-tuple will be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_rtable_entry_to_5t(const pfe_rtable_entry_t *entry, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Clean the destination */
	(void)memset(tuple, 0, sizeof(pfe_5_tuple_t));

	if ((uint8_t)IPV4 == entry->phys_entry->flag_ipv6)
	{
		/*	SRC + DST IP */
		(void)memcpy(&tuple->src_ip.v4, &entry->phys_entry->ipv.v4.sip, 4);
		(void)memcpy(&tuple->dst_ip.v4, &entry->phys_entry->ipv.v4.dip, 4);
		tuple->src_ip.is_ipv4 = TRUE;
		tuple->dst_ip.is_ipv4 = TRUE;
	}
	else if ((uint8_t)IPV6 == entry->phys_entry->flag_ipv6)
	{
		/*	SRC + DST IP */
		(void)memcpy(&tuple->src_ip.v6, &entry->phys_entry->ipv.v6.sip[0], 16);
		(void)memcpy(&tuple->dst_ip.v6, &entry->phys_entry->ipv.v6.dip[0], 16);
		tuple->src_ip.is_ipv4 = FALSE;
		tuple->dst_ip.is_ipv4 = FALSE;
	}
	else
	{
		NXP_LOG_ERROR("Unknown IP version\n");
		return EINVAL;
	}

	tuple->sport = oal_ntohs(entry->phys_entry->sport);
	tuple->dport = oal_ntohs(entry->phys_entry->dport);
	tuple->proto = entry->phys_entry->proto;

	return EOK;
}

/**
 * @brief		Convert entry into 5-tuple representation (output values)
 * @details		Returns entry values as it will behave after header fields
 * 				are changed. See pfe_rtable_entry_set_out_xxx().
 * @param[in]	entry The entry to be converted
 * @param[out]	tuple Pointer where the 5-tuple will be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_rtable_entry_to_5t_out(const pfe_rtable_entry_t *entry, pfe_5_tuple_t *tuple)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == tuple)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Clean the destination */
	(void)memset(tuple, 0, sizeof(pfe_5_tuple_t));

	if ((uint8_t)IPV6 == entry->phys_entry->flag_ipv6)
	{
		/*	SRC + DST IP */
		(void)memcpy(&tuple->src_ip.v6, &entry->phys_entry->args.ipv.v6.sip[0], 16);
		(void)memcpy(&tuple->dst_ip.v6, &entry->phys_entry->args.ipv.v6.dip[0], 16);
		tuple->src_ip.is_ipv4 = FALSE;
		tuple->dst_ip.is_ipv4 = FALSE;
	}
	else
	{
		/*	SRC + DST IP */
		(void)memcpy(&tuple->src_ip.v4, &entry->phys_entry->args.ipv.v4.sip, 4);
		(void)memcpy(&tuple->dst_ip.v4, &entry->phys_entry->args.ipv.v4.dip, 4);
		tuple->src_ip.is_ipv4 = TRUE;
		tuple->dst_ip.is_ipv4 = TRUE;
	}

	tuple->sport = oal_ntohs(entry->phys_entry->args.sport);
	tuple->dport = oal_ntohs(entry->phys_entry->args.dport);
	tuple->proto = entry->phys_entry->proto;

	return EOK;
}

/**
 * @brief		Match entry with latest criterion provided via pfe_rtable_get_first()
 * @param[in]	crit Select criterion
 * @param[in]	arg Criterion argument
 * @param[in]	entry The entry to be matched
 * @retval		TRUE Entry matches the criterion
 * @retval		FALSE Entry does not match the criterion
 */
static bool_t pfe_rtable_match_criterion(pfe_rtable_get_criterion_t crit, const pfe_rtable_criterion_arg_t *arg, pfe_rtable_entry_t *entry)
{
	bool_t match = FALSE;
	pfe_5_tuple_t five_tuple;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == arg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	switch (crit)
	{
		case RTABLE_CRIT_ALL:
			match = TRUE;
			break;

		case RTABLE_CRIT_ALL_IPV4:
			match = ((uint8_t)IPV4 == entry->phys_entry->flag_ipv6);
			break;

		case RTABLE_CRIT_ALL_IPV6:
			match = ((uint8_t)IPV6 == entry->phys_entry->flag_ipv6);
			break;

		case RTABLE_CRIT_BY_DST_IF:
			match = (pfe_phy_if_get_id(arg->iface) == (pfe_ct_phy_if_id_t)entry->phys_entry->e_phy_if);
			break;

		case RTABLE_CRIT_BY_ROUTE_ID:
			match = (TRUE == entry->route_id_valid) && (arg->route_id == entry->route_id);
			break;

		case RTABLE_CRIT_BY_ID5T:
			match = (arg->id5t == entry->phys_entry->id5t);
			break;

		case RTABLE_CRIT_BY_5_TUPLE:
			if (EOK != pfe_rtable_entry_to_5t(entry, &five_tuple))
			{
				NXP_LOG_ERROR("Entry conversion failed\n");
				match = FALSE;
			}
			else
			{
				match = (0 == memcmp(&five_tuple, &arg->five_tuple, sizeof(pfe_5_tuple_t)));
			}
			break;

		default:
			NXP_LOG_ERROR("Unknown criterion\n");
			match = FALSE;
            break;
	}

	return match;
}

/**
 * @brief		Get first record from the table matching given criterion
 * @details		Intended to be used with pfe_rtable_get_next
 * @param[in]	rtable The routing table instance
 * @param[in]	crit Get criterion
 * @param[in]	art Pointer to criterion argument. Every value shall to be in HOST endian format.
 * @return		The entry or NULL if not found
 * @warning		The routing table must be locked for the time the function and its returned entry
 * 				is being used since the entry might become asynchronously invalid (timed-out).
 */
pfe_rtable_entry_t *pfe_rtable_get_first(pfe_rtable_t *rtable, pfe_rtable_get_criterion_t crit, void *arg)
{
	LLIST_t *item;
	pfe_rtable_entry_t *entry = NULL;
	bool_t match = FALSE;
    bool_t known_crit = TRUE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == rtable) || (NULL == arg)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Remember criterion and argument for possible subsequent pfe_rtable_get_next() calls */
	rtable->cur_crit = crit;
	switch (rtable->cur_crit)
	{
		case RTABLE_CRIT_ALL:
		case RTABLE_CRIT_ALL_IPV4:
		case RTABLE_CRIT_ALL_IPV6:
			break;

		case RTABLE_CRIT_BY_DST_IF:
			rtable->cur_crit_arg.iface = (pfe_phy_if_t *)arg;
			break;

		case RTABLE_CRIT_BY_ROUTE_ID:
			(void)memcpy(&rtable->cur_crit_arg.route_id, arg, sizeof(uint32_t));
			break;

		case RTABLE_CRIT_BY_ID5T:
			(void)memcpy(&rtable->cur_crit_arg.id5t, arg, sizeof(uint32_t));
			break;

		case RTABLE_CRIT_BY_5_TUPLE:
			(void)memcpy(&rtable->cur_crit_arg.five_tuple, arg, sizeof(pfe_5_tuple_t));
			break;

		default:
		{
			NXP_LOG_ERROR("Unknown criterion\n");
            known_crit = FALSE;
            break;
		}
	}

	/*	Search for first matching entry */
	if ((FALSE == LLIST_IsEmpty(&rtable->active_entries)) && (TRUE == known_crit))
	{
		/*	Protect table accesses */
		if (unlikely(EOK != oal_mutex_lock(rtable->lock)))
        {
            NXP_LOG_DEBUG("Mutex lock failed\n");
        };

		/*	Get first matching entry */
		LLIST_ForEach(item, &rtable->active_entries)
		{
			/*	Get data */
			entry = LLIST_Data(item, pfe_rtable_entry_t, list_entry);

			/*	Remember current item to know where to start later */
			rtable->cur_item = item->prNext;
			if (NULL != entry)
			{
				if (TRUE == pfe_rtable_match_criterion(rtable->cur_crit, &rtable->cur_crit_arg, entry))
				{
					match = TRUE;
					break;
				}
			}
		}

        if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
        {
            NXP_LOG_DEBUG("Mutex unlock failed\n");
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
 * @brief		Get next record from the table
 * @details		Intended to be used with pfe_rtable_get_first.
 * @param[in]	rtable The routing table instance
 * @return		The entry or NULL if not found
 * @warning		The routing table must be locked for the time the function and its returned entry
 * 				is being used since the entry might become asynchronously invalid (timed-out).
 */
pfe_rtable_entry_t *pfe_rtable_get_next(pfe_rtable_t *rtable)
{
	pfe_rtable_entry_t *entry;
	bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == rtable))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (rtable->cur_item == &rtable->active_entries)
	{
		/*	No more entries */
		entry = NULL;
	}
	else
	{
		/*	Protect table accesses */
        if (unlikely(EOK != oal_mutex_lock(rtable->lock)))
        {
            NXP_LOG_DEBUG("Mutex lock failed\n");
        }

		while (rtable->cur_item != &rtable->active_entries)
		{
			/*	Get data */
			entry = LLIST_Data(rtable->cur_item, pfe_rtable_entry_t, list_entry);

			/*	Remember current item to know where to start later */
			rtable->cur_item = rtable->cur_item->prNext;

			if (NULL != entry)
			{
				if (TRUE == pfe_rtable_match_criterion(rtable->cur_crit, &rtable->cur_crit_arg, entry))
				{
					match = TRUE;
					break;
				}
			}
		}

        if (unlikely(EOK != oal_mutex_unlock(rtable->lock)))
        {
            NXP_LOG_DEBUG("Mutex unlock failed\n");
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
