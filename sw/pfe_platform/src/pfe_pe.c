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
 * @addtogroup  dxgr_PFE_PE
 * @{
 *
 * @file		pfe_pe.c
 * @brief		The PE module source file.
 * @details		This file contains PE-related functionality:
 * 				- Firmware loading
 * 				- Internal memories access
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "oal.h"
#include "hal.h"

#include "elf_cfg.h"
#include "elf.h"
#include "pfe_cbus.h"
#include "pfe_pe.h"
#include "pfe_mmap.h"

#define BYTES_TO_4B_ALIGNMENT(x)	(4U - ((x) & 0x3U))

/*	Processing Engine representation */
struct __pfe_pe_tag
{
	pfe_pe_type_t type;					/* PE type */
	void *cbus_base_va;					/* CBUS base (virtual) */
	uint8_t id;							/* PE HW ID (0..N) */

	/*	DMEM */
	addr_t dmem_elf_base_va;			/* PE's DMEM base address (virtual, as seen by PE) */
	addr_t dmem_size;					/* PE's DMEM region length */

	/*	IMEM */
	addr_t imem_elf_base_va;			/* PE's IMEM base address (virtual, as seen by PE) */
	addr_t imem_size;					/* PE's IMEM size */

	/*	LMEM */
	addr_t lmem_base_addr_pa;			/* PE's LMEM base address (physical, as seen by PE) */
	addr_t lmem_size;					/* PE's LMEM size */

	/*	DDR */
	void *ddr_base_addr_pa;				/* PE's DDR base address (physical, as seen by host) */
	void *ddr_base_addr_va;				/* PE's DDR base address (virtual) */
	addr_t ddr_size;					/* PE's DDR size */

	/*	Indirect Access */
	void *mem_access_wdata;				/* PE's _MEM_ACCESS_WDATA register address (virtual) */
	void *mem_access_addr;				/* PE's _MEM_ACCESS_ADDR register address (virtual) */
	void *mem_access_rdata;				/* PE's _MEM_ACCESS_RDATA register address (virtual) */

	/*	Debug */
	bool_t dmem_pestatus_available;		/* If TRUE then the 'dmem_pestatus_pa' is valid */
	uint32_t dmem_pestatus_pa;			/* Address within PE's DMEM where runtime status data is located (physical) */

    /* FW Errors*/
    uint32_t error_record_addr;         /* Error record storage address in DMEM */
    uint32_t last_error_write_index;    /* Last seen value of write index in the record */
    void *fw_err_section;               /* Error descriptions elf section storage */

	/*	MMap */
	bool_t mmap_data_available;			/* If TRUE then the 'pfe_pe_mmap_ptr' is valid */
	uint32_t mmap_data_pa;				/* Address within PE's memory where memory map data is located (physical) */
};

typedef enum
{
	PFE_PE_DMEM,
	PFE_PE_IMEM
} pfe_pe_mem_t;

/**
 * @brief		Write to CLASS internal bus peripherals (ccu, pe-lem) from the host
 * 				through indirect access registers.
 * @param[in]	pe The PE instance
 * @param[in]	val	value to write
 * @param[in]	addr Address to write to (must be aligned on size)
 * @param[in]	size Number of bytes to write (1, 2 or 4)
 */
static void pfe_pe_class_bus_write(pfe_pe_t *pe, uint32_t val, uint32_t addr, uint8_t size)
{
	uint32_t offset = addr & 0x3U;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	hal_write32((addr & CLASS_BUS_ACCESS_BASE_MASK), pe->cbus_base_va + CLASS_BUS_ACCESS_BASE);

	addr = (addr & ~CLASS_BUS_ACCESS_BASE_MASK) | (size << 24);

	hal_write32((val << (offset << 3)), pe->cbus_base_va + CLASS_BUS_ACCESS_WDATA);
	hal_write32(addr, pe->cbus_base_va + CLASS_BUS_ACCESS_ADDR);
}

/**
 * @brief		Read data from PE memory
 * @details		Reads PE internal memory from the host through indirect access registers.
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory to access
 * @param[in]	addr Read address (physical within PE memory space, for better performance should be aligned to 32-bits)
 * @param[in]	size Number of bytes to read (maximum 4)
 * @return		The data read (in PE endianess, i.e BE).
 */
static uint32_t pfe_pe_mem_read(pfe_pe_t *pe, pfe_pe_mem_t mem, addr_t addr, uint8_t size)
{
	uint32_t val;
	uint32_t mask;
	uint32_t memsel;
	uint8_t offset;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (unlikely(addr & 0x3U))
	{
		if (size > BYTES_TO_4B_ALIGNMENT(addr))
		{
			/*	Here we need to split the read into two reads */

			/*	Read 1 (LS bytes). Recursive call. Limited to single recursion. */
			val = pfe_pe_mem_read(pe, mem, addr, BYTES_TO_4B_ALIGNMENT(addr));
			offset = 4U - (addr & 0x3U);
			size -= offset;
			addr += offset;

			/*	Read 2 (MS bytes). Recursive call. Limited to single recursion. */
			val |= (pfe_pe_mem_read(pe, mem, addr, size) << (8U * offset));

			return val;
		}
	}

	if (size != 4U)
	{
		mask = (1U << (size * 8U)) - 1U;
	}
	else
	{
		mask = 0xffffffffU;
	}

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	addr = (addr & 0xfffffU)				/* Address (low 20bits) */
				| PE_IBUS_READ				/* Direction (r/w) */
				| memsel					/* Memory selector */
				| PE_IBUS_PE_ID(pe->id)		/* PE instance */
				| PE_IBUS_WREN(0U);			/* Byte(s) selector, unused for read operations */

	/*	Indirect access interface is byte swapping data being read */
	hal_write32((uint32_t)addr, pe->mem_access_addr);
	val = oal_ntohl(hal_read32(pe->mem_access_rdata));

	if (unlikely(addr & 0x3U))
	{
		/*	Move the value to the desired address offset */
		val = (val >> (8U * (addr & 0x3U)));
	}

	return (val & mask);
}

/**
 * @brief		Write data into PE memory
 * @details		Writes PE internal memory from the host through indirect access registers.
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory to access
 * @param[in]	addr Write address (physical within PE memory space, for better performance should be aligned to 32-bits)
 * @param[in]	val Value to write (in PE endianess, i.e BE)
 * @param[in]	size Number of bytes to write (maximum 4)
 */
static void pfe_pe_mem_write(pfe_pe_t *pe, pfe_pe_mem_t mem, uint32_t val, addr_t addr, uint8_t size)
{
	/*	The bytesel is 4-bit value representing bytes which shall be written
		to addressed 4-byte word. It's like 'write enable' for particular bytes.

		Configuration such:
							---+----+----+----+----+---
			WRITE DATA:		   | B3 | B2 | B1 | B0 |
							---+----+----+----+----+---
			SELECTOR:		     0    1    1    0

		writes to PE memory only bytes marked with 1s:

			ADDRESS: ----------+
							   |
							---+----+----+----+----+---
			PE MEM DATA:	   | x  | B2 | B1 | x  |
							---+----+----+----+----+---

		where 'x' means no change in destination memory. See below for usage.
	*/
	uint8_t bytesel = 0U;
	uint32_t memsel;
	uint32_t offset;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (unlikely(addr & 0x3U))
	{
		offset = BYTES_TO_4B_ALIGNMENT(addr);

		if (size <= offset)
		{
			/*	Move the value to the desired address offset */
			val = val << (8U * (addr & 0x3U));
			/*	Enable writes of depicted bytes */
			bytesel = (((1U << size) - 1U) << (offset - size));
		}
		else
		{
			/*	Here we need to split the write into two writes */

			/*	Write 1 (LS bytes). Recursive call. Limited to single recursion. */
			pfe_pe_mem_write(pe, mem, val, addr, offset);
			val >>= 8U * offset;
			size -= offset;
			addr += offset;

			/*	Write 2 (MS bytes). Recursive call. Limited to single recursion. */
			pfe_pe_mem_write(pe, mem, val, addr, size);

			return;
		}
	}
	else
	{
		/*	Destination is aligned */
		bytesel = PE_IBUS_BYTES(size);
	}

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	addr = (addr & 0xfffffU)			/* Address (low 20bits) */
			| PE_IBUS_WRITE				/* Direction (r/w) */
			| memsel					/* Memory selector */
			| PE_IBUS_PE_ID(pe->id)		/* PE instance */
			| PE_IBUS_WREN(bytesel);	/* Byte(s) selector */

	/*	Indirect access interface is byte swapping data being written */
	hal_write32(oal_htonl(val), pe->mem_access_wdata);
	hal_write32((uint32_t)addr, pe->mem_access_addr);
}

/**
 * @brief		Memset for PE memories
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory to access
 * @param[in]	val Byte value to be used to fill the memory block
 * @param[in]	addr Address of the memory block within DMEM
 * @param[in]	len Number of bytes to fill
 */
static void pfe_pe_mem_memset(pfe_pe_t *pe, pfe_pe_mem_t mem, uint8_t val, addr_t addr, uint32_t len)
{
	uint32_t val32 = val | (val << 8) | (val << 16) | (val << 24);
	uint32_t offset;

	if (addr & 0x3U)
	{
		/*	Write unaligned bytes to align the address */
		offset = BYTES_TO_4B_ALIGNMENT(addr);
		offset = (len < offset) ? len : offset;
		pfe_pe_mem_write(pe, mem, val32, addr, offset);
		len = (len >= offset) ? (len - offset) : 0U;
		addr += offset;
	}

	for ( ; len>=4U; len-=4U, addr+=4U)
	{
		/*	Write aligned words */
		pfe_pe_mem_write(pe, mem, val32, addr, 4U);
	}

	if (len > 0U)
	{
		/*	Write the rest */
		pfe_pe_mem_write(pe, mem, val32, addr, len);
	}
}

/**
 * @brief		Memset for DMEM
 * @param[in]	pe The PE instance
 * @param[in]	val Byte value to be used to fill the memory block
 * @param[in]	addr Address of the memory block within DMEM
 * @param[in]	len Number of bytes to fill
 */
void pfe_pe_dmem_memset(pfe_pe_t *pe, uint8_t val, addr_t addr, uint32_t len)
{
	pfe_pe_mem_memset(pe, PFE_PE_DMEM, val, addr, len);
}

/**
 * @brief		Memset for DMEM
 * @param[in]	pe The PE instance
 * @param[in]	val Byte value to be used to fill the memory block
 * @param[in]	addr Address of the memory block within DMEM
 * @param[in]	len Number of bytes to fill
 */
void pfe_pe_imem_memset(pfe_pe_t *pe, uint8_t val, addr_t addr, uint32_t len)
{
	pfe_pe_mem_memset(pe, PFE_PE_IMEM, val, addr, len);
}

/**
 * @brief		Write 'len' bytes to DMEM
 * @details		Writes a buffer to PE internal data memory (DMEM) from the host
 * 				through indirect access registers.
 * @note		Function expects the source data to be in host endian format.
 * @param[in]	pe The PE instance
 * @param[in]	src Buffer source address (virtual)
 * @param[in]	dst DMEM destination address (physical within PE, must be 32bit aligned)
 * @param[in]	len Number of bytes to read
 *
 */
void pfe_pe_memcpy_from_host_to_dmem_32(pfe_pe_t *pe, addr_t dst, const void *src, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	const uint8_t *src_byteptr = src;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (dst & 0x3U)
	{
		/*	Write unaligned bytes to align the destination address */
		offset = BYTES_TO_4B_ALIGNMENT(dst);
		offset = (len < offset) ? len : offset;
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, dst, offset);
		src_byteptr += offset;
		dst += offset;
		len = (len >= offset) ? (len - offset) : 0U;
	}

	for ( ; len>=4U; len-=4U, src_byteptr+=4U, dst+=4U)
	{
		/*	4-byte writes */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, (uint32_t)dst, 4U);
	}

	if (0U != len)
	{
		/*	The rest */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, (uint32_t)dst, len);
	}
}

