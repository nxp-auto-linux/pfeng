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
 * @file		pfe_mmap.h
 * @brief		The PFE memory configuration file (LS1012a)
 * @details		This file contains memory distribution policies as required by application.
 * @note		Various variants of this file should exist for various purposes (please
 *				keep this file clean, not containing platform- or application-specific
 *				preprocessor switches).
 *				
 */

#ifndef PFE_MMAP_H_
#define PFE_MMAP_H_

/*	Address re-mapping macros */
#define PFE_MMAP_DDR_PHYS_TO_PFE(p)			(p)
#define PFE_MMAP_DDR_PFE_TO_PHYS(p)			(p)
#define PFE_MMAP_IRAM_PHYS_TO_PFE(p)		(((uint32_t) (p)) | 0x80000000U)
#define PFE_MMAP_IRAM_PFE_TO_PHYS(p)		(((uint32_t) (p)) & 0x7FFFFFFFU)
#define PFE_MMAP_CCSR_PHYS_TO_PFE(p)		(((uint32_t) (p)) | 0x80000000U)
#define PFE_MMAP_CCSR_PFE_TO_PHYS(p)		(((uint32_t) (p)) & 0x7FFFFFFFU)
#define PFE_MMAP_CBUS_PHYS_TO_PFE(p)		(((p) - PFE_CBUS_PHYS_BASE_ADDR) + PFE_CBUS_PHYS_BASE_ADDR_FROM_PFE) /*Translates to PFE address map */

#ifndef SZ_1K
#define SZ_1K 								1024U
#endif

#ifndef SZ_1M
#define SZ_1M 								(1024U * 1024U)
#endif

/* DDR Mapping */
#define PFE_MMAP_ROUTE_TABLE_BASEADDR		0U
#define PFE_MMAP_ROUTE_TABLE_HASH_BITS		15U	/* 32K entries */
#define PFE_MMAP_ROUTE_TABLE_SIZE			((1U << PFE_MMAP_ROUTE_TABLE_HASH_BITS) * CLASS_ROUTE_SIZE)
#define PFE_MMAP_BMU2_DDR_BASEADDR			(PFE_MMAP_ROUTE_TABLE_BASEADDR + PFE_MMAP_ROUTE_TABLE_SIZE)
#define PFE_MMAP_BMU2_BUF_COUNT				(4096U - 256U)			/* This is to get a total DDR size of 12MiB */
#define PFE_MMAP_BMU2_DDR_SIZE				(PFE_CFG_DDR_BUF_SIZE * PFE_MMAP_BMU2_BUF_COUNT)
#define PFE_MMAP_UTIL_CODE_BASEADDR			(PFE_MMAP_BMU2_DDR_BASEADDR + PFE_MMAP_BMU2_DDR_SIZE)
#define PFE_MMAP_UTIL_CODE_SIZE				(128U * SZ_1K)
#define PFE_MMAP_UTIL_DDR_DATA_BASEADDR		(PFE_MMAP_UTIL_CODE_BASEADDR + PFE_MMAP_UTIL_CODE_SIZE)
#define PFE_MMAP_UTIL_DDR_DATA_SIZE			(64U * SZ_1K)
#define PFE_MMAP_CLASS_DDR_DATA_BASEADDR	(PFE_MMAP_UTIL_DDR_DATA_BASEADDR + PFE_MMAP_UTIL_DDR_DATA_SIZE)
#define PFE_MMAP_CLASS_DDR_DATA_SIZE		(32U * SZ_1K)
#define PFE_MMAP_TMU_DDR_DATA_BASEADDR		(PFE_MMAP_CLASS_DDR_DATA_BASEADDR + PFE_MMAP_CLASS_DDR_DATA_SIZE)
#define PFE_MMAP_TMU_DDR_DATA_SIZE			(32U * SZ_1K)
#define PFE_MMAP_TMU_LLM_BASEADDR			(PFE_MMAP_TMU_DDR_DATA_BASEADDR + PFE_MMAP_TMU_DDR_DATA_SIZE)
#define PFE_MMAP_TMU_LLM_QUEUE_LEN			(8U * 512U)			/* Must be power of two and at least 16 * 8 = 128 bytes */
#define PFE_MMAP_TMU_LLM_SIZE				(4U * 16U * TMU_LLM_QUEUE_LEN)	/* (4 TMU's x 16 queues x queue_len) */

#define PFE_MMAP_DDR_MAX_SIZE				(PFE_MMAP_TMU_LLM_BASEADDR + PFE_MMAP_TMU_LLM_SIZE)

/* IRAM Mapping */
#define PFE_MMAP_IPSEC_IRAM_BASEADDR		0U
#define PFE_MMAP_IPSEC_IRAM_SIZE			0x2000U

/* LMEM Mapping */
#define PFE_MMAP_BMU1_LMEM_BASEADDR			0U
#define PFE_MMAP_BMU1_LMEM_SIZE				((1UL << PFE_CFG_BMU1_BUF_SIZE) * PFE_CFG_BMU1_BUF_COUNT)
#define PFE_MMAP_PE_LMEM_BASE				(PFE_MMAP_BMU1_LMEM_BASEADDR + PFE_MMAP_BMU1_LMEM_SIZE) 
#define PFE_MMAP_PE_LMEM_SIZE				(CBUS_LMEM_SIZE - PFE_MMAP_BMU1_LMEM_SIZE)

#define PFE_MMAP_IPSEC_LMEM_BASEADDR		(BMU1_LMEM_BASEADDR + BMU1_LMEM_SIZE)	
#define PFE_MMAP_IPSEC_LMEM_SIZE			(30U * 1024U)

#endif /* PFE_MMAP_H_ */

/** @}*/
