/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"

#ifndef PFE_CFG_PFE_SLAVE
#include "hal.h"
#include "linked_list.h"

#include "pfe_cbus.h"
#include "pfe_ct.h"
#include "pfe_log_if.h"
#include "pfe_pe.h"
#include "pfe_class.h"
#include "blalloc.h" /* Block allocator to assign interface IDs */
#include "pfe_platform_cfg.h"

struct pfe_log_if_tag
{
	pfe_phy_if_t *parent;			/*!< Parent physical interface */
	pfe_class_t *class;				/*!< Classifier */
	addr_t dmem_base;				/*!< Place in CLASS/DMEM where HW logical interface structure is stored */
	char_t *name;					/*!< Interface name */
	pfe_ct_log_if_t log_if_class;	/*!< Cached copy of the DMEM structure */
	pfe_mac_db_t *mac_db;			/* MAC database */
	oal_mutex_t lock;
};


/**
 * @brief	Pool of logical interface IDs. Module-local singleton.
 */
static blalloc_t *pfe_log_if_id_pool = NULL;

static errno_t pfe_log_if_read_from_class(const pfe_log_if_t *iface, pfe_ct_log_if_t *class_if, uint32_t pe_idx);
static errno_t pfe_log_if_write_to_class_nostats(const pfe_log_if_t *iface, pfe_ct_log_if_t *class_if);
static errno_t pfe_log_if_write_to_class(const pfe_log_if_t *iface, pfe_ct_log_if_t *class_if);

/**
 * @brief		Read interface structure from classifier memory
 * @param[in]	iface The interface instance
 * @param[in]	class_if Pointer where the structure shall be written
 * @param[in]	pe_idx Index of the PE which memory shall be read (statistic counters do differ of each PE)
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
static errno_t pfe_log_if_read_from_class(const pfe_log_if_t *iface, pfe_ct_log_if_t *class_if, uint32_t pe_idx)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_if) || (NULL == iface) || (0U == iface->dmem_base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*
		Read current interface configuration from classifier. Returned data will be in
		__NETWORK__ endian format.
	*/
	return pfe_class_read_dmem(iface->class, pe_idx, class_if, iface->dmem_base, sizeof(pfe_ct_log_if_t));
}

/**
 * @brief		Write interface structure to classifier memory skipping interface statistics
 * @param[in]	iface The interface instance
 * @param[in]	class_if Pointer to the structure to be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
static errno_t pfe_log_if_write_to_class_nostats(const pfe_log_if_t *iface, pfe_ct_log_if_t *class_if)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_if) || (NULL == iface) || (0U == iface->dmem_base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Be sure that class_stats are at correct place */
	ct_assert_offsetof((sizeof(pfe_ct_log_if_t) - sizeof(pfe_ct_class_algo_stats_t)) == offsetof(pfe_ct_log_if_t, class_stats));

	return pfe_class_write_dmem(iface->class, -1, iface->dmem_base, (void *)class_if,
							    sizeof(pfe_ct_log_if_t) - sizeof(pfe_ct_class_algo_stats_t));
}

/**
 * @brief		Write interface structure to classifier with statistics
 * @param[in]	iface The interface instance
 * @param[in]	class_if Pointer to the structure to be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
static errno_t pfe_log_if_write_to_class(const pfe_log_if_t *iface, pfe_ct_log_if_t *class_if)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_if) || (NULL == iface) || (0U == iface->dmem_base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return pfe_class_write_dmem(iface->class, -1, iface->dmem_base, (void *)class_if, sizeof(pfe_ct_log_if_t));
}

/**
 * @brief		Create new logical interface instance
 * @param[in]	parent The parent physical interface
 * @param[in]	name Name of the interface
 * @return		The interface instance or NULL if failed
 */
