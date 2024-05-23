/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
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
#include "pfe_feature_mgr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static void igqos_class_read_entry_data(addr_t base_va, uint32_t entry[]);
static void igqos_class_prepare_entry_data(addr_t base_va, const uint32_t entry[]);
static void igqos_class_clear_entry_data(addr_t base_va);
static void igqos_class_request_entry_cmd(addr_t base_va, bool_t write, bool_t is_lru, uint32_t addr);
static void igqos_class_write_flow_cmd(addr_t base_va, uint32_t addr);
static void igqos_class_read_flow_cmd(addr_t base_va, uint32_t addr);
static void igqos_class_write_lru_cmd(addr_t base_va, uint32_t addr);
static uint32_t igqos_wred_queue_enable_bit(pfe_iqos_queue_t queue);

static void igqos_class_read_entry_data(addr_t base_va, uint32_t entry[])
{
	uint32_t ii;

	for (ii = 0; ii < ENTRY_DATA_REG_CNT; ii++)
	{
		entry[ii] = hal_read32(base_va + CSR_IGQOS_ENTRY_DATA_REG(ii));
	}
}

static void igqos_class_prepare_entry_data(addr_t base_va, const uint32_t entry[])
{
	uint32_t ii;

	for (ii = 0; ii < ENTRY_DATA_REG_CNT; ii++)
	{
		hal_write32(entry[ii], base_va + CSR_IGQOS_ENTRY_DATA_REG(ii));
	}
}

static void igqos_class_clear_entry_data(addr_t base_va)
{
	uint32_t ii;

	for (ii = 0; ii < ENTRY_DATA_REG_CNT; ii++)
	{
		hal_write32(0, base_va + CSR_IGQOS_ENTRY_DATA_REG(ii));
	}
}

static void igqos_class_request_entry_cmd(addr_t base_va, bool_t write, bool_t is_lru, uint32_t addr)
{
	uint32_t val;

	val = CMDCNTRL_CMD_TAB_ADDR(addr);
	if (TRUE == write)
	{
		val |= CMDCNTRL_CMD_WRITE;
	}
	else
	{
		val |= CMDCNTRL_CMD_READ;
	}

	if (TRUE == is_lru)
	{
		val |= (uint32_t)CMDCNTRL_CMD_TAB_SELECT_LRU;
	}

	hal_write32(val, base_va + CSR_IGQOS_ENTRY_CMDCNTRL);
}

static void igqos_class_write_flow_cmd(addr_t base_va, uint32_t addr)
{
	igqos_class_request_entry_cmd(base_va, TRUE, FALSE, addr);
}

static void igqos_class_read_flow_cmd(addr_t base_va, uint32_t addr)
{
	igqos_class_request_entry_cmd(base_va, FALSE, FALSE, addr);
}

static void igqos_class_write_lru_cmd(addr_t base_va, uint32_t addr)
{
	igqos_class_request_entry_cmd(base_va, TRUE, TRUE, addr);
}

/**
 * @brief		HW-specific initialization function
 * @param[in]	base_va Base address of GPI register space (virtual)
 */
