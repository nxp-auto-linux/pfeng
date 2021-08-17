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
 