pfe_log_if_t *pfe_log_if_create(pfe_phy_if_t *parent, const char_t *name)
{
	pfe_log_if_t *iface;
	addr_t id;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == parent) || (NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL == pfe_log_if_id_pool)
	{
		/*	Create pool of logical interface IDs */
		pfe_log_if_id_pool = blalloc_create(PFE_CFG_MAX_LOG_IFS, 0U);
		if (NULL == pfe_log_if_id_pool)
		{
			NXP_LOG_ERROR("Unable to create pool of IDs\n");
			return NULL;
		}
		else
		{
			NXP_LOG_DEBUG("Pool configured to support %d logical interfaces\n", PFE_CFG_MAX_LOG_IFS);
		}
	}

	/*	Allocate interface ID */
	if (EOK != blalloc_alloc_offs(pfe_log_if_id_pool, 1, 0, &id))
	{
		NXP_LOG_ERROR("Could not allocate interface ID\n");
		return NULL;
	}

	iface = oal_mm_malloc(sizeof(pfe_log_if_t));
	if (NULL == iface)
	{
		return NULL;
	}
	else
	{
		(void)memset(iface, 0, sizeof(pfe_log_if_t));
		iface->parent = parent;
		iface->class = pfe_phy_if_get_class(parent);
		iface->mac_db = pfe_mac_db_create();
		if (NULL == iface->mac_db)
		{
			NXP_LOG_ERROR("Could not create MAC db\n");
			oal_mm_free(iface);
			return NULL;
		}

		if (EOK != oal_mutex_init(&iface->lock))
		{
			NXP_LOG_ERROR("Could not initialize mutex\n");
			(void)pfe_mac_db_destroy(iface->mac_db);
			oal_mm_free(iface);
			return NULL;
		}

		iface->name = oal_mm_malloc(strlen(name) + 1U);
		if (NULL == iface->name)
		{
			NXP_LOG_ERROR("Malloc failed\n");
			(void)pfe_mac_db_destroy(iface->mac_db);
			(void)oal_mutex_destroy(&iface->lock);
			oal_mm_free(iface);
			return NULL;
		}
		else
		{
			(void)strcpy(iface->name, name);
		}

		/* Get the DMEM for logical interface */
		iface->dmem_base = pfe_class_dmem_heap_alloc(iface->class, sizeof(pfe_ct_log_if_t));
		if(0U == iface->dmem_base)
		{
			NXP_LOG_ERROR("No DMEM\n");
			(void)pfe_mac_db_destroy(iface->mac_db);
			oal_mm_free(iface->name);
			(void)oal_mutex_destroy(&iface->lock);
			oal_mm_free(iface);
			return NULL;
		}

		/*	Initialize the local and CLASS logical interface structure */
		(void)memset(&iface->log_if_class, 0, sizeof(pfe_ct_log_if_t));
		iface->log_if_class.next = 0;
		iface->log_if_class.id = (uint8_t)(id & 0xffU);
		iface->log_if_class.m_rules = (pfe_ct_if_m_rules_t)IF_MATCH_NONE;

		/* Be sure that statistics are zeroed (endianness doesn't mater for this) */
		iface->log_if_class.class_stats.accepted  = 0;
		iface->log_if_class.class_stats.rejected  = 0;
		iface->log_if_class.class_stats.discarded = 0;
		iface->log_if_class.class_stats.processed = 0;

		/* Write to class with stats (overriding the statistics with 0) */
		if (EOK != pfe_log_if_write_to_class(iface, &iface->log_if_class))
		{
			NXP_LOG_ERROR("Could not update DMEM (%s)\n", iface->name);

			if (EOK != pfe_phy_if_del_log_if(parent, iface))
			{
				NXP_LOG_DEBUG("Could not delete %s from %s\n", iface->name, pfe_phy_if_get_name(parent));
			}
			pfe_class_dmem_heap_free(iface->class, iface->dmem_base);
			(void)pfe_mac_db_destroy(iface->mac_db);
			oal_mm_free(iface->name);
			(void)oal_mutex_destroy(&iface->lock);
			oal_mm_free(iface);
			return NULL;
		}

		/*	Bind logical IF with physical IF */
		if (EOK != pfe_phy_if_add_log_if(parent, iface))
		{
			NXP_LOG_ERROR("Can't bind %s to %s\n", iface->name, pfe_phy_if_get_name(parent));
			pfe_class_dmem_heap_free(iface->class, iface->dmem_base);
			(void)pfe_mac_db_destroy(iface->mac_db);
			oal_mm_free(iface->name);
			(void)oal_mutex_destroy(&iface->lock);
			oal_mm_free(iface);
			return NULL;
		}
	};

	return iface;
}

/**
 * @brief		Get interface ID
 * @param[in]	iface The interface instance
 * @return		The interface ID
 */
__attribute__((pure)) uint8_t pfe_log_if_get_id(const pfe_log_if_t *iface)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0xffU;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return iface->log_if_class.id;
}

/**
 * @brief		Get parent physical interface
 * @param[in]	iface The interface instance
 * @return		Physical interface instance or NULL if failed
 */
__attribute__((pure)) pfe_phy_if_t *pfe_log_if_get_parent(const pfe_log_if_t *iface)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return iface->parent;
}

