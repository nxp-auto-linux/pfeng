/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
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
#include "pfe_feature_mgr.h"
#ifdef PFE_CFG_TARGET_OS_LINUX
#include <linux/iopoll.h>
#endif

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#define CLK_DIV_LOG2 (8U - 1U) /* Value of CLK_DIV_LOG2 log2(clk_div/2) */
#define CLK_DIV ((uint64_t)1U << (CLK_DIV_LOG2 + 1U)) /* 256 */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* usage scope: pfe_tmu_cfg_get_phy_config */
/*	List of QoS configuration for each physical interface terminated with invalid entry */
static const pfe_tmu_phy_cfg_t tmu_phys[] = {
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

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"

#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static errno_t pfe_tmu_cntx_mem_write(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t data);
static errno_t pfe_tmu_cntx_mem_read(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t *data);
static errno_t pfe_tmu_context_memory(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue_temp, uint16_t min, uint16_t max);
static uint8_t pfe_tmu_hif_q_to_tmu_q(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_LINUX
static DEFINE_SPINLOCK(tmu_lock);
#define pfe_tmu_spinlock_lock() spin_lock_bh(&tmu_lock)
#define pfe_tmu_spinlock_unlock() spin_unlock_bh(&tmu_lock)
#else
#define pfe_tmu_spinlock_lock()
#define pfe_tmu_spinlock_unlock()
#endif /* PFE_CFG_TARGET_OS_LINUX */

/* PHY lookup table */
static const pfe_ct_phy_if_id_t phy_if_id_temp[TLITE_PHYS_CNT] =
{
	PFE_PHY_IF_ID_EMAC0, PFE_PHY_IF_ID_EMAC1, PFE_PHY_IF_ID_EMAC2,
	PFE_PHY_IF_ID_HIF, PFE_PHY_IF_ID_HIF_NOCPY, PFE_PHY_IF_ID_UTIL
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CONST_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		Return QoS configuration of given physical interface
 * @param[in]	phy The physical interface to get QoS configuration for
 * @return		Pointer to the configuration or NULL if not found
 */
const pfe_tmu_phy_cfg_t *pfe_tmu_cfg_get_phy_config(pfe_ct_phy_if_id_t phy)
{
	uint32_t ii;

	const pfe_tmu_phy_cfg_t *phy_config = NULL;

	for (ii=0U; tmu_phys[ii].id != PFE_PHY_IF_ID_INVALID; ii++)
	{
		if (tmu_phys[ii].id == phy)
		{
			phy_config = &tmu_phys[ii];
			break;
		}
	}

	return phy_config;
}

/**
 * @brief		Initialize TMU reclaim memory
 * @details		This implements reclaim memory initialization workaround.
 * 				It is necessary to call this function to initialize the ECC for
 * 				TMU reclaim memory.
 * @warning		Function should be called before @ref pfe_tmu_cfg_init.
 * @param[in]	cbus_base_va The cbus base address
 */
void pfe_tmu_reclaim_init(addr_t cbus_base_va)
{
	uint8_t queue;
	uint32_t ii;
	uint32_t dropped_packets = 0U;
	uint32_t retries = 0U;

	hal_write32(0x1U, cbus_base_va + TMU_CNTX_ACCESS_CTRL);

	/*	Initialize queues */
	for (ii = 0U; ii < (uint32_t)TLITE_PHYS_CNT; ii++)
	{
		for (queue = 0U; queue < (uint8_t)TLITE_PHY_QUEUES_CNT; queue++)
		{
			hal_write32(((ii & 0x1fUL) << 8U) | ((uint32_t)queue & 0x7UL), cbus_base_va + TMU_PHY_QUEUE_SEL);
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
	}

	if (FALSE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
	{
		/* Queue 0 PHY 0*/
		/* WRED min 0 max 0*/
		if(EOK == pfe_tmu_context_memory(cbus_base_va, PFE_PHY_IF_ID_EMAC0, 0U, 0U, 0U))
		{

			/* Initialize internal TMU FIFO (length is hard coded in verilog)*/
			for(ii = 0U; ii < (uint32_t)(TLITE_INQ_FIFODEPTH / 2U); ii++)
			{
				/* 64bit write, it covers both registers TMU_PHY_INQ_PKTPTR and TMU_PHY_INQ_PKTINFO */
				hal_write64(0ULL, cbus_base_va + TMU_PHY_INQ_PKTPTR);
			}

			do
			{
				oal_time_usleep(10U);
				/*	Queue 0 */
				/*	curQ_drop_cnt is @ position 2 per queue */
				(void)pfe_tmu_cntx_mem_read(cbus_base_va, PFE_PHY_IF_ID_EMAC0, (8U * 0U) + 2U, &dropped_packets);

				retries++;
			}
			while ((TLITE_INQ_FIFODEPTH != dropped_packets) && (10U > retries));

			if (dropped_packets != TLITE_INQ_FIFODEPTH)
			{
				NXP_LOG_ERROR("Failed to initialize TMU reclaim memory %u\n", (uint_t)dropped_packets);
			}

			/* Set queue to default mode */
			(void)pfe_tmu_q_mode_set_default(cbus_base_va, PFE_PHY_IF_ID_EMAC0, 0U);
		}
	}
}

errno_t pfe_tmu_q_reset_tail_drop_policy(addr_t cbus_base_va)
{
	uint8_t queue;
	uint32_t ii;
	errno_t ret;

	for (ii = 0U; ii < (uint32_t)TLITE_PHYS_CNT; ii++)
	{
		if ((uint32_t)PFE_PHY_IF_ID_EMAC2 >= ii)
		{
            /* EMACs - for endpoint performance improvement */
			ret = pfe_tmu_q_mode_set_tail_drop(cbus_base_va, phy_if_id_temp[ii], 0U, TLITE_OPT_Q0_SIZE);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Can't set the default queue size for PHY#%u queue 0: %d\n", (uint_t)ii, (int_t)ret);
				break;
			}
			else
			{

				for (queue = 1U; queue < (uint8_t)TLITE_PHY_QUEUES_CNT; queue++)
				{
					ret = pfe_tmu_q_mode_set_tail_drop(cbus_base_va, phy_if_id_temp[ii], queue, (uint16_t)TLITE_OPT_Q1_7_SIZE);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Can't set the default queue size for PHY#%u queue %hhu: %d\n", (uint_t)ii, queue, (int_t)ret);
						break;
					}
				}
			}
		}
		else if((uint32_t)PFE_PHY_IF_ID_HIF == ii)
		{   /* HIF - special case for ERR051211 workaround */
			for (queue = 0U; queue < TLITE_PHY_QUEUES_CNT; queue++)
			{
				ret = pfe_tmu_q_mode_set_tail_drop(cbus_base_va, phy_if_id_temp[ii], queue, TLITE_HIF_MAX_Q_SIZE);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't set the default queue size for PHY#%u queue %hhu: %d\n", (uint_t)ii, queue, (int_t)ret);
					break;
				}
			}
		}
		else
		{   /* Other: UTIL, HIF_NOCPY */
			for (queue = 0U; queue < (uint8_t)TLITE_PHY_QUEUES_CNT; queue++)
			{
				ret = pfe_tmu_q_mode_set_tail_drop(cbus_base_va, phy_if_id_temp[ii], queue, (uint16_t)TLITE_MAX_Q_SIZE);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't set the default queue size for PHY#%u queue %hhu: %d\n", (uint_t)ii, queue, (int_t)ret);
					break;
				}
			}
		}
		if (EOK != ret)
		{
			return ret;
		}
	}

	return ret;
}

/**
 * @brief		Initialize and configure the TMU
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	cfg Pointer to the configuration structure
 * @return		EOK if success
 */
errno_t pfe_tmu_cfg_init(addr_t cbus_base_va, const pfe_tmu_cfg_t *cfg)
{
	uint8_t queue;
	uint32_t ii;
	errno_t ret = EOK;
	uint32_t regval;

	if (TRUE == cfg->on_g3)
	{
		regval = hal_read32(cbus_base_va + TMU_TEQ_CTRL);
		regval |= 0x4U;
		hal_write32(regval, cbus_base_va + TMU_TEQ_CTRL);
	}

	hal_write32(0x0U, cbus_base_va + TMU_PHY0_TDQ_CTRL);
	hal_write32(0x0U, cbus_base_va + TMU_PHY1_TDQ_CTRL);
	hal_write32(0x0U, cbus_base_va + TMU_PHY2_TDQ_CTRL);
	hal_write32(0x0U, cbus_base_va + TMU_PHY3_TDQ_CTRL);
	hal_write32(0x0U, cbus_base_va + TMU_PHY5_TDQ_CTRL);	/* UTIL PE */

	/*	Reset */
	pfe_tmu_cfg_reset(cbus_base_va);

	/*	INQ */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_EGPI1_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY0_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_EGPI2_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY1_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_EGPI3_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY2_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_HGPI_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY16_INQ_ADDR);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_HGPI_BASE_ADDR + GPI_INQ_PKTPTR, cbus_base_va + TMU_PHY3_INQ_ADDR);
	/* The macro UTIL_INQ_PKTPTR already contains the CBUS_UTIL_CSR_BASE_ADDR (difference to above lines) */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + UTIL_INQ_PKTPTR, cbus_base_va + TMU_PHY5_INQ_ADDR); /* UTIL */

	/*	Context memory initialization */
	for (ii = 0U; ii < (uint32_t)TLITE_PHYS_CNT; ii++)
	{
		/* NOTE: Do not access the direct registers here it may result in bus fault.*/

		/* TDQ and SHP address range for PHY5(Util) does not exist on G2 */
		if ((TRUE == cfg->on_g3) || ((FALSE == cfg->on_g3) && (PFE_PHY_IF_ID_UTIL != phy_if_id_temp[ii])))
		{
			/*	Initialize HW schedulers. Invalidate all inputs. */
			pfe_tmu_sch_cfg_init(cbus_base_va, phy_if_id_temp[ii], 0U);
			pfe_tmu_sch_cfg_init(cbus_base_va, phy_if_id_temp[ii], 1U);

			/*	Initialize shapers. Make sure they are not connected. */
			pfe_tmu_shp_cfg_init(cbus_base_va, phy_if_id_temp[ii], 0U);
			pfe_tmu_shp_cfg_init(cbus_base_va, phy_if_id_temp[ii], 1U);
			pfe_tmu_shp_cfg_init(cbus_base_va, phy_if_id_temp[ii], 2U);
			pfe_tmu_shp_cfg_init(cbus_base_va, phy_if_id_temp[ii], 3U);

			/*	Set default topology:
				 - All shapers are disabled and not associated with any queue
				 - Scheduler 0 is not used
				 - Queue[n]->SCH1.input[n]
			*/
			for (queue = 0U; queue < (uint8_t)TLITE_PHY_QUEUES_CNT; queue++)
			{
				/*	Scheduler 1 */
				ret = pfe_tmu_sch_cfg_bind_queue(cbus_base_va, phy_if_id_temp[ii], 1U, queue, queue);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't bind queue to scheduler: %d\n", ret);
					ret = ENOEXEC;
					break;
				}
			}

			if (EOK == ret)
			{
				ret = pfe_tmu_sch_cfg_set_rate_mode(cbus_base_va, phy_if_id_temp[ii], 1U, RATE_MODE_DATA_RATE);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not set scheduler 1 rate mode: %d\n", ret);
					ret = ENOEXEC;
				}
				else
				{
					ret = pfe_tmu_sch_cfg_set_algo(cbus_base_va, phy_if_id_temp[ii], 1U, SCHED_ALGO_RR);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Could not set scheduler 1 algo: %d\n", ret);
						ret = ENOEXEC;
					}
				}
			}

		}

		if (EOK == ret)
		{
			/*	Set default queue mode */
			for (queue = 0U; queue < (uint8_t)TLITE_PHY_QUEUES_CNT; queue++)
			{
				if((uint32_t)PFE_PHY_IF_ID_HIF == ii)
				{   /* HIF - special case for ERR051211 workaround */
					ret = pfe_tmu_q_mode_set_tail_drop(cbus_base_va, phy_if_id_temp[ii], queue, TLITE_HIF_MAX_Q_SIZE);
				}
				else
				{   /* Other */
					ret = pfe_tmu_q_mode_set_tail_drop(cbus_base_va, phy_if_id_temp[ii], queue, TLITE_MAX_Q_SIZE);
				}

				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't set default queue mode: %d\n", ret);
					break;
				}
			}
		}
		/* Incase one of the ret != EOK break */
		if (EOK != ret)
		{
			break;
		}
	}

	if (EOK == ret)
	{
		hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_FREE_CTRL, cbus_base_va + TMU_BMU_INQ_ADDR);
		hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU2_BASE_ADDR + BMU_FREE_CTRL, cbus_base_va + TMU_BMU2_INQ_ADDR);
		hal_write32(0x100U, cbus_base_va + TMU_AFULL_THRES);
		hal_write32(0xfcU, cbus_base_va + TMU_INQ_WATERMARK);
		hal_write32(0xfU, cbus_base_va + TMU_PHY0_TDQ_CTRL);
		hal_write32(0xfU, cbus_base_va + TMU_PHY1_TDQ_CTRL);
		hal_write32(0xfU, cbus_base_va + TMU_PHY2_TDQ_CTRL);
		hal_write32(0xfU, cbus_base_va + TMU_PHY16_TDQ_CTRL);
		hal_write32(0xfU, cbus_base_va + TMU_PHY3_TDQ_CTRL);
		hal_write32(0xfU, cbus_base_va + TMU_PHY5_TDQ_CTRL);	/* UTIL */
	}

	return ret;
}

