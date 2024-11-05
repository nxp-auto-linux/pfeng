/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @file		pfe_l2br.c
 * @brief		The L2 bridge module source file.
 * @details		This file contains L2 bridge-related functionality.
 *
 * 				The bridge consists of multiple bridge domains:
 * 				1.) The default domain
 * 				2.) Set of particular standard VLAN domains
 * 				3.) The fall-back domain
 *
 * 				The default domain
 * 				------------------
 * 				Default domain is used by the classification process when packet
 * 				without VLAN tag has been received and hardware assigned a default
 * 				VLAN ID.
 *
 * 				The standard domain
 * 				-------------------
 * 				Standard VLAN domain. Specifies what to do when packet with VLAN
 *				ID matching the domain is received.
 *
 * 				The fall-back domain
 * 				-------------------
 * 				This domain is used when packet with unknown VLAN ID (does not match
 * 				any standard domain) is received.
 *
 */

#include "pfe_cfg.h"
#include "linked_list.h"

#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_l2br_table.h"
#include "pfe_feature_mgr.h"
#include "pfe_l2br.h"

/**
 * @brief	The L2 Bridge instance structure
 */
struct __pfe_l2br_tag
{
	pfe_class_t *class;
	pfe_l2br_table_t *mac_table;
	pfe_l2br_table_t *vlan_table;
	pfe_l2br_domain_t *default_domain;
	pfe_l2br_domain_t *fallback_domain;
	LLIST_t domains;							/*!< List of standard domains */
	uint32_t domain_stats_table_addr;
	uint16_t domain_stats_table_size;
	LLIST_t static_entries;						/*!< List of static entries */
	uint16_t def_vlan;							/*!< Default VLAN */
	uint32_t dmem_fb_bd_base;					/*!< Address within classifier memory where the fall-back bridge domain structure is located */
	uint32_t dmem_def_bd_base;					/*!< Address within classifier memory where the default bridge domain structure is located */
	uint32_t dmem_vlan_hash_base;
	oal_mutex_t *mutex;							/*!< Mutex protecting shared resources */
	pfe_l2br_domain_get_crit_t cur_crit;		/*!< Current 'get' criterion (to get domains) */
	pfe_l2br_static_ent_get_crit_t cur_crit_ent;/*!< Current 'get' criterion (to get static entry) */
	LLIST_t *curr_domain;						/*!< The current domain list item */
	LLIST_t *curr_static_ent;					/*!< The current static entry list item */
	union
	{
		uint16_t vlan;
		pfe_phy_if_t *phy_if;
	} cur_domain_crit_arg;								/*!< Current domain criterion argument */
	struct
	{
		uint16_t vlan;
		pfe_mac_addr_t mac;
	} cur_static_ent_crit_arg;							/*!< Current static entry argument */
};

/**
 * @brief	The L2 Bridge Domain representation type
 */
struct __pfe_l2br_domain_tag
{
	uint16_t vlan;
	uint8_t  stats_index;
	union
	{
		pfe_ct_vlan_table_result_t action_data;
		uint64_t action_data_u64val;
	} u;

	pfe_l2br_table_entry_t *vlan_entry;			/*!< This is entry within VLAN table representing the domain */
	pfe_l2br_t *bridge;
	bool_t is_default;							/*!< If TRUE then this is default bridge domain */
	bool_t is_fallback;							/*!< If TRUE the this is fall-back bridge domain */
	oal_mutex_t *mutex;							/*!< Mutex protecting shared resources */
	pfe_l2br_domain_if_get_crit_t cur_crit;		/*!< Current 'get' criterion (to get interfaces) */
	LLIST_t *cur_item;							/*!< The current interface list item */
	union /* Placeholder union for future use */
	{
		pfe_ct_phy_if_id_t id;
		pfe_phy_if_t *phy_if;
	} cur_crit_arg;								/*!< Current criterion argument */
	LLIST_t ifaces;								/*!< List of associated interfaces */
	LLIST_t list_entry;							/*!< Entry for linked-list purposes */
};

struct __pfe_l2br_static_entry_tag
{
	union
	{
		pfe_ct_mac_table_result_t action_data;
		uint64_t action_data_u64val;
	} u;
	uint16_t vlan;
	pfe_mac_addr_t mac;						/*!< Mac address to be matched */
	pfe_l2br_table_entry_t *entry;			/*!< This is entry within MAC table representing the static entry */
	pfe_l2br_t *bridge;
	LLIST_t list_entry;						/*!< Entry for linked-list purposes */
};

/**
 * @brief	Internal linked-list element
 */
typedef struct
{
	void *ptr;
	LLIST_t list_entry;
} pfe_l2br_list_entry_t;

/**
 * @brief	MAC table flush types
 */
typedef enum
{
	PFE_L2BR_FLUSH_ALL_MAC,
	PFE_L2BR_FLUSH_STATIC_MAC,
	PFE_L2BR_FLUSH_LEARNED_MAC
} pfe_l2br_flush_types;

/**
 * @brief	Flags for 2-field MAC table entry (flags)
 */
typedef enum
{
	MAC_VALID_FLAG = (1U << 3),         /*!< MAC_ENTRY_VALID_FLAG */
	MAC_COL_PTR_VALID_FLAG = (1U << 2), /*!< MAC_ENTRY_COL_PTR_VALID_FLAG */
	MAC_RESERVED1_FLAG = (1U << 1),     /*!< MAC_ENTRY_RESERVED1_FLAG */
	MAC_RESERVED2_FLAG = (1U << 0)      /*!< MAC_ENTRY_RESERVED2_FLAG */
} pfe_l2br_table_entry_flags_t;

/**
 * @brief	Valid flags for 2-field MAC table entry (pfe_mac2f_table_entry_t.field_valids)
 */
typedef enum
{
	MAC_ENTRY_MAC_VALID = (1U << 0),   		/*!< (Field1 = MAC Valid)	*/
	MAC_ENTRY_VLAN_VALID = (1U << 1),   		/*!< (Field2 = VLAN Valid)	*/
	MAC_ENTRY_RESERVED1_VALID = (1U << 2),   	/*!< RESERVED				*/
	MAC_ENTRY_RESERVED2_VALID = (1U << 3),   	/*!< RESERVED				*/
	MAC_ENTRY_RESERVED3_VALID = (1U << 4),   	/*!< RESERVED				*/
	MAC_ENTRY_RESERVED4_VALID = (1U << 5),	/*!< RESERVED				*/
	MAC_ENTRY_RESERVED5_VALID = (1U << 6),	/*!< RESERVED				*/
	MAC_ENTRY_RESERVED6_VALID = (1U << 7),	/*!< RESERVED				*/
} pfe_l2br_table_entry_valid_bits_t;


#define VLAN_STATS_VEC_SIZE 128

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_CLEARED_8
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static uint8_t stats_index[VLAN_STATS_VEC_SIZE];

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_CLEARED_8
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static errno_t pfe_bd_write_to_class(const pfe_l2br_t *bridge, uint32_t base, const pfe_ct_bd_entry_t *class_entry);
static errno_t pfe_l2br_update_hw_entry(pfe_l2br_domain_t *domain);
static pfe_l2br_domain_t *pfe_l2br_create_default_domain(pfe_l2br_t *bridge, uint16_t vlan);
static pfe_l2br_domain_t *pfe_l2br_create_fallback_domain(pfe_l2br_t *bridge);
static bool_t pfe_l2br_domain_match_if_criterion(const pfe_l2br_domain_t *domain, const pfe_phy_if_t *iface);
static bool_t pfe_l2br_domain_match_criterion(const pfe_l2br_t *bridge, pfe_l2br_domain_t *domain);
static bool_t pfe_l2br_static_entry_match_criterion(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t *static_ent);
static errno_t pfe_l2br_set_mac_aging_timeout(pfe_class_t *class, const uint16_t timeout);
static errno_t pfe_l2br_static_entry_destroy_nolock(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent);
static errno_t pfe_l2br_config_domain(const pfe_l2br_t *bridge, pfe_l2br_domain_t *domain);
static errno_t pfe_l2br_check_duplicate_configed_domain(pfe_l2br_t *bridge, pfe_l2br_domain_t *domain, uint16_t vlan);
static errno_t pfe_l2br_set_static_entry(const pfe_l2br_t *bridge, uint16_t vlan, const pfe_mac_addr_t mac, uint32_t new_fw_list, pfe_l2br_static_entry_t **static_entry);
static void pfe_l2br_create_domain(const pfe_class_t *class, pfe_l2br_t **bridge, uint16_t def_vlan, uint16_t def_aging_time);
static errno_t pfe_l2br_flush_static_mac_table(pfe_l2br_t *bridge);
static errno_t pfe_l2br_flush_all_mac_table(const pfe_l2br_t *bridge);
static errno_t pfe_l2br_flush_learned_mac_table(const pfe_l2br_t *bridge, pfe_l2br_table_entry_t *entry, pfe_l2br_table_iterator_t *l2t_iter);
static errno_t pfe_l2br_static_add_entry(pfe_l2br_t *bridge, uint16_t vlan, const pfe_mac_addr_t mac, uint32_t new_fw_list, pfe_l2br_static_entry_t *static_entry);
static errno_t pfe_l2br_domain_destroy_instance(pfe_l2br_domain_t *domain);
static errno_t pfe_l2br_domain_flush_by_if_static(const pfe_l2br_domain_t *domain, const pfe_l2br_t *bridge, uint32_t iface_bitflag);
static errno_t pfe_l2br_domain_flush_by_if_dynamic_static(const pfe_l2br_domain_t *domain, const pfe_l2br_t *bridge, pfe_l2br_table_iterator_t *l2t_iter, pfe_l2br_table_entry_t *entry, uint32_t iface_bitflag);

/**
 * @brief		Write bridge domain structure to classifier memory
 * @param[in]	bridge The bridge instance
 * @param[in]	base Memory location where to write
 * @param[in]	class_entry Pointer to the structure to be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
static errno_t pfe_bd_write_to_class(const pfe_l2br_t *bridge, uint32_t base, const pfe_ct_bd_entry_t *class_entry)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_entry) || (NULL == bridge) || (0U == base)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_class_write_dmem(bridge->class, -1, (addr_t)base, (const void *)class_entry, sizeof(pfe_ct_bd_entry_t));
	}
	return ret;
}

/**
 * @brief		Write bridge domain structure to classifier memory
 * @param[in]	domain Pointer to the structure to be written
 * @param[in]	base Memory location where to write
 */
static void pfe_l2br_update_hw_ll_entry(pfe_l2br_domain_t *domain, uint32_t base)
{
	pfe_ct_bd_entry_t sw_bd;
	uint64_t tmp64;
	bool_t need_shift = FALSE;

	/*	Sanity check */
	ct_assert(sizeof(pfe_ct_bd_entry_t) <= sizeof(uint64_t));

	/*	Check if bitfields within structure are aligned as expected */
	(void)memset(&sw_bd, 0, sizeof(pfe_ct_bd_entry_t));
	tmp64 = (1ULL << 63);
	(void)memcpy(&sw_bd, &tmp64, sizeof(uint64_t));
	if (0U == sw_bd.val)
	{
		need_shift = TRUE;
	}

	/*	Convert VLAN table result to fallback domain representation */
	sw_bd.val = domain->u.action_data.val;

	if (TRUE == need_shift)
	{
		tmp64 = (uint64_t)sw_bd.val << 9;
		(void)memcpy(&sw_bd, &tmp64, sizeof(pfe_ct_bd_entry_t));
	}

	/*	Convert to network endian */
	/* TODO: oal_swap64 or so */
#if defined(PFE_CFG_TARGET_OS_QNX)
	ENDIAN_SWAP64(&sw_bd.val);
#elif defined(PFE_CFG_TARGET_OS_LINUX)
	tmp64 = cpu_to_be64p((uint64_t *)&sw_bd);
	(void)memcpy(&sw_bd, &tmp64, sizeof(uint64_t));
#elif defined(PFE_CFG_TARGET_OS_AUTOSAR)
	*(uint64_t*)&sw_bd = cpu_to_be64(*(uint64_t*)&sw_bd);
#else
#error("todo")
#endif

	/*	Update classifier memory (all PEs) */
	if (EOK != pfe_bd_write_to_class(domain->bridge, base, &sw_bd))
	{
		NXP_LOG_ERROR("Class memory write failed\n");
	}
}

/**
 * @brief		Get hash of a vlan id
 * @param[in]	vlan_id from witch the hash is computed
 * @retval		Hash of the vlan id
 */
static inline uint8_t fp_l2br_vlan_table_get_hash(uint16_t vlan_id)
{
	return (uint8_t)(vlan_id & 0x3FU);
}

/**
 * @brief		Write vlan entry to classifier memory
 * @param[in]	bridge The bridge instance
 * @param[in]	pos Entry position in vlan table
 * @param[in]	class_entry Pointer to the structure to be written
 */
static void pfe_vlan_write_to_class(const pfe_l2br_t *bridge, uint32_t pos, const l2br_vlan_hash_entry_t *class_entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_entry) || (NULL == bridge) || (0U == pos)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != pfe_class_write_dmem(bridge->class, -1, (addr_t)(bridge->dmem_vlan_hash_base + pos*sizeof(l2br_vlan_hash_entry_t)), (const void *)class_entry, sizeof(l2br_vlan_hash_entry_t)))
		{
			NXP_LOG_ERROR("Class memory write failed\n");
		}
	}
	return;
}

/**
 * @brief		Read vlan entry to classifier memory
 * @param[in]	bridge The bridge instance
 * @param[in]	pos Entry position in vlan table
 * @param[out]	class_entry Pointer to the structure to be read
 */
