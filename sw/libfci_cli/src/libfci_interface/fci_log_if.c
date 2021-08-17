/* =========================================================================
 *  (c) Copyright 2006-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017, 2019-2021 NXP
 *
 *  NXP Confidential. This software is owned or controlled by NXP and may only
 *  be used strictly in accordance with the applicable license terms. By
 *  expressly accepting such terms or by downloading, installing, activating
 *  and/or otherwise using the software, you are agreeing that you have read,
 *  and that you agree to comply with and are bound by, such license terms. If
 *  you do not agree to be bound by the applicable license terms, then you may
 *  not retain, install, activate or otherwise use the software.
 *
 *  This file contains sample code only. It is not part of the production code deliverables.
 * ========================================================================= */
 
 
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
#include "fci_common.h"
#include "fci_log_if.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief          Network-to-host (ntoh) function for a logical interface struct.
 * @param[in,out]  p_rtn_logif  The logical interface struct to be converted.
 */
static void ntoh_logif(fpp_log_if_cmd_t* p_rtn_logif)
{
    assert(NULL != p_rtn_logif);
    
    
    p_rtn_logif->id = ntohl(p_rtn_logif->id);
    p_rtn_logif->parent_id = ntohl(p_rtn_logif->parent_id);
    p_rtn_logif->egress = ntohl(p_rtn_logif->egress);
    ntoh_enum(&(p_rtn_logif->flags), sizeof(fpp_if_flags_t));
    ntoh_enum(&(p_rtn_logif->match), sizeof(fpp_if_m_rules_t));
    
    p_rtn_logif->arguments.vlan = ntohs(p_rtn_logif->arguments.vlan);
    p_rtn_logif->arguments.ethtype = ntohs(p_rtn_logif->arguments.ethtype);
    p_rtn_logif->arguments.sport = ntohs(p_rtn_logif->arguments.sport);
    p_rtn_logif->arguments.dport = ntohs(p_rtn_logif->arguments.dport);
    p_rtn_logif->arguments.ipv.v4.sip = ntohl(p_rtn_logif->arguments.ipv.v4.sip);
    p_rtn_logif->arguments.ipv.v4.dip = ntohl(p_rtn_logif->arguments.ipv.v4.dip);
    p_rtn_logif->arguments.ipv.v6.sip[0] = ntohl(p_rtn_logif->arguments.ipv.v6.sip[0]);
    p_rtn_logif->arguments.ipv.v6.sip[1] = ntohl(p_rtn_logif->arguments.ipv.v6.sip[1]);
    p_rtn_logif->arguments.ipv.v6.sip[2] = ntohl(p_rtn_logif->arguments.ipv.v6.sip[2]);
    p_rtn_logif->arguments.ipv.v6.sip[3] = ntohl(p_rtn_logif->arguments.ipv.v6.sip[3]);
    p_rtn_logif->arguments.ipv.v6.dip[0] = ntohl(p_rtn_logif->arguments.ipv.v6.dip[0]);
    p_rtn_logif->arguments.ipv.v6.dip[1] = ntohl(p_rtn_logif->arguments.ipv.v6.dip[1]);
    p_rtn_logif->arguments.ipv.v6.dip[2] = ntohl(p_rtn_logif->arguments.ipv.v6.dip[2]);
    p_rtn_logif->arguments.ipv.v6.dip[3] = ntohl(p_rtn_logif->arguments.ipv.v6.dip[3]);
    p_rtn_logif->arguments.hif_cookie = ntohl(p_rtn_logif->arguments.hif_cookie);
    
    p_rtn_logif->stats.processed = ntohl(p_rtn_logif->stats.processed);
    p_rtn_logif->stats.accepted = ntohl(p_rtn_logif->stats.accepted);
    p_rtn_logif->stats.rejected = ntohl(p_rtn_logif->stats.rejected);
    p_rtn_logif->stats.discarded = ntohl(p_rtn_logif->stats.discarded);
}
 
 
/*
 * @brief          Host-to-network (hton) function for a logical interface struct.
 * @param[in,out]  p_rtn_logif  The logical interface struct to be converted.
 */
