/* =========================================================================
 *  Copyright 2020-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_ct.h"
#include "pfe_spd_acc.h"
#include "pfe_feature_mgr.h"

#include "pfe_cfg.h"
#include "libfci.h"
#include "fpp.h"
#include "fpp_ext.h"

#include "fci_internal.h"
#include "fci.h"
#include "fci_spd.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_CLEARED_16
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* usage scope: fci_spd_cmd*/
static uint16_t search_position = 0U;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_CLEARED_16
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

errno_t fci_spd_cmd(fci_msg_t *msg, uint16_t *fci_ret, fpp_spd_cmd_t *reply_buf, uint32_t *reply_len)
{
    fci_t *fci_context = (fci_t *)&__context;
    errno_t ret = EOK;
    fpp_spd_cmd_t *spd_cmd;
    pfe_if_db_entry_t *pfe_if_db_entry = NULL;
    pfe_phy_if_t *phy_if = NULL;
    pfe_ct_spd_entry_t spd_entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == msg) || (NULL == fci_ret) || (NULL == reply_buf) || (NULL == reply_len)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
    else if (unlikely(FALSE == fci_context->fci_initialized))
	{
    	NXP_LOG_ERROR("Context not initialized\n");
		ret = EPERM;
	}
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
        if (*reply_len < sizeof(fpp_spd_cmd_t))
        {
            NXP_LOG_WARNING("Buffer length does not match expected value (fpp_spd_cmd_t)\n");
            ret = EINVAL;
        }
        else
        {
            /*	No data written to reply buffer (yet) */
            *reply_len = 0U;
            (void)memset(&spd_entry, 0, sizeof(pfe_ct_spd_entry_t));

            /*	Check that the FW feature is available (enabled) in FW */
            if (FALSE == pfe_feature_mgr_is_available("IPsec"))
            {
                /* Unavailable feature. Respond with FCI error code. */
                NXP_LOG_WARNING("Feature 'IPsec' is not available (not enabled in FW).\n");
                *fci_ret = FPP_ERR_FW_FEATURE_NOT_AVAILABLE;
                ret = EOK;
            }
            else
            {
                /*	Initialize the reply buffer */
                (void)memset(reply_buf, 0, sizeof(fpp_spd_cmd_t));

                spd_cmd = (fpp_spd_cmd_t *)(msg->msg_cmd.payload);
                /* Get the physical interface reference - needed for all commands */
                ret = pfe_if_db_polled_lock(&fci_context->if_session_id);
                if (EOK != ret)
                {
                    *fci_ret = FPP_ERR_IF_RESOURCE_ALREADY_LOCKED;
                    NXP_LOG_WARNING("DB lock failed\n");
                }
                else
                {
                    ret = pfe_if_db_get_first(fci_context->phy_if_db, fci_context->if_session_id, IF_DB_CRIT_BY_NAME, spd_cmd->name, &pfe_if_db_entry);
                    /* We first unlock the database and then examine the result */
                    if (EOK != pfe_if_db_unlock(fci_context->if_session_id))
                    {
                        *fci_ret = FPP_ERR_IF_WRONG_SESSION_ID;
                        NXP_LOG_ERROR("DB unlock failed\n");
                        ret = ENOENT;
                    }
                    else
                    {
                        if((EOK != ret) || (NULL == pfe_if_db_entry))
                        {
                            NXP_LOG_WARNING("Interface %s not found\n", spd_cmd->name);
                            *fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
                        }
                        else
                        {
                            phy_if = pfe_if_db_entry_get_phy_if(pfe_if_db_entry);
                            if(NULL == phy_if)
                            {
                                NXP_LOG_WARNING("Failed to get PHY if from DB entry\n");
                                *fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
                                ret = ENOENT;
                            }
                            else
                            {
                                /* Now interpret the command for given physical interface */
                                switch(spd_cmd->action)
                                {
                                    case FPP_ACTION_REGISTER:

                                        if(0U != ((uint8_t)spd_cmd->flags & (uint8_t)FPP_SPD_FLAG_IPv6))
                                        {
                                            spd_entry.flags |= SPD_FLAG_IPv6;
                                            (void)memcpy(&spd_entry.ipv.v6.sip, &spd_cmd->saddr[0], 16U);
                                            (void)memcpy(&spd_entry.ipv.v6.dip, &spd_cmd->daddr[0], 16U);
                                        }
                                        else
                                        {
                                            spd_entry.ipv.v4.sip = spd_cmd->saddr[0];
                                            spd_entry.ipv.v4.dip = spd_cmd->daddr[0];
                                        }
                                        if(0U != ((uint8_t)spd_cmd->flags & (uint8_t)FPP_SPD_FLAG_SPORT_OPAQUE))
                                        {
                                            spd_entry.flags |= SPD_FLAG_SPORT_OPAQUE;
                                        }
                                        if(0U != ((uint8_t)spd_cmd->flags & (uint8_t)FPP_SPD_FLAG_DPORT_OPAQUE))
                                        {
                                            spd_entry.flags |= SPD_FLAG_DPORT_OPAQUE;
                                        }
                                        spd_entry.proto = spd_cmd->protocol;
                                        spd_entry.sport = spd_cmd->sport;
                                        spd_entry.dport  = spd_cmd-> dport;
                                        spd_entry.sad_entry = spd_cmd->sa_id;
                                        spd_entry.spi = spd_cmd->spi;
                                        /* Using the fact that enums have same values for the corresponding flags */
                                        spd_entry.action = (pfe_ct_spd_entry_action_t)spd_cmd->spd_action;

                                        ret = pfe_spd_acc_add_rule(phy_if, oal_ntohs(spd_cmd->position), &spd_entry);
                                        if(EOK != ret)
                                        {
                                            *fci_ret = FPP_ERR_INTERNAL_FAILURE;
                                        }
                                        else
                                        {
                                            *fci_ret = FPP_ERR_OK;
                                        }
                                        break;

                                    case FPP_ACTION_DEREGISTER:
                                        ret = pfe_spd_acc_remove_rule(phy_if, oal_ntohs(spd_cmd->position));
                                        if(EOK != ret)
                                        {
                                            *fci_ret = FPP_ERR_INTERNAL_FAILURE;
                                        }
                                        else
                                        {
                                            *fci_ret = FPP_ERR_OK;
                                        }
                                        break;

                                    case FPP_ACTION_QUERY:
                                        search_position = 0U;
                                        fallthrough;

                                    case FPP_ACTION_QUERY_CONT:
                                        ret = pfe_spd_acc_get_rule(phy_if, search_position, &spd_entry);
                                        if(EOK == ret)
                                        {
                                            /* Construct reply */
                                            (void)memset(reply_buf, 0, sizeof(fpp_spd_cmd_t));
                                            (void)strcpy(reply_buf->name, spd_cmd->name);
                                            if(0 != (spd_entry.flags & SPD_FLAG_IPv6))
                                            {
                                                reply_buf->flags |= FPP_SPD_FLAG_IPv6;
                                                (void)memcpy(&reply_buf->saddr[0], &spd_entry.ipv.v6.sip[0], 16U);
                                                (void)memcpy(&reply_buf->daddr[0], &spd_entry.ipv.v6.dip[0], 16U);
                                            }
                                            else
                                            {
                                                reply_buf->saddr[0] = spd_entry.ipv.v4.sip;
                                                reply_buf->daddr[0] = spd_entry.ipv.v4.dip;
                                            }
                                            if(0 != (spd_entry.flags & SPD_FLAG_SPORT_OPAQUE))
                                            {
                                                reply_buf->flags |= FPP_SPD_FLAG_SPORT_OPAQUE;
                                            }
                                            if(0 != (spd_entry.flags & SPD_FLAG_DPORT_OPAQUE))
                                            {
                                                reply_buf->flags |= FPP_SPD_FLAG_DPORT_OPAQUE;
                                            }
                                            reply_buf->protocol = spd_entry.proto;
                                            reply_buf->sport = spd_entry.sport;
                                            reply_buf->dport = spd_entry.dport;
                                            reply_buf->sa_id = spd_entry.sad_entry;
                                            reply_buf->spi = spd_entry.spi;
                                            /* Using the fact that enums have same values for the corresponding flags */
                                            reply_buf->spd_action = (fpp_spd_action_t)spd_entry.action;
                                            reply_buf->position = oal_htons(search_position);
                                            search_position++;
                                            *fci_ret = FPP_ERR_OK;
                                            *reply_len = sizeof(fpp_spd_cmd_t);
                                            ret = EOK;
                                        }
                                        else
                                        {
                                            *fci_ret = FPP_ERR_IF_ENTRY_NOT_FOUND;
                                            ret = EOK;
                                        }
                                        break;

                                    default:
                                    {
                                        NXP_LOG_WARNING("Connection Command: Unknown action received: 0x%x\n", spd_cmd->action);
                                        *fci_ret = FPP_ERR_UNKNOWN_ACTION;
                                        ret = EINVAL;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_PFE_MASTER */

