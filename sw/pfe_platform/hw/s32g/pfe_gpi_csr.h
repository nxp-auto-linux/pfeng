/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
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

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

#define GPI_VERSION							0x000U
#define GPI_CTRL							0x004U
#define GPI_RX_CONFIG						0x008U
#define GPI_HDR_SIZE						0x00cU
#define GPI_BUF_SIZE						0x010U
#define GPI_LMEM_ALLOC_ADDR					0x014U
#define GPI_LMEM_FREE_ADDR					0x018U
#define GPI_DDR_ALLOC_ADDR					0x01cU
#define GPI_DDR_FREE_ADDR					0x020U
#define GPI_CLASS_ADDR						0x024U
#define GPI_DRX_FIFO						0x028U
#define GPI_TRX_FIFO						0x02cU
#define GPI_INQ_PKTPTR						0x030U
#define GPI_DDR_DATA_OFFSET					0x034U
#define GPI_LMEM_DATA_OFFSET				0x038U
#define GPI_TMLF_TX							0x04cU
#define GPI_DTX_ASEQ						0x050U
#define GPI_FIFO_STATUS						0x054U
#define GPI_FIFO_DEBUG						0x058U
#define GPI_TX_PAUSE_TIME					0x05cU
#define GPI_LMEM_SEC_BUF_DATA_OFFSET		0x060U
#define GPI_DDR_SEC_BUF_DATA_OFFSET			0x064U
#define GPI_CSR_TOE_CHKSUM_EN				0x068U
#define GPI_OVERRUN_DROPCNT					0x06cU
#define GPI_TX_DBUG_REG1					0x070U
#define GPI_TX_DBUG_REG2					0x074U
#define GPI_TX_DBUG_REG3					0x078U
#define GPI_TX_DBUG_REG4					0x07cU
#define GPI_TX_DBUG_REG5					0x080U
#define GPI_TX_DBUG_REG6					0x084U
#define GPI_RX_DBUG_REG1					0x090U
#define GPI_RX_DBUG_REG2					0x094U

#define GPI_PORT_SHP0_CTRL					0x098U
#define GPI_PORT_SHP0_WGHT					0x09cU
#define GPI_PORT_SHP0_STATUS				0x100U

#define GPI_BMU1_PHY_LOW_WATERMARK			0x104U
#define GPI_BMU1_PHY_HIGH_WATERMARK			0x108U
#define GPI_BMU2_PHY_LOW_WATERMARK			0x10cU
#define GPI_BMU2_PHY_HIGH_WATERMARK			0x110U

#define GPI_FW_CONTROL						0x114U
#define GPI_USE_CLASS_INQ_AFULL				0x118U

#define GPI_PORT_SHP1_CTRL					0x11cU
#define GPI_PORT_SHP1_WGHT					0x120U
#define GPI_PORT_SHP1_STATUS				0x124U
#define GPI_PORT_SHP_CONFIG					0x128U
#define GPI_CSR_SHP_DROPCNT					0x12cU

#define GPI_FW_CONTROL1						0x130U
#define GPI_RXF_FIFO_LOW_WATERMARK			0x134U
#define GPI_RXF_FIFO_HIGH_WATERMARK			0x138U

#define GPI_EMAC_1588_TIMESTAMP_EN			0x13cU

#define GPI_PORT_SHP0_MIN_CREDIT			0x140U
#define GPI_PORT_SHP1_MIN_CREDIT			0x144U
#define GPI_LMEM2_FREE_ADDR					0x148U
#define GPI_CSR_AXI_WRITE_DONE_ADDR			0x14cU