static void hton_logif(fpp_log_if_cmd_t* p_rtn_logif)
{
    assert(NULL != p_rtn_logif);
    
    
    p_rtn_logif->id = htonl(p_rtn_logif->id);
    p_rtn_logif->parent_id = htonl(p_rtn_logif->parent_id);
    p_rtn_logif->egress = htonl(p_rtn_logif->egress);
    hton_enum(&(p_rtn_logif->flags), sizeof(fpp_if_flags_t));
    hton_enum(&(p_rtn_logif->match), sizeof(fpp_if_m_rules_t));
    
    p_rtn_logif->arguments.vlan = htons(p_rtn_logif->arguments.vlan);
    p_rtn_logif->arguments.ethtype = htons(p_rtn_logif->arguments.ethtype);
    p_rtn_logif->arguments.sport = htons(p_rtn_logif->arguments.sport);
    p_rtn_logif->arguments.dport = htons(p_rtn_logif->arguments.dport);
    p_rtn_logif->arguments.ipv.v4.sip = htonl(p_rtn_logif->arguments.ipv.v4.sip);
    p_rtn_logif->arguments.ipv.v4.dip = htonl(p_rtn_logif->arguments.ipv.v4.dip);
    p_rtn_logif->arguments.ipv.v6.sip[0] = htonl(p_rtn_logif->arguments.ipv.v6.sip[0]);
    p_rtn_logif->arguments.ipv.v6.sip[1] = htonl(p_rtn_logif->arguments.ipv.v6.sip[1]);
    p_rtn_logif->arguments.ipv.v6.sip[2] = htonl(p_rtn_logif->arguments.ipv.v6.sip[2]);
    p_rtn_logif->arguments.ipv.v6.sip[3] = htonl(p_rtn_logif->arguments.ipv.v6.sip[3]);
    p_rtn_logif->arguments.ipv.v6.dip[0] = htonl(p_rtn_logif->arguments.ipv.v6.dip[0]);
    p_rtn_logif->arguments.ipv.v6.dip[1] = htonl(p_rtn_logif->arguments.ipv.v6.dip[1]);
    p_rtn_logif->arguments.ipv.v6.dip[2] = htonl(p_rtn_logif->arguments.ipv.v6.dip[2]);
    p_rtn_logif->arguments.ipv.v6.dip[3] = htonl(p_rtn_logif->arguments.ipv.v6.dip[3]);
    p_rtn_logif->arguments.hif_cookie = htonl(p_rtn_logif->arguments.hif_cookie);
    
    p_rtn_logif->stats.processed = htonl(p_rtn_logif->stats.processed);
    p_rtn_logif->stats.accepted = htonl(p_rtn_logif->stats.accepted);
    p_rtn_logif->stats.rejected = htonl(p_rtn_logif->stats.rejected);
    p_rtn_logif->stats.discarded = htonl(p_rtn_logif->stats.discarded);
}
 
 
/*
 * @brief       Set/unset a bitflag in a logical interface struct.
 * @param[out]  p_rtn_logif  The logical interface struct to be modified.
 * @param[in]   enable  New state of the bitflag.
 * @param[in]   flag    The bitflag.
 */
static void set_flag(fpp_log_if_cmd_t* p_rtn_logif, bool enable,
                     fpp_if_flags_t flag)
{
    assert(NULL != p_rtn_logif);
    
    
    if (enable)
    {
        p_rtn_logif->flags |= flag;
    }
    else
    {
        p_rtn_logif->flags &= (fpp_if_flags_t)(~flag);
    }
}
 
 
/*
 * @brief       Set/unset a match rule bitflag in a logical interface stuct.
 * @param[out]  p_rtn_logif  The logical interface struct to be modified.
 * @param[in]   enable       New state of the bitflag.
 * @param[in]   match_rule   The match rule bitflag.
 */
