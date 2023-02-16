/* =========================================================================
 *  Copyright 2017-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef FCI_H_
#define FCI_H_

#include "oal.h"
#include "pfe_emac.h" /* pfe_mac_addr_t */
#include "pfe_rtable.h" /* pfe_rtable_t, pfe_rtable_dst_if_t */
#include "pfe_l2br.h" /* pfe_l2br_t */
#include "pfe_class.h" /* pfe_class_t */
#include "pfe_if_db.h"
#include "pfe_tmu.h"	/* pfe_tmu_t */
#include "fci_msg.h"

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
#include "fci_ownership_mask.h"
#endif /* #ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT */

/**
 * @brief	Information passed into the fci_init() function
 * @note	For future use
 */
typedef struct
{
	pfe_rtable_t *rtable;	/* The routing table object */
	pfe_l2br_t *l2_bridge;	/* The L2 bridge instance */
	pfe_class_t *class;		/* The classifier instance */
	pfe_if_db_t *phy_if_db;	/* Pointer to platform driver phy_if DB */
	pfe_if_db_t *log_if_db;	/* Pointer to platform driver log_if DB */
	pfe_tmu_t *tmu;			/* Pointer to platform driver tmu */
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	pfe_fci_owner_hif_id_t hif_fci_owner_chnls_mask;	/* Bit mask representing allowed FCI ownership */
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
} fci_init_info_t;

/**
 * @brief	FCI instance type
 */
typedef struct fci_tag fci_t;

typedef struct
{
	uint32_t stats;
} pfe_fp_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Send message to all FCI clients
 * @param[in]	msg Pointer to the buffer containing payload to be sent
 * @param[in]	rep Pointer to buffer where reply data shall be stored
 * @return		EOK if success, error code otherwise
 */
errno_t fci_core_client_send_broadcast(fci_msg_t *msg, fci_msg_t *rep);

errno_t fci_init(fci_init_info_t *info, const char_t *const identifier);
void fci_fini(void);

uint32_t pfe_fp_get_text_statistics(pfe_fp_t *temp, struct seq_file *seq, uint8_t verb_level);

errno_t fci_process_ipc_message(fci_msg_t *msg, fci_msg_t *rep_msg);	/* This is here because FCI proxy RPC calls need it. */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* FCI_H_ */
