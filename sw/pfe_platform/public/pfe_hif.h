/* =========================================================================
 *  Copyright 2018-2020 NXP
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
 * @defgroup    dxgr_PFE_HIF HIF
 * @brief		The Host Interface
 * @details     This is the software representation of the HIF block.
 * 
 * @addtogroup  dxgr_PFE_HIF
 * @{
 * 
 * @file		pfe_hif.h
 * @brief		The HIF module header file.
 * @details		This file contains HIF-related API.
 *
 */

#ifndef PUBLIC_PFE_HIF_H_
#define PUBLIC_PFE_HIF_H_

#include "pfe_hif_ring.h"
#include "pfe_hif_chnl.h"

typedef enum
{
	HIF_CHNL_INVALID = 0,
	HIF_CHNL_0 = (1 << 0),
	HIF_CHNL_1 = (1 << 1),
	HIF_CHNL_2 = (1 << 2),
	HIF_CHNL_3 = (1 << 3)
} pfe_hif_chnl_id_t;

/*	Way to translate physical interface ID to HIF channel ID... */
#include "pfe_ct.h"
static inline pfe_hif_chnl_id_t pfe_hif_chnl_from_phy_id(pfe_ct_phy_if_id_t phy)
{
	if (phy == PFE_PHY_IF_ID_HIF0)
	{
		return HIF_CHNL_0;
	}
	else if (phy == PFE_PHY_IF_ID_HIF1)
	{
		return HIF_CHNL_1;
	}
	else if (phy == PFE_PHY_IF_ID_HIF2)
	{
		return HIF_CHNL_2;
	}
	else if (phy == PFE_PHY_IF_ID_HIF3)
	{
		return HIF_CHNL_3;
	}
	else
	{
		return HIF_CHNL_INVALID;
	}
}

typedef struct __pfe_hif_tag pfe_hif_t;

pfe_hif_t *pfe_hif_create(void *base_va, pfe_hif_chnl_id_t channels);
pfe_hif_chnl_t *pfe_hif_get_channel(pfe_hif_t *hif, pfe_hif_chnl_id_t channel_id);
void pfe_hif_destroy(pfe_hif_t *hif);

#ifdef PFE_CFG_PFE_MASTER
errno_t pfe_hif_isr(pfe_hif_t *hif);
void pfe_hif_irq_mask(pfe_hif_t *hif);
void pfe_hif_irq_unmask(pfe_hif_t *hif);
uint32_t pfe_hif_get_text_statistics(pfe_hif_t *hif, char_t *buf, uint32_t buf_len, uint8_t verb_level);
#endif /* PFE_CFG_PFE_MASTER */

#endif /* PUBLIC_PFE_HIF_H_ */

/** @}*/
/** @}*/
