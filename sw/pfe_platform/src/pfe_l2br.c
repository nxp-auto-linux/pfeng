/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
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
	LLIST_t static_entries;						/*!< List of static entries */
	uint16_t def_vlan;							/*!< Default VLAN */
	uint32_t dmem_fb_bd_base;					/*!< Address within classifier memory where the fall-back bridge domain structure is located */
	uint32_t dmem_def_bd_base;					/*!< Address within classifier memory where the default bridge domain structure is located */
	oal_mutex_t *mutex;							/*!< Mutex protecting shared resources */
	pfe_l2br_domain_get_crit_t cur_crit;		/*!< Current 'get' criterion (to get domains) */
	pfe_l2br_static_ent_get_crit_t cur_crit_ent;/*!< Current 'get' criterion (to get static entry) */
	LLIST_t *curr_domain;						/*!< The current domain list item */
	LLIST_t *curr_static_ent;					/*!< The current static entry list item */
	union
	{
		uint16_t vlan;
		pfe_phy_if_t *phy_if;
	} cur_domain_crit_arg;								/*!< Current domain criterion argument */
	struct
	{
		uint16_t vlan;
		pfe_mac_addr_t mac;
	} cur_static_ent_crit_arg;							/*!< Current static entry argument */
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
	} u;

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

struct __pfe_l2br_static_entry_tag
{
	union
	{
		pfe_ct_mac_table_result_t action_data;
		uint64_t action_data_u64val;
	} u;
	uint16_t vlan;
	pfe_mac_addr_t mac;						/*!< Mac address to be matched */
	pfe_l2br_table_entry_t *entry;			/*!< This is entry within MAC table representing the static entry */
	pfe_l2br_t *bridge;
	LLIST_t list_entry;						/*!< Entry for linked-list purposes */
};

/**
 * @brief	Internal linked-list element
 */
typedef struct 
{
	void *ptr;
	LLIST_t list_entry;
} pfe_l2br_list_entry_t;

/**
 * @brief	MAC table flush types
 */
typedef enum
{
	PFE_L2BR_FLUSH_ALL_MAC,
	PFE_L2BR_FLUSH_STATIC_MAC,
	PFE_L2BR_FLUSH_LEARNED_MAC
} pfe_l2br_flush_types;

static errno_t pfe_bd_write_to_class(const pfe_l2br_t *bridge, uint32_t base, pfe_ct_bd_entry_t *class_entry);
static errno_t pfe_l2br_update_hw_entry(pfe_l2br_domain_t *domain);
static pfe_l2br_domain_t *pfe_l2br_create_default_domain(pfe_l2br_t *bridge, uint16_t vlan);
static pfe_l2br_domain_t *pfe_l2br_create_fallback_domain(pfe_l2br_t *bridge);
static bool_t pfe_l2br_domain_match_if_criterion(const pfe_l2br_domain_t *domain, const pfe_phy_if_t *iface);
static bool_t pfe_l2br_domain_match_criterion(const pfe_l2br_t *bridge, pfe_l2br_domain_t *domain);
static bool_t pfe_l2br_static_entry_match_criterion(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t *static_ent);
static errno_t pfe_l2br_set_mac_aging_timeout(pfe_class_t *class, const uint16_t timeout);

/**
 * @brief		Write bridge domain structure to classifier memory
 * @param[in]	bridge The bridge instance
 * @param[in]	base Memory location where to write
 * @param[in]	class_entry Pointer to the structure to be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
static errno_t pfe_bd_write_to_class(const pfe_l2br_t *bridge, uint32_t base, pfe_ct_bd_entry_t *class_entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_entry) || (NULL == bridge) || (0U == base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_class_write_dmem(bridge->class, -1, (addr_t)base, (void *)class_entry, sizeof(pfe_ct_bd_entry_t));
}

static void pfe_l2br_update_hw_ll_entry(pfe_l2br_domain_t *domain, uint32_t base)
{
	pfe_ct_bd_entry_t sw_bd;
	uint64_t tmp64;
	bool_t need_shift = FALSE;

	/*	Sanity check */
	ct_assert(sizeof(pfe_ct_bd_entry_t) <= sizeof(uint64_t));

	/*	Check if bitfields within structure are aligned as expected */
	(void)memset(&sw_bd, 0, sizeof(pfe_ct_bd_entry_t));
	tmp64 = (1ULL << 63);
	(void)memcpy(&sw_bd, &tmp64, sizeof(uint64_t));
	if (0U == sw_bd.val)
	{
		need_shift = TRUE;
	}

	/*	Convert VLAN table result to fallback domain representation */
	sw_bd.val = domain->u.action_data.val;

	if (TRUE == need_shift)
	{
		tmp64 = (uint64_t)sw_bd.val << 9;
		(void)memcpy(&sw_bd, &tmp64, sizeof(pfe_ct_bd_entry_t));
	}

	/*	Convert to network endian */
	/* TODO: oal_swap64 or so */
#if defined(PFE_CFG_TARGET_OS_QNX)
	ENDIAN_SWAP64(&sw_bd.val);
