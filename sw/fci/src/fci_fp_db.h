/* =========================================================================
 *  Copyright 2019-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */
#ifndef FCI_FP_DB_H
#define FCI_FP_DB_H

#include "pfe_class.h"
/**
* @def PFE_FP_RULE_POSITION_LAST
* @brief Macro to define position in function pfe_fp_add_rule_to_table() as the last one
*/
#define FCI_FP_RULE_POSITION_LAST  (0xFFU + 1U)
/**
* @def PFE_FP_RULE_POSITION_FIRST
* @brief Macro to define position in function pfe_fp_add_rule_to_table() as the first one
*/
#define FCI_FP_RULE_POSITION_FIRST 0x0U

typedef struct fci_fp_table_tag fci_fp_table_t;

/**
 * @brief	Rule data details type
 */
typedef struct
{
	char *rule_name;
    uint32_t data;
    uint32_t mask;
    uint16_t offset;
    pfe_ct_fp_flags_t flags;
} fci_fp_rule_info_t;

/**
* @brief Criterion for table database search
*/
typedef enum
{
    FP_TABLE_CRIT_ALL,
    FP_TABLE_CRIT_NAME,
    FP_TABLE_CRIT_ADDRESS
} fci_fp_table_criterion_t;

/**
* @brief Argument (requested value) for table database
*/
typedef union
{
    char_t *name;
    uint32_t address;
} fci_fp_table_criterion_arg_t;

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

/* Initialize the module */
void fci_fp_db_init(void);
/* Create set of rules */
errno_t fci_fp_db_create_rule(char_t *name, uint32_t data, uint32_t mask, uint16_t offset, pfe_ct_fp_flags_t flags, char_t *next_rule);
/* Create set of tables */
errno_t fci_fp_db_create_table(char_t *name);
/* Group rules into tables */
errno_t fci_fp_db_add_rule_to_table(char_t *table_name, char_t *rule_name, uint16_t position);
/* Write tables into DMEM */
errno_t fci_fp_db_push_table_to_hw(pfe_class_t *class, char_t *table_name);
/* Get the DMEM address to allow table usage */
uint32_t fci_fp_db_get_table_dmem_addr(char_t *table_name);

/* Remove table from DMEM */
errno_t fci_fp_db_pop_table_from_hw(char_t *table_name);
/* Remove a single rule from the table */
errno_t fci_fp_db_remove_rule_from_table(char_t *rule_name);
/* Remove all rules from the table (and destroy the table) */
errno_t fci_fp_db_destroy_table(char_t *name, bool_t force);
/* Destroy the rule */
errno_t fci_fp_db_destroy_rule(char_t *name);

/* DB query functions */
fci_fp_table_t *fci_fp_db_get_first(fci_fp_table_criterion_t crit, void *arg);

/* Get the table from address */
errno_t fci_fp_db_get_table_from_addr(uint32_t addr, char_t **table_name);

#if !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS)
/* Printing the information */
uint32_t fci_fp_print_tables(char_t *buf, uint32_t buf_len, uint8_t verb_level);
#endif /* !defined(PFE_CFG_TARGET_OS_AUTOSAR) || defined(PFE_CFG_TEXT_STATS) */

/* FCI queries */
errno_t fci_fp_db_get_first_rule(fci_fp_rule_info_t *rule_info, char_t **next_rule);
errno_t fci_fp_db_get_next_rule(fci_fp_rule_info_t *rule_info, char_t **next_rule);
errno_t fci_fp_db_get_table_first_rule(char_t *table_name, fci_fp_rule_info_t *rule_info, char_t **next_rule);
errno_t fci_fp_db_get_table_next_rule(char_t *table_name, fci_fp_rule_info_t *rule_info, char_t **next_rule);

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif

