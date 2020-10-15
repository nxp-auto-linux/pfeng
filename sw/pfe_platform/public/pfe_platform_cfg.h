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

#ifndef SRC_PFE_PLATFORM_CFG_H_
#define SRC_PFE_PLATFORM_CFG_H_

#define TMU_TYPE_TMU		1U
#define TMU_TYPE_TMU_LITE	2U

/**
 * @brief	Number of entries of a HIF ring
 * @note	Must be power of 2
 */
#define PFE_HIF_RING_CFG_LENGTH				256U

/*
 * @brief TMU variant
 */
#define PFE_CFG_TMU_VARIANT					TMU_TYPE_TMU_LITE

/*
 * @brief	The PFE HIF IRQ ID as seen by the host
 */
#define PFE_CFG_HIF_IRQ_ID					204

/*
 * @brief	Maximum supported number of channels
 */
#define HIF_CFG_MAX_CHANNELS				4U

/**
 * @brief	Maximum number of logical interfaces
 * @details	This is the maximum number supported by driver. Real
 *			number is limited by amount of available DMEM.
 */
#define PFE_CFG_MAX_LOG_IFS					256U

/**
 * @brief	The CLASS_PE_SYS_CLK_RATIO[csr_clmode]
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
#if defined(PFE_CFG_TARGET_OS_LINUX) && defined(PFE_CFG_TARGET_ARCH_x86_64)
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
#define PFE_CFG_UTIL_ELF_DMEM_BASE			PFE_CFG_CLASS_ELF_DMEM_BASE

/**
 * @brief	Size of DMEM per UTIL PE
 */
#define PFE_CFG_UTIL_DMEM_SIZE				PFE_CFG_CLASS_DMEM_SIZE

/**
 * @brief	IMEM base address as defined by .elf
 */
#define PFE_CFG_UTIL_ELF_IMEM_BASE			PFE_CFG_CLASS_ELF_IMEM_BASE

/**
 * @brief	Size of IMEM per UTIL PE
 */
#define PFE_CFG_UTIL_IMEM_SIZE				PFE_CFG_CLASS_IMEM_SIZE


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
 * @brief	Offset in LMEM where BMU1 buffers area starts
 */
#define PFE_CFG_BMU1_LMEM_BASEADDR			0U

/**
 * @brief	Size of BMU1 buffers area in number of bytes
 */
#define PFE_CFG_BMU1_LMEM_SIZE				((1UL << PFE_CFG_BMU1_BUF_SIZE) * PFE_CFG_BMU1_BUF_COUNT)

/**
 * @brief	Offset in LMEM, where PE memory area starts
 */
#define PFE_CFG_PE_LMEM_BASE				(PFE_CFG_BMU1_LMEM_BASEADDR + PFE_CFG_BMU1_LMEM_SIZE)

/**
 * @brief	Size of PE memory area in number of bytes
 */
#define PFE_CFG_PE_LMEM_SIZE				(CBUS_LMEM_SIZE - PFE_CFG_BMU1_LMEM_SIZE)

/**
 * @brief	Translates from host CPU physical address space to PFE address space
 */
#define PFE_CFG_MEMORY_PHYS_TO_PFE(p)			(p)

/**
 * @brief	Translates from PFE address space to host CPU physical address space
 */
#define PFE_CFG_MEMORY_PFE_TO_PHYS(p)			(p)

/**
 * @brief	Firmware files for particular HW blocks
 */
#define PFE_CFG_CLASS_FIRMWARE_FILENAME		"/tmp/class_s32g.elf"
#define PFE_CFG_TMU_FIRMWARE_FILENAME		"/tmp/tbd.elf"
#define PFE_CFG_UTIL_FIRMWARE_FILENAME		"/tmp/upe_s32g.elf"

/* LMEM defines */
#define PFE_CFG_LMEM_HDR_SIZE		0x0070U
#define PFE_CFG_LMEM_BUF_SIZE_LN2	0x8U /* 256 */
#define PFE_CFG_LMEM_BUF_SIZE		(1U << PFE_CFG_LMEM_BUF_SIZE_LN2)

/* DDR defines */
#define PFE_CFG_DDR_HDR_SIZE		0x0200U 
#define PFE_CFG_DDR_BUF_SIZE_LN2	0xbU /* 2048 */
#define PFE_CFG_DDR_BUF_SIZE		(1U << PFE_CFG_DDR_BUF_SIZE_LN2)

/* RO defines */
#define PFE_CFG_RO_HDR_SIZE			0x0010U

#endif /* SRC_PFE_PLATFORM_CFG_H_ */
