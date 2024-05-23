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

#include "linked_list.h"
#include "pfe_platform_cfg.h"
#include "pfe_cbus.h"
#include "pfe_emac_csr.h"
#include "pfe_emac.h"

struct pfe_emac_tag
{
	addr_t cbus_base_va;			/*	CBUS base virtual address */
	addr_t emac_base_offset;		/*	MAC base offset within CBUS space */
	addr_t emac_base_va;			/*	MAC base address (virtual) */
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
	bool_t ext_ts;				/*  IEEE1588 external timestamp mode */
	pfe_gpi_t *gpi;				/* gpi handle, to export gpi services for this emac instance */
};

typedef struct
{
	pfe_mac_addr_t addr;		/*	The MAC address */
	uint32_t hash;				/*	Associated hash value (valid if in_hash_grp is TRUE) */
	bool_t in_hash_grp;			/*	If TRUE then the address belongs to a hash group */
	uint8_t addr_slot_idx;		/*	If 'in_hash_grp' is FALSE then this value specifies index of
									individual address slot the address is stored in */
	LLIST_t iterator;			/*	List chain entry */
	pfe_drv_id_t owner;	/*	Identification of the driver that owns this entry */
} pfe_mac_addr_db_entry_t;

/* usage scope: pfe_emac_mdio_lock */
static uint32_t key_seed = 123U;

static bool_t pfe_emac_flush_criterion_eval(const pfe_mac_addr_db_entry_t *entry, pfe_emac_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner);
static void pfe_emac_addr_db_init(pfe_emac_t *emac);
static errno_t pfe_emac_addr_db_add(pfe_emac_t *emac, const pfe_mac_addr_t addr, bool_t in_hash_grp, uint32_t data, pfe_drv_id_t owner);
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_hash(const pfe_emac_t *emac, uint32_t hash, uint32_t hw_mask);
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_addr(const pfe_emac_t *emac, const pfe_mac_addr_t addr, pfe_drv_id_t owner);
static errno_t pfe_emac_addr_db_del_entry(const pfe_emac_t *emac, pfe_mac_addr_db_entry_t *entry);
static void pfe_emac_addr_db_drop_all(const pfe_emac_t *emac);
static errno_t pfe_emac_del_addr_nolock(pfe_emac_t *emac, const pfe_mac_addr_t addr, pfe_drv_id_t owner);
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_get_first(const pfe_emac_t *emac);
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_slot(const pfe_emac_t *emac, uint8_t slot);

/**
 * @brief		Evaluate given DB entry against specified criterion
 * @param[in]	entry DB entry to evaluate
 * @param[in]	crit All, Owner, Type or Owner&Type criterion
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @param[in]	owner Required owner of MAC address
 * @return		TRUE if entry does match with criterion, FALSE otherwise
 */
static bool_t pfe_emac_flush_criterion_eval(const pfe_mac_addr_db_entry_t *entry, pfe_emac_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner)
{
	bool_t ret = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (crit == EMAC_CRIT_BY_OWNER)
		{
			/* Return the first address where owner match */
			if (entry->owner == owner)
			{
				/* Break if entry match with the rule */
				ret = TRUE;
			}
		}
		else if (crit == EMAC_CRIT_BY_TYPE)
		{
			/* Break if entry match with the rule */
			ret = pfe_emac_check_crit_by_type(entry->addr, type);
		}
		else if (crit == EMAC_CRIT_BY_OWNER_AND_TYPE)
		{
			if (entry->owner == owner)
			{
				/* Break if entry match with the rule */
				ret = pfe_emac_check_crit_by_type(entry->addr, type);
			}
		}
		else if (crit == EMAC_CRIT_ALL)
		{
			/* Break if entry match with the rule */
			ret = TRUE;
		}
		else
		{
			NXP_LOG_WARNING("Unknown criterion\n");
		}
	}
	return ret;
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
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_Init(&emac->mac_addr_list);
	}
}

/**
 * @brief		Add MAC address to internal DB
 * @details		Access to the shared resources => needs to be called within the critical section!
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to add
 * @param[in]	in_hash_grp TRUE if the address is matched by EMAC by its hash
 * @param[in]	data Address slot index associated with the new entry. Only valid when in_hash_grp==FALSE.
 * 					 When in_hash_grp==TRUE this is the hash value.
 */
