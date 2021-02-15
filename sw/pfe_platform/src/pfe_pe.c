/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_platform.h"
#include "elf_cfg.h"
#include "elf.h"
#include "pfe_cbus.h"
#include "pfe_pe.h"

#define BYTES_TO_4B_ALIGNMENT(x)	(4U - ((x) & 0x3U))
#define INVALID_FEATURES_BASE 0xFFFFFFFF
/**
 * @brief	Mutex protecting access to common mem_access_* registers
 */
static oal_mutex_t mem_access_lock;
static bool_t mem_access_lock_init = FALSE;

/*	Processing Engine representation */
struct pfe_pe_tag
{
	pfe_ct_pe_type_t type;				/* PE type */
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

	/* FW Errors*/
	uint32_t error_record_addr;			/* Error record storage address in DMEM */
	uint32_t last_error_write_index;	/* Last seen value of write index in the record */
	void *fw_err_section;				/* Error descriptions elf section storage */
	uint32_t fw_err_section_size;		/* Size of the above section */
	/* FW features */
	void *fw_feature_section;			/* Features descriptions elf section storage */
	uint32_t fw_feature_section_size;	/* Size of the above section */
	uint32_t fw_features_base;          /* Extracted base address of the features table */
	uint32_t fw_features_size;          /* Number of entries in the features table */

	/*	MMap */
	pfe_ct_pe_mmap_t *mmap_data;		/* Buffer containing the memory map data */
	/* Mutex */
	oal_mutex_t lock_mutex;				/* Locking PE API mutex */
};

typedef enum
{
	PFE_PE_DMEM,
	PFE_PE_IMEM
} pfe_pe_mem_t;

static void pfe_pe_memcpy_from_host_to_imem_32_nolock(pfe_pe_t *pe, addr_t dst, const void *src, uint32_t len);
static bool_t pfe_pe_is_active(pfe_pe_t *pe);
static void pfe_pe_memcpy_from_imem_to_host_32_nolock(pfe_pe_t *pe, void *dst, addr_t src, uint32_t len);
static void pfe_pe_mem_memset_nolock(pfe_pe_t *pe, pfe_pe_mem_t mem, uint8_t val, addr_t addr, uint32_t len);
static errno_t pfe_pe_set_number(pfe_pe_t *pe);

/**
 * @brief		Query if PE is active
 * @details		PE is active if it is running (executing firmware code) and is not gracefully stopped
 * @param[in]	pe The PE instance
 * @return		TRUE if PE is active, FALSE if not
 */
static bool_t pfe_pe_is_active(pfe_pe_t *pe)
{
	pfe_ct_pe_sw_state_monitor_t state_monitor;

	if(NULL == pe->mmap_data)
	{   /* Not loaded */
		NXP_LOG_WARNING("PE %u; Firmware not loaded\n", pe->id);
		return FALSE;
	}

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &state_monitor, oal_ntohl(pe->mmap_data->common.state_monitor), sizeof(pfe_ct_pe_sw_state_monitor_t));

	if(PFE_FW_STATE_STOPPED == state_monitor.state)
	{   /* Stopped */
		return FALSE;
	}

	if(PFE_FW_STATE_UNINIT == state_monitor.state)
	{   /* Not initialized yet */
		return FALSE;
	}
	/* PFE_FW_STATE_INIT == state_monitor.state is considered as running because
	   the transition to next state is short */

	return TRUE;
}

/**
* @brief Lock PE access
* @param[in] pe PE which access shall be locked
* @return EOK if success, error code otherwise
*/
errno_t pfe_pe_lock(pfe_pe_t *pe)
{
    return oal_mutex_lock(&pe->lock_mutex);
}

/**
* @brief Unlock PE access
* @param[in] pe PE which access shall be unlocked
* @return EOK if success, error code otherwise
*/
errno_t pfe_pe_unlock(pfe_pe_t *pe)
{
    return oal_mutex_unlock(&pe->lock_mutex);
}

/**
 * @brief		Lock PE memory
 * @details		While locked, the PE can't access internal memory. Invoke the PE graceful
 *				stop request and wait for confirmation.
 * @param[in]	pe The PE instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_pe_mem_lock(pfe_pe_t *pe)
{
	PFE_PTR(pfe_ct_pe_misc_control_t) misc_dmem;
	pfe_ct_pe_misc_control_t misc_ctrl = {0};
	uint32_t timeout = 10;

	if (NULL == pe->mmap_data)
	{
		return ENOEXEC;
	}

	misc_dmem = oal_ntohl(pe->mmap_data->common.pe_misc_control);
	if (0U == misc_dmem)
	{
		return EINVAL;
	}

	if (EOK != oal_mutex_lock(&pe->lock_mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
		return EPERM;
	}

	/*	Read the misc control structure from DMEM */
	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &misc_ctrl, misc_dmem, sizeof(pfe_ct_pe_misc_control_t));

	if (0U != misc_ctrl.graceful_stop_request)
	{
		if (0U != misc_ctrl.graceful_stop_confirmation)
		{
			NXP_LOG_ERROR("Locking locked memory\n");
		}
		else
		{
			NXP_LOG_ERROR("Duplicate stop request\n");
		}
		if (EOK != oal_mutex_unlock(&pe->lock_mutex))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
		return EPERM;
	}
	else
	{
		/*	Writing the non-zero value triggers the request */
		misc_ctrl.graceful_stop_request = 0xffU;
		/*	PE will respond with setting this to non-zero value */
		misc_ctrl.graceful_stop_confirmation = 0x0U;
	}

	/*	Use 'nolock' variant here. Accessing this data can't lead to conflicts. */
	pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe, misc_dmem, &misc_ctrl, sizeof(pfe_ct_pe_misc_control_t));

	if (FALSE == pfe_pe_is_active(pe))
	{
		/*	Access to PE memories is considered to be safe */
		if (EOK != oal_mutex_unlock(&pe->lock_mutex))
		{
			NXP_LOG_ERROR("mutex unlock failed\n");
		}
		return EOK;
	}

	/*	Wait for response */
	do
	{
		if (0U == timeout)
		{
			NXP_LOG_ERROR("Timed-out\n");

			/*	Cancel the request */
			misc_ctrl.graceful_stop_request = 0U;

			/*	Use 'nolock' variant here. Accessing this data can't lead to conflicts. */
			pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe, misc_dmem, &misc_ctrl, sizeof(pfe_ct_pe_misc_control_t));
			if (EOK != oal_mutex_unlock(&pe->lock_mutex))
			{
				NXP_LOG_ERROR("mutex unlock failed\n");
			}
			return ETIME;
		}

		oal_time_usleep(10U);
		timeout--;
		pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &misc_ctrl, misc_dmem, sizeof(pfe_ct_pe_misc_control_t));

	} while(0U == misc_ctrl.graceful_stop_confirmation);
	if (EOK != oal_mutex_unlock(&pe->lock_mutex))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}
	return EOK;
}