#elif defined(PFE_CFG_TARGET_OS_LINUX)
	tmp64 = cpu_to_be64p((uint64_t *)&sw_bd);
	(void)memcpy(&sw_bd, &tmp64, sizeof(uint64_t));
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
		ret = pfe_l2br_table_entry_set_action_data(domain->vlan_entry, domain->u.action_data_u64val);
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
		(void)memset(domain, 0, sizeof(pfe_l2br_domain_t));
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
            
			domain->u.action_data.item.forward_list = 0U;
			domain->u.action_data.item.untag_list = 0U;
			domain->u.action_data.item.ucast_hit_action = (uint64_t)L2BR_ACT_DISCARD;
			domain->u.action_data.item.ucast_miss_action = (uint64_t)L2BR_ACT_DISCARD;
			domain->u.action_data.item.mcast_hit_action = (uint64_t)L2BR_ACT_DISCARD;
			domain->u.action_data.item.mcast_miss_action = (uint64_t)L2BR_ACT_DISCARD;

			/*	Set action data */
			ret = pfe_l2br_table_entry_set_action_data(domain->vlan_entry, domain->u.action_data_u64val);
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
	errno_t ret = EOK;
	LLIST_t *aux, *item;
	pfe_l2br_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
    else
    {
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
            (void)pfe_l2br_table_entry_destroy(domain->vlan_entry);
            domain->vlan_entry = NULL;
        }

        if (TRUE == domain->is_fallback)
        {
            /*	Disable the fall-back domain traffic */
            ret = pfe_l2br_domain_set_ucast_action(domain, L2BR_ACT_DISCARD, L2BR_ACT_DISCARD);
            if (EOK == ret)
            {
                ret = pfe_l2br_domain_set_mcast_action(domain, L2BR_ACT_DISCARD, L2BR_ACT_DISCARD);
            }
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
            if (&domain->list_entry == domain->bridge->curr_domain)
            {
                /*	Remember the change so we can call destroy() between get_first()
                    and get_next() calls. */
                domain->bridge->curr_domain = domain->bridge->curr_domain->prNext;
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

#if defined(PFE_CFG_NULL_ARG_CHECK)
    }
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return ret;
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

	(void)memset(domain, 0, sizeof(pfe_l2br_domain_t));
	domain->bridge = bridge;
	domain->vlan_entry = NULL;
	domain->is_fallback = TRUE;
	LLIST_Init(&domain->ifaces);

	domain->mutex = oal_mm_malloc(sizeof(oal_mutex_t));
	if (NULL == domain->mutex)
	{
		NXP_LOG_DEBUG("Memory allocation failed\n");
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

		NXP_LOG_INFO("Fall-back bridge domain @ 0x%x (class)\n", (uint_t)bridge->dmem_fb_bd_base);
		NXP_LOG_INFO("Default bridge domain @ 0x%x (class)\n", (uint_t)bridge->dmem_def_bd_base);

		domain->u.action_data.item.forward_list = 0U;
		domain->u.action_data.item.untag_list = 0U;
		domain->u.action_data.item.ucast_hit_action = (uint64_t)L2BR_ACT_DISCARD;
		domain->u.action_data.item.ucast_miss_action = (uint64_t)L2BR_ACT_DISCARD;
		domain->u.action_data.item.mcast_hit_action = (uint64_t)L2BR_ACT_DISCARD;
		domain->u.action_data.item.mcast_miss_action = (uint64_t)L2BR_ACT_DISCARD;

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

	domain->u.action_data.item.ucast_hit_action = (uint64_t)hit;
	domain->u.action_data.item.ucast_miss_action = (uint64_t)miss;

	return pfe_l2br_update_hw_entry(domain);
}

/**
 * @brief		Get unicast actions
 * @param[in]	domain The bridge domain instance
 * @param[out]	hit Pointer to memory where hit action shall be written
 * @param[out]	miss Pointer to memory where miss action shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_get_ucast_action(const pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t *hit, pfe_ct_l2br_action_t *miss)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == hit) || (NULL == miss)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	*hit = (pfe_ct_l2br_action_t)(domain->u.action_data.item.ucast_hit_action);
	*miss = (pfe_ct_l2br_action_t)(domain->u.action_data.item.ucast_miss_action);

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

	domain->u.action_data.item.mcast_hit_action = (uint64_t)hit;
	domain->u.action_data.item.mcast_miss_action = (uint64_t)miss;

	return pfe_l2br_update_hw_entry(domain);
}

/**
 * @brief		Get multicast actions
 * @param[in]	domain The bridge domain instance
 * @param[out]	hit Pointer to memory where hit action shall be written
 * @param[out]	miss Pointer to memory where miss action shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_get_mcast_action(const pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t *hit, pfe_ct_l2br_action_t *miss)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == hit) || (NULL == miss)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	*hit = (pfe_ct_l2br_action_t)(domain->u.action_data.item.mcast_hit_action);
	*miss = (pfe_ct_l2br_action_t)(domain->u.action_data.item.mcast_miss_action);

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
	domain->u.action_data.item.forward_list |= (uint64_t)1U << (uint8_t)id;

	if (FALSE == tagged)
	{
		domain->u.action_data.item.untag_list |= (uint64_t)1U << (uint8_t)id;
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
errno_t pfe_l2br_domain_del_if(pfe_l2br_domain_t *domain, const pfe_phy_if_t *iface)
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
			domain->u.action_data.item.forward_list &= ~((uint64_t)1U << (uint8_t)id);
			domain->u.action_data.item.untag_list &= ~((uint64_t)1U << (uint8_t)id);

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
__attribute__((pure)) uint32_t pfe_l2br_domain_get_if_list(const pfe_l2br_domain_t *domain)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return domain->u.action_data.item.forward_list;
}

/**
 * @brief		Get list of associated physical interfaces in 'untag' mode
 * @param[in]	domain The domain instance
 * @return		Bitmask representing physical interface IDs. Every bit represents ID
 * 				corresponding to its position. Bit (1 << 3) represents ID=3. The IDs
 * 				match the pfe_ct_phy_if_id_t values.
 */
__attribute__((pure)) uint32_t pfe_l2br_domain_get_untag_if_list(const pfe_l2br_domain_t *domain)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return domain->u.action_data.item.untag_list;
}

