/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2020-2022,2024 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_spd.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_CLEARED_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static pfe_ct_ipsec_spd_t *pfe_spds[PFE_PHY_IF_ID_MAX];

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_CLEARED_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_VAR_INIT_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

static pfe_class_t *class_ptr = NULL;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_VAR_INIT_32
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/*
* @brief Replaces the SPD in the Class PEs DMEM by a new version
* @param[in] phy_if Physical interface which owns the updated SPD (separate SPD for each phy_if)
* @param[in] spd SPD to be used from now by the phy_if
* @param[in] size Size of the spd in bytes.
* @details There is a time interval when 2 instances of the SPD exist (the old one and the new one).
*/
static errno_t pfe_spd_update_phyif(pfe_phy_if_t *phy_if, pfe_ct_ipsec_spd_t *spd, const uint32_t size)
{
    errno_t ret;
    addr_t dmem_addr;
    uint32_t old_addr;
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely((NULL == phy_if) || (NULL == spd)))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else if(unlikely(NULL == class_ptr))
    {
        NXP_LOG_ERROR("Init function not called\n");
        ret = EINVAL;
    }
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
        /* Allocate memory for the new version of the SPD */
        dmem_addr = pfe_class_dmem_heap_alloc(class_ptr, size);
        if(0U == dmem_addr)
        {   /* No memory */
            ret = ENOMEM;
        }
        else
        {   /* Got memory */
            /* Set correct DMEM pointer */
            spd->entries = oal_htonl(dmem_addr + sizeof(pfe_ct_ipsec_spd_t));
            /* Copy the new SPD into allocated memory */
            (void)pfe_class_write_dmem(class_ptr, -1, dmem_addr, (void *)spd, size);
            /* Get the address of the old memory before it is lost */
            old_addr = pfe_phy_if_get_spd(phy_if);
            /* Replace the old SPD pointer by the new one */
            (void)pfe_phy_if_set_spd(phy_if, dmem_addr);
            /* Free the old SPD memory */
            pfe_class_dmem_heap_free(class_ptr, old_addr);
            ret = EOK;
        }
    }
    return ret;
}

/*
* @brief Destroys all SPD information stored in PHY
* @param[in] phy_if Physical interface with SPD to be destroyed
*/
static void pfe_spd_destroy_phyif(pfe_phy_if_t *phy_if)
{
	uint32_t baddr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(unlikely(NULL == phy_if))
	{
		NXP_LOG_ERROR("NULL argument received\n");		
	}
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
	    baddr = pfe_phy_if_get_spd(phy_if);
	    pfe_class_dmem_heap_free(class_ptr, baddr);
	    if (EOK != pfe_phy_if_set_spd(phy_if, 0))
	    {
	    	NXP_LOG_ERROR("PHY SPD memory could't be cleared\n");
	    }
    }
}

/**
* @brief Function initializes the module
* @param[in] class Reference to the class to be used
*/
void pfe_spd_init(pfe_class_t *class)
{
	uint32_t idx;

#if defined(PFE_CFG_NULL_ARG_CHECK)
	if(unlikely(NULL == class))
	{
		NXP_LOG_ERROR("NULL argument received\n");
	}
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
	    /* Initialize the internal context */
	    class_ptr = class;

	    for(idx = 0U; idx < (sizeof(pfe_spds)/sizeof(pfe_spds[0U])); idx++)
	    {
	    	pfe_spds[idx] = NULL;
	    }
    }
}

