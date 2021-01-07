/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @file		pfe_l2br.c
 * @brief		The L2 bridge module source file.
 * @details		This file contains L2 bridge-related functionality.
 *
 * 				The bridge consists of multiple bridge domains:
 * 				1.) The default domain
 * 				2.) Set of particular standard VLAN domains
 * 				3.) The fall-back domain
 *
 * 				The default domain
 * 				------------------
 * 				Default domain is used by the classification process when packet
 * 				without VLAN tag has been received and hardware assigned a default
 * 				VLAN ID.
 *
 * 				The standard domain
 * 				-------------------
 * 				Standard VLAN domain. Specifies what to do when packet with VLAN
 *				ID matching the domain is received.
 *
 * 				The fall-back domain
 * 				-------------------
 * 				This domain is used when packet with unknown VLAN ID (does not match
 * 				any standard domain) is received.
 *
 */

#include "pfe_cfg.h"
#include "linked_list.h"

#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_l2br_table.h"
#include "pfe_l2br.h"

/**
 * @brief	Tick period for internal timer in seconds
 * @details	The timer is used to generate MAC table aging period.
 */
#define PFE_L2BR_CFG_TICK_PERIOD_SEC	300U

/**
 * @brief	The L2 Bridge instance structure
 */
struct __pfe_l2br_tag
{
	pfe_class_t *class;
	pfe_l2br_table_t *mac_table;
	pfe_l2br_table_t *vlan_table;
	pfe_l2br_domain_t *default_domain;
	pfe_l2br_domain_t *fallback_domain;
	LLIST_t domains;							/*!< List of standard domains */
	uint16_t def_vlan;							/*!< Default VLAN */
	uint32_t dmem_fb_bd_base;					/*!< Address within classifier memory where the fall-back bridge domain structure is located */
	uint32_t dmem_def_bd_base;					/*!< Address within classifier memory where the default bridge domain structure is located */
	oal_thread_t *worker;						/*!< Worker thread */
	oal_mbox_t *mbox;							/*!< Message box to communicate with the worker thread */
	oal_mutex_t *mutex;							/*!< Mutex protecting shared resources */
	pfe_l2br_domain_get_crit_t cur_crit;		/*!< Current 'get' criterion (to get domains) */
	LLIST_t *cur_item;							/*!< The current domain list item */
	union
	{
		uint16_t vlan;
		pfe_phy_if_t *phy_if;
	} cur_crit_arg;								/*!< Current criterion argument */
};

/**
 * @brief	The L2 Bridge Domain representation type
 */
struct __pfe_l2br_domain_tag
{
	uint16_t vlan;
	union
	{
		pfe_ct_vlan_table_result_t action_data;
		uint64_t action_data_u64val;
	};

	pfe_l2br_table_entry_t *vlan_entry;			/*!< This is entry within VLAN table representing the domain */
	pfe_l2br_t *bridge;
	bool_t is_default;							/*!< If TRUE then this is default bridge domain */
	bool_t is_fallback;							/*!< If TRUE the this is fall-back bridge domain */
	oal_mutex_t *mutex;							/*!< Mutex protecting shared resources */
	pfe_l2br_domain_if_get_crit_t cur_crit;		/*!< Current 'get' criterion (to get interfaces) */
	LLIST_t *cur_item;							/*!< The current interface list item */
	union /* Placeholder union for future use */
	{
		pfe_ct_phy_if_id_t id;
		pfe_phy_if_t *phy_if;
	} cur_crit_arg;								/*!< Current criterion argument */
	LLIST_t ifaces;								/*!< List of associated interfaces */
	LLIST_t list_entry;							/*!< Entry for linked-list purposes */
};

/**
 * @brief	Internal linked-list element
 */
typedef struct __pfe_l2br_list_entry_tag
{
	void *ptr;
	LLIST_t list_entry;
} pfe_l2br_list_entry_t;

/**
 * @brief	Worker thread signals
 * @details	Driver is sending signals to the worker thread to request specific
 * 			operations.
 */
enum pfe_rtable_worker_signals
{
	SIG_WORKER_STOP,	/*!< Stop the thread */
	SIG_TIMER_TICK		/*!< Pulse from timer */
};

static errno_t pfe_bd_write_to_class(pfe_l2br_t *bridge, uint32_t base, pfe_ct_bd_entry_t *class_entry);
static errno_t pfe_l2br_update_hw_entry(pfe_l2br_domain_t *domain);
static pfe_l2br_domain_t *pfe_l2br_create_default_domain(pfe_l2br_t *bridge, uint16_t vlan);
static pfe_l2br_domain_t *pfe_l2br_create_fallback_domain(pfe_l2br_t *bridge);
static void *pfe_l2br_worker_func(void *arg);
static void pfe_l2br_do_timeouts(pfe_l2br_t *bridge);
static bool_t pfe_l2br_domain_match_if_criterion(pfe_l2br_domain_t *domain, pfe_phy_if_t *iface);
static bool_t pfe_l2br_domain_match_criterion(pfe_l2br_t *bridge, pfe_l2br_domain_t *domain);

