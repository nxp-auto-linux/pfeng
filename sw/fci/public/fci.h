/* =========================================================================
 *  Copyright 2017-2021 NXP
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
#include "fci_msg.h"


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
} fci_init_info_t;

/**
 * @brief	FCI instance type
 */
typedef struct fci_tag fci_t;

errno_t fci_init(fci_init_info_t *info, const char_t *const identifier);
void fci_fini(void);
errno_t fci_core_client_send_broadcast(fci_msg_t *msg, fci_msg_t *rep);

typedef struct 
{
	uint32_t stats;
} pfe_fp_t;

uint32_t pfe_fp_get_text_statistics(pfe_fp_t *temp, char_t *buf, uint32_t buf_len, uint8_t verb_level);

#endif /* FCI_H_ */
