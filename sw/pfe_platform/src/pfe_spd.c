/* =========================================================================
 *  
 *  Copyright (c) 2021 Imagination Technologies Limited
 *  Copyright 2020 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_ct.h"
#include "pfe_class.h"
#include "pfe_phy_if.h"
static pfe_ct_ipsec_spd_t *pfe_spds[PFE_PHY_IF_ID_MAX] = {NULL};
static pfe_class_t *class_ptr = NULL;

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
        return EINVAL;
    }
    if(unlikely(NULL == class_ptr))
    {
        NXP_LOG_ERROR("Init function not called\n");
        return EINVAL;
    }
#endif
    /* Allocate memory for the new version of the SPD */
    dmem_addr = pfe_class_dmem_heap_alloc(class_ptr, size);
    if(0 == dmem_addr)
    {   /* No memory */
        ret = ENOMEM;
    }
    else
    {   /* Got memory */
        /* Set correct DMEM pointer */
        spd->entries = oal_htonl(dmem_addr + sizeof(pfe_ct_ipsec_spd_t));
        /* Copy the new SPD into allocated memory */
        pfe_class_write_dmem(class_ptr, -1, (void *)dmem_addr, spd, size);
        /* Get the address of the old memory before it is lost */
        old_addr = pfe_phy_if_get_spd(phy_if);
        /* Replace the old SPD pointer by the new one */
        pfe_phy_if_set_spd(phy_if, dmem_addr);
        /* Free the old SPD memory */
        pfe_class_dmem_heap_free(class_ptr, old_addr);
        ret = EOK;
    }
    return ret;
}

/**
* @brief Function initializes the module
* @param[in] class Reference to the class to be used
*/
void pfe_spd_init(pfe_class_t *class)
{
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely(NULL == class))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        return;
    }
#endif
    /* Initialize the internal context */
    class_ptr = class;
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
        return EINVAL;
    }
    if(unlikely(NULL == class_ptr))
    {
        NXP_LOG_ERROR("Init function not called\n");
        return EINVAL;
    }
#endif
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
            spd->no_ip_action = SPD_ACT_BYPASS; /* As required by the spec. */ //todo - make it configurable AAVB-2450           
            /* Set the new entry */
            memcpy(&entries[0U], entry, sizeof(pfe_ct_spd_entry_t));
            /* Store the new database */
            pfe_spds[phy_if_id] = spd;
            /* Write spd to PE memory and update the physical interface */
            dmem_addr = pfe_class_dmem_heap_alloc(class_ptr, sizeof(pfe_ct_ipsec_spd_t) + sizeof(pfe_ct_spd_entry_t));
            if(0 != dmem_addr)
            {
                spd->entries = oal_htonl(dmem_addr + sizeof(pfe_ct_ipsec_spd_t));
                pfe_class_write_dmem(class_ptr, -1, (void *)dmem_addr, spd, sizeof(pfe_ct_ipsec_spd_t) + sizeof(pfe_ct_spd_entry_t));
                pfe_phy_if_set_spd(phy_if, dmem_addr);
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
            {   /* Adding to the last position */
                position = old_count;
                memcpy(&entries[0], &old_entries[0], old_count * sizeof(pfe_ct_spd_entry_t));
                memcpy(&entries[old_count], entry, sizeof(pfe_ct_spd_entry_t));
            }
            else if(0 == position)
            {   /* Inserting to the 1st position */
                memcpy(&entries[1], &old_entries[0], old_count * sizeof(pfe_ct_spd_entry_t));
                memcpy(&entries[0], entry, sizeof(pfe_ct_spd_entry_t));
            }
            else
            {   /* Inserting at given position */
                memcpy(&entries[0], &old_entries[0], position * sizeof(pfe_ct_spd_entry_t));
                memcpy(&entries[position + 1], &old_entries[position], (old_count - position) * sizeof(pfe_ct_spd_entry_t));
                memcpy(&entries[position], entry, sizeof(pfe_ct_spd_entry_t));
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
            ret = ENOMEM;
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
        return EINVAL;
    }
    if(unlikely(NULL == class_ptr))
    {
        NXP_LOG_ERROR("Init function not called\n");
        return EINVAL;
    }
#endif
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
            pfe_phy_if_set_spd(phy_if, 0U);
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
                    memcpy(&entries[0], &old_entries[0], entry_count * sizeof(pfe_ct_spd_entry_t));
                }
                else
                {   /* Removing other than last position - copy the tail over the entry */
                    memcpy(&entries[0], &old_entries[0U], position * sizeof(pfe_ct_spd_entry_t));
                    memcpy(&entries[position], &old_entries[position + 1U], (entry_count - position) * sizeof(pfe_ct_spd_entry_t));
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
                ret = ENOMEM;
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
errno_t pfe_spd_get_rule(pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry)
{
	errno_t ret;
	uint32_t entry_count;
    pfe_ct_spd_entry_t *entries;
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely((NULL == phy_if) || (NULL == entry)))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
#endif
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
            memcpy(entry, &entries[position], sizeof(pfe_ct_spd_entry_t));
            ret = EOK;
        }
    }
    return ret;
}
