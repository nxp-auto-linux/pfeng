/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

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
