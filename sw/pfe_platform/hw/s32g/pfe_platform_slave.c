/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"

#ifdef PFE_CFG_PFE_SLAVE
#include "hal.h"
#include "pfe_cbus.h"
#include "pfe_platform_cfg.h"
#include "pfe_platform.h"
#include "pfe_ct.h"
#include "pfe_idex.h"

#ifdef PFE_CFG_FCI_ENABLE
#include "fci.h"
#endif /* PFE_CFG_FCI_ENABLE */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_INIT_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static pfe_platform_t pfe = {.probed = FALSE};
/* usage scope: pfe_platform_create_hif*/
static pfe_hif_chnl_id_t ids[HIF_CFG_MAX_CHANNELS] = {HIF_CHNL_0, HIF_CHNL_1, HIF_CHNL_2, HIF_CHNL_3};
/* usage scope: pfe_platform_create_ifaces*/
static struct
{
    char_t *name;
    pfe_ct_phy_if_id_t id;
    pfe_mac_addr_t mac;
}
phy_ifs[] =
{
        {.name = "emac0", .id = PFE_PHY_IF_ID_EMAC0, .mac = GEMAC0_MAC},
        {.name = "emac1", .id = PFE_PHY_IF_ID_EMAC1, .mac = GEMAC1_MAC},
        {.name = "emac2", .id = PFE_PHY_IF_ID_EMAC2, .mac = GEMAC2_MAC},
        {.name = "hif0", .id = PFE_PHY_IF_ID_HIF0, .mac = {0},},
        {.name = "hif1", .id = PFE_PHY_IF_ID_HIF1, .mac = {0},},
        {.name = "hif2", .id = PFE_PHY_IF_ID_HIF2, .mac = {0},},
        {.name = "hif3", .id = PFE_PHY_IF_ID_HIF3, .mac = {0},},
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
        {.name = "hifncpy", .id = PFE_PHY_IF_ID_HIF_NOCPY, .mac = {0}},
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */
        {.name = NULL, .id = PFE_PHY_IF_ID_INVALID, .mac = {0}}
};

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Eth_43_PFE_MemMap.h"

#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
 * @brief		IDEX RPC callback
 */
void pfe_platform_idex_rpc_cbk(pfe_ct_phy_if_id_t sender, uint32_t id, void *buf, uint16_t buf_len, void *arg)
{
	pfe_platform_t *platform = (pfe_platform_t *)arg;

	(void)sender;
	(void)id;
	(void)buf;
	(void)buf_len;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == platform))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	(void)platform;

	NXP_LOG_INFO("Got IDEX RPC request (reference for future use)\n");

	/*	Report execution status to caller */
	if (EOK != pfe_idex_set_rpc_ret_val(EINVAL, NULL, 0U))
	{
		NXP_LOG_ERROR("Could not send RPC response\n");
	}

	return;
}

/**
 * @brief		Assign HIF to the platform
 */