/**
 * @brief		Read 'len' bytes from DMEM
 * @details		Reads PE internal data memory (DMEM) into a host memory through indirect
 *				access registers.
 * @param[in]	pe The PE instance
 * @param[in]	src DMEM source address (physical within PE, must be 32bit aligned)
 * @param[in]	dst Destination address (virtual)
 * @param[in]	len Number of bytes to read
 *
 */
void pfe_pe_memcpy_from_dmem_to_host_32(pfe_pe_t *pe, void *dst, addr_t src, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	uint8_t *dst_byteptr = dst;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (src & 0x3U)
	{
		/*	Read unaligned bytes to align the source address */
		offset = BYTES_TO_4B_ALIGNMENT(src);
		offset = (len < offset) ? len : offset;
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src, offset);
		memcpy(dst_byteptr, &val, offset);
		dst_byteptr += offset;
		src += offset;
		len = (len >= offset) ? (len - offset) : 0U;
	}

	for ( ; len>=4U; len-=4U, src+=4U, dst_byteptr+=4U)
	{
		/*	4-byte reads */
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src, 4U);
		*((uint32_t *)dst_byteptr) = val;
	}

	if (0U != len)
	{
		/*	The rest */
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src, len);
		memcpy(dst_byteptr, &val, len);
	}
}

