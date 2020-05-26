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

/**
 * @addtogroup  dxgr_PFE_EMAC
 * @{
 *
 * @file		s32g/pfe_emac_csr.h
 * @brief		The EMAC module registers definition file (s32g).
 * @details
 *
 */

#ifndef SRC_PFE_EMAC_CSR_H_
#define SRC_PFE_EMAC_CSR_H_

#define EMAC_CFG_TYPE_MTIP

#include "pfe_emac.h"

/**
 * @def		EMAC_PFE_VARIANT
 * @brief	Specify if the MAC is the PFE or full GMAC variant.
 */
#define EMAC_PFE_VARIANT


#define MAC_CONFIGURATION						0x0000U
#define MAC_EXT_CONFIGURATION					0x0004U
#define MAC_PACKET_FILTER						0x0008U
#define MAC_WATCHDOG_TIMEOUT					0x000cU
#define MAC_HASH_TABLE_REG0						0x0010U
#define MAC_HASH_TABLE_REG1						0x0014U
#if !defined(EMAC_PFE_VARIANT)
#define MAC_HASH_TABLE_REG2						0x0018U
#define MAC_HASH_TABLE_REG3						0x001cU
#define MAC_HASH_TABLE_REG4						0x0020U
#define MAC_HASH_TABLE_REG5						0x0024U
#define MAC_HASH_TABLE_REG6						0x0028U
#define MAC_HASH_TABLE_REG7						0x002cU
#endif /* EMAC_PFE_VARIANT */
#define MAC_HASH_TABLE_REG(n)					(MAC_HASH_TABLE_REG0 + ((n) * 4))
#define MAC_VLAN_TAG_CTRL						0x0050U
#define MAC_VLAN_TAG_DATA						0x0054U
#if !defined(EMAC_PFE_VARIANT)
#define MAC_VLAN_HASH_TABLE						0x0058U
#endif /* EMAC_PFE_VARIANT */
#define MAC_VLAN_INCL							0x0060U
#define MAC_INNER_VLAN_INCL						0x0064U
#define MAC_Q0_TX_FLOW_CTRL						0x0070U
#define MAC_Q1_TX_FLOW_CTRL						0x0074U
#define MAC_Q2_TX_FLOW_CTRL						0x0078U
#define MAC_Q3_TX_FLOW_CTRL						0x007cU
#define MAC_Q4_TX_FLOW_CTRL						0x0080U
#define MAC_RX_FLOW_CTRL						0x0090U
#define MAC_RXQ_CTRL4							0x0094U
#define MAC_TXQ_PRTY_MAP0						0x0098U
#define MAC_TXQ_PRTY_MAP1						0x009cU
#define MAC_RXQ_CTRL0							0x00a0U
#define MAC_RXQ_CTRL1							0x00a4U
#define MAC_RXQ_CTRL2							0x00a8U
#define MAC_RXQ_CTRL3							0x00acU
#define MAC_INTERRUPT_STATUS					0x00b0U
#define MAC_INTERRUPT_ENABLE					0x00b4U
#define MAC_RX_TX_STATUS						0x00b8U
#define MAC_PMT_CONTROL_STATUS					0x00c0U
#define MAC_RWK_PACKET_FILTER					0x00c4U
#define MAC_PHYIF_CONTROL_STATUS				0x00f8U
#define MAC_VERSION								0x0110U
#define MAC_DEBUG								0x0114U
#define MAC_HW_FEATURE0							0x011cU
#define MAC_HW_FEATURE1							0x0120U
#define MAC_HW_FEATURE2							0x0124U
#define MAC_HW_FEATURE3							0x0128U
#define MAC_DPP_FSM_INTERRUPT_STATUS			0x0140U
#define MAC_FSM_CONTROL							0x0148U
#define MAC_FSM_ACT_TIMER						0x014cU
#define MAC_SNPS_SCS_REG1						0x0150U
#define MAC_MDIO_ADDRESS						0x0200U
#define MAC_MDIO_DATA							0x0204U
#if !defined(EMAC_PFE_VARIANT)
#define MAC_ARP_ADDRESS							0x0210U
#endif /* EMAC_PFE_VARIANT */
#define MAC_CSR_SW_CTRL							0x0230U
#define MAC_FPE_CTRL_STS						0x0234U
#define MAC_EXT_CFG1							0x0238U
#define MAC_PRESN_TIME_NS						0x0240U
#define MAC_PRESN_TIME_UPDT						0x0244U
#define MAC_ADDRESS0_HIGH						0x0300U
#define MAC_ADDRESS0_LOW						0x0304U
#if defined(EMAC_PFE_VARIANT)
#define MAC_ADDRESS1_HIGH						0x0308U
#define MAC_ADDRESS1_LOW						0x030cU
#define MAC_ADDRESS2_HIGH						0x0310U
#define MAC_ADDRESS2_LOW						0x0314U
#define MAC_ADDRESS3_HIGH						0x0318U
#define MAC_ADDRESS3_LOW						0x031cU
#define MAC_ADDRESS4_HIGH						0x0320U
#define MAC_ADDRESS4_LOW						0x0324U
#define MAC_ADDRESS5_HIGH						0x0328U
#define MAC_ADDRESS5_LOW						0x032cU
#define MAC_ADDRESS6_HIGH						0x0330U
#define MAC_ADDRESS6_LOW						0x0334U
#define MAC_ADDRESS7_HIGH						0x0338U
#define MAC_ADDRESS7_LOW						0x033cU
#endif /* EMAC_PFE_VARIANT */
#define MAC_ADDRESS_HIGH(n)						(MAC_ADDRESS0_HIGH + ((n) * 8U))
#define MAC_ADDRESS_LOW(n)						(MAC_ADDRESS0_LOW + ((n) * 8U))