static errno_t pfe_platform_create_hif(pfe_platform_t *platform, const pfe_platform_config_t *config)
{
	uint32_t ii;
	pfe_hif_chnl_t *chnl;
	pfe_hif_chnl_id_t channel_mask;
	addr_t hif_cbus_base_va = platform->cbus_baseaddr + CBUS_HIF_BASE_ADDR;
	bool_t master_up = FALSE;
	uint32_t slave_tmout = PFE_CFG_SLAVE_HIF_MASTER_UP_TMOUT;

	if (FALSE == config->disable_master_detect)
	{

		/*	Wait for Master up before creation of the HIF channels. If the Slave were not waiting here for
			Master and continue with HIF channel initialization, then the Master would reset Slave's HIF
			configuration during Master init. */
		NXP_LOG_INFO("Wait for Master UP ...\n");
		while(FALSE == master_up)
		{
			channel_mask = config->hif_chnls_mask;
			for (ii = 0U; ii < HIF_CFG_MAX_CHANNELS; (channel_mask >>= 1), ii++)
			{
				if (0 != (channel_mask & 0x01U))
				{
					if (0U != (pfe_hif_chnl_cfg_ltc_get(hif_cbus_base_va, ii) & MASTER_UP))
					{
						master_up = TRUE;
						break;
					}
				}
			}

			if (TRUE == master_up)
			{
				break;
			}

			oal_time_usleep(1000);

			/*	Decrement only for slave_tmout > 0 */
			if (0U != slave_tmout)
			{
				slave_tmout--;
				if (0U == slave_tmout)
				{
					NXP_LOG_INFO("Detection Master UP timeouted\n");
					return ETIMEDOUT;
				}
			}
		}
		NXP_LOG_INFO("Detected Master UP\n");
	}
	else
	{
		NXP_LOG_INFO("Master UP detectection disabled\n");
	}

	platform->hif = pfe_hif_create(hif_cbus_base_va, config->hif_chnls_mask);
	if (NULL == platform->hif)
	{
		NXP_LOG_ERROR("Couldn't create HIF instance\n");
		return ENODEV;
	}

	#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	pfe_hif_set_master_detect_cfg(platform->hif, !config->disable_master_detect);
	#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

	/*	Enable channel interrupts */
	for (ii = 0U; ii < HIF_CFG_MAX_CHANNELS; ii++)
	{
		chnl = pfe_hif_get_channel(platform->hif, ids[ii]);
		if (NULL == chnl)
		{
			/* not requested HIF channel, skipping */
			continue;
		}

		pfe_hif_chnl_irq_unmask(chnl);
	}

	return EOK;
}

/**
 * @brief		Release HIF-related resources
 */
static void pfe_platform_destroy_hif(pfe_platform_t *platform)
{
	if (platform->hif)
	{
		pfe_hif_destroy(platform->hif);
		platform->hif = NULL;
	}
}


#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
/**
 * @brief		Assign HIF NOCPY to the platform
 */
static errno_t pfe_platform_create_hif_nocpy(pfe_platform_t *platform)
{
    uint16_t lmem_header_size;

	if(PFE_S32G3_VERSION == platform->pfe_version)
	{   /* S32G3 */
		lmem_header_size = 48U;
	}
	else
	{   /* S32G2 */
		lmem_header_size = 112U;
	}

	platform->hif_nocpy = pfe_hif_nocpy_create(pfe.cbus_baseaddr + CBUS_HIF_NOCPY_BASE_ADDR, platform->bmu[1], lmem_header_size);

	if (NULL == platform->hif_nocpy)
	{
		NXP_LOG_ERROR("Couldn't create HIF NOCPY instance\n");
		return ENODEV;
	}
	else
	{
		pfe_hif_chnl_irq_unmask(pfe_hif_nocpy_get_channel(platform->hif_nocpy, PFE_HIF_CHNL_NOCPY_ID));
	}

	return EOK;
}

/**
 * @brief		Release HIF-related resources
 */
static void pfe_platform_destroy_hif_nocpy(pfe_platform_t *platform)
{
	if (platform->hif_nocpy)
	{
		pfe_hif_nocpy_destroy(platform->hif_nocpy);
		platform->hif_nocpy = NULL;
	}
}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

/**
 * @brief		Register logical interface
 * @details		Add logical interface to internal database
 */
