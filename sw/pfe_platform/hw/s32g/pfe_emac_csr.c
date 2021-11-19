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
#include "pfe_cbus.h"
#include "pfe_emac_csr.h"

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

	return res;
}

/**
 * @brief		Convert EMAC mode to string
 * @details		Helper function for statistics to convert phy mode to string.
 * @param[in]	mode 	phy mode
 * @return		pointer to string
 */
static inline const char_t* phy_mode_to_str(uint32_t mode)
{
	/* Mode conversion table */
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
 * @brief		HW-specific initialization function
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	mode MII mode to be configured @see pfe_emac_mii_mode_t
 * @param[in]	speed Speed to be configured @see pfe_emac_speed_t
 * @param[in]	duplex Duplex type to be configured @see pfe_emac_duplex_t
 * @return		EOK if success, error code if invalid configuration is detected
 */
errno_t pfe_emac_cfg_init(addr_t base_va, pfe_emac_mii_mode_t mode,  pfe_emac_speed_t speed, pfe_emac_duplex_t duplex)
{
	uint32_t reg;

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

	hal_write32(0x1U, base_va + MTL_DPP_CONTROL);
	reg = hal_read32(base_va + MAC_Q0_TX_FLOW_CTRL);
	reg &= ~TX_FLOW_CONTROL_ENABLE(1U);
	hal_write32(reg, base_va + MAC_Q0_TX_FLOW_CTRL);
	hal_write32(0U, base_va + MAC_INTERRUPT_ENABLE);
	hal_write32(0U
			| ARP_OFFLOAD_ENABLE(0U)
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
			| JUMBO_PACKET_ENABLE(0U)
			| PORT_SELECT(0U)		/* To be set up by pfe_emac_cfg_set_speed() */
			| SPEED(0U)				/* To be set up by pfe_emac_cfg_set_speed() */
			| DUPLEX_MODE(1U)		/* To be set up by pfe_emac_cfg_set_duplex() */
			| LOOPBACK_MODE(0U)
			| CARRIER_SENSE_BEFORE_TX(0U)
			| DISABLE_RECEIVE_OWN(0)
			| DISABLE_CARRIER_SENSE_TX(0U)
			| DISABLE_RETRY(0U)
			| BACK_OFF_LIMIT(MIN_N_10)
			| DEFERRAL_CHECK(0U)
			| PREAMBLE_LENGTH_TX(PREAMBLE_7B)
			| TRANSMITTER_ENABLE(0U)
			| RECEIVER_ENABLE(0U)
			, base_va + MAC_CONFIGURATION);

	hal_write32((uint32_t)0U
			| FORWARD_ERROR_PACKETS(1U)
			, base_va + MTL_RXQ0_OPERATION_MODE);

	hal_write32(0U, base_va + MTL_TXQ0_OPERATION_MODE);
	hal_write32(GIANT_PACKET_SIZE_LIMIT(1522U), base_va + MAC_EXT_CONFIGURATION);

	hal_write32(0U, base_va + MAC_TIMESTAMP_CONTROL);
	hal_write32(0U, base_va + MAC_SUB_SECOND_INCREMENT);

	/*	Set speed */
	if (EOK != pfe_emac_cfg_set_speed(base_va, speed))
	{
		return EINVAL;
	}

	/*	Set MII mode */
	if (EOK != pfe_emac_cfg_set_mii_mode(base_va, mode))
	{
		return EINVAL;
	}

	/*	Set duplex */
	if (EOK != pfe_emac_cfg_set_duplex(base_va, duplex))
	{
		return EINVAL;
	}

	return EOK;
}

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
		return EOK;
	}

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
		return ETIME;
	}

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
		return ETIME;
	}

	return EOK;
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
		return ETIME;
	}

	return EOK;
}

/**
 * @brief			Get syste time
 * @param[in]		base_va Base address
 * @param[in,out]	sec Seconds
 * @param[in,out]	nsec NanoSeconds
 */
void pfe_emac_cfg_get_ts_time(addr_t base_va, uint32_t *sec, uint32_t *nsec)
{
	*sec = hal_read32(base_va + MAC_SYSTEM_TIME_SECONDS);
	*nsec = hal_read32(base_va + MAC_SYSTEM_TIME_NANOSECONDS);
}