/**
 * @brief		Write 'len'bytes to IMEM
 * @details		Writes a buffer to PE internal instruction memory (IMEM) from the host
 *				through indirect access registers.
 * @note		Function expects the source data to be in host endian format.
 * @param[in]	pe The PE instance
 * @param[in]	src Buffer source address (host, virtual)
 * @param[in]	dst IMEM destination address (physical within PE, must be 32bit aligned)
 * @param[in]	len Number of bytes to copy
 */
static void pfe_pe_memcpy_from_host_to_imem_32(pfe_pe_t *pe, addr_t dst, const void *src, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	const uint8_t *src_byteptr = src;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (dst & 0x3U)
	{
		/*	Write unaligned bytes to align the destination address */
		offset = BYTES_TO_4B_ALIGNMENT(dst);
		offset = (len < offset) ? len : offset;
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_IMEM, val, dst, offset);
		src_byteptr += offset;
		dst += offset;
		len = (len >= offset) ? (len - offset) : 0U;
	}

	for ( ; len>=4U; len-=4U, src_byteptr+=4U, dst+=4U)
	{
		/*	4-byte writes */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_IMEM, val, (uint32_t)dst, 4U);
	}

	if (0U != len)
	{
		/*	The rest */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_IMEM, val, (uint32_t)dst, len);
	}
}

/**
 * @brief		Read 'len' bytes from IMEM
 * @details		Reads PE internal instruction memory (IMEM) into a host memory through indirect
 *				access registers.
 * @param[in]	pe The PE instance
 * @param[in]	src IMEM source address (physical within PE, must be 32bit aligned)
 * @param[in]	dst Destination address (host, virtual)
 * @param[in]	len Number of bytes to read
 *
 */
void pfe_pe_memcpy_from_imem_to_host_32(pfe_pe_t *pe, void *dst, addr_t src, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	uint8_t *dst_byteptr = dst;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (src & 0x3U)
	{
		/*	Read unaligned bytes to align the source address */
		offset = BYTES_TO_4B_ALIGNMENT(src);
		offset = (len < offset) ? len : offset;
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src, offset);
		memcpy(dst_byteptr, &val, offset);
		dst_byteptr += offset;
		src += offset;
		len = (len >= offset) ? (len - offset) : 0U;
	}

	for ( ; len>=4U; len-=4U, src+=4U, dst_byteptr+=4U)
	{
		/*	4-byte reads */
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src, 4U);
		*((uint32_t *)dst_byteptr) = val;
	}

	if (0U != len)
	{
		/*	The rest */
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src, len);
		memcpy(dst_byteptr, &val, len);
	}
}

/**
 * @brief		Write data to the cluster memory (LMEM)
 * @param[in]	pe The PE instance
 * @param[in]	dst PE LMEM destination address (must be 32bit aligned)
 * @param[in]	src Buffer source address
 * @param[in]	len Number of bytes to copy
 */
static void pfe_pe_memcpy_from_host_to_lmem_32(pfe_pe_t *pe, addr_t dst, const void *src, uint32_t len)
{
	uint32_t len32 = len >> 2;
	uint32_t i;
	/* Avoid void pointer arithmetics */
	const uint8_t *src_byteptr = src;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == src_byteptr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	for (i = 0U; i < len32; i++, src_byteptr += 4U, dst += 4U)
	{
		pfe_pe_class_bus_write(pe, *(uint32_t *)src_byteptr, dst, 4U);
	}

	if (len & 0x2U)
	{
		pfe_pe_class_bus_write(pe, *(uint16_t *)src_byteptr, dst, 2U);
		src_byteptr += 2U;
		dst += 2U;
	}

	if (len & 0x1U)
	{
		pfe_pe_class_bus_write(pe, *(uint8_t *)src_byteptr, dst, 1U);
		src_byteptr++;
		dst++;
	}
}

/**
 * @brief		Write value to the cluster memory (PE_LMEM)
 * @param[in]	pe The PE instance
 * @param[in]	dst PE LMEM destination address (must be 32bit aligned)
 * @param[in]	val Value to write
 * @param[in]	len Number of bytes to write
 */
static void pfe_pe_memset_lmem(pfe_pe_t *pe, addr_t dst, uint32_t val, uint32_t len)
{
	uint32_t len32 = len >> 2;
	uint32_t i;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	val = val | (val << 8) | (val << 16) | (val << 24);

	for (i = 0U; i < len32; i++, dst += 4U)
	{
		pfe_pe_class_bus_write(pe, val, dst, 4U);
	}

	if (len & 0x2U)
	{
		pfe_pe_class_bus_write(pe, val, dst, 2U);
		dst += 2U;
	}

	if (len & 0x1U)
	{
		pfe_pe_class_bus_write(pe, val, dst, 1U);
		dst++;
	}
}