/**
 * @brief		Unlock PE memory
 * @details		While locked, the PE can't access internal memory. Here the memory
 * 				will be unlocked.
 * @param[in]	pe The PE instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_pe_mem_unlock(pfe_pe_t *pe)
{
	PFE_PTR(pfe_ct_pe_misc_control_t) misc_dmem;
	pfe_ct_pe_misc_control_t misc_ctrl = {0};

	if (NULL == pe->mmap_data)
	{
		return ENOEXEC;
	}

	misc_dmem = oal_ntohl(pe->mmap_data->common.pe_misc_control);
	if (0U == misc_dmem)
	{
		return EINVAL;
	}
	if (EOK != oal_mutex_lock(&pe->lock_mutex))
	{
		NXP_LOG_DEBUG("mutex lock failed\n");
		return EPERM;
	}
	/*	Cancel the stop request */
	misc_ctrl.graceful_stop_request = 0U;

	/*	Use 'nolock' variant here. Accessing this data can't lead to conflicts. */
	pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe, misc_dmem, &misc_ctrl, sizeof(pfe_ct_pe_misc_control_t));
	if (EOK != oal_mutex_unlock(&pe->lock_mutex))
	{
		NXP_LOG_ERROR("mutex unlock failed\n");
	}
	return EOK;
}

/**
 * @brief		Read data from PE memory
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory to access
 * @param[in]	addr Read address (should be aligned to 32 bits)
 * @param[in]	size Number of bytes to read (maximum 4)
 * @return		The data read (BE).
 */
static uint32_t pfe_pe_mem_read(pfe_pe_t *pe, pfe_pe_mem_t mem, addr_t addr, uint8_t size)
{
	uint32_t val;
	uint32_t mask;
	uint32_t memsel;
	uint8_t offset;
	uint8_t size_temp =size;
	addr_t adrr_temp = addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (unlikely(addr & 0x3U))
	{
		if (size_temp > BYTES_TO_4B_ALIGNMENT(addr))
		{
			/*	Here we need to split the read into two reads */

			/*	Read 1 (LS bytes). Recursive call. Limited to single recursion. */
			val = pfe_pe_mem_read(pe, mem, addr, (uint8_t)BYTES_TO_4B_ALIGNMENT(addr));
			offset = (uint8_t)(4U - (addr & 0x3U));
			size_temp = size - offset;
			adrr_temp = addr + offset;

			/*	Read 2 (MS bytes). Recursive call. Limited to single recursion. */
			val |= (pfe_pe_mem_read(pe, mem, adrr_temp, size_temp) << (8U * offset));

			return val;
		}
	}

	if (size_temp != 4U)
	{
		mask = ((uint32_t)1U << (size_temp * 8U)) - 1U;
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

	adrr_temp = (adrr_temp & 0xfffffU)
				| PE_IBUS_READ
				| memsel
				| PE_IBUS_PE_ID(pe->id)
				| PE_IBUS_WREN(0U);

	if (EOK != oal_mutex_lock(&mem_access_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	hal_write32((uint32_t)adrr_temp, pe->mem_access_addr);
	val = oal_ntohl(hal_read32(pe->mem_access_rdata));

	if (EOK != oal_mutex_unlock(&mem_access_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	if (unlikely(adrr_temp & 0x3U))
	{
		/*	Move the value to the desired address offset */
		val = (val >> (8U * (adrr_temp & 0x3U)));
	}

	return (val & mask);
}

/**
 * @brief		Write data into PE memory
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory to access
 * @param[in]	addr Write address (should be aligned to 32 bits)
 * @param[in]	val Value to write (BE)
 * @param[in]	size Number of bytes to write (maximum 4)
 */
static void pfe_pe_mem_write(pfe_pe_t *pe, pfe_pe_mem_t mem, uint32_t val, addr_t addr, uint8_t size)
{
	uint8_t bytesel = 0U;
	uint32_t memsel;
	uint8_t offset;
	uint32_t val_temp = val;
	uint8_t size_temp = size;
	addr_t addr_temp = addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (unlikely((addr_temp & 0x3U) != 0U))
	{
		offset = BYTES_TO_4B_ALIGNMENT(addr_temp);

		if (size_temp <= offset)
		{
			/*	Move the value to the desired address offset */
			val_temp = val << (8U * (addr_temp & 0x3U));
			/*	Enable writes of depicted bytes */
			bytesel = (((1U << size_temp) - 1U) << (offset - size_temp));
		}
		else
		{
			/*	Here we need to split the write into two writes */

			/*	Write 1 (LS bytes). Recursive call. Limited to single recursion. */
			pfe_pe_mem_write(pe, mem, val_temp, addr, offset);
			val_temp >>= 8U * offset;
			size_temp = size - offset;
			addr_temp = addr + offset;

			/*	Write 2 (MS bytes). Recursive call. Limited to single recursion. */
			pfe_pe_mem_write(pe, mem, val_temp, addr_temp, size_temp);

			return;
		}
	}
	else
	{
		/*	Destination is aligned */
		bytesel = (uint8_t)PE_IBUS_BYTES(size_temp);
	}

	if (PFE_PE_DMEM == mem)
	{
		memsel = PE_IBUS_ACCESS_DMEM;
	}
	else
	{
		memsel = PE_IBUS_ACCESS_IMEM;
	}

	addr_temp = (addr_temp & 0xfffffU)
			| PE_IBUS_WRITE
			| memsel
			| PE_IBUS_PE_ID(pe->id)
			| PE_IBUS_WREN(bytesel);

	if (EOK != oal_mutex_lock(&mem_access_lock))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	hal_write32(oal_htonl(val_temp), pe->mem_access_wdata);
	hal_write32((uint32_t)addr_temp, pe->mem_access_addr);

	if (EOK != oal_mutex_unlock(&mem_access_lock))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}
}

/**
 * @brief		Memset for PE memories
 * @param[in]	pe The PE instance
 * @param[in]	mem Memory to access
 * @param[in]	val Byte value to be used to fill the memory block
 * @param[in]	addr Address of the memory block within DMEM
 * @param[in]	len Number of bytes to fill
 */
static void pfe_pe_mem_memset_nolock(pfe_pe_t *pe, pfe_pe_mem_t mem, uint8_t val, addr_t addr, uint32_t len)
{
	uint32_t val32 = (uint32_t)val | ((uint32_t)val << 8) | ((uint32_t)val << 16) | ((uint32_t)val << 24);
	uint32_t offset;
	uint32_t temp = len;
	uint32_t adrr_temp = addr;

	if ((addr & 0x3U) != 0U)
	{
		/*	Write unaligned bytes to align the address */
		offset = BYTES_TO_4B_ALIGNMENT(addr);
		offset = (len < offset) ? len : offset;
		pfe_pe_mem_write(pe, mem, val32, addr, (uint8_t)offset);
		temp = (len >= offset) ? (len - offset) : 0U;
		adrr_temp = addr + offset;
	}

	while (temp>=4U)
	{
		/*	Write aligned words */
		pfe_pe_mem_write(pe, mem, val32, adrr_temp, 4U);
		temp-=4U;
		adrr_temp+=4U;
	}

	if (temp > 0U)
	{
		/*	Write the rest */
		pfe_pe_mem_write(pe, mem, val32, adrr_temp, (uint8_t)temp);
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
	if (EOK != pfe_pe_mem_lock(pe))
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return;
	}

	pfe_pe_mem_memset_nolock(pe, PFE_PE_DMEM, val, addr, len);

	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
	}
}

/**
 * @brief		Memset for IMEM
 * @param[in]	pe The PE instance
 * @param[in]	val Byte value to be used to fill the memory block
 * @param[in]	addr Address of the memory block within IMEM
 * @param[in]	len Number of bytes to fill
 */
void pfe_pe_imem_memset(pfe_pe_t *pe, uint8_t val, addr_t addr, uint32_t len)
{
	if (EOK != pfe_pe_mem_lock(pe))
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return;
	}

	pfe_pe_mem_memset_nolock(pe, PFE_PE_IMEM, val, addr, len);

	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
	}
}

/**
 * @brief		Write 'len' bytes to DMEM
 * @note		Function expects the source data to be in host endian format.
 * @param[in]	pe The PE instance
 * @param[in]	src Buffer source address (virtual)
 * @param[in]	dst DMEM destination address (must be 32-bit aligned)
 * @param[in]	len Number of bytes to read
 */
void pfe_pe_memcpy_from_host_to_dmem_32_nolock(pfe_pe_t *pe, addr_t dst, const void *src, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	const uint8_t *src_byteptr = src;
	addr_t dst_temp = dst;
	uint32_t len_temp = len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((dst_temp & 0x3U) != 0U)
	{
		/*	Write unaligned bytes to align the destination address */
		offset = BYTES_TO_4B_ALIGNMENT(dst_temp);
		offset = (len < offset) ? len : offset;
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, dst_temp, (uint8_t)offset);
		src_byteptr += offset;
		dst_temp = dst + offset;
		len_temp = (len >= offset) ? (len - offset) : 0U;
	}

	while (len_temp>=4U)
	{
		/*	4-byte writes */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, (uint32_t)dst_temp, 4U);
		len_temp-=4U;
		src_byteptr+=4U;
		dst_temp+=4U;
	}

	if (0U != len_temp)
	{
		/*	The rest */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_DMEM, val, (uint32_t)dst_temp, (uint8_t)len_temp);
	}
}

