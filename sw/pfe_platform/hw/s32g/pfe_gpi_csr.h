/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PFE_GPI_CSR_H_
#define PFE_GPI_CSR_H_

#include "pfe_gpi.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#define GPI_VERSION					0x000U
#define GPI_CTRL					0x004U
#define GPI_RX_CONFIG					0x008U
#define GPI_HDR_SIZE					0x00cU
#define GPI_BUF_SIZE					0x010U
#define GPI_LMEM_ALLOC_ADDR				0x014U
#define GPI_LMEM_FREE_ADDR				0x018U
#define GPI_DDR_ALLOC_ADDR				0x01cU
#define GPI_DDR_FREE_ADDR				0x020U
#define GPI_CLASS_ADDR					0x024U
#define GPI_DRX_FIFO					0x028U
#define GPI_TRX_FIFO					0x02cU
#define GPI_INQ_PKTPTR					0x030U
#define GPI_DDR_DATA_OFFSET				0x034U
#define GPI_LMEM_DATA_OFFSET				0x038U
#define GPI_TMLF_TX					0x04cU
#define GPI_DTX_ASEQ					0x050U
#define GPI_FIFO_STATUS					0x054U
#define GPI_FIFO_DEBUG					0x058U
#define GPI_TX_PAUSE_TIME				0x05cU
#define GPI_LMEM_SEC_BUF_DATA_OFFSET			0x060U
#define GPI_DDR_SEC_BUF_DATA_OFFSET			0x064U
#define GPI_CSR_TOE_CHKSUM_EN				0x068U
#define GPI_OVERRUN_DROPCNT				0x06cU
#define GPI_TX_DBUG_REG1				0x070U
#define GPI_TX_DBUG_REG2				0x074U
#define GPI_TX_DBUG_REG3				0x078U
#define GPI_TX_DBUG_REG4				0x07cU
#define GPI_TX_DBUG_REG5				0x080U
#define GPI_TX_DBUG_REG6				0x084U
#define GPI_RX_DBUG_REG1				0x090U
#define GPI_RX_DBUG_REG2				0x094U

#define GPI_PORT_SHP0_CTRL				0x098U
#define GPI_PORT_SHP0_WGHT				0x09cU
#define GPI_PORT_SHP0_STATUS				0x100U

#define GPI_BMU1_PHY_LOW_WATERMARK			0x104U
#define GPI_BMU1_PHY_HIGH_WATERMARK			0x108U
#define GPI_BMU2_PHY_LOW_WATERMARK			0x10cU
#define GPI_BMU2_PHY_HIGH_WATERMARK			0x110U

#define GPI_FW_CONTROL					0x114U
#define GPI_USE_CLASS_INQ_AFULL				0x118U

#define GPI_PORT_SHP1_CTRL				0x11cU
#define GPI_PORT_SHP1_WGHT				0x120U
#define GPI_PORT_SHP1_STATUS				0x124U
#define GPI_PORT_SHP_CONFIG				0x128U
#define GPI_CSR_SHP_DROPCNT				0x12cU

#define GPI_FW_CONTROL1					0x130U
#define GPI_RXF_FIFO_LOW_WATERMARK			0x134U
#define GPI_RXF_FIFO_HIGH_WATERMARK			0x138U

#define GPI_EMAC_1588_TIMESTAMP_EN			0x13cU

#define GPI_PORT_SHP0_MIN_CREDIT			0x140U
#define GPI_PORT_SHP1_MIN_CREDIT			0x144U
#define GPI_PORT_SHP_MIN_CREDIT(i)			((addr_t)0x140U + ((i) * (addr_t)4U))

#define GPI_LMEM2_FREE_ADDR				0x148U
#define GPI_CSR_AXI_WRITE_DONE_ADDR			0x14cU

