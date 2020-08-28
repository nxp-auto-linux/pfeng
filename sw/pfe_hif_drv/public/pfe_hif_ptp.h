/* =========================================================================
 *  Copyright 2020 NXP
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