/**
 * @brief		Write 'len' bytes to DMEM
 * @note		Function expects the source data to be in host endian format.
 * @param[in]	pe The PE instance
 * @param[in]	src Buffer source address (virtual)
 * @param[in]	dst DMEM destination address (must be 32-bit aligned)
 * @param[in]	len Number of bytes to read
 */
void pfe_pe_memcpy_from_host_to_dmem_32(pfe_pe_t *pe, addr_t dst, const void *src, uint32_t len)
{
	if (EOK != pfe_pe_mem_lock(pe))
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return;
	}

	pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe, dst, src, len);

	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
	}
}

/**
 * @brief		Read 'len' bytes from DMEM
 * @param[in]	pe The PE instance
 * @param[in]	src DMEM source address (must be 32-bit aligned)
 * @param[in]	dst Destination address (virtual)
 * @param[in]	len Number of bytes to read
 *
 */
void pfe_pe_memcpy_from_dmem_to_host_32_nolock(pfe_pe_t *pe, void *dst, addr_t src, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	uint8_t *dst_byteptr = dst;
	addr_t src_temp = src;
	uint32_t len_temp = len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((src_temp & 0x3U) != 0U)
	{
		/*	Read unaligned bytes to align the source address */
		offset = BYTES_TO_4B_ALIGNMENT(src_temp);
		offset = (len < offset) ? len : offset;
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src_temp, (uint8_t)offset);
		(void)memcpy((void*)dst_byteptr, (const void*)&val, offset);
		dst_byteptr += offset;
		src_temp = src + offset;
		len_temp = (len >= offset) ? (len - offset) : 0U;
	}

	while (len_temp>=4U)
	{
		/*	4-byte reads */
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src_temp, 4U);
		*((uint32_t *)dst_byteptr) = val;
		len_temp-=4U;
		src_temp+=4U;
		dst_byteptr+=4U;
	}

	if (0U != len_temp)
	{
		/*	The rest */
		val = pfe_pe_mem_read(pe, PFE_PE_DMEM, (uint32_t)src_temp, (uint8_t)len_temp);
		(void)memcpy((void*)dst_byteptr, (const void*)&val, len_temp);
	}
}

/**
 * @brief		Read 'len' bytes from DMEM
 * @param[in]	pe The PE instance
 * @param[in]	src DMEM source address (must be 32-bit aligned)
 * @param[in]	dst Destination address (virtual)
 * @param[in]	len Number of bytes to read
 *
 */
void pfe_pe_memcpy_from_dmem_to_host_32(pfe_pe_t *pe, void *dst, addr_t src, uint32_t len)
{
	if (EOK != pfe_pe_mem_lock(pe))
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return;
	}

	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, dst, src, len);

	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
	}
}

/**
 * @brief		Read 'len' bytes from DMEM from each PE
 * @details		Reads PE internal data memory (DMEM) into a host memory through indirect
 *				access registers. The result from each PE are stored consecutively in memory
 *				pointed by dst.
 * @param[in]	pe Array of the PE instances
 * @param[in]	src DMEM source address (physical within PE, must be 32bit aligned)
 * @param[in]	dst Destination address (virtual) the size required to store the data is pe_count*len
 * @param[in]	buffer_len Destination buffer length
 * @param[in]	read_len Number of bytes to read (from one PE)
 *
 */
errno_t pfe_pe_gather_memcpy_from_dmem_to_host_32(pfe_pe_t **pe, int32_t pe_count, void *dst, addr_t src, uint32_t buffer_len, uint32_t read_len)
{
	int32_t ii = 0U;
	errno_t ret = EOK;
	errno_t ret_store = EOK;

	while (ii < pe_count)
	{
		ret = pfe_pe_mem_lock(pe[ii]);
		if(EOK != ret)
		{
			NXP_LOG_DEBUG("Memory unlock failed\n");
			/* Free all already locked PEs */
			for(; ii >= 0; ii--)
			{
				(void)pfe_pe_mem_unlock(pe[ii]);
			}
			return ret;
		}
		ii++;
	}

	/* Perform the read from required PEs*/
	for(ii = 0; ii < pe_count; ii++)
	{
		/* Check if there is still memory  */
		if(buffer_len >= ((read_len * (uint32_t)ii) + read_len))
		{
			pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe[ii], (void *)((uint8_t*)dst + (read_len * (uint32_t)ii)), src, read_len);
		}
		else
		{
			ret = ENOMEM;
			break;
		}
	}

	for(ii = 0; ii < pe_count; ii++)
	{
		/* Backup return from mecpy */
		ret_store = ret;
		ret = pfe_pe_mem_unlock(pe[ii]);
		if(EOK != ret)
		{
			NXP_LOG_DEBUG("Memory unlock failed\n");
			/* Error occurred override the return from previous actions */
			ret_store = ret;
		}
	}

	/* Restore return to before unlock */
	ret = ret_store;

	return ret;
}

