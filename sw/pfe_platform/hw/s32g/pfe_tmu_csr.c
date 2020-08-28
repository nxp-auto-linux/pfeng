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

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_tmu_csr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#if (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) && (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14)
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

static const pfe_ct_phy_if_id_t phys[] = 
{
    PFE_PHY_IF_ID_EMAC0,
    PFE_PHY_IF_ID_EMAC1,
    PFE_PHY_IF_ID_EMAC2,
    PFE_PHY_IF_ID_HIF_NOCPY,
    PFE_PHY_IF_ID_HIF,
    PFE_PHY_IF_ID_UTIL
};

static errno_t pfe_tmu_cntx_mem_write(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t data);
static errno_t pfe_tmu_cntx_mem_read(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t *data);

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
	for (ii=0U; ii < (sizeof(phys) / sizeof(pfe_ct_phy_if_id_t)); ii++)
	{
		/*	Initialize queues */
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			hal_write32(0x1U, cbus_base_va + TMU_CNTX_ACCESS_CTRL);
			hal_write32((((uint32_t)phys[ii] & 0x1fU) << 8) | (queue & 0x7U), cbus_base_va + TMU_PHY_QUEUE_SEL);
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

		/*	Initialize HW schedulers/shapers */
		pfe_tmu_sch_cfg_init(cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phys[ii], 0U));
		pfe_tmu_sch_cfg_init(cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phys[ii], 1U));

		pfe_tmu_shp_cfg_init(cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phys[ii], 0U));
		pfe_tmu_shp_cfg_init(cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phys[ii], 1U));
		pfe_tmu_shp_cfg_init(cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phys[ii], 2U));
		pfe_tmu_shp_cfg_init(cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR(phys[ii], 3U));

		/*	Set default topology:
			 - all shapers are disabled and not associated with any queue
			 - queue[n]->SCH0.input[n], SCH0.out->SCH1.input[0]
		*/
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			/*	Scheduler 0 */
			if (EOK != pfe_tmu_sch_cfg_bind_queue(cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phys[ii], 0U), queue, queue))
			{
				NXP_LOG_ERROR("Can't bind queue to scheduler\n");
				return ENOEXEC;
			}
		}

		/*	Connect Scheduler 0 output to Scheduler 1 input 0 */
		if (EOK != pfe_tmu_sch_cfg_bind_sch_output(cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phys[ii], 1U), 0U,
										cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phys[ii], 0U), cbus_base_va))
		{
			NXP_LOG_ERROR("Can't bind scheduler output to another scheduler\n");
			return ENOEXEC;
		}

		/*	Scheduler 1. Make sure that the input 0 is not connected to any queue (because of Scheduler 0 output) */
		if (EOK != pfe_tmu_sch_cfg_bind_queue(cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR(phys[ii], 1U), 0U, 0xffU))
		{
			NXP_LOG_ERROR("Can't bind queue to scheduler\n");
			return ENOEXEC;
		}

		/*	Set default queue mode */
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			/* ret = pfe_tmu_q_mode_set_default(cbus_base_va, phys[ii], queue); */
			ret = pfe_tmu_q_mode_set_tail_drop(cbus_base_va, phys[ii], queue, 255U);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Can't set default queue mode\n");
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
	/*	curQ_trans_cnt is @ position 3 per queue */
	return pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue) + 3U, cnt);
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
		return EINVAL;
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
 * @brief		Get number of queues per given PHY
 * @param[in]	phy The physical interface
 * @return		Number of available queues
 */
uint8_t pfe_tmu_cfg_get_q_count(pfe_ct_phy_if_id_t phy)
{
	if (phy > PFE_PHY_IF_ID_MAX)
	{
		NXP_LOG_ERROR("Invalid PHY ID (%d)\n", phy);
		return 0U;
	}
	else
	{
		return TLITE_PHY_QUEUES_CNT;
	}
}

/**
 * @brief		Set shaper credit limits
 * @details		Value units depend on chosen shaper mode
 * @param[in]	shp_base_va Shaper base address (VA)
 * @param[in]	max_credit Maximum credit value. Must be positive.
 * @param[in]	min_credit Minimum credit value. Must be negative.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_shp_cfg_set_limits(void *shp_base_va, int32_t max_credit, int32_t min_credit)
{
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
 * @brief		Enable shaper
 * @param[in]	cbus_base_va CBUS base address (VA)
 * @param[in]	shp_base_va Shaper base address (VA)
 * @param[in]	mode Shaper mode
 * @param[in]	isl Idle slope in units per second as given by chosen mode
 *					(bits-per-second, packets-per-second)
 */