/**
 * @brief		Set 'next' pointer of the logical interface
 * @details		The value is used to form a simple linked list of logical interface structures
 * 				within the classifier memory. Classifier can then walk through the list with
 * 				every packet, try to find a matching logical interface, and perform subsequent
 * 				actions (for instance distribute the packet to the right destination given by
 * 				the logical interface configuration). Note that last entry in the list shall
 * 				have the 'next' value set to zero.
 * @param[in]	iface The interface instance
 * @param[in]	next_dmem_ptr Addr in DMEM where next logical interface is stored (lined list entry)
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_set_next_dmem_ptr(pfe_log_if_t *iface, addr_t next_dmem_ptr)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	iface->log_if_class.next = oal_htonl((uint32_t)next_dmem_ptr);
	if (EOK != pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class))
	{
		NXP_LOG_ERROR("Interface update failed\n");
		ret = ENOEXEC;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get 'next' pointer of the logical interface (DMEM)
 * @param[in]	iface The interface instance
 * @param[in]	next_dmem_ptr Pointer where the value shall be stored
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_get_next_dmem_ptr(pfe_log_if_t *iface, addr_t *next_dmem_ptr)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == next_dmem_ptr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	*next_dmem_ptr = oal_ntohl(iface->log_if_class.next);

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Get pointer to logical interface within DMEM
 * @param[in]	iface The interface instance (HOST)
 * @param[in]	dmem_base Pointer where the interface instance (DMEM) pointer will be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_get_dmem_base(const pfe_log_if_t *iface, addr_t *dmem_base)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == dmem_base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	*dmem_base = iface->dmem_base;
	return EOK;
}

/**
 * @brief		Destroy interface instance
 * @param[in]	iface The interface instance
 */
void pfe_log_if_destroy(pfe_log_if_t *iface)
{
	errno_t ret;

	if (NULL != iface)
	{
		ret = pfe_mac_db_destroy(iface->mac_db);
		if (EOK != ret)
		{
			NXP_LOG_WARNING("unable to destroy MAC database: %d\n", ret);
		}

		ret = pfe_phy_if_del_log_if(iface->parent, iface);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Could not remove %s from parent instance: %d\n", iface->name, ret);
		}

		if (NULL != iface->name)
		{
			oal_mm_free(iface->name);
			iface->name = NULL;
		}

		/*	Release the interface ID */
		blalloc_free_offs(pfe_log_if_id_pool, iface->log_if_class.id);

		(void)memset(&iface->log_if_class, 0, sizeof(pfe_ct_log_if_t));
		if (EOK != pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class))
		{
			NXP_LOG_ERROR("Iface invalidation failed\n");
		}

		if (NULL_ADDR != iface->dmem_base)
		{
			pfe_class_dmem_heap_free(iface->class, iface->dmem_base);
		}

		if (EOK != oal_mutex_destroy(&iface->lock))
		{
			NXP_LOG_DEBUG("Could not destroy mutex\n");
		}

		oal_mm_free(iface);
	}
}

/**
 * @brief		Check if match is OR
 * @details		Set new match rules. All previously configured ones will be
 * 				overwritten.
 * @param[in]	iface The interface instance
 * @retval		TRUE match is OR type
 * @retval		FALSE match is AND type
 */