/**
 * @brief		Write 'len'bytes to IMEM
 * @note		Function expects the source data to be in host endian format.
 * @param[in]	pe The PE instance
 * @param[in]	src Buffer source address (host, virtual)
 * @param[in]	dst IMEM destination address (must be 32-bit aligned)
 * @param[in]	len Number of bytes to copy
 */
static void pfe_pe_memcpy_from_host_to_imem_32_nolock(pfe_pe_t *pe, addr_t dst, const void *src, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	addr_t dst_temp = dst;
	/* Avoid void pointer arithmetics */
	const uint8_t *src_byteptr = src;
	uint32_t len_temp = len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((dst_temp & 0x3U) != 0U)
	{
		/*	Write unaligned bytes to align the destination address */
		offset = BYTES_TO_4B_ALIGNMENT(dst_temp);
		offset = (len < offset) ? len : offset;
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_IMEM, val, dst_temp, (uint8_t)offset);
		src_byteptr += offset;
		dst_temp = dst + offset;
		len_temp = (len >= offset) ? (len - offset) : 0U;
	}

	while(len_temp>=4U)
	{
		/*	4-byte writes */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_IMEM, val, (uint32_t)dst_temp, 4U);
		len_temp-=4U;
		src_byteptr+=4U;
		dst_temp+=4U;
	}

	if (0U != len_temp)
	{
		/*	The rest */
		val = *(uint32_t *)src_byteptr;
		pfe_pe_mem_write(pe, PFE_PE_IMEM, val, (uint32_t)dst_temp, (uint8_t)len_temp);
	}
}

/**
 * @brief		Write 'len'bytes to IMEM
 * @note		Function expects the source data to be in host endian format.
 * @param[in]	pe The PE instance
 * @param[in]	src Buffer source address (host, virtual)
 * @param[in]	dst IMEM destination address (must be 32-bit aligned)
 * @param[in]	len Number of bytes to copy
 */
void pfe_pe_memcpy_from_host_to_imem_32(pfe_pe_t *pe, addr_t dst, const void *src, uint32_t len)
{
	if (EOK != pfe_pe_mem_lock(pe))
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return;
	}

	pfe_pe_memcpy_from_host_to_imem_32_nolock(pe, dst, src, len);

	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
	}
}

/**
 * @brief		Read 'len' bytes from IMEM
 * @param[in]	pe The PE instance
 * @param[in]	src IMEM source address (physical within PE, must be 32-bit aligned)
 * @param[in]	dst Destination address (host, virtual)
 * @param[in]	len Number of bytes to read
 *
 */
static void pfe_pe_memcpy_from_imem_to_host_32_nolock(pfe_pe_t *pe, void *dst, addr_t src, uint32_t len)
{
	uint32_t val;
	uint32_t offset;
	/* Avoid void pointer arithmetics */
	uint8_t *dst_byteptr = dst;
	addr_t src_temp = src;
	uint32_t len_temp = len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((src_temp & 0x3U) != 0U)
	{
		/*	Read unaligned bytes to align the source address */
		offset = BYTES_TO_4B_ALIGNMENT(src_temp);
		offset = (len < offset) ? len : offset;
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src_temp, (uint8_t)offset);
		(void)memcpy((void*)dst_byteptr, (const void*)&val, offset);
		dst_byteptr += offset;
		src_temp = src + offset;
		len_temp = (len >= offset) ? (len - offset) : 0U;
	}

	while (len_temp>=4U)
	{
		/*	4-byte reads */
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src_temp, 4U);
		*((uint32_t *)dst_byteptr) = val;
		len_temp-=4U;
		src_temp+=4U;
		dst_byteptr+=4U;
	}

	if (0U != len_temp)
	{
		/*	The rest */
		val = pfe_pe_mem_read(pe, PFE_PE_IMEM, (uint32_t)src_temp, (uint8_t)len_temp);
		(void)memcpy((void*)dst_byteptr, (const void*)&val, len_temp);
	}
}

/**
 * @brief		Read 'len' bytes from IMEM
 * @param[in]	pe The PE instance
 * @param[in]	src IMEM source address (physical within PE, must be 32-bit aligned)
 * @param[in]	dst Destination address (host, virtual)
 * @param[in]	len Number of bytes to read
 *
 */
void pfe_pe_memcpy_from_imem_to_host_32(pfe_pe_t *pe, void *dst, addr_t src, uint32_t len)
{
	if (EOK != pfe_pe_mem_lock(pe))
	{
		NXP_LOG_DEBUG("Memory lock failed\n");
		return;
	}

	pfe_pe_memcpy_from_imem_to_host_32_nolock(pe, dst, src, len);

	if (EOK != pfe_pe_mem_unlock(pe))
	{
		NXP_LOG_DEBUG("Memory unlock failed\n");
	}
}

/**
 * @brief		Load an elf section into DMEM
 * @details		Size and load address need to be at least 32-bit aligned
 * @param[in]	pe The PE instance
 * @param[in]	sdata Pointer to the elf section data
 * @param[in]	addr Load address of the section
 * @param[in]	size Size of the section
 * @param[in]	type Section type
 * @retval		EOK Success
 * @retval		EINVAL Unsupported section type or wrong input address alignment
 */
static errno_t pfe_pe_load_dmem_section(pfe_pe_t *pe, void *sdata, addr_t addr, addr_t size, uint32_t type)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == sdata)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (((addr_t)(sdata) & 0x3U) != (addr & 0x3U))
	{
		NXP_LOG_ERROR("Load address 0x%p and elf file address 0x%p don't have the same alignment\n", (void *)addr, sdata);
		return EINVAL;
	}

	if ((addr & 0x3U) != 0U)
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
		case (uint32_t)SHT_PROGBITS:
		{
#if defined(FW_WRITE_CHECK_EN)
			void *buf = oal_mm_malloc(size);
#endif /* FW_WRITE_CHECK_EN */

			/*	Write section data */
			pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe, addr - pe->dmem_elf_base_va, sdata, size);

#if defined(FW_WRITE_CHECK_EN)
			pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, buf, addr, size);

			if (0 != memcmp(buf, sdata, size))
			{
				NXP_LOG_ERROR("DMEM data inconsistent\n");
			}

			oal_mm_free(buf);
#endif /* FW_WRITE_CHECK_EN */

			break;
		}

		case (uint32_t)SHT_NOBITS:
		{
			pfe_pe_dmem_memset(pe, 0U, addr, size);
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unsupported section type: 0x%x\n", type);
			ret = EINVAL;
			break;
		}
	}

	return ret;
}

/**
 * @brief		Load an elf section into IMEM
 * @details		Code needs to be at least 16bit aligned and only PROGBITS sections are supported
 * @param[in]	pe The PE instance
 * @param[in]	data Pointer to the elf section data
 * @param[in]	addr Load address of the section
 * @param[in]	size Size of the section
 * @param[in]	type Type of the section
 * @retval		EOK Success
 * @retval		EFAULT Wrong input address alignment
 * @retval		EINVAL Unsupported section type
 */
