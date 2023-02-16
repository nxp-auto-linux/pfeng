/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
/**
 * @addtogroup  dxgr_FCI
 * @{
 *
 * @file		fci_hm.c
 * @brief		Health Monitor management functions.
 * @details		All Health Monitor-related functionality provided by the FCI should be
 * 				implemented within this file.
 *
 */

#include "pfe_cfg.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"
#include "pfe_hm.h"

#include "fci_internal.h"
#include "fci.h"

#ifdef PFE_CFG_FCI_ENABLE

/**
 * @brief			Callback from Health Monitor (HM) module. Intended for sending of FPP_CMD_HEALTH_MONITOR_EVENT.
 * @details			This callback is used by FCI as a notification of some HM activity.
 * 					Function parameter is not utilized. A full search through HM database is done instead, 
 * 					to make sure all existing HM items were reported.
 * @param[in]		unused 	Health Monitor item as reported by HM module.
 *							Not needed in this callback, but required by HM callback function signature.
 */
static void fci_hm_cb(pfe_hm_item_t *unused)
{
	fci_t *fci_context = (fci_t *)&__context;
	errno_t ret;
	pfe_hm_item_t item;
	fci_msg_t msg = {0};

	(void)unused;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(FALSE == fci_context->fci_initialized))
	{
		NXP_LOG_RAW_ERROR("Context not initialized\n");
		ret = EPERM;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		msg.type = FCI_MSG_CMD;
		msg.msg_cmd.code = FPP_CMD_HEALTH_MONITOR_EVENT;
		msg.msg_cmd.length = sizeof(fpp_health_monitor_cmd_t);

		/* Consume items from HM database and send FCI event for each of them. */
		do
		{
			if (FALSE == fci_context->is_some_client)  /* Consume only if there is someone to send data to. */
			{
				ret = EPERM;
				NXP_LOG_RAW_DEBUG("No client to send data to.\n");
			}
			else
			{
				ret = pfe_hm_get(&item);
				if (EOK != ret)
				{
					if (ENOENT == ret)
					{
						NXP_LOG_RAW_DEBUG("No more items in HM database\n");
					}
					else
					{
						NXP_LOG_RAW_ERROR("Failed to get item from HM database: %d\n", ret);
					}
				}
				else
				{
					fpp_health_monitor_cmd_t hm_event = {0};
					hm_event.action = 0;
					hm_event.id = oal_htons(item.id);
					hm_event.type = item.type;
					hm_event.src = item.src;
#ifdef PFE_CFG_HM_STRINGS_ENABLED
					strncpy(hm_event.desc, pfe_hm_get_event_str(item.id), (FPP_HEALTH_MONITOR_DESC_SIZE-1U));
#endif /* PFE_CFG_HM_STRINGS_ENABLED */
					{
						/* Indented code block needed because ct_assert() otherwise causes
						 * compilation error 'ISO C90 forbids mixed declarations and code' */
						ct_assert(sizeof(msg.msg_cmd.payload) >= sizeof(fpp_health_monitor_cmd_t));
						memcpy(msg.msg_cmd.payload, &hm_event, sizeof(fpp_health_monitor_cmd_t));
					}
					ret = fci_core_client_send_broadcast(&msg, NULL);
				}
			}
		}
		while (EOK == ret);
	}
}

/**
 * @brief	Read HM items from HM database and send FCI event for each reported HM item.
 */
void fci_hm_send_events(void)
{
	fci_hm_cb(NULL);
}

/**
 * @brief	Register FCI callback in Health Monitor module. This needs to be called during FCI init.
 * @return	EOK if success, error code otherwise.
 */
errno_t fci_hm_cb_register(void)
{
	if (TRUE == pfe_hm_register_event_cb(fci_hm_cb))
	{
		return EOK;
	}
	else
	{
		return EINVAL;
	}
}

/**
 * @brief	Deregister FCI callback from Health Monitor module. This needs to be called during FCI fini.
 * @return	EOK if success, error code otherwise.
 */
errno_t fci_hm_cb_deregister(void)
{
	return EOK; /* HM module currently does not support callback deregistration. */
}

#endif /* PFE_CFG_FCI_ENABLE */