void pfe_gpi_cfg_init(addr_t base_va, const pfe_gpi_cfg_t *cfg)
{
	uint32_t regval;
	hal_write32(0x0U, base_va + GPI_EMAC_1588_TIMESTAMP_EN);
	if (cfg->emac_1588_ts_en)
	{
		hal_write32(0xe01U, base_va + GPI_EMAC_1588_TIMESTAMP_EN);
	}

	hal_write32(((cfg->alloc_retry_cycles << 16) | GPI_DDR_BUF_EN | GPI_LMEM_BUF_EN), base_va + GPI_RX_CONFIG);
	hal_write32((uint32_t)(PFE_CFG_DDR_HDR_SIZE << 16) | cfg->lmem_header_size, base_va + GPI_HDR_SIZE);
	hal_write32((PFE_CFG_DDR_BUF_SIZE << 16) | PFE_CFG_LMEM_BUF_SIZE, base_va + GPI_BUF_SIZE);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_ALLOC_CTRL, base_va + GPI_LMEM_ALLOC_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_FREE_CTRL, base_va + GPI_LMEM_FREE_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU2_BASE_ADDR + BMU_ALLOC_CTRL, base_va + GPI_DDR_ALLOC_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU2_BASE_ADDR + BMU_FREE_CTRL, base_va + GPI_DDR_FREE_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CLASS_INQ_PKTPTR, base_va + GPI_CLASS_ADDR);
	hal_write32(PFE_CFG_DDR_HDR_SIZE, base_va + GPI_DDR_DATA_OFFSET);
	hal_write32(0x30U, base_va + GPI_LMEM_DATA_OFFSET);
	hal_write32(cfg->lmem_header_size, base_va + GPI_LMEM_SEC_BUF_DATA_OFFSET);
	hal_write32(cfg->gpi_tmlf_txthres, base_va + GPI_TMLF_TX);
	hal_write32(cfg->gpi_dtx_aseq_len, base_va + GPI_DTX_ASEQ);
	hal_write32(1, base_va + GPI_CSR_TOE_CHKSUM_EN);
	if (TRUE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3) || cfg->g2_ordered_class_writes)
	{
		regval = hal_read32(base_va + GPI_CSR_AXI_WRITE_DONE_ADDR);
		regval |= 0x3U;
		hal_write32(regval, base_va + GPI_CSR_AXI_WRITE_DONE_ADDR);
	}
	hal_write32(0xFFFFFFFFU, base_va + CSR_IGQOS_LRU_TIMER_VALUE);
}

/**
 * @brief		Reset the GPI
 * @param[in]	base_va Base address of GPI register space (virtual)
 * @retval		EOK Success
 * @retval		ETIMEDOUT Reset procedure timed-out
 */
errno_t pfe_gpi_cfg_reset(addr_t base_va)
{
	errno_t ret = EOK;
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
		ret = ETIMEDOUT;
	}

	return ret;
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

/* Ingress QoS */

void pfe_gpi_cfg_qos_default_init(addr_t base_va)
{
	/* reset CONTROL */
	hal_write32(0, base_va + CSR_IGQOS_CONTROL);

	/* reset sub-blocks: wred, shapers */
	pfe_gpi_cfg_wred_default_init(base_va);
	pfe_gpi_cfg_shp_default_init(base_va, 0);
	pfe_gpi_cfg_shp_default_init(base_va, 1);

	/* reset TPID */
	hal_write32(((uint32_t)IGQOS_TPID_DOT1Q << 16) | IGQOS_TPID_DOT1Q, base_va + CSR_IGQOS_TPID);
	/* reset IGQOS CLASS */
	hal_write32(IGQOS_CLASS_TPID0_EN | IGQOS_CLASS_TPID1_EN, base_va + CSR_IGQOS_CLASS);
}

void pfe_gpi_cfg_qos_enable(addr_t base_va)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_CONTROL);

	reg |= (uint32_t)IGQOS_CONTROL_QOS_EN;
	hal_write32(reg, base_va + CSR_IGQOS_CONTROL);
}

void pfe_gpi_cfg_qos_disable(addr_t base_va)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_CONTROL);

	reg &= (uint32_t)(~(uint32_t)IGQOS_CONTROL_QOS_EN);
	hal_write32(reg, base_va + CSR_IGQOS_CONTROL);
}

bool_t pfe_gpi_cfg_qos_is_enabled(addr_t base_va)
{
	bool_t ret = FALSE;
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_CONTROL);

	if ((reg & IGQOS_CONTROL_QOS_EN) == IGQOS_CONTROL_QOS_EN)
	{
		ret = TRUE;
	}

	return ret;
}

/**
 * @brief		Write classification table entry at given address
 * @param[in]	base_va	Base address of GPI register space (virtual)
 * @param[in]	addr	Classification table entry address, from 0 to @CSR_IGQOS_ENTRY_TABLE_LEN - 1
 * @param[in]	entry[]	Table entry data as array of 8 x 32b values
 * @param[in]	base_va Base address of GPI register space (virtual)
 */
void pfe_gpi_cfg_qos_write_flow_entry_req(addr_t base_va, uint32_t addr, const uint32_t entry[])
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		igqos_class_prepare_entry_data(base_va, entry);
		igqos_class_write_flow_cmd(base_va, addr);
	}
}