/**
* @brief Function destroys the module
* @param[in] phy_if_db Database of physical interfaces
*/
void pfe_spd_destroy(pfe_if_db_t *phy_if_db)
{
	uint32_t idx;
	uint32_t session_id = 0;
	errno_t ret = EOK;
	pfe_phy_if_t *phy_if = NULL;
	pfe_if_db_entry_t *if_db_entry = NULL;

	/* Clean the DB */
	for(idx = 0U; idx < (sizeof(pfe_spds)/sizeof(pfe_spds[0U])); idx++)
	{
		if(NULL == pfe_spds[idx])
		{
			continue;
		}

		phy_if = NULL;
		if_db_entry = NULL;

		if(EOK != pfe_if_db_polled_lock(&session_id))
		{
			NXP_LOG_ERROR("DB lock failed\n");
		}

		/* Get PHY from DB*/
		ret = pfe_if_db_get_first(phy_if_db, session_id, IF_DB_CRIT_BY_ID, (void *)(addr_t)idx, &if_db_entry);
		if (ret == EOK)
		{
			phy_if = pfe_if_db_entry_get_phy_if(if_db_entry);

			if (NULL != phy_if)
			{
				/* Clean all SPD info from the PHY */
				pfe_spd_destroy_phyif(phy_if);
			}
			else
			{
				NXP_LOG_ERROR("Invalid PHY instance\n");
			}
		}
		else
		{
			NXP_LOG_ERROR("Couldn't get PHY instance\n");
		}

		oal_mm_free(pfe_spds[idx]);
		pfe_spds[idx] = NULL;

		if(EOK != pfe_if_db_unlock(session_id))
		{
			NXP_LOG_ERROR("DB unlock failed\n");
		}

	}

	/* Forget class */
	class_ptr = NULL;
}

