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
#include "fci_qos.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief          Network-to-host (ntoh) function for a queue struct.
 * @param[in,out]  p_rtn_que  The queue struct to be converted.
 */
static void ntoh_que(fpp_qos_queue_cmd_t* p_rtn_que)
{
    assert(NULL != p_rtn_que);
    
    
    p_rtn_que->min = ntohl(p_rtn_que->min);
    p_rtn_que->max = ntohl(p_rtn_que->max);
}
 
 
/*
 * @brief          Host-to-network (hton) function for a queue struct.
 * @param[in,out]  p_rtn_que  The queue struct to be converted.
 */
static void hton_que(fpp_qos_queue_cmd_t* p_rtn_que)
{
    assert(NULL != p_rtn_que);
    
    
    p_rtn_que->min = htonl(p_rtn_que->min);
    p_rtn_que->max = htonl(p_rtn_que->max);
}
 
 
/*
 * @brief          Network-to-host (ntoh) function for a scheduler struct.
 * @param[in,out]  p_rtn_sch  The scheduler struct to be converted.
 */
static void ntoh_sch(fpp_qos_scheduler_cmd_t* p_rtn_sch)
{
    assert(NULL != p_rtn_sch);
    
    
    p_rtn_sch->input_en = ntohl(p_rtn_sch->input_en);
    for (uint8_t i = 0u; (32u > i); (++i))
    {
        p_rtn_sch->input_w[i] = ntohl(p_rtn_sch->input_w[i]);
    }
}
 
 
/*
 * @brief          Host-to-network (hton) function for a scheduler struct.
 * @param[in,out]  p_rtn_sch  The scheduler struct to be converted.
 */
static void hton_sch(fpp_qos_scheduler_cmd_t* p_rtn_sch)
{
    assert(NULL != p_rtn_sch);
    
    
    p_rtn_sch->input_en = htonl(p_rtn_sch->input_en);
    for (uint8_t i = 0u; (32u > i); (++i))
    {
        p_rtn_sch->input_w[i] = htonl(p_rtn_sch->input_w[i]);
    }
}
 
 
/*
 * @brief          Network-to-host (ntoh) function for a shaper struct.
 * @param[in,out]  p_rtn_shp  The shaper struct to be converted.
 */
static void ntoh_shp(fpp_qos_shaper_cmd_t* p_rtn_shp)
{
    assert(NULL != p_rtn_shp);
    
    
    p_rtn_shp->isl = ntohl(p_rtn_shp->isl);
    p_rtn_shp->max_credit = (int32_t)(ntohl(p_rtn_shp->max_credit));
    p_rtn_shp->min_credit = (int32_t)(ntohl(p_rtn_shp->min_credit));
}
 
 
/*
 * @brief          Host-to-network (hton) function for a shaper struct.
 * @param[in,out]  p_rtn_shp  The shaper struct to be converted.
 */