/**
 * @brief		Write bridge domain structure to classifier memory
 * @param[in]	bridge The bridge instance
 * @param[in]	base Memory location where to write
 * @param[in]	class_entry Pointer to the structure to be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
static errno_t pfe_bd_write_to_class(pfe_l2br_t *bridge, uint32_t base, pfe_ct_bd_entry_t *class_entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_entry) || (NULL == bridge) || (0U == base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_class_write_dmem(bridge->class, -1, (void *)(addr_t)base, class_entry, sizeof(pfe_ct_bd_entry_t));
}


static void pfe_l2br_update_hw_ll_entry(pfe_l2br_domain_t *domain, uint32_t base)
{
	pfe_ct_bd_entry_t sw_bd;
	uint64_t tmp64;
	bool_t need_shift = FALSE;

	/*	Sanity check */
	ct_assert(sizeof(pfe_ct_bd_entry_t) <= sizeof(uint64_t));

	/*	Check if bitfields within structure are aligned as expected */
	memset(&sw_bd, 0, sizeof(pfe_ct_bd_entry_t));
	tmp64 = (1ULL << 63);
	memcpy(&sw_bd, &tmp64, sizeof(uint64_t));
	if (0U == sw_bd.val)
	{
		need_shift = TRUE;
	}

	/*	Convert VLAN table result to fallback domain representation */
	sw_bd.val = domain->action_data.val;

	if (TRUE == need_shift)
	{
		tmp64 = (uint64_t)sw_bd.val << 9;
		memcpy(&sw_bd, &tmp64, sizeof(pfe_ct_bd_entry_t));
	}

	/*	Convert to network endian */
	/* TODO: oal_swap64 or so */
#if defined(PFE_CFG_TARGET_OS_QNX)
	ENDIAN_SWAP64(&sw_bd.val);
#elif defined(PFE_CFG_TARGET_OS_LINUX)
	tmp64 = cpu_to_be64p((uint64_t *)&sw_bd);
	memcpy(&sw_bd, &tmp64, sizeof(uint64_t));
#elif defined(PFE_CFG_TARGET_OS_AUTOSAR)
	*(uint64_t*)&sw_bd = cpu_to_be64(*(uint64_t*)&sw_bd);
#else
#error("todo")
#endif

	/*	Update classifier memory (all PEs) */
	if (EOK != pfe_bd_write_to_class(domain->bridge, base, &sw_bd))
	{
		NXP_LOG_DEBUG("Class memory write failed\n");
	}

}


/**
 * @brief		Update HW entry according to domain setup
 * @details		Function is intended to propagate domain configuration from host SW instance
 * 				form to PFE HW/FW representation.
 * @param[in]	domain The domain instance
 * @return		EOK or error code in case of failure
 */
static errno_t pfe_l2br_update_hw_entry(pfe_l2br_domain_t *domain)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	In case of fall-back domain the classifier memory must be updated too */
	if (TRUE == domain->is_fallback)
	{
		/*	Update classifier memory (all PEs) */
		pfe_l2br_update_hw_ll_entry(domain, domain->bridge->dmem_fb_bd_base );
	}
	else
	{
        /*	In case of fall-back domain the classifier memory must be updated too */
        if (TRUE == domain->is_default)
        {
            /*	Update classifier memory (all PEs) */
            pfe_l2br_update_hw_ll_entry(domain, domain->bridge->dmem_def_bd_base );
        }
		/*	Update standard or default domain entry */
		ret = pfe_l2br_table_entry_set_action_data(domain->vlan_entry, domain->action_data_u64val);
		if (EOK != ret)
		{
			NXP_LOG_DEBUG("Can't set action data: %d\n", ret);
			return ENOEXEC;
		}

		/*	Propagate change to HW table */
		ret = pfe_l2br_table_update_entry(domain->bridge->vlan_table, domain->vlan_entry);
		if (EOK != ret)
		{
			NXP_LOG_DEBUG("Can't update VLAN table entry: %d\n", ret);
			return ENOEXEC;
		}
	}

	return EOK;
}

/**
 * @brief		Create L2 bridge domain instance
 * @details		By default, new domain is configured to drop all matching packets. Use the
 *				pfe_l2br_domain_set_[ucast/mcast]_action() to finish the configuration. The
 *				instance is automatically bound to the bridge and can be retrieved by
 *				the pfe_l2br_get_first_domain()/pfe_l2br_get_next_domain() calls.
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 * @retval		ETIMEDOUT Timed out
 * @retval		EPERM Operation not permitted (domain already created)
 */