/**
 * @brief		Load an elf section into DMEM
 * @details		Size and load address need to be at least 32-bit aligned
 * @param[in]	pe The PE instance
 * @param[in]	sdata Pointer to the elf section data
 * @param[in]	shdr Pointer to the elf section header
 * @retval		EOK Success
 * @retval		EINVAL Unsupported section type or wrong input address alignment
 */
static errno_t pfe_pe_load_dmem_section(pfe_pe_t *pe, void *sdata, Elf32_Shdr *shdr)
{
	addr_t addr = shdr->sh_addr;
	addr_t size = shdr->sh_size;
	uint32_t type = shdr->sh_type;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == sdata) || (NULL == shdr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (((addr_t)(sdata) & 0x3U) != (addr & 0x3U))
	{
		NXP_LOG_ERROR("Load address 0x%p and elf file address 0x%p don't have the same alignment\n", (void *)addr, sdata);
		return EINVAL;
	}

	if (addr & 0x3U)
	{
		NXP_LOG_ERROR("Load address 0x%p is not 32bit aligned\n", (void *)addr);
		return EINVAL;
	}

	switch (type)
	{
		case 0x7000002aU: /* MIPS.abiflags */
		{
			/* Skip the section */
			break;
		}
		case SHT_PROGBITS:
		{
#if defined(FW_WRITE_CHECK_EN)
			void *buf = oal_mm_malloc(size);
#endif /* FW_WRITE_CHECK_EN */

			/*	Write section data to DMEM. Convert destination address from .elf to DMEM base. */
			pfe_pe_memcpy_from_host_to_dmem_32(pe, addr - pe->dmem_elf_base_va, sdata, size);

#if defined(FW_WRITE_CHECK_EN)
			pfe_pe_memcpy_from_dmem_to_host_32(pe, buf, addr, size);

			if (0 != memcmp(buf, sdata, size))
			{
				NXP_LOG_ERROR("DMEM data inconsistent\n");
			}

			oal_mm_free(buf);
#endif /* FW_WRITE_CHECK_EN */

			break;
		}

		case SHT_NOBITS:
		{
			pfe_pe_dmem_memset(pe, 0U, addr, size);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unsupported section type: 0x%x\n", type);
			return EINVAL;
		}
	}

	return EOK;
}

/**
 * @brief		Load an elf section into IMEM
 * @details		Code needs to be at least 16bit aligned and only PROGBITS sections are supported
 * @param[in]	pe The PE instance
 * @param[in]	data Pointer to the elf section data
 * @param[in]	shdr Pointer to the elf section header
 * @retval		EOK Success
 * @retval		EFAULT Wrong input address alignment
 * @retval		EINVAL Unsupported section type
 */
static errno_t pfe_pe_load_imem_section(pfe_pe_t *pe, const void *data, Elf32_Shdr *shdr)
{
	addr_t addr = shdr->sh_addr;
	addr_t size = shdr->sh_size;
	uint32_t type = shdr->sh_type;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == data) || (NULL == shdr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Check alignment first */
	if (((addr_t)(data) & 0x3U) != (addr & 0x1U))
	{
		NXP_LOG_ERROR("Load address 0x%p and elf file address 0x%p) don't have the same alignment\n", (void *)addr, data);
		return EFAULT;
	}

	if (addr & 0x1U)
	{
		NXP_LOG_ERROR("Load address 0x%p is not 16bit aligned\n", (void *)addr);
		return EFAULT;
	}

	if (size & 0x1U)
	{
		NXP_LOG_ERROR("Load size 0x%p is not 16bit aligned\n", (void *)size);
		return EFAULT;
	}

	switch (type)
	{
		case 0x7000002aU: /* MIPS.abiflags */
		{
			/* Skip the section */
			break;
		}
		case SHT_PROGBITS:
		{
#if defined(FW_WRITE_CHECK_EN)
			void *buf = oal_mm_malloc(size);
#endif /* FW_WRITE_CHECK_EN */

			/*	Write section data to IMEM. Convert destination address from .elf to IMEM base. */
			pfe_pe_memcpy_from_host_to_imem_32(pe, addr - pe->imem_elf_base_va, data, size);

#if defined(FW_WRITE_CHECK_EN)
			pfe_pe_memcpy_from_imem_to_host_32(pe, buf, addr, size);

			if (0 != memcmp(buf, data, size))
			{
				NXP_LOG_ERROR("IMEM data inconsistent\n");
			}

			oal_mm_free(buf);
			buf = NULL;
#endif /* FW_WRITE_CHECK_EN */

			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unsupported section type: 0x%x\n", type);
			return EINVAL;
		}
	}

	return EOK;
}

/**
 * @brief		Load an elf section into PE LMEM
 * @details		Data needs to be at least 32bit aligned, NOBITS sections are correctly initialized to 0
 * @param[in] 	pe The PE instance
 * @param[in]	data pointer to the elf section data
 * @param[in]	shdr pointer to the elf section header
 * @retval		EOK Success
 * @retval		EFAULT Wrong input address alignment
 * @retval		EINVAL Unsupported section type
 */
static errno_t pfe_pe_load_lmem_section(pfe_pe_t *pe, const void *data, Elf32_Shdr *shdr)
{
	addr_t addr = shdr->sh_addr;
	addr_t size = shdr->sh_size;
	uint32_t type = shdr->sh_type;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == data) || (NULL == shdr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Check the alignment */
	if (((addr_t)(data) & 0x3U) != (addr & 0x3U))
	{
		NXP_LOG_ERROR("Load address 0x%p and elf file address 0x%p don't have the same alignment\n", (void *)addr, data);
		return EFAULT;
	}

	if (addr & 0x3U)
	{
		NXP_LOG_ERROR("Load address 0x%p is not 32bit aligned\n", (void *)addr);
		return EFAULT;
	}

	switch (type)
	{
		case SHT_PROGBITS:
		{
			pfe_pe_memcpy_from_host_to_lmem_32(pe, addr, data, size);
			break;
		}

		case SHT_NOBITS:
		{
			pfe_pe_memset_lmem(pe, addr, 0, size);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unsupported section type: 0x%x\n", type);
			return EINVAL;
		}
	}

	return EOK;
}

