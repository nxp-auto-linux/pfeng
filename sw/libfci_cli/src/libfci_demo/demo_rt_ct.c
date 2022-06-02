/* =========================================================================
 *  Copyright 2020-2022 NXP
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
#include "demo_rt_ct.h"
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to get data from PFE ============== */
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested route from PFE.
 *              Identify the route by its ID.
 * @param[in]   p_cl      FCI client
 * @param[out]  p_rtn_rt  Space for data from PFE.
 * @param[in]   id        ID of the requested route.
 *                        Route IDs are user-defined. See demo_rt_add().
 * @return      FPP_ERR_OK : The requested route was found.
 *                           A copy of its configuration data was stored into p_rtn_rt.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_rt_get_by_id(FCI_CLIENT* p_cl, fpp_rt_cmd_t* p_rtn_rt, uint32_t id)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_rt);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_rt_cmd_t cmd_to_fci = {0};
    fpp_rt_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                    sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) && (ntohl(reply_from_fci.id) != id))
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                        sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_rt = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_rt_get_by_id() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested IPv4 conntrack 
 *              from PFE. Identify the conntrack by a specific tuple of parameters.
 * @param[in]   p_cl       FCI client
 * @param[out]  p_rtn_ct   Space for data from PFE.
 * @param[in]   p_ct_data  Configuration data for IPv4 conntrack identification.
 *                         To identify a conntrack, all the following data must be 
 *                         correctly set:
 *                           --> protocol
 *                           --> saddr
 *                           --> daddr
 *                           --> sport
 *                           --> dport
 *                         REMINDER: It is assumed that data are in a network byte order.
 * @return      FPP_ERR_OK : The requested IPv4 conntrack was found.
 *                           A copy of its configuration was stored into p_rtn_ct.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_ct_get_by_tuple(FCI_CLIENT* p_cl, fpp_ct_cmd_t* p_rtn_ct, 
                                     const fpp_ct_cmd_t* p_ct_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_ct);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_ct_cmd_t cmd_to_fci = {0};
    fpp_ct_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                    sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) &&
           !(
             /* both sides are in network byte order (thus no byte order conversion needed) */
             ((reply_from_fci.protocol) == (p_ct_data->protocol)) && 
             ((reply_from_fci.sport) == (p_ct_data->sport)) && 
             ((reply_from_fci.dport) == (p_ct_data->dport)) &&
             ((reply_from_fci.saddr) == (p_ct_data->saddr)) && 
             ((reply_from_fci.daddr) == (p_ct_data->daddr))
            )
          )
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                        sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_ct = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_ct_get_by_tuple() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get configuration data of a requested IPv6 conntrack 
 *              from PFE. Identify the conntrack by a specific tuple of parameters.
 * @param[in]   p_cl        FCI client
 * @param[out]  p_rtn_ct6   Space for data from PFE.
 * @param[in]   p_ct6_data  Configuration data for IPv6 conntrack identification.
 *                          To identify a conntrack, all the following data must be 
 *                          correctly set:
 *                            --> protocol
 *                            --> saddr
 *                            --> daddr
 *                            --> sport
 *                            --> dport
 *                          REMINDER: It is assumed that data are in a network byte order.
 * @return      FPP_ERR_OK : The requested IPv6 conntrack was found.
 *                           A copy of its configuration was stored into p_rtn_ct6.
 *                           REMINDER: Data from PFE are in a network byte order.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No data copied.
 */
