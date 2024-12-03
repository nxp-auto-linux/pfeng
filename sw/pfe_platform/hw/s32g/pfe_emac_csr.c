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
#include "pfe_hm.h"
#include "pfe_cbus.h"
#include "pfe_emac_csr.h"
#include "pfe_feature_mgr.h"

#ifndef ETH_HLEN
#define ETH_HLEN		14U
#endif
#ifndef ETH_FCS_LEN
#define ETH_FCS_LEN		4U
#endif
#ifndef VLAN_HLEN
#define VLAN_HLEN		4U
#endif

#define PFE_EMAC_PKT_OVERHEAD	(PFE_MIN_DSA_OVERHEAD + ETH_HLEN + ETH_FCS_LEN)
#define PFE_EMAC_STD_MAXFRMSZ	(PFE_EMAC_STD_MTU + PFE_EMAC_PKT_OVERHEAD)
#define PFE_EMAC_JUMBO_MAXFRMSZ	(PFE_EMAC_JUMBO_MTU + PFE_EMAC_PKT_OVERHEAD)

#ifdef PFE_CFG_PFE_MASTER
#define PFE_CFG_MDIO_ACCESS_TIMEOUT 100U
/* Mode conversion table */
/* usage scope: phy_mode_to_str */
static const char_t * const phy_mode[] =
{
        "GMII_MII",
        "RGMII",
        "SGMII",
        "TBI",
        "RMII",
        "RTBI",
        "SMII",
        "RevMII",
        "INVALID",
};

static inline uint32_t reverse_bits_32(uint32_t u32Data);
static inline uint32_t crc32_reversed(const uint8_t *const data, const uint32_t len);

static inline const char_t* phy_mode_to_str(uint32_t mode);
static const char *emac_speed_to_str(pfe_emac_speed_t speed);

static inline uint32_t reverse_bits_32(uint32_t u32Data)
{
    uint8_t u8Index;
    uint32_t u32DataTemp = u32Data;
	uint32_t u32RevData = 0U;

	for(u8Index = 0U; u8Index < 32U; u8Index++)
	{
		u32RevData = (u32RevData << 1U) | (u32DataTemp & 0x1U);
		u32DataTemp >>= 1U;
	}

	return u32RevData;
}

static inline uint32_t crc32_reversed(const uint8_t *const data, const uint32_t len)
{
	const uint32_t poly = 0xEDB88320U;
	uint32_t res = 0xffffffffU;
	uint32_t ii, jj;

	for (ii=0U; ii<len; ii++)
	{
		res ^= (uint32_t)data[ii];

		for (jj=0U; jj<8U; jj++)
		{
			if ((res & 0x1U) != 0U)
			{
				res = res >> 1U;
				res = (uint32_t)(res ^ poly);
			}
			else
			{
				res = res >> 1U;
			}
		}
	}

	return reverse_bits_32(~res);
}

/**
 * @brief		Convert EMAC mode to string
 * @details		Helper function for statistics to convert phy mode to string.
 * @param[in]	mode 	phy mode
 * @return		pointer to string
 */
static inline const char_t* phy_mode_to_str(uint32_t mode)
{
	/* Initialize to invalid */
	uint32_t index  = ((uint32_t)(sizeof(phy_mode))/(uint32_t)(sizeof(phy_mode[0]))) - 1UL;

	if(((uint32_t)(sizeof(phy_mode))/(uint32_t)(sizeof(phy_mode[0]))) > mode)
	{
		index = mode;
	}

	return phy_mode[index];
}

/**
 * @brief		Convert EMAC speed to string
 * @details		Helper function for statistics to convert emac speed to string.
 * @param[in]	speed 	emac speed
 * @return		pointer to string
 */
static const char *emac_speed_to_str(pfe_emac_speed_t speed)
{
	const char *ret;

	switch (speed)
	{
		case EMAC_SPEED_10_MBPS:
			ret = "10 Mbps";
			break;
		case EMAC_SPEED_100_MBPS:
			ret = "100 Mbps";
			break;
		case EMAC_SPEED_1000_MBPS:
			ret = "1 Gbps";
			break;
		case EMAC_SPEED_2500_MBPS:
			ret = "2.5 Gbps";
			break;
		default:
			ret = "unknown";
			break;
	}
	return ret;
}

/**
 * @brief       HW-specific pre-initialization function related to ERR050221
 *              This function should be called before any READ of MAC registers
 * @param[in]   base_va Base address of MAC register space (virtual)
 */
void pfe_emac_cfg_pre_init(addr_t base_va)
{
    hal_write32(0x1U, base_va + MTL_DPP_CONTROL);
}

/**
 * @brief		HW-specific initialization function
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	mode MII mode to be configured @see pfe_emac_mii_mode_t
 * @param[in]	speed Speed to be configured @see pfe_emac_speed_t
 * @param[in]	duplex Duplex type to be configured @see pfe_emac_duplex_t
 * @return		EOK if success, error code if invalid configuration is detected
 */
