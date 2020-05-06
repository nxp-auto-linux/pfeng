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
 * @defgroup    dxgr_PFE_HIF_RING HIF BD Ring
 * @brief		The HIF Buffer Descriptor ring
 * @details     This is the software representation of the HIF buffer descriptor ring.
 * 
 * @addtogroup  dxgr_PFE_HIF_RING
 * @{
 * 
 * @file		pfe_hif_ring.h
 * @brief		The HIF BD ring driver header file.
 * @details		
 *
 */

#ifndef PUBLIC_PFE_HIF_RING_H_
#define PUBLIC_PFE_HIF_RING_H_

#if defined(PFE_CFG_HIF_NOCPY_DIRECT)
#include "pfe_ct.h" /* pfe_ct_phy_if_id_t */
#endif /* PFE_CFG_HIF_NOCPY_DIRECT */

typedef struct __pfe_hif_ring_tag pfe_hif_ring_t;

pfe_hif_ring_t *pfe_hif_ring_create(bool_t rx, uint16_t seqnum, bool_t nocpy) __attribute__((cold));
uint32_t pfe_hif_ring_get_len(pfe_hif_ring_t *ring) __attribute__((pure, hot));
errno_t pfe_hif_ring_destroy(pfe_hif_ring_t *ring) __attribute__((cold));
void *pfe_hif_ring_get_base_pa(pfe_hif_ring_t *ring) __attribute__((pure, cold));
void *pfe_hif_ring_get_wb_tbl_pa(pfe_hif_ring_t *ring) __attribute__((pure, cold));
uint32_t pfe_hif_ring_get_wb_tbl_len(pfe_hif_ring_t *ring) __attribute__((pure, cold));
errno_t pfe_hif_ring_enqueue_buf(pfe_hif_ring_t *ring, void *buf_pa, uint32_t length, bool_t lifm) __attribute__((hot));
errno_t pfe_hif_ring_dequeue_buf(pfe_hif_ring_t *ring, void **buf_pa, uint32_t *length, bool_t *lifm) __attribute__((hot));
#ifdef PFE_CFG_HIF_TX_FIFO_FIX
errno_t pfe_hif_ring_dequeue_plain(pfe_hif_ring_t *ring, bool_t *lifm, uint32_t *len) __attribute__((hot));
#else
errno_t pfe_hif_ring_dequeue_plain(pfe_hif_ring_t *ring, bool_t *lifm) __attribute__((hot));
#endif /* PFE_CFG_HIF_TX_FIFO_FIX */
errno_t pfe_hif_ring_drain_buf(pfe_hif_ring_t *ring, void **buf_pa) __attribute__((cold));
bool_t pfe_hif_ring_is_below_wm(pfe_hif_ring_t *ring) __attribute__((pure, hot));
void pfe_hif_ring_lock(pfe_hif_ring_t *ring) __attribute__((hot));
void pfe_hif_ring_unlock(pfe_hif_ring_t *ring) __attribute__((hot));
void pfe_hif_ring_invalidate(pfe_hif_ring_t *ring) __attribute__((cold));
uint32_t pfe_hif_ring_get_fill_level(pfe_hif_ring_t *ring) __attribute__((pure, hot));
uint32_t pfe_hif_ring_dump(pfe_hif_ring_t *ring, char_t *name, char_t *buf, uint32_t size, uint8_t verb_level);
#if defined(PFE_CFG_HIF_NOCPY_DIRECT)
void pfe_hif_ring_set_egress_if(pfe_hif_ring_t *ring, pfe_ct_phy_if_id_t id) __attribute__((hot));
#endif /* PFE_CFG_HIF_NOCPY_DIRECT */

#endif /* PUBLIC_PFE_HIF_RING_H_ */

/** @}*/
/** @}*/
