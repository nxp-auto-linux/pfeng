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
 * @addtogroup  dxgr_PFE_L2BR_TABLE
 * @{
 *
 * @file		pfe_l2br_table.c
 * @brief		The L2 bridge table module source file.
 * @details		This file contains L2 bridge table-related functionality.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "oal.h"
#include "hal.h"

#include "pfe_platform_cfg.h" /* due to pfe_interface_t */
#include "pfe_cbus.h"
#include "pfe_l2br_table.h"
#include "pfe_l2br_table_csr.h"

/*	MAC address type must be 48-bits long */
oal_ct_assert(sizeof(pfe_mac_addr_t) * 8 == 48);

/**
 * @brief HASH registers associated with a table
 */
typedef struct __pfe_mac_table_regs_tag
{
	void *cmd_reg;				/* REQ1_CMD_REG */
	void *mac1_addr_reg;		/* REQ1_MAC1_ADDR_REG */
	void *mac2_addr_reg;		/* REQ1_MAC2_ADDR_REG */
	void *mac3_addr_reg;		/* REQ1_MAC3_ADDR_REG */
	void *mac4_addr_reg;		/* REQ1_MAC4_ADDR_REG */
	void *mac5_addr_reg;		/* REQ1_MAC5_ADDR_REG */
	void *entry_reg;			/* REQ1_ENTRY_REG */
	void *status_reg;			/* REQ1_STATUS_REG */
	void *direct_reg;			/* REQ1_DIRECT_REG */
	void *free_entries_reg;		/* FREE LIST ENTRIES */
	void *free_head_ptr_reg;	/* FREE LIST HEAD PTR */
	void *free_tail_ptr_reg;	/* FREE LIST TAIL PTR */
} pfe_mac_table_regs_t;

/**
 * @brief	The L2 Bridge table instance structure
 */
struct __pfe_l2br_table_tag
{
	void *cbus_base_va;							/*!< CBUS base virtual address					*/
	pfe_l2br_table_type_t type;					/*!< Table type									*/
	pfe_mac_table_regs_t regs;					/*!< Registers (VA)								*/
	uint16_t hash_space_depth;					/*!< Hash space depth in number of entries		*/
	uint16_t coll_space_depth;					/*!< Collision space depth in number of entries */
	pfe_l2br_table_get_criterion_t cur_crit;	/*!< Current criterion							*/
	uint32_t cur_hash_addr;						/*!< Current address within hash space			*/
	uint32_t cur_coll_addr;						/*!< Current address within collision space		*/
};

/**
 * @brief	2-field MAC table entry
 */
typedef struct __attribute__((packed)) __pfe_mac2f_table_entry_tag
{
	pfe_mac_addr_t mac;										/*!< [47:0]												*/
	uint32_t vlan 								: 13;		/*!< [60:48]											*/
	uint32_t action_data						: 31;		/*!< [91:61], see pfe_ct_mac_table_result_t		*/
	uint32_t field_valids						: 8;		/*!< [99:92], see pfe_mac2f_table_entry_valid_bits_t	*/
	uint32_t port								: 4;		/*!< [103:100]											*/
	uint32_t col_ptr							: 16;		/*!< [119:104]											*/
	uint32_t flags								: 4;		/*!< [123:120], see pfe_mac2f_table_entry_flags_t		*/
} pfe_mac2f_table_entry_t;

/**
 * @brief	Flags for 2-field MAC table entry (pfe_mac2f_table_entry_t.flags)
 */
typedef enum
{
	MAC2F_ENTRY_VALID_FLAG = (1U << 3),        	/*!< MAC2F_ENTRY_VALID_FLAG			*/
	MAC2F_ENTRY_COL_PTR_VALID_FLAG = (1U << 2),	/*!< MAC2F_ENTRY_COL_PTR_VALID_FLAG	*/
	MAC2F_ENTRY_RESERVED1_FLAG = (1U << 1),    	/*!< MAC2F_ENTRY_RESERVED1_FLAG		*/
	MAC2F_ENTRY_RESERVED2_FLAG = (1U << 0)     	/*!< MAC2F_ENTRY_RESERVED2_FLAG		*/
} pfe_mac2f_table_entry_flags_t;

/**
 * @brief	Valid flags for 2-field MAC table entry (pfe_mac2f_table_entry_t.field_valids)
 */
typedef enum
{
	MAC2F_ENTRY_MAC_VALID = (1U << 0),   		/*!< (Field1 = MAC Valid)	*/
	MAC2F_ENTRY_VLAN_VALID = (1U << 1),   		/*!< (Field2 = VLAN Valid)	*/
	MAC2F_ENTRY_RESERVED1_VALID = (1U << 2),   	/*!< RESERVED				*/
	MAC2F_ENTRY_RESERVED2_VALID = (1U << 3),   	/*!< RESERVED				*/
	MAC2F_ENTRY_RESERVED3_VALID = (1U << 4),   	/*!< RESERVED				*/
	MAC2F_ENTRY_RESERVED4_VALID = (1U << 5),	/*!< RESERVED				*/
	MAC2F_ENTRY_RESERVED5_VALID = (1U << 6),	/*!< RESERVED				*/
	MAC2F_ENTRY_RESERVED6_VALID = (1U << 7),	/*!< RESERVED				*/
} pfe_mac2f_table_entry_valid_bits_t;

/**
 * @brief	VLAN table entry
 */
typedef struct __attribute__((packed)) __pfe_vlan_table_entry_tag
{
	uint32_t vlan								: 13;	/*!< [12:0]											*/
	uint64_t action_data						: 55;	/*!< [67:13], see pfe_vlan_table_action_entry_t		*/
	uint32_t field_valids						: 8;	/*!< [75:68], see pfe_vlan_table_entry_valid_bits_t	*/
	uint32_t port								: 4;	/*!< [79:76]										*/
	uint32_t col_ptr							: 16;	/*!< [95:80]										*/
	uint32_t flags								: 4;	/*!< [99:96], see pfe_vlan_table_entry_flags_t		*/
} pfe_vlan_table_entry_t;

/**
 * @brief	Flags for VLAN table entry (pfe_vlan_table_entry_t.flags)
 */
typedef enum
{
	VLAN_ENTRY_VALID_FLAG = (1U << 3),        	/*!< VLAN_ENTRY_VALID_FLAG			*/
	VLAN_ENTRY_COL_PTR_VALID_FLAG = (1U << 2),	/*!< VLAN_ENTRY_COL_PTR_VALID_FLAG	*/
	VLAN_ENTRY_RESERVED1_FLAG = (1U << 1),    	/*!< VLAN_ENTRY_RESERVED1_FLAG		*/
	VLAN_ENTRY_RESERVED2_FLAG = (1U << 0)     	/*!< VLAN_ENTRY_RESERVED2_FLAG		*/
} pfe_vlan_table_entry_flags_t;

/**
 * @brief	Valid flags for VLAN table entry (pfe_vlan_table_entry_t.field_valids)
 */
typedef enum
{
	VLAN_ENTRY_VLAN_VALID = (1U << 0),   	/*!< (Field1 = VLAN Valid)		*/
	VLAN_ENTRY_RESERVED1_VALID = (1U << 1), /*!< RESERVED					*/
	VLAN_ENTRY_RESERVED2_VALID = (1U << 2), /*!< RESERVED					*/
	VLAN_ENTRY_RESERVED3_VALID = (1U << 3), /*!< RESERVED					*/
	VLAN_ENTRY_RESERVED4_VALID = (1U << 4), /*!< RESERVED					*/
	VLAN_ENTRY_RESERVED5_VALID = (1U << 5),	/*!< RESERVED					*/
	VLAN_ENTRY_RESERVED6_VALID = (1U << 6),	/*!< RESERVED					*/
	VLAN_ENTRY_RESERVED7_VALID = (1U << 7),	/*!< RESERVED					*/
} pfe_vlan_table_entry_valid_bits_t;

struct __pfe_l2br_table_entry_tag
{
	union
	{
		pfe_mac2f_table_entry_t mac2f_entry;
		pfe_vlan_table_entry_t vlan_entry;
	};

	pfe_l2br_table_type_t type;
	bool_t action_data_set;
	bool_t mac_addr_set;
	bool_t vlan_set;
};

static errno_t pfe_l2br_table_init_cmd(pfe_l2br_table_t *l2br);
static errno_t pfe_l2br_table_write_cmd(pfe_l2br_table_t *l2br, uint16_t addr, pfe_l2br_table_entry_t *entry);
static errno_t pfe_l2br_table_read_cmd(pfe_l2br_table_t *l2br, uint16_t addr, pfe_l2br_table_entry_t *entry);
#if 0 /*	For future use */
static uint32_t pfe_l2br_table_entry_get_hash(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
#endif /* 0 */
static errno_t pfe_l2br_wait_for_cmd_done(pfe_l2br_table_t *l2br, uint32_t *status_val);
static errno_t pfe_l2br_entry_to_cmd_args(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);
static uint32_t pfe_l2br_table_get_col_ptr(pfe_l2br_table_entry_t *entry);
static bool_t pfe_l2br_table_entry_match_criterion(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry);

/**
 * @brief		Match entry with latest criterion provided via pfe_l2br_table_get_first()
 * @param[in]	l2br The L2 Bridge Table instance
 * @param[in]	entry The entry to be matched
 * @retval		True Entry matches the criterion
 * @retval		False Entry does not match the criterion
 */
static bool_t pfe_l2br_table_entry_match_criterion(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
{
	bool_t match = FALSE;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	switch (l2br->cur_crit)
	{
		case L2BR_TABLE_CRIT_ALL:
		{
			match = TRUE;
			break;
		}

		case L2BR_TABLE_CRIT_VALID:
		{
			switch (l2br->type)
			{
				case PFE_L2BR_TABLE_MAC2F:
				{
					match = (0U != (entry->mac2f_entry.flags & MAC2F_ENTRY_VALID_FLAG));
					break;
				}

				case PFE_L2BR_TABLE_VLAN:
				{
					match = (0U != (entry->vlan_entry.flags & VLAN_ENTRY_VALID_FLAG));
					break;
				}

				default:
				{
					NXP_LOG_ERROR("Invalid table type\n");
					break;
				}
			}

			break;
		}

		default:
		{
			NXP_LOG_ERROR("Unknown criterion\n");
			break;
		}
	}

	return match;
}

/**
 * @brief		Get collision pointer
 * @param[in]	entry The table entry instance
 * @return		Collision pointer or 0 if not found
 */
static uint32_t pfe_l2br_table_get_col_ptr(pfe_l2br_table_entry_t *entry)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	switch (entry->type)
	{
		case PFE_L2BR_TABLE_MAC2F:
		{
			if (entry->mac2f_entry.flags & MAC2F_ENTRY_COL_PTR_VALID_FLAG)
			{
				return entry->mac2f_entry.col_ptr;
			}

			break;
		}

		case PFE_L2BR_TABLE_VLAN:
		{
			if (entry->vlan_entry.flags & VLAN_ENTRY_COL_PTR_VALID_FLAG)
			{
				return entry->vlan_entry.col_ptr;
			}

			break;
		}

		default:
		{
			NXP_LOG_ERROR("Invalid table type\n");
			break;
		}
	}

	return 0U;
}

#if 0 /*	For future use */
/**
 * @brief		Get hash
 * @param[in]	l2br The L2 Bridge table instance
 * @param[in]	entry Entry
 * @return		Hash value computed from input entry as defined by PFE HW.
 */
static uint32_t pfe_l2br_table_entry_get_hash(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
{
	uint16_t *entry16 = (uint16_t *)entry;
	uint32_t hash = 0U;
	bool_t match = FALSE;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		if (0U != (entry->mac2f_entry.field_valids & MAC2F_ENTRY_MAC_VALID))
		{
			hash ^= entry16[0] ^ entry16[1] ^ entry16[2]; /* MAC address */
			match = TRUE;
		}

		if (0U != (entry->mac2f_entry.field_valids & MAC2F_ENTRY_VLAN_VALID))
		{
			hash ^= (entry16[3] & 0x1ffffU); /* VLAN is 13-bits here */
			match = TRUE;
		}
	}
	else if (PFE_L2BR_TABLE_VLAN == l2br->type)
	{
		if (0U != (entry->vlan_entry.field_valids & VLAN_ENTRY_VLAN_VALID))
		{
			hash ^= (entry16[0] & 0x1ffffU); /* VLAN is 13-bits here */
			match = TRUE;
		}
	}
	else
	{
		NXP_LOG_DEBUG("Invalid table type\n");
		return 0U;
	}

	if (FALSE == match)
	{
		NXP_LOG_DEBUG("No valid entry to compute hash. Result is invalid.\n");
		return 0U;
	}

	return hash & (l2br->hash_space_depth - 1U);
}
#endif /* 0 */

/**
 * @brief		Convert entry to command arguments
 * @details		Function will write necessary data to registers as preparation
 * 				of subsequent command (ADD/DEL/UPDATE/SEARCH).
 * @param[in]	entry The entry
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 */
static errno_t pfe_l2br_entry_to_cmd_args(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
{
	uint32_t *entry32 = (uint32_t *)entry;
	uint64_t action_data = 0ULL;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Prepare command arguments */
	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		/*	Write MAC (in network byte order) and VLAN */
		hal_write32(oal_htonl(entry32[0]), l2br->regs.mac1_addr_reg);
		hal_write32(oal_htons(entry32[1] & 0x0000ffffU) | (entry32[1] & 0xffff0000U), l2br->regs.mac2_addr_reg);

		/*	Write action entry. There is 31-bits of action data */
		hal_write32(entry->mac2f_entry.action_data & 0x7fffffffU, l2br->regs.entry_reg);
	}
	else if (PFE_L2BR_TABLE_VLAN == l2br->type)
	{
		/*	Write VLAN */
		hal_write32(entry->vlan_entry.vlan, l2br->regs.mac1_addr_reg);

		/*
		 	Write action entry. There is 55-bits of action data.

		 	According to the TRM v2.16 chapter 4.5.5.14 the action data MSB (upper 23-bits) shall be
			put into the DIRECT_REG while LSB (lower 32-bits) shall be put into the ENTRY_REG.
		*/
		action_data = entry->vlan_entry.action_data & 0xffffffffU;
		hal_write32(action_data, l2br->regs.entry_reg);
		action_data = (entry->vlan_entry.action_data >> 32) & 0x7fffffU;
		hal_write32(action_data, l2br->regs.direct_reg);
	}
	else
	{
		NXP_LOG_ERROR("Invalid table type\n");
		return EINVAL;
	}

	return EOK;
}

/**
 * @brief		Update table entry
 * @details		Associates new action data with the entry.
 * @param[in]	l2br The L2 Bridge Table instance
 * @param[in]	entry Entry to be updated
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 * @retval		ENOENT Entry not found
 * @retval		ENOEXEC Command failed
 * @retval		ETIMEDOUT Command timed-out
 */
errno_t pfe_l2br_table_update_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
{
	uint32_t status, cmd;
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Prepare command arguments */
	ret = pfe_l2br_entry_to_cmd_args(l2br, entry);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Entry-to-args conversion failed: %d\n", ret);
		return ret;
	}

	/*	Argument registers are prepared. Compile the UPDATE command. */
	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		if ((FALSE == entry->mac_addr_set) && (FALSE == entry->vlan_set))
		{
			NXP_LOG_DEBUG("MAC or VLAN must be set\n");
			return EINVAL;
		}

		cmd = L2BR_CMD_UPDATE | ((entry->mac2f_entry.field_valids & 0x1fU) << 8) | (0 << 13);
	}
	else if (PFE_L2BR_TABLE_VLAN == l2br->type)
	{
		if (FALSE == entry->vlan_set)
		{
			NXP_LOG_DEBUG("VLAN must be set\n");
			return EINVAL;
		}

		cmd = L2BR_CMD_UPDATE | ((entry->vlan_entry.field_valids & 0x1fU) << 8) | (0 << 13);
	}
	else
	{
		NXP_LOG_ERROR("Invalid table type\n");
		return EINVAL;
	}

	/*	Issue the UPDATE command */
	hal_write32(cmd, l2br->regs.cmd_reg);

	ret = pfe_l2br_wait_for_cmd_done(l2br, &status);
	if (EOK != ret)
	{
		return ret;
	}

	if (0U != (status & STATUS_REG_SIG_ENTRY_NOT_FOUND))
	{
		NXP_LOG_DEBUG("Attempting to update non-existing entry\n");
		return ENOENT;
	}

	if (0U == (status & STATUS_REG_SIG_ENTRY_ADDED))
	{
		NXP_LOG_ERROR("Table entry UPDATE CMD failed\n");
		return ENOEXEC;
	}

	return EOK;
}

