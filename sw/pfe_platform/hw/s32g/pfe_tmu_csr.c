/* =========================================================================
 *
 *  Copyright (c) 2020 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_tmu_csr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

/*	List of QoS configuration for each physical interface terminated with invalid entry */
static const pfe_tmu_phy_cfg_t phys[] = {
	{.id = PFE_PHY_IF_ID_EMAC0, .q_cnt = 8U, .sch_cnt = 2U, .shp_cnt = 4U},
	{.id = PFE_PHY_IF_ID_EMAC1, .q_cnt = 8U, .sch_cnt = 2U, .shp_cnt = 4U},
	{.id = PFE_PHY_IF_ID_EMAC2, .q_cnt = 8U, .sch_cnt = 2U, .shp_cnt = 4U},
	{.id = PFE_PHY_IF_ID_HIF0, .q_cnt = 2U, .sch_cnt = 0U, .shp_cnt = 0U},
	{.id = PFE_PHY_IF_ID_HIF1, .q_cnt = 2U, .sch_cnt = 0U, .shp_cnt = 0U},
	{.id = PFE_PHY_IF_ID_HIF2, .q_cnt = 2U, .sch_cnt = 0U, .shp_cnt = 0U},
	{.id = PFE_PHY_IF_ID_HIF3, .q_cnt = 2U, .sch_cnt = 0U, .shp_cnt = 0U},
	{.id = PFE_PHY_IF_ID_HIF, .q_cnt = 8U, .sch_cnt = 2U, .shp_cnt = 4U},
	{.id = PFE_PHY_IF_ID_HIF_NOCPY, .q_cnt = 8U, .sch_cnt = 2U, .shp_cnt = 4U},
	{.id = PFE_PHY_IF_ID_UTIL, .q_cnt = 8U, .sch_cnt = 2U, .shp_cnt = 4U},
	{.id = PFE_PHY_IF_ID_INVALID}
};

#define CLK_DIV_LOG2 (8U - 1U) /* clk_div_log2 = log2(clk_div/2) */
#define CLK_DIV (1U << (CLK_DIV_LOG2 + 1U)) /* 256 */

static errno_t pfe_tmu_cntx_mem_write(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t data);
static errno_t pfe_tmu_cntx_mem_read(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t *data);

/**
 * @brief		Return QoS configuration of given physical interface
 * @param[in]	phy The physical interface to get QoS configuration for
 * @return		Pointer to the configuration or NULL if not found
 */
const pfe_tmu_phy_cfg_t *const pfe_tmu_cfg_get_phy_config(pfe_ct_phy_if_id_t phy)
{
	uint32_t ii;

	for (ii=0U; phys[ii].id != PFE_PHY_IF_ID_INVALID; ii++)
	{
		if (phys[ii].id == phy)
		{
			return &phys[ii];
		}
	}

	return NULL;
}

/**
 * @brief		Initialize and configure the TMU
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	cfg Pointer to the configuration structure
 * @return		EOK if success
 */