int demo_ct6_get_by_tuple(FCI_CLIENT* p_cl, fpp_ct6_cmd_t* p_rtn_ct6, 
                                      const fpp_ct6_cmd_t* p_ct6_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_ct6);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_ct6_cmd_t cmd_to_fci = {0};
    fpp_ct6_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                    sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop (with a search condition) */
    while ((FPP_ERR_OK == rtn) &&
           !(
             /* both sides are in network byte order (thus no byte order conversion needed) */
             ((reply_from_fci.protocol) == (p_ct6_data->protocol)) && 
             ((reply_from_fci.sport) == (p_ct6_data->sport)) && 
             ((reply_from_fci.dport) == (p_ct6_data->dport)) &&
             (0 == memcmp(reply_from_fci.saddr, p_ct6_data->saddr, (4 * sizeof(uint32_t)))) &&
             (0 == memcmp(reply_from_fci.daddr, p_ct6_data->daddr, (4 * sizeof(uint32_t))))
            )
          )
    {
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                        sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* if a query is successful, then assign the data */
    if (FPP_ERR_OK == rtn)
    {
        *p_rtn_ct6 = reply_from_fci;
    }
    
    print_if_error(rtn, "demo_ct6_get_by_tuple() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to update data in PFE ============= */
 
 
/*
 * @brief       Use FCI calls to update configuration of a target IPv4 conntrack in PFE.
 * @details     For conntracks, only a few selected parameters can be modified.
 *              See FCI API Reference, chapter FPP_CMD_IPV4_CONNTRACK, 
 *              subsection "Action FPP_ACTION_UPDATE".
 * @param[in]   p_cl       FCI client
 * @param[in]   p_ct_data  Local data struct which represents a new configuration of 
 *                         the target IPv4 conntrack.
 *                         Initial data can be obtained via demo_ct_get_by_tuple().
 * @return      FPP_ERR_OK : Configuration of the target IPv4 conntrack was
 *                           successfully updated in PFE.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct_update(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct_data);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_ct_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct_data;
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct_update() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to update configuration of a target IPv6 conntrack in PFE.
 * @details     For conntracks, only a few selected parameters can be modified.
 *              See FCI API Reference, chapter FPP_CMD_IPV6_CONNTRACK, 
 *              subsection "Action FPP_ACTION_UPDATE".
 * @param[in]   p_cl        FCI client
 * @param[in]   p_ct6_data  Local data struct which represents a new configuration of 
 *                          the target IPv6 conntrack.
 *                          Initial data can be obtained via demo_ct6_get_by_tuple().
 * @return      FPP_ERR_OK : Configuration of the target IPv6 conntrack was
 *                           successfully updated in PFE.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct6_update(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct6_data);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_ct6_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct6_data;
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_UPDATE;
    rtn = fci_write(p_cl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct6_update() failed!");
    
    return (rtn);
}
 
 
 
 
/*
 * @brief       Use FCI calls to set timeout for IPv4 TCP conntracks in PFE.
 * @param[in]   p_cl     FCI client
 * @param[in]   timeout  Timeout [seconds]
 * @return      FPP_ERR_OK : New timeout was set.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct_timeout_tcp(FCI_CLIENT* p_cl, uint32_t timeout)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_timeout_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.protocol = htons(6u);  /* 6 == tcp */
    cmd_to_fci.timeout_value1 = htonl(timeout);
    
    /* send data */
    rtn = fci_write(p_cl, FPP_CMD_IPV4_SET_TIMEOUT, sizeof(fpp_timeout_cmd_t), 
                                                   (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct_timeout_tcp() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to set timeout for IPv4 UDP conntracks in PFE.
 * @param[in]   p_cl      FCI client
 * @param[in]   timeout   Timeout [seconds]
 * @return      FPP_ERR_OK : New timeout was set.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct_timeout_udp(FCI_CLIENT* p_cl, uint32_t timeout)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_timeout_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.protocol = htons(17u);  /* 17 == udp */
    cmd_to_fci.timeout_value1 = htonl(timeout);
    
    /* send data */
    rtn = fci_write(p_cl, FPP_CMD_IPV4_SET_TIMEOUT, sizeof(fpp_timeout_cmd_t), 
                                                   (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct_timeout_udp() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to set timeout for all other IPv4 conntracks than TCP/UDP ones.
 * @param[in]   p_cl     FCI client
 * @param[in]   timeout  Timeout [seconds]
 * @return      FPP_ERR_OK : New timeout was set.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct_timeout_others(FCI_CLIENT* p_cl, uint32_t timeout)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_timeout_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.protocol = htons(0u);  /* 0 == others */
    cmd_to_fci.timeout_value1 = htonl(timeout);
    
    /* send data */
    rtn = fci_write(p_cl, FPP_CMD_IPV4_SET_TIMEOUT, sizeof(fpp_timeout_cmd_t), 
                                                   (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct_timeout_others() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : use FCI calls to add/del items in PFE =========== */
 
 
/*
 * @brief       Use FCI calls to create a new route in PFE.
 * @details     In the context of PFE, a "route" is a configuration data element that 
 *              specifies which physical interface of PFE shall be used as an egress interface
 *              and what destination MAC address shall be set in the routed traffic.
 *              These "routes" are used as a part of IPv4/IPv6 conntracks.
 * @param[in]   p_cl       FCI client
 * @param[in]   id         ID of the new route.
 * @param[in]   p_rt_data  Configuration data of the new route.
 *                         To create a new route, a local data struct must be created,
 *                         configured and then passed to this function.
 *                         See [localdata_rt] to learn more.
 * @return      FPP_ERR_OK : New route was created.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_rt_add(FCI_CLIENT* p_cl, uint32_t id, const fpp_rt_cmd_t* p_rt_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_rt_data);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_rt_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_rt_data;
    cmd_to_fci.id = htonl(id);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), 
                                           (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_rt_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to destroy the target route in PFE.
 * @param[in]   p_cl  FCI client
 * @param[in]   id    ID of the route to destroy.
 * @return      FPP_ERR_OK : The route was destroyed.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_rt_del(FCI_CLIENT* p_cl, uint32_t id)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_rt_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci.id = htonl(id);
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IP_ROUTE, sizeof(fpp_rt_cmd_t), 
                                           (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_rt_del() failed!");
    
    return (rtn);
}
 
 
 
 
/*
 * @brief       Use FCI calls to create a new IPv4 conntrack in PFE.
 * @param[in]   p_cl       FCI client
 * @param[in]   p_ct_data  Configuration data of the new IPv4 conntrack.
 *                         To create a new IPv4 conntrack, a local data struct must 
 *                         be created, configured and then passed to this function.
 *                         See [localdata_ct] to learn more.
 * @return      FPP_ERR_OK : New IPv4 conntrack was created.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct_add(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct_data);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_ct_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct_data;
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to destroy the target IPv4 conntrack in PFE.
 * @param[in]   p_cl       FCI client
 * @param[in]   p_ct_data  Configuration data for IPv4 conntrack identification.
 *                         To identify a conntrack, all the following data must be 
 *                         correctly set:
 *                           --> protocol
 *                           --> saddr
 *                           --> daddr
 *                           --> sport
 *                           --> dport
 *                         REMINDER: It is assumed that data are in a network byte order.
 * @return      FPP_ERR_OK : The IPv4 conntrack was destroyed.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct_del(FCI_CLIENT* p_cl, const fpp_ct_cmd_t* p_ct_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct_data);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_ct_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct_data;
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IPV4_CONNTRACK, sizeof(fpp_ct_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct_del() failed!");
    
    return (rtn);
}
 
 
 
 
/*
 * @brief       Use FCI calls to create a new IPv6 conntrack in PFE.
 * @param[in]   p_cl        FCI client
 * @param[in]   p_ct6_data  Configuration data of the new IPv6 conntrack.
 *                          To create a new IPv6 conntrack, a local data struct must 
 *                          be created, configured and then passed to this function.
 *                          See [localdata_ct6] to learn more.
 * @return      FPP_ERR_OK : New IPv6 conntrack was created.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct6_add(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct6_data);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_ct6_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct6_data;
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_REGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct6_add() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to destroy the target IPv6 conntrack in PFE.
 * @param[in]   p_cl        FCI client
 * @param[in]   p_ct6_data  Configuration data for IPv6 conntrack identification.
 *                          To identify a conntrack, all the following data must be 
 *                          correctly set:
 *                            --> protocol
 *                            --> saddr
 *                            --> daddr
 *                            --> sport
 *                            --> dport
 *                          REMINDER: It is assumed that data are in a network byte order.
 * @return      FPP_ERR_OK : The IPv6 conntrack was destroyed.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_ct6_del(FCI_CLIENT* p_cl, const fpp_ct6_cmd_t* p_ct6_data)
{
    assert(NULL != p_cl);
    assert(NULL != p_ct6_data);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    fpp_ct6_cmd_t cmd_to_fci = {0};
    
    /* prepare data */
    cmd_to_fci = *p_ct6_data;
    
    /* send data */
    cmd_to_fci.action = FPP_ACTION_DEREGISTER;
    rtn = fci_write(p_cl, FPP_CMD_IPV6_CONNTRACK, sizeof(fpp_ct6_cmd_t), 
                                                 (unsigned short*)(&cmd_to_fci));
    
    print_if_error(rtn, "demo_ct6_del() failed!");
    
    return (rtn);
}
 
 
 
 
/*
 * @brief       Use FCI calls to reset (clear) all IPv4 routes & conntracks in PFE.
 * @param[in]   p_cl  FCI client
 * @return      FPP_ERR_OK : All IPv4 routes & conntracks were cleared.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_rtct_reset_ip4(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    /* prepare data */
    /* empty */
    
    /* send data */
    rtn = fci_write(p_cl, FPP_CMD_IPV4_RESET, 0, NULL);
    
    print_if_error(rtn, "demo_rtct_reset_ip4() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to reset (clear) all IPv6 routes & conntracks in PFE.
 * @param[in]   p_cl  FCI clientf
 * @return      FPP_ERR_OK : All IPv6 routes & conntracks were cleared.
 *              other      : Some error occurred (represented by the respective error code).
 */
int demo_rtct_reset_ip6(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    /* prepare data */
    /* empty */
    
    /* send data */
    rtn = fci_write(p_cl, FPP_CMD_IPV6_RESET, 0, NULL);
    
    print_if_error(rtn, "demo_rtct_reset_ip6() failed!");
    
    return (rtn);
}
 
 
/* ==== PUBLIC FUNCTIONS : modify local data (no FCI calls) ================ */
/*
 * @defgroup    localdata_rt  [localdata_rt]
 * @brief:      Functions marked as [localdata_rt] access only local data. 
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_rt (a struct with configuration data).
 *              When adding a new route, there are no "initial data" to be obtained from PFE.
 *              Simply declare a local data struct and configure it.
 *              Then, after all modifications are done and finished,
 *              call demo_rt_add() to create a new route in PFE.
 *              REMINDER: In the context of PFE, a "route" is a configuration data element
 *                        which is used as a part of IPv4/IPv6 conntracks.
 */
 
 
/*
 * @brief          Set a route as an IPv4 route. If the route was previously set as 
 *                 an IPv6 route, then the IPv6 flag is removed.
 * @details        [localdata_rt]
 *                 Symbol names are a bit confusing (inherited from another project).
 *                 FPP_IP_ROUTE_6o4 == route is an IPv4 route
 *                 FPP_IP_ROUTE_4o6 == route is an IPv6 route
 *                 It is forbidden to set both flags at the same time (undefined behavior).
 * @param[in,out]  p_rt  Local data to be modified.
 */
void demo_rt_ld_set_as_ip4(fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    p_rt->flags &= htonl(~FPP_IP_ROUTE_4o6);
    p_rt->flags |= htonl(FPP_IP_ROUTE_6o4);
}
 
 
/*
 * @brief          Set a route as an IPv6 route. If the route was previously set as
 *                 an IPv4 route, then the IPv4 flag is removed.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt  Local data to be modified.
 *                 Symbol names are a bit confusing (inherited from another project).
 *                 FPP_IP_ROUTE_6o4 == route is an IPv4 route
 *                 FPP_IP_ROUTE_4o6 == route is an IPv6 route
 *                 It is forbidden to set both flags at the same time (undefined behavior).
 * @param[in,out]  p_rt  Local data to be modified.
 */
void demo_rt_ld_set_as_ip6(fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    p_rt->flags &= htonl(~FPP_IP_ROUTE_6o4);
    p_rt->flags |= htonl(FPP_IP_ROUTE_4o6);
}
 
 
/*
 * @brief          Set a source MAC address of a route.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt  Local data to be modified.
 *                 p_src_mac  Source MAC address.
 */
void demo_rt_ld_set_src_mac(fpp_rt_cmd_t* p_rt, const uint8_t p_src_mac[6])
{
    assert(NULL != p_rt);
    assert(NULL != p_src_mac);
    memcpy((p_rt->src_mac), p_src_mac, (6 * sizeof(uint8_t)));
}
 
 
/*
 * @brief          Set a destination MAC address of a route.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt       Local data to be modified.
 *                 p_dst_mac  Destination MAC address.
 */
void demo_rt_ld_set_dst_mac(fpp_rt_cmd_t* p_rt, const uint8_t p_dst_mac[6])
{
    assert(NULL != p_rt);
    assert(NULL != p_dst_mac);
    memcpy((p_rt->dst_mac), p_dst_mac, (6 * sizeof(uint8_t)));
}
 
 
/*
 * @brief          Set an egress physical interface of a route.
 * @details        [localdata_rt]
 * @param[in,out]  p_rt          Local data to be modified.
 * @param[in]      p_phyif_name  Name of a physical interface which shall be used as egress.
 *                               Names of physical interfaces are hardcoded.
 *                               See the FCI API Reference, chapter Interface Management.
 */
void demo_rt_ld_set_egress_phyif(fpp_rt_cmd_t* p_rt, const char* p_phyif_name)
{
    assert(NULL != p_rt);
    assert(NULL != p_phyif_name);
    set_text((p_rt->output_device), p_phyif_name, IFNAMSIZ);
}
 
 
 
 
/*
 * @defgroup    localdata_ct  [localdata_ct]
 * @brief:      Functions marked as [localdata_ct] access only local data. 
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_ct (a struct with configuration data).
 *              When adding a new IPv4 conntrack, there are no "initial data" to be obtained 
 *              from PFE. Simply declare a local data struct and configure it.
 *              Then, after all modifications are done and finished,
 *              call demo_ct_add() to create a new IPv4 conntrack in PFE. 
 */
 
 
/*
 * @brief          Set a protocol type of an IPv4 conntrack.
 * @details        [localdata_ct]
 * @param[in,out]  p_ct      Local data to be modified.
 * @param[in]      protocol  IP protocol ID
 *                           See "IANA Assigned Internet Protocol Number":
 *                https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 */
void demo_ct_ld_set_protocol(fpp_ct_cmd_t* p_ct, uint16_t protocol)
{
    assert(NULL != p_ct);
    p_ct->protocol = htons(protocol);
}
 
 
/*
 * @brief          Set a ttl decrement flag of an IPv4 conntrack.
 * @details        [localdata_ct]
 *                 If set, then the TTL value of a packet is decremented when
 *                 the packet is routed by the IPv4 conntrack.
 * @param[in,out]  p_ct  Local data to be modified.
 * @param[in]      set   Request to enable/disable the ttl decrement.
 */
void demo_ct_ld_set_ttl_decr(fpp_ct_cmd_t* p_ct, bool set)
{
    assert(NULL != p_ct);
    
    if (set)
    {
        p_ct->flags |= htons(CTCMD_FLAGS_TTL_DECREMENT);
    }
    else
    {
        p_ct->flags &= htons((uint16_t)(~CTCMD_FLAGS_TTL_DECREMENT));
    }
}
 
 
/*
 * @brief          Set "orig direction" data of an IPv4 conntrack.
 * @details        [localdata_ct]
 * @param[in,out]  p_ct      Local data to be modified.
 * @param[in]      saddr     IPv4 source address.
 * @param[in]      daddr     IPv4 destination address.
 * @param[in]      sport     Source port.
 * @param[in]      dport     Destination port.
 * @param[in]      vlan      VLAN tag.
 *                             ZERO     : No VLAN tag modifications in this direction.
 *                             non ZERO : --> If a packet is not tagged, 
 *                                            then a VLAN tag is added.
 *                                        --> If a packet is already tagged, 
 *                                            then the VLAN tag is replaced.
 * @param[in]      route_id  ID of a route for the orig direction.
 *                           The route must already exist in PFE.
 *                           See demo_rt_add().
 * @param[in]      unidir_orig_only  Request to make the conntrack unidirectional
 *                                   (orig direction only).
 *                                   If true and the conntrack was previously 
 *                                   configured as "reply direction only",
 *                                   it gets newly reconfigured as "orig direction only".
 */
void demo_ct_ld_set_orig_dir(fpp_ct_cmd_t* p_ct, uint32_t saddr, uint32_t daddr,
                             uint16_t sport, uint16_t dport, uint16_t vlan, 
                             uint32_t route_id, bool unidir_orig_only)
{
    assert(NULL != p_ct);
    
    p_ct->saddr = htonl(saddr);
    p_ct->daddr = htonl(daddr);
    p_ct->sport = htons(sport);
    p_ct->dport = htons(dport);
    p_ct->vlan  = htons(vlan);
    p_ct->route_id = htonl(route_id);
    
    if (unidir_orig_only)
    {
        p_ct->route_id_reply = htonl(0u);
        p_ct->flags |= htons(CTCMD_FLAGS_REP_DISABLED);
        p_ct->flags &= htons((uint16_t)(~CTCMD_FLAGS_ORIG_DISABLED));
    }
}
 
 
/*
 * @brief          Set "reply direction" data of an IPv4 conntrack.
 * @details        [localdata_ct]
 * @param[in,out]  p_ct               Local data to be modified.
 * @param[in]      saddr_reply        IPv4 source address (reply direction).
 * @param[in]      daddr_reply        IPv4 destination address (reply direction).
 * @param[in]      sport_reply        Source port (reply direction).
 * @param[in]      dport_reply        Destination port (reply direction).
 * @param[in]      vlan_reply         VLAN tag (reply direction).
 *                                      ZERO     : No VLAN tag modifications in this direction
 *                                      non ZERO : --> If a packet is not tagged, 
 *                                                     then a VLAN tag is added.
 *                                                 --> If a packet is already tagged, 
 *                                                     then the VLAN tag is replaced.
 * @param[in]      route_id_reply     ID of a route for the reply direction.
 *                                    The route must already exist in PFE.
 *                                    See demo_rt_add().
 * @param[in]      unidir_reply_only  Request to make the conntrack unidirectional 
 *                                    (reply direction only).
 *                                    If true and the conntrack was previously 
 *                                    configured as "orig direction only", 
 *                                    it gets newly reconfigured as "reply direction only".
 */
void demo_ct_ld_set_reply_dir(fpp_ct_cmd_t* p_ct, uint32_t saddr_reply, uint32_t daddr_reply,
                              uint16_t sport_reply, uint16_t dport_reply, uint16_t vlan_reply,
                              uint32_t route_id_reply, bool unidir_reply_only)
{
    assert(NULL != p_ct);
    
    p_ct->saddr_reply = htonl(saddr_reply);
    p_ct->daddr_reply = htonl(daddr_reply);
    p_ct->sport_reply = htons(sport_reply);
    p_ct->dport_reply = htons(dport_reply);
    p_ct->vlan_reply  = htons(vlan_reply);
    p_ct->route_id_reply = htonl(route_id_reply);
    
    if (unidir_reply_only)
    {
        p_ct->route_id = htonl(0u);
        p_ct->flags |= htons(CTCMD_FLAGS_ORIG_DISABLED);
        p_ct->flags &= htons((uint16_t)(~CTCMD_FLAGS_REP_DISABLED));
    }
}
 
 
 
 
/*
 * @defgroup    localdata_ct6  [localdata_ct6]
 * @brief:      Functions marked as [localdata_ct6] access only local data. 
 *              No FCI calls are made.
 * @details:    These functions have a parameter p_ct6 (a struct with configuration data).
 *              When adding a new IPv6 conntrack, there are no "initial data" to be obtained 
 *              from PFE. Simply declare a local data struct and configure it.
 *              Then, after all modifications are done and finished,
 *              call demo_ct6_add() to create a new IPv6 conntrack in PFE. 
 */
 
 
/*
 * @brief          Set a protocol type of an IPv6 conntrack.
 * @details        [localdata_ct6]
 * @param[in,out]  p_ct6     Local data to be modified.
 * @param[in]      protocol  IP protocol ID
 *                           See "IANA Assigned Internet Protocol Number":
 *                https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
 */
void demo_ct6_ld_set_protocol(fpp_ct6_cmd_t* p_ct6, uint16_t protocol)
{
    assert(NULL != p_ct6);
    p_ct6->protocol = htons(protocol);
}
 
 
/*
 * @brief          Set a ttl decrement flag of an IPv6 conntrack.
 * @details        [localdata_ct6]
 *                 If set, then the TTL value of a packet is decremented when
 *                 the packet is routed by the IPv6 conntrack.
 * @param[in,out]  p_ct6  Local data to be modified.
 * @param[in]      set    Request to enable/disable the ttl decrement.
 */
void demo_ct6_ld_set_ttl_decr(fpp_ct6_cmd_t* p_ct6, bool set)
{
    assert(NULL != p_ct6);
    
    if (set)
    {
        p_ct6->flags |= htons(CTCMD_FLAGS_TTL_DECREMENT);
    }
    else
    {
        p_ct6->flags &= htons((uint16_t)(~CTCMD_FLAGS_TTL_DECREMENT));
    }
}
 
 
/*
 * @brief          Set "orig direction" data of an IPv6 conntrack.
 * @details        [localdata_ct6]
 * @param[in,out]  p_ct6     Local data to be modified.
 * @param[in]      p_saddr   IPv6 source address.
 * @param[in]      p_daddr   IPv6 destination address.
 * @param[in]      sport     Source port.
 * @param[in]      dport     Destination port.
 * @param[in]      vlan      VLAN tag
 *                             ZERO     : No VLAN tag modifications in this direction.
 *                             non ZERO : --> If a packet is not tagged, 
 *                                            then a VLAN tag is added.
 *                                        --> If a packet is already tagged, 
 *                                            then the VLAN tag is replaced.
 * @param[in]      route_id  ID of a route for the orig direction.
 *                           The route must already exist in PFE.
 *                           See demo_rt_add().
 * @param[in]      unidir_orig_only  Request to make the conntrack unidirectional
 *                                   (orig direction only).
 *                                   If true and the conntrack was previously 
 *                                   configured as "reply direction only",
 *                                   it gets newly reconfigured as "orig direction only".
 */
void demo_ct6_ld_set_orig_dir(fpp_ct6_cmd_t* p_ct6, const uint32_t p_saddr[4],
                              const uint32_t p_daddr[4],
                              uint16_t sport, uint16_t dport, uint16_t vlan,
                              uint32_t route_id, bool unidir_orig_only)
{
    assert(NULL != p_ct6);
    assert(NULL != p_saddr);
    assert(NULL != p_daddr);
    
    p_ct6->saddr[0] = htonl(p_saddr[0]);
    p_ct6->saddr[1] = htonl(p_saddr[1]);
    p_ct6->saddr[2] = htonl(p_saddr[2]);
    p_ct6->saddr[3] = htonl(p_saddr[3]);
    
    p_ct6->daddr[0] = htonl(p_daddr[0]);
    p_ct6->daddr[1] = htonl(p_daddr[1]);
    p_ct6->daddr[2] = htonl(p_daddr[2]);
    p_ct6->daddr[3] = htonl(p_daddr[3]);
    
    p_ct6->sport = htons(sport);
    p_ct6->dport = htons(dport);
    p_ct6->vlan  = htons(vlan);
    p_ct6->route_id = htonl(route_id);
    
    if (unidir_orig_only)
    {
        p_ct6->route_id_reply = htonl(0u);
        p_ct6->flags |= htons(CTCMD_FLAGS_REP_DISABLED);
        p_ct6->flags &= htons((uint16_t)(~CTCMD_FLAGS_ORIG_DISABLED));
    }
}
 
 
/*
 * @brief          Set "reply direction" data of an IPv6 conntrack.
 * @details        [localdata_ct6]
 * @param[in,out]  p_ct6              Local data to be modified.
 * @param[in]      p_saddr_reply      IPv6 source address (reply direction).
 * @param[in]      p_daddr_reply      IPv6 destination address (reply direction).
 * @param[in]      sport_reply        Source port (reply direction).
 * @param[in]      dport_reply        Destination port (reply direction).
 * @param[in]      vlan_reply         VLAN tag (reply direction).
 *                                      ZERO     : No VLAN tag modifications in this direction
 *                                      non ZERO : --> If a packet is not tagged, 
 *                                                     then a VLAN tag is added.
 *                                                 --> If a packet is already tagged, 
 *                                                     then the VLAN tag is replaced.
 * @param[in]      route_id_reply     ID of a route for the reply direction.
 * @param[in]      unidir_reply_only  Request to make the conntrack unidirectional 
 *                                    (reply direction only).
 *                                    If true and the conntrack was previously 
 *                                    configured as "orig direction only", 
 *                                    it gets newly reconfigured as "reply direction only".
 */
void demo_ct6_ld_set_reply_dir(fpp_ct6_cmd_t* p_ct6, const uint32_t p_saddr_reply[4],
                               const uint32_t p_daddr_reply[4],
                               uint16_t sport_reply, uint16_t dport_reply,uint16_t vlan_reply,
                               uint32_t route_id_reply, bool unidir_reply_only)
{
    assert(NULL != p_ct6);
    assert(NULL != p_saddr_reply);
    assert(NULL != p_daddr_reply);
    
    p_ct6->saddr_reply[0] = htonl(p_saddr_reply[0]);
    p_ct6->saddr_reply[1] = htonl(p_saddr_reply[1]);
    p_ct6->saddr_reply[2] = htonl(p_saddr_reply[2]);
    p_ct6->saddr_reply[3] = htonl(p_saddr_reply[3]);
    
    p_ct6->daddr_reply[0] = htonl(p_daddr_reply[0]);
    p_ct6->daddr_reply[1] = htonl(p_daddr_reply[1]);
    p_ct6->daddr_reply[2] = htonl(p_daddr_reply[2]);
    p_ct6->daddr_reply[3] = htonl(p_daddr_reply[3]);
    
    p_ct6->sport_reply = htons(sport_reply);
    p_ct6->dport_reply = htons(dport_reply);
    p_ct6->vlan_reply  = htons(vlan_reply);
    p_ct6->route_id_reply = htonl(route_id_reply);
    
    if (unidir_reply_only)
    {
        p_ct6->route_id = htonl(0u);
        p_ct6->flags |= htons(CTCMD_FLAGS_ORIG_DISABLED);
        p_ct6->flags &= htons((uint16_t)(~CTCMD_FLAGS_REP_DISABLED));
    }
}
 
 
/* ==== PUBLIC FUNCTIONS : query local data (no FCI calls) ================= */
 
 
/*
 * @brief      Query whether a route is an IPv4 route.
 * @details    [localdata_rt]
 * @param[in]  p_rt  Local data to be queried.
 * @return     The route:
 *             true  : is an IPv4 route.
 *             false : is NOT an IPv4 route.
 */
bool demo_rt_ld_is_ip4(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    return (bool)(ntohl(p_rt->flags) & FPP_IP_ROUTE_6o4);
}
 
 
/*
 * @brief      Query whether a route is an IPv6 route.
 * @details    [localdata_rt]
 * @param[in]  p_rt  Local data to be queried.
 * @return     The route:
 *             true  : is an IPv6 route.
 *             false : is NOT an IPv6 route.
 */
bool demo_rt_ld_is_ip6(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    return (bool)(ntohl(p_rt->flags) & FPP_IP_ROUTE_4o6);
}
 
 
/*
 * @brief      Query the ID of a route.
 * @details    [localdata_rt]
 * @param[in]  p_rt  Local data to be queried.
 * @return     ID of the route.
 */
uint32_t demo_rt_ld_get_route_id(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    return ntohl(p_rt->id);
}
 
 
/*
 * @brief      Query the source MAC of a route.
 * @details    [localdata_rt]
 * @param[in]  p_rt  Local data to be queried.
 * @return     Source MAC which shall be set in the routed traffic.
 */
const uint8_t* demo_rt_ld_get_src_mac(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    return (p_rt->src_mac);
}
 
 
/*
 * @brief      Query the destination MAC of a route.
 * @details    [localdata_rt]
 * @param[in]  p_rt  Local data to be queried.
 * @return     Destination MAC which shall be set in the routed traffic.
 */
const uint8_t* demo_rt_ld_get_dst_mac(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    return (p_rt->dst_mac);
}
 
 
/*
 * @brief      Query the egress interface of a route.
 * @details    [localdata_rt]
 * @param[in]  p_rt  Local data to be queried.
 * @return     Name of a physical interface which shall be used as an egress interface.
 */
const char* demo_rt_ld_get_egress_phyif(const fpp_rt_cmd_t* p_rt)
{
    assert(NULL != p_rt);
    return (p_rt->output_device);
}
 
 
 
 
/*
 * @brief      Query whether an IPv4 conntrack serves as a NAT.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : does serve as a NAT.
 *             false : does NOT serve as a NAT.
 */
bool demo_ct_ld_is_nat(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    /* no need to transform byte order when comparing members of one struct */
    return (bool)(((p_ct->daddr_reply) != (p_ct->saddr)) || 
                  ((p_ct->saddr_reply) != (p_ct->daddr)));
}
 
 
/*
 * @brief      Query whether an IPv4 conntrack serves as a PAT.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : does serve as a PAT.
 *             false : does NOT serve as a PAT.
 */
bool demo_ct_ld_is_pat(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    /* no need to transform byte order when comparing members of one struct */
    return (bool)(((p_ct->dport_reply) != (p_ct->sport)) || 
                  ((p_ct->sport_reply) != (p_ct->dport)));
}
 
 
/*
 * @brief      Query whether an IPv4 conntrack modifies VLAN tags.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : does modify VLAN tags of serviced packets.
 *             false : does NOT modify VLAN tags of serviced packets.
 */
bool demo_ct_ld_is_vlan_tagging(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    /* no need to transform byte order when comparing with ZERO */
    return (bool)((0u != p_ct->vlan) || (0u != p_ct->vlan_reply));
}
 
 
/*
 * @brief      Query whether an IPv4 conntrack decrements packet's TTL counter or not.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPV4 conntrack:
 *             true  : does decrement TTL counter.
 *             false : does NOT decrement TTL counter.
 */
bool demo_ct_ld_is_ttl_decr(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)(ntohs(p_ct->flags) & CTCMD_FLAGS_TTL_DECREMENT);
}
 
 
/*
 * @brief      Query whether an IPv4 conntrack is orig direction only.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : is orig direction only.
 *             false : is NOT orig direction only.
 */
bool demo_ct_ld_is_orig_only(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)(ntohs(p_ct->flags) & CTCMD_FLAGS_REP_DISABLED);
}
 
 
/*
 * @brief      Query whether an IPv4 conntrack is reply direction only.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     The IPv4 conntrack:
 *             true  : is reply direction only.
 *             false : is NOT reply direction only.
 */
bool demo_ct_ld_is_reply_only(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return (bool)(ntohs(p_ct->flags) & CTCMD_FLAGS_ORIG_DISABLED);
}
 
 
/*
 * @brief      Query the protocol of an IPv4 conntrack.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     IP Protocol ID
 */
uint16_t demo_ct_ld_get_protocol(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohs(p_ct->protocol);
}
 
 
/*
 * @brief      Query the source address of an IPv4 conntrack.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Source IPv4 address.
 */
uint32_t demo_ct_ld_get_saddr(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->saddr);
}
 
 
/*
 * @brief      Query the destination address of an IPv4 conntrack.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Destination IPv4 address.
 */
uint32_t demo_ct_ld_get_daddr(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->daddr);
}
 
 
/*
 * @brief      Query the source port of an IPv4 conntrack.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Source port.
 */
uint16_t demo_ct_ld_get_sport(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohs(p_ct->sport);
}
 
 
/*
 * @brief      Query the destination port of an IPv4 conntrack.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Destination port.
 */
uint16_t demo_ct_ld_get_dport(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohs(p_ct->dport);
}
 
 
/*
 * @brief      Query the used VLAN tag of an IPv4 conntrack.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     VLAN tag. 0 == no VLAN tagging in this direction.
 */
uint16_t demo_ct_ld_get_vlan(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohs(p_ct->vlan);
}
 
 
/*
 * @brief      Query the route ID of an IPv4 conntrack.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Route ID.
 */
uint32_t demo_ct_ld_get_route_id(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->route_id);
}
 
 
/*
 * @brief      Query the source address of an IPv4 conntrack (reply direction).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Source IPv4 address (reply direction).
 */
uint32_t demo_ct_ld_get_saddr_reply(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->saddr_reply);
}
 
 
/*
 * @brief      Query the destination address of an IPv4 conntrack (reply direction).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Destination IPv4 address (reply direction).
 */
uint32_t demo_ct_ld_get_daddr_reply(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->daddr_reply);
}
 
 
/*
 * @brief      Query the source port of an IPv4 conntrack (reply direction).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Source port (reply direction).
 */
uint16_t demo_ct_ld_get_sport_reply(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohs(p_ct->sport_reply);
}
 
 
/*
 * @brief      Query the destination port of an IPv4 conntrack (reply direction).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Destination port (reply direction).
 */
uint16_t demo_ct_ld_get_dport_reply(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohs(p_ct->dport_reply);
}
 
 
/*
 * @brief      Query the used VLAN tag of an IPv4 conntrack (reply direction).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     VLAN tag (reply direction). 0 == no VLAN tagging in this direction.
 */
uint16_t demo_ct_ld_get_vlan_reply(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohs(p_ct->vlan_reply);
}
 
 
/*
 * @brief      Query the route ID of an IPv4 conntrack (reply direction).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Route ID (reply direction).
 */
uint32_t demo_ct_ld_get_route_id_reply(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->route_id_reply);
}
 
 
/*
 * @brief      Query the flags of an IPv4 conntrack.
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Flags bitset at time when the data was obtained from PFE.
 */