errno_t pfe_l2br_domain_create(pfe_l2br_t *bridge, uint16_t vlan)
{
	pfe_l2br_domain_t *domain;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	domain = oal_mm_malloc(sizeof(pfe_l2br_domain_t));

	if (NULL == domain)
	{
		NXP_LOG_DEBUG("malloc() failed\n");
	}
	else
	{
		memset(domain, 0, sizeof(pfe_l2br_domain_t));
		domain->bridge = bridge;
		domain->vlan = vlan;
		domain->is_default = FALSE;
		domain->list_entry.prNext = NULL;
		domain->list_entry.prPrev = NULL;
		LLIST_Init(&domain->ifaces);

		domain->mutex = oal_mm_malloc(sizeof(oal_mutex_t));
		if (NULL == domain->mutex)
		{
			NXP_LOG_DEBUG("Memory allocation failed\n");
			ret = ENOMEM;
			goto free_and_fail;
		}

		ret = oal_mutex_init(domain->mutex);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Could not initialize mutex\n");
			oal_mm_free(domain->mutex);
			domain->mutex = NULL;
			goto free_and_fail;
		}

		/*	Check if the domain is not duplicate */
		if (NULL != pfe_l2br_get_first_domain(bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)vlan))
		{
			NXP_LOG_ERROR("Domain with vlan %d does already exist\n", domain->vlan);
			ret = EPERM;
			goto free_and_fail;
		}
		else
		{
			/*	Prepare VLAN table entry. At the beginning the bridge entry does not contain
				any ports. */
			domain->vlan_entry = pfe_l2br_table_entry_create(bridge->vlan_table);
			if (NULL == domain->vlan_entry)
			{
				NXP_LOG_DEBUG("Can't create vlan table entry\n");
				ret = ENOEXEC;
				goto free_and_fail;
			}

			/*	Set VLAN */
			ret = pfe_l2br_table_entry_set_vlan(domain->vlan_entry, domain->vlan);
			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Can't set vlan: %d\n", ret);
				goto free_and_fail;
			}

			domain->action_data.forward_list = 0U;
			domain->action_data.untag_list = 0U;
			domain->action_data.ucast_hit_action = L2BR_ACT_DISCARD;
			domain->action_data.ucast_miss_action = L2BR_ACT_DISCARD;
			domain->action_data.mcast_hit_action = L2BR_ACT_DISCARD;
			domain->action_data.mcast_miss_action = L2BR_ACT_DISCARD;

			/*	Set action data */
			ret = pfe_l2br_table_entry_set_action_data(domain->vlan_entry, domain->action_data_u64val);
			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Can't set action data: %d\n", ret);
				goto free_and_fail;
			}

			/*	Add new VLAN table entry */
			ret = pfe_l2br_table_add_entry(domain->bridge->vlan_table, domain->vlan_entry);
			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Could not add VLAN table entry: %d\n", ret);
				goto free_and_fail;
			}

			/*	Remember the domain instance in global list */
			if (EOK != oal_mutex_lock(bridge->mutex))
			{
				NXP_LOG_DEBUG("Mutex lock failed\n");
			}

			LLIST_AddAtEnd(&domain->list_entry, &bridge->domains);

			if (EOK != oal_mutex_unlock(bridge->mutex))
			{
				NXP_LOG_DEBUG("Mutex unlock failed\n");
			}
		}
	}

	return EOK;

free_and_fail:
	if (EOK != pfe_l2br_domain_destroy(domain))
	{
		NXP_LOG_ERROR("Unable to destroy bridge domain\n");
	}

	return ret;
}

/**
 * @brief		Destroy L2 bridge domain instance
 * @param[in]	bridge The bridge domain instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_l2br_domain_destroy(pfe_l2br_domain_t *domain)
{
	errno_t ret;
	LLIST_t *aux, *item;
	pfe_l2br_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Remove all associated interfaces */
	if (FALSE == LLIST_IsEmpty(&domain->ifaces))
	{
		NXP_LOG_INFO("Non-empty bridge domain is being destroyed\n");
		LLIST_ForEachRemovable(item, aux, &domain->ifaces)
		{
			entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
			oal_mm_free(entry);
			entry = NULL;
		}
	}

	if (NULL != domain->vlan_entry)
	{
		/*	Remove entry from the table */
		ret = pfe_l2br_table_del_entry(domain->bridge->vlan_table, domain->vlan_entry);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't delete entry from VLAN table: %d\n", ret);
			return ENOEXEC;
		}

		/*	Release the table entry instance */
		pfe_l2br_table_entry_destroy(domain->vlan_entry);
		domain->vlan_entry = NULL;
	}

	if (TRUE == domain->is_fallback)
	{
		/*	Disable the fall-back domain traffic */
		pfe_l2br_domain_set_ucast_action(domain, L2BR_ACT_DISCARD, L2BR_ACT_DISCARD);
		pfe_l2br_domain_set_mcast_action(domain, L2BR_ACT_DISCARD, L2BR_ACT_DISCARD);
	}

	/*	Remove the domain instance from global list if it has been added there */
	if (EOK != oal_mutex_lock(domain->bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	/*	If instance has not been added to the list of domains yet (mainly due to
		pfe_l2br_domain_create() failure, just skip this step */
	if ((NULL != domain->list_entry.prPrev) && (NULL != domain->list_entry.prNext))
	{
		if (&domain->list_entry == domain->bridge->cur_item)
		{
			/*	Remember the change so we can call destroy() between get_first()
				and get_next() calls. */
			domain->bridge->cur_item = domain->bridge->cur_item->prNext;
		}

		LLIST_Remove(&domain->list_entry);
	}

	if (EOK != oal_mutex_unlock(domain->bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	if (NULL != domain->mutex)
	{
		(void)oal_mutex_destroy(domain->mutex);
		oal_mm_free(domain->mutex);
		domain->mutex = NULL;
	}

	oal_mm_free(domain);

	return EOK;
}

/**
 * @brief		Create default L2 bridge domain instance
 * @details		Create default bridge domain (empty, no interface assigned)
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @return		The instance or NULL if failed
 */
static pfe_l2br_domain_t *pfe_l2br_create_default_domain(pfe_l2br_t *bridge, uint16_t vlan)
{
	errno_t ret;
	pfe_l2br_domain_t *domain = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = pfe_l2br_domain_create(bridge, vlan);

	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Argument is NULL\n");
		return NULL;
	}
	else
	{
		domain = pfe_l2br_get_first_domain(bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)vlan);
		if (NULL == domain)
		{
			NXP_LOG_ERROR("Default domain not found\n");
		}
		else
		{
			domain->is_default = TRUE;
		}
	}

	return domain;
}

/**
 * @brief		Create fall-back L2 bridge domain instance
 * @details		Create fall-back bridge domain (empty, no interface assigned)
 * @param[in]	bridge The L2 bridge instance
 * @return		The instance or NULL if failed
 */
static pfe_l2br_domain_t *pfe_l2br_create_fallback_domain(pfe_l2br_t *bridge)
{
	pfe_ct_class_mmap_t class_mmap;
	pfe_l2br_domain_t *domain;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	domain = oal_mm_malloc(sizeof(pfe_l2br_domain_t));
	if (NULL == domain)
	{
		NXP_LOG_DEBUG("Memory allocation failed\n");
		return NULL;
	}

	memset(domain, 0, sizeof(pfe_l2br_domain_t));
	domain->bridge = bridge;
	domain->vlan_entry = NULL;
	domain->is_fallback = TRUE;
	LLIST_Init(&domain->ifaces);

	domain->mutex = oal_mm_malloc(sizeof(oal_mutex_t));
	if (NULL == domain->mutex)
	{
		NXP_LOG_DEBUG("Memory allocation failed\n");
		ret = ENOMEM;
		return NULL;
	}

	ret = oal_mutex_init(domain->mutex);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Could not initialize mutex\n");
		oal_mm_free(domain->mutex);
		domain->mutex = NULL;
		return NULL;
	}

	if (EOK != pfe_class_get_mmap(bridge->class, 0, &class_mmap))
	{
		NXP_LOG_ERROR("Could not get memory map\n");
		oal_mm_free(domain);
		domain = NULL;
	}
	else
	{
		bridge->dmem_fb_bd_base = oal_ntohl(class_mmap.dmem_fb_bd_base);
		bridge->dmem_def_bd_base = oal_ntohl(class_mmap.dmem_def_bd_base);

		NXP_LOG_INFO("Fall-back bridge domain @ 0x%x (class)\n", bridge->dmem_fb_bd_base);
		NXP_LOG_INFO("Default bridge domain @ 0x%x (class)\n", bridge->dmem_def_bd_base);

		domain->action_data.forward_list = 0U;
		domain->action_data.untag_list = 0U;
		domain->action_data.ucast_hit_action = L2BR_ACT_DISCARD;
		domain->action_data.ucast_miss_action = L2BR_ACT_DISCARD;
		domain->action_data.mcast_hit_action = L2BR_ACT_DISCARD;
		domain->action_data.mcast_miss_action = L2BR_ACT_DISCARD;

		if (EOK != pfe_l2br_update_hw_entry(domain))
		{
			oal_mm_free(domain);
			domain = NULL;
		}

		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Remember the domain instance in global list */
		LLIST_AddAtEnd(&domain->list_entry, &bridge->domains);

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}
	}

	return domain;
}

