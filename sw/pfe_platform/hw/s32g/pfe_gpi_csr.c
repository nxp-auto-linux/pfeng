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
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_gpi_csr.h"
#include "pfe_bmu_csr.h"
#include "pfe_class_csr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

static void pfe_gpi_cfg_init_inqos(addr_t base_va);

/**
 * @brief		Initialize ingress QoS module
 * @param[in]	base_va Base address of GPI register space (virtual)
 */
static void pfe_gpi_cfg_init_inqos(addr_t base_va)
{
	uint32_t ii;
	uint32_t val;

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
 * @param[in]	cbus_va CBUS base address (virtual)
 * @param[in]	base_va Base address of GPI register space (virtual)
 * @return		EOK if success, error code if invalid configuration is detected
 */
errno_t pfe_gpi_cfg_init(addr_t cbus_va, addr_t base_va, const pfe_gpi_cfg_t *cfg)
{
	addr_t gpi_cbus_offset = base_va - cbus_va;

	switch (gpi_cbus_offset)
	{
		case CBUS_EGPI1_BASE_ADDR:
		case CBUS_EGPI2_BASE_ADDR:
		case CBUS_EGPI3_BASE_ADDR:
			pfe_gpi_cfg_init_inqos(base_va);
			break;
		default:
			/*Do Nothing*/
			break;
	}

	hal_write32(0x0U, base_va + GPI_EMAC_1588_TIMESTAMP_EN);
	if (cfg->emac_1588_ts_en)
	{
		hal_write32(0xe01U, base_va + GPI_EMAC_1588_TIMESTAMP_EN);
	}

	hal_write32(((cfg->alloc_retry_cycles << 16) | GPI_DDR_BUF_EN | GPI_LMEM_BUF_EN), base_va + GPI_RX_CONFIG);
	hal_write32((PFE_CFG_DDR_HDR_SIZE << 16) | PFE_CFG_LMEM_HDR_SIZE, base_va + GPI_HDR_SIZE);
	hal_write32((PFE_CFG_DDR_BUF_SIZE << 16) | PFE_CFG_LMEM_BUF_SIZE, base_va + GPI_BUF_SIZE);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_ALLOC_CTRL, base_va + GPI_LMEM_ALLOC_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_FREE_CTRL, base_va + GPI_LMEM_FREE_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU2_BASE_ADDR + BMU_ALLOC_CTRL, base_va + GPI_DDR_ALLOC_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU2_BASE_ADDR + BMU_FREE_CTRL, base_va + GPI_DDR_FREE_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CLASS_INQ_PKTPTR, base_va + GPI_CLASS_ADDR);
	hal_write32(PFE_CFG_DDR_HDR_SIZE, base_va + GPI_DDR_DATA_OFFSET);
	hal_write32(0x30U, base_va + GPI_LMEM_DATA_OFFSET);
	hal_write32(PFE_CFG_LMEM_HDR_SIZE, base_va + GPI_LMEM_SEC_BUF_DATA_OFFSET);
	hal_write32(cfg->gpi_tmlf_txthres, base_va + GPI_TMLF_TX);
	hal_write32(cfg->gpi_dtx_aseq_len, base_va + GPI_DTX_ASEQ);
	hal_write32(1, base_va + GPI_CSR_TOE_CHKSUM_EN);

	return EOK;
}

/**
 * @brief		Reset the GPI
 * @param[in]	base_va Base address of GPI register space (virtual)
 * @retval		EOK Success
 * @retval		ETIMEDOUT Reset procedure timed-out
 */
errno_t pfe_gpi_cfg_reset(addr_t base_va)
{
	uint32_t timeout = 20U;
	uint32_t reg = hal_read32(base_va + GPI_CTRL);

	reg |= 0x2U;

	hal_write32(reg, base_va + GPI_CTRL);

	do
	{
		oal_time_usleep(100U);
		reg = hal_read32(base_va + GPI_CTRL);
		--timeout;
	} while (((reg & 0x2U) != 0U) && (timeout > 0U));

	if (0U == timeout)
	{
		return ETIMEDOUT;
	}

	return EOK;
}

/**
 * @brief		Enable the GPI module
 * @param[in]	base_va Base address of GPI register space (virtual)
 */
void pfe_gpi_cfg_enable(addr_t base_va)
{
	uint32_t reg = hal_read32(base_va + GPI_CTRL);

	hal_write32(reg | 0x1U, base_va + GPI_CTRL);
}

/**
 * @brief		Disable the GPI module
 * @param[in]	base_va Base address of GPI register space (virtual)
 */
void pfe_gpi_cfg_disable(addr_t base_va)
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
uint32_t pfe_gpi_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Debug registers */
	if(verb_level >= 10U)
	{
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_FIFO_DEBUG   : 0x%x\n", hal_read32(base_va + GPI_FIFO_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_TX_DBUG_REG1 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG1));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_TX_DBUG_REG2 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG2));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_TX_DBUG_REG3 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG3));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_TX_DBUG_REG4 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG4));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_TX_DBUG_REG5 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG5));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_TX_DBUG_REG6 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG6));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_FIFO_DEBUG   : 0x%x\n", hal_read32(base_va + GPI_FIFO_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_FIFO_DEBUG   : 0x%x\n", hal_read32(base_va + GPI_FIFO_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_FIFO_DEBUG   : 0x%x\n", hal_read32(base_va + GPI_FIFO_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_FIFO_DEBUG   : 0x%x\n", hal_read32(base_va + GPI_FIFO_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_RX_DBUG_REG1 : 0x%x\n", hal_read32(base_va + GPI_RX_DBUG_REG1));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_RX_DBUG_REG2 : 0x%x\n", hal_read32(base_va + GPI_RX_DBUG_REG2));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "GPI_FIFO_STATUS  : 0x%x\n", hal_read32(base_va + GPI_FIFO_STATUS));
	}

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
