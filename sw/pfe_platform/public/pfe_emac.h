/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_EMAC_H_
#define PUBLIC_PFE_EMAC_H_

#include "pfe_ct.h"
#include "pfe_gpi.h"

#define PFE_EMAC_STD_MTU	1500U
#define PFE_EMAC_JUMBO_MTU	9000U
#define PFE_MIN_DSA_OVERHEAD	4U

typedef enum
{
	EMAC_MODE_INVALID,
	EMAC_MODE_MII,
	EMAC_MODE_RMII,
	EMAC_MODE_RGMII,
	EMAC_MODE_SGMII
} pfe_emac_mii_mode_t;

typedef enum
{
	EMAC_SPEED_INVALID,
	EMAC_SPEED_10_MBPS,
	EMAC_SPEED_100_MBPS,
	EMAC_SPEED_1000_MBPS,
	EMAC_SPEED_2500_MBPS
} pfe_emac_speed_t;

typedef enum
{
	EMAC_DUPLEX_INVALID,
	EMAC_DUPLEX_HALF,
	EMAC_DUPLEX_FULL
} pfe_emac_duplex_t;

typedef enum
{
	EMAC_LINK_SPEED_INVALID,
	EMAC_LINK_SPEED_2_5_MHZ,
	EMAC_LINK_SPEED_25_MHZ,
	EMAC_LINK_SPEED_125_MHZ
} pfe_emac_link_speed_t;

typedef enum
{
	PFE_FLUSH_MODE_ALL,
	PFE_FLUSH_MODE_UNI,
	PFE_FLUSH_MODE_MULTI
} pfe_flush_mode_t;

typedef struct pfe_emac_tag pfe_emac_t;

/**
 * @brief	The MAC address type
 * @details	Bytes are represented as:
 * 			\code
 * 				pfe_mac_addr_t mac;
 *
 * 				emac[0] = 0xaa;
 * 				emac[1] = 0xbb;
 * 				emac[2] = 0xcc;
 * 				emac[3] = 0xdd;
 * 				emac[4] = 0xee;
 * 				emac[5] = 0xff;
 *
 * 				printf("The MAC address is: %x:%x:%x:%x:%x:%x\n",
 * 						mac[0], emac[1], mac[2], mac[3], mac[4], mac[5]);
 * 			\endcode
 */
typedef uint8_t pfe_mac_addr_t[6];

/**
 * @brief	Possible types of MAC addresses used while getting or flushing
 */
typedef enum
{
	PFE_TYPE_UC,
	PFE_TYPE_MC,
	PFE_TYPE_BC,
	PFE_TYPE_ANY
} pfe_mac_type_t;

/**
 * @brief	Temporary solution for remap of mac_db criterion to emac criterion.
 * 			Order of the enum items must match with pfe_mac_db_crit_t defined in pfe_mac_db.h
 */
typedef enum __attribute__ ((packed)) {
	EMAC_CRIT_BY_TYPE = 0U,
	EMAC_CRIT_BY_OWNER,
	EMAC_CRIT_BY_OWNER_AND_TYPE,
	EMAC_CRIT_ALL,
	EMAC_CRIT_INVALID,
} pfe_emac_crit_t;

static inline const char_t *
pfe_emac_mii_mode_to_str(pfe_emac_mii_mode_t mode)
{
	static const char_t * names[] =
		{"Invalid", "MII", "RMII", "RGMII", "SGMII"};

	if (mode <= EMAC_MODE_SGMII)
	{
		return names[mode];
	}
	else
	{
		return "out-of-range";
	}
}

/**
 * @brief		Check if given MAC address is zero
 * @param[in]	addr The address to check
 * @return		TRUE if the input address is zero
 */