static errno_t pfe_emac_addr_db_add(pfe_emac_t *emac, const pfe_mac_addr_t addr, bool_t in_hash_grp, uint32_t data, pfe_drv_id_t owner)
{
	pfe_mac_addr_db_entry_t *entry;
	errno_t                  ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Create new list entry */
#ifdef PFE_CFG_TARGET_OS_LINUX
		entry = kzalloc(sizeof(pfe_mac_addr_db_entry_t), GFP_ATOMIC); /* temporary fix for AAVB-3946 */
#else
		entry = oal_mm_malloc(sizeof(pfe_mac_addr_db_entry_t));
#endif
		if (NULL == entry)
		{
			NXP_LOG_ERROR("oal_mm_malloc() failed\n");
			ret = ENOMEM;
		}
		else
		{

			if (in_hash_grp)
			{
				entry->hash = data;
				entry->in_hash_grp = TRUE;
			}
			else
			{
				entry->addr_slot_idx = (uint8_t)data;
				entry->in_hash_grp = FALSE;
			}

			entry->owner = owner;

			/*	Add entry to the list */
			(void)memcpy(entry->addr, addr, sizeof(pfe_mac_addr_t));

			LLIST_AddAtEnd(&entry->iterator, &emac->mac_addr_list);
			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Remove MAC address from internal DB
 * @details		Access to the shared resources => needs to be called within the critical section!
 * @param[in]	emac The EMAC instance
 * @param[in]	entry The DB entry to delete
 * @retval		EOK Success
 * @retval		EINVAL Entry is NULL
 */
static errno_t pfe_emac_addr_db_del_entry(const pfe_emac_t *emac, pfe_mac_addr_db_entry_t *entry)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#else
	(void)emac;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_Remove(&entry->iterator);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief		Drop all entries from the DB
 * @details		Access to the shared resources => needs to be called within the critical section!
 * @param[in]	emac The EMAC instance
 */
static void pfe_emac_addr_db_drop_all(const pfe_emac_t *emac)
{
	pfe_mac_addr_db_entry_t *entry;
	LLIST_t *curItem, *tmp_item;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Release the MAC address DB */
		LLIST_ForEachRemovable(curItem, tmp_item, &emac->mac_addr_list)
		{
			entry = LLIST_Data(curItem, pfe_mac_addr_db_entry_t, iterator);
			LLIST_Remove(&entry->iterator);
			oal_mm_free(entry);
		}
	}
}

/**
 * @brief		Search a MAC address within internal DB of registered addresses
 * @details		Access to the shared resources => needs to be called within the critical section!
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to search
 * @param[in]	owner The identification of driver instance
 * @return		The DB entry if found or NULL if address is not present
 */
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_addr(const pfe_emac_t *emac, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	pfe_mac_addr_db_entry_t *entry = NULL;
	LLIST_t *                curItem;
	bool_t                   match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		entry = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_ForEach(curItem, &emac->mac_addr_list)
		{
			entry = LLIST_Data(curItem, pfe_mac_addr_db_entry_t, iterator);
			if ((entry->owner == owner) && (0 == memcmp(addr, entry->addr, sizeof(pfe_mac_addr_t))))
			{
				match = TRUE;
				break;
			}
		}
	}

	if (FALSE == match)
	{
		entry = NULL;
	}

	return entry;
}

/**
 * @brief		Get the very first address from the DB
 * @details		Access to the shared resources => needs to be called within the critical section!
 * @param[in]	emac The EMAC instance
 * @return		The DB entry if found or NULL if address is not present
 */
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_get_first(const pfe_emac_t *emac)
{
	pfe_mac_addr_db_entry_t *db_entry;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		db_entry = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (TRUE == LLIST_IsEmpty(&emac->mac_addr_list))
		{
			db_entry = NULL;
		}
		else
		{
			db_entry = LLIST_Data(emac->mac_addr_list.prNext, pfe_mac_addr_db_entry_t, iterator);
		}
	}

	return db_entry;
}

/**
 * @brief		Search a MAC address within internal DB of registered addresses based on hash value
 * @details		Access to the shared resources => needs to be called within the critical section!
 * @param[in]	emac The EMAC instance
 * @param[in]	hash The hash to search
 * @param[in]	hw_mask A mask to select the hash bits that are actually used by the HW device that stores
 *			the hashed MAC addresses
 * @return		The DB entry if found or NULL if address is not present
 */
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_hash(const pfe_emac_t *emac, uint32_t hash, uint32_t hw_mask)
{
	pfe_mac_addr_db_entry_t *entry = NULL;
	LLIST_t *                curItem;
	bool_t                   match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		entry = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_ForEach(curItem, &emac->mac_addr_list)
		{
			entry = LLIST_Data(curItem, pfe_mac_addr_db_entry_t, iterator);
			if ((entry->hash & hw_mask) == (hash & hw_mask))
			{
				match = TRUE;
				break;
			}
		}
	}

	if (FALSE == match)
	{
		entry = NULL;
	}
	return entry;
}

