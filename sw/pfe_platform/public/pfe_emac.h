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
 * @addtogroup	dxgrPFE_PLATFORM
 * @{
 * 
 * @defgroup    dxgr_PFE_EMAC EMAC
 * @brief		The Ethernet Peripheral (MAC)
 * @details     This is the software representation of the EMAC block.
 * 
 * @addtogroup  dxgr_PFE_EMAC
 * @{
 * 
 * @file		pfe_emac.h
 * @brief		The EMAC module header file.
 * @details		This file contains EMAC-related API.
 *
 */

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

/** @}*/
/** @}*/