errno_t pfe_tmu_cfg_init(void *cbus_base_va, pfe_tmu_cfg_t *cfg)
{
	uint32_t ii, queue;
	errno_t ret;

	(void)cfg;

	hal_write32(0x0U, cbus_base_va + TMU_PHY0_TDQ_CTRL);
	hal_write32(0x0U, cbus_base_va + TMU_PHY1_TDQ_CTRL);
	hal_write32(0x0U, cbus_base_va + TMU_PHY2_TDQ_CTRL);
	hal_write32(0x0U, cbus_base_va + TMU_PHY3_TDQ_CTRL);
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	hal_write32(0x0U, cbus_base_va + TMU_PHY4_TDQ_CTRL);
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
	hal_write32(0x0U, cbus_base_va + TMU_PHY5_TDQ_CTRL);	/* UTIL PE */

	/*	Reset */
	pfe_tmu_cfg_reset(cbus_base_va);

	/*	INQ */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_EGPI1_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY0_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_EGPI2_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY1_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_EGPI3_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY2_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_HGPI_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY16_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_HGPI_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY3_INQ_ADDR);
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_HIF_NOCPY_BASE_ADDR + HIF_NOCPY_RX_INQ0_PKTPTR, cbus_base_va + TMU_PHY4_INQ_ADDR);
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
    /* The macro UTIL_INQ_PKTPTR already contains the CBUS_UTIL_CSR_BASE_ADDR (difference to above lines) */
    hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + UTIL_INQ_PKTPTR, cbus_base_va + TMU_PHY5_INQ_ADDR); /* UTIL */

	/*	Context memory initialization */
	for (ii=0U; ii < TLITE_PHYS_CNT; ii++)
	{
		/*	Initialize queues */
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			hal_write32(0x1U, cbus_base_va + TMU_CNTX_ACCESS_CTRL);
			hal_write32(((ii & 0x1fU) << 8) | (queue & 0x7U), cbus_base_va + TMU_PHY_QUEUE_SEL);
			hal_nop();

			/*	Clear direct access registers */
			hal_write32(0U, cbus_base_va + TMU_CURQ_PTR);
			hal_write32(0U, cbus_base_va + TMU_CURQ_PKT_CNT);
			hal_write32(0U, cbus_base_va + TMU_CURQ_DROP_CNT);
			hal_write32(0U, cbus_base_va + TMU_CURQ_TRANS_CNT);
			hal_write32(0U, cbus_base_va + TMU_CURQ_QSTAT);
			hal_write32(0U, cbus_base_va + TMU_HW_PROB_CFG_TBL0);
			hal_write32(0U, cbus_base_va + TMU_HW_PROB_CFG_TBL1);
			hal_write32(0U, cbus_base_va + TMU_CURQ_DEBUG);
		}

		/*	Initialize HW schedulers. Invalidate all inputs. */
		pfe_tmu_sch_cfg_init(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 0U);
		pfe_tmu_sch_cfg_init(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 1U);

		/*	Initialize shapers. Make sure they are not connected. */
		pfe_tmu_shp_cfg_init(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 0U);
		pfe_tmu_shp_cfg_init(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 1U);
		pfe_tmu_shp_cfg_init(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 2U);
		pfe_tmu_shp_cfg_init(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 3U);

		/*	Set default topology:
			 - All shapers are disabled and not associated with any queue
			 - Scheduler 0 is not used
			 - Queue[n]->SCH1.input[n]
		*/
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			/*	Scheduler 1 */
			ret = pfe_tmu_sch_cfg_bind_queue(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 1U, queue, queue);
			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Can't bind queue to scheduler: %d\n", ret);
				return ENOEXEC;
			}
		}

		ret = pfe_tmu_sch_cfg_set_rate_mode(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 1U, RATE_MODE_DATA_RATE);
		if (EOK != ret)
		{
			NXP_LOG_DEBUG("Could not set scheduler 1 rate mode: %d\n", ret);
			return ENOEXEC;
		}

		ret = pfe_tmu_sch_cfg_set_algo(cbus_base_va, (pfe_ct_phy_if_id_t)ii, 1U, SCHED_ALGO_RR);
		if (EOK != ret)
		{
			NXP_LOG_DEBUG("Could not set scheduler 1 algo: %d\n", ret);
			return ENOEXEC;
		}

		/*	Set default queue mode */
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			/* ret = pfe_tmu_q_mode_set_default(cbus_base_va, phys[ii], queue); */
			ret = pfe_tmu_q_mode_set_tail_drop(cbus_base_va, (pfe_ct_phy_if_id_t)ii, queue, 255U);
			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Can't set default queue mode: %d\n", ret);
				return ret;
			}
		}
	}

	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_FREE_CTRL, cbus_base_va + TMU_BMU_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU2_BASE_ADDR + BMU_FREE_CTRL, cbus_base_va + TMU_BMU2_INQ_ADDR);
	hal_write32(0x100U, cbus_base_va + TMU_AFULL_THRES);
	hal_write32(0xfcU, cbus_base_va + TMU_INQ_WATERMARK);
	hal_write32(0xfU, cbus_base_va + TMU_PHY0_TDQ_CTRL);
	hal_write32(0xfU, cbus_base_va + TMU_PHY1_TDQ_CTRL);
	hal_write32(0xfU, cbus_base_va + TMU_PHY2_TDQ_CTRL);
	hal_write32(0xfU, cbus_base_va + TMU_PHY16_TDQ_CTRL);
	hal_write32(0xfU, cbus_base_va + TMU_PHY3_TDQ_CTRL);
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	hal_write32(0xfU, cbus_base_va + TMU_PHY4_TDQ_CTRL);
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
    hal_write32(0xfU, cbus_base_va + TMU_PHY5_TDQ_CTRL);	/* UTIL */

	return EOK;
}

/**
 * @brief		Issue TMU reset
 * @param[in]	cbus_base_va The cbus base address
 */
void pfe_tmu_cfg_reset(void *cbus_base_va)
{
	uint32_t timeout = 20U;
	uint32_t reg;

	hal_write32(0x1U, cbus_base_va + TMU_CTRL);

	do
	{
		oal_time_usleep(100U);
		timeout--;
		reg = hal_read32(cbus_base_va + TMU_CTRL);
	} while ((0U != (reg & 0x1U)) && (timeout > 0U));

	if (0U == timeout)
	{
		NXP_LOG_ERROR("FATAL: TMU reset timed-out\n");
	}
}

/**
 * @brief		Enable the TMU block
 * @param[in]	cbus_base_va The cbus base address
 */
void pfe_tmu_cfg_enable(void *cbus_base_va)
{
	/*	nop */
	(void)cbus_base_va;
}

/**
 * @brief		Disable the TMU block
 * @param[in]	base_va The cbus base address
 */
void pfe_tmu_cfg_disable(void *cbus_base_va)
{
	/*	nop */
	(void)cbus_base_va;
}

/**
 * @brief		Send packet directly via TMU
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy Physical interface identifier
 * @param[in]	queue TX queue identifier
 * @param[in]	buf_pa Buffer physical address
 * @param[in]	len Number of bytes to send
 */
void pfe_tmu_cfg_send_pkt(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, void *buf_pa, uint16_t len)
{
	hal_write32((uint32_t)((addr_t)PFE_CFG_MEMORY_PHYS_TO_PFE(buf_pa) & 0xffffffffU), cbus_base_va + TMU_PHY_INQ_PKTPTR);
	hal_write32(((uint32_t)phy << 24) | (queue << 16) | len, cbus_base_va + TMU_PHY_INQ_PKTINFO);
}