/**
 * @brief		Search a MAC address within internal DB of registered addresses based on slot index
 * @details		Access to the shared resources => needs to be called within the critical section!
* @param[in]	emac The EMAC instance
 * @param[in]	slot The slot index to search
 * @return		The DB entry if found or NULL if address is not present
 */
static pfe_mac_addr_db_entry_t *pfe_emac_addr_db_find_by_slot(const pfe_emac_t *emac, uint8_t slot)
{
	pfe_mac_addr_db_entry_t *entry = NULL;
	bool_t                   match = FALSE;
	LLIST_t *curItem;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		entry = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_ForEach(curItem, &emac->mac_addr_list)
		{
			entry = LLIST_Data(curItem, pfe_mac_addr_db_entry_t, iterator);
			if (entry->addr_slot_idx == slot)
			{
				match = TRUE;
				break;
			}
		}
	}

	if (FALSE == match)
	{
		entry = NULL;
	}
	return entry;
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
pfe_emac_t *pfe_emac_create(addr_t cbus_base_va, addr_t emac_base, pfe_emac_mii_mode_t mode, pfe_emac_speed_t speed, pfe_emac_duplex_t duplex)
{
	pfe_emac_t *emac;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		emac = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		emac = oal_mm_malloc(sizeof(pfe_emac_t));

		if (NULL == emac)
		{
			NXP_LOG_ERROR("Unable to allocate memory\n");
		}
		else
		{
			(void)memset(emac, 0, sizeof(pfe_emac_t));
			emac->cbus_base_va = cbus_base_va;
			emac->emac_base_offset = emac_base;
			emac->emac_base_va = (emac->cbus_base_va + emac->emac_base_offset);
			emac->mode = EMAC_MODE_INVALID;
			emac->speed = EMAC_SPEED_INVALID;
			emac->duplex = EMAC_DUPLEX_INVALID;

			if (EOK != oal_mutex_init(&emac->mutex))
			{
				NXP_LOG_ERROR("Mutex init failed\n");
				oal_mm_free(emac);
				emac = NULL;
			}
			else
			{

				if (EOK != oal_mutex_init(&emac->ts_mutex))
				{
					NXP_LOG_ERROR("TS mutex init failed\n");
					(void)oal_mutex_destroy(&emac->mutex);
					oal_mm_free(emac);
					emac = NULL;
				}
				else
				{
					if (EOK != oal_mutex_lock(&emac->mutex))
					{
						NXP_LOG_ERROR("Mutex lock failed\n");
					}

					/*	All slots are free */
					emac->mac_addr_slots = 0U;

					/*	Initialize the MAC address DB */
					pfe_emac_addr_db_init(emac);

            		/* ERR050221: to ensure parity_en is set before any registers READ to MAC */
            		pfe_emac_cfg_pre_init(emac->emac_base_va);

					/*	Disable the HW */
					pfe_emac_disable(emac);

					/*	Initialize the HW */
					if (EOK != pfe_emac_cfg_init(emac->emac_base_va, mode, speed, duplex))
					{
						if (EOK != oal_mutex_unlock(&emac->mutex))
						{
							NXP_LOG_ERROR("Mutex unlock failed\n");
						}

						/*	Invalid configuration */
						NXP_LOG_ERROR("Invalid configuration requested\n");
						(void)oal_mutex_destroy(&emac->mutex);
						(void)oal_mutex_destroy(&emac->ts_mutex);
						oal_mm_free(emac);
						emac = NULL;
					}
					else
					{
						emac->mode = mode;
						emac->speed = speed;
						emac->duplex = duplex;

						/*	Disable loop-back */
						pfe_emac_disable_loopback(emac);

						/*	Disable promiscuous mode */
						pfe_emac_disable_promisc_mode(emac);

						/*	Disable broadcast */
						pfe_emac_disable_broadcast(emac);

						if (EOK != oal_mutex_unlock(&emac->mutex))
						{
							NXP_LOG_ERROR("Mutex unlock failed\n");
						}
					}
				}
			}
		}
	}

	return emac;
}