uint16_t demo_ct_ld_get_flags(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohs(p_ct->flags);
}
 
 
/*
 * @brief      Query the statistics of an IPv4 conntrack (number of frames).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Number of frames that used the conntrack.
 */
uint32_t demo_ct_ld_get_stt_hit(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->stats.hit);
}
 
 
/*
 * @brief      Query the statistics of an IPv6 conntrack (number of bytes).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Sum of bytesizes of all frames that used the conntrack.
 */
uint32_t demo_ct_ld_get_stt_hit_bytes(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->stats.hit_bytes);
}
 
 
/*
 * @brief      Query the statistics of an IPv4 conntrack (number of frames).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Number of frames that used the conntrack (reply direction).
 */
uint32_t demo_ct_ld_get_stt_reply_hit(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->stats_reply.hit);
}
 
 
/*
 * @brief      Query the statistics of an IPv4 conntrack (number of bytes).
 * @details    [localdata_ct]
 * @param[in]  p_ct  Local data to be queried.
 * @return     Sum of bytesizes of all frames that used the conntrack (reply direction).
 */
uint32_t demo_ct_ld_get_stt_reply_hit_bytes(const fpp_ct_cmd_t* p_ct)
{
    assert(NULL != p_ct);
    return ntohl(p_ct->stats_reply.hit_bytes);
}
 
 
 
 
/*
 * @brief      Query whether an IPv6 conntrack serves as a NAT.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : does serve as a NAT.
 *             false : does NOT serve as a NAT.
 */