/**
 * @brief		Write TMU context memory
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	loc Location to be written
 * @param[out]	data Data to be written
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_tmu_cntx_mem_write(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t data)
{
	uint32_t reg;
	uint32_t timeout = 20U;

	hal_write32(0U, cbus_base_va + TMU_CNTX_ACCESS_CTRL);

	switch (phy)
	{
		case PFE_PHY_IF_ID_EMAC0:
		case PFE_PHY_IF_ID_EMAC1:
		case PFE_PHY_IF_ID_EMAC2:
		case PFE_PHY_IF_ID_HIF_NOCPY:
        case PFE_PHY_IF_ID_UTIL:
		{
			break;
		}
		case PFE_PHY_IF_ID_HIF:
		case PFE_PHY_IF_ID_HIF0:
		case PFE_PHY_IF_ID_HIF1:
		case PFE_PHY_IF_ID_HIF2:
		case PFE_PHY_IF_ID_HIF3:
		{
			phy = PFE_PHY_IF_ID_HIF;
			break;
		}
		default:
		{
			return EINVAL;
		}
	}

	hal_write32(((phy & 0x1fU) << 16) | loc, cbus_base_va + TMU_CNTX_ADDR);
	hal_write32(data, cbus_base_va + TMU_CNTX_DATA);
	hal_write32(0x3U, cbus_base_va + TMU_CNTX_CMD);

	do
	{
		oal_time_usleep(10U);
		timeout--;
		reg = hal_read32(cbus_base_va + TMU_CNTX_CMD);
	} while ((0U == (reg & 0x4U)) && (timeout > 0U));

	if (0U == timeout)
	{
		return ETIMEDOUT;
	}

	return EOK;
}

/**
 * @brief		Read TMU context memory
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	loc Location to be read
 * @param[out]	data Pointer to memory where read data shall be written
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_tmu_cntx_mem_read(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t *data)
{
	uint32_t reg;
	uint32_t timeout = 20U;

	hal_write32(0U, cbus_base_va + TMU_CNTX_ACCESS_CTRL);

	switch (phy)
	{
		case PFE_PHY_IF_ID_EMAC0:
		case PFE_PHY_IF_ID_EMAC1:
		case PFE_PHY_IF_ID_EMAC2:
		case PFE_PHY_IF_ID_HIF_NOCPY:
        case PFE_PHY_IF_ID_UTIL:
		{
			break;
		}
		case PFE_PHY_IF_ID_HIF:
		case PFE_PHY_IF_ID_HIF0:
		case PFE_PHY_IF_ID_HIF1:
		case PFE_PHY_IF_ID_HIF2:
		case PFE_PHY_IF_ID_HIF3:
		{
			phy = PFE_PHY_IF_ID_HIF;
			break;
		}
		default:
		{
			return EINVAL;
		}
	}

	hal_write32(((phy & 0x1fU) << 16) | loc, cbus_base_va + TMU_CNTX_ADDR);
	hal_write32(0x2U, cbus_base_va + TMU_CNTX_CMD);

	do
	{
		oal_time_usleep(10U);
		timeout--;
		reg = hal_read32(cbus_base_va + TMU_CNTX_CMD);
	} while ((0U == (reg & 0x4U)) && (timeout > 0U));

	if (0U == timeout)
	{
		return ETIMEDOUT;
	}

	*data = hal_read32(cbus_base_va + TMU_CNTX_DATA);

	return EOK;
}

static uint8_t pfe_tmu_hif_q_to_tmu_q(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
	uint32_t reg, ii;
	int8_t hif_queue = -1;

	/*	Convert HIF channel `queue` (range 0-`n`) to TMU queue (range 0-`m`) */
	if ((phy == PFE_PHY_IF_ID_HIF0)
			|| (phy == PFE_PHY_IF_ID_HIF1)
			|| (phy == PFE_PHY_IF_ID_HIF2)
			|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		reg = hal_read32(cbus_base_va + CBUS_HIF_BASE_ADDR + HIF_RX_QUEUE_MAP_CH_NO_ADDR);
		for (ii=0U; ii<8U; ii++)
		{
			if (((uint32_t)phy - PFE_PHY_IF_ID_HIF0) == ((reg >> (ii * 4)) & 0xfU))
			{
				hif_queue++;
				if (queue == hif_queue)
				{
					return ii;
				}
			}
		}
	}

	return PFE_TMU_INVALID_QUEUE;
}

/**
 * @brief		Get number of packets in the queue
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[out]	level Pointer to memory where the fill level value shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_cfg_get_fill_level(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *level)
{
	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return EINVAL;
		}
	}

	/*	curQ_pkt_cnt is @ position 1 per queue */
	return pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue) + 1U, level);
}

/**
 * @brief		Get number of dropped packets for the queue
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[out]	cnt Pointer to memory where the drop count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_cfg_get_drop_count(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt)
{
	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return EINVAL;
		}
	}

	/*	curQ_drop_cnt is @ position 2 per queue */
	return pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue) + 2U, cnt);
}

/**
 * @brief		Get number of transmitted packets for the queue
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[out]	cnt Pointer to memory where the TX count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_cfg_get_tx_count(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt)
{
	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return EINVAL;
		}
	}

	/*	curQ_trans_cnt is @ position 3 per queue */
	return pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue) + 3U, cnt);
}

/**
 * @brief		Get queue mode
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[out]	max Pointer to memory where 'max' value shall be written
 * @param[out]	max Pointer to memory where 'min' value shall be written
 * @return		The queue mode
 */
pfe_tmu_queue_mode_t pfe_tmu_q_get_mode(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *min, uint32_t *max)
{
	uint32_t reg;
	errno_t ret;
	pfe_tmu_queue_mode_t mode;

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return TMU_Q_MODE_INVALID;
		}
	}

	/*	curQ_Qmax[8:0], curQ_Qmin[8:0], curQ_cfg[1:0] are @ position 4 per queue */
	ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue) + 4U, &reg);
	if (EOK != ret)
	{
		return TMU_Q_MODE_INVALID;
	}

	switch (reg & 0x3U)
	{
		case 1U:
		{
			mode = TMU_Q_MODE_TAIL_DROP;
			*max = (reg >> 11) & 0x1ffU;
			*min = 0U;
			break;
		}

		case 2U:
		{
			mode = TMU_Q_MODE_WRED;
			*max = (reg >> 11) & 0x1ffU;
			*min = (reg >> 2) & 0x1ffU;
			break;
		}

		default:
		{
			mode = TMU_Q_MODE_DEFAULT;
			*max = 0U;
			*min = 0U;
			break;
		}
	}

	return mode;
}