/**
 * @brief		Get EMAC instance index
 * @param[in]	emac The EMAC instance
 * @return		Index (0, 1, 2, ..) or 255 if failed
 */
uint8_t pfe_emac_get_index(const pfe_emac_t *emac)
{
	uint8_t emac_idx;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		emac_idx = 255U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		emac_idx = pfe_emac_cfg_get_index(emac->emac_base_va, emac->cbus_base_va);
	}
	return emac_idx;
}

errno_t pfe_emac_bind_gpi(pfe_emac_t *emac, pfe_gpi_t *gpi)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == gpi)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		emac->gpi = gpi;
		ret = EOK;
	}

	return ret;
}

pfe_gpi_t *pfe_emac_get_gpi(const pfe_emac_t *emac)
{
	return emac->gpi;
}

/**
 * @brief		Enable the EMAC
 * @details		Data transmission/reception is possible after this call
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_enable(emac->emac_base_va, TRUE);
	}
}

/**
 * @brief		Disable the EMAC
 * @details		No data transmission/reception is possible after this call
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_enable(emac->emac_base_va, FALSE);
	}
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
		ret = EINVAL;
	}
	else
	{
		emac->i_clk_hz = i_clk_hz;
		emac->o_clk_hz = o_clk_hz;
		emac->ext_ts = eclk;

		oal_mutex_lock_sleep(&emac->ts_mutex);

		ret = pfe_emac_cfg_enable_ts(emac->emac_base_va, eclk, i_clk_hz, o_clk_hz);

		oal_mutex_unlock(&emac->ts_mutex);
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

	oal_mutex_lock_sleep(&emac->ts_mutex);

	if (TRUE == emac->ext_ts)
	{
		NXP_LOG_DEBUG("Cannot adjust timestamping clock frequency on EMAC%u working in external timestamp mode\n", pfe_emac_get_index(emac));
		ret = EPERM;
	}
	else
	{
		emac->adj_ppb = ppb;
		emac->adj_sign = sgn;
		ret = pfe_emac_cfg_adjust_ts_freq(emac->emac_base_va, emac->i_clk_hz, emac->o_clk_hz, ppb, sgn);
	}

	oal_mutex_unlock(&emac->ts_mutex);

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
	errno_t ret;
	if ((NULL == ppb) || (NULL == sgn))
	{
		ret = EINVAL;
	}
	else
	{
		oal_mutex_lock_sleep(&emac->ts_mutex);

		*ppb = emac->adj_ppb;
		*sgn = emac->adj_sign;

		oal_mutex_unlock(&emac->ts_mutex);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief			Get current time
 * @param[in]		emac THe EMAC instance
 * @param[in,out]	sec Pointer where seconds value shall be written
 * @param[in,out]	nsec Pointer where nano-seconds value shall be written
 * @param[in,out]	sec_hi Pointer where higher-word-seconds value shall be written
 * @return			EOK if success, error code otherwise
 */
errno_t pfe_emac_get_ts_time(pfe_emac_t *emac, uint32_t *sec, uint32_t *nsec, uint16_t *sec_hi)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if ((NULL == sec) || (NULL == nsec) || (NULL == sec_hi))
		{
			ret = EINVAL;
		}
		else
		{
			oal_mutex_lock_sleep(&emac->ts_mutex);

			pfe_emac_cfg_get_ts_time(emac->emac_base_va, sec, nsec, sec_hi);

			oal_mutex_unlock(&emac->ts_mutex);
			ret = EOK;
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

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		oal_mutex_lock_sleep(&emac->ts_mutex);

		if (TRUE == emac->ext_ts)
		{
			NXP_LOG_DEBUG("Cannot adjust timestamping time on EMAC%u working in external timestamp mode\n", pfe_emac_get_index(emac));
			ret = EPERM;
		}
		else
		{
			ret = pfe_emac_cfg_adjust_ts_time(emac->emac_base_va, sec, nsec, sgn);
		}

		oal_mutex_unlock(&emac->ts_mutex);
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
 * @param[in]	sec_hi New higher-word-seconds value
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_emac_set_ts_time(pfe_emac_t *emac, uint32_t sec, uint32_t nsec, uint16_t sec_hi)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		oal_mutex_lock_sleep(&emac->ts_mutex);

		if (TRUE == emac->ext_ts)
		{
			NXP_LOG_DEBUG("Cannot set timestamping time on EMAC%u working in external timestamp mode\n", pfe_emac_get_index(emac));
			ret = EPERM;
		}
		else
		{
			ret = pfe_emac_cfg_set_ts_time(emac->emac_base_va, sec, nsec, sec_hi);
		}

		oal_mutex_unlock(&emac->ts_mutex);
	}

	return ret;
}

