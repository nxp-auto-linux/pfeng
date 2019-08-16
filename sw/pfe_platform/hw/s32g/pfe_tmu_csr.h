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
 * @addtogroup  dxgr_PFE_TMU
 * @{
 * 
 * @file		pfe_tmu_csr.h
 * @brief		The TMU LITE module registers definition file (s32g).
 * @details		
 *
 */

#ifndef _TMU_CSR_H_
#define _TMU_CSR_H_

#include "pfe_tmu.h"

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
#define TMU_SCH_SHP_RANGE			(CBUS_TMU_CSR_BASE_ADDR + 0x1a0U)

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

/*	TMU/TLITE Scheduler */
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

/*	TMU/TLITE Shaper */
#define TMU_SHP_CTRL			(0x00U)
#define TMU_SHP_WGHT			(0x04U)
#define TMU_SHP_MAX_CREDIT		(0x08U)
#define TMU_SHP_CTRL2			(0x0cU)
#define TMU_SHP_MIN_CREDIT		(0x10U)
#define TMU_SHP_STATUS			(0x14U)

/*	Properties */
#define TLITE_PHYS_CNT			5U	/*	Number of PHYs */
#define TLITE_PHY_QUEUES_CNT	8U	/*	Number of queues per PHY */
#define TLITE_SCH_INPUTS_CNT	8U	/*	Number of inputs per scheduler */

errno_t pfe_tmu_q_cfg_get_fill_level(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *level);
errno_t pfe_tmu_q_cfg_get_drop_count(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt);
errno_t pfe_tmu_q_cfg_get_tx_count(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint32_t *cnt);
errno_t pfe_tmu_q_mode_set_default(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue);
errno_t pfe_tmu_q_mode_set_tail_drop(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint16_t max);
errno_t pfe_tmu_q_mode_set_wred(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint16_t min, uint16_t max);
errno_t pfe_tmu_q_set_wred_probability(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t prob);
errno_t pfe_tmu_q_get_wred_probability(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, uint8_t zone, uint8_t *prob);
uint8_t pfe_tmu_q_get_wred_zones(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue);
uint8_t pfe_tmu_cfg_get_q_count(pfe_ct_phy_if_id_t phy);

void pfe_tmu_shp_cfg_init(void *shp_base_va);
errno_t pfe_tmu_shp_cfg_enable(void *cbus_base_va, void *shp_base_va, pfe_tmu_rate_mode_t mode, uint32_t isl);
errno_t pfe_tmu_shp_cfg_set_limits(void *shp_base_va, int32_t max_credit, int32_t min_credit);
void pfe_tmu_shp_cfg_disable(void *shp_base_va);

void pfe_tmu_sch_cfg_init(void *sch_base_va);
errno_t pfe_tmu_sch_cfg_set_rate_mode(void *sch_base_va, pfe_tmu_rate_mode_t mode);
errno_t pfe_tmu_sch_cfg_set_algo(void *sch_base_va, pfe_tmu_sched_algo_t algo);
errno_t pfe_tmu_sch_cfg_set_input_weight(void *sch_base_va, uint8_t input, uint32_t weight);
errno_t pfe_tmu_sch_cfg_bind_sch_output(void *sch_base_va, uint8_t input, void *sch_base_va_out, void *cbus_base);
errno_t pfe_tmu_sch_cfg_bind_queue(void *sch_base_va, uint8_t input, uint8_t queue);

errno_t pfe_tmu_cfg_init(void *cbus_base_va, pfe_tmu_cfg_t *cfg);
void pfe_tmu_cfg_reset(void *cbus_base_va);
void pfe_tmu_cfg_enable(void *cbus_base_va);
void pfe_tmu_cfg_disable(void *cbus_base_va);
void pfe_tmu_cfg_send_pkt(void *cbus_base_va, pfe_ct_phy_if_id_t phy, uint8_t queue, void *buf_pa, uint16_t len);
uint32_t pfe_tmu_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level);

#endif /* _TMU_CSR_H_ */

/** @}*/
