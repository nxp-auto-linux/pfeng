/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @file		pfe_log_if_slave.c
 * @brief		The PFE logical interface module source file (slave)
 * @details		TThis file contains logical interface-related functionality for
 *				the slave driver variant. All logical interface instance
 *				manipulation is done via RPC in way that local driver only
 *				sends requests to master driver which performs the actual
 *				requested operations.
 *
 */

#include "pfe_cfg.h"
#ifdef PFE_CFG_PFE_SLAVE

#include "oal.h"
#include "hal.h"

#include "pfe_ct.h"
#include "pfe_log_if.h"
#include "pfe_idex.h" /* The RPC provider */
#include "pfe_platform_rpc.h" /* The RPC codes and data structures */

struct __pfe_log_if_tag
{
	pfe_phy_if_t *parent;			/*!< Parent physical interface */
	char_t *name;					/*!< Interface name */
	uint8_t id;						/*!< Interface ID */
	oal_mutex_t lock;
};

static errno_t pfe_log_if_db_lock(void)
{
	errno_t ret;

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_IF_LOCK, NULL, 0, NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Unable to lock interface DB: %d\n", ret);
	}

	return ret;
}

static errno_t pfe_log_if_db_unlock(void)
{
	errno_t ret;

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_IF_UNLOCK, NULL, 0, NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Unable to lock interface DB: %d\n", ret);
	}

	return ret;
}

/**
 * @brief		Create new logical interface instance
 * @param[in]	parent The parent physical interface
 * @param[in]	name Name of the interface
 * @return		The interface instance or NULL if failed
 */
pfe_log_if_t *pfe_log_if_create(pfe_phy_if_t *parent, char_t *name)
{
	pfe_platform_rpc_pfe_log_if_create_arg_t arg = {0U};
	pfe_platform_rpc_pfe_log_if_create_ret_t rpc_ret = {0U};
	pfe_log_if_t *iface;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == parent) || (NULL == name)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)pfe_log_if_db_lock();

	/*	Ask master to create new logical interface */
	arg.phy_if_id = pfe_phy_if_get_id(parent);
	strncpy(arg.name, name, PFE_RPC_MAX_IF_NAME_LEN-1U);
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_CREATE, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));

	(void)pfe_log_if_db_unlock();

	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't create logical interface: %d\n", ret);
		return NULL;
	}

	iface = oal_mm_malloc(sizeof(pfe_log_if_t));
	if (NULL == iface)
	{
		NXP_LOG_ERROR("Malloc failed\n");
		return NULL;
	}
	else
	{
		memset(iface, 0, sizeof(pfe_log_if_t));
		iface->parent = parent;
		iface->id = rpc_ret.log_if_id;

		if (EOK != oal_mutex_init(&iface->lock))
		{
			NXP_LOG_ERROR("Could not initialize mutex\n");
			oal_mm_free(iface);
			return NULL;
		}

		iface->name = oal_mm_malloc(strlen(name) + 1U);
		if (NULL == iface->name)
		{
			NXP_LOG_ERROR("Malloc failed\n");
			oal_mutex_destroy(&iface->lock);
			oal_mm_free(iface);
			return NULL;
		}
		else
		{
			strcpy(iface->name, name);
		}
	}

	return iface;
}

/**
 * @brief		Get interface ID
 * @param[in]	iface The interface instance
 * @return		The interface ID
 */
__attribute__((pure)) uint8_t pfe_log_if_get_id(pfe_log_if_t *iface)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0xffU;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return iface->id;
}

/**
 * @brief		Get parent physical interface
 * @param[in]	iface The interface instance
 * @return		Physical interface instance or NULL if failed
 */
__attribute__((pure)) pfe_phy_if_t *pfe_log_if_get_parent(pfe_log_if_t *iface)
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
 * @brief		Destroy interface instance
 * @param[in]	iface The interface instance
 */
