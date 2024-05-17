/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2020-2023 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_ct.h"
#include "blalloc.h"
#include "pfe_spd.h"
#include "pfe_spd_acc.h"

#ifdef PFE_CFG_PFE_MASTER
#ifdef PFE_CFG_FCI_ENABLE

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* HW acceleration of the SPD entry search must be supported by a proper configuration
   which we do here */

#if 0 /* todo AAVB-2539 */
#if defined(PFE_CFG_RTABLE_ENABLE)
static blalloc_t *pfe_spd_acc_id_pool = NULL;
static pfe_rtable_t *rtable_ptr = NULL;
#endif
#endif

/**
* @brief Initializes the module
* @param[in] class Reference to the class
* @param[in] rtable Reference to the rtable
* @return EOK or an error code in case of failure
* @note No other function call is allowed before call of the pfe_spd_acc_init()
*/
errno_t pfe_spd_acc_init(pfe_class_t *class, const pfe_rtable_t *rtable)
{
    errno_t ret = EOK;
#if 0 /* todo AAVB-2539 */
    addr_t id;
#endif
    /* Initialize submodules */
    pfe_spd_init(class);
#if 0 /* todo AAVB-2539 */
#if defined(PFE_CFG_RTABLE_ENABLE)
    rtable_ptr = rtable;
    /* Create a pool of unique IDs */
    pfe_spd_acc_id_pool = blalloc_create(pfe_rtable_get_size(rtable), 0U);
    if(NULL != pfe_spd_acc_id_pool)
    {
        /* Allocate the 1st ID equal to 0 which is reserved */
        if(EOK != blalloc_alloc_offs(pfe_spd_acc_id_pool, 1, 0, &id))
        {
            NXP_LOG_ERROR("ID allocator failure\n");
            blalloc_destroy(pfe_spd_acc_id_pool);
            pfe_spd_acc_id_pool = NULL;
        }
    }
    else
    {
        ret = ENOMEM;
    }
#endif
#else
    (void)rtable;
#endif
    return ret;

}

/**
* @brief Destroys the module
* @note No other function is allowed to be called after pfe_spd_acc_destroy() call except pfe_spd_acc_init()
*/
void pfe_spd_acc_destroy(pfe_if_db_t *phy_if_db)
{
#if 0 /* todo AAVB-2539 */
#if defined(PFE_CFG_RTABLE_ENABLE)
    if(NULL != pfe_spd_acc_id_pool)
    {
        blalloc_free_offs_size(pfe_spd_acc_id_pool, 0U, 1U);
        blalloc_destroy(pfe_spd_acc_id_pool);
        pfe_spd_acc_id_pool = NULL;
    }
    rtable_ptr = NULL;
#endif
#endif

    pfe_spd_destroy(phy_if_db);

#if 0 /* todo AAVB-2539 */
#if defined(PFE_CFG_RTABLE_ENABLE)
    /* Forget platform instances */
    pfe_spd_acc_id_pool = NULL;
    rtable_ptr = NULL;
#endif
#endif
}