static void set_mr_flag(fpp_log_if_cmd_t* p_rtn_logif, bool enable,
                        fpp_if_m_rules_t match_rule)
{
    assert(NULL != p_rtn_logif);
    
    
    if (enable)
    {
        p_rtn_logif->match |= match_rule;
    }
    else
    {
        p_rtn_logif->match &= (fpp_if_flags_t)(~match_rule);
    }
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested logical interface
 *              from the PFE. Identify the interface by its name.
 * @details     To use this function properly, the PFE interface database must be
 *              locked for exclusive access. See fci_log_if_get_by_name_sa() for 
 *              an example how to lock the database.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_logif  Space for data from the PFE.
 * @param[in]   p_name       Name of the requested logical interface.
 *                           Names of logical interfaces are user-defined.
 *                           See fci_log_if_add().
 * @return      FPP_ERR_OK : Requested logical interface was found.
 *                           A copy of its configuration was stored into p_rtn_logif.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_log_if_get_by_name(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, 
                           const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_logif);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_logif(&reply_from_fci);  /* set correct byte order */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) && (strcmp(p_name, reply_from_fci.name)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                        sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_logif(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_logif = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested logical interface
 *              from the PFE. Identify the interface by its name.
 * @details     This is a standalone (_sa) function.
 *              It shows how to properly access a logical interface. Namely:
 *              1. Lock the interface database for exclusive access by this FCI client.
 *              2. Execute one or more FCI calls which access 
 *                 physical or logical interfaces.
 *              3. Unlock the interface database's exclusive access lock.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_logif  Space for data from the PFE.
 * @param[in]   p_name       Name of the requested logical interface.
 *                           Names of logical interfaces are user-defined.
 *                           See fci_log_if_add().
 * @return      FPP_ERR_OK : Requested logical interface was found.
 *                           A copy of its configuration was stored into p_rtn_logif.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
inline int fci_log_if_get_by_name_sa(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, 
                                     const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_logif);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    /* lock the interface database for exclusive access by this FCI client */
    rtn = fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
    
    /* execute "payload" - FCI calls which access physical or logical interfaces */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_log_if_get_by_name(p_cl, p_rtn_logif, p_name);
    }
    
    /* unlock the interface database's exclusive access lock */
    /* result of the unlock action is returned only if previous "payload" actions were OK */
    const int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested logical interface
 *              from the PFE. Identify the interface by its ID.
 * @details     To use this function properly, the PFE interface database must be
 *              locked for exclusive access. See fci_log_if_get_by_name_sa() for 
 *              an example how to lock the database.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_logif  Space for data from the PFE.
 * @param[in]   id           ID of the requested logical interface.
 *                           IDs of logical interfaces are assigned automatically.
 *                           Hint: It is better to identify logical interfaces by 
 *                                 their interface names.
 * @return      FPP_ERR_OK : Requested logical interface was found.
 *                           A copy of its configuration was stored into p_rtn_logif.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_log_if_get_by_id(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, uint32_t id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_logif);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_logif(&reply_from_fci);  /* set correct byte order */
    
    /* query loop */
    while ((FPP_ERR_OK == rtn) && (id != (reply_from_fci.id)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                        sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_logif(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_logif = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target logical interface 
 *                 in the PFE.
 * @details        To use this function properly, the PFE interface database must be
 *                 locked for exclusive access. See fci_log_if_update_sa() for 
 *                 an example how to lock the database.
 * @param[in]      p_cl     FCI client instance
 * @param[in,out]  p_logif  Data struct which represents a new configuration of
 *                          the target logical interface.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @return         FPP_ERR_OK : Configuration of the target logical interface was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
int fci_log_if_update(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_cl);
    assert(NULL != p_logif);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_log_if_cmd_t cmd_to_fci = (*p_logif);
    
    /* send data */
    hton_logif(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t), 
                                         (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_log_if_get_by_id(p_cl, p_logif, (p_logif->id));
    }
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target logical interface 
 *                 in the PFE.
 * @details        This is a standalone (_sa) function.
 *                 It shows how to properly access a logical interface. Namely:
 *                 1. Lock the interface database for exclusive access by this FCI client.
 *                 2. Execute one or more FCI calls which access 
 *                    physical or logical interfaces.
 *                 3. Unlock the interface database's exclusive access lock.
 * @param[in]      p_cl     FCI client instance
 * @param[in,out]  p_logif  Data struct which represents a new configuration of
 *                          the target logical interface.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @return         FPP_ERR_OK : Configuration of the target logical interface was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
inline int fci_log_if_update_sa(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_cl);
    assert(NULL != p_logif);
    
    
    int rtn = FPP_ERR_FCI;
    
    /* lock the interface database for exclusive access by this FCI client */
    rtn = fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
    
    /* execute "payload" - FCI calls which access physical or logical interfaces */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_log_if_update(p_cl, p_logif);
    }
    
    /* unlock the interface database's exclusive access lock */
    /* result of the unlock action is returned only if previous "payload" actions were OK */
    const int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */
 
 
/*
 * @brief       Use FCI calls to create a new logical interface in the PFE.
 * @details     To use this function properly, the PFE interface database must be
 *              locked for exclusive access. See fci_log_if_update_sa() for 
 *              an example how to lock the database.
 * @param[in]   p_cl           FCI client instance
 * @param[out]  p_rtn_logif    Space for data from the PFE.
 *                             Will contain a copy of configuration data of 
 *                             the newly created logical interface.
 *                             Can be NULL. If NULL, then there is no local data to fill.
 * @param[in]   p_name         Name of the new logical interface.
 *                             The name is user-defined.
 * @param[in]   p_parent_name  Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : New logical interface was created.
 *                           If applicable, then its configuration data were 
 *                           copied into p_rtn_logif.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_log_if_add(FCI_CLIENT* p_cl, fpp_log_if_cmd_t* p_rtn_logif, const char* p_name, 
                   const char* p_parent_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_name);
    assert(NULL != p_parent_name);
    /* 'p_rtn_logif' is allowed to be NULL */
    
    
    int rtn = FPP_ERR_FCI;
    fpp_log_if_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, IFNAMSIZ);
    if (FPP_ERR_OK == rtn)
    {
        rtn = set_text((cmd_to_fci.parent_name), p_parent_name, IFNAMSIZ);
    }
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        hton_logif(&cmd_to_fci);  /* set correct byte order */
        cmd_to_fci.action = FPP_ACTION_REGISTER;
        rtn = fci_write(p_cl, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    }
    
    /* read back and update caller data (if applicable) */
    if ((FPP_ERR_OK == rtn) && (NULL != p_rtn_logif))
    {
        rtn = fci_log_if_get_by_name(p_cl, p_rtn_logif, p_name);
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to destroy the target logical interface in the PFE.
 * @details    To use this function properly, the PFE interface database must be
 *             locked for exclusive access. See fci_log_if_update_sa() for 
 *             an example how to lock the database.
 * @param[in]  p_cl    FCI client instance
 * @param[in]  p_name  Name of the logical interface to destroy.
 * @return     FPP_ERR_OK : Logical interface was destroyed.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_log_if_del(FCI_CLIENT* p_cl, const char* p_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_name);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_log_if_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.name), p_name, IFNAMSIZ);
    
    /* send data */
    if (FPP_ERR_OK == rtn)
    {
        hton_logif(&cmd_to_fci);  /* set correct byte order */
        cmd_to_fci.action = FPP_ACTION_DEREGISTER;
        rtn = fci_write(p_cl, FPP_CMD_LOG_IF, sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci));
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_logif  [localdata_logif]
 * @brief:      Functions marked as [localdata_logif] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_log_if_update() or fci_log_if_update_sa() to
 *              update configuration of the real logical interface in the PFE.
 */
 
 
/*
 * @brief          Enable ("up") a logical interface.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or 
 *                          fci_log_if_get_by_id().
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_enable(fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    p_logif->flags |= FPP_IF_ENABLED;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Disable ("down") a logical interface.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or 
 *                          fci_log_if_get_by_id().
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_disable(fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    p_logif->flags &= (fpp_if_flags_t)(~FPP_IF_ENABLED);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset promiscuous mode of a logical interface.
 * @details        [localdata_logif]
 *                 Promiscuous mode of a logical interface means the interface
 *                 will accept all incoming traffic, regardless of active match rules.
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or 
 *                          fci_log_if_get_by_id().
 * @param[in]      promisc  A request to set/unset the promiscuous mode.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_promisc(fpp_log_if_cmd_t* p_logif, bool promisc)
{
    assert(NULL != p_logif);
    set_flag(p_logif, promisc, FPP_IF_PROMISC);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset loopback mode of a logical interface.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif   Local data to be modified.
 *                           Initial data can be obtained via fci_log_if_get_by_name() or 
 *                           fci_log_if_get_by_id().
 * @param[in]      loopback  A request to set/unset the loopback mode.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_loopback(fpp_log_if_cmd_t* p_logif, bool loopback)
{
    assert(NULL != p_logif);
    set_flag(p_logif, loopback, FPP_IF_LOOPBACK);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set match mode (chaining mode of match rules).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      match_mode_is_or  A request to set match mode.
 *                                   For details about logical interface match modes, see
 *                                   description of a fpp_if_flags_t type in
 *                                   the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_match_mode_or(fpp_log_if_cmd_t* p_logif, bool match_mode_is_or)
{
    assert(NULL != p_logif);
    set_flag(p_logif, match_mode_is_or, FPP_IF_MATCH_OR);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset inverted mode of traffic acceptance.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      discard_on_match  A request to set/unset inverted mode.
 *                                   For details about logical interface inverted mode, see
 *                                   description of a fpp_if_flags_t type in
 *                                   the FCI API Reference.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_discard_on_m(fpp_log_if_cmd_t* p_logif, bool discard_on_match)
{
    assert(NULL != p_logif);
    set_flag(p_logif, discard_on_match, FPP_IF_DISCARD);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set target physical interfaces (egress vector) which 
 *                 shall receive a copy of the accepted traffic.
 * @details        [localdata_logif]
 *                 New egress vector fully replaces the old one.
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      egress   Target physical interfaces (egress vector). A bitset.
 *                          Each physical interface is represented by one bit.
 *                          Conversion between physical interface ID and a corresponding
 *                          egress vector bit is (1u << "physical interface ID").
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_egress_phyifs(fpp_log_if_cmd_t* p_logif, uint32_t egress)
{
    assert(NULL != p_logif);
    p_logif->egress = egress;
    return (FPP_ERR_OK);
}
 
 
 
 
/*
 * @brief          Clear all match rules and zeroify all match rule arguments.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_clear_all_mr(fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    p_logif->match = 0u;
    memset(&(p_logif->arguments), 0, sizeof(fpp_if_m_args_t));
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_eth(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_ETH);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_vlan(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_VLAN);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_pppoe(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_PPPOE);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_arp(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_ARP);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_mcast(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_MCAST);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_ip4(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_IPV4);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_ip6(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_IPV6);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_ipx(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_IPX);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_bcast(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_BCAST);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_udp(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_UDP);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_tcp(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_TCP);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_icmp(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_ICMP);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule.
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_type_igmp(fpp_log_if_cmd_t* p_logif, bool do_set)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_TYPE_IGMP);
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      vlan     New VLAN ID for this match rule.
 *                          When this match rule is active, is compares value of its
 *                          'vlan' argument with value of the traffic's 'VID' field.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_vlan(fpp_log_if_cmd_t* p_logif, bool do_set, uint16_t vlan)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_VLAN);
    p_logif->arguments.vlan = vlan;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      proto    New IP Protocol Number for this match rule.
 *                          When this match rule is active, is compares value of its
 *                          'proto' argument with value of the traffic's 'Protocol' field.
 *                          See "IANA Assigned Internet Protocol Number":
 *                https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_proto(fpp_log_if_cmd_t* p_logif, bool do_set, uint8_t proto)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_PROTO);
    p_logif->arguments.proto = proto;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      sport    New source port value for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'sport' argument with value of the traffic's 'source port' field.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_sport(fpp_log_if_cmd_t* p_logif, bool do_set, uint16_t sport)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_SPORT);
    p_logif->arguments.sport = sport;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      dport    New destination port value for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'dport' argument with value of the traffic's 
 *                          'destination port' field.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_dport(fpp_log_if_cmd_t* p_logif, bool do_set, uint16_t dport)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_DPORT);
    p_logif->arguments.dport = dport;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      p_sip6   New source IPv6 address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'sip' argument with value of the traffic's 
 *                          'source address' (applicable on IPv6 traffic only).
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_sip6(fpp_log_if_cmd_t* p_logif, bool do_set, 
                              const uint32_t p_sip6[4])
{
    assert(NULL != p_logif);
    assert(NULL != p_sip6);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_SIP6);
    memcpy((p_logif->arguments.ipv.v6.sip), p_sip6, (4 * sizeof(uint32_t)));
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      p_dip6   New destination IPv6 address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'dip' argument with value of the traffic's 
 *                          'destination address' (applicable on IPv6 traffic only).
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_dip6(fpp_log_if_cmd_t* p_logif, bool do_set, 
                              const uint32_t p_dip6[4])
{
    assert(NULL != p_logif);
    assert(NULL != p_dip6);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_DIP6);
    memcpy((p_logif->arguments.ipv.v6.dip), p_dip6, (4 * sizeof(uint32_t)));
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      sip      New source IPv4 address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'sip' argument with value of the traffic's 
 *                          'source address' (applicable on IPv4 traffic only).
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_sip(fpp_log_if_cmd_t* p_logif, bool do_set, uint32_t sip)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_SIP);
    p_logif->arguments.ipv.v4.sip = sip;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      dip      New destination IPv4 address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'dip' argument with value of the traffic's 
 *                          'destination address' (applicable on IPv4 traffic only).
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_dip(fpp_log_if_cmd_t* p_logif, bool do_set, uint32_t dip)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_DIP);
    p_logif->arguments.ipv.v4.dip = dip;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      ethtype  New EtherType number for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'ethtype' argument with value of the traffic's 'EtherType' field.
 *                          See "IANA EtherType number (IEEE 802)":
 *                https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_ethtype(fpp_log_if_cmd_t* p_logif, bool do_set, uint16_t ethtype)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_ETHTYPE);
    p_logif->arguments.ethtype = ethtype;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      fp_table0_name  Name of a new FlexibleParser table for this match rule.
 *                                 When this match rule is active, it inspects the traffic
 *                                 according to rules listed in the referenced
 *                                 FlexibleParser table.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_fp0(fpp_log_if_cmd_t* p_logif, bool do_set, 
                             const char* fp_table0_name)
{
    assert(NULL != p_logif);
    /* 'fp_table0_name' is allowed to be NULL */
    
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_FP0);
    return set_text((p_logif->arguments.fp_table0), fp_table0_name, IFNAMSIZ);
}