/**
 * @brief		Match entry with latest criterion provided via pfe_l2br_domain_get_first_if()
 * @param[in]	domain The L2 bridge domain instance
 * @param[in]	iface The interface to be matched
 * @retval		TRUE Interface matches the criterion
 * @retval		FALSE Interface does not match the criterion
 */
static bool_t pfe_l2br_domain_match_if_criterion(const pfe_l2br_domain_t *domain, const pfe_phy_if_t *iface)
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
			match = TRUE;
			break;

		case L2BD_IF_BY_PHY_IF_ID:
			match = (domain->cur_crit_arg.id == pfe_phy_if_get_id(iface));
			break;

		case L2BD_IF_BY_PHY_IF:
			match = (domain->cur_crit_arg.phy_if == iface);
			break;

		default:
			NXP_LOG_ERROR("Unknown criterion\n");
			match = FALSE;
			break;
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
	pfe_phy_if_t *phy_if = NULL;
	bool_t match = FALSE;
    bool_t known_crit = TRUE;
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
		case L2BD_IF_CRIT_ALL:
			break;

		case L2BD_IF_BY_PHY_IF_ID:
			domain->cur_crit_arg.id = (pfe_ct_phy_if_id_t)(addr_t)arg;
			break;

		case L2BD_IF_BY_PHY_IF:
#if defined(PFE_CFG_NULL_ARG_CHECK)
			if (unlikely(NULL == arg))
			{
				NXP_LOG_ERROR("NULL argument received\n");
				return NULL;
			}
#endif /* PFE_CFG_NULL_ARG_CHECK */
			domain->cur_crit_arg.phy_if = (pfe_phy_if_t *)arg;
			break;

		default:
			NXP_LOG_ERROR("Unknown criterion\n");
            known_crit = FALSE;
            break;
	}

	if ((FALSE == LLIST_IsEmpty(&domain->ifaces)) && (TRUE == known_crit))
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

            if (true == pfe_l2br_domain_match_if_criterion(domain, phy_if))
            {
                match = TRUE;
                break;
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
errno_t pfe_l2br_domain_get_vlan(const pfe_l2br_domain_t *domain, uint16_t *vlan)
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
__attribute__((pure)) bool_t pfe_l2br_domain_is_default(const pfe_l2br_domain_t *domain)
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
__attribute__((pure)) bool_t pfe_l2br_domain_is_fallback(const pfe_l2br_domain_t *domain)
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
 * @brief		Create L2 bridge static entry
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @param[in]	mac Static entry MAC address
 * @param[in]	new_fw_list forward list of static entry
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOMEM Failure when allocating memory
 * @retval		ETIMEDOUT Timed out
 * @retval		EPERM Operation not permitted (static entry already created)
 */
errno_t pfe_l2br_static_entry_create(pfe_l2br_t *bridge, uint16_t vlan, pfe_mac_addr_t mac, uint32_t new_fw_list)
{
	pfe_l2br_static_entry_t *static_entry, *static_ent_tmp;
	bool_t match = FALSE;
	LLIST_t *item;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	static_entry = oal_mm_malloc(sizeof(pfe_l2br_static_entry_t));

	if (NULL == static_entry)
	{
		NXP_LOG_ERROR("malloc() failed\n");
		return ENOMEM;
	}
	else
	{
		(void)memset(static_entry, 0, sizeof(pfe_l2br_static_entry_t));
		static_entry->vlan = vlan;
		(void)memcpy(static_entry->mac, mac, sizeof(pfe_mac_addr_t));

		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
		{
			/*	Get first matching entry */
			LLIST_ForEach(item, &bridge->static_entries)
			{
				/*	Get data */
				static_ent_tmp = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);

				/*	Remember current item to know where to start later */
				bridge->curr_static_ent = item->prNext;
				if ((static_ent_tmp->vlan == vlan) && (0 == memcmp(static_ent_tmp->mac, mac, sizeof(pfe_mac_addr_t))))
				{
					match = TRUE;
					break;
				}
			}
		}

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}

		if (TRUE == match)
		{
			NXP_LOG_ERROR("Duplicit entry\n");
			/* Entry is duplicit */
			oal_mm_free(static_entry);
			return EPERM;
		}

		static_entry->entry = pfe_l2br_table_entry_create(bridge->mac_table);

		if (NULL == static_entry->entry)
		{
			NXP_LOG_ERROR("malloc() failed\n");
			return ENOMEM;
		}

		/* Configure action data */
		static_entry->u.action_data.val = 0;
		static_entry->u.action_data.item.static_flag = 1;
		static_entry->u.action_data.item.fresh_flag = 0U;
		static_entry->u.action_data.item.local_l3 = 0U;
		static_entry->u.action_data.item.forward_list = new_fw_list;

		if (EOK != pfe_l2br_table_entry_set_vlan(static_entry->entry, vlan))
		{
			NXP_LOG_ERROR("Couldn't set vlan\n");
			goto table_err;
		}

		if (EOK != pfe_l2br_table_entry_set_mac_addr(static_entry->entry, mac))
		{
			NXP_LOG_ERROR("Couldn't set mac address\n");
			goto table_err;
		}

		if (EOK != pfe_l2br_table_entry_set_action_data(static_entry->entry, static_entry->u.action_data_u64val))
		{
			NXP_LOG_ERROR("Couldn't set action data\n");
			goto table_err;
		}

		if (EOK != pfe_l2br_table_add_entry(bridge->mac_table, static_entry->entry))
		{
			NXP_LOG_ERROR("Couldn't set action data\n");
			goto table_err;
		}

		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		LLIST_AddAtEnd(&static_entry->list_entry, &bridge->static_entries);

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
		}
	}

	return EOK;
