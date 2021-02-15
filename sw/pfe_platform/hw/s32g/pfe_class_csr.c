/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2021 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_class_csr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

/**
 * @brief		Initialize and configure the CLASS block
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	cfg Pointer to the configuration structure
 */
void pfe_class_cfg_set_config(void *base_va, pfe_class_cfg_t *cfg)
{
	(void)cfg;

	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_FREE_CTRL, base_va + CLASS_BMU1_BUF_FREE);
	hal_write32(CLASS_PE0_RO_DM_ADDR0_VAL, base_va + CLASS_PE0_RO_DM_ADDR0);
	hal_write32(CLASS_PE0_RO_DM_ADDR1_VAL, base_va + CLASS_PE0_RO_DM_ADDR1);
	hal_write32(CLASS_PE0_QB_DM_ADDR0_VAL, base_va + CLASS_PE0_QB_DM_ADDR0);
	hal_write32(CLASS_PE0_QB_DM_ADDR1_VAL, base_va + CLASS_PE0_QB_DM_ADDR1);
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + TMU_PHY_INQ_PKTPTR, base_va + CLASS_TM_INQ_ADDR);
	hal_write32(0x18U, base_va + CLASS_MAX_BUF_CNT);
	hal_write32(0x14U, base_va + CLASS_AFULL_THRES);
	hal_write32(0x3c0U, base_va + CLASS_INQ_AFULL_THRES);
	hal_write32(0x1U, base_va + CLASS_USE_TMU_INQ);
	hal_write32(0x1U, base_va + CLASS_PE_SYS_CLK_RATIO);
	hal_write32(0U, base_va + CLASS_L4_CHKSUM);
	hal_write32((PFE_CFG_RO_HDR_SIZE << 16) | PFE_CFG_LMEM_HDR_SIZE, base_va + CLASS_HDR_SIZE);
	hal_write32(PFE_CFG_LMEM_BUF_SIZE, base_va + CLASS_LMEM_BUF_SIZE);

	hal_write32(0U
			| RT_TWO_LEVEL_REF(FALSE)
			| PHYNO_IN_HASH(FALSE)
			| PARSE_ROUTE_EN(FALSE)
			| VLAN_AWARE_BRIDGE(TRUE)
			| PARSE_BRIDGE_EN(TRUE)
			| IPALIGNED_PKT(FALSE)
			| ARC_HIT_CHECK_EN(FALSE)
			| VLAN_AWARE_BRIDGE_PHY1(FALSE)
			| VLAN_AWARE_BRIDGE_PHY2(FALSE)
			| VLAN_AWARE_BRIDGE_PHY3(FALSE)
			| CLASS_TOE(FALSE)
			| ASYM_HASH(ASYM_HASH_NORMAL)
			| SYM_RTENTRY(FALSE)
			| QB2BUS_ENDIANESS(TRUE)
			| LEN_CHECK(FALSE)
			, base_va + CLASS_ROUTE_MULTI);
}

/**
 * @brief		Reset the classifier block
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_reset(void *base_va)
{
	hal_write32(PFE_CORE_SW_RESET, base_va + CLASS_TX_CTRL);
}

/**
 * @brief		Enable the classifier block
 * @details		Enable all classifier PEs
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_enable(void *base_va)
{
	hal_write32(PFE_CORE_ENABLE, base_va + CLASS_TX_CTRL);
}

/**
 * @brief		Disable the classifier block
 * @details		Disable all classifier PEs
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_disable(void *base_va)
{
	hal_write32(PFE_CORE_DISABLE, base_va + CLASS_TX_CTRL);
}

/**
 * @brief		Set up routing table
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	rtable_pa Physical address of the routing table space
 * @param[in]	rtable_len Number of entries in the table
 * @param[in]	entry_size Routing table entry size in number of bytes
 */
void pfe_class_cfg_set_rtable(void *base_va, void *rtable_pa, uint32_t rtable_len, uint32_t entry_size)
{
	uint8_t ii;
	uint32_t reg = hal_read32(base_va + CLASS_ROUTE_MULTI);

	if (NULL == rtable_pa)
	{
		hal_write32(reg & (~PARSE_ROUTE_EN(TRUE)), base_va + CLASS_ROUTE_MULTI);
		return;
	}

	/* rtable not NULL, add it */
	if (entry_size > ROUTE_ENTRY_SIZE(0xffffffffu))
	{
		NXP_LOG_ERROR("Entry size exceeds maximum value\n");
	}

	reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
	if (0U != (reg & PARSE_ROUTE_EN(TRUE)))
	{
		if (entry_size != 128U)
		{
			NXP_LOG_ERROR("FATAL: Route table entry length exceeds 128bytes\n");
		}
	}

	for (ii=0U; ii<(sizeof(rtable_len) * 8U); ii++)
	{
		if (0U != (rtable_len & (1UL << ii)))
		{
			if (0U != (rtable_len & ~(1UL << ii)))
			{
				NXP_LOG_WARNING("Routing table length is not a power of 2\n");
			}

			if ((ii < 6U) || (ii > 20U))
			{
				NXP_LOG_WARNING("Table length out of boundaries\n");
			}

			break;
		}
	}

	hal_write32((uint32_t)((addr_t)rtable_pa & 0xffffffffU), base_va + CLASS_ROUTE_TABLE_BASE);
	hal_write32(0UL
				| ROUTE_HASH_SIZE(ii)
				| ROUTE_ENTRY_SIZE(entry_size)
				, base_va + CLASS_ROUTE_HASH_ENTRY_SIZE);

	reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
	hal_write32(reg | PARSE_ROUTE_EN(TRUE), base_va + CLASS_ROUTE_MULTI);
}