void pfe_gpi_cfg_qos_clear_flow_entry_req(addr_t base_va, uint32_t addr)
{
	igqos_class_clear_entry_data(base_va);
	igqos_class_write_flow_cmd(base_va, addr);
}

void pfe_gpi_cfg_qos_clear_lru_entry_req(addr_t base_va, uint32_t addr)
{
	igqos_class_clear_entry_data(base_va);
	igqos_class_write_lru_cmd(base_va, addr);
}

void pfe_gpi_cfg_qos_rd_fl_entry_req(addr_t base_va, uint32_t addr)
{
	igqos_class_read_flow_cmd(base_va, addr);
}

void pfe_gpi_cfg_qos_rd_fl_entry_resp(addr_t base_va, uint32_t entry[])
{
	igqos_class_read_entry_data(base_va, entry);
}

bool_t pfe_gpi_cfg_qos_entry_ready(addr_t base_va)
{
	bool_t ret = FALSE;
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_ENTRY_CMDSTATUS);

	if (0U != (reg & 0x1U))
	{
		ret = TRUE;
	}

	return ret;
}

/* WRED configuration */
void pfe_gpi_cfg_wred_default_init(addr_t base_va)
{
	uint32_t val;

	/* reset the IGQOS_QOS register */
	hal_write32(0, base_va + CSR_IGQOS_QOS);

	val  = (uint32_t)PFE_IQOS_WRED_WEIGHT_DEFAULT << 16;
	val |= (uint32_t)PFE_IQOS_WRED_ZONE4_PROB_DEFAULT << 12;
	val |= (uint32_t)PFE_IQOS_WRED_ZONE3_PROB_DEFAULT << 8;
	val |= PFE_IQOS_WRED_ZONE2_PROB_DEFAULT << 4;
	val |= PFE_IQOS_WRED_ZONE1_PROB_DEFAULT;

	hal_write32(val, base_va + CSR_IQGOS_DMEMQ_ZONE_PROB);
	hal_write32(val, base_va + CSR_IGQOS_LMEMQ_ZONE_PROB);
	hal_write32(val, base_va + CSR_IGQOS_RXFQ_ZONE_PROB);

	val = PFE_IQOS_WRED_DMEM_FULL_THR_DEFAULT;
	hal_write32(val, base_va + CSR_IGQOS_DMEMQ_FULL_THRESH);

	val = ((uint32_t)PFE_IQOS_WRED_DMEM_MIN_THR_DEFAULT << 16) |
	      PFE_IQOS_WRED_DMEM_MAX_THR_DEFAULT;
	hal_write32(val, base_va + CSR_IGQOS_DMEMQ_DROP_THRESH);

	val = PFE_IQOS_WRED_FULL_THR_DEFAULT;
	hal_write32(val, base_va + CSR_IGQOS_LMEMQ_FULL_THRESH);
	hal_write32(val, base_va + CSR_IGQOS_RXFQ_FULL_THRESH);

	val = ((uint32_t)PFE_IQOS_WRED_MIN_THR_DEFAULT << 16) |
	      PFE_IQOS_WRED_MAX_THR_DEFAULT;
	hal_write32(val, base_va + CSR_IGQOS_LMEMQ_DROP_THRESH);
	hal_write32(val, base_va + CSR_IGQOS_RXFQ_DROP_THRESH);
}

static uint32_t igqos_wred_queue_enable_bit(pfe_iqos_queue_t queue)
{
	uint32_t ret;

	switch (queue)
	{
		case PFE_IQOS_Q_DMEM:
		{
			ret = (uint32_t)IGQOS_QOS_WRED_DMEMQ_EN;
			break;
		}
		case PFE_IQOS_Q_LMEM:
		{
			ret = (uint32_t)IGQOS_QOS_WRED_LMEMQ_EN;
			break;
		}
		case PFE_IQOS_Q_RXF:
		{
			ret = (uint32_t)IGQOS_QOS_WRED_RXFQ_EN;
			break;
		}
		default:
		{
			ret = (uint32_t)IGQOS_QOS_WRED_DMEMQ_EN;
			break;
		}
	}

	return ret;
}