#define MMC_CONTROL								0x0700U
#define MMC_RX_INTERRUPT						0x0704U
#define MMC_TX_INTERRUPT						0x0708U
#define MMC_RX_INTERRUPT_MASK					0x070cU
#define MMC_TX_INTERRUPT_MASK					0x0710U

#define TX_OCTET_COUNT_GOOD_BAD					0x0714U
#define TX_PACKET_COUNT_GOOD_BAD				0x0718U
#define TX_BROADCAST_PACKETS_GOOD				0x071cU
#define TX_MULTICAST_PACKETS_GOOD				0x0720U
#define TX_64OCTETS_PACKETS_GOOD_BAD			0x0724U
#define TX_65TO127OCTETS_PACKETS_GOOD_BAD		0x0728U
#define TX_128TO255OCTETS_PACKETS_GOOD_BAD		0x072cU
#define TX_256TO511OCTETS_PACKETS_GOOD_BAD		0x0730U
#define TX_512TO1023OCTETS_PACKETS_GOOD_BAD		0x0734U
#define TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD		0x0738U
#define TX_UNICAST_PACKETS_GOOD_BAD				0x073cU
#define TX_MULTICAST_PACKETS_GOOD_BAD			0x0740U
#define TX_BROADCAST_PACKETS_GOOD_BAD			0x0744U
#define TX_UNDERFLOW_ERROR_PACKETS				0x0748U
#define TX_SINGLE_COLLISION_GOOD_PACKETS		0x074cU
#define TX_MULTIPLE_COLLISION_GOOD_PACKETS		0x0750U
#define TX_DEFERRED_PACKETS						0x0754U
#define TX_LATE_COLLISION_PACKETS				0x0758U
#define TX_EXCESSIVE_COLLISION_PACKETS			0x075cU
#define TX_CARRIER_ERROR_PACKETS				0x0760U
#define TX_OCTET_COUNT_GOOD						0x0764U
#define TX_PACKET_COUNT_GOOD					0x0768U
#define TX_EXCESSIVE_DEFERRAL_ERROR				0x076cU
#define TX_PAUSE_PACKETS						0x0770U
#define TX_VLAN_PACKETS_GOOD					0x0774U
#define TX_OSIZE_PACKETS_GOOD					0x0778U

#define RX_PACKETS_COUNT_GOOD_BAD				0x0780U
#define RX_OCTET_COUNT_GOOD_BAD					0x0784U
#define RX_OCTET_COUNT_GOOD						0x0788U
#define RX_BROADCAST_PACKETS_GOOD				0x078cU
#define RX_MULTICAST_PACKETS_GOOD				0x0790U
#define RX_CRC_ERROR_PACKETS					0x0794U
#define RX_ALIGNMENT_ERROR_PACKETS				0x0798U
#define RX_RUNT_ERROR_PACKETS					0x079cU
#define RX_JABBER_ERROR_PACKETS					0x07a0U
#define RX_UNDERSIZE_PACKETS_GOOD				0x07a4U
#define RX_OVERSIZE_PACKETS_GOOD				0x07a8U
#define RX_64OCTETS_PACKETS_GOOD_BAD			0x07acU
#define RX_65TO127OCTETS_PACKETS_GOOD_BAD		0x07b0U
#define RX_128TO255OCTETS_PACKETS_GOOD_BAD		0x07b4U
#define RX_256TO511OCTETS_PACKETS_GOOD_BAD		0x07b8U
#define RX_512TO1023OCTETS_PACKETS_GOOD_BAD		0x07bcU
#define RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD		0x07c0U
#define RX_UNICAST_PACKETS_GOOD					0x07c4U
#define RX_LENGTH_ERROR_PACKETS					0x07c8U
#define RX_OUT_OF_RANGE_TYPE_PACKETS			0x07ccU
#define RX_PAUSE_PACKETS						0x07d0U
#define RX_FIFO_OVERFLOW_PACKETS				0x07d4U
#define RX_VLAN_PACKETS_GOOD_BAD				0x07d8U
#define RX_WATCHDOG_ERROR_PACKETS				0x07dcU
#define RX_RECEIVE_ERROR_PACKETS				0x07e0U
#define RX_CONTROL_PACKETS_GOOD					0x07e4U

#define MMC_FPE_TX_INTERRUPT					0x08a0U
#define MMC_FPE_TX_INTERRUPT_MASK				0x08a4U
#define MMC_TX_FPE_FRAGMENT_CNTR				0x08a8U
#define MMC_TX_HOLD_REQ_CNTR					0x08acU
#define MMC_FPE_RX_INTERRUPT					0x08c0U
#define MMC_FPE_RX_INTERRUPT_MASK				0x08c4U
#define MMC_RX_PACKET_ASSEMBLY_ERR_CNTR			0x08c8U
#define MMC_RX_PACKET_SMD_ERR_CNTR				0x08ccU
#define MMC_RX_PACKET_ASSEMBLY_OK_CNTR			0x08d0U
#define MMC_RX_FPE_FRAGMENT_CNTR				0x08d4U