/**
 * @brief		Check if memory region belongs to PE's DMEM
 * @param[in]	pe The PE instance
 * @param[in]	addr Address (as seen by PE) to be checked
 * @param[in]	size Length of the region to be checked
 * @return		TRUE if given range belongs to PEs DMEM
 */
static bool_t pfe_pe_is_dmem(pfe_pe_t *pe, addr_t addr, uint32_t size)
{
	addr_t reg_end;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	reg_end = pe->dmem_elf_base_va + pe->dmem_size;

	if ((addr >= pe->dmem_elf_base_va) && ((addr + size) < reg_end))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Check if memory region belongs to PE's IMEM
 * @param[in]	pe The PE instance
 * @param[in]	addr Address (as seen by PE) to be checked
 * @param[in]	size Length of the region to be checked
 * @return		TRUE if given range belongs to PEs IMEM
 */
static bool_t pfe_pe_is_imem(pfe_pe_t *pe, addr_t addr, uint32_t size)
{
	addr_t reg_end;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	reg_end = pe->imem_elf_base_va + pe->imem_size;

	if ((addr >= pe->imem_elf_base_va) && ((addr + size) < reg_end))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Check if memory region belongs to PE's LMEM
 * @param[in]	pe The PE instance
 * @param[in]	addr Address (as seen by PE) to be checked
 * @param[in]	size Length of the region to be checked
 * @return		TRUE if given range belongs to PEs LMEM
 */
static bool_t pfe_pe_is_lmem(pfe_pe_t *pe, addr_t addr, uint32_t size)
{
	addr_t reg_end;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	reg_end = pe->lmem_base_addr_pa + pe->lmem_size;

	if ((addr >= pe->lmem_base_addr_pa) && ((addr + size) < reg_end))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Write elf section to PE memory
 * @details		Function expects the section data is in host endian format
 * @param[in]	pe The PE instance
 * @param[in]	sdata Pointer to the data described by 'shdr'
 * @param[in]	shdr Elf section header pointer
 */
static errno_t pfe_pe_load_elf_section(pfe_pe_t *pe, void *sdata, Elf32_Shdr *shdr)
{
	errno_t ret_val;
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == sdata) || (NULL == shdr)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret_val = EINVAL;
	}
	else
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (pfe_pe_is_dmem(pe, shdr->sh_addr, shdr->sh_size))
	{
		/*	Section belongs to DMEM */
		ret_val = pfe_pe_load_dmem_section(pe, sdata, shdr);
	}
	else if (pfe_pe_is_imem(pe, shdr->sh_addr, shdr->sh_size))
	{
		/*	Section belongs to IMEM */
		ret_val = pfe_pe_load_imem_section(pe, sdata, shdr);
	}
	else if (pfe_pe_is_lmem(pe, shdr->sh_addr, shdr->sh_size))
	{
		/*	Section belongs to LMEM */
		ret_val = pfe_pe_load_lmem_section(pe, sdata, shdr);
	}
	else
	{
		NXP_LOG_ERROR("Unsupported memory range 0x%08x\n", shdr->sh_addr);
		ret_val = EINVAL;
	}

	return ret_val;
}

/**
 * @brief		Create new PE instance
 * @param[in]	cbus_base_va CBUS base address (virtual)
 * @param[in]	type Type of PE to create @see pfe_pe_type_t
 * @param[in]	id PE ID
 * @return		The PE instance or NULL if failed
 */
pfe_pe_t * pfe_pe_create(void *cbus_base_va, pfe_pe_type_t type, uint8_t id)
{
	pfe_pe_t *pe = NULL;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (type != PE_TYPE_INVALID && type < PE_TYPE_MAX)
	{
		pe = oal_mm_malloc(sizeof(pfe_pe_t));

		if (NULL != pe)
		{
			memset(pe, 0, sizeof(pfe_pe_t));
			pe->type = type;
			pe->cbus_base_va = cbus_base_va;
			pe->id = id;
		}
	}

	return pe;
}

/**
 * @brief		Set DMEM base address for .elf mapping
 * @details		Information will be used by pfe_pe_load_firmware() to determine how and
 * 				which sections of the input .elf file will be written to DMEM.
 * @param[in]	pe The PE instance
 * @param[in]	elf_base DMEM base virtual address within .elf
 * @param[in]	len DMEM memory length
 */
void pfe_pe_set_dmem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	pe->dmem_elf_base_va = elf_base;
	pe->dmem_size = len;
}

/**
 * @brief		Set IMEM base address for .elf mapping
 * @details		Information will be used by pfe_pe_load_firmware() to determine how and
 * 				which sections of the input .elf file will be written to IMEM.
 * @param[in]	pe The PE instance
 * @param[in]	elf_base_va IMEM base virtual address within .elf
 * @param[in]	len IMEM memory length
 */
void pfe_pe_set_imem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	pe->imem_elf_base_va = elf_base;
	pe->imem_size = len;
}

/**
 * @brief		Set LMEM base address
 * @details		Information will be used by pfe_pe_load_firmware() to determine how and
 * 				which sections of the input .elf file will be written to LMEM.
 * @param[in]	pe The PE instance
 * @param[in]	elf_base_va LMEM base virtual address within .elf
 * @param[in]	len LMEM memory length
 */
void pfe_pe_set_lmem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	pe->lmem_base_addr_pa = elf_base;
	pe->lmem_size = len;
}

/**
 * @brief		Set DDR base address
 * @param[in]	pe The PE instance
 * @param[in]	base_pa DDR base physical address as seen by host
 * @param[in]	base_va DDR base virtual address
 * @param[in]	len DDR region length
 */
void pfe_pe_set_ddr(pfe_pe_t *pe, void *base_pa, void *base_va, addr_t len)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	pe->ddr_base_addr_pa = base_pa;
	pe->ddr_base_addr_va = base_va;
	pe->ddr_size = len;
}

/**
 * @brief		Set indirect access registers
 * @details		Internal PFE memories can be accessed from host using indirect
 * 				access registers. This function sets CBUS addresses of this
 * 				registers.
 * @param[in]	pe The PE instance
 * @param[in]	wdata_reg The WDATA register address as appears on CBUS
 * @param[in]	rdata_reg The RDATA register address as appears on CBUS
 * @param[in]	addr_reg The ADDR register address as appears on CBUS
 */