/**
 * @brief		Delete entry from table
 * @details		Entry is removed from table if exists. If does not exist, the call is
 *				returns success (EOK).
 * @param[in]	l2br The L2 Bridge Table instance
 * @param[in]	data Entry to be deleted
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 * @retval		ETIMEDOUT Command timed-out
 */
errno_t pfe_l2br_table_del_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
{
	uint32_t status, cmd;
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Prepare command arguments */
	ret = pfe_l2br_entry_to_cmd_args(l2br, entry);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Entry-to-args conversion failed: %d\n", ret);
		return ret;
	}

	/*	Argument registers are prepared. Compile the DEL command. */
	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		if ((FALSE == entry->mac_addr_set) && (FALSE == entry->vlan_set))
		{
			NXP_LOG_DEBUG("MAC or VLAN must be set\n");
			return EINVAL;
		}

		cmd = L2BR_CMD_DELETE | ((entry->mac2f_entry.field_valids & 0x1fU) << 8) | (0 << 13);
	}
	else if (PFE_L2BR_TABLE_VLAN == l2br->type)
	{
		if (FALSE == entry->vlan_set)
		{
			NXP_LOG_DEBUG("VLAN must be set\n");
			return EINVAL;
		}

		cmd = L2BR_CMD_DELETE | ((entry->vlan_entry.field_valids & 0x1fU) << 8) | (0 << 13);
	}
	else
	{
		NXP_LOG_ERROR("Invalid table type\n");
		return EINVAL;
	}

	/*	Issue the DEL command */
	hal_write32(cmd, l2br->regs.cmd_reg);

	ret = pfe_l2br_wait_for_cmd_done(l2br, &status);
	if (EOK != ret)
	{
		return ret;
	}

	if (0U != (status & STATUS_REG_SIG_ENTRY_NOT_FOUND))
	{
		NXP_LOG_DEBUG("Attempting to delete non-existing entry\n");
	}

	return EOK;
}

