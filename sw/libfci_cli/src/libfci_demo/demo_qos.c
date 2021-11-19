/* =========================================================================
 *  Copyright 2020-2021 NXP
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
 
 
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
 
#include <stdint.h>
#include <stdbool.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
#include "demo_common.h"
#include "demo_qos.h"
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested QoS queue
 *              from PFE. Identify the QoS queue by the name of a parent 
 *              physical interface and by the queue's ID.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_que     Space for data from PFE.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 *              que_id        ID of the requested QoS queue.
 * @return      FPP_ERR_OK : The requested QoS queue was found.
 *                           A copy of its configuration data was stored into p_rtn_que.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_qos_que_get_by_id(FCI_CLIENT* p_cl, fpp_qos_queue_cmd_t* p_rtn_que, 
                           const char* p_phyif_name, uint8_t que_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_que);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_queue_cmd_t cmd_to_fci = {0};
    fpp_qos_queue_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.id = que_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query (get the QoS queue directly; no need for a loop) */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_QUEUE,
                        sizeof(fpp_qos_queue_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_que = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_qos_que_get_by_id() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested QoS scheduler
 *              from PFE. Identify the QoS scheduler by the name of a parent 
 *              physical interface and by the scheduler's ID.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_que     Space for data from PFE.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 *              sch_id        ID of the requested QoS scheduler.
 * @return      FPP_ERR_OK : The requested QoS scheduler was found.
 *                           A copy of its configuration data was stored into p_rtn_sch.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_qos_sch_get_by_id(FCI_CLIENT* p_cl, fpp_qos_scheduler_cmd_t* p_rtn_sch, 
                           const char* p_phyif_name, uint8_t sch_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_sch);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_scheduler_cmd_t cmd_to_fci = {0};
    fpp_qos_scheduler_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.id = sch_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query (get the QoS scheduler directly; no need for a loop) */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_SCHEDULER,
                        sizeof(fpp_qos_scheduler_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_sch = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_qos_sch_get_by_id() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested QoS shaper
 *              from PFE. Identify the QoS shaper by the name of a parent 
 *              physical interface and by the shaper's ID.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_que     Space for data from PFE.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 *              shp_id        ID of the requested QoS shaper.
 * @return      FPP_ERR_OK : The requested QoS shaper was found.
 *                           A copy of its configuration data was stored into p_rtn_shp.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_qos_shp_get_by_id(FCI_CLIENT* p_cl, fpp_qos_shaper_cmd_t* p_rtn_shp, 
                           const char* p_phyif_name, uint8_t shp_id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_shp);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_qos_shaper_cmd_t cmd_to_fci = {0};
    fpp_qos_shaper_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* prepare data */
    cmd_to_fci.id = shp_id;
    rtn = set_text((cmd_to_fci.if_name), p_phyif_name, IFNAMSIZ);
    
    /* do the query (get the QoS shaper directly; no need for a loop) */
    if (FPP_ERR_OK == rtn)
    {
        cmd_to_fci.action = FPP_ACTION_QUERY;
        rtn = fci_query(p_cl, FPP_CMD_QOS_SHAPER,
                        sizeof(fpp_qos_shaper_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_shp = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_qos_shp_get_by_id() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
 
/*
 * @brief          Use FCI calls to update configuration of a target QoS queue
 *                 in PFE.
 * @param[in]      p_cl   FCI client
 * @param[in,out]  p_que  Local data struct which represents a new configuration of 
 *                        the target QoS queue.
 *                        Initial data can be obtained via demo_qos_que_get_by_id().
 * @return        FPP_ERR_OK : Configuration of the target QoS queue was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             The local data struct not updated.
 */
int demo_qos_que_update(FCI_CLIENT* p_cl, fpp_qos_queue_cmd_t* p_que)
{
    assert(NULL != p_cl);
    assert(NULL != p_que);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_qos_queue_cmd_t cmd_to_fci = (*p_que);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_QOS_QUEUE, sizeof(fpp_qos_queue_cmd_t), 
                                        (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_que_get_by_id(p_cl, p_que, (p_que->if_name), (p_que->id));
    }
    
    print_if_error(rtn, "demo_qos_que_update() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target QoS scheduler
 *                 in PFE.
 * @param[in]      p_cl   FCI client
 * @param[in,out]  p_sch  Local data struct which represents a new configuration of 
 *                        the target QoS scheduler.
 *                        Initial data can be obtained via demo_qos_sch_get_by_id().
 * @return        FPP_ERR_OK : Configuration of the target QoS scheduler was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             The local data struct not updated.
 */
int demo_qos_sch_update(FCI_CLIENT* p_cl, fpp_qos_scheduler_cmd_t* p_sch)
{
    assert(NULL != p_cl);
    assert(NULL != p_sch);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_qos_scheduler_cmd_t cmd_to_fci = (*p_sch);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_QOS_SCHEDULER, sizeof(fpp_qos_scheduler_cmd_t), 
                                                (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_sch_get_by_id(p_cl, p_sch, (p_sch->if_name), (p_sch->id));
    }
    
    print_if_error(rtn, "demo_qos_sch_update() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief          Use FCI calls to update configuration of a target QoS shaper
 *                 in PFE.
 * @param[in]      p_cl   FCI client
 * @param[in,out]  p_shp  Local data struct which represents a new configuration of 
 *                        the target QoS shaper.
 *                        Initial data can be obtained via demo_qos_shp_get_by_id().
 * @return        FPP_ERR_OK : Configuration of the target QoS shaper was
 *                             successfully updated in PFE.
 *                             The local data struct was automatically updated with 
 *                             readback data from PFE.
 *                other      : Some error occurred (represented by the respective error code).
 *                             The local data struct not updated.
 */
int demo_qos_shp_update(FCI_CLIENT* p_cl, fpp_qos_shaper_cmd_t* p_shp)
{
    assert(NULL != p_cl);
    assert(NULL != p_shp);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_qos_shaper_cmd_t cmd_to_fci = (*p_shp);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_QOS_SHAPER, sizeof(fpp_qos_shaper_cmd_t), 
                                             (unsigned short*)(&cmd_to_fci));
    
    /* read back and update caller data */
    if (FPP_ERR_OK == rtn)
    {
        rtn = demo_qos_shp_get_by_id(p_cl, p_shp, (p_shp->if_name), (p_shp->id));
    }
    
    print_if_error(rtn, "demo_qos_shp_update() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_que  [localdata_que]
 * @brief:      Functions marked as [localdata_que] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_que (a struct with configuration data).
 *              Initial data for p_que can be obtained via demo_qos_que_get_by_id().
 *              If some modifications are made to local data, then after all modifications
 *              are done and finished, call demo_qos_que_update() to update
 *              the configuration of a real QoS queue in PFE.
 */
 
 
/*
 * @brief          Set a mode (queue discipline) of a QoS queue.
 * @details        [localdata_que]
 * @param[in,out]  p_que     Local data to be modified.
 * @param[in]      que_mode  Queue mode (queue discipline).
 *                           For valid modes, see FCI API Reference, 
 *                           chapter 'fpp_qos_queue_cmd_t'.
 */
void demo_qos_que_ld_set_mode(fpp_qos_queue_cmd_t* p_que, uint8_t que_mode)
{
    assert(NULL != p_que);
    p_que->mode = que_mode;
}
 
 
/*
 * @brief          Set a minimal threshold of a QoS queue.
 * @details        [localdata_que]
 *                 Meaning of a minimal threshold depends on 
 *                 a queue mode of the given QoS queue.
 * @param[in,out]  p_que  Local data to be modified.
 * @param[in]      min    Minimal threshold.
 */
void demo_qos_que_ld_set_min(fpp_qos_queue_cmd_t* p_que, uint32_t min)
{
    assert(NULL != p_que);
    p_que->min = htonl(min);
}
 
 
/*
 * @brief          Set a maximal threshold of a QoS queue.
 * @details        [localdata_que]
 *                 Meaning of a maximal threshold depends on 
 *                 a queue mode of the given QoS queue.
 * @param[in,out]  p_que  Local data to be modified.
 * @param[in]      max    Maximal threshold.
 */
void demo_qos_que_ld_set_max(fpp_qos_queue_cmd_t* p_que, uint32_t max)
{
    assert(NULL != p_que);
    p_que->max = htonl(max);
}
 
 
/*
 * @brief          Set packet drop probability of a particular QoS queue's zone.
 * @details        [localdata_que]
 *                 Meaningful only for the que mode WRED.
 * @param[in,out]  p_que     Local data to be modified.
 * @param[in]      zprob_id  ID of a probability zone.
 *                           There may be less than 32 zones actually implemented in PFE.
 *                           (32 is just the max array limit)
 *                           See FCI API Reference, chapter Egress QoS.
 * @param[in]      percentage  Drop probability in [%].
 */
void demo_qos_que_ld_set_zprob(fpp_qos_queue_cmd_t* p_que, uint8_t zprob_id, 
                               uint8_t percentage)
{
    assert(NULL != p_que);
    if (32u > zprob_id)
    {
        p_que->zprob[zprob_id] = percentage;
    }
}
 
 
 
 
/*
 * @defgroup    localdata_sch  [localdata_sch]
 * @brief:      Functions marked as [localdata_sch] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_sch (a struct with configuration data).
 *              Initial data for p_sch can be obtained via demo_qos_sch_get_by_id().
 *              If some modifications are made to local data, then after all modifications
 *              are done and finished, call demo_qos_sch_update() to update
 *              the configuration of a real QoS scheduler in PFE.
 */
 
 
/*
 * @brief          Set a mode of a QoS scheduler.
 * @details        [localdata_sch]
 * @param[in,out]  p_sch     Local data to be modified.
 * @param[in]      sch_mode  Scheduler mode.
 *                           For valid modes, see FCI API Reference, 
 *                           chapter 'fpp_qos_scheduler_cmd_t'.
 */
void demo_qos_sch_ld_set_mode(fpp_qos_scheduler_cmd_t* p_sch, uint8_t sch_mode)
{
    assert(NULL != p_sch);
    p_sch->mode = sch_mode;
}
 
 
/*
 * @brief          Set a selection algorithm of a QoS scheduler.
 * @details        [localdata_sch]
 * @param[in,out]  p_sch  Local data to be modified.
 * @param[in]      algo   Selection algorithm.
 *                        For valid modes, see the FCI API Reference, 
 *                        chapter 'fpp_qos_scheduler_cmd_t'.
 */
void demo_qos_sch_ld_set_algo(fpp_qos_scheduler_cmd_t* p_sch, uint8_t algo)
{
    assert(NULL != p_sch);
    p_sch->algo = algo;
}
 
 
/*
 * @brief          Set an input (and its properties) of a QoS scheduler.
 * @details        [localdata_sch]
 * @param[in,out]  p_sch     Local data to be modified.
 * @param[in]      input_id  ID of the scheduler's input.
 *                           There may be less than 32 inputs per scheduler 
 *                           actually implemented in PFE. (32 is just the max array limit)
 *                           See FCI API Reference, chapter Egress QoS.
 *                 enable    Request to enable/disable the given scheduler input.
 *                 src       Data source which is connected to the given sscheduler input.
 *                           See FCI API Reference, chapter Egress QoS.
 *                 weight    Weight ("importance") of the given scheduler input.
 */
void demo_qos_sch_ld_set_input(fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id, 
                               bool enable, uint8_t src, uint32_t weight)
{
    assert(NULL != p_sch);
    
    if (32u > input_id)
    {
        if (enable)
        {
            p_sch->input_en |= htonl(1uL << input_id);
        }
        else
        {
            p_sch->input_en &= htonl((uint32_t)(~(1uL << input_id)));
        }
        
        p_sch->input_w[input_id] = htonl(weight);
        p_sch->input_src[input_id] = src;
    }
}
 
 
 
 
/*
 * @defgroup    localdata_shp  [localdata_shp]
 * @brief:      Functions marked as [localdata_shp] access only local data.
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_shp (a struct with configuration data).
 *              Initial data for p_shp can be obtained via demo_qos_shp_get_by_id().
 *              If some modifications are made to local data, then after all modifications
 *              are done and finished, call demo_shp_sch_update() to update
 *              the configuration of a real QoS shaper in PFE.
 */
 
 
/*
 * @brief          Set a mode of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp     Local data to be modified.
 * @param[in]      shp_mode  Shaper mode.
 *                           For valid modes, see FCI API Reference, 
 *                           chapter 'fpp_qos_shaper_cmd_t'.
 */
void demo_qos_shp_ld_set_mode(fpp_qos_shaper_cmd_t* p_shp, uint8_t shp_mode)
{
    assert(NULL != p_shp);
    p_shp->mode = shp_mode;
}
 
 
/*
 * @brief          Set a position of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp     Local data to be modified.
 * @param[in]      position  Position of the QoS shaper in a QoS configuration.
 *                           For valid positions, see FCI API Reference, chapter Egress QoS.
 */
void demo_qos_shp_ld_set_position(fpp_qos_shaper_cmd_t* p_shp, uint8_t position)
{
    assert(NULL != p_shp);
    p_shp->position = position;
}
 
 
/*
 * @brief          Set an idle slope rate of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp  Local data to be modified.
 * @param[in]      isl    Idle slope rate (units per second).
 *                        Units depend on the mode of a QoS shaper.
 */
void demo_qos_shp_ld_set_isl(fpp_qos_shaper_cmd_t* p_shp, uint32_t isl)
{
    assert(NULL != p_shp);
    p_shp->isl = htonl(isl);
}
 
 
/*
 * @brief          Set a minimal credit of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp       Local data to be modified.
 * @param[in]      min_credit  Minimal credit.
 */
void demo_qos_shp_ld_set_min_credit(fpp_qos_shaper_cmd_t* p_shp, int32_t min_credit)
{
    assert(NULL != p_shp);
    p_shp->min_credit = (int32_t)(htonl(min_credit));
}
 
 
/*
 * @brief          Set a maximal credit of a QoS shaper.
 * @details        [localdata_shp]
 * @param[in,out]  p_shp       Local data to be modified.
 * @param[in]      min_credit  Maximal credit.
 */
void demo_qos_shp_ld_set_max_credit(fpp_qos_shaper_cmd_t* p_shp, int32_t max_credit)
{
    assert(NULL != p_shp);
    p_shp->max_credit = (int32_t)(htonl(max_credit));
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query the name of a parent physical interface of a QoS queue.
 * @details    [localdata_que]
 * @param[in]  p_que  Local data to be queried.
 * @return     Name of a parent physical interface.
 */
const char* demo_qos_que_ld_get_if_name(const fpp_qos_queue_cmd_t* p_que)
{
    assert(p_que);
    return (p_que->if_name);
}
 
 
/*
 * @brief      Query the ID of a QoS queue.
 * @details    [localdata_que]
 * @param[in]  p_que  Local data to be queried.
 * @return     ID of a QoS queue.
 */
uint8_t demo_qos_que_ld_get_id(const fpp_qos_queue_cmd_t* p_que)
{
    assert(p_que);
    return (p_que->id);
}
 
 
/*
 * @brief      Query the mode of a QoS queue.
 * @details    [localdata_que]
 * @param[in]  p_que  Local data to be queried.
 * @return     Mode of a QoS queue.
 */
uint8_t demo_qos_que_ld_get_mode(const fpp_qos_queue_cmd_t* p_que)
{
    assert(p_que);
    return (p_que->mode);
}
 
 
/*
 * @brief      Query the minimal threshold of a QoS queue.
 * @details    [localdata_que]
 * @param[in]  p_que  Local data to be queried.
 * @return     Minimal threshold of a QoS queue.
 */
uint32_t demo_qos_que_ld_get_min(const fpp_qos_queue_cmd_t* p_que)
{
    assert(p_que);
    return ntohl(p_que->min);
}
 
 
/*
 * @brief      Query the maximal threshold of a QoS queue.
 * @details    [localdata_que]
 * @param[in]  p_que  Local data to be queried.
 * @return     Maximal threshold of a QoS queue.
 */
uint32_t demo_qos_que_ld_get_max(const fpp_qos_queue_cmd_t* p_que)
{
    assert(p_que);
    return ntohl(p_que->max);
}
 
 
/*
 * @brief      Query the percentage chance for packet drop.
 * @details    [localdata_que]
 * @param[in]  p_que     Local data to be queried.
 * @param[in]  zprob_id  ID of a probability zone.
 *                       There may be less than 32 zones actually implemented in PFE.
 *                       (32 is just the max array limit)
 *                       See FCI API Reference, chapter Egress QoS.
 * @return     Percentage drop chance of the given probability zone.
 */
uint8_t demo_qos_que_ld_get_zprob_by_id(const fpp_qos_queue_cmd_t* p_que, uint8_t zprob_id)
{
    assert(p_que);
    return ((32u > zprob_id) ? (p_que->zprob[zprob_id]) : (255u));
}
 
 
 
 
 /*
 * @brief      Query the name of a parent physical interface of a QoS scheduler.
 * @details    [localdata_sch]
 * @param[in]  p_sch  Local data to be queried.
 * @return     Name of a parent physical interface.
 */
const char* demo_qos_sch_ld_get_if_name(const fpp_qos_scheduler_cmd_t* p_sch)
{
    assert(p_sch);
    return (p_sch->if_name);
}
 
 
/*
 * @brief      Query the ID of a QoS scheduler.
 * @details    [localdata_sch]
 * @param[in]  p_sch  Local data to be queried.
 * @return     ID of a QoS scheduler.
 */
uint8_t demo_qos_sch_ld_get_id(const fpp_qos_scheduler_cmd_t* p_sch)
{
    assert(p_sch);
    return (p_sch->id);
}
 
 
/*
 * @brief      Query the mode of a QoS scheduler.
 * @details    [localdata_sch]
 * @param[in]  p_sch  Local data to be queried.
 * @return     Mode of a QoS scheduler.
 */
uint8_t demo_qos_sch_ld_get_mode(const fpp_qos_scheduler_cmd_t* p_sch)
{
    assert(p_sch);
    return (p_sch->mode);
}
 
 
/*
 * @brief      Query the selection algorithm of a QoS scheduler.
 * @details    [localdata_sch]
 * @param[in]  p_sch  Local data to be queried.
 * @return     Selection algorithm of a QoS scheduler.
 */
uint8_t demo_qos_sch_ld_get_algo(const fpp_qos_scheduler_cmd_t* p_sch)
{
    assert(p_sch);
    return (p_sch->algo);
}
 
 
/*
 * @brief      Query whether an input of a QoS scheduler is enabled or not.
 * @details    [localdata_sch]
 * @param[in]  p_sch     Local data to be queried.
 * @param[in]  input_id  Queried scheduler input.
 * @return     At time when the data was obtained from PFE, the input of the QoS scheduler:
 *             true  : was enabled
 *             false : was disabled
 */
bool demo_qos_sch_ld_is_input_enabled(const fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id)
{
    assert(NULL != p_sch);
    return (bool)((32u > input_id) ? (ntohl(p_sch->input_en) & (1uL << input_id)) : (0u));
}
 
 
/*
 * @brief      Query the weight of a QoS scheduler input.
 * @details    [localdata_sch]
 * @param[in]  p_sch     Local data to be queried.
 * @param[in]  input_id  Queried scheduler input.
 * @return     Weight of a QoS scheduler input.
 */
uint32_t demo_qos_sch_ld_get_input_weight(const fpp_qos_scheduler_cmd_t* p_sch, 
                                          uint8_t input_id)
{
    assert(NULL != p_sch);
    return ((32u > input_id) ? (ntohl(p_sch->input_w[input_id])) : (0uL));
}
 
 
/*
 * @brief      Query the traffic source of a QoS scheduler input.
 * @details    [localdata_sch]
 * @param[in]  p_sch     Local data to be queried.
 * @param[in]  input_id  Queried scheduler input.
 * @return     Traffic source of a QoS scheduler input.
 */
uint8_t demo_qos_sch_ld_get_input_src(const fpp_qos_scheduler_cmd_t* p_sch, uint8_t input_id)
{
    assert(NULL != p_sch);
    return ((32u > input_id) ? (p_sch->input_src[input_id]) : (0uL));
}
 
 
 
 
 /*
 * @brief      Query the name of a parent physical interface of a QoS shaper.
 * @details    [localdata_shp]
 * @param[in]  p_shp  Local data to be queried.
 * @return     Name of a parent physical interface.
 */
const char* demo_qos_shp_ld_get_if_name(const fpp_qos_shaper_cmd_t* p_shp)
{
    assert(p_shp);
    return (p_shp->if_name);
}
 
 
/*
 * @brief      Query the ID of a QoS shaper.
 * @details    [localdata_shp]
 * @param[in]  p_shp  Local data to be queried.
 * @return     ID of a QoS shaper.
 */
uint8_t demo_qos_shp_ld_get_id(const fpp_qos_shaper_cmd_t* p_shp)
{
    assert(p_shp);
    return (p_shp->id);
}
 
 
 /*
 * @brief      Query the position of a QoS shaper.
 * @details    [localdata_shp]
 * @param[in]  p_shp  Local data to be queried.
 * @return     Position of a QoS shaper.
 */
uint8_t demo_qos_shp_ld_get_position(const fpp_qos_shaper_cmd_t* p_shp)
{
    assert(p_shp);
    return (p_shp->position);
}
 
 
/*
 * @brief      Query the mode of a QoS shaper.
 * @details    [localdata_shp]
 * @param[in]  p_shp  Local data to be queried.
 * @return     Mode of a QoS shaper.
 */
uint8_t demo_qos_shp_ld_get_mode(const fpp_qos_shaper_cmd_t* p_shp)
{
    assert(p_shp);
    return (p_shp->mode);
}
 
 
/*
 * @brief      Query the idle slope of a QoS shaper.
 * @details    [localdata_shp]
 * @param[in]  p_shp  Local data to be queried.
 * @return     Idle slope of a QoS shaper.
 */
uint32_t demo_qos_shp_ld_get_isl(const fpp_qos_shaper_cmd_t* p_shp)
{
    assert(p_shp);
    return ntohl(p_shp->isl);
}
 
 
/*
 * @brief      Query the maximal credit of a QoS shaper.
 * @details    [localdata_shp]
 * @param[in]  p_shp  Local data to be queried.
 * @return     Maximal credit of a QoS shaper.
 */
int32_t demo_qos_shp_ld_get_max_credit(const fpp_qos_shaper_cmd_t* p_shp)
{
    assert(p_shp);
    return (int32_t)(ntohl(p_shp->max_credit));
}
 
 
/*
 * @brief      Query the minimal credit of a QoS shaper.
 * @details    [localdata_shp]
 * @param[in]  p_shp  Local data to be queried.
 * @return     Minimal credit of a QoS shaper.
 */
int32_t demo_qos_shp_ld_get_min_credit(const fpp_qos_shaper_cmd_t* p_shp)
{
    assert(p_shp);
    return (int32_t)(ntohl(p_shp->min_credit));
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available QoS queues of 
 *             a given physical interface and execute a callback print function for 
 *             each QoS queue.
 * @param[in]  p_cl          FCI client
 * @param[in]  p_cb_print    Callback print function.
 *                           --> If the callback returns ZERO, then all is OK and 
 *                               a next QoS queue is picked for a print process.
 *                           --> If the callback returns NON-ZERO, then some problem is 
 *                               assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name  Name of a parent physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through all available QoS queues of 
 *                          the given physical interface.
 *             other      : Some error occurred (represented by the respective error code).
 *                          No count was stored.
 */
int demo_qos_que_print_by_phyif(FCI_CLIENT* p_cl, demo_qos_que_cb_print_t p_cb_print, 
                                const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
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
    
    print_if_error(rtn, "demo_qos_que_print_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available QoS queues in PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_count   Space to store the count of QoS queues.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted all applicable QoS queues.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_qos_que_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                                    const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
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
    
    print_if_error(rtn, "demo_qos_que_get_count_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all available QoS schedulers of
 *             a given physical interface and execute a callback print function for 
 *             each QoS scheduler.
 * @param[in]  p_cl          FCI client
 * @param[in]  p_cb_print    Callback print function.
 *                           --> If the callback returns ZERO, then all is OK and 
 *                               a next QoS scheduler is picked for a print process.
 *                           --> If the callback returns NON-ZERO, then some problem is 
 *                               assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name  Name of a parent physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through QoS schedulers of 
 *                          the given physical interface.
 *             other      : Some error occurred (represented by the respective error code).
 *                          No count was stored.
 */
int demo_qos_sch_print_by_phyif(FCI_CLIENT* p_cl, demo_qos_sch_cb_print_t p_cb_print, 
                                const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
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
    
    print_if_error(rtn, "demo_qos_sch_print_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available QoS schedulers in PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_count   Space to store the count of QoS schedulers.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted all applicable QoS schedulers.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_qos_sch_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                                    const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
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
    
    print_if_error(rtn, "demo_qos_sch_get_count_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all available QoS shapers of 
 *             a given physical interface and execute a callback print function for 
 *             each QoS shaper.
 * @param[in]  p_cl          FCI client
 * @param[in]  p_cb_print    Callback print function.
 *                           --> If the callback returns ZERO, then all is OK and 
 *                               a next QoS shaper is picked for a print process.
 *                           --> If the callback returns NON-ZERO, then some problem is 
 *                               assumed and this function terminates prematurely.
 * @param[in]  p_phyif_name  Name of a parent physical interface.
 *                           Names of physical interfaces are hardcoded.
 *                           See FCI API Reference, chapter Interface Management.
 * @return     FPP_ERR_OK : Successfully iterated through all available QoS shapers of 
 *                          the given physical interface.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_qos_shp_print_by_phyif(FCI_CLIENT* p_cl, demo_qos_shp_cb_print_t p_cb_print, 
                                const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
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
    
    print_if_error(rtn, "demo_qos_shp_print_by_phyif() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available QoS shapers in PFE which
 *              are a part of a given parent physical interface.
 * @param[in]   p_cl          FCI client
 * @param[out]  p_rtn_count   Space to store the count of QoS shapers.
 * @param[in]   p_phyif_name  Name of a parent physical interface.
 *                            Names of physical interfaces are hardcoded.
 *                            See FCI API Reference, chapter Interface Management.
 * @return      FPP_ERR_OK : Successfully counted all applicable QoS shapers.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_qos_shp_get_count_by_phyif(FCI_CLIENT* p_cl, uint32_t* p_rtn_count, 
                                    const char* p_phyif_name)
{
    assert(NULL != p_cl);
    assert(NULL != p_phyif_name);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
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
    
    print_if_error(rtn, "demo_qos_shp_get_count_by_phyif() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
