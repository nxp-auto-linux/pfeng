/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_HIF_RING_H_
#define PUBLIC_PFE_HIF_RING_H_

typedef struct pfe_hif_ring_tag pfe_hif_ring_t;

pfe_hif_ring_t *pfe_hif_ring_create(bool_t rx, bool_t nocpy) __attribute__((cold));
uint32_t pfe_hif_ring_get_len(const pfe_hif_ring_t *ring) __attribute__((pure, hot));
errno_t pfe_hif_ring_destroy(pfe_hif_ring_t *ring) __attribute__((cold));
void *pfe_hif_ring_get_base_pa(const pfe_hif_ring_t *ring) __attribute__((pure, cold));
void *pfe_hif_ring_get_wb_tbl_pa(const pfe_hif_ring_t *ring) __attribute__((pure, cold));
uint32_t pfe_hif_ring_get_wb_tbl_len(const pfe_hif_ring_t *ring) __attribute__((pure, cold));
errno_t pfe_hif_ring_enqueue_buf(pfe_hif_ring_t *ring, const void *buf_pa, uint32_t length, bool_t lifm) __attribute__((hot));
errno_t pfe_hif_ring_dequeue_buf(pfe_hif_ring_t *ring, void **buf_pa, uint32_t *length, bool_t *lifm) __attribute__((hot));
errno_t pfe_hif_ring_dequeue_plain(pfe_hif_ring_t *ring, bool_t *lifm) __attribute__((hot));
errno_t pfe_hif_ring_drain_buf(pfe_hif_ring_t *ring, void **buf_pa) __attribute__((cold));
bool_t pfe_hif_ring_is_below_wm(const pfe_hif_ring_t *ring) __attribute__((pure, hot));
void pfe_hif_ring_invalidate(const pfe_hif_ring_t *ring) __attribute__((cold));
uint32_t pfe_hif_ring_get_fill_level(const pfe_hif_ring_t *ring) __attribute__((pure, hot));
bool_t pfe_hif_ring_is_on_head(const pfe_hif_ring_t *ring) __attribute__((pure, hot));

void pfe_hif_ring_dump(pfe_hif_ring_t *ring, char_t *name, void *dev, void (*dev_print)(void *dev, const char *fmt, ...), uint8_t verb_level);

#endif /* PUBLIC_PFE_HIF_RING_H_ */