/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      fp_table0_name  Name of a new FlexibleParser table for this match rule.
 *                                 When this match rule is active, it inspects the traffic
 *                                 according to rules listed in the referenced
 *                                 FlexibleParser table.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_fp1(fpp_log_if_cmd_t* p_logif, bool do_set, 
                             const char* fp_table1_name)
{
    assert(NULL != p_logif);
    /* 'fp_table1_name' is allowed to be NULL */
    
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_FP1);
    return set_text((p_logif->arguments.fp_table1), fp_table1_name, IFNAMSIZ);
}

/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      p_smac   New source MAC address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'smac' argument with value of the traffic's 'source MAC' field.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_smac(fpp_log_if_cmd_t* p_logif, bool do_set, const uint8_t p_smac[6])
{
    assert(NULL != p_logif);
    assert(NULL != p_smac);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_SMAC);
    memcpy((p_logif->arguments.smac), p_smac, (6 * sizeof(uint8_t)));
    return (FPP_ERR_OK);
}

/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      p_dmac   New destination MAC address for this match rule.
 *                          When this match rule is active, it compares value of its
 *                          'dmac' argument with value of the traffic's 
 *                          'destination MAC' field.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_dmac(fpp_log_if_cmd_t* p_logif, bool do_set, const uint8_t p_dmac[6])
{
    assert(NULL != p_logif);
    assert(NULL != p_dmac);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_DMAC);
    memcpy((p_logif->arguments.dmac), p_dmac, (6 * sizeof(uint8_t)));
    return (FPP_ERR_OK);
}

