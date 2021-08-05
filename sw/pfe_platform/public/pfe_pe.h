/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_PE_H_
#define PFE_PE_H_

#include "pfe_ct.h"


typedef struct pfe_pe_tag pfe_pe_t;

pfe_pe_t * pfe_pe_create(addr_t cbus_base_va, pfe_ct_pe_type_t type, uint8_t id);
void pfe_pe_set_dmem(pfe_pe_t *pe, addr_t elf_base, addr_t len);
void pfe_pe_set_imem(pfe_pe_t *pe, addr_t elf_base, addr_t len);
void pfe_pe_set_lmem(pfe_pe_t *pe, addr_t elf_base, addr_t len);
void pfe_pe_set_ddr(pfe_pe_t *pe, addr_t base_pa, addr_t base_va, addr_t len);
void pfe_pe_set_iaccess(pfe_pe_t *pe, uint32_t wdata_reg, uint32_t rdata_reg, uint32_t addr_reg);
errno_t pfe_pe_load_firmware(pfe_pe_t *pe, const void *elf);
errno_t pfe_pe_get_mmap(const pfe_pe_t *pe, pfe_ct_pe_mmap_t *mmap);
void pfe_pe_memcpy_from_host_to_dmem_32(pfe_pe_t *pe, addr_t dst_addr, const void *src_ptr, uint32_t len);
void pfe_pe_memcpy_from_dmem_to_host_32(pfe_pe_t *pe, void *dst_ptr, addr_t src_addr, uint32_t len);
errno_t pfe_pe_gather_memcpy_from_dmem_to_host_32(pfe_pe_t **pe, int32_t pe_count, void *dst_ptr, addr_t src_addr, uint32_t buffer_len, uint32_t read_len);
void pfe_pe_memcpy_from_host_to_imem_32(pfe_pe_t *pe, addr_t dst_addr, const void *src_ptr, uint32_t len);
void pfe_pe_memcpy_from_imem_to_host_32(pfe_pe_t *pe, void *dst_ptr, addr_t src_addr, uint32_t len);
void pfe_pe_dmem_memset(pfe_pe_t *pe, uint8_t val, addr_t addr, uint32_t len);
void pfe_pe_imem_memset(pfe_pe_t *pe, uint8_t val, addr_t addr, uint32_t len);
errno_t pfe_pe_get_fw_feature_entry(pfe_pe_t *pe, uint32_t id, pfe_ct_feature_desc_t **entry);
errno_t pfe_pe_get_pe_stats_nolock(pfe_pe_t *pe, uint32_t addr, pfe_ct_pe_stats_t *stats);
errno_t pfe_pe_get_classify_stats_nolock(pfe_pe_t *pe, uint32_t addr, pfe_ct_classify_stats_t *stats);
errno_t pfe_pe_get_class_algo_stats_nolock(pfe_pe_t *pe, uint32_t addr, pfe_ct_class_algo_stats_t *stats);
uint32_t pfe_pe_stat_to_str(const pfe_ct_class_algo_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level);
pfe_ct_pe_sw_state_t pfe_pe_get_fw_state(pfe_pe_t *pe);
uint32_t pfe_pe_get_text_statistics(pfe_pe_t *pe, char_t *buf, uint32_t buf_len, uint8_t verb_level);
void pfe_pe_destroy(pfe_pe_t *pe);
errno_t pfe_pe_check_mmap(const pfe_pe_t *pe);
errno_t pfe_pe_get_fw_errors_nolock(pfe_pe_t *pe);
errno_t pfe_pe_get_data_nolock(pfe_pe_t *pe, pfe_ct_buffer_t *buf);
errno_t pfe_pe_put_data_nolock(pfe_pe_t *pe, pfe_ct_buffer_t *buf);
errno_t pfe_pe_mem_lock(pfe_pe_t *pe);
errno_t pfe_pe_mem_unlock(pfe_pe_t *pe);
errno_t pfe_pe_lock(pfe_pe_t *pe);
errno_t pfe_pe_unlock(pfe_pe_t *pe);
char *pfe_pe_get_fw_feature_str_base(const pfe_pe_t *pe);


#endif /* PFE_PE_H_ */