#if !defined(EMAC_PFE_VARIANT)
#define MAC_L3_L4_CONTROL0						0x0900U
#define MAC_LAYER4_ADDRESS0						0x0904U
#define MAC_LAYER3_ADDR0_REG0					0x0910U
#define MAC_LAYER3_ADDR1_REG0					0x0914U
#define MAC_LAYER3_ADDR2_REG0					0x0918U
#define MAC_LAYER3_ADDR3_REG0					0x091cU
#define MAC_L3_L4_CONTROL1						0x0930U
#define MAC_LAYER4_ADDRESS1						0x0934U
#define MAC_LAYER3_ADDR0_REG1					0x0940U
#define MAC_LAYER3_ADDR1_REG1					0x0944U
#define MAC_LAYER3_ADDR2_REG1					0x0948U
#define MAC_LAYER3_ADDR3_REG1					0x094cU
#define MAC_L3_L4_CONTROL2						0x0960U
#define MAC_LAYER4_ADDRESS2						0x0964U
#define MAC_LAYER3_ADDR0_REG2					0x0970U
#define MAC_LAYER3_ADDR1_REG2					0x0974U
#define MAC_LAYER3_ADDR2_REG2					0x0978U
#define MAC_LAYER3_ADDR3_REG2					0x097cU
#define MAC_L3_L4_CONTROL3						0x0990U
#define MAC_LAYER4_ADDRESS3						0x0994U
#define MAC_LAYER3_ADDR0_REG3					0x09a0U
#define MAC_LAYER3_ADDR1_REG3					0x09a4U
#define MAC_LAYER3_ADDR2_REG3					0x09a8U
#define MAC_LAYER3_ADDR3_REG3					0x09acU
#define MAC_L3_L4_CONTROL4						0x09c0U
#define MAC_LAYER4_ADDRESS4						0x09c4U
#define MAC_LAYER3_ADDR0_REG4					0x09d0U
#define MAC_LAYER3_ADDR1_REG4					0x09d4U
#define MAC_LAYER3_ADDR2_REG4					0x09d8U
#define MAC_LAYER3_ADDR3_REG4					0x09dcU
#define MAC_L3_L4_CONTROL5						0x09f0U
#define MAC_LAYER4_ADDRESS5						0x09f4U
#define MAC_LAYER3_ADDR0_REG5					0x0a00U
#define MAC_LAYER3_ADDR1_REG5					0x0a04U
#define MAC_LAYER3_ADDR2_REG5					0x0a08U
#define MAC_LAYER3_ADDR3_REG5					0x0a0cU
#define MAC_L3_L4_CONTROL6						0x0a20U
#define MAC_LAYER4_ADDRESS6						0x0a24U
#define MAC_LAYER3_ADDR0_REG6					0x0a30U
#define MAC_LAYER3_ADDR1_REG6					0x0a34U
#define MAC_LAYER3_ADDR2_REG6					0x0a38U
#define MAC_LAYER3_ADDR3_REG6					0x0a3cU
#define MAC_L3_L4_CONTROL7						0x0a50U
#define MAC_LAYER4_ADDRESS7						0x0a54U
#define MAC_LAYER3_ADDR0_REG7					0x0a60U
#define MAC_LAYER3_ADDR1_REG7					0x0a64U
#define MAC_LAYER3_ADDR2_REG7					0x0a68U
#define MAC_LAYER3_ADDR3_REG7					0x0a6cU
#endif /* EMAC_PFE_VARIANT */

#define MAC_TIMESTAMP_CONTROL					0x0b00U
#define MAC_SUB_SECOND_INCREMENT				0x0b04U
#define MAC_SYSTEM_TIME_SECONDS					0x0b08U
#define MAC_SYSTEM_TIME_NANOSECONDS				0x0b0cU
#define MAC_SYSTEM_TIME_SECONDS_UPDATE			0x0b10U
#define MAC_SYSTEM_TIME_NANOSECONDS_UPDATE		0x0b14U
#define MAC_TIMESTAMP_ADDEND					0x0b18U
#define MAC_SYSTEM_TIME_HIGHER_WORD_SECONDS		0x0b1cU
#define MAC_TIMESTAMP_STATUS					0x0b20U
#define MAC_TX_TIMESTAMP_STATUS_NANOSECONDS		0x0b30U
#define MAC_TX_TIMESTAMP_STATUS_SECONDS			0x0b34U
#define MAC_AUXILIARY_CONTROL					0x0b40U
#define MAC_AUXILIARY_TIMESTAMP_NANOSECONDS		0x0b48U
#define MAC_AUXILIARY_TIMESTAMP_SECONDS			0x0b4cU
#define MAC_TIMESTAMP_INGRESS_ASYM_CORR			0x0b50U
#define MAC_TIMESTAMP_EGRESS_ASYM_CORR			0x0b54U
#define MAC_TIMESTAMP_INGRESS_CORR_NANOSECOND	0x0b58U
#define MAC_TIMESTAMP_EGRESS_CORR_NANOSECOND	0x0b5cU
#define MAC_TIMESTAMP_INGRESS_CORR_SUBNANOSEC	0x0b60U
#define MAC_TIMESTAMP_EGRESS_CORR_SUBNANOSEC	0x0b64U
#define MAC_TIMESTAMP_INGRESS_LATENCY			0x0b68U
#define MAC_TIMESTAMP_EGRESS_LATENCY			0x0b6cU
#define MAC_PPS_CONTROL							0x0b70U
#define MAC_PPS0_TARGET_TIME_SECONDS			0x0b80U
#define MAC_PPS0_TARGET_TIME_NANOSECONDS		0x0b84U
#define MAC_PPS0_INTERVAL						0x0b88U
#define MAC_PPS0_WIDTH							0x0b8cU
#define MAC_PPS1_TARGET_TIME_SECONDS			0x0b90U
#define MAC_PPS1_TARGET_TIME_NANOSECONDS		0x0b94U
#define MAC_PPS1_INTERVAL						0x0b98U
#define MAC_PPS1_WIDTH							0x0b9cU
#define MAC_PPS2_TARGET_TIME_SECONDS			0x0ba0U
#define MAC_PPS2_TARGET_TIME_NANOSECONDS		0x0ba4U
#define MAC_PPS2_INTERVAL						0x0ba8U
#define MAC_PPS2_WIDTH							0x0bacU
#define MAC_PPS3_TARGET_TIME_SECONDS			0x0bb0U
#define MAC_PPS3_TARGET_TIME_NANOSECONDS		0x0bb4U
#define MAC_PPS3_INTERVAL						0x0bb8U
#define MAC_PPS3_WIDTH							0x0bbcU
#define MAC_PTO_CONTROL							0x0bc0U
#define MAC_SOURCE_PORT_IDENTITY0				0x0bc4U
#define MAC_SOURCE_PORT_IDENTITY1				0x0bc8U
#define MAC_SOURCE_PORT_IDENTITY2				0x0bccU
#define MAC_LOG_MESSAGE_INTERVAL				0x0bd0U