/*
 * @brief          Set/unset the given match rule (and its argument).
 * @details        [localdata_logif]
 * @param[in,out]  p_logif  Local data to be modified.
 *                          Initial data can be obtained via fci_log_if_get_by_name() or
 *                          fci_log_if_get_by_id().
 * @param[in]      do_set   Request to set or unset the given match rule.
 * @param[in]      hif_cookie  New hif cookiee value for this match rule.
 *                             When this match rule is active, it compares value of its
 *                            'hif_cookiee' argument with value of the hif_cookie tag.
 *                             Hif_cookie tag is a part of internal overhead data, attached
 *                             to the traffic by the host's driver.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_log_if_ld_set_mr_hif_cookie(fpp_log_if_cmd_t* p_logif, bool do_set, 
                                    uint32_t hif_cookie)
{
    assert(NULL != p_logif);
    set_mr_flag(p_logif, do_set, FPP_IF_MATCH_HIF_COOKIE);
    p_logif->arguments.hif_cookie = hif_cookie;
    return (FPP_ERR_OK);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query status of an "enable" flag.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 *                      Initial data can be obtained via fci_log_if_get_by_name() or 
 *                      fci_log_if_get_by_id().
 * @return     At time when the data was obtained, the logical interface:
 *             true  : was enabled ("up")
 *             false : was disabled ("down")
 */
