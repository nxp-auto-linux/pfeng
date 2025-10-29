/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2025 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @file		pfe_phy_if_slave.c
 * @brief		The PFE physical interface module source file (slave).
 * @details		This file contains physical interface-related functionality for
 *				the slave driver variant. All physical interface instance
 *				manipulation is done via RPC in way that local driver only
 *				sends requests to master driver which performs the actual
 *				requested operations.
 */
#include "pfe_cfg.h"
#include "oal.h"

#include "hal.h"
#include "pfe_platform_cfg.h"
#include "pfe_ct.h"
#include "linked_list.h"
#include "pfe_phy_if.h"
#include "pfe_idex.h" /* The RPC provider */
#include "pfe_platform_rpc.h" /* The RPC codes and data structures */

struct pfe_phy_if_tag
{
	pfe_ct_phy_if_id_t id;
	char_t *name;
	pfe_mac_db_t *mac_db; /* MAC database */
	oal_mutex_t lock;
	bool_t is_enabled;
};

static errno_t pfe_phy_if_disable_nolock(pfe_phy_if_t *iface);

static errno_t pfe_phy_if_db_lock(void)
{
	/*	Wait 1ms */
	const uint32_t timeout_step = 1000U;
	/*	Maximum Timeout */
	uint32_t timeout_us = PFE_CFG_IF_DB_RPC_RETRY_LOCK_TIME * 1000U;
	errno_t ret = EAGAIN;

	/* Try to lock IF DB */
	for (; timeout_us > 0U; timeout_us -= timeout_step)
	{
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_IF_LOCK, NULL, 0, NULL, 0U);
		if (EAGAIN == ret)
		{
			NXP_LOG_WARNING("Problem to acquire IF_DB lock - EAGAIN -> RETRY \n");
			oal_time_usleep(timeout_step);
		}
		else
		{
			/* Got lock or communication with Master is not possible */
			break;
		}
	}

	if (EOK != ret)
	{
		NXP_LOG_ERROR("Unable to lock interface DB: %d\n", ret);
	}

	return ret;
}

static errno_t pfe_phy_if_db_unlock(void)
{
	errno_t ret;

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_IF_UNLOCK, NULL, 0, NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Unable to unlock interface DB: %d\n", ret);
	}

	return ret;
}

/**
 * @brief		Create new physical interface instance
 * @param[in]	class The classifier instance
 * @param[in]	id The PFE firmware is using HW interface identifiers to distinguish
 * 				between particular interfaces. The set of available IDs (the
 * 				pfe_ct_phy_if_id_t) shall remain compatible with the firmware.
 * @param[in]	name Name of the interface
 * @return		The local copy of the interface instance of NULL if memory error
 */
pfe_phy_if_t *pfe_phy_if_create(pfe_class_t *class, pfe_ct_phy_if_id_t id, const char_t *name)
{
	pfe_phy_if_t *iface;

	/*	NOTE: Don't get remote phy_if instance, it is prerequisite to exist on Master */

	if (NULL == name)
	{
		NXP_LOG_ERROR("Invalid empty PHY_IF name\n");
		return NULL;
	}

	iface = oal_mm_malloc(sizeof(pfe_phy_if_t));
	if (NULL == iface)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		return NULL;
	}

	memset(iface, 0, sizeof(pfe_phy_if_t));
	iface->id = id;
	iface->mac_db = pfe_mac_db_create();
	if (NULL == iface->mac_db)
	{
		NXP_LOG_ERROR("Could not create MAC database\n");
		goto err;
	}

	iface->name = oal_mm_malloc(strlen(name) + 1U);
	if (NULL == iface->name)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		goto err;
	}
	strcpy(iface->name, name);
	if (EOK != oal_mutex_init(&iface->lock))
	{
		NXP_LOG_ERROR("Could not initialize mutex\n");
		goto err;
	}

	return iface;

err:
	if (NULL != iface->mac_db)
	{
		if (EOK != pfe_mac_db_destroy(iface->mac_db))
		{
			NXP_LOG_WARNING("Unable to destroy PHY_IF '%s' MAC database\n", name);
		}
	}
	if (NULL != iface->name)
	{
		oal_mm_free(iface->name);
	}
	if (EOK != oal_mutex_destroy(&iface->lock))
	{
		NXP_LOG_WARNING("Could not destroy mutex\n");
	}
	oal_mm_free(iface);
	return NULL;
}