#define MTL_OPERATION_MODE						0x0c00U
#define MTL_DBG_CTL								0x0c08U
#define MTL_DBG_STS								0x0c0cU
#define MTL_FIFO_DEBUG_DATA						0x0c10U
#define MTL_INTERRUPT_STATUS					0x0c20U
#define MTL_RXQ_DMA_MAP0						0x0c30U
#define MTL_RXQ_DMA_MAP1						0x0c34U
#define MTL_TBS_CTRL							0x0c40U
#define MTL_EST_CONTROL							0x0c50U
#define MTL_EST_STATUS							0x0c58U
#define MTL_EST_SCH_ERROR						0x0c60U
#define MTL_EST_FRM_SIZE_ERROR					0x0c64U
#define MTL_EST_FRM_SIZE_CAPTURE				0x0c68U
#define MTL_EST_INTR_ENABLE						0x0c70U
#define MTL_EST_GCL_CONTROL						0x0c80U
#define MTL_EST_GCL_DATA						0x0c84U
#define MTL_FPE_CTRL_STS						0x0c90U
#define MTL_FPE_ADVANCE							0x0c94U
#define MTL_RXP_CONTROL_STATUS					0x0ca0U
#define MTL_RXP_INTERRUPT_CONTROL_STATUS		0x0ca4U
#define MTL_RXP_DROP_CNT						0x0ca8U
#define MTL_RXP_ERROR_CNT						0x0cacU
#define MTL_RXP_INDIRECT_ACC_CONTROL_STATUS		0x0cb0U
#define MTL_RXP_INDIRECT_ACC_DATA				0x0cb4U
#define MTL_ECC_CONTROL							0x0cc0U
#define MTL_SAFETY_INTERRUPT_STATUS				0x0cc4U
#define MTL_ECC_INTERRUPT_ENABLE				0x0cc8U
#define MTL_ECC_INTERRUPT_STATUS				0x0cccU
#define MTL_ECC_ERR_STS_RCTL					0x0cd0U
#define MTL_ECC_ERR_ADDR_STATUS					0x0cd4U
#define MTL_ECC_ERR_CNTR_STATUS					0x0cd8U
#define MTL_DPP_CONTROL							0x0ce0U

#define MTL_TXQ0_OPERATION_MODE					0x0d00U
#define MTL_TXQ0_UNDERFLOW						0x0d04U
#define MTL_TXQ0_DEBUG							0x0d08U
#define MTL_TXQ0_ETS_STATUS						0x0d14U
#define MTL_TXQ0_QUANTUM_WEIGHT					0x0d18U
#define MTL_Q0_INTERRUPT_CONTROL_STATUS			0x0d2cU
#define MTL_RXQ0_OPERATION_MODE					0x0d30U
#define MTL_RXQ0_MISSED_PACKET_OVERFLOW_CNT		0x0d34U
#define MTL_RXQ0_DEBUG							0x0d38U
#define MTL_RXQ0_CONTROL						0x0d3cU

#if !defined(EMAC_PFE_VARIANT)
#define MTL_TXQ1_OPERATION_MODE					0x0d40U
#define MTL_TXQ1_UNDERFLOW						0x0d44U
#define MTL_TXQ1_DEBUG							0x0d48U
#define MTL_TXQ1_ETS_CONTROL					0x0d50U
#define MTL_TXQ1_ETS_STATUS						0x0d54U
#define MTL_TXQ1_QUANTUM_WEIGHT					0x0d58U
#define MTL_TXQ1_SENDSLOPECREDIT				0x0d5cU
#define MTL_TXQ1_HICREDIT						0x0d60U
#define MTL_TXQ1_LOCREDIT						0x0d64U
#define MTL_Q1_INTERRUPT_CONTROL_STATUS			0x0d6cU
#define MTL_RXQ1_OPERATION_MODE					0x0d70U
#define MTL_RXQ1_MISSED_PACKET_OVERFLOW_CNT		0x0d74U
#define MTL_RXQ1_DEBUG							0x0d78U
#define MTL_RXQ1_CONTROL						0x0d7cU

#define MTL_TXQ2_OPERATION_MODE					0x0d80U
#define MTL_TXQ2_UNDERFLOW						0x0d84U
#define MTL_TXQ2_DEBUG							0x0d88U
#define MTL_TXQ2_ETS_CONTROL					0x0d90U
#define MTL_TXQ2_ETS_STATUS						0x0d94U
#define MTL_TXQ2_QUANTUM_WEIGHT					0x0d98U
#define MTL_TXQ2_SENDSLOPECREDIT				0x0d9cU
#define MTL_TXQ2_HICREDIT						0x0da0U
#define MTL_TXQ2_LOCREDIT						0x0da4U
#define MTL_Q2_INTERRUPT_CONTROL_STATUS			0x0dacU
#define MTL_RXQ2_OPERATION_MODE					0x0db0U
#define MTL_RXQ2_MISSED_PACKET_OVERFLOW_CNT		0x0db4U
#define MTL_RXQ2_DEBUG							0x0db8U
#define MTL_RXQ2_CONTROL						0x0dbcU

