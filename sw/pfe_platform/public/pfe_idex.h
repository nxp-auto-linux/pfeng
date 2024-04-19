/* =========================================================================
 *  Copyright 2019-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_IDEX_H_
#define PUBLIC_PFE_IDEX_H_

#include "pfe_hif_drv.h"
#include "pfe_hif.h"

/**
 * @brief		RPC request callback type
 * @details		To be called when IDEX has received RPC request
 * @warning		Don't block or sleep within the body
 * @param[in]	sender RPC originator identifier
 * @param[in]	id Request identifier
 * @param[in]	buf Pointer to request argument. Can be NULL.
 * @param[in]	buf_len Lenght of request argument. Can be zero.
 * @param[in]	arg Custom argument provided via pfe_idex_init()
 */
typedef void (*pfe_idex_rpc_cbk_t)(pfe_ct_phy_if_id_t sender, uint32_t id, void *buf, uint16_t buf_len, void *arg);
typedef void (*pfe_idex_tx_conf_free_cbk_t)(void *frame);

errno_t pfe_idex_init(pfe_hif_drv_t *hif_drv, pfe_ct_phy_if_id_t master, pfe_hif_t *hif, pfe_idex_rpc_cbk_t cbk, void *arg, pfe_idex_tx_conf_free_cbk_t txcf_cbk);
errno_t pfe_idex_rpc(pfe_ct_phy_if_id_t dst_phy, uint32_t id, const void *buf, uint16_t buf_len, void *resp, uint16_t resp_len);
errno_t pfe_idex_master_rpc(uint32_t id, const void *buf, uint16_t buf_len, void *resp, uint16_t resp_len);
errno_t pfe_idex_set_rpc_ret_val(errno_t retval, void *resp, uint16_t resp_len);
void pfe_idex_down(void);
void pfe_idex_fini(void);
void pfe_idex_mark_down(void);
bool_t pfe_idex_status(void);
void pfe_idex_get_text_statistics(struct seq_file *seq, uint8_t verb_level);

#endif /* PUBLIC_PFE_IDEX_H_ */