/**
 * @brief		Set unicast actions
 * @param[in]	domain The bridge domain instance
 * @param[in]	hit Action to be taken when destination MAC address (uni-cast) of a packet
 *					matching the domain is found in the MAC table
 * @param[in]	miss Action to be taken when destination MAC address (uni-cast) of a packet
 *					 matching the domain is not found in the MAC table
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_set_ucast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t hit, pfe_ct_l2br_action_t miss)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	domain->action_data.ucast_hit_action = hit;
	domain->action_data.ucast_miss_action = miss;

	return pfe_l2br_update_hw_entry(domain);
}

/**
 * @brief		Get unicast actions
 * @param[in]	domain The bridge domain instance
 * @param[out]	hit Pointer to memory where hit action shall be written
 * @param[out]	miss Pointer to memory where miss action shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_get_ucast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t *hit, pfe_ct_l2br_action_t *miss)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == hit) || (NULL == miss)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	*hit = domain->action_data.ucast_hit_action;
	*miss = domain->action_data.ucast_miss_action;

	return EOK;
}

/**
 * @brief		Set multi-cast actions
 * @param[in]	domain The bridge domain instance
 * @param[in]	hit Action to be taken when destination MAC address (multi-cast) of a packet
 *					matching the domain is found in the MAC table
 * @param[in]	miss Action to be taken when destination MAC address (multi-cast) of a packet
 *					 matching the domain is not found in the MAC table
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_set_mcast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t hit, pfe_ct_l2br_action_t miss)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	domain->action_data.mcast_hit_action = hit;
	domain->action_data.mcast_miss_action = miss;

	return pfe_l2br_update_hw_entry(domain);
}

/**
 * @brief		Get multicast actions
 * @param[in]	domain The bridge domain instance
 * @param[out]	hit Pointer to memory where hit action shall be written
 * @param[out]	miss Pointer to memory where miss action shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_get_mcast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t *hit, pfe_ct_l2br_action_t *miss)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == hit) || (NULL == miss)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	*hit = domain->action_data.mcast_hit_action;
	*miss = domain->action_data.mcast_miss_action;

	return EOK;
}

/**
 * @brief		Add an interface to bridge domain
 * @param[in]	domain The bridge domain instance
 * @param[in]	iface Interface to be added
 * @param[in]	tagged TRUE means the interface is 'tagged', FALSE stands for 'un-tagged'
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 * @retval		EEXIST Already added
 */
