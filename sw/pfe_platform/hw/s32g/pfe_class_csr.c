/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
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
#include "pfe_feature_mgr.h"

#ifndef PFE_CBUS_H_
#error Missing cbus.h
#endif /* PFE_CBUS_H_ */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static errno_t pfe_class_cfg_validate_rtable_len(uint32_t rtable_len, uint8_t *rtable_idx);

/**
 * @brief		Initialize and configure the CLASS block
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	cfg Pointer to the configuration structure
 */
void pfe_class_cfg_set_config(addr_t base_va, const pfe_class_cfg_t *cfg)
{
	uint32_t regval;
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
	hal_write32(((uint32_t)cfg->ro_header_size << 16U) | (uint32_t)cfg->lmem_header_size, base_va + CLASS_HDR_SIZE);
	hal_write32(PFE_CFG_LMEM_BUF_SIZE, base_va + CLASS_LMEM_BUF_SIZE);
	hal_write32(CLASS_TPID0_TPID1_VAL, base_va + CLASS_TPID0_TPID1);
	hal_write32(CLASS_TPID2_VAL, base_va + CLASS_TPID2);
	regval = hal_read32(base_va + CLASS_AXI_CTRL_ADDR);
	if (TRUE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
	{
		regval &= ~AXI_DBUS_BURST_SIZE(0x3ffU);
		regval |= AXI_DBUS_BURST_SIZE(0x200U);
		regval |= 0x3U;
		hal_write32(regval, base_va + CLASS_AXI_CTRL_ADDR);
	}
	else if (cfg->g2_ordered_class_writes)
	{
		regval |= 0x3U;
		hal_write32(regval, base_va + CLASS_AXI_CTRL_ADDR);
	}

	hal_write32(0U
			| RT_TWO_LEVEL_REF(FALSE)
			| PHYNO_IN_HASH(FALSE)
			| PARSE_ROUTE_EN(FALSE)
			| VLAN_AWARE_BRIDGE(TRUE)
			| PARSE_BRIDGE_EN(FALSE)
			| IPALIGNED_PKT(FALSE)
			| ARC_HIT_CHECK_EN(FALSE)
			| VLAN_AWARE_BRIDGE_PHY1(FALSE)
			| VLAN_AWARE_BRIDGE_PHY2(FALSE)
			| VLAN_AWARE_BRIDGE_PHY3(FALSE)
			| CLASS_TOE(FALSE)
			| ASYM_HASH(ASYM_HASH_SIP_SPORT_CRC)
			| SYM_RTENTRY(FALSE)
			| QB2BUS_ENDIANESS(TRUE)
			| LEN_CHECK(FALSE)
			, base_va + CLASS_ROUTE_MULTI);
}

/**
 * @brief		Reset the classifier block
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_reset(addr_t base_va)
{
	hal_write32(PFE_CORE_SW_RESET, base_va + CLASS_TX_CTRL);
}

/**
 * @brief		Enable the classifier block
 * @details		Enable all classifier PEs
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_enable(addr_t base_va)
{
	hal_write32(PFE_CORE_ENABLE, base_va + CLASS_TX_CTRL);
}

/**
 * @brief		Disable the classifier block
 * @details		Disable all classifier PEs
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_disable(addr_t base_va)
{
	hal_write32(PFE_CORE_DISABLE, base_va + CLASS_TX_CTRL);
}

/**
 * @brief		Validate rtable length
 * @param[in]	rtable_len Number of entries in the table
 * @param[in]	rtable_idx Pointer to the rtable index
 */
static errno_t pfe_class_cfg_validate_rtable_len(uint32_t rtable_len, uint8_t *rtable_idx)
{
	errno_t ret = EOK;
	uint8_t idx;

	/* Validate that rtable_len is a power of 2 and it's within boundaries. */
	for (idx = 0U; idx < (sizeof(rtable_len) * 8U); idx++)
	{
		if (0U != (rtable_len & (1UL << idx)))
		{
			if (0U != (rtable_len & ~(1UL << idx)))
			{
				NXP_LOG_ERROR("Routing table length is not a power of 2\n");
				ret = EINVAL;
			}
			else if ((idx < 6U) || (idx > 20U))
			{
				NXP_LOG_ERROR("Table length out of boundaries\n");
				ret = EINVAL;
			}
			else
			{
				ret = EOK;
			}

			break;
		}
	}
	
	*rtable_idx = idx;
	
	return ret;
}

/**
 * @brief		Set up routing table
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	rtable_pa Physical address of the routing table space
 * @param[in]	rtable_len Number of entries in the table
 * @param[in]	entry_size Routing table entry size in number of bytes
 * @return		Execution status, EOK if success, error code otherwise
 */
errno_t pfe_class_cfg_set_rtable(addr_t base_va, addr_t rtable_pa, uint32_t rtable_len, uint32_t entry_size)
{
	uint32_t reg;
	errno_t ret = EOK;
	uint8_t rtable_idx;

	if (NULL_ADDR == rtable_pa)
	{
		pfe_class_cfg_rtable_lookup_disable(base_va);
		ret = EOK;
	}
	else
	{

		/* rtable not NULL, add it */
		if (entry_size > ROUTE_ENTRY_SIZE(0xffffffffu))
		{
			NXP_LOG_ERROR("Entry size exceeds maximum value\n");
			ret = EINVAL;
		}
		else
		{
			/* Validate rtable entry size if route parsing is already enabled. */
			reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
			if (0U != (reg & PARSE_ROUTE_EN(TRUE)))
			{
				if (entry_size != 128U)
				{
					NXP_LOG_ERROR("FATAL: Route table entry length exceeds 128bytes\n");
					ret = EINVAL;
				}
			}

			if (EOK == ret)
			{
				ret = pfe_class_cfg_validate_rtable_len(rtable_len, &rtable_idx);

				if (EOK == ret)
				{
					hal_write32((uint32_t)(rtable_pa & 0xffffffffU), base_va + CLASS_ROUTE_TABLE_BASE);
					hal_write32(0UL
								| ROUTE_HASH_SIZE(rtable_idx)
								| ROUTE_ENTRY_SIZE(entry_size)
								, base_va + CLASS_ROUTE_HASH_ENTRY_SIZE);

					/* Don't enable PARSE_ROUTE_EN here as it will be enabled only when needed later. */
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Set default VLAN ID
 * @details		Every packet without VLAN tag set received via physical interface will
 * 				be treated as packet with VLAN equal to this default VLAN ID.
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	vlan The default VLAN ID (12bit)
 */
void pfe_class_cfg_set_def_vlan(addr_t base_va, uint16_t vlan)
{
	hal_write32(0UL
			| USE_DEFAULT_VLANID(TRUE)
			| DEF_VLANID((uint32_t)vlan & (uint32_t)0xfffU)
			, base_va + CLASS_VLAN_ID);
}

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)

/**
 * @brief		Get CLASS statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the CLASS block.
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	seq 		Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_class_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level)
{
	uint32_t reg;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Debug registers */
		if(verb_level >= 10U)
		{
			seq_printf(seq, "CLASS_PE0_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE0_DEBUG));
			seq_printf(seq, "CLASS_PE1_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE1_DEBUG));
			seq_printf(seq, "CLASS_PE2_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE2_DEBUG));
			seq_printf(seq, "CLASS_PE3_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE3_DEBUG));
			seq_printf(seq, "CLASS_PE4_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE4_DEBUG));
			seq_printf(seq, "CLASS_PE5_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE5_DEBUG));
			seq_printf(seq, "CLASS_PE6_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE6_DEBUG));
			seq_printf(seq, "CLASS_PE7_DEBUG\t0x%x\n", hal_read32(base_va + CLASS_PE7_DEBUG));
			seq_printf(seq, "CLASS_STATE\t0x%x\n", hal_read32(base_va + CLASS_STATE));
			seq_printf(seq, "CLASS_QB_BUF_AVAIL\t0x%x\n", hal_read32(base_va + CLASS_QB_BUF_AVAIL));
			seq_printf(seq, "CLASS_RO_BUF_AVAIL\t0x%x\n", hal_read32(base_va + CLASS_RO_BUF_AVAIL));
			seq_printf(seq, "CLASS_DEBUG_BUS01\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS01));
			seq_printf(seq, "CLASS_DEBUG_BUS23\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS23));
			seq_printf(seq, "CLASS_DEBUG_BUS45\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS45));
			seq_printf(seq, "CLASS_DEBUG_BUS67\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS67));
			seq_printf(seq, "CLASS_DEBUG_BUS89\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS89));
			seq_printf(seq, "CLASS_DEBUG_BUS1011\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS1011));
			seq_printf(seq, "CLASS_DEBUG_BUS12\t0x%x\n", hal_read32(base_va + CLASS_DEBUG_BUS12));
			seq_printf(seq, "CLASS_PHY1_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_RX_PKTS));
			seq_printf(seq, "CLASS_PHY1_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS));
			seq_printf(seq, "CLASS_PHY1_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V4_PKTS));
			seq_printf(seq, "CLASS_PHY1_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V6_PKTS));
			seq_printf(seq, "CLASS_PHY1_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY1_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY1_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_RX_PKTS));
			seq_printf(seq, "CLASS_PHY1_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS));
			seq_printf(seq, "CLASS_PHY1_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V4_PKTS));
			seq_printf(seq, "CLASS_PHY1_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V6_PKTS));
			seq_printf(seq, "CLASS_PHY1_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY1_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY1_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_RX_PKTS));
			seq_printf(seq, "CLASS_PHY1_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS));
			seq_printf(seq, "CLASS_PHY1_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V4_PKTS));
			seq_printf(seq, "CLASS_PHY1_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_V6_PKTS));
			seq_printf(seq, "CLASS_PHY1_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY1_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY2_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_RX_PKTS));
			seq_printf(seq, "CLASS_PHY2_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_L3_FAIL_PKTS));
			seq_printf(seq, "CLASS_PHY2_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_V4_PKTS));
			seq_printf(seq, "CLASS_PHY2_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_V6_PKTS));
			seq_printf(seq, "CLASS_PHY2_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_CHKSUM_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY2_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_TTL_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY3_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_RX_PKTS));
			seq_printf(seq, "CLASS_PHY3_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_L3_FAIL_PKTS));
			seq_printf(seq, "CLASS_PHY3_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_V4_PKTS));
			seq_printf(seq, "CLASS_PHY3_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_V6_PKTS));
			seq_printf(seq, "CLASS_PHY3_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_CHKSUM_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY3_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_TTL_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY1_ICMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_ICMP_PKTS));
			seq_printf(seq, "CLASS_PHY1_IGMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_IGMP_PKTS));
			seq_printf(seq, "CLASS_PHY1_TCP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_TCP_PKTS));
			seq_printf(seq, "CLASS_PHY1_UDP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY1_UDP_PKTS));
			seq_printf(seq, "CLASS_PHY2_ICMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_ICMP_PKTS));
			seq_printf(seq, "CLASS_PHY2_IGMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_IGMP_PKTS));
			seq_printf(seq, "CLASS_PHY2_TCP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_TCP_PKTS));
			seq_printf(seq, "CLASS_PHY2_UDP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY2_UDP_PKTS));
			seq_printf(seq, "CLASS_PHY3_ICMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_ICMP_PKTS));
			seq_printf(seq, "CLASS_PHY3_IGMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_IGMP_PKTS));
			seq_printf(seq, "CLASS_PHY3_TCP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_TCP_PKTS));
			seq_printf(seq, "CLASS_PHY3_UDP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY3_UDP_PKTS));
			seq_printf(seq, "CLASS_PHY4_ICMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_ICMP_PKTS));
			seq_printf(seq, "CLASS_PHY4_IGMP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_IGMP_PKTS));
			seq_printf(seq, "CLASS_PHY4_TCP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_TCP_PKTS));
			seq_printf(seq, "CLASS_PHY4_UDP_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_UDP_PKTS));
			seq_printf(seq, "CLASS_PHY4_RX_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_RX_PKTS));
			seq_printf(seq, "CLASS_PHY4_L3_FAIL_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_L3_FAIL_PKTS));
			seq_printf(seq, "CLASS_PHY4_V4_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_V4_PKTS));
			seq_printf(seq, "CLASS_PHY4_V6_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_V6_PKTS));
			seq_printf(seq, "CLASS_PHY4_CHKSUM_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_CHKSUM_ERR_PKTS));
			seq_printf(seq, "CLASS_PHY4_TTL_ERR_PKTS\t0x%x\n", hal_read32(base_va + CLASS_PHY4_TTL_ERR_PKTS));
		}

		if(verb_level >= 9U)
		{
			/*	Get version */
			reg = hal_read32(base_va + CLASS_VERSION);
			seq_printf(seq, "Revision\t0x%x\n", (reg >> 24U) & 0xffU);
			seq_printf(seq, "Version \t0x%x\n", (reg >> 16U) & 0xffU);
			seq_printf(seq, "ID      \t0x%x\n", reg & 0xffffU);
		}
			/*	CLASS_ROUTE_MULTI */
			reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
			seq_printf(seq, "CLASS_ROUTE_MULTI \t0x%x\n", reg);

			/*	CLASS_STATE */
			reg = hal_read32(base_va + CLASS_STATE);
			seq_printf(seq, "CLASS_STATE       \t0x%x\n", reg);

			reg = hal_read32(base_va + CLASS_QB_BUF_AVAIL);
			seq_printf(seq, "CLASS_QB_BUF_AVAIL\t0x%x\n", reg);

			reg = hal_read32(base_va + CLASS_RO_BUF_AVAIL);
			seq_printf(seq, "CLASS_RO_BUF_AVAIL\t0x%x\n", reg);

			reg = hal_read32(base_va + CLASS_PE0_DEBUG);
			seq_printf(seq, "PE0 PC\t0x%x\n", reg & 0xffffU);
			reg = hal_read32(base_va + CLASS_PE1_DEBUG);
			seq_printf(seq, "PE1 PC\t0x%x\n", reg & 0xffffU);
			reg = hal_read32(base_va + CLASS_PE2_DEBUG);
			seq_printf(seq, "PE2 PC\t0x%x\n", reg & 0xffffU);
			reg = hal_read32(base_va + CLASS_PE3_DEBUG);
			seq_printf(seq, "PE3 PC\t0x%x\n", reg & 0xffffU);
			reg = hal_read32(base_va + CLASS_PE4_DEBUG);
			seq_printf(seq, "PE4 PC\t0x%x\n", reg & 0xffffU);
			reg = hal_read32(base_va + CLASS_PE5_DEBUG);
			seq_printf(seq, "PE5 PC\t0x%x\n", reg & 0xffffU);
			reg = hal_read32(base_va + CLASS_PE6_DEBUG);
			seq_printf(seq, "PE6 PC\t0x%x\n", reg & 0xffffU);
			reg = hal_read32(base_va + CLASS_PE7_DEBUG);
			seq_printf(seq, "PE7 PC\t0x%x\n", reg & 0xffffU);

			if (TRUE == pfe_feature_mgr_is_available(PFE_HW_FEATURE_RUN_ON_G3))
			{
				seq_printf(seq, "Packets freed by HW: %u\n",
					hal_read32(base_va + CLASS_PE_CUM_DROP_COUNT_ADDR));
			}

			/*	Get info per PHY */
			seq_printf(seq, "[PHY1]\n");

			seq_printf(seq, "RX\t%10u TX\t%10u\nIPV4\t%10u IPV6\t%10u\n",
					hal_read32(base_va + CLASS_PHY1_RX_PKTS),
					hal_read32(base_va + CLASS_PHY1_TX_PKTS),
					hal_read32(base_va + CLASS_PHY1_V4_PKTS),
					hal_read32(base_va + CLASS_PHY1_V6_PKTS));

			seq_printf(seq, "ICMP\t%10u IGMP\t%10u TCP\t%10u UDP\t%10u\n",
					hal_read32(base_va + CLASS_PHY1_ICMP_PKTS),
					hal_read32(base_va + CLASS_PHY1_IGMP_PKTS),
					hal_read32(base_va + CLASS_PHY1_TCP_PKTS),
					hal_read32(base_va + CLASS_PHY1_UDP_PKTS));

			seq_printf(seq, "L3 Fail\t%10u CSUM Fail\t%10u TTL Fail\t%10u\n",
					hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS),
					hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS),
					hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));

			seq_printf(seq, "[PHY2]\n");

			seq_printf(seq, "RX\t%10u TX\t%10u\t IPV4\t%10u IPV6\t%10u\n",
					hal_read32(base_va + CLASS_PHY2_RX_PKTS),
					hal_read32(base_va + CLASS_PHY2_TX_PKTS),
					hal_read32(base_va + CLASS_PHY2_V4_PKTS),
					hal_read32(base_va + CLASS_PHY2_V6_PKTS));

			seq_printf(seq, "ICMP\t%10u IGMP\t%10u TCP\t%10u UDP\t%10u\n",
					hal_read32(base_va + CLASS_PHY2_ICMP_PKTS),
					hal_read32(base_va + CLASS_PHY2_IGMP_PKTS),
					hal_read32(base_va + CLASS_PHY2_TCP_PKTS),
					hal_read32(base_va + CLASS_PHY2_UDP_PKTS));

			seq_printf(seq, "L3 Fail\t%10u CSUM Fail\t%10u TTL Fail\t%10u\n",
					hal_read32(base_va + CLASS_PHY2_L3_FAIL_PKTS),
					hal_read32(base_va + CLASS_PHY2_CHKSUM_ERR_PKTS),
					hal_read32(base_va + CLASS_PHY2_TTL_ERR_PKTS));

			seq_printf(seq, "[PHY3]\n");

			seq_printf(seq, "RX\t%10u TX\t%10u\nIPV4\t%10u IPV6\t%10u\n",
					hal_read32(base_va + CLASS_PHY3_RX_PKTS),
					hal_read32(base_va + CLASS_PHY3_TX_PKTS),
					hal_read32(base_va + CLASS_PHY3_V4_PKTS),
					hal_read32(base_va + CLASS_PHY3_V6_PKTS));

			seq_printf(seq, "ICMP\t%10u IGMP\t%10u TCP\t%10u UDP\t%10u\n",
					hal_read32(base_va + CLASS_PHY3_ICMP_PKTS),
					hal_read32(base_va + CLASS_PHY3_IGMP_PKTS),
					hal_read32(base_va + CLASS_PHY3_TCP_PKTS),
					hal_read32(base_va + CLASS_PHY3_UDP_PKTS));

			seq_printf(seq, "L3 Fail\t%10u CSUM Fail\t%10u TTL Fail\t%10u\n",
					hal_read32(base_va + CLASS_PHY3_L3_FAIL_PKTS),
					hal_read32(base_va + CLASS_PHY3_CHKSUM_ERR_PKTS),
					hal_read32(base_va + CLASS_PHY3_TTL_ERR_PKTS));

			seq_printf(seq, "[PHY4]\n");

			seq_printf(seq, "RX\t%10u TX\t%10u\nIPV4\t%10u IPV6\t%10u\n",
					hal_read32(base_va + CLASS_PHY4_RX_PKTS),
					hal_read32(base_va + CLASS_PHY4_TX_PKTS),
					hal_read32(base_va + CLASS_PHY4_V4_PKTS),
					hal_read32(base_va + CLASS_PHY4_V6_PKTS));

			seq_printf(seq, "ICMP\t%10u IGMP\t%10u TCP\t%10u UDP\t%10u\n",
					hal_read32(base_va + CLASS_PHY4_ICMP_PKTS),
					hal_read32(base_va + CLASS_PHY4_IGMP_PKTS),
					hal_read32(base_va + CLASS_PHY4_TCP_PKTS),
					hal_read32(base_va + CLASS_PHY4_UDP_PKTS));

			seq_printf(seq, "L3 Fail\t%10u CSUM Fail\t%10u TTL Fail\t%10u\n",
					hal_read32(base_va + CLASS_PHY4_L3_FAIL_PKTS),
					hal_read32(base_va + CLASS_PHY4_CHKSUM_ERR_PKTS),
					hal_read32(base_va + CLASS_PHY4_TTL_ERR_PKTS));
	}

	return 0;
}

#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

/**
 * @brief		Enable HW lookup of routing table
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_rtable_lookup_enable(const addr_t base_va)
{
	uint32_t reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
	hal_write32(reg | PARSE_ROUTE_EN(TRUE), base_va + CLASS_ROUTE_MULTI);

	NXP_LOG_INFO("Enabling RTable lookup PARSE_ROUTE_EN\n");
}

/**
 * @brief		Enable HW lookup of routing table
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_rtable_lookup_disable(const addr_t base_va)
{
	uint32_t reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
	hal_write32(reg & (~PARSE_ROUTE_EN(TRUE)), base_va + CLASS_ROUTE_MULTI);

	NXP_LOG_INFO("Disabling RTable lookup PARSE_ROUTE_EN\n");
}

/**
 * @brief		Enable HW bridge lookup
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_bridge_lookup_enable(const addr_t base_va)
{
	uint32_t reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
	hal_write32(reg | PARSE_BRIDGE_EN(TRUE), base_va + CLASS_ROUTE_MULTI);

	NXP_LOG_INFO("Enabling HW bridge lookup PARSE_BRIDGE_EN\n");
}

/**
 * @brief		Disable HW bridge lookup
 * @param[in]	base_va Base address of CLASS register space (virtual)
 */
void pfe_class_cfg_bridge_lookup_disable(const addr_t base_va)
{
	uint32_t reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
	hal_write32(reg & (~PARSE_BRIDGE_EN(TRUE)), base_va + CLASS_ROUTE_MULTI);

	NXP_LOG_INFO("Disabling HW bridge lookup PARSE_BRIDGE_EN\n");
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