/**
 * @brief		Configure queue in default mode
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_mode_set_default(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return EINVAL;
		}
	}

	/*	If bit 1 is zero then in case when LLM is full the TMU will wait. */
	hal_write32(0x0U | (0x0U << 1), cbus_base_va + TMU_TEQ_CTRL);

	/*	Put the queue to default mode */
	/*	curQ_Qmax[8:0], curQ_Qmin[8:0], curQ_cfg[1:0] are @ position 4 per queue */
	return pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue) + 4U, 0U);
}

/**
 * @brief		Configure queue in tail-drop mode
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[out]	max This is the maximum fill level the queue can achieve. When exceeded
 * 					the enqueue requests will result in packet drop.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_mode_set_tail_drop(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint16_t max)
{
	uint32_t reg;

	if (max > 0xffU)
	{
		return EINVAL;
	}

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return EINVAL;
		}
	}

	/*	curQ_Qmax[8:0], curQ_Qmin[8:0], curQ_cfg[1:0] are @ position 4 per queue */
	reg = (max << 11) | (0U << 2) | (0x1U << 0);
	return pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue) + 4U, reg);
}

/**
 * @brief		Configure queue in WRED mode
 * @details		There are 8 WRED zones with configurable drop probabilities. Zones are given
 * 				by queue fill level thresholds as:
 *
 * 										zone_threshold[n] = n*((max - min) / 8)
 *
 *				The WRED decides if packets shall be dropped using following algorithm:
 *
 *								if ((queueFillLevel > min) && (rnd() <= currentZoneProbability))
 *									DROP;
 *								else if (queueFillLevel >= max)
 *									DROP;
 *								fi
 *
 *				where
 *					- queueFillLevel is current fill level
 *					- rnd() is (pseudo) random number generator
 *					- currentZoneProbability is value assigned to current zone
 *					- probability for zone above max is 100%
 *					- probability for zone below min is 0%
 *
 *				Once queue is set to WRED mode, all zone probabilities are set to zero.
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[in]	min See algorithm above
 * @param[in]	max See algorithm above
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_mode_set_wred(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint16_t min, uint16_t max)
{
	uint32_t reg;
	errno_t ret;

	if ((max > 0x1ffU) || (min > 0x1ffU))
	{
		NXP_LOG_ERROR("Queue WRED 'min' and/or 'max' argument out of range\n");
		return EINVAL;
	}

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return EINVAL;
		}
	}

	/*	Initialize probabilities. Probability tables are @ position 5 and 6 per queue. */
	/*	Context memory position 5 (curQ_hw_prob_cfg_tbl0):
	 	 	[4:0]	Zone0 value
	 		[9:5]	Zone1 value
	 		[14:10]	Zone2 value
	 		[19:15]	Zone3 value
	 		[24:20]	Zone4 value
	 		[29:25]	Zone5 value
	 	Context memory position 6 (curQ_hw_prob_cfg_tbl1):
	 	 	[4:0]	Zone6 value
	 	 	[9:5]	Zone7 value
	*/
	reg = 0U;
	ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue) + 5U, reg);
	if (EOK != ret)
	{
		return ret;
	}

	ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue) + 6U, reg);
	if (EOK != ret)
	{
		return ret;
	}

	/*	curQ_Qmax[8:0], curQ_Qmin[8:0], curQ_cfg[1:0] are @ position 4 per queue */
	reg = (max << 11) | (min << 2) | 0x2U;
	ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue) + 4U, reg);
	if (EOK != ret)
	{
		return ret;
	}

	return EOK;
}

/**
 * @brief		Set WRED zone drop probability
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[in]	zone The WRED zone (0-7). See pfe_tmu_q_mode_set_wred.
 * @param[in]	prob Zone probability [%]
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_set_wred_probability(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t prob)
{
	uint32_t ret;
	uint32_t reg;
	uint8_t pos;

	if ((prob > 100U) || (zone > 7U))
	{
		return EINVAL;
	}

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return EINVAL;
		}
	}

	/*	Context memory position 5 (curQ_hw_prob_cfg_tbl0):
			[4:0]	Zone0 value
			[9:5]	Zone1 value
			[14:10]	Zone2 value
			[19:15]	Zone3 value
			[24:20]	Zone4 value
			[29:25]	Zone5 value
		Context memory position 6 (curQ_hw_prob_cfg_tbl1):
			[4:0]	Zone6 value
			[9:5]	Zone7 value
	*/
	pos = 5U + (zone / 6U);

	ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue) + pos, &reg);
	if (EOK != ret)
	{
		return ret;
	}

	reg &= ~(0x1fU << (5U * (zone % 6U)));
	reg |= (((0x1fU * (uint32_t)prob) / 100U) & 0x1fU) << (5U * (zone % 6U));

	ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue) + pos, reg);
	if (EOK != ret)
	{
		return ret;
	}

	return EOK;
}

