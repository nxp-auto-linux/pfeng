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
#include "fpp.h"
#include "fpp_ext.h"
#include "libfci.h"
#include "fci_common.h"
#include "fci_ep.h"
 
 
/* ==== PUBLIC FUNCTIONS =================================================== */
 
 
/*
 * @brief       Open connection to an FCI endpoint as a command-mode FCI client.
 * @details     Command-mode client can send FCI commands.
 * @param[out]  pp_rtn_cl  The newly created FCI client.
 * @return      FPP_ERR_OK : The FCI client was successfully created.
 *              other      : Failed to create the FCI client.
 */
int fci_ep_open_in_cmd_mode(FCI_CLIENT** pp_rtn_cl)
{  
    assert(NULL != pp_rtn_cl);
    
    
    int rtn = FPP_ERR_FCI;
    
    FCI_CLIENT* p_cl = fci_open(FCI_CLIENT_DEFAULT, FCI_GROUP_NONE);
    if (NULL == p_cl)
    {
        rtn = FPP_ERR_FCI;
    }
    else
    {
        *pp_rtn_cl = p_cl;
        rtn = FPP_ERR_OK;
    }
    
    return (rtn);
}
 
 
/*
 * @brief      Close connection to the FCI endpoint and destroy the FCI client.
 * @param[in]  p_cl  The FCI client to be destroyed.
 * @return     FPP_ERR_OK : The FCI client was successfully destroyed.
 *             other      : Failed to destroy the FCI client instance.
 */
int fci_ep_close(FCI_CLIENT* p_cl)
{
    assert(NULL != p_cl);
    return fci_close(p_cl);
}
 
 
/* ========================================================================= */
 