bool_t pfe_log_if_is_match_or(pfe_log_if_t *iface)
{
	bool_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = ((uint32_t)IF_FL_MATCH_OR ==
			((uint32_t)(oal_ntohl(iface->log_if_class.flags)) & (uint32_t)IF_FL_MATCH_OR));

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Set match type to OR match
 * @details		Logical interface rules will be matched with OR logic
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_set_match_or(pfe_log_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags |= oal_htonl(IF_FL_MATCH_OR);

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Set match type to AND match
 * @details		Logical interface rules will be matched with AND logic
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_set_match_and(pfe_log_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags &= oal_htonl(~(uint32_t)IF_FL_MATCH_OR);

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Set match rules
 * @details		Set new match rules. All previously configured ones will be
 * 				overwritten.
 * @param[in]	iface The interface instance
 * @param[in]	rules Rules to be set. See pfe_ct_if_m_rules_t.
 * @param[in]	args Pointer to the structure with arguments.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_set_match_rules(pfe_log_if_t *iface, pfe_ct_if_m_rules_t rules, const pfe_ct_if_m_args_t *args)
{
	errno_t ret = EOK;
	pfe_ct_if_m_rules_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL == args)
	{
		/*	Argument is mandatory */
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	/*	Copy the argument */
	(void)memcpy(&iface->log_if_class.m_args, args, sizeof(pfe_ct_if_m_args_t));

	/*	Backup current rules to temporary variable */
	tmp = iface->log_if_class.m_rules;
	iface->log_if_class.m_rules = (pfe_ct_if_m_rules_t)oal_htonl(rules);
	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.m_rules = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Add match rule
 * @param[in]	iface The interface instance
 * @param[in]	rule Rule to be added. See pfe_ct_if_m_rules_t. Function accepts
 * 					 only single rule per call.
 * @param[in]	arg Pointer to buffer containing rule argument data. The argument
 * 					data shall be in network byte order. Type of the argument can
 * 					be retrieved from the pfe_ct_if_m_args_t.
 * @param[in]	arg_len Length of the rule argument. Due to sanity check.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_add_match_rule(pfe_log_if_t *iface, pfe_ct_if_m_rules_t rule, const void *arg, uint32_t arg_len)
{
	errno_t ret = EINVAL;
	pfe_ct_if_m_rules_t tmp;
	pfe_ct_if_m_args_t m_args;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (0U == (uint32_t)rule)
	{
		return EINVAL;
	}

	/*	Check if only single rule is requested */
	if (0U != ((uint32_t)rule & ((uint32_t)rule-1U)))
	{
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	/*	Validate and copy argument */
	switch (rule)
	{
		case IF_MATCH_VLAN:
		{
			if (arg_len == sizeof(m_args.vlan))
			{
				iface->log_if_class.m_args.vlan = *((uint16_t *)arg);
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_PROTO:
		{
			if (arg_len == sizeof(m_args.proto))
			{
				iface->log_if_class.m_args.proto = *((uint8_t *)arg);
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_SPORT:
		{
			if (arg_len == sizeof(m_args.sport))
			{
				iface->log_if_class.m_args.sport = *((uint16_t *)arg);
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_DPORT:
		{
			if (arg_len == sizeof(m_args.dport))
			{
				iface->log_if_class.m_args.dport = *((uint16_t *)arg);
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_SIP6:
		{
			if (arg_len == sizeof(m_args.ipv.v6.sip))
			{
				(void)memcpy((void*)(iface->log_if_class.m_args.ipv.v6.sip), (const void*)arg, sizeof(m_args.ipv.v6.sip));
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_DIP6:
		{
			if (arg_len == sizeof(m_args.ipv.v6.dip))
			{
				(void)memcpy((void*)(iface->log_if_class.m_args.ipv.v6.dip), (const void*)arg, sizeof(m_args.ipv.v6.dip));
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_SIP:
		{
			if (arg_len == sizeof(m_args.ipv.v4.sip))
			{
				(void)memcpy((void*)(&iface->log_if_class.m_args.ipv.v4.sip), (const void*)arg, sizeof(m_args.ipv.v4.sip));
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_DIP:
		{
			if (arg_len == sizeof(m_args.ipv.v4.dip))
			{
				(void)memcpy((void*)(&iface->log_if_class.m_args.ipv.v4.dip), (const void*)arg, sizeof(m_args.ipv.v4.dip));
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_ETHTYPE:
		{
			if (arg_len == sizeof(m_args.ethtype))
			{
				iface->log_if_class.m_args.ethtype = *((uint16_t *)arg);
				ret = EOK;
			}

			break;
		}
		case IF_MATCH_FP0:
		{
			if (arg_len == sizeof(m_args.fp0_table))
			{
				iface->log_if_class.m_args.fp0_table = *((PFE_PTR(pfe_ct_fp_table_t) *)arg);
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_FP1:
		{
			if (arg_len == sizeof(m_args.fp1_table))
			{
				iface->log_if_class.m_args.fp1_table = *((PFE_PTR(pfe_ct_fp_table_t) *)arg);
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_SMAC:
		{
			if (arg_len == sizeof(m_args.smac))
			{
				(void)memcpy((void*)(iface->log_if_class.m_args.smac), (const void*)arg, sizeof(m_args.smac));
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_DMAC:
		{
			if (arg_len == sizeof(m_args.dmac))
			{
				(void)memcpy((void*)(iface->log_if_class.m_args.dmac), (const void*)arg, sizeof(m_args.dmac));
				ret = EOK;
			}

			break;
		}

		case IF_MATCH_HIF_COOKIE:
		{
			if (arg_len == sizeof(m_args.hif_cookie))
			{
				iface->log_if_class.m_args.hif_cookie = *((uint32_t *)arg);
				ret = EOK;
			}

			break;
		}

		default:
		{
			if (arg_len != 0U)
			{
				NXP_LOG_DEBUG("Unexpected argument\n");
			}
			else
			{
				ret = EOK;
			}
			break;
		}
	}

	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Invalid matching rule argument\n");
	}
	else
	{
		tmp = iface->log_if_class.m_rules;
		iface->log_if_class.m_rules |= (pfe_ct_if_m_rules_t)oal_htonl(rule);
		ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
		if (EOK != ret)
		{
			/*	Revert */
			iface->log_if_class.m_rules = tmp;
		}
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Delete match rule(s)
 * @param[in]	iface The interface instance
 * @param[in]	rule Rule or multiple rules to be deleted. See pfe_ct_if_m_rules_t.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_del_match_rule(pfe_log_if_t *iface, pfe_ct_if_m_rules_t rule)
{
	errno_t ret = EOK;
	pfe_ct_if_m_rules_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.m_rules;
	iface->log_if_class.m_rules &= (pfe_ct_if_m_rules_t)oal_htonl(~rule);
	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.m_rules = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get match rules
 * @param[in]	iface The interface instance
 * @param[in]	rules Pointer to location where rules shall be written
 * @param[in]	args Pointer to location where rules arguments shall be written. Can be NULL.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_get_match_rules(pfe_log_if_t *iface, pfe_ct_if_m_rules_t *rules, pfe_ct_if_m_args_t *args)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == rules)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	*rules = (pfe_ct_if_m_rules_t)oal_ntohl(iface->log_if_class.m_rules);

	if (NULL != args)
	{
		(void)memcpy(args, &iface->log_if_class.m_args, sizeof(pfe_ct_if_m_args_t));
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Add new MAC address
 * @param[in]	iface The interface instance
 * @param[in]	addr The MAC address to add
 * @param[in]	owner The identification of driver instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_add_mac_addr(pfe_log_if_t *iface, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = pfe_mac_db_add_addr(iface->mac_db, addr, owner);
	if(EOK == ret)
	{
		/*	Configure underlying physical interface */
		if (EOK != pfe_phy_if_add_mac_addr(iface->parent, addr, owner))
		{
			NXP_LOG_ERROR("Could not add MAC address (%s, parent: %s)\n", iface->name, pfe_phy_if_get_name(iface->parent));
			/* Delete the MAC address from database */
			ret = pfe_mac_db_del_addr(iface->mac_db, addr, owner);
			if (EOK != ret) {
				NXP_LOG_ERROR("Unable to delete MAC address: %d\n", ret);
			}
			ret = ENOEXEC;
		}
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
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
 * @retval		ENOENT MAC address not found in local database
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_del_mac_addr(pfe_log_if_t *iface, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = pfe_mac_db_del_addr(iface->mac_db, addr, owner);
	if(EOK != ret)
	{
		NXP_LOG_WARNING("Unable to remove MAC address from log_if MAC database: %d\n", ret);
	}
	else
	{
		ret = pfe_phy_if_del_mac_addr(iface->parent, addr, owner);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Unable to del MAC address: %d\n", ret);

			/* Removal of MAC address from phy failed, put it back to DB */
			ret = pfe_mac_db_add_addr(iface->mac_db, addr, owner);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Unable to put back the MAC address into log_if MAC database: %d\n", ret);
			}
		}
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get handle of internal MAC database
 * @param[in]	iface The interface instance
 * @retval		Database handle.
 */
pfe_mac_db_t *pfe_log_if_get_mac_db(const pfe_log_if_t *iface)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return iface->mac_db;
}

/**
 * @brief		Get associated MAC address (first in the list of MAC addresses)
 * @param[in]	iface The interface instance
 * @param[out]	addr Where to copy to address
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT No address assigned
 */
errno_t pfe_log_if_get_mac_addr(pfe_log_if_t *iface, pfe_mac_addr_t addr)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = pfe_mac_db_get_first_addr(iface->mac_db, MAC_DB_CRIT_ALL, PFE_TYPE_ANY, PFE_CFG_LOCAL_IF, addr);
	if(EOK != ret)
	{
		NXP_LOG_WARNING("unable to get MAC address: %d\n", ret);
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Delete MAC addresses added by owner with defined type
 * @param[in]	iface The interface instance
 * @param[in]	crit All, Owner, Type or Owner&Type criterion
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @param[in]	owner Required owner of MAC address
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_flush_mac_addrs(pfe_log_if_t *iface, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	/*	Send command to underlying physical interface */
	if (EOK != pfe_phy_if_flush_mac_addrs(iface->parent, crit, type, owner))
	{
		NXP_LOG_ERROR("Could not flush MAC addresses (%s, parent: %s, owner ID: %d)\n", iface->name, pfe_phy_if_get_name(iface->parent), owner);
		ret = ENOEXEC;
	}
	else
	{
		/*	Remove associated MAC addresses due to flush mode */
		ret = pfe_mac_db_flush(iface->mac_db, crit, type, owner);
		if(EOK != ret)
		{
			NXP_LOG_ERROR("Unable to flush MAC address from phy_if MAC database: %d\n", ret);
		}
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief			Get mask of egress interfaces
 * @param[in]		iface The interface instance
 * @param[in,out]	egress mask (in host format), constructed like
 * 					egress |= 1 << phy_if_id (for each configured phy_if)
 * @retval			EOK Success
 */
errno_t pfe_log_if_get_egress_ifs(pfe_log_if_t *iface, uint32_t *egress)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == egress)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	*egress = (uint32_t)oal_ntohl(iface->log_if_class.e_phy_ifs);

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief			Set mask of egress interfaces
 * @param[in]		iface The interface instance
 * @param[in]		egress mask (in host format), constructed like
 * 					egress |= 1 << phy_if_id (for each configured phy_if)
 * @retval			EOK Success
 */
errno_t pfe_log_if_set_egress_ifs(pfe_log_if_t *iface, uint32_t egress)
{
	uint32_t tmp;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = oal_ntohl(iface->log_if_class.e_phy_ifs);

	iface->log_if_class.e_phy_ifs = oal_htonl(egress);
	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.e_phy_ifs = oal_htonl(tmp);
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Add egress physical interface
 * @details		Logical interfaces can be used to classify and route
 * 				packets. When ingress packet is not classified using any
 * 				other classification mechanism (L3 router, L2 bridge, ...)
 * 				then matching ingress logical interface is considered
 * 				to provide list of physical interfaces the packet shall be
 * 				forwarded to. This function provides way to add physical
 * 				interface into the list.
 * @param[in]	iface The interface instance
 * @param[in]	phy_if The physical interface to be added to the list of
 * 					   egress interfaces.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_add_egress_if(pfe_log_if_t *iface, const pfe_phy_if_t *phy_if)
{
	errno_t ret = EOK;
	uint32_t tmp;
	pfe_ct_phy_if_id_t phy_if_id;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == phy_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	phy_if_id = pfe_phy_if_get_id(phy_if);
	if (PFE_PHY_IF_ID_INVALID <= phy_if_id)
	{
		NXP_LOG_ERROR("Invalid PHY IF ID\n");
		return EINVAL;
	}

	tmp = oal_ntohl(iface->log_if_class.e_phy_ifs);

	iface->log_if_class.e_phy_ifs = oal_htonl(tmp | (1UL << (uint8_t)phy_if_id));
	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.e_phy_ifs = oal_htonl(tmp);
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Remove egress physical interface
 * @details		See the pfe_log_if_add_egress_if().
 * @param[in]	iface The interface instance
 * @param[in]	phy_if The physical interface to be removed
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_del_egress_if(pfe_log_if_t *iface, const pfe_phy_if_t *phy_if)
{
	errno_t ret = EOK;
	uint32_t tmp;
	pfe_ct_phy_if_id_t phy_if_id;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == phy_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	phy_if_id = pfe_phy_if_get_id(phy_if);
	if (PFE_PHY_IF_ID_INVALID <= phy_if_id)
	{
		NXP_LOG_ERROR("Invalid PHY IF ID\n");
		return EINVAL;
	}

	tmp = oal_ntohl(iface->log_if_class.e_phy_ifs);

	iface->log_if_class.e_phy_ifs = oal_htonl(tmp & (uint32_t)(~(1UL << (uint8_t)phy_if_id)));
	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.e_phy_ifs = oal_htonl(tmp);
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Enable the interface
 * @details		Only enabled logical interfaces will be used by firmware
 * 				to match ingress frames.
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_enable(pfe_log_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	NXP_LOG_DEBUG("Enabling %s\n", iface->name);

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp | oal_htonl(IF_FL_ENABLED));

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	if (EOK == ret)
	{
		/*	Enable the underlying physical interface */
		ret = pfe_phy_if_enable(iface->parent);
		if (EOK != ret)
		{
			/*	Revert */
			if (EOK != oal_mutex_lock(&iface->lock))
			{
				NXP_LOG_DEBUG("mutex lock failed\n");
			}

			iface->log_if_class.flags = tmp;
			ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Could not revert DMEM change\n");
			}

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_DEBUG("mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief		Disable the interface
 * @details		Only enabled logical interfaces will be used by firmware
 * 				to match ingress frames.
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_disable(pfe_log_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	NXP_LOG_DEBUG("Disabling %s\n", iface->name);

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp & (oal_htonl(~(uint32_t)IF_FL_ENABLED)));

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	if (EOK == ret)
	{
		/*	Disable the underlying physical interface */
		ret = pfe_phy_if_disable(iface->parent);
		if (EOK != ret)
		{
			/*	Revert */
			if (EOK != oal_mutex_lock(&iface->lock))
			{
				NXP_LOG_DEBUG("mutex lock failed\n");
			}

			iface->log_if_class.flags = tmp;
			ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Could not revert DMEM change\n");
			}

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_DEBUG("mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief		Check if interface is enabled
 * @param[in]	iface The interface instance
 * @return		TRUE if enabled, FALSE otherwise
 */
__attribute__((pure)) bool_t pfe_log_if_is_enabled(pfe_log_if_t *iface)
{
	bool_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = (0U != ((uint32_t)(oal_ntohl(iface->log_if_class.flags)) & (uint32_t)IF_FL_ENABLED));

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief               Enable loopback mode
 * @details             Function sets logical interface to loopback mode and
 *                              also enables loopback mode on underlying physical
 *                              interface.
 * @param[in]   iface The interface instance
 * @retval              EOK Success
 * @retval              EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_loopback_enable(pfe_log_if_t *iface)
{
	errno_t ret = EOK;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp | IF_FL_LOOPBACK);

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*      Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	if (EOK == ret)
	{
		/*      Enable the underlying physical interface */
		ret = pfe_phy_if_loopback_enable(iface->parent);
		if (EOK != ret)
		{
			/*      Revert */
			if (EOK != oal_mutex_lock(&iface->lock))
			{
				NXP_LOG_DEBUG("mutex lock failed\n");
			}

			iface->log_if_class.flags &= ~IF_FL_LOOPBACK;
			ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Could not revert DMEM change\n");
			}

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_DEBUG("mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief               Disable loopback mode
 * @details             Function sets logical interface to loopback mode and
 *                              also disable loopback mode on underlying physical
 *                              interface.
 * @param[in]   iface The interface instance
 * @retval              EOK Success
 * @retval              EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_loopback_disable(pfe_log_if_t *iface)
{
	errno_t ret = EOK;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp & ~(uint32_t)IF_FL_LOOPBACK);
	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*      Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	if (EOK == ret)
	{
		/*      Disable the underlying physical interface */
		ret = pfe_phy_if_loopback_disable(iface->parent);
		if (EOK != ret)
		{
			/*      Revert */
			if (EOK != oal_mutex_lock(&iface->lock))
			{
				NXP_LOG_DEBUG("mutex lock failed\n");
			}
			iface->log_if_class.flags |= IF_FL_LOOPBACK;
			ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Could not revert DMEM change\n");
			}

			if (EOK != oal_mutex_unlock(&iface->lock))
			{
				NXP_LOG_DEBUG("mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief               Check if interface is in loopback mode
 * @param[in]   iface The interface instance
 * @return              TRUE if loopback mode is enabled, FALSE otherwise
 */
__attribute__((pure)) bool_t pfe_log_if_is_loopback(pfe_log_if_t *iface)
{
	bool_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = (0 != (iface->log_if_class.flags & IF_FL_LOOPBACK));

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Enable promiscuous mode
 * @details		Function sets logical interface to promiscuous mode and
 * 				also enables promiscuous mode on underlying physical
 * 				interface.
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_promisc_enable(pfe_log_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags |= oal_htonl(IF_FL_PROMISC);

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Disable promiscuous mode
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_promisc_disable(pfe_log_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp & (oal_htonl(~(uint32_t)IF_FL_PROMISC)));

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Check if interface is in promiscuous mode
 * @param[in]	iface The interface instance
 * @return		TRUE if promiscuous mode is enabled, FALSE otherwise
 */
__attribute__((pure)) bool_t pfe_log_if_is_promisc(pfe_log_if_t *iface)
{
	bool_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = (0U != ((uint32_t)(oal_ntohl(iface->log_if_class.flags)) & (uint32_t)IF_FL_PROMISC));

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Enable ALLMULTI mode
 * @details		Function sets logical interface to ALLMULTI mode and
 * 				also enables ALLMULTI mode on underlying physical
 * 				interface.
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_allmulti_enable(const pfe_log_if_t *iface)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Propagate the mode to the underlying physical interface */
	ret = pfe_phy_if_allmulti_enable(iface->parent);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Could not enable the ALLMULTI mode\n");
	}

	return ret;
}

/**
 * @brief		Disable ALLMULTI mode
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_allmulti_disable(const pfe_log_if_t *iface)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Propagate the mode to the underlying physical interface */
	ret = pfe_phy_if_allmulti_disable(iface->parent);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Could not disable the ALLMULTI mode\n");
	}

	return ret;
}

/**
 * @brief		Enable discarding frames accepted by logical interface
 * @details		Function configures logical interface to discard all accepted frames instead of
 *              passing them to the configured egress interfaces.
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_discard_enable(pfe_log_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp | oal_htonl(IF_FL_DISCARD));

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Disable discarding frames accepted by logical interface
 * @details		Function configures logical interface to stop to discard all accepted frames
 *              and to pass them to the configured egress interfaces.
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_log_if_discard_disable(pfe_log_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_flags_t tmp;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	tmp = iface->log_if_class.flags;
	iface->log_if_class.flags = (pfe_ct_if_flags_t)((uint32_t)tmp & oal_htonl(~(uint32_t)IF_FL_DISCARD));

	ret = pfe_log_if_write_to_class_nostats(iface, &iface->log_if_class);
	if (EOK != ret)
	{
		/*	Revert */
		iface->log_if_class.flags = tmp;
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Check if interface is configured to discard accepted frames
 * @param[in]	iface The interface instance
 * @return		TRUE if discarding is enabled, FALSE otherwise
 */
__attribute__((pure)) bool_t pfe_log_if_is_discard(pfe_log_if_t *iface)
{
	bool_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = (0U != ((uint32_t)(oal_ntohl(iface->log_if_class.flags)) & (uint32_t)IF_FL_DISCARD));

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get interface name
 * @param[in]	iface The interface instance
 * @return		Pointer to name string or NULL if failed/not found
 */
__attribute__((pure)) const char_t *pfe_log_if_get_name(const pfe_log_if_t *iface)
{
    static const char_t *unknown = "(unknown)";

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return ((NULL != iface)? iface->name : unknown);
}

/**
 * @brief		Get log interface statistics
 * @param[in]	iface The interface instance
 * @param[out]	stat Statistic structure
 * @retval		EOK Success
 * @retval		NOMEM Not possible to allocate memory for read
 */
errno_t pfe_log_if_get_stats(const pfe_log_if_t *iface, pfe_ct_class_algo_stats_t *stat)
{
	uint32_t i = 0;
	errno_t ret = EOK;
	addr_t offset = 0;
	uint32_t buff_len = 0;
	pfe_ct_class_algo_stats_t * stats = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == stat)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)memset(stat,0,sizeof(pfe_ct_class_algo_stats_t));

	/* Store offset to stats */
	offset = offsetof(pfe_ct_log_if_t,class_stats);

	/* Prepare memory */
	buff_len = sizeof(pfe_ct_class_algo_stats_t) * pfe_class_get_num_of_pes(iface->class);
	stats = oal_mm_malloc(buff_len);
	if(NULL == stats)
	{
		return ENOMEM;
	}
	/* Gather memory from all PEs*/
	ret = pfe_class_gather_read_dmem(iface->class, stats, (iface->dmem_base + offset), buff_len, sizeof(pfe_ct_class_algo_stats_t));

	/* Calculate total statistics */
	for(i = 0U; i < pfe_class_get_num_of_pes(iface->class); i++)
	{
		/* Store statistics */
		stat->accepted	+= oal_ntohl(stats[i].accepted);
		stat->discarded	+= oal_ntohl(stats[i].discarded);
		stat->processed	+= oal_ntohl(stats[i].processed);
		stat->rejected	+= oal_ntohl(stats[i].rejected);
	}
	oal_mm_free(stats);

	/* Convert statistics back to network endian */
	stat->accepted	= oal_htonl(stat->accepted);
	stat->discarded	= oal_htonl(stat->discarded);
	stat->processed	= oal_htonl(stat->processed);
	stat->rejected	= oal_htonl(stat->rejected);

	return ret;
}

/**
 * @brief		Return logical interface runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	iface 		The logical interface instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_log_if_get_text_statistics(const pfe_log_if_t *iface, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;
	pfe_ct_log_if_t log_if_class = {0U};
	bool_t printed_rules = FALSE;
	uint32_t i;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	/* Repeat read for all PEs (just because of statistics) */
	for(i = 0U; i < pfe_class_get_num_of_pes(iface->class); i++)
	{

		if (EOK != pfe_log_if_read_from_class(iface, &log_if_class, i))
		{
			len += oal_util_snprintf(buf + len, buf_len - len, "[LogIF @ p0x%p]: Unable to read PE %u DMEM\n", (void *)iface->dmem_base, i);
		}
		else
		{
			if(FALSE == printed_rules)
			{
				len += oal_util_snprintf(buf + len, buf_len - len, "[LogIF '%s' @ p0x%p]\n", pfe_log_if_get_name(iface), (void *)iface->dmem_base);
				len += oal_util_snprintf(buf + len, buf_len - len, "Match Rules: 0x%x\n", oal_ntohl(log_if_class.m_rules));
				len += oal_util_snprintf(buf + len, buf_len - len, "Mode       : 0x%x\n", log_if_class.mode);
				len += oal_util_snprintf(buf + len, buf_len - len, "Flags      : 0x%x\n", oal_ntohl(log_if_class.flags));
				printed_rules = TRUE; /* Avoid printing it multiple times*/
			}
			len += oal_util_snprintf(buf + len, buf_len - len, "- Statistics from PE %u -\n", i);
			len += pfe_pe_stat_to_str(&log_if_class.class_stats, buf + len, buf_len - len, verb_level);
		}
	}

	return len;
}

#endif /* ! PFE_CFG_PFE_SLAVE */