#define MTL_TXQ3_OPERATION_MODE					0x0dc0U
#define MTL_TXQ3_UNDERFLOW						0x0dc4U
#define MTL_TXQ3_DEBUG							0x0dc8U
#define MTL_TXQ3_ETS_CONTROL					0x0dd0U
#define MTL_TXQ3_ETS_STATUS						0x0dd4U
#define MTL_TXQ3_QUANTUM_WEIGHT					0x0dd8U
#define MTL_TXQ3_SENDSLOPECREDIT				0x0ddcU
#define MTL_TXQ3_HICREDIT						0x0de0U
#define MTL_TXQ3_LOCREDIT						0x0de4U
#define MTL_Q3_INTERRUPT_CONTROL_STATUS			0x0decU
#define MTL_RXQ3_OPERATION_MODE					0x0df0U
#define MTL_RXQ3_MISSED_PACKET_OVERFLOW_CNT		0x0df4U
#define MTL_RXQ3_DEBUG							0x0df8U
#define MTL_RXQ3_CONTROL						0x0dfcU

#define MTL_TXQ4_OPERATION_MODE					0x0e00U
#define MTL_TXQ4_UNDERFLOW						0x0e04U
#define MTL_TXQ4_DEBUG							0x0e08U
#define MTL_TXQ4_ETS_CONTROL					0x0e10U
#define MTL_TXQ4_ETS_STATUS						0x0e14U
#define MTL_TXQ4_QUANTUM_WEIGHT					0x0e18U
#define MTL_TXQ4_SENDSLOPECREDIT				0x0e1cU
#define MTL_TXQ4_HICREDIT						0x0e20U
#define MTL_TXQ4_LOCREDIT						0x0e24U
#define MTL_Q4_INTERRUPT_CONTROL_STATUS			0x0e2cU
#define MTL_RXQ4_OPERATION_MODE					0x0e30U
#define MTL_RXQ4_MISSED_PACKET_OVERFLOW_CNT		0x0e34U
#define MTL_RXQ4_DEBUG							0x0e38U
#define MTL_RXQ4_CONTROL						0x0e3cU
#endif /* EMAC_PFE_VARIANT */

#define DMA_MODE								0x1000U
#define DMA_SYSBUS_MODE							0x1004U
#define DMA_INTERRUPT_STATUS					0x1008U
#define DMA_DEBUG_STATUS0						0x100cU
#define DMA_DEBUG_STATUS1						0x1010U
#define AXI4_TX_AR_ACE_CONTROL					0x1020U
#define AXI4_RX_AW_ACE_CONTROL					0x1024U
#define AXI4_TXRX_AWAR_ACE_CONTROL				0x1028U
#define AXI_LPI_ENTRY_INTERVAL					0x1040U
#define DMA_TBS_CTRL							0x1050U
#define DMA_SAFETY_INTERRUPT_STATUS				0x1080U

#define DMA_CH0_CONTROL							0x1100U
#define DMA_CH0_TX_CONTROL						0x1104U
#define DMA_CH0_RX_CONTROL						0x1108U
#define DMA_CH0_TXDESC_LIST_ADDRESS				0x1114U
#define DMA_CH0_RXDESC_LIST_ADDRESS				0x111cU
#define DMA_CH0_TXDESC_TAIL_POINTER				0x1120U
#define DMA_CH0_RXDESC_TAIL_POINTER				0x1128U
#define DMA_CH0_TXDESC_RING_LENGTH				0x112cU
#define DMA_CH0_RXDESC_RING_LENGTH				0x1130U
#define DMA_CH0_INTERRUPT_ENABLE				0x1134U
#define DMA_CH0_RX_INTERRUPT_WATCHDOG_TIMER		0x1138U
#define DMA_CH0_SLOT_FUNCTION_CONTROL_STATUS	0x113cU
#define DMA_CH0_CURRENT_APP_TXDESC				0x1144U
#define DMA_CH0_CURRENT_APP_RXDESC				0x114cU
#define DMA_CH0_CURRENT_APP_TXBUFFER			0x1154U
#define DMA_CH0_CURRENT_APP_RXBUFFER			0x115cU
#define DMA_CH0_STATUS							0x1160U
#define DMA_CH0_MISS_FRAME_CNT					0x1164U
#define DMA_CH0_RXP_ACCEPT_CNT					0x1168U
#define DMA_CH0_RX_ERI_CNT						0x116cU

#if !defined (EMAC_PFE_VARIANT)
#define DMA_CH1_CONTROL							0x1180U
#define DMA_CH1_TX_CONTROL						0x1184U
#define DMA_CH1_RX_CONTROL						0x1188U
#define DMA_CH1_TXDESC_LIST_ADDRESS				0x1194U
#define DMA_CH1_RXDESC_LIST_ADDRESS				0x119cU
#define DMA_CH1_TXDESC_TAIL_POINTER				0x11a0U
#define DMA_CH1_RXDESC_TAIL_POINTER				0x11a8U
#define DMA_CH1_TXDESC_RING_LENGTH				0x11acU
#define DMA_CH1_RXDESC_RING_LENGTH				0x11b0U
#define DMA_CH1_INTERRUPT_ENABLE				0x11b4U
#define DMA_CH1_RX_INTERRUPT_WATCHDOG_TIMER		0x11b8U
#define DMA_CH1_SLOT_FUNCTION_CONTROL_STATUS	0x11bcU
#define DMA_CH1_CURRENT_APP_TXDESC				0x11c4U
#define DMA_CH1_CURRENT_APP_RXDESC				0x11ccU
#define DMA_CH1_CURRENT_APP_TXBUFFER			0x11d4U
#define DMA_CH1_CURRENT_APP_RXBUFFER			0x11dcU
#define DMA_CH1_STATUS							0x11e0U
#define DMA_CH1_MISS_FRAME_CNT					0x11e4U
#define DMA_CH1_RXP_ACCEPT_CNT					0x11e8U
#define DMA_CH1_RX_ERI_CNT						0x11ecU

