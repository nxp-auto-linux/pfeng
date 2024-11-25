/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef TMU_CSR_H_
#define TMU_CSR_H_

#include "pfe_tmu.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#define TMU_VERSION					(CBUS_TMU_CSR_BASE_ADDR + 0x000U)
#define TMU_INQ_WATERMARK			(CBUS_TMU_CSR_BASE_ADDR + 0x004U)
#define TMU_PHY_INQ_PKTPTR			(CBUS_TMU_CSR_BASE_ADDR + 0x008U)
#define TMU_PHY_INQ_PKTINFO			(CBUS_TMU_CSR_BASE_ADDR + 0x00cU)
#define TMU_PHY_INQ_STAT			(CBUS_TMU_CSR_BASE_ADDR + 0x010U)
#define TMU_PHY_QUEUE_SEL			(CBUS_TMU_CSR_BASE_ADDR + 0x014U)
#define TMU_CURQ_PTR				(CBUS_TMU_CSR_BASE_ADDR + 0x018U)
#define TMU_CURQ_PKT_CNT			(CBUS_TMU_CSR_BASE_ADDR + 0x01cU)
#define TMU_CURQ_DROP_CNT			(CBUS_TMU_CSR_BASE_ADDR + 0x020U)
#define TMU_CURQ_TRANS_CNT			(CBUS_TMU_CSR_BASE_ADDR + 0x024U)
#define TMU_CURQ_QSTAT				(CBUS_TMU_CSR_BASE_ADDR + 0x028U)
#define TMU_HW_PROB_CFG_TBL0		(CBUS_TMU_CSR_BASE_ADDR + 0x02cU)
#define TMU_HW_PROB_CFG_TBL1		(CBUS_TMU_CSR_BASE_ADDR + 0x030U)
#define TMU_CURQ_DEBUG				(CBUS_TMU_CSR_BASE_ADDR + 0x034U)
#define TMU_CTRL					(CBUS_TMU_CSR_BASE_ADDR + 0x038U)
#define TMU_BMU_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x03cU)
#define TMU_AFULL_THRES				(CBUS_TMU_CSR_BASE_ADDR + 0x040U)
#define TMU_BMU_BUF_SIZE			(CBUS_TMU_CSR_BASE_ADDR + 0x044U)
#define TMU_MAX_BUF_CNT				(CBUS_TMU_CSR_BASE_ADDR + 0x048U)
#define TMU_TEQ_CTRL				(CBUS_TMU_CSR_BASE_ADDR + 0x04cU)
#define TMU_BMU2_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x050U)
#define TMU_DDR_DATA_OFFSET			(CBUS_TMU_CSR_BASE_ADDR + 0x054U)
#define TMU_LMEM_BUF_SIZE			(CBUS_TMU_CSR_BASE_ADDR + 0x058U)
#define TMU_LMEM_DATA_OFFSET		(CBUS_TMU_CSR_BASE_ADDR + 0x05cU)
#define TMU_LMEM_BASE_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x060U)

#define TMU_PHY0_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x064U)
#define TMU_PHY1_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x068U)
#define TMU_PHY2_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x06cU)
#define TMU_PHY3_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x070U)
#define TMU_PHY4_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x074U)
#define TMU_PHY5_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x078U)
#define TMU_PHY6_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x07cU)
#define TMU_PHY7_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x080U)
#define TMU_PHY8_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x084U)
#define TMU_PHY9_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x088U)
#define TMU_PHY10_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x08cU)
#define TMU_PHY11_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x090U)
#define TMU_PHY12_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x094U)
#define TMU_PHY13_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x098U)
#define TMU_PHY14_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x09cU)
#define TMU_PHY15_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x0a0U)
#define TMU_PHY16_INQ_ADDR			(CBUS_TMU_CSR_BASE_ADDR + 0x0a4U)
#define TMU_PHYn_INQ_ADDR(n)		(TMU_PHY0_INQ_ADDR + ((n) * 4U)