/**
 * @brief		Add entry to table
 * @param[in]	l2br The L2 Bridge Table instance
 * @param[in]	data Entry to be added
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 * @retval		ENOEXEC Command failed
 * @retval		ETIMEDOUT Command timed-out
 */
errno_t pfe_l2br_table_add_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
{
	uint32_t status, cmd;
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Prepare command arguments */
	ret = pfe_l2br_entry_to_cmd_args(l2br, entry);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Entry-to-args conversion failed: %d\n", ret);
		return ret;
	}

	/*	Argument registers are prepared. Compile the ADD command. */
	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		if (((FALSE == entry->mac_addr_set) && (FALSE == entry->vlan_set))
				|| (FALSE == entry->action_data_set))
		{
			NXP_LOG_DEBUG("MAC/VLAN and action must be set\n");
			return EINVAL;
		}

		cmd = L2BR_CMD_ADD | ((entry->mac2f_entry.field_valids & 0x1fU) << 8) | (0 << 13) | (entry->mac2f_entry.port << 16);
	}
	else if (PFE_L2BR_TABLE_VLAN == l2br->type)
	{
		if ((FALSE == entry->vlan_set) || (FALSE == entry->action_data_set))
		{
			NXP_LOG_DEBUG("VLAN and action must be set\n");
			return EINVAL;
		}

		cmd = L2BR_CMD_ADD | ((entry->vlan_entry.field_valids & 0x1fU) << 8) | (0 << 13) | (entry->vlan_entry.port << 16);
	}
	else
	{
		NXP_LOG_ERROR("Invalid table type\n");
		return EINVAL;
	}

	/*	Issue the ADD command */
	hal_write32(cmd, l2br->regs.cmd_reg);

	ret = pfe_l2br_wait_for_cmd_done(l2br, &status);
	if (EOK != ret)
	{
		return ret;
	}

	if (0U == (status & STATUS_REG_SIG_ENTRY_ADDED))
	{
		NXP_LOG_ERROR("Table entry ADD CMD failed\n");
		return ENOEXEC;
	}

	return EOK;
}

