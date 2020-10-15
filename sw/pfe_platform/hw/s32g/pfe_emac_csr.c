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

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_emac_csr.h"

#if ((PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_FPGA_5_0_4) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14) \
	&& (PFE_CFG_IP_VERSION != PFE_CFG_IP_VERSION_NPU_7_14a))
#error Unsupported IP version
#endif /* PFE_CFG_IP_VERSION */

/* Mode conversion table */
static const char_t * phy_mode[] =
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

static inline uint32_t crc32_reversed(const uint8_t *const data, const uint32_t len)
{
	const uint32_t poly = 0xEDB88320U;
	uint32_t res = 0xffffffffU;
	uint32_t ii, jj;

	for (ii=0U; ii<len; ii++)
	{
		res ^= (uint32_t)data[ii];

		for (jj=0; jj<8; jj++)
		{
			if ((res & 0x1U) != 0U)
			{
				res = res >> 1;
				res = (uint32_t)(res ^ poly);
			}
			else
			{
				res = res >> 1;
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
	/* Initialize to invalid */
	uint8_t index  = (sizeof(phy_mode)/sizeof(phy_mode[0])) - 1;

	if((sizeof(phy_mode)/sizeof(phy_mode[0])) > mode)
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
	switch (speed)
	{
		default:
		case EMAC_SPEED_10_MBPS:
			return "10 Mbps";
		case EMAC_SPEED_100_MBPS:
			return "100 Mbps";
		case EMAC_SPEED_1000_MBPS:
			return "1 Gbps";
		case EMAC_SPEED_2500_MBPS:
			return "2.5 Gbps";
	}
}

/**
 * @brief		HW-specific initialization function
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	mode MII mode to be configured @see pfe_emac_mii_mode_t
 * @param[in]	speed Speed to be configured @see pfe_emac_speed_t
 * @param[in]	duplex Duplex type to be configured @see pfe_emac_duplex_t
 * @return		EOK if success, error code if invalid configuration is detected
 */
errno_t pfe_emac_cfg_init(void *base_va, pfe_emac_mii_mode_t mode,  pfe_emac_speed_t speed, pfe_emac_duplex_t duplex)
{
	uint32_t reg;

	hal_write32(0U, (addr_t)base_va + MAC_CONFIGURATION);
	hal_write32(0x8000ffeeU, (addr_t)base_va + MAC_ADDRESS0_HIGH);
	hal_write32(0xddccbbaaU, (addr_t)base_va + MAC_ADDRESS0_LOW);
	hal_write32(0U
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
			, (addr_t)base_va + MAC_PACKET_FILTER);

	reg = hal_read32((addr_t)base_va + MAC_Q0_TX_FLOW_CTRL);
	reg &= ~TX_FLOW_CONTROL_ENABLE(1U);
	hal_write32(reg, (addr_t)base_va + MAC_Q0_TX_FLOW_CTRL);
	hal_write32(0U, (addr_t)base_va + MAC_INTERRUPT_ENABLE);
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
			, (addr_t)base_va + MAC_CONFIGURATION);

	hal_write32(0U
			| FORWARD_ERROR_PACKETS(1U)
			, (addr_t)base_va + MTL_RXQ0_OPERATION_MODE);

	hal_write32(0U, (addr_t)base_va + MTL_TXQ0_OPERATION_MODE);
	hal_write32(GIANT_PACKET_SIZE_LIMIT(0x3000U), (addr_t)base_va + MAC_EXT_CONFIGURATION);
	hal_write32(0x1U, (addr_t)base_va + MTL_DPP_CONTROL);

	hal_write32(0U, (addr_t)base_va + MAC_TIMESTAMP_CONTROL);
	hal_write32(0U, (addr_t)base_va + MAC_SUB_SECOND_INCREMENT);

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
errno_t pfe_emac_cfg_enable_ts(void *base_va, bool_t eclk, uint32_t i_clk_hz, uint32_t o_clk_hz)
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
			| ENABLE_TIMESTAMP(1U), (addr_t)base_va + MAC_TIMESTAMP_CONTROL);
	regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);

	if (!eclk)
	{
		/*	Get output period [ns] */
		ss = (val / 1000ULL) / o_clk_hz;

		/*	Get sub-nanosecond part */
		sns = (val / (uint64_t)o_clk_hz) - (((val / 1000ULL) / (uint64_t)o_clk_hz) * 1000ULL);

		NXP_LOG_INFO("IEEE1588: Input Clock: %dHz, Output: %dHz, Accuracy: %d.%dns\n", i_clk_hz, o_clk_hz, ss, sns);

		if (0U == (regval & DIGITAL_ROLLOVER(1)))
		{
			/*	Binary roll-over, 0.465ns accuracy */
			ss = (ss * 1000U) / 456U;
		}

		sns = (sns * 256U) / 1000U;
	}
	else
	{
		NXP_LOG_INFO("IEEE1588: Using external timestamp input\n");
	}

	/*	Set 'increment' values */
	hal_write32(((uint8_t)ss << 16) | ((uint8_t)sns << 8), (addr_t)base_va + MAC_SUB_SECOND_INCREMENT);

	/*	Set initial 'addend' value */
	hal_write32(((uint64_t)o_clk_hz << 32) / (uint64_t)i_clk_hz, (addr_t)base_va + MAC_TIMESTAMP_ADDEND);

	regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
	hal_write32(regval | UPDATE_ADDEND(1), (addr_t)base_va + MAC_TIMESTAMP_CONTROL);
	ii = 0U;
	do
	{
		regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
		oal_time_usleep(100U);
	} while ((regval & UPDATE_ADDEND(1)) && (ii++ < 10U));

	if (ii >= 10U)
	{
		return ETIME;
	}

	/*	Set 'update' values */
	hal_write32(0U, (addr_t)base_va + MAC_STSU);
	hal_write32(0U, (addr_t)base_va + MAC_STNSU);

	regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
	regval |= INITIALIZE_TIMESTAMP(1);
	hal_write32(regval, (addr_t)base_va + MAC_TIMESTAMP_CONTROL);

	ii = 0U;
	do
	{
		regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
		oal_time_usleep(100U);
	} while ((regval & INITIALIZE_TIMESTAMP(1)) && (ii++ < 10U));

	if (ii >= 10U)
	{
		return ETIME;
	}

	return EOK;
}