/**
 * @brief		Destroy interface instance
 * @param[in]	iface The interface instance
 */
void pfe_phy_if_destroy(pfe_phy_if_t *iface)
{
	pfe_platform_rpc_pfe_phy_if_flush_mac_addrs_arg_t arg;
	errno_t ret;

	memset(&arg, 0U, sizeof(pfe_platform_rpc_pfe_phy_if_flush_mac_addrs_arg_t));

	if (NULL != iface)
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/*	Ask the master driver to remove all associated MAC addresses */
		arg.phy_if_id = iface->id;
		arg.crit = MAC_DB_CRIT_ALL;
		arg.type = PFE_TYPE_ANY;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_FLUSH_MAC_ADDRS, &arg, sizeof(arg), NULL, 0U);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("PFE_PLATFORM_RPC_PFE_PHY_IF_FLUSH_MAC_ADDRS failed: %d\n", ret);
		}

		/* Destroy local MAC database */
		ret = pfe_mac_db_destroy(iface->mac_db);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Unable to destroy MAC database: %d\n", ret);
		}

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

/**
 * @brief		Release interface instance resources
 * @param[in]	iface The interface instance
 */
void pfe_phy_if_release(pfe_phy_if_t *iface)
{
	errno_t ret;

	if (NULL != iface)
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		/* Destroy local MAC database */
		ret = pfe_mac_db_destroy(iface->mac_db);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Unable to destroy MAC database: %d\n", ret);
		}

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

/**
 * @brief		Check if interface is enabled
 * @param[in]	iface The interface instance
 * @retval		TRUE if enabled
 * @retval		FALSE if disabled
 */
bool_t pfe_phy_if_is_enabled(pfe_phy_if_t *iface)
{
	errno_t ret;
	bool_t status = FALSE;
	pfe_platform_rpc_pfe_phy_if_is_enabled_arg_t arg = {0};
	pfe_platform_rpc_pfe_phy_if_is_enabled_ret_t rpc_ret = {0};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		status = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		(void)pfe_phy_if_db_lock();

		/*	Ask the master driver to enable the interface */
		arg.phy_if_id = iface->id;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
		if (EOK != ret)
		{
			NXP_LOG_ERROR("PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED failed: %d\n", ret);
		}
		else
		{
			status = rpc_ret.status;
		}

		(void)pfe_phy_if_db_unlock();

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return status;
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
	pfe_platform_rpc_pfe_phy_if_enable_arg_t arg;

	memset(&arg, 0U, sizeof(pfe_platform_rpc_pfe_phy_if_enable_arg_t));

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

		(void)pfe_phy_if_db_lock();

		/*	Ask the master driver to enable the interface */
		arg.phy_if_id = iface->id;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE, &arg, sizeof(arg), NULL, 0U);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE failed: %d\n", ret);
		}

		(void)pfe_phy_if_db_unlock();

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
}

errno_t pfe_phy_if_disable_nolock(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_platform_rpc_pfe_phy_if_disable_arg_t arg;

	memset(&arg, 0U, sizeof(pfe_platform_rpc_pfe_phy_if_disable_arg_t));

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)pfe_phy_if_db_lock();

		/*	Ask the master driver to disable the interface */
		arg.phy_if_id = iface->id;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE, &arg, sizeof(arg), NULL, 0U);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE failed: %d\n", ret);
		}

		(void)pfe_phy_if_db_unlock();
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
 * @brief		Check if phy_if in promiscuous mode
 * @param[in]	iface The interface instance
 * @retval		TRUE promiscuous mode is enabled
 * @retval		FALSE  promiscuous mode is disbaled
 */