table_err:
	oal_mm_free(static_entry);
	static_entry = NULL;
	return EINVAL;
}

static errno_t pfe_l2br_static_entry_destroy_nolock(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	LLIST_Remove(&static_ent->list_entry);

	ret = pfe_l2br_table_del_entry(bridge->mac_table, static_ent->entry);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Static entry couldn't be deleted from HW table (errno %d)\n", ret);
	}

	oal_mm_free(static_ent);

	return ret;
}

/**
 * @brief		Destroy L2 bridge static entry
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to be deleted
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ETIMEDOUT Timed out
 */
errno_t pfe_l2br_static_entry_destroy(pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	ret = pfe_l2br_static_entry_destroy_nolock(bridge, static_ent);

	if (EOK != oal_mutex_unlock(bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Change L2 static entry forward list
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to change
 * @param[in]	new_fw_list New forward list
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Entry couldn't be updated
 */
errno_t pfe_l2br_static_entry_replace_fw_list(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent, uint32_t new_fw_list)
{
	uint32_t tmp;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	tmp = static_ent->u.action_data.item.forward_list;
	static_ent->u.action_data.item.forward_list = new_fw_list;

	if (EOK != pfe_l2br_table_entry_set_action_data(static_ent->entry, static_ent->u.action_data_u64val))
	{
		static_ent->u.action_data.item.forward_list = tmp;
		NXP_LOG_ERROR("Couldn't set action data\n");
		return EINVAL;
	}

	if (EOK != pfe_l2br_table_update_entry(bridge->mac_table, static_ent->entry))
	{
		static_ent->u.action_data.item.forward_list = tmp;
		NXP_LOG_ERROR("Couldn't update entry\n");
		return ENOENT;
	}

	return EOK;
}

/**
 * @brief Sets the local L3 flag (marks/unmarks the MAC address as local one)
 * @param[in]		bridge Bridge instance
 * @param[in]		static_ent Static entry to change
 * @param[in]		local Value to be set
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 * @retval			ENOENT Entry couldn't be updated
 */
errno_t pfe_l2br_static_entry_set_local_flag(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent, bool_t local)
{
	uint32_t tmp;
    
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	/* Make changes */
	tmp = static_ent->u.action_data.item.local_l3;
	static_ent->u.action_data.item.local_l3 = ((FALSE != local)? 1U : 0U);
	/* Propagate changes to l2br table */
	if (EOK != pfe_l2br_table_entry_set_action_data(static_ent->entry, static_ent->u.action_data_u64val))
	{
		static_ent->u.action_data.item.local_l3 = tmp;
		NXP_LOG_ERROR("Couldn't set action data\n");
		return EINVAL;
	}
	/* Write to the HW */
	if (EOK != pfe_l2br_table_update_entry(bridge->mac_table, static_ent->entry))
	{
		static_ent->u.action_data.item.local_l3 = tmp;
		NXP_LOG_ERROR("Couldn't update entry\n");
		return ENOENT;
	}
	return EOK;
}

/**
 * @brief Sets the src_discard flag (enables/disables discard of frames with given SRC MAC address)
 * @param[in]		bridge Bridge instance
 * @param[in]		static_ent Static entry to change
 * @param[in]		src_discard Value to be set
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 * @retval			ENOENT Entry couldn't be updated
 */
errno_t pfe_l2br_static_entry_set_src_discard_flag(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent, bool_t src_discard)
{
	uint32_t tmp;
    
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	/* Make changes */
	tmp = static_ent->u.action_data.item.src_discard;
	static_ent->u.action_data.item.src_discard = ((FALSE != src_discard)? 1U : 0U);
	/* Propagate changes to l2br table */
	if (EOK != pfe_l2br_table_entry_set_action_data(static_ent->entry, static_ent->u.action_data_u64val))
	{
		static_ent->u.action_data.item.src_discard = tmp;
		NXP_LOG_ERROR("Couldn't set action data\n");
		return EINVAL;
	}
	/* Write to the HW */
	if (EOK != pfe_l2br_table_update_entry(bridge->mac_table, static_ent->entry))
	{
		static_ent->u.action_data.item.src_discard = tmp;
		NXP_LOG_ERROR("Couldn't update entry\n");
		return ENOENT;
	}
	return EOK;
}

/**
 * @brief Sets the dst_discard flag (enables/disables discard of frames with given SRC MAC address)
 * @param[in]		bridge Bridge instance
 * @param[in]		static_ent Static entry to change
 * @param[in]		dst_discard Value to be set
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 * @retval			ENOENT Entry couldn't be updated
 */
errno_t pfe_l2br_static_entry_set_dst_discard_flag(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent, bool_t dst_discard)
{
	uint32_t tmp;
    errno_t ret = EINVAL;
    
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
    else
    {
#endif /* PFE_CFG_NULL_ARG_CHECK */
        /* Make changes */
        tmp = static_ent->u.action_data.item.dst_discard;
        static_ent->u.action_data.item.dst_discard = ((FALSE != dst_discard)? 1U : 0U);
        /* Propagate changes to l2br table */
        ret = pfe_l2br_table_entry_set_action_data(static_ent->entry, static_ent->u.action_data_u64val);
        if (EOK != ret)
        {
            static_ent->u.action_data.item.dst_discard = tmp;
            NXP_LOG_ERROR("Couldn't set action data\n");
            ret = EINVAL;
        }
        else
        {
            /* Write to the HW */
            if (EOK != pfe_l2br_table_update_entry(bridge->mac_table, static_ent->entry))
            {
                static_ent->u.action_data.item.dst_discard = tmp;
                NXP_LOG_ERROR("Couldn't update entry\n");
                ret = ENOENT;
            }
        }
#if defined(PFE_CFG_NULL_ARG_CHECK)
    }
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return ret;
}

/**
 * @brief Reads the state of local L3 flag
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to read from
 * @param[out]	local State of the local L3 flag
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 */
errno_t pfe_l2br_static_entry_get_local_flag(const pfe_l2br_t *bridge, const pfe_l2br_static_entry_t* static_ent, bool_t *local)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent) || (NULL == local)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#else
    (void)bridge;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	*local = ((0U != static_ent->u.action_data.item.local_l3)? TRUE : FALSE);
	return EOK;
}

/**
 * @brief Reads the state of src_discard flag
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to read from
 * @param[out]	src_discard State of the src_discard flag
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 */
errno_t pfe_l2br_static_entry_get_src_discard_flag(pfe_l2br_t *bridge, const pfe_l2br_static_entry_t* static_ent, bool_t *src_discard)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent) || (NULL == src_discard)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#else
    (void)bridge;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	*src_discard = ((0U != static_ent->u.action_data.item.src_discard)? TRUE : FALSE);
	return EOK;
}