/**
 * @brief		Issue TMU reset
 * @param[in]	cbus_base_va The cbus base address
 */
void pfe_tmu_cfg_reset(addr_t cbus_base_va)
{
	uint32_t timeout = 200U;
	uint32_t reg;

	hal_write32(0x1U, cbus_base_va + TMU_CTRL);

	do
	{
		oal_time_usleep(10U);
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
void pfe_tmu_cfg_enable(addr_t cbus_base_va)
{
	/*	nop */
	(void)cbus_base_va;
}

/**
 * @brief		Disable the TMU block
 * @param[in]	base_va The cbus base address
 */
void pfe_tmu_cfg_disable(addr_t cbus_base_va)
{
	/*	nop */
	(void)cbus_base_va;
}

/**
 * @brief		Write TMU context memory
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	loc Location to be written
 * @param[out]	data Data to be written
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_tmu_cntx_mem_write(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t data)
{
	uint32_t reg;
	uint32_t timeout = 200U;
	pfe_ct_phy_if_id_t phy_temp = phy;
	errno_t ret = EOK;

	pfe_tmu_spinlock_lock();

	hal_write32(0U, cbus_base_va + TMU_CNTX_ACCESS_CTRL);

	switch (phy)
	{
		case PFE_PHY_IF_ID_EMAC0:
		case PFE_PHY_IF_ID_EMAC1:
		case PFE_PHY_IF_ID_EMAC2:
		case PFE_PHY_IF_ID_HIF_NOCPY:
		case PFE_PHY_IF_ID_UTIL:
			/*Do Nothing*/
			break;
		case PFE_PHY_IF_ID_HIF:
		case PFE_PHY_IF_ID_HIF0:
		case PFE_PHY_IF_ID_HIF1:
		case PFE_PHY_IF_ID_HIF2:
		case PFE_PHY_IF_ID_HIF3:
			phy_temp = PFE_PHY_IF_ID_HIF;
			break;
		default:
			ret = EINVAL;
			break;
	}

	if(ret == EOK)
	{
		hal_write32((((uint32_t)phy_temp & (uint32_t)0x1fU) << 16U) | (uint32_t)loc, cbus_base_va + TMU_CNTX_ADDR);
		hal_write32(data, cbus_base_va + TMU_CNTX_DATA);
		hal_write32(0x3U, cbus_base_va + TMU_CNTX_CMD);
		do
		{
			oal_time_usleep(1U);
			timeout--;
			reg = hal_read32(cbus_base_va + TMU_CNTX_CMD);
		} while ((0U == (reg & 0x4U)) && (timeout > 0U));

		if (0U == timeout)
		{
			ret = ETIMEDOUT;
		}
	}

	pfe_tmu_spinlock_unlock();

	return ret;
}

