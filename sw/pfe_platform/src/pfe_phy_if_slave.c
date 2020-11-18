/* =========================================================================
 *
 *  Copyright (c) 2020 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
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
#ifdef PFE_CFG_PFE_SLAVE

#include "oal.h"
#include "hal.h"

#include "pfe_ct.h"
#include "linked_list.h"
#include "pfe_phy_if.h"
#include "pfe_idex.h" /* The RPC provider */
#include "pfe_platform_rpc.h" /* The RPC codes and data structures */

struct __pfe_phy_if_tag
{
	pfe_ct_phy_if_id_t id;
	char_t *name;
	LLIST_t mac_addr_list;
	oal_mutex_t lock;
	bool_t is_enabled;
};

typedef struct __pfe_mac_addr_entry_tag
{
	pfe_mac_addr_t addr;	/*	The MAC address */
	LLIST_t iterator;		/*	List chain entry */
} pfe_mac_addr_list_entry_t;

static bool_t pfe_phy_if_has_log_if_nolock(pfe_phy_if_t *iface, pfe_log_if_t *log_if);

static errno_t pfe_phy_if_db_lock(void)
{
	errno_t ret;

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_IF_LOCK, NULL, 0, NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Unable to lock interface DB: %d\n", ret);
	}

	return ret;
}

static errno_t pfe_phy_if_db_unlock(void)
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
 * @brief		Create new physical interface instance
 * @param[in]	class The classifier instance
 * @param[in]	id The PFE firmware is using HW interface identifiers to distinguish
 * 				between particular interfaces. The set of available IDs (the
 * 				pfe_ct_phy_if_id_t) shall remain compatible with the firmware.
 * @param[in]	name Name of the interface
 * @return		The interface instance or NULL if failed
 */
pfe_phy_if_t *pfe_phy_if_create(pfe_class_t *class, pfe_ct_phy_if_id_t id, char_t *name)
{
	pfe_platform_rpc_pfe_phy_if_create_arg_t req = {0U};
	pfe_phy_if_t *iface;
	errno_t ret;

	/*	Get remote phy_if instance */
	req.phy_if_id = id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_CREATE, &req, sizeof(req), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("Can't get remote instance: %d\n", ret);
		return NULL;
	}

	iface = oal_mm_malloc(sizeof(pfe_phy_if_t));
	if (NULL == iface)
	{
		return NULL;
	}
	else
	{
		memset(iface, 0, sizeof(pfe_phy_if_t));
		LLIST_Init(&iface->mac_addr_list);
		iface->id = id;
	}

	if (NULL == name)
	{
		iface->name = NULL;
	}
	else
	{
		iface->name = oal_mm_malloc(strlen(name) + 1U);
		if (NULL == iface->name)
		{
			NXP_LOG_ERROR("Memory allocation failed\n");
			oal_mm_free(iface);
			return NULL;
		}

		strcpy(iface->name, name);
	}
	
	if (EOK != oal_mutex_init(&iface->lock))
	{
		NXP_LOG_ERROR("Could not initialize mutex\n");
		oal_mm_free(iface);
		iface = NULL;
		return NULL;
	}

	return iface;
}

/**
 * @brief		Destroy interface instance
 * @param[in]	iface The interface instance
 * @return		EOK success, error code otherwise
 */
errno_t pfe_phy_if_destroy(pfe_phy_if_t *iface)
{
	LLIST_t *item, *tmp_item;
	pfe_mac_addr_list_entry_t *entry;
	pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t arg = {0};
	errno_t ret = EOK;

	if (NULL != iface)
	{
		if (EOK != oal_mutex_lock(&iface->lock))
		{
			NXP_LOG_DEBUG("mutex lock failed\n");
		}

		/*	Remove all associated MAC addresses */
		LLIST_ForEachRemovable(item, tmp_item, &iface->mac_addr_list)
		{
			entry = LLIST_Data(item, pfe_mac_addr_list_entry_t, iterator);
			if (NULL != entry)
			{
				ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg.mac_addr));

				/*	Ask the master driver to delete the MAC address */
				memcpy(&arg.mac_addr[0], entry->addr, sizeof(arg.mac_addr));
				arg.phy_if_id = iface->id;
				ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR, &arg, sizeof(arg), NULL, 0U);
				if (EOK != ret)
				{
					NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR failed: %d\n", ret);
				}

				/*	In any case remove the address from local list and dispose
				 	related memory. */
				LLIST_Remove(&entry->iterator);
				oal_mm_free(entry);
				entry = NULL;
			}
		}

		if (EOK != oal_mutex_unlock(&iface->lock))
		{
			NXP_LOG_DEBUG("mutex unlock failed\n");
		}

		if (NULL != iface->name)
		{
			oal_mm_free(iface->name);
			iface->name = NULL;
		}

		if (EOK != oal_mutex_destroy(&iface->lock))
		{
			NXP_LOG_DEBUG("Could not destroy mutex\n");
		}

		oal_mm_free(iface);
	}
	
	return ret;
}