void pfe_pe_set_iaccess(pfe_pe_t *pe, uint32_t wdata_reg, uint32_t rdata_reg, uint32_t addr_reg)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	pe->mem_access_addr = (void *)((addr_t)pe->cbus_base_va + addr_reg);
	pe->mem_access_rdata = (void *)((addr_t)pe->cbus_base_va + rdata_reg);
	pe->mem_access_wdata = (void *)((addr_t)pe->cbus_base_va + wdata_reg);
}

/**
 * @brief		Upload firmware into PEs memory
 * @param[in]	pe The PE instance
 * @param[in]	elf The elf file object to be uploaded
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_pe_load_firmware(pfe_pe_t *pe, const void *elf)
{
	uint32_t ii;
	errno_t ret;
	uint32_t section_idx;
	ELF_File_t *elf_file = (ELF_File_t *)elf;
	Elf32_Shdr *shdr = NULL;
	void *buf;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == elf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Attempt to get section containing firmware runtime status */
	if (TRUE == ELF_SectFindName(elf_file, ".dmem_pestatus", &section_idx, NULL, NULL))
	{
		/*	Get section header */
		shdr = &elf_file->arSectHead32[section_idx];

		/*	PE status data is at this address within PE memory space */
		pe->dmem_pestatus_pa = shdr->sh_addr;
		if (FALSE == pfe_pe_is_dmem(pe, pe->dmem_pestatus_pa, 0U))
		{
			NXP_LOG_ERROR("PE status section is not in DMEM\n");
		}
		else
		{
			pe->dmem_pestatus_available = TRUE;
		}
	}
	else
	{
		NXP_LOG_WARNING("Section not found (.dmem_pestatus). Runtime FW statistics will not be available\n");
	}

	/*	Attempt to get section containing firmware memory map data */
	if (TRUE == ELF_SectFindName(elf_file, ".pfe_pe_mmap", &section_idx, NULL, NULL))
	{
		/*	Get section header */
		shdr = &elf_file->arSectHead32[section_idx];

		/*	MMAP data is at this address within PE memory space */
		pe->mmap_data_pa = shdr->sh_addr;
		if (FALSE == pfe_pe_is_dmem(pe, pe->mmap_data_pa, 0U))
		{
			NXP_LOG_ERROR("Memory map section is not in DMEM\n");
		}
		else
		{
			pe->mmap_data_available = TRUE;
		}
	}
	else
	{
		NXP_LOG_WARNING("Section not found (.pfe_pe_mmap). Memory map will not be available\n");
	}

	/*	.elf data must be in BIG ENDIAN */
	if (1U == elf_file->Header.e_ident[EI_DATA])
	{
		NXP_LOG_DEBUG("Unexpected .elf format (little endian)\n");
		return EINVAL;
	}

#if !defined(GLOBAL_CFG_RUN_ON_VDK)
	/*	Try to upload all sections of the .elf */
	for (ii=0U; ii<elf_file->Header.r32.e_shnum; ii++)
	{
		/* Special handling for the .errors section - remember it in a buffer for later accesses */
		if(0 == strcmp(".errors", (char *)(elf_file->acSectNames+elf_file->arSectHead32[ii].sh_name)))
		{
			if(NULL != pe->fw_err_section)
			{   /* Free old section (possibly different) during the FW reload */
				oal_mm_free(pe->fw_err_section);
			}
			
			/* Get memory for the section storage */
			pe->fw_err_section = oal_mm_malloc(elf_file->arSectHead32[ii].sh_size);
			if(NULL == pe->fw_err_section)
			{
				NXP_LOG_ERROR("No memory to store .errors section (%u Bytes)\n", elf_file->arSectHead32[ii].sh_size);
				return ENOMEM;
			}
			/* Copy (load) section into the storage */
			memcpy(pe->fw_err_section, elf_file->pvData + elf_file->arSectHead32[ii].sh_offset, elf_file->arSectHead32[ii].sh_size);
			continue;
		}
		else
		{
			pe->fw_err_section = NULL;
			NXP_LOG_WARNING(".errors section not present. FW error reporting will not be available.\n");
		}

		if (!(elf_file->arSectHead32[ii].sh_flags & (SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR)))
		{
			/*	Skip the section */
			NXP_LOG_DEBUG("Skipping section %s\n", elf_file->acSectNames+elf_file->arSectHead32[ii].sh_name);
			continue;
		}

		buf = elf_file->pvData + elf_file->arSectHead32[ii].sh_offset;

		/*	Upload the section */
		ret = pfe_pe_load_elf_section(pe, buf, &elf_file->arSectHead32[ii]);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Couldn't upload firmware section %s, %d bytes @ 0x%08x. Reason: %d\n",
							elf_file->acSectNames+elf_file->arSectHead32[ii].sh_name,
							elf_file->arSectHead32[ii].sh_size,
							elf_file->arSectHead32[ii].sh_addr, ret);
			return ret;
		}
		/* Clear the internal copy of the index on each FW load because 
		   FW will also start from 0 */
		pe->last_error_write_index = 0U;
		/* The error record address could change when new FW was loaded */
		pe->error_record_addr = 0U;
	}
#else
	NXP_LOG_INFO("[RUN_ON_VDK]: Uploading CLASS firmware FAKED :)\n");
#endif /* !GLOBAL_CFG_RUN_ON_VDK */

	return EOK;
}

/**
 * @brief		Get pointer to PE's memory where memory map data is stored
 * @param[in]	pe The PE instance
 * @param[out]	mmap Pointer where memory map shall be written (values are in network byte order)
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Requested data not available
 */
errno_t pfe_pe_get_mmap(pfe_pe_t *pe, pfe_ct_pe_mmap_t *mmap)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == mmap)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (TRUE == pe->mmap_data_available)
	{
		pfe_pe_memcpy_from_dmem_to_host_32(pe, mmap, pe->mmap_data_pa, sizeof(pfe_ct_pe_mmap_t));
		return EOK;
	}
	else
	{
		return ENOENT;
	}
}