/**
 * @brief		Read TMU context memory
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	loc Location to be read
 * @param[out]	data Pointer to memory where read data shall be written
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_tmu_cntx_mem_read(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t loc, uint32_t *data)
{
	uint32_t reg;
	uint32_t timeout = 20U;
	pfe_ct_phy_if_id_t phy_temp = phy;
	errno_t ret = EOK;

	pfe_tmu_spinlock_lock();

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
			phy_temp = PFE_PHY_IF_ID_HIF;
			break;
		}
		default:
		{
			ret = EINVAL;
			break;
		}
	}


	if(ret == EOK)
	{
		hal_write32((((uint32_t)phy_temp & (uint32_t)0x1fU) << 16U) | (uint32_t)loc, cbus_base_va + TMU_CNTX_ADDR);
		hal_write32(0x2U, cbus_base_va + TMU_CNTX_CMD);

#ifdef PFE_CFG_TARGET_OS_LINUX
		ret = readx_poll_timeout_atomic(hal_read32, cbus_base_va + TMU_CNTX_CMD, reg, (reg & 0x4U), 2, 2 * 5 * timeout);
		*data = hal_read32(cbus_base_va + TMU_CNTX_DATA);
		ret = -ret;
#else
		do
		{
			oal_time_usleep(10U);
			timeout--;
			reg = hal_read32(cbus_base_va + TMU_CNTX_CMD);
		} while ((0U == (reg & 0x4U)) && (timeout > 0U));

		if (0U == timeout)
		{
			ret = ETIMEDOUT;
		}
		else
		{
			*data = hal_read32(cbus_base_va + TMU_CNTX_DATA);
		}
#endif
	}

	pfe_tmu_spinlock_unlock();

	return ret;
}

static uint8_t pfe_tmu_hif_q_to_tmu_q(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
	uint32_t reg, ii;
	int8_t hif_queue = -1;
	uint8_t tmu_queue = PFE_TMU_INVALID_QUEUE;

	/*	Convert HIF channel `queue` (range 0-`n`) to TMU queue (range 0-`m`) */
	if ((phy == PFE_PHY_IF_ID_HIF0)
			|| (phy == PFE_PHY_IF_ID_HIF1)
			|| (phy == PFE_PHY_IF_ID_HIF2)
			|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		reg = hal_read32(cbus_base_va + CBUS_HIF_BASE_ADDR + HIF_RX_QUEUE_MAP_CH_NO_ADDR);
		for (ii=0U; ii<8U; ii++)
		{
			if ((uint32_t)((uint32_t)phy - (uint32_t)PFE_PHY_IF_ID_HIF0) == ((reg >> (ii * 4U)) & 0xfU))
			{
				hif_queue++;
				if (queue == (uint8_t)hif_queue)
				{
					tmu_queue = (uint8_t)ii;
					break;
				}
			}
		}
	}

	return tmu_queue;
}

