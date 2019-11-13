/* =========================================================================
 *  Copyright 2017-2019 NXP
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

#ifndef _FCI_H_
#define _FCI_H_

#include "oal.h"
#include "pfe_emac.h" /* pfe_mac_addr_t */
#include "pfe_rtable.h" /* pfe_rtable_t, pfe_rtable_dst_if_t */
#include "pfe_l2br.h" /* pfe_l2br_t */
#include "pfe_class.h" /* pfe_class_t */
#include "pfe_if_db.h"


/**
 * @brief	Information passed into the fci_init() function
 * @note	For future use
 */
typedef struct fci_init_info_tag
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
typedef struct __fci_tag fci_t;

errno_t fci_init(fci_init_info_t *info, const char_t *const identifier);
void fci_fini(void);

#endif /* _FCI_H_ */