#if 0 /* todo AAVB-2539 */
/**
* @brief Converts the type pfe_ct_spd_entry_t into pfe_rtable_entry_t
* @details The conversion of pfe_ct_spd_entry_t into pfe_rtable_entry_t is needed in a specific
*          way when some parameters are not used/set.
*/
static inline errno_t pfe_spd_acc_convert_to_rt_entry(pfe_ct_spd_entry_t *entry, pfe_rtable_entry_t *rt_entry)
{
    pfe_ip_addr_t sip, dip;
    errno_t ret = EOK;
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely((NULL == entry) || (NULL == rt_entry)))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
#endif
    /* Set the 5-tuple values */
    if(0U == (entry->flags & SPD_FLAG_IPv6))
    {   /* IPv4 */
        memcpy(&sip.v4, &entry->u.v4.sip, 4U);
        memcpy(&dip.v4, &entry->u.v4.dip, 4U);
        sip.is_ipv4 = TRUE;
        dip.is_ipv4 = TRUE;
        ret = pfe_rtable_entry_set_sip(rt_entry, &sip);
        if(ret != EOK)
        {
            return ret;
        }
        ret = pfe_rtable_entry_set_dip(rt_entry, &dip);
        if(ret != EOK)
        {
            return ret;
        }
    }
    else
    {   /* IPv6 */
        memcpy(&sip.v6, entry->u.v6.sip, 16U);
        memcpy(&dip.v6, entry->u.v6.dip, 16U);
        sip.is_ipv4 = TRUE;
        dip.is_ipv4 = TRUE;
        ret = pfe_rtable_entry_set_sip(rt_entry, &sip);
        if(ret != EOK)
        {
            return ret;
        }
        ret = pfe_rtable_entry_set_dip(rt_entry, &dip);
        if(ret != EOK)
        {
            return ret;
        }
    }
    pfe_rtable_entry_set_sport(rt_entry, entry->sport);
    pfe_rtable_entry_set_dport(rt_entry, entry->dport);
    pfe_rtable_entry_set_proto(rt_entry, entry->proto);
    /* Set the destination interface to UTIL to keep database correct */
    pfe_rtable_entry_set_dstif_id(rt_entry, PFE_PHY_IF_ID_UTIL);
    return ret;
}
#endif

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
*          If the rule can be accelerated by the HW the appropriate route entry is created too.
*/

errno_t pfe_spd_acc_add_rule(pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry)
{
    errno_t ret;
#if 0 /* todo AAVB-2539 */
    addr_t id;
    pfe_rtable_entry_t *rt_entry;
#endif
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if((NULL == phy_if) || (NULL == entry))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
#endif
#if 0 /* todo AAVB-2539 */
#if defined(PFE_CFG_RTABLE_ENABLE)
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely((NULL == rtable_ptr) || (NULL == pfe_spd_acc_id_pool)))
    {
        NXP_LOG_ERROR("Init function not called\n");
        return;
    }
#endif
    /* Check whether the acceleration is possible (5-tupple exists)
        - UDP or TCP protocol
        - ports are not opaque
    */
    if(((17 == entry->proto) || (6 == entry->proto)) &&
         (0U == (entry->flags & (SPD_FLAG_DPORT_OPAQUE | SPD_FLAG_DPORT_OPAQUE))))
    {   /* Acceleration is possible */
        /* Allocate and set an unique entry identifier */
        if(EOK == blalloc_alloc_offs(pfe_spd_acc_id_pool, 1U, 0, &id))
        {
            /* Prepare IP route entry to allow acceleration */
            rt_entry = pfe_rtable_entry_create();
            if(NULL != rt_entry)
            {
                /* Prepare IP entry */
                ret = pfe_spd_acc_convert_to_rt_entry(entry, rt_entry);
                if(EOK != ret)
                {   /* Conversion failure - revert changes */
                    pfe_rtable_entry_free(NULL, rt_entry);
                    blalloc_free_offs_size(pfe_spd_acc_id_pool, id, 1U);
                    /* We can still add entry as not accelerated here */
                }
                else
                {
                    pfe_rtable_entry_set_id5t(rt_entry, id);
                    ret = pfe_rtable_add_entry(rtable_ptr, rt_entry);
                    if(EOK == ret)
                    {

                        /* Write entry */
                        entry->id5t = oal_htonl((uint32_t)id);
                        entry->flags |= SPD_FLAG_5T;
                        ret = pfe_spd_add_rule(phy_if, position, entry);
                        if(EOK != ret)
                        {   /* Failed to set SPD entry */
                            /* Revert the changes */
                            pfe_rtable_del_entry(rtable_ptr, rt_entry);
                            pfe_rtable_entry_free(rtable_ptr, rt_entry);
                            blalloc_free_offs_size(pfe_spd_acc_id_pool, id, 1U);
                            /* No need to try again adding entry as not accelerated because
                               the same function would be called and fail as well */
                        }
                        else
                        {
                            NXP_LOG_DEBUG("SPD entry %u of interface %u is accelerated (%u)\n", position, pfe_phy_if_get_id(phy_if), id);
                        }
                        /* We are done, do not execute the code used to add not accelerated entry */
                        return ret;
                    }
                    else
                    {   /* Failed to add route entry */
                        /* Free the unused entry */
                        pfe_rtable_entry_free(rtable_ptr, rt_entry);
                        /* Free unique ID previously allocated */
                        blalloc_free_offs_size(pfe_spd_acc_id_pool, id, 1U);
                        /* We can still add entry as not accelerated here */
                    }
                }
            }
            else
            {   /* Failed to allocate route entry */
                /* Free unique ID previously allocated */
                blalloc_free_offs_size(pfe_spd_acc_id_pool, id, 1U);
                /* We can still add entry as not accelerated here */
            }
        } /* No else needed - nothing to do clean on failure. We can still add entry as not accelerated here */
    }