static errno_t pfe_tmu_context_memory(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue_temp, uint16_t min, uint16_t max)
{
	uint32_t reg;
	errno_t ret;
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
	ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue_temp) + 5U, reg);
	if (EOK == ret)
	{
		ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue_temp) + 6U, reg);
		if (EOK == ret)
		{
			/*	curQ_Qmax[8:0], curQ_Qmin[8:0], curQ_cfg[1:0] are @ position 4 per queue */
			reg = ((uint32_t)max << 11U) | ((uint32_t)min << 2U) | 0x2UL;
			ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue_temp) + 4U, reg);
		}
	}
	return ret;
}

/**
 * @brief		Get number of packets in the queue
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[out]	level Pointer to memory where the fill level value shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_cfg_get_fill_level(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *level)
{
	uint8_t queue_temp = queue;
	errno_t ret = EOK;

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		queue_temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == queue_temp)
		{
			ret = EINVAL;
		}
	}

	if (EOK == ret)
	{
		/*	curQ_pkt_cnt is @ position 1 per queue */
		ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue_temp) + 1U, level);
	}
	return ret;
}

/**
 * @brief		Get number of dropped packets for the queue
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[out]	cnt Pointer to memory where the drop count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_cfg_get_drop_count(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt)
{
	uint32_t drops;
	errno_t ret = EOK;
	uint8_t temp = queue;
	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == temp)
		{
			ret = EINVAL;
		}
	}

	if (EOK == ret)
	{
		/*	curQ_drop_cnt is @ position 2 per queue */
		ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * temp) + 2U, &drops);

		/* S32G2: Mitigate side effect of TMU reclaim memory workaround */
		if (EOK == ret)
		{
			if ((phy == PFE_PHY_IF_ID_EMAC0) &&
				(0U == queue) &&
				(FALSE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3)))
			{
				*cnt = drops - TLITE_INQ_FIFODEPTH;
			}
			else
			{
				*cnt = drops;
			}
		}
	}
	return ret;
}

