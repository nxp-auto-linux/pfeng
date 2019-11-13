/* =========================================================================
 *  Copyright 2018-2019 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ========================================================================= */

/**
 * @addtogroup  dxgr_PFE_PHY_IF
 * @{
 *
 * @file		pfe_phy_if_slave.c
 * @brief		The PFE physical interface module source file (slave).
 * @details		This file contains physical interface-related functionality for
 *				the slave driver variant. All physical interface instance
 *				manipulation is done via RPC in way that local driver only
 *				sends requests to master driver which performs the actual
 *				requested operations.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

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
	pfe_phy_if_cbk_t callback;
	void *callback_arg;
	oal_mutex_t lock;
	bool_t is_enabled;
};

typedef struct __pfe_mac_addr_entry_tag
{
	pfe_mac_addr_t addr;	/*	The MAC address */
	LLIST_t iterator;		/*	List chain entry */
} pfe_mac_addr_list_entry_t;

static bool_t pfe_phy_if_has_log_if_nolock(pfe_phy_if_t *iface, pfe_log_if_t *log_if);

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
				_ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg.mac_addr));

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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	NXP_LOG_DEBUG("%s: Not supported in slave variant\n", __func__);
	ret = ENOTSUP;

	return ret;
}

static bool_t pfe_phy_if_has_log_if_nolock(pfe_phy_if_t *iface, pfe_log_if_t *log_if)
{
	pfe_platform_rpc_pfe_phy_if_has_log_if_arg_t arg = {0};
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Ask master driver if such logical interface is associated with the physical one */
	arg.phy_if_id = iface->id;

	_ct_assert(sizeof(arg.log_if_id) == sizeof(uint8_t));
	arg.log_if_id = pfe_log_if_get_id(log_if);

	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF, &arg, sizeof(arg), NULL, 0U);
	if (EOK == ret)
	{
		return TRUE;
	}
	else if (ENOENT == ret)
	{
		return FALSE;
	}
	else
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_HAS_LOG_IF failed: %d\n", ret);
		return FALSE;
	}
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == iface) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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
	/* TODO implement this AAVB-2111*/
	return IF_OP_DISABLED;
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Update the interface structure */
	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	/*	Ask the master driver to change the operation mode */
	arg.phy_if_id = iface->id;
	arg.op_mode = mode;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_SET_OP_MODE failed: %d\n", ret);
	}

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == hif) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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
	/* TODO implement this AAVB-2111*/
	return FALSE;
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	/*	Ask the master driver to enable the interface */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_ENABLE failed: %d\n", ret);
	}

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Ask the master driver to disable the interface */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_DISABLE failed: %d\n", ret);
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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
	/* TODO implement this AAVB-2111*/
	return FALSE;
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	/*	Ask the master driver to enable the promiscuous mode */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_ENABLE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_PROMICS_ENABLE failed: %d\n", ret);
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
errno_t pfe_phy_if_promisc_disable(pfe_phy_if_t *iface)
{
	errno_t ret = EOK;
	pfe_platform_rpc_pfe_phy_if_promisc_enable_arg_t arg = {0};

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	/*	Ask the master driver to disable the promiscuous mode */
	arg.phy_if_id = iface->id;
	ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_PFE_PHY_IF_PROMISC_DISABLE, &arg, sizeof(arg), NULL, 0U);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("PFE_PLATFORM_RPC_PFE_PHY_IF_PROMICS_DISABLE failed: %d\n", ret);
	}

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

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

		_ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg.mac_addr));

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	_ct_assert(sizeof(pfe_mac_addr_t) == sizeof(arg.mac_addr));

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

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
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return PFE_PHY_IF_ID_INVALID;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

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
 * @brief		Set event callback
 * @details		Routine will bind a callback with interface instance which will
 * 				be called on various interface-related events. See pfe_phy_if_event_t.
 * @param[in]	iface The interface instance
 * @param[in]	callback Routine to be called
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_set_callback(pfe_phy_if_t *iface, pfe_phy_if_cbk_t callback, void *arg)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	iface->callback = callback;
	iface->callback_arg = arg;

	if (EOK != oal_mutex_unlock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex unlock failed\n");
	}

	return EOK;
}

/**
 * @brief		Delete event callback
 * @param[in]	iface The interface instance
 * @param[in]	callback Routine to be removed
 * @retval		EOK Success
 * @retval		ENOENT Callback not found
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_phy_if_del_callback(pfe_phy_if_t *iface, pfe_phy_if_cbk_t callback)
{
	errno_t ret = EOK;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&iface->lock))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
	}

	if (iface->callback == callback)
	{
		iface->callback = NULL;
		iface->callback_arg = NULL;
	}
	else
	{
		ret = ENOENT;
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == iface))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	len += oal_util_snprintf(buf + len, buf_len - len, "[PhyIF 0x%x]: Unable to read DMEM (not implemented)\n", iface->id);
	
	return len;
}

/** @}*/