errno_t pfe_l2br_domain_add_if(pfe_l2br_domain_t *domain, pfe_phy_if_t *iface, bool_t tagged)
{
	errno_t ret;
	pfe_ct_phy_if_id_t id;
	pfe_l2br_list_entry_t *entry;
	LLIST_t *item;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check duplicates */
	id = pfe_phy_if_get_id(iface);
	if (FALSE == LLIST_IsEmpty(&domain->ifaces))
	{
		LLIST_ForEach(item, &domain->ifaces)
		{
			entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
			if (((pfe_phy_if_t *)entry->ptr == iface))
			{
				NXP_LOG_INFO("Interface %d already added\n", id);
				return EEXIST;
			}
		}
	}

	entry = oal_mm_malloc(sizeof(pfe_l2br_list_entry_t));
	if (NULL == entry)
	{
		NXP_LOG_DEBUG("Malloc failed\n");
		return ENOMEM;
	}

	/*	Add it to this domain = update VLAN table entry */
	domain->action_data.forward_list |= (1U << (uint32_t)id);

	if (FALSE == tagged)
	{
		domain->action_data.untag_list |= (1U << (uint32_t)id);
	}

	ret = pfe_l2br_update_hw_entry(domain);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't update VLAN table entry: %d\n", ret);
		oal_mm_free(entry);
		return ENOEXEC;
	}

	/*	Remember the interface instance in global list */
	if (EOK != oal_mutex_lock(domain->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	entry->ptr = (void *)iface;
	LLIST_AddAtEnd(&entry->list_entry, &domain->ifaces);

	if (EOK != oal_mutex_unlock(domain->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Remove interface from bridge domain
 * @param[in]	domain The bridge domain instance
 * @param[in]	iface Interface to be deleted
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_l2br_domain_del_if(pfe_l2br_domain_t *domain, pfe_phy_if_t *iface)
{
	errno_t ret;
	LLIST_t *aux, *item;
	pfe_l2br_list_entry_t *entry;
	bool_t match = FALSE;
	pfe_ct_phy_if_id_t id;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Remove the interface instance from global list if it has been added there */
	if (EOK != oal_mutex_lock(domain->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	LLIST_ForEachRemovable(item, aux, &domain->ifaces)
	{
		entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
		if (entry->ptr == (void *)iface)
		{
			/*	Found in list */
			id = pfe_phy_if_get_id((pfe_phy_if_t *)entry->ptr);

			/*	Update HW */
			domain->action_data.forward_list &= ~(1U << (uint32_t)id);
			domain->action_data.untag_list &= ~(1U << (uint32_t)id);

			ret = pfe_l2br_update_hw_entry(domain);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("VLAN table entry update failed: %d\n", ret);
				return ENOEXEC;
			}

			/*	Release the list entry */
			if (&entry->list_entry == domain->cur_item)
			{
				/*	Remember the change so we can call del_if() between get_first()
					and get_next() calls. */
				domain->cur_item = domain->cur_item->prNext;
			}

			LLIST_Remove(item);
			oal_mm_free(entry);
			entry = NULL;

			match = TRUE;
		}
	}

	if (EOK != oal_mutex_unlock(domain->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	if (FALSE == match)
	{
		NXP_LOG_DEBUG("Interface not found\n");
		return ENOENT;
	}

	return EOK;
}

/**
 * @brief		Get list of associated physical interfaces
 * @param[in]	domain The domain instance
 * @return		Bitmask representing physical interface IDs. Every bit represents ID
 * 				corresponding to its position. Bit (1 << 3) represents ID=3. The IDs
 * 				match the pfe_ct_phy_if_id_t values.
 */
__attribute__((pure)) uint32_t pfe_l2br_domain_get_if_list(pfe_l2br_domain_t *domain)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return domain->action_data.forward_list;
}

/**
 * @brief		Get list of associated physical interfaces in 'untag' mode
 * @param[in]	domain The domain instance
 * @return		Bitmask representing physical interface IDs. Every bit represents ID
 * 				corresponding to its position. Bit (1 << 3) represents ID=3. The IDs
 * 				match the pfe_ct_phy_if_id_t values.
 */
__attribute__((pure)) uint32_t pfe_l2br_domain_get_untag_if_list(pfe_l2br_domain_t *domain)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return domain->action_data.untag_list;
}

/**
 * @brief		Match entry with latest criterion provided via pfe_l2br_domain_get_first_if()
 * @param[in]	domain The L2 bridge domain instance
 * @param[in]	iface The interface to be matched
 * @retval		TRUE Interface matches the criterion
 * @retval		FALSE Interface does not match the criterion
 */
static bool_t pfe_l2br_domain_match_if_criterion(pfe_l2br_domain_t *domain, pfe_phy_if_t *iface)
{
	bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	switch (domain->cur_crit)
	{
		case L2BD_IF_CRIT_ALL:
		{
			match = TRUE;
			break;
		}

		case L2BD_IF_BY_PHY_IF_ID:
		{
			match = (domain->cur_crit_arg.id == pfe_phy_if_get_id(iface));
			break;
		}

		case L2BD_IF_BY_PHY_IF:
		{
			match = (domain->cur_crit_arg.phy_if == iface);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown criterion\n");
			match = FALSE;
			break;
		}
	}

	return match;
}

/**
 * @brief		Get first interface belonging to the domain matching given criterion
 * @param[in]	domain The domain instance
 * @param[in]	crit Get criterion
 * @param[in]	arg Pointer to criterion argument
 * @return		The interface instance or NULL if not found
 * @internal
 * @warning		Do not call this function from within the l2br module since it modifies
 * 				internal state. Caller does rely on fact that there are no unexpected,
 * 				hidden calls of this function.
 * @endinternal
 */
pfe_phy_if_t *pfe_l2br_domain_get_first_if(pfe_l2br_domain_t *domain, pfe_l2br_domain_if_get_crit_t crit, void *arg)
{
	LLIST_t *item;
	pfe_phy_if_t *phy_if;
	bool_t match = FALSE;
	pfe_l2br_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Remember criterion and argument for possible subsequent pfe_l2br_get_next_domain() calls */
	domain->cur_crit = crit;
	switch (domain->cur_crit)
	{
		case L2BD_CRIT_ALL:
		{
			break;
		}

		case L2BD_IF_BY_PHY_IF_ID:
		{
			domain->cur_crit_arg.id = (pfe_ct_phy_if_id_t)arg;
			break;
		}

		case L2BD_IF_BY_PHY_IF:
		{
#if defined(PFE_CFG_NULL_ARG_CHECK)
			if (unlikely(NULL == arg))
			{
				NXP_LOG_ERROR("NULL argument received\n");
				return NULL;
			}
#endif /* PFE_CFG_NULL_ARG_CHECK */
			domain->cur_crit_arg.phy_if = (pfe_phy_if_t *)arg;
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown criterion\n");
			return NULL;
		}
	}

	if (FALSE == LLIST_IsEmpty(&domain->ifaces))
	{
		/*	Get first matching entry */
		LLIST_ForEach(item, &domain->ifaces)
		{
			/*	Get data */
			entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
			phy_if = (pfe_phy_if_t *)entry->ptr;

			/*	Remember current item to know where to start later */
			domain->cur_item = item->prNext;
			if (TRUE == pfe_l2br_domain_match_if_criterion(domain, phy_if))
			{
				match = TRUE;
				break;
			}
		}
	}

	if (TRUE == match)
	{
		return phy_if;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Get next interface from the domain
 * @details		Intended to be used with pfe_l2br_domain_get_first_if().
 * @param[in]	domain The domain instance
 * @return		The interface instance or NULL if not found
 */
pfe_phy_if_t *pfe_l2br_domain_get_next_if(pfe_l2br_domain_t *domain)
{
	pfe_phy_if_t *phy_if;
	bool_t match = FALSE;
	pfe_l2br_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (domain->cur_item == &domain->ifaces)
	{
		/*	No more entries */
		phy_if = NULL;
	}
	else
	{
		while (domain->cur_item != &domain->ifaces)
		{
			/*	Get data */
			entry = LLIST_Data(domain->cur_item, pfe_l2br_list_entry_t, list_entry);
			phy_if = (pfe_phy_if_t *)entry->ptr;

			/*	Remember current item to know where to start later */
			domain->cur_item = domain->cur_item->prNext;

			if (NULL != domain)
			{
				if (true == pfe_l2br_domain_match_if_criterion(domain, phy_if))
				{
					match = TRUE;
					break;
				}
			}
		}
	}

	if (true == match)
	{
		return phy_if;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Get VLAN ID
 * @param[in]	domain The domain instance
 * @param[out]	vlan Pointer to memory where the VLAN ID shall be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_l2br_domain_get_vlan(pfe_l2br_domain_t *domain, uint16_t *vlan)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == vlan)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	*vlan = domain->vlan;

	return EOK;
}

/**
 * @brief		Query if domain is default domain
 * @param[in]	domain The domain instance
 * @retval		TRUE Is default
 * @retval		FALSE Is not default
 */
__attribute__((pure)) bool_t pfe_l2br_domain_is_default(pfe_l2br_domain_t *domain)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return domain->is_default;
}

/**
 * @brief		Query if domain is fall-back domain
 * @param[in]	domain The domain instance
 * @retval		TRUE Is fall-back
 * @retval		FALSE Is not fall-back
 */
__attribute__((pure)) bool_t pfe_l2br_domain_is_fallback(pfe_l2br_domain_t *domain)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return domain->is_fallback;
}

/**
 * @brief		Worker function running within internal thread
 */
static void *pfe_l2br_worker_func(void *arg)
{
	pfe_l2br_t *bridge = (pfe_l2br_t *)arg;
	errno_t err;
	oal_mbox_msg_t msg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	while (3)
	{
		err = oal_mbox_receive(bridge->mbox, &msg);
		if (EOK != err)
		{
			NXP_LOG_WARNING("mbox: Problem receiving message: %d", err);
		}
		else
		{
			switch (msg.payload.code)
			{
				case SIG_WORKER_STOP:
				{
					/*	Exit the thread */
					oal_mbox_ack_msg(&msg);
					return NULL;
				}

				case SIG_TIMER_TICK:
				{
					pfe_l2br_do_timeouts(bridge);
					break;
				}
			}
		}

		oal_mbox_ack_msg(&msg);
	}

	return NULL;
}

/**
 * @brief		Perform aging
 * @param[in]	bridge The bridge instance
 */
static void pfe_l2br_do_timeouts(pfe_l2br_t *bridge)
{
	errno_t ret;
	pfe_l2br_table_entry_t *entry;
	char_t text_buf[256];

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Create entry storage */
	entry = pfe_l2br_table_entry_create(bridge->mac_table);

	/*	Go through all entries */
	ret = pfe_l2br_table_get_first(bridge->mac_table, L2BR_TABLE_CRIT_VALID, entry);
	while (EOK == ret)
	{
		if (FALSE == pfe_l2br_table_entry_is_static(entry))
		{
			if (TRUE == pfe_l2br_table_entry_is_fresh(entry))
			{
				ret = pfe_l2br_table_del_entry(bridge->mac_table, entry);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not delete MAC table entry: %d\n", ret);
				}
				else
				{
					pfe_l2br_table_entry_to_str(entry, text_buf, 256);
					NXP_LOG_DEBUG("Aging:\n%s\n", text_buf);
				}
			}
			else
			{
				/*	Reset the fresh flag */
				ret = pfe_l2br_table_entry_set_fresh(bridge->mac_table, entry, TRUE);
				if (EOK != ret)
				{
					NXP_LOG_DEBUG("Can't set fresh flag: %d\n", ret);
				}

				/*	Update the entry */
				ret = pfe_l2br_table_update_entry(bridge->mac_table, entry);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Unable to update MAC table entry: %d\n", ret);
				}
			}
		}

		ret = pfe_l2br_table_get_next(bridge->mac_table, entry);
	}

	/*	Release entry storage */
	(void)pfe_l2br_table_entry_destroy(entry);
}

/**
 * @brief		Create L2 bridge instance
 * @param[in]	class The classifier instance
 * @param[in]	def_vlan Default VLAN
 * @param[in]	mac_table The MAC table instance
 * @param[in]	vlan_table The VLAN table instance
 * @return		The instance or NULL if failed
 */
pfe_l2br_t *pfe_l2br_create(pfe_class_t *class, uint16_t def_vlan, pfe_l2br_table_t *mac_table, pfe_l2br_table_t *vlan_table)
{
	pfe_l2br_t *bridge;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == mac_table) || (NULL == vlan_table)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	bridge = oal_mm_malloc(sizeof(pfe_l2br_t));

	if (NULL == bridge)
	{
		NXP_LOG_DEBUG("malloc() failed\n");
	}
	else
	{
		memset(bridge, 0, sizeof(pfe_l2br_t));
		bridge->class = class;
		bridge->mac_table = mac_table;
		bridge->vlan_table = vlan_table;
		bridge->def_vlan = def_vlan;
		LLIST_Init(&bridge->domains);

		bridge->mutex = oal_mm_malloc(sizeof(oal_mutex_t));
		if (NULL == bridge->mutex)
		{
			NXP_LOG_DEBUG("Memory allocation failed\n");
			goto free_and_fail;
		}

		if (EOK != oal_mutex_init(bridge->mutex))
		{
			NXP_LOG_ERROR("Could not initialize mutex\n");
			oal_mm_free(bridge->mutex);
			bridge->mutex = NULL;
			goto free_and_fail;
		}

		/*	Create default domain */
		bridge->default_domain = pfe_l2br_create_default_domain(bridge, def_vlan);
		if (NULL == bridge->default_domain)
		{
			NXP_LOG_DEBUG("Could not create default domain\n");
			goto free_and_fail;
		}

		/*	Create fallback domain */
		bridge->fallback_domain = pfe_l2br_create_fallback_domain(bridge);
		if (NULL == bridge->fallback_domain)
		{
			NXP_LOG_DEBUG("Could not create fallback domain\n");
			goto free_and_fail;
		}

		/*	Configure classifier */
		(void)pfe_class_set_default_vlan(class, def_vlan);

		/*	Create mbox */
		bridge->mbox = oal_mbox_create();
		if (NULL == bridge->mbox)
		{
			NXP_LOG_ERROR("MBox creation failed\n");
			goto free_and_fail;
		}

		/*	Create worker thread */
		bridge->worker = oal_thread_create(&pfe_l2br_worker_func, bridge, "l2br worker", 0);
		if (NULL == bridge->worker)
		{
			NXP_LOG_ERROR("Couldn't start worker thread\n");
			goto free_and_fail;
		}
		else
		{
			if (EOK != oal_mbox_attach_timer(bridge->mbox, PFE_L2BR_CFG_TICK_PERIOD_SEC * 1000, SIG_TIMER_TICK))
			{
				NXP_LOG_ERROR("Unable to attach timer\n");
				goto free_and_fail;
			}
		}
	}

	return bridge;

free_and_fail:

	(void)pfe_l2br_destroy(bridge);
	return NULL;
}

/**
 * @brief		Destroy L2 Bridge instance
 * @param[in]	bridge The bridge instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_l2br_destroy(pfe_l2br_t *bridge)
{
	errno_t ret;

	if (NULL != bridge)
	{
		if (NULL != bridge->mbox)
		{
			if (EOK != oal_mbox_detach_timer(bridge->mbox))
			{
				NXP_LOG_DEBUG("Could not detach timer\n");
			}
		}

		if (NULL != bridge->worker)
		{
			NXP_LOG_INFO("Stopping bridge worker...\n");

			ret = oal_mbox_send_signal(bridge->mbox, SIG_WORKER_STOP);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Signal failed: %d\n", ret);
			}
			else
			{
				ret = oal_thread_join(bridge->worker, NULL);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't join the worker thread: %d\n", ret);
				}
				else
				{
					NXP_LOG_INFO("L2 bridge worker stopped\n");
				}
			}
		}

		if (NULL != bridge->mbox)
		{
			oal_mbox_destroy(bridge->mbox);
			bridge->mbox = NULL;
		}

		if (NULL != bridge->default_domain)
		{
			pfe_l2br_domain_destroy(bridge->default_domain);
			bridge->default_domain = NULL;
		}

		if (NULL != bridge->fallback_domain)
		{
			pfe_l2br_domain_destroy(bridge->fallback_domain);
			bridge->fallback_domain = NULL;
		}

		if (FALSE == LLIST_IsEmpty(&bridge->domains))
		{
			NXP_LOG_WARNING("Bridge is being destroyed but still contains some active domains\n");
		}

		if (NULL != bridge->mutex)
		{
			if (EOK != oal_mutex_destroy(bridge->mutex))
			{
				NXP_LOG_DEBUG("Could not destroy mutex\n");
			}

			oal_mm_free(bridge->mutex);
			bridge->mutex = NULL;
		}

		oal_mm_free(bridge);
	}
	else
	{
		NXP_LOG_DEBUG("Argument is NULL\n");
		return EINVAL;
	}

	return EOK;
}

/**
 * @brief		Get the default bridge domain instance
 * @param[in]	bridge The bridge instance
 * @return		The domain instance or NULL if failed
 */
__attribute__((pure)) pfe_l2br_domain_t *pfe_l2br_get_default_domain(pfe_l2br_t *bridge)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return bridge->default_domain;
}

/**
 * @brief		Get the fall-back bridge domain instance
 * @param[in]	bridge The bridge instance
 * @return		The domain instance or NULL if failed
 */
__attribute__((pure)) pfe_l2br_domain_t *pfe_l2br_get_fallback_domain(pfe_l2br_t *bridge)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return bridge->fallback_domain;
}