void pfe_log_if_destroy(pfe_log_if_t *iface)
{
	pfe_platform_rpc_pfe_log_if_destroy_arg_t req = {0U};
	errno_t ret;

	if (NULL != iface)
	{
		(void)pfe_log_if_db_lock();

		/*	Ask the master to unbind and destroy the logical interface */
		req.log_if_id = iface->id;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_DESTROY, &req, sizeof(req), NULL, 0U);
		if (EOK != ret)
		{
			NXP_LOG_DEBUG("Can't destroy remote instance: %d\n", ret);
		}

		if (NULL != iface->name)
		{
			oal_mm_free(iface->name);
			iface->name = NULL;
		}

		(void)pfe_log_if_db_unlock();

		if (EOK != oal_mutex_destroy(&iface->lock))
		{
			NXP_LOG_DEBUG("Could not destroy mutex\n");
		}

		oal_mm_free(iface);
	}
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
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_log_if_set_match_and_arg_t req = {0};

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_OR, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't set match to OR type on interfaces: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

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
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_log_if_set_match_and_arg_t req = {0};

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_AND, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't set match to AND type on interfaces: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

	return ret;
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
	pfe_platform_rpc_pfe_log_if_is_match_or_arg_t req = {0};
	pfe_platform_rpc_pfe_log_if_is_match_or_ret_t rpc_ret = {0};
	errno_t ret = EOK;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Query the master driver */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_IS_MATCH_OR, &req, sizeof(req), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't get match OR/AND status: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

	if (EOK != ret)
	{
		return FALSE;
	}
	else
	{
		return rpc_ret.status;
	}
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
errno_t pfe_log_if_set_match_rules(pfe_log_if_t *iface, pfe_ct_if_m_rules_t rules, pfe_ct_if_m_args_t *args)
{
	pfe_platform_rpc_pfe_log_if_set_match_rules_arg_t req = {0};
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL == args)
	{
		return EINVAL;
	}

	req.log_if_id = iface->id;
	req.rules = oal_htonl(rules);
	memcpy(&req.args, args, sizeof(req.args));

	(void)pfe_log_if_db_lock();

	/*	Ask master driver to add match rules */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MATCH_RULES, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't set match rules: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

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
errno_t pfe_log_if_add_match_rule(pfe_log_if_t *iface, pfe_ct_if_m_rules_t rule, void *arg, uint32_t arg_len)
{
	pfe_platform_rpc_pfe_log_if_add_match_rule_arg_t req = {0};
	errno_t ret = EINVAL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (0 == rule)
	{
		return EINVAL;
	}

	/*	Check if only single rule is requested */
	if (0 != (rule & (rule-1)))
	{
		return EINVAL;
	}

	if ((0U != arg_len) && (NULL == arg))
	{
		return EINVAL;
	}

	if (arg_len > sizeof(req.arg))
	{
		return EINVAL;
	}

	req.log_if_id = iface->id;
	req.rule = oal_htonl(rule);
	req.arg_len = oal_htonl(arg_len);
	memcpy(&req.arg, arg, sizeof(req.arg));

	(void)pfe_log_if_db_lock();

	/*	Ask master driver to add the match rule */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_ADD_MATCH_RULE, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't add match rule: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

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
	pfe_platform_rpc_pfe_log_if_del_match_rule_arg_t req = {0};
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;
	req.rule = oal_htonl(rule);

	(void)pfe_log_if_db_lock();

	/*	Ask master driver to delete the rule(s) */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_DEL_MATCH_RULE, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't delete match rule(s): %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

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
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_log_if_get_match_rules_arg_t req = {0};
	pfe_platform_rpc_pfe_log_if_get_match_rules_ret_t rpc_ret = {0};
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == rules)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Query master driver */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_GET_MATCH_RULES, &req, sizeof(req), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't get match rule(s): %d\n", ret);
	}
	else
	{
		*rules = oal_ntohl(rpc_ret.rules);
		if (NULL != args)
		{
			memcpy(args, &rpc_ret.args, sizeof(pfe_ct_if_m_args_t));
		}
	}

	(void)pfe_log_if_db_unlock();

	return ret;
}

