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

#include "linked_list.h"
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_emac_csr.h"
#include "pfe_emac.h"

struct __pfe_emac_tag
{
	void *cbus_base_va;			/*	CBUS base virtual address */
	void *emac_base_offset;		/*	MAC base offset within CBUS space */
	void *emac_base_va;			/*	MAC base address (virtual) */
	LLIST_t mac_addr_list;		/*	List of all registered MAC addresses within the EMAC */
	uint8_t mac_addr_slots;		/*	Bitmask representing local address slots where '1' means 'slot is used' */
	pfe_emac_mii_mode_t mode;	/*	Current MII mode */
	pfe_emac_speed_t speed;		/*	Current speed */
	pfe_emac_duplex_t duplex;	/*	Current duplex */
	oal_mutex_t mutex;			/*	Mutex */
	bool_t mdio_locked;			/*	If TRUE then MDIO access is locked and 'mdio_key' is valid */
	uint32_t mdio_key;
	oal_mutex_t ts_mutex;		/*	Mutex protecting IEEE1588 resources */
	uint32_t i_clk_hz;			/*	IEEE1588 input clock */
	uint32_t o_clk_hz;			/*	IEEE1588 desired output clock */
	uint32_t adj_ppb;			/*	IEEE1588 frequency adjustment value */
	bool_t adj_sign;			/*	IEEE1588 frequency adjustment sign (TRUE - positive, FALSE - negative) */
};

typedef struct __pfe_mac_addr_entry_tag
{
	pfe_mac_addr_t addr;	/*	The MAC address */
	uint32_t hash;			/*	Associated hash value (valid if in_hash_grp is TRUE) */
	bool_t in_hash_grp;		/*	If TRUE then the address belongs to a hash group */
	uint8_t addr_slot_idx;	/*	If 'in_hash_grp' is FALSE then this value specifies index of
								individual address slot the address is stored in */
	LLIST_t iterator;		/*	List chain entry */
} pfe_mac_addr_db_entry_t;

static void pfe_emac_addr_db_init(pfe_emac_t *emac);
static errno_t pfe_emac_addr_db_add(pfe_emac_t *emac, pfe_mac_addr_t addr, bool_t in_hash_grp, uint32_t idx);
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_hash(pfe_emac_t *emac, uint32_t hash);
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_addr(pfe_emac_t *emac, pfe_mac_addr_t addr);
static errno_t pfe_emac_addr_db_del_entry(pfe_emac_t *emac, pfe_mac_addr_db_entry_t *entry);
#if 0
static errno_t pfe_emac_addr_db_del(pfe_emac_t *emac, pfe_mac_addr_t addr);
#endif /* 0 */
static void pfe_emac_addr_db_drop_all(pfe_emac_t *emac);

/**
 * @brief		Check if given MAC address is broadcast
 * @param[in]	addr The address to check
 * @return		TRUE if the input address is broadcast
 */