#define DMA_CH2_CONTROL							0x1200U
#define DMA_CH2_TX_CONTROL						0x1204U
#define DMA_CH2_RX_CONTROL						0x1208U
#define DMA_CH2_TXDESC_LIST_ADDRESS				0x1214U
#define DMA_CH2_RXDESC_LIST_ADDRESS				0x121cU
#define DMA_CH2_TXDESC_TAIL_POINTER				0x1220U
#define DMA_CH2_RXDESC_TAIL_POINTER				0x1228U
#define DMA_CH2_TXDESC_RING_LENGTH				0x122cU
#define DMA_CH2_RXDESC_RING_LENGTH				0x1230U
#define DMA_CH2_INTERRUPT_ENABLE				0x1234U
#define DMA_CH2_RX_INTERRUPT_WATCHDOG_TIMER		0x1238U
#define DMA_CH2_SLOT_FUNCTION_CONTROL_STATUS	0x123cU
#define DMA_CH2_CURRENT_APP_TXDESC				0x1244U
#define DMA_CH2_CURRENT_APP_RXDESC				0x124cU
#define DMA_CH2_CURRENT_APP_TXBUFFER			0x1254U
#define DMA_CH2_CURRENT_APP_RXBUFFER			0x125cU
#define DMA_CH2_STATUS							0x1260U
#define DMA_CH2_MISS_FRAME_CNT					0x1264U
#define DMA_CH2_RXP_ACCEPT_CNT					0x1268U
#define DMA_CH2_RX_ERI_CNT						0x126cU

#define DMA_CH3_CONTROL							0x1280U
#define DMA_CH3_TX_CONTROL						0x1284U
#define DMA_CH3_RX_CONTROL						0x1288U
#define DMA_CH3_TXDESC_LIST_ADDRESS				0x1294U
#define DMA_CH3_RXDESC_LIST_ADDRESS				0x129cU
#define DMA_CH3_TXDESC_TAIL_POINTER				0x12a0U
#define DMA_CH3_RXDESC_TAIL_POINTER				0x12a8U
#define DMA_CH3_TXDESC_RING_LENGTH				0x12acU
#define DMA_CH3_RXDESC_RING_LENGTH				0x12b0U
#define DMA_CH3_INTERRUPT_ENABLE				0x12b4U
#define DMA_CH3_RX_INTERRUPT_WATCHDOG_TIMER		0x12b8U
#define DMA_CH3_SLOT_FUNCTION_CONTROL_STATUS	0x12bcU
#define DMA_CH3_CURRENT_APP_TXDESC				0x12c4U
#define DMA_CH3_CURRENT_APP_RXDESC				0x12ccU
#define DMA_CH3_CURRENT_APP_TXBUFFER			0x12d4U
#define DMA_CH3_CURRENT_APP_RXBUFFER			0x12dcU
#define DMA_CH3_STATUS							0x12e0U
#define DMA_CH3_MISS_FRAME_CNT					0x12e4U
#define DMA_CH3_RXP_ACCEPT_CNT					0x12e8U
#define DMA_CH3_RX_ERI_CNT						0x12ecU

#define DMA_CH4_CONTROL							0x1300U
#define DMA_CH4_TX_CONTROL						0x1304U
#define DMA_CH4_RX_CONTROL						0x1308U
#define DMA_CH4_TXDESC_LIST_ADDRESS				0x1314U
#define DMA_CH4_RXDESC_LIST_ADDRESS				0x131cU
#define DMA_CH4_TXDESC_TAIL_POINTER				0x1320U
#define DMA_CH4_RXDESC_TAIL_POINTER				0x1328U
#define DMA_CH4_TXDESC_RING_LENGTH				0x132cU
#define DMA_CH4_RXDESC_RING_LENGTH				0x1330U
#define DMA_CH4_INTERRUPT_ENABLE				0x1334U
#define DMA_CH4_RX_INTERRUPT_WATCHDOG_TIMER		0x1338U
#define DMA_CH4_SLOT_FUNCTION_CONTROL_STATUS	0x133cU
#define DMA_CH4_CURRENT_APP_TXDESC				0x1344U
#define DMA_CH4_CURRENT_APP_RXDESC				0x134cU
#define DMA_CH4_CURRENT_APP_TXBUFFER			0x1354U
#define DMA_CH4_CURRENT_APP_RXBUFFER			0x135cU
#define DMA_CH4_STATUS							0x1360U
#define DMA_CH4_MISS_FRAME_CNT					0x1364U
#define DMA_CH4_RXP_ACCEPT_CNT					0x1368U
#define DMA_CH4_RX_ERI_CNT						0x136cU
#endif /* EMAC_PFE_VARIANT */

/*	MAC_PACKET_FILTER Bits */
#define DROP_NON_TCP_UDP(x)				((!!x) ? (1U << 21) : 0U)	/* DNTU */
#define L3_L4_FILTER_ENABLE(x)			((!!x) ? (1U << 20) : 0U)	/* IPFE */
#define VLAN_TAG_FILTER_ENABLE(x)		((!!x) ? (1U << 16) : 0U)	/* VTFE */
#define HASH_OR_PERFECT_FILTER(x)		((!!x) ? (1U << 10) : 0U)	/* HPF  */
#define SA_FILTER(x)					((!!x) ? (1U << 9) : 0U)	/* SAF  */
#define SA_INVERSE_FILTER(x)			((!!x) ? (1U << 8) : 0U)	/* SAIF  */
#define PASS_CONTROL_PACKETS(x)			(((x) & 3U) << 6)			/* PCF  */
#define BLOCK_ALL						0x0U
#define	FORWARD_ALL_EXCEPT_PAUSE		0x1U
#define FORWARD_ALL						0x2U
#define FORWARD_ADDRESS_FILTERED		0x3U
#define DISABLE_BROADCAST_PACKETS(x)	((!!x) ? (1U << 5) : 0U)	/* DBF  */
#define PASS_ALL_MULTICAST(x)			((!!x) ? (1U << 4) : 0U)	/* PM  */
#define DA_INVERSE_FILTER(x)			((!!x) ? (1U << 3) : 0U)	/* DAIF  */
#define HASH_MULTICAST(x)				((!!x) ? (1U << 2) : 0U)	/* HMC  */
#define HASH_UNICAST(x)					((!!x) ? (1U << 1) : 0U)	/* HUC  */
#define PROMISCUOUS_MODE(x)				((!!x) ? (1U << 0) : 0U)	/* PR  */

