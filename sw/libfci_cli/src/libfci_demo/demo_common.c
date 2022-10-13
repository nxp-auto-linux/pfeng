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
#include <stdio.h>
 
#include <pthread.h>
#include <signal.h>
 
#include <stdint.h>
#include <stddef.h>
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
 
#include "demo_common.h"
 
/* ==== TYPEDEFS & DATA ==================================================== */
 
 static pthread_t pthread_events_catching = {0};
 static volatile sig_atomic_t events_catching_is_running = 0;
 
/* ==== PRIVATE FUNCTIONS ================================================== */
 
 
 /*
 * @brief      Main function in the parallel events-processing thread.
 * @param[in]  p_cl  FCI client passed as (void*) according to create_thread() convention.
 * @return     Always NULL.
 */
static void* events_catching(void* p_cl)
{
    assert(NULL != p_cl);
    
    int tmp = 0;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &tmp);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &tmp);
    
    /*
        fci_catch is blocking "indefinitely". It will stop blocking only when the registered
        callback function (the one registered with fci_cb_register()) returns anything else
        than FCI_CB_CONTINUE.
    */
    fci_catch((FCI_CLIENT*)(p_cl));
    
    events_catching_is_running = 0;
    return NULL;
}
 
 
/* ==== PUBLIC FUNCTIONS =================================================== */
 
 
/*
 * @brief      Check rtn value and print error text if (FPP_ERR_OK != rtn).
 * @param[in]  rtn          Current return value of a caller function.
 * @param[in]  p_txt_error  Text to be printed if (FPP_ERR_OK != rtn).
 */
void print_if_error(int rtn, const char* p_txt_error)
{
    assert(NULL != p_txt_error);
    
    if (FPP_ERR_OK != rtn)
    {
        printf("ERROR (%d): %s\n", rtn, p_txt_error);
    }
}
 
 
/*
 * @brief          Network-to-host (ntoh) function for enum datatypes.
 * @param[in,out]  p_rtn  Value which is to be converted to a host byte order.
 * @param[in]      size   Byte size of the value.
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
 * @param[in,out]  p_rtn  Value which is to be converted to a network byte order.
 * @param[in]      size   Byte size of the value.
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
 * @brief       Check and set text.
 * @param[out]  p_dst   Destination text array (to be modified).
 * @param[in]   p_src   Source text array.
 *                      Can be NULL or empty (""). If NULL or empty, then
 *                      the destination text array is zeroed.
 * @param[in]   dst_ln  Size of the destination text array.
 * @return      FPP_ERR_OK : Function executed successfully.
 *              other      : Some error occured (represented by the respective error code).
 */