static void hton_shp(fpp_qos_shaper_cmd_t* p_rtn_shp)
{
    assert(NULL != p_rtn_shp);
    
    
    p_rtn_shp->isl = htonl(p_rtn_shp->isl);
    p_rtn_shp->max_credit = (int32_t)(htonl(p_rtn_shp->max_credit));
    p_rtn_shp->min_credit = (int32_t)(htonl(p_rtn_shp->min_credit));
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested QoS queue
 *              from the PFE. Identify the QoS queue by name of the parent 
 *              physical interface and by the queue's ID.
 * @param[in]   p_cl          FCI client instance
 * @param[out]  p_rtn_que     Space for data from the PFE.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 *              que_id        ID of the requested QoS queue.
 * @return      FPP_ERR_OK : Requested QoS queue was found.
 *                           A copy of its configuration was stored into p_rtn_que.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_qos_que_get_by_id(FCI_CLIENT* p_cl, fpp_qos_queue_cmd_t* p_rtn_que, 
                          const char* p_phyif_name, uint8_t que_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_que);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_queue_cmd_t cmd_to_fci = {0};
    fpp_qos_queue_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.id = que_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_QUEUE,
                        sizeof(fpp_qos_queue_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_que(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_que = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested QoS scheduler
 *              from the PFE. Identify the QoS scheduler by name of the parent 
 *              physical interface and by the scheduler's ID.
 * @param[in]   p_cl          FCI client instance
 * @param[out]  p_rtn_que     Space for data from the PFE.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 *              sch_id        ID of the requested QoS scheduler.
 * @return      FPP_ERR_OK : Requested QoS scheduler was found.
 *                           A copy of its configuration was stored into p_rtn_sch.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_qos_sch_get_by_id(FCI_CLIENT* p_cl, fpp_qos_scheduler_cmd_t* p_rtn_sch, 
                          const char* p_phyif_name, uint8_t sch_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_sch);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_scheduler_cmd_t cmd_to_fci = {0};
    fpp_qos_scheduler_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.id = sch_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_SCHEDULER,
                        sizeof(fpp_qos_scheduler_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_sch(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_sch = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested QoS shaper
 *              from the PFE. Identify the QoS shaper by name of the parent 
 *              physical interface and by the shaper's ID.
 * @param[in]   p_cl          FCI client instance
 * @param[out]  p_rtn_que     Space for data from the PFE.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 *              shp_id        ID of the requested QoS shaper.
 * @return      FPP_ERR_OK : Requested QoS shaper was found.
 *                           A copy of its configuration was stored into p_rtn_shp.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_qos_shp_get_by_id(FCI_CLIENT* p_cl, fpp_qos_shaper_cmd_t* p_rtn_shp, 
                          const char* p_phyif_name, uint8_t shp_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_shp);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_shaper_cmd_t cmd_to_fci = {0};
    fpp_qos_shaper_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.id = shp_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* start query process */
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_SHAPER,
                        sizeof(fpp_qos_shaper_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_shp(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_shp = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target QoS queue
 *                 in the PFE.
 * @param[in]      p_cl   FCI client instance
 * @param[in,out]  p_que  Data struct which represents a new configuration of 
 *                        the target QoS queue.
 *                        Initial data can be obtained via fci_qos_que_get_by_id().
 * @return         FPP_ERR_OK : Configuration of the QoS queue was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
int fci_qos_que_update(FCI_CLIENT* p_cl, fpp_qos_queue_cmd_t* p_que)
{
    assert(NULL != p_cl);
    assert(NULL != p_que);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_qos_queue_cmd_t cmd_to_fci = (*p_que);
    
    /* send data */
    hton_que(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_QOS_QUEUE, sizeof(fpp_qos_queue_cmd_t), 
                                        (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_qos_que_get_by_id(p_cl, p_que, (p_que->if_name), (p_que->id));
    }
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target QoS scheduler
 *                 in the PFE.
 * @param[in]      p_cl   FCI client instance
 * @param[in,out]  p_que  Data struct which represents a new configuration of 
 *                        the target QoS scheduler.
 *                        Initial data can be obtained via fci_qos_sch_get_by_id().
 * @return         FPP_ERR_OK : Configuration of the QoS scheduler was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
int fci_qos_sch_update(FCI_CLIENT* p_cl, fpp_qos_scheduler_cmd_t* p_sch)
{
    assert(NULL != p_cl);
    assert(NULL != p_sch);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_qos_scheduler_cmd_t cmd_to_fci = (*p_sch);
    
    /* send data */
    hton_sch(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_QOS_SCHEDULER, sizeof(fpp_qos_scheduler_cmd_t), 
                                                (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_qos_sch_get_by_id(p_cl, p_sch, (p_sch->if_name), (p_sch->id));
    }
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target QoS shaper
 *                 in the PFE.
 * @param[in]      p_cl   FCI client instance
 * @param[in,out]  p_que  Data struct which represents a new configuration of 
 *                        the target QoS shaper.
 *                        Initial data can be obtained via fci_qos_shp_get_by_id().
 * @return         FPP_ERR_OK : Configuration of the QoS shaper was
 *                              successfully updated in the PFE.
 *                              Local data struct was automatically updated with 
 *                              readback data from the PFE.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data struct not updated.
 */
int fci_qos_shp_update(FCI_CLIENT* p_cl, fpp_qos_shaper_cmd_t* p_shp)
{
    assert(NULL != p_cl);
    assert(NULL != p_shp);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_qos_shaper_cmd_t cmd_to_fci = (*p_shp);
    
    /* send data */
    hton_shp(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_QOS_SHAPER, sizeof(fpp_qos_shaper_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_qos_shp_get_by_id(p_cl, p_shp, (p_shp->if_name), (p_shp->id));
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_que  [localdata_que]
 * @brief:      Functions marked as [localdata_que] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_qos_que_update() to 
 *              update the given QoS queue in the PFE.
 */
 
 
/*
 * @brief          Set mode (queue discipline) of a QoS queue.
 * @details        [localdata_que]
 * @param[in,out]  p_que     Local data to be modified.
 *                           Initial data can be obtained via fci_qos_que_get_by_id().
 * @param[in]      que_mode  queue mode (queue discipline)
 *                           For valid modes, see the FCI API Reference, 
 *                           chapter 'queue mode'.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_que_ld_set_mode(fpp_qos_queue_cmd_t* p_que, uint8_t que_mode)
{
    assert(NULL != p_que);
    p_que->mode = que_mode;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set minimal threshold of a QoS queue.
 * @details        [localdata_que]
 *                 Meaning of minimal threshold depends on que mode.
 * @param[in,out]  p_que  Local data to be modified.
 *                        Initial data can be obtained via fci_qos_que_get_by_id().
 * @param[in]      min    minimal threshold
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_que_ld_set_min(fpp_qos_queue_cmd_t* p_que, uint32_t min)
{
    assert(NULL != p_que);
    p_que->min = min;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set queue mode (queue discipline) of a QoS queue.
 * @details        [localdata_que]
 *                 Meaning of maximal threshold depends on que mode.
 * @param[in,out]  p_que  Local data to be modified.
 *                        Initial data can be obtained via fci_qos_que_get_by_id().
 * @param[in]      max    maximal threshold
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_que_ld_set_max(fpp_qos_queue_cmd_t* p_que, uint32_t max)
{
    assert(NULL != p_que);
    p_que->max = max;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set packet drop probability of a particular QoS queue's zone.
 * @details        [localdata_que]
 *                 Meaningful only for que mode WRED.
 * @param[in,out]  p_que  Local data to be modified.
 *                        Initial data can be obtained via fci_qos_que_get_by_id().
 * @param[in]      zprob_id    id of a probability zone
 *                             There may be less than 32 zones actually implemented in PFE.
 *                             (32 is just the max array limit)
 *                             See the FCI API Reference, chapter Egress QoS.
 * @param[in]      percentage  drop probability in [%]
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_que_ld_set_zprob(fpp_qos_queue_cmd_t* p_que, uint8_t zprob_id, 
                             uint8_t percentage)
{
    assert(NULL != p_que);
    int rtn = FPP_ERR_FCI;
    if (32u > zprob_id)
    {
        p_que->zprob[zprob_id] = percentage;
        rtn = FPP_ERR_OK;
    }
    return (rtn);
}
 
 
/*
 * @defgroup    localdata_sch  [localdata_sch]
 * @brief:      Functions marked as [localdata_sch] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_qos_sch_update() to 
 *              update the given QoS scheduler in the PFE.
 */
 
 
/*
 * @brief          Set mode of a QoS scheduler.
 * @details        [localdata_sch]
 * @param[in,out]  p_sch     Local data to be modified.
 *                           Initial data can be obtained via fci_qos_sch_get_by_id().
 * @param[in]      sch_mode  scheduler mode
 *                           For valid modes, see the FCI API Reference, 
 *                           chapter 'schedulermode'.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_sch_ld_set_mode(fpp_qos_scheduler_cmd_t* p_sch, uint8_t sch_mode)
{
    assert(NULL != p_sch);
    p_sch->mode = sch_mode;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set selection algorithm of a QoS scheduler.
 * @details        [localdata_sch]
 * @param[in,out]  p_sch     Local data to be modified.
 *                           Initial data can be obtained via fci_qos_sch_get_by_id().
 * @param[in]      algo      selection algorithm
 *                           For valid modes, see the FCI API Reference, 
 *                           chapter 'scheduler algorithm'.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_sch_ld_set_algo(fpp_qos_scheduler_cmd_t* p_sch, uint8_t algo)
{
    assert(NULL != p_sch);
    p_sch->algo = algo;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set input of a QoS scheduler.
 * @details        [localdata_sch]
 * @param[in,out]  p_sch     Local data to be modified.
 *                           Initial data can be obtained via fci_qos_sch_get_by_id().
 * @param[in]      input_id  ID of the scheduler's input.
 *                           There may be less than 32 inputs per scheduler 
 *                           actually implemented in PFE. (32 is just the max array limit)
 *                           See the FCI API Reference, chapter Egress QoS.
 *                 enable    A request to enable/disable the given scheduler input.
 *                 src       Data source which is connected to the given sscheduler input.
 *                           See the FCI API Reference, chapter Egress QoS.
 *                 weight    Weight ("importance") of the given scheduler input.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_sch_ld_set_input(fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id, 
                             bool enable, uint8_t src, uint32_t weight)
{
    assert(NULL != p_sch);
    
    int rtn = FPP_ERR_FCI;
    if (32u > input_id)
    {
        if (enable)
        {
            p_sch->input_en |= (1uL << input_id);
        }
        else
        {
            p_sch->input_en &= ~(1uL << input_id);
        }
        
        p_sch->input_src[input_id] = src;
        p_sch->input_w[input_id] = weight;
        
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @defgroup    localdata_shp  [localdata_shp]
 * @brief:      Functions marked as [localdata_shp] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_qos_shp_update() to 
 *              update the given QoS shaper in the PFE.
 */
 
 
/*
 * @brief          Set mode of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp     Local data to be modified.
 *                           Initial data can be obtained via fci_qos_shp_get_by_id().
 * @param[in]      shp_mode  shaper mode
 *                           For valid modes, see the FCI API Reference, 
 *                           chapter 'shapermode'.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_shp_ld_set_mode(fpp_qos_shaper_cmd_t* p_shp, uint8_t shp_mode)
{
    assert(NULL != p_shp);
    p_shp->mode = shp_mode;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set position of a QoS shaper in a QoS configuration.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp     Local data to be modified.
 *                           Initial data can be obtained via fci_qos_shp_get_by_id().
 * @param[in]      position  position of the QoS shaper
 *                           For valid positions, see the FCI API Reference, 
 *                           chapter Egress QoS.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_shp_ld_set_position(fpp_qos_shaper_cmd_t* p_shp, uint8_t position)
{
    assert(NULL != p_shp);
    p_shp->position = position;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set idle slope rate of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp     Local data to be modified.
 *                           Initial data can be obtained via fci_qos_shp_get_by_id().
 * @param[in]      isl       idle slope rate (units per second)
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_shp_ld_set_isl(fpp_qos_shaper_cmd_t* p_shp, uint32_t isl)
{
    assert(NULL != p_shp);
    p_shp->isl = isl;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set minimal credit of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp       Local data to be modified.
 *                             Initial data can be obtained via fci_qos_shp_get_by_id().
 * @param[in]      min_credit  minimal credit
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_shp_ld_set_min_credit(fpp_qos_shaper_cmd_t* p_shp, int32_t min_credit)
{
    assert(NULL != p_shp);
    p_shp->min_credit = min_credit;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set maximal credit of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp       Local data to be modified.
 *                             Initial data can be obtained via fci_qos_shp_get_by_id().
 * @param[in]      min_credit  maximal credit
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_qos_shp_ld_set_max_credit(fpp_qos_shaper_cmd_t* p_shp, int32_t max_credit)
{
    assert(NULL != p_shp);
    p_shp->max_credit = max_credit;
    return (FPP_ERR_OK);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query whether an input of a QoS scheduler is enabled or not.
 * @details    [localdata_sch]
 * @param[in]  p_sch  Local data to be queried.
 *                    Initial data can be obtained via fci_qos_sch_get_by_id().
 * @param[in]  input_id  Queried scheduler input.
 * @return     At time when the data was obtained, the given scheduler input:
 *             true  : was enabled
 *             false : was disabled
 */
bool fci_qos_sch_ld_is_input_enabled(const fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id)
{
    assert(NULL != p_sch);
    return (bool)((32u > input_id) ? ((1uL << input_id) & (p_sch->input_en)) : (false));
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through QoS queues of the given physical
 *             interface and execute a callback print function for each QoS queue.
 * @param[in]  p_cl           FCI client instance
 * @param[in]  p_cb_print     Callback print function.
 *                            --> If the callback returns zero, then all is OK and 
 *                                the next QoS queue is picked for a print process.
 *                            --> If the callback returns non-zero, then some problem is 
 *                                assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name   Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through QoS queues of 
 *                          the given physical interface.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_qos_que_print_by_phyif(FCI_CLIENT* p_cl, fci_qos_que_cb_print_t p_cb_print, 
                               const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_queue_cmd_t cmd_to_fci = {0};
    fpp_qos_queue_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t que_id = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.id = que_id;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_QUEUE,
                            sizeof(fpp_qos_queue_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_que(&reply_from_fci);  /* set correct byte order */
            
            if (FPP_ERR_OK == rtn)
            {
                rtn = p_cb_print(&reply_from_fci);
            }
            
            que_id++;
        }
        
        /* query loop runs till there are no more QoS queues to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_QOS_QUEUE_NOT_FOUND == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all QoS queues in the PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client instance
 * @param[out]  p_rtn_count   Space to store the count of QoS queues.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted QoS queues.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_qos_que_get_count_by_phyif(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, 
                                   const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_queue_cmd_t cmd_to_fci = {0};
    fpp_qos_queue_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t que_id = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.id = que_id;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_QUEUE,
                            sizeof(fpp_qos_queue_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            /* no need to set correct byte order (we are just counting QoS queues) */
            
            if (FPP_ERR_OK == rtn)
            {
                que_id++;
            }
        }
        
        /* query loop runs till there are no more QoS queues to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_QOS_QUEUE_NOT_FOUND == rtn)
        {
            *p_rtn_count = que_id;
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through QoS schedulers of the given physical
 *             interface and execute a callback print function for each QoS scheduler.
 * @param[in]  p_cl           FCI client instance
 * @param[in]  p_cb_print     Callback print function.
 *                            --> If the callback returns zero, then all is OK and 
 *                                the next QoS scheduler is picked for a print process.
 *                            --> If the callback returns non-zero, then some problem is 
 *                                assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name   Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through QoS schedulers of 
 *                          the given physical interface.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_qos_sch_print_by_phyif(FCI_CLIENT* p_cl, fci_qos_sch_cb_print_t p_cb_print, 
                               const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_scheduler_cmd_t cmd_to_fci = {0};
    fpp_qos_scheduler_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t sch_id = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.id = sch_id;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_SCHEDULER,
                            sizeof(fpp_qos_scheduler_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_sch(&reply_from_fci);  /* set correct byte order */
            
            if (FPP_ERR_OK == rtn)
            {
                rtn = p_cb_print(&reply_from_fci);
            }
            
            sch_id++;
        }
        
        /* query loop runs till there are no more QoS schedulers to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_QOS_SCHEDULER_NOT_FOUND == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all QoS schedulers in the PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client instance
 * @param[out]  p_rtn_count   Space to store the count of QoS schedulers.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted QoS schedulers.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_qos_sch_get_count_by_phyif(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, 
                                   const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_scheduler_cmd_t cmd_to_fci = {0};
    fpp_qos_scheduler_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t sch_id = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.id = sch_id;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_SCHEDULER,
                            sizeof(fpp_qos_scheduler_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            /* no need to set correct byte order (we are just counting QoS schedulers) */
            
            if (FPP_ERR_OK == rtn)
            {
                sch_id++;
            }
        }
        
        /* query loop runs till there are no more QoS schedulers to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_QOS_SCHEDULER_NOT_FOUND == rtn)
        {
            *p_rtn_count = sch_id;
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through QoS shapers of the given physical
 *             interface and execute a callback print function for each QoS shaper.
 * @param[in]  p_cl           FCI client instance
 * @param[in]  p_cb_print     Callback print function.
 *                            --> If the callback returns zero, then all is OK and 
 *                                the next QoS shaper is picked for a print process.
 *                            --> If the callback returns non-zero, then some problem is 
 *                                assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name   Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through QoS shapers of 
 *                          the given physical interface.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_qos_shp_print_by_phyif(FCI_CLIENT* p_cl, fci_qos_shp_cb_print_t p_cb_print, 
                               const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_shaper_cmd_t cmd_to_fci = {0};
    fpp_qos_shaper_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t shp_id = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.id = shp_id;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_SHAPER,
                            sizeof(fpp_qos_shaper_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_shp(&reply_from_fci);  /* set correct byte order */
            
            if (FPP_ERR_OK == rtn)
            {
                rtn = p_cb_print(&reply_from_fci);
            }
            
            shp_id++;
        }
        
        /* query loop runs till there are no more QoS shapers to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_QOS_SHAPER_NOT_FOUND == rtn)
        {
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all QoS shapers in the PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client instance
 * @param[out]  p_rtn_count   Space to store the count of QoS shapers.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See the FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted QoS shapers.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_qos_shp_get_count_by_phyif(FCI_CLIENT* p_cl, uint16_t* p_rtn_count, 
                                   const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_qos_shaper_cmd_t cmd_to_fci = {0};
    fpp_qos_shaper_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query */
    if (FPP_ERR_OK == rtn)
    {
        /* query loop */
        uint8_t shp_id = 0u;
        while (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.id = shp_id;
            cmd_to_fci.action = FPP_ACTION_QUERY;
            rtn = fci_query(p_cl, FPP_CMD_QOS_SHAPER,
                            sizeof(fpp_qos_shaper_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            /* no need to set correct byte order (we are just counting QoS queues) */
            
            if (FPP_ERR_OK == rtn)
            {
                shp_id++;
            }
        }
        
        /* query loop runs till there are no more QoS shapers to report */
        /* the following error is therefore OK and expected (it ends the query loop) */
        if (FPP_ERR_QOS_SHAPER_NOT_FOUND == rtn)
        {
            *p_rtn_count = shp_id;
            rtn = FPP_ERR_OK;
        }
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
