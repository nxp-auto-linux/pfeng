/* =========================================================================
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @file		fci_ownership_mask.h
 * @brief		The FCI ownership permission mask
 */

#ifndef PUBLIC_FCI_OWNERSHIP_MASK_H_
#define PUBLIC_FCI_OWNERSHIP_MASK_H_

#include "pfe_ct.h"

/**
 * @brief	The bitmap list of HIF channels that are allowed to take FCI ownership
 */
typedef enum
{
	FCI_OWNER_HIF_INVALID = 0,
	FCI_OWNER_HIF_0 = (1 << 0),
	FCI_OWNER_HIF_1 = (1 << 1),
	FCI_OWNER_HIF_2 = (1 << 2),
	FCI_OWNER_HIF_3 = (1 << 3),
	FCI_OWNER_HIF_NOCPY = (1 << 4)
} pfe_fci_owner_hif_id_t;

static const pfe_fci_owner_hif_id_t pfe_fci_owner_hif_ids[] =
{
	[PFE_PHY_IF_ID_HIF_NOCPY] = FCI_OWNER_HIF_NOCPY,
	[PFE_PHY_IF_ID_HIF0] = FCI_OWNER_HIF_0,
	[PFE_PHY_IF_ID_HIF1] = FCI_OWNER_HIF_1,
	[PFE_PHY_IF_ID_HIF2] = FCI_OWNER_HIF_2,
	[PFE_PHY_IF_ID_HIF3] = FCI_OWNER_HIF_3,
	[PFE_PHY_IF_ID_INVALID] = FCI_OWNER_HIF_INVALID
};

/**
 * @brief		Convert interface id to bitmask value representing HIF channel that is allowed to take FCI ownership
 * @param[in]	phy interface id
 * @return		FCI owner HIF permission bitmask value
 */
static inline pfe_fci_owner_hif_id_t pfe_fci_owner_hif_from_phy_id(pfe_ct_phy_if_id_t phy)
{
	pfe_fci_owner_hif_id_t ret_val = FCI_OWNER_HIF_INVALID;

	if (PFE_PHY_IF_ID_INVALID >= phy)
	{
		ret_val = pfe_fci_owner_hif_ids[phy];
	}

	return ret_val;
}

#endif /* PUBLIC_FCI_OWNERSHIP_MASK_H_ */