static void pfe_vlan_read_from_class(const pfe_l2br_t *bridge, uint32_t pos, l2br_vlan_hash_entry_t *class_entry)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class_entry) || (NULL == bridge) || (0U == pos)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != pfe_class_read_dmem(bridge->class, 0, class_entry, (addr_t)(bridge->dmem_vlan_hash_base + pos*sizeof(l2br_vlan_hash_entry_t)), sizeof(l2br_vlan_hash_entry_t)))
		{
			NXP_LOG_ERROR("Class memory read failed\n");
		}
	}
	return;
}

/**
 * @brief		Fill vlan entry based on domain information
 * @param[in]	domain Pointer to vlan domain entry
 * @param[out]	entry Pointer to the class structure
 */
static void pfe_l2br_vlan_action_to_entry(pfe_l2br_domain_t *domain, l2br_vlan_hash_entry_t *entry)
{
	uint64_t tmp64;
	bool_t need_shift = FALSE;

	entry->vlan = oal_htons(domain->vlan);
	tmp64 = (1ULL << 63);
	(void)memcpy(&entry->entry.val, &tmp64, sizeof(uint64_t));

	if (0U == entry->entry.val)
	{
		need_shift = TRUE;
	}

	/*      Convert VLAN table result to fallback domain representation */
	entry->entry.val = domain->u.action_data.val;

	if (TRUE == need_shift)
	{
		tmp64 = (uint64_t)entry->entry.val << 9;
		(void)memcpy(&entry->entry.val, &tmp64, sizeof(uint64_t));
	}

	/*      Convert to network endian */
	tmp64 = cpu_to_be64p((uint64_t *)&entry->entry.val);
	(void)memcpy(&entry->entry.val, &tmp64, sizeof(uint64_t));

	entry->flags = MAC_VALID_FLAG;
	entry->field_valids = MAC_ENTRY_VLAN_VALID;

	return;
}

/**
 * @brief		Find and update a given vlan domain to vlan class memory.
 * @param[in]	domain Pointer to vlan domain structure to update.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Vlan is not in class memory
 */
errno_t pfe_l2br_update_vlan_hash_entry(pfe_l2br_domain_t *domain)
{
	l2br_vlan_hash_entry_t vlan_new_entry, vlan_current_entry;
	uint8_t pos;
	errno_t ret = ENOENT;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)memset(&vlan_new_entry, 0, sizeof(l2br_vlan_hash_entry_t));
		(void)memset(&vlan_current_entry, 0, sizeof(l2br_vlan_hash_entry_t));

		pfe_l2br_vlan_action_to_entry(domain, &vlan_new_entry);

		/*Read the entry at the vlan hash position*/
		pos = fp_l2br_vlan_table_get_hash(domain->vlan);
		pfe_vlan_read_from_class(domain->bridge, pos, &vlan_current_entry);

		/*If the vlan exists in hash update the entry */
		if (vlan_current_entry.vlan == vlan_new_entry.vlan)
		{
			vlan_current_entry.entry.val = vlan_new_entry.entry.val;
			vlan_current_entry.flags |= MAC_VALID_FLAG;
			vlan_current_entry.field_valids |= MAC_ENTRY_VLAN_VALID;

			pfe_vlan_write_to_class(domain->bridge, pos, &vlan_current_entry);
			ret = EOK;
		}
		else
		{
			/*entry is full with other entry find a place in collision*/
			if (vlan_current_entry.flags & MAC_VALID_FLAG)
			{
				/*go through all callisions*/
				while (vlan_current_entry.flags & MAC_COL_PTR_VALID_FLAG)
				{
					pos = oal_ntohs(vlan_current_entry.col_ptr);
					pfe_vlan_read_from_class(domain->bridge, pos, &vlan_current_entry);
					if (vlan_new_entry.vlan == vlan_current_entry.vlan)
					{
						pfe_vlan_write_to_class(domain->bridge, pos, &vlan_new_entry);
						/*Found the entry in the collision domain*/
						ret = EOK;
						break;
					}
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Find the position and add a given vlan domain to vlan class memory.
 * @param[in]	domain Pointer to vlan domain structure to add.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		EEXIST Vlan already exists in class memory
 */
errno_t pfe_l2br_add_vlan_hash_entry(pfe_l2br_domain_t *domain)
{
	l2br_vlan_hash_entry_t vlan_new_entry, vlan_current_entry, vlan_tmp_entry;	
	uint8_t pos, prev_entry_pos, hash_size = 0, coll_space = 0;
	errno_t  ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{

		(void)memset(&vlan_new_entry, 0, sizeof(l2br_vlan_hash_entry_t));
		(void)memset(&vlan_current_entry, 0, sizeof(l2br_vlan_hash_entry_t));
		(void)memset(&vlan_tmp_entry, 0, sizeof(l2br_vlan_hash_entry_t));

		ret = pfe_feature_mgr_table_get_payload("software_vlan_table", FW_FEATURE_TABLE_CONFIG, "size", &hash_size);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Wrong vlan software table config size\n");
			return ret;
		}

		ret = pfe_feature_mgr_table_get_payload("software_vlan_table", FW_FEATURE_TABLE_CONFIG, "coll_space", &coll_space);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Wrong vlan software table config collision space\n");
			return ret;
		}

		pfe_l2br_vlan_action_to_entry(domain, &vlan_new_entry);

		/*Read the entry at the vlan hash position*/
		pos = fp_l2br_vlan_table_get_hash(domain->vlan);
		pfe_vlan_read_from_class(domain->bridge, pos, &vlan_current_entry);

		/*If the vlan exists in hash update the entry */
		if (vlan_current_entry.vlan == vlan_new_entry.vlan)
		{
			ret = EEXIST;
		}
		else
		{
			/*entry is full with other entry find a place in collision*/
			if (vlan_current_entry.flags & MAC_VALID_FLAG)
			{
				prev_entry_pos = pos;
				/*go through all callisions*/
				while (vlan_current_entry.flags & MAC_COL_PTR_VALID_FLAG)
				{
					pos = oal_ntohs(vlan_current_entry.col_ptr);
					prev_entry_pos = pos;
					pfe_vlan_read_from_class(domain->bridge, pos, &vlan_current_entry);
					if (vlan_new_entry.vlan == vlan_current_entry.vlan)
					{
						/*Found the entry in the collision domain*/
						ret = EEXIST;
						break;
					}
				}

				if (vlan_new_entry.vlan != vlan_current_entry.vlan)
				{
					/*The new entry is not in collision. Find a place to add entry */
					vlan_tmp_entry = vlan_current_entry;

					if (pos < coll_space)
					{
						pos = coll_space;
						pfe_vlan_read_from_class(domain->bridge, pos, &vlan_tmp_entry);
					}
					/*Find the first free postion in the collision*/
					while (vlan_tmp_entry.flags & MAC_VALID_FLAG) 
					{
						pos ++;
						if (pos >= hash_size)
						{
							ret =  ENOMEM;
							break;
						}	
						pfe_vlan_read_from_class(domain->bridge, pos, &vlan_tmp_entry);
					}
					if (pos < hash_size)
					{
						/*   Write the new entry to pos */
						pfe_vlan_write_to_class(domain->bridge, pos, &vlan_new_entry);

						/*Update prev vlan id with collision pointer to the new vlan entry*/
						vlan_current_entry.flags |= MAC_COL_PTR_VALID_FLAG;
						vlan_current_entry.col_ptr = oal_htons(pos);
						pfe_vlan_write_to_class(domain->bridge, prev_entry_pos, &vlan_current_entry);
					}
				}
			}
			else
			{
				/*   Write the new entry in hash */
				pfe_vlan_write_to_class(domain->bridge, pos, &vlan_new_entry);
			}
		}
	}

	return ret;
}

/**
 * @brief		Delete a given vlan domain from vlan class memory.
 * @param[in]	domain Pointer to vlan domain structure to be removed.
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 **/
errno_t pfe_l2br_delete_vlan_hash_entry(pfe_l2br_domain_t *domain)
{
	l2br_vlan_hash_entry_t vlan_zero_entry, vlan_current_entry, vlan_prev_entry, vlan_tmp_entry;
	uint8_t pos, prev_entry_pos, next_entry_pos;
	bool erase_collision_entry = FALSE;
	errno_t  ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{

		(void)memset(&vlan_zero_entry, 0, sizeof(l2br_vlan_hash_entry_t));
		(void)memset(&vlan_current_entry, 0, sizeof(l2br_vlan_hash_entry_t));
		(void)memset(&vlan_prev_entry, 0, sizeof(l2br_vlan_hash_entry_t));
		(void)memset(&vlan_tmp_entry, 0, sizeof(l2br_vlan_hash_entry_t));

		pos = fp_l2br_vlan_table_get_hash(domain->vlan);
		pfe_vlan_read_from_class(domain->bridge, pos, &vlan_current_entry);

		/*If the entry is in hash*/
		if (domain->vlan == oal_ntohs(vlan_current_entry.vlan))
		{
			/*If the entry has collision*/
			if (vlan_current_entry.flags & MAC_COL_PTR_VALID_FLAG)
			{
				next_entry_pos = oal_ntohs(vlan_current_entry.col_ptr);
				pfe_vlan_read_from_class(domain->bridge, next_entry_pos, &vlan_tmp_entry);
				erase_collision_entry = TRUE;
			}
			/*Write to hash the next collision entry*/
			pfe_vlan_write_to_class(domain->bridge, pos, &vlan_tmp_entry);

			if (TRUE == erase_collision_entry)
			{
				pfe_vlan_write_to_class(domain->bridge, next_entry_pos, &vlan_zero_entry);
			}
		}
		else
		{
			/*go through all collisions*/
			while (vlan_current_entry.flags & MAC_COL_PTR_VALID_FLAG)
			{
				prev_entry_pos = pos;
				vlan_prev_entry = vlan_current_entry;

				pos = oal_ntohs(vlan_current_entry.col_ptr);
				pfe_vlan_read_from_class(domain->bridge, pos, &vlan_current_entry);

				/* entry found */
				if (domain->vlan == oal_ntohs(vlan_current_entry.vlan))
				{
					/* there is an entry after in collision */
					if (vlan_current_entry.flags & MAC_COL_PTR_VALID_FLAG)
					{
						vlan_prev_entry.col_ptr = vlan_current_entry.col_ptr;
					}
					else
					{
						vlan_prev_entry.flags = 0;
						vlan_prev_entry.col_ptr = 0;
					}
					/* Update collision pointer on the prev position*/
					pfe_vlan_write_to_class(domain->bridge, prev_entry_pos, &vlan_prev_entry);

					pfe_vlan_write_to_class(domain->bridge, pos, &vlan_zero_entry);
					break;
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Update HW entry according to domain setup
 * @details		Function is intended to propagate domain configuration from host SW instance
 * 				form to PFE HW/FW representation.
 * @param[in]	domain The domain instance
 * @return		EOK or error code in case of failure
 */
static errno_t pfe_l2br_update_hw_entry(pfe_l2br_domain_t *domain)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	In case of fall-back domain the classifier memory must be updated too */
		if (TRUE == domain->is_fallback)
		{
			/*	Update classifier memory (all PEs) */
			pfe_l2br_update_hw_ll_entry(domain, domain->bridge->dmem_fb_bd_base);
			if (TRUE == pfe_feature_mgr_is_available("software_vlan_table"))
			{
				pfe_l2br_update_vlan_hash_entry(domain);
			}
			ret = EOK;
		}
		else
		{
			/*	In case of fall-back domain the classifier memory must be updated too */
			if (TRUE == domain->is_default)
			{
				/*	Update classifier memory (all PEs) */
				pfe_l2br_update_hw_ll_entry(domain, domain->bridge->dmem_def_bd_base);
				if (TRUE == pfe_feature_mgr_is_available("software_vlan_table"))
				{
					pfe_l2br_update_vlan_hash_entry(domain);
				}
			}
			/*	Update standard or default domain entry */
			ret = pfe_l2br_table_entry_set_action_data(domain->vlan_entry, domain->u.action_data_u64val);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Can't set action data: %d\n", ret);
				ret = ENOEXEC;
			}
			else
			{
				if (TRUE == pfe_feature_mgr_is_available("software_vlan_table"))
				{
					ret = pfe_l2br_update_vlan_hash_entry(domain);
				}
				else
				{
					/*	Propagate change to HW table */
					ret = pfe_l2br_table_update_entry(domain->bridge->vlan_table, domain->vlan_entry);
				}
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't update VLAN table entry: %d\n", ret);
					ret = ENOEXEC;
				}
			}
		}
	}
	return ret;
}

/**
 * @brief		Get the next free index in the vlan stats table
 * @return		The index
 */
static uint8_t pfe_l2br_domain_get_free_stats_index(const pfe_l2br_t *bridge)
{
	/*Index 0 is reseved for fallback domain and domains outside stats range.
	 * The fallback domain does not call pfe_l2br_domain_create*/
	uint8_t index = 1;

	while (index < bridge->domain_stats_table_size)
	{
		if (stats_index[index] == 0U)
		{
			stats_index[index] = 1U;
			break;
		}
		index ++;
	}

	/* domain outside stats range. */
	if (index == bridge->domain_stats_table_size)
	{
		index = 0;
	}

	return index;
}

/**
 * @brief		Free the index in the stats table
 * @param[in]	Index of the table
 */
static void pfe_l2br_domain_free_stats_index(uint8_t index)
{
	stats_index[index] = 0U;
}

/**
 * @brief		Create the vlan stats table
 * @details		Create and allocate in dmem the space for stats table that
 * 				include all configured vlans
 * @param[in]   Class instance
 * @param[in]	Number of configured vlan
 * @return		DMEM address of the table
 */
static uint32_t pfe_l2br_create_vlan_stats_table(pfe_class_t *class, uint16_t vlan_count)
{
	addr_t                   addr;
	uint32_t                 size;
	pfe_ct_vlan_statistics_t temp;
	errno_t                  res;
	pfe_ct_class_mmap_t      mmap;

	/* Calculate needed size */
	size = (uint32_t)((uint32_t)vlan_count * sizeof(pfe_ct_vlan_stats_t));
	/* Allocate DMEM */
	addr = pfe_class_dmem_heap_alloc(class, size);
	if (0U == addr)
	{
		NXP_LOG_ERROR("Not enough DMEM memory\n");
	}
	else
	{
		res = pfe_class_get_mmap(class, 0, &mmap);

		if (EOK != res)
		{
			NXP_LOG_ERROR("Cannot get class memory map\n");
			addr = (uint32_t)res;
		}
		else
		{
			/* Write the table header */
			temp.vlan_count = oal_htons(vlan_count);
			temp.vlan = oal_htonl(addr);
			/*It is safe to write the table pointer because PEs are gracefully stopped
     * and configuration read*/
			res = pfe_class_write_dmem(class, -1, oal_ntohl(mmap.vlan_statistics), (void *)&temp, sizeof(pfe_ct_vlan_statistics_t));
			if (EOK != res)
			{
				NXP_LOG_ERROR("Cannot write to DMEM\n");
				pfe_class_dmem_heap_free(class, addr);
				addr = 0U;
			}
		}
	}
	/* Return the DMEM address */
	return addr;
}

/**
 * @brief		Destroy the vlan stats table
 * @details		Free from dmem the space filled by the table
 * @param[in]	Table address
 * @param[in]   Class instance
 */
static errno_t pfe_l2br_destroy_vlan_stats_table(pfe_class_t *class, uint32_t table_address)
{
	pfe_ct_vlan_statistics_t temp = { 0 };
	pfe_ct_class_mmap_t      mmap;
	errno_t                  res;

	if (0U == table_address)
	{
		res = EOK;
	}
	else
	{
		res = pfe_class_get_mmap(class, 0, &mmap);

		if (EOK != res)
		{
			NXP_LOG_ERROR("Cannot get class memory map\n");
		}
		else
		{
			/*It is safe to write the table pointer because PEs are gracefully stopped
	 * and configuration read*/
			res = pfe_class_write_dmem(class, -1, oal_ntohl(mmap.vlan_statistics), (void *)&temp, sizeof(pfe_ct_vlan_statistics_t));
			if (EOK != res)
			{
				NXP_LOG_ERROR("Cannot write to DMEM\n");
			}
			else
			{
				pfe_class_dmem_heap_free(class, table_address);
			}
		}
	}
	return res;
}

/**
 * @brief		Config for bridge domain
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	domain the bridge domain
 * @retval		EOK Success
 * @retval		ENOEXEC Command failed
 */
static errno_t pfe_l2br_config_domain(const pfe_l2br_t *bridge, pfe_l2br_domain_t *domain)
{
	errno_t ret = EOK;

	/*	Prepare VLAN table entry. At the beginning the bridge entry does not contain
		any ports. */
	domain->vlan_entry = pfe_l2br_table_entry_create(bridge->vlan_table);
	if (NULL == domain->vlan_entry)
	{
		NXP_LOG_ERROR("Can't create vlan table entry\n");
		if (EOK != pfe_l2br_domain_destroy(domain))
		{
			NXP_LOG_ERROR("Unable to destroy bridge domain\n");
		}
		ret = ENOEXEC;
	}
	else
	{
		/*	Set VLAN */
		ret = pfe_l2br_table_entry_set_vlan(domain->vlan_entry, domain->vlan);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't set vlan: %d\n", ret);
			if (EOK != pfe_l2br_domain_destroy(domain))
			{
				NXP_LOG_ERROR("Unable to destroy bridge domain\n");
			}
		}
		else
		{
			domain->u.action_data.item.forward_list = 0U;
			domain->u.action_data.item.untag_list = 0U;
			domain->u.action_data.item.ucast_hit_action = (uint64_t)L2BR_ACT_DISCARD;
			domain->u.action_data.item.ucast_miss_action = (uint64_t)L2BR_ACT_DISCARD;
			domain->u.action_data.item.mcast_hit_action = (uint64_t)L2BR_ACT_DISCARD;
			domain->u.action_data.item.mcast_miss_action = (uint64_t)L2BR_ACT_DISCARD;
			domain->u.action_data.item.stats_index = pfe_l2br_domain_get_free_stats_index(bridge);

			if (domain->u.action_data.item.stats_index == 0U)
			{
				NXP_LOG_ERROR("No more space for vlan statistics.The stats will be added to vlan 0 fallback\n");
			}

			domain->stats_index = (uint8_t)domain->u.action_data.item.stats_index;

			/*	Set action data */
			ret = pfe_l2br_table_entry_set_action_data(domain->vlan_entry, domain->u.action_data_u64val);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Can't set action data: %d\n", ret);
				if (EOK != pfe_l2br_domain_destroy(domain))
				{
					NXP_LOG_ERROR("Unable to destroy bridge domain\n");
				}
			}
			else
			{
				if (TRUE == pfe_feature_mgr_is_available("software_vlan_table"))
				{
					ret = pfe_l2br_add_vlan_hash_entry(domain);
				}
				else
				{
					/*	Add new VLAN table entry */
					ret = pfe_l2br_table_add_entry(domain->bridge->vlan_table, domain->vlan_entry);
				}

				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not add VLAN table entry: %d\n", ret);
					if (EOK != pfe_l2br_domain_destroy(domain))
					{
						NXP_LOG_ERROR("Unable to destroy bridge domain\n");
					}
				}
			}
		}
	}

	return ret;
}