static inline bool_t pfe_emac_is_zero(const pfe_mac_addr_t addr)
{
	if (0x0U == (addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Check if given MAC address is broadcast
 * @param[in]	addr The address to check
 * @return		TRUE if the input address is broadcast
 */
static inline bool_t pfe_emac_is_broad(const pfe_mac_addr_t addr)
{
	if (0xffU == (addr[0] & addr[1] & addr[2] & addr[3] & addr[4] & addr[5]))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Check if given MAC address is multicast
 * @param[in]	addr The address to check
 * @return		TRUE if the input address is multicast
 */
static inline bool_t pfe_emac_is_multi(const pfe_mac_addr_t addr)
{
	if ((FALSE == pfe_emac_is_broad(addr)) && (0U != (addr[0] & 0x1U)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Check if entry match with the rule
 * @param[in]	addr The address to check
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @return		TRUE if entry match with the rule, FALSE otherwise
 */
static inline bool_t pfe_emac_check_crit_by_type(const pfe_mac_addr_t addr, pfe_mac_type_t type)
{
    bool_t ret = FALSE;
    if ((type == PFE_TYPE_ANY) ||
        ((type == PFE_TYPE_MC) && (TRUE  == pfe_emac_is_multi(addr))) ||
        ((type == PFE_TYPE_BC) && (TRUE  == pfe_emac_is_broad(addr))) ||
        ((type == PFE_TYPE_UC) && (FALSE  == pfe_emac_is_multi(addr)))
       )
    {
        ret = TRUE;
    }
    return ret;
}

pfe_emac_t *pfe_emac_create(addr_t cbus_base_va, addr_t emac_base, pfe_emac_mii_mode_t mode, pfe_emac_speed_t speed, pfe_emac_duplex_t duplex);
uint8_t pfe_emac_get_index(const pfe_emac_t *emac);
errno_t pfe_emac_bind_gpi(pfe_emac_t *emac, pfe_gpi_t *gpi);
pfe_gpi_t *pfe_emac_get_gpi(const pfe_emac_t *emac);
void pfe_emac_enable(const pfe_emac_t *emac);
void pfe_emac_disable(const pfe_emac_t *emac);
errno_t pfe_emac_enable_ts(pfe_emac_t *emac, uint32_t i_clk_hz, uint32_t o_clk_hz);
errno_t pfe_emac_set_ts_freq_adjustment(pfe_emac_t *emac, uint32_t ppb, bool_t sgn);
errno_t pfe_emac_get_ts_freq_adjustment(pfe_emac_t *emac, uint32_t *ppb, bool_t *sgn);
errno_t pfe_emac_set_ts_time(pfe_emac_t *emac, uint32_t sec, uint32_t nsec, uint16_t sec_hi);
errno_t pfe_emac_adjust_ts_time(pfe_emac_t *emac, uint32_t sec, uint32_t nsec, bool_t sgn);
errno_t pfe_emac_get_ts_time(pfe_emac_t *emac, uint32_t *sec, uint32_t *nsec, uint16_t *sec_hi);
void pfe_emac_enable_loopback(const pfe_emac_t *emac);
void pfe_emac_disable_loopback(const pfe_emac_t *emac);
void pfe_emac_enable_promisc_mode(const pfe_emac_t *emac);
void pfe_emac_disable_promisc_mode(const pfe_emac_t *emac);
void pfe_emac_enable_allmulti_mode(const pfe_emac_t *emac);
void pfe_emac_disable_allmulti_mode(const pfe_emac_t *emac);
void pfe_emac_enable_broadcast(const pfe_emac_t *emac);
void pfe_emac_disable_broadcast(const pfe_emac_t *emac);
void pfe_emac_enable_tx_flow_control(const pfe_emac_t *emac);
void pfe_emac_disable_tx_flow_control(const pfe_emac_t *emac);
void pfe_emac_enable_rx_flow_control(const pfe_emac_t *emac);
void pfe_emac_disable_rx_flow_control(const pfe_emac_t *emac);
void pfe_emac_get_flow_control(const pfe_emac_t *emac, bool_t *tx_enable, bool_t *rx_enable);
errno_t pfe_emac_set_max_frame_length(const pfe_emac_t *emac, uint32_t len);
pfe_emac_mii_mode_t pfe_emac_get_mii_mode(const pfe_emac_t *emac);
errno_t pfe_emac_get_link_config(const pfe_emac_t *emac, pfe_emac_speed_t *speed, pfe_emac_duplex_t *duplex);
errno_t pfe_emac_get_link_status(const pfe_emac_t *emac, pfe_emac_link_speed_t *link_speed, pfe_emac_duplex_t *duplex, bool_t *link);
errno_t pfe_emac_set_link_speed(const pfe_emac_t *emac, pfe_emac_speed_t link_speed);
errno_t pfe_emac_set_link_duplex(const pfe_emac_t *emac, pfe_emac_duplex_t duplex);
errno_t pfe_emac_mdio_lock(pfe_emac_t *emac, uint32_t *key);
errno_t pfe_emac_mdio_unlock(pfe_emac_t *emac, uint32_t key);
errno_t pfe_emac_mdio_read22(pfe_emac_t *emac, uint8_t pa, uint8_t ra, uint16_t *val, uint32_t key);
errno_t pfe_emac_mdio_write22(pfe_emac_t *emac, uint8_t pa, uint8_t ra, uint16_t val, uint32_t key);
errno_t pfe_emac_mdio_read45(pfe_emac_t *emac, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t *val, uint32_t key);
errno_t pfe_emac_mdio_write45(pfe_emac_t *emac, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t val, uint32_t key);
errno_t pfe_emac_add_addr(pfe_emac_t *emac, const pfe_mac_addr_t addr, pfe_drv_id_t owner);
errno_t pfe_emac_flush_mac_addrs(pfe_emac_t *emac, pfe_emac_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner);
errno_t pfe_emac_get_addr(pfe_emac_t *emac, pfe_mac_addr_t addr);
errno_t pfe_emac_del_addr(pfe_emac_t *emac, const pfe_mac_addr_t addr, pfe_drv_id_t owner);
void pfe_emac_destroy(pfe_emac_t *emac);

uint32_t pfe_emac_get_text_statistics(const pfe_emac_t *emac, struct seq_file *seq, uint8_t verb_level);

uint32_t pfe_emac_get_rx_cnt(const pfe_emac_t *emac);
uint32_t pfe_emac_get_tx_cnt(const pfe_emac_t *emac);
uint32_t pfe_emac_get_stat_value(const pfe_emac_t *emac, uint32_t stat_id);
errno_t pfe_emac_isr(pfe_emac_t *emac);
void pfe_emac_irq_mask(pfe_emac_t *emac);
void pfe_emac_irq_unmask(pfe_emac_t *emac);

#endif /* PUBLIC_PFE_EMAC_H_ */