errno_t pfe_platform_register_log_if(const pfe_platform_t *platform, pfe_log_if_t *log_if)
{
	uint32_t session_id;
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == platform) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("Null argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = pfe_if_db_lock(&session_id);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("DB lock failed: %d\n", ret);
		return ret;
	}

	/*	Register in platform to db */
	ret = pfe_if_db_add(platform->log_if_db, session_id, log_if, PFE_CFG_LOCAL_IF);
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Could not register %s: %d\n", pfe_log_if_get_name(log_if), ret);
		pfe_log_if_destroy(log_if);
	}

	if (EOK != pfe_if_db_unlock(session_id))
	{
		NXP_LOG_DEBUG("DB unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Unregister logical interface
 * @details		Logical interface will be removed from internal database
 * @warning		Should be called only with locked DB
 */
errno_t pfe_platform_unregister_log_if(const pfe_platform_t *platform, pfe_log_if_t *log_if)
{
	errno_t ret = EOK;
	pfe_if_db_entry_t *entry = NULL;
	uint32_t session_id;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == platform) || (NULL == log_if)))
	{
		NXP_LOG_ERROR("Null argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	ret = pfe_if_db_lock(&session_id);
	if (EOK != ret)
	{
		NXP_LOG_DEBUG("DB lock failed: %d\n", ret);
		return ret;
	}

	ret = pfe_if_db_get_first(platform->log_if_db, session_id, IF_DB_CRIT_BY_INSTANCE, (void *)log_if, &entry);
	if (NULL == entry)
	{
		ret = ENOENT;
	}
	else if (EOK == ret)
	{
		ret = pfe_if_db_remove(platform->log_if_db, session_id, entry);
	}

	if (EOK != pfe_if_db_unlock(session_id))
	{
		NXP_LOG_DEBUG("DB unlock failed\n");
	}

	return ret;
}

/**
 * @brief		Register physical interface
 */
static errno_t pfe_platform_register_phy_if(const pfe_platform_t *platform, uint32_t session_id, pfe_phy_if_t *phy_if)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely((NULL == platform) || (NULL == phy_if)))
	{
		NXP_LOG_ERROR("Null argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Owner of the interface is local driver instance */
	ret = pfe_if_db_add(platform->phy_if_db, session_id, phy_if, PFE_CFG_LOCAL_IF);

	return ret;
}

/**
 * @brief		Get physical interface by its ID
 * @param[in]	platform Platform instance
 * @param[in]	id Physical interface ID
 * @return		Logical interface instance or NULL if failed.
 */
pfe_phy_if_t *pfe_platform_get_phy_if_by_id(const pfe_platform_t *platform, pfe_ct_phy_if_id_t id)
{
	pfe_if_db_entry_t *entry = NULL;
	uint32_t session_id;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == platform))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}

	if (unlikely(NULL == platform->phy_if_db))
	{
		NXP_LOG_ERROR("Physical interface DB not found\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (EOK != pfe_if_db_lock(&session_id))
	{
		NXP_LOG_DEBUG("DB lock failed\n");
	}

	pfe_if_db_get_first(platform->phy_if_db, session_id, IF_DB_CRIT_BY_ID, (void *)(addr_t)id, &entry);

	if (EOK != pfe_if_db_unlock(session_id))
	{
		NXP_LOG_DEBUG("DB unlock failed\n");
	}

	return pfe_if_db_entry_get_phy_if(entry);
}

/**
 * @brief		Assign interfaces to the platform.
 */
errno_t pfe_platform_create_ifaces(pfe_platform_t *platform)
{
	int32_t ii;
	pfe_phy_if_t *phy_if = NULL;
	errno_t ret = EOK;
	uint32_t session_id;

	if (NULL == platform->phy_if_db)
	{
		/*	Create database */
		platform->phy_if_db = pfe_if_db_create(PFE_IF_DB_PHY);
		if (NULL == platform->phy_if_db)
		{
			NXP_LOG_DEBUG("Can't create physical interface DB\n");
			return ENODEV;
		}

		if (EOK != pfe_if_db_lock(&session_id))
		{
			NXP_LOG_DEBUG("DB lock failed\n");
		}

		/*	Create physical interfaces */
		for (ii=0; phy_ifs[ii].id!=PFE_PHY_IF_ID_INVALID; ii++)
		{
			/*	Create physical IF */
			phy_if = pfe_phy_if_create(NULL, phy_ifs[ii].id, phy_ifs[ii].name);
			if (NULL == phy_if)
			{
				NXP_LOG_ERROR("Couldn't create %s\n", phy_ifs[ii].name);
				ret = ENODEV;
				break;
			}
			else
			{
				/*	Register in platform */
				if (EOK != pfe_platform_register_phy_if(platform, session_id, phy_if))
				{
					NXP_LOG_ERROR("Could not register %s\n", pfe_phy_if_get_name(phy_if));
					pfe_phy_if_destroy(phy_if);
					phy_if = NULL;
					ret = ENODEV;
					break;
				}
			}
		}

		if (EOK != pfe_if_db_unlock(session_id))
		{
			NXP_LOG_DEBUG("DB unlock failed\n");
		}

		if (EOK != ret)
		{
			return ret;
		}
	}

	if (NULL == platform->log_if_db)
	{
		platform->log_if_db = pfe_if_db_create(PFE_IF_DB_LOG);
		if (NULL == platform->log_if_db)
		{
			NXP_LOG_DEBUG("Can't create logical interface DB\n");
			return ENODEV;
		}
	}

	return ret;
}

#if defined(PFE_CFG_FCI_ENABLE)

/**
 * @brief		Start the FCI endpoint
 *
 */
static errno_t pfe_platform_create_fci(pfe_platform_t *platform)
{
	errno_t ret = EOK;

	ret = fci_init(NULL, "pfe_fci_slave");
	if (EOK != ret)
	{
		NXP_LOG_ERROR("Could not create the FCI endpoint\n");
		return ret;
	}

	platform->fci_created = TRUE;
	return EOK;
}

/**
 * @brief		Release FCI-related resources
 */
static void pfe_platform_destroy_fci(pfe_platform_t *platform)
{
	fci_fini();
	platform->fci_created = FALSE;
}

#endif /* PFE_CFG_FCI_ENABLE */

/**
 * @brief	The platform initialization function
 * @details	Initializes the PFE HW platform and prepares it for usage according to configuration.
 */
errno_t pfe_platform_init(const pfe_platform_config_t *config)
{
	errno_t ret = EOK;

	memset(&pfe, 0U, sizeof(pfe_platform_t));

	/*	Map CBUS address space */
	pfe.cbus_baseaddr = (addr_t)oal_mm_dev_map((void *)config->cbus_base, config->cbus_len);
	if (0ULL == pfe.cbus_baseaddr)
	{
		NXP_LOG_ERROR("Can't map PPFE CBUS\n");
		goto exit;
	}
	else
	{
		NXP_LOG_INFO("PFE CBUS p0x%p mapped @ v0x%"PRINTADDR_T"\n", (void *)config->cbus_base, pfe.cbus_baseaddr);
	}

    pfe.pfe_version = *(uint32_t*)(void*)((addr_t)pfe.cbus_baseaddr + CBUS_GLOBAL_CSR_BASE_ADDR + WSP_VERSION);
	NXP_LOG_INFO("HW version 0x%x\n", (uint_t)pfe.pfe_version);

	ret = pfe_platform_create_hif(&pfe, config);
	if (EOK != ret)
	{
		goto exit;
	}

#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	/*	HIF NOCPY */
	ret = pfe_platform_create_hif_nocpy(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

#if defined(PFE_CFG_FCI_ENABLE)
	ret = pfe_platform_create_fci(&pfe);
	if (EOK != ret)
	{
		goto exit;
	}
#endif /* PFE_CFG_FCI_ENABLE */

	pfe.probed = TRUE;

	return EOK;

exit:
	(void)pfe_platform_remove();
	return ret;
}

/**
 * @brief		Destroy PFE platform
 */
errno_t pfe_platform_remove(void)
{
	errno_t ret;

#if defined(PFE_CFG_FCI_ENABLE)
	pfe_platform_destroy_fci(&pfe);
#endif /* PFE_CFG_FCI_ENABLE */

	pfe_platform_destroy_hif(&pfe);
#if defined(PFE_CFG_HIF_NOCPY_SUPPORT)
	pfe_platform_destroy_hif_nocpy(&pfe);
#endif /* PFE_CFG_HIF_NOCPY_SUPPORT */

	if (0ULL != pfe.cbus_baseaddr)
	{
		ret = oal_mm_dev_unmap((void *)pfe.cbus_baseaddr, PFE_CFG_CBUS_LENGTH/* <- FIXME, should use value used on init instead */);
		if (EOK != ret)
		{
			NXP_LOG_ERROR("Can't unmap PPFE CBUS: %d\n", ret);
			return ret;
		}
	}

	pfe.cbus_baseaddr = 0x0ULL;
	pfe.probed = FALSE;

	return EOK;
}

pfe_platform_t * pfe_platform_get_instance(void)
{
	if (pfe.probed)
	{
		return &pfe;
	}
	else
	{
		return NULL;
	}
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /*PFE_CFG_PFE_SLAVE*/