static inline bool_t pfe_emac_is_broad(pfe_mac_addr_t addr)
{
	static const pfe_mac_addr_t bc = {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU};
	
	if (0 == memcmp(addr, bc, sizeof(pfe_mac_addr_t)))
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
static inline bool_t pfe_emac_is_multi(pfe_mac_addr_t addr)
{
	if ((FALSE == pfe_emac_is_broad(addr)) && (0 != (addr[0] & 0x1U)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief		Initialize the internal MAC address DB
 * @param[in]	emac The EMAC instance
 */
static void pfe_emac_addr_db_init(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	LLIST_Init(&emac->mac_addr_list);
}

/**
 * @brief		Add MAC address to internal DB
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to add
 * @param[in]	in_hash_grp TRUE if the address is matched by EMAC by its hash
 * @param[in]	data Address slot index associated with the new entry. Only valid when in_hash_grp==FALSE.
 * 					 When in_hash_grp==TRUE this is the hash value.
 */
static errno_t pfe_emac_addr_db_add(pfe_emac_t *emac, pfe_mac_addr_t addr, bool_t in_hash_grp, uint32_t data)
{
	pfe_mac_addr_db_entry_t *entry;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Create new list entry */
	entry = oal_mm_malloc(sizeof(pfe_mac_addr_db_entry_t));
	if (NULL == entry)
	{
		NXP_LOG_ERROR("oal_mm_malloc() failed\n");
		return ENOMEM;
	}
	
	if (in_hash_grp)
	{
		entry->hash = data;
		entry->in_hash_grp = TRUE;
	}
	else
	{
		entry->addr_slot_idx = data;
		entry->in_hash_grp = FALSE;
	}
	
	/*	Add entry to the list */
	memcpy(entry->addr, addr, sizeof(pfe_mac_addr_t));
	
	LLIST_AddAtEnd(&entry->iterator, &emac->mac_addr_list);
	
	return EOK;
}

#if 0
/**
 * @brief		Remove MAC address from internal DB
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to delete
 * @retval		EOK Success
 * @retval		ENOENT Address not found
 */
static errno_t pfe_emac_addr_db_del(pfe_emac_t *emac, pfe_mac_addr_t addr)
{
	pfe_mac_addr_db_entry_t *entry;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	entry = pfe_emac_addr_db_find_by_addr(emac, addr);
	
	if (NULL == entry)
	{
		return ENOENT;
	}
	else
	{
		LLIST_Remove(&entry->iterator);
		return EOK;
	}
}
#endif /* 0 */

/**
 * @brief		Remove MAC address from internal DB
 * @param[in]	emac The EMAC instance
 * @param[in]	entry The DB entry to delete
 * @retval		EOK Success
 * @retval		EINVAL Entry is NULL
 */
static errno_t pfe_emac_addr_db_del_entry(pfe_emac_t *emac, pfe_mac_addr_db_entry_t *entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#else
	(void)emac;
#endif /* PFE_CFG_NULL_ARG_CHECK */

	LLIST_Remove(&entry->iterator);
	return EOK;
}

/**
 * @brief		Drop all entries from the DB
 * @param[in]	emac The EMAC instance
 */
static void pfe_emac_addr_db_drop_all(pfe_emac_t *emac)
{
	pfe_mac_addr_db_entry_t *entry;
	LLIST_t *item, *tmp_item;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Release the MAC address DB */
	LLIST_ForEachRemovable(item, tmp_item, &emac->mac_addr_list)
	{
		entry = LLIST_Data(item, pfe_mac_addr_db_entry_t, iterator);
		LLIST_Remove(&entry->iterator);
		oal_mm_free(entry);
	}
}

/**
 * @brief		Search a MAC address within internal DB of registered addresses
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to search
 * @return		The DB entry if found or NULL if address is not present
 */
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_addr(pfe_emac_t *emac, pfe_mac_addr_t addr)
{
	pfe_mac_addr_db_entry_t *entry = NULL;
	LLIST_t *item;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	LLIST_ForEach(item, &emac->mac_addr_list)
	{
		entry = LLIST_Data(item, pfe_mac_addr_db_entry_t, iterator);
		if (0 == memcmp(addr, entry->addr, sizeof(pfe_mac_addr_t)))
		{
			return entry;
		}
	}
	
	return NULL;
}

/**
 * @brief		Get the very first address from the DB
 * @param[in]	emac The EMAC instance
 * @return		The DB entry if found or NULL if address is not present
 */
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_get_first(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (TRUE == LLIST_IsEmpty(&emac->mac_addr_list))
	{
		return NULL;
	}
	else
	{
		return LLIST_Data(emac->mac_addr_list.prNext, pfe_mac_addr_db_entry_t, iterator);
	}
}

/**
 * @brief		Search a MAC address within internal DB of registered addresses based on hash value
 * @param[in]	emac The EMAC instance
 * @param[in]	hash The hash to search
 * @return		The DB entry if found or NULL if address is not present
 */
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_hash(pfe_emac_t *emac, uint32_t hash)
{
	pfe_mac_addr_db_entry_t *entry = NULL;
	LLIST_t *item;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	LLIST_ForEach(item, &emac->mac_addr_list)
	{
		entry = LLIST_Data(item, pfe_mac_addr_db_entry_t, iterator);
		if (entry->hash == hash)
		{
			return entry;
		}
	}
	
	return NULL;
}

/**
 * @brief		Search a MAC address within internal DB of registered addresses based on slot index
 * @param[in]	emac The EMAC instance
 * @param[in]	slot The slot index to search
 * @return		The DB entry if found or NULL if address is not present
 */
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_slot(pfe_emac_t *emac, uint8_t slot)
{
	pfe_mac_addr_db_entry_t *entry = NULL;
	LLIST_t *item;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	LLIST_ForEach(item, &emac->mac_addr_list)
	{
		entry = LLIST_Data(item, pfe_mac_addr_db_entry_t, iterator);
		if (entry->addr_slot_idx == slot)
		{
			return entry;
		}
	}
	
	return NULL;
}

/**
 * @brief		Create new EMAC instance
 * @details		Creates and initializes MAC instance
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	emac_base EMAC base address offset within CBUS address space
 * @param[in]	mode The MII mode to be used @see pfe_emac_mii_mode_t
 * @param[in]	speed Speed @see pfe_emac_speed_t
 * @param[in]	duplex The duplex type @see pfe_emac_duplex_t
 * @return		The EMAC instance or NULL if failed
 */
pfe_emac_t *pfe_emac_create(void *cbus_base_va, void *emac_base, pfe_emac_mii_mode_t mode, pfe_emac_speed_t speed, pfe_emac_duplex_t duplex)
{
	pfe_emac_t *emac;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	emac = oal_mm_malloc(sizeof(pfe_emac_t));
	
	if (NULL == emac)
	{
		return NULL;
	}
	else
	{
		memset(emac, 0, sizeof(pfe_emac_t));
		emac->cbus_base_va = cbus_base_va;
		emac->emac_base_offset = emac_base;
		emac->emac_base_va = (void *)((addr_t)emac->cbus_base_va + (addr_t)emac->emac_base_offset);
		emac->mode = EMAC_MODE_INVALID;
		emac->speed = EMAC_SPEED_INVALID;
		emac->duplex = EMAC_DUPLEX_INVALID;

		if (EOK != oal_mutex_init(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex init failed\n");
			oal_mm_free(emac);
			return NULL;
		}
		
		if (EOK != oal_mutex_init(&emac->ts_mutex))
		{
			NXP_LOG_ERROR("TS mutex init failed\n");
			oal_mutex_destroy(&emac->mutex);
			oal_mm_free(emac);
			return NULL;
		}

		/*	All slots are free */
		emac->mac_addr_slots = 0U;
		
		/*	Initialize the MAC address DB */
		pfe_emac_addr_db_init(emac);
		
		/*	Disable the HW */
		pfe_emac_disable(emac);
		
		/*	Initialize the HW */
		if (EOK != pfe_emac_cfg_init(emac->emac_base_va, mode, speed, duplex))
		{
			/*	Invalid configuration */
			NXP_LOG_ERROR("Invalid configuration requested\n");
			oal_mutex_destroy(&emac->mutex);
			oal_mutex_destroy(&emac->ts_mutex);
			oal_mm_free(emac);
			emac = NULL;
			return NULL;
		}
		else
		{
			emac->mode = mode;
			emac->speed = speed;
			emac->duplex = duplex;
		}

		/*	Disable loop-back */
		pfe_emac_disable_loopback(emac);
		
		/*	Disable promiscuous mode */
		pfe_emac_disable_promisc_mode(emac);
		
		/*	Disable broadcast */
		pfe_emac_disable_broadcast(emac);
	}
	
	return emac;
}

/**
 * @brief		Enable the EMAC
 * @details		Data transmission/reception is possible after this call
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_enable(emac->emac_base_va, TRUE);
}

/**
 * @brief		Disable the EMAC
 * @details		No data transmission/reception is possible after this call
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_enable(emac->emac_base_va, FALSE);
}

/**
 * @brief		Enable timestamping
 * @param[in]	emac The EMAC instance
 * @param[in]	i_clk_hz Input reference clock frequency (Hz) when internal timer is
 * 					   used. The timer ticks with 1/clk_hz period. If zero then external
 * 					   clock reference is used.
 * @param[in]	o_clk_hz Desired output clock frequency. This one will be used to
 * 						 increment IEEE1588 system time. Directly impacts the timer
 * 						 accuracy and must be less than i_clk_hz. If zero then external
 * 						 clock reference is used.
 */
errno_t pfe_emac_enable_ts(pfe_emac_t *emac, uint32_t i_clk_hz, uint32_t o_clk_hz)
{
	errno_t ret;
	bool_t eclk = (i_clk_hz == 0U) || (o_clk_hz == 0U);

	if (!eclk && (i_clk_hz <= o_clk_hz))
	{
		NXP_LOG_ERROR("Invalid clock configuration\n");
		return EINVAL;
	}

	emac->i_clk_hz = i_clk_hz;
	emac->o_clk_hz = o_clk_hz;

	if (EOK != oal_mutex_lock(&emac->ts_mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	ret = pfe_emac_cfg_enable_ts(emac->emac_base_va, eclk, i_clk_hz, o_clk_hz);

	if (EOK != oal_mutex_unlock(&emac->ts_mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	return ret;
}

/**
 * @brief		Adjust timestamping clock frequency to compensate drift
 * @param[in]	emac The EMAC instance
 * @param[in]	ppb Frequency change in [ppb]
 * @param[in]	pos The ppb sign. If TRUE then the value is positive, else it is negative
 */
errno_t pfe_emac_set_ts_freq_adjustment(pfe_emac_t *emac, uint32_t ppb, bool_t sgn)
{
	errno_t ret;

	if (EOK != oal_mutex_lock(&emac->ts_mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	emac->adj_ppb = ppb;
	emac->adj_sign = sgn;

	ret = pfe_emac_cfg_adjust_ts_freq(emac->emac_base_va, emac->i_clk_hz, emac->o_clk_hz, ppb, sgn);

	if (EOK != oal_mutex_unlock(&emac->ts_mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	return ret;
}

/**
 * @brief			Get current adjustment value
 * @param[in]		emac The EMAC instance
 * @param[in,out]	ppb Pointer where the current adjustment value in ppb shall be written
 * @param[in,out]	sgn Pointer where the sign flag shall be written (TRUE means that
 * 					the 'ppb' is positive, FALSE means it is nagative)
 * @return			EOK if success, error code otherwise
 */
errno_t pfe_emac_get_ts_freq_adjustment(pfe_emac_t *emac, uint32_t *ppb, bool_t *sgn)
{
	if ((NULL == ppb) || (NULL == sgn))
	{
		return EINVAL;
	}
	else
	{
		if (EOK != oal_mutex_lock(&emac->ts_mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		*ppb = emac->adj_ppb;
		*sgn = emac->adj_sign;

		if (EOK != oal_mutex_unlock(&emac->ts_mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		return EOK;
	}
}

/**
 * @brief			Get current time
 * @param[in]		emac THe EMAC instance
 * @param[in,out]	sec Pointer where seconds value shall be written
 * @param[in,out]	nsec Pointer where nano-seconds value shall be written
 * @return			EOK if success, error code otherwise
 */
errno_t pfe_emac_get_ts_time(pfe_emac_t *emac, uint32_t *sec, uint32_t *nsec)
{
	errno_t ret;

	if ((NULL == sec) || (NULL == nsec))
	{
		ret = EINVAL;
	}
	else
	{
		if (EOK != oal_mutex_lock(&emac->ts_mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		pfe_emac_cfg_get_ts_time(emac->emac_base_va, sec, nsec);
		ret = EOK;

		if (EOK != oal_mutex_unlock(&emac->ts_mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Adjust current time
 * @details		Current timer value will be adjusted by adding or subtracting the
 * 				desired value.
 * @param[in]	emac The EMAC instance
 * @param[in]	sec Seconds
 * @param[in]	nsec NanoSeconds
 * @param[in]	sgn Sign of the adjustment. If TRUE then the adjustment will be positive
 * 					('sec' and 'nsec' will be added to the current time. If FALSE then the
 * 					adjustment will be negative ('sec' and 'nsec' will be subtracted from
 *					the current time).
 */
errno_t pfe_emac_adjust_ts_time(pfe_emac_t *emac, uint32_t sec, uint32_t nsec, bool_t sgn)
{
	errno_t ret;

	if (EOK != oal_mutex_lock(&emac->ts_mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	ret = pfe_emac_cfg_adjust_ts_time(emac->emac_base_va, sec, nsec, sgn);

	if (EOK != oal_mutex_unlock(&emac->ts_mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	return ret;
}

/**
 * @brief		Set current time
 * @details		Funcion will set new system time. Current timer value
 * 				will be overwritten with the desired value.
 * @param[in]	emac The EMAC instance
 * @param[in]	sec New seconds value
 * @param[in]	nsec New nano-seconds value
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_emac_set_ts_time(pfe_emac_t *emac, uint32_t sec, uint32_t nsec)
{
	errno_t ret;

	if (EOK != oal_mutex_lock(&emac->ts_mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	ret = pfe_emac_cfg_set_ts_time(emac->emac_base_va, sec, nsec);

	if (EOK != oal_mutex_unlock(&emac->ts_mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	return ret;
}

/**
 * @brief		Enable the local loop-back mode
 * @details		This function controls the EMAC internal loop-back mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable_loopback(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_loopback(emac->emac_base_va, TRUE);
}

/**
 * @brief		Disable loop-back mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable_loopback(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_loopback(emac->emac_base_va, FALSE);
}

/**
 * @brief		Enable promiscuous mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable_promisc_mode(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_promisc_mode(emac->emac_base_va, TRUE);
}

/**
 * @brief		Disable promiscuous mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable_promisc_mode(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_promisc_mode(emac->emac_base_va, FALSE);
}

/**
 * @brief		Enable broadcast reception
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable_broadcast(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_broadcast(emac->emac_base_va, TRUE);
}

/**
 * @brief		Disable broadcast reception
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable_broadcast(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_broadcast(emac->emac_base_va, FALSE);
}

/**
 * @brief		Enable flow control
 * @details		Enables PAUSE frames processing
 * @param		emac The EMAC instance
 */
void pfe_emac_enable_flow_control(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_flow_control(emac->emac_base_va, TRUE);
}

/**
 * @brief		Disable flow control
 * @details		Disables PAUSE frames processing
 * @param		emac The EMAC instance
 */
void pfe_emac_disable_flow_control(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_set_flow_control(emac->emac_base_va, FALSE);
}

/**
 * @brief		Set maximum frame length
 * @param		emac The EMAC instance
 * @param		len New frame length
 * @return		EOK if success errno otherwise
 */
errno_t pfe_emac_set_max_frame_length(pfe_emac_t *emac, uint32_t len)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = pfe_emac_cfg_set_max_frame_length(emac->emac_base_va, len);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Attempt to set unsupported frame length value\n");
	}

	return ret;
}

/**
 * @brief		Get current MII mode
 * @param[in]	emac The EMAC instance
 * @return		Currently configured MII mode @see pfe_emac_mii_mode_t
 */
pfe_emac_mii_mode_t pfe_emac_get_mii_mode(pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EMAC_MODE_INVALID;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	return emac->mode;
}

/**
 * @brief		Get the EMAC link configuration
 * @param[in]	emac The EMAC instance
 * @param[out]	speed The EMAC speed configuration @see pfe_emac_speed_t
 * @param[out]	duplex The EMAC duplex configuration @see pfe_emac_duplex_t
 * @return		EOK if success
 */
errno_t pfe_emac_get_link_config(pfe_emac_t *emac, pfe_emac_speed_t *speed, pfe_emac_duplex_t *duplex)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = pfe_emac_cfg_get_link_config(emac->emac_base_va, speed, duplex);

	return ret;
}

/**
 * @brief		Get the EMAC link status
 * @param[in]	emac The EMAC instance
 * @param[out]	speed The EMAC link speed @see pfe_emac_link_speed_t
 * @param[out]	duplex The EMAC duplex status @see pfe_emac_duplex_t
 * @param[out]	link The EMAC link status
 * @return		EOK if success
 */
errno_t pfe_emac_get_link_status(pfe_emac_t *emac, pfe_emac_link_speed_t *link_speed, pfe_emac_duplex_t *duplex, bool_t *link)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = pfe_emac_cfg_get_link_status(emac->emac_base_va, link_speed, duplex, link);

	return ret;
}

/**
 * @brief		Assign MAC address to a hash group
 * @details		Address resolution will be done using the hash instead of exact match
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to be added
 * @return		EOK if success
 * @return		EINVAL Requested address is broadcast
 * @note		Must not be preempted by: pfe_emac_del_addr(), pfe_emac_add_addr(), pfe_emac_destroy()
 */
errno_t pfe_emac_add_addr_to_hash_group(pfe_emac_t *emac, pfe_mac_addr_t addr)
{
	int32_t hash;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (pfe_emac_is_broad(addr))
	{
		/*	Can't add broadcast address */
		return EINVAL;
	}

	/*	Check if the address is already added */
	if (NULL != pfe_emac_addr_db_find_by_addr(emac, addr))
	{
		return EOK;
	}
	
	/*	Get the hash */ 
	hash = pfe_emac_cfg_get_hash(emac->emac_base_va, addr);
	
	/*	Store address into EMAC's internal DB together with 'in_hash_grp' flag and hash */
	pfe_emac_addr_db_add(emac, addr, TRUE, hash);
	
	/*	Configure the HW */
	if (pfe_emac_is_multi(addr))
	{
		/*	Multicast */
		pfe_emac_cfg_set_multi_group(emac->emac_base_va, hash, TRUE);
	}
	else
	{
		/*	Unicast */
		pfe_emac_cfg_set_uni_group(emac->emac_base_va, hash, TRUE);
	}
	
	return EOK;
}

/**
 * @brief		Remove MAC address from EMAC
 * @details		Address resolution will be done using exact match with the added address 
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to delete
 * @retval		EOK Success
 * @retval		ENOENT Address not found
 * @note		Must not be preempted by: pfe_emac_add_addr_to_hash_group(), pfe_emac_add_addr(), pfe_emac_destroy()
 */
errno_t pfe_emac_del_addr(pfe_emac_t *emac, pfe_mac_addr_t addr)
{
	pfe_mac_addr_db_entry_t *entry, local_entry;
	errno_t ret;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Get address entry from the internal DB */
	entry = pfe_emac_addr_db_find_by_addr(emac, addr);
	if (NULL == entry)
	{
		return ENOENT;
	}
	
	/*	Remember the entry */
	local_entry = *entry;
	
	/*	Remove the entry form DB */
	ret = pfe_emac_addr_db_del_entry(emac, entry);
	if (EOK != ret)
	{
		return ret;
	}
	else
	{
		/*	Release the entry */
		oal_mm_free(entry);
		entry = NULL;
	}
	
	if (TRUE == local_entry.in_hash_grp)
	{
		/*	Check if the hash group the address belongs to contains another addresses */
		if (NULL != pfe_emac_addr_db_find_by_hash(emac, local_entry.hash))
		{
			/*	Hash group contains more addresses. Keep the HW configured. */
			;
		}
		else
		{
			/*	Configure the HW */
			if (pfe_emac_is_multi(addr))
			{
				/*	Multicast */
				pfe_emac_cfg_set_multi_group(emac->emac_base_va, local_entry.hash, FALSE);
			}
			else
			{
				/*	Unicast */
				pfe_emac_cfg_set_uni_group(emac->emac_base_va, local_entry.hash, FALSE);
			}
		}
	}
	else
	{
		pfe_mac_addr_t zero_addr;
				
		/*	Prepare zero-filled address */
		memset(zero_addr, 0, sizeof(pfe_mac_addr_t));
		
		/*	Clear the specific slot */
		pfe_emac_cfg_write_addr_slot(emac->emac_base_va, zero_addr, local_entry.addr_slot_idx);
		
		/*	Mark the slot as unused */	
		emac->mac_addr_slots &= ~(1U << local_entry.addr_slot_idx);
	}
	
	return EOK;
}

/**
 * @brief		Assign an individual MAC address to EMAC
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to add
 * @retval		EOK Success
 * @retval		ENOSPC The EMAC has not a free address slot
 * @retval		ENOMEM Not enough memory
 * @retval		EEXIST Address already added
 * @note		Must not be preempted by: pfe_emac_add_addr_to_hash_group(), pfe_emac_del_addr(), pfe_emac_destroy()
 */
errno_t pfe_emac_add_addr(pfe_emac_t *emac, pfe_mac_addr_t addr)
{
	uint32_t slot;
	errno_t ret;
	pfe_mac_addr_db_entry_t *entry;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Check if address is already registered */
	entry = pfe_emac_addr_db_find_by_addr(emac, addr);
	if (NULL != entry)
	{
#if 0
		if (TRUE == entry->in_hash_grp)
		{
			/*
				Entry already exists within a hash group. We can keep address only in hash group.
			 	Remove the original entry and let the function do the rest.
			*/
			ret = pfe_emac_del_addr(emac, addr);
			if (EOK != ret)
			{
				return ret;
			}
		}
		else
		{
			/*	Address is already registered within a specific slot (entry->addr_slot_idx) */
			return EOK;
		}
#else
		/*	Duplicates are not allowed */
		return EEXIST;
#endif /* 0 */
	}

	/*	Try to get free individual address slot */
	for (slot=0U; slot<EMAC_CFG_INDIVIDUAL_ADDR_SLOTS_COUNT; slot++)
	{
		if (0U == (emac->mac_addr_slots & (1U << slot)))
		{
			/*	Found */
			break;
		}
	}

	if (EMAC_CFG_INDIVIDUAL_ADDR_SLOTS_COUNT == slot)
	{
		return ENOSPC;
	}

	/*	Add address into the internal DB together with slot index */
	ret = pfe_emac_addr_db_add(emac, addr, FALSE, slot);
	if (EOK != ret)
	{
		return ret;
	}

	/*	Mark the slot as used */
	emac->mac_addr_slots |= (1U << slot);

	/*	Write the address to HW as individual address */
	pfe_emac_cfg_write_addr_slot(emac->emac_base_va, addr, slot);

	return EOK;
}

/**
 * @brief		Get individual MAC address of EMAC
 * @param[in]	emac The EMAC instance
 * @param[out]	addr Pointer to location where the address shall be stored
 * @retval		EOK Success
 * @retval		ENOENT No MAC address associated with the EMAC
 */
errno_t pfe_emac_get_addr(pfe_emac_t *emac, pfe_mac_addr_t addr)
{
	pfe_mac_addr_db_entry_t *entry;
	
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Return address from the 0th individual address slot */
	entry = pfe_emac_addr_db_find_by_slot(emac, 0U);
	if (NULL == entry)
	{
		/*	Individual slots are empty. Check if there is something in the hash table. */
		entry = pfe_emac_addr_db_get_first(emac);
		if (NULL == entry)
		{
			return ENOENT;
		}
	}

	memcpy(addr, entry->addr, sizeof(pfe_mac_addr_t));
	return EOK;
}

/**
 * @brief		Destroy MAC instance
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_destroy(pfe_emac_t *emac)
{
	pfe_mac_addr_t zero_addr;
	LLIST_t *item, *tmp_item;
	pfe_mac_addr_db_entry_t *entry;

	/*	Prepare zero-filled address */
	memset(zero_addr, 0, sizeof(pfe_mac_addr_t));
	
	if (NULL != emac)
	{
		/*	Remove all registered MAC addresses */
		LLIST_ForEachRemovable(item, tmp_item, &emac->mac_addr_list)
		{
			entry = LLIST_Data(item, pfe_mac_addr_db_entry_t, iterator);
			if (EOK != pfe_emac_del_addr(emac, entry->addr))
			{
				NXP_LOG_WARNING("Can't remove MAC address within the destroy function\n");
			}
		}
		
		/*	Disable traffic */
		pfe_emac_disable(emac);
		
		/*	Disable TS */
		pfe_emac_cfg_disable_ts(emac->emac_base_va);

		/*	Dispose the MAC address DB */
		pfe_emac_addr_db_drop_all(emac);
		
		/*	Destroy mutex */
		oal_mutex_destroy(&emac->mutex);
		oal_mutex_destroy(&emac->ts_mutex);
		
		/*	Release the EMAC instance */
		oal_mm_free(emac);
	}
}

/**
 * @brief		Lock access to MDIO resource
 * @details		Once locked, only lock owner can perform MDIO accesses
 * @param[in]	emac The EMAC instance
 * @param[out]	key Pointer to memory where the key to be used for access to locked MDIO and for
 * 					unlock shall be stored
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_emac_mdio_lock(pfe_emac_t *emac, uint32_t *key)
{
	errno_t ret;
	static uint32_t key_seed = 123U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == key)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (TRUE == emac->mdio_locked)
	{
		ret = EPERM;
	}
	else
	{
		/*	Perform lock + generate and store access key */
		emac->mdio_locked = TRUE;
		emac->mdio_key = key_seed++;
		*key = emac->mdio_key;
		ret = EOK;
	}

	if (EOK != oal_mutex_unlock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Unlock access to MDIO resource
 * @details		Once locked, only lock owner can perform MDIO accesses
 * @param[in]	emac The EMAC instance
 * @param[in]	key The key
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_emac_mdio_unlock(pfe_emac_t *emac, uint32_t key)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (TRUE == emac->mdio_locked)
	{
		if (key == emac->mdio_key)
		{
			emac->mdio_locked = FALSE;
			ret = EOK;
		}
		else
		{
			ret = EPERM;
		}
	}
	else
	{
		ret = ENOLCK;
	}

	if (EOK != oal_mutex_unlock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Read value from the MDIO bus using Clause 22
 * @param[in]	emac The EMAC instance
 * @param[in]	pa PHY address
 * @param[in]	ra Register address
 * @param[out]	val If success the the read value is written here
 * @param[in]	key Access key in case the resource is locked
 * @retval		EOK Success
 */
errno_t pfe_emac_mdio_read22(pfe_emac_t *emac, uint8_t pa, uint8_t ra, uint16_t *val, uint32_t key)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == val)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (TRUE == emac->mdio_locked)
	{
		/*	Locked. Check key. */
		if (key == emac->mdio_key)
		{
			ret = pfe_emac_cfg_mdio_read22(emac->emac_base_va, pa, ra, val);
		}
		else
		{
			ret = EPERM;
		}
	}
	else
	{
		/*	Unlocked. No check required. */
		ret = pfe_emac_cfg_mdio_read22(emac->emac_base_va, pa, ra, val);
	}

	if (EOK != oal_mutex_unlock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Write value to the MDIO bus using Clause 22
 * @param[in]	emac The EMAC instance
 * @param[in]	pa PHY address
 * @param[in]	ra Register address
 * @param[in]	val Value to be written
 * @param[in]	key Access key in case the resource is locked
 * @retval		EOK Success
 */
errno_t pfe_emac_mdio_write22(pfe_emac_t *emac, uint8_t pa, uint8_t ra, uint16_t val, uint32_t key)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (TRUE == emac->mdio_locked)
	{
		/*	Locked. Check key. */
		if (key == emac->mdio_key)
		{
			ret = pfe_emac_cfg_mdio_write22(emac->emac_base_va, pa, ra, val);
		}
		else
		{
			ret = EPERM;
		}
	}
	else
	{
		/*	Unlocked. No check required. */
		ret = pfe_emac_cfg_mdio_write22(emac->emac_base_va, pa, ra, val);
	}

	if (EOK != oal_mutex_unlock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Read value from the MDIO bus using Clause 45
 * @param[in]	emac The EMAC instance
 * @param[in]	pa PHY address
 * @param[in]	dev Device address 
 * @param[in]	ra Register address
 * @param[out]	val If success the the read value is written here
 * @param[in]	key Access key in case the resource is locked
 * @retval		EOK Success
 */
errno_t pfe_emac_mdio_read45(pfe_emac_t *emac, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t *val, uint32_t key)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == val)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (TRUE == emac->mdio_locked)
	{
		/*	Locked. Check key. */
		if (key == emac->mdio_key)
		{
			ret = pfe_emac_cfg_mdio_read45(emac->emac_base_va, pa, dev, ra, val);
		}
		else
		{
			ret = EPERM;
		}
	}
	else
	{
		/*	Unlocked. No check required. */
		ret = pfe_emac_cfg_mdio_read45(emac->emac_base_va, pa, dev, ra, val);
	}

	if (EOK != oal_mutex_unlock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Write value to the MDIO bus using Clause 45
 * @param[in]	emac The EMAC instance
 * @param[in]	pa PHY address
 * @param[in]	dev Device address
 * @param[in]	ra Register address
 * @param[in]	val Value to be written
 * @param[in]	key Access key in case the resource is locked
 * @retval		EOK Success
 */
errno_t pfe_emac_mdio_write45(pfe_emac_t *emac, uint8_t pa, uint8_t dev, uint16_t ra, uint16_t val, uint32_t key)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != oal_mutex_lock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex lock failed\n");
	}

	if (TRUE == emac->mdio_locked)
	{
		/*	Locked. Check key. */
		if (key == emac->mdio_key)
		{
			ret = pfe_emac_cfg_mdio_write45(emac->emac_base_va, pa, dev, ra, val);
		}
		else
		{
			ret = EPERM;
		}
	}
	else
	{
		/*	Unlocked. No check required. */
		ret = pfe_emac_cfg_mdio_write45(emac->emac_base_va, pa, dev, ra, val);
	}

	if (EOK != oal_mutex_unlock(&emac->mutex))
	{
		NXP_LOG_DEBUG("Mutex unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Return EMAC runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	gpi 		The EMAC instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_emac_get_text_statistics(pfe_emac_t *emac, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */
	len += pfe_emac_cfg_get_text_stat(emac->emac_base_va, buf + len, buf_len - len, verb_level);

	return len;
}