/**
 * @brief		Check duplicate for configed bridge domain
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	domain the bridge domain
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @retval		EOK Success
 * @retval		EPERM Operation not permitted (domain already created)
 */
static errno_t pfe_l2br_check_duplicate_configed_domain(pfe_l2br_t *bridge, pfe_l2br_domain_t *domain, uint16_t vlan)
{
	errno_t ret = EOK;

	/*	Check if the domain is not duplicate */
	if (NULL != pfe_l2br_get_first_domain(bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)vlan))
	{
		NXP_LOG_ERROR("Domain with vlan %d does already exist\n", domain->vlan);
		if (EOK != pfe_l2br_domain_destroy(domain))
		{
			NXP_LOG_ERROR("Unable to destroy bridge domain\n");
		}
		ret = EPERM;
	}
	else
	{
		ret = pfe_l2br_config_domain(bridge, domain);

		if(EOK == ret)
		{
			/*	Remember the domain instance in global list */
			if (EOK != oal_mutex_lock(bridge->mutex))
			{
				NXP_LOG_ERROR("Mutex lock failed\n");
			}

			LLIST_AddAtEnd(&domain->list_entry, &bridge->domains);

			if (EOK != oal_mutex_unlock(bridge->mutex))
			{
				NXP_LOG_ERROR("Mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief		Create L2 bridge domain instance
 * @details		By default, new domain is configured to drop all matching packets. Use the
 *				pfe_l2br_domain_set_[ucast/mcast]_action() to finish the configuration. The
 *				instance is automatically bound to the bridge and can be retrieved by
 *				the pfe_l2br_get_first_domain()/pfe_l2br_get_next_domain() calls.
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 * @retval		ETIMEDOUT Timed out
 * @retval		EPERM Operation not permitted (domain already created)
 */
errno_t pfe_l2br_domain_create(pfe_l2br_t *bridge, uint16_t vlan)
{
	pfe_l2br_domain_t *domain;
	errno_t            ret = EOK;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		domain = oal_mm_malloc(sizeof(pfe_l2br_domain_t));

		if (NULL == domain)
		{
			NXP_LOG_ERROR("malloc() failed\n");
			ret = ENOMEM;
		}
		else
		{
			(void)memset(domain, 0, sizeof(pfe_l2br_domain_t));
			domain->bridge = bridge;
			domain->vlan = vlan;
			domain->is_default = FALSE;
			domain->list_entry.prNext = NULL;
			domain->list_entry.prPrev = NULL;
			LLIST_Init(&domain->ifaces);

			domain->mutex = oal_mm_malloc(sizeof(oal_mutex_t));
			if (NULL == domain->mutex)
			{
				NXP_LOG_ERROR("Memory allocation failed\n");
				ret = ENOMEM;
				if (EOK != pfe_l2br_domain_destroy(domain))
				{
					NXP_LOG_ERROR("Unable to destroy bridge domain\n");
				}
			}
			else
			{

				ret = oal_mutex_init(domain->mutex);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not initialize mutex\n");
					oal_mm_free(domain->mutex);
					domain->mutex = NULL;
					if (EOK != pfe_l2br_domain_destroy(domain))
					{
						NXP_LOG_ERROR("Unable to destroy bridge domain\n");
					}
				}
				else
				{
					ret = pfe_l2br_check_duplicate_configed_domain(bridge, domain, vlan);
				}
			}
		}
	}

	return ret;
}

static errno_t pfe_l2br_domain_destroy_instance(pfe_l2br_domain_t *domain)
{
	errno_t ret = EOK;

	if (TRUE == domain->is_fallback)
	{
		/*	Disable the fall-back domain traffic */
		ret = pfe_l2br_domain_set_ucast_action(domain, L2BR_ACT_DISCARD, L2BR_ACT_DISCARD);
		if (EOK == ret)
		{
			ret = pfe_l2br_domain_set_mcast_action(domain, L2BR_ACT_DISCARD, L2BR_ACT_DISCARD);
		}
	}

	/*	Remove the domain instance from global list if it has been added there */
	if (EOK != oal_mutex_lock(domain->bridge->mutex))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	/*	If instance has not been added to the list of domains yet (mainly due to
		pfe_l2br_domain_create() failure, just skip this step */
	if ((NULL != domain->list_entry.prPrev) && (NULL != domain->list_entry.prNext))
	{
		if (&domain->list_entry == domain->bridge->curr_domain)
		{
			/*	Remember the change so we can call destroy() between get_first()
				and get_next() calls. */
			domain->bridge->curr_domain = domain->bridge->curr_domain->prNext;
		}

		LLIST_Remove(&domain->list_entry);
	}

	pfe_l2br_domain_free_stats_index(domain->stats_index);

	if (EOK != oal_mutex_unlock(domain->bridge->mutex))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	if (NULL != domain->mutex)
	{
		(void)oal_mutex_destroy(domain->mutex);
		oal_mm_free(domain->mutex);
		domain->mutex = NULL;
	}
	return ret;
}

/**
 * @brief		Destroy L2 bridge domain instance
 * @param[in]	bridge The bridge domain instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_l2br_domain_destroy(pfe_l2br_domain_t *domain)
{
	errno_t                ret = EOK;
	LLIST_t *              aux, *item;
	pfe_l2br_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Remove all associated interfaces */
		if (FALSE == LLIST_IsEmpty(&domain->ifaces))
		{
			NXP_LOG_INFO("Non-empty bridge domain is being destroyed\n");
			LLIST_ForEachRemovable(item, aux, &domain->ifaces)
			{
				entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
				oal_mm_free(entry);
				entry = NULL;
			}
		}

		if (NULL != domain->vlan_entry)
		{
			if (TRUE == pfe_feature_mgr_is_available("software_vlan_table"))
			{
				ret = pfe_l2br_delete_vlan_hash_entry(domain);
			}
			else
			{

				/*	Remove entry from the table */
				ret = pfe_l2br_table_del_entry(domain->bridge->vlan_table, domain->vlan_entry);
			}

			if (EOK != ret)
			{
				NXP_LOG_ERROR("Can't delete entry from VLAN table: %d\n", ret);
				ret = ENOEXEC;
			}
			else
			{
				/*	Release the table entry instance */
				(void)pfe_l2br_table_entry_destroy(domain->vlan_entry);
				domain->vlan_entry = NULL;
			}
		}

		if (EOK == ret)
		{
			ret = pfe_l2br_domain_destroy_instance(domain);
			oal_mm_free(domain);
		}
	}

	return ret;
}

/**
 * @brief		Create default L2 bridge domain instance
 * @details		Create default bridge domain (empty, no interface assigned)
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @return		The instance or NULL if failed
 */
static pfe_l2br_domain_t *pfe_l2br_create_default_domain(pfe_l2br_t *bridge, uint16_t vlan)
{
	errno_t             ret;
	pfe_l2br_domain_t * domain;
	pfe_ct_class_mmap_t class_mmap;
	uint32_t vlan_hash_addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		domain = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != pfe_class_get_mmap(bridge->class, 0, &class_mmap))
		{
			NXP_LOG_ERROR("Could not get memory map\n");
			domain = NULL;
		}
		else
		{

			bridge->dmem_def_bd_base = oal_ntohl(class_mmap.dmem_def_bd_base);

			if (EOK == pfe_feature_mgr_enable("software_vlan_table"))
			{
				ret = pfe_feature_mgr_table_get_payload("software_vlan_table", FW_FEATURE_TABLE_CONFIG, "vlan_hash", (uint8_t*) &vlan_hash_addr);
				if (EOK == ret)
				{
					bridge->dmem_vlan_hash_base = vlan_hash_addr;
					NXP_LOG_INFO("Software vlan hash table @ p0x%x\n", vlan_hash_addr);
				}
				else
				{
					/*Fall back to hardware vlan table*/
					pfe_feature_mgr_disable("software_vlan_table");
					NXP_LOG_INFO("Hardware vlan hash table\n");
				}
			}
			else
			{
				NXP_LOG_INFO("Hardware vlan hash table\n");
			}

			ret = pfe_l2br_domain_create(bridge, vlan);

			if (EOK != ret)
			{
				NXP_LOG_DEBUG("Can't create default domain\n");
				domain = NULL;
			}
			else
			{
				domain = pfe_l2br_get_first_domain(bridge, L2BD_CRIT_BY_VLAN, (void *)(addr_t)vlan);
				if (NULL == domain)
				{
					NXP_LOG_ERROR("Default domain not found\n");
				}
				else
				{
					domain->is_default = TRUE;
					if (EOK != pfe_l2br_update_hw_entry(domain))
					{
						oal_mm_free(domain);
						domain = NULL;
					}
				}
			}
		}
	}
	return domain;
}

