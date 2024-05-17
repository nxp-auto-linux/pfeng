/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "hal.h"

#include "pfe_cbus.h"
#include "pfe_hif.h"
#include "pfe_platform_cfg.h"

struct pfe_hif_tag
{
	addr_t cbus_base_va;			/*	CBUS base virtual address */
	pfe_hif_chnl_t **channels;
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
	bool_t disable_master_detect;	/* Shall be Master-detect disabled? */
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#ifdef PFE_CFG_PARANOID_IRQ
	oal_mutex_t lock;			/*	Mutex to lock access to HW resources */
#endif /* PFE_CFG_PARANOID_IRQ */
};

#ifdef PFE_CFG_PFE_MASTER
/**
 * @brief		Master HIF ISR
 * @param[in]	hif The HIF instance
 * @return		EOK if interrupt has been processed
 */
errno_t pfe_hif_isr(pfe_hif_t *hif)
{
	errno_t ret;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return EINVAL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_lock(&hif->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	/*	Run the low-level ISR to identify and process the interrupt */
	ret = pfe_hif_cfg_isr(hif->cbus_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_unlock(&hif->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	return ret;
}

/**
 * @brief		Mask HIF interrupts
 * @details		Only affects HIF IRQs, not channel IRQs.
 * @param[in]	hif The HIF instance
 */
void pfe_hif_irq_mask(pfe_hif_t *hif)
{
#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_lock(&hif->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	pfe_hif_cfg_irq_mask(hif->cbus_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_unlock(&hif->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */
}

/**
 * @brief		Unmask HIF interrupts
 * @details		Only affects HIF IRQs, not channel IRQs.
 * @param[in]	hif The HIF instance
 */
void pfe_hif_irq_unmask(pfe_hif_t *hif)
{
#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_lock(&hif->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	pfe_hif_cfg_irq_unmask(hif->cbus_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_unlock(&hif->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */
}
#endif /* PFE_CFG_PFE_MASTER */

#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
void pfe_hif_set_master_detect_cfg(pfe_hif_t *hif, bool_t on)
{
	hif->disable_master_detect = (!on);
}

bool_t pfe_hif_get_master_detect_cfg(const pfe_hif_t *hif)
{
	return (!hif->disable_master_detect);
}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

/**
 * @brief		Create new HIF instance
 * @details		Creates and initializes HIF instance
 * @param[in]	cbus_base_va CBUS base virtual address
 * @param[in]	channels Bitmask specifying channels to be managed by the instance
 * @return		The HIF instance or NULL if failed
 */
pfe_hif_t *pfe_hif_create(addr_t cbus_base_va, pfe_hif_chnl_id_t channels)
{
	pfe_hif_t *hif;
	int32_t ii;

#ifdef PFE_CFG_PFE_MASTER
	errno_t ret;
#endif /* PFE_CFG_PFE_MASTER */

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL_ADDR == cbus_base_va))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	if (channels >= (1 << HIF_CFG_MAX_CHANNELS))
	{
		return NULL;
	}

	hif = oal_mm_malloc(sizeof(pfe_hif_t));

	if (NULL == hif)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		return NULL;
	}
	else
	{
		memset(hif, 0, sizeof(pfe_hif_t));
		hif->cbus_base_va = cbus_base_va;
	}

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_init(&hif->lock))
	{
		NXP_LOG_ERROR("Can't initialize HIF mutex\n");
		oal_mm_free(hif);
		return NULL;
	}

	if (EOK != oal_mutex_lock(&hif->lock))
	{
		NXP_LOG_ERROR("Mutex lock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	/*	Do HIF HW initialization */
	ret = pfe_hif_cfg_init(hif->cbus_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
	if (EOK != oal_mutex_unlock(&hif->lock))
	{
		NXP_LOG_ERROR("Mutex unlock failed\n");
	}
#endif /* PFE_CFG_PARANOID_IRQ */

	if (EOK != ret)
	{
		NXP_LOG_ERROR("HIF configuration failed: %d\n", ret);
		oal_mm_free(hif);
		return NULL;
	}
#endif /* PFE_CFG_PFE_MASTER */

	/*	Create channels */
	hif->channels = oal_mm_malloc(HIF_CFG_MAX_CHANNELS * sizeof(pfe_hif_chnl_t *));
	if (NULL == hif->channels)
	{
		NXP_LOG_ERROR("Unable to allocate memory\n");
		oal_mm_free(hif);
		return NULL;
	}
	else
	{
		for (ii=0; ii < HIF_CFG_MAX_CHANNELS; (channels >>= 1), ii++)
		{
			if (0 != (channels & 0x1))
			{
				hif->channels[ii] = pfe_hif_chnl_create(hif->cbus_base_va, ii, NULL);
				if (NULL == hif->channels[ii])
				{
					/*	Destroy already created channels */
					for (; ii>=0; ii--)
					{
						if (NULL != hif->channels[ii])
						{
							pfe_hif_chnl_destroy(hif->channels[ii]);
							hif->channels[ii] = NULL;
						}
					}
				}
				else
				{
					/*	Disable both directions */
					pfe_hif_chnl_rx_disable(hif->channels[ii]);
					pfe_hif_chnl_tx_disable(hif->channels[ii]);
				}
			}
			else
			{
				hif->channels[ii] = NULL;
			}
		}
	}

	return hif;
}

/**
 * @brief		Get channel instance according to its ID
 * @details		The channel ID corresponds with indexing within
 * 				the hardware (0, 1, 2 ... HIF_CFG_MAX_CHANNELS-1)
 * @param[in]	hif The HIF instance
 * @param[in]	channel_id The channel ID
 * @return		The HIF channel instance or NULL if failed
 */
pfe_hif_chnl_t *pfe_hif_get_channel(const pfe_hif_t *hif, pfe_hif_chnl_id_t channel_id)
{
	uint32_t ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return NULL;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	/*	Get array index from channel ID */
	for (ii=0U; (channel_id > 0); (channel_id >>= 1), ii++)
	{
		if (0 != (channel_id & 0x1))
		{
			break;
		}
	}

	if (ii < HIF_CFG_MAX_CHANNELS)
	{
		return hif->channels[ii];
	}

	return NULL;
}

/**
 * @brief		Destroy HIF instance
 * @param[in]	hif The HIF instance
 */
void pfe_hif_destroy(pfe_hif_t *hif)
{
	uint32_t ii;

	if (NULL != hif)
	{

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
		/* Clean Master detect flags for all HIF channels */
		pfe_hif_clear_master_up(hif);
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */
#endif /* PFE_CFG_PFE_MASTER */

		if (NULL != hif->channels)
		{
			for (ii=0U; ii<HIF_CFG_MAX_CHANNELS; ii++)
			{
				if (NULL != hif->channels[ii])
				{
					pfe_hif_chnl_rx_disable(hif->channels[ii]);
					pfe_hif_chnl_tx_disable(hif->channels[ii]);

					pfe_hif_chnl_destroy(hif->channels[ii]);
					hif->channels[ii] = NULL;
				}
			}

			oal_mm_free(hif->channels);
			hif->channels = NULL;
		}

#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_lock(&hif->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
#endif /* PFE_CFG_PARANOID_IRQ */

		/*	Finalize the HIF */
		pfe_hif_cfg_fini(hif->cbus_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_unlock(&hif->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}

		if (EOK != oal_mutex_destroy(&hif->lock))
		{
			NXP_LOG_ERROR("Unable to destroy HIF mutex\n");
		}
#endif /* PFE_CFG_PARANOID_IRQ */

		oal_mm_free(hif);
	}
}

#ifdef PFE_CFG_PFE_SLAVE
/**
 * @brief		Return TRUE if Master UP flag is set
 * @param[in]	hif The HIF instance
 */
bool_t pfe_hif_get_master_up(const pfe_hif_t *hif)
{
	uint32_t ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if ((NULL == hif) || (NULL == hif->channels))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return FALSE;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	for (ii = 0U; ii < HIF_CFG_MAX_CHANNELS; ii++)
	{
		if (NULL != hif->channels[ii])
		{
			return (0U != (pfe_hif_chnl_cfg_ltc_get(hif->cbus_base_va, ii) & MASTER_UP));
		}
	}

	return FALSE;
}
#endif /* PFE_CFG_PFE_SLAVE */

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_MULTI_INSTANCE_SUPPORT
/**
 * @brief		Reset master detect flags in all HIF channels
 * @param[in]	hif The HIF instance
 */
void pfe_hif_clear_master_up(const pfe_hif_t *hif)
{
	uint32_t ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if ((NULL == hif) || (NULL == hif->channels))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	for (ii = 0U; ii < HIF_CFG_MAX_CHANNELS; ii++)
	{
		/* We can't use channel object because we need to set also
		   not configured channels */
		pfe_hif_chnl_cfg_ltc_set(hif->cbus_base_va, ii, 0U);
	}
}

/**
 * @brief		Set master detect flags in all HIF channels
 * @details		Set flag to MASTER_UP and optionally to HIF_OCCUPIED
 * @param[in]	hif The HIF instance
 */
void pfe_hif_set_master_up(const pfe_hif_t *hif)
{
	uint32_t ii;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if ((NULL == hif) || (NULL == hif->channels))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	for (ii = 0U; ii < HIF_CFG_MAX_CHANNELS; ii++)
	{
		/* We can't use channel object because we need to set also
		   not configured channels */
		if (NULL != hif->channels[ii])
			pfe_hif_chnl_cfg_ltc_set(hif->cbus_base_va, ii, MASTER_UP | HIF_OCCUPIED);
		else
			pfe_hif_chnl_cfg_ltc_set(hif->cbus_base_va, ii, MASTER_UP);
	}
}
#endif /* PFE_CFG_MULTI_INSTANCE_SUPPORT */

/**
 * @brief		Return HIF runtime statistics in text form
 * @details		Function writes formatted text into given buffer.
 * @param[in]	hif 		The HIF instance
 * @param[in]	buf 		Pointer to the buffer to write to
 * @param[in]	size 		Buffer length
 * @param[in]	verb_level 	Verbosity level
 * @return		Number of bytes written to the buffer
 *
 */
uint32_t pfe_hif_get_text_statistics(const pfe_hif_t *hif, char_t *buf, uint32_t buf_len, uint8_t verb_level)
{
	uint32_t len = 0U;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		return 0U;
	}
#endif /* PFE_CFG_NULL_ARG_CHECK */

	len += pfe_hif_cfg_get_text_stat(hif->cbus_base_va, buf, buf_len, verb_level);

	return len;
}

/**
 * @brief		HIF error polling
 * @param[in]	hif The HIF instance
 * @return		the HIF error interrupt source
 */
uint32_t pfe_hif_get_err_poll(pfe_hif_t *hif)
{
	uint32_t int_src;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if (unlikely(NULL == hif))
	{
		NXP_LOG_ERROR("NULL argument received\n");
		int_src = 0x0U;
	}
	else
#endif /* PFE_CFG_NULL_ARG_CHECK */
	{
#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_lock(&hif->lock))
		{
			NXP_LOG_ERROR("Mutex lock failed\n");
		}
#endif /* PFE_CFG_PARANOID_IRQ */

		/*	Run the low-level polling to identify and process the interrupt error src */
		int_src = pfe_hif_cfg_get_err_poll(hif->cbus_base_va);

#ifdef PFE_CFG_PARANOID_IRQ
		if (EOK != oal_mutex_unlock(&hif->lock))
		{
			NXP_LOG_ERROR("Mutex unlock failed\n");
		}
#endif /* PFE_CFG_PARANOID_IRQ */
	}

	return int_src;
}
#endif /* PFE_CFG_PFE_MASTER */