/**
 * @brief		Get number of transmitted packets for the queue
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @param[out]	cnt Pointer to memory where the TX count shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_tmu_q_cfg_get_tx_count(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt)
{
	uint8_t temp = queue;
	errno_t ret = EOK;

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == temp)
		{
			ret = EINVAL;
		}
	}

	if (EOK == ret)
	{
		/*	curQ_trans_cnt is @ position 3 per queue */
		ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * temp) + 3U, cnt);
	}

	return ret;
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
pfe_tmu_queue_mode_t pfe_tmu_q_get_mode(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *min, uint32_t *max)
{
	uint32_t reg;
	errno_t ret;
	pfe_tmu_queue_mode_t mode = TMU_Q_MODE_DEFAULT;
	uint8_t temp = queue;

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == temp)
		{
			mode = TMU_Q_MODE_INVALID;
		}
	}
	if (TMU_Q_MODE_DEFAULT == mode)
	{
		/*	curQ_Qmax[8:0], curQ_Qmin[8:0], curQ_cfg[1:0] are @ position 4 per queue */
		ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * temp) + 4U, &reg);
		if (EOK != ret)
		{
			mode = TMU_Q_MODE_INVALID;
		}
		else
		{
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
errno_t pfe_tmu_q_mode_set_default(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue)
{
	uint8_t temp = queue;
	errno_t ret = EOK;

	if ((phy == PFE_PHY_IF_ID_HIF0)
				|| (phy == PFE_PHY_IF_ID_HIF1)
				|| (phy == PFE_PHY_IF_ID_HIF2)
				|| (phy == PFE_PHY_IF_ID_HIF3))
	{
		temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
		if (PFE_TMU_INVALID_QUEUE == temp)
		{
			ret = EINVAL;
		}
	}

	if (EOK == ret)
	{

		/*	If bit 1 is zero then in case when LLM is full the TMU will wait. */
		hal_write32((uint32_t)0x0U | ((uint32_t)0x0U << 1), cbus_base_va + TMU_TEQ_CTRL);

		/*	Put the queue to default mode */
		/*	curQ_Qmax[8:0], curQ_Qmin[8:0], curQ_cfg[1:0] are @ position 4 per queue */
		ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * temp) + 4U, 0U);
	}

	return ret;
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
errno_t pfe_tmu_q_mode_set_tail_drop(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint16_t max)
{
	uint32_t reg;
	uint8_t queue_temp = queue;
	errno_t ret = EOK;

	if (TLITE_MAX_ENTRIES < max)
	{
		ret = EINVAL;
	}
	else
	{

		if ((phy == PFE_PHY_IF_ID_HIF0)
					|| (phy == PFE_PHY_IF_ID_HIF1)
					|| (phy == PFE_PHY_IF_ID_HIF2)
					|| (phy == PFE_PHY_IF_ID_HIF3))
		{
			queue_temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
			if (PFE_TMU_INVALID_QUEUE == queue_temp)
			{
				ret = EINVAL;
			}
		}

		if (EOK == ret)
		{
			/*	curQ_Qmax[8:0], curQ_Qmin[8:0], curQ_cfg[1:0] are @ position 4 per queue */
			reg = ((uint32_t)max << (uint32_t)11U) | ((uint32_t)0U << (uint32_t)2U) | ((uint32_t)0x1U << 0U);
			ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue_temp) + 4U, reg);
		}
	}

	return ret;
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
errno_t pfe_tmu_q_mode_set_wred(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint16_t min, uint16_t max)
{
	errno_t ret = EOK;
	uint8_t queue_temp = queue;

	if ((max > 0x1ffU) || (min > 0x1ffU))
	{
		NXP_LOG_ERROR("Queue WRED 'min' and/or 'max' argument out of range\n");
		ret = EINVAL;
	}
	else
	{
		if ((phy == PFE_PHY_IF_ID_HIF0)
					|| (phy == PFE_PHY_IF_ID_HIF1)
					|| (phy == PFE_PHY_IF_ID_HIF2)
					|| (phy == PFE_PHY_IF_ID_HIF3))
		{
			queue_temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
			if (PFE_TMU_INVALID_QUEUE == queue_temp)
			{
				ret = EINVAL;
			}
		}

		if (EOK == ret)
		{
			ret = pfe_tmu_context_memory(cbus_base_va, phy, queue_temp, min, max);
		}
	}
	return ret;
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
errno_t pfe_tmu_q_set_wred_probability(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t prob)
{
	errno_t ret = EOK;
	uint32_t reg;
	uint8_t pos;
	uint8_t queue_temp = queue;

	if ((prob > 100U) || (zone > 7U))
	{
		ret = EINVAL;
	}
	else
	{
		if ((phy == PFE_PHY_IF_ID_HIF0)
					|| (phy == PFE_PHY_IF_ID_HIF1)
					|| (phy == PFE_PHY_IF_ID_HIF2)
					|| (phy == PFE_PHY_IF_ID_HIF3))
		{
			queue_temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
			if (PFE_TMU_INVALID_QUEUE == queue_temp)
			{
				ret = EINVAL;
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
		if (EOK == ret)
		{
			pos = 5U + (zone / 6U);

			ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue_temp) + pos, &reg);
			if (EOK == ret)
			{
				reg &= ~(0x1fUL << (5U * (zone % 6U)));
				reg |= (((0x1fU * (uint32_t)prob) / 100U) & 0x1fU) << (5U * (zone % 6U));

				ret = pfe_tmu_cntx_mem_write(cbus_base_va, phy, (8U * queue_temp) + pos, reg);
			}
		}
	}

	return ret;
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
errno_t pfe_tmu_q_get_wred_probability(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t *prob)
{
	errno_t ret = EOK;
	uint32_t reg;
	uint8_t pos;
	uint8_t queue_temp = queue;

	if (zone > 7U)
	{
		ret = EINVAL;
	}
	else
	{
		if ((phy == PFE_PHY_IF_ID_HIF0)
					|| (phy == PFE_PHY_IF_ID_HIF1)
					|| (phy == PFE_PHY_IF_ID_HIF2)
					|| (phy == PFE_PHY_IF_ID_HIF3))
		{
			queue_temp = pfe_tmu_hif_q_to_tmu_q(cbus_base_va, phy, queue);
			if (PFE_TMU_INVALID_QUEUE == queue_temp)
			{
				ret = EINVAL;
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
		if (EOK == ret)
		{
			pos = 5U + (zone / 6U);

			ret = pfe_tmu_cntx_mem_read(cbus_base_va, phy, (8U * queue_temp) + pos, &reg);
			if (EOK == ret)
			{
				reg = reg >> (5U * (zone % 6U));
				*prob = (uint8_t)(((reg & 0x1fU) * 100U) / 0x1fU);
			}
		}
	}

	return ret;
}

/**
 * @brief		Get number of WRED probability zones between 'min' and 'max' threshold
 * @param[in]	cbus_base_va The cbus base address
 * @param[in]	phy The physical interface
 * @param[in]	queue The queue ID
 * @return		Number of zones
 */
uint8_t pfe_tmu_q_get_wred_zones(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue)
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
errno_t pfe_tmu_shp_cfg_set_limits(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy,
		uint8_t shp, int32_t max_credit, int32_t min_credit)
{
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);
	errno_t ret;

	if ((max_credit > 0x3fffff) || (max_credit < 0))
	{
		NXP_LOG_ERROR("Max credit value exceeded\n");
		ret = EINVAL;
	}
	else if ((min_credit < -0x3fffff) || (min_credit > 0))
	{
		NXP_LOG_ERROR("Min credit value exceeded\n");
		ret = EINVAL;
	}
	else
	{
		hal_write32((uint32_t)max_credit << 10, shp_base_va + TMU_SHP_MAX_CREDIT);
		hal_write32(-min_credit, shp_base_va + TMU_SHP_MIN_CREDIT);
		ret = EOK;
	}

	return ret;
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
errno_t pfe_tmu_shp_cfg_get_limits(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy,
		uint8_t shp, int32_t *max_credit, int32_t *min_credit)
{
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);

	*max_credit = (int32_t)hal_read32(shp_base_va + TMU_SHP_MAX_CREDIT) >> 10;
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
errno_t pfe_tmu_shp_cfg_set_position(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp, uint8_t pos)
{
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);
	uint32_t reg;
	errno_t ret;

	if ((pos > 16U) && (pos != PFE_TMU_INVALID_POSITION))
	{
		NXP_LOG_ERROR("Invalid shaper position: %d\n", pos);
		ret = EINVAL;
	}
	else
	{
		reg = hal_read32(shp_base_va + TMU_SHP_CTRL2);
		reg &= ~(0x1fU << 1);
		reg |= (((uint32_t)pos & (uint32_t)0x1fU) << 1);
		hal_write32(reg, shp_base_va + TMU_SHP_CTRL2);
		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Get shaper position
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 * @return		Shaper position
 */
uint8_t pfe_tmu_shp_cfg_get_position(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);
	return (uint8_t)((hal_read32(shp_base_va + TMU_SHP_CTRL2) >> 1U) & 0x1fU);
}

/**
 * @brief		Enable shaper
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	shp Shaper instance/index
 */
errno_t pfe_tmu_shp_cfg_enable(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	uint32_t reg;
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);

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
errno_t pfe_tmu_shp_cfg_set_rate_mode(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t shp, pfe_tmu_rate_mode_t mode)
{
	uint32_t reg;
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);
	errno_t ret = EOK;

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
		ret = EINVAL;
	}
	if (EOK == ret)
	{
		hal_write32(reg, shp_base_va + TMU_SHP_CTRL2);
	}
	return ret;
}

/**
 * @brief		Get shaper rate mode
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @return		Shaper rate mode
 */
pfe_tmu_rate_mode_t pfe_tmu_shp_cfg_get_rate_mode(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	uint32_t reg;
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);
	pfe_tmu_rate_mode_t rate_mode;

	reg = hal_read32(shp_base_va + TMU_SHP_CTRL);
	if (0U == (reg & 0x1U))
	{
		/*	Shaper is disabled */
		rate_mode = RATE_MODE_INVALID;
	}
	else
	{
		reg = hal_read32(shp_base_va + TMU_SHP_CTRL2);
		if (0U != (reg & 0x1U))
		{
			rate_mode = RATE_MODE_PACKET_RATE;
		}
		else
		{
			rate_mode = RATE_MODE_DATA_RATE;
		}
	}
	return rate_mode;
}

/**
 * @brief		Set shaper idle slope
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @param[in]	isl Idle slope in units per second as given by chosen mode
 *					(bits-per-second, packets-per-second)
 */
errno_t pfe_tmu_shp_cfg_set_idle_slope(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t shp, uint32_t isl)
{
	uint32_t reg;
	errno_t ret = EOK;
	uint64_t wgt;
	uint64_t sys_clk_hz;
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);

	reg = hal_read32(cbus_base_va + CBUS_GLOBAL_CSR_BASE_ADDR + WSP_CLK_FRQ);
	sys_clk_hz = (reg & 0xffffULL) * 1000000ULL;
	NXP_LOG_INFO("Using PFE sys_clk value %"PRINT64"uHz\n", sys_clk_hz);

	/*	Set weight (added to credit counter with each sys_clk_hz/clk_div tick) */
	/*	The '+1' in (isl + 1ULL) is needed for mitigating integer division inaccuracy */
	switch (pfe_tmu_shp_cfg_get_rate_mode(cbus_base_va, phy, shp))
	{
		case RATE_MODE_DATA_RATE:
		{
			/*	ISL is bps, WGT is [bytes-per-tick] */
			wgt = ((uint64_t)(isl + 1ULL) * CLK_DIV * (1ULL << 12)) / (8ULL * sys_clk_hz);
			break;
		}

		case RATE_MODE_PACKET_RATE:
		{
			/*	ISL is pps, WGT is [packets-per-tick] */
			wgt = ((uint64_t)(isl + 1ULL) * CLK_DIV * (1ULL << 12)) / (sys_clk_hz);
			break;
		}

		default:
		{
			ret = EINVAL;
			break;
		}
	}

	if(ret == EOK)
	{
		if (wgt > 0xfffffU)
		{
			NXP_LOG_WARNING("Shaper weight exceeds max value\n");
		}

		hal_write32(wgt & 0xfffffU, shp_base_va + TMU_SHP_WGHT);
		NXP_LOG_INFO("Shaper weight set to %u.%u\n",
			(uint_t)((wgt >> 12) & 0xffU), (uint_t)(wgt & 0xfffU));

		/*	Set clk_div */
		reg = hal_read32(shp_base_va + TMU_SHP_CTRL);
		reg &= 0x1U;
		hal_write32(reg | (CLK_DIV_LOG2 << 1), shp_base_va + TMU_SHP_CTRL);
		NXP_LOG_INFO("Shaper tick is %"PRINT64"uHz\n", sys_clk_hz / CLK_DIV);
	}

	return ret;
}