/**
 * @brief		Set default VLAN ID
 * @details		Every packet without VLAN tag set received via physical interface will
 * 				be treated as packet with VLAN equal to this default VLAN ID.
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	vlan The default VLAN ID (12bit)
 */
void pfe_class_cfg_set_def_vlan(void *base_va, uint16_t vlan)
{
	hal_write32(0UL
			| USE_DEFAULT_VLANID(TRUE)
			| DEF_VLANID((uint32_t)vlan & (uint32_t)0xfffU)
			, base_va + CLASS_VLAN_ID);
}

/**
 * @brief		Get CLASS statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the CLASS block.
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_class_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/* Debug registers */
	if(verb_level >= 10U)
	{
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PE0_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE0_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PE1_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE1_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PE2_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE2_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PE3_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE3_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PE4_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE4_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PE5_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE5_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PE6_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE6_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PE7_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE7_DEBUG));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_STATE\t0x%x\n", hal_read32(base_va + CLASS_STATE));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_QB_BUF_AVAIL\t0x%x\n", hal_read32(base_va + CLASS_QB_BUF_AVAIL));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_RO_BUF_AVAIL\t0x%x\n", hal_read32(base_va + CLASS_RO_BUF_AVAIL));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_DEBUG_BUS01\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS01));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_DEBUG_BUS23\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS23));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_DEBUG_BUS45\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS45));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_DEBUG_BUS67\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS67));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_DEBUG_BUS89\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS89));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_DEBUG_BUS1011\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS1011));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_DEBUG_BUS12\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS12));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_RX_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V4_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V6_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_RX_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V4_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V6_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_RX_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V4_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V6_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_RX_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_L3_FAIL_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_V4_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_V6_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_CHKSUM_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_TTL_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_RX_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_L3_FAIL_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_V4_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_V6_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_CHKSUM_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_TTL_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_ICMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_ICMP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_IGMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_IGMP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_TCP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_TCP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY1_UDP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_UDP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_ICMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_ICMP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_IGMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_IGMP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_TCP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_TCP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY2_UDP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_UDP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_ICMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_ICMP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_IGMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_IGMP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_TCP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_TCP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY3_UDP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_UDP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_ICMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_ICMP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_IGMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_IGMP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_TCP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_TCP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_UDP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_UDP_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_RX_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_L3_FAIL_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_V4_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_V6_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_CHKSUM_ERR_PKTS));
		len += (uint32_t)oal_util_snprintf(buf + len, size - len, "CLASS_PHY4_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_TTL_ERR_PKTS));
	}

	if(verb_level >= 9U)
	{
		/*	Get version */
		reg = hal_read32(base_va + CLASS_VERSION);
		len += oal_util_snprintf(buf + len, size - len, "Revision\t0x%x\n", (reg >> 24U) & 0xffU);
		len += oal_util_snprintf(buf + len, size - len, "Version \t0x%x\n", (reg >> 16U) & 0xffU);
		len += oal_util_snprintf(buf + len, size - len, "ID      \t0x%x\n", reg & 0xffffU);
	}
		/*	CLASS_ROUTE_MULTI */
		reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
		len += oal_util_snprintf(buf + len, size - len, "CLASS_ROUTE_MULTI \t0x%x\n", reg);

		/*	CLASS_STATE */
		reg = hal_read32(base_va + CLASS_STATE);
		len += oal_util_snprintf(buf + len, size - len, "CLASS_STATE       \t0x%x\n", reg);

		reg = hal_read32(base_va + CLASS_QB_BUF_AVAIL);
		len += oal_util_snprintf(buf + len, size - len, "CLASS_QB_BUF_AVAIL\t0x%x\n", reg);

		reg = hal_read32(base_va + CLASS_RO_BUF_AVAIL);
		len += oal_util_snprintf(buf + len, size - len, "CLASS_RO_BUF_AVAIL\t0x%x\n", reg);

		reg = hal_read32(base_va + CLASS_PE0_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE0 PC\t0x%x\n", reg & 0xffffU);
		reg = hal_read32(base_va + CLASS_PE1_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE1 PC\t0x%x\n", reg & 0xffffU);
		reg = hal_read32(base_va + CLASS_PE2_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE2 PC\t0x%x\n", reg & 0xffffU);
		reg = hal_read32(base_va + CLASS_PE3_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE3 PC\t0x%x\n", reg & 0xffffU);
		reg = hal_read32(base_va + CLASS_PE4_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE4 PC\t0x%x\n", reg & 0xffffU);
		reg = hal_read32(base_va + CLASS_PE5_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE5 PC\t0x%x\n", reg & 0xffffU);
		reg = hal_read32(base_va + CLASS_PE6_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE6 PC\t0x%x\n", reg & 0xffffU);
		reg = hal_read32(base_va + CLASS_PE7_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE7 PC\t0x%x\n", reg & 0xffffU);

		/*	Get info per PHY */
		len += oal_util_snprintf(buf + len, size - len, "[PHY1]\n");

		len += oal_util_snprintf(buf + len, size - len, "RX\t%10u TX\t%10u\nIPV4\t%10u IPV6\t%10u\n",
				hal_read32(base_va + CLASS_PHY1_RX_PKTS),
				hal_read32(base_va + CLASS_PHY1_TX_PKTS),
				hal_read32(base_va + CLASS_PHY1_V4_PKTS),
				hal_read32(base_va + CLASS_PHY1_V6_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "ICMP\t%10u IGMP\t%10u TCP\t%10u UDP\t%10u\n",
				hal_read32(base_va + CLASS_PHY1_ICMP_PKTS),
				hal_read32(base_va + CLASS_PHY1_IGMP_PKTS),
				hal_read32(base_va + CLASS_PHY1_TCP_PKTS),
				hal_read32(base_va + CLASS_PHY1_UDP_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "L3 Fail\t%10u CSUM Fail\t%10u TTL Fail\t%10u\n",
				hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS),
				hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS),
				hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "[PHY2]\n");

		len += oal_util_snprintf(buf + len, size - len, "RX\t%10u TX\t%10u\t IPV4\t%10u IPV6\t%10u\n",
				hal_read32(base_va + CLASS_PHY2_RX_PKTS),
				hal_read32(base_va + CLASS_PHY2_TX_PKTS),
				hal_read32(base_va + CLASS_PHY2_V4_PKTS),
				hal_read32(base_va + CLASS_PHY2_V6_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "ICMP\t%10u IGMP\t%10u TCP\t%10u UDP\t%10u\n",
				hal_read32(base_va + CLASS_PHY2_ICMP_PKTS),
				hal_read32(base_va + CLASS_PHY2_IGMP_PKTS),
				hal_read32(base_va + CLASS_PHY2_TCP_PKTS),
				hal_read32(base_va + CLASS_PHY2_UDP_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "L3 Fail\t%10u CSUM Fail\t%10u TTL Fail\t%10u\n",
				hal_read32(base_va + CLASS_PHY2_L3_FAIL_PKTS),
				hal_read32(base_va + CLASS_PHY2_CHKSUM_ERR_PKTS),
				hal_read32(base_va + CLASS_PHY2_TTL_ERR_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "[PHY3]\n");

		len += oal_util_snprintf(buf + len, size - len, "RX\t%10u TX\t%10u\nIPV4\t%10u IPV6\t%10u\n",
				hal_read32(base_va + CLASS_PHY3_RX_PKTS),
				hal_read32(base_va + CLASS_PHY3_TX_PKTS),
				hal_read32(base_va + CLASS_PHY3_V4_PKTS),
				hal_read32(base_va + CLASS_PHY3_V6_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "ICMP\t%10u IGMP\t%10u TCP\t%10u UDP\t%10u\n",
				hal_read32(base_va + CLASS_PHY3_ICMP_PKTS),
				hal_read32(base_va + CLASS_PHY3_IGMP_PKTS),
				hal_read32(base_va + CLASS_PHY3_TCP_PKTS),
				hal_read32(base_va + CLASS_PHY3_UDP_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "L3 Fail\t%10u CSUM Fail\t%10u TTL Fail\t%10u\n",
				hal_read32(base_va + CLASS_PHY3_L3_FAIL_PKTS),
				hal_read32(base_va + CLASS_PHY3_CHKSUM_ERR_PKTS),
				hal_read32(base_va + CLASS_PHY3_TTL_ERR_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "[PHY4]\n");

		len += oal_util_snprintf(buf + len, size - len, "RX\t%10u TX\t%10u\nIPV4\t%10u IPV6\t%10u\n",
				hal_read32(base_va + CLASS_PHY4_RX_PKTS),
				hal_read32(base_va + CLASS_PHY4_TX_PKTS),
				hal_read32(base_va + CLASS_PHY4_V4_PKTS),
				hal_read32(base_va + CLASS_PHY4_V6_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "ICMP\t%10u IGMP\t%10u TCP\t%10u UDP\t%10u\n",
				hal_read32(base_va + CLASS_PHY4_ICMP_PKTS),
				hal_read32(base_va + CLASS_PHY4_IGMP_PKTS),
				hal_read32(base_va + CLASS_PHY4_TCP_PKTS),
				hal_read32(base_va + CLASS_PHY4_UDP_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "L3 Fail\t%10u CSUM Fail\t%10u TTL Fail\t%10u\n",
				hal_read32(base_va + CLASS_PHY4_L3_FAIL_PKTS),
				hal_read32(base_va + CLASS_PHY4_CHKSUM_ERR_PKTS),
				hal_read32(base_va + CLASS_PHY4_TTL_ERR_PKTS));

	return len;
}