/**
* @brief Adds a rule to the SPD at given position
* @param[in] phy_if Physical interface which SPD shall be updated
* @param[in] position Position where to insert the entry
* @param[in] entry Entry to be inserted into the SPD
* @details If there is not SPD created (1st rule) the function creates one and stores the specified entry there.
*          Otherwise the rule is stored at specified position (rule already existing at that position will immediately follow
*          the newly added rule i.e. position 0 means the rule is inserted as the 1st one). Specifying position greater than
*          the number of rules stores the rule as the last one.
*          The SPD update is immediately propagated to the Class PEs DMEM.
*/
errno_t pfe_spd_add_rule(pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry)
{
    pfe_ct_phy_if_id_t phy_if_id = pfe_phy_if_get_id(phy_if);
    pfe_ct_ipsec_spd_t *spd;
    pfe_ct_spd_entry_t *entries;        /* New SPD entries pointer in host memory */
    pfe_ct_spd_entry_t *old_entries;    /* Old SPD entries pointer in host memory */
    errno_t ret;
    uint32_t new_count, old_count; /* Entries count */
    addr_t dmem_addr;

#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely((NULL == phy_if) || (NULL == entry)))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else if(unlikely(NULL == class_ptr))
    {
        NXP_LOG_ERROR("Init function not called\n");
        ret = EINVAL;
    }
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
        /* Check whether there is a database already */
        if(NULL == pfe_spds[phy_if_id])
        {   /* 1st rule, create the database */
            /* Allocate memory for the database and one rule */
            new_count = 1U;
            spd = oal_mm_malloc(sizeof(pfe_ct_ipsec_spd_t) + sizeof(pfe_ct_spd_entry_t));
            entries = (pfe_ct_spd_entry_t *)(&spd[1]);
            if(NULL != spd)
            {
                /* Initialize the database content */
                spd->entry_count = oal_htonl(new_count);
                spd->no_ip_action = SPD_ACT_BYPASS; /* As required by the spec. */ /* todo - make it configurable AAVB-2450 */
                /* Set the new entry */
                (void)memcpy(&entries[0U], entry, sizeof(pfe_ct_spd_entry_t));
                /* Store the new database */
                pfe_spds[phy_if_id] = spd;
                /* Write spd to PE memory and update the physical interface */
                dmem_addr = pfe_class_dmem_heap_alloc(class_ptr, sizeof(pfe_ct_ipsec_spd_t) + sizeof(pfe_ct_spd_entry_t));
                if(0U != dmem_addr)
                {
                    spd->entries = oal_htonl(dmem_addr + sizeof(pfe_ct_ipsec_spd_t));
                    (void)pfe_class_write_dmem(class_ptr, -1, dmem_addr, (void *)spd, sizeof(pfe_ct_ipsec_spd_t) + sizeof(pfe_ct_spd_entry_t));
                    (void)pfe_phy_if_set_spd(phy_if, dmem_addr);
                    ret = EOK;
                }
                else
                {   /* Failed */
                    /* Avoid memory leaks */
                    oal_mm_free(spd);
                    ret = ENOMEM;
                }
            }
            else
            {
                NXP_LOG_ERROR("Unable to allocate memory\n");
                ret = ENOMEM;
            }
        }
        else
        {   /* New rule into existing database */
            /* Allocate memory for copy of the existing database + one more rule */
            old_count = oal_ntohl(pfe_spds[phy_if_id]->entry_count);
            new_count =  old_count + 1U;
            spd = oal_mm_malloc(sizeof(pfe_ct_ipsec_spd_t) + (new_count * sizeof(pfe_ct_spd_entry_t)));
            entries = (pfe_ct_spd_entry_t *)(&spd[1]);
            old_entries = (pfe_ct_spd_entry_t *)(&pfe_spds[phy_if_id][1]);
            if(NULL != spd)
            {
                /* Update the database content */
                spd->entry_count = oal_htonl(new_count);
                spd->no_ip_action = pfe_spds[phy_if_id]->no_ip_action;
                spd->entries = pfe_spds[phy_if_id]->entries;
                /* Check where to add
                   Copy data from existing database to the new one leaving space for the new entry
                   Copy the new entry into the reserved space */
                if(position >= old_count)
                {
                    (void)memcpy(&entries[0], &old_entries[0], old_count * sizeof(pfe_ct_spd_entry_t));
                    (void)memcpy(&entries[old_count], entry, sizeof(pfe_ct_spd_entry_t));
                }
                else if(0U == position)
                {   /* Inserting to the 1st position */
                    (void)memcpy(&entries[1], &old_entries[0], old_count * sizeof(pfe_ct_spd_entry_t));
                    (void)memcpy(&entries[0], entry, sizeof(pfe_ct_spd_entry_t));
                }
                else
                {   /* Inserting at given position */
                    (void)memcpy(&entries[0], &old_entries[0], ((uint32_t)position * sizeof(pfe_ct_spd_entry_t)));
                    (void)memcpy(&entries[position + 1U], &old_entries[position], (old_count - position) * sizeof(pfe_ct_spd_entry_t));
                    (void)memcpy(&entries[position], entry, sizeof(pfe_ct_spd_entry_t));
                }
                /* Write spd to PE memory and update the physical interface
                   Release PE memory used by old database */
                ret = pfe_spd_update_phyif(phy_if, spd, sizeof(pfe_ct_ipsec_spd_t) + (new_count * sizeof(pfe_ct_spd_entry_t)));

                if(EOK == ret)
                {
                    /* Release memory of the old database version */
                    oal_mm_free(pfe_spds[phy_if_id]);
                    /* Store the new database */
                    pfe_spds[phy_if_id] = spd;
                }
                else
                {   /* Failed to update the PE memory */
                    /* Just forget the new SPD version and keep using the old one */
                    oal_mm_free(spd);
                }
            }
            else
            {
                NXP_LOG_ERROR("Unable to allocate memory\n");
                ret = ENOMEM;
            }
        }
    }
    return ret;
}