/**
 * @brief		Destroy PE instance
 * @param[in]	pe The PE instance
 */
void pfe_pe_destroy(pfe_pe_t *pe)
{
	if (NULL != pe)
	{
		if(NULL != pe->fw_err_section)
		{
			oal_mm_free(pe->fw_err_section);
			pe->fw_err_section = NULL;
		}
		
		oal_mm_free(pe);
	}
}


/**
 * @brief		Reads out errors reported by the PE Firmware and prints them on debug console
 * @param[in]	pe PE which error report shall be read out
 * @return EOK on succes or error code
 */
errno_t pfe_pe_get_fw_errors(pfe_pe_t *pe)
{
	pfe_ct_error_record_t error_record; /* Copy of the PE error record */
	uint32_t read_start;                /* Starting position in error record to read */
	uint32_t i;
	uint32_t errors_count;

	if(0U == pe->error_record_addr)
	{   /* The mmap has not been queried for error record yet */
		pfe_ct_pe_mmap_t pfe_pe_mmap;
		/* Query map for the error record address */
		if (EOK != pfe_pe_get_mmap(pe, &pfe_pe_mmap))
		{
			NXP_LOG_ERROR("Could not get memory map\n");
			return ENOENT;
		}
		/* Remember the error record address */
		pe->error_record_addr = oal_ntohl(pfe_pe_mmap.error_record);
	}
	if(NULL == pe->fw_err_section)
	{	/* Avoid running uninitialized */
		return ENOENT;
	}

	/* Copy error record from PE to local memory */
	pfe_pe_memcpy_from_dmem_to_host_32(pe, &error_record, pe->error_record_addr, sizeof(pfe_ct_error_record_t));
	/* Get the number of new errors */
	errors_count = oal_ntohl(error_record.write_index) - pe->last_error_write_index;
	/* First unread error */
	read_start = pe->last_error_write_index;
	/* Where to continue next time */
	pe->last_error_write_index = oal_ntohl(error_record.write_index);
	if(0U != errors_count)
	{   /* New errors reported - go through them */
		if(errors_count > FP_ERROR_RECORD_SIZE)
		{
			NXP_LOG_WARNING("FW error log overflow by %u\n", errors_count - FP_ERROR_RECORD_SIZE + 1U);
			/* Overflow has occurred - the write_index contains oldest record */
			read_start = oal_ntohl(error_record.write_index);
			errors_count = FP_ERROR_RECORD_SIZE;
		}
			
		for(i = 0U; i < errors_count; i++)
		{
			uint32_t error_addr;
			uint32_t error_line;
			pfe_ct_error_t *error_ptr;
			char_t *error_str;
			char_t *error_file;

			error_addr = oal_ntohl(error_record.errors[(read_start + i) & (FP_ERROR_RECORD_SIZE - 1U)]);

			/* Get to the error message through the .errors section */
			error_ptr = pe->fw_err_section + error_addr;
			error_str = pe->fw_err_section + oal_ntohl(error_ptr->message);
			error_file =  pe->fw_err_section + oal_ntohl(error_ptr->file);
			error_line = oal_ntohl(error_ptr->line);
			NXP_LOG_ERROR("PE%d: %s line %u: %s\n", pe->id, error_file, error_line, error_str);
		}
	}

	return EOK;
}

/**
 * @brief Reads and validates PE mmap
 * @param[in] pe The PE instance
 */
errno_t pfe_pe_check_mmap(pfe_pe_t *pe)
{
	pfe_ct_pe_mmap_t pfe_pe_mmap;

	/*	Get mmap base from PE[0] since all PEs have the same memory map */
	if (EOK != pfe_pe_get_mmap(pe, &pfe_pe_mmap))
	{
		NXP_LOG_ERROR("Could not get memory map\n");
		return ENOENT;
	}

	if (oal_ntohl(pfe_pe_mmap.size) != sizeof(pfe_ct_pe_mmap_t))
	{
		NXP_LOG_ERROR("Structure length mismatch: found %u, but required %u\n", (uint32_t)oal_ntohl(pfe_pe_mmap.size), (uint32_t)sizeof(pfe_ct_pe_mmap_t));
		return EINVAL;
	}

	NXP_LOG_INFO("[FW VERSION] %d.%d.%d, Build: %s, %s (%s), ID: 0x%x\n",
			pfe_pe_mmap.version.major,
			pfe_pe_mmap.version.minor,
			pfe_pe_mmap.version.patch,
			(char_t *)pfe_pe_mmap.version.date,
			(char_t *)pfe_pe_mmap.version.time,
			(char_t *)pfe_pe_mmap.version.vctrl,
			pfe_pe_mmap.version.id);

	NXP_LOG_INFO("[PE %u MMAP]\n \
			DMEM Heap Base: 0x%08x (%d bytes)\n \
			PHY IF Base   : 0x%08x (%d bytes)\n",
			pe->id,
			oal_ntohl(pfe_pe_mmap.dmem_heap_base),
			oal_ntohl(pfe_pe_mmap.dmem_heap_size),
			oal_ntohl(pfe_pe_mmap.dmem_phy_if_base),
			oal_ntohl(pfe_pe_mmap.dmem_phy_if_size));
	return EOK;

}

/**
 * @brief		Copies PE (global) statistics into a prepared buffer
 * @param[in]	pe		PE which statistics shall be read
 * @param[in]	addr	Address within the PE DMEM where the statistics are located
 * @param[out]	stats	Buffer where to copy the statistics from the PE DMEM
 * @retval		EOK		Success
 * @retval		EINVAL	Invalid argument
 */