bool demo_ct6_ld_is_nat(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    /* no need to transform byte order when comparing members of one struct */
    return (bool)((0 != memcmp(p_ct6->daddr_reply, p_ct6->saddr, (4 * sizeof(uint32_t)))) ||
                  (0 != memcmp(p_ct6->saddr_reply, p_ct6->daddr, (4 * sizeof(uint32_t)))));
}
 
 
/*
 * @brief      Query whether an IPv6 conntrack serves as a PAT.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : does serve as a PAT.
 *             false : does NOT serve as a PAT.
 */
bool demo_ct6_ld_is_pat(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    /* no need to transform byte order when comparing members of one struct */
    return (bool)(((p_ct6->dport_reply) != (p_ct6->sport)) || 
                  ((p_ct6->sport_reply) != (p_ct6->dport)));
}
 
 
/*
 * @brief      Query whether an IPv6 conntrack modifies VLAN tags.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : does modify VLAN tags of serviced packets.
 *             false : does NOT modify VLAN tags of serviced packets.
 */
bool demo_ct6_ld_is_vlan_tagging(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    /* no need to transform byte order when comparing with ZERO */
    return (bool)((0u != p_ct6->vlan) || (0u != p_ct6->vlan_reply));
}
 
 
/*
 * @brief      Query whether an IPv6 conntrack decrements packet's TTL counter or not.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPV6 conntrack:
 *             true  : does decrement TTL counter.
 *             false : does NOT decrement TTL counter.
 */