/**
* @brief Removes the rule at a given position
* @param[in] phy_if Physical interface which spd shall be updated
* @param[in] position Position of the rule to be removed
*/
errno_t pfe_spd_remove_rule(pfe_phy_if_t * phy_if, uint16_t position)
{
    pfe_ct_phy_if_id_t phy_if_id = pfe_phy_if_get_id(phy_if);
    pfe_ct_spd_entry_t *entries;        /* New SPD entries pointer in host memory */
    pfe_ct_spd_entry_t *old_entries;    /* Old SPD entries pointer in host memory */
    errno_t ret;
    uint32_t entry_count;
	uint32_t old_addr;
    pfe_ct_ipsec_spd_t *spd;

#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely(NULL == phy_if))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else if(unlikely(NULL == class_ptr))
    {
        NXP_LOG_ERROR("Init function not called\n");
        ret = EINVAL;
    }
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
        if(NULL == pfe_spds[phy_if_id])
        {
           ret = EINVAL;
        }
        else
        {
            entry_count = oal_ntohl(pfe_spds[phy_if_id]->entry_count);
            if(entry_count < 2U)
            {   /* Removing the last entry */
                /* Just free the memory */
                oal_mm_free(pfe_spds[phy_if_id]);
                pfe_spds[phy_if_id] = NULL;
                /* Update the physical interface */
                old_addr = pfe_phy_if_get_spd(phy_if);
                (void)pfe_phy_if_set_spd(phy_if, 0U);
                /* Release the PE memory */
                pfe_class_dmem_heap_free(class_ptr, old_addr);
                ret = EOK;
            }
            else
            {
                /* Calculate new entry count */
                entry_count -= 1U;
                /* Allocate the memory for new version of database containing one entry less */
                spd = oal_mm_malloc(sizeof(pfe_ct_ipsec_spd_t) + (entry_count * sizeof(pfe_ct_spd_entry_t)));
                entries = (pfe_ct_spd_entry_t *)(&spd[1]);
                old_entries = (pfe_ct_spd_entry_t *)(&pfe_spds[phy_if_id][1]);
                if(NULL != spd)
                {
                    /* Update the database content */
                    spd->entry_count = oal_htonl(entry_count);
                    spd->no_ip_action = pfe_spds[phy_if_id]->no_ip_action;
                    spd->entries = pfe_spds[phy_if_id]->entries;

                    if(position >= entry_count)
                    {   /* Removing the last position - copy all but the last entry */
                        (void)memcpy(&entries[0], &old_entries[0], entry_count * sizeof(pfe_ct_spd_entry_t));
                    }
                    else
                    {   /* Removing other than last position - copy the tail over the entry */
                        (void)memcpy(&entries[0], &old_entries[0U], ((uint32_t)position * sizeof(pfe_ct_spd_entry_t)));
                        (void)memcpy(&entries[position], &old_entries[position + 1U], (entry_count - position) * sizeof(pfe_ct_spd_entry_t));
                    }

                    /* Write spd to PE memory and update the physical interface
                       Release PE memory used by old database */
                    ret = pfe_spd_update_phyif(phy_if, spd, sizeof(pfe_ct_ipsec_spd_t) + (entry_count * sizeof(pfe_ct_spd_entry_t)));
                    if(EOK == ret)
                    {
                        /* Release memory of the old database version */
                        oal_mm_free(pfe_spds[phy_if_id]);
                        /* Store the new database */
                        pfe_spds[phy_if_id] = spd;
                    }
                    else
                    {   /* Failed to update the PE memory */
                        /* Just forget the new SPD version and keep using the old one */
                        oal_mm_free(spd);
                    }
                }
                else
                {
                    NXP_LOG_ERROR("Unable to allocate memory\n");
                    ret = ENOMEM;
                }
            }
        }
    }
    return ret;
}

/**
* @brief Retrieves rule at given position
* @param[in] phy_if Physical interface which SPD shall be queried
* @param[in] position Position of the rule to be read
* @param[out] entry Retrieved rule, valid only if EOK is returned
*/
errno_t pfe_spd_get_rule(const pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry)
{
	errno_t ret;
	uint32_t entry_count;
    pfe_ct_spd_entry_t *entries;
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely((NULL == phy_if) || (NULL == entry)))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        ret = EINVAL;
    }
    else
#endif /* PFE_CFG_NULL_ARG_CHECK */
    {
        pfe_ct_phy_if_id_t phy_if_id = pfe_phy_if_get_id(phy_if);

        if(NULL == pfe_spds[phy_if_id])
        {
            ret = ENOENT;
        }
        else
        {
            entry_count = oal_ntohl(pfe_spds[phy_if_id]->entry_count);
            if(position >= entry_count)
            {
                ret = ENOENT;
            }
            else
            {
                /* Simply copy the requested rule from the database */
                entries = (pfe_ct_spd_entry_t *)(&pfe_spds[phy_if_id][1]);
                (void)memcpy(entry, &entries[position], sizeof(pfe_ct_spd_entry_t));
                ret = EOK;
            }
        }
    }
    return ret;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_PFE_MASTER */