/**
 * @brief		Get WRED zone drop probability
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[in]	zone The WRED zone (0-7). See pfe_tmu_q_mode_set_wred.
 * @param[in]	prob Pointer to memory where zone probability [%] shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_get_wred_probability(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t *prob)
{
	uint32_t ret;
	uint32_t reg;
	uint8_t pos;

	if (zone > 7U)
	{
		return EINVAL;
	}

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue)
		{
			return EINVAL;
		}
	}

	/*	Context memory position 5 (curQ_hw_prob_cfg_tbl0):
			[4:0]	Zone0 value
			[9:5]	Zone1 value
			[14:10]	Zone2 value
			[19:15]	Zone3 value
			[24:20]	Zone4 value
			[29:25]	Zone5 value
		Context memory position 6 (curQ_hw_prob_cfg_tbl1):
			[4:0]	Zone6 value
			[9:5]	Zone7 value
	*/

	pos = 5U + (zone / 6U);

	ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue) + pos, &reg);
	if (EOK != ret)
	{
		return ret;
	}

	reg = reg >> (5U * (zone % 6U));
	*prob = ((reg & 0x1fU) * 100U) / 0x1fU;

	return EOK;
}

/**
 * @brief		Get number of WRED probability zones between 'min' and 'max' threshold
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @return		Number of zones
 */
uint8_t pfe_tmu_q_get_wred_zones(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
	(void) cbus_base_va;
	(void) phy;
	(void) queue;

	return 8U;
}

/**
 * @brief		Set shaper credit limits
 * @details		Value units depend on chosen shaper mode
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 * @param[in]	max_credit Maximum credit value. Must be positive.
 * @param[in]	min_credit Minimum credit value. Must be negative.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_cfg_set_limits(void *cbus_base_va, pfe_ct_phy_if_id_t phy,
		uint8_t shp, int32_t max_credit, int32_t min_credit)
{
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	if ((max_credit > 0x3fffff) || (max_credit < 0))
	{
		NXP_LOG_ERROR("Max credit value exceeded\n");
		return EINVAL;
	}

	if ((min_credit < -0x3fffff) || (min_credit > 0))
	{
		NXP_LOG_ERROR("Min credit value exceeded\n");
		return EINVAL;
	}

	hal_write32(max_credit << 10, shp_base_va + TMU_SHP_MAX_CREDIT);
	hal_write32(-min_credit, shp_base_va + TMU_SHP_MIN_CREDIT);

	return EOK;
}

/**
 * @brief		Get shaper credit limits
 * @details		Value units depend on chosen shaper mode
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 * @param[in]	max_credit Pointer to memory where maximum credit value shall be written
 * @param[in]	min_credit Pointer to memory where minimum credit value shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_cfg_get_limits(void *cbus_base_va, pfe_ct_phy_if_id_t phy,
		uint8_t shp, int32_t *max_credit, int32_t *min_credit)
{
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	*max_credit = (int32_t)(hal_read32(shp_base_va + TMU_SHP_MAX_CREDIT) >> 10);
	*min_credit = -(int32_t)hal_read32(shp_base_va + TMU_SHP_MIN_CREDIT);

	return EOK;
}

/**
 * @brief		Set shaper position
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 * @param[in]	pos New shaper position
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_cfg_set_position(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp, uint8_t pos)
{
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);
	uint32_t reg;

	if ((pos >= 16U) && (pos != PFE_TMU_INVALID_POSITION))
	{
		NXP_LOG_ERROR("Invalid shaper position: %d\n", pos);
		return EINVAL;
	}

	reg = hal_read32(shp_base_va + TMU_SHP_CTRL2);
	reg &= ~(0x1fU << 1);
	reg |= ((pos & 0x1fU) << 1);
	hal_write32(reg, shp_base_va + TMU_SHP_CTRL2);

	return EOK;
}

/**
 * @brief		Get shaper position
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 * @return		Shaper position
 */
uint8_t pfe_tmu_shp_cfg_get_position(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);
	return (hal_read32(shp_base_va + TMU_SHP_CTRL2) >> 1U) & 0x1fU;
}

/**
 * @brief		Enable shaper
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 */
errno_t pfe_tmu_shp_cfg_enable(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	uint32_t reg;
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	/*	Enable the shaper */
	reg = hal_read32(shp_base_va + TMU_SHP_CTRL) | 0x1U;
	hal_write32(reg, shp_base_va + TMU_SHP_CTRL);

	return EOK;
}

/**
 * @brief		Set shaper rate mode
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	mode Shaper mode
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_cfg_set_rate_mode(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t shp, pfe_tmu_rate_mode_t mode)
{
	uint32_t reg;
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	reg = hal_read32(shp_base_va + TMU_SHP_CTRL2);
	if (mode == RATE_MODE_DATA_RATE)
	{
		reg &= ~(1U << 0);
	}
	else if (mode == RATE_MODE_PACKET_RATE)
	{
		reg |= 1U;
	}
	else
	{
		return EINVAL;
	}

	hal_write32(reg, shp_base_va + TMU_SHP_CTRL2);
	return EOK;
}

/**
 * @brief		Get shaper rate mode
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @return		Shaper rate mode
 */
pfe_tmu_rate_mode_t pfe_tmu_shp_cfg_get_rate_mode(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	uint32_t reg;
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	reg = hal_read32(shp_base_va + TMU_SHP_CTRL);
	if (0U == (reg & 0x1U))
	{
		/*	Shaper is disabled */
		return RATE_MODE_INVALID;
	}

	reg = hal_read32(shp_base_va + TMU_SHP_CTRL2);
	if (0U != (reg & 0x1U))
	{
		return RATE_MODE_PACKET_RATE;
	}
	else
	{
		return RATE_MODE_DATA_RATE;
	}
}

/**
 * @brief		Set shaper idle slope
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	isl Idle slope in units per second as given by chosen mode
 *					(bits-per-second, packets-per-second)
 */
