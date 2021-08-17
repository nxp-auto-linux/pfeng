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
#include "fci_rt_ct.h"
 
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
/*
 * @brief          Network-to-host (ntoh) function for a route struct.
 * @param[in,out]  p_rtn_rt  The route struct to be converted.
 */
static void ntoh_rt(fpp_rt_cmd_t* p_rtn_rt)
{
    assert(NULL != p_rtn_rt);
    
    
    p_rtn_rt->id = ntohl(p_rtn_rt->id);
    p_rtn_rt->flags = ntohl(p_rtn_rt->flags);
}
 
 
/*
 * @brief          Host-to-network (hton) function for a route struct.
 * @param[in,out]  p_rtn_rt  The route struct to be converted.
 */
static void hton_rt(fpp_rt_cmd_t* p_rtn_rt)
{
    assert(NULL != p_rtn_rt);
    
    
    p_rtn_rt->id = htonl(p_rtn_rt->id);
    p_rtn_rt->flags = htonl(p_rtn_rt->flags);
}
 
 
/*
 * @brief          Network-to-host (ntoh) function for an IPv4 conntrack struct.
 * @param[in,out]  p_rtn_ct  The IPv4 conntrack struct to be converted.
 */
static void ntoh_ct(fpp_ct_cmd_t* p_rtn_ct)
{
    assert(NULL != p_rtn_ct);
    
    
    p_rtn_ct->saddr = ntohl(p_rtn_ct->saddr);
    p_rtn_ct->daddr = ntohl(p_rtn_ct->daddr);
    p_rtn_ct->sport = ntohs(p_rtn_ct->sport);
    p_rtn_ct->dport = ntohs(p_rtn_ct->dport);
    p_rtn_ct->saddr_reply = ntohl(p_rtn_ct->saddr_reply);
    p_rtn_ct->daddr_reply = ntohl(p_rtn_ct->daddr_reply);
    p_rtn_ct->sport_reply = ntohs(p_rtn_ct->sport_reply);
    p_rtn_ct->dport_reply = ntohs(p_rtn_ct->dport_reply);
    p_rtn_ct->protocol = ntohs(p_rtn_ct->protocol);
    p_rtn_ct->flags = ntohs(p_rtn_ct->flags);
    p_rtn_ct->route_id = ntohl(p_rtn_ct->route_id);
    p_rtn_ct->route_id_reply = ntohl(p_rtn_ct->route_id_reply);
}
 
 
/*
 * @brief          Host-to-network (hton) function for the IPv4 conntrack struct.
 * @param[in,out]  p_rtn_ct  The IPv4 conntrack struct to be converted.
 */
static void hton_ct(fpp_ct_cmd_t* p_rtn_ct)
{
    assert(NULL != p_rtn_ct);
    
    
    p_rtn_ct->saddr = htonl(p_rtn_ct->saddr);
    p_rtn_ct->daddr = htonl(p_rtn_ct->daddr);
    p_rtn_ct->sport = htons(p_rtn_ct->sport);
    p_rtn_ct->dport = htons(p_rtn_ct->dport);
    p_rtn_ct->saddr_reply = htonl(p_rtn_ct->saddr_reply);
    p_rtn_ct->daddr_reply = htonl(p_rtn_ct->daddr_reply);
    p_rtn_ct->sport_reply = htons(p_rtn_ct->sport_reply);
    p_rtn_ct->dport_reply = htons(p_rtn_ct->dport_reply);
    p_rtn_ct->protocol = htons(p_rtn_ct->protocol);
    p_rtn_ct->flags = htons(p_rtn_ct->flags);
    p_rtn_ct->route_id = htonl(p_rtn_ct->route_id);
    p_rtn_ct->route_id_reply = htonl(p_rtn_ct->route_id_reply);
}
 
 
/*
 * @brief          Network-to-host (ntoh) function for the IPv6 conntrack struct.
 * @param[in,out]  p_rtn_ct  The IPv6 conntrack struct to be converted.
 */
static void ntoh_ct6(fpp_ct6_cmd_t* p_rtn_ct6)
{
    assert(NULL != p_rtn_ct6);
    
    
    p_rtn_ct6->saddr[0] = ntohl(p_rtn_ct6->saddr[0]);
    p_rtn_ct6->saddr[1] = ntohl(p_rtn_ct6->saddr[1]);
    p_rtn_ct6->saddr[2] = ntohl(p_rtn_ct6->saddr[2]);
    p_rtn_ct6->saddr[3] = ntohl(p_rtn_ct6->saddr[3]);
    p_rtn_ct6->daddr[0] = ntohl(p_rtn_ct6->daddr[0]);
    p_rtn_ct6->daddr[1] = ntohl(p_rtn_ct6->daddr[1]);
    p_rtn_ct6->daddr[2] = ntohl(p_rtn_ct6->daddr[2]);
    p_rtn_ct6->daddr[3] = ntohl(p_rtn_ct6->daddr[3]);
    p_rtn_ct6->sport = ntohs(p_rtn_ct6->sport);
    p_rtn_ct6->dport = ntohs(p_rtn_ct6->dport);
    p_rtn_ct6->saddr_reply[0] = ntohl(p_rtn_ct6->saddr_reply[0]);
    p_rtn_ct6->saddr_reply[1] = ntohl(p_rtn_ct6->saddr_reply[1]);
    p_rtn_ct6->saddr_reply[2] = ntohl(p_rtn_ct6->saddr_reply[2]);
    p_rtn_ct6->saddr_reply[3] = ntohl(p_rtn_ct6->saddr_reply[3]);
    p_rtn_ct6->daddr_reply[0] = ntohl(p_rtn_ct6->daddr_reply[0]);
    p_rtn_ct6->daddr_reply[1] = ntohl(p_rtn_ct6->daddr_reply[1]);
    p_rtn_ct6->daddr_reply[2] = ntohl(p_rtn_ct6->daddr_reply[2]);
    p_rtn_ct6->daddr_reply[3] = ntohl(p_rtn_ct6->daddr_reply[3]);
    p_rtn_ct6->sport_reply = ntohs(p_rtn_ct6->sport_reply);
    p_rtn_ct6->dport_reply = ntohs(p_rtn_ct6->dport_reply);
    p_rtn_ct6->protocol = ntohs(p_rtn_ct6->protocol);
    p_rtn_ct6->flags = ntohs(p_rtn_ct6->flags);
    p_rtn_ct6->route_id = ntohl(p_rtn_ct6->route_id);
    p_rtn_ct6->route_id_reply = ntohl(p_rtn_ct6->route_id_reply);
}
 
 
/*
 * @brief          Host-to-network (hton) function for the IPv6 conntrack struct.
 * @param[in,out]  p_rtn_ct  The IPv6 conntrack struct to be converted.
 */
