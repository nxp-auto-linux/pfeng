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

#ifndef PUBLIC_PFE_BMU_H_
#define PUBLIC_PFE_BMU_H_

typedef struct __pfe_bmu_tag pfe_bmu_t;

typedef struct
{
	void *pool_pa;				/*	Buffer pool base (physical, as seen by PFE). Needs to be aligned to buf_cnt * buf_size. */
	void *pool_va;				/*  Buffer pool base (virtual) */
	uint32_t max_buf_cnt;		/*	Maximum number of buffers that can be used */
	uint32_t buf_size;			/*	Buffer size of each of the buffers allocated and freed (size = 2^buf_size) */
	uint32_t bmu_ucast_thres;
	uint32_t bmu_mcast_thres;
	uint32_t int_mem_loc_cnt;
	uint32_t buf_mem_loc_cnt;
} pfe_bmu_cfg_t;

pfe_bmu_t *pfe_bmu_create(void *cbus_base_va, void *bmu_base, pfe_bmu_cfg_t *cfg) __attribute__((cold));
errno_t pfe_bmu_isr(pfe_bmu_t *bmu) __attribute__((cold));
void pfe_bmu_irq_mask(pfe_bmu_t *bmu);
void pfe_bmu_irq_unmask(pfe_bmu_t *bmu);
void pfe_bmu_enable(pfe_bmu_t *bmu) __attribute__((cold));
void pfe_bmu_reset(pfe_bmu_t *bmu) __attribute__((cold));
void pfe_bmu_disable(pfe_bmu_t *bmu) __attribute__((cold));
void *pfe_bmu_alloc_buf(pfe_bmu_t *bmu) __attribute__((hot));
void *pfe_bmu_get_va(pfe_bmu_t *bmu, void *pa) __attribute__((hot, pure));
void *pfe_bmu_get_pa(pfe_bmu_t *bmu, void *va) __attribute__((hot, pure));
uint32_t pfe_bmu_get_buf_size(pfe_bmu_t *bmu) __attribute__((cold, pure));
void pfe_bmu_free_buf(pfe_bmu_t *bmu, void *buffer) __attribute__((hot));
uint32_t pfe_bmu_get_text_statistics(pfe_bmu_t *bmu, char_t *buf, uint32_t buf_len, uint8_t verb_level) __attribute__((cold));
void pfe_bmu_destroy(pfe_bmu_t *bmu) __attribute__((cold));

#endif /* PUBLIC_PFE_BMU_H_ */