/**
 * @brief		Return classifier instance associated with interface
 * @param[in]	iface The interface instance
 * @return		The classifier instance
 */
__attribute__((pure)) pfe_class_t *pfe_phy_if_get_class(pfe_phy_if_t *iface)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return NULL;
}

/**
 * @brief		Add logical interface
 * @details		First added logical interface will become the default one. Default is used
 * 				when packet is not matching any other logical interface within the physical one.
 * @param[in]	iface The physical interface instance
 * @param[in]	log_if The logical interface instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 * @retval		EEXIST Entry exists
 */
errno_t pfe_phy_if_add_log_if(pfe_phy_if_t *iface, pfe_log_if_t *log_if)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	NXP_LOG_DEBUG("%s: Not supported in slave variant\n", __func__);
	ret = ENOTSUP;

	return ret;
}

static bool_t pfe_phy_if_has_log_if_nolock(pfe_phy_if_t *iface, pfe_log_if_t *log_if)
{
	pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t arg = {0};
	errno_t ret;
	bool_t val = TRUE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Ask master driver if such logical interface is associated with the physical one */
	arg.phy_if_id = iface->id;

	ct_assert(sizeof(arg.log_if_id) == sizeof(uint8_t));
	arg.log_if_id = pfe_log_if_get_id(log_if);

	(void)pfe_phy_if_db_lock();

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF, &arg, sizeof(arg), NULL, 0U);
	if (EOK == ret)
	{
		val = TRUE;
	}
	else if (ENOENT == ret)
	{
		val = FALSE;
	}
	else
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF failed: %d\n", ret);
		val = FALSE;
	}

	(void)pfe_phy_if_db_unlock();

	return val;
}

/**
 * @brief		Check if physical interface contains given logical interface
 * @param[in]	iface The physical interface instance
 * @param[in]	log_if The logical interface instance
 * @return		TRUE if logical interface is bound to the physical one. False
 * 				otherwise.
 */
bool_t pfe_phy_if_has_log_if(pfe_phy_if_t *iface, pfe_log_if_t *log_if)
{
	bool_t match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	match = pfe_phy_if_has_log_if_nolock(iface, log_if);

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
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
 */
errno_t pfe_phy_if_del_log_if(pfe_phy_if_t *iface, pfe_log_if_t *log_if)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	NXP_LOG_DEBUG("%s: Not supported in slave variant\n", __func__);
	ret = ENOTSUP;

	return ret;
}

/**
 * @brief		Get operational mode
 * @param[in]	iface The interface instance
 * @retval		Current phy_if mode. See pfe_ct_if_op_mode_t.
 */
pfe_ct_if_op_mode_t pfe_phy_if_get_op_mode(pfe_phy_if_t *iface)
{
	errno_t ret;
	pfe_ct_if_op_mode_t mode = IF_OP_DISABLED;
	pfe_platform_rpc_pfe_phy_if_get_op_mode_arg_t arg = {0};
	pfe_platform_rpc_pfe_phy_if_get_op_mode_ret_t rpc_ret = {0};

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return IF_OP_DISABLED;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Update the interface structure */
	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	(void)pfe_phy_if_db_lock();

	/*	Ask the master driver to change the operation mode */
	arg.phy_if_id = iface->id;

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_GET_OP_MODE, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_GET_OP_MODE failed: %d\n", ret);
	}
	else
	{
		mode = rpc_ret.mode;
	}

	(void)pfe_phy_if_db_unlock();

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return mode;
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
	errno_t ret;
	pfe_platform_rpc_pfe_phy_if_set_op_mode_arg_t arg = {0};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Update the interface structure */