static void hton_ct6(fpp_ct6_cmd_t* p_rtn_ct6)
{
    assert(NULL != p_rtn_ct6);
    
    
    p_rtn_ct6->saddr[0] = htonl(p_rtn_ct6->saddr[0]);
    p_rtn_ct6->saddr[1] = htonl(p_rtn_ct6->saddr[1]);
    p_rtn_ct6->saddr[2] = htonl(p_rtn_ct6->saddr[2]);
    p_rtn_ct6->saddr[3] = htonl(p_rtn_ct6->saddr[3]);
    p_rtn_ct6->daddr[0] = htonl(p_rtn_ct6->daddr[0]);
    p_rtn_ct6->daddr[1] = htonl(p_rtn_ct6->daddr[1]);
    p_rtn_ct6->daddr[2] = htonl(p_rtn_ct6->daddr[2]);
    p_rtn_ct6->daddr[3] = htonl(p_rtn_ct6->daddr[3]);
    p_rtn_ct6->sport = htons(p_rtn_ct6->sport);
    p_rtn_ct6->dport = htons(p_rtn_ct6->dport);
    p_rtn_ct6->saddr_reply[0] = htonl(p_rtn_ct6->saddr_reply[0]);
    p_rtn_ct6->saddr_reply[1] = htonl(p_rtn_ct6->saddr_reply[1]);
    p_rtn_ct6->saddr_reply[2] = htonl(p_rtn_ct6->saddr_reply[2]);
    p_rtn_ct6->saddr_reply[3] = htonl(p_rtn_ct6->saddr_reply[3]);
    p_rtn_ct6->daddr_reply[0] = htonl(p_rtn_ct6->daddr_reply[0]);
    p_rtn_ct6->daddr_reply[1] = htonl(p_rtn_ct6->daddr_reply[1]);
    p_rtn_ct6->daddr_reply[2] = htonl(p_rtn_ct6->daddr_reply[2]);
    p_rtn_ct6->daddr_reply[3] = htonl(p_rtn_ct6->daddr_reply[3]);
    p_rtn_ct6->sport_reply = htons(p_rtn_ct6->sport_reply);
    p_rtn_ct6->dport_reply = htons(p_rtn_ct6->dport_reply);
    p_rtn_ct6->protocol = htons(p_rtn_ct6->protocol);
    p_rtn_ct6->flags = htons(p_rtn_ct6->flags);
    p_rtn_ct6->route_id = htonl(p_rtn_ct6->route_id);
    p_rtn_ct6->route_id_reply = htonl(p_rtn_ct6->route_id_reply);
}
 
 
/*
 * @brief          Host-to-network (hton) function for the timeout struct.
 * @param[in,out]  p_rtn_ct  The timeout struct to be converted.
 */