/**
 * @brief		Set system time
 * @details		Current time will be overwritten with the desired value
 * @param[in]	base_va Base address
 * @param[in]	sec Seconds
 * @param[in]	nsec NanoSeconds
 */
errno_t pfe_emac_cfg_set_ts_time(addr_t base_va, uint32_t sec, uint32_t nsec)
{
	uint32_t regval, ii;

	if (nsec > 0x7fffffffU)
	{
		return EINVAL;
	}

	hal_write32(sec, base_va + MAC_STSU);
	hal_write32(nsec, base_va + MAC_STNSU);

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
		return ETIME;
	}

	return EOK;
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

	if (nsec_temp > 0x7fffffffU)
	{
		return EINVAL;
	}

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
			return EINVAL;
		}
	}

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
		return ETIME;
	}

	return EOK;
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
	if(ret == EOK)
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
	NXP_LOG_INFO("The PHY mode selection is done using a HW interface. See the 'phy_intf_sel' signal.\n");
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
			ret = EINVAL;
			break;
	}

	if(ret == EOK)
	{
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
		maxlen = 9026U;
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
		maxlen = 1526U;
	}

	if (je && !edvlp)
	{
		maxlen = 9022U;
	}

	if (!je && !s2kp && gpslce && !edvlp)
	{
		reg = hal_read32(base_va + MAC_EXT_CONFIGURATION);
		maxlen = reg & GIANT_PACKET_SIZE_LIMIT((uint32_t)-1);
		maxlen += 4U;
	}

	if (!je && !s2kp && !gpslce && !edvlp)
	{
		maxlen = 1522U;
	}

	if (len > maxlen)
	{
		return EINVAL;
	}

	return EOK;
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
	oal_time_udelay(10);
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
	uint32_t val = (hash & 0xfc000000U) >> 26U;
	uint8_t hash_table_idx = ((uint8_t)val & 0x40U) >> 6U;
	uint8_t pos = ((uint8_t)val & 0x1fU);

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

	if (reg == old_reg)
	{
		return;
	}

	hal_write32(reg, base_va + MAC_HASH_TABLE_REG(hash_table_idx));
	/*	Wait at least 4 clock cycles ((G)MII) */
	oal_time_udelay(10);
	hal_write32(reg, base_va + MAC_HASH_TABLE_REG(hash_table_idx));
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

	*en = reg & TX_FLOW_CONTROL_ENABLE(1);
}

