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
 * @addtogroup  dxgr_PFE_CLASS
 * @{
 *
 * @file		pfe_class_csr.h
 * @brief		The CLASS module low-level API (s32g).
 * @details
 *
 */

#include "oal.h"
#include "hal.h"
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_class_csr.h"
#include <string.h>

/**
 * @brief		Initialize and configure the CLASS block
 * @param[in]	base_va Base address of CLASS register space (virtual)
 * @param[in]	cfg Pointer to the configuration structure
 */
void pfe_class_cfg_set_config(void *base_va, pfe_class_cfg_t *cfg)
{
	/*	LMEM buffer free address (BMU1). CLASS will write here when an LMEM buffer needs to be released. */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + CBUS_BMU1_BASE_ADDR + BMU_FREE_CTRL, base_va + CLASS_BMU1_BUF_FREE);

	/*	1st and 2nd buffer address (DMEM) on the re-order side */
	hal_write32(CLASS_PE0_RO_DM_ADDR0_VAL, base_va + CLASS_PE0_RO_DM_ADDR0);
	/*	3rd and 4th buffer address (DMEM) on the re-order side */
	hal_write32(CLASS_PE0_RO_DM_ADDR1_VAL, base_va + CLASS_PE0_RO_DM_ADDR1);

	/*	1st and 3nd buffer address (DMEM) on the queuing side */
	hal_write32(CLASS_PE0_QB_DM_ADDR0_VAL, base_va + CLASS_PE0_QB_DM_ADDR0);
	/*	3rd and 4th buffer address (DMEM) on the re-order side */
	hal_write32(CLASS_PE0_QB_DM_ADDR1_VAL, base_va + CLASS_PE0_QB_DM_ADDR1);

	/*	TMU input queue register address */
	hal_write32(PFE_CFG_CBUS_PHYS_BASE_ADDR + TMU_PHY_INQ_PKTPTR, base_va + CLASS_TM_INQ_ADDR);

	/*	Maximum buffer count for llm FIFO */
	hal_write32(0x18U, base_va + CLASS_MAX_BUF_CNT);

	/*	Threshold for llm FIFO */
	hal_write32(0x14U, base_va + CLASS_AFULL_THRES);

	/*	Class INQ FIFO is almost full threshold */
	hal_write32(0x3c0U, base_va + CLASS_INQ_AFULL_THRES);

#if 0
	/*	TSQ FIFO threshold */
	hal_write32(0x14U, base_va + CLASS_TSQ_FIFO_THRES);

	/*	TSQ max count */
	hal_write32(0x20U, base_va + CLASS_TSQ_MAX_CNT);
#endif /* 0 */

	/*	*/
	hal_write32(0x1U, base_va + CLASS_USE_TMU_INQ);

	/*	System clock ratio; TODO: Register should be read-only but the reference driver is writing it... */
	hal_write32(0x1U, base_va + CLASS_PE_SYS_CLK_RATIO);

	/*	Disable TCP/UDP/IPv4 checksum drop */
	hal_write32(0U, base_va + CLASS_L4_CHKSUM);

  #if 0
	hal_write32((PFE_CFG_DDR_HDR_SIZE << 16) | PFE_CFG_LMEM_HDR_SIZE, base_va + CLASS_HDR_SIZE);
	hal_write32(PFE_CFG_LMEM_BUF_SIZE, base_va + CLASS_LMEM_BUF_SIZE);
	hal_write32(CLASS_ROUTE_ENTRY_SIZE(cfg->route_entry_size) | CLASS_ROUTE_HASH_SIZE(cfg->route_hash_size), base_va + CLASS_ROUTE_HASH_ENTRY_SIZE);
	hal_write32(HIF_PKT_CLASS_EN| HIF_PKT_OFFSET(cfg->pkt_parse_offset), base_va + CLASS_HIF_PARSE);
  #else
    (void)cfg;
  #endif

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

  #if 0
	hal_write32(PFE_MMAP_DDR_PHYS_TO_PFE(cfg->route_table_base_pa), base_va + CLASS_ROUTE_TABLE_BASE);
  #endif
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

	/* First try NULL rtable, what means "disable hw route fetch" */
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
		/*	According to PFE reference manual, in this case the PFE HW requires
			that entry must be 128bytes long. */
		if (entry_size != 128U)
		{
			NXP_LOG_ERROR("FATAL: Route table entry length exceeds 128bytes\n");
		}
	}

	for (ii=0U; ii<(sizeof(rtable_len) * 8U); ii++)
	{
		if (0U != (rtable_len & (1 << ii)))
		{
			if (0U != (rtable_len & ~(1U << ii)))
			{
				NXP_LOG_WARNING("Routing table length is not a power of 2\n");
			}

			if ((ii < 6) || (ii > 20))
			{
				/*	RTL limitation, hash length will be set to 20bits */
				NXP_LOG_WARNING("Table length out of boundaries\n");
			}

			break;
		}
	}

	hal_write32((uint32_t)((addr_t)rtable_pa & 0xffffffffU), base_va + CLASS_ROUTE_TABLE_BASE);
	hal_write32(0U
				| ROUTE_HASH_SIZE(ii)
				| ROUTE_ENTRY_SIZE(entry_size)
				, base_va + CLASS_ROUTE_HASH_ENTRY_SIZE);

	/* enable hw route fetch */
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
	hal_write32(0U
			| USE_DEFAULT_VLANID(TRUE)
			| DEF_VLANID(vlan & 0xfffU)
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

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if(verb_level >= 9U)
	{
		/*	Get version */
		reg = hal_read32(base_va + CLASS_VERSION);
		len += oal_util_snprintf(buf + len, size - len, "Revision             : 0x%x\n", (reg >> 24) & 0xff);
		len += oal_util_snprintf(buf + len, size - len, "Version              : 0x%x\n", (reg >> 16) & 0xff);
		len += oal_util_snprintf(buf + len, size - len, "ID                   : 0x%x\n", reg & 0xffff);
	}
		/*	CLASS_ROUTE_MULTI */
		reg = hal_read32(base_va + CLASS_ROUTE_MULTI);
		len += oal_util_snprintf(buf + len, size - len, "CLASS_ROUTE_MULTI    : 0x%x\n", reg);

		/*	CLASS_STATE */
		reg = hal_read32(base_va + CLASS_STATE);
		len += oal_util_snprintf(buf + len, size - len, "CLASS_STATE          : 0x%x\n", reg);

		reg = hal_read32(base_va + CLASS_QB_BUF_AVAIL);
		len += oal_util_snprintf(buf + len, size - len, "CLASS_QB_BUF_AVAIL   : 0x%x\n", reg);

		reg = hal_read32(base_va + CLASS_RO_BUF_AVAIL);
		len += oal_util_snprintf(buf + len, size - len, "CLASS_RO_BUF_AVAIL   : 0x%x\n", reg);

		reg = hal_read32(base_va + CLASS_PE0_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE0 PC               : 0x%x\n", reg & 0xffff);
		reg = hal_read32(base_va + CLASS_PE1_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE1 PC               : 0x%x\n", reg & 0xffff);
		reg = hal_read32(base_va + CLASS_PE2_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE2 PC               : 0x%x\n", reg & 0xffff);
		reg = hal_read32(base_va + CLASS_PE3_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE3 PC               : 0x%x\n", reg & 0xffff);
		reg = hal_read32(base_va + CLASS_PE4_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE4 PC               : 0x%x\n", reg & 0xffff);
		reg = hal_read32(base_va + CLASS_PE5_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE5 PC               : 0x%x\n", reg & 0xffff);
		reg = hal_read32(base_va + CLASS_PE6_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE6 PC               : 0x%x\n", reg & 0xffff);
		reg = hal_read32(base_va + CLASS_PE7_DEBUG);
		len += oal_util_snprintf(buf + len, size - len, "PE7 PC               : 0x%x\n", reg & 0xffff);

		/*	Get info per PHY */
		len += oal_util_snprintf(buf + len, size - len, "[PHY1]\n");

		len += oal_util_snprintf(buf + len, size - len, "RX     : %10u, TX       : %10u, IPV4    : %10u, IPV6: %10u\n",
				hal_read32(base_va + CLASS_PHY1_RX_PKTS),
				hal_read32(base_va + CLASS_PHY1_TX_PKTS),
				hal_read32(base_va + CLASS_PHY1_V4_PKTS),
				hal_read32(base_va + CLASS_PHY1_V6_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "ICMP   : %10u, IGMP     : %10u, TCP     : %10u, UDP : %10u\n",
				hal_read32(base_va + CLASS_PHY1_ICMP_PKTS),
				hal_read32(base_va + CLASS_PHY1_IGMP_PKTS),
				hal_read32(base_va + CLASS_PHY1_TCP_PKTS),
				hal_read32(base_va + CLASS_PHY1_UDP_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "L3 Fail: %10u, CSUM Fail: %10u, TTL Fail: %10u\n",
				hal_read32(base_va + CLASS_PHY1_L3_FAIL_PKTS),
				hal_read32(base_va + CLASS_PHY1_CHKSUM_ERR_PKTS),
				hal_read32(base_va + CLASS_PHY1_TTL_ERR_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "[PHY2]\n");

		len += oal_util_snprintf(buf + len, size - len, "RX     : %10u, TX       : %10u, IPV4    : %10u, IPV6: %10u\n",
				hal_read32(base_va + CLASS_PHY2_RX_PKTS),
				hal_read32(base_va + CLASS_PHY2_TX_PKTS),
				hal_read32(base_va + CLASS_PHY2_V4_PKTS),
				hal_read32(base_va + CLASS_PHY2_V6_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "ICMP   : %10u, IGMP     : %10u, TCP     : %10u, UDP : %10u\n",
				hal_read32(base_va + CLASS_PHY2_ICMP_PKTS),
				hal_read32(base_va + CLASS_PHY2_IGMP_PKTS),
				hal_read32(base_va + CLASS_PHY2_TCP_PKTS),
				hal_read32(base_va + CLASS_PHY2_UDP_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "L3 Fail: %10u, CSUM Fail: %10u, TTL Fail: %10u\n",
				hal_read32(base_va + CLASS_PHY2_L3_FAIL_PKTS),
				hal_read32(base_va + CLASS_PHY2_CHKSUM_ERR_PKTS),
				hal_read32(base_va + CLASS_PHY2_TTL_ERR_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "[PHY3]\n");

		len += oal_util_snprintf(buf + len, size - len, "RX     : %10u, TX       : %10u, IPV4    : %10u, IPV6: %10u\n",
				hal_read32(base_va + CLASS_PHY3_RX_PKTS),
				hal_read32(base_va + CLASS_PHY3_TX_PKTS),
				hal_read32(base_va + CLASS_PHY3_V4_PKTS),
				hal_read32(base_va + CLASS_PHY3_V6_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "ICMP   : %10u, IGMP     : %10u, TCP     : %10u, UDP : %10u\n",
				hal_read32(base_va + CLASS_PHY3_ICMP_PKTS),
				hal_read32(base_va + CLASS_PHY3_IGMP_PKTS),
				hal_read32(base_va + CLASS_PHY3_TCP_PKTS),
				hal_read32(base_va + CLASS_PHY3_UDP_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "L3 Fail: %10u, CSUM Fail: %10u, TTL Fail: %10u\n",
				hal_read32(base_va + CLASS_PHY3_L3_FAIL_PKTS),
				hal_read32(base_va + CLASS_PHY3_CHKSUM_ERR_PKTS),
				hal_read32(base_va + CLASS_PHY3_TTL_ERR_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "[PHY4]\n");

		len += oal_util_snprintf(buf + len, size - len, "RX     : %10u, TX       : %10u, IPV4    : %10u, IPV6: %10u\n",
				hal_read32(base_va + CLASS_PHY4_RX_PKTS),
				hal_read32(base_va + CLASS_PHY4_TX_PKTS),
				hal_read32(base_va + CLASS_PHY4_V4_PKTS),
				hal_read32(base_va + CLASS_PHY4_V6_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "ICMP   : %10u, IGMP     : %10u, TCP     : %10u, UDP : %10u\n",
				hal_read32(base_va + CLASS_PHY4_ICMP_PKTS),
				hal_read32(base_va + CLASS_PHY4_IGMP_PKTS),
				hal_read32(base_va + CLASS_PHY4_TCP_PKTS),
				hal_read32(base_va + CLASS_PHY4_UDP_PKTS));

		len += oal_util_snprintf(buf + len, size - len, "L3 Fail: %10u, CSUM Fail: %10u, TTL Fail: %10u\n",
				hal_read32(base_va + CLASS_PHY4_L3_FAIL_PKTS),
				hal_read32(base_va + CLASS_PHY4_CHKSUM_ERR_PKTS),
				hal_read32(base_va + CLASS_PHY4_TTL_ERR_PKTS));

	return len;
}

/** @}*/