errno_t pfe_emac_cfg_init(addr_t base_va, pfe_emac_mii_mode_t mode,
							pfe_emac_speed_t speed, pfe_emac_duplex_t duplex)
{
	uint32_t reg;
	errno_t ret;

	hal_write32(0U, base_va + MAC_CONFIGURATION);
	hal_write32(0x8000ffeeU, base_va + MAC_ADDRESS0_HIGH);
	hal_write32(0xddccbbaaU, base_va + MAC_ADDRESS0_LOW);
	hal_write32(0U
			| RECEIVE_ALL(0U)
			| DROP_NON_TCP_UDP(0U)
			| L3_L4_FILTER_ENABLE(0U)
			| VLAN_TAG_FILTER_ENABLE(0U)
			| HASH_OR_PERFECT_FILTER(1U)
			| SA_FILTER(0U)
			| SA_INVERSE_FILTER(0U)
			| PASS_CONTROL_PACKETS(FORWARD_ALL_EXCEPT_PAUSE)
			| DISABLE_BROADCAST_PACKETS(0U)
			| PASS_ALL_MULTICAST(0U)
			| DA_INVERSE_FILTER(0U)
			| HASH_MULTICAST(1U)
			| HASH_UNICAST(1U)
			| PROMISCUOUS_MODE(0U)
			, base_va + MAC_PACKET_FILTER);

	reg = hal_read32(base_va + MAC_Q0_TX_FLOW_CTRL);
	reg &= ~TX_FLOW_CONTROL_ENABLE(1U);
	hal_write32(reg, base_va + MAC_Q0_TX_FLOW_CTRL);
	hal_write32(0U, base_va + MAC_INTERRUPT_ENABLE);
	hal_write32(0xffffffffU, base_va + MMC_RX_INTERRUPT_MASK);
	hal_write32(0xffffffffU, base_va + MMC_TX_INTERRUPT_MASK);
	hal_write32(0xffffffffU, base_va + MMC_IPC_RX_INTERRUPT_MASK);

	/* Enable ECC, timeout and parity chcecking */
	hal_write32(0U
			| ECC_TX(1U)
			| ECC_RX(1U)
			| ECC_EST(1U)
			| ECC_RXP(1U)
			| ECC_TSO(1U)
			, base_va + MTL_ECC_CONTROL);
	reg = hal_read32(base_va + MAC_FSM_ACT_TIMER);
	hal_write32(reg
			| LARGE_MODE_TIMEOUT(0x2U)
			| NORMAL_MODE_TIMEOUT(0x2U)
			/*	Select according to real CSR clock frequency. S32G: CSR_CLK = 300MHz => 300 ticks */
			| 0x12CUL
			, base_va + MAC_FSM_ACT_TIMER);
	hal_write32(0U
			| DATA_PARITY_PROTECTION(1U)
			| SLAVE_PARITY_CHECK(1U)
			, base_va + MTL_DPP_CONTROL);
	hal_write32(0U
			| FSM_PARITY_ENABLE(1U)
			| FSM_TIMEOUT_ENABLE(1U)
			, base_va + MAC_FSM_CONTROL);

	reg = 0U | ARP_OFFLOAD_ENABLE(0U)
                 | SA_INSERT_REPLACE_CONTROL(CTRL_BY_SIGNALS)
                 | CHECKSUM_OFFLOAD(1U)
                 | INTER_PACKET_GAP(0U)
                 | GIANT_PACKET_LIMIT_CONTROL(1U)
                 | SUPPORT_2K_PACKETS(0U)
                 | CRC_STRIPPING_FOR_TYPE(1U)
                 | AUTO_PAD_OR_CRC_STRIPPING(1U)
                 | WATCHDOG_DISABLE(1U)
                 | PACKET_BURST_ENABLE(0U)
                 | JABBER_DISABLE(1U)
                 | PORT_SELECT(0U)               /* To be set up by pfe_emac_cfg_set_speed() */
                 | SPEED(0U)                             /* To be set up by pfe_emac_cfg_set_speed() */
                 | DUPLEX_MODE(1U)               /* To be set up by pfe_emac_cfg_set_duplex() */
                 | LOOPBACK_MODE(0U)
                 | CARRIER_SENSE_BEFORE_TX(0U)
                 | DISABLE_RECEIVE_OWN(0)
                 | DISABLE_CARRIER_SENSE_TX(0U)
                 | DISABLE_RETRY(0U)
                 | BACK_OFF_LIMIT(MIN_N_10)
                 | DEFERRAL_CHECK(0U)
                 | PREAMBLE_LENGTH_TX(PREAMBLE_7B)
                 | TRANSMITTER_ENABLE(0U)
                 | RECEIVER_ENABLE(0U);

	if (TRUE == pfe_feature_mgr_is_available("jumbo_frames"))
	{
		reg |= JUMBO_PACKET_ENABLE(1U);
	}
	else
	{
		reg |= JUMBO_PACKET_ENABLE(0U);
	}

	hal_write32(reg, base_va + MAC_CONFIGURATION);

	hal_write32((uint32_t)0U
			| FORWARD_ERROR_PACKETS(1U)
			, base_va + MTL_RXQ0_OPERATION_MODE);

	hal_write32(0U, base_va + MTL_TXQ0_OPERATION_MODE);
	if (TRUE == pfe_feature_mgr_is_available("jumbo_frames"))
	{
		hal_write32(GIANT_PACKET_SIZE_LIMIT(PFE_EMAC_JUMBO_MAXFRMSZ), base_va + MAC_EXT_CONFIGURATION);
	}
	else
	{
		hal_write32(GIANT_PACKET_SIZE_LIMIT(PFE_EMAC_STD_MAXFRMSZ), base_va + MAC_EXT_CONFIGURATION);
	}

	hal_write32(0U, base_va + MAC_TIMESTAMP_CONTROL);
	hal_write32(0U, base_va + MAC_SUB_SECOND_INCREMENT);

	/*	Set speed */
	if (EOK != pfe_emac_cfg_set_speed(base_va, speed))
	{
		ret = EINVAL;
	}
	else
	{
		/*	Set MII mode */
		if (EOK != pfe_emac_cfg_set_mii_mode(base_va, mode))
		{
			ret = EINVAL;
		}
		else
		{
			/*	Set duplex */
			if (EOK != pfe_emac_cfg_set_duplex(base_va, duplex))
			{
				ret = EINVAL;
			}
			else
			{
				ret = EOK;
			}
		}
	}

	return ret;
}

/**
 * @brief		Get EMAC instance index
 * @param[in]	emac_base The EMAC base address
 * @param[in]	cbus_base The PFE CBUS base address
 * @return		Index (0, 1, 2, ..) or 255 if failed
 */
uint8_t pfe_emac_cfg_get_index(addr_t emac_base, addr_t cbus_base)
{
	uint8_t idx;

	switch ((addr_t)emac_base - (addr_t)cbus_base)
	{
		case CBUS_EMAC1_BASE_ADDR:
		{
			idx = 0U;
			break;
		}

		case CBUS_EMAC2_BASE_ADDR:
		{
			idx = 1U;
			break;
		}

		case CBUS_EMAC3_BASE_ADDR:
		{
			idx = 2U;
			break;
		}

		default:
		{
			idx = 255U;
			break;
		}
	}

	return idx;
}

void pfe_emac_cfg_tx_disable(addr_t base_va)
{
	hal_write32(0U, base_va + MAC_TIMESTAMP_CONTROL);
}

/**
 * @brief		Set MAC duplex
 * @param[in]	base_va Base address to be written
 * @param[in]	duplex Duplex type to be configured @see pfe_emac_duplex_t
 * @return		EOK if success, error code when invalid configuration is requested
 */
errno_t pfe_emac_cfg_set_duplex(addr_t base_va, pfe_emac_duplex_t duplex)
{
	uint32_t reg = hal_read32(base_va + MAC_CONFIGURATION) & ~(DUPLEX_MODE(1U));
	errno_t ret = EOK;

	switch (duplex)
	{
		case EMAC_DUPLEX_HALF:
		{
			reg |= DUPLEX_MODE(0U);
			break;
		}

		case EMAC_DUPLEX_FULL:
		{
			reg |= DUPLEX_MODE(1U);
			break;
		}

		default:
			ret = EINVAL;
			break;
	}
	if(EOK == ret)
	{
		hal_write32(reg, base_va + MAC_CONFIGURATION);
	}

	return ret;
}

/**
 * @brief		Set MAC MII mode
 * @param[in]	base_va Base address to be written
 * @param[in]	mode MII mode to be configured @see pfe_emac_mii_mode_t
 * @return		EOK if success, error code when invalid configuration is requested
 */
errno_t pfe_emac_cfg_set_mii_mode(addr_t base_va, pfe_emac_mii_mode_t mode)
{
	/*
		 The PHY mode selection is done using a HW interface. See the "phy_intf_sel" signal.
	*/
	(void)base_va;
	(void)mode;

	return EOK;
}

/**
 * @brief		Set MAC speed
 * @param[in]	base_va Base address to be written
 * @param[in]	speed Speed to be configured @see pfe_emac_speed_t
 * @return		EOK if success, error code when invalid configuration is requested
 */
errno_t pfe_emac_cfg_set_speed(addr_t base_va, pfe_emac_speed_t speed)
{
	uint32_t reg = hal_read32(base_va + MAC_CONFIGURATION) & ~(PORT_SELECT(1U) | SPEED(1U));
	errno_t ret = EOK;

	switch (speed)
	{
		case EMAC_SPEED_10_MBPS:
		{
			reg |= PORT_SELECT(1U);
			reg |= SPEED(0U);
			break;
		}

		case EMAC_SPEED_100_MBPS:
		{
			reg |= PORT_SELECT(1U);
			reg |= SPEED(1U);
			break;
		}

		case EMAC_SPEED_1000_MBPS:
		{
			reg |= PORT_SELECT(0);
			reg |= SPEED(0);
			break;
		}

		case EMAC_SPEED_2500_MBPS:
		{
			reg |= PORT_SELECT(0);
			reg |= SPEED(1);
			break;
		}

		default:
		{
			ret = EINVAL;
			break;
		}
	}

	if (EOK == ret)
	{
		/*	Configure speed in EMAC registers */
		hal_write32(reg, base_va + MAC_CONFIGURATION);
	}

	return ret;
}

