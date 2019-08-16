/* =========================================================================
 *  Copyright 2018 NXP
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

/**
* @page misra_violations MISRA-C:2004 violations
*
* @section elg_cfg_h_REF_1
* Violates MISRA 2004 TODO Rule TODO, 
* 
*
*/

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
#define ELF_CFG_ELF64_SUPPORTED TRUE

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