/**
 * @brief	Disable timestamping
 */
void pfe_emac_cfg_disable_ts(void *base_va)
{
	hal_write32(0U, (addr_t)base_va + MAC_TIMESTAMP_CONTROL);
}

/**
 * @brief		Adjust timestamping clock frequency
 * @param[in]	base_va Base address
 * @param[in]	ppb Frequency change in [ppb]
 * @param[in]	sgn If TRUE then 'ppb' is positive, else it is negative
 */
errno_t pfe_emac_cfg_adjust_ts_freq(void *base_va, uint32_t i_clk_hz, uint32_t o_clk_hz, uint32_t ppb, bool_t sgn)
{
	uint32_t nil, delta, regval, ii;

	/*	Nil drift addend: 1^32 / (o_clk_hz / i_clk_hz) */
	nil = ((uint64_t)o_clk_hz << 32) / (uint64_t)i_clk_hz;

	/*	delta = x * ppb * 0.000000001 */
	delta = ((uint64_t)nil * (uint64_t)ppb) / 1000000000ULL;

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
	hal_write32(regval, (addr_t)base_va + MAC_TIMESTAMP_ADDEND);

	/*	Wait for completion */
	ii = 0U;
	do
	{
		regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
		oal_time_usleep(100U);
	} while ((regval & UPDATE_ADDEND(1)) && (ii++ < 10U));

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
void pfe_emac_cfg_get_ts_time(void *base_va, uint32_t *sec, uint32_t *nsec)
{
	*sec = hal_read32((addr_t)base_va + MAC_SYSTEM_TIME_SECONDS);
	*nsec = hal_read32((addr_t)base_va + MAC_SYSTEM_TIME_NANOSECONDS);
}

/**
 * @brief		Set system time
 * @details		Current time will be overwritten with the desired value
 * @param[in]	base_va Base address
 * @param[in]	sec Seconds
 * @param[in]	nsec NanoSeconds
 */
errno_t pfe_emac_cfg_set_ts_time(void *base_va, uint32_t sec, uint32_t nsec)
{
	uint32_t regval, ii;

	if (nsec > 0x7fffffffU)
	{
		return EINVAL;
	}

	hal_write32(sec, (addr_t)base_va + MAC_STSU);
	hal_write32(nsec, (addr_t)base_va + MAC_STNSU);

	/*	Initialize time */
	regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
	regval |= INITIALIZE_TIMESTAMP(1);
	hal_write32(regval, (addr_t)base_va + MAC_TIMESTAMP_CONTROL);

	/*	Wait for completion */
	ii = 0U;
	do
	{
		regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
		oal_time_usleep(100U);
	} while ((regval & INITIALIZE_TIMESTAMP(1)) && (ii++ < 10U));

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
errno_t pfe_emac_cfg_adjust_ts_time(void *base_va, uint32_t sec, uint32_t nsec, bool_t sgn)
{
	uint32_t regval, ii;

	if (nsec > 0x7fffffffU)
	{
		return EINVAL;
	}

	regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);

	if (!sgn)
	{
		if (0U != (regval & DIGITAL_ROLLOVER(1)))
		{
			nsec = 1000000000U - nsec;
		}
		else
		{
			nsec = (1U << 31) - nsec;
		}

		sec = -sec;
	}
	else
	{
		nsec |= 1U << 31;
	}

	if (0U != (regval & DIGITAL_ROLLOVER(1)))
	{
		if (nsec > 0x3b9ac9ffU)
		{
			return EINVAL;
		}
	}

	hal_write32(sec, (addr_t)base_va + MAC_STSU);
	hal_write32(ADDSUB(!sgn) | nsec, (addr_t)base_va + MAC_STNSU);

	/*	Trigger the update */
	regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
	regval |= UPDATE_TIMESTAMP(1);
	hal_write32(regval, (addr_t)base_va + MAC_TIMESTAMP_CONTROL);

	/*	Wait for completion */
	ii = 0U;
	do
	{
		regval = hal_read32((addr_t)base_va + MAC_TIMESTAMP_CONTROL);
		oal_time_usleep(100U);
	} while ((regval & UPDATE_TIMESTAMP(1)) && (ii++ < 10U));

	if (ii >= 10U)
	{
		return ETIME;
	}

	return EOK;
}

void pfe_emac_cfg_tx_disable(void *base_va)
{
	hal_write32(0U, (addr_t)base_va + MAC_TIMESTAMP_CONTROL);
}

/**
 * @brief		Set MAC duplex
 * @param[in]	base_va Base address to be written
 * @param[in]	duplex Duplex type to be configured @see pfe_emac_duplex_t
 * @return		EOK if success, error code when invalid configuration is requested
 */
errno_t pfe_emac_cfg_set_duplex(void *base_va, pfe_emac_duplex_t duplex)
{
	uint32_t reg = hal_read32((addr_t)base_va + MAC_CONFIGURATION) & ~(DUPLEX_MODE(1U));

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
			return EINVAL;
	}

	hal_write32(reg, (addr_t)base_va + MAC_CONFIGURATION);

	return EOK;
}