errno_t pfe_tmu_shp_cfg_set_idle_slope(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t shp, uint32_t isl)
{
	uint32_t reg;
	uint64_t wgt;
	uint32_t sys_clk_hz;
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	reg = hal_read32(cbus_base_va + CBUS_GLOBAL_CSR_BASE_ADDR + WSP_CLK_FRQ);
	sys_clk_hz = (reg & 0xffffU) * 1000000U;
	NXP_LOG_INFO("Using PFE sys_clk value %dHz\n", sys_clk_hz);

	/*	Set weight (added to credit counter with each sys_clk_hz/clk_div tick) */
	switch (pfe_tmu_shp_cfg_get_rate_mode(cbus_base_va, phy, shp))
	{
		case RATE_MODE_DATA_RATE:
		{
			/*	ISL is bps, WGT is [bytes-per-tick] */
			wgt = ((uint64_t)isl * (uint64_t)CLK_DIV * (1ULL << 12)) / (8ULL * (uint64_t)sys_clk_hz);
			break;
		}

		case RATE_MODE_PACKET_RATE:
		{
			/*	ISL is pps, WGT is [packets-per-tick] */
			wgt = ((uint64_t)isl * (uint64_t)CLK_DIV * (1ULL << 12)) / ((uint64_t)sys_clk_hz);
			break;
		}

		default:
		{
			return EINVAL;
		}
	}

	if (wgt > 0xfffffU)
	{
		NXP_LOG_WARNING("Shaper weight exceeds max value\n");
	}

	hal_write32(wgt & 0xfffffU, shp_base_va + TMU_SHP_WGHT);
	NXP_LOG_INFO("Shaper weight set to %d.%d\n",
		(uint32_t)((wgt >> 12) & 0xffU), (uint32_t)(wgt & 0xfffU));

	/*	Set clk_div */
	reg = hal_read32(shp_base_va + TMU_SHP_CTRL);
	reg &= 0x1U;
	hal_write32(reg | (CLK_DIV_LOG2 << 1), shp_base_va + TMU_SHP_CTRL);
	NXP_LOG_INFO("Shaper tick is %dHz\n", sys_clk_hz / CLK_DIV);

	return EOK;
}

/**
 * @brief		Get shaper idle slope
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @return		Current idle slope value
 */
uint32_t pfe_tmu_shp_cfg_get_idle_slope(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	uint32_t sys_clk_hz;
	uint32_t wgt, reg;
	uint64_t isl;
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	reg = hal_read32(cbus_base_va + CBUS_GLOBAL_CSR_BASE_ADDR + WSP_CLK_FRQ);
	sys_clk_hz = (reg & 0xffffU) * 1000000U;
	wgt = hal_read32(shp_base_va + TMU_SHP_WGHT) & 0xfffffU;
	isl = ((uint64_t)wgt * 8ULL * (uint64_t)sys_clk_hz) / ((uint64_t)CLK_DIV * (1ULL << 12));

	return (uint32_t)isl;
}

/**
 * @brief		Disable shaper
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 */
void pfe_tmu_shp_cfg_disable(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	uint32_t reg = hal_read32(shp_base_va + TMU_SHP_CTRL) & ~(uint32_t)0x1U;
	hal_write32(reg, shp_base_va + TMU_SHP_CTRL);
}

/**
 * @brief		Initialize shaper
 * @details		After initialization the shaper is disabled and not connected
 * 				to any queue.
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 */
void pfe_tmu_shp_cfg_init(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	void *shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phy, shp);

	/*	Disable */
	pfe_tmu_shp_cfg_disable(cbus_base_va, phy, shp);

	/*	Set invalid position */
	hal_write32((TLITE_SHP_INVALID_POS << 1), shp_base_va + TMU_SHP_CTRL2);

	/*	Set default limits */
	hal_write32(0U, shp_base_va + TMU_SHP_MAX_CREDIT);
	hal_write32(0U, shp_base_va + TMU_SHP_MIN_CREDIT);
}

/**
 * @brief		Initialize scheduler
 * @details		After initialization the scheduler is not connected
 * 				to any queue.
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 */
void pfe_tmu_sch_cfg_init(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);

	hal_write32(0xffffffffU, sch_base_va + TMU_SCH_Q_ALLOC0);
	hal_write32(0xffffffffU, sch_base_va + TMU_SCH_Q_ALLOC1);

	if (0U == sch)
	{
		hal_write32(0xfU, sch_base_va + TMU_SCH_POS);
	}
}

/**
 * @brief		Set rate mode
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	mode The rate mode to be used by scheduler
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_set_rate_mode(void *cbus_base_va, pfe_ct_phy_if_id_t phy,
		uint8_t sch, pfe_tmu_rate_mode_t mode)
{
	uint32_t reg;
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);

	if (mode == RATE_MODE_DATA_RATE)
	{
		reg = 0U;
	}
	else if  (mode == RATE_MODE_PACKET_RATE)
	{
		reg = 1U;
	}
	else
	{
		return EINVAL;
	}

	hal_write32(reg, sch_base_va + TMU_SCH_BIT_RATE);

	return EOK;
}

/**
 * @brief		Get rate mode
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @return		Current rate mode or RATE_MODE_INVALID in case of error
 */
pfe_tmu_rate_mode_t pfe_tmu_sch_cfg_get_rate_mode(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	uint32_t reg;
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);
	pfe_tmu_rate_mode_t rmode = RATE_MODE_INVALID;

	reg = hal_read32(sch_base_va + TMU_SCH_BIT_RATE);

	if (0U == reg)
	{
		rmode = RATE_MODE_DATA_RATE;
	}
	else if (1U == reg)
	{
		rmode = RATE_MODE_PACKET_RATE;
	}
	else
	{
		rmode = RATE_MODE_INVALID;
	}

	return rmode;
}