/**
 * @brief		Enable the local loop-back mode
 * @details		This function controls the EMAC internal loop-back mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable_loopback(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_loopback(emac->emac_base_va, TRUE);
	}
}

/**
 * @brief		Disable loop-back mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable_loopback(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_loopback(emac->emac_base_va, FALSE);
	}
}

/**
 * @brief		Enable promiscuous mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable_promisc_mode(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_promisc_mode(emac->emac_base_va, TRUE);
	}
}

/**
 * @brief		Disable promiscuous mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable_promisc_mode(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_promisc_mode(emac->emac_base_va, FALSE);
	}
}

/**
 * @brief		Enable ALLMULTI mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable_allmulti_mode(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_allmulti_mode(emac->emac_base_va, TRUE);
	}
}

/**
 * @brief		Disable ALLMULTI mode
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable_allmulti_mode(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_allmulti_mode(emac->emac_base_va, FALSE);
	}
}

/**
 * @brief		Enable broadcast reception
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_enable_broadcast(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_broadcast(emac->emac_base_va, TRUE);
	}
}

/**
 * @brief		Disable broadcast reception
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_disable_broadcast(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_broadcast(emac->emac_base_va, FALSE);
	}
}

void pfe_emac_get_flow_control(const pfe_emac_t *emac, bool_t *tx_enable, bool_t *rx_enable)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac) || unlikely(NULL == tx_enable) ||
		unlikely(NULL == rx_enable))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_get_tx_flow_control(emac->emac_base_va,tx_enable);
		pfe_emac_cfg_get_rx_flow_control(emac->emac_base_va,rx_enable);
	}
}

/**
 * @brief		Enable tx flow control
 * @details		Enables PAUSE frames processing
 * @param		emac The EMAC instance
 */
void pfe_emac_enable_tx_flow_control(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		pfe_emac_cfg_set_tx_flow_control(emac->emac_base_va, TRUE);
	}
}

/**
 * @brief		Disable tx flow control
 * @details		Disables PAUSE frames processing
 * @param		emac The EMAC instance
 */
void pfe_emac_disable_tx_flow_control(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
		pfe_emac_cfg_set_tx_flow_control (emac->emac_base_va, FALSE);
    }
}

/**
 * @brief               Enable rx flow control
 * @details             Enables PAUSE frames processing
 * @param               emac The EMAC instance
 */
void pfe_emac_enable_rx_flow_control(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
        pfe_emac_cfg_set_rx_flow_control(emac->emac_base_va, TRUE);
	}
}

/**
 * @brief               Disable rx flow control
 * @details             Disables PAUSE frames processing
 * @param               emac The EMAC instance
 */
void pfe_emac_disable_rx_flow_control(const pfe_emac_t *emac)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
        pfe_emac_cfg_set_rx_flow_control(emac->emac_base_va, FALSE);
	}
}


/**
 * @brief		Set maximum frame length
 * @param		emac The EMAC instance
 * @param		len New frame length
 * @return		EOK if success errno otherwise
 */
errno_t pfe_emac_set_max_frame_length(const pfe_emac_t *emac, uint32_t len)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_emac_cfg_set_max_frame_length(emac->emac_base_va, len);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Attempt to set unsupported frame length value\n");
		}
	}
	return ret;
}

/**
 * @brief		Get current MII mode
 * @param[in]	emac The EMAC instance
 * @return		Currently configured MII mode @see pfe_emac_mii_mode_t
 */
pfe_emac_mii_mode_t pfe_emac_get_mii_mode(const pfe_emac_t *emac)
{
	pfe_emac_mii_mode_t mii_mode;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		mii_mode = EMAC_MODE_INVALID;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		mii_mode = emac->mode;
	}
	return mii_mode;
}

/**
 * @brief		Get the EMAC link configuration
 * @param[in]	emac The EMAC instance
 * @param[out]	speed The EMAC speed configuration @see pfe_emac_speed_t
 * @param[out]	duplex The EMAC duplex configuration @see pfe_emac_duplex_t
 * @return		EOK if success
 */