static errno_t pfe_pe_load_imem_section(pfe_pe_t *pe, const void *data, addr_t addr, addr_t size, uint32_t type)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == data)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check alignment first */
	if (((addr_t)(data) & 0x3U) != (addr & 0x1U))
	{
		NXP_LOG_ERROR("Load address 0x%p and elf file address 0x%p) don't have the same alignment\n", (void *)addr, data);
		return EFAULT;
	}

	if ((addr & 0x1U) != 0U)
	{
		NXP_LOG_ERROR("Load address 0x%p is not 16bit aligned\n", (void *)addr);
		return EFAULT;
	}

	if ((size & 0x1U) != 0U)
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
		case (uint32_t)SHT_PROGBITS:
		{
#if defined(FW_WRITE_CHECK_EN)
			void *buf = oal_mm_malloc(size);
#endif /* FW_WRITE_CHECK_EN */

			/*	Write section data */
			pfe_pe_memcpy_from_host_to_imem_32_nolock(pe, addr - pe->imem_elf_base_va, data, size);

#if defined(FW_WRITE_CHECK_EN)
			pfe_pe_memcpy_from_imem_to_host_32_nolock(pe, buf, addr, size);

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
			ret = EINVAL;
			break;
		}
	}

	return ret;
}

/**
 * @brief		Check if memory region belongs to DMEM
 * @param[in]	pe The PE instance
 * @param[in]	addr Address to be checked
 * @param[in]	size Length of the region to be checked
 * @return		TRUE if given range belongs to DMEM
 */
static bool_t pfe_pe_is_dmem(pfe_pe_t *pe, addr_t addr, uint32_t size)
{
	addr_t reg_end;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
 * @brief		Check if memory region belongs to IMEM
 * @param[in]	pe The PE instance
 * @param[in]	addr Address to be checked
 * @param[in]	size Length of the region to be checked
 * @return		TRUE if given range belongs to IMEM
 */
static bool_t pfe_pe_is_imem(pfe_pe_t *pe, addr_t addr, uint32_t size)
{
	addr_t reg_end;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
 * @brief		Check if memory region belongs to LMEM
 * @param[in]	pe The PE instance
 * @param[in]	addr Address to be checked
 * @param[in]	size Length of the region to be checked
 * @return		TRUE if given range belongs to LMEM
 */
static bool_t pfe_pe_is_lmem(pfe_pe_t *pe, addr_t addr, uint32_t size)
{
	addr_t reg_end;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
 * @param[in]	load_addr Address where to load the section
 * @param[in]	size Size of the section to load
 * @param[in]	type Type of the section to load
 */
static errno_t pfe_pe_load_elf_section(pfe_pe_t *pe, void *sdata, addr_t load_addr, addr_t size, uint32_t type)
{
	errno_t ret_val;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == sdata)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret_val = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (pfe_pe_is_dmem(pe, load_addr, size))
	{
		/*	Section belongs to DMEM */
		ret_val = pfe_pe_load_dmem_section(pe, sdata, load_addr, size, type);
	}
	else if (pfe_pe_is_imem(pe, load_addr, size))
	{
		/*	Section belongs to IMEM */
		ret_val = pfe_pe_load_imem_section(pe, sdata, load_addr, size, type);
	}
	else if (pfe_pe_is_lmem(pe, load_addr, size))
	{
		/*	Section belongs to LMEM */
		NXP_LOG_ERROR("LMEM not supported (yet)\n");
		ret_val = EINVAL;
	}
	else
	{
		NXP_LOG_ERROR("Unsupported memory range %p\n", (void *)load_addr);
		ret_val = EINVAL;
	}

	return ret_val;
}

/**
 * @brief Translates section virtual address into load address
 * @param[in] elf_file Elf file containing the section to translate the address
 * @param[in] shdr Section header of the section to translate the address
 * @details Elf file section header contains only section virtual address which is used by the
 *          running software. The virtual address needs to be translated to load address which
 *          is address where the section is loaded into memory. In most cases the virtual and
 *          load address are equal.
 * @return Load address of the given section or 0 on failure.
 */
static addr_t pfe_pe_get_elf_sect_load_addr(ELF_File_t *elf_file, Elf32_Shdr *shdr)
{
	addr_t virt_addr = shdr->sh_addr;
	addr_t load_addr;
	addr_t offset;
	Elf32_Phdr *phdr;
	uint_t ii;

	/* Go through all program headers to find one containing the section */
	for (ii=0U; ii<elf_file->Header.r32.e_phnum; ii++)
	{
		phdr = &elf_file->arProgHead32[ii];
		if((virt_addr >= phdr->p_vaddr) &&
		   (virt_addr <= (phdr->p_vaddr + phdr->p_memsz - shdr->sh_size)))
		{   /* Address belongs into this segment */
			/* Calculate the offset between segment load and virtual address */
			offset = phdr->p_vaddr - phdr->p_paddr;
			/* Same offset applies also for sections in the segment */
			load_addr = virt_addr - offset;
			return load_addr;
		}
	}
	/* No segment containing the section was found ! */
	NXP_LOG_ERROR("Translation of 0x%"PRINTADDR_T"x failed, fallback used\n", virt_addr);

	return 0;
}

/**
 * @brief		Create new PE instance
 * @param[in]	cbus_base_va CBUS base address (virtual)
 * @param[in]	type Type of PE to create @see pfe_pe_type_t
 * @param[in]	id PE ID
 * @return		The PE instance or NULL if failed
 */
pfe_pe_t * pfe_pe_create(void *cbus_base_va, pfe_ct_pe_type_t type, uint8_t id)
{
	pfe_pe_t *pe = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if ((type != PE_TYPE_INVALID) && (type < PE_TYPE_MAX))
	{
		pe = oal_mm_malloc(sizeof(pfe_pe_t));

		if (NULL != pe)
		{
			(void)memset(pe, 0, sizeof(pfe_pe_t));
			pe->type = type;
			pe->cbus_base_va = cbus_base_va;
			pe->id = id;
			pe->fw_err_section = NULL;
			pe->mmap_data = NULL;
			if(EOK != oal_mutex_init(&pe->lock_mutex))
			{
				NXP_LOG_DEBUG("Mutex init failed\n");
				oal_mm_free(pe);
				pe = NULL;
			}

			if (FALSE == mem_access_lock_init)
			{
				if (EOK == oal_mutex_init(&mem_access_lock))
				{
					mem_access_lock_init = TRUE;
				}
				else
				{
					NXP_LOG_DEBUG("Mutex (mem_access_lock) init failed\n");
					if (NULL != pe)
					{
						(void)oal_mutex_destroy(&pe->lock_mutex);
						oal_mm_free(pe);
						pe = NULL;
					}
				}
			}
		}
	}

	return pe;
}

/**
 * @brief		Set DMEM base address for .elf mapping
 * @warning		Not intended to be called when PE is running
 * @param[in]	pe The PE instance
 * @param[in]	elf_base DMEM base virtual address within .elf
 * @param[in]	len DMEM memory length
 */
void pfe_pe_set_dmem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->dmem_elf_base_va = elf_base;
	pe->dmem_size = len;
	pfe_pe_mem_memset_nolock(pe, PFE_PE_DMEM, 0U, 0U, len);
}

/**
 * @brief		Set IMEM base address for .elf mapping
 * @warning		Not intended to be called when PE is running
 * @param[in]	pe The PE instance
 * @param[in]	elf_base_va IMEM base virtual address within .elf
 * @param[in]	len IMEM memory length
 */
void pfe_pe_set_imem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->imem_elf_base_va = elf_base;
	pe->imem_size = len;
	pfe_pe_mem_memset_nolock(pe, PFE_PE_IMEM, 0U, 0U, len);
}

