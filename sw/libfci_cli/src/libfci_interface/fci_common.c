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
 
