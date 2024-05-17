/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef SRC_PFE_PLATFORM_CFG_H_
#define SRC_PFE_PLATFORM_CFG_H_

#define TMU_TYPE_TMU		1U
#define TMU_TYPE_TMU_LITE	2U

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
 * @brief	BMU1 buffer size in Bytes
 */
#define PFE_CFG_BMU1_BUF_SIZE				256U	/* 256 bytes */

/**
 * @brief	BMU2 buffer size in bytes
 * @note	As a H/W requirement, the BMU2 buffer size must not be
 *		changed from the current value of 2 KB.
 */
#define PFE_CFG_BMU2_BUF_SIZE				2048U	/* 2K bytes */

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
#define PFE_CFG_BMU1_LMEM_SIZE				(PFE_CFG_BMU1_BUF_SIZE * PFE_CFG_BMU1_BUF_COUNT)

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
#define PFE_CFG_LMEM_BUF_SIZE_LN2	0x8U /* 256 */
#define PFE_CFG_LMEM_BUF_SIZE		(1UL << PFE_CFG_LMEM_BUF_SIZE_LN2)

/* DDR defines */
#define PFE_CFG_DDR_HDR_SIZE		0x0200U  /* Hardwired in HW */
#define PFE_CFG_DDR_BUF_SIZE_LN2	0xbU /* 2048 */
#define PFE_CFG_DDR_BUF_SIZE		(1UL << PFE_CFG_DDR_BUF_SIZE_LN2)

/* Maximal count of entries within hash area of routing table */
#define PFE_CFG_RT_HASH_ENTRIES_MAX_CNT 1048576U

/* Value read from version register on S32G3 */
#define PFE_S32G3_VERSION 0x00000101U

#endif /* SRC_PFE_PLATFORM_CFG_H_ */