errno_t pfe_emac_cfg_get_link_config(addr_t base_va, pfe_emac_speed_t *speed, pfe_emac_duplex_t *duplex)
{
	uint32_t reg = hal_read32(base_va + MAC_CONFIGURATION);

	/* speed */
	switch (GET_LINE_SPEED(reg))
	{
		case 0x01U:
			*speed = EMAC_SPEED_2500_MBPS;
			break;
		case 0x02U:
			*speed = EMAC_SPEED_10_MBPS;
			break;
		case 0x03U:
			*speed = EMAC_SPEED_100_MBPS;
			break;
		case 0x0U:
		default:
			*speed = EMAC_SPEED_1000_MBPS;
			break;
	}

	/* duplex */
	*duplex = (1U == GET_DUPLEX_MODE(reg)) ? EMAC_DUPLEX_FULL : EMAC_DUPLEX_HALF;

	return EOK;
}

/**
 * @brief		Get MAC configured link parameters
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[out]	clock_speed Currently configured link speed
 * @param[out]	duplex Currently configured Duplex type @see pfe_emac_duplex_t
 * @param[out]	link Current link state
 * @return		EOK if success
 */
errno_t pfe_emac_cfg_get_link_status(addr_t base_va, pfe_emac_link_speed_t *link_speed, pfe_emac_duplex_t *duplex, bool_t *link)
{
	uint32_t reg = hal_read32(base_va + MAC_PHYIF_CONTROL_STATUS);

	/* speed */
	switch (LNKSPEED(reg))
	{
		case 0x01U:
			*link_speed = EMAC_LINK_SPEED_25_MHZ;
			break;
		case 0x02U:
			*link_speed = EMAC_LINK_SPEED_125_MHZ;
			break;
		case 0x03U:
			*link_speed = EMAC_LINK_SPEED_INVALID;
			break;
		case 0x0U:
		default:
			*link_speed = EMAC_LINK_SPEED_2_5_MHZ;
			break;
	}

	/* duplex */
	*duplex = (1U == LNKMOD(reg)) ? EMAC_DUPLEX_FULL : EMAC_DUPLEX_HALF;

	/* link */
	*link = LNKSTS(reg) == 1U;

	return EOK;
}

/**
 * @brief		Set maximum frame length
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	len The new maximum frame length
 * @return		EOK if success, error code if invalid value is requested
 */
errno_t pfe_emac_cfg_set_max_frame_length(addr_t base_va, uint32_t len)
{
	uint32_t reg, maxlen = 0U;
	bool_t je, s2kp, gpslce, edvlp;
	errno_t ret;

	/*
		In this case the function just performs check whether the requested length
		is supported by the current MAC configuration. When change is needed then
		particular parameters (JE, S2KP, GPSLCE, DVLP, and GPSL must be changed).
	*/

	reg = hal_read32(base_va + MAC_CONFIGURATION);
	je = !!(reg & JUMBO_PACKET_ENABLE(1U));
	s2kp = !!(reg & SUPPORT_2K_PACKETS(1U));
	gpslce = !!(reg & GIANT_PACKET_LIMIT_CONTROL(1U));

	reg = hal_read32(base_va + MAC_VLAN_TAG_CTRL);
	edvlp = !!(reg & ENABLE_DOUBLE_VLAN(1U));

	if (je && edvlp)
	{
		maxlen = PFE_EMAC_JUMBO_MAXFRMSZ + VLAN_HLEN;
	}

	if (!je && s2kp)
	{
		maxlen = 2000U;
	}

	if (!je && !s2kp && gpslce && edvlp)
	{
		reg = hal_read32(base_va + MAC_EXT_CONFIGURATION);
		maxlen = reg & GIANT_PACKET_SIZE_LIMIT((uint32_t)-1);
		maxlen += 8U;
	}

	if (!je && !s2kp && !gpslce && edvlp)
	{
		maxlen = PFE_EMAC_STD_MAXFRMSZ + VLAN_HLEN;
	}

	if (je && !edvlp)
	{
		maxlen = PFE_EMAC_JUMBO_MAXFRMSZ;
	}

	if (!je && !s2kp && gpslce && !edvlp)
	{
		reg = hal_read32(base_va + MAC_EXT_CONFIGURATION);
		maxlen = reg & GIANT_PACKET_SIZE_LIMIT((uint32_t)-1);
		maxlen += VLAN_HLEN;
	}

	if (!je && !s2kp && !gpslce && !edvlp)
	{
		maxlen = PFE_EMAC_STD_MAXFRMSZ;
	}

	if (len > maxlen)
	{
		ret = EINVAL;
	}
	else
	{
		ret = EOK;
	}

	return ret;
}

/**
 * @brief		Write MAC address to a specific individual address slot
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	addr The MAC address to be written
 * @param[in]	slot Index of slot where the address shall be written
 * @note		Maximum number of slots is given by EMAC_CFG_INDIVIDUAL_ADDR_SLOTS_COUNT
 */
void pfe_emac_cfg_write_addr_slot(addr_t base_va, const pfe_mac_addr_t addr, uint8_t slot)
{
	uint32_t bottom = ((uint32_t)addr[3] << 24U) | ((uint32_t)addr[2] << 16U) | ((uint32_t)addr[1] << 8U) | ((uint32_t)addr[0] << 0U);
	uint32_t top = ((uint32_t)addr[5] << 8U) | ((uint32_t)addr[4] << 0U);

	/*	All-zeros MAC address is special case (invalid entry) */
	if ((0U != top) || (0U != bottom))
	{
		top |= 0x80000000U;
	}

	hal_write32(top, base_va + MAC_ADDRESS_HIGH(slot));
	hal_write32(bottom, base_va + MAC_ADDRESS_LOW(slot));
}

/**
 * @brief		Convert MAC address to its hash representation
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	addr The MAC address to compute the hash for
 * @retval		The hash value as represented/used by the HW
 */
uint32_t pfe_emac_cfg_get_hash(addr_t base_va, const pfe_mac_addr_t addr)
{
	(void)base_va;

	return crc32_reversed((uint8_t *)addr, 6U);
}

/**
 * @brief		Enable/Disable individual address group defined by 'hash'
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	hash The hash value
 * @param[in]	en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_hash_group(addr_t base_va, uint32_t hash, bool_t en)
{
	uint32_t reg, old_reg;
	uint32_t val;
	uint8_t hash_table_idx, pos;

	/*
	 * NOTE:
	 * The algorithm calculates value to write into Hash table
	 * (Refer to the register description of MAC_HASH_TABLE_REG in RM for more details)
	 *    - Step 1:  Compute the CRC value of the destination MAC address (see crc32_reversed())
	 *    - Step 2:  Reverse 32 bits of CRC result (see crc32_reversed())
	 *    - Step 3:  Select the appropriate register bit to set.
	 *
	 * In this function, it executes Step 3 in above algorithm.
	 * Currently, 64-bit Hash is used, so the upper 6 bits after passing through the CRC calculator are used to index the bit to set in the Hash table
	 * The MSB in these group represents the index of the register to be used
	 * The remaining 5 bits reveals information on the position to set in the corresponding register
	 */

	val = (hash & EMAC_CFG_ADDR_HASH_MASK) >> 26U;    /* only the EMAC_CFG_ADDR_HASH_MASK (6) bits of the CRC result are used */
	hash_table_idx = ((uint8_t)val & 0x20U) >> 5U;    /* MSB represents Hash table register index (0/1) */
	pos = ((uint8_t)val & 0x1fU);                     /* Remaining 5 bits illustrates the bit to set in corresponding register  */

	reg = hal_read32(base_va + MAC_HASH_TABLE_REG(hash_table_idx));
	old_reg = reg;

	if (en)
	{
		reg |= ((uint32_t)1U << pos);
	}
	else
	{
		reg &= ~((uint32_t)1U << pos);
	}

	if (reg != old_reg)
	{
		hal_write32(reg, base_va + MAC_HASH_TABLE_REG(hash_table_idx));
	}
}