bool demo_ct6_ld_is_ttl_decr(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)(ntohs(p_ct6->flags) & CTCMD_FLAGS_TTL_DECREMENT);
}
 
 
/*
 * @brief      Query whether an IPv6 conntrack is orig direction only.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : is orig direction only.
 *             false : is NOT orig direction only.
 */
bool demo_ct6_ld_is_orig_only(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)(ntohs(p_ct6->flags) & CTCMD_FLAGS_REP_DISABLED);
}
 
 
/*
 * @brief      Query whether an IPv6 conntrack is reply direction only.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     The IPv6 conntrack:
 *             true  : is reply direction only.
 *             false : is NOT reply direction only.
 */
bool demo_ct6_ld_is_reply_only(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return (bool)(ntohs(p_ct6->flags) & CTCMD_FLAGS_ORIG_DISABLED);
}
 
 
/*
 * @brief      Query the protocol of an IPv6 conntrack.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     IP Protocol ID.
 */
uint16_t demo_ct6_ld_get_protocol(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohs(p_ct6->protocol);
}
 
 
/*
 * @brief      Query the source address of an IPv6 conntrack.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Source IPv6 address.
 */
const uint32_t* demo_ct6_ld_get_saddr(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    static uint32_t rtn_saddr[4] = {0u};
    
    rtn_saddr[0] = ntohl(p_ct6->saddr[0]);
    rtn_saddr[1] = ntohl(p_ct6->saddr[1]);
    rtn_saddr[2] = ntohl(p_ct6->saddr[2]);
    rtn_saddr[3] = ntohl(p_ct6->saddr[3]);
    
    return (rtn_saddr);
}
 
 
/*
 * @brief      Query the destination address of an IPv6 conntrack.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Destination IPv4 address.
 */
