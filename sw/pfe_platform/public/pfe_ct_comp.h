/* =========================================================================
 *  
 *  Copyright 2022-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef HW_S32G_PFE_CT_COMP_H_
#define HW_S32G_PFE_CT_COMP_H_
#include "pfe_ct.h"

typedef struct __attribute__((packed,aligned(4)))
{
	const char name[16];
	PFE_PTR(uint8_t) const data;
	const uint8_t size;
	const uint8_t multiplicity;
	const uint8_t reserved[2];
} pfe_ct_feature_tbl_entry_t ;

typedef struct __attribute__((packed,aligned(4)))
{
	pfe_ct_feature_desc_t feature;
	PFE_PTR(const pfe_ct_feature_tbl_entry_t) cfg;
	PFE_PTR(const pfe_ct_feature_tbl_entry_t) stats;
}pfe_ct_feature_desc_ext_t;

ct_assert(sizeof(pfe_ct_feature_desc_ext_t) == 24);

typedef struct __attribute__((packed, aligned(4)))
{
	uint16_t vlan;                   /* VLAN value if applicable */
	pfe_ct_vlan_table_result_t entry; /* Entry value - port map and other */
	uint8_t field_valids;            /* see pfe_mac2f_table_entry_valid_bits_t */
	uint8_t flags;                   /* see pfe_mac2f_table_entry_flags_t */
	uint16_t col_ptr;		 /* Collision entry pointer */
} l2br_vlan_hash_entry_t;

ct_assert(sizeof(l2br_vlan_hash_entry_t) == 16);

#endif /* HW_S32G_PFE_CT_COMP_H_ */
