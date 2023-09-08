/* =========================================================================
 *  Copyright 2022,2023 NXP
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
#include "pfe_idex.h" /* The RPC provider */
#include "pfe_platform_rpc.h" /* The RPC codes and data structures */

struct pfe_emac_tag
{
	addr_t cbus_base_va;			/*	CBUS base virtual address */
	addr_t emac_base_offset;		/*	MAC base offset within CBUS space */
	addr_t emac_base_va;			/*	MAC base address (virtual) */
	oal_mutex_t mutex;			/*	Mutex */
	bool_t mdio_locked;			/*	If TRUE then MDIO access is locked and 'mdio_key' is valid */
	uint32_t mdio_key;

	oal_mutex_t ts_mutex;		/*	Mutex protecting IEEE1588 resources */
	uint32_t i_clk_hz;			/*	IEEE1588 input clock */
	uint32_t o_clk_hz;			/*	IEEE1588 desired output clock */
	uint32_t adj_ppb;			/*	IEEE1588 frequency adjustment value */
	bool_t adj_sign;			/*	IEEE1588 frequency adjustment sign (TRUE - positive, FALSE - negative) */
	bool_t ext_ts;				/*  IEEE1588 external timestamp mode */
};

/* usage scope: pfe_emac_mdio_lock */
static uint32_t key_seed = 123U;

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
			emac->emac_base_va = (emac->cbus_base_va + emac->emac_base_offset);

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

/* Direct time synchronization */

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