/**
 * @brief Reads the state of dst_discard flag
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to read from
 * @param[out]	dst_discard State of the dst_discard flag
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 */
errno_t pfe_l2br_static_entry_get_dst_discard_flag(const pfe_l2br_t *bridge, const pfe_l2br_static_entry_t* static_ent, bool_t *dst_discard)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent) || (NULL == dst_discard)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#else
    (void)bridge;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	*dst_discard = ((0U != static_ent->u.action_data.item.dst_discard)? TRUE : FALSE);
	return EOK;
}

/**
 * @brief Reads the forward list
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to read from
 * @return		Bitmask representing egress physical interfaces. Every bit represents ID
 * 				corresponding to its position. Bit (1 << 3) represents ID=3. The IDs
 * 				match the pfe_ct_phy_if_id_t values.
 */
__attribute__((pure)) uint32_t pfe_l2br_static_entry_get_fw_list(const pfe_l2br_static_entry_t* static_ent)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == static_ent)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return static_ent->u.action_data.item.forward_list;
}

/**
 * @brief		Get vlan from L2 static entry
 * @param[in]	static_ent Static entry
 * @return		Vlan of static entry
 */
__attribute__((pure)) uint16_t pfe_l2br_static_entry_get_vlan(const pfe_l2br_static_entry_t *static_ent)
{
	return static_ent->vlan;
}

/**
 * @brief		Get MAC from L2 static entry
 * @param[in]	static_ent Static entry
 * @return		Mac of static entry
 */
void pfe_l2br_static_entry_get_mac(const pfe_l2br_static_entry_t *static_ent, pfe_mac_addr_t mac)
{
	(void)memcpy(mac, static_ent->mac, sizeof(pfe_mac_addr_t));
}

/**
 * @brief		Get first L2 static entry based on criterion
 * @param[in]	bridge Bridge instance
 * @param[in]	crit Static entry to change forward list
 * @param[in]	arg1 Argument for criterion
 * @param[in]	arg2 Argument for criterion
 * @return		Static entry on success or NULL on failure
 */