static void hton_timeout(fpp_timeout_cmd_t* p_rtn_timeout)
{
    assert(NULL != p_rtn_timeout);
    
    
    p_rtn_timeout->protocol = htons(p_rtn_timeout->protocol);
    p_rtn_timeout->sam_4o6_timeout = htons(p_rtn_timeout->sam_4o6_timeout);
    p_rtn_timeout->timeout_value1 = htonl(p_rtn_timeout->timeout_value1);
    p_rtn_timeout->timeout_value2 = htonl(p_rtn_timeout->timeout_value2);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from the PFE ========== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested route from the PFE.
 *              Identify the route by its ID.
 * @param[in]   p_cl      FCI client instance
 * @param[out]  p_rtn_rt  Space for data from the PFE.
 * @param[in]   id        ID of the requested route.
 *                        Route IDs are user-defined. See fci_rt_add().
 * @return      FPP_ERR_OK : Requested route was found.
 *                           A copy of its configuration was stored into p_rtn_rt.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_rt_get_by_id(FCI_CLIENT* p_cl, fpp_rt_cmd_t* p_rtn_rt, uint32_t id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_rt);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_rt_cmd_t cmd_to_fci = {0};
    fpp_rt_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                    sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_rt(&reply_from_fci);  /* set correct byte order */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) && (id != (reply_from_fci.id)))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                        sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_rt(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_rt = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested IPv4 conntrack 
 *              from the PFE. Identify the conntrack by a specific tuple of parameters.
 * @param[in]   p_cl       FCI client instance
 * @param[out]  p_rtn_ct   Space for data from the PFE.
 * @param[in]   p_ct_data  Configuration data for IPv4 conntrack identification.
 *                         To identify a conntrack, all following data must be correctly set:
 *                           --> protocol
 *                           --> saddr
 *                           --> daddr
 *                           --> sport
 *                           --> dport
 * @return      FPP_ERR_OK : Requested route was found.
 *                           A copy of its configuration was stored into p_rtn_ct.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_ct_get_by_tuple(FCI_CLIENT* p_cl, fpp_ct_cmd_t* p_rtn_ct, 
                                    const fpp_ct_cmd_t* p_ct_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_ct);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_ct_cmd_t cmd_to_fci = {0};
    fpp_ct_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                    sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_ct(&reply_from_fci);  /* set correct byte order */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) &&
           (
            ((p_ct_data->protocol) == (reply_from_fci.protocol)) && 
            ((p_ct_data->sport) == (reply_from_fci.sport)) && 
            ((p_ct_data->dport) == (reply_from_fci.dport)) &&
            ((p_ct_data->saddr) == (reply_from_fci.saddr)) && 
            ((p_ct_data->daddr) == (reply_from_fci.daddr)) 
           )
          )
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                        sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_ct(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_ct = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested IPv6 conntrack 
 *              from the PFE. Identify the conntrack by a specific tuple of parameters.
 * @param[in]   p_cl        FCI client instance
 * @param[out]  p_rtn_ct6   Space for data from the PFE.
 * @param[in]   p_ct6_data  Configuration data for IPv6 conntrack identification.
 *                          To identify a conntrack, all following data must be correctly set:
 *                            --> protocol
 *                            --> saddr
 *                            --> daddr
 *                            --> sport
 *                            --> dport
 * @return      FPP_ERR_OK : Requested route was found.
 *                           A copy of its configuration was stored into p_rtn_ct6.
 *              other      : Some error occured (represented by the respective error code).
 *                           No data copied.
 */
int fci_ct6_get_by_tuple(FCI_CLIENT* p_cl, fpp_ct6_cmd_t* p_rtn_ct6, 
                                     const fpp_ct6_cmd_t* p_ct6_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_ct6);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_ct6_cmd_t cmd_to_fci = {0};
    fpp_ct6_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                    sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_ct6(&reply_from_fci);  /* set correct byte order */
    
    /* query loop (with the search condition) */
    while ((FPP_ERR_OK == rtn) &&
           (
            ((p_ct6_data->protocol) == (reply_from_fci.protocol)) && 
            ((p_ct6_data->sport) == (reply_from_fci.sport)) && 
            ((p_ct6_data->dport) == (reply_from_fci.dport)) &&
            (0 == memcmp(p_ct6_data->saddr, reply_from_fci.saddr, (4 * sizeof(uint32_t)))) &&
            (0 == memcmp(p_ct6_data->daddr, reply_from_fci.daddr, (4 * sizeof(uint32_t))))
           )
          )
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                        sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        ntoh_ct6(&reply_from_fci);  /* set correct byte order */
    }
    
    /* if search successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_ct6 = reply_from_fci;
    }
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in the PFE ========= */
 
 
/*
 * @brief       Use FCI calls to update configuration of a target IPv4 conntrack in the PFE.
 * @details     NOTE: For conntracks, only a few selected parameters can be modified.
 *                    See FCI API Reference, chapter FPP_CMD_IPV4_CONNTRACK, 
 *              subsection "Action FPP_ACTION_UPDATE"
 * @param[in]   p_cl       FCI client instance
 * @param[in]   p_ct_data  Data struct which represents a new configuration of 
 *                         the target IPv4 conntrack.
 *                         Initial data can be obtained via fci_ct_get_by_tuple().
 * @return      FPP_ERR_OK : Configuration of the target IPv4 conntrack was
 *                           successfully updated in the PFE.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct_update(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct_data);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_ct_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct_data;
    
    /* send data */
    hton_ct(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to update configuration of a target IPv6 conntrack in the PFE.
 * @details     NOTE: For conntracks, only a few selected parameters can be modified.
 *                    See FCI API Reference, chapter FPP_CMD_IPV6_CONNTRACK, 
 *              subsection "Action FPP_ACTION_UPDATE"
 * @param[in]   p_cl        FCI client instance
 * @param[in]   p_ct6_data  Data struct which represents a new configuration of 
 *                          the target IPv6 conntrack.
 *                          Initial data can be obtained via fci_ct6_get_by_tuple().
 * @return      FPP_ERR_OK : Configuration of the target IPv6 conntrack was
 *                           successfully updated in the PFE.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct6_update(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct6_data);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_ct6_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct6_data;
    
    /* send data */
    hton_ct6(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to set timeout for IPv4 TCP conntracks in the PFE.
 * @param[in]   p_cl     FCI client instance
 * @param[in]   timeout  Timeout [seconds]
 * @param[in]   is_4o6   Set true if the timeout is intended for 
 *                       IPv4 over IPv6 tunnel connections.
 * @return      FPP_ERR_OK : New timeout was set.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct_timeout_tcp(FCI_CLIENT* p_cl, uint32_t timeout, bool is_4o6)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_timeout_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.protocol = 6u;  /* 6 == tcp */
    cmd_to_fci.timeout_value1 = timeout;
    cmd_to_fci.sam_4o6_timeout = ((is_4o6) ? (1u) : (0u));
    
    /* send data */
    hton_timeout(&cmd_to_fci);  /* set correct byte order */
    rtn = fci_write(p_cl, FPP_CMD_IPV4_SET_TIMEOUT, sizeof(fpp_timeout_cmd_t), 
                                                   (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to set timeout for IPv4 UDP conntracks in the PFE.
 * @param[in]   p_cl      FCI client instance
 * @param[in]   timeout   Timeout [seconds]
 * @param[in]   timeout2  Separate timeout for unidirectional IPv4 UDP conntracks.
 *                        If zero, then it is ignored and 'timeout' value is used for both
 *                        bidirectional and unidirectional conntracks.
 * @param[in]   is_4o6    Set true if the timeout is intended for 
 *                        IPv4 over IPv6 tunnel connections.
 * @return      FPP_ERR_OK : New timeout was set.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct_timeout_udp(FCI_CLIENT* p_cl, uint32_t timeout, uint32_t timeout2, bool is_4o6)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_timeout_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.protocol = 17u;  /* 17 == udp */
    cmd_to_fci.timeout_value1 = timeout;
    cmd_to_fci.timeout_value2 = timeout2;
    cmd_to_fci.sam_4o6_timeout = ((is_4o6) ? (1u) : (0u));
    
    /* send data */
    hton_timeout(&cmd_to_fci);  /* set correct byte order */
    rtn = fci_write(p_cl, FPP_CMD_IPV4_SET_TIMEOUT, sizeof(fpp_timeout_cmd_t), 
                                                   (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to set timeout for all other IPv4 conntracks than TCP/UDP ones.
 * @param[in]   p_cl     FCI client instance
 * @param[in]   timeout  Timeout [seconds]
 * @param[in]   is_4o6    Set true if the timeout is intended for 
 *                        IPv4 over IPv6 tunnel connections.
 * @return      FPP_ERR_OK : New timeout was set.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct_timeout_others(FCI_CLIENT* p_cl, uint32_t timeout, bool is_4o6)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_timeout_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.protocol = 0u;  /* 0 == others */
    cmd_to_fci.timeout_value1 = timeout;
    cmd_to_fci.sam_4o6_timeout = ((is_4o6) ? (1u) : (0u));
    
    /*  send data  */
    hton_timeout(&cmd_to_fci);  /* set correct byte order */
    rtn = fci_write(p_cl, FPP_CMD_IPV4_SET_TIMEOUT, sizeof(fpp_timeout_cmd_t), 
                                                   (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in the PFE ======= */
 
 
/*
 * @brief       Use FCI calls to create a new IP route in the PFE.
 * @param[in]   p_cl       FCI client instance
 * @param[in]   id         ID of the new route.
 * @param[in]   p_rt_data  Configuration data for the new IP route.
 *                         To create a new IP route, a local data struct must be created,
 *                         configured and then passed to this function.
 *                         See [localdata_rt] functions to learn more.
 * @return      FPP_ERR_OK : New IP route was created.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_rt_add(FCI_CLIENT* p_cl, uint32_t id, const fpp_rt_cmd_t* p_rt_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_rt_data);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_rt_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_rt_data;
    cmd_to_fci.id = id;
    
    /* send data */
    hton_rt(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), 
                                           (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to destroy the target IP route in the PFE.
 * @param[in]   p_cl  FCI client instance
 * @param[in]   id    ID of the route to destroy.
 * @return      FPP_ERR_OK : IP route was destroyed.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_rt_del(FCI_CLIENT* p_cl, uint32_t id)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_rt_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.id = id;
    
    /* send data */
    hton_rt(&cmd_to_fci);  /*  set correct byte order  */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), 
                                           (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to create a new IPv4 conntrack in the PFE.
 * @param[in]   p_cl       FCI client instance
 * @param[in]   p_ct_data  Configuration data for the new IPv4 conntrack.
 *                         To create a new IPv4 conntrack, a local data struct must 
 *                         be created, configured and then passed to this function.
 *                         See [localdata_ct] functions to learn more.
 * @return      FPP_ERR_OK : New IPv4 conntrack was created.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct_add(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct_data);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_ct_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct_data;
    
    /* send data */
    hton_ct(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to destroy the target IPv4 conntrack in the PFE.
 * @param[in]   p_cl  FCI client instance
 * @param[in]   p_ct_data  Configuration data for IPv4 conntrack identification.
 *                         To identify a conntrack, all following data must be correctly set:
 *                           --> protocol
 *                           --> saddr
 *                           --> daddr
 *                           --> sport
 *                           --> dport
 * @return      FPP_ERR_OK : IPv4 conntrack was destroyed.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct_del(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct_data);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_ct_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct_data;
    
    /* send data */
    hton_ct(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to create a new IPv6 conntrack in the PFE.
 * @param[in]   p_cl       FCI client instance
 * @param[in]   p_ct_data  Configuration data for the new IPv6 conntrack.
 *                         To create a new IPv6 conntrack, a local data struct must 
 *                         be created, configured and then passed to this function.
 *                         See [localdata_ct6] functions to learn more.
 * @return      FPP_ERR_OK : New IPv6 conntrack was created.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct6_add(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct6_data);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_ct6_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct6_data;
    
    /* send data */
    hton_ct6(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to destroy the target IPv6 conntrack in the PFE.
 * @param[in]   p_cl  FCI client instance
 * @param[in]   p_ct_data  Configuration data for IPv6 conntrack identification.
 *                         To identify a conntrack, all following data must be correctly set:
 *                           --> protocol
 *                           --> saddr
 *                           --> daddr
 *                           --> sport
 *                           --> dport
 * @return      FPP_ERR_OK : IPv6 conntrack was destroyed.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_ct6_del(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct6_data);
    
    
    int rtn = FPP_ERR_FCI;
    fpp_ct6_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct6_data;
    
    /* send data */
    hton_ct6(&cmd_to_fci);  /* set correct byte order */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to reset (clear) all IPv4 routes & conntracks in the PFE.
 * @param[in]   p_cl  FCI client instance
 * @return      FPP_ERR_OK : All IPv4 routes & conntracks were cleared.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_rtct_reset_ip4(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    
    /* prepare data */
    /* empty */
    
    /* send data */
    rtn = fci_write(p_cl, FPP_CMD_IPV4_RESET, 0, NULL);
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to reset (clear) all IPv6 routes & conntracks in the PFE.
 * @param[in]   p_cl  FCI client instance
 * @return      FPP_ERR_OK : All IPv6 routes & conntracks were cleared.
 *              other      : Some error occured (represented by the respective error code).
 */
int fci_rtct_reset_ip6(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    
    
    int rtn = FPP_ERR_FCI;
    
    /* prepare data */
    /* empty */
    
    /* send data */
    rtn = fci_write(p_cl, FPP_CMD_IPV6_RESET, 0, NULL);
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_rt  [localdata_rt]
 * @brief:      Functions marked as [localdata_rt] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_rt_add() to 
 *              create a new IP route with given configuration in the PFE.
 */
 
 
/*
 * @brief          Set a route as IPv4 route. If the route was previously set as IPv6, then 
 *                 the IPv6 flag is removed.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt  Local data to be modified.
 *                       For IP routes, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_rt_ld_set_as_ip4(fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    p_rt->flags &= ~(FPP_IP_ROUTE_4o6);
    p_rt->flags |= FPP_IP_ROUTE_6o4;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set a route as IPv6 route. If the route was previously set as IPv4, then 
 *                 the IPv4 flag is removed.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt  Local data to be modified.
 *                       For IP routes, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_rt_ld_set_as_ip6(fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    p_rt->flags &= ~(FPP_IP_ROUTE_6o4);
    p_rt->flags |= FPP_IP_ROUTE_4o6;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set destination MAC address of a route.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt  Local data to be modified.
 *                       For IP routes, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 *                 p_src_mac  Source MAC address.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_rt_ld_set_src_mac(fpp_rt_cmd_t* p_rt, const uint8_t p_src_mac[6])
{
    assert(NULL != p_rt);
    assert(NULL != p_src_mac);
    memcpy((p_rt->src_mac), p_src_mac, (6 * sizeof(uint8_t)));
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set destination MAC address of a route.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt  Local data to be modified.
 *                       For IP routes, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 *                 p_dst_mac  Destination MAC address.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_rt_ld_set_dst_mac(fpp_rt_cmd_t* p_rt, const uint8_t p_dst_mac[6])
{
    assert(NULL != p_rt);
    assert(NULL != p_dst_mac);
    memcpy((p_rt->dst_mac), p_dst_mac, (6 * sizeof(uint8_t)));
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set egress physical interface of a route.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt  Local data to be modified.
 *                       For IP routes, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @param[in]      p_phyif_name  Name of a physical interface which shall be used as egress.
 *                               Names of physical interfaces are hardcoded.
 *                               See the FCI API Reference, chapter Interface Management.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_rt_ld_set_egress_phyif(fpp_rt_cmd_t* p_rt, const char* p_phyif_name)
{
    assert(NULL != p_rt);
    assert(NULL != p_phyif_name);
    
    return set_text((p_rt->output_device), p_phyif_name, IFNAMSIZ);
}
 
 
/*
 * @defgroup    localdata_ct  [localdata_ct]
 * @brief:      Functions marked as [localdata_ct] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_ct_add() to 
 *              create a new IPv4 conntrack with given configuration in the PFE.
 */
 
 
/*
 * @brief          Set protocol type of an IPv4 conntrack.
 * @details        [localdata_ct]
 * @param[in,out]  p_ct  Local data to be modified.
 *                       For conntracks, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @param[in]      protocol  IP protocol ID
 *                           See "IANA Assigned Internet Protocol Number":
 *                https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_ct_ld_set_protocol(fpp_ct_cmd_t* p_ct, uint16_t protocol)
{
    assert(NULL != p_ct);
    p_ct->protocol = protocol;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set ttl decrementing of an IPv4 conntrack.
 * @details        [localdata_ct]
 * @param[in,out]  p_ct  Local data to be modified.
 *                       For conntracks, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @param[in]      enable  A request to enable/disable ttl decrementing.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_ct_ld_set_ttl_decr(fpp_ct_cmd_t* p_ct, bool enable)
{
    assert(NULL != p_ct);
    if (enable)
    {
        p_ct->flags |= CTCMD_FLAGS_TTL_DECREMENT;
    }
    else
    {
        p_ct->flags &= (uint16_t)(~CTCMD_FLAGS_TTL_DECREMENT);
    }
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set "orig direction" of an IPv4 conntrack.
 * @details        [localdata_ct]
 * @param[in,out]  p_ct  Local data to be modified.
 *                       For conntracks, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @param[in]      saddr     IPv4 source address
 * @param[in]      daddr     IPv4 destination address
 * @param[in]      sport     Source port
 * @param[in]      dport     Destination port
 * @param[in]      route_id  ID of a route for the orig direction.
 *                           The route must already exist in the PFE.
 * @param[in]      vlan      VLAN tag
 *                           0     : no VLAN tagging 
 *                           non 0 : --> if packet not tagged, then VLAN tag is added.
 *                                   --> if packet already tagged, then VLAN tag is replaced.
 * @param[in]      unidir_orig_only  Request to make the conntrack unidirectional
 *                                   (orig direction only).
 *                                   If conntrack was previously configured to be
 *                                   "reply direction only", it gets reconfigured to be
 *                                   orig direction only.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_ct_ld_set_orig_dir(fpp_ct_cmd_t* p_ct, uint32_t saddr, uint32_t daddr,
                           uint16_t sport, uint16_t dport,
                           uint32_t route_id, uint16_t vlan, 
                           bool unidir_orig_only)
{
    assert(NULL != p_ct);
    
    
    p_ct->saddr = saddr;
    p_ct->daddr = daddr;
    p_ct->sport = sport;
    p_ct->dport = dport;
    p_ct->route_id = route_id;
    p_ct->vlan = vlan;
    if (unidir_orig_only)
    {
        p_ct->route_id_reply = 0uL;
        p_ct->flags |= CTCMD_FLAGS_REP_DISABLED;
        p_ct->flags &= ~(CTCMD_FLAGS_ORIG_DISABLED);
    }
    
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set "reply direction" of an IPv4 conntrack.
 * @details        [localdata_ct]
 * @param[in,out]  p_ct  Local data to be modified.
 *                       For conntracks, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @param[in]      saddr_reply     IPv4 source address (reply direction)
 * @param[in]      daddr_reply     IPv4 destination address (reply direction)
 * @param[in]      sport_reply     Source port (reply direction)
 * @param[in]      dport_reply     Destination port (reply direction)
 * @param[in]      route_id_reply  ID of a route for the orig direction.
 * @param[in]      vlan_reply      VLAN tag (reply direction)
 *                                 0     : no VLAN tagging 
 *                                 non 0 : --> if packet not tagged, then VLAN tag is added.
 *                                         --> if packet already tagged, then 
 *                                             VLAN tag is replaced.
 * @param[in]      unidir_reply_only  Request to make the conntrack unidirectional 
 *                                    (reply direction only).
 *                                    If conntrack was previously configured to be
 *                                    "orig direction only", it gets reconfigured to be
 *                                    reply direction only.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_ct_ld_set_reply_dir(fpp_ct_cmd_t* p_ct, uint32_t saddr_reply, uint32_t daddr_reply,
                            uint16_t sport_reply, uint16_t dport_reply,
                            uint32_t route_id_reply, uint16_t vlan_reply,
                            bool unidir_reply_only)
{
    assert(NULL != p_ct);
    
    
    p_ct->saddr_reply = saddr_reply;
    p_ct->daddr_reply = daddr_reply;
    p_ct->sport_reply = sport_reply;
    p_ct->dport_reply = dport_reply;
    p_ct->route_id_reply = route_id_reply;
    p_ct->vlan_reply = vlan_reply;
    if (unidir_reply_only)
    {
        p_ct->route_id = 0uL;
        p_ct->flags |= CTCMD_FLAGS_ORIG_DISABLED;
        p_ct->flags &= ~(CTCMD_FLAGS_REP_DISABLED);
    }
    
    return (FPP_ERR_OK);
}
 
 
/*
 * @defgroup    localdata_ct6  [localdata_ct6]
 * @brief:      Functions marked as [localdata_ct] guarantee that 
 *              only local data are accessed.
 * @details:    These functions do not make any FCI calls.
 *              If some local data modifications are made, then after all local data changes
 *              are done and finished, call fci_ct6_add() to 
 *              create a new IPv4 conntrack with given configuration in the PFE.
 */
 
 
/*
 * @brief          Set protocol type of an IPv6 conntrack.
 * @details        [localdata_ct6]
 * @param[in,out]  p_ct  Local data to be modified.
 *                       For conntracks, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @param[in]      protocol  IP protocol ID
 *                           See "IANA Assigned Internet Protocol Number":
 *                https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_ct6_ld_set_protocol(fpp_ct6_cmd_t* p_ct6, uint16_t protocol)
{
    assert(NULL != p_ct6);
    p_ct6->protocol = protocol;
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set ttl decrementing of an IPv6 conntrack.
 * @details        [localdata_ct6]
 * @param[in,out]  p_ct6  Local data to be modified.
 *                        For conntracks, there are no "initial data" to be obtained from PFE.
 *                        Simply declare a local data struct and configure it.
 * @param[in]      enable  A request to enable/disable ttl decrementing.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_ct6_ld_set_ttl_decr(fpp_ct6_cmd_t* p_ct6, bool enable)
{
    assert(NULL != p_ct6);
    if (enable)
    {
        p_ct6->flags |= CTCMD_FLAGS_TTL_DECREMENT;
    }
    else
    {
        p_ct6->flags &= (uint16_t)(~CTCMD_FLAGS_TTL_DECREMENT);
    }
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set "orig direction" of an IPv6 conntrack.
 * @details        [localdata_ct6]
 * @param[in,out]  p_ct  Local data to be modified.
 *                       For conntracks, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @param[in]      saddr     IPv6 source address
 * @param[in]      daddr     IPv6 destination address
 * @param[in]      sport     Source port
 * @param[in]      dport     Destination port
 * @param[in]      route_id  ID of a route for the orig direction.
 *                           The route must already exist in the PFE.
 * @param[in]      vlan      VLAN tag
 *                           0     : no VLAN tagging 
 *                           non 0 : --> if packet not tagged, then VLAN tag is added.
 *                                   --> if packet already tagged, then VLAN tag is replaced.
 * @param[in]      unidir_orig_only  Request to make the conntrack unidirectional
 *                                   (orig direction only).
 *                                   If conntrack was previously configured to be
 *                                   "reply direction only", it gets reconfigured to be
 *                                   orig direction only.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_ct6_ld_set_orig_dir(fpp_ct6_cmd_t* p_ct6, const uint32_t p_saddr[4], 
                            const uint32_t p_daddr[4],
                            uint16_t sport, uint16_t dport,
                            uint32_t route_id, uint16_t vlan, 
                            bool unidir_orig_only)
{
    assert(NULL != p_ct6);
    assert(NULL != p_saddr);
    assert(NULL != p_daddr);
    
    
    memcpy((p_ct6->saddr), p_saddr, (4 * sizeof(uint32_t)));
    memcpy((p_ct6->daddr), p_daddr, (4 * sizeof(uint32_t)));
    p_ct6->sport = sport;
    p_ct6->dport = dport;
    p_ct6->route_id = route_id;
    p_ct6->vlan = vlan;
    if (unidir_orig_only)
    {
        p_ct6->route_id_reply = 0uL;
        p_ct6->flags |= CTCMD_FLAGS_REP_DISABLED;
        p_ct6->flags &= ~(CTCMD_FLAGS_ORIG_DISABLED);
    }
    
    return (FPP_ERR_OK);
}
 
 
/*
 * @brief          Set "reply direction" of an IPv6 conntrack.
 * @details        [localdata_ct6]
 * @param[in,out]  p_ct  Local data to be modified.
 *                       For conntracks, there are no "initial data" to be obtained from PFE.
 *                       Simply declare a local data struct and configure it.
 * @param[in]      saddr_reply     IPv6 source address (reply direction)
 * @param[in]      daddr_reply     IPv6 destination address (reply direction)
 * @param[in]      sport_reply     Source port (reply direction)
 * @param[in]      dport_reply     Destination port (reply direction)
 * @param[in]      route_id_reply  ID of a route for the orig direction.
 * @param[in]      vlan_reply      VLAN tag (reply direction)
 *                                 0     : no VLAN tagging 
 *                                 non 0 : --> if packet not tagged, then VLAN tag is added.
 *                                         --> if packet already tagged, then 
 *                                             VLAN tag is replaced.
 * @param[in]      unidir_reply_only  Request to make the conntrack unidirectional 
 *                                    (reply direction only).
 *                                    If conntrack was previously configured to be
 *                                    "orig direction only", it gets reconfigured to be
 *                                    reply direction only.
 * @return         FPP_ERR_OK : Local data were successfully modified.
 *                 other      : Some error occured (represented by the respective error code).
 *                              Local data not modified.
 */
int fci_ct6_ld_set_reply_dir(fpp_ct6_cmd_t* p_ct6, const uint32_t p_saddr_reply[4], 
                             const uint32_t p_daddr_reply[4],
                             uint16_t sport_reply, uint16_t dport_reply,
                             uint32_t route_id_reply, uint16_t vlan_reply,
                             bool unidir_reply_only)
{
    assert(NULL != p_ct6);
    assert(NULL != p_saddr_reply);
    assert(NULL != p_daddr_reply);
    
    
    memcpy((p_ct6->saddr_reply), p_saddr_reply, (4 * sizeof(uint32_t)));
    memcpy((p_ct6->daddr_reply), p_daddr_reply, (4 * sizeof(uint32_t)));
    p_ct6->sport_reply = sport_reply;
    p_ct6->dport_reply = dport_reply;
    p_ct6->route_id_reply = route_id_reply;
    p_ct6->vlan_reply = vlan_reply;
    if (unidir_reply_only)
    {
        p_ct6->route_id = 0uL;
        p_ct6->flags |= CTCMD_FLAGS_ORIG_DISABLED;
        p_ct6->flags &= ~(CTCMD_FLAGS_REP_DISABLED);
    }
    
    return (FPP_ERR_OK);
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query whether the route is IPv4 route.
 * @details    [localdata_rt]
 * @param[in]  p_rt  Local data to be queried.
 * @return     The route:
 *             true  : is an IPv4 route.
 *             false : is NOT an IPv4 route.
 */
bool fci_rt_ld_is_ip4(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    return (bool)(FPP_IP_ROUTE_6o4 & (p_rt->flags));
}
 
 
/*
 * @brief      Query whether the route is IPv6 route.
 * @details    [localdata_rt]
 * @param[in]  p_rt  Local data to be queried.
 * @return     The route:
 *             true  : is an IPv6 route.
 *             false : is NOT an IPv6 route.
 */
bool fci_rt_ld_is_ip6(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    return (bool)(FPP_IP_ROUTE_4o6 & (p_rt->flags));
}
 
 
/*
 * @brief      Query whether the IPv4 conntrack serves as a NAT.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : does serve as a NAT.
 *             false : does NOT serve as a NAT.
 */
bool fci_ct_ld_is_nat(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)(((p_ct->daddr_reply) != (p_ct->saddr)) || ((p_ct->saddr_reply) != (p_ct->daddr)));
}
 
 
/*
 * @brief      Query whether the IPv4 conntrack serves as a PAT.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : does serve as a PAT.
 *             false : does NOT serve as a PAT.
 */
bool fci_ct_ld_is_pat(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)(((p_ct->dport_reply) != (p_ct->sport)) || ((p_ct->sport_reply) != (p_ct->dport)));
}
 
 
/*
 * @brief      Query whether the IPv4 conntrack modifies VLAN tags.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : does modify VLAN tags of served packets.
 *             false : does NOT modify VLAN tags of served packets.
 */
bool fci_ct_ld_is_vlan_tagging(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)((0u != p_ct->vlan) || (0u != p_ct->vlan_reply));
}
 
 
/*
 * @brief      Query whether the IPv4 conntrack decrements packet's TTL counter or not.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPV4 conntrack:
 *             true  : does decrement TTL counter.
 *             false : does NOT decrement TTL counter.
 */
bool fci_ct_ld_is_ttl_decr(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)(CTCMD_FLAGS_TTL_DECREMENT & (p_ct->flags));
}
 
 
/*
 * @brief      Query whether the IPv4 conntrack is orig direction only.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : is orig direction only.
 *             false : is NOT orig direction only.
 */
bool fci_ct_ld_is_orig_only(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)(CTCMD_FLAGS_REP_DISABLED & (p_ct->flags));
}
 
 
/*
 * @brief      Query whether the IPv4 conntrack is reply direction only.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : is reply direction only.
 *             false : is NOT reply direction only.
 */
bool fci_ct_ld_is_reply_only(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)(CTCMD_FLAGS_ORIG_DISABLED & (p_ct->flags));
}
 
 
/*
 * @brief      Query whether the IPv6 conntrack serves as a NAT.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : does serve as a NAT.
 *             false : does NOT serve as a NAT.
 */
bool fci_ct6_ld_is_nat(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)(memcmp((p_ct6->daddr_reply), (p_ct6->saddr), (4 * sizeof(uint32_t))) ||
                  memcmp((p_ct6->saddr_reply), (p_ct6->daddr), (4 * sizeof(uint32_t))));
}
 
 
/*
 * @brief      Query whether the IPv6 conntrack serves as a PAT.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : does serve as a PAT.
 *             false : does NOT serve as a PAT.
 */
bool fci_ct6_ld_is_pat(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)(((p_ct6->dport_reply) != (p_ct6->sport)) || ((p_ct6->sport_reply) != (p_ct6->dport)));
}
 
 
/*
 * @brief      Query whether the IPv6 conntrack modifies VLAN tags.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : does modify VLAN tags of served packets.
 *             false : does NOT modify VLAN tags of served packets.
 */
bool fci_ct6_ld_is_vlan_tagging(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)((0u != p_ct6->vlan) || (0u != p_ct6->vlan_reply));
}
 
 
/*
 * @brief      Query whether the IPv6 conntrack decrements packet's TTL counter or not.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPV6 conntrack:
 *             true  : does decrement TTL counter.
 *             false : does NOT decrement TTL counter.
 */
bool fci_ct6_ld_is_ttl_decr(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)(CTCMD_FLAGS_TTL_DECREMENT & (p_ct6->flags));
}
 
 
/*
 * @brief      Query whether the IPv6 conntrack is orig direction only.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : is orig direction only.
 *             false : is NOT orig direction only.
 */
bool fci_ct6_ld_is_orig_only(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)(CTCMD_FLAGS_REP_DISABLED & (p_ct6->flags));
}
 
 
/*
 * @brief      Query whether the IPv6 conntrack is reply direction only.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : is reply direction only.
 *             false : is NOT reply direction only.
 */
bool fci_ct6_ld_is_reply_only(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)(CTCMD_FLAGS_ORIG_DISABLED & (p_ct6->flags));
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all IP routes in the PFE and
 *             execute a callback print function for each reported IP route.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next IP route is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @param[in]  print_ip4   Set true to print IPv4 routes.
 * @param[in]  print_ip6   Set true to print IPv6 routes.
 * @return     FPP_ERR_OK : Successfully iterated through all IP routes.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_rt_print_all(FCI_CLIENT* p_cl, fci_rt_cb_print_t p_cb_print, bool print_ip4, bool print_ip6)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_rt_cmd_t cmd_to_fci = {0};
    fpp_rt_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                    sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_rt(&reply_from_fci);  /* set correct byte order */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        if ((print_ip4) && (FPP_IP_ROUTE_6o4 & (reply_from_fci.flags)))
        {
            rtn = p_cb_print(&reply_from_fci);  /* print IPv4 route info */
        }
        if ((print_ip6) && (FPP_IP_ROUTE_4o6 & (reply_from_fci.flags)))
        {
            rtn = p_cb_print(&reply_from_fci);  /* print IPv6 route info */
        }
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                            sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_rt(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more IP routes to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_RT_ENTRY_NOT_FOUND == rtn) 
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all IP routes in the PFE.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of IP routes.
 * @return      FPP_ERR_OK : Successfully counted all IP routes.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_rt_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_rt_cmd_t cmd_to_fci = {0};
    fpp_rt_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                    sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting routes) */
    
    /*  query loop  */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                        sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting routes) */
    }
    
    /* query loop runs till there are no more IP routes to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_RT_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all IPv4 conntracks in the PFE and
 *             execute a callback print function for each reported IPv4 conntrack.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next IPv4 conntrack is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all IPv4 conntracks.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_ct_print_all(FCI_CLIENT* p_cl, fci_ct_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_ct_cmd_t cmd_to_fci = {0};
    fpp_ct_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                    sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_ct(&reply_from_fci);  /* set correct byte order */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                            sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_ct(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more IPv4 conntracks to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_CT_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all IPv4 conntracks in the PFE.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of IPv4 conntracks.
 * @return      FPP_ERR_OK : Successfully counted all IPv4 conntracks.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_ct_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_ct_cmd_t cmd_to_fci = {0};
    fpp_ct_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                    sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting conntracks) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                        sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting conntracks) */
    }
    
    /* query loop runs till there are no more IPv4 conntracks to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_CT_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Use FCI calls to iterate through all IPv6 conntracks in the PFE and
 *             execute a callback print function for each reported IPv6 conntrack.
 * @param[in]  p_cl        FCI client instance
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns zero, then all is OK and 
 *                             the next IPv6 conntrack is picked for a print process.
 *                         --> If the callback returns non-zero, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all IPv6 conntracks.
 *             other      : Some error occured (represented by the respective error code).
 */
int fci_ct6_print_all(FCI_CLIENT* p_cl, fci_ct6_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_ct6_cmd_t cmd_to_fci = {0};
    fpp_ct6_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                    sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    ntoh_ct6(&reply_from_fci);  /* set correct byte order */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        rtn = p_cb_print(&reply_from_fci);
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                            sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
            ntoh_ct6(&reply_from_fci);  /* set correct byte order */
        }
    }
    
    /* query loop runs till there are no more IPv6 conntracks to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_CT_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all IPv6 conntracks in the PFE.
 * @param[in]   p_cl         FCI client instance
 * @param[out]  p_rtn_count  Space to store the count of IPv6 conntracks.
 * @return      FPP_ERR_OK : Successfully counted all IPv6 conntracks.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occured (represented by the respective error code).
 *                           No count was stored.
 */
int fci_ct6_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    
    int rtn = FPP_ERR_FCI;
    
    fpp_ct6_cmd_t cmd_to_fci = {0};
    fpp_ct6_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                    sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    /* no need to set correct byte order (we are just counting conntracks) */
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                        sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
        /* no need to set correct byte order (we are just counting conntracks) */
    }
    
    /* query loop runs till there are no more IPv6 conntracks to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_CT_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
