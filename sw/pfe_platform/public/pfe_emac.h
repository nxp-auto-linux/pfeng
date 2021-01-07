/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2018-2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#ifndef PUBLIC_PFE_EMAC_H_
#define PUBLIC_PFE_EMAC_H_

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

typedef struct __pfe_emac_tag pfe_emac_t;

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

pfe_emac_t *pfe_emac_create(void *cbus_base_va, void *emac_base, pfe_emac_mii_mode_t mode, pfe_emac_speed_t speed, pfe_emac_duplex_t duplex);
void pfe_emac_enable(pfe_emac_t *emac);
void pfe_emac_disable(pfe_emac_t *emac);
errno_t pfe_emac_enable_ts(pfe_emac_t *emac, uint32_t i_clk_hz, uint32_t o_clk_hz);
errno_t pfe_emac_set_ts_freq_adjustment(pfe_emac_t *emac, uint32_t ppb, bool_t sgn);
errno_t pfe_emac_get_ts_freq_adjustment(pfe_emac_t *emac, uint32_t *ppb, bool_t *sgn);
errno_t pfe_emac_set_ts_time(pfe_emac_t *emac, uint32_t sec, uint32_t nsec);
errno_t pfe_emac_adjust_ts_time(pfe_emac_t *emac, uint32_t sec, uint32_t nsec, bool_t sgn);
errno_t pfe_emac_get_ts_time(pfe_emac_t *emac, uint32_t *sec, uint32_t *nsec);
void pfe_emac_enable_loopback(pfe_emac_t *emac);
void pfe_emac_disable_loopback(pfe_emac_t *emac);
void pfe_emac_enable_promisc_mode(pfe_emac_t *emac);
void pfe_emac_disable_promisc_mode(pfe_emac_t *emac);
void pfe_emac_enable_broadcast(pfe_emac_t *emac);
void pfe_emac_disable_broadcast(pfe_emac_t *emac);
void pfe_emac_enable_flow_control(pfe_emac_t *emac);
void pfe_emac_disable_flow_control(pfe_emac_t *emac);
errno_t pfe_emac_set_max_frame_length(pfe_emac_t *emac, uint32_t len);
pfe_emac_mii_mode_t pfe_emac_get_mii_mode(pfe_emac_t *emac);
errno_t pfe_emac_get_link_config(pfe_emac_t *emac, pfe_emac_speed_t *speed, pfe_emac_duplex_t *duplex);
errno_t pfe_emac_get_link_status(pfe_emac_t *emac, pfe_emac_link_speed_t *link_speed, pfe_emac_duplex_t *duplex, bool_t *link);
errno_t pfe_emac_mdio_lock(pfe_emac_t *emac, uint32_t *key);
errno_t pfe_emac_mdio_unlock(pfe_emac_t *emac, uint32_t key);
errno_t pfe_emac_mdio_read22(pfe_emac_t *emac, uint8_t pa, uint8_t ra, uint16_t *val, uint32_t key);
errno_t pfe_emac_mdio_write22(pfe_emac_t *emac, uint8_t pa, uint8_t ra, uint16_t val, uint32_t key);
errno_t pfe_emac_mdio_read45(pfe_emac_t *emac, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t *val, uint32_t key);
errno_t pfe_emac_mdio_write45(pfe_emac_t *emac, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t val, uint32_t key);
errno_t pfe_emac_add_addr(pfe_emac_t *emac, pfe_mac_addr_t addr);
errno_t pfe_emac_add_addr_to_hash_group(pfe_emac_t *emac, pfe_mac_addr_t addr);
errno_t pfe_emac_get_addr(pfe_emac_t *emac, pfe_mac_addr_t addr);
errno_t pfe_emac_del_addr(pfe_emac_t *emac, pfe_mac_addr_t addr);
void pfe_emac_destroy(pfe_emac_t *emac);
uint32_t pfe_emac_get_text_statistics(pfe_emac_t *emac, char_t *buf, uint32_t buf_len, uint8_t verb_level);

void pfe_emac_test(void *cbus_base_va, void *emac_base);

#endif /* PUBLIC_PFE_EMAC_H_ */