#ifndef PFE_CFG_TARGET_OS_AUTOSAR
	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

	(void)pfe_phy_if_db_lock();

	/*	Ask the master driver to change the operation mode */
	arg.phy_if_id = iface->id;
	arg.op_mode = mode;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE failed: %d\n", ret);
	}

	(void)pfe_phy_if_db_unlock();

#ifndef PFE_CFG_TARGET_OS_AUTOSAR
	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

	return ret;
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
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	We're not going to allow slave driver to do this */
	NXP_LOG_ERROR("%s: Not supported\n", __func__);
	ret = ENOTSUP;

	return ret;
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
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == hif) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	We're not going to allow slave driver to do this */
	NXP_LOG_ERROR("%s: Not supported\n", __func__);
	ret = ENOTSUP;

	return ret;
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
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	We're not going to allow slave driver to do this */
	NXP_LOG_ERROR("%s: Not supported\n", __func__);
	ret = ENOTSUP;

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
	errno_t ret;
	bool_t status = FALSE;
	pfe_platform_rpc_pfe_phy_if_is_enabled_arg_t arg = {0};
	pfe_platform_rpc_pfe_phy_if_is_enabled_ret_t rpc_ret = {0};

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	(void)pfe_phy_if_db_lock();

	/*	Ask the master driver to enable the interface */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED failed: %d\n", ret);
	}
	else
	{
		status = rpc_ret.status;
	}

	(void)pfe_phy_if_db_unlock();

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
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
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_phy_if_enable_arg_t arg = {0};

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

	(void)pfe_phy_if_db_lock();

	/*	Ask the master driver to enable the interface */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE failed: %d\n", ret);
	}

	(void)pfe_phy_if_db_unlock();

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

errno_t pfe_phy_if_disable_nolock(pfe_phy_if_t *iface)
{
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_phy_if_disable_arg_t arg = {0};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)pfe_phy_if_db_lock();

	/*	Ask the master driver to disable the interface */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE failed: %d\n", ret);
	}

	(void)pfe_phy_if_db_unlock();

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
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	ret = pfe_phy_if_disable_nolock(iface);

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	(void)pfe_phy_if_db_lock();

	/*	Ask the master driver to enable the interface */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_IS_ENABLED failed: %d\n", ret);
	}
	else
	{
		status = rpc_ret.status;
	}

	(void)pfe_phy_if_db_unlock();

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
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
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t arg = {0};

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

	(void)pfe_phy_if_db_lock();

	/*	Ask the master driver to enable the promiscuous mode */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_PROMICS_ENABLE failed: %d\n", ret);
	}

	(void)pfe_phy_if_db_unlock();

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
errno_t pfe_phy_if_promisc_disable(pfe_phy_if_t *iface)
{
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t arg = {0};

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

	(void)pfe_phy_if_db_lock();

	/*	Ask the master driver to disable the promiscuous mode */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_DISABLE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_PROMICS_DISABLE failed: %d\n", ret);
	}

	(void)pfe_phy_if_db_unlock();

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Add MAC address
 * @param[in]	iface The interface instance
 * @param[in]	addr The MAC address to add
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_phy_if_add_mac_addr(pfe_phy_if_t *iface, pfe_mac_addr_t addr)
{
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_phy_if_add_mac_addr_arg_t arg = {0};
	pfe_mac_addr_list_entry_t *entry;

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

	(void)pfe_phy_if_db_lock();

	/*	Add address to local list */
	entry = oal_mm_malloc(sizeof(pfe_mac_addr_list_entry_t));
	if (NULL == entry)
	{
		NXP_LOG_ERROR("Memory allocation failed\n");
		ret = ENOMEM;
	}
	else
	{
		memcpy(entry->addr, addr, sizeof(pfe_mac_addr_t));
		LLIST_AddAtEnd(&entry->iterator, &iface->mac_addr_list);

		ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg.mac_addr));

		/*	Ask the master driver to add the MAC address */
		memcpy(&arg.mac_addr[0], addr, sizeof(arg.mac_addr));
		arg.phy_if_id = iface->id;
		ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR, &arg, sizeof(arg), NULL, 0U);
		if (EOK != ret)
		{
			NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_ADD_MAC_ADDR failed: %d\n", ret);

			/*	Remove the address from local list */
			LLIST_Remove(&entry->iterator);
			oal_mm_free(entry);
			entry = NULL;
		}
	}

	(void)pfe_phy_if_db_unlock();

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
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Address not found
 */
