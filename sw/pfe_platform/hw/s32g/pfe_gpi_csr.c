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
 * @addtogroup  dxgr_PFE_GPI
 * @{
 *
 * @file		pfe_gpi_csr.c
 * @brief		The GPI module low-level API (s32g).
 * @details		Applicable for IP versions listed below. Also applicable
 * 				for ETGPI and HGPI except IGQOS registers.
 */

#include "oal.h"
#include "hal.h"
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_gpi_csr.h"
#include "pfe_bmu_csr.h"
#include "pfe_class_csr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

/*	Supported IPs. Defines are validated within pfe_cbus.h. */
#if (GLOBAL_CFG_IP_VERSION != IP_VERSION_FPGA_5_0_4) && (GLOBAL_CFG_IP_VERSION != IP_VERSION_NPU_7_14)
#error Unsupported IP version
#endif /* GLOBAL_CFG_IP_VERSION */

static void pfe_gpi_cfg_init_inqos(void *base_va);

/**
 * @brief		Initialize ingress QoS module
 * @param[in]	base_va Base address of GPI register space (virtual)
 */
static void pfe_gpi_cfg_init_inqos(void *base_va)
{
	uint32_t ii;
	uint32_t val;

	/*
	 * The way how tables are initialized is proposed way how to do it. From
	 * performance perspective this could be pretty much optimized.
	 */

	/*	Entry table */
	for (ii=0U; ii<IGQOS_ENTRY_TABLE_LEN; ii++)
	{
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG0);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG1);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG2);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG3);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG4);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG5);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG6);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG7);

		val = CMDCNTRL_CMD_WRITE
				| CMDCNTRL_CMD_TAB_ADDR(ii)
				| CMDCNTRL_CMD_TAB_SELECT_ENTRY;
		hal_write32(val, base_va + CSR_IGQOS_ENTRY_CMDCNTRL);
	}

	/*	LRU table */
	for (ii=0U; ii<IGQOS_LRU_TABLE_LEN; ii++)
	{
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG0);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG1);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG2);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG3);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG4);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG5);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG6);
		hal_write32(0U, base_va + CSR_IGQOS_ENTRY_DATA_REG7);

		val = CMDCNTRL_CMD_WRITE
				| CMDCNTRL_CMD_TAB_ADDR(ii)
				| CMDCNTRL_CMD_TAB_SELECT_LRU;
		hal_write32(val, base_va + CSR_IGQOS_ENTRY_CMDCNTRL);
	}
}

/**
 * @brief		HW-specific initialization function
 * @details		This function is called during HW initialization routine and should
 * 				ensure that all necessary values are correctly configured before
 * 				the MAC is enabled.
 * @param[in]	cbus_va CBUS base address (virtual)
 * @param[in]	base_va Base address of GPI register space (virtual)
 * @return		EOK if success, error code if invalid configuration is detected
 */
errno_t pfe_gpi_cfg_init(void *cbus_va, void *base_va, pfe_gpi_cfg_t *cfg)
{
	uint32_t reg;
	addr_t gpi_cbus_offset = (addr_t)base_va - (addr_t)cbus_va;

	switch (gpi_cbus_offset)
	{
		case CBUS_EGPI1_BASE_ADDR:
		case CBUS_EGPI2_BASE_ADDR:
		case CBUS_EGPI3_BASE_ADDR:
		{
			/*	Initialize ingress QoS entry table */
			NXP_LOG_DEBUG("GPI (@cbus 0x%x): Initializing INQOS memory\n", (uint32_t)gpi_cbus_offset);
			pfe_gpi_cfg_init_inqos(base_va);
			break;
		}

		default:
		{
			break;
		}
	}

	/*	GPI_EMAC_1588_TIMESTAMP_EN */
	hal_write32(0x0U, base_va + GPI_EMAC_1588_TIMESTAMP_EN); /* ENABLE PADDING AND CRC(FSC) INSERTION */

	/*	GPI_EMAC_1588_TIMESTAMP_EN */
	if (cfg->emac_1588_ts_en)
	{
		/*	Enable 1588 timestamp, CRC insertion(?), tx pbl = 7 */
		reg = hal_read32(base_va + GPI_EMAC_1588_TIMESTAMP_EN);
		reg |= 0xe01U;
		hal_write32(reg, base_va + GPI_EMAC_1588_TIMESTAMP_EN);
	}

	/*	GPI_RX_CONFIG */
	hal_write32(((cfg->alloc_retry_cycles << 16) | GPI_DDR_BUF_EN | GPI_LMEM_BUF_EN), base_va + GPI_RX_CONFIG);

	/*	GPI_HDR_SIZE */
	hal_write32((PFE_CFG_DDR_HDR_SIZE << 16) | PFE_CFG_LMEM_HDR_SIZE, base_va + GPI_HDR_SIZE);

	/*	GPI_BUF_SIZE */
	hal_write32((PFE_CFG_DDR_BUF_SIZE << 16) | PFE_CFG_LMEM_BUF_SIZE, base_va + GPI_BUF_SIZE);

	/*	GPI_LMEM_ALLOC_ADDR */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_ALLOC_CTRL, base_va + GPI_LMEM_ALLOC_ADDR);

	/*	GPI_LMEM_FREE_ADDR */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_FREE_CTRL, base_va + GPI_LMEM_FREE_ADDR);

	/*	GPI_DDR_ALLOC_ADDR */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU2_BASE_ADDR + BMU_ALLOC_CTRL, base_va + GPI_DDR_ALLOC_ADDR);

	/*	GPI_DDR_FREE_ADDR */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU2_BASE_ADDR + BMU_FREE_CTRL, base_va + GPI_DDR_FREE_ADDR);

	/*	GPI_CLASS_ADDR */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CLASS_INQ_PKTPTR, base_va + GPI_CLASS_ADDR);

	/*	GPI_DDR_DATA_OFFSET */
	hal_write32(PFE_CFG_DDR_HDR_SIZE, base_va + GPI_DDR_DATA_OFFSET);

	/*	GPI_LMEM_DATA_OFFSET */
	hal_write32(0x30U, base_va + GPI_LMEM_DATA_OFFSET);

	/*	GPI_LMEM_SEC_BUF_DATA_OFFSET */
	hal_write32(PFE_CFG_LMEM_HDR_SIZE, base_va + GPI_LMEM_SEC_BUF_DATA_OFFSET);

	/*	GPI_TMLF_TX */
	hal_write32(cfg->gpi_tmlf_txthres, base_va + GPI_TMLF_TX);

	/*	GPI_DTX_ASEQ */
	hal_write32(cfg->gpi_dtx_aseq_len, base_va + GPI_DTX_ASEQ);

	/*	IP/TCP/UDP Checksum Offload */
	hal_write32(1, base_va + GPI_CSR_TOE_CHKSUM_EN);

	return EOK;
}