/**
 * @brief		Set MAC address
 * @param[in]	iface The interface instance
 * @param[in]	addr The MAC address to add
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_set_mac_addr(pfe_log_if_t *iface, pfe_mac_addr_t addr)
{
	pfe_platform_rpc_pfe_log_if_set_mac_addr_arg_t req = {0};
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifndef PFE_CFG_TARGET_OS_AUTOSAR
	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

	/*	Set new MAC address */
	req.log_if_id = iface->id;
	ct_assert(sizeof(req.addr) == sizeof(pfe_mac_addr_t));
	memcpy(req.addr, addr, sizeof(pfe_mac_addr_t));

	(void)pfe_log_if_db_lock();

	/*	Request the change */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_SET_MAC_ADDR, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't set MAC address: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();
#ifndef PFE_CFG_TARGET_OS_AUTOSAR
	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */
	return ret;
}

/**
 * @brief		Get associated MAC address
 * @param[in]	iface The interface instance
 * @param[out]	addr Where to copy to address
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT No address assigned
 */
errno_t pfe_log_if_get_mac_addr(pfe_log_if_t *iface, pfe_mac_addr_t addr)
{
	pfe_platform_rpc_pfe_log_if_get_mac_addr_arg_t req = {0};
	pfe_platform_rpc_pfe_log_if_get_mac_addr_ret_t rpc_ret = {0};
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == addr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Execute query */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_GET_MAC_ADDR, &req, sizeof(req), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't get MAC address: %d\n", ret);
	}
	else
	{
		ct_assert(sizeof(rpc_ret.addr) == sizeof(pfe_mac_addr_t));
		memcpy(addr, rpc_ret.addr, sizeof(pfe_mac_addr_t));
	}

	(void)pfe_log_if_db_unlock();

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
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_log_if_get_egress_arg_t req = {0};
	pfe_platform_rpc_pfe_log_if_get_egress_ret_t rpc_ret = {0};

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == egress)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_GET_EGRESS, &req, sizeof(req), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't get egress interfaces: %d\n", ret);
	}
	else
	{
		*egress = rpc_ret.egress;
	}

	(void)pfe_log_if_db_unlock();

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
errno_t pfe_log_if_add_egress_if(pfe_log_if_t *iface, pfe_phy_if_t *phy_if)
{
	pfe_platform_rpc_pfe_log_if_add_egress_if_arg_t req = {0};
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == phy_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;
	req.phy_if_id = pfe_phy_if_get_id(phy_if);

	(void)pfe_log_if_db_lock();

	/*	Ask the master driver to add new egress interface */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_ADD_EGRESS_IF, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't add egress interface: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

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
errno_t pfe_log_if_del_egress_if(pfe_log_if_t *iface, pfe_phy_if_t *phy_if)
{
	pfe_platform_rpc_pfe_log_if_del_egress_if_arg_t req = {0};
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == phy_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;
	req.phy_if_id = pfe_phy_if_get_id(phy_if);

	(void)pfe_log_if_db_lock();

	/*	Ask the master driver to remove egress interface */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_DEL_EGRESS_IF, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't delete egress interface: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

	return ret;
}

/**
 * @brief		Clear associated MAC address
 * @param[in]	iface The interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_log_if_clear_mac_addr(pfe_log_if_t *iface)
{
	pfe_platform_rpc_pfe_log_if_clear_mac_addr_arg_t req = {0};
	errno_t ret;

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Ask the master driver to clear MAC address */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_CLEAR_MAC_ADDR, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't clear MAC address: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

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
	pfe_platform_rpc_pfe_log_if_enable_arg_t req = {0};
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Enable the interface */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_ENABLE, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't enable interface: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

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
	pfe_platform_rpc_pfe_log_if_disable_arg_t req = {0};
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Enable the interface */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_DISABLE, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't disable interface: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

	return ret;
}