void pfe_gpi_cfg_wred_enable(addr_t base_va, pfe_iqos_queue_t queue)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_QOS);

	reg |= igqos_wred_queue_enable_bit(queue);
	hal_write32(reg, base_va + CSR_IGQOS_QOS);
}

void pfe_gpi_cfg_wred_disable(addr_t base_va, pfe_iqos_queue_t queue)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_QOS);

	reg &= ~igqos_wred_queue_enable_bit(queue);
	hal_write32(reg, base_va + CSR_IGQOS_QOS);
}

bool_t pfe_gpi_cfg_wred_is_enabled(addr_t base_va, pfe_iqos_queue_t queue)
{
	bool_t ret = FALSE;
	uint32_t wred_q_en = igqos_wred_queue_enable_bit(queue);
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_QOS);

	if ((reg & wred_q_en) == wred_q_en)
	{
		ret = TRUE;
	}

	return ret;
}

void pfe_gpi_cfg_wred_set_prob(addr_t base_va, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t val)
{
	uint32_t reg = hal_read32(base_va + CSR_IQGOS_ZONE_PROB((uint32_t)queue));

	reg &= ~((uint32_t)0xfU << ((uint8_t)zone * 4U));
	reg |= (((uint32_t)0xfU & val) << ((uint8_t)zone * 4U));
	hal_write32(reg, base_va + CSR_IQGOS_ZONE_PROB((uint32_t)queue));
}

void pfe_gpi_cfg_wred_get_prob(addr_t base_va, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t *val)
{
	uint32_t reg = hal_read32(base_va + CSR_IQGOS_ZONE_PROB((uint32_t)queue));

	reg >>= ((uint8_t)zone * 4U);
	reg &= (uint32_t)0xfU;
	*val = (uint8_t)reg;
}

void pfe_gpi_cfg_wred_set_thr(addr_t base_va, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t val)
{
	uint32_t reg;
	int16_t off = 0;

	switch (thr)
	{
		case PFE_IQOS_WRED_FULL_THR:
		{
			hal_write32(val, base_va + CSR_IQGOS_FULL_THRESH((uint32_t)queue));
			break;
		}
		case PFE_IQOS_WRED_MIN_THR:
		{
			off = 16;
			reg = hal_read32(base_va + CSR_IQGOS_DROP_THRESH((uint32_t)queue));
			reg &= (uint32_t)(~((uint32_t)0xFFFFU << (uint8_t)off));
			reg |= ((uint32_t)val << (uint8_t)off);
			hal_write32(reg, base_va + CSR_IQGOS_DROP_THRESH((uint32_t)queue));
			break;
		}
		case PFE_IQOS_WRED_MAX_THR:
		{
			reg = hal_read32(base_va + CSR_IQGOS_DROP_THRESH((uint32_t)queue));
			reg &= (uint32_t)(~((uint32_t)0xFFFFU << (uint8_t)off));
			reg |= ((uint32_t)val << (uint8_t)off);
			hal_write32(reg, base_va + CSR_IQGOS_DROP_THRESH((uint32_t)queue));
			break;
		}
		default:
		{
			/* Invalid threshold */
			break;
		}
	}
}

void pfe_gpi_cfg_wred_get_thr(addr_t base_va, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t *val)
{
	uint32_t reg;
	int32_t off = 0;

	switch (thr)
	{
		case PFE_IQOS_WRED_FULL_THR:
		{
			*val = (uint16_t)hal_read32(base_va + CSR_IQGOS_FULL_THRESH((uint32_t)queue));
			break;
		}
		case PFE_IQOS_WRED_MIN_THR:
		{
			off = 16;
			reg = hal_read32(base_va + CSR_IQGOS_DROP_THRESH((uint32_t)queue));
			reg >>= (uint8_t)off;
			reg &= (uint32_t)0xFFFFU;
			*val = (uint16_t)reg;
			break;
		}
		case PFE_IQOS_WRED_MAX_THR:
		{
			reg = hal_read32(base_va + CSR_IQGOS_DROP_THRESH((uint32_t)queue));
			reg >>= (uint8_t)off;
			reg &= (uint32_t)0xFFFFU;
			*val = (uint16_t)reg;
			break;
		}
		default:
		{
			/* Invalid threshold */
			break;
		}
	}
}