pfe_l2br_static_entry_t *pfe_l2br_static_entry_get_first(pfe_l2br_t *bridge, pfe_l2br_static_ent_get_crit_t crit, void* arg1,const void *arg2)
{
	bool_t match = FALSE;
	LLIST_t *item;
	pfe_l2br_static_entry_t *static_ent;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	bridge->cur_crit_ent = crit;

	switch (bridge->cur_crit_ent)
	{
		case L2SENT_CRIT_ALL:
			break;
            
		case L2SENT_CRIT_BY_MAC:
			(void)memcpy((void *)bridge->cur_static_ent_crit_arg.mac, arg2, sizeof(pfe_mac_addr_t));
			break;
            
		case L2SENT_CRIT_BY_VLAN:
			bridge->cur_static_ent_crit_arg.vlan = (uint16_t)((addr_t)arg1);
			break;
            
		case L2SENT_CRIT_BY_MAC_VLAN:
			bridge->cur_static_ent_crit_arg.vlan = (uint16_t)((addr_t)arg1);
			(void)memcpy((void *)bridge->cur_static_ent_crit_arg.mac, arg2, sizeof(pfe_mac_addr_t));
			break;
            
        default:
            NXP_LOG_DEBUG("Invalid static entry type");
            break;
	}

	if (EOK != oal_mutex_lock(bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
	{
		/*	Get first matching entry */
		LLIST_ForEach(item, &bridge->static_entries)
		{
			/*	Get data */
			static_ent = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);

			/*	Remember current item to know where to start later */
			bridge->curr_static_ent = item->prNext;
			if (TRUE == pfe_l2br_static_entry_match_criterion(bridge, static_ent))
			{
				match = TRUE;
				break;
			}
		}
	}

	if (EOK != oal_mutex_unlock(bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	if (TRUE == match)
	{
		return static_ent;
	}
	else
	{
		return NULL;
	}
}
/**
 * @brief		Get next L2 static entry
 * @param[in]	bridge Bridge instance
 * @return		Static entry on success or NULL on failure
 * @warning		Intended to be called after pfe_l2br_static_entry_get_first.
 */
pfe_l2br_static_entry_t *pfe_l2br_static_entry_get_next(pfe_l2br_t *bridge)
{
	pfe_l2br_static_entry_t *static_ent;
	bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (bridge->curr_static_ent == &bridge->static_entries)
	{
		/*	No more entries */
		static_ent = NULL;
	}
	else
	{
		while (bridge->curr_static_ent != &bridge->static_entries)
		{
			/*	Get data */
			static_ent = LLIST_Data(bridge->curr_static_ent, pfe_l2br_static_entry_t, list_entry);

			/*	Remember current item to know where to start later */
			bridge->curr_static_ent = bridge->curr_static_ent->prNext;

			if (NULL != static_ent)
			{
				if (true == pfe_l2br_static_entry_match_criterion(bridge, static_ent))
				{
					match = TRUE;
					break;
				}
			}
		}
	}

	if (EOK != oal_mutex_unlock(bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return (TRUE == match) ? static_ent : NULL;
}

/**
 * @brief		Match static entry
 * @param[in]	bridge The bridge instance
 * @param[in]	static_ent Static entry to be matched to criterion parameters
 */
static bool_t pfe_l2br_static_entry_match_criterion(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t *static_ent)
{
	bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	switch (bridge->cur_crit_ent)
	{
		case L2SENT_CRIT_ALL:
			match = TRUE;
			break;

		case L2SENT_CRIT_BY_MAC:
			if (0 == memcmp(static_ent->mac, bridge->cur_static_ent_crit_arg.mac, sizeof(pfe_mac_addr_t)))
			{
				match = TRUE;
			}
			break;

		case L2SENT_CRIT_BY_VLAN:
			match = (static_ent->vlan == bridge->cur_static_ent_crit_arg.vlan);
			break;
            
		case L2SENT_CRIT_BY_MAC_VLAN:
			match = (static_ent->vlan == bridge->cur_static_ent_crit_arg.vlan);
			if (TRUE == match) {
				if (0 == memcmp(static_ent->mac, bridge->cur_static_ent_crit_arg.mac, sizeof(pfe_mac_addr_t)))
				{
					match = TRUE;
				}
				else
				{
					match = FALSE;
				}
			}
			break;

		default:
			NXP_LOG_ERROR("Unknown criterion\n");
			match = FALSE;
            break;
	}

	return match;
}

/*
 * @brief		Flush MAC table entries
 * @param[in]	bridge The bridge instance
 * @param[in]	type Type of the flush
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_l2br_flush(pfe_l2br_t *bridge, pfe_l2br_flush_types type)
{
	errno_t ret = EOK, query_ret;
	pfe_l2br_table_entry_t *entry;
	pfe_l2br_static_entry_t *sentry;
	pfe_l2br_table_iterator_t *l2t_iter;
	LLIST_t *item, *aux;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Create entry storage */
	entry = pfe_l2br_table_entry_create(bridge->mac_table);

	/*	 Create iterator */
	l2t_iter = pfe_l2br_iterator_create();

	if (EOK != oal_mutex_lock(bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	switch (type)
	{
		case PFE_L2BR_FLUSH_STATIC_MAC:
		{
			/*	Remove all static entries from local DB */
			if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
			{
				/*	Get first matching entry */
				LLIST_ForEachRemovable(item, aux, &bridge->static_entries)
				{
					/*	Get data */
					sentry = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);

					/*	Destroy entry. LLIST_Remove() is inside... */
					ret = pfe_l2br_static_entry_destroy_nolock(bridge, sentry);
					if (EOK != ret)
					{
						NXP_LOG_DEBUG("Unable to remove static entry: %d\n", ret);
					}
				}
			}

			break;
		}

		case PFE_L2BR_FLUSH_ALL_MAC:
		{
			/*	Remove all static entries from local DB. This must be done before
				the pfe_l2br_table_flush() because otherwise would report "entry
				not found" messages. */
			if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
			{
				/*	Get first matching entry */
				LLIST_ForEachRemovable(item, aux, &bridge->static_entries)
				{
					/*	Get data */
					sentry = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);

					/*	Destroy entry. LLIST_Remove() is inside... */
					ret = pfe_l2br_static_entry_destroy_nolock(bridge, sentry);
					if (EOK != ret)
					{
						NXP_LOG_DEBUG("Unable to remove static entry: %d\n", ret);
					}
				}
			}

			/*	Flush MAC table */

#if 0 /* AAVB-3136: THIS DOES NOT WORK. PFE GETS STUCK. */
			ret = pfe_l2br_table_flush(bridge->mac_table);
#else
			ret = pfe_l2br_table_init(bridge->mac_table);
#endif /* AAVB-3136 */
			if (EOK != ret)
			{
				NXP_LOG_ERROR("MAC table flush failed: %d\n", ret);
			}
			else
			{
				NXP_LOG_INFO("MAC table flushed\n");
			}

			break;
		}

		case PFE_L2BR_FLUSH_LEARNED_MAC:
		{
			/*	Go through all entries */
			query_ret = pfe_l2br_table_get_first(bridge->mac_table, l2t_iter, L2BR_TABLE_CRIT_VALID, entry);
			while (EOK == query_ret)
			{
				if (FALSE == pfe_l2br_table_entry_is_static(entry))
				{
					/*	Remove non-static entry from table */
					ret = pfe_l2br_table_del_entry(bridge->mac_table, entry);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Could not delete MAC table entry: %d\n", ret);
					}
				}

				query_ret = pfe_l2br_table_get_next(bridge->mac_table, l2t_iter, entry);
			}

			break;
		}

		default:
		{
			NXP_LOG_DEBUG("Invalid flush type");
			ret = EINVAL;
			break;
		}
	}

	if (EOK != oal_mutex_unlock(bridge->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	/*	Release entry storage */
	(void)pfe_l2br_table_entry_destroy(entry);

	/*  Release iterator */
	(void)pfe_l2br_iterator_destroy(l2t_iter);

	return ret;
}

/**
 * @brief		Flush all learned MAC table entries
 * @param[in]	bridge The bridge instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_flush_learned(pfe_l2br_t *bridge)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_l2br_flush(bridge, PFE_L2BR_FLUSH_LEARNED_MAC);
}

/**
 * @brief		Flush all static MAC table entries
 * @param[in]	bridge The bridge instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_flush_static(pfe_l2br_t *bridge)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_l2br_flush(bridge, PFE_L2BR_FLUSH_STATIC_MAC);
}

/**
 * @brief		Flush all MAC table entries
 * @param[in]	bridge The bridge instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_flush_all(pfe_l2br_t *bridge)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_l2br_flush(bridge, PFE_L2BR_FLUSH_ALL_MAC);
}

/**
 * @brief		Checks whether the firmware feature with given name is enabled
 * @param[in]	class Class instance to checks
 * @param[in]	name Name of the feature to check
 * @retval		TRUE Feature is enable
 * @retval		FALSE Feature is not enable
 */

static bool_t pfe_platform_class_feature_enabled(const pfe_class_t *class, const char *name)
{
	pfe_fw_feature_t *fw_feature;
	uint8_t variant = 0;
    bool_t retval = FALSE;
     
    /* Does the feature exist? */
	if(EOK == pfe_class_get_feature(class, &fw_feature, name))
	{   /* Feature exists */
		/* Get variant */
		if (EOK == pfe_fw_feature_get_variant(fw_feature, &variant))
        {
            if (variant == 1U)
            {
                retval = TRUE;
            }   
        }
	}

	return retval;
}

/**
 * @brief		Enable the firmware feature with given name
 * @param[in]	class Class instance
 * @param[in]	name Name of the feature to enable
 * @retval		EOK if success, error code otherwise
 */

static errno_t pfe_platform_class_feature_enable(const pfe_class_t *class, const char *name)
{
	pfe_fw_feature_t *fw_feature;
	errno_t ret = EINVAL;
	uint8_t variant = 0;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
    else
    {
#endif /* PFE_CFG_NULL_ARG_CHECK */

        /* Does the feature exist? */
        ret = pfe_class_get_feature(class, &fw_feature, name);
        if(EOK != ret)
        {
            NXP_LOG_ERROR("%s FW feature doesn't exist!\n", name);
        }
        else
        {
            /* Get feature variant */
            if (EOK == pfe_fw_feature_get_variant(fw_feature, &variant))
            {
                if (variant != 1U)
                {
                    NXP_LOG_ERROR("%s FW feature has invalid variant!\n", name);
                    ret = EINVAL;
                }
                else
                {
                    /* Enable feature */
                    ret = pfe_fw_feature_set_val(fw_feature, 1U);
                    if(EOK != ret)
                    {
                        NXP_LOG_ERROR("%s FW feature can't be enabled!\n", name);
                    }
                }
            }
        }
#if defined(PFE_CFG_NULL_ARG_CHECK)
    }
#endif

	return ret;
}

/**
 * @brief		Create L2 bridge instance
 * @param[in]	class The classifier instance
 * @param[in]	def_vlan Default VLAN
 * @param[in]	def_aging_time Default aging timeout in seconds.
 * @param[in]	mac_table The MAC table instance
 * @param[in]	vlan_table The VLAN table instance
 * @return		The instance or NULL if failed
 */
pfe_l2br_t *pfe_l2br_create(pfe_class_t *class, uint16_t def_vlan, uint16_t def_aging_time, pfe_l2br_table_t *mac_table, pfe_l2br_table_t *vlan_table)
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
		(void)memset(bridge, 0, sizeof(pfe_l2br_t));
		bridge->class = class;
		bridge->mac_table = mac_table;
		bridge->vlan_table = vlan_table;
		bridge->def_vlan = def_vlan;
		LLIST_Init(&bridge->domains);
		LLIST_Init(&bridge->static_entries);

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

		if (EOK != pfe_l2br_set_mac_aging_timeout(bridge->class, def_aging_time))
		{
			NXP_LOG_DEBUG("Could not set mac aging timeout\n");
			goto free_and_fail;
		}

		/*	If the FW aging is off, turn it on */
		if (pfe_platform_class_feature_enabled(bridge->class, "l2_bridge_aging")==FALSE)
		{
			if (EOK != pfe_platform_class_feature_enable(bridge->class, "l2_bridge_aging"))
			{
				NXP_LOG_ERROR("Could not enable L2 bridge aging FW feature\n");
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
	if (NULL != bridge)
	{
		if (NULL != bridge->default_domain)
		{
			if (EOK == pfe_l2br_domain_destroy(bridge->default_domain))
            {
                bridge->default_domain = NULL;
            }
			else
            {
                NXP_LOG_DEBUG("Could not destroy default domain\n");
            }
		}

		if (NULL != bridge->fallback_domain)
		{
			if (EOK == pfe_l2br_domain_destroy(bridge->fallback_domain))
            {
                bridge->fallback_domain = NULL;
            }
			else
            {
                NXP_LOG_DEBUG("Could not destroy fallback domain\n");
            }
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
__attribute__((pure)) pfe_l2br_domain_t *pfe_l2br_get_default_domain(const pfe_l2br_t *bridge)
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
__attribute__((pure)) pfe_l2br_domain_t *pfe_l2br_get_fallback_domain(const pfe_l2br_t *bridge)
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
static bool_t pfe_l2br_domain_match_criterion(const pfe_l2br_t *bridge, pfe_l2br_domain_t *domain)
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
			match = TRUE;
			break;

		case L2BD_CRIT_BY_VLAN:
			match = (domain->vlan == bridge->cur_domain_crit_arg.vlan);
			break;

		case L2BD_BY_PHY_IF:
			/*	Find out if domain contains given interface */
			if (FALSE == LLIST_IsEmpty(&domain->ifaces))
			{
				LLIST_ForEach(item, &domain->ifaces)
				{
					entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
					match = (((pfe_phy_if_t *)entry->ptr) == bridge->cur_domain_crit_arg.phy_if);
					if (TRUE == match)
					{
						break;
					}
				}
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
 * @brief		Get first L2 bridge domain instance according to given criterion
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	crit Get criterion
 * @param[in]	arg Pointer to criterion argument
 * @return		The domain instance or NULL if not found
 */
pfe_l2br_domain_t *pfe_l2br_get_first_domain(pfe_l2br_t *bridge, pfe_l2br_domain_get_crit_t crit, void *arg)
{
	LLIST_t *item;
	pfe_l2br_domain_t *domain = NULL;
	bool_t match = FALSE;
    bool_t known_crit = TRUE;

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
			break;

		case L2BD_CRIT_BY_VLAN:
			bridge->cur_domain_crit_arg.vlan = (uint16_t)((addr_t)arg & 0xffffU);
			break;

		case L2BD_BY_PHY_IF:
			bridge->cur_domain_crit_arg.phy_if = (pfe_phy_if_t *)arg;
			break;

		default:
			NXP_LOG_ERROR("Unknown criterion\n");
            known_crit = FALSE;
            break;
	}

	if ((FALSE == LLIST_IsEmpty(&bridge->domains)) && (TRUE == known_crit))
	{
		/*	Get first matching entry */
		LLIST_ForEach(item, &bridge->domains)
		{
			/*	Get data */
			domain = LLIST_Data(item, pfe_l2br_domain_t, list_entry);

			/*	Remember current item to know where to start later */
			bridge->curr_domain = item->prNext;
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

	if (bridge->curr_domain == &bridge->domains)
	{
		/*	No more entries */
		domain = NULL;
	}
	else
	{
		while (bridge->curr_domain != &bridge->domains)
		{
			/*	Get data */
			domain = LLIST_Data(bridge->curr_domain, pfe_l2br_domain_t, list_entry);

			/*	Remember current item to know where to start later */
			bridge->curr_domain = bridge->curr_domain->prNext;

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
 * @brief Configures the l2 bridge mac aging timeout
 * @param[in] class The classifier instance
 * @param[in] timeout Timeout time in seconds.
 * @return Either EOK or error code.
 */
static errno_t pfe_l2br_set_mac_aging_timeout(pfe_class_t *class, const uint16_t timeout)
{
	pfe_ct_class_mmap_t mmap;
	pfe_ct_misc_config_t misc_config;

	errno_t ret = EOK;
    uint32_t ff_addr;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	misc_config.l2_mac_aging_timeout = oal_htons(timeout);

    /* Get the memory map */
	/* All PEs share the same memory map therefore we can read
	   arbitrary one (in this case 0U)
	   Also mac aging algorithm will work only on core 0*/
	ret = pfe_class_get_mmap(class, 0U, &mmap);
	if(EOK == ret)
	{
        /* Get the misc address */
        ff_addr = oal_ntohl(mmap.common.misc_config);
        /* Write new address of misc config */
        ret = pfe_class_write_dmem(class, 0, (addr_t)ff_addr, (void *)&misc_config, sizeof(pfe_ct_misc_config_t));
    }
    return ret;
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
uint32_t pfe_l2br_get_text_statistics(const pfe_l2br_t *bridge, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
    uint32_t len = 0U;
    pfe_l2br_table_entry_t *entry;
    pfe_l2br_table_iterator_t* l2t_iter;
    errno_t ret;
    uint32_t count = 0U;

	/* We keep unused parameter verb_level for consistency with rest of the *_get_text_statistics() functions */
	(void)verb_level;
    
    /* Get memory */
    entry = pfe_l2br_table_entry_create(bridge->mac_table);
    /* Get the first entry */
    l2t_iter = pfe_l2br_iterator_create();

    ret = pfe_l2br_table_get_first(bridge->mac_table, l2t_iter, L2BR_TABLE_CRIT_VALID, entry);
    while (EOK == ret)
    {
        /* Print out the entry */
        len += pfe_l2br_table_entry_to_str(entry, buf + len, buf_len - len);
        count++;
        /* Get the next entry */
        ret = pfe_l2br_table_get_next(bridge->mac_table, l2t_iter, entry);
    }
    len += oal_util_snprintf(buf + len, buf_len - len, "\nEntries count: %u\n", count);
    /* Free memory */
    (void)pfe_l2br_table_entry_destroy(entry);
    (void)pfe_l2br_iterator_destroy(l2t_iter);
    return len;
}