/**
 * @brief		Reset the GPI
 * @details		Function initiates reset of the given GPI module and waits
 * 				until it is done.
 * @param[in]	base_va Base address of GPI register space (virtual)
 * @retval		EOK Success
 * @retval		ETIMEDOUT Reset procedure timed-out
 */
errno_t pfe_gpi_cfg_reset(void *base_va)
{
	uint32_t timeout = 20U;
	uint32_t reg = hal_read32(base_va + GPI_CTRL);

	reg |= 0x2U;

	hal_write32(reg, base_va + GPI_CTRL);

	do
	{
		oal_time_usleep(100U);
		reg = hal_read32(base_va + GPI_CTRL);
	} while ((reg & 0x2U) && (--timeout > 0U));

	if (0U == timeout)
	{
		return ETIMEDOUT;
	}

	return EOK;
}

/**
 * @brief		Enable the GPI module
 * @details
 * @param[in]	base_va Base address of GPI register space (virtual)
 */
void pfe_gpi_cfg_enable(void *base_va)
{
	uint32_t reg = hal_read32(base_va + GPI_CTRL);

	hal_write32(reg | 0x1U, base_va + GPI_CTRL);
}

/**
 * @brief		Disable the GPI module
 * @details
 * @param[in]	base_va Base address of GPI register space (virtual)
 */
void pfe_gpi_cfg_disable(void *base_va)
{
	uint32_t reg = hal_read32(base_va + GPI_CTRL);

	hal_write32(reg & ~(0x1U), base_va + GPI_CTRL);
}

/**
 * @brief		Get GPI statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the GPI block.
 * @param[in]	base_va 	Base address of GPI register space (virtual)
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_gpi_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Get version */
	if(verb_level >= 9U)
	{
		reg = hal_read32(base_va + GPI_VERSION);
		len += oal_util_snprintf(buf + len, size - len, "Revision             : 0x%x\n", (reg >> 24) & 0xffU);
		len += oal_util_snprintf(buf + len, size - len, "Version              : 0x%x\n", (reg >> 16) & 0xffU);
		len += oal_util_snprintf(buf + len, size - len, "ID                   : 0x%x\n", reg & 0xffffU);
	}

	reg = hal_read32(base_va + GPI_FIFO_STATUS);
	len += oal_util_snprintf(buf + len, size - len, "TX Underrun          : 0x%x\n", reg);
	hal_write32(0, base_va + GPI_FIFO_STATUS);

	reg = hal_read32(base_va + GPI_FIFO_DEBUG);
	len += oal_util_snprintf(buf + len, size - len, "TX FIFO Packets      : 0x%x\n", reg & 0x1fU);
	len += oal_util_snprintf(buf + len, size - len, "RX FIFO Packets      : 0x%x\n", (reg >> 6) & 0x1fU);
	len += oal_util_snprintf(buf + len, size - len, "TX FIFO Level        : 0x%x\n", (reg >> 12) & 0xffU);
	len += oal_util_snprintf(buf + len, size - len, "RX FIFO Level        : 0x%x\n", (reg >> 20) & 0xffU);

	reg = hal_read32(base_va + GPI_DTX_ASEQ);
	len += oal_util_snprintf(buf + len, size - len, "ASEQ Length          : 0x%x\n", reg);

	reg = hal_read32(base_va + GPI_EMAC_1588_TIMESTAMP_EN);
	len += oal_util_snprintf(buf + len, size - len, "1588 Enable register : 0x%x\n", reg);

	reg = hal_read32(base_va + GPI_OVERRUN_DROPCNT);
	len += oal_util_snprintf(buf + len, size - len, "Overrun Drop Counter : 0x%x\n", reg);
	hal_write32(0, base_va + GPI_OVERRUN_DROPCNT);

	return len;
}

/** @}*/
