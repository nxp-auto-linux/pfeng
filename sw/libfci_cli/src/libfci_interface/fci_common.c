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
#include <stddef.h>
#include <arpa/inet.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
#include "fci_common.h"
 
 
/* ==== PUBLIC FUNCTIONS =================================================== */
 
 
/*
 * @brief          Network-to-host (ntoh) function for enum datatypes.
 * @param[in,out]  p_rtn  The value which is to be converted to the host byte order.
 * @param[in]      size   Size of the value datatype (in bytes).
 */
void ntoh_enum(void* p_rtn, size_t size)
{
    assert(NULL != p_rtn);
    
    switch (size)
    {
        case (sizeof(uint16_t)):
            *((uint16_t*)p_rtn) = ntohs(*((uint16_t*)p_rtn));
        break;
        
        case (sizeof(uint32_t)):
            *((uint32_t*)p_rtn) = ntohl(*((uint32_t*)p_rtn));
        break;
        
        default:
            /* do nothing ; 'uint8_t' falls into this category as well */
        break;
    }
}
 
 
/*
 * @brief          Host-to-network (hton) function for enum datatypes.
 * @param[in,out]  p_rtn  The value which is to be converted to the network byte order.
 * @param[in]      size   Size of the value datatype (in bytes).
 */
void hton_enum(void* p_rtn, size_t size)
{
    assert(NULL != p_rtn);
    
    switch (size)
    {
        case (sizeof(uint16_t)):
            *((uint16_t*)p_rtn) = htons(*((uint16_t*)p_rtn));
        break;
        
        case (sizeof(uint32_t)):
            *((uint32_t*)p_rtn) = htonl(*((uint32_t*)p_rtn));
        break;
        
        default:
            /* do nothing ; 'uint8_t' falls into this category as well */
        break;
    }
}
 
 
/*
 * @brief       Check and set text
 * @param[out]  p_dst   Destination text array (to be modified).
 * @param[in]   p_src   Source text array.
 *                      Can be NULL or empty (""). If NULL or empty, then
 *                      the destination text array is zeroed.
 * @param[in]   dst_ln  Size of the destination text array.
 * @return      FPP_ERR_OK : Function executed successfully.
 *              other      : Some error occured (represented by the respective error code).
 */
int set_text(char* p_dst, const char* p_src, const uint_fast16_t dst_ln)
{
    assert(NULL != p_dst);
    assert(0u != dst_ln);
    /* 'p_src' is allowed to be NULL */
    
    
    int rtn = FPP_ERR_FCI;
    
    if ((NULL == p_src) || ('\0' == p_src[0]))
    {
        /* zeroify dst */
        memset(p_dst, 0, dst_ln);
        rtn = FPP_ERR_OK;
    }
    else if ((strlen(p_src) + 1u) > dst_ln)
    {
        rtn = FPP_ERR_FCI_INVTXTLN;
    }
    else
    {
        /* set dst */
        strncpy(p_dst, p_src, dst_ln);
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Lock the interface database for exclusive access by this FCI client.
 * @details    The interface database is stored in the PFE.
 *             For details, see FCI API Reference, description of FPP_CMD_IF_LOCK_SESSION.
 * @param[in]  p_cl  FCI client instance
 * @return     FPP_ERR_OK : Lock successful
 *             other      : Lock not successful
 */
int fci_if_session_lock(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    return fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0u, NULL);
}
 
 
/*
 * @brief      Unlock the interface database's exclusive access lock.
 * @param[in]  p_cl  FCI client instance
 * @param[in]  rtn   Caller's current return value
 * @return     If caller provides NON-ZERO rtn, then 
 *             this function returns the provided rtn value.
 *             If caller provides ZERO rtn, then return values are:
 *             FPP_ERR_OK : Unlock successful
 *             other      : Unlock not successful
 */
int fci_if_session_unlock(FCI_CLIENT* p_cl, int rtn)
{
    assert(NULL != p_cl);
    
    
    int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0u, NULL);    
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