bool_t pfe_phy_if_is_promisc(pfe_phy_if_t *iface)
{
	errno_t ret;
	bool_t status = FALSE;
	pfe_platform_rpc_pfe_phy_if_is_promisc_arg_t arg = {0};
	pfe_platform_rpc_pfe_phy_if_is_promisc_ret_t rpc_ret = {0};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		status = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex lock failed\n");
		}

		(void)pfe_phy_if_db_lock();

		/*	Ask the master driver to enable the interface */
		arg.phy_if_id = iface->id;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_IS_PROMISC, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
		if (EOK != ret)
		{
			NXP_LOG_ERROR("PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED failed: %d\n", ret);
		}
		else
		{
			status = rpc_ret.status;
		}

		(void)pfe_phy_if_db_unlock();

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return status;
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
	pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t arg;

	memset(&arg, 0U, sizeof(pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t));

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

		(void)pfe_phy_if_db_lock();

		/*	Ask the master driver to enable the promiscuous mode */
		arg.phy_if_id = iface->id;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE, &arg, sizeof(arg), NULL, 0U);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("PFE_PLATFORM_RPC_PFE_PHY_IF_PROMICS_ENABLE failed: %d\n", ret);
		}

		(void)pfe_phy_if_db_unlock();

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
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
errno_t pfe_phy_if_add_mac_addr(pfe_phy_if_t *iface, const pfe_mac_addr_t addr, pfe_ct_phy_if_id_t owner)
{
	errno_t ret;
	pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t arg;

	ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg.mac_addr));

	memset(&arg, 0U, sizeof(pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t));

	(void)owner; /* Owner will be added directly to the RPC */

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

		(void)pfe_phy_if_db_lock();

		/*	Add address to local database */
		ret = pfe_mac_db_add_addr(iface->mac_db, addr, owner);
		if(EOK == ret)
		{
			/*	Ask the master driver to add the MAC address */
			memcpy(&arg.mac_addr[0], addr, sizeof(arg.mac_addr));
			arg.phy_if_id = iface->id;
			ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR, &arg, sizeof(arg), NULL, 0U);
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
					NXP_LOG_WARNING("PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR failed: %d\n", ret);
					/* Remove the address from local database */
					ret = pfe_mac_db_del_addr(iface->mac_db, addr, owner);
					if(EOK != ret)
					{
						NXP_LOG_ERROR("Unable to remove MAC address from phy_if MAC database: %d\n", ret);
					}
					ret = ENOENT;
				}
			}
		}

		(void)pfe_phy_if_db_unlock();

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
errno_t pfe_phy_if_del_mac_addr(pfe_phy_if_t *iface, const pfe_mac_addr_t addr, pfe_ct_phy_if_id_t owner)
{
	errno_t ret;
	pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t arg;

	ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg.mac_addr));

	memset(&arg, 0U, sizeof(pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t));

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

		(void)pfe_phy_if_db_lock();

		ret = pfe_mac_db_del_addr(iface->mac_db, addr, owner);
		if(EOK != ret)
		{
			NXP_LOG_ERROR("Unable to remove MAC address from phy_if MAC database: %d\n", ret);
		}
		else
		{
			/*	Ask the master driver to delete the MAC address */
			memcpy(&arg.mac_addr[0], addr, sizeof(arg.mac_addr));
			arg.phy_if_id = iface->id;
			ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR, &arg, sizeof(arg), NULL, 0U);
			if (EOK != ret)
			{
				NXP_LOG_WARNING("PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR failed: %d\n", ret);

				/* Removal of MAC address by master failed, put it back to DB */
				ret = pfe_mac_db_add_addr(iface->mac_db, addr, owner);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Unable to put back the MAC address into phy_if MAC database: %d\n", ret);
				}
			}
		}

		(void)pfe_phy_if_db_unlock();

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
 * @param[in]	owner Required owner of MAC address
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_phy_if_flush_mac_addrs(pfe_phy_if_t *iface, pfe_mac_db_crit_t crit, pfe_mac_type_t type, pfe_ct_phy_if_id_t owner)
{
	errno_t ret;
	pfe_platform_rpc_pfe_phy_if_flush_mac_addrs_arg_t arg;
	(void)owner; /* Owner will be added directly to the RPC */

	memset(&arg, 0U, sizeof(pfe_platform_rpc_pfe_phy_if_flush_mac_addrs_arg_t));

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

		(void)pfe_phy_if_db_lock();

		/*	Ask the master driver to flush owner's MAC addresses due to flush mode */
		arg.phy_if_id = iface->id;
		arg.crit = crit;
		arg.type = type;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_FLUSH_MAC_ADDRS, &arg, sizeof(arg), NULL, 0U);
		if (EOK != ret)
		{
			NXP_LOG_WARNING("PFE_PLATFORM_RPC_PFE_PHY_IF_FLUSH_MAC_ADDRS failed: %d\n", ret);
		}
		else
		{
			/*	Remove MAC addresses also from local database */
			ret = pfe_mac_db_flush(iface->mac_db, crit, type, owner);
			if(EOK != ret)
			{
				NXP_LOG_ERROR("Unable to flush MAC address from phy_if MAC database: %d\n", ret);
			}
		}

		(void)pfe_phy_if_db_unlock();

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
	}

	return ret;
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