errno_t pfe_tmu_shp_cfg_enable(void *cbus_base_va, void *shp_base_va, pfe_tmu_rate_mode_t mode, uint32_t isl)
{
	uint32_t reg, wgt;
	uint32_t sys_clk_hz;
	static const uint32_t clk_div = 256U;

	reg = hal_read32(cbus_base_va + WSP_CLK_FRQ);
	sys_clk_hz = (reg & 0xffffU) * 1000000U;
	NXP_LOG_INFO("TMU: Using PFE sys_clk value %dHz\n", sys_clk_hz);

	if (RATE_MODE_DATA_RATE == mode)
	{
		wgt = (isl * clk_div * (2U << 12)) / (8U * sys_clk_hz); /* [bytes] */
	}
	else if (RATE_MODE_PACKET_RATE == mode)
	{
		NXP_LOG_ERROR("Packet rate mode not implemented\n");
		return EINVAL;
	}
	else
	{
		return EINVAL;
	}

	/*	Set weight */
	hal_write32(wgt & 0xfffffU, shp_base_va + TMU_SHP_WGHT);

	NXP_LOG_INFO("Shaper weight set to %d.%d\n", (wgt >> 12), (wgt & 0xfffU));

	/*	Set clock divisor and enable the shaper */
	reg = hal_read32(shp_base_va + TMU_SHP_CTRL) | 0x1U;
	reg |= (clk_div & 0x7fffffffU) << 1;
	hal_write32(reg, shp_base_va + TMU_SHP_CTRL);

	if (0U == clk_div)
	{
		NXP_LOG_WARNING("Shaper clock divider is zero\n");
	}
	else
	{
		NXP_LOG_INFO("Shaper tick is %dHz\n", sys_clk_hz / clk_div);
	}

	return EOK;
}

/**
 * @brief		Disable shaper
 * @param[in]	shp_base_va Shaper base address (VA)
 */
void pfe_tmu_shp_cfg_disable(void *shp_base_va)
{
	uint32_t reg = hal_read32(shp_base_va + TMU_SHP_CTRL) & ~(uint32_t)0x1U;
	hal_write32(reg, shp_base_va + TMU_SHP_CTRL);
}

/**
 * @brief		Initialize shaper
 * @details		After initialization the shaper is disabled and not connected
 * 				to any queue.
 * @param[in]	shp_base_va Shaper base address (VA)
 */
void pfe_tmu_shp_cfg_init(void *shp_base_va)
{
	/*	Disable */
	pfe_tmu_shp_cfg_disable(shp_base_va);

	/*	Set invalid position */
	hal_write32((0x1fU << 1), shp_base_va + TMU_SHP_CTRL2);

	/*	Set default limits */
	hal_write32(0U, shp_base_va + TMU_SHP_MAX_CREDIT);
	hal_write32(0U, shp_base_va + TMU_SHP_MIN_CREDIT);
}

/**
 * @brief		Initialize scheduler
 * @details		After initialization the scheduler is not connected
 * 				to any queue.
 * @param[in]	sch_base_va Scheduler base address (VA)
 */
void pfe_tmu_sch_cfg_init(void *sch_base_va)
{
	hal_write32(0xffffffffU, sch_base_va + TMU_SCH_Q_ALLOC0);
	hal_write32(0xffffffffU, sch_base_va + TMU_SCH_Q_ALLOC1);
}