bool fci_log_if_ld_is_enabled(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (bool)(FPP_IF_ENABLED & (p_logif->flags));
}
 
 
/*
 * @brief      Query status of an "enable" flag (inverted logic).
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 *                      Initial data can be obtained via fci_log_if_get_by_name() or 
 *                      fci_log_if_get_by_id().
 * @return     At time when the data was obtained, the logical interface:
 *             true  : was disabled ("down")
 *             false : was enabled ("up")
 */
bool fci_log_if_ld_is_disabled(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return !fci_log_if_ld_is_enabled(p_logif);
}
 
 
/*
 * @brief      Query status of a "promiscuous mode" flag.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 *                      Initial data can be obtained via fci_log_if_get_by_name() or 
 *                      fci_log_if_get_by_id().
 * @return     At time when the data was obtained, the logical interface:
 *             true  : was in a promiscuous mode
 *             false : was NOT in a promiscuous mode
 */
bool fci_log_if_ld_is_promisc(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (bool)(FPP_IF_PROMISC & (p_logif->flags));
}
 
 
/*
 * @brief      Query status of a "loopback" flag.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 *                      Initial data can be obtained via fci_log_if_get_by_name() or 
 *                      fci_log_if_get_by_id().
 * @return     At time when the data was obtained, the logical interface:
 *             true  : was in a loopback mode
 *             false : was NOT in a loopback mode
 */
