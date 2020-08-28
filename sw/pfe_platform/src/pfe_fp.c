/* =========================================================================
 *  Copyright 2019-2020 NXP
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
#include "pfe_cfg.h"
#include "oal.h"
#include "pfe_class.h"
#include "pfe_fp.h"

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
uint32_t pfe_fp_create_table(pfe_class_t *class, uint8_t rules_count)
{
    addr_t addr;
    uint32_t size;
    pfe_ct_fp_table_t temp;
    errno_t res;

    /* Calculate needed size */
    size = sizeof(pfe_ct_fp_table_t) + (rules_count * sizeof(pfe_ct_fp_rule_t));
    /* Allocate DMEM */
    addr = pfe_class_dmem_heap_alloc(class, size);
    if(0U == addr)
    {
        NXP_LOG_ERROR("Not enough DMEM memory\n");
        return 0U;
    }
    /* Write the table header */
    temp.count = rules_count;
    temp.rules = oal_htonl(addr + sizeof(pfe_ct_fp_table_t));
    res = pfe_class_write_dmem(class, -1, (void *)addr, &temp, sizeof(pfe_ct_fp_table_t));
    if(EOK != res)
    {
        NXP_LOG_ERROR("Cannot write to DMEM\n");
        pfe_class_dmem_heap_free(class, addr);
        addr = 0U;
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
uint32_t pfe_fp_table_write_rule(pfe_class_t *class, uint32_t table_address, pfe_ct_fp_rule_t *rule, uint8_t position)
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
    res = pfe_class_write_dmem(class, -1, (void *)addr, &temp, sizeof(pfe_ct_fp_rule_t));
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
void pfe_fp_destroy_table(pfe_class_t *class, uint32_t table_address)
{
    /* Just free the memory */
    pfe_class_dmem_heap_free(class, table_address);
}