#define CSR_IQGOS_DMEMQ_ZONE_PROB			0x150U
#define CSR_IGQOS_DMEMQ_FULL_THRESH			0x154U
#define CSR_IGQOS_DMEMQ_DROP_THRESH			0x158U
#define CSR_IGQOS_LMEMQ_ZONE_PROB			0x15cU
#define CSR_IGQOS_LMEMQ_FULL_THRESH			0x160U
#define CSR_IGQOS_LMEMQ_DROP_THRESH			0x164U
#define CSR_IGQOS_RXFQ_ZONE_PROB			0x168U
#define CSR_IGQOS_RXFQ_FULL_THRESH			0x16cU
#define CSR_IGQOS_RXFQ_DROP_THRESH			0x170U
#define CSR_IQGOS_ZONE_PROB(q)				((addr_t)0x150U + ((q) * (addr_t)0xcU))
#define CSR_IQGOS_FULL_THRESH(q)			((addr_t)0x154U + ((q) * (addr_t)0xcU))
#define CSR_IQGOS_DROP_THRESH(q)			((addr_t)0x158U + ((q) * (addr_t)0xcU))

#define CSR_IGQOS_CONTROL				0x174U
#define CSR_IGQOS_CLASS					0x178U
#define CSR_IGQOS_QOS					0x17cU
#define CSR_IGQOS_ENTRY_CMDSTATUS			0x180U
#define CSR_IGQOS_ENTRY_CMDCNTRL			0x184U
#define CSR_IGQOS_ENTRY_DATA_REG(i)			((addr_t)0x188U + ((i) * (addr_t)4U))
#define CSR_IGQOS_QUEUE_STATUS				0x1a8U
#define CSR_IGQOS_STAT_CLASS_DROP_CNT			0x1acU
#define CSR_IGQOS_STAT_LMEM_QUEUE_DROP_CNT		0x1b0U
#define CSR_IGQOS_STAT_DMEM_QUEUE_DROP_CNT		0x1b4U
#define CSR_IGQOS_STAT_RXF_QUEUE_DROP_CNT		0x1b8U
#define CSR_IGQOS_STAT_SHAPER0_DROP_CNT			0x1bcU
#define CSR_IGQOS_STAT_SHAPER1_DROP_CNT			0x1c0U
#define CSR_IGQOS_STAT_SHAPER_DROP_CNT(i)		((addr_t)CSR_IGQOS_STAT_SHAPER0_DROP_CNT + ((i) * (addr_t)4U))
#define CSR_IGQOS_STAT_MANAGED_PACKET_CNT		0x1c4U
#define CSR_IGQOS_STAT_UNMANAGED_PACKET_CNT		0x1c8U
#define CSR_IGQOS_STAT_RESERVED_PACKET_CNT		0x1ccU
#define CSR_IGQOS_STAT_GEN_CNT1				0x1d0U
#define CSR_IGQOS_STAT_GEN_CNT2				0x1d4U
#define CSR_IGQOS_STAT_GEN_CNT3				0x1d8U
#define CSR_IGQOS_STAT_GEN_CNT4				0x1dcU

#define CSR_IGQOS_PORT_SHP0_CTRL			0x1e0U
#define CSR_IGQOS_PORT_SHP0_WGHT			0x1e4U
#define CSR_IGQOS_PORT_SHP0_STATUS			0x1e8U
#define CSR_IGQOS_PORT_SHP1_CTRL			0x1ecU
#define CSR_IGQOS_PORT_SHP1_WGHT			0x1f0U
#define CSR_IGQOS_PORT_SHP1_STATUS			0x1f4U
#define CSR_IGQOS_PORT_SHP_CTRL(i)			((addr_t)0x1e0U + ((i) * (addr_t)0xcU))
#define CSR_IGQOS_PORT_SHP_WGHT(i)			((addr_t)0x1e4U + ((i) * (addr_t)0xcU))
#define CSR_IGQOS_PORT_SHP_STATUS(i)		(0x1e8U + ((i) * 0xcU))

#define CSR_IGQOS_PORT_SHP_CONFIG			0x1f8U
#define CSR_IGQOS_CSR_SHP_DROPCNT			0x1fcU