bool fci_log_if_ld_is_loopback(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (bool)(FPP_IF_LOOPBACK & (p_logif->flags));
}
 
 
/*
 * @brief      Query status of a "match mode" flag (chaining mode of match rules).
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 *                      Initial data can be obtained via fci_log_if_get_by_name() or 
 *                      fci_log_if_get_by_id().
 * @return     At time when the data was obtained, the logical interface:
 *             true  : was using OR match mode
 *             false : was using AND match mode
 */
bool fci_log_if_ld_is_match_mode_or(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (bool)(FPP_IF_MATCH_OR & (p_logif->flags));
}
 
 
/*
 * @brief      Query status of a "discard on match" flag.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 *                      Initial data can be obtained via fci_log_if_get_by_name() or 
 *                      fci_log_if_get_by_id().
 * @return     At time when the data was obtained, the logical interface:
 *             true  : was discarding traffic that passed matching process
 *             false : was NOT discarding traffic that passed matching process
 */
bool fci_log_if_ld_is_discard_on_m(const fpp_log_if_cmd_t* p_logif)
{
    assert(NULL != p_logif);
    return (bool)(FPP_IF_DISCARD & (p_logif->flags));
}
 
 
/*
 * @brief      Query whether a physical interface is a member of the egress vector or not.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 *                      Initial data can be obtained via fci_log_if_get_by_name() or 
 *                      fci_log_if_get_by_id().
 * @param[in]  egress_bitflag  Queried physical interface. A bitflag.
 *                             Each physical interface is represented by one bit.
 *                             Conversion between physical interface ID and a corresponding
 *                             egress vector bit is (1u << "physical interface ID").
 *                             Hint: It is recommended to always query only a single bitflag.
 * @return     At time when the data was obtained, the logical interface:
 *             true  : had at least one queried egress bitflag set
 *             false : had none of the queried egress bitflags set
 */
bool fci_log_if_ld_is_egress_phyifs(const fpp_log_if_cmd_t* p_logif, uint32_t egress_bitflag)
{
    assert(NULL != p_logif);
    return (bool)(egress_bitflag & (p_logif->match));
}
 
 
/*
 * @brief      Query whether a match rule is active or not.
 * @details    [localdata_logif]
 * @param[in]  p_logif  Local data to be queried.
 *                      Initial data can be obtained via fci_log_if_get_by_name() or 
 *                      fci_log_if_get_by_id().
 * @param[in]  match_rule  Queried match rule.
 *                         Hint: It is recommended to always query only a single match rule.
 * @return     At time when the data was obtained, the logical interface:
 *             true  : had at least one queried match rule set
 *             false : had none of the queried match rules set
 */