errno_t pfe_pe_get_pe_stats(pfe_pe_t *pe, uint32_t addr, pfe_ct_pe_stats_t *stats)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == stats)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	if (unlikely(0U == addr))
	{
		NXP_LOG_ERROR("NULL argument for DMEM received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */
	pfe_pe_memcpy_from_dmem_to_host_32(pe, stats, addr, sizeof(pfe_ct_pe_stats_t));
	return EOK;
}

/**
 * @brief		Copies PE clasification algorithms statistics into a prepared buffer
 * @param[in]	pe		PE which statistics shall be read
 * @param[in]	addr	Address within the PE DMEM where the statistics are located
 * @param[out]	stats	Buffer where to copy the statistics from the PE DMEM
 * @retval		EOK		Success
 * @retval		EINVAL	Invalid argument
 */
errno_t pfe_pe_get_classify_stats(pfe_pe_t *pe, uint32_t addr, pfe_ct_classify_stats_t *stats)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == stats)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	if (unlikely(0U == addr))
	{
		NXP_LOG_ERROR("NULL argument for DMEM received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */
	pfe_pe_memcpy_from_dmem_to_host_32(pe, stats, addr, sizeof(pfe_ct_classify_stats_t));
	return EOK;
}

/**
 * @brief		Copies classification algorithm or logical interface statistics into a prepared buffer
 * @param[in]	pe		PE which statistics shall be read
 * @param[in]	addr	Address within the PE DMEM where the statistics are located
 * @param[out]	stats	Buffer where to copy the statistics from the PE DMEM
 * @retval		EOK		Success
 * @retval		EINVAL	Invalid argument
 */
errno_t pfe_pe_get_class_algo_stats(pfe_pe_t *pe, uint32_t addr, pfe_ct_class_algo_stats_t *stats)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == stats)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
	if (unlikely(0U == addr))
	{
		NXP_LOG_ERROR("NULL argument for DMEM received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */
	pfe_pe_memcpy_from_dmem_to_host_32(pe, stats, addr, sizeof(pfe_ct_class_algo_stats_t));
	return EOK;
}

/**
 * @brief		Converts statistics of a logical interface or classification algorithm into a text form
 * @param[in]	stat		Statistics to convert
 * @param[out]	buf			Buffer where to write the text
 * @param[in]	buf_len		Buffer length
 * @param[in]	verb_level	Verbosity level
 * @return		Number of bytes written into the output buffer
 */
uint32_t pfe_pe_stat_to_str(pfe_ct_class_algo_stats_t *stat, char *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

	(void)verb_level;
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == stat) || (NULL == buf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames processed: %u\n", oal_ntohl(stat->processed));
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames accepted:  %u\n", oal_ntohl(stat->accepted));
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames rejected:  %u\n", oal_ntohl(stat->rejected));
	len += oal_util_snprintf(buf + len, buf_len - len, "Frames discarded: %u\n", oal_ntohl(stat->discarded));
	return len;
}


/**
 * @brief		Return PE runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	pe 			The PE instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	buf_len 	Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 *
 */
uint32_t pfe_pe_get_text_statistics(pfe_pe_t *pe, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;
	pfe_ct_pe_mmap_t mmap;
	pfe_ct_pe_stats_t pfe_ct_pe_stats;
	pfe_ct_classify_stats_t pfe_classification_stats;
	uint32_t i;

	/* Get the pfe_ct_pe_mmap_t structure from PE */
	if(EOK != pfe_pe_get_mmap(pe, &mmap))
	{
		return 0U;
	}
	if(0U != oal_ntohl(mmap.pe_stats))
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "- PE statistics -\n");
		/* Get the pfe_ct_pe_stats_t structure (pe_stats) */
		if(EOK != pfe_pe_get_pe_stats(pe, oal_ntohl(mmap.pe_stats), &pfe_ct_pe_stats))
		{
			return 0U;
		}
		/* Print the PE stats */
		len += oal_util_snprintf(buf + len, buf_len - len, "Frames processed: %u\n", oal_ntohl(pfe_ct_pe_stats.processed));
		len += oal_util_snprintf(buf + len, buf_len - len, "Frames discarded: %u\n", oal_ntohl(pfe_ct_pe_stats.discarded));
		for(i = 0U; i < PFE_PHY_IF_ID_MAX + 1U; i++)
		{
			len += oal_util_snprintf(buf + len, buf_len - len, "Frames with %u replicas: %u\n", i + 1U, oal_ntohl(pfe_ct_pe_stats.replicas[i]));
		}
		len += oal_util_snprintf(buf + len, buf_len - len, "Frames with HIF_TX_INJECT: %u\n", oal_ntohl(pfe_ct_pe_stats.injected));
	}
	else
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "PE statistics not available\n");
	}
	if(0U != oal_ntohl(mmap.classification_stats))
	{
		/* Get the pfe_classify_stats_t structure (classification_stats) */
		if(EOK != pfe_pe_get_classify_stats(pe, oal_ntohl(mmap.classification_stats), &pfe_classification_stats))
		{
			return 0U;
		}
		/* Print statistics for each classification algorithm */
		len += oal_util_snprintf(buf + len, buf_len - len, "- Flexible router -\n");
		len += pfe_pe_stat_to_str(&pfe_classification_stats.flexible_router, buf + len, len, verb_level);
		len += oal_util_snprintf(buf + len, buf_len - len, "- IP Router -\n");
		len += pfe_pe_stat_to_str(&pfe_classification_stats.ip_router, buf + len, len, verb_level);
		len += oal_util_snprintf(buf + len, buf_len - len, "- L2 Bridge -\n");
		len += pfe_pe_stat_to_str(&pfe_classification_stats.l2_bridge, buf + len, len, verb_level);
		len += oal_util_snprintf(buf + len, buf_len - len, "- VLAN Bridge -\n");
		len += pfe_pe_stat_to_str(&pfe_classification_stats.vlan_bridge, buf + len, len, verb_level);
		len += oal_util_snprintf(buf + len, buf_len - len, "- Logical Interfaces -\n");
		len += pfe_pe_stat_to_str(&pfe_classification_stats.log_if, buf + len, len, verb_level);
		len += oal_util_snprintf(buf + len, buf_len - len, "- InterHIF -\n");
		len += pfe_pe_stat_to_str(&pfe_classification_stats.hif_to_hif, buf + len, len, verb_level);
	}
	else
	{
		len += oal_util_snprintf(buf + len, buf_len - len, "Classification alagorithms statistics not available\n");
	}
	return len;
}

/** @}*/