#define CSR_IGQOS_PORT_SHP0_MIN_CREDIT			0x200U
#define CSR_IGQOS_PORT_SHP1_MIN_CREDIT			0x204U
#define CSR_IGQOS_PORT_SHP_MIN_CREDIT(i)		((addr_t)0x200U + ((i) * (addr_t)0x4U))

#define CSR_IGQOS_LRU_TIMER_VALUE			0x208U
#define CSR_IGQOS_LRU_ENTRY				0x20cU
#define CSR_IGQOS_SMEM_OFFSET				0x210U
#define CSR_IGQOS_LMEM_OFFSET				0x214U
#define CSR_IGQOS_TPID					0x218U
#define CSR_IGQOS_DEBUG					0x21cU
#define CSR_IGQOS_DEBUG1				0x220U
#define CSR_IGQOS_DEBUG2				0x224U
#define CSR_IGQOS_DEBUG3				0x228U
#define CSR_IGQOS_DEBUG4				0x22cU
#define CSR_IGQOS_DEBUG5				0x230U
#define CSR_IGQOS_DEBUG6				0x234U
#define CSR_IGQOS_DEBUG7				0x238U
#define CSR_IGQOS_STAT_TOTAL_DROP_CNT			0x23cU
#define CSR_IGQOS_LRU_TIMER				0x240U
#define CSR_IGQOS_LRU_TIMER_LOAD_VALUE			0x244U

/* reg values */
#define mask32(width)					((uint32_t)(((uint32_t)1U << (width)) - 1U))
#define IGQOS_CONTROL_QOS_EN				BIT(0)
#define IGQOS_TPID_DOT1Q				0x8100U

#define IGQOS_CLASS_TPID0_EN				BIT(4)
#define IGQOS_CLASS_TPID1_EN				BIT(5)

#define IGQOS_QOS_WRED_LMEMQ_EN				BIT(0)
#define IGQOS_QOS_WRED_DMEMQ_EN				BIT(1)
#define IGQOS_QOS_WRED_RXFQ_EN				BIT(2)
#define IGQOS_WRED_EN					(IGQOS_QOS_WRED_LMEMQ_EN | IGQOS_QOS_WRED_DMEMQ_EN | \
							IGQOS_QOS_WRED_RXFQ_EN)

#ifndef BIT
	#define BIT(x)  (1UL << (x))
#endif
#define IGQOS_PORT_SHP_FRACW_WIDTH			8
#define IGQOS_PORT_SHP_INTW_WIDTH			3
#define IGQOS_PORT_SHP_WEIGHT_MASK			mask32(IGQOS_PORT_SHP_FRACW_WIDTH + IGQOS_PORT_SHP_INTW_WIDTH)
#define IGQOS_PORT_SHP_MODE_PPS(i)			BIT(i)
#define IGQOS_PORT_SHP_TYPE_POS(i)			(((i) + 1U) * 2U)
#define IGQOS_PORT_SHP_TYPE_MASK			0x3U
#define IGQOS_PORT_SHP_CLKDIV_POS			1
#define IGQOS_PORT_SHP_CLKDIV_MASK			0xfU
#define IGQOS_PORT_SHP_MAX_CREDIT_POS			8U
#define IGQOS_PORT_SHP_CREDIT_MASK			0x3fffffU
#define IGQOS_PORT_SHP_CREDIT_MAX			0x3fffff

#define ENTRY_TABLE_SIZE				64U
#define ENTRY_DATA_REG_CNT				8U

#define CMDCNTRL_CMD_WRITE				0x1U
#define CMDCNTRL_CMD_READ				0x2U
#define CMDCNTRL_CMD_TAB_ADDR(x)			((((uint32_t)(x)) & 0x7fU) << 8)
#define CMDCNTRL_CMD_TAB_SELECT_LRU			BIT(16)