/**
 * @brief		Create fall-back L2 bridge domain instance
 * @details		Create fall-back bridge domain (empty, no interface assigned)
 * @param[in]	bridge The L2 bridge instance
 * @return		The instance or NULL if failed
 */
static pfe_l2br_domain_t *pfe_l2br_create_fallback_domain(pfe_l2br_t *bridge)
{
	pfe_ct_class_mmap_t class_mmap;
	pfe_l2br_domain_t * domain;
	errno_t             ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		domain = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		domain = oal_mm_malloc(sizeof(pfe_l2br_domain_t));
		if (NULL == domain)
		{
			NXP_LOG_ERROR("Memory allocation failed\n");
		}
		else
		{
			(void)memset(domain, 0, sizeof(pfe_l2br_domain_t));
			domain->bridge = bridge;
			domain->vlan_entry = NULL;
			domain->is_fallback = TRUE;
			LLIST_Init(&domain->ifaces);

			domain->mutex = oal_mm_malloc(sizeof(oal_mutex_t));
			if (NULL == domain->mutex)
			{
				NXP_LOG_ERROR("Memory allocation failed\n");
				oal_mm_free(domain);
				domain = NULL;
			}
			else
			{
				ret = oal_mutex_init(domain->mutex);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Could not initialize mutex\n");
					oal_mm_free(domain->mutex);
					domain->mutex = NULL;
					domain = NULL;
				}
				else
				{
					if (EOK != pfe_class_get_mmap(bridge->class, 0, &class_mmap))
					{
						NXP_LOG_ERROR("Could not get memory map\n");
						oal_mm_free(domain);
						domain = NULL;
					}
					else
					{
						bridge->dmem_fb_bd_base = oal_ntohl(class_mmap.dmem_fb_bd_base);

						NXP_LOG_INFO("Fall-back bridge domain @ 0x%x (class)\n", (uint_t)bridge->dmem_fb_bd_base);
						NXP_LOG_INFO("Default bridge domain @ 0x%x (class)\n", (uint_t)bridge->dmem_def_bd_base);

						domain->u.action_data.item.forward_list = 0U;
						domain->u.action_data.item.untag_list = 0U;
						domain->u.action_data.item.ucast_hit_action = (uint64_t)L2BR_ACT_DISCARD;
						domain->u.action_data.item.ucast_miss_action = (uint64_t)L2BR_ACT_DISCARD;
						domain->u.action_data.item.mcast_hit_action = (uint64_t)L2BR_ACT_DISCARD;
						domain->u.action_data.item.mcast_miss_action = (uint64_t)L2BR_ACT_DISCARD;

						if (EOK != pfe_l2br_update_hw_entry(domain))
						{
							oal_mm_free(domain);
							domain = NULL;
						}

						if (EOK != oal_mutex_lock(bridge->mutex))
						{
							NXP_LOG_ERROR("Mutex lock failed\n");
						}

						/*	Remember the domain instance in global list */
						LLIST_AddAtEnd(&domain->list_entry, &bridge->domains);

						if (EOK != oal_mutex_unlock(bridge->mutex))
						{
							NXP_LOG_ERROR("Mutex unlock failed\n");
						}
					}
				}
			}
		}
	}
	return domain;
}

/**
 * @brief		Set unicast actions
 * @param[in]	domain The bridge domain instance
 * @param[in]	hit Action to be taken when destination MAC address (uni-cast) of a packet
 *					matching the domain is found in the MAC table
 * @param[in]	miss Action to be taken when destination MAC address (uni-cast) of a packet
 *					 matching the domain is not found in the MAC table
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_set_ucast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t hit, pfe_ct_l2br_action_t miss)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		domain->u.action_data.item.ucast_hit_action = (uint64_t)hit;
		domain->u.action_data.item.ucast_miss_action = (uint64_t)miss;

		ret = pfe_l2br_update_hw_entry(domain);
	}
	return ret;
}

/**
 * @brief		Get unicast actions
 * @param[in]	domain The bridge domain instance
 * @param[out]	hit Pointer to memory where hit action shall be written
 * @param[out]	miss Pointer to memory where miss action shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_get_ucast_action(const pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t *hit, pfe_ct_l2br_action_t *miss)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == hit) || (NULL == miss)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*hit = (pfe_ct_l2br_action_t)(domain->u.action_data.item.ucast_hit_action);
		*miss = (pfe_ct_l2br_action_t)(domain->u.action_data.item.ucast_miss_action);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief		Set multi-cast actions
 * @param[in]	domain The bridge domain instance
 * @param[in]	hit Action to be taken when destination MAC address (multi-cast) of a packet
 *					matching the domain is found in the MAC table
 * @param[in]	miss Action to be taken when destination MAC address (multi-cast) of a packet
 *					 matching the domain is not found in the MAC table
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_set_mcast_action(pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t hit, pfe_ct_l2br_action_t miss)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		domain->u.action_data.item.mcast_hit_action = (uint64_t)hit;
		domain->u.action_data.item.mcast_miss_action = (uint64_t)miss;
		ret = pfe_l2br_update_hw_entry(domain);
	}
	return ret;
}

/**
 * @brief		Get multicast actions
 * @param[in]	domain The bridge domain instance
 * @param[out]	hit Pointer to memory where hit action shall be written
 * @param[out]	miss Pointer to memory where miss action shall be written
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_domain_get_mcast_action(const pfe_l2br_domain_t *domain, pfe_ct_l2br_action_t *hit, pfe_ct_l2br_action_t *miss)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == hit) || (NULL == miss)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*hit = (pfe_ct_l2br_action_t)(domain->u.action_data.item.mcast_hit_action);
		*miss = (pfe_ct_l2br_action_t)(domain->u.action_data.item.mcast_miss_action);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief		Add an interface to bridge domain
 * @param[in]	domain The bridge domain instance
 * @param[in]	iface Interface to be added
 * @param[in]	tagged TRUE means the interface is 'tagged', FALSE stands for 'un-tagged'
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 * @retval		EEXIST Already added
 */
errno_t pfe_l2br_domain_add_if(pfe_l2br_domain_t *domain, pfe_phy_if_t *iface, bool_t tagged)
{
	errno_t                ret = EOK;
	pfe_ct_phy_if_id_t     id;
	pfe_l2br_list_entry_t *entry;
	LLIST_t *              item;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Check duplicates */
		id = pfe_phy_if_get_id(iface);
		if (FALSE == LLIST_IsEmpty(&domain->ifaces))
		{
			LLIST_ForEach(item, &domain->ifaces)
			{
				entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
				if (((pfe_phy_if_t *)entry->ptr == iface))
				{
					NXP_LOG_INFO("Interface %d already added\n", id);
					ret = EEXIST;
					break;
				}
			}
		}
		if (EOK == ret)
		{
			entry = oal_mm_malloc(sizeof(pfe_l2br_list_entry_t));
			if (NULL == entry)
			{
				NXP_LOG_ERROR("Malloc failed\n");
				ret = ENOMEM;
			}
			else
			{
				/*	Add it to this domain = update VLAN table entry */
				domain->u.action_data.item.forward_list |= (uint64_t)1U << (uint8_t)id;

				if (FALSE == tagged)
				{
					domain->u.action_data.item.untag_list |= (uint64_t)1U << (uint8_t)id;
				}

				ret = pfe_l2br_update_hw_entry(domain);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("Can't update VLAN table entry: %d\n", ret);
					oal_mm_free(entry);
					ret = ENOEXEC;
				}
				else
				{
					/*	Remember the interface instance in global list */
					if (EOK != oal_mutex_lock(domain->mutex))
					{
						NXP_LOG_ERROR("Mutex lock failed\n");
					}

					entry->ptr = (void *)iface;
					LLIST_AddAtEnd(&entry->list_entry, &domain->ifaces);

					if (EOK != oal_mutex_unlock(domain->mutex))
					{
						NXP_LOG_ERROR("Mutex unlock failed\n");
					}
				}
			}
		}
	}
	return ret;
}

/**
 * @brief		Remove interface from bridge domain
 * @param[in]	domain The bridge domain instance
 * @param[in]	iface Interface to be deleted
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOEXEC Command failed
 */
errno_t pfe_l2br_domain_del_if(pfe_l2br_domain_t *domain, const pfe_phy_if_t *iface)
{
	errno_t                ret = EOK;
	LLIST_t *              aux, *item;
	pfe_l2br_list_entry_t *entry;
	bool_t                 match = FALSE;
	pfe_ct_phy_if_id_t     id;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Remove the interface instance from global list if it has been added there */
		if (EOK != oal_mutex_lock(domain->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		LLIST_ForEachRemovable(item, aux, &domain->ifaces)
		{
			entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
			if (entry->ptr == (void *)iface)
			{
				/*	Found in list */
				id = pfe_phy_if_get_id((pfe_phy_if_t *)entry->ptr);

				/*	Update HW */
				domain->u.action_data.item.forward_list &= ~((uint64_t)1U << (uint8_t)id);
				domain->u.action_data.item.untag_list &= ~((uint64_t)1U << (uint8_t)id);

				ret = pfe_l2br_update_hw_entry(domain);
				if (EOK != ret)
				{
					NXP_LOG_ERROR("VLAN table entry update failed: %d\n", ret);
					if (EOK != oal_mutex_unlock(domain->mutex))
					{
						NXP_LOG_ERROR("Mutex unlock failed\n");
					}
					ret = ENOEXEC;
					break;
				}
				else
				{
					/*	Release the list entry */
					if (&entry->list_entry == domain->cur_item)
					{
						/*	Remember the change so we can call del_if() between get_first()
					and get_next() calls. */
						domain->cur_item = domain->cur_item->prNext;
					}

					LLIST_Remove(item);
					oal_mm_free(entry);
					entry = NULL;

					match = TRUE;
				}
			}
		}

		if (EOK == ret)
		{
			if (EOK != oal_mutex_unlock(domain->mutex))
			{
				NXP_LOG_ERROR("Mutex unlock failed\n");
			}

			if (FALSE == match)
			{
				NXP_LOG_DEBUG("Interface not found\n");
				ret = ENOENT;
			}
		}
	}
	return ret;
}

static errno_t pfe_l2br_domain_flush_by_if_static(const pfe_l2br_domain_t *domain, const pfe_l2br_t *bridge, uint32_t iface_bitflag)
{
	errno_t ret = EOK;
	LLIST_t *item, *dummy = NULL;
	pfe_l2br_static_entry_t *sentry = NULL;
	uint64_t updated_forward_list;

	/*	Flush interface-related static entries */
	if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
	{
		LLIST_ForEachRemovable(item, dummy, &bridge->static_entries)
		{
			/*	Get static entry */
			sentry = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);
			if (sentry == NULL)
			{
				NXP_LOG_ERROR("NULL static entry detected!\n");
			}
			else
			{
				/*	Check static entry */
				if ((sentry->vlan == domain->vlan) && (0U != (sentry->u.action_data.item.forward_list & iface_bitflag)))
				{
					updated_forward_list = sentry->u.action_data.item.forward_list & ~iface_bitflag;
					if(0U != updated_forward_list)
					{
						/*Another interface exists in this entry, just update forward list*/
						ret = pfe_l2br_static_entry_replace_fw_list(bridge, sentry, updated_forward_list);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("Unable to update forward list of static entry: %d\n", ret);
						}
					}
					else
					{
						/*	Remove static entry. LLIST_Remove() is inside... */
						ret = pfe_l2br_static_entry_destroy_nolock(bridge, sentry);
						if (EOK != ret)
						{
							NXP_LOG_ERROR("Unable to remove static entry: %d\n", ret);
						}
					}
				}
			}
		}
	}

	return ret;
}

static errno_t pfe_l2br_domain_flush_by_if_dynamic_static(const pfe_l2br_domain_t *domain, const pfe_l2br_t *bridge, pfe_l2br_table_iterator_t *l2t_iter, pfe_l2br_table_entry_t *entry, uint32_t iface_bitflag)
{
	errno_t ret = EOK;
    errno_t ret_query = EOK;
	uint16_t entry_vlan = 0U;
	pfe_ct_mac_table_result_t entry_action_data = {.val = 0U};

	ret = pfe_l2br_domain_flush_by_if_static(domain, bridge, iface_bitflag);
	if (EOK == ret)
	{
		/*	Flush interface-related dynamic entries */
		ret_query = pfe_l2br_table_get_first(bridge->mac_table, l2t_iter, L2BR_TABLE_CRIT_VALID, entry);
		while (EOK == ret_query)
		{
			entry_vlan = (uint16_t)pfe_l2br_table_entry_get_vlan(entry);
			entry_action_data.val = (uint32_t)pfe_l2br_table_entry_get_action_data(entry);

			/*	Check entry */
			if ((entry_vlan == domain->vlan) && (0U != (entry_action_data.item.forward_list & iface_bitflag)))
			{
				if(0U != (entry_action_data.item.forward_list & ~iface_bitflag))
				{
					/*Have more interfaces active, just remove this one*/

					entry_action_data.item.forward_list &= ~iface_bitflag;

					ret = pfe_l2br_table_entry_set_action_data(entry, entry_action_data.val);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Could not update MAC table entry: %d\n", ret);
					}
				}
				else
				{
					/*	Remove entry */
					ret = pfe_l2br_table_del_entry(bridge->mac_table, entry);
					if (EOK != ret)
					{
						NXP_LOG_ERROR("Could not delete MAC table entry: %d\n", ret);
					}
				}
			}

			/* Get the next entry */
			ret_query = pfe_l2br_table_get_next(bridge->mac_table, l2t_iter, entry);
		}

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

	}

	return ret;
}

