/* =========================================================================
 *  Copyright 2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "linked_list.h"
#include "pfe_platform_cfg.h"
//#include "pfe_emac_csr.h"
#include "pfe_cbus.h"
#include "pfe_emac.h"
#include "pfe_idex.h" /* The RPC provider */
#include "pfe_platform_rpc.h" /* The RPC codes and data structures */

struct pfe_emac_tag
{
	addr_t cbus_base_va;		/*	CBUS base virtual address */
	addr_t emac_base_offset;	/*	MAC base offset within CBUS space */

	oal_mutex_t mutex;			/*	Mutex */
	bool_t mdio_locked;			/*	If TRUE then MDIO access is locked and 'mdio_key' is valid */
	uint32_t mdio_key;
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_INIT_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* usage scope: pfe_emac_mdio_lock */
static uint32_t key_seed = 123U;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_INIT_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

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

		if (NULL != emac)
		{
			(void)memset(emac, 0, sizeof(pfe_emac_t));
			emac->cbus_base_va = cbus_base_va;
			emac->emac_base_offset = emac_base;

			if (EOK != oal_mutex_init(&emac->mutex))
			{
				NXP_LOG_ERROR("Mutex init failed\n");
				oal_mm_free(emac);
				emac = NULL;
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
	uint8_t idx;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 255U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	switch ((addr_t)emac->emac_base_offset)
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

/**
 * @brief		Destroy MAC instance
 * @param[in]	emac The EMAC instance
 */
void pfe_emac_destroy(pfe_emac_t *emac)
{
	if (NULL != emac)
	{
		if (EOK != oal_mutex_lock(&emac->mutex))
		{
			NXP_LOG_DEBUG("Mutex lock failed\n");
		}

		/*	Destroy mutex */
		(void)oal_mutex_destroy(&emac->mutex);

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
			emac->mdio_key = key_seed;
			key_seed++;
			*key = emac->mdio_key;
			ret = EOK;
		}

		if (EOK != oal_mutex_unlock(&emac->mutex))
		{
			NXP_LOG_DEBUG("Mutex unlock failed\n");
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
	pfe_platform_rpc_mdio_proxy_arg_t arg = { 0 };
	pfe_platform_rpc_mdio_proxy_ret_t rpc_ret;
	errno_t ret;
	uint8_t idx;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == val)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		idx = pfe_emac_get_index(emac);
		if (idx > 2)
		{
			NXP_LOG_ERROR("Invalid EMAC id: %d\n", idx);
			ret = EINVAL;
		}
		else
		{
			arg.emac_id = idx;
			arg.op = PFE_PLATFORM_RPC_MDIO_OP_READ_CL22;
			arg.pa = pa;
			arg.ra = ra;

			if (EOK != oal_mutex_lock(&emac->mutex))
			{
				NXP_LOG_DEBUG("Mutex lock failed\n");
			}

			if (TRUE == emac->mdio_locked)
			{
				/*	Locked. Check key. */
				if (key == emac->mdio_key)
				{
					ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_MDIO_PROXY, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
					if (EOK != ret)
					{
						NXP_LOG_ERROR("PFE_PLATFORM_RPC_MDIO_PROXY failed: %d\n", ret);
					}
					else
					{
						*val = rpc_ret.val;
					}
				}
				else
				{
					ret = EPERM;
				}
			}
			else
			{
				/*	Unlocked. No check required. */
				ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_MDIO_PROXY, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
				if (EOK != ret)
				{
					NXP_LOG_ERROR("PFE_PLATFORM_RPC_MDIO_PROXY failed: %d\n", ret);
				}
				else
				{
					*val = rpc_ret.val;
				}
			}

			if (EOK != oal_mutex_unlock(&emac->mutex))
			{
				NXP_LOG_DEBUG("Mutex unlock failed\n");
			}
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
	pfe_platform_rpc_mdio_proxy_arg_t arg = { 0 };
	pfe_platform_rpc_mdio_proxy_ret_t rpc_ret;
	errno_t ret;
	uint8_t idx;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		idx = pfe_emac_get_index(emac);
		if (idx > 2)
		{
			NXP_LOG_ERROR("Invalid EMAC id: %d\n", idx);
			ret = EINVAL;
		}
		else
		{
			arg.emac_id = idx;
			arg.op = PFE_PLATFORM_RPC_MDIO_OP_WRITE_CL22;
			arg.pa = pa;
			arg.ra = ra;
			arg.val = val;

			if (EOK != oal_mutex_lock(&emac->mutex))
			{
				NXP_LOG_DEBUG("Mutex lock failed\n");
			}

			if (TRUE == emac->mdio_locked)
			{
				/*	Locked. Check key. */
				if (key == emac->mdio_key)
				{
					ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_MDIO_PROXY, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
					if (EOK != ret)
					{
						NXP_LOG_ERROR("PFE_PLATFORM_RPC_MDIO_PROXY failed: %d\n", ret);
					}
				}
				else
				{
					ret = EPERM;
				}
			}
			else
			{
				/*	Unlocked. No check required. */
				ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_MDIO_PROXY, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
				if (EOK != ret)
				{
					NXP_LOG_ERROR("PFE_PLATFORM_RPC_MDIO_PROXY failed: %d\n", ret);
				}
			}

			if (EOK != oal_mutex_unlock(&emac->mutex))
			{
				NXP_LOG_DEBUG("Mutex unlock failed\n");
			}
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
	pfe_platform_rpc_mdio_proxy_arg_t arg = { 0 };
	pfe_platform_rpc_mdio_proxy_ret_t rpc_ret;
	errno_t ret;
	uint8_t idx;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == emac) || (NULL == val)))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		idx = pfe_emac_get_index(emac);
		if (idx > 2)
		{
			NXP_LOG_ERROR("Invalid EMAC id: %d\n", idx);
			ret = EINVAL;
		}
		else
		{
			arg.emac_id = idx;
			arg.op = PFE_PLATFORM_RPC_MDIO_OP_READ_CL45;
			arg.pa = pa;
			arg.dev = dev;
			arg.ra = ra;

			if (EOK != oal_mutex_lock(&emac->mutex))
			{
				NXP_LOG_DEBUG("Mutex lock failed\n");
			}

			if (TRUE == emac->mdio_locked)
			{
				/*	Locked. Check key. */
				if (key == emac->mdio_key)
				{
					ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_MDIO_PROXY, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
					if (EOK != ret)
					{
						NXP_LOG_ERROR("PFE_PLATFORM_RPC_MDIO_PROXY failed: %d\n", ret);
					}
					else
					{
						*val = rpc_ret.val;
					}
				}
				else
				{
					ret = EPERM;
				}
			}
			else
			{
				/*	Unlocked. No check required. */
				ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_MDIO_PROXY, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
				if (EOK != ret)
				{
					NXP_LOG_ERROR("PFE_PLATFORM_RPC_MDIO_PROXY failed: %d\n", ret);
				}
				else
				{
					*val = rpc_ret.val;
				}
			}

			if (EOK != oal_mutex_unlock(&emac->mutex))
			{
				NXP_LOG_DEBUG("Mutex unlock failed\n");
			}
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
	pfe_platform_rpc_mdio_proxy_arg_t arg = { 0 };
	pfe_platform_rpc_mdio_proxy_ret_t rpc_ret;
	errno_t ret;
	uint8_t idx;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == emac))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		ret = EINVAL;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
		idx = pfe_emac_get_index(emac);
		if (idx > 2)
		{
			NXP_LOG_ERROR("Invalid EMAC id: %d\n", idx);
			ret = EINVAL;
		}
		else
		{
			arg.emac_id = idx;
			arg.op = PFE_PLATFORM_RPC_MDIO_OP_WRITE_CL45;
			arg.pa = pa;
			arg.dev = dev;
			arg.ra = ra;
			arg.val = val;

			if (EOK != oal_mutex_lock(&emac->mutex))
			{
				NXP_LOG_DEBUG("Mutex lock failed\n");
			}

			if (TRUE == emac->mdio_locked)
			{
				/*	Locked. Check key. */
				if (key == emac->mdio_key)
				{
					ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_MDIO_PROXY, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
					if (EOK != ret)
					{
						NXP_LOG_ERROR("PFE_PLATFORM_RPC_MDIO_PROXY failed: %d\n", ret);
					}
				}
				else
				{
					ret = EPERM;
				}
			}
			else
			{
				/*	Unlocked. No check required. */
				ret = pfe_idex_master_rpc(PFE_PLATFORM_RPC_MDIO_PROXY, &arg, sizeof(arg), &rpc_ret, sizeof(rpc_ret));
				if (EOK != ret)
				{
					NXP_LOG_ERROR("PFE_PLATFORM_RPC_MDIO_PROXY failed: %d\n", ret);
				}
			}

			if (EOK != oal_mutex_unlock(&emac->mutex))
			{
				NXP_LOG_DEBUG("Mutex unlock failed\n");
			}
		}
	}

	return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