const uint32_t* demo_ct6_ld_get_daddr(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    static uint32_t rtn_daddr[4] = {0u};
    
    rtn_daddr[0] = ntohl(p_ct6->daddr[0]);
    rtn_daddr[1] = ntohl(p_ct6->daddr[1]);
    rtn_daddr[2] = ntohl(p_ct6->daddr[2]);
    rtn_daddr[3] = ntohl(p_ct6->daddr[3]);
    
    return (rtn_daddr);
}
 
 
/*
 * @brief      Query the source port of an IPv6 conntrack.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Source port.
 */
uint16_t demo_ct6_ld_get_sport(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohs(p_ct6->sport);
}
 
 
/*
 * @brief      Query the destination port of an IPv6 conntrack.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Destination port.
 */
uint16_t demo_ct6_ld_get_dport(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohs(p_ct6->dport);
}
 
 
/*
 * @brief      Query the used VLAN tag of an IPv6 conntrack.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     VLAN tag. 0 == no VLAN tagging in this direction.
 */
uint16_t demo_ct6_ld_get_vlan(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohs(p_ct6->vlan);
}
 
 
/*
 * @brief      Query the route ID of an IPv6 conntrack.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Route ID
 */
uint32_t demo_ct6_ld_get_route_id(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohl(p_ct6->route_id);
}
 
 
/*
 * @brief      Query the source address of an IPv6 conntrack (reply direction).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Source IPv6 address (reply direction).
 */