#define GPI_LMEM_BUF_EN					0x1U
#define GPI_DDR_BUF_EN					0x2U
#define HGPI_LMEM_RTRY_CNT				0x40U
#define HGPI_TMLF_TXTHRES				0xBCU
#define HGPI_ASEQ_LEN					0x40U

/* Class table entry formatting.
 * Each entry was divided into 8 32-bit registers.
 * The bitfield ranges were extracted directly from the h/w ref man.
 */
#define GPI_QOS_FLOW_REG_OFF(table_offset)	((table_offset) % 32)
#define GPI_QOS_FLOW_ARG_WIDTH(off1, off2)	((off2) - (off1))
/* data entry reg 0 */
#define GPI_QOS_FLOW_TYPE_OFF		GPI_QOS_FLOW_REG_OFF(0)
#define GPI_QOS_FLOW_TYPE_WIDTH		GPI_QOS_FLOW_ARG_WIDTH(0, 10)
#define GPI_QOS_FLOW_VLAN_ID_OFF	GPI_QOS_FLOW_REG_OFF(10)
#define GPI_QOS_FLOW_VLAN_ID_WIDTH 	GPI_QOS_FLOW_ARG_WIDTH(10, 22)
#define GPI_QOS_FLOW_TOS_OFF		GPI_QOS_FLOW_REG_OFF(22)
#define GPI_QOS_FLOW_TOS_WIDTH		GPI_QOS_FLOW_ARG_WIDTH(22, 30)
#define GPI_QOS_FLOW_PROT_OFF		GPI_QOS_FLOW_REG_OFF(30)
#define GPI_QOS_FLOW_PROT_WIDTH		GPI_QOS_FLOW_ARG_WIDTH(30, 32)
/* data entry reg 1 */
#define GPI_QOS_FLOW_PROT_UP_OFF	GPI_QOS_FLOW_REG_OFF(32)
#define GPI_QOS_FLOW_PROT_UP_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(30, 38)
#define GPI_QOS_FLOW_SIP_OFF		GPI_QOS_FLOW_REG_OFF(38)
#define GPI_QOS_FLOW_SIP_WIDTH		GPI_QOS_FLOW_ARG_WIDTH(38, 64)
/* data entry reg 2 */
#define GPI_QOS_FLOW_SIP_UP_OFF		GPI_QOS_FLOW_REG_OFF(64)
#define GPI_QOS_FLOW_SIP_UP_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(64, 70)
#define GPI_QOS_FLOW_DIP_OFF		GPI_QOS_FLOW_REG_OFF(70)
#define GPI_QOS_FLOW_DIP_WIDTH		GPI_QOS_FLOW_ARG_WIDTH(70, 96)
/* data entry reg 3 */
#define GPI_QOS_FLOW_DIP_UP_OFF		GPI_QOS_FLOW_REG_OFF(96)
#define GPI_QOS_FLOW_DIP_UP_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(96, 102)
#define GPI_QOS_FLOW_SPORT_MIN_OFF	GPI_QOS_FLOW_REG_OFF(102)
#define GPI_QOS_FLOW_SPORT_MIN_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(102, 118)
#define GPI_QOS_FLOW_SPORT_MAX_OFF	GPI_QOS_FLOW_REG_OFF(118)
#define GPI_QOS_FLOW_SPORT_MAX_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(118, 128)
/* data entry reg 4 */
#define GPI_QOS_FLOW_SPORT_MAX_UP_OFF	GPI_QOS_FLOW_REG_OFF(128)
#define GPI_QOS_FLOW_SPORT_MAX_UP_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(128, 134)
#define GPI_QOS_FLOW_DPORT_MIN_OFF	GPI_QOS_FLOW_REG_OFF(134)
#define GPI_QOS_FLOW_DPORT_MIN_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(134, 150)
#define GPI_QOS_FLOW_DPORT_MAX_OFF	GPI_QOS_FLOW_REG_OFF(150)
#define GPI_QOS_FLOW_DPORT_MAX_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(150, 160)
/* data entry reg 5 */
#define GPI_QOS_FLOW_DPORT_MAX_UP_OFF	GPI_QOS_FLOW_REG_OFF(160)
#define GPI_QOS_FLOW_DPORT_MAX_UP_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(160, 166)
#define GPI_QOS_FLOW_VALID_ENTRY_OFF	GPI_QOS_FLOW_REG_OFF(166)
#define GPI_QOS_FLOW_VALID_ENTRY_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(166, 167)
#define GPI_QOS_FLOW_TYPE_M_OFF		GPI_QOS_FLOW_REG_OFF(167)
#define GPI_QOS_FLOW_TYPE_M_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(167, 177)
#define GPI_QOS_FLOW_VLAN_ID_M_OFF	GPI_QOS_FLOW_REG_OFF(177)
#define GPI_QOS_FLOW_VLAN_ID_M_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(177, 189)
#define GPI_QOS_FLOW_TOS_M_OFF		GPI_QOS_FLOW_REG_OFF(189)
#define GPI_QOS_FLOW_TOS_M_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(189, 192)
/* data entry reg 6 */
#define GPI_QOS_FLOW_TOS_M_UP_OFF	GPI_QOS_FLOW_REG_OFF(192)
#define GPI_QOS_FLOW_TOS_M_UP_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(192, 197)
#define GPI_QOS_FLOW_PROT_M_OFF		GPI_QOS_FLOW_REG_OFF(197)
#define GPI_QOS_FLOW_PROT_M_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(197, 205)
#define GPI_QOS_FLOW_SIP_M_OFF		GPI_QOS_FLOW_REG_OFF(205)
#define GPI_QOS_FLOW_SIP_M_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(205, 211)
#define GPI_QOS_FLOW_DIP_M_OFF		GPI_QOS_FLOW_REG_OFF(211)
#define GPI_QOS_FLOW_DIP_M_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(211, 217)
#define GPI_QOS_FLOW_SPORT_M_OFF	GPI_QOS_FLOW_REG_OFF(217)
#define GPI_QOS_FLOW_SPORT_M_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(217, 218)
#define GPI_QOS_FLOW_DPORT_M_OFF	GPI_QOS_FLOW_REG_OFF(218)
#define GPI_QOS_FLOW_DPORT_M_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(218, 219)
#define GPI_QOS_FLOW_ACT_DROP_OFF	GPI_QOS_FLOW_REG_OFF(219)
#define GPI_QOS_FLOW_ACT_DROP_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(219, 220)
#define GPI_QOS_FLOW_ACT_RES_OFF	GPI_QOS_FLOW_REG_OFF(220)
#define GPI_QOS_FLOW_ACT_RES_WIDTH	GPI_QOS_FLOW_ARG_WIDTH(220, 221)