/**
 * @brief		Set scheduler algorithm
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	algo The algorithm to be used
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_set_algo(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch, pfe_tmu_sched_algo_t algo)
{
	uint32_t reg;
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);

	if (algo == SCHED_ALGO_PQ)
	{
		reg = 0U;
	}
	else if (algo == SCHED_ALGO_DWRR)
	{
		reg = 2U;
	}
	else if (algo == SCHED_ALGO_RR)
	{
		reg = 3U;
	}
	else if (algo == SCHED_ALGO_WRR)
	{
		if (RATE_MODE_PACKET_RATE != pfe_tmu_sch_cfg_get_rate_mode(cbus_base_va, phy, sch))
		{
			/*	See RTL and WRR pseudocode */
			NXP_LOG_ERROR("WRR only supported in Packet Rate scheduler mode\n");
			return EINVAL;
		}
		else
		{
			reg = 4U;
		}
	}
	else
	{
		return EINVAL;
	}

	hal_write32(reg, sch_base_va + TMU_SCH_CTRL);

	return EOK;
}

/**
 * @brief		Get scheduler algorithm
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	algo The algorithm to be used
 * @return		EOK if success, error code otherwise
 */
pfe_tmu_sched_algo_t pfe_tmu_sch_cfg_get_algo(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	uint32_t reg;
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);
	pfe_tmu_sched_algo_t algo = SCHED_ALGO_INVALID;

	reg = hal_read32(sch_base_va + TMU_SCH_CTRL);

	switch (reg & 0xfU)
	{
		case 0x0U:
		{
			algo = SCHED_ALGO_PQ;
			break;
		}

		case 0x2U:
		{
			algo = SCHED_ALGO_DWRR;
			break;
		}

		case 0x3U:
		{
			algo = SCHED_ALGO_RR;
			break;
		}

		case 0x4U:
		{
			algo = SCHED_ALGO_WRR;
			break;
		}

		default:
		{
			algo = SCHED_ALGO_INVALID;
			break;
		}
	}

	return algo;
}

/**
 * @brief		Set input weight
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	input Scheduler input
 * @param[in]	weight The weight value to be used by chosen scheduling algorithm
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_set_input_weight(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint32_t weight)
{
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);

	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		return EINVAL;
	}

	hal_write32(weight, sch_base_va + TMU_SCH_Qn_WGHT(input));

	return EOK;
}

/**
 * @brief		Get input weight
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	input Scheduler input
 * @return		The programmed weight value to be used by chosen scheduling algorithm
 */
uint32_t pfe_tmu_sch_cfg_get_input_weight(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);

	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		return 0U;
	}

	return hal_read32(sch_base_va + TMU_SCH_Qn_WGHT(input));
}

/**
 * @brief		Connect queue to some scheduler input
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	input Scheduler input the queue shall be connected to
 * @param[in]	queue Queue to be connected to the scheduler input. 0xff will invalidate
 * 					the input.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_bind_queue(void *cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint8_t queue)
{
	uint32_t reg;
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);

	if ((queue >= TLITE_PHY_QUEUES_CNT) && (queue != TLITE_SCH_INVALID_INPUT))
	{
		NXP_LOG_ERROR("Invalid queue\n");
		return EINVAL;
	}

	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		return EINVAL;
	}

	/*	Update appropriate "ALLOC_Q" register */
	reg = hal_read32(sch_base_va + TMU_SCH_Q_ALLOCn(input / 4U));
	reg &= ~(0xffU << (8U * (input % 4U)));
	reg |= ((queue & 0x1fU) << (8U * (input % 4U)));
	hal_write32(reg, sch_base_va + TMU_SCH_Q_ALLOCn(input / 4U));

	return EOK;
}

/**
 * @brief		Get queue connected to given scheduler input
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	input Scheduler input to be queried
 * @return		Queue ID connected to the input or PFE_TMU_INVALID_QUEUE if not present
 */
uint8_t pfe_tmu_sch_cfg_get_bound_queue(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	uint32_t reg;
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);
	uint8_t queue;

	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		return PFE_TMU_INVALID_QUEUE;
	}

	reg = hal_read32(sch_base_va + TMU_SCH_Q_ALLOCn(input / 4U));
	queue = (reg >> (8U * (input % 4U))) & 0xffU;

	return (queue >= TLITE_PHY_QUEUES_CNT) ? PFE_TMU_INVALID_QUEUE : queue;
}

/**
 * @brief		Connect output of some other scheduler to current scheduler input
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	src_sch Source scheduler instance/index
 * @param[in]	dst_sch Destination scheduler instance/index
 * @param[in]	input The 'dst_sch' scheduler input where the output of 'src_sch' shall be connected
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_bind_sched_output(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t src_sch, uint8_t dst_sch, uint8_t input)
{
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, src_sch);

	/*	Scheduler0 -> Scheduler1 is the only possible option */
	if ((src_sch != 0U) || (dst_sch != 1U))
	{
		NXP_LOG_ERROR("Scheduler 0 output can only be connected to Scheduler 1 input\n");
		return EINVAL;
	}

	/*	Invalidate the original Scheduler1 input */
	if (EOK != pfe_tmu_sch_cfg_bind_queue(cbus_base_va, phy, dst_sch, input, PFE_TMU_INVALID_QUEUE))
	{
		return EINVAL;
	}

	/*	Connect Scheduler0 to given Scheduler1 input */
	hal_write32(input & 0xfU, sch_base_va + TMU_SCH_POS);

	return EOK;
}

/**
 * @brief		Get scheduler which output is connected to given scheduler input
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	input Scheduler input to be queried
 * @return		ID of the connected scheduler or PFE_TMU_INVALID_SCHEDULER
 */