/* Shaper configuration */

uint32_t pfe_gpi_cfg_get_sys_clk_mhz(addr_t cbus_base_va)
{
	uint32_t reg;

	/* replace this with global csr driver function, when available */
	reg = hal_read32(cbus_base_va + CBUS_GLOBAL_CSR_BASE_ADDR + WSP_CLK_FRQ);

	return reg & 0xffffU;
}

void pfe_gpi_cfg_shp_default_init(addr_t base_va, uint8_t id)
{
	/* reset the weight reg */
	hal_write32(0, base_va + CSR_IGQOS_PORT_SHP_WGHT(id));
	/* reset the min credit reg */
	hal_write32(0, base_va + GPI_PORT_SHP_MIN_CREDIT(id));

	/* reset CONFIG settings for shaper #id */
	pfe_gpi_cfg_shp_set_type(base_va, id, PFE_IQOS_SHP_PORT_LEVEL);
	pfe_gpi_cfg_shp_set_mode(base_va, id, PFE_IQOS_SHP_BPS);

	/* reset CTRL */
	hal_write32(0, base_va + CSR_IGQOS_PORT_SHP_CTRL(id));
}

void pfe_gpi_cfg_shp_enable(addr_t base_va, uint8_t id)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CTRL(id));

	reg |= 0x1U;
	hal_write32(reg, base_va + CSR_IGQOS_PORT_SHP_CTRL(id));
}

void pfe_gpi_cfg_shp_disable(addr_t base_va, uint8_t id)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CTRL(id));

	reg &= (uint32_t)(~((uint32_t)0x1U));
	hal_write32(reg, base_va + CSR_IGQOS_PORT_SHP_CTRL(id));
}

bool_t pfe_gpi_cfg_shp_is_enabled(addr_t base_va, uint8_t id)
{
	bool_t ret = FALSE;
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CTRL(id));

	if (0U != (reg & 0x1U))
	{
		ret = TRUE;
	}

	return ret;
}

void pfe_gpi_cfg_shp_set_type(addr_t base_va, uint8_t id, pfe_iqos_shp_type_t type)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CONFIG);

	reg &= (uint32_t)(~((uint32_t)IGQOS_PORT_SHP_TYPE_MASK << IGQOS_PORT_SHP_TYPE_POS(id)));
	reg |= (((uint32_t)type & IGQOS_PORT_SHP_TYPE_MASK) << IGQOS_PORT_SHP_TYPE_POS(id));

	hal_write32(reg, base_va + CSR_IGQOS_PORT_SHP_CONFIG);
}

void pfe_gpi_cfg_shp_get_type(addr_t base_va, uint8_t id, pfe_iqos_shp_type_t *type)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CONFIG);

	reg >>= IGQOS_PORT_SHP_TYPE_POS(id);
	reg  &= (uint32_t)IGQOS_PORT_SHP_TYPE_MASK;
	switch (reg)
	{
		case 0U:
		{
			*type = PFE_IQOS_SHP_PORT_LEVEL;
			break;
		}
		case 1U:
		{
			*type = PFE_IQOS_SHP_BCAST;
			break;
		}
		case 2U:
		{
			*type = PFE_IQOS_SHP_MCAST;
			break;
		}
		default:
		{
			/* Invalid type */
			break;
		}
	}
}

void pfe_gpi_cfg_shp_set_mode(addr_t base_va, uint8_t id, pfe_iqos_shp_rate_mode_t mode)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CONFIG);

	reg &= (uint32_t)(~(IGQOS_PORT_SHP_MODE_PPS(id)));
	if (PFE_IQOS_SHP_PPS == mode)
	{
		reg |= (uint32_t)IGQOS_PORT_SHP_MODE_PPS(id);
	}

	hal_write32(reg, base_va + CSR_IGQOS_PORT_SHP_CONFIG);
}

void pfe_gpi_cfg_shp_get_mode(addr_t base_va, uint8_t id, pfe_iqos_shp_rate_mode_t *mode)
{
	uint32_t reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CONFIG);

	if (0U != (reg & IGQOS_PORT_SHP_MODE_PPS(id)))
	{
		*mode = PFE_IQOS_SHP_PPS;
	}
	else
	{
		*mode = PFE_IQOS_SHP_BPS;
	}
}