/**
 * @brief		Match entry with latest criterion provided via pfe_l2br_get_first_domain()
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	domain The domain to be matched
 * @retval		TRUE Domain matches the criterion
 * @retval		FALSE Domain does not match the criterion
 */
static bool_t pfe_l2br_domain_match_criterion(pfe_l2br_t *bridge, pfe_l2br_domain_t *domain)
{
	bool_t match = FALSE;
	LLIST_t *item;
	pfe_l2br_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == domain)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	switch (bridge->cur_crit)
	{
		case L2BD_CRIT_ALL:
		{
			match = TRUE;
			break;
		}

		case L2BD_CRIT_BY_VLAN:
		{
			match = (domain->vlan == bridge->cur_crit_arg.vlan);
			break;
		}

		case L2BD_BY_PHY_IF:
		{
			/*	Find out if domain contains given interface */
			if (FALSE == LLIST_IsEmpty(&domain->ifaces))
			{
				LLIST_ForEach(item, &domain->ifaces)
				{
					entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
					match = (((pfe_phy_if_t *)entry->ptr) == bridge->cur_crit_arg.phy_if);
					if (TRUE == match)
					{
						break;
					}
				}
			}

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
 * @brief		Get first L2 bridge domain instance according to given criterion
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	crit Get criterion
 * @param[in]	arg Pointer to criterion argument
 * @return		The domain instance or NULL if not found
 */
pfe_l2br_domain_t *pfe_l2br_get_first_domain(pfe_l2br_t *bridge, pfe_l2br_domain_get_crit_t crit, void *arg)
{
	LLIST_t *item;
	pfe_l2br_domain_t *domain;
	bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Remember criterion and argument for possible subsequent pfe_l2br_get_next_domain() calls */
	bridge->cur_crit = crit;
	switch (bridge->cur_crit)
	{
		case L2BD_CRIT_ALL:
		{
			break;
		}

		case L2BD_CRIT_BY_VLAN:
		{
			bridge->cur_crit_arg.vlan = (uint16_t)((addr_t)arg & 0xffffU);
			break;
		}

		case L2BD_BY_PHY_IF:
		{
			bridge->cur_crit_arg.phy_if = (pfe_phy_if_t *)arg;
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown criterion\n");
			return NULL;
		}
	}

	if (FALSE == LLIST_IsEmpty(&bridge->domains))
	{
		/*	Get first matching entry */
		LLIST_ForEach(item, &bridge->domains)
		{
			/*	Get data */
			domain = LLIST_Data(item, pfe_l2br_domain_t, list_entry);

			/*	Remember current item to know where to start later */
			bridge->cur_item = item->prNext;
			if (TRUE == pfe_l2br_domain_match_criterion(bridge, domain))
			{
				match = TRUE;
				break;
			}
		}
	}

	if (TRUE == match)
	{
		return domain;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Get next domain from the bridge
 * @details		Intended to be used with pfe_l2br_get_first_domain().
 * @param[in]	bridge The L2 bridge instance
 * @return		The domain instance or NULL if not found
 * @warning		The returned entry must not be accessed after fci_l2br_domain_destroy(entry) has been called.
 */
pfe_l2br_domain_t *pfe_l2br_get_next_domain(pfe_l2br_t *bridge)
{
	pfe_l2br_domain_t *domain;
	bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (bridge->cur_item == &bridge->domains)
	{
		/*	No more entries */
		domain = NULL;
	}
	else
	{
		while (bridge->cur_item != &bridge->domains)
		{
			/*	Get data */
			domain = LLIST_Data(bridge->cur_item, pfe_l2br_domain_t, list_entry);

			/*	Remember current item to know where to start later */
			bridge->cur_item = bridge->cur_item->prNext;

			if (NULL != domain)
			{
				if (true == pfe_l2br_domain_match_criterion(bridge, domain))
				{
					match = TRUE;
					break;
				}
			}
		}
	}

	if (true == match)
	{
		return domain;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief		Return L2 Bridge runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	bridge		The L2 Bridge instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_l2br_get_text_statistics(pfe_l2br_t *bridge, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;
    pfe_l2br_table_entry_t *entry;
    errno_t ret;
    uint32_t count = 0U;

    /* Get memory */
    entry = pfe_l2br_table_entry_create(bridge->mac_table);
    /* Get the first entry */
    ret = pfe_l2br_table_get_first(bridge->mac_table, L2BR_TABLE_CRIT_VALID, entry);
    while (EOK == ret)
    {
        /* Print out the entry */
        len += pfe_l2br_table_entry_to_str(entry, buf + len, buf_len - len);
        count++;
        /* Get the next entry */
        ret = pfe_l2br_table_get_next(bridge->mac_table, entry);
    }
    len += oal_util_snprintf(buf + len, buf_len - len, "\nEntries count: %u\n", count);
    /* Free memory */
    (void)pfe_l2br_table_entry_destroy(entry);
    return len;
}