/**
 * @brief			Search entry in table
 * @param[in]		l2br The L2 Bridge Table instance
 * @param[in,out]	data Reference entry to be used for lookup. This entry will be updated by
 * 						 values read from the table.
 * @retval			EOK Success
 * @retval			EINVAL Invalid/missing argument
 * @retval			ENOENT Entry not found
 * @retval			ETIMEDOUT Command timed-out
 */
errno_t pfe_l2br_table_search_entry(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
{
	uint32_t status, cmd;
	errno_t ret;
	uint64_t action_data = 0ULL;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Prepare command arguments */
	ret = pfe_l2br_entry_to_cmd_args(l2br, entry);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Entry-to-args conversion failed: %d\n", ret);
		return ret;
	}

	/*	Argument registers are prepared. Compile the SEARCH command. */
	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		if ((FALSE == entry->mac_addr_set) && (FALSE == entry->vlan_set))
		{
			NXP_LOG_DEBUG("MAC or VLAN must be set\n");
			return EINVAL;
		}

		cmd = L2BR_CMD_SEARCH | ((entry->mac2f_entry.field_valids & 0x1fU) << 8) | (0 << 13) | (entry->mac2f_entry.port << 16);
	}
	else if (PFE_L2BR_TABLE_VLAN == l2br->type)
	{
		if (FALSE == entry->vlan_set)
		{
			NXP_LOG_DEBUG("VLAN must be set\n");
			return EINVAL;
		}

		cmd = L2BR_CMD_SEARCH | ((entry->vlan_entry.field_valids & 0x1fU) << 8) | (0 << 13) | (entry->vlan_entry.port << 16);
	}
	else
	{
		NXP_LOG_ERROR("Invalid table type\n");
		return EINVAL;
	}

	/*	Issue the SEARCH command */
	hal_write32(cmd, l2br->regs.cmd_reg);

	ret = pfe_l2br_wait_for_cmd_done(l2br, &status);
	if (EOK != ret)
	{
		return ret;
	}

	if (0U != (status & STATUS_REG_SIG_ENTRY_NOT_FOUND))
	{
		NXP_LOG_DEBUG("L2BR table entry not found\n");
		return ENOENT;
	}

	if (0U == (status & STATUS_REG_MATCH))
	{
		NXP_LOG_DEBUG("L2BR table entry mismatch\n");
		return ENOENT;
	}

	/*	Get action data */
	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		action_data = hal_read32(l2br->regs.entry_reg) & 0x7fffffffU;
		entry->mac2f_entry.action_data = action_data;
	}
	else if (PFE_L2BR_TABLE_VLAN == l2br->type)
	{
		action_data = hal_read32(l2br->regs.entry_reg);
		action_data |= ((uint64_t)hal_read32(l2br->regs.direct_reg) << 32);
		entry->vlan_entry.action_data = (action_data & 0x7fffffffffffffULL);
		/*
		 	TODO: Issue: VLAN table action data can't be read by host.

		 	Seems that the SEARCH command does not update the DIRECT_REG value. This is OK for
		 	2-field table where action data is 31-bits long but it is an issue in case of VLAN
			table where action data is 55-bits long. Need to confirm with vendor if this behavior
		 	is only a bug of FPGA bitfile (v5.0.4) or it is a potential bug in the PFE IP.

		 	Usage of the VLAN table commands is described by chapter 4.5.5.14 of the TRM v2.16.
		 	According to the manual the SEARCH command shall return the 'Entry MSB' via the
		 	DIRECT_REG and 'Entry LSB' via ENTRY_REG. ENTRY_REG gives correct value but
		 	DIRECT_REG remains zero.

		 	Note that when entry is fetched via direct memory access (MEM_READ command) the
			action data is present and correctly read from the table.
		*/
	}
	else
	{
		;
	}

	return EOK;
}

/**
 * @brief			Get first entry from table
 * @param[in]		l2br The L2 Bridge Table instance
 * @param[in]		crit Get criterion
 * @param[out]		entry Entry will be written at this location
 * @retval			EOK Success
 * @retval			EINVAL Invalid/missing argument
 * @retval			ENOENT Entry not found
 * @retval			ETIMEDOUT Command timed-out
 */
errno_t pfe_l2br_table_get_first(pfe_l2br_table_t *l2br, pfe_l2br_table_get_criterion_t crit, pfe_l2br_table_entry_t *entry)
{
	errno_t ret = ENOENT;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Remember criterion and argument for possible subsequent pfe_l2br_table_get_next() calls */
	l2br->cur_crit = crit;

	/*	Get entries from address 0x0 */
	for (l2br->cur_hash_addr=0U, l2br->cur_coll_addr=0U; l2br->cur_hash_addr<l2br->hash_space_depth; l2br->cur_hash_addr++)
	{
		ret = pfe_l2br_table_read_cmd(l2br, l2br->cur_hash_addr, entry);
		if (EOK != ret)
		{
			NXP_LOG_DEBUG("Can not read table entry from location %d\n", l2br->cur_hash_addr);
			break;
		}
		else
		{
			if (TRUE == pfe_l2br_table_entry_match_criterion(l2br, entry))
			{
				/*	Remember entry to be processed next. If collision pointer is zero then
					next hash table entry will be processed. */
				l2br->cur_coll_addr = pfe_l2br_table_get_col_ptr(entry);
				return EOK;
			}
		}
	}

	return ENOENT;
}