const uint32_t* demo_ct6_ld_get_saddr_reply(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    static uint32_t rtn_saddr_reply[4] = {0u};
    
    rtn_saddr_reply[0] = ntohl(p_ct6->saddr_reply[0]);
    rtn_saddr_reply[1] = ntohl(p_ct6->saddr_reply[1]);
    rtn_saddr_reply[2] = ntohl(p_ct6->saddr_reply[2]);
    rtn_saddr_reply[3] = ntohl(p_ct6->saddr_reply[3]);
    
    return (rtn_saddr_reply);
}
 
 
/*
 * @brief      Query the destination address of an IPv6 conntrack (reply direction).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Destination IPv6 address (reply direction).
 */
const uint32_t* demo_ct6_ld_get_daddr_reply(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    static uint32_t rtn_daddr_reply[4] = {0u};
    
    rtn_daddr_reply[0] = ntohl(p_ct6->daddr_reply[0]);
    rtn_daddr_reply[1] = ntohl(p_ct6->daddr_reply[1]);
    rtn_daddr_reply[2] = ntohl(p_ct6->daddr_reply[2]);
    rtn_daddr_reply[3] = ntohl(p_ct6->daddr_reply[3]);
    
    return (rtn_daddr_reply);
}
 
 
/*
 * @brief      Query the source port of an IPv6 conntrack (reply direction).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Source port (reply direction).
 */
uint16_t demo_ct6_ld_get_sport_reply(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohs(p_ct6->sport_reply);
}
 
 
/*
 * @brief      Query the destination port of an IPv6 conntrack (reply direction).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Destination port (reply direction).
 */
uint16_t demo_ct6_ld_get_dport_reply(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohs(p_ct6->dport_reply);
}
 
 
/*
 * @brief      Query the used VLAN tag of an IPv6 conntrack (reply direction).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     VLAN tag (reply direction). 0 == no VLAN tagging in this direction.
 */
uint16_t demo_ct6_ld_get_vlan_reply(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohs(p_ct6->vlan_reply);
}
 
 
/*
 * @brief      Query the route ID of an IPv6 conntrack (reply direction).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Route ID (reply direction).
 */
uint32_t demo_ct6_ld_get_route_id_reply(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohl(p_ct6->route_id_reply);
}
 
 
/*
 * @brief      Query the flags of an IPv6 conntrack.
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Flags bitset at time when the data was obtained from PFE.
 */
uint16_t demo_ct6_ld_get_flags(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohs(p_ct6->flags);
}
 
 
/*
 * @brief      Query the statistics of an IPv6 conntrack (number of frames).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Number of frames that used the conntrack.
 */