#define TMU_PHY0_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0acU)
#define TMU_PHY1_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0b0U)
#define TMU_PHY2_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0b4U)
#define TMU_PHY3_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0b8U)
#define TMU_PHY4_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0bcU)
#define TMU_PHY5_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0c0U)
#define TMU_PHY6_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0c4U)
#define TMU_PHY7_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0c8U)
#define TMU_PHY8_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0ccU)
#define TMU_PHY9_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0d0U)
#define TMU_PHY10_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0d4U)
#define TMU_PHY11_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0d8U)
#define TMU_PHY12_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0dcU)
#define TMU_PHY13_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0e0U)
#define TMU_PHY14_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0e4U)
#define TMU_PHY15_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0e8U)
#define TMU_PHY16_TDQ_IIFG_CFG		(CBUS_TMU_CSR_BASE_ADDR + 0x0ecU)

#define TMU_PHY0_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x0f0U)
#define TMU_PHY1_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x0f4U)
#define TMU_PHY2_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x0f8U)
#define TMU_PHY3_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x0fcU)
#define TMU_PHY4_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x100U)
#define TMU_PHY5_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x104U)
#define TMU_PHY6_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x108U)
#define TMU_PHY7_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x10cU)
#define TMU_PHY8_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x110U)
#define TMU_PHY9_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x114U)
#define TMU_PHY10_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x118U)
#define TMU_PHY11_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x11cU)
#define TMU_PHY12_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x120U)
#define TMU_PHY13_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x124U)
#define TMU_PHY14_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x128U)
#define TMU_PHY15_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x12cU)
#define TMU_PHY16_TDQ_CTRL			(CBUS_TMU_CSR_BASE_ADDR + 0x130U)

#define TMU_CNTX_ACCESS_CTRL		(CBUS_TMU_CSR_BASE_ADDR + 0x134U)
#define TMU_CNTX_ADDR				(CBUS_TMU_CSR_BASE_ADDR + 0x138U)
#define TMU_CNTX_DATA				(CBUS_TMU_CSR_BASE_ADDR + 0x13cU)
#define TMU_CNTX_CMD				(CBUS_TMU_CSR_BASE_ADDR + 0x140U)

#define TMU_DBG_BUS_TOP				(CBUS_TMU_CSR_BASE_ADDR + 0x144U)
#define TMU_DBG_BUS_PP0				(CBUS_TMU_CSR_BASE_ADDR + 0x148U)
#define TMU_DBG_BUS_PP1				(CBUS_TMU_CSR_BASE_ADDR + 0x14cU)
#define TMU_DBG_BUS_PP2				(CBUS_TMU_CSR_BASE_ADDR + 0x150U)
#define TMU_DBG_BUS_PP3				(CBUS_TMU_CSR_BASE_ADDR + 0x154U)
#define TMU_DBG_BUS_PP4				(CBUS_TMU_CSR_BASE_ADDR + 0x158U)
#define TMU_DBG_BUS_PP5				(CBUS_TMU_CSR_BASE_ADDR + 0x15cU)
#define TMU_DBG_BUS_PP6				(CBUS_TMU_CSR_BASE_ADDR + 0x160U)
#define TMU_DBG_BUS_PP7				(CBUS_TMU_CSR_BASE_ADDR + 0x164U)
#define TMU_DBG_BUS_PP8				(CBUS_TMU_CSR_BASE_ADDR + 0x168U)
#define TMU_DBG_BUS_PP9				(CBUS_TMU_CSR_BASE_ADDR + 0x16cU)
#define TMU_DBG_BUS_PP10			(CBUS_TMU_CSR_BASE_ADDR + 0x170U)
#define TMU_DBG_BUS_PP11			(CBUS_TMU_CSR_BASE_ADDR + 0x174U)
#define TMU_DBG_BUS_PP12			(CBUS_TMU_CSR_BASE_ADDR + 0x178U)
#define TMU_DBG_BUS_PP13			(CBUS_TMU_CSR_BASE_ADDR + 0x17cU)
#define TMU_DBG_BUS_PP14			(CBUS_TMU_CSR_BASE_ADDR + 0x180U)
#define TMU_DBG_BUS_PP15			(CBUS_TMU_CSR_BASE_ADDR + 0x184U)
#define TMU_DBG_BUS_PP16			(CBUS_TMU_CSR_BASE_ADDR + 0x188U)