/**
 * @brief		Set MAC MII mode
 * @param[in]	base_va Base address to be written
 * @param[in]	mode MII mode to be configured @see pfe_emac_mii_mode_t
 * @return		EOK if success, error code when invalid configuration is requested
 */
errno_t pfe_emac_cfg_set_mii_mode(void *base_va, pfe_emac_mii_mode_t mode)
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
errno_t pfe_emac_cfg_set_speed(void *base_va, pfe_emac_speed_t speed)
{
	uint32_t reg = hal_read32((addr_t)base_va + MAC_CONFIGURATION) & ~(PORT_SELECT(1U) | SPEED(1U));

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
			return EINVAL;
	}

	hal_write32(reg, (addr_t)base_va + MAC_CONFIGURATION);

	return EOK;
}

errno_t pfe_emac_cfg_get_link_config(void *base_va, pfe_emac_speed_t *speed, pfe_emac_duplex_t *duplex)
{
	uint32_t reg = hal_read32((addr_t)base_va + MAC_CONFIGURATION);

	/* speed */
	switch (GET_LINE_SPEED(reg))
	{
		default:
		case 0x0U:
			*speed = EMAC_SPEED_1000_MBPS;
			break;
		case 0x01U:
			*speed = EMAC_SPEED_2500_MBPS;
			break;
		case 0x02U:
			*speed = EMAC_SPEED_10_MBPS;
			break;
		case 0x03U:
			*speed = EMAC_SPEED_100_MBPS;
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
errno_t pfe_emac_cfg_get_link_status(void *base_va, pfe_emac_link_speed_t *link_speed, pfe_emac_duplex_t *duplex, bool_t *link)
{
	uint32_t reg = hal_read32((addr_t)base_va + MAC_PHYIF_CONTROL_STATUS);

	/* speed */
	switch (LNKSPEED(reg))
	{
		default:
		case 0x0U:
			*link_speed = EMAC_LINK_SPEED_2_5_MHZ;
			break;
		case 0x01U:
			*link_speed = EMAC_LINK_SPEED_25_MHZ;
			break;
		case 0x02U:
			*link_speed = EMAC_LINK_SPEED_125_MHZ;
			break;
		case 0x03U:
			*link_speed = EMAC_LINK_SPEED_INVALID;
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
errno_t pfe_emac_cfg_set_max_frame_length(void *base_va, uint32_t len)
{
	uint32_t reg, maxlen = 0U;
	bool_t je, s2kp, gpslce, edvlp;

	/*
		In this case the function just performs check whether the requested length
	 	is supported by the current MAC configuration. When change is needed then
	 	particular parameters (JE, S2KP, GPSLCE, DVLP, and GPSL must be changed).
	*/

	reg = hal_read32((addr_t)base_va + MAC_CONFIGURATION);
	je = !!(reg & JUMBO_PACKET_ENABLE(1U));
	s2kp = !!(reg & SUPPORT_2K_PACKETS(1U));
	gpslce = !!(reg & GIANT_PACKET_LIMIT_CONTROL(1U));

	reg = hal_read32((addr_t)base_va + MAC_VLAN_TAG_CTRL);
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
		reg = hal_read32((addr_t)base_va + MAC_EXT_CONFIGURATION);
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
		reg = hal_read32((addr_t)base_va + MAC_EXT_CONFIGURATION);
		maxlen = reg & GIANT_PACKET_SIZE_LIMIT((uint32_t)-1);
		maxlen += 4U;
	}

	if (!je && !s2kp && !gpslce && !edvlp)
	{
		maxlen = 1522U;
	}

	if (0U == maxlen)
	{
		return EINVAL;
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
void pfe_emac_cfg_write_addr_slot(void *base_va, pfe_mac_addr_t addr, uint8_t slot)
{
	uint32_t bottom = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | (addr[0] << 0);
	uint32_t top = (addr[5] << 8) | (addr[4] << 0);

	/*	All-zeros MAC address is special case (invalid entry) */
	if ((0U != top) || (0U != bottom))
	{
		top |= 0x80000000U;
	}

	hal_write32(top, (addr_t)base_va + MAC_ADDRESS_HIGH(slot));
	hal_write32(bottom, (addr_t)base_va + MAC_ADDRESS_LOW(slot));
	oal_time_usleep(100);
	hal_write32(bottom, (addr_t)base_va + MAC_ADDRESS_LOW(slot));
}

/**
 * @brief		Convert MAC address to its hash representation
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	addr The MAC address to compute the hash for
 * @retval		The hash value as represented/used by the HW
 */
uint32_t pfe_emac_cfg_get_hash(void *base_va, pfe_mac_addr_t addr)
{
	(void)base_va;

	return crc32_reversed((uint8_t *)&addr, 6U);
}

/**
 * @brief		Enable/Disable individual address group defined by 'hash'
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	hash The hash value
 * @param[in]	en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_uni_group(void *base_va, int32_t hash, bool_t en)
{
	uint32_t reg;
	uint32_t val = (uint32_t)(hash & 0xfc000000U) >> 26;
	uint8_t hash_table_idx = (val & 0x40U) >> 6;
	uint8_t pos = (val & 0x1fU);

	reg = hal_read32((addr_t)base_va + MAC_HASH_TABLE_REG(hash_table_idx));

	if (en)
	{
		reg |= (uint32_t)(1U << pos);
	}
	else
	{
		reg &= (uint32_t)~(1U << pos);
	}

	hal_write32(reg, (addr_t)base_va + MAC_HASH_TABLE_REG(hash_table_idx));
	/*	Wait at least 4 clock cycles ((G)MII) */
	oal_time_usleep(100);
	hal_write32(reg, (addr_t)base_va + MAC_HASH_TABLE_REG(hash_table_idx));
}

/**
 * @brief		Enable/Disable multicast address group defined by 'hash'
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	hash The hash value
 * @param[in]	en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_multi_group(void *base_va, int32_t hash, bool_t en)
{
	pfe_emac_cfg_set_uni_group(base_va, hash, en);
}

/**
 * @brief		Enable/Disable loopback mode
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_loopback(void *base_va, bool_t en)
{
	uint32_t reg = hal_read32((addr_t)base_va + MAC_CONFIGURATION) & ~(LOOPBACK_MODE(1));

	reg |= LOOPBACK_MODE(en);

	hal_write32(reg, (addr_t)base_va + MAC_CONFIGURATION);
}

/**
 * @brief		Enable/Disable promiscuous mode
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_promisc_mode(void *base_va, bool_t en)
{
	uint32_t reg = hal_read32((addr_t)base_va + MAC_PACKET_FILTER) & ~(PROMISCUOUS_MODE(1));

	reg |= PROMISCUOUS_MODE(en);

	hal_write32(reg, (addr_t)base_va + MAC_PACKET_FILTER);
}

/**
 * @brief		Enable/Disable broadcast reception
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_broadcast(void *base_va, bool_t en)
{
	uint32_t reg = hal_read32((addr_t)base_va + MAC_PACKET_FILTER) & ~(DISABLE_BROADCAST_PACKETS(1));

	reg |= DISABLE_BROADCAST_PACKETS(!en);

	hal_write32(reg, (addr_t)base_va + MAC_PACKET_FILTER);
}

/**
 * @brief		Enable/Disable the Ethernet controller
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_enable(void *base_va, bool_t en)
{
	uint32_t reg = hal_read32((addr_t)base_va + MAC_CONFIGURATION);

	reg &= ~(TRANSMITTER_ENABLE(1) | RECEIVER_ENABLE(1));
	reg |= TRANSMITTER_ENABLE(en) | RECEIVER_ENABLE(en);

	hal_write32(reg, (addr_t)base_va + MAC_CONFIGURATION);
}

/**
 * @brief		Enable/Disable the flow control
 * @details		Once enabled the MAC shall process PAUSE frames
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param		en TRUE means ENABLE, FALSE means DISABLE
 */
void pfe_emac_cfg_set_flow_control(void *base_va, bool_t en)
{
	uint32_t reg, ii=0U;

	do
	{
		reg = hal_read32((addr_t)base_va + MAC_Q0_TX_FLOW_CTRL);
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

	hal_write32(reg, (addr_t)base_va + MAC_Q0_TX_FLOW_CTRL);
}

/**
 * @brief		Read value from the MDIO bus using Clause 22
 * @param[in]	base_va Base address of MAC register space (virtual)
 * @param[in]	pa Address of the PHY to read (5-bit)
 * @param[in]	ra Address of the register in the PHY to be read (5-bit)
 * @param[out]	val If success the the read value is written here (16 bit)
 * @retval		EOK Success
 */
errno_t pfe_emac_cfg_mdio_read22(void *base_va, uint8_t pa, uint8_t ra, uint16_t *val)
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
	while(GMII_BUSY(1) == ((reg = hal_read32(base_va + MAC_MDIO_ADDRESS)) & GMII_BUSY(1)))
	{
		if (timeout-- == 0U)
		{
			return ETIME;
		}
		oal_time_usleep(10);
	}

	/*	Get the data */
	reg = hal_read32(base_va + MAC_MDIO_DATA);
	*val = GMII_DATA(reg);

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
errno_t pfe_emac_cfg_mdio_read45(void *base_va, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t *val)
{
	uint32_t reg;
	uint32_t timeout = 500U;

	/* Set the register addresss to read */
	reg = GMII_REGISTER_ADDRESS(ra);
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
	*val = GMII_DATA(reg);

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
errno_t pfe_emac_cfg_mdio_write22(void *base_va, uint8_t pa, uint8_t ra, uint16_t val)
{
	uint32_t reg;
	uint32_t timeout = 500U;

	reg = GMII_DATA(val);
	hal_write32(reg, (addr_t)base_va + MAC_MDIO_DATA);

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

	hal_write32(reg, (addr_t)base_va + MAC_MDIO_ADDRESS);
	while(GMII_BUSY(1) == (hal_read32((addr_t)base_va + MAC_MDIO_ADDRESS) & GMII_BUSY(1)))
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
errno_t pfe_emac_cfg_mdio_write45(void *base_va, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t val)
{
	uint32_t reg;
	uint32_t timeout = 500U;

	reg = GMII_DATA(val) | GMII_REGISTER_ADDRESS(ra);
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
 * @brief		Get EMAC statistics in text form
 * @details		This is a HW-specific function providing detailed text statistics
 * 				about the EMAC block.
 * @param[in]	base_va 	Base address of EMAC register space (virtual)
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_emac_cfg_get_text_stat(void *base_va, char_t *buf, uint32_t size, uint8_t verb_level)
{
	uint32_t len = 0U;
	uint32_t reg;
	pfe_emac_speed_t speed;
	pfe_emac_duplex_t duplex;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */


	/*	Get version */
	reg = hal_read32((addr_t)base_va + MAC_VERSION);
	len += oal_util_snprintf(buf + len, size - len, "SNPVER                    : 0x%x\n", reg & 0xffU);
	len += oal_util_snprintf(buf + len, size - len, "USERVER                   : 0x%x\n", (reg >> 8) & 0xffU);

	reg = hal_read32((addr_t)base_va + RX_PACKETS_COUNT_GOOD_BAD);
	len += oal_util_snprintf(buf + len, size - len, "RX_PACKETS_COUNT_GOOD_BAD : 0x%x\n", reg);
	reg = hal_read32((addr_t)base_va + TX_PACKET_COUNT_GOOD_BAD);
	len += oal_util_snprintf(buf + len, size - len, "TX_PACKET_COUNT_GOOD_BAD  : 0x%x\n", reg);

	pfe_emac_cfg_get_link_config(base_va, &speed, &duplex);
	reg = hal_read32((addr_t)base_va + MAC_CONFIGURATION);
	len += oal_util_snprintf(buf + len, size - len, "MAC_CONFIGURATION         : 0x%x [speed: %s]\n", reg, emac_speed_to_str(speed));

	reg = (hal_read32((addr_t)base_va + MAC_HW_FEATURE0) >> 28U) & 0x07U;
	len += oal_util_snprintf(buf + len, size - len, "ACTPHYSEL(MAC_HW_FEATURE0): %s\n", phy_mode_to_str(reg));

	/* Error debugging */
	if(verb_level >= 8)
	{
		reg = hal_read32((addr_t)base_va + TX_UNDERFLOW_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_UNDERFLOW_ERROR_PACKETS        : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_SINGLE_COLLISION_GOOD_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_SINGLE_COLLISION_GOOD_PACKETS  : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_MULTIPLE_COLLISION_GOOD_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_MULTIPLE_COLLISION_GOOD_PACKETS: 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_DEFERRED_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_DEFERRED_PACKETS               : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_LATE_COLLISION_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_LATE_COLLISION_PACKETS         : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_EXCESSIVE_COLLISION_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_EXCESSIVE_COLLISION_PACKETS    : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_CARRIER_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_CARRIER_ERROR_PACKETS          : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_EXCESSIVE_DEFERRAL_ERROR);
		len += oal_util_snprintf(buf + len, size - len, "TX_EXCESSIVE_DEFERRAL_ERROR       : 0x%x\n", reg);

		reg = hal_read32((addr_t)base_va + TX_OSIZE_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OSIZE_PACKETS_GOOD             : 0x%x\n", reg);

		reg = hal_read32((addr_t)base_va + RX_CRC_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_CRC_ERROR_PACKETS              : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_ALIGNMENT_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_ALIGNMENT_ERROR_PACKETS        : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_RUNT_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_RUNT_ERROR_PACKETS             : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_JABBER_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_JABBER_ERROR_PACKETS           : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_LENGTH_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_LENGTH_ERROR_PACKETS           : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_OUT_OF_RANGE_TYPE_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_OUT_OF_RANGE_TYPE_PACKETS      : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_FIFO_OVERFLOW_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_FIFO_OVERFLOW_PACKETS          : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_RECEIVE_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_RECEIVE_ERROR_PACKETS          : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_RECEIVE_ERROR_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_RECEIVE_ERROR_PACKETS          : 0x%x\n", reg);
	}

	/* Cast/vlan/flow control */
	if(verb_level >= 3)
	{
		reg = hal_read32((addr_t)base_va + TX_UNICAST_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_UNICAST_PACKETS_GOOD_BAD       : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_BROADCAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_BROADCAST_PACKETS_GOOD         : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_BROADCAST_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_BROADCAST_PACKETS_GOOD_BAD     : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_MULTICAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_MULTICAST_PACKETS_GOOD         : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_MULTICAST_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_MULTICAST_PACKETS_GOOD_BAD     : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_VLAN_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_VLAN_PACKETS_GOOD              : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_PAUSE_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "TX_PAUSE_PACKETS                  : 0x%x\n", reg);
	}

	if(verb_level >= 4)
	{
		reg = hal_read32((addr_t)base_va + RX_UNICAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_UNICAST_PACKETS_GOOD           : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_BROADCAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_BROADCAST_PACKETS_GOOD         : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_MULTICAST_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_MULTICAST_PACKETS_GOOD         : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_VLAN_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_VLAN_PACKETS_GOOD_BAD          : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_PAUSE_PACKETS);
		len += oal_util_snprintf(buf + len, size - len, "RX_PAUSE_PACKETS                  : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_CONTROL_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_CONTROL_PACKETS_GOOD           : 0x%x\n", reg);
	}

	if(verb_level >= 1)
	{
		reg = hal_read32((addr_t)base_va + TX_OCTET_COUNT_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OCTET_COUNT_GOOD                : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_OCTET_COUNT_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OCTET_COUNT_GOOD_BAD            : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_64OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_64OCTETS_PACKETS_GOOD_BAD       : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_65TO127OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_65TO127OCTETS_PACKETS_GOOD_BAD  : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_128TO255OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_128TO255OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_256TO511OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_256TO511OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_512TO1023OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_512TO1023OCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
	}

	if(verb_level >= 5)
	{
		reg = hal_read32((addr_t)base_va + TX_OSIZE_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OSIZE_PACKETS_GOOD              : 0x%x\n", reg);
	}

	if(verb_level >= 2)
	{
		reg = hal_read32((addr_t)base_va + RX_OCTET_COUNT_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_OCTET_COUNT_GOOD                : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_OCTET_COUNT_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_OCTET_COUNT_GOOD_BAD            : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_64OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_64OCTETS_PACKETS_GOOD_BAD       : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_65TO127OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_65TO127OCTETS_PACKETS_GOOD_BAD  : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_128TO255OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_128TO255OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_256TO511OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_256TO511OCTETS_PACKETS_GOOD_BAD : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_512TO1023OCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_512TO1023OCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD);
		len += oal_util_snprintf(buf + len, size - len, "RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD: 0x%x\n", reg);
	}

	if(verb_level >= 5)
	{
		reg = hal_read32((addr_t)base_va + RX_OVERSIZE_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "TX_OSIZE_PACKETS_GOOD              : 0x%x\n", reg);
		reg = hal_read32((addr_t)base_va + RX_UNDERSIZE_PACKETS_GOOD);
		len += oal_util_snprintf(buf + len, size - len, "RX_UNDERSIZE_PACKETS_GOOD          : 0x%x\n", reg);
	}

	return len;
}