bool fci_log_if_ld_is_match_rule(const fpp_log_if_cmd_t* p_logif, fpp_if_m_rules_t match_rule)
{
    assert(NULL != p_logif);
    return (bool)(match_rule & (p_logif->match));
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all logical interfaces in the PFE and
 *             execute a callback print function for each reported logical interface.
 * @details    To use this function properly, the PFE interface database must be
 *             locked for exclusive access. See fci_log_if_print_all_sa() for 
 *             an example how to lock the database.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next logical interface is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all logical interfaces.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_log_if_print_all(FCI_CLIENT* p_cl, fci_log_if_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_logif(&reply_from_fci);  /* set correct byte order */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                            sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_logif(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more logical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all logical interfaces in the PFE and
 *             execute a callback print function for each reported logical interface.
 * @details    This is a standalone (_sa) function.
 *             It shows how to properly access a logical interface. Namely:
 *             1. Lock the interface database for exclusive access by this FCI client.
 *             2. Execute one or more FCI calls which access 
 *                physical or logical interfaces.
 *             3. Unlock the interface database's exclusive access lock.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next logical interface is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all logical interfaces.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_log_if_print_all_sa(FCI_CLIENT* p_cl, fci_log_if_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    /* lock the interface database for exclusive access by this FCI client */
    rtn = fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0, NULL);
    
    /* execute "payload" - FCI calls which access physical or logical interfaces */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_log_if_print_all(p_cl, p_cb_print);
    }
    
    /* unlock the interface database's exclusive access lock */
    /* result of the unlock action is returned only if previous "payload" actions were OK */
    const int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all logical interfaces in the PFE which
 *             are children of a given parent physical interface. Execute a print function
 *             for each reported logical interface.
 * @details    To use this function properly, the PFE interface database must be
 *             locked for exclusive access. See fci_log_if_print_all_sa() for 
 *             an example how to lock the database.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next logical interface is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @param[in]   p_parent_name  Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See the FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through all suitable logical interfaces.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_log_if_print_by_parent(FCI_CLIENT* p_cl, fci_log_if_cb_print_t p_cb_print, 
                               const char* p_parent_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_parent_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_logif(&reply_from_fci);  /* set correct byte order */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        if (0 == strcmp((reply_from_fci.parent_name), p_parent_name))
        {
            rtn = p_cb_print(&reply_from_fci);
        }
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                            sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_logif(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more logical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all logical interfaces in the PFE.
 * @details     To use this function properly, the PFE interface database must be
 *              locked for exclusive access. See fci_log_if_print_all_sa() for 
 *              an example how to lock the database.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of logical interfaces.
 * @return      FPP_ERR_OK : Successfully counted logical interfaces.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_log_if_get_count(FCI_CLIENT* p_cl, uint16_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting interfaces) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                        sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting interfaces) */
    }
    
    /* query loop runs till there are no more logical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all logical interfaces in the PFE which
 *              are children of a given parent physical interface.
 * @details     To use this function properly, the PFE interface database must be
 *              locked for exclusive access. See fci_log_if_print_all_sa() for 
 *              an example how to lock the database.
 * @param[in]   p_cl           FCI client instance
 * @param[out]  p_rtn_count    Space to store the count of logical interfaces.
 * @param[in]   p_parent_name  Name of a parent physical interface.
 *                             Names of physical interfaces are hardcoded.
 *                             See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted logical interfaces.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_log_if_get_count_by_parent(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, 
                                   const char* p_parent_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_log_if_cmd_t cmd_to_fci = {0};
    fpp_log_if_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint16_t count = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                    sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting interfaces) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        if (0 == strcmp((reply_from_fci.parent_name), p_parent_name))
        {
            count++;
        }
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_LOG_IF,
                        sizeof(fpp_log_if_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting interfaces) */
    }
    
    /* query loop runs till there are no more logical interfaces to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_IF_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