#define TMU_METER_ADDR				(CBUS_TMU_CSR_BASE_ADDR + 0x190U)
#define TMU_METER_CFG0				(CBUS_TMU_CSR_BASE_ADDR + 0x194U)
#define TMU_METER_CFG1				(CBUS_TMU_CSR_BASE_ADDR + 0x198U)
#define TMU_METER_CMD				(CBUS_TMU_CSR_BASE_ADDR + 0x19cU)

#define TLITE_TDQ_PHY0_CSR_BASE_ADDR		(CBUS_TMU_CSR_BASE_ADDR + 0x1000U)
#define TLITE_TDQ_PHY1_CSR_BASE_ADDR		(CBUS_TMU_CSR_BASE_ADDR + 0x2000U)
#define TLITE_TDQ_PHY2_CSR_BASE_ADDR		(CBUS_TMU_CSR_BASE_ADDR + 0x3000U)
#define TLITE_TDQ_PHY3_CSR_BASE_ADDR		(CBUS_TMU_CSR_BASE_ADDR + 0x4000U)
#define TLITE_TDQ_PHY4_CSR_BASE_ADDR		(CBUS_TMU_CSR_BASE_ADDR + 0x5000U)
#define TLITE_TDQ_PHYn_CSR_BASE_ADDR(n)		(TLITE_TDQ_PHY0_CSR_BASE_ADDR + ((n) * 0x1000U))

#define TLITE_SCHED0_BASE_OFFSET			0x000U
#define TLITE_SCHED1_BASE_OFFSET			0x100U
#define TLITE_SCHED_OFFSET_MASK				0xfffU

#define TLITE_PHY0_SCHED0_BASE_ADDR			(TLITE_TDQ_PHY0_CSR_BASE_ADDR + TLITE_SCHED0_BASE_OFFSET)
#define TLITE_PHY0_SCHED1_BASE_ADDR			(TLITE_TDQ_PHY0_CSR_BASE_ADDR + TLITE_SCHED1_BASE_OFFSET)
#define TLITE_PHY0_SHP0_BASE_ADDR			(TLITE_TDQ_PHY0_CSR_BASE_ADDR + 0x200U)
#define TLITE_PHY0_SHP1_BASE_ADDR			(TLITE_TDQ_PHY0_CSR_BASE_ADDR + 0x300U)
#define TLITE_PHY0_SHP2_BASE_ADDR			(TLITE_TDQ_PHY0_CSR_BASE_ADDR + 0x400U)
#define TLITE_PHY0_SHP3_BASE_ADDR			(TLITE_TDQ_PHY0_CSR_BASE_ADDR + 0x500U)

#define TLITE_PHYn_SCHED0_BASE_ADDR(n)		(TLITE_TDQ_PHYn_CSR_BASE_ADDR(n) + 0x000U)
#define TLITE_PHYn_SCHED1_BASE_ADDR(n)		(TLITE_TDQ_PHYn_CSR_BASE_ADDR(n) + 0x100U)
#define TLITE_PHYn_SHP0_BASE_ADDR(n)		(TLITE_TDQ_PHYn_CSR_BASE_ADDR(n) + 0x200U)
#define TLITE_PHYn_SHP1_BASE_ADDR(n)		(TLITE_TDQ_PHYn_CSR_BASE_ADDR(n) + 0x300U)
#define TLITE_PHYn_SHP2_BASE_ADDR(n)		(TLITE_TDQ_PHYn_CSR_BASE_ADDR(n) + 0x400U)
#define TLITE_PHYn_SHP3_BASE_ADDR(n)		(TLITE_TDQ_PHYn_CSR_BASE_ADDR(n) + 0x500U)

#define TLITE_PHYn_SCHEDm_BASE_ADDR(n, m)	(TLITE_PHYn_SCHED0_BASE_ADDR(n) + ((m) * 0x100U))
#define TLITE_PHYn_SHPm_BASE_ADDR(n, m)		(TLITE_PHYn_SHP0_BASE_ADDR(n) + ((m) * 0x100U))