/**
 * @brief		Set LMEM base address
 * @param[in]	pe The PE instance
 * @param[in]	elf_base_va LMEM base virtual address within .elf
 * @param[in]	len LMEM memory length
 */
void pfe_pe_set_lmem(pfe_pe_t *pe, addr_t elf_base, addr_t len)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->ddr_base_addr_pa = base_pa;
	pe->ddr_base_addr_va = base_va;
	pe->ddr_size = len;
}

/**
 * @brief		Set indirect access registers
 * @param[in]	pe The PE instance
 * @param[in]	wdata_reg The WDATA register address as appears on CBUS
 * @param[in]	rdata_reg The RDATA register address as appears on CBUS
 * @param[in]	addr_reg The ADDR register address as appears on CBUS
 */
void pfe_pe_set_iaccess(pfe_pe_t *pe, uint32_t wdata_reg, uint32_t rdata_reg, uint32_t addr_reg)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pe->mem_access_addr = (void *)((addr_t)pe->cbus_base_va + addr_reg);
	pe->mem_access_rdata = (void *)((addr_t)pe->cbus_base_va + rdata_reg);
	pe->mem_access_wdata = (void *)((addr_t)pe->cbus_base_va + wdata_reg);
}

/**
 * @brief Sets the PE ID in the FW
 * @param[in] pe The PE which ID shall be set in the FW
 * @return EOK if success, error code otherwise
 */
errno_t pfe_pe_set_number(pfe_pe_t *pe)
{
	if(NULL == pe->mmap_data)
	{
		NXP_LOG_WARNING("Memory map is not known\n");
		return ENOENT;
	}

	pfe_pe_memcpy_from_host_to_dmem_32_nolock(pe, oal_ntohl(pe->mmap_data->common.pe_id), &pe->id, sizeof(uint8_t));

	return EOK;
}

static void print_fw_issue(pfe_ct_pe_mmap_t *fw_mmap)
{
#ifdef NXP_LOG_ENABLED 
	NXP_LOG_ERROR("Unsupported firmware detected: Found revision %d.%d.%d (fwAPI:%s), required fwAPI %s\n",
			fw_mmap->common.version.major, fw_mmap->common.version.minor, fw_mmap->common.version.patch, fw_mmap->common.version.cthdr,
			TOSTRING(PFE_CFG_PFE_CT_H_MD5));
#else
	(void)fw_mmap;
#endif
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
	addr_t load_addr;
	void *buf;
	errno_t ret;
	uint32_t section_idx;
	ELF_File_t *elf_file = (ELF_File_t *)elf;
	Elf32_Shdr *shdr = NULL;
	static pfe_ct_pe_mmap_t *tmp_mmap = NULL;
    uint32_t mmap_size;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == elf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Attempt to get section containing firmware memory map data */
	if (TRUE == ELF_SectFindName(elf_file, ".pfe_pe_mmap", &section_idx, NULL, NULL))
	{
		/*	Load section to RAM */
		shdr = &elf_file->arSectHead32[section_idx];
        /* Get the mmap size */
        (void)memcpy((void*)&mmap_size, (const void*)(elf_file->pvData + shdr->sh_offset), sizeof(uint32_t));
        /* Convert mmap size endian ! */
        mmap_size = oal_ntohl(mmap_size);
        tmp_mmap = (pfe_ct_pe_mmap_t *)oal_mm_malloc(mmap_size);
		if (NULL == tmp_mmap)
		{
			ret = ENOMEM;
			goto free_and_fail;
		}
		else
		{
			/*  Firmware version check */
			static const char_t mmap_version_str[] = TOSTRING(PFE_CFG_PFE_CT_H_MD5);
			(void)memcpy((void*)tmp_mmap, (const void*)(elf_file->pvData + shdr->sh_offset), mmap_size);
			if(0 != strcmp(mmap_version_str, tmp_mmap->common.version.cthdr))
			{
				ret = EINVAL;
				print_fw_issue(tmp_mmap);
				goto free_and_fail;
			}
			NXP_LOG_INFO("pfe_ct.h file version\"%s\"\n",mmap_version_str);
			/*	Indicate that mmap_data is available */
			pe->mmap_data = tmp_mmap;
		}
	}
	else
	{
		NXP_LOG_WARNING("Section not found (.pfe_pe_mmap). Memory map will not be available.\n");
	}

	/*	Attempt to get section containing firmware diagnostic data */
	if (TRUE == ELF_SectFindName(elf_file, ".errors", &section_idx, NULL, NULL))
	{
		/*	Load section to RAM */
		shdr = &elf_file->arSectHead32[section_idx];
		buf = oal_mm_malloc(shdr->sh_size);
		if (NULL == buf)
		{
			ret = ENOMEM;
			goto free_and_fail;
		}
		else
		{
			(void)memcpy(buf, elf_file->pvData + shdr->sh_offset, shdr->sh_size);
			pe->fw_err_section_size = shdr->sh_size;
			/*	Indicate that fw_err_section is available */
			pe->fw_err_section = buf;
		}
	}
	else
	{
		NXP_LOG_WARNING("Section not found (.errors). FW error reporting will not be available.\n");
	}


	/*	Attempt to get section containing firmware supported features */
	if (TRUE == ELF_SectFindName(elf_file, ".features", &section_idx, NULL, NULL))
	{
		/*	Load section to RAM */
		shdr = &elf_file->arSectHead32[section_idx];
		buf = oal_mm_malloc(shdr->sh_size);
		if (NULL == buf)
		{
			ret = ENOMEM;
			goto free_and_fail;
		}
		else
		{
			memcpy(buf, elf_file->pvData + shdr->sh_offset, shdr->sh_size);
			pe->fw_feature_section_size = shdr->sh_size;
			/*	Indicate that fw_feature_section is available */
			pe->fw_feature_section = buf;
			pe->fw_features_base = INVALID_FEATURES_BASE; /* Invalid value */
		}
	}
	else
	{
		NXP_LOG_WARNING("Section not found (.features). FW features management will not be available.\n");
	}

	/*	.elf data must be in BIG ENDIAN */
	if (1U == elf_file->Header.e_ident[EI_DATA])
	{
		NXP_LOG_DEBUG("Unexpected .elf format (little endian)\n");
		ret = EINVAL;
		goto free_and_fail;
	}

	/*	Try to upload all sections of the .elf */
	for (ii=0U; ii<elf_file->Header.r32.e_shnum; ii++)
	{
		if (!(elf_file->arSectHead32[ii].sh_flags & (uint32_t)(((uint32_t)SHF_WRITE) | ((uint32_t)SHF_ALLOC) | ((uint32_t)SHF_EXECINSTR))))
		{
			/*	Skip the section */
			continue;
		}

		buf = elf_file->pvData + elf_file->arSectHead32[ii].sh_offset;
		/* Translate elf virtual address to load address */
		load_addr = pfe_pe_get_elf_sect_load_addr(elf_file, &elf_file->arSectHead32[ii]);
		if(0U == load_addr)
		{	/* Failed */
			ret = EINVAL;
			goto free_and_fail;
		}

		/*	Upload the section */
		ret = pfe_pe_load_elf_section(pe, buf, load_addr, elf_file->arSectHead32[ii].sh_size, elf_file->arSectHead32[ii].sh_type);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Couldn't upload firmware section %s, %d bytes @ 0x%08x. Reason: %d\n",
							elf_file->acSectNames+elf_file->arSectHead32[ii].sh_name,
							elf_file->arSectHead32[ii].sh_size,
							elf_file->arSectHead32[ii].sh_addr, ret);
			goto free_and_fail;
		}
	}

	/* Clear the internal copy of the index on each FW load because
	   FW will also start from 0 */
	pe->last_error_write_index = 0U;
	pe->error_record_addr = 0U;
	/* Set the PE number in the FW */
	(void)pfe_pe_set_number(pe);
	return EOK;