#endif
#endif
    /* If we got there then either
       - Acceleration is not possible (only 5-tuple exact match supported by HW)
       - Something failed in the code above but we still can try to add not accelerated entry
    */

    /* Ensure correct value is in id5t */
    entry->id5t = 0U;
    entry->flags &= ~SPD_FLAG_5T;
    /* Write entry as is */
    ret = pfe_spd_add_rule(phy_if, position, entry);
    return ret;
}

/**
* @brief Removes the rule at a given position
* @param[in] phy_if Physical interface which spd shall be updated
* @param[in] position Position of the rule to be removed
*/
errno_t pfe_spd_acc_remove_rule(pfe_phy_if_t * phy_if, uint16_t position)
{
    errno_t ret;
#if 0 /* todo AAVB-2539 */
    pfe_ct_spd_entry_t entry;
    pfe_rtable_entry_t *rt_entry;
#endif
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely(NULL == phy_if))
    {
        NXP_LOG_ERROR("NULL argument received\n");
        return EINVAL;
    }
#endif
#if 0 /* todo AAVB-2539 */
#if defined(PFE_CFG_RTABLE_ENABLE)
#if defined(PFE_CFG_NULL_ARG_CHECK)
    if(unlikely((NULL == rtable_ptr) || (NULL == pfe_spd_acc_id_pool)))
    {
        NXP_LOG_ERROR("Init function not called\n");
        return;
    }
#endif
    /* Get the rule at given position */
    ret = pfe_spd_get_rule(phy_if, position, &entry);
    if(EOK != ret)
    {
        return ret;
    }
    /* Check whether the entry is an accelerated one */
    if(0U != entry.id5t)
    {   /* Accelerated entry */
        /* Get the route entry */
        rt_entry = pfe_rtable_get_first(rtable_ptr, RTABLE_CRIT_BY_ID5T, (void *)(addr_t)entry.id5t);
        if(unlikely(NULL == rt_entry))
        {   /* This should not be possible */
            NXP_LOG_ERROR("No route entry for 5-tuple id %u found\n", entry.id5t);
            /* We probably leak some memory here however we can keep going on */
        }
        else
        {
            /* Remove IP route entry and destroy it */
            pfe_rtable_del_entry(rtable_ptr, rt_entry);
            pfe_rtable_entry_free(rtable_ptr, rt_entry);
        }
        /* Free the unique identifier */
        blalloc_free_offs_size(pfe_spd_acc_id_pool, entry.id5t, 1U);
    }
#endif
#endif
    /* In each case remove the entry */
    ret = pfe_spd_remove_rule(phy_if, position);
    return ret;
}


/**
* @brief Retrieves rule at given position
* @param[in] phy_if Physical interface which SPD shall be queried
* @param[in] position Position of the rule to be read
* @param[out] entry Retrieved rule, valid only if EOK is returned
*/
errno_t pfe_spd_acc_get_rule(const pfe_phy_if_t *phy_if, uint16_t position, pfe_ct_spd_entry_t *entry)
{
    return pfe_spd_get_rule(phy_if, position, entry);
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* PFE_CFG_FCI_ENABLE */
#endif /* PFE_CFG_PFE_MASTER */