/*	MAC_CONFIGURATION Bits */
#define ARP_OFFLOAD_ENABLE(x)			((!!x) ? (1U << 31) : 0U)	/* ARPEN  */
#define SA_INSERT_REPLACE_CONTROL(x)	(((x) & 0x7U) << 28)	/* SARC   */
#define CTRL_BY_SIGNALS					0x0U
#define INSERT_MAC0						0x2U
#define INSERT_MAC1						0x6U
#define REPLACE_BY_MAC0					0x3U
#define	REPLACE_BY_MAC1					0x7U
#define CHECKSUM_OFFLOAD(x)				((!!x) ? (1U << 27) : 0U)	/* IPC    */
#define INTER_PACKET_GAP(x)				(((x) & 0x7U) << 24)		/* IPG	  */
#define GIANT_PACKET_LIMIT_CONTROL(x)	((!!x) ? (1U << 23) : 0U)	/* GPSLCE */
#define SUPPORT_2K_PACKETS(x)			((!!x) ? (1U << 22) : 0U)	/* S2KP   */
#define CRC_STRIPPING_FOR_TYPE(x)		((!!x) ? (1U << 21) : 0U)	/* CST    */
#define	AUTO_PAD_OR_CRC_STRIPPING(x)	((!!x) ? (1U << 20) : 0U)	/* ACS    */
#define	WATCHDOG_DISABLE(x)				((!!x) ? (1U << 19) : 0U)	/* WD     */
#define	PACKET_BURST_ENABLE(x)			((!!x) ? (1U << 18) : 0U)	/* BE     */
#define	JABBER_DISABLE(x)				((!!x) ? (1U << 17) : 0U)	/* JD     */
#define	JUMBO_PACKET_ENABLE(x)			((!!x) ? (1U << 16) : 0U)	/* JE     */
#define PORT_SELECT(x)					((!!x) ? (1U << 15) : 0U)	/* PS     */
#define SPEED(x)						((!!x) ? (1U << 14) : 0U)	/* FES    */
#define GET_LINE_SPEED(x)				(((x) >> 14) & 3U)			/* FES+PS */
#define DUPLEX_MODE(x)					((!!x) ? (1U << 13) : 0U)	/* DM     */
#define GET_DUPLEX_MODE(x)				(((x) >> 13) & 1U)			/* DM     */
#define LOOPBACK_MODE(x)				((!!x) ? (1U << 12) : 0U)	/* LM     */
#define CARRIER_SENSE_BEFORE_TX(x)		((!!x) ? (1U << 11) : 0U)	/* ECRSFD */
#define DISABLE_RECEIVE_OWN(x)			((!!x) ? (1U << 10) : 0U)	/* DO     */
#define DISABLE_CARRIER_SENSE_TX(x)		((!!x) ? (1U << 9) : 0U)	/* DCRS   */
#define DISABLE_RETRY(x)				((!!x) ? (1U << 8) : 0U)	/* DR     */
#define BACK_OFF_LIMIT(x)				(((x) & 3U) << 5)			/* BL     */
#define MIN_N_10						0x0U
#define MIN_N_8							0x1U
#define MIN_N_4							0x2U
#define MIN_N_1							0x3U
#define DEFERRAL_CHECK(x)				((!!x) ? (1U << 4) : 0U)	/* DC     */
#define PREAMBLE_LENGTH_TX(x)			(((x) & 3U) << 2)			/* PRELEN */
#define PREAMBLE_7B						0x0U
#define PREAMBLE_5B						0x1U
#define PREAMBLE_3B						0x2U
#define TRANSMITTER_ENABLE(x)			((!!x) ? (1U << 1) : 0U)	/* TE     */
#define RECEIVER_ENABLE(x)				((!!x) ? (1U << 0) : 0U)	/* RE     */

/*	MAC_VLAN_TAG_CTRL Bits */
#define ENABLE_DOUBLE_VLAN(x)			((!!x) ? (1U << 26) : 0U)	/* EDVLP  */

/*	MAC_EXT_CONFIGURATION Bits */
#define GIANT_PACKET_SIZE_LIMIT(x)		(((x) & 0x3fffU) << 0)		/* GPSL   */

/*	MAC_Q0_TX_FLOW_CTRL Bits */
#define TX_FLOW_CONTROL_ENABLE(x)		((!!x) ? (1U << 1) : 0U)	/* TFE    */
#define BUSY_OR_BACKPRESSURE_ACTIVE(x)	((!!x) ? (1U << 0) : 0U)	/* FCB_BPA */

