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

#ifndef SRC_PFE_EMAC_CSR_H_
#define SRC_PFE_EMAC_CSR_H_

#include "pfe_emac.h"

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

#define MAC_CONFIGURATION						0x0000U
#define MAC_EXT_CONFIGURATION					0x0004U
#define MAC_PACKET_FILTER						0x0008U
#define MAC_WATCHDOG_TIMEOUT					0x000cU
#define MAC_HASH_TABLE_REG0						0x0010U
#define MAC_HASH_TABLE_REG1						0x0014U
#define MAC_HASH_TABLE_REG(n)					(MAC_HASH_TABLE_REG0 + ((n) * 4))
#define MAC_VLAN_TAG_CTRL						0x0050U
#define MAC_VLAN_TAG_DATA						0x0054U
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
#define MAC_CSR_SW_CTRL							0x0230U
#define MAC_FPE_CTRL_STS						0x0234U
#define MAC_EXT_CFG1							0x0238U
#define MAC_PRESN_TIME_NS						0x0240U
#define MAC_PRESN_TIME_UPDT						0x0244U
#define MAC_ADDRESS0_HIGH						0x0300U
#define MAC_ADDRESS0_LOW						0x0304U
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

#define MAC_TIMESTAMP_CONTROL					0x0b00U
#define MAC_SUB_SECOND_INCREMENT				0x0b04U
#define MAC_SYSTEM_TIME_SECONDS					0x0b08U
#define MAC_SYSTEM_TIME_NANOSECONDS				0x0b0cU
#define MAC_STSU								0x0b10U
#define MAC_STNSU								0x0b14U
#define MAC_TIMESTAMP_ADDEND					0x0b18U
#define MTL_OPERATION_MODE						0x0c00U
#define MTL_DPP_CONTROL							0x0ce0U
#define MTL_TXQ0_OPERATION_MODE					0x0d00U
#define MTL_RXQ0_OPERATION_MODE					0x0d30U

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
#define ENABLE_DOUBLE_VLAN(x)			((!!x) ? (1U << 26) : 0U)	/* EDVLP  */
#define GIANT_PACKET_SIZE_LIMIT(x)		(((x) & 0x3fffU) << 0)		/* GPSL   */
#define TX_FLOW_CONTROL_ENABLE(x)		((!!x) ? (1U << 1) : 0U)	/* TFE    */
#define BUSY_OR_BACKPRESSURE_ACTIVE(x)	((!!x) ? (1U << 0) : 0U)	/* FCB_BPA */
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
#define GMII_DATA(x)					((x) & 0xffffU)
#define GMII_REGISTER_ADDRESS(x)		(((x) & 0xffffU) << 16)
#define FINE_UPDATE(x)					((!!x) ? (1U << 1) : 0U)	/* TSCFUPDT   */
#define UPDATE_TIMESTAMP(x)				((!!x) ? (1U << 3) : 0U)	/* TSUPDT     */
#define FORWARD_ERROR_PACKETS(x)		((!!x) ? (1U << 4) : 0U)	/* FEP        */
#define UPDATE_ADDEND(x)				((!!x) ? (1U << 5) : 0U)	/* TSADDREG   */
#define ENABLE_TIMESTAMP(x)				((!!x) ? (1U << 0) : 0U)	/* TSENA      */
#define INITIALIZE_TIMESTAMP(x)			((!!x) ? (1U << 2) : 0U)	/* TSINIT     */
#define ENABLE_TIMESTAMP_FOR_All(x)		((!!x) ? (1U << 8) : 0U)	/* TSENALL    */
#define DIGITAL_ROLLOVER(x)				((!!x) ? (1U << 9) : 0U)	/* TSCTRLSSR  */
#define ENABLE_PTP_PROCESSING(x)		((!!x) ? (1U << 11) : 0U)	/* TSIPENA    */
#define PTP_OVER_IPV4(x)				((!!x) ? (1U << 13) : 0U)	/* TSIPV4ENA  */
#define PTP_OVER_IPV6(x)				((!!x) ? (1U << 12) : 0U)	/* TSIPV6ENA  */
#define PTP_OVER_ETH(x)					((!!x) ? (1U << 11) : 0U)	/* TSIPENA    */
#define PTPV2(x)						((!!x) ? (1U << 10) : 0U)	/* TSVER2ENA  */
#define SELECT_PTP_PACKETS(x)			(((x) & 0x3U) << 16)		/* SNAPTYPSEL */
#define EXTERNAL_TIME(x)				((!!x) ? (1U << 20) : 0U)	/* ESTI */
#define LNKSTS(x)						(((x) >> 19) & 0x1U)
#define LNKSPEED(x)						(((x) >> 17) & 0x3U)
#define LNKMOD(x)						(((x) >> 16) & 0x1U)
#define ADDSUB(x)						((!!x) ? (1U << 31) : 0U)

/**
 * @brief	Number of HW slots able to hold individual MAC addresses
 * @details	The HW can have multiple individual MAC addresses assigned at
 * 			a time. The number is limited and this parameter specifies
 * 			number of available HW resources.
 */
#define EMAC_CFG_INDIVIDUAL_ADDR_SLOTS_COUNT	8U

errno_t pfe_emac_cfg_init(void *base_va, pfe_emac_mii_mode_t mode, pfe_emac_speed_t speed, pfe_emac_duplex_t duplex);
errno_t pfe_emac_cfg_enable_ts(void *base_va, bool_t eclk, uint32_t i_clk_hz, uint32_t o_clk_hz);
void pfe_emac_cfg_disable_ts(void *base_va);
errno_t pfe_emac_cfg_adjust_ts_freq(void *base_va, uint32_t i_clk_hz, uint32_t o_clk_hz, uint32_t ppb, bool_t sgn);
void pfe_emac_cfg_get_ts_time(void *base_va, uint32_t *sec, uint32_t *nsec);
errno_t pfe_emac_cfg_set_ts_time(void *base_va, uint32_t sec, uint32_t nsec);
errno_t pfe_emac_cfg_adjust_ts_time(void *base_va, uint32_t sec, uint32_t nsec, bool_t sgn);
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