int set_text(char* p_dst, const char* p_src, const uint16_t dst_ln)
{
    assert(NULL != p_dst);
    assert(0u != dst_ln);
    /* 'p_src' is allowed to be NULL */
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    if ((NULL == p_src) || ('\0' == p_src[0]))
    {
        /* zeroify dst */
        memset(p_dst, 0, dst_ln);
        rtn = FPP_ERR_OK;
    }
    else if ((strlen(p_src) + 1u) > dst_ln)
    {
        rtn = FPP_ERR_INTERNAL_FAILURE;  /* src is too long */
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
 * @brief      Lock the interface database of PFE for exclusive access by this FCI client.
 * @details    The interface database is stored in PFE.
 * @param[in]  p_cl  FCI client
 * @return     FPP_ERR_OK : Lock successful
 *             other      : Lock not successful
 */
int demo_if_session_lock(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    return fci_write(p_cl, FPP_CMD_IF_LOCK_SESSION, 0u, NULL);
}
 
 
/*
 * @brief      Unlock exclusive access lock of the PFE's interface database.
 * @details    The exclusive access lock can be unlocked only by a FCI client which 
 *             currently holds exclusive access to the interface database.
 * @param[in]  p_cl  FCI client
 * @param[in]  rtn   Current return value of a caller function.
 * @return     If a caller function provides NON-ZERO rtn, then that rtn value is returned.
 *             If a caller function provides ZERO rtn, then return values are:
 *             FPP_ERR_OK : Unlock successful
 *             other      : Unlock not successful
 */
int demo_if_session_unlock(FCI_CLIENT* p_cl, int rtn)
{
    assert(NULL != p_cl);
    
    int rtn_unlock = fci_write(p_cl, FPP_CMD_IF_UNLOCK_SESSION, 0u, NULL);
    rtn = ((FPP_ERR_OK == rtn) ? (rtn_unlock) : (rtn));
    
    return (rtn);
}
 
 
/*
 * @brief       Open connection to an FCI endpoint as a command-mode FCI client.
 * @details     Command-mode client can configure PFE via the FCI endpoint by 
 *              issuing FCI commands.
 * @param[out]  pp_rtn_cl  Pointer to a newly created FCI client.
 * @return      FPP_ERR_OK : New FCI client was successfully created.
 *              other      : Failed to create a FCI client.
 */
int demo_client_open_in_cmd_mode(FCI_CLIENT** pp_rtn_cl)
{  
    assert(NULL != pp_rtn_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    FCI_CLIENT* p_cl = fci_open(FCI_CLIENT_DEFAULT, FCI_GROUP_NONE);
    if (NULL != p_cl)
    {
        *pp_rtn_cl = p_cl;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Close connection to a FCI endpoint and destroy the associated FCI client.
 * @param[in]  p_cl  The FCI client to be destroyed.
 * @return     FPP_ERR_OK : The FCI client was successfully destroyed.
 *             other      : Failed to destroy the FCI client instance.
 */
int demo_client_close(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    return fci_close(p_cl);
}
 
 
/*
 * @brief      Initialize a parallel thread for FCI events catching.
 * @details    FCI events are sent by PFE driver to FCI clients.
 *             Client must register a callback in order to receive FCI events.
 * @param[in]  p_cl         The FCI client
 * @param[in]  p_cb_events  Callback function which will be used to process caught FCI events.
 * @return     FPP_ERR_OK : FCI callback was registered and parallel thread was started.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_events_catching_init(FCI_CLIENT* p_cl, demo_events_cb_t p_cb_events)
{
    assert(NULL != p_cl);
    assert(NULL != p_cb_events);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    /* this demo allows only one parallel thread for FCI events catching */
    if (0 != events_catching_is_running)
    {
        rtn = FPP_ERR_INTERNAL_FAILURE;
        print_if_error(rtn, "demo_events_catching_init(): Some instance of the parallel "
                            "thread for catching FCI events is already up and running.\n");
    }
    else
    {
        events_catching_is_running = 1;
        rtn = FPP_ERR_OK;
    }
    
    /* register a callback function (to process caught FCI events) for target FCI client */
    if (FPP_ERR_OK == rtn)
    {
        rtn = fci_register_cb(p_cl, p_cb_events);
        if (FPP_ERR_OK != rtn)
        {
            events_catching_is_running = 0;
            print_if_error(rtn, "demo_events_catching_init(): Failed to register a callback "
                                "for processing of caught FCI events.\n");
        }
    }
    
    /* create a parallel thread which hosts fci_catch() */
    /*
        fci_catch() calls its registered callback function each time some 
        FCI event is caught (arrives from PFE driver).
    */
    if (FPP_ERR_OK == rtn)
    {
        rtn = pthread_create(&pthread_events_catching, NULL, &events_catching, p_cl);
        if (0 != rtn)
        {
            events_catching_is_running = 0;
            print_if_error(rtn, "demo_events_catching_init(): Failed to create a parallel "
                                "thread for catching FCI events.\n");
        }
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Stop and destroy the parallel thread for FCI events catching.
 * @param[in]  p_cl         The FCI client
 * @return     FPP_ERR_OK : FCI callback was unregistered and parallel thread was stopped.
 *             other      : Some error occurred (represented by the respective error code).
 */
int demo_events_catching_fini(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    
    int rtn = FPP_ERR_INTERNAL_FAILURE;
    
    /* check that some parallel thread for FCI catching is currently running */
    if (0 == events_catching_is_running)
    {
        rtn = FPP_ERR_INTERNAL_FAILURE;
        print_if_error(rtn, "demo_events_catching_fini(): No parallel thread for catching "
                            "FCI events was detected.\n");
    }
    else
    {
        rtn = FPP_ERR_OK;
    }
    
    /* no need to "unregister" callback function, it will be unregistered by fci_close() */
    
    /* stop and destroy the parallel thread which hosts fci_catch() */
    if (FPP_ERR_OK == rtn)
    {
        rtn = pthread_cancel(pthread_events_catching);
        if (0 != rtn)
        {
            print_if_error(rtn, "demo_events_catching_fini(): Failed to cancel the parallel "
                                "thread for catching FCI events.\n");
        }
    }
    if (FPP_ERR_OK == rtn)
    {
        rtn = pthread_join(pthread_events_catching, NULL);
        if (0 != rtn)
        {
            print_if_error(rtn, "demo_events_fini(): Failed to join the parallel thread "
                                "for catching FCI events.\n");
        }
    }
    
    /* if all OK, then signal that no parallel thread is currently running */
    if (FPP_ERR_OK == rtn)
    {
        events_catching_is_running = 0;
    }
    
    return (rtn);
}
 
 
/* ========================================================================= */
 