/**
 * @brief		Check if interface is enabled
 * @param[in]	iface The interface instance
 * @return		TRUE if enabled, FALSE otherwise
 */
__attribute__((pure)) bool_t pfe_log_if_is_enabled(pfe_log_if_t *iface)
{
	pfe_platform_rpc_pfe_log_if_is_enabled_arg_t req = {0};
	pfe_platform_rpc_pfe_log_if_is_enabled_ret_t rpc_ret = {0};
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Query the master driver */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_IS_ENABLED, &req, sizeof(req), &rpc_ret, sizeof(rpc_ret));

	(void)pfe_log_if_db_unlock();

	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't get interface enable status: %d\n", ret);
		return FALSE;
	}
	else
	{
		return rpc_ret.status;
	}
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
	pfe_platform_rpc_pfe_log_if_promisc_enable_arg_t req = {0};
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Enable promiscuous mode */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_PROMISC_ENABLE, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't enable promiscuous mode: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

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
	pfe_platform_rpc_pfe_log_if_promisc_disable_arg_t req = {0};
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Enable promiscuous mode */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_PROMISC_DISABLE, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't disable promiscuous mode: %d\n", ret);
	}

	(void)pfe_log_if_db_unlock();

	return ret;
}

/**
 * @brief		Check if interface is in promiscuous mode
 * @param[in]	iface The interface instance
 * @return		TRUE if promiscuous mode is enabled, FALSE otherwise
 */
__attribute__((pure)) bool_t pfe_log_if_is_promisc(pfe_log_if_t *iface)
{
	pfe_platform_rpc_pfe_log_if_is_promisc_arg_t req = {0};
	pfe_platform_rpc_pfe_log_if_is_promisc_ret_t rpc_ret = {0};
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	req.log_if_id = iface->id;

	(void)pfe_log_if_db_lock();

	/*	Query the master driver */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_IS_PROMISC, &req, sizeof(req), &rpc_ret, sizeof(rpc_ret));

	(void)pfe_log_if_db_unlock();

	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't get promiscuous status: %d\n", ret);
		return FALSE;
	}
	else
	{
		return rpc_ret.status;
	}
}

/**
 * @brief		Get interface name
 * @param[in]	iface The interface instance
 * @return		Pointer to name string or NULL if failed/not found.
 */
__attribute__((pure)) char_t *pfe_log_if_get_name(pfe_log_if_t *iface)
{
	static char_t *unknown = "(unknown)";

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL == iface)
	{
		return unknown;
	}
	else
	{
		return iface->name;
	}
}

/**
 * @brief		Get log interface statistics
 * @param[in]	iface The interface instance
 * @param[out]	stat Statistic structure
 * @retval		EOK Success
 * @retval		NOMEM Not possible to allocate memory for read
 */
errno_t pfe_log_if_get_stats(pfe_log_if_t *iface, pfe_ct_class_algo_stats_t *stat)
{
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_log_if_stats_arg_t arg = {0};
	pfe_platform_rpc_pfe_log_if_stats_ret_t rpc_ret = {0};


#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	arg.log_if_id = iface->id;

	/*	Query the master driver */
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_LOG_IF_STATS, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't get interface enable status: %d\n", ret);
	}
	else
	{
		memcpy(stat, &rpc_ret.stats, sizeof(rpc_ret.stats));
	}

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
uint32_t pfe_log_if_get_text_statistics(pfe_log_if_t *iface, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	/*	Ask the master driver if interface is in promiscuous mode */
	NXP_LOG_ERROR("%s: Not supported yet\n", __func__);
	len += oal_util_snprintf(buf + len, buf_len - len, "%s: Unable to get statistics (not implemented)\n", __func__);

	return len;
}

#endif /* PFE_CFG_PFE_SLAVE */