/**
 * @brief		Set rate mode
 * @param[in]	sch_base_va Scheduler base address (VA)
 * @param[in]	mode The rate mode to be used by scheduler
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_set_rate_mode(void *sch_base_va, pfe_tmu_rate_mode_t mode)
{
	uint32_t reg;

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
 * @brief		Set scheduler algorithm
 * @param[in]	sch_base_va Scheduler base address (VA)
 * @param[in]	algo The algorithm to be used
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_set_algo(void *sch_base_va, pfe_tmu_sched_algo_t algo)
{
	uint32_t reg;

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
		reg = 4U;
	}
	else
	{
		return EINVAL;
	}

	hal_write32(reg, sch_base_va + TMU_SCH_CTRL);

	return EOK;
}

/**
 * @brief		Set input weight
 * @param[in]	sch_base_va Scheduler base address (VA)
 * @param[in]	input Scheduler input
 * @param[in]	weight The weight value to be used by chosen scheduling algorithm
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_set_input_weight(void *sch_base_va, uint8_t input, uint32_t weight)
{
	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		return EINVAL;
	}

	hal_write32(weight, sch_base_va + TMU_SCH_Qn_WGHT(input));

	return EOK;
}

/**
 * @brief		Connect queue to some scheduler input
 * @param[in]	sch_base_va Scheduler base address (VA)
 * @param[in]	input Scheduler input the queue shall be connected to
 * @param[in]	queue Queue to be connected to the scheduler input. 0xff will invalidate
 * 					the input.
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_bind_queue(void *sch_base_va, uint8_t input, uint8_t queue)
{
	uint32_t reg;

	if ((queue >= TLITE_PHY_QUEUES_CNT) && (queue != 0xffU))
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
 * @brief		Connect another scheduler output to some scheduler input
 * @param[in]	sch_base_va Scheduler base address (VA)
 * @param[in]	input Scheduler input the queue shall be connected to
 * @param[in]	sch_base_va_out Base address of scheduler (VA) which output shall be connected
 *							to the scheduler input. 0xff will invalidate the input.
 * @param[in]	cbus_base CBUS base address (VA).
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_sch_cfg_bind_sch_output(void *sch_base_va, uint8_t input, void *sch_base_va_out, void *cbus_base)
{
	addr_t sched_offset;

	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		return EINVAL;
	}

	/*	Find out which HW scheduler is referenced by 'sch_base_va' */
	sched_offset = ((addr_t)sch_base_va_out - (addr_t)cbus_base) & TLITE_SCHED_OFFSET_MASK;
	if (TLITE_SCHED0_BASE_OFFSET != sched_offset)
	{
		/*	'Out' must be scheduler0 */
		NXP_LOG_ERROR("Only connection SCHED0 -> SCHED1 is supported\n");
		return EINVAL;
	}

	/*	Find out which HW scheduler is referenced by 'sch_base_va_out' */
	sched_offset = ((addr_t)sch_base_va - (addr_t)cbus_base) & TLITE_SCHED_OFFSET_MASK;
	if (TLITE_SCHED1_BASE_OFFSET != sched_offset)
	{
		/*	Target must be scheduler1 */
		NXP_LOG_ERROR("Only connection SCHED0 -> SCHED1 is supported\n");
		return EINVAL;
	}

	/*	Connect SCHED0 to given SCHED1 input */
	hal_write32(input & 0xfU, sch_base_va_out + TMU_SCH_POS);

	return EOK;
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
	for (ii=0U; ii < (sizeof(phys) / sizeof(pfe_ct_phy_if_id_t)); ii++)
	{
		len += oal_util_snprintf(buf + len, size - len, "[PHY: %d]\n", (int_t)phys[ii]);
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			/*	Fill level */
			level = (EOK != pfe_tmu_q_cfg_get_fill_level(base_va, phys[ii], queue, &reg)) ? 0xffffffffU : reg;

			/*	Number of dropped packets */
			drops = (EOK != pfe_tmu_q_cfg_get_drop_count(base_va, phys[ii], queue, &reg)) ? 0xffffffffU : reg;

			/*	Transmitted packets */
			tx = (EOK != pfe_tmu_q_cfg_get_tx_count(base_va, phys[ii], queue, &reg)) ? 0xffffffffU : reg;

			if ((0U == level) && (0U == drops) && (0U == tx))
			{
				/*	Don't print emtpy queues */
				continue;
			}

			len += oal_util_snprintf(buf + len, size - len, "  [QUEUE: %d]\n", queue);

			/*	curQ_cfg is @ position 4 per queue */
			if (EOK != pfe_tmu_cntx_mem_read(base_va, phys[ii], (8U * queue) + 4U, &reg))
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
					for (zone=0U; zone<pfe_tmu_q_get_wred_zones(base_va, phys[ii], queue); zone++)
					{
						if (EOK != pfe_tmu_q_get_wred_probability(base_va, phys[ii], queue, zone, &prob))
						{
							len += oal_util_snprintf(buf + len, size - len, "      Zone %d    : ERROR\n", zone);
						}
						else
						{
							len += oal_util_snprintf(buf + len, size - len, "      Zone %d    : reg\n", zone, prob);
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