uint32_t demo_ct6_ld_get_stt_hit(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohl(p_ct6->stats.hit);
}
 
 
/*
 * @brief      Query the statistics of an IPv6 conntrack (number of bytes).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Sum of bytesizes of all frames that used the conntrack.
 */
uint32_t demo_ct6_ld_get_stt_hit_bytes(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohl(p_ct6->stats.hit_bytes);
}
 
 
/*
 * @brief      Query the statistics of an IPv6 conntrack (number of frames).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Number of frames that used the conntrack (reply direction).
 */
uint32_t demo_ct6_ld_get_stt_reply_hit(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohl(p_ct6->stats_reply.hit);
}
 
 
/*
 * @brief      Query the statistics of an IPv6 conntrack (number of bytes).
 * @details    [localdata_ct6]
 * @param[in]  p_ct6  Local data to be queried.
 * @return     Sum of bytesizes of all frames that used the conntrack (reply direction).
 */
uint32_t demo_ct6_ld_get_stt_reply_hit_bytes(const fpp_ct6_cmd_t* p_ct6)
{
    assert(NULL != p_ct6);
    return ntohl(p_ct6->stats_reply.hit_bytes);
}
 
 
/* ==== PUBLIC FUNCTIONS : misc ============================================ */
 
 
/*
 * @brief      Use FCI calls to iterate through all available routes in PFE and
 *             execute a callback print function for each applicable route.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next route is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @param[in]  print_ip4   Set true to print IPv4 routes.
 * @param[in]  print_ip6   Set true to print IPv6 routes.
 * @return     FPP_ERR_OK : Successfully iterated through all applicable routes.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_rt_print_all(FCI_CLIENT* p_cl, demo_rt_cb_print_t p_cb_print, 
                      bool print_ip4, bool print_ip6)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_rt_cmd_t cmd_to_fci = {0};
    fpp_rt_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                    sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        if ((print_ip4) && demo_rt_ld_is_ip4(&reply_from_fci))
        {
            rtn = p_cb_print(&reply_from_fci);  /* print IPv4 route */
        }
        if ((print_ip6) && demo_rt_ld_is_ip6(&reply_from_fci))
        {
            rtn = p_cb_print(&reply_from_fci);  /* print IPv6 route */
        }
        
        if (FPP_ERR_OK == rtn)
        {
            cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
            rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                            sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                            &reply_length, (unsigned short*)(&reply_from_fci));
        }
    }
    
    /* query loop runs till there are no more routes to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_RT_ENTRY_NOT_FOUND == rtn) 
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_rt_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available routes in PFE.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_count  Space to store the count of routes.
 * @return      FPP_ERR_OK : Successfully counted all available routes.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_rt_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_rt_cmd_t cmd_to_fci = {0};
    fpp_rt_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                    sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /*  query loop  */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IP_ROUTE,
                        sizeof(fpp_rt_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more routes to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_RT_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_rt_get_count() failed!");
    
    return (rtn);
}
 
 
 
 
/*
 * @brief      Use FCI calls to iterate through all available IPv4 conntracks in PFE and
 *             execute a callback print function for each reported IPv4 conntrack.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next IPv4 conntrack is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all available IPv4 conntracks.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_ct_print_all(FCI_CLIENT* p_cl, demo_ct_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_ct_cmd_t cmd_to_fci = {0};
    fpp_ct_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                    sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
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
        }
    }
    
    /* query loop runs till there are no more IPv4 conntracks to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_CT_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_ct_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available IPv4 conntracks in PFE.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_count  Space to store the count of IPv4 conntracks.
 * @return      FPP_ERR_OK : Successfully counted all available IPv4 conntracks.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_ct_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_ct_cmd_t cmd_to_fci = {0};
    fpp_ct_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                    sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IPV4_CONNTRACK,
                        sizeof(fpp_ct_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more IPv4 conntracks to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_CT_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_ct_get_count() failed!");
    
    return (rtn);
}
 
 
 
 
/*
 * @brief      Use FCI calls to iterate through all available IPv6 conntracks in PFE and
 *             execute a callback print function for each reported IPv6 conntrack.
 * @param[in]  p_cl        FCI client
 * @param[in]  p_cb_print  Callback print function.
 *                         --> If the callback returns ZERO, then all is OK and 
 *                             a next IPv6 conntrack is picked for a print process.
 *                         --> If the callback returns NON-ZERO, then some problem is 
 *                             assumed and this function terminates prematurely.
 * @return     FPP_ERR_OK : Successfully iterated through all available IPv6 conntracks.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_ct6_print_all(FCI_CLIENT* p_cl, demo_ct6_cb_print_t p_cb_print)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_print);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_ct6_cmd_t cmd_to_fci = {0};
    fpp_ct6_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                    sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
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
        }
    }
    
    /* query loop runs till there are no more IPv6 conntracks to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_CT_ENTRY_NOT_FOUND == rtn)
    {
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_ct6_print_all() failed!");
    
    return (rtn);
}
 
 
/*
 * @brief       Use FCI calls to get a count of all available IPv6 conntracks in PFE.
 * @param[in]   p_cl         FCI client
 * @param[out]  p_rtn_count  Space to store the count of IPv6 conntracks.
 * @return      FPP_ERR_OK : Successfully counted all available IPv6 conntracks.
 *                           Count was stored into p_rtn_count.
 *              other      : Some error occurred (represented by the respective error code).
 *                           No count was stored.
 */
int demo_ct6_get_count(FCI_CLIENT* p_cl, uint32_t* p_rtn_count)
{
    assert(NULL != p_cl);
    assert(NULL != p_rtn_count);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    fpp_ct6_cmd_t cmd_to_fci = {0};
    fpp_ct6_cmd_t reply_from_fci = {0};
    unsigned short reply_length = 0u;
    uint32_t count = 0u;
        
    /* start query process */
    cmd_to_fci.action = FPP_ACTION_QUERY;
    rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                    sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                    &reply_length, (unsigned short*)(&reply_from_fci));
    
    /* query loop */
    while (FPP_ERR_OK == rtn)
    {
        count++;
        
        cmd_to_fci.action = FPP_ACTION_QUERY_CONT;
        rtn = fci_query(p_cl, FPP_CMD_IPV6_CONNTRACK,
                        sizeof(fpp_ct6_cmd_t), (unsigned short*)(&cmd_to_fci),
                        &reply_length, (unsigned short*)(&reply_from_fci));
    }
    
    /* query loop runs till there are no more IPv6 conntracks to report */
    /* the following error is therefore OK and expected (it ends the query loop) */
    if (FPP_ERR_CT_ENTRY_NOT_FOUND == rtn)
    {
        *p_rtn_count = count;
        rtn = FPP_ERR_OK;
    }
    
    print_if_error(rtn, "demo_ct6_get_count() failed!");
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