void pfe_gpi_cfg_shp_set_isl_weight(addr_t base_va, uint8_t id, uint32_t clk_div_log2, uint32_t weight)
{
	uint32_t reg;

	reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CTRL(id));
	reg &= (uint32_t)(~(IGQOS_PORT_SHP_CLKDIV_MASK << IGQOS_PORT_SHP_CLKDIV_POS));
	reg |= (clk_div_log2 & IGQOS_PORT_SHP_CLKDIV_MASK) << IGQOS_PORT_SHP_CLKDIV_POS;
	hal_write32(reg, base_va + CSR_IGQOS_PORT_SHP_CTRL(id));

	hal_write32(weight & IGQOS_PORT_SHP_WEIGHT_MASK, base_va + CSR_IGQOS_PORT_SHP_WGHT(id));
}

void pfe_gpi_cfg_shp_get_isl_weight(addr_t base_va, uint8_t id, uint32_t *weight)
{
	uint32_t reg;

	reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_WGHT(id));
	*weight = reg & IGQOS_PORT_SHP_WEIGHT_MASK;
}

void pfe_gpi_cfg_shp_set_limits(addr_t base_va, uint8_t id, uint32_t max_credit, uint32_t min_credit)
{
	uint32_t reg;

	/* reset MIN_CREDIT reg */
	hal_write32(min_credit & IGQOS_PORT_SHP_CREDIT_MASK,
		    base_va + CSR_IGQOS_PORT_SHP_MIN_CREDIT(id));

	reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CTRL(id));
	reg &= ~(IGQOS_PORT_SHP_CREDIT_MASK << IGQOS_PORT_SHP_MAX_CREDIT_POS);
	reg |= ((max_credit & IGQOS_PORT_SHP_CREDIT_MASK) << IGQOS_PORT_SHP_MAX_CREDIT_POS);
	hal_write32(reg, base_va + CSR_IGQOS_PORT_SHP_CTRL(id));
}

void pfe_gpi_cfg_shp_get_limits(addr_t base_va, uint8_t id, uint32_t *max_credit, uint32_t *min_credit)
{
	uint32_t reg;

	reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_MIN_CREDIT(id));
	reg &= IGQOS_PORT_SHP_CREDIT_MASK;
	*min_credit = reg;

	reg = hal_read32(base_va + CSR_IGQOS_PORT_SHP_CTRL(id));
	reg >>= IGQOS_PORT_SHP_MAX_CREDIT_POS;
	reg &= IGQOS_PORT_SHP_CREDIT_MASK;
	*max_credit = reg;
}

/* read the shaper drop packets counter register, which is clear on read */
uint32_t pfe_gpi_cfg_shp_get_drop_cnt(addr_t base_va, uint8_t id)
{
	return hal_read32(base_va + CSR_IGQOS_STAT_SHAPER_DROP_CNT(id));
}

