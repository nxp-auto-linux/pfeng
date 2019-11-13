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
 * @defgroup    dxgr_PFE_CLASS CLASS
 * @brief		The Classifier
 * @details     This is the software representation of the classifier block.
 *
 * @addtogroup  dxgr_PFE_CLASS
 * @{
 *
 * @file		pfe_class.h
 * @brief		The CLASS module header file.
 * @details		This file contains CLASS-related API.
 *
 */

#ifndef PFE_CLASS_H_
#define PFE_CLASS_H_

 #include "pfe_ct.h" /* fw/host shared types */

typedef struct __pfe_classifier_tag pfe_class_t;

typedef struct
{
	bool_t resume;					/*	Resume flag */
	bool_t toe_mode;					/*	TCP offload mode */
	uint32_t pe_sys_clk_ratio;		/*	Clock mode ratio for sys_clk and pe_clk */
	uint32_t pkt_parse_offset;		/*	Offset which says from which point packet needs to be parsed */
	void * route_table_base_pa;		/*	Route table physical address */
	void * route_table_base_va;		/*	Route table virtual address */
	uint32_t route_entry_size;		/*	Route entry size */
	uint32_t route_hash_size;		/*	Route hash size (bits) */
	void * ddr_base_va;				/*	DDR region base address (virtual) */
	void * ddr_base_pa;				/*	DDR region base address (physical) */
	uint32_t ddr_size;			/*	Size of the DDR region */
} pfe_class_cfg_t;

pfe_class_t *pfe_class_create(void *cbus_base_va, uint32_t pe_num, pfe_class_cfg_t *cfg);
void pfe_class_enable(pfe_class_t *class);
void pfe_class_reset(pfe_class_t *class);
void pfe_class_disable(pfe_class_t *class);
errno_t pfe_class_load_firmware(pfe_class_t *class, const void *elf);
errno_t pfe_class_get_mmap(pfe_class_t *class, int32_t pe_idx, pfe_ct_pe_mmap_t *mmap);
errno_t pfe_class_write_dmem(pfe_class_t *class, int32_t pe_idx, void *dst, void *src, uint32_t len);
errno_t pfe_class_read_dmem(pfe_class_t *class, uint32_t pe_idx, void *dst, void *src, uint32_t len);
errno_t pfe_class_read_pmem(pfe_class_t *class, uint32_t pe_idx, void *dst, void *src, uint32_t len);
errno_t pfe_class_set_rtable(pfe_class_t *class, void *rtable_pa, uint32_t rtable_len, uint32_t entry_size);
errno_t pfe_class_set_default_vlan(pfe_class_t *class, uint16_t vlan);
uint32_t pfe_class_get_num_of_pes(pfe_class_t *class);
uint32_t pfe_class_get_text_statistics(pfe_class_t *class, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_class_destroy(pfe_class_t *class);
addr_t pfe_class_dmem_heap_alloc(pfe_class_t *class, uint32_t size);
void pfe_class_dmem_heap_free(pfe_class_t *class, addr_t addr);
errno_t pfe_class_set_flexible_filter(pfe_class_t *class, const uint32_t dmem_addr);

#endif /* PFE_CLASS_H_ */

/** @}*/
/** @}*/