#define TLITE_PHYn_SCHm_CTRL(n, m)			(TLITE_PHYn_SCHEDm_BASE_ADDR(n, m) + TMU_SCH_CTRL)
#define TLITE_PHYn_SCHm_Ql_WGHT(n, m, l)	(TLITE_PHYn_SCHEDm_BASE_ADDR(n, m) + TMU_SCH_Qn_WGHT(l))
#define TLITE_PHYn_SCHm_Q_ALLOCl(n, m, l)	(TLITE_PHYn_SCHEDm_BASE_ADDR(n, m) + TMU_SCH_Q_ALLOCn(l))
#define TLITE_PHYn_SCHm_BIT_RATE(n, m)		(TLITE_PHYn_SCHEDm_BASE_ADDR(n, m) + TMU_SCH_BIT_RATE)
#define TLITE_PHYn_SCHm_POS(n, m)			(TLITE_PHYn_SCHEDm_BASE_ADDR(n, m) + TMU_SCH_POS)

#define TLITE_PHYn_SHPm_CTRL(n, m)			(TLITE_PHYn_SHPm_BASE_ADDR(n, m) + TMU_SHP_CTRL)
#define TLITE_PHYn_SHPm_WGHT(n, m)			(TLITE_PHYn_SHPm_BASE_ADDR(n, m) + TMU_SHP_WGHT)
#define TLITE_PHYn_SHPm_MAX_CREDIT(n, m)	(TLITE_PHYn_SHPm_BASE_ADDR(n, m) + TMU_SHP_MAX_CREDIT)
#define TLITE_PHYn_SHPm_CTRL2(n, m)			(TLITE_PHYn_SHPm_BASE_ADDR(n, m) + TMU_SHP_CTRL2)
#define TLITE_PHYn_SHPm_MIN_CREDIT(n, m)	(TLITE_PHYn_SHPm_BASE_ADDR(n, m) + TMU_SHP_MIN_CREDIT)
#define TLITE_PHYn_SHPm_STATUS(n, m)		(TLITE_PHYn_SHPm_BASE_ADDR(n, m) + TMU_SHP_STATUS)

#define TMU_SCH_CTRL			(0x00U)
#define TMU_SCH_Q0_WGHT			(0x20U)
#define TMU_SCH_Q1_WGHT			(0x24U)
#define TMU_SCH_Q2_WGHT			(0x28U)
#define TMU_SCH_Q3_WGHT			(0x2cU)
#define TMU_SCH_Q4_WGHT			(0x30U)
#define TMU_SCH_Q5_WGHT			(0x34U)
#define TMU_SCH_Q6_WGHT			(0x38U)
#define TMU_SCH_Q7_WGHT			(0x3cU)
#define TMU_SCH_Qn_WGHT(n)		(TMU_SCH_Q0_WGHT + ((n) * 4U))
#define TMU_SCH_Q_ALLOC0		(0x40U)
#define TMU_SCH_Q_ALLOC1		(0x44U)
#define TMU_SCH_Q_ALLOCn(n)		(TMU_SCH_Q_ALLOC0 + ((n) * 4U))
#define TMU_SCH_BIT_RATE		(0x48U)
#define TMU_SCH_POS				(0x54U)
#define TMU_SHP_CTRL			(0x00U)
#define TMU_SHP_WGHT			(0x04U)
#define TMU_SHP_MAX_CREDIT		(0x08U)
#define TMU_SHP_CTRL2			(0x0cU)
#define TMU_SHP_MIN_CREDIT		(0x10U)
#define TMU_SHP_STATUS			(0x14U)
#define TLITE_PHYS_CNT			6U
#define TLITE_PHY_QUEUES_CNT	8U
#define TLITE_SCH_INPUTS_CNT	8U
#define TLITE_SHP_INVALID_POS	0x1fU
#define TLITE_SCH_INVALID_INPUT 0xffU

#define TLITE_INQ_FIFODEPTH		32U

/* Max number of buffers in ALL queues for one phy is 255, queues are 8 */

#define TLITE_MAX_ENTRIES		255U
#define TLITE_MAX_Q_SIZE		((uint16_t)TLITE_MAX_ENTRIES / 8U)
#define TLITE_HIF_MAX_Q_SIZE	16U /* Agreed default hardcoded value for ERR051211 workaround */
#define TLITE_HIF_MAX_ENTRIES	(2U * TLITE_HIF_MAX_Q_SIZE)