/**
 * @brief		Get GPI statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the GPI block.
 * @param[in]	base_va 	Base address of GPI register space (virtual)
 * @param[in]	seq 		Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_gpi_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level)
{
	uint32_t reg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Debug registers */
		if(verb_level >= 10U)
		{
			seq_printf(seq, "GPI_FIFO_DEBUG   : 0x%x\n", hal_read32(base_va + GPI_FIFO_DEBUG));
			seq_printf(seq, "GPI_TX_DBUG_REG1 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG1));
			seq_printf(seq, "GPI_TX_DBUG_REG2 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG2));
			seq_printf(seq, "GPI_TX_DBUG_REG3 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG3));
			seq_printf(seq, "GPI_TX_DBUG_REG4 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG4));
			seq_printf(seq, "GPI_TX_DBUG_REG5 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG5));
			seq_printf(seq, "GPI_TX_DBUG_REG6 : 0x%x\n", hal_read32(base_va + GPI_TX_DBUG_REG6));
			seq_printf(seq, "GPI_RX_DBUG_REG1 : 0x%x\n", hal_read32(base_va + GPI_RX_DBUG_REG1));
			seq_printf(seq, "GPI_RX_DBUG_REG2 : 0x%x\n", hal_read32(base_va + GPI_RX_DBUG_REG2));
			seq_printf(seq, "GPI_FIFO_STATUS  : 0x%x\n", hal_read32(base_va + GPI_FIFO_STATUS));
		}

		/*	Get version */
		if(verb_level >= 9U)
		{
			reg = hal_read32(base_va + GPI_VERSION);
			seq_printf(seq, "Revision             : 0x%x\n", (reg >> 24) & 0xffU);
			seq_printf(seq, "Version              : 0x%x\n", (reg >> 16) & 0xffU);
			seq_printf(seq, "ID                   : 0x%x\n", reg & 0xffffU);
		}

		/*	Ingress QoS counters */
		reg = hal_read32(base_va + CSR_IGQOS_QUEUE_STATUS);
		seq_printf(seq, "IGQOS queue status   : 0x%x\n", reg);
		reg = hal_read32(base_va + CSR_IGQOS_STAT_CLASS_DROP_CNT);
		seq_printf(seq, "IGQOS CLASS drop cnt : 0x%x\n", reg);
		reg = hal_read32(base_va + CSR_IGQOS_STAT_LMEM_QUEUE_DROP_CNT);
		seq_printf(seq, "IGQOS LMEM drop cnt  : 0x%x\n", reg);
		reg = hal_read32(base_va + CSR_IGQOS_STAT_DMEM_QUEUE_DROP_CNT);
		seq_printf(seq, "IGQOS DMEM drop cnt  : 0x%x\n", reg);
		reg = hal_read32(base_va + CSR_IGQOS_STAT_RXF_QUEUE_DROP_CNT);
		seq_printf(seq, "IGQOS RXF drop cnt   : 0x%x\n", reg);
		reg = pfe_gpi_cfg_shp_get_drop_cnt(base_va, 0);
		seq_printf(seq, "IGQOS SHP0 drop cnt  : 0x%x\n", reg);
		reg = pfe_gpi_cfg_shp_get_drop_cnt(base_va, 1);
		seq_printf(seq, "IGQOS SHP1 drop cnt  : 0x%x\n", reg);
		reg = hal_read32(base_va + CSR_IGQOS_STAT_MANAGED_PACKET_CNT);
		seq_printf(seq, "IGQOS managed pkts   : 0x%x\n", reg);
		reg = hal_read32(base_va + CSR_IGQOS_STAT_UNMANAGED_PACKET_CNT);
		seq_printf(seq, "IGQOS unmanaged pkts : 0x%x\n", reg);
		reg = hal_read32(base_va + CSR_IGQOS_STAT_RESERVED_PACKET_CNT);
		seq_printf(seq, "IGQOS reserved pkts  : 0x%x\n", reg);

		reg = hal_read32(base_va + GPI_FIFO_STATUS);
		seq_printf(seq, "TX Underrun          : 0x%x\n", reg);
		hal_write32(0, base_va + GPI_FIFO_STATUS);

		reg = hal_read32(base_va + GPI_FIFO_DEBUG);
		seq_printf(seq, "TX FIFO Packets      : 0x%x\n", reg & 0x1fU);
		seq_printf(seq, "RX FIFO Packets      : 0x%x\n", (reg >> 6) & 0x1fU);
		seq_printf(seq, "TX FIFO Level        : 0x%x\n", (reg >> 12) & 0xffU);
		seq_printf(seq, "RX FIFO Level        : 0x%x\n", (reg >> 20) & 0xffU);

		reg = hal_read32(base_va + GPI_DTX_ASEQ);
		seq_printf(seq, "ASEQ Length          : 0x%x\n", reg);

		reg = hal_read32(base_va + GPI_EMAC_1588_TIMESTAMP_EN);
		seq_printf(seq, "1588 Enable register : 0x%x\n", reg);

		reg = hal_read32(base_va + GPI_OVERRUN_DROPCNT);
		seq_printf(seq, "Overrun Drop Counter : 0x%x\n", reg);
		hal_write32(0, base_va + GPI_OVERRUN_DROPCNT);
	}

	return 0;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