errno_t pfe_emac_get_link_config(const pfe_emac_t *emac, pfe_emac_speed_t *speed, pfe_emac_duplex_t *duplex)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_emac_cfg_get_link_config(emac->emac_base_va, speed, duplex);
	}

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
errno_t pfe_emac_get_link_status(const pfe_emac_t *emac, pfe_emac_link_speed_t *link_speed, pfe_emac_duplex_t *duplex, bool_t *link)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_emac_cfg_get_link_status(emac->emac_base_va, link_speed, duplex, link);
	}
	return ret;
}

/**
 * @brief		Set the EMAC link speed
 * @param[in]	emac The EMAC instance
 * @param[in]	link_speed The EMAC link speed @see pfe_emac_link_speed_t
 * @return		EOK if success
 * @details		This function can be used for runtime changes of speed (eg. after auto-negotiation).
 */
errno_t pfe_emac_set_link_speed(const pfe_emac_t *emac, pfe_emac_speed_t link_speed)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_emac_cfg_set_speed(emac->emac_base_va, link_speed);
	}

	return ret;
}

/**
 * @brief		Set the EMAC link duplex
 * @param[in]	emac The EMAC instance
 * @param[in]	duplex The EMAC duplex @see pfe_emac_duplex_t
 * @return		EOK if success
 * @details		This function can be used for runtime changes of duplex (eg. after auto-negotiation).
 */
errno_t pfe_emac_set_link_duplex(const pfe_emac_t *emac, pfe_emac_duplex_t duplex)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_emac_cfg_set_duplex(emac->emac_base_va, duplex);
	}

	return ret;
}

/**
 * @brief		Delete MAC addresses added by owner with defined type
 * @param[in]	emac The EMAC instance
 * @param[in]	crit All, Owner, Type or Owner&Type criterion
 * @param[in]	type Required type of MAC address (Broadcast, Multicast, Unicast, ANY) criterion
 * @param[in]	owner The identification of driver instance
 * @return		EOK if success
 * @return		EINVAL Requested address is broadcast
 * @note		Must not be preempted by: pfe_emac_del_addr(), pfe_emac_add_addr(), pfe_emac_destroy()
 */
