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
 * @file		pfe_platform_cfg.h
 * @brief		The PFE platform configuration file (LS1012a) -----BUT NEED TO BE COMPLETED AND SET TO S32G-----
 * @details		This file contains HW platform-specific configuration options which are usually
 * 				bounded to a given hardware implementation.
 * @note		Various variants of this file should exist for various HW platforms (please
 *				keep this file clean, not containing platform-specific preprocessor switches).
 *
 */

#ifndef SRC_PFE_PLATFORM_CFG_H_
#define SRC_PFE_PLATFORM_CFG_H_

#define TMU_TYPE_TMU		1U
#define TMU_TYPE_TMU_LITE	2U

/**
 * @brief    Number of entries of a HIF ring
 * @note    Must be power of 2
 */
#define PFE_HIF_RING_CFG_LENGTH             256U

/*
 * @brief TMU variant
 */
#define PFE_CFG_TMU_VARIANT	                TMU_TYPE_TMU_LITE

/*
 * @brief 	Accessible memory space base (PA)
 * @details	This is PFE-accessible address space base (npu2ddr+upe2hdbus AXI masters)
 * @warning	Address range given by this base and PFE_CFG_DDR_MASTER_LEN must be
 *			reserved to be exclusively accessible by PFE.
 */
#define PFE_CFG_DDR_MASTER_ADDR				0x00400000ULL	/* S32G */

/*
 * @brief	Length of the PFE DDR memory
 */
#define PFE_CFG_DDR_MASTER_LEN				0x01000000ULL	/* S32G: 16MB */

/*
 * @brief 	The PFE HIF IRQ ID as seen by the host
 */
#define PFE_CFG_HIF_IRQ_ID					204	/* HIF (copy) IRQ */

/*
 * @brief	Maximum supported number of channels
 */
#define HIF_CFG_MAX_CHANNELS				4U

/**
 * @brief	Maximum number of logical interfaces
 */
#define PFE_CFG_MAX_LOG_IFS					16U

/**
 * @brief	The CLASS_PE_SYS_CLK_RATIO[csr_clmode]
 * @details	See the IMG-NPU Technical Reference Manual
 */
#define PFE_CFG_CLMODE						1U	/* SYS/AXI = 250MHz, HFE = 500MHz */

/**
 * @brief	Maximum number of buffers - BMU1
 */
#define	PFE_CFG_BMU1_BUF_COUNT				0x200U

/**
 * @brief	BMU1 buffer size
 * @details	Value = log2(size)
 */
#define PFE_CFG_BMU1_BUF_SIZE				0x8U	/* 256 bytes */

/**
 * @brief	Maximum number of buffers - BMU2
 */
#if defined(TARGET_OS_LINUX) && defined(TARGET_ARCH_x86_64)
/* Linux x86 has issue with big memory buffers
*/
#define	PFE_CFG_BMU2_BUF_COUNT				0x200U
#else
#define	PFE_CFG_BMU2_BUF_COUNT				0x400U
#endif

/**
 * @brief	BMU2 buffer size
 * @details	Value = log2(size)
 */
#define PFE_CFG_BMU2_BUF_SIZE				0xbU	/* 2048 bytes */

/**
 * @brief	DMEM base address as defined by .elf
 */
#define PFE_CFG_CLASS_ELF_DMEM_BASE			0x20000000UL

/**
 * @brief	Size of DMEM per CLASS PE
 */
#define PFE_CFG_CLASS_DMEM_SIZE				0x00004000UL	/* 16k */

/**
 * @brief	IMEM base address as defined by .elf
 */
#define PFE_CFG_CLASS_ELF_IMEM_BASE			0x9fc00000UL

/**
 * @brief	Size of IMEM per CLASS PE
 */
#define PFE_CFG_CLASS_IMEM_SIZE				0x00008000UL	/* 32kB */

/**
 * @brief	DMEM base address as defined by .elf
 */
#define PFE_CFG_TMU_ELF_DMEM_BASE			0x00000000UL

/**
 * @brief	Size of DMEM per TMU PE
 */
#define PFE_CFG_TMU_DMEM_SIZE				0x00000800UL	/* 2kB */

/**
 * @brief	IMEM base address as defined by .elf
 */
#define PFE_CFG_TMU_ELF_IMEM_BASE			0x00010000UL

/**
 * @brief	Size of IMEM per TMU PE
 */
#define PFE_CFG_TMU_IMEM_SIZE				0x00002000UL	/* 8kB */

/**
 * @brief	DMEM base address as defined by .elf
 */
#define PFE_CFG_UTIL_ELF_DMEM_BASE			0x00000000UL

/**
 * @brief	Size of DMEM per UTIL PE
 */
#define PFE_CFG_UTIL_DMEM_SIZE				0x00002000UL

/**
 * @brief	Physical CBUS base address as seen by PFE
 */
#define PFE_CFG_CBUS_PHYS_BASE_ADDR			0xc0000000U

/**
 * @brief	Physical CBUS base address as seen by CPUs
 */
#define PFE_CFG_CBUS_PHYS_BASE_ADDR_CPU		0x46000000U

/**
 * @brief	CBUS length
 */
#define PFE_CFG_CBUS_LENGTH					0x01000000U

/**
 * @brief	Local physical interface identifier
 * @details	In multi-instance environment, where multiple platform drivers
 * 			can be deployed, this identifier represents the physical interface
 *			(usually HIF channel) associated with the current driver instance.
 */
#ifdef GLOBAL_CFG_PFE_MASTER
#define PFE_CFG_LOCAL_PHY_IF_ID				PFE_PHY_IF_ID_HIF0
#endif /* GLOBAL_CFG_PFE_MASTER */

#ifdef GLOBAL_CFG_PFE_SLAVE
#define PFE_CFG_LOCAL_PHY_IF_ID				PFE_PHY_IF_ID_HIF1
#endif /* GLOBAL_CFG_PFE_SLAVE */


/**
 * @brief	Firmware files for particular HW blocks
 */
#define PFE_CFG_CLASS_FIRMWARE_FILENAME		"/tmp/class_s32g.elf"
#define PFE_CFG_TMU_FIRMWARE_FILENAME		"/tmp/tbd.elf"
#define PFE_CFG_UTIL_FIRMWARE_FILENAME		"/tmp/tbd.elf"

/* LMEM defines */
#define PFE_CFG_LMEM_HDR_SIZE		0x0030U
#define PFE_CFG_LMEM_BUF_SIZE_LN2	0x8U
#define PFE_CFG_LMEM_BUF_SIZE		(1U << PFE_CFG_LMEM_BUF_SIZE_LN2)

/* DDR defines */
#define PFE_CFG_DDR_HDR_SIZE		0x0200U
#define PFE_CFG_DDR_BUF_SIZE_LN2	0xbU
#define PFE_CFG_DDR_BUF_SIZE		(1U << PFE_CFG_DDR_BUF_SIZE_LN2)

#endif /* SRC_PFE_PLATFORM_CFG_H_ */

/** @}*/