free_and_fail:
	if (NULL != pe->mmap_data)
	{
		oal_mm_free(pe->mmap_data);
		pe->mmap_data = NULL;
	}

	if (NULL != pe->fw_err_section)
	{
		oal_mm_free(pe->fw_err_section);
		pe->fw_err_section = NULL;
		pe->fw_err_section_size = 0U;
	}

	if (NULL != pe->fw_feature_section)
	{
		oal_mm_free(pe->fw_feature_section);
		pe->fw_feature_section = NULL;
		pe->fw_feature_section_size = 0U;
	}

	return ret;
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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == pe) || (NULL == mmap)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (NULL != pe->mmap_data)
	{
		(void)memcpy(mmap, pe->mmap_data, sizeof(pfe_ct_pe_mmap_t));
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
		if (NULL != pe->mmap_data)
		{
			oal_mm_free(pe->mmap_data);
			pe->mmap_data = NULL;
		}

		if (NULL != pe->fw_err_section)
		{
			oal_mm_free(pe->fw_err_section);
			pe->fw_err_section = NULL;
			pe->fw_err_section_size = 0U;
		}

		if (NULL != pe->fw_feature_section)
		{
			oal_mm_free(pe->fw_feature_section);
			pe->fw_feature_section = NULL;
			pe->fw_feature_section_size = 0U;
		}

		(void)oal_mutex_destroy(&pe->lock_mutex);
		oal_mm_free(pe);
	}
}

/**
* @brief Returns a string base from the features description section
* @param[in] pe PE to be used
* @return Either the string base or NULL.
*/
char *pfe_pe_get_fw_feature_str_base(pfe_pe_t *pe)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	if(INVALID_FEATURES_BASE != pe->fw_features_base)
	{
		return (char *)(pe->fw_feature_section);
	}
	return NULL;
}

/**
* @brief Returns feature description from special .elf section
* @param[in] pe PE to be used
* @param[in] id Id of the feature - its position in the section.
* @param[out] entry Pointer to the description is stored here
* @retun Either error code on failure or EOK
*/
errno_t pfe_pe_get_fw_feature_entry(pfe_pe_t *pe, uint32_t id, pfe_ct_feature_desc_t **entry)
{
	uint32_t entry_ptr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == pe))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Check whether the section with features description is available */
	if(NULL == pe->fw_feature_section)
	{	/* Avoid running uninitialized */
		return ENOENT;
	}
	/* Get the pointer to the descriptions and count of the features
	   (do it only once and remember the values)
	*/
	if(INVALID_FEATURES_BASE == pe->fw_features_base)
	{   /* The mmap has not been queried for error record yet */
		pfe_ct_pe_mmap_t pfe_pe_mmap;
		/* Query map for the error record address */
		if (EOK != pfe_pe_get_mmap(pe, &pfe_pe_mmap))
		{
			NXP_LOG_ERROR("Could not get memory map\n");
			return ENOENT;
		}
		/* Remember the features record address and size */
		pe->fw_features_base = oal_ntohl(pfe_pe_mmap.common.version.features);
		if(pe->fw_features_base > pe->fw_feature_section_size)
		{
			NXP_LOG_ERROR("Invalid address of features record 0x%x\n", pe->fw_features_base);
			pe->fw_features_base = INVALID_FEATURES_BASE;
			return EIO;
		}
		pe->fw_features_size = oal_ntohl(pfe_pe_mmap.common.version.features_count);
	}

	/* Check if the requested id does exist */
	if(id < pe->fw_features_size)
	{   /* Entry at given id does exist */
		entry_ptr = oal_ntohl(*(uint32_t *)((addr_t)pe->fw_feature_section + (pe->fw_features_base + (id * sizeof(PFE_PTR(pfe_ct_feature_desc_t))))));
		*entry = (pfe_ct_feature_desc_t *)((addr_t)pe->fw_feature_section + entry_ptr);
		return EOK;
	}
	return ENOENT;
}

/**
 * @brief		Reads out errors reported by the PE Firmware and prints them on debug console
 * @param[in]	pe PE which error report shall be read out
 * @return EOK on success or error code
 */