/**
 * @brief		Flush all MAC table entries of given bridge domain which are related to target interface.
 * @param[in]	domain The L2 bridge domain instance
 * @param[in]	iface The interface
 * @retval		EOK if success, error code if failure
 */
errno_t pfe_l2br_domain_flush_by_if(const pfe_l2br_domain_t *domain, const pfe_phy_if_t *iface)
{
	errno_t                    ret = EOK;
	pfe_l2br_table_entry_t *   entry = NULL;
	pfe_l2br_table_iterator_t *l2t_iter = NULL;
	uint32_t                   iface_bitflag = 0U;
	const pfe_l2br_t *         bridge = NULL;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == domain->bridge) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{

		bridge = domain->bridge;
		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
			ret = EPERM;
		}
		else
		{
			/*	Initalize auxiliary tools for MAC table searching */
			iface_bitflag = (uint32_t)1 << (uint32_t)pfe_phy_if_get_id(iface);
			entry = pfe_l2br_table_entry_create(bridge->mac_table);
			l2t_iter = pfe_l2br_iterator_create();

			ret = pfe_l2br_domain_flush_by_if_dynamic_static(domain, bridge, l2t_iter, entry, iface_bitflag);
			/*	Release entry storage */
			(void)pfe_l2br_table_entry_destroy(entry);

			/*  Release iterator */
			(void)pfe_l2br_iterator_destroy(l2t_iter);
		}
	}
	return ret;
}

/**
 * @brief		Get list of associated physical interfaces
 * @param[in]	domain The domain instance
 * @return		Bitmask representing physical interface IDs. Every bit represents ID
 * 				corresponding to its position. Bit (1 << 3) represents ID=3. The IDs
 * 				match the pfe_ct_phy_if_id_t values.
 */
__attribute__((pure)) uint32_t pfe_l2br_domain_get_if_list(const pfe_l2br_domain_t *domain)
{
	uint32_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = domain->u.action_data.item.forward_list;
	}
	return ret;
}

/**
 * @brief		Get list of associated physical interfaces in 'untag' mode
 * @param[in]	domain The domain instance
 * @return		Bitmask representing physical interface IDs. Every bit represents ID
 * 				corresponding to its position. Bit (1 << 3) represents ID=3. The IDs
 * 				match the pfe_ct_phy_if_id_t values.
 */
__attribute__((pure)) uint32_t pfe_l2br_domain_get_untag_if_list(const pfe_l2br_domain_t *domain)
{
	uint32_t untag_if_list;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		untag_if_list = 0;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		untag_if_list = domain->u.action_data.item.untag_list;
	}
	return untag_if_list;
}

/**
 * @brief		Match entry with latest criterion provided via pfe_l2br_domain_get_first_if()
 * @param[in]	domain The L2 bridge domain instance
 * @param[in]	iface The interface to be matched
 * @retval		TRUE Interface matches the criterion
 * @retval		FALSE Interface does not match the criterion
 */
static bool_t pfe_l2br_domain_match_if_criterion(const pfe_l2br_domain_t *domain, const pfe_phy_if_t *iface)
{
	bool_t match;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == iface)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		match = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch (domain->cur_crit)
		{
			case L2BD_IF_CRIT_ALL:
				match = TRUE;
				break;

			case L2BD_IF_BY_PHY_IF_ID:
				match = (domain->cur_crit_arg.id == pfe_phy_if_get_id(iface));
				break;

			case L2BD_IF_BY_PHY_IF:
				match = (domain->cur_crit_arg.phy_if == iface);
				break;

			default:
				NXP_LOG_WARNING("Unknown criterion\n");
				match = FALSE;
				break;
		}
	}
	return match;
}

/**
 * @brief		Get first interface belonging to the domain matching given criterion
 * @param[in]	domain The domain instance
 * @param[in]	crit Get criterion
 * @param[in]	arg Pointer to criterion argument
 * @return		The interface instance or NULL if not found
 * @internal
 * @warning		Do not call this function from within the l2br module since it modifies
 * 				internal state. Caller does rely on fact that there are no unexpected,
 * 				hidden calls of this function.
 * @endinternal
 */
pfe_phy_if_t *pfe_l2br_domain_get_first_if(pfe_l2br_domain_t *domain, pfe_l2br_domain_if_get_crit_t crit, void *arg)
{
	LLIST_t *              item;
	pfe_phy_if_t *         phy_if = NULL;
	bool_t                 match = FALSE;
	bool_t                 known_crit = TRUE;
	pfe_l2br_list_entry_t *entry;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	bool_t                 is_arg_valid = TRUE;
#endif /* PFE_CFG_NULL_ARG_CHECK */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		phy_if = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Remember criterion and argument for possible subsequent pfe_l2br_get_next_domain() calls */
		domain->cur_crit = crit;
		switch (domain->cur_crit)
		{
			case L2BD_IF_CRIT_ALL:
				break;

			case L2BD_IF_BY_PHY_IF_ID:
				domain->cur_crit_arg.id = (pfe_ct_phy_if_id_t)(addr_t)arg;
				break;

			case L2BD_IF_BY_PHY_IF:
#if defined(PFE_CFG_NULL_ARG_CHECK)
				if (unlikely(NULL == arg))
				{
					NXP_LOG_ERROR("NULL argument received\n");
					phy_if = NULL;
					is_arg_valid = FALSE;
				}
				else
#endif /* PFE_CFG_NULL_ARG_CHECK */
				{
					domain->cur_crit_arg.phy_if = (pfe_phy_if_t *)arg;
				}
				break;

			default:
				NXP_LOG_WARNING("Unknown criterion\n");
				known_crit = FALSE;
				break;
		}

#if defined(PFE_CFG_NULL_ARG_CHECK)
		if (TRUE == is_arg_valid)
#endif /* PFE_CFG_NULL_ARG_CHECK */
		{
			if ((FALSE == LLIST_IsEmpty(&domain->ifaces)) && (TRUE == known_crit))
			{
				/*	Get first matching entry */
				LLIST_ForEach(item, &domain->ifaces)
				{
					/*	Get data */
					entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
					phy_if = (pfe_phy_if_t *)entry->ptr;

					/*	Remember current item to know where to start later */
					domain->cur_item = item->prNext;
					if (TRUE == pfe_l2br_domain_match_if_criterion(domain, phy_if))
					{
						match = TRUE;
						break;
					}
				}
			}
		}

		if (TRUE != match)
		{
			phy_if = NULL;
		}
	}
	return phy_if;
}

/**
 * @brief		Get next interface from the domain
 * @details		Intended to be used with pfe_l2br_domain_get_first_if().
 * @param[in]	domain The domain instance
 * @return		The interface instance or NULL if not found
 */
pfe_phy_if_t *pfe_l2br_domain_get_next_if(pfe_l2br_domain_t *domain)
{
	pfe_phy_if_t *         phy_if;
	bool_t                 match = FALSE;
	pfe_l2br_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		phy_if = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (FALSE == LLIST_IsEmpty(&domain->ifaces))
		{
			if (domain->cur_item == &domain->ifaces)
			{
				/*	No more entries */
				phy_if = NULL;
			}
			else
			{
				while (domain->cur_item != &domain->ifaces)
				{
					/*	Get data */
					entry = LLIST_Data(domain->cur_item, pfe_l2br_list_entry_t, list_entry);
					phy_if = (pfe_phy_if_t *)entry->ptr;

					/*	Remember current item to know where to start later */
					domain->cur_item = domain->cur_item->prNext;

					if (true == pfe_l2br_domain_match_if_criterion(domain, phy_if))
					{
						match = TRUE;
						break;
					}
				}
			}
		}

		if (TRUE != match)
		{
			phy_if = NULL;
		}
	}
	return phy_if;
}

/**
 * @brief		Get VLAN ID
 * @param[in]	domain The domain instance
 * @param[out]	vlan Pointer to memory where the VLAN ID shall be written
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_l2br_domain_get_vlan(const pfe_l2br_domain_t *domain, uint16_t *vlan)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == domain) || (NULL == vlan)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*vlan = domain->vlan;
		ret = EOK;
	}
	return ret;
}

/**
 * @brief		Query if domain is default domain
 * @param[in]	domain The domain instance
 * @retval		TRUE Is default
 * @retval		FALSE Is not default
 */
__attribute__((pure)) bool_t pfe_l2br_domain_is_default(const pfe_l2br_domain_t *domain)
{
	bool_t is_deft;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		is_deft = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		is_deft = domain->is_default;
	}
	return is_deft;
}

/**
 * @brief		Query if domain is fall-back domain
 * @param[in]	domain The domain instance
 * @retval		TRUE Is fall-back
 * @retval		FALSE Is not fall-back
 */
__attribute__((pure)) bool_t pfe_l2br_domain_is_fallback(const pfe_l2br_domain_t *domain)
{
	bool_t is_fallback;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		is_fallback = FALSE;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		is_fallback = domain->is_fallback;
	}
	return is_fallback;
}

/**
 * @brief		Add L2 bridge static entry
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @param[in]	mac Static entry MAC address
 * @param[in]	new_fw_list forward list of static entry
 * @param[in]	static_entry pointer store config static entry
 * @retval		EOK Success
 * @retval		EPERM Operation not permitted (static entry already created)
 */
static errno_t pfe_l2br_set_static_entry(const pfe_l2br_t *bridge, uint16_t vlan, const pfe_mac_addr_t mac, uint32_t new_fw_list, pfe_l2br_static_entry_t **static_entry)
{
	errno_t ret = EINVAL;

	(*static_entry)->entry = pfe_l2br_table_entry_create(bridge->mac_table);

	if (NULL == (*static_entry)->entry)
	{
		NXP_LOG_ERROR("malloc() failed\n");
		oal_mm_free(*static_entry);
		ret = ENOMEM;
	}
	else
	{
		/* Configure action data */
		(*static_entry)->u.action_data.val = 0;
		(*static_entry)->u.action_data.item.static_flag = 1;
		(*static_entry)->u.action_data.item.fresh_flag = 0U;
		(*static_entry)->u.action_data.item.local_l3 = 0U;
		(*static_entry)->u.action_data.item.forward_list = new_fw_list;

		if (EOK != pfe_l2br_table_entry_set_vlan((*static_entry)->entry, vlan))
		{
			NXP_LOG_ERROR("Couldn't set vlan\n");
			pfe_l2br_table_entry_destroy((*static_entry)->entry);
			oal_mm_free(*static_entry);
			ret = EINVAL;
		}
		else if (EOK != pfe_l2br_table_entry_set_mac_addr((*static_entry)->entry, mac))
		{
			NXP_LOG_ERROR("Couldn't set mac address\n");
			pfe_l2br_table_entry_destroy((*static_entry)->entry);
			oal_mm_free(*static_entry);
			ret = EINVAL;
		}
		else if (EOK != pfe_l2br_table_entry_set_action_data((*static_entry)->entry, (*static_entry)->u.action_data_u64val))
		{
			NXP_LOG_ERROR("Couldn't set action data\n");
			pfe_l2br_table_entry_destroy((*static_entry)->entry);
			oal_mm_free(*static_entry);
			ret = EINVAL;
		}
		else if (EOK != pfe_l2br_table_add_entry(bridge->mac_table, (*static_entry)->entry))
		{
			NXP_LOG_ERROR("Couldn't set action data\n");
			pfe_l2br_table_entry_destroy((*static_entry)->entry);
			oal_mm_free(*static_entry);
			ret	 = EINVAL;
		}
		else
		{
			ret = EOK;
		}
	}

	return ret;
}

/**
 * @brief		Add L2 bridge static entry
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @param[in]	mac Static entry MAC address
 * @param[in]	new_fw_list forward list of static entry
 * @param[in]	static_entry store static entry
 * @retval		EOK Success
 * @retval		EPERM Operation not permitted (static entry already created)
 */