errno_t pfe_emac_flush_mac_addrs(pfe_emac_t *emac, pfe_emac_crit_t crit, pfe_mac_type_t type, pfe_drv_id_t owner)
{
	const pfe_mac_addr_db_entry_t *entry = NULL;
	LLIST_t *item, *tmp_item;
	errno_t ret = EOK;
	pfe_mac_addr_t addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		LLIST_ForEachRemovable(item, tmp_item, &emac->mac_addr_list)
		{
			entry = LLIST_Data(item, pfe_mac_addr_db_entry_t, iterator);
			if ((NULL != entry) && (entry->owner == owner))
			{
				(void)memcpy(addr, entry->addr, sizeof(pfe_mac_addr_t));
				if (TRUE == pfe_emac_flush_criterion_eval(entry, crit, type, owner))
				{
					ret = pfe_emac_del_addr_nolock(emac, entry->addr, entry->owner);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Can't remove MAC address within the flush function\n");
						break;
					}
					else
					{
						NXP_LOG_DEBUG("Address %02x:%02x:%02x:%02x:%02x:%02x removed from owner ID %d\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], owner);
					}
				}
			}
			else
			{
				;
			}
		}

		if (EOK != oal_mutex_unlock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Remove MAC address from EMAC
 * @details		Address resolution will be done using exact match with the added address
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to delete
 * @param[in]	owner The identification of driver instance
 * @retval		EOK Success
 * @retval		ENOENT Address not found
 * @note		Must not be preempted by: pfe_emac_add_addr(), pfe_emac_destroy()
 */
errno_t pfe_emac_del_addr(pfe_emac_t *emac, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		ret = pfe_emac_del_addr_nolock(emac, addr, owner);

		if (EOK != oal_mutex_unlock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Remove MAC address from EMAC without entering the critical section
 * @details		Address resolution will be done using exact match with the added address
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to delete
 * @param[in]	owner The identification of driver instance
 * @retval		EOK Success
 * @retval		ENOENT Address not found
 * @note		Must not be preempted by: pfe_emac_add_addr(), pfe_emac_destroy()
 */
static errno_t pfe_emac_del_addr_nolock(pfe_emac_t *emac, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	pfe_mac_addr_db_entry_t *entry, local_entry;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Get address entry from the internal DB */
		entry = pfe_emac_addr_db_find_by_addr(emac, addr, owner);

		if (NULL == entry)
		{
			ret = ENOENT;
		}
		else
		{
			/*	Remember the entry */
			local_entry = *entry;

			/*	Remove the entry form DB */
			ret = pfe_emac_addr_db_del_entry(emac, entry);

			if (EOK == ret)
			{
				/*	Release the entry */
				oal_mm_free(entry);
				entry = NULL;

				/* eventhought the entry pointer is freed the local_entry still store the value of the entry pointer */
				if (TRUE == local_entry.in_hash_grp)
				{
					/*	Check if the hash group the address belongs to contains another addresses */
					if (NULL != pfe_emac_addr_db_find_by_hash(emac, local_entry.hash, EMAC_CFG_ADDR_HASH_MASK))
					{
						/*	Hash group contains more addresses. Keep the HW configured. */
						;
					}
					else
					{
						/*	Configure the HW */
						pfe_emac_cfg_set_hash_group(emac->emac_base_va, local_entry.hash, FALSE);
					}
				}
				else
				{
					pfe_mac_addr_t zero_addr;

					/*	Prepare zero-filled address */
					(void)memset(zero_addr, 0, sizeof(pfe_mac_addr_t));

					/*	Clear the specific slot */
					pfe_emac_cfg_write_addr_slot(emac->emac_base_va, zero_addr, local_entry.addr_slot_idx);

					/*	Mark the slot as unused */
					emac->mac_addr_slots &= ~(1U << local_entry.addr_slot_idx);
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Assign an individual MAC address to EMAC
 * @param[in]	emac The EMAC instance
 * @param[in]	addr The MAC address to add
 * @retval		EOK Success
 * @retval		ENOSPC The EMAC has not a free address slot
 * @retval		ENOMEM Not enough memory
 * @retval		EEXIST Address already added
 * @note		Must not be preempted by: pfe_emac_del_addr(), pfe_emac_destroy()
 */
errno_t pfe_emac_add_addr(pfe_emac_t *emac, const pfe_mac_addr_t addr, pfe_drv_id_t owner)
{
	uint32_t slot;
	errno_t ret;
	const pfe_mac_addr_db_entry_t *entry;
	int32_t hash;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		/*	Check if address is already registered */
		entry = pfe_emac_addr_db_find_by_addr(emac, addr, owner);

		if (NULL != entry)
		{
			/*	Duplicates are not allowed */
			ret = EEXIST;
		}
		else
		{
			/*	Try to get free individual address slot */
			for (slot=0U; slot<EMAC_CFG_INDIVIDUAL_ADDR_SLOTS_COUNT; slot++)
			{
				if (0U == (emac->mac_addr_slots & (1U << slot)))
				{
					/*	Found */
					break;
				}
			}

			/* Slots are full, add hash of the address into the hash table */
			if (EMAC_CFG_INDIVIDUAL_ADDR_SLOTS_COUNT == slot)
			{
				if (pfe_emac_is_broad(addr))
				{
					/*	Can't add broadcast address */
					ret = EINVAL;
				}
				else
				{

					/*	Get the hash */
					hash = pfe_emac_cfg_get_hash(emac->emac_base_va, addr);

					/*	Store address into EMAC's internal DB together with 'in_hash_grp' flag and hash */
					ret = pfe_emac_addr_db_add(emac, addr, TRUE, hash, owner);

					if (EOK == ret)
					{
						/*	Configure the HW */
						pfe_emac_cfg_set_hash_group(emac->emac_base_va, hash, TRUE);
					}
				}
			}
			/* There is free address slot, use it */
			else
			{
				/*	Add address into the internal DB together with slot index */
				ret = pfe_emac_addr_db_add(emac, addr, FALSE, slot, owner);

				if (EOK == ret)
				{
					/*	Mark the slot as used */
					emac->mac_addr_slots |= (1U << slot);

					/*	Write the address to HW as individual address */
					pfe_emac_cfg_write_addr_slot(emac->emac_base_va, addr, (uint8_t)slot);
				}
			}
		}

		if (EOK != oal_mutex_unlock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}

	return ret;
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
	const pfe_mac_addr_db_entry_t *entry;
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		/*	Return address from the 0th individual address slot */
		entry = pfe_emac_addr_db_find_by_slot(emac, 0U);
		if (NULL == entry)
		{
			/*	Individual slots are empty. Check if there is something in the hash table. */
			entry = pfe_emac_addr_db_get_first(emac);
			if (NULL == entry)
			{
				ret = ENOENT;
			}
		}

		if (EOK == ret)
		{
			(void)memcpy(addr, entry->addr, sizeof(pfe_mac_addr_t));
		}

		if (EOK != oal_mutex_unlock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Destroy MAC instance
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_destroy(pfe_emac_t *emac)
{
	pfe_mac_addr_t zero_addr;
	LLIST_t *curItem, *tmp_item;
	const pfe_mac_addr_db_entry_t *entry;

	/*	Prepare zero-filled address */
	(void)memset(zero_addr, 0, sizeof(pfe_mac_addr_t));

	if (NULL != emac)
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		/*	Remove all registered MAC addresses */
		LLIST_ForEachRemovable(curItem, tmp_item, &emac->mac_addr_list)
		{
			entry = LLIST_Data(curItem, pfe_mac_addr_db_entry_t, iterator);
			if (EOK != pfe_emac_del_addr_nolock(emac, entry->addr, entry->owner))
			{
				NXP_LOG_ERROR("Can't remove MAC address within the destroy function\n");
			}
		}

		/*	Disable traffic */
		pfe_emac_disable(emac);

		/*	Disable TS */
		pfe_emac_cfg_disable_ts(emac->emac_base_va);

		/*	Dispose the MAC address DB */
		pfe_emac_addr_db_drop_all(emac);

		if (EOK != oal_mutex_unlock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		/*	Destroy mutex */
		(void)oal_mutex_destroy(&emac->mutex);
		(void)oal_mutex_destroy(&emac->ts_mutex);

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

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == key)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		if (TRUE == emac->mdio_locked)
		{
			ret = EPERM;
		}
		else
		{
			/*	Perform lock + generate and store access key */
			emac->mdio_locked = TRUE;
			emac->mdio_key = key_seed;
			key_seed++;
			*key = emac->mdio_key;
			ret = EOK;
		}

		if (EOK != oal_mutex_unlock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
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
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
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
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
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
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
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
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
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
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
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
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
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
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
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
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
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
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
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
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Get number of received packets
 * @param[in]	emac The EMAC instance
 * @return		Number of received packets
 */
uint32_t pfe_emac_get_rx_cnt(const pfe_emac_t *emac)
{
	uint32_t rx_cnt;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		rx_cnt = 0xFFFFFFFFU;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		rx_cnt = pfe_emac_cfg_get_rx_cnt(emac->emac_base_va);
	}
	return rx_cnt;
}

/**
 * @brief		Get number of transmitted packets
 * @param[in]	emac The EMAC instance
 * @return		Number of transmitted packets
 */
uint32_t pfe_emac_get_tx_cnt(const pfe_emac_t *emac)
{
	uint32_t tx_cnt;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		tx_cnt = 0xFFFFFFFFU;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		tx_cnt = pfe_emac_cfg_get_tx_cnt(emac->emac_base_va);
	}
	return tx_cnt;
}

/**
 * @brief		Return EMAC runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	gpi 		The EMAC instance
 * @param[in]	seq			Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_emac_get_text_statistics(const pfe_emac_t *emac, struct seq_file *seq, uint8_t verb_level)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */

	pfe_emac_cfg_get_text_stat(emac->emac_base_va, seq, verb_level);

	return 0;
}

/**
 * @brief		Get EMAC statistic in numeric form
 * @details		This is a HW-specific function providing single statistic
 * 				value from the EMAC block.
 * @param[in]	emac		The EMAC instance
 * @param[in]	stat_id		ID of required statistic (offset of register)
 * @return		Value of requested statistic
 */
uint32_t pfe_emac_get_stat_value(const pfe_emac_t *emac, uint32_t stat_id)
{
	uint32_t stat_value;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		stat_value = 0xFFFFFFFFU;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		stat_value = pfe_emac_cfg_get_stat_value(emac->emac_base_va, stat_id);
	}
	return stat_value;
}

/**
 * @brief		EMAC ISR
 * @param[in]	emac The EMAC instance
 */
errno_t pfe_emac_isr(pfe_emac_t *emac)
{
	errno_t ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
		ret = pfe_emac_cfg_isr(emac->emac_base_va, emac->cbus_base_va);
		if (EOK != oal_mutex_unlock(&emac->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}

	return ret;
}

/**
 * @brief		Mask EMAC interrupts
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_irq_mask(pfe_emac_t *emac)
{
	(void)emac;
}

/**
 * @brief		Unmask EMAC interrupts
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_irq_unmask(pfe_emac_t *emac)
{
	(void)emac;
}