/**
 * @brief			Get next entry from table
 * @param[in]		l2br The L2 Bridge Table instance
 * @param[in]		addr Address within the table to read entry from
 * @param[out]		entry Entry will be written at this location
 * @retval			EOK Success
 * @retval			EINVAL Invalid/missing argument
 * @retval			ENOENT Entry not found
 * @retval			ETIMEDOUT Command timed-out
 */
errno_t pfe_l2br_table_get_next(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry)
{
	uint32_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Get entries from last address */
	while (l2br->cur_hash_addr < l2br->hash_space_depth)
	{
		if (0U == l2br->cur_coll_addr)
		{
			/*	Collision pointer zero is invalid here since it points to very first entry
				within hash table, i.e. valid collision pointer can't be zero. Get next entry
				from hash space. */
			l2br->cur_hash_addr++;
			ret = pfe_l2br_table_read_cmd(l2br, l2br->cur_hash_addr, entry);
		}
		else
		{
			/*	Collision pointer is valid, get entry from current collision space address */
			ret = pfe_l2br_table_read_cmd(l2br, l2br->cur_coll_addr, entry);
		}

		if (EOK != ret)
		{
			NXP_LOG_DEBUG("Can not read table entry\n");
			break;
		}
		else
		{
			if (TRUE == pfe_l2br_table_entry_match_criterion(l2br, entry))
			{
				/*	Remember entry to be processed next. If collision pointer is zero then
				 	next hash table entry will be processed. */
				l2br->cur_coll_addr = pfe_l2br_table_get_col_ptr(entry);
				return EOK;
			}
		}
	}

	return EINVAL;
}

/**
 * @brief		Wait for command completion
 * @details		Function will wait until previously issued command has completed.
 * @param[in]	l2br The L2 Bridge Table instance
 * @param[out]	status_val If not NULL, the function will write content of status register there.
 * @retval		EOK Success
 * @retval		ETIMEDOUT Timed out
 */
static errno_t pfe_l2br_wait_for_cmd_done(pfe_l2br_table_t *l2br, uint32_t *status_val)
{
	uint32_t ii = 100U;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == l2br))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Wait for command completion */
	while (0U == (hal_read32(l2br->regs.status_reg) & STATUS_REG_CMD_DONE))
	{
		ii--;
		oal_time_usleep(10);

		if (0U == ii)
		{
			break;
		}
	}

	if (NULL != status_val)
	{
		*status_val = hal_read32(l2br->regs.status_reg);
	}

	/*	Clear the STATUS register */
	hal_write32(0xffffffffU, l2br->regs.status_reg);

	if (0U == ii)
	{
		return ETIMEDOUT;
	}
	else
	{
		return EOK;
	}
}

/**
 * @brief		Direct MEM WRITE command
 * @param[in]	l2br The L2 Bridge table instance
 * @param[in]	addr Address within the table (index of entry to be written)
 * @param[in]	wdata Entry data. Shall match the table type. Shall be pfe_mac2f_table_entry_t
 *					  or pfe_vlan_table_entry_t.
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 * @retval		ETIMEDOUT Command timed-out
 */
static errno_t pfe_l2br_table_write_cmd(pfe_l2br_table_t *l2br, uint16_t addr, pfe_l2br_table_entry_t *entry)
{
	uint32_t *wdata = (uint32_t *)entry;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Issue the WRITE command */
	if (addr >= (l2br->hash_space_depth + l2br->coll_space_depth))
	{
		NXP_LOG_ERROR("Hash table address 0x%x is out of range\n", addr);
		return EINVAL;
	}

	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		wdata = (uint32_t *)&entry->mac2f_entry;
	}
	else if (PFE_L2BR_TABLE_VLAN == l2br->type)
	{
		wdata = (uint32_t *)&entry->vlan_entry;
	}
	else
	{
		NXP_LOG_ERROR("Invalid table type\n");
	}

	hal_write32(wdata[0], l2br->regs.mac1_addr_reg);	/* wdata[31:0]    */
	hal_write32(wdata[1], l2br->regs.mac2_addr_reg);	/* wdata[63:32]   */
	hal_write32(wdata[2], l2br->regs.mac3_addr_reg);	/* wdata[95:64]   */
	hal_write32(wdata[3], l2br->regs.mac4_addr_reg);	/* wdata[127:96]  */

	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		/*	The 2-field MAC table is longer */
		hal_write32(wdata[4], l2br->regs.mac5_addr_reg);	/* wdata[159:128]  */
		hal_write32(wdata[5], l2br->regs.entry_reg);		/* wdata[191:160]  */
	}

	hal_write32(L2BR_CMD_MEM_WRITE | (addr << 16), l2br->regs.cmd_reg);

	return pfe_l2br_wait_for_cmd_done(l2br, NULL);
}

/**
 * @brief		Direct MEM READ command
 * @param[in]	l2br The L2 Bridge table instance
 * @param[in]	addr Address within the table (index of entry to be read)
 * @param[out]	data Pointer to memory where entry will be written. See pfe_mac_2f_table_entry_t
 * 					 or pfe_vlan_table_entry_t.
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 * @retval		ETIMEDOUT Command timed-out
 */
static errno_t pfe_l2br_table_read_cmd(pfe_l2br_table_t *l2br, uint16_t addr, pfe_l2br_table_entry_t *entry)
{
	errno_t ret;
	uint32_t *rdata = (uint32_t *)entry;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Issue the READ command */
	if (addr >= (l2br->hash_space_depth + l2br->coll_space_depth))
	{
		NXP_LOG_ERROR("Hash table address 0x%x is out of range\n", addr);
		return EINVAL;
	}

	hal_write32(L2BR_CMD_MEM_READ | (addr << 16), l2br->regs.cmd_reg);

	ret = pfe_l2br_wait_for_cmd_done(l2br, NULL);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Table read failed: %d\n", ret);
		return ret;
	}

	rdata[0] = hal_read32(l2br->regs.mac1_addr_reg);
	rdata[1] = hal_read32(l2br->regs.mac2_addr_reg);
	rdata[2] = hal_read32(l2br->regs.mac3_addr_reg);
	rdata[3] = hal_read32(l2br->regs.mac4_addr_reg);

	if (PFE_L2BR_TABLE_MAC2F == l2br->type)
	{
		uint32_t data32 = oal_htonl(rdata[0]);
		uint16_t data16 = oal_htons(rdata[1] & 0xffffU);

		rdata[4] = hal_read32(l2br->regs.mac5_addr_reg);
		rdata[5] = hal_read32(l2br->regs.entry_reg);

		/*	Seems that the direct access registers do endian conversion from PFE(BE) to host(LE). The
			MAC address bytes are therefore out of order. Need correction here. */
		memcpy(&entry->mac2f_entry.mac[0], &data32, sizeof(uint32_t));
		memcpy(&entry->mac2f_entry.mac[4], &data16, sizeof(uint16_t));

		entry->mac_addr_set = TRUE;
	}

	entry->type = l2br->type;
	entry->vlan_set = TRUE;
	entry->action_data_set = TRUE;

	return EOK;
}