errno_t pfe_pe_get_fw_errors(pfe_pe_t *pe)
{
	pfe_ct_error_record_t error_record; /* Copy of the PE error record */
	uint32_t read_start;                /* Starting position in error record to read */
	uint32_t i;
	uint32_t errors_count;

	if(NULL == pe->fw_err_section)
	{	/* Avoid running uninitialized */
		return ENOENT;
	}

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
		pe->error_record_addr = oal_ntohl(pfe_pe_mmap.common.error_record);
	}

	/* Copy error record from PE to local memory */
	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &error_record, pe->error_record_addr, sizeof(pfe_ct_error_record_t));
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
			 uint32_t error_val;

			error_addr = oal_ntohl(error_record.errors[(read_start + i) & (FP_ERROR_RECORD_SIZE - 1U)]);
			error_val = oal_ntohl(error_record.values[(read_start + i) & (FP_ERROR_RECORD_SIZE - 1U)]);
			if(error_addr > pe->fw_err_section_size)
			{
				NXP_LOG_ERROR("Invalid error address from FW 0x%x\n", error_addr);
				break;
			}
			/* Get to the error message through the .errors section */
			error_ptr = pe->fw_err_section + error_addr;
			if(oal_ntohl(error_ptr->message) > pe->fw_err_section_size)
			{
				NXP_LOG_ERROR("Invalid error message from FW 0x%x", oal_ntohl(error_ptr->message));
				break;
			}
			error_str = pe->fw_err_section + oal_ntohl(error_ptr->message);
			if(oal_ntohl(error_ptr->file) > pe->fw_err_section_size)
			{
				NXP_LOG_ERROR("Invalid file name from FW 0x%x", oal_ntohl(error_ptr->file));
				break;
			}
			error_file =  pe->fw_err_section + oal_ntohl(error_ptr->file);
			error_line = oal_ntohl(error_ptr->line);
			NXP_LOG_ERROR("PE%d: %s line %u: %s (0x%x)\n", pe->id, error_file, error_line, error_str, error_val);
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

	NXP_LOG_INFO("[FW VERSION] %d.%d.%d, Build: %s, %s (%s), ID: 0x%x\n",
			pfe_pe_mmap.common.version.major,
			pfe_pe_mmap.common.version.minor,
			pfe_pe_mmap.common.version.patch,
			(char_t *)pfe_pe_mmap.common.version.build_date,
			(char_t *)pfe_pe_mmap.common.version.build_time,
			(char_t *)pfe_pe_mmap.common.version.vctrl,
			pfe_pe_mmap.common.version.id);

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
#if defined(PFE_CFG_NULL_ARG_CHECK)
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
#endif /* PFE_CFG_NULL_ARG_CHECK */
	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, stats, addr, sizeof(pfe_ct_pe_stats_t));
	return EOK;
}

/**
 * @brief		Copies PE classification algorithms statistics into a prepared buffer
 * @param[in]	pe		PE which statistics shall be read
 * @param[in]	addr	Address within the PE DMEM where the statistics are located
 * @param[out]	stats	Buffer where to copy the statistics from the PE DMEM
 * @retval		EOK		Success
 * @retval		EINVAL	Invalid argument
 */
errno_t pfe_pe_get_classify_stats(pfe_pe_t *pe, uint32_t addr, pfe_ct_classify_stats_t *stats)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
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
#endif /* PFE_CFG_NULL_ARG_CHECK */
	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, stats, addr, sizeof(pfe_ct_classify_stats_t));
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
#if defined(PFE_CFG_NULL_ARG_CHECK)
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
#endif /* PFE_CFG_NULL_ARG_CHECK */
	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, stats, addr, sizeof(pfe_ct_class_algo_stats_t));
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
#if defined(PFE_CFG_NULL_ARG_CHECK)
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
 * @brief Translates state pfe_ct_pe_sw_state_t to a string
 * @param[in] state State to be translated
 * @return String representation of the state
 */
static inline const char_t *pfe_pe_get_fw_state_str(pfe_ct_pe_sw_state_t state)
{
	const char *ret;
	switch(state)
	{
		case PFE_FW_STATE_UNINIT:
			ret = "UNINIT";
			break;
		case PFE_FW_STATE_INIT:
			ret = "INIT";
			break;
		case PFE_FW_STATE_FRAMEWAIT:
			ret = "FRAMEWAIT";
			break;
		case PFE_FW_STATE_FRAMEPARSE:
			ret = "FRAMEPARSE";
			break;
		case PFE_FW_STATE_FRAMECLASSIFY:
			ret = "FRAMECLASSIFY";
			break;
		case PFE_FW_STATE_FRAMEDISCARD:
			ret = "FRAMEDISCARD";
			break;
		case PFE_FW_STATE_FRAMEMODIFY:
			ret = "FRAMEMODIFY";
			break;
		case PFE_FW_STATE_FRAMESEND:
			ret = "FRAMESEND";
			break;
		case PFE_FW_STATE_STOPPED:
			ret = "STOPPED";
			break;
		default:
			ret = "Unknown";
			break;
	}
	return ret;
}

/**
* @brief Reads and prints measurements from the PE memory
* @param[in] pe PE which shall be read
* @param[in] count Number of measurements in the PE memory to be read
* @param[in] ptr Location of the measurements record in the PE memory
* @param[in] buf Output text data buffer
* @param[in] buf_len Size of the output text data buffer
* @param[in] verb_level Verbosity level
* @return Number of bytes written into the text buffer
*/
static uint32_t pfe_pe_get_measurements(pfe_pe_t *pe, uint32_t count, uint32_t ptr, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	pfe_ct_measurement_t *m = NULL;
	uint_t i;
	uint32_t len = 0U;

    (void)verb_level;
	if(0U == ptr)
	{   /* This shall not happen - FW did not initialize data correctly */
		NXP_LOG_ERROR("Inconsistent data in pfe_pe_mmap\n");
		return 0U;
	}

	/* Get buffer to read data from DMEM */
	m = oal_mm_malloc(sizeof(pfe_ct_measurement_t) * count);
	if(NULL == m)
	{   /* Memory allocation failed */
	    return 0U;
	}
	/* Copy the data into the allocated buffer */
	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, m, ptr, sizeof(pfe_ct_measurement_t) * count);
	/* Print the data */
	for(i = 0U; i < count; i++)
	{
		/* Variables just to make code more readable */
		uint32_t avg = oal_ntohl(m[i].avg);
		uint32_t min = oal_ntohl(m[i].min);
		uint32_t max = oal_ntohl(m[i].max);
		uint32_t cnt = oal_ntohl(m[i].cnt);
		/* Just print the data without interpreting them */
		len += oal_util_snprintf(buf + len, buf_len - len, "Measurement %u:\tmin %10u\tmax %10u\tavg %10u\tcnt %10u\n", i, min, max, avg, cnt);
	}
	/* Free the allocated buffer */
	oal_mm_free(m);

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
	pfe_ct_pe_sw_state_monitor_t state_monitor;

	/* Get the pfe_ct_pe_mmap_t structure from PE */
	if (NULL == pe->mmap_data)
	{
		return 0U;
	}
	len += oal_util_snprintf(buf + len, buf_len - len, "\nPE %u\n----\n", pe->id);
	len += oal_util_snprintf(buf + len, buf_len - len, "- PE state monitor -\n");
	pfe_pe_memcpy_from_dmem_to_host_32_nolock(pe, &state_monitor, oal_ntohl(pe->mmap_data->common.state_monitor), sizeof(pfe_ct_pe_sw_state_monitor_t));
	len += oal_util_snprintf(buf + len, buf_len - len, "FW State: %u (%s), counter %u\n",state_monitor.state,
	                         pfe_pe_get_fw_state_str(state_monitor.state), oal_ntohl(state_monitor.counter));

    /* This is a class PE therefore we may access the specific data */
	if(0U != oal_ntohl(pe->mmap_data->common.measurement_count))
	{   /* The FW provides processing time measurements */
		len += oal_util_snprintf(buf + len, buf_len - len, "- Measurements -\n");
		len += pfe_pe_get_measurements(pe, oal_ntohl(pe->mmap_data->common.measurement_count),
		                                oal_ntohl(pe->mmap_data->common.measurements),
		                                buf + len, buf_len - len, verb_level);
	}

	return len;
}