/**
 * @brief		Get shaper idle slope
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @parma[in]	phy Physical interface ID
 * @param[in]	shp The shaper ID
 * @return		Current idle slope value
 */
uint32_t pfe_tmu_shp_cfg_get_idle_slope(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	uint64_t sys_clk_hz;
	uint32_t wgt, reg;
	uint64_t isl;
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);

	reg = hal_read32(cbus_base_va + CBUS_GLOBAL_CSR_BASE_ADDR + WSP_CLK_FRQ);
	sys_clk_hz = (reg & 0xffffULL) * 1000000ULL;
	wgt = hal_read32(shp_base_va + TMU_SHP_WGHT) & 0xfffffU;

	switch (pfe_tmu_shp_cfg_get_rate_mode(cbus_base_va, phy, shp))
	{
		case RATE_MODE_DATA_RATE:
		{
			isl = ((uint64_t)wgt * 8ULL * sys_clk_hz) / (CLK_DIV * (1ULL << 12));
		}
		break;

		case RATE_MODE_PACKET_RATE:
		{
			isl = ((uint64_t)wgt * sys_clk_hz) / (CLK_DIV * (1ULL << 12));
		}
		break;

		default:
		{
			isl = 0ULL;
			break;
		}
	}

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
void pfe_tmu_shp_cfg_disable(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);

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
void pfe_tmu_shp_cfg_init(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp)
{
	addr_t shp_base_va = cbus_base_va + TLITE_PHYn_SHPm_BASE_ADDR((uint32_t)phy, shp);

	/*	Disable */
	pfe_tmu_shp_cfg_disable(cbus_base_va, phy, shp);

	/*	Set invalid position */
	hal_write32(((uint32_t)TLITE_SHP_INVALID_POS << 1), shp_base_va + TMU_SHP_CTRL2);

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
void pfe_tmu_sch_cfg_init(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);

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
errno_t pfe_tmu_sch_cfg_set_rate_mode(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy,
		uint8_t sch, pfe_tmu_rate_mode_t mode)
{
	uint32_t reg;
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);
	errno_t ret = EOK;

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
		ret = EINVAL;
	}

	if (EOK == ret)
	{
		hal_write32(reg, sch_base_va + TMU_SCH_BIT_RATE);
	}

	return ret;
}