/**
 * @brief		Issue the INIT command
 * @details		Function will perform complete HW table initialization as described
 * 				by PFE reference manual.
 * @param[in]	l2br The L2 bridge table instance
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 * @retval		ENOEXEC Command failed
 * @retval		ETIMEDOUT Command timed-out
 */
static errno_t pfe_l2br_table_init_cmd(pfe_l2br_table_t *l2br)
{
	errno_t ret;
	uint32_t ii, status;
	union
	{
		pfe_mac2f_table_entry_t mac2f_entry;
		pfe_vlan_table_entry_t vlan_entry;
	} entry = {0U};

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == l2br))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	/*	Issue the INIT command */
	hal_write32(L2BR_CMD_INIT, l2br->regs.cmd_reg);
	ret = pfe_l2br_wait_for_cmd_done(l2br, &status);
	if (EOK != ret)
	{
		return ret;
	}

	if (0U == (status & STATUS_REG_SIG_INIT_DONE))
	{
		NXP_LOG_ERROR("Table INIT CMD failed\n");
		return ENOEXEC;
	}

	/*	Initialize MAC ADDR registers */
	hal_write32(0U, l2br->regs.mac1_addr_reg);
	hal_write32(0U, l2br->regs.mac2_addr_reg);
	hal_write32(0U, l2br->regs.mac3_addr_reg);
	hal_write32(0U, l2br->regs.mac4_addr_reg);
	hal_write32(0U, l2br->regs.mac5_addr_reg);

	/*	Chain the collision space */
	for (ii=0U; ii<l2br->coll_space_depth; ii++)
	{
		if (PFE_L2BR_TABLE_MAC2F == l2br->type)
		{
			entry.mac2f_entry.col_ptr = l2br->hash_space_depth + ii + 1U;
			entry.mac2f_entry.field_valids = MAC2F_ENTRY_COL_PTR_VALID_FLAG;
		}
		else if (PFE_L2BR_TABLE_VLAN == l2br->type)
		{
			entry.vlan_entry.col_ptr = l2br->hash_space_depth + ii + 1U;
			entry.vlan_entry.field_valids = VLAN_ENTRY_COL_PTR_VALID_FLAG;
		}
		else
		{
			NXP_LOG_ERROR("Invalid table type\n");
			return EINVAL;
		}

		/*	Collision space begins right after last hash space entry */
		ret = pfe_l2br_table_write_cmd(l2br, l2br->hash_space_depth + ii, (void *)&entry);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Collision space init failed: %d\n", ret);
			return ret;
		}
	}

	/*	Set free list head */
	hal_write32(l2br->hash_space_depth, l2br->regs.free_head_ptr_reg);

	/*	Set free list tail */
	hal_write32(l2br->hash_space_depth + l2br->coll_space_depth - 1U, l2br->regs.free_tail_ptr_reg);

	/*	Set number of entries in the free list */
	hal_write32(l2br->coll_space_depth, l2br->regs.free_entries_reg);

	return EOK;
}

/**
 * @brief		Create L2 bridge table instance
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	type Type of the table. See pfe_l2br_table_type_t.
 * @return		The L2 Bridge table instance or NULL if failed
 */
pfe_l2br_table_t *pfe_l2br_table_create(void *cbus_base_va, pfe_l2br_table_type_t type)
{
	pfe_l2br_table_t *l2br;
	errno_t ret;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	l2br = oal_mm_malloc(sizeof(pfe_l2br_table_t));

	if (NULL == l2br)
	{
		return NULL;
	}
	else
	{
		memset(l2br, 0, sizeof(pfe_l2br_table_t));
		l2br->cbus_base_va = cbus_base_va;
		l2br->type = type;
	}

	switch (type)
	{
		case PFE_L2BR_TABLE_MAC2F:
		{
			l2br->regs.cmd_reg = l2br->cbus_base_va + HOST_MAC2F_CMD_REG;
			l2br->regs.mac1_addr_reg = l2br->cbus_base_va + HOST_MAC2F_MAC1_ADDR_REG;
			l2br->regs.mac2_addr_reg = l2br->cbus_base_va + HOST_MAC2F_MAC2_ADDR_REG;
			l2br->regs.mac3_addr_reg = l2br->cbus_base_va + HOST_MAC2F_MAC3_ADDR_REG;
			l2br->regs.mac4_addr_reg = l2br->cbus_base_va + HOST_MAC2F_MAC4_ADDR_REG;
			l2br->regs.mac5_addr_reg = l2br->cbus_base_va + HOST_MAC2F_MAC5_ADDR_REG;
			l2br->regs.entry_reg = l2br->cbus_base_va + HOST_MAC2F_ENTRY_REG;
			l2br->regs.status_reg = l2br->cbus_base_va + HOST_MAC2F_STATUS_REG;
			l2br->regs.direct_reg = l2br->cbus_base_va + HOST_MAC2F_DIRECT_REG;
			l2br->regs.free_entries_reg = l2br->cbus_base_va + HOST_MAC2F_FREE_LIST_ENTRIES;
			l2br->regs.free_head_ptr_reg = l2br->cbus_base_va + HOST_MAC2F_FREE_LIST_HEAD_PTR;
			l2br->regs.free_tail_ptr_reg = l2br->cbus_base_va + HOST_MAC2F_FREE_LIST_TAIL_PTR;
			l2br->hash_space_depth = _MAC2F_TABLE_HASH_ENTRIES;
			l2br->coll_space_depth = _MAC2F_TABLE_COLL_ENTRIES;
			break;
		}

		case PFE_L2BR_TABLE_VLAN:
		{
			l2br->regs.cmd_reg = l2br->cbus_base_va + HOST_VLAN_CMD_REG;
			l2br->regs.mac1_addr_reg = l2br->cbus_base_va + HOST_VLAN_MAC1_ADDR_REG;
			l2br->regs.mac2_addr_reg = l2br->cbus_base_va + HOST_VLAN_MAC2_ADDR_REG;
			l2br->regs.mac3_addr_reg = l2br->cbus_base_va + HOST_VLAN_MAC3_ADDR_REG;
			l2br->regs.mac4_addr_reg = l2br->cbus_base_va + HOST_VLAN_MAC4_ADDR_REG;
			l2br->regs.mac5_addr_reg = l2br->cbus_base_va + HOST_VLAN_MAC5_ADDR_REG;
			l2br->regs.entry_reg = l2br->cbus_base_va + HOST_VLAN_ENTRY_REG;
			l2br->regs.status_reg = l2br->cbus_base_va + HOST_VLAN_STATUS_REG;
			l2br->regs.direct_reg = l2br->cbus_base_va + HOST_VLAN_DIRECT_REG;
			l2br->regs.free_entries_reg = l2br->cbus_base_va + HOST_VLAN_FREE_LIST_ENTRIES;
			l2br->regs.free_head_ptr_reg = l2br->cbus_base_va + HOST_VLAN_FREE_LIST_HEAD_PTR;
			l2br->regs.free_head_ptr_reg = l2br->cbus_base_va + HOST_VLAN_FREE_LIST_HEAD_PTR;
			l2br->regs.free_tail_ptr_reg = l2br->cbus_base_va + HOST_VLAN_FREE_LIST_TAIL_PTR;
			l2br->regs.free_tail_ptr_reg = l2br->cbus_base_va + HOST_VLAN_FREE_LIST_TAIL_PTR;
			l2br->hash_space_depth = _VLAN_TABLE_HASH_ENTRIES;
			l2br->coll_space_depth = _VLAN_TABLE_COLL_ENTRIES;
			break;
		}

		default:
		{
			NXP_LOG_ERROR("Invalid table type\n");
			oal_mm_free(l2br);
			return NULL;
		}
	}

	/*	Initialize the table */
	ret = pfe_l2br_table_init_cmd(l2br);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Table initialization failed: %d\n", ret);
		oal_mm_free(l2br);
		l2br = NULL;
	}

	return l2br;
}

