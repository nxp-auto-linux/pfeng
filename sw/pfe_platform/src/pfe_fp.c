/* =========================================================================
 *  
 *  Copyright (c) 2019 Imagination Technologies Limited
 *  Copyright (c) 2020-2021 Imagination Technologies Limited
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_class.h"
#include "pfe_fp.h"

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/**
* @brief Initializes the module
*/
/* Magic function - QNX runtime linker fails to find the rest of the functions if this
   one is not called from somewhere in the pfe_platform */
void pfe_fp_init(void)
{
    ;
}

/**
* @brief Creates the flexible parser table
* @param[in] class Classifier to create the table
* @param[in] rules_count Number of rules in the table
* @details Allocates DMEM memory for the whole table including the rules and prepares the
*          table header. Rules must be written separately by the pfe_fp_table_write_rule()
*          function. The table is reference by the returned DMEM address.
* @return 0 on failure or the DMEM address of the table.
*/
uint32_t pfe_fp_create_table(pfe_class_t *class, uint16_t rules_count)
{
    addr_t addr;
    uint32_t size;
    pfe_ct_fp_table_t temp;
    errno_t res;

    /* Calculate needed size */
    size = (uint32_t)sizeof (pfe_ct_fp_table_t) + ((uint32_t)rules_count * sizeof (pfe_ct_fp_rule_t));
    /* Allocate DMEM */
    addr = pfe_class_dmem_heap_alloc (class, size);
    if (0U == addr)
    {
        NXP_LOG_ERROR("Not enough DMEM memory\n");
    }
    else
    {
        /* Write the table header */
        temp.count = rules_count;
        temp.rules = oal_htonl (addr + sizeof (pfe_ct_fp_table_t));
        (void)memset (&temp.fp_stats, 0, sizeof (pfe_ct_class_flexi_parser_stats_t));
        res = pfe_class_write_dmem (class, -1, addr, (void *)&temp, sizeof (pfe_ct_fp_table_t));
        if (EOK != res)
        {
            NXP_LOG_ERROR("Cannot write to DMEM\n");
            pfe_class_dmem_heap_free (class, addr);
            addr = 0U;
        }
    }
    /* Return the DMEM address */
    return addr;
}

/**
* @brief Writes a rule into the flexible parser table
* @param[in] class Classifier used to create the table
* @param[in] table_address Address of the table returned by the pfe_fp_create_table()
* @param[in] rule Rule to be written into the table
* @param[in] position Position of the rule in the table. Must be less than rules_count passed into pfe_fp_create_table().
* @details Function writes the rule at specified position in the previously created table.
* @return 0 on failure otherwise the DMEM address of the rule.
*/
uint32_t pfe_fp_table_write_rule(pfe_class_t *class, uint32_t table_address, const pfe_ct_fp_rule_t *rule, uint16_t position)
{
    pfe_ct_fp_rule_t temp;
    addr_t addr;
    errno_t res;

    /* Fill in a temporary structure - handle Endians */
    temp.data = rule->data;
    temp.mask = rule->mask;
    temp.offset = rule->offset;
    temp.next_idx = rule->next_idx;
    temp.flags = rule->flags;
    /* Calculate position in the DMEM */
    addr = table_address + sizeof(pfe_ct_fp_table_t) + (position * sizeof(pfe_ct_fp_rule_t));
    /* Write into the DMEM */
    res = pfe_class_write_dmem(class, -1, addr, (void *)&temp, sizeof(pfe_ct_fp_rule_t));
    if(EOK != res)
    {
        NXP_LOG_ERROR("Cannot write to DMEM\n");
        addr = 0U;
    }
    return addr;
}

/**
* @brief Destroys the flexible parser table
* @param[in] class Classifier used to create the table
* @param[in] table_address Address returned by the pfe_fp_create_table()
*/
void pfe_fp_destroy_table(const pfe_class_t *class, uint32_t table_address)
{
    /* Just free the memory */
    pfe_class_dmem_heap_free(class, table_address);
}

errno_t pfe_fp_table_get_statistics(pfe_class_t *class, uint32_t pe_idx, uint32_t table_address, pfe_ct_class_flexi_parser_stats_t *stats)
{
    pfe_ct_fp_table_t temp;
    errno_t res;
    addr_t addr;

    (void)memset(&temp.fp_stats, 0, sizeof(pfe_ct_class_flexi_parser_stats_t));
    addr = table_address;
    res = pfe_class_read_dmem(class, (int32_t)pe_idx, (void *)&temp, addr, sizeof(pfe_ct_fp_table_t));
    if(EOK != res)
    {
        NXP_LOG_ERROR("Cannot read from DMEM\n");
    }
    else
    {
        (void)memcpy(stats, &temp.fp_stats, sizeof(pfe_ct_class_flexi_parser_stats_t));
    }

    return res;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

