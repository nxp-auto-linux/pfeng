/* =========================================================================
 *  Copyright 2019 NXP
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

/**
 * @addtogroup	dxgrPFE_PLATFORM
 * @{
 *
 * @defgroup    dxgr_PFE_IDEX IDEX
 * @brief		The Inter Driver EXchange module
 * @details     IDEX module is responsible for inter-driver communication in
 * 				master-slave scenario to communicate platform driver runtime
 * 				and control data.
 *
 * @addtogroup  dxgr_PFE_IDEX
 * @{
 *
 * @file		pfe_idex.h
 * @brief		The IDEX module header file.
 * @details		This file contains IDEX-related API.
 *
 */

#ifndef PUBLIC_PFE_IDEX_H_
#define PUBLIC_PFE_IDEX_H_

#include "pfe_hif_drv.h"

/**
 * @brief		RPC request callback type
 * @details		To be called when IDEX has received RPC request
 * @warning		Don't block or sleep within the body
 * @see			pfe_idex_set_rpc_cbk()
 * @param[in]	id Request identifier
 * @param[in]	buf Pointer to request argument. Can be NULL.
 * @param[in]	buf_len Lenght of request argument. Can be zero.
 * @param[in]	arg Custom argument provided via pfe_idex_set_rpc_cbk()
 */
typedef void (*pfe_idex_rpc_cbk_t)(uint32_t id, void *buf, uint16_t buf_len, void *arg);

errno_t pfe_idex_init(pfe_hif_drv_t *hif_drv);
errno_t pfe_idex_rpc(pfe_ct_phy_if_id_t dst_phy, uint32_t id, void *buf, uint16_t buf_len, void *resp, uint16_t resp_len);
errno_t pfe_idex_master_rpc(uint32_t id, void *buf, uint16_t buf_len, void *resp, uint16_t resp_len);
errno_t pfe_idex_set_rpc_cbk(pfe_idex_rpc_cbk_t cbk, void *arg);
errno_t pfe_idex_set_rpc_ret_val(errno_t retval, void *resp, uint16_t resp_len);
void pfe_idex_fini(void);

#endif /* PUBLIC_PFE_IDEX_H_ */

/** @}*/
/** @}*/