#define TLITE_OPT_Q0_SIZE		150U /* optimal size for the default queue (q0) */
#define TLITE_OPT_Q1_7_SIZE		((uint16_t)((uint16_t)TLITE_MAX_ENTRIES - TLITE_OPT_Q0_SIZE) / 7U)

/*	Implementation of the pfe_tmu_phy_cfg_t */
struct pfe_tmu_phy_cfg_tag
{
	pfe_ct_phy_if_id_t id;
	uint8_t q_cnt;
	uint8_t sch_cnt;
	uint8_t shp_cnt;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

const pfe_tmu_phy_cfg_t* pfe_tmu_cfg_get_phy_config(pfe_ct_phy_if_id_t phy);

errno_t pfe_tmu_q_cfg_get_fill_level(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *level);
errno_t pfe_tmu_q_cfg_get_drop_count(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt);
errno_t pfe_tmu_q_cfg_get_tx_count(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt);
pfe_tmu_queue_mode_t pfe_tmu_q_get_mode(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *min, uint32_t *max);
errno_t pfe_tmu_q_mode_set_default(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue);
errno_t pfe_tmu_q_mode_set_tail_drop(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint16_t max);
errno_t pfe_tmu_q_mode_set_wred(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint16_t min, uint16_t max);
errno_t pfe_tmu_q_set_wred_probability(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t prob);
errno_t pfe_tmu_q_get_wred_probability(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t *prob);
uint8_t pfe_tmu_q_get_wred_zones(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue);
errno_t pfe_tmu_q_reset_tail_drop_policy(addr_t cbus_base_va);

void pfe_tmu_shp_cfg_init(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_cfg_enable(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_cfg_set_rate_mode(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp, pfe_tmu_rate_mode_t mode);
pfe_tmu_rate_mode_t pfe_tmu_shp_cfg_get_rate_mode(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_cfg_set_idle_slope(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp, uint32_t isl);
uint32_t pfe_tmu_shp_cfg_get_idle_slope(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp);
errno_t pfe_tmu_shp_cfg_set_limits(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp, int32_t max_credit, int32_t min_credit);
errno_t pfe_tmu_shp_cfg_get_limits(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp, int32_t *max_credit, int32_t *min_credit);
errno_t pfe_tmu_shp_cfg_set_position(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp, uint8_t pos);
uint8_t pfe_tmu_shp_cfg_get_position(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp);
void pfe_tmu_shp_cfg_disable(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t shp);

void pfe_tmu_sch_cfg_init(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch);
errno_t pfe_tmu_sch_cfg_set_rate_mode(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, pfe_tmu_rate_mode_t mode);
pfe_tmu_rate_mode_t pfe_tmu_sch_cfg_get_rate_mode(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch);
errno_t pfe_tmu_sch_cfg_set_algo(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, pfe_tmu_sched_algo_t algo);
pfe_tmu_sched_algo_t pfe_tmu_sch_cfg_get_algo(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch);
errno_t pfe_tmu_sch_cfg_set_input_weight(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint32_t weight);
uint32_t pfe_tmu_sch_cfg_get_input_weight(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);
errno_t pfe_tmu_sch_cfg_bind_sched_output(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t src_sch, uint8_t dst_sch, uint8_t input);
uint8_t pfe_tmu_sch_cfg_get_bound_sched_output(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);
errno_t pfe_tmu_sch_cfg_bind_queue(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input, uint8_t queue);
uint8_t pfe_tmu_sch_cfg_get_bound_queue(addr_t cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t sch, uint8_t input);

errno_t pfe_tmu_cfg_init(addr_t cbus_base_va, const pfe_tmu_cfg_t *cfg);
void pfe_tmu_reclaim_init(addr_t cbus_base_va);
void pfe_tmu_cfg_reset(addr_t cbus_base_va);
void pfe_tmu_cfg_enable(addr_t cbus_base_va);
void pfe_tmu_cfg_disable(addr_t cbus_base_va);

uint32_t pfe_tmu_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* TMU_CSR_H_ */