/**
 * @brief		Get rate mode
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @return		Current rate mode or RATE_MODE_INVALID in case of error
 */
pfe_tmu_rate_mode_t pfe_tmu_sch_cfg_get_rate_mode(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	uint32_t reg;
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);
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
errno_t pfe_tmu_sch_cfg_set_algo(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch, pfe_tmu_sched_algo_t algo)
{
	uint32_t reg;
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);
	errno_t ret = EOK;

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
			ret = EINVAL;
		}
		else
		{
			reg = 4U;
		}
	}
	else
	{
		ret = EINVAL;
	}

	if (EOK == ret)
	{
		hal_write32(reg, sch_base_va + TMU_SCH_CTRL);
	}

	return ret;
}

/**
 * @brief		Get scheduler algorithm
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	algo The algorithm to be used
 * @return		EOK if success, error code otherwise
 */
pfe_tmu_sched_algo_t pfe_tmu_sch_cfg_get_algo(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch)
{
	uint32_t reg;
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);
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
errno_t pfe_tmu_sch_cfg_set_input_weight(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint32_t weight)
{
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);
	errno_t ret;

	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		ret = EINVAL;
	}
	else
	{
		hal_write32(weight, sch_base_va + TMU_SCH_Qn_WGHT(input));
		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Get input weight
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	input Scheduler input
 * @return		The programmed weight value to be used by chosen scheduling algorithm
 */
uint32_t pfe_tmu_sch_cfg_get_input_weight(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);
	uint32_t input_weight;

	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		input_weight = 0U;
	}
	else
	{
		input_weight = hal_read32(sch_base_va + TMU_SCH_Qn_WGHT(input));
	}

	return input_weight;
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
errno_t pfe_tmu_sch_cfg_bind_queue(addr_t cbus_base_va,
		pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint8_t queue)
{
	uint32_t reg;
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);
	errno_t ret;

	if ((queue >= TLITE_PHY_QUEUES_CNT) && (queue != TLITE_SCH_INVALID_INPUT))
	{
		NXP_LOG_ERROR("Invalid queue\n");
		ret = EINVAL;
	}
	else if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		ret = EINVAL;
	}
	else
	{
		/*	Update appropriate "ALLOC_Q" register */
		reg = hal_read32(sch_base_va + TMU_SCH_Q_ALLOCn(input / 4U));
		reg &= ~(0xffUL << (8U * (input % 4U)));
		reg |= (((uint32_t)queue & 0x1fUL) << (8U * (input % 4U)));
		hal_write32(reg, sch_base_va + TMU_SCH_Q_ALLOCn(input / 4U));
		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Get queue connected to given scheduler input
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	input Scheduler input to be queried
 * @return		Queue ID connected to the input or PFE_TMU_INVALID_QUEUE if not present
 */
uint8_t pfe_tmu_sch_cfg_get_bound_queue(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	uint32_t reg;
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, sch);
	uint8_t queue;

	if (input >= TLITE_SCH_INPUTS_CNT)
	{
		NXP_LOG_ERROR("Scheduler input (%d) out of range\n", input);
		queue = PFE_TMU_INVALID_QUEUE;
	}
	else
	{
		reg = hal_read32(sch_base_va + TMU_SCH_Q_ALLOCn(input / 4U));
		queue = (uint8_t)(reg >> (8U * (input % 4U))) & 0xffU;

		if (TLITE_PHY_QUEUES_CNT <= queue)
		{
			queue = PFE_TMU_INVALID_QUEUE;
		}
	}

	return queue;
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
errno_t pfe_tmu_sch_cfg_bind_sched_output(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t src_sch, uint8_t dst_sch, uint8_t input)
{
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, src_sch);
	errno_t ret;

	/*	Scheduler0 -> Scheduler1 is the only possible option */
	if ((src_sch != 0U) || (dst_sch != 1U))
	{
		NXP_LOG_ERROR("Scheduler 0 output can only be connected to Scheduler 1 input\n");
		ret = EINVAL;
	}
	else
	{

		/*	Invalidate the original Scheduler1 input */
		if (EOK != pfe_tmu_sch_cfg_bind_queue(cbus_base_va, phy, dst_sch, input, PFE_TMU_INVALID_QUEUE))
		{
			ret = EINVAL;
		}
		else
		{
			/*	Connect Scheduler0 to given Scheduler1 input */
			hal_write32((uint32_t)input & (uint32_t)0xfU, sch_base_va + TMU_SCH_POS);
			ret = EOK;
		}
	}
	return ret;
}

/**
 * @brief		Get scheduler which output is connected to given scheduler input
 * @param[in]	cbus_base_va The cbus base address (VA)
 * @param[in]	phy The physical interface
 * @param[in]	sch Scheduler instance/index
 * @param[in]	input Scheduler input to be queried
 * @return		ID of the connected scheduler or PFE_TMU_INVALID_SCHEDULER
 */
