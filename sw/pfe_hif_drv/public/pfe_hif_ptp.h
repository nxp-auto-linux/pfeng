/* =========================================================================
 *
 *  Copyright (c) 2020 Imagination Technologies Limited
 *  Copyright 2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_HIF_PTP_H_
#define PUBLIC_PFE_HIF_PTP_H_

#include "linked_list.h"

typedef struct
{
	oal_mutex_t *lock;
	LLIST_t entries;
	oal_thread_t *worker;
	uint8_t count;
	bool_t reported;
} pfe_hif_ptp_ts_db_t;

errno_t pfe_hif_ptp_ts_db_init(pfe_hif_ptp_ts_db_t *db);
void pfe_hif_ptp_ts_db_fini(pfe_hif_ptp_ts_db_t *db);
errno_t pfe_hif_ptp_ts_db_push_msg(pfe_hif_ptp_ts_db_t *db, bool_t rx,
		uint16_t refnum, uint8_t type, uint16_t port, uint16_t seq_id);
errno_t pfe_hif_ptp_ts_db_push_ts(pfe_hif_ptp_ts_db_t *db,
		uint16_t refnum, uint32_t ts_sec, uint32_t ts_nsec);
errno_t pfe_hif_ptp_ts_db_pop(pfe_hif_ptp_ts_db_t *db,
		uint8_t type, uint16_t port, uint16_t seq_id,
			uint32_t *ts_sec, uint32_t *ts_nsec, bool_t rx);


#endif /* PUBLIC_PFE_HIF_PTP_H_ */