void pfe_emac_cfg_get_rx_flow_control(addr_t base_va, bool_t* en)
{
	uint32_t reg = hal_read32(base_va + MAC_RX_FLOW_CTRL);

	*en = reg & RX_FLOW_CONTROL_ENABLE(1);
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
		return;
	}

	reg &= ~(TX_FLOW_CONTROL_ENABLE(1));
	reg |= TX_FLOW_CONTROL_ENABLE(en);

	reg |= TX_PAUSE_TIME(DEFAULT_PAUSE_QUANTA);
	reg |= TX_PAUSE_LOW_TRASHOLD(0x0);

	hal_write32(reg, base_va + MAC_Q0_TX_FLOW_CTRL);
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
	uint32_t timeout = 500U;
	reg = GMII_BUSY(1U)
			| CLAUSE45_ENABLE(0U)
			| GMII_OPERATION_CMD(GMII_READ)
			| SKIP_ADDRESS_PACKET(0U)
			/*	Select according to real CSR clock frequency. S32G: CSR_CLK = XBAR_CLK = 400MHz */
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
			return ETIME;
		}
		timeout--;
		oal_time_usleep(10);
	}
	while(GMII_BUSY(1) == (reg & GMII_BUSY(1)));

	/*	Get the data */
	reg = hal_read32(base_va + MAC_MDIO_DATA);
	*val = (uint16_t)GMII_DATA(reg);

	return EOK;
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
	uint32_t timeout = 500U;

	/* Set the register addresss to read */
	reg = (uint32_t)GMII_REGISTER_ADDRESS(ra);
	hal_write32(reg, base_va + MAC_MDIO_DATA);

	reg = GMII_BUSY(1U)
			| CLAUSE45_ENABLE(1U)
			| GMII_OPERATION_CMD(GMII_READ)
			| SKIP_ADDRESS_PACKET(0U)
			/*	Select according to real CSR clock frequency. S32G: CSR_CLK = XBAR_CLK = 400MHz */
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
			return ETIME;
		}

		oal_time_usleep(10);
	}

	/*	Get the data */
	reg = hal_read32(base_va + MAC_MDIO_DATA);
	*val = (uint16_t)GMII_DATA(reg);

	return EOK;
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
	uint32_t timeout = 500U;

	reg = (uint32_t)GMII_DATA(val);
	hal_write32(reg, base_va + MAC_MDIO_DATA);

	reg = GMII_BUSY(1U)
				| CLAUSE45_ENABLE(0U)
				| GMII_OPERATION_CMD(GMII_WRITE)
				| SKIP_ADDRESS_PACKET(0U)
				/*	Select according to real CSR clock frequency. S32G: CSR_CLK = XBAR_CLK = 400MHz */
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
			return ETIME;
		}
		oal_time_usleep(10);
	}

	return EOK;
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
	uint32_t timeout = 500U;

	reg = (uint32_t)(GMII_DATA(val) | GMII_REGISTER_ADDRESS(ra));
	hal_write32(reg, base_va + MAC_MDIO_DATA);

	reg = GMII_BUSY(1U)
				| CLAUSE45_ENABLE(1U)
				| GMII_OPERATION_CMD(GMII_WRITE)
				| SKIP_ADDRESS_PACKET(0U)
				/*	Select according to real CSR clock frequency. S32G: CSR_CLK = XBAR_CLK = 400MHz */
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
			return ETIME;
		}

		oal_time_usleep(10);
	}

	return EOK;
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
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_emac_cfg_get_text_stat(addr_t base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg;
	pfe_emac_speed_t speed;
	pfe_emac_duplex_t duplex;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */


	/*	Get version */
	reg = hal_read32(base_va + MAC_VERSION);
	len += oal_util_snprintf(buf + len, size - len, "SNPVER                    : 0x%x\n", reg & 0xffU);
	len += oal_util_snprintf(buf + len, size - len, "USERVER                   : 0x%x\n", (reg >> 8) & 0xffU);

	reg = hal_read32(base_va + RX_PACKETS_COUNT_GOOD_BAD);
	len += oal_util_snprintf(buf + len, size - len, "RX_PACKETS_COUNT_GOOD_BAD : 0x%x\n", reg);
	reg = hal_read32(base_va + TX_PACKET_COUNT_GOOD_BAD);
	len += oal_util_snprintf(buf + len, size - len, "TX_PACKET_COUNT_GOOD_BAD  : 0x%x\n", reg);

	(void)pfe_emac_cfg_get_link_config(base_va, &speed, &duplex);
	reg = hal_read32(base_va + MAC_CONFIGURATION);
	len += oal_util_snprintf(buf + len, size - len, "MAC_CONFIGURATION         : 0x%x [speed: %s]\n", reg, emac_speed_to_str(speed));

	reg = (hal_read32(base_va + MAC_HW_FEATURE0) >> 28U) & 0x07U;
	len += oal_util_snprintf(buf + len, size - len, "ACTPHYSEL(MAC_HW_FEATURE0): %s\n", phy_mode_to_str(reg));

	/* Error debugging */
	if(verb_level >= 8U)
	{
		reg = hal_read32(base_va + TX_UNDERFLOW_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_UNDERFLOW_ERROR_PACKETS        : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_SINGLE_COLLISION_GOOD_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_SINGLE_COLLISION_GOOD_PACKETS  : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_MULTIPLE_COLLISION_GOOD_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_MULTIPLE_COLLISION_GOOD_PACKETS: 0x%x\n", reg);
		reg = hal_read32(base_va + TX_DEFERRED_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_DEFERRED_PACKETS               : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_LATE_COLLISION_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_LATE_COLLISION_PACKETS         : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_EXCESSIVE_COLLISION_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_EXCESSIVE_COLLISION_PACKETS    : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_CARRIER_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_CARRIER_ERROR_PACKETS          : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_EXCESSIVE_DEFERRAL_ERROR);
		len += oal_util_snprintf(buf + len, size - len, "TX_EXCESSIVE_DEFERRAL_ERROR       : 0x%x\n", reg);

		reg = hal_read32(base_va + TX_OSIZE_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OSIZE_PACKETS_GOOD             : 0x%x\n", reg);

		reg = hal_read32(base_va + RX_CRC_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_CRC_ERROR_PACKETS              : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_ALIGNMENT_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_ALIGNMENT_ERROR_PACKETS        : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_RUNT_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_RUNT_ERROR_PACKETS             : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_JABBER_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_JABBER_ERROR_PACKETS           : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_LENGTH_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_LENGTH_ERROR_PACKETS           : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_OUT_OF_RANGE_TYPE_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_OUT_OF_RANGE_TYPE_PACKETS      : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_FIFO_OVERFLOW_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_FIFO_OVERFLOW_PACKETS          : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_RECEIVE_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_RECEIVE_ERROR_PACKETS          : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_RECEIVE_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_RECEIVE_ERROR_PACKETS          : 0x%x\n", reg);
	}

	/* Cast/vlan/flow control */
	if(verb_level >= 3U)
	{
		reg = hal_read32(base_va + TX_UNICAST_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_UNICAST_PACKETS_GOOD_BAD       : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_BROADCAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_BROADCAST_PACKETS_GOOD         : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_BROADCAST_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_BROADCAST_PACKETS_GOOD_BAD     : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_MULTICAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_MULTICAST_PACKETS_GOOD         : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_MULTICAST_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_MULTICAST_PACKETS_GOOD_BAD     : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_VLAN_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_VLAN_PACKETS_GOOD              : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_PAUSE_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_PAUSE_PACKETS                  : 0x%x\n", reg);
	}

	if(verb_level >= 4U)
	{
		reg = hal_read32(base_va + RX_UNICAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_UNICAST_PACKETS_GOOD           : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_BROADCAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_BROADCAST_PACKETS_GOOD         : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_MULTICAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_MULTICAST_PACKETS_GOOD         : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_VLAN_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_VLAN_PACKETS_GOOD_BAD          : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_PAUSE_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_PAUSE_PACKETS                  : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_CONTROL_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_CONTROL_PACKETS_GOOD           : 0x%x\n", reg);
	}

	if(verb_level >= 1U)
	{
		reg = hal_read32(base_va + TX_OCTET_COUNT_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OCTET_COUNT_GOOD                : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_OCTET_COUNT_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OCTET_COUNT_GOOD_BAD            : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_64OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_64OCTETS_PACKETS_GOOD_BAD       : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_65TO127OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_65TO127OCTETS_PACKETS_GOOD_BAD  : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_128TO255OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_128TO255OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_256TO511OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_256TO511OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32(base_va + TX_512TO1023OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_512TO1023OCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
		reg = hal_read32(base_va + TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
	}

	if(verb_level >= 5U)
	{
		reg = hal_read32(base_va + TX_OSIZE_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OSIZE_PACKETS_GOOD              : 0x%x\n", reg);
	}

	if(verb_level >= 2U)
	{
		reg = hal_read32(base_va + RX_OCTET_COUNT_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_OCTET_COUNT_GOOD                : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_OCTET_COUNT_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_OCTET_COUNT_GOOD_BAD            : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_64OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_64OCTETS_PACKETS_GOOD_BAD       : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_65TO127OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_65TO127OCTETS_PACKETS_GOOD_BAD  : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_128TO255OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_128TO255OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_256TO511OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_256TO511OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_512TO1023OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_512TO1023OCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
		reg = hal_read32(base_va + RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
	}

	if(verb_level >= 5U)
	{
		reg = hal_read32(base_va + RX_OVERSIZE_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_OSIZE_PACKETS_GOOD              : 0x%x\n", reg);
		reg = hal_read32(base_va + RX_UNDERSIZE_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_UNDERSIZE_PACKETS_GOOD          : 0x%x\n", reg);
	}

	return len;
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
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0xFFFFFFFFU;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return hal_read32(base_va + stat_id);
}