uint8_t pfe_tmu_sch_cfg_get_bound_sched_output(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input)
{
	addr_t sch_base_va = cbus_base_va + TLITE_PHYn_SCHEDm_BASE_ADDR((uint32_t)phy, 0U);
	uint32_t reg;
	uint8_t sched_id;

	/*	Scheduler0 -> Scheduler1 is the only possible option */
	if (sch != 1U)
	{
		sched_id = PFE_TMU_INVALID_SCHEDULER;
	}

	reg = hal_read32(sch_base_va + TMU_SCH_POS) & 0xffU;

	if (input == reg)
	{
		sched_id = 0U;
	}
	else
	{
		sched_id = PFE_TMU_INVALID_SCHEDULER;
	}

	return sched_id;
}

/**
 * @brief		Get TMU statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the TMU block.
 * @param[in]	base_va 	Base address of TMU register space (virtual)
 * @param[in]	seq 		Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 *
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_tmu_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level)
{
	uint32_t reg, ii;
	uint8_t prob, queue, zone;
	uint32_t level, drops, tx;

	/* Debug registers */
	if(verb_level >= 10U)
	{
		seq_printf(seq, "TMU_PHY_INQ_PKTPTR  : 0x%x\n", hal_read32(base_va + TMU_PHY_INQ_PKTPTR));
		seq_printf(seq, "TMU_PHY_INQ_PKTINFO : 0x%x\n", hal_read32(base_va + TMU_PHY_INQ_PKTINFO));
		seq_printf(seq, "TMU_PHY_INQ_STAT    : 0x%x\n", hal_read32(base_va + TMU_PHY_INQ_STAT));
		seq_printf(seq, "TMU_DBG_BUS_TOP     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_TOP));
		seq_printf(seq, "TMU_DBG_BUS_PP0     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP0));
		seq_printf(seq, "TMU_DBG_BUS_PP1     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP1));
		seq_printf(seq, "TMU_DBG_BUS_PP2     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP2));
		seq_printf(seq, "TMU_DBG_BUS_PP3     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP3));
		seq_printf(seq, "TMU_DBG_BUS_PP4     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP4));
		seq_printf(seq, "TMU_DBG_BUS_PP5     : 0x%x\n", hal_read32(base_va + TMU_DBG_BUS_PP5));
	}

	if(verb_level >= 9U)
	{
		/*	Get version */
		reg = hal_read32(base_va + TMU_VERSION);
		seq_printf(seq, "Revision             : 0x%x\n", (reg >> 24) & 0xffU);
		seq_printf(seq, "Version              : 0x%x\n", (reg >> 16) & 0xffU);
		seq_printf(seq, "ID                   : 0x%x\n", reg & 0xffffU);
	}

	reg = hal_read32(base_va + TMU_CTRL);
	seq_printf(seq, "TMU_CTRL             : 0x%x\n", reg);
	reg = hal_read32(base_va + TMU_PHY_INQ_STAT);
	seq_printf(seq, "TMU_PHY_INQ_STAT     : 0x%x\n", reg);
	reg = hal_read32(base_va + TMU_PHY_INQ_PKTPTR);
	seq_printf(seq, "TMU_PHY_INQ_PKTPTR   : 0x%x\n", reg);
	reg = hal_read32(base_va + TMU_PHY_INQ_PKTINFO);
	seq_printf(seq, "TMU_PHY_INQ_PKTINFO  : 0x%x\n", reg);

	/*	Print per-queue statistics */
	for (ii=0U; ii < TLITE_PHYS_CNT; ii++)
	{
		seq_printf(seq, "[PHY: %d]\n", (int_t)ii);
		for (queue=0U; queue<TLITE_PHY_QUEUES_CNT; queue++)
		{
			/*	Fill level */
			level = (EOK != pfe_tmu_q_cfg_get_fill_level(base_va, phy_if_id_temp[ii], queue, &reg)) ? 0xffffffffU : reg;

			/*	Number of dropped packets */
			drops = (EOK != pfe_tmu_q_cfg_get_drop_count(base_va, phy_if_id_temp[ii], queue, &reg)) ? 0xffffffffU : reg;

			/*	Transmitted packets */
			tx = (EOK != pfe_tmu_q_cfg_get_tx_count(base_va, phy_if_id_temp[ii], queue, &reg)) ? 0xffffffffU : reg;

			if ((0U == level) && (0U == drops) && (0U == tx))
			{
				/*	Don't print emtpy queues */
				continue;
			}

			seq_printf(seq, "  [QUEUE: %d]\n", queue);

			/*	curQ_cfg is @ position 4 per queue */
			if (EOK != pfe_tmu_cntx_mem_read(base_va, phy_if_id_temp[ii], (8U * queue) + 4U, &reg))
			{
				NXP_LOG_ERROR("    Context memory read failed\n");
				continue;
			}

			/*	Configuration */
			switch (reg & 0x3U)
			{
				case 0x0U:
				{
					seq_printf(seq, "    Mode       : Default\n");
					break;
				}

				case 0x1U:
				{
					seq_printf(seq, "    Mode       : Tail drop (max: %d)\n", (reg >> 11) & 0x1ffU);
					break;
				}

				case 0x2U:
				{
					seq_printf(seq, "    Mode       : WRED (max: %d, min: %d)\n", (reg >> 11) & 0x1ffU, (reg >> 2) & 0x1ffU);
					for (zone=0U; zone<pfe_tmu_q_get_wred_zones(base_va, phy_if_id_temp[ii], queue); zone++)
					{
						if (EOK != pfe_tmu_q_get_wred_probability(base_va, phy_if_id_temp[ii], queue, zone, &prob))
						{
							seq_printf(seq, "      Zone %d   : ERROR\n", zone);
						}
						else
						{
							seq_printf(seq, "      Zone %d   : %d\n", zone, prob);
						}
					}

					break;
				}

				default:
				{
					seq_printf(seq, "    Mode       : ERROR\n");
					break;
				}
			}

			seq_printf(seq, "    Fill level : % 8d Drops: % 8d, TX: % 8d\n", level, drops, tx);
		}
	}


	return 0;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/** @}*/