/*	MAC_MDIO_ADDRESS Bits */
#define GMII_BUSY(x)					((!!x) ? (1U << 0) : 0U)	/* GB     */
#define CLAUSE45_ENABLE(x)				((!!x) ? (1U << 1) : 0U)	/* C45E   */
#define GMII_OPERATION_CMD(x)			(((x) & 0x3U) << 2)
#define GMII_WRITE						0x1U
#define GMII_POST_INC_ADDR_CLAUSE45		0x2U
#define GMII_READ						0x3U
#define SKIP_ADDRESS_PACKET(x)			((!!x) ? (1U << 4) : 0U)	/* SKAP   */
#define CSR_CLOCK_RANGE(x)				(((x) & 0xfU) << 8)			/* CR     */
#define CSR_CLK_60_100_MHZ_MDC_CSR_DIV_42		0x0U
#define CSR_CLK_100_150_MHZ_MDC_CSR_DIV_62		0x1U
#define CSR_CLK_20_35_MHZ_MDC_CSR_DIV_16		0x2U
#define CSR_CLK_35_60_MHZ_MDC_CSR_DIV_26		0x3U
#define CSR_CLK_150_250_MHZ_MDC_CSR_DIV_102		0x4U
#define CSR_CLK_250_300_MHZ_MDC_CSR_DIV_124		0x5U
#define CSR_CLK_300_500_MHZ_MDC_CSR_DIV_204		0x6U
#define CSR_CLK_500_800_MHZ_MDC_CSR_DIV_324		0x7U
#define CSR_DIV_4						0x8U
#define CSR_DIV_6						0x9U
#define CSR_DIV_8						0xaU
#define CSR_DIV_10						0xbU
#define CSR_DIV_12						0xcU
#define CSR_DIV_14						0xdU
#define CSR_DIV_16						0xeU
#define CSR_DIV_18						0xfU
#define NUM_OF_TRAILING_CLOCKS(x)		(((x) & 0x7U) << 12)		/* NTC    */
#define REG_DEV_ADDR(x)					(((x) & 0x1fU) << 16)		/* RDA    */
#define PHYS_LAYER_ADDR(x)				(((x) & 0x1fU) << 21)		/* PA     */
#define BACK_TO_BACK(x)					((!!x) ? (1U << 26) : 0U)	/* BTB    */
#define PREAMBLE_SUPPRESSION(x)			((!!x) ? (1U << 27) : 0U)	/* PSE    */

/*	MAC_MDIO_DATA Bits */
#define GMII_DATA(x)					((x) & 0xffffU)
#define GMII_REGISTER_ADDRESS(x)		(((x) & 0xffffU) << 16)

/*	MTL_RXQ0_OPERATION_MODE Bits */
#define FORWARD_ERROR_PACKETS(x)		((!!x) ? (1U << 4) : 0U)	/* FEP    */

/*	MAC_TIMESTAMP_CONTROL Bits */
#define ENABLE_TIMESTAMP(x)				((!!x) ? (1U << 0) : 0U)	/* TSENA      */
#define INITIALIZE_TIMESTAMP(x)			((!!x) ? (1U << 2) : 0U)	/* TSINIT     */
#define ENABLE_TIMESTAMP_FOR_All(x)		((!!x) ? (1U << 8) : 0U)	/* TSENALL    */
#define ENABLE_PTP_PROCESSING(x)		((!!x) ? (1U << 11) : 0U)	/* TSIPENA    */
#define SELECT_PTP_PACKETS(x)			(((x) & 0x3U) << 16)			/* SNAPTYPSEL */

/* MAC_PHYIF_Control_Status Bits */
#define LNKSTS(x)						(((x) >> 19) & 0x1U)
#define LNKSPEED(x)						(((x) >> 17) & 0x3U)
#define LNKMOD(x)						(((x) >> 16) & 0x1U)

/**
 * @brief	Number of HW slots able to hold individual MAC addresses
 * @details	The HW can have multiple individual MAC addresses assigned at
 * 			a time. The number is limited and this parameter specifies
 * 			number of available HW resources.
 */
#define EMAC_CFG_INDIVIDUAL_ADDR_SLOTS_COUNT	8U

errno_t pfe_emac_cfg_init(void *base_va, pfe_emac_mii_mode_t mode, pfe_emac_speed_t speed, pfe_emac_duplex_t duplex);
errno_t pfe_emac_cfg_set_duplex(void *base_va, pfe_emac_duplex_t duplex);
errno_t pfe_emac_cfg_set_mii_mode(void *base_va, pfe_emac_mii_mode_t mode);
errno_t pfe_emac_cfg_set_speed(void *base_va, pfe_emac_speed_t speed);
errno_t pfe_emac_cfg_set_max_frame_length(void *base_va, uint32_t len);
errno_t pfe_emac_cfg_get_link_config(void *base_va, pfe_emac_speed_t *speed, pfe_emac_duplex_t *duplex);
errno_t pfe_emac_cfg_get_link_status(void *base_va, pfe_emac_link_speed_t *link_speed, pfe_emac_duplex_t *duplex, bool_t *link);
void pfe_emac_cfg_write_addr_slot(void *base_va, pfe_mac_addr_t addr, uint8_t slot);
uint32_t pfe_emac_cfg_get_hash(void *base_va, pfe_mac_addr_t addr);
void pfe_emac_cfg_set_uni_group(void *base_va, int32_t hash, bool_t en);
void pfe_emac_cfg_set_multi_group(void *base_va, int32_t hash, bool_t en);
void pfe_emac_cfg_set_loopback(void *base_va, bool_t en);
void pfe_emac_cfg_set_promisc_mode(void *base_va, bool_t en);
void pfe_emac_cfg_set_broadcast(void *base_va, bool_t en);
void pfe_emac_cfg_set_enable(void *base_va, bool_t en);
void pfe_emac_cfg_set_flow_control(void *base_va, bool_t en);
errno_t pfe_emac_cfg_mdio_read22(void *base_va, uint8_t pa, uint8_t ra, uint16_t *val);
errno_t pfe_emac_cfg_mdio_read45(void *base_va, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t *val);
errno_t pfe_emac_cfg_mdio_write22(void *base_va, uint8_t pa, uint8_t ra, uint16_t val);
errno_t pfe_emac_cfg_mdio_write45(void *base_va, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t val);
uint32_t pfe_emac_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level);

#endif /* SRC_PFE_EMAC_CSR_H_ */

/** @}*/