errno_t pfe_phy_if_del_mac_addr(pfe_phy_if_t *iface, pfe_mac_addr_t addr)
{
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_phy_if_del_mac_addr_arg_t arg = {0};
	pfe_mac_addr_list_entry_t *entry;
	LLIST_t *item;
	bool_t found = FALSE;

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

	(void)pfe_phy_if_db_lock();

	ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg.mac_addr));

	/*	Ask the master driver to delete the MAC address */
	memcpy(&arg.mac_addr[0], addr, sizeof(arg.mac_addr));
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_DEL_MAC_ADDR failed: %d\n", ret);
	}
	else
	{
		/*	Get the local list entry */
		LLIST_ForEach(item, &iface->mac_addr_list)
		{
			entry = LLIST_Data(item, pfe_mac_addr_list_entry_t, iterator);
			if (0 == memcmp(addr, entry->addr, sizeof(pfe_mac_addr_t)))
			{
				found = TRUE;
				break;
			}
		}

		if (FALSE == found)
		{
			NXP_LOG_DEBUG("FATAL: MAC address not found\n");
		}
		else
		{
			/*	Remove the address from local list */
			LLIST_Remove(&entry->iterator);
			oal_mm_free(entry);
			entry = NULL;
		}
	}

	(void)pfe_phy_if_db_unlock();

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get MAC address
 * @param[in]	iface The interface instance
 * @param[out]	addr The MAC address will be written here
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT No address found
 */
errno_t pfe_phy_if_get_mac_addr(pfe_phy_if_t *iface, pfe_mac_addr_t addr)
{
	errno_t ret = EOK;
	pfe_mac_addr_list_entry_t *entry;

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

	(void)pfe_phy_if_db_lock();

	if (FALSE == LLIST_IsEmpty(&iface->mac_addr_list))
	{
		/*	Get first address from the list */
		entry = (pfe_mac_addr_list_entry_t *)LLIST_Data(iface->mac_addr_list.prNext, pfe_mac_addr_list_entry_t, iterator);

		/*	Provide the MAC address */
		memcpy(addr, entry->addr, sizeof(pfe_mac_addr_t));
		ret = EOK;
	}
	else
	{
		/*	No address assigned */
		ret = ENOENT;
	}

	(void)pfe_phy_if_db_unlock();

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Get HW ID of the interface
 * @param[in]	iface The interface instance
 * @return		Interface ID
 */
__attribute__((pure)) pfe_ct_phy_if_id_t pfe_phy_if_get_id(pfe_phy_if_t *iface)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return PFE_PHY_IF_ID_INVALID;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return iface->id;
}

/**
 * @brief		Get name
 * @param[in]	iface The interface instance
 * @return		Pointer to interface name string or NULL if not found/failed
 */
__attribute__((pure)) char_t *pfe_phy_if_get_name(pfe_phy_if_t *iface)
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

	return iface->name;
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
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_phy_if_stats_arg_t arg = {0};
	pfe_platform_rpc_pfe_phy_if_stats_ret_t rpc_ret = {0};

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == stat)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_STATS, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_IS_STATS failed: %d\n", ret);
	}
	else
	{
		memcpy(stat,&rpc_ret.stats,sizeof(rpc_ret.stats));
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Return physical interface runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	iface 		The physical interface instance
 * @param[in]	buf 		A pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_phy_if_get_text_statistics(pfe_phy_if_t *iface, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	len += oal_util_snprintf(buf + len, buf_len - len, "[PhyIF 0x%x]: Unable to read DMEM (not implemented)\n", iface->id);
	
	return len;
}

#endif /* PFE_CFG_PFE_SLAVE */