static errno_t pfe_l2br_static_add_entry(pfe_l2br_t *bridge, uint16_t vlan, const pfe_mac_addr_t mac, uint32_t new_fw_list, pfe_l2br_static_entry_t *static_entry)
{
	bool_t match = FALSE;
	LLIST_t *item;
	errno_t ret = EOK;
	pfe_l2br_static_entry_t *static_ent_tmp;

	if (EOK != oal_mutex_lock(bridge->mutex))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}

	if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
	{
		/*	Get first matching entry */
		LLIST_ForEach(item, &bridge->static_entries)
		{
			/*	Get data */
			static_ent_tmp = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);

			/*	Remember current item to know where to start later */
			bridge->curr_static_ent = item->prNext;
			if ((static_ent_tmp->vlan == vlan) && (0 == memcmp(static_ent_tmp->mac, mac, sizeof(pfe_mac_addr_t))))
			{
				match = TRUE;
				break;
			}
		}
	}

	if (EOK != oal_mutex_unlock(bridge->mutex))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}

	if (TRUE == match)
	{
		NXP_LOG_ERROR("Duplicit entry\n");
		/* Entry is duplicit */
		oal_mm_free(static_entry);
		ret = EPERM;
	}
	else
	{
		ret = pfe_l2br_set_static_entry(bridge, vlan, mac, new_fw_list, &static_entry);
		if(EOK == ret)
		{
			if (EOK != oal_mutex_lock(bridge->mutex))
			{
				NXP_LOG_ERROR("Mutex lock failed\n");
			}

			LLIST_AddAtEnd(&static_entry->list_entry, &bridge->static_entries);

			if (EOK != oal_mutex_unlock(bridge->mutex))
			{
				NXP_LOG_ERROR("Mutex unlock failed\n");
			}
		}
	}

	return ret;
}

/**
 * @brief		Create L2 bridge static entry
 * @param[in]	vlan VLAN ID to identify the bridge domain
 * @param[in]	mac Static entry MAC address
 * @param[in]	new_fw_list forward list of static entry
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOMEM Failure when allocating memory
 * @retval		ETIMEDOUT Timed out
 * @retval		EPERM Operation not permitted (static entry already created)
 */
errno_t pfe_l2br_static_entry_create(pfe_l2br_t *bridge, uint16_t vlan, const pfe_mac_addr_t mac, uint32_t new_fw_list)
{
	pfe_l2br_static_entry_t *static_entry;
	errno_t                  ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		static_entry = oal_mm_malloc(sizeof(pfe_l2br_static_entry_t));

		if (NULL == static_entry)
		{
			NXP_LOG_ERROR("malloc() failed\n");
			ret = ENOMEM;
		}
		else
		{
			(void)memset(static_entry, 0, sizeof(pfe_l2br_static_entry_t));
			static_entry->vlan = vlan;
			(void)memcpy(static_entry->mac, mac, sizeof(pfe_mac_addr_t));

			ret = pfe_l2br_static_add_entry(bridge, vlan, mac, new_fw_list, static_entry);
			if (EOK != ret)
			{
				static_entry = NULL;
			}
		}
	}

	return ret;
}

static errno_t pfe_l2br_static_entry_destroy_nolock(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		LLIST_Remove(&static_ent->list_entry);

		ret = pfe_l2br_table_del_entry(bridge->mac_table, static_ent->entry);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Static entry couldn't be deleted from HW table (errno %d)\n", ret);
		}

		oal_mm_free(static_ent);
	}
	return ret;
}

/**
 * @brief		Destroy L2 bridge static entry
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to be deleted
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ETIMEDOUT Timed out
 */
errno_t pfe_l2br_static_entry_destroy(pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		ret = pfe_l2br_static_entry_destroy_nolock(bridge, static_ent);

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}
	return ret;
}

/**
 * @brief		Change L2 static entry forward list
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to change
 * @param[in]	new_fw_list New forward list
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Entry couldn't be updated
 */
errno_t pfe_l2br_static_entry_replace_fw_list(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent, uint32_t new_fw_list)
{
	errno_t  ret;
	uint32_t tmp;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		tmp = static_ent->u.action_data.item.forward_list;
		static_ent->u.action_data.item.forward_list = new_fw_list;

		if (EOK != pfe_l2br_table_entry_set_action_data(static_ent->entry, static_ent->u.action_data_u64val))
		{
			static_ent->u.action_data.item.forward_list = tmp;
			NXP_LOG_ERROR("Couldn't set action data\n");
			ret = EINVAL;
		}

		else if (EOK != pfe_l2br_table_update_entry(bridge->mac_table, static_ent->entry))
		{
			static_ent->u.action_data.item.forward_list = tmp;
			NXP_LOG_ERROR("Couldn't update entry\n");
			ret = ENOENT;
		}
		else
		{
			ret = EOK;
		}
	}
	return ret;
}

/**
 * @brief Sets the local L3 flag (marks/unmarks the MAC address as local one)
 * @param[in]		bridge Bridge instance
 * @param[in]		static_ent Static entry to change
 * @param[in]		local Value to be set
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 * @retval			ENOENT Entry couldn't be updated
 */
errno_t pfe_l2br_static_entry_set_local_flag(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent, bool_t local)
{
	uint32_t tmp;
	errno_t  ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Make changes */
		tmp = static_ent->u.action_data.item.local_l3;
		static_ent->u.action_data.item.local_l3 = ((FALSE != local) ? 1U : 0U);
		/* Propagate changes to l2br table */
		if (EOK != pfe_l2br_table_entry_set_action_data(static_ent->entry, static_ent->u.action_data_u64val))
		{
			static_ent->u.action_data.item.local_l3 = tmp;
			NXP_LOG_ERROR("Couldn't set action data\n");
			ret = EINVAL;
		}
		/* Write to the HW */
		else if (EOK != pfe_l2br_table_update_entry(bridge->mac_table, static_ent->entry))
		{
			static_ent->u.action_data.item.local_l3 = tmp;
			NXP_LOG_ERROR("Couldn't update entry\n");
			ret = ENOENT;
		}
		else
		{
			ret = EOK;
		}
	}
	return ret;
}

/**
 * @brief Sets the src_discard flag (enables/disables discard of frames with given SRC MAC address)
 * @param[in]		bridge Bridge instance
 * @param[in]		static_ent Static entry to change
 * @param[in]		src_discard Value to be set
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 * @retval			ENOENT Entry couldn't be updated
 */
errno_t pfe_l2br_static_entry_set_src_discard_flag(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent, bool_t src_discard)
{
	uint32_t tmp;
	errno_t  ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/* Make changes */
		tmp = static_ent->u.action_data.item.src_discard;
		static_ent->u.action_data.item.src_discard = ((FALSE != src_discard) ? 1U : 0U);
		/* Propagate changes to l2br table */
		if (EOK != pfe_l2br_table_entry_set_action_data(static_ent->entry, static_ent->u.action_data_u64val))
		{
			static_ent->u.action_data.item.src_discard = tmp;
			NXP_LOG_ERROR("Couldn't set action data\n");
			ret = EINVAL;
		}
		/* Write to the HW */
		else if (EOK != pfe_l2br_table_update_entry(bridge->mac_table, static_ent->entry))
		{
			static_ent->u.action_data.item.src_discard = tmp;
			NXP_LOG_ERROR("Couldn't update entry\n");
			ret = ENOENT;
		}
		else
		{
			ret = EOK;
		}
	}
	return ret;
}

/**
 * @brief Sets the dst_discard flag (enables/disables discard of frames with given SRC MAC address)
 * @param[in]		bridge Bridge instance
 * @param[in]		static_ent Static entry to change
 * @param[in]		dst_discard Value to be set
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 * @retval			ENOENT Entry couldn't be updated
 */
errno_t pfe_l2br_static_entry_set_dst_discard_flag(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t* static_ent, bool_t dst_discard)
{
	uint32_t tmp;
    errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
        /* Make changes */
        tmp = static_ent->u.action_data.item.dst_discard;
        static_ent->u.action_data.item.dst_discard = ((FALSE != dst_discard)? 1U : 0U);
        /* Propagate changes to l2br table */
        ret = pfe_l2br_table_entry_set_action_data(static_ent->entry, static_ent->u.action_data_u64val);
        if (EOK != ret)
        {
            static_ent->u.action_data.item.dst_discard = tmp;
            NXP_LOG_ERROR("Couldn't set action data\n");
            ret = EINVAL;
        }
        else
        {
            /* Write to the HW */
            if (EOK != pfe_l2br_table_update_entry(bridge->mac_table, static_ent->entry))
            {
                static_ent->u.action_data.item.dst_discard = tmp;
                NXP_LOG_ERROR("Couldn't update entry\n");
                ret = ENOENT;
            }
        }
    }

	return ret;
}

/**
 * @brief Reads the state of local L3 flag
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to read from
 * @param[out]	local State of the local L3 flag
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 */
errno_t pfe_l2br_static_entry_get_local_flag(const pfe_l2br_t *bridge, const pfe_l2br_static_entry_t* static_ent, bool_t *local)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent) || (NULL == local)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#else
	(void)bridge;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*local = ((0U != static_ent->u.action_data.item.local_l3) ? TRUE : FALSE);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Reads the state of src_discard flag
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to read from
 * @param[out]	src_discard State of the src_discard flag
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 */
errno_t pfe_l2br_static_entry_get_src_discard_flag(pfe_l2br_t *bridge, const pfe_l2br_static_entry_t* static_ent, bool_t *src_discard)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent) || (NULL == src_discard)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#else
	(void)bridge;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*src_discard = ((0U != static_ent->u.action_data.item.src_discard) ? TRUE : FALSE);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Reads the state of dst_discard flag
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to read from
 * @param[out]	dst_discard State of the dst_discard flag
 * @retval EOK		Success
 * @retval EINVAL	Invalid or missing argument
 */
errno_t pfe_l2br_static_entry_get_dst_discard_flag(const pfe_l2br_t *bridge, const pfe_l2br_static_entry_t* static_ent, bool_t *dst_discard)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent) || (NULL == dst_discard)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#else
	(void)bridge;
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		*dst_discard = ((0U != static_ent->u.action_data.item.dst_discard) ? TRUE : FALSE);
		ret = EOK;
	}
	return ret;
}

/**
 * @brief Reads the forward list
 * @param[in]	bridge Bridge instance
 * @param[in]	static_ent Static entry to read from
 * @return		Bitmask representing egress physical interfaces. Every bit represents ID
 * 				corresponding to its position. Bit (1 << 3) represents ID=3. The IDs
 * 				match the pfe_ct_phy_if_id_t values.
 */
__attribute__((pure)) uint32_t pfe_l2br_static_entry_get_fw_list(const pfe_l2br_static_entry_t* static_ent)
{
	uint32_t fw_list;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (NULL == static_ent)
	{
		NXP_LOG_ERROR("NULL argument received\n");
		fw_list = 0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		fw_list = static_ent->u.action_data.item.forward_list;
	}
	return fw_list;
}

/**
 * @brief		Get vlan from L2 static entry
 * @param[in]	static_ent Static entry
 * @return		Vlan of static entry
 */
__attribute__((pure)) uint16_t pfe_l2br_static_entry_get_vlan(const pfe_l2br_static_entry_t *static_ent)
{
	return static_ent->vlan;
}

/**
 * @brief		Get MAC from L2 static entry
 * @param[in]	static_ent Static entry
 * @return		Mac of static entry
 */
void pfe_l2br_static_entry_get_mac(const pfe_l2br_static_entry_t *static_ent, pfe_mac_addr_t mac)
{
	(void)memcpy(mac, static_ent->mac, sizeof(pfe_mac_addr_t));
}

/**
 * @brief		Get first L2 static entry based on criterion
 * @param[in]	bridge Bridge instance
 * @param[in]	crit Static entry to change forward list
 * @param[in]	arg1 Argument for criterion
 * @param[in]	arg2 Argument for criterion
 * @return		Static entry on success or NULL on failure
 */
pfe_l2br_static_entry_t *pfe_l2br_static_entry_get_first(pfe_l2br_t *bridge, pfe_l2br_static_ent_get_crit_t crit, void* arg1,const void *arg2)
{
	bool_t                   match = FALSE;
	LLIST_t *                item;
	pfe_l2br_static_entry_t *static_ent;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		static_ent = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		bridge->cur_crit_ent = crit;

		switch (bridge->cur_crit_ent)
		{
			case L2SENT_CRIT_ALL:
				break;

			case L2SENT_CRIT_BY_MAC:
				(void)memcpy((void *)bridge->cur_static_ent_crit_arg.mac, arg2, sizeof(pfe_mac_addr_t));
				break;

			case L2SENT_CRIT_BY_VLAN:
				bridge->cur_static_ent_crit_arg.vlan = (uint16_t)((addr_t)arg1);
				break;

			case L2SENT_CRIT_BY_MAC_VLAN:
				bridge->cur_static_ent_crit_arg.vlan = (uint16_t)((addr_t)arg1);
				(void)memcpy((void *)bridge->cur_static_ent_crit_arg.mac, arg2, sizeof(pfe_mac_addr_t));
				break;

			default:
				NXP_LOG_WARNING("Invalid static entry type");
				break;
		}

		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
		{
			/*	Get first matching entry */
			LLIST_ForEach(item, &bridge->static_entries)
			{
				/*	Get data */
				static_ent = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);

				/*	Remember current item to know where to start later */
				bridge->curr_static_ent = item->prNext;
				if (TRUE == pfe_l2br_static_entry_match_criterion(bridge, static_ent))
				{
					match = TRUE;
					break;
				}
			}
		}

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		if (TRUE != match)
		{
			static_ent = NULL;
		}
	}
	return static_ent;
}
/**
 * @brief		Get next L2 static entry
 * @param[in]	bridge Bridge instance
 * @return		Static entry on success or NULL on failure
 * @warning		Intended to be called after pfe_l2br_static_entry_get_first.
 */
pfe_l2br_static_entry_t *pfe_l2br_static_entry_get_next(pfe_l2br_t *bridge)
{
	pfe_l2br_static_entry_t *static_ent;
	bool_t                   match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		static_ent = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
		{
			if (bridge->curr_static_ent == &bridge->static_entries)
			{
				/*	No more entries */
				static_ent = NULL;
			}
			else
			{
				while (bridge->curr_static_ent != &bridge->static_entries)
				{
					/*	Get data */
					static_ent = LLIST_Data(bridge->curr_static_ent, pfe_l2br_static_entry_t, list_entry);

					/*	Remember current item to know where to start later */
					bridge->curr_static_ent = bridge->curr_static_ent->prNext;

					if (NULL != static_ent)
					{
						if (true == pfe_l2br_static_entry_match_criterion(bridge, static_ent))
						{
							match = TRUE;
							break;
						}
					}
				}
			}
		}

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		if (TRUE != match)
		{
			static_ent = NULL;
		}
	}
	return static_ent;
}