/**
 * @brief		Enable/Disable loopback mode
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_loopback(addr_t base_va, bool_t en)
{
	uint32_t reg = hal_read32(base_va + MAC_CONFIGURATION) & ~(LOOPBACK_MODE(1));

	reg |= LOOPBACK_MODE(en);

	hal_write32(reg, base_va + MAC_CONFIGURATION);
}

/**
 * @brief		Enable/Disable promiscuous mode
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_promisc_mode(addr_t base_va, bool_t en)
{
	uint32_t reg = hal_read32(base_va + MAC_PACKET_FILTER) & ~(PROMISCUOUS_MODE(1));

	reg |= PROMISCUOUS_MODE(en);

	hal_write32(reg, base_va + MAC_PACKET_FILTER);
}

/**
 * @brief		Enable/Disable ALLMULTI mode
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_allmulti_mode(addr_t base_va, bool_t en)
{
	uint32_t reg = hal_read32(base_va + MAC_PACKET_FILTER) & ~(PASS_ALL_MULTICAST(1));

	reg |= PASS_ALL_MULTICAST(en);

	hal_write32(reg, base_va + MAC_PACKET_FILTER);
}

/**
 * @brief		Enable/Disable broadcast reception
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_broadcast(addr_t base_va, bool_t en)
{
	uint32_t reg = hal_read32(base_va + MAC_PACKET_FILTER) & ~(DISABLE_BROADCAST_PACKETS(1));

	reg |= DISABLE_BROADCAST_PACKETS(!en);

	hal_write32(reg, base_va + MAC_PACKET_FILTER);
}

/**
 * @brief		Enable/Disable the Ethernet controller
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_enable(addr_t base_va, bool_t en)
{
	uint32_t reg = hal_read32(base_va + MAC_CONFIGURATION);

	reg &= ~(TRANSMITTER_ENABLE(1) | RECEIVER_ENABLE(1));
	reg |= TRANSMITTER_ENABLE(en) | RECEIVER_ENABLE(en);

	hal_write32(reg, base_va + MAC_CONFIGURATION);
}

void pfe_emac_cfg_get_tx_flow_control(addr_t base_va, bool_t* en)
{
	uint32_t reg = hal_read32(base_va + MAC_Q0_TX_FLOW_CTRL);

	*en = (0U == (reg & TX_FLOW_CONTROL_ENABLE(1))) ? FALSE : TRUE;
}

void pfe_emac_cfg_get_rx_flow_control(addr_t base_va, bool_t* en)
{
	uint32_t reg = hal_read32(base_va + MAC_RX_FLOW_CTRL);

	*en = (0U == (reg & RX_FLOW_CONTROL_ENABLE(1))) ? FALSE : TRUE;
}

/**
 * @brief		Enable/Disable the tx flow control
 * @details		Once enabled the MAC shall send PAUSE frames
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_tx_flow_control(addr_t base_va, bool_t en)
{
	uint32_t reg, ii=0U;

	do
	{
		reg = hal_read32(base_va + MAC_Q0_TX_FLOW_CTRL);
		oal_time_usleep(100U);
		ii++;
	} while ((reg & BUSY_OR_BACKPRESSURE_ACTIVE(1)) && (ii < 10U));

	if (ii >= 10U)
	{
		NXP_LOG_ERROR("Flow control is busy, exiting...\n");
	}
	else
	{
		reg &= ~(TX_FLOW_CONTROL_ENABLE(1));
		reg |= TX_FLOW_CONTROL_ENABLE(en);

		reg |= TX_PAUSE_TIME(DEFAULT_PAUSE_QUANTA);
		reg |= TX_PAUSE_LOW_TRASHOLD(0x0);

		hal_write32(reg, base_va + MAC_Q0_TX_FLOW_CTRL);
	}
}

/**
 * @brief               Enable/Disable the rx flow control
 * @details             Once enabled the MAC shall process PAUSE frames
 * @param[in]   base_va Base address of MAC register space (virtual)
 * @param               en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_rx_flow_control(addr_t base_va, bool_t en)
{
	int32_t reg =  hal_read32(base_va + MAC_RX_FLOW_CTRL);

	reg &= ~(RX_FLOW_CONTROL_ENABLE(1));
	reg |= RX_FLOW_CONTROL_ENABLE(en);

	hal_write32(reg, base_va + MAC_RX_FLOW_CTRL);
}

/**
 * @brief		Read value from the MDIO bus using Clause 22
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	pa Address of the PHY to read (5-bit)
 * @param[in]	ra Address of the register in the PHY to be read (5-bit)
 * @param[out]	val If success the the read value is written here (16 bit)
 * @retval		EOK Success
 */
errno_t pfe_emac_cfg_mdio_read22(addr_t base_va, uint8_t pa, uint8_t ra, uint16_t *val)
{
	uint32_t reg;
	uint32_t timeout = PFE_CFG_MDIO_ACCESS_TIMEOUT;
	errno_t ret = EOK;

	reg = GMII_BUSY(1U)
			| CLAUSE45_ENABLE(0U)
			| GMII_OPERATION_CMD(GMII_READ)
			| SKIP_ADDRESS_PACKET(0U)
			/*	Select according to real CSR clock frequency. S32G: CSR_CLK = XBAR_CLK = 300MHz */
			| CSR_CLOCK_RANGE(CSR_CLK_300_500_MHZ_MDC_CSR_DIV_204)
			| NUM_OF_TRAILING_CLOCKS(0U)
			| REG_DEV_ADDR(ra)
			| PHYS_LAYER_ADDR(pa)
			| BACK_TO_BACK(0U)
			| PREAMBLE_SUPPRESSION(0U);


	hal_write32(reg, base_va + MAC_MDIO_ADDRESS);
	do
	{
		reg = hal_read32(base_va + MAC_MDIO_ADDRESS);
		if (timeout == 0U)
		{
			ret = ETIME;
			break;
		}
		timeout--;
		oal_time_usleep(10);
	}
	while(GMII_BUSY(1) == (reg & GMII_BUSY(1)));

	if (EOK == ret)
	{
		/*	Get the data */
		reg = hal_read32(base_va + MAC_MDIO_DATA);
		*val = (uint16_t)GMII_DATA(reg);
	}

	return ret;
}

/**
* @brief		Read value from the MDIO bus using Clause 45
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	pa  Address of the PHY to read (5-bit)
 * @param[in]	dev Selects the device in the PHY to read (5-bit)
 * @param[in]	ra  Register address in the device to read  (16-bit)
 * @param[out]	val If success the the read value is written here (16-bit)
 * @retval		EOK Success
 */