#define flow_arg_lower(name, arg)	(mask32(GPI_QOS_FLOW_##name##_WIDTH) & (arg))
#define flow_arg_upper(name, arg)	(mask32(GPI_QOS_FLOW_##name##_UP_WIDTH) & \
					 ((arg) >> GPI_QOS_FLOW_##name##_WIDTH))

#define entry_arg_set_lower(name, arg)	(flow_arg_lower(name, arg) << GPI_QOS_FLOW_##name##_OFF)
#define entry_arg_set_upper(name, arg)	(flow_arg_upper(name, arg) << GPI_QOS_FLOW_##name##_UP_OFF)
#define entry_arg_set(name, arg)	entry_arg_set_lower(name, arg)

#define entry_arg_get_lower(name, entry) (((entry) >> GPI_QOS_FLOW_##name##_OFF) & \
					  mask32(GPI_QOS_FLOW_##name##_WIDTH))
#define entry_arg_get_upper(name, entry) (((entry) & mask32(GPI_QOS_FLOW_##name##_UP_WIDTH)) << \
					  GPI_QOS_FLOW_##name##_WIDTH)
#define entry_arg_get(name, entry)	entry_arg_get_lower(name, entry)

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

void pfe_gpi_cfg_init(addr_t base_va, const pfe_gpi_cfg_t *cfg);
errno_t pfe_gpi_cfg_reset(addr_t base_va);
void pfe_gpi_cfg_enable(addr_t base_va);
void pfe_gpi_cfg_disable(addr_t base_va);
/* IGQOS global API */
void pfe_gpi_cfg_qos_default_init(addr_t base_va);
void pfe_gpi_cfg_qos_enable(addr_t base_va);
void pfe_gpi_cfg_qos_disable(addr_t base_va);
bool_t pfe_gpi_cfg_qos_is_enabled(addr_t base_va);
/* IGQOS Class API */
void pfe_gpi_cfg_qos_write_flow_entry_req(addr_t base_va, uint32_t addr, const uint32_t entry[]);
void pfe_gpi_cfg_qos_clear_flow_entry_req(addr_t base_va, uint32_t addr);
void pfe_gpi_cfg_qos_clear_lru_entry_req(addr_t base_va, uint32_t addr);
void pfe_gpi_cfg_qos_rd_fl_entry_req(addr_t base_va, uint32_t addr);
void pfe_gpi_cfg_qos_rd_fl_entry_resp(addr_t base_va, uint32_t entry[]);
bool_t pfe_gpi_cfg_qos_entry_ready(addr_t base_va);
/* IGQOS WRED API */
void pfe_gpi_cfg_wred_default_init(addr_t base_va);
void pfe_gpi_cfg_wred_enable(addr_t base_va, pfe_iqos_queue_t queue);
void pfe_gpi_cfg_wred_disable(addr_t base_va, pfe_iqos_queue_t queue);
bool_t pfe_gpi_cfg_wred_is_enabled(addr_t base_va, pfe_iqos_queue_t queue);
void pfe_gpi_cfg_wred_set_prob(addr_t base_va, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t val);
void pfe_gpi_cfg_wred_get_prob(addr_t base_va, pfe_iqos_queue_t queue, pfe_iqos_wred_zone_t zone, uint8_t *val);
void pfe_gpi_cfg_wred_set_thr(addr_t base_va, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t val);
void pfe_gpi_cfg_wred_get_thr(addr_t base_va, pfe_iqos_queue_t queue, pfe_iqos_wred_thr_t thr, uint16_t *val);
/* IGQOS Shaper API */
void pfe_gpi_cfg_shp_default_init(addr_t base_va, uint8_t id);
void pfe_gpi_cfg_shp_enable(addr_t base_va, uint8_t id);
void pfe_gpi_cfg_shp_disable(addr_t base_va, uint8_t id);
bool_t pfe_gpi_cfg_shp_is_enabled(addr_t base_va, uint8_t id);
void pfe_gpi_cfg_shp_set_type(addr_t base_va, uint8_t id, pfe_iqos_shp_type_t type);
void pfe_gpi_cfg_shp_get_type(addr_t base_va, uint8_t id, pfe_iqos_shp_type_t *type);
void pfe_gpi_cfg_shp_set_mode(addr_t base_va, uint8_t id, pfe_iqos_shp_rate_mode_t mode);
void pfe_gpi_cfg_shp_get_mode(addr_t base_va, uint8_t id, pfe_iqos_shp_rate_mode_t *mode);
void pfe_gpi_cfg_shp_set_isl_weight(addr_t base_va, uint8_t id, uint32_t clk_div_log2, uint32_t weight);
void pfe_gpi_cfg_shp_get_isl_weight(addr_t base_va, uint8_t id, uint32_t *weight);
void pfe_gpi_cfg_shp_set_limits(addr_t base_va, uint8_t id, uint32_t max_credit, uint32_t min_credit);
void pfe_gpi_cfg_shp_get_limits(addr_t base_va, uint8_t id, uint32_t *max_credit, uint32_t *min_credit);
uint32_t pfe_gpi_cfg_shp_get_drop_cnt(addr_t base_va, uint8_t id);

uint32_t pfe_gpi_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_GPI_CSR_H_ */