uint8_t pfe_tmu_sch_cfg_get_bound_sched_output(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	void *sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phy, sch);
	uint32_t reg;

	/*	Scheduler0 -> Scheduler1 is the only possible option */
	if (sch != 1U)
	{
		return PFE_TMU_INVALID_SCHEDULER;
	}

	reg = hal_read32(sch_base_va + TMU_SCH_POS) & 0xffU;

	if (input == reg)
	{
		return 0U;
	}
	else
	{
		return PFE_TMU_INVALID_SCHEDULER;
	}
}

/**
 * @brief		Get TMU statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the TMU block.
 * @param[in]	base_va 	Base address of TMU register space (virtual)
 * @param[in]	buf 		Pointer to buffer to be written
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 *
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_tmu_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg, ii, queue, zone;
	uint8_t prob;
	uint32_t level, drops, tx;

	/* Debug registers */
	if(verb_level >= 10U)
	{
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_PHY_INQ_PKTPTR  : 0x%x\n", hal_read32(base_va + TMU_PHY_INQ_PKTPTR));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_PHY_INQ_PKTINFO : 0x%x\n", hal_read32(base_va + TMU_PHY_INQ_PKTINFO));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_PHY_INQ_STAT    : 0x%x\n", hal_read32(base_va + TMU_PHY_INQ_STAT));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_DBG_BUS_TOP     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_TOP));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_DBG_BUS_PP0     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP0));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_DBG_BUS_PP1     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP1));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_DBG_BUS_PP2     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP2));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_DBG_BUS_PP3     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP3));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_DBG_BUS_PP4     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP4));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "TMU_DBG_BUS_PP5     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP5));
	}

	if(verb_level >= 9U)
	{
		/*	Get version */
		reg = hal_read32(base_va + TMU_VERSION);
		len += oal_util_snprintf(buf + len, size - len, "Revision             : 0x%x\n", (reg >> 24) & 0xff);
		len += oal_util_snprintf(buf + len, size - len, "Version              : 0x%x\n", (reg >> 16) & 0xff);
		len += oal_util_snprintf(buf + len, size - len, "ID                   : 0x%x\n", reg & 0xffff);
	}

	reg = hal_read32(base_va + TMU_CTRL);
	len += oal_util_snprintf(buf + len, size - len, "TMU_CTRL             : 0x%x\n", reg);
	reg = hal_read32(base_va + TMU_PHY_INQ_STAT);
	len += oal_util_snprintf(buf + len, size - len, "TMU_PHY_INQ_STAT     : 0x%x\n", reg);
	reg = hal_read32(base_va + TMU_PHY_INQ_PKTPTR);
	len += oal_util_snprintf(buf + len, size - len, "TMU_PHY_INQ_PKTPTR   : 0x%x\n", reg);
	reg = hal_read32(base_va + TMU_PHY_INQ_PKTINFO);
	len += oal_util_snprintf(buf + len, size - len, "TMU_PHY_INQ_PKTINFO  : 0x%x\n", reg);

	/*	Print per-queue statistics */
	for (ii=0U; ii < TLITE_PHYS_CNT; ii++)
	{
		len += oal_util_snprintf(buf + len, size - len, "[PHY: %d]\n", (int_t)ii);
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			/*	Fill level */
			level = (EOK != pfe_tmu_q_cfg_get_fill_level(base_va, ii, queue, &reg)) ? 0xffffffffU : reg;

			/*	Number of dropped packets */
			drops = (EOK != pfe_tmu_q_cfg_get_drop_count(base_va, ii, queue, &reg)) ? 0xffffffffU : reg;

			/*	Transmitted packets */
			tx = (EOK != pfe_tmu_q_cfg_get_tx_count(base_va, ii, queue, &reg)) ? 0xffffffffU : reg;

			if ((0U == level) && (0U == drops) && (0U == tx))
			{
				/*	Don't print emtpy queues */
				continue;
			}

			len += oal_util_snprintf(buf + len, size - len, "  [QUEUE: %d]\n", queue);

			/*	curQ_cfg is @ position 4 per queue */
			if (EOK != pfe_tmu_cntx_mem_read(base_va, ii, (8U * queue) + 4U, &reg))
			{
				NXP_LOG_ERROR("    Context memory read failed\n");
				continue;
			}

			/*	Configuration */
			switch (reg & 0x3U)
			{
				case 0x0U:
				{
					len += oal_util_snprintf(buf + len, size - len, "    Mode       : Default\n");
					break;
				}

				case 0x1U:
				{
					len += oal_util_snprintf(buf + len, size - len, "    Mode       : Tail drop (max: %d)\n", (reg >> 11) & 0x1ffU);
					break;
				}

				case 0x2U:
				{
					len += oal_util_snprintf(buf + len, size - len, "    Mode       : WRED (max: %d, min: %d)\n", (reg >> 11) & 0x1ffU, (reg >> 2) & 0x1ffU);
					for (zone=0U; zone<pfe_tmu_q_get_wred_zones(base_va, ii, queue); zone++)
					{
						if (EOK != pfe_tmu_q_get_wred_probability(base_va, ii, queue, zone, &prob))
						{
							len += oal_util_snprintf(buf + len, size - len, "      Zone %d   : ERROR\n", zone);
						}
						else
						{
							len += oal_util_snprintf(buf + len, size - len, "      Zone %d   : %d\n", zone, prob);
						}
					}

					break;
				}

				default:
				{
					len += oal_util_snprintf(buf + len, size - len, "    Mode       : ERROR\n");
					break;
				}
			}

			len += oal_util_snprintf(buf + len, size - len, "    Fill level : % 8d Drops: % 8d, TX: % 8d\n", level, drops, tx);
		}
	}


	return len;
}

/** @}*/
