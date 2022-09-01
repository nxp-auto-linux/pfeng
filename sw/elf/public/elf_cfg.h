/* =========================================================================
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @addtogroup dxgr_ELF
 * @{
 * 
 * @file			elf_cfg.h
 * @version			0.0.0.0
 *
 * @brief			Configuration header file for the ELF module.
 *
 */
/*==================================================================================================
==================================================================================================*/

/*==================================================================================================
                                         MISRA VIOLATIONS
==================================================================================================*/

#ifndef ELF_CFG_H
    #define ELF_CFG_H

#ifndef TRUE
    #define TRUE 1
#endif /* TRUE */
#ifndef FALSE
    #define FALSE 0
#endif /* FALSE */

/*==================================================================================================
                                         INCLUDE FILES
 1) system and project includes
 2) needed interfaces from external units
 3) internal and external interfaces from this unit
==================================================================================================*/  

/*==================================================================================================
                               SOURCE FILE VERSION INFORMATION
==================================================================================================*/

/*==================================================================================================
                                      FILE VERSION CHECKS
==================================================================================================*/

/*==================================================================================================
                                           CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                       DEFINES AND MACROS
==================================================================================================*/
/**
* @def      ELF_CFG_PROGRAM_TABLE_USED
* @brief    Configures whether functions for loading binary from program table will be built.
* @details  This is the standard way of loading ELFs.
*/
#define ELF_CFG_PROGRAM_TABLE_USED TRUE

/**
* @def      ELF_CFG_SECTION_TABLE_USED
* @brief    Configures whether functions for loading binary from section table will be built.
* @details  This is a non-standard way of loading ELFs.
*/
#define ELF_CFG_SECTION_TABLE_USED TRUE

/**
* @def      ELF_CFG_ELF32_SUPPORTED
* @brief    Configures whether 32-bit ELF format support shall be built.
*/
#define ELF_CFG_ELF32_SUPPORTED TRUE

/**
* @def      ELF_CFG_ELF64_SUPPORTED
* @brief    Configures whether 64-bit ELF format support shall be built.
*/
#ifndef ELF_CFG_ELF64_SUPPORTED
#define ELF_CFG_ELF64_SUPPORTED FALSE
#endif

/**
* @def      ELF_CFG_SECTION_PRINT_ENABLED
* @brief    Configures whether function ELF_PrintSections shall be built.
* @details  Normally it will be FALSE.
*/
#define ELF_CFG_SECTION_PRINT_ENABLED TRUE

/*==================================================================================================
                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

#endif /* ELF_CFG_H */

/** @}*/