/**
 * @brief		Destroy L2 bridge table instance
 * @param[in]	l2br The L2 bridge table instance
 */
void pfe_l2br_table_destroy(pfe_l2br_table_t *l2br)
{
	if (NULL != l2br)
	{
		oal_mm_free(l2br);
	}
}

/**
 * @brief		Create and initialize L2 bridge table entry instance
 * @note		When not needed entry shall be released by pfe_l2br_table_entry_destroy()
 * @param[in]	l2br The L2 bridge table instance
 * @return		Bridge table entry instance or NULL if failed
 */
pfe_l2br_table_entry_t *pfe_l2br_table_entry_create(pfe_l2br_table_t *l2br)
{
	pfe_l2br_table_entry_t *entry = NULL;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == l2br))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	entry = oal_mm_malloc(sizeof(pfe_l2br_table_entry_t));
	if (NULL == entry)
	{
		NXP_LOG_ERROR("malloc() failed\n");
	}
	else
	{
		memset(entry, 0, sizeof(pfe_l2br_table_entry_t));
		entry->type = l2br->type;
	}

	/*	TODO: Only for debug purposes */
	entry->action_data_set = FALSE;
	entry->mac_addr_set = FALSE;
	entry->vlan_set = FALSE;

	return entry;
}

/**
 * @brief		Destroy entry created by pfe_l2br_table_entry_create()
 * @param[in]	entry The entry to be destroyed
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 */
errno_t pfe_l2br_table_entry_destroy(pfe_l2br_table_entry_t *entry)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	oal_mm_free(entry);

	return EOK;
}

/**
 * @brief		Set MAC address
 * @param[in]	entry The entry
 * @param[in]	mac_addr MAC address to be associated with the entry
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 * @retval		EPERM Operation not permitted
 */
errno_t pfe_l2br_table_entry_set_mac_addr(pfe_l2br_table_entry_t *entry, pfe_mac_addr_t mac_addr)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (PFE_L2BR_TABLE_MAC2F == entry->type)
	{
		memcpy(entry->mac2f_entry.mac, mac_addr, sizeof(pfe_mac_addr_t));
		entry->mac2f_entry.field_valids |= MAC2F_ENTRY_MAC_VALID;
	}
	else if (PFE_L2BR_TABLE_VLAN == entry->type)
	{
		NXP_LOG_DEBUG("Unsupported entry type\n");
		return EPERM;
	}
	else
	{
		NXP_LOG_DEBUG("Invalid entry type\n");
		return EINVAL;
	}

	entry->mac_addr_set = TRUE;

	return EOK;
}

/**
 * @brief		Set VLAN
 * @param[in]	entry The entry
 * @param[in]	mac_addr VLAN tag to be associated with the entry (13-bit)
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 */
errno_t pfe_l2br_table_entry_set_vlan(pfe_l2br_table_entry_t *entry, uint16_t vlan)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (PFE_L2BR_TABLE_MAC2F == entry->type)
	{
		entry->mac2f_entry.vlan = (vlan & 0x1fffU);
		entry->mac2f_entry.field_valids |= MAC2F_ENTRY_VLAN_VALID;
	}
	else if (PFE_L2BR_TABLE_VLAN == entry->type)
	{
		entry->vlan_entry.vlan = (vlan & 0x1fffU);
		entry->vlan_entry.field_valids |= VLAN_ENTRY_VLAN_VALID;
	}
	else
	{
		NXP_LOG_DEBUG("Invalid entry type\n");
		return EINVAL;
	}

	entry->vlan_set = TRUE;

	return EOK;
}

/**
 * @brief		Associate action data with table entry
 * @details		Action data vector is available as output of entry match event.
 * @param[in]	entry The entry
 * @param[in]	action The action data
 * @retval		EOK Success
 * @retval		EINVAL Invalid/missing argument
 */
errno_t pfe_l2br_table_entry_set_action_data(pfe_l2br_table_entry_t *entry, uint64_t action_data)
{
#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (PFE_L2BR_TABLE_MAC2F == entry->type)
	{
		if (action_data > 0x7fffffffU)
		{
			NXP_LOG_DEBUG("Action data too long. Max 31bits allowed for MAC table.\n");
		}

		entry->mac2f_entry.action_data = (uint32_t)(action_data & 0x7fffffffU);
	}
	else if (PFE_L2BR_TABLE_VLAN == entry->type)
	{
		if (action_data > 0x7fffffffffffffULL)
		{
			NXP_LOG_DEBUG("Action data too long. Max 55bits allowed for VLAN table.\n");
		}

		entry->vlan_entry.action_data = (uint64_t)(action_data & 0x7fffffffffffffULL);
	}
	else
	{
		NXP_LOG_DEBUG("Invalid entry type\n");
		return EINVAL;
	}

	entry->action_data_set = TRUE;

	return EOK;
}