/**
 * @brief		Match static entry
 * @param[in]	bridge The bridge instance
 * @param[in]	static_ent Static entry to be matched to criterion parameters
 */
static bool_t pfe_l2br_static_entry_match_criterion(const pfe_l2br_t *bridge, pfe_l2br_static_entry_t *static_ent)
{
	bool_t match = FALSE;;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == static_ent)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch (bridge->cur_crit_ent)
		{
			case L2SENT_CRIT_ALL:
				match = TRUE;
				break;

			case L2SENT_CRIT_BY_MAC:
				if (0 == memcmp(static_ent->mac, bridge->cur_static_ent_crit_arg.mac, sizeof(pfe_mac_addr_t)))
				{
					match = TRUE;
				}
				break;

			case L2SENT_CRIT_BY_VLAN:
				match = (static_ent->vlan == bridge->cur_static_ent_crit_arg.vlan);
				break;

			case L2SENT_CRIT_BY_MAC_VLAN:
				match = (static_ent->vlan == bridge->cur_static_ent_crit_arg.vlan);
				if (TRUE == match)
				{
					if (0 == memcmp(static_ent->mac, bridge->cur_static_ent_crit_arg.mac, sizeof(pfe_mac_addr_t)))
					{
						match = TRUE;
					}
					else
					{
						match = FALSE;
					}
				}
				break;

			default:
				NXP_LOG_WARNING("Unknown criterion\n");
				match = FALSE;
				break;
		}
	}

	return match;
}

/*
 * @brief		Flush static MAC table entries
 * @param[in]	bridge The bridge instance
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_l2br_flush_static_mac_table(pfe_l2br_t *bridge)
{
	errno_t ret = EOK;
	LLIST_t *item;
	LLIST_t *aux;
	pfe_l2br_static_entry_t *sentry;

	/*	Remove all static entries from local DB */
	if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
	{
		/*	Get first matching entry */
		LLIST_ForEachRemovable(item, aux, &bridge->static_entries)
		{
			/*	Get data */
			sentry = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);

			/*	Destroy entry. LLIST_Remove() is inside... */
			ret = pfe_l2br_static_entry_destroy_nolock(bridge, sentry);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Unable to remove static entry: %d\n", ret);
			}
		}
	}

	return ret;
}

/*
 * @brief		Flush all MAC table entries
 * @param[in]	bridge The bridge instance
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_l2br_flush_all_mac_table(const pfe_l2br_t *bridge)
{
	errno_t ret = EOK;
	LLIST_t *item;
	LLIST_t *aux;
	pfe_l2br_static_entry_t *sentry;

	/*	Remove all static entries from local DB. This must be done before
	the pfe_l2br_table_flush() because otherwise would report "entry
	not found" messages. */
	if (FALSE == LLIST_IsEmpty(&bridge->static_entries))
	{
		/*	Get first matching entry */
		LLIST_ForEachRemovable(item, aux, &bridge->static_entries)
		{
			/*	Get data */
			sentry = LLIST_Data(item, pfe_l2br_static_entry_t, list_entry);

			/*	Destroy entry. LLIST_Remove() is inside... */
			ret = pfe_l2br_static_entry_destroy_nolock(bridge, sentry);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Unable to remove static entry: %d\n", ret);
			}
		}
	}

	/*	Flush MAC table */
#if 0 /* AAVB-3136: THIS DOES NOT WORK. PFE GETS STUCK. */
	ret = pfe_l2br_table_flush(bridge->mac_table);
#else
	ret = pfe_l2br_table_init(bridge->mac_table);
#endif /* AAVB-3136 */
	if (EOK != ret)
	{
		NXP_LOG_ERROR("MAC table flush failed: %d\n", ret);
	}
	else
	{
		NXP_LOG_INFO("MAC table flushed\n");
	}
	return ret;
}

/*
 * @brief		Flush learned MAC table entries
 * @param[in]	bridge The bridge instance
 * @param[in]	entry Entry will be written at this location
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_l2br_flush_learned_mac_table(const pfe_l2br_t *bridge, pfe_l2br_table_entry_t *entry, pfe_l2br_table_iterator_t *l2t_iter)
{
	errno_t ret = EOK;
	errno_t query_ret;

	/*	Go through all entries */
	query_ret = pfe_l2br_table_get_first(bridge->mac_table, l2t_iter, L2BR_TABLE_CRIT_VALID, entry);
	while (EOK == query_ret)
	{
		if (FALSE == pfe_l2br_table_entry_is_static(entry))
		{
			/*	Remove non-static entry from table */
			ret = pfe_l2br_table_del_entry(bridge->mac_table, entry);
			if (EOK != ret)
			{
				NXP_LOG_ERROR("Could not delete MAC table entry: %d\n", ret);
			}
		}

		query_ret = pfe_l2br_table_get_next(bridge->mac_table, l2t_iter, entry);
	}

	return ret;
}

/*
 * @brief		Flush MAC table entries
 * @param[in]	bridge The bridge instance
 * @param[in]	type Type of the flush
 * @return		EOK if success, error code otherwise
 */
static errno_t pfe_l2br_flush(pfe_l2br_t *bridge, pfe_l2br_flush_types type)
{
	errno_t                    ret = EOK;
	pfe_l2br_table_entry_t *   entry;
	pfe_l2br_table_iterator_t *l2t_iter;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Create entry storage */
		entry = pfe_l2br_table_entry_create(bridge->mac_table);

		/*	 Create iterator */
		l2t_iter = pfe_l2br_iterator_create();

		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		switch (type)
		{
			case PFE_L2BR_FLUSH_STATIC_MAC:
			{
				ret = pfe_l2br_flush_static_mac_table(bridge);
				break;
			}

			case PFE_L2BR_FLUSH_ALL_MAC:
			{
				/*	Remove all static entries from local DB. This must be done before
				the pfe_l2br_table_flush() because otherwise would report "entry
				not found" messages. */
				ret = pfe_l2br_flush_all_mac_table(bridge);
				break;
			}

			case PFE_L2BR_FLUSH_LEARNED_MAC:
			{
				ret = pfe_l2br_flush_learned_mac_table(bridge, entry, l2t_iter);
				break;
			}

			default:
			{
				NXP_LOG_WARNING("Invalid flush type");
				ret = EINVAL;
				break;
			}
		}

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		/*	Release entry storage */
		(void)pfe_l2br_table_entry_destroy(entry);

		/*  Release iterator */
		(void)pfe_l2br_iterator_destroy(l2t_iter);
	}
	return ret;
}

/**
 * @brief		Flush all learned MAC table entries
 * @param[in]	bridge The bridge instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_flush_learned(pfe_l2br_t *bridge)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_l2br_flush(bridge, PFE_L2BR_FLUSH_LEARNED_MAC);
	}
	return ret;
}

/**
 * @brief		Flush all static MAC table entries
 * @param[in]	bridge The bridge instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_flush_static(pfe_l2br_t *bridge)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_l2br_flush(bridge, PFE_L2BR_FLUSH_STATIC_MAC);
	}
	return ret;
}

/**
 * @brief		Flush all MAC table entries
 * @param[in]	bridge The bridge instance
 * @return		EOK if success, error code otherwise
 */
errno_t pfe_l2br_flush_all(pfe_l2br_t *bridge)
{
	errno_t ret;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		ret = pfe_l2br_flush(bridge, PFE_L2BR_FLUSH_ALL_MAC);
	}
	return ret;
}
/**
 * @brief		Create L2 bridge domain instance
 * @param[in]	class The classifier instance
 * @param[in]	bridge The bridge instance
 * @param[in]	def_vlan Default VLAN
 * @param[in]	def_aging_time Default aging timeout in seconds.
 * @return		The instance or NULL if failed
 */
static void pfe_l2br_create_domain(const pfe_class_t *class, pfe_l2br_t **bridge, uint16_t def_vlan, uint16_t def_aging_time)
{
	/*	Create default domain */
	(*bridge)->default_domain = pfe_l2br_create_default_domain(*bridge, def_vlan);
	if (NULL == (*bridge)->default_domain)
	{
		NXP_LOG_ERROR("Could not create default domain\n");
		(void)pfe_l2br_destroy(*bridge);
		*bridge = NULL;
	}
	else
	{
		/*	Create fallback domain */
		(*bridge)->fallback_domain = pfe_l2br_create_fallback_domain(*bridge);
		if (NULL == (*bridge)->fallback_domain)
		{
			NXP_LOG_ERROR("Could not create fallback domain\n");
			(void)pfe_l2br_destroy(*bridge);
			*bridge = NULL;
		}
		else
		{
			/*	Configure classifier */
			(void)pfe_class_set_default_vlan(class, def_vlan);

			if (EOK != pfe_l2br_set_mac_aging_timeout((*bridge)->class, def_aging_time))
			{
				NXP_LOG_ERROR("Could not set mac aging timeout\n");
				(void)pfe_l2br_destroy(*bridge);
				*bridge = NULL;
			}

			/*	If the FW aging is off, turn it on */
			else if (FALSE == pfe_feature_mgr_is_available("l2_bridge_aging"))
			{
				if (EOK != pfe_feature_mgr_enable("l2_bridge_aging"))
				{
					NXP_LOG_ERROR("Could not enable L2 bridge aging in FW\n");
					(void)pfe_l2br_destroy(*bridge);
					*bridge = NULL;
				}
			}
			else
			{
				/* Required by MISRA */
			}
		}
	}
}

/**
 * @brief		Create L2 bridge instance
 * @param[in]	class The classifier instance
 * @param[in]	def_vlan Default VLAN
 * @param[in]	def_aging_time Default aging timeout in seconds.
 * @param[in]	mac_table The MAC table instance
 * @param[in]	vlan_table The VLAN table instance
 * @return		The instance or NULL if failed
 */
pfe_l2br_t *pfe_l2br_create(pfe_class_t *class, uint16_t def_vlan, uint16_t def_aging_time, uint16_t vlan_stats_size, pfe_l2br_table_t *mac_table, pfe_l2br_table_t *vlan_table)
{
	pfe_l2br_t *bridge;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == class) || (NULL == mac_table) || (NULL == vlan_table)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		bridge = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		bridge = oal_mm_malloc(sizeof(pfe_l2br_t));

		if (NULL == bridge)
		{
			NXP_LOG_ERROR("malloc() failed\n");
		}
		else
		{
			(void)memset(bridge, 0, sizeof(pfe_l2br_t));
			bridge->class = class;
			bridge->mac_table = mac_table;
			bridge->vlan_table = vlan_table;
			bridge->def_vlan = def_vlan;
			LLIST_Init(&bridge->domains);
			LLIST_Init(&bridge->static_entries);

			bridge->mutex = oal_mm_malloc(sizeof(oal_mutex_t));
			if (NULL == bridge->mutex)
			{
				NXP_LOG_ERROR("Memory allocation failed\n");
				(void)pfe_l2br_destroy(bridge);
				bridge = NULL;
			}
			else if (EOK != oal_mutex_init(bridge->mutex))
			{
				NXP_LOG_ERROR("Could not initialize mutex\n");
				oal_mm_free(bridge->mutex);
				bridge->mutex = NULL;
				(void)pfe_l2br_destroy(bridge);
				bridge = NULL;
			}
			else
			{

				bridge->domain_stats_table_size = vlan_stats_size;

				(void)memset(&stats_index, 0, sizeof(stats_index));

				bridge->domain_stats_table_addr = pfe_l2br_create_vlan_stats_table(class, vlan_stats_size);

				pfe_l2br_create_domain(class, &bridge, def_vlan, def_aging_time);
			}
		}
	}

	return bridge;
}

/**
 * @brief		Destroy L2 Bridge instance
 * @param[in]	bridge The bridge instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 */
errno_t pfe_l2br_destroy(pfe_l2br_t *bridge)
{
	errno_t ret;
	if (NULL != bridge)
	{
		if (NULL != bridge->default_domain)
		{
			if (EOK == pfe_l2br_domain_destroy(bridge->default_domain))
			{
				bridge->default_domain = NULL;
			}
			else
			{
				NXP_LOG_ERROR("Could not destroy default domain\n");
			}
		}

		if (NULL != bridge->fallback_domain)
		{
			if (EOK == pfe_l2br_domain_destroy(bridge->fallback_domain))
			{
				bridge->fallback_domain = NULL;
			}
			else
			{
				NXP_LOG_ERROR("Could not destroy fallback domain\n");
			}
		}

		if (FALSE == LLIST_IsEmpty(&bridge->domains))
		{
			NXP_LOG_WARNING("Bridge is being destroyed but still contains some active domains\n");
		}

		if (EOK != pfe_l2br_destroy_vlan_stats_table(bridge->class, bridge->domain_stats_table_addr))
		{
			NXP_LOG_ERROR("Could not destroy vlan stats\n");
		}

		if (NULL != bridge->mutex)
		{
			if (EOK != oal_mutex_destroy(bridge->mutex))
			{
				NXP_LOG_ERROR("Could not destroy mutex\n");
			}

			oal_mm_free(bridge->mutex);
			bridge->mutex = NULL;
		}

		oal_mm_free(bridge);
		ret = EOK;
	}
	else
	{
		NXP_LOG_DEBUG("Argument is NULL\n");
		ret = EINVAL;
	}

	return ret;
}

