/* =========================================================================
 *  Copyright 2018-2019 NXP
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
 * @defgroup    dxgr_PFE_L2BR_TABLE L2 Bridge Table
 * @brief		The L2 Bridge Table
 * @details     This is the software representation of L2 Bridge Table.
 *
 * @addtogroup  dxgr_PFE_L2BR_TABLE
 * @{
 *
 * @file		pfe_l2br_table.h
 * @brief		The L2 Bridge Table module header file.
 * @details		This file contains L2 Bridge tables-related API.
 *
 */

#ifndef PUBLIC_PFE_L2BR_TABLE_H_
#define PUBLIC_PFE_L2BR_TABLE_H_

#include "pfe_phy_if.h" /* pfe_interface_t */

typedef enum
{
	PFE_L2BR_TABLE_INVALID,
	PFE_L2BR_TABLE_MAC2F,		/*	2-field MAC table (MAC+VLAN) */
	PFE_L2BR_TABLE_VLAN			/*	VLAN table */
} pfe_l2br_table_type_t;

/**
 * @brief	L2 bridge table select criteria type
 */
typedef enum
{
	L2BR_TABLE_CRIT_ALL,			/*!< Match any entry in the table */
	L2BR_TABLE_CRIT_VALID			/*!< Match only valid entries */
} pfe_l2br_table_get_criterion_t;

typedef struct __pfe_l2br_table_tag pfe_l2br_table_t;
typedef struct __pfe_l2br_table_entry_tag pfe_l2br_table_entry_t;

pfe_l2br_table_t *pfe_l2br_table_create(void *cbus_base_va, pfe_l2br_table_type_t type);
void pfe_l2br_table_destroy(pfe_l2br_table_t *l2br);
errno_t pfe_l2br_table_add_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_del_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_update_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_search_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_get_first(pfe_l2br_table_t *l2br, pfe_l2br_table_get_criterion_t crit, pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_get_next(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);

pfe_l2br_table_entry_t *pfe_l2br_table_entry_create(pfe_l2br_table_t *l2br);
errno_t pfe_l2br_table_entry_destroy(pfe_l2br_table_entry_t *entry);
errno_t pfe_l2br_table_entry_set_mac_addr(pfe_l2br_table_entry_t *entry, pfe_mac_addr_t mac_addr);
errno_t pfe_l2br_table_entry_set_vlan(pfe_l2br_table_entry_t *entry, uint16_t vlan);
errno_t pfe_l2br_table_entry_set_action_data(pfe_l2br_table_entry_t *entry, uint64_t action_data);
errno_t pfe_l2br_table_entry_set_fresh(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry, bool_t is_fresh);
bool_t pfe_l2br_table_entry_is_fresh(pfe_l2br_table_entry_t *entry) __attribute__((pure));
errno_t pfe_l2br_table_entry_set_static(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry, bool_t is_static);
bool_t pfe_l2br_table_entry_is_static(pfe_l2br_table_entry_t *entry) __attribute__((pure));
uint32_t pfe_l2br_table_entry_to_str(pfe_l2br_table_entry_t *entry, char_t *buf, uint32_t buf_len);

#endif /* PUBLIC_PFE_L2BR_TABLE_H_ */

/** @}*/
/** @}*/