errno_t pfe_emac_cfg_mdio_read45(addr_t base_va, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t *val)
{
	uint32_t reg;
	uint32_t timeout = PFE_CFG_MDIO_ACCESS_TIMEOUT;
	errno_t ret = EOK;

	/* Set the register addresss to read */
	reg = (uint32_t)GMII_REGISTER_ADDRESS(ra);
	hal_write32(reg, base_va + MAC_MDIO_DATA);

	reg = GMII_BUSY(1U)
			| CLAUSE45_ENABLE(1U)
			| GMII_OPERATION_CMD(GMII_READ)
			| SKIP_ADDRESS_PACKET(0U)
			/*	Select according to real CSR clock frequency. S32G: CSR_CLK = XBAR_CLK = 300MHz */
			| CSR_CLOCK_RANGE(CSR_CLK_300_500_MHZ_MDC_CSR_DIV_204)
			| NUM_OF_TRAILING_CLOCKS(0U)
			| REG_DEV_ADDR(dev)
			| PHYS_LAYER_ADDR(pa)
			| BACK_TO_BACK(0U)
			| PREAMBLE_SUPPRESSION(0U);

	hal_write32(reg, base_va + MAC_MDIO_ADDRESS);
	while(GMII_BUSY(1) == (hal_read32(base_va + MAC_MDIO_ADDRESS) & GMII_BUSY(1)))
	{
		if (timeout-- == 0U)
		{
			ret = ETIME;
			break;
		}

		oal_time_usleep(10);
	}

	if (EOK == ret)
	{
		/*	Get the data */
		reg = hal_read32(base_va + MAC_MDIO_DATA);
		*val = (uint16_t)GMII_DATA(reg);
	}

	return ret;
}

/**
 * @brief		Write value to the MDIO bus using Clause 22
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	pa Address of the PHY to be written (5-bit)
 * @param[in]	ra Address of the register in the PHY to be written (5-bit)
 * @param[in]	val Value to be written into the register (16 bit)
 * @retval		EOK Success
 */
errno_t pfe_emac_cfg_mdio_write22(addr_t base_va, uint8_t pa, uint8_t ra, uint16_t val)
{
	uint32_t reg;
	uint32_t timeout = PFE_CFG_MDIO_ACCESS_TIMEOUT;
	errno_t ret = EOK;

	reg = (uint32_t)GMII_DATA(val);
	hal_write32(reg, base_va + MAC_MDIO_DATA);

	reg = GMII_BUSY(1U)
				| CLAUSE45_ENABLE(0U)
				| GMII_OPERATION_CMD(GMII_WRITE)
				| SKIP_ADDRESS_PACKET(0U)
				/*	Select according to real CSR clock frequency. S32G: CSR_CLK = XBAR_CLK = 300MHz */
				| CSR_CLOCK_RANGE(CSR_CLK_300_500_MHZ_MDC_CSR_DIV_204)
				| NUM_OF_TRAILING_CLOCKS(0U)
				| REG_DEV_ADDR(ra)
				| PHYS_LAYER_ADDR(pa)
				| BACK_TO_BACK(0U)
				| PREAMBLE_SUPPRESSION(0U);

	hal_write32(reg, base_va + MAC_MDIO_ADDRESS);
	while(GMII_BUSY(1) == (hal_read32(base_va + MAC_MDIO_ADDRESS) & GMII_BUSY(1)))
	{
		if (timeout-- == 0U)
		{
			ret = ETIME;
			break;
		}
		oal_time_usleep(10);
	}

	return ret;
}

/**
* @brief		Write value to the MDIO bus using Clause 45
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	pa  Address of the PHY to be written (5-bit)
 * @param[in]   dev Device in the PHY to be written (5-bit)
 * @param[in]	ra  Address of the register in the device to be written (16-bit)
 * @param[in]	val Value to be written (16-bit)
 * @retval		EOK Success
 */
errno_t pfe_emac_cfg_mdio_write45(addr_t base_va, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t val)
{
	uint32_t reg;
	uint32_t timeout = PFE_CFG_MDIO_ACCESS_TIMEOUT;
	errno_t ret = EOK;

	reg = (uint32_t)(GMII_DATA(val) | GMII_REGISTER_ADDRESS(ra));
	hal_write32(reg, base_va + MAC_MDIO_DATA);

	reg = GMII_BUSY(1U)
				| CLAUSE45_ENABLE(1U)
				| GMII_OPERATION_CMD(GMII_WRITE)
				| SKIP_ADDRESS_PACKET(0U)
				/*	Select according to real CSR clock frequency. S32G: CSR_CLK = XBAR_CLK = 300MHz */
				| CSR_CLOCK_RANGE(CSR_CLK_300_500_MHZ_MDC_CSR_DIV_204)
				| NUM_OF_TRAILING_CLOCKS(0U)
				| REG_DEV_ADDR(dev)
				| PHYS_LAYER_ADDR(pa)
				| BACK_TO_BACK(0U)
				| PREAMBLE_SUPPRESSION(0U);

	hal_write32(reg, base_va + MAC_MDIO_ADDRESS);
	while(GMII_BUSY(1) == (hal_read32(base_va + MAC_MDIO_ADDRESS) & GMII_BUSY(1)))
	{
		if (timeout-- == 0U)
		{
			ret = ETIME;
			break;
		}

		oal_time_usleep(10);
	}

	return ret;
}

/**
 * @brief		Get number of transmitted packets
 * @param[in]	base_va Base address of EMAC register space (virtual)
 * @return		Number of transmitted packets
 */
uint32_t pfe_emac_cfg_get_tx_cnt(addr_t base_va)
{
	return hal_read32(base_va + TX_PACKET_COUNT_GOOD_BAD);
}

/**
 * @brief		Get number of received packets
 * @param[in]	base_va Base address of EMAC register space (virtual)
 * @return		Number of received packets
 */
uint32_t pfe_emac_cfg_get_rx_cnt(addr_t base_va)
{
	return hal_read32(base_va + RX_PACKETS_COUNT_GOOD_BAD);
}