#define CSR_IQGOS_DMEMQ_ZONE_PROB			0x150U
#define CSR_IGQOS_DMEMQFULL_THRESH			0x154U
#define CSR_IGQOS_DMEMQDROP_THRESH			0x158U
#define CSR_IGQOS_LMEMQ_ZONE_PROB			0x15cU
#define CSR_IGQOS_LMEMQFULL_THRESH			0x160U
#define CSR_IGQOS_LMEMQDROP_THRESH			0x164U
#define CSR_IGQOS_RXFQ_ZONE_PROB			0x168U
#define CSR_IGQOS_RXFQFULL_THRESH			0x16cU
#define CSR_IGQOS_RXFQDROP_THRESH			0x170U
#define CSR_IGQOS_CONTROL					0x174U
#define CSR_IGQOS_CLASS						0x178U
#define CSR_IGQOS_ENTRY_CMDSTATUS			0x180U
#define CSR_IGQOS_ENTRY_CMDCNTRL			0x184U
#define CSR_IGQOS_ENTRY_DATA_REG0			0x188U
#define CSR_IGQOS_ENTRY_DATA_REG1			0x18cU
#define CSR_IGQOS_ENTRY_DATA_REG2			0x190U
#define CSR_IGQOS_ENTRY_DATA_REG3			0x194U
#define CSR_IGQOS_ENTRY_DATA_REG4			0x198U
#define CSR_IGQOS_ENTRY_DATA_REG5			0x19cU
#define CSR_IGQOS_ENTRY_DATA_REG6			0x1a0U
#define CSR_IGQOS_ENTRY_DATA_REG7			0x1a4U
#define CSR_IGQOS_QUEUE_STATUS				0x1a8U
#define CSR_IGQOS_STAT_CLASS_DROP_CNT		0x1acU
#define CSR_IGQOS_STAT_LMEM_QUEUE_DROP_CNT	0x1b0U
#define CSR_IGQOS_STAT_DMEM_QUEUE_DROP_CNT	0x1b4U
#define CSR_IGQOS_STAT_RXF_QUEUE_DROP_CNT	0x1b8U
#define CSR_IGQOS_STAT_SHAPER0_DROP_CNT		0x1bcU
#define CSR_IGQOS_STAT_SHAPER1_DROP_CNT		0x1c0U
#define CSR_IGQOS_STAT_MANAGED_PACKET_CNT	0x1c4U
#define CSR_IGQOS_STAT_UNMANAGED_PACKET_CNT	0x1c8U
#define CSR_IGQOS_STAT_RESERVED_PACKET_CNT	0x1ccU
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
#define CSR_IGQOS_PORT_SHP_CONFIG			0x1f8U
#define CSR_IGQOS_CSR_SHP_DROPCNT			0x1fcU
#define CSR_IGQOS_PORT_SHP0_MIN_CREDIT		0x200U
#define CSR_IGQOS_PORT_SHP1_MIN_CREDIT		0x204U
#define CSR_IGQOS_LRU_TIMER_VALUE			0x208U
#define CSR_IGQOS_LRU_ENTRY					0x20cU
#define CSR_IGQOS_SMEM_OFFSET				0x210U
#define CSR_IGQOS_LMEM_OFFSET				0x214U
#define CSR_IGQOS_TPID						0x218U
#define CSR_IGQOS_DEBUG						0x21cU
#define CSR_IGQOS_DEBUG1					0x220U
#define CSR_IGQOS_DEBUG2					0x224U
#define CSR_IGQOS_DEBUG3					0x228U
#define CSR_IGQOS_DEBUG4					0x22cU
#define CSR_IGQOS_DEBUG5					0x230U
#define CSR_IGQOS_DEBUG6					0x234U
#define CSR_IGQOS_DEBUG7					0x238U
#define CSR_IGQOS_STAT_TOTAL_DROP_CNT		0x23cU
#define CSR_IGQOS_LRU_TIMER					0x240U
#define CSR_IGQOS_LRU_TIMER_LOAD_VALUE		0x244U
#define IGQOS_ENTRY_TABLE_LEN				128U
#define IGQOS_LRU_TABLE_LEN					128U
#define CMDCNTRL_CMD_WRITE					0x1U
#define CMDCNTRL_CMD_READ					0x2U
#define CMDCNTRL_CMD_TAB_ADDR(x)			((((uint32_t)x) & 0x7fU) << 8U)
#define CMDCNTRL_CMD_TAB_SELECT_LRU			(1UL << 16U)
#define CMDCNTRL_CMD_TAB_SELECT_ENTRY		(0UL << 16U)

typedef struct {
	uint32_t lmem_rtry_cnt;
	uint32_t tmlf_txthres;
	uint32_t aseq_len;
} GPI_CFG;

#define GPI_LMEM_BUF_EN					0x1U
#define GPI_DDR_BUF_EN					0x2U
#define HGPI_LMEM_RTRY_CNT				0x40U
#define HGPI_TMLF_TXTHRES				0xBCU
#define HGPI_ASEQ_LEN					0x40U

errno_t pfe_gpi_cfg_init(addr_t cbus_va, addr_t base_va, const pfe_gpi_cfg_t *cfg);
errno_t pfe_gpi_cfg_reset(addr_t base_va);
void pfe_gpi_cfg_enable(addr_t base_va);
void pfe_gpi_cfg_disable(addr_t base_va);
uint32_t pfe_gpi_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level);

#endif /* PFE_GPI_CSR_H_ */