/**
 * @brief		Set 'fresh' bit value
 * @param[in]	entry The entry
 * @param[in]	is_fresh The 'fresh' bit value to be set
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Entry not found
 * @retval		ENOEXEC Command failed
 * @retval		ETIMEDOUT Command timed-out
 */
errno_t pfe_l2br_table_entry_set_fresh(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry, bool_t is_fresh)
{
	uint32_t action_data;
	pfe_ct_mac_table_result_t *mac_entry = (pfe_ct_mac_table_result_t *)&action_data;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if ((l2br->type != PFE_L2BR_TABLE_MAC2F) || (entry->type != PFE_L2BR_TABLE_MAC2F))
	{
		/*	Only MAC table entries can be currently 'fresh' */
		return EINVAL;
	}

	/*	Update the action entry */
	action_data = entry->mac2f_entry.action_data;
	mac_entry->fresh_flag = (TRUE == is_fresh) ? 1U : 0U;
	entry->mac2f_entry.action_data = action_data;

	return EOK;
}

/**
 * @brief		Get 'fresh' bit value
 * @details		Fresh bit within an entry indicates that entry is actively being
 * 				used by packet classification process within the PFE. Can be used
 * 				to measure time since the entry has been used last time.
 * @param[in]	entry The entry
 * @return		TRUE if entry is fresh, FALSE otherwise
 */
__attribute__((pure)) bool_t pfe_l2br_table_entry_is_fresh(pfe_l2br_table_entry_t *entry)
{
	uint32_t action_data;
	pfe_ct_mac_table_result_t *mac_entry;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	action_data = entry->mac2f_entry.action_data;
	mac_entry = (pfe_ct_mac_table_result_t *)&action_data;

	if (PFE_L2BR_TABLE_MAC2F == entry->type)
	{
		return (0U != mac_entry->fresh_flag);
	}
	else
	{
		NXP_LOG_DEBUG("Invalid entry type\n");
		return FALSE;
	}
}

/**
 * @brief		Set 'static' bit value
 * @details		Setting the static bit makes the entry static meaning that it is not subject
 * 				of aging.
 * @param[in]	entry The entry
 * @param[in]	is_static The 'static' bit value to be set
 * @retval		EOK Success
 * @retval		EINVAL Invalid or missing argument
 * @retval		ENOENT Entry not found
 * @retval		ENOEXEC Command failed
 * @retval		ETIMEDOUT Command timed-out
 */
errno_t pfe_l2br_table_entry_set_static(pfe_l2br_table_t *l2br, pfe_l2br_table_entry_t *entry, bool_t is_static)
{
	uint32_t action_data;
	pfe_ct_mac_table_result_t *mac_entry = (pfe_ct_mac_table_result_t *)&action_data;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == l2br) || (NULL == entry)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if ((l2br->type != PFE_L2BR_TABLE_MAC2F) || (entry->type != PFE_L2BR_TABLE_MAC2F))
	{
		/*	Only MAC table entries can be currently 'static' */
		return EINVAL;
	}

	/*	Update the action entry */
	action_data = entry->mac2f_entry.action_data;
	mac_entry->static_flag = (TRUE == is_static) ? 1U : 0U;
	entry->mac2f_entry.action_data = action_data;

	return EOK;
}

/**
 * @brief		Get 'static' bit value
 * @details		Static bit indicates that entry is static and is not subject of aging.
 * @param[in]	entry The entry
 * @return		TRUE if entry is fresh, FALSE otherwise
 */
__attribute__((pure)) bool_t pfe_l2br_table_entry_is_static(pfe_l2br_table_entry_t *entry)
{
	uint32_t action_data = entry->mac2f_entry.action_data;
	pfe_ct_mac_table_result_t *mac_entry = (pfe_ct_mac_table_result_t *)&action_data;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == entry))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (PFE_L2BR_TABLE_MAC2F == entry->type)
	{
		return (0U != mac_entry->static_flag);
	}
	else
	{
		NXP_LOG_DEBUG("Invalid entry type\n");
		return FALSE;
	}
}

/**
 * @brief		Convert entry to string representation
 * @param[in]	entry The entry
 * @param[in]	buf Buffer to write the final string to
 * @param[in]	buf_len Buffer length
 */
uint32_t pfe_l2br_table_entry_to_str(pfe_l2br_table_entry_t *entry, char_t *buf, uint32_t buf_len)
{
	uint32_t len = 0U;

#if defined(GLOBAL_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == entry) || (NULL == buf)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* GLOBAL_CFG_NULL_ARG_CHECK */

	if (PFE_L2BR_TABLE_MAC2F == entry->type)
	{
		len += snprintf(buf + len, buf_len - len, "[MAC+VLAN Table Entry]\n");
		len += snprintf(buf + len, buf_len - len, "MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
				entry->mac2f_entry.mac[0],
				entry->mac2f_entry.mac[1],
				entry->mac2f_entry.mac[2],
				entry->mac2f_entry.mac[3],
				entry->mac2f_entry.mac[4],
				entry->mac2f_entry.mac[5]);
		len += snprintf(buf + len, buf_len - len, "VLAN       : 0x%x\n", entry->mac2f_entry.vlan);
		len += snprintf(buf + len, buf_len - len, "Action Data: 0x%x\n", entry->mac2f_entry.action_data);
		len += snprintf(buf + len, buf_len - len, "Port       : 0x%x\n", entry->mac2f_entry.port);
		len += snprintf(buf + len, buf_len - len, "Col Ptr    : 0x%x\n", entry->mac2f_entry.col_ptr);
		len += snprintf(buf + len, buf_len - len, "Flags      : 0x%x\n", entry->mac2f_entry.flags);
	}
	else if (PFE_L2BR_TABLE_VLAN == entry->type)
	{
		len += snprintf(buf + len, buf_len - len, "[VLAN Table Entry]\n");
		len += snprintf(buf + len, buf_len - len, "VLAN       : 0x%x\n", entry->vlan_entry.vlan);
		len += snprintf(buf + len, buf_len - len, "Action Data: 0x%llx\n", (uint64_t)entry->vlan_entry.action_data);
		len += snprintf(buf + len, buf_len - len, "Port       : 0x%x\n", entry->vlan_entry.port);
		len += snprintf(buf + len, buf_len - len, "Col Ptr    : 0x%x\n", entry->vlan_entry.col_ptr);
		len += snprintf(buf + len, buf_len - len, "Flags      : 0x%x\n", entry->vlan_entry.flags);
	}
	else
	{
		len += snprintf(buf + len, buf_len - len, "Invalid entry type\n");
	}
    return len;
}

/** @}*/