/**
 * @brief		Get EMAC statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the EMAC block.
 * @param[in]	base_va 	Base address of EMAC register space (virtual)
 * @param[in]	seq 		Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_emac_cfg_get_text_stat(addr_t base_va, struct seq_file *seq, uint8_t verb_level)
{
	uint32_t reg;
	pfe_emac_speed_t speed;
	pfe_emac_duplex_t duplex;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Get version */
		reg = hal_read32(base_va + MAC_VERSION);
		seq_printf(seq,  "SNPVER                    : 0x%x\n", reg & 0xffU);
		seq_printf(seq,  "USERVER                   : 0x%x\n", (reg >> 8) & 0xffU);

		reg = hal_read32(base_va + RX_PACKETS_COUNT_GOOD_BAD);
		seq_printf(seq,  "RX_PACKETS_COUNT_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_PACKET_COUNT_GOOD_BAD);
		seq_printf(seq,  "TX_PACKET_COUNT_GOOD_BAD  : 0x%x\n", reg);

		(void)pfe_emac_cfg_get_link_config(base_va, &speed, &duplex);
		reg = hal_read32(base_va + MAC_CONFIGURATION);
		seq_printf(seq,  "MAC_CONFIGURATION         : 0x%x [speed: %s]\n", reg, emac_speed_to_str(speed));

		reg = (hal_read32(base_va + MAC_HW_FEATURE0) >> 28U) & 0x07U;
		seq_printf(seq,  "ACTPHYSEL(MAC_HW_FEATURE0): %s\n", phy_mode_to_str(reg));

		/* Error debugging */
		if(verb_level >= 8U)
		{
			reg = hal_read32(base_va + TX_UNDERFLOW_ERROR_PACKETS);
			seq_printf(seq,  "TX_UNDERFLOW_ERROR_PACKETS        : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_SINGLE_COLLISION_GOOD_PACKETS);
			seq_printf(seq,  "TX_SINGLE_COLLISION_GOOD_PACKETS  : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_MULTIPLE_COLLISION_GOOD_PACKETS);
			seq_printf(seq,  "TX_MULTIPLE_COLLISION_GOOD_PACKETS: 0x%x\n", reg);
			reg = hal_read32(base_va + TX_DEFERRED_PACKETS);
			seq_printf(seq,  "TX_DEFERRED_PACKETS               : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_LATE_COLLISION_PACKETS);
			seq_printf(seq,  "TX_LATE_COLLISION_PACKETS         : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_EXCESSIVE_COLLISION_PACKETS);
			seq_printf(seq,  "TX_EXCESSIVE_COLLISION_PACKETS    : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_CARRIER_ERROR_PACKETS);
			seq_printf(seq,  "TX_CARRIER_ERROR_PACKETS          : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_EXCESSIVE_DEFERRAL_ERROR);
			seq_printf(seq,  "TX_EXCESSIVE_DEFERRAL_ERROR       : 0x%x\n", reg);

			reg = hal_read32(base_va + TX_OSIZE_PACKETS_GOOD);
			seq_printf(seq,  "TX_OSIZE_PACKETS_GOOD             : 0x%x\n", reg);

			reg = hal_read32(base_va + RX_CRC_ERROR_PACKETS);
			seq_printf(seq,  "RX_CRC_ERROR_PACKETS              : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_ALIGNMENT_ERROR_PACKETS);
			seq_printf(seq,  "RX_ALIGNMENT_ERROR_PACKETS        : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_RUNT_ERROR_PACKETS);
			seq_printf(seq,  "RX_RUNT_ERROR_PACKETS             : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_JABBER_ERROR_PACKETS);
			seq_printf(seq,  "RX_JABBER_ERROR_PACKETS           : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_LENGTH_ERROR_PACKETS);
			seq_printf(seq,  "RX_LENGTH_ERROR_PACKETS           : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_OUT_OF_RANGE_TYPE_PACKETS);
			seq_printf(seq,  "RX_OUT_OF_RANGE_TYPE_PACKETS      : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_FIFO_OVERFLOW_PACKETS);
			seq_printf(seq,  "RX_FIFO_OVERFLOW_PACKETS          : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_RECEIVE_ERROR_PACKETS);
			seq_printf(seq,  "RX_RECEIVE_ERROR_PACKETS          : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_RECEIVE_ERROR_PACKETS);
			seq_printf(seq,  "RX_RECEIVE_ERROR_PACKETS          : 0x%x\n", reg);

			reg = hal_read32(base_va + MTL_ECC_ERR_CNTR_STATUS);
			seq_printf(seq,  "MTL_ECC_CORRECTABLE_ERRORS        : 0x%x\n", (reg & 0xffU));
			seq_printf(seq,  "MTL_ECC_UNCORRECTABLE_ERRORS      : 0x%x\n", ((reg >> 16U) & 0xfU));
		}

		/* Cast/vlan/flow control */
		if(verb_level >= 3U)
		{
			reg = hal_read32(base_va + TX_UNICAST_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_UNICAST_PACKETS_GOOD_BAD       : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_BROADCAST_PACKETS_GOOD);
			seq_printf(seq,  "TX_BROADCAST_PACKETS_GOOD         : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_BROADCAST_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_BROADCAST_PACKETS_GOOD_BAD     : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_MULTICAST_PACKETS_GOOD);
			seq_printf(seq,  "TX_MULTICAST_PACKETS_GOOD         : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_MULTICAST_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_MULTICAST_PACKETS_GOOD_BAD     : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_VLAN_PACKETS_GOOD);
			seq_printf(seq,  "TX_VLAN_PACKETS_GOOD              : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_PAUSE_PACKETS);
			seq_printf(seq,  "TX_PAUSE_PACKETS                  : 0x%x\n", reg);
		}

		if(verb_level >= 4U)
		{
			reg = hal_read32(base_va + RX_UNICAST_PACKETS_GOOD);
			seq_printf(seq,  "RX_UNICAST_PACKETS_GOOD           : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_BROADCAST_PACKETS_GOOD);
			seq_printf(seq,  "RX_BROADCAST_PACKETS_GOOD         : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_MULTICAST_PACKETS_GOOD);
			seq_printf(seq,  "RX_MULTICAST_PACKETS_GOOD         : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_VLAN_PACKETS_GOOD_BAD);
			seq_printf(seq,  "RX_VLAN_PACKETS_GOOD_BAD          : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_PAUSE_PACKETS);
			seq_printf(seq,  "RX_PAUSE_PACKETS                  : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_CONTROL_PACKETS_GOOD);
			seq_printf(seq,  "RX_CONTROL_PACKETS_GOOD           : 0x%x\n", reg);
		}

		if(verb_level >= 1U)
		{
			reg = hal_read32(base_va + TX_OCTET_COUNT_GOOD);
			seq_printf(seq,  "TX_OCTET_COUNT_GOOD                : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_OCTET_COUNT_GOOD_BAD);
			seq_printf(seq,  "TX_OCTET_COUNT_GOOD_BAD            : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_64OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_64OCTETS_PACKETS_GOOD_BAD       : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_65TO127OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_65TO127OCTETS_PACKETS_GOOD_BAD  : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_128TO255OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_128TO255OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_256TO511OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_256TO511OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
			reg = hal_read32(base_va + TX_512TO1023OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_512TO1023OCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
			reg = hal_read32(base_va + TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
		}

		if(verb_level >= 5U)
		{
			reg = hal_read32(base_va + TX_OSIZE_PACKETS_GOOD);
			seq_printf(seq,  "TX_OSIZE_PACKETS_GOOD              : 0x%x\n", reg);
		}

		if(verb_level >= 2U)
		{
			reg = hal_read32(base_va + RX_OCTET_COUNT_GOOD);
			seq_printf(seq,  "RX_OCTET_COUNT_GOOD                : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_OCTET_COUNT_GOOD_BAD);
			seq_printf(seq,  "RX_OCTET_COUNT_GOOD_BAD            : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_64OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "RX_64OCTETS_PACKETS_GOOD_BAD       : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_65TO127OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "RX_65TO127OCTETS_PACKETS_GOOD_BAD  : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_128TO255OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "RX_128TO255OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_256TO511OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "RX_256TO511OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_512TO1023OCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "RX_512TO1023OCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
			reg = hal_read32(base_va + RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD);
			seq_printf(seq,  "RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
		}

		if(verb_level >= 5U)
		{
			reg = hal_read32(base_va + RX_OVERSIZE_PACKETS_GOOD);
			seq_printf(seq,  "RX_OSIZE_PACKETS_GOOD              : 0x%x\n", reg);
			reg = hal_read32(base_va + RX_UNDERSIZE_PACKETS_GOOD);
			seq_printf(seq,  "RX_UNDERSIZE_PACKETS_GOOD          : 0x%x\n", reg);
		}
	}

	return 0;
}

/**
 * @brief		Get EMAC statistic in numeric form
 * @details		This is a HW-specific function providing single statistic
 * 				value from the EMAC block.
 * @param[in]	base_va 	Base address of EMAC register space (virtual)
 * @param[in]	stat_id		ID of required statistic (offset of register)
 * @return		Value of requested statistic
 */
uint32_t pfe_emac_cfg_get_stat_value(addr_t base_va, uint32_t stat_id)
{
	uint32_t stat_value;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		stat_value = 0xFFFFFFFFU;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		stat_value = hal_read32(base_va + stat_id);
	}
	return stat_value;
}

/**
 * @brief		Reports events corresponding to triggered interrupts to HM
 * @param[in]	id ID of the Peripheral that triggered the interrupt
 * @param[in]	events	List of events, ordered by interrupt flag position (0-31)
 * @param[in]	events_len	Amount of events defined
 * @param[in]	interrupts	Interrupts flags
 */
static void pfe_emac_cfg_report_hm_event(uint8_t id, const pfe_hm_evt_t events[], uint8_t events_len, uint32_t flags)
{
	static const pfe_hm_src_t hm_src[] =
	{
		HM_SRC_EMAC0,
		HM_SRC_EMAC1,
		HM_SRC_EMAC2,
	};

	uint8_t index = 0;
	pfe_hm_src_t src = HM_SRC_EMAC0;

	if (id < sizeof(hm_src)/sizeof(hm_src[0]))
	{
		src = hm_src[id];
	}
	else
	{
		NXP_LOG_ERROR("Argument out of range");
	}

	while ((0U != flags) && (index < events_len))
	{
		if ((0U != (flags & 0x1UL)) && (events[index] != HM_EVT_NONE))
		{
			if ((events[index] == HM_EVT_EMAC_ECC_RX_FIFO_CORRECTABLE) || (events[index] == HM_EVT_EMAC_ECC_TX_FIFO_CORRECTABLE))
			{
				pfe_hm_report_warning(src, events[index], "");
			}
			else
			{
				pfe_hm_report_error(src, events[index], "");
			}
		}
		index++;
		flags >>= 1U;
	}
}

/**
 * @brief		EMAC ISR
 * @details		Process triggered interrupts.
 * @param[in]	base_va EMAC register space base address
 * @param[in]	cbus_base The PFE CBUS base address
 * @return		EOK if interrupt has been handled, error code otherwise
 */
errno_t pfe_emac_cfg_isr(addr_t base_va, addr_t cbus_base)
{
	uint8_t instance_id = pfe_emac_cfg_get_index(base_va, cbus_base);

	static const pfe_hm_evt_t mtl_ecc_events[] =
	{
		HM_EVT_EMAC_ECC_TX_FIFO_CORRECTABLE,
		HM_EVT_EMAC_ECC_TX_FIFO_ADDRESS,
		HM_EVT_EMAC_ECC_TX_FIFO_UNCORRECTABLE,
		HM_EVT_NONE,
		HM_EVT_EMAC_ECC_RX_FIFO_CORRECTABLE,
		HM_EVT_EMAC_ECC_RX_FIFO_ADDRESS,
		HM_EVT_EMAC_ECC_RX_FIFO_UNCORRECTABLE,
	};

	static const pfe_hm_evt_t dpp_fsm_events[] =
	{
		HM_EVT_EMAC_APP_TX_PARITY,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_EMAC_MTL_PARITY,
		HM_EVT_NONE,
		HM_EVT_EMAC_APP_RX_PARITY,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_EMAC_FSM_TX_TIMEOUT,
		HM_EVT_EMAC_FSM_RX_TIMEOUT,
		HM_EVT_NONE,
		HM_EVT_EMAC_FSM_APP_TIMEOUT,
		HM_EVT_EMAC_FSM_PTP_TIMEOUT,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_EMAC_MASTER_TIMEOUT,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_NONE,
		HM_EVT_EMAC_FSM_PARITY,
	};

	uint32_t mtl_ecc_status = hal_read32(base_va + MTL_ECC_INTERRUPT_STATUS);
	uint32_t dpp_fsm_status = hal_read32(base_va + MAC_DPP_FSM_INTERRUPT_STATUS);

	pfe_emac_cfg_report_hm_event(
			instance_id,
			mtl_ecc_events,
			sizeof(mtl_ecc_events)/sizeof(mtl_ecc_events[0]),
			mtl_ecc_status);

	pfe_emac_cfg_report_hm_event(
			instance_id,
			dpp_fsm_events,
			sizeof(dpp_fsm_events)/sizeof(dpp_fsm_events[0]),
			dpp_fsm_status);

	/* Clear interrupts */
	hal_write32(mtl_ecc_status, base_va + MTL_ECC_INTERRUPT_STATUS);
	hal_write32(dpp_fsm_status, base_va + MAC_DPP_FSM_INTERRUPT_STATUS);

	return EOK;
}

#endif /* PFE_CFG_PFE_MASTER */

/**
 * @brief		Enable timestamping
 * @param[in]	base_va Base address
 * @param[in]	eclk TRUE means to use external clock reference (chain)
 * @param[in]	i_clk_hz Reference clock frequency
 * @param[in]	o_clk_hz Requested nominal output frequency
 * @param[in]	en TRUE means ENABLE, FALSE means DISABLE
 */
errno_t pfe_emac_cfg_enable_ts(addr_t base_va, bool_t eclk, uint32_t i_clk_hz, uint32_t o_clk_hz)
{
	uint64_t val = 1000000000000ULL;
	uint32_t ss = 0U, sns = 0U;
	uint32_t regval, ii;
	errno_t ret;

	hal_write32(0U
			| EXTERNAL_TIME(eclk)
			| SELECT_PTP_PACKETS(0x1U)
			| PTP_OVER_IPV4(1U)
			| PTP_OVER_IPV6(1U)
			| PTP_OVER_ETH(1U)
			| PTPV2(1U)
			| DIGITAL_ROLLOVER(1U)
			| FINE_UPDATE(1U)
			| ENABLE_TIMESTAMP(1U)
			| ENABLE_TIMESTAMP_FOR_All(1U), base_va + MAC_TIMESTAMP_CONTROL);
	regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);

	if (eclk == TRUE)
	{
		NXP_LOG_INFO("IEEE1588: Using external timestamp input\n");
		ret = EOK;
	}
	else
	{
		/*	Get output period [ns] */
		ss = (val / 1000ULL) / o_clk_hz;

		/*	Get sub-nanosecond part */
		sns = (val / (uint64_t)o_clk_hz) - (((val / 1000ULL) / (uint64_t)o_clk_hz) * 1000ULL);

		NXP_LOG_INFO("IEEE1588: Input Clock: %uHz, Output: %uHz, Accuracy: %u.%uns\n", (uint_t)i_clk_hz, (uint_t)o_clk_hz, (uint_t)ss, (uint_t)sns);

		if (0U == (regval & DIGITAL_ROLLOVER(1)))
		{
			/*	Binary roll-over, 0.465ns accuracy */
			ss = (ss * 1000U) / 465U;
		}

		sns = (sns * 256U) / 1000U;

		/*	Set 'increment' values */
		hal_write32(((uint32_t)ss << 16U) | ((uint32_t)sns << 8U), base_va + MAC_SUB_SECOND_INCREMENT);

		/*	Set initial 'addend' value */
		hal_write32(((uint64_t)o_clk_hz << 32U) / (uint64_t)i_clk_hz, base_va + MAC_TIMESTAMP_ADDEND);

		regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
		hal_write32(regval | UPDATE_ADDEND(1), base_va + MAC_TIMESTAMP_CONTROL);
		ii = 0U;
		do
		{
			regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
			oal_time_usleep(100U);
			if (((regval & UPDATE_ADDEND(1)) != 0U) && (ii < 10U))
			{
				++ii;
			}
			else
			{
				break;
			}
		} while (TRUE);

		if (ii >= 10U)
		{
			ret = ETIME;
		}
		else
		{
			ret = EOK;
		}

		if (EOK == ret)
		{
			/*	Set 'update' values */
			hal_write32(0U, base_va + MAC_STSU);
			hal_write32(0U, base_va + MAC_STNSU);

			regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
			regval |= INITIALIZE_TIMESTAMP(1);
			hal_write32(regval, base_va + MAC_TIMESTAMP_CONTROL);

			ii = 0U;
			do
			{
				regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
				oal_time_usleep(100U);
				if (((regval & INITIALIZE_TIMESTAMP(1)) != 0U) && (ii < 10U))
				{
					++ii;
				}
				else
				{
					break;
				}
			} while (TRUE);

			if (ii >= 10U)
			{
				ret = ETIME;
			}
		}
	}

	return ret;
}

/**
 * @brief	Disable timestamping
 */
void pfe_emac_cfg_disable_ts(addr_t base_va)
{
	hal_write32(0U, base_va + MAC_TIMESTAMP_CONTROL);
}

/**
 * @brief		Adjust timestamping clock frequency
 * @param[in]	base_va Base address
 * @param[in]	ppb Frequency change in [ppb]
 * @param[in]	sgn If TRUE then 'ppb' is positive, else it is negative
 */
errno_t pfe_emac_cfg_adjust_ts_freq(addr_t base_va, uint32_t i_clk_hz, uint32_t o_clk_hz, uint32_t ppb, bool_t sgn)
{
	uint32_t nil, delta, regval, ii;
	errno_t ret;

	/*	Nil drift addend: 1^32 / (o_clk_hz / i_clk_hz) */
	nil = (uint32_t)(((uint64_t)o_clk_hz << 32U) / (uint64_t)i_clk_hz);

	/*	delta = x * ppb * 0.000000001 */
	delta = (uint32_t)(((uint64_t)nil * (uint64_t)ppb) / 1000000000ULL);

	/*	Adjust the 'addend' */
	if (sgn)
	{
		if (((uint64_t)nil + (uint64_t)delta) > 0xffffffffULL)
		{
			NXP_LOG_WARNING("IEEE1588: Frequency adjustment out of positive range\n");
			regval = 0xffffffffU;
		}
		else
		{
			regval = nil + delta;
		}
	}
	else
	{
		if (delta > nil)
		{
			NXP_LOG_WARNING("IEEE1588: Frequency adjustment out of negative range\n");
			regval = 0U;
		}
		else
		{
			regval = nil - delta;
		}
	}

	/*	Update the 'addend' value */
	hal_write32(regval, base_va + MAC_TIMESTAMP_ADDEND);

	/*	Request update 'addend' value */
	regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
	hal_write32(regval | UPDATE_ADDEND(1), (addr_t)base_va + MAC_TIMESTAMP_CONTROL);

	/*	Wait for completion */
	ii = 0U;
	do
	{
		regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
		oal_time_usleep(100U);
		if (((regval & UPDATE_ADDEND(1)) != 0U) && (ii < 10U))
		{
			++ii;
		}
		else
		{
			break;
		}
	} while (TRUE);

	if (ii >= 10U)
	{
		ret = ETIME;
	}
	else
	{
		ret = EOK;
	}

	return ret;
}

/**
 * @brief			Get system time
 * @param[in]		base_va Base address
 * @param[in,out]	sec Seconds
 * @param[in,out]	nsec NanoSeconds
 * @param[in,out]	sec_hi Higher Word Seconds
 */
void pfe_emac_cfg_get_ts_time(addr_t base_va, uint32_t *sec, uint32_t *nsec, uint16_t *sec_hi)
{
	uint32_t sec_tmp;

	*sec = hal_read32(base_va + MAC_SYSTEM_TIME_SECONDS);
	do {
		sec_tmp = *sec;
		*nsec = hal_read32(base_va + MAC_SYSTEM_TIME_NANOSECONDS);
		*sec_hi = (uint16_t)hal_read32(base_va + MAC_STS_HIGHER_WORD);
		*sec = hal_read32(base_va + MAC_SYSTEM_TIME_SECONDS);
	} while (*sec != sec_tmp);
}

/**
 * @brief		Set system time
 * @details		Current time will be overwritten with the desired value
 * @param[in]	base_va Base address
 * @param[in]	sec Seconds
 * @param[in]	nsec NanoSeconds
 * @param[in]	sec_hi Higher Word Seconds
 */
errno_t pfe_emac_cfg_set_ts_time(addr_t base_va, uint32_t sec, uint32_t nsec, uint16_t sec_hi)
{
	uint32_t regval, ii;
	errno_t ret;

	if (nsec > 0x7fffffffU)
	{
		ret = EINVAL;
	}
	else
	{
		hal_write32(sec, base_va + MAC_STSU);
		hal_write32(nsec, base_va + MAC_STNSU);
		hal_write32(sec_hi, base_va + MAC_STS_HIGHER_WORD);

		/*	Initialize time */
		regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
		regval |= INITIALIZE_TIMESTAMP(1);
		hal_write32(regval, base_va + MAC_TIMESTAMP_CONTROL);

		/*	Wait for completion */
		ii = 0U;
		do
		{
			regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
			oal_time_usleep(100U);
			if (((regval & INITIALIZE_TIMESTAMP(1)) != 0U) && (ii < 10U))
			{
				++ii;
			}
			else
			{
				break;
			}
		} while (TRUE);

		if (ii >= 10U)
		{
			ret = ETIME;
		}
		else
		{
			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Adjust system time
 * @param[in]	base_va Base address
 * @param[in]	sec Seconds
 * @param[in]	nsec NanoSeconds
 * @param[in]	sgn Sing of the adjustment (TRUE - positive, FALSE - negative)
 */
errno_t pfe_emac_cfg_adjust_ts_time(addr_t base_va, uint32_t sec, uint32_t nsec, bool_t sgn)
{
	uint32_t regval, ii;
	uint32_t nsec_temp = nsec;
	int32_t sec_temp = sec;
	errno_t ret;

	if (nsec_temp > 0x7fffffffU)
	{
		ret = EINVAL;
	}
	else
	{
		ret = EOK;

		regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);

		if (!sgn)
		{
			if (0U != (regval & DIGITAL_ROLLOVER(1)))
			{
				nsec_temp = 1000000000U - nsec;
			}
			else
			{
				nsec_temp = (1UL << 31U) - nsec;
			}

			sec_temp = -sec_temp;
		}

		if (0U != (regval & DIGITAL_ROLLOVER(1)))
		{
			if (nsec_temp > 0x3b9ac9ffU)
			{
				ret = EINVAL;
			}
		}

		if (EOK == ret)
		{
			hal_write32(sec_temp, base_va + MAC_STSU);
			hal_write32(ADDSUB(!sgn) | nsec_temp, base_va + MAC_STNSU);

			/*	Trigger the update */
			regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
			regval |= UPDATE_TIMESTAMP(1);
			hal_write32(regval, base_va + MAC_TIMESTAMP_CONTROL);

			/*	Wait for completion */
			ii = 0U;
			do
			{
				regval = hal_read32(base_va + MAC_TIMESTAMP_CONTROL);
				oal_time_usleep(100U);
				if (((regval & UPDATE_TIMESTAMP(1)) != 0U) && (ii < 10U))
				{
					++ii;
				}
				else
				{
					break;
				}
			} while (TRUE);

			if (ii >= 10U)
			{
				ret = ETIME;
			}
		}
	}

	return ret;
}