/**
 * @brief		Get the default bridge domain instance
 * @param[in]	bridge The bridge instance
 * @return		The domain instance or NULL if failed
 */
__attribute__((pure)) pfe_l2br_domain_t *pfe_l2br_get_default_domain(const pfe_l2br_t *bridge)
{
	pfe_l2br_domain_t *default_domain;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		default_domain = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		default_domain = bridge->default_domain;
	}
	return default_domain;
}

/**
 * @brief		Get the fall-back bridge domain instance
 * @param[in]	bridge The bridge instance
 * @return		The domain instance or NULL if failed
 */
__attribute__((pure)) pfe_l2br_domain_t *pfe_l2br_get_fallback_domain(const pfe_l2br_t *bridge)
{
	pfe_l2br_domain_t *fallback_domain;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		fallback_domain = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		fallback_domain = bridge->fallback_domain;
	}
	return fallback_domain;
}

/**
 * @brief		Match entry with latest criterion provided via pfe_l2br_get_first_domain()
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	domain The domain to be matched
 * @retval		TRUE Domain matches the criterion
 * @retval		FALSE Domain does not match the criterion
 */
static bool_t pfe_l2br_domain_match_criterion(const pfe_l2br_t *bridge, pfe_l2br_domain_t *domain)
{
	bool_t                 match = FALSE;
	LLIST_t *              item;
	pfe_l2br_list_entry_t *entry;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == domain)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		switch (bridge->cur_crit)
		{
			case L2BD_CRIT_ALL:
				match = TRUE;
				break;

			case L2BD_CRIT_BY_VLAN:
				match = (domain->vlan == bridge->cur_domain_crit_arg.vlan);
				break;

			case L2BD_BY_PHY_IF:
				/*	Find out if domain contains given interface */
				if (FALSE == LLIST_IsEmpty(&domain->ifaces))
				{
					LLIST_ForEach(item, &domain->ifaces)
					{
						entry = LLIST_Data(item, pfe_l2br_list_entry_t, list_entry);
						match = (((pfe_phy_if_t *)entry->ptr) == bridge->cur_domain_crit_arg.phy_if);
						if (TRUE == match)
						{
							break;
						}
					}
				}
				break;

			default:
				NXP_LOG_WARNING("Unknown criterion\n");
				match = FALSE;
				break;
		}
	}
	return match;
}

/**
 * @brief		Get first L2 bridge domain instance according to given criterion
 * @param[in]	bridge The L2 bridge instance
 * @param[in]	crit Get criterion
 * @param[in]	arg Pointer to criterion argument
 * @return		The domain instance or NULL if not found
 */
pfe_l2br_domain_t *pfe_l2br_get_first_domain(pfe_l2br_t *bridge, pfe_l2br_domain_get_crit_t crit, void *arg)
{
	LLIST_t *          item;
	pfe_l2br_domain_t *domain = NULL;
	bool_t             match = FALSE;
	bool_t             known_crit = TRUE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		domain = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		/*	Remember criterion and argument for possible subsequent pfe_l2br_get_next_domain() calls */
		bridge->cur_crit = crit;
		switch (bridge->cur_crit)
		{
			case L2BD_CRIT_ALL:
				break;

			case L2BD_CRIT_BY_VLAN:
				bridge->cur_domain_crit_arg.vlan = (uint16_t)((addr_t)arg & 0xffffU);
				break;

			case L2BD_BY_PHY_IF:
				bridge->cur_domain_crit_arg.phy_if = (pfe_phy_if_t *)arg;
				break;

			default:
				NXP_LOG_WARNING("Unknown criterion\n");
				known_crit = FALSE;
				break;
		}

		if ((FALSE == LLIST_IsEmpty(&bridge->domains)) && (TRUE == known_crit))
		{
			/*	Get first matching entry */
			LLIST_ForEach(item, &bridge->domains)
			{
				/*	Get data */
				domain = LLIST_Data(item, pfe_l2br_domain_t, list_entry);

				/*	Remember current item to know where to start later */
				bridge->curr_domain = item->prNext;
				if (TRUE == pfe_l2br_domain_match_criterion(bridge, domain))
				{
					match = TRUE;
					break;
				}
			}
		}

		if (TRUE != match)
		{
			domain = NULL;
		}
	}
	return domain;
}

/**
 * @brief		Get next domain from the bridge
 * @details		Intended to be used with pfe_l2br_get_first_domain().
 * @param[in]	bridge The L2 bridge instance
 * @return		The domain instance or NULL if not found
 * @warning		The returned entry must not be accessed after fci_l2br_domain_destroy(entry) has been called.
 */
pfe_l2br_domain_t *pfe_l2br_get_next_domain(pfe_l2br_t *bridge)
{
	pfe_l2br_domain_t *domain;
	bool_t             match = FALSE;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		domain = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		if (FALSE == LLIST_IsEmpty(&bridge->domains))
		{
			if (bridge->curr_domain == &bridge->domains)
			{
				/*	No more entries */
				domain = NULL;
			}
			else
			{
				while (bridge->curr_domain != &bridge->domains)
				{
					/*	Get data */
					domain = LLIST_Data(bridge->curr_domain, pfe_l2br_domain_t, list_entry);

					/*	Remember current item to know where to start later */
					bridge->curr_domain = bridge->curr_domain->prNext;

					if (NULL != domain)
					{
						if (true == pfe_l2br_domain_match_criterion(bridge, domain))
						{
							match = TRUE;
							break;
						}
					}
				}
			}
		}

		if (TRUE != match)
		{
			domain = NULL;
		}
	}
	return domain;
}

/**
 * @brief Configures the l2 bridge mac aging timeout
 * @param[in] class The classifier instance
 * @param[in] timeout Timeout time in seconds.
 * @return Either EOK or error code.
 */
static errno_t pfe_l2br_set_mac_aging_timeout(pfe_class_t *class, const uint16_t timeout)
{
	pfe_ct_class_mmap_t  mmap;
	pfe_ct_misc_config_t misc_config;
	errno_t  ret;
	uint32_t ff_addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		misc_config.l2_mac_aging_timeout = oal_htons(timeout);

		/* Get the memory map */
		/* All PEs share the same memory map therefore we can read
		arbitrary one (in this case 0U)
		Also mac aging algorithm will work only on core 0*/
		ret = pfe_class_get_mmap(class, 0, &mmap);
		if (EOK == ret)
		{
			/* Get the misc address */
			ff_addr = oal_ntohl(mmap.common.misc_config);
			/* Write new address of misc config */
			ret = pfe_class_write_dmem(class, 0, (addr_t)ff_addr, (void *)&misc_config, sizeof(pfe_ct_misc_config_t));
		}
	}
	return ret;
}

/**
 * @brief		Return L2 Bridge runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	bridge		The L2 Bridge instance
 * @param[in]	seq			Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_l2br_get_text_statistics(const pfe_l2br_t *bridge, struct seq_file *seq, uint8_t verb_level)
{
    pfe_l2br_table_entry_t *entry;
    pfe_l2br_table_iterator_t* l2t_iter;
    errno_t ret;
    uint32_t count = 0U;

	/* We keep unused parameter verb_level for consistency with rest of the *_get_text_statistics() functions */
	(void)verb_level;

    /* Get memory */
    entry = pfe_l2br_table_entry_create(bridge->mac_table);
    /* Get the first entry */
    l2t_iter = pfe_l2br_iterator_create();

    ret = pfe_l2br_table_get_first(bridge->mac_table, l2t_iter, L2BR_TABLE_CRIT_VALID, entry);
    while (EOK == ret)
    {
        /* Print out the entry */
        pfe_l2br_table_entry_to_str(entry, seq);
        count++;
        /* Get the next entry */
        ret = pfe_l2br_table_get_next(bridge->mac_table, l2t_iter, entry);
    }
    seq_printf(seq, "\n MAC entries count: %u\n", count);
    /* Free memory */
    (void)pfe_l2br_table_entry_destroy(entry);
    (void)pfe_l2br_iterator_destroy(l2t_iter);
    return 0;
}

/**
 * @brief       Get Entry from L2 static entry
 * @param[in]   static_ent Static entry
 * @return      entry
 */
pfe_l2br_table_entry_t *pfe_l2br_static_entry_get_entry(const pfe_l2br_static_entry_t *static_ent)
{
	pfe_l2br_table_entry_t *new_entry;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == static_ent))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		new_entry = NULL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		new_entry = static_ent->entry;
	}
	return new_entry;
}

/**
 * @brief		Get L2 bridge domain(vlan) statistics
 * @param[in]	class		The classifier instance
 * @param[in]	vlan_index 	Index in vlan statistics table
 * @param[out]	stat        Statistic structure
 * @retval		EOK         Success
 * @retval		ENOMEM       Not possible to allocate memory for read
 */
errno_t pfe_l2br_get_domain_stats(const pfe_l2br_t *bridge, pfe_ct_vlan_stats_t *stat, uint8_t vlan_index)
{
	uint32_t             i = 0U;
	errno_t              ret = EOK;
	pfe_ct_vlan_stats_t *stats = NULL;
	uint16_t             offset = 0;
	size_t               stats_size;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == bridge) || (NULL == stat)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		(void)memset(stat, 0, sizeof(pfe_ct_vlan_stats_t));

		stats_size = sizeof(pfe_ct_vlan_stats_t) * pfe_class_get_num_of_pes(bridge->class);
		stats = oal_mm_malloc(stats_size);

		if (NULL == stats)
		{
			NXP_LOG_ERROR("Memory allocation failed\n");
			ret = ENOMEM;
		}
		else
		{
			if (0U < stats_size)
			{
				(void)memset(stats, 0, stats_size);
			}

			offset = (uint16_t)sizeof(pfe_ct_vlan_stats_t) * (uint16_t)vlan_index;

			while (i < pfe_class_get_num_of_pes(bridge->class))
			{
				/* Gather memory from all PEs*/
				ret = pfe_class_read_dmem((void *)bridge->class, (int32_t)i, &stats[i], bridge->domain_stats_table_addr + offset, sizeof(pfe_ct_vlan_stats_t));

				/* Calculate total statistics */
				stat->ingress += oal_ntohl(stats[i].ingress);
				stat->egress += oal_ntohl(stats[i].egress);
				stat->ingress_bytes += oal_ntohl(stats[i].ingress_bytes);
				stat->egress_bytes += oal_ntohl(stats[i].egress_bytes);
				++i;
			}

			oal_mm_free(stats);

			/* Convert statistics back to network endian */
			stat->ingress		= oal_htonl(stat->ingress);
			stat->egress		= oal_htonl(stat->egress);
			stat->ingress_bytes	= oal_htonl(stat->ingress_bytes);
			stat->egress_bytes	= oal_htonl(stat->egress_bytes);
		}
	}
	return ret;
}

/**
 * @brief		Clear vlan statistics
 * @param[in]	class		The classifier instance
 * @param[in]	vlan_index	Index in vlan statistics table
 * @retval		EOK Success
 * @retval		NOMEM Not possible to allocate memory for read
 */
errno_t pfe_l2br_clear_domain_stats(const pfe_l2br_t *bridge, uint8_t vlan_index)
{
	errno_t             ret;
	pfe_ct_vlan_stats_t stat = { 0 };
	uint16_t            offset = 0;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == bridge))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		offset = (uint16_t)sizeof(pfe_ct_vlan_stats_t) * (uint16_t)vlan_index;

		if (EOK != oal_mutex_lock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}

		ret = pfe_class_write_dmem((void *)bridge->class, -1, bridge->domain_stats_table_addr + offset, &stat, sizeof(pfe_ct_vlan_stats_t));

		if (EOK != oal_mutex_unlock(bridge->mutex))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
	}
	return ret;
}

/**
 * @brief		Return L2 Bridge domain(vlan) statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	bridge		The L2 Bridge instance
 * @param[in]	seq			Pointer to debugfs seq_file
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 */
uint32_t pfe_l2br_domain_get_text_statistics(pfe_l2br_t *bridge, struct seq_file *seq, uint8_t verb_level)
{
	errno_t ret;
	pfe_ct_vlan_stats_t stats = {0};
	pfe_l2br_domain_t *domain = NULL;

	/* We keep unused parameter verb_level for consistency with rest of the *_get_text_statistics() functions */
	(void)verb_level;

	domain = pfe_l2br_get_first_domain(bridge, L2BD_CRIT_ALL, NULL);

	while (domain != NULL)
	{
		ret = pfe_l2br_get_domain_stats (bridge, &stats, domain->stats_index);
		if(EOK != ret)
		{
			NXP_LOG_ERROR("Get domain statistics failed\n");
			break;
		}
		seq_printf(seq, "Vlan [%4d] ingress: %12d       egress: %12d\n", domain->vlan, oal_ntohl(stats.ingress), oal_ntohl(stats.egress));
		seq_printf(seq, "      ingress_bytes: %12d egress_bytes: %12d\n", oal_ntohl(stats.ingress_bytes), oal_ntohl(stats.egress_bytes));
		domain = pfe_l2br_get_next_domain(bridge);
	}

	return 0;
}

/**
 * @brief		Get L2 bridge domain(vlan) statistics
 * @param[in]	domain		The classifier instance
 * @return		Index in vlan statistics table
 */

uint8_t pfe_l2br_get_vlan_stats_index(const pfe_l2br_domain_t *domain)
{
	uint8_t stats_idx;
#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == domain))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		stats_idx = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		stats_idx = domain->stats_index;
	}
	return stats_idx;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

