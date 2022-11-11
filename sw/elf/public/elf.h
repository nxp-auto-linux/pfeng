/* =========================================================================
 *  Copyright 2018-2022 NXP
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 * ========================================================================= */

/**
 * @defgroup	dxgr_ELF ELF Parser
 * @brief		The ELF parser
 * @details
 *
 * @addtogroup dxgr_ELF
 * @{
 *
 * @file			elf.h
 * @version			0.0.0.0
 *
 * @brief			Header file for the ELF module.
 *
 */
/*==================================================================================================
==================================================================================================*/

/*==================================================================================================
                                         MISRA VIOLATIONS
==================================================================================================*/

#ifndef ELF_H
    #define ELF_H

/*==================================================================================================
                                         INCLUDE FILES
 1) system and project includes
 2) needed interfaces from external units
 3) internal and external interfaces from this unit
==================================================================================================*/
#include <uapi/linux/elf.h>
#include "oal.h"

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
#define ELF_NAMED_SECT_IDX_FLAG     0x80000000U

/* Macros for change of endianness */
#define ENDIAN_SW_2B(VAL) ( (((VAL)&0xFF00U)>>8U) | (((VAL)&0x00FFU)<<8U) )
#define ENDIAN_SW_4B(VAL) ( (((VAL)&0xFF000000U)>>24U) | (((VAL)&0x000000FFU)<<24U) \
                          | (((VAL)&0x00FF0000U)>>8U) | (((VAL)&0x0000FF00U)<<8U) \
                          )
#define ENDIAN_SW_8B(VAL) ( (((VAL)&0xFF00000000000000U)>>56U) | (((VAL)&0x00000000000000FFU)<<56U) \
                          | (((VAL)&0x00FF000000000000U)>>40U) | (((VAL)&0x000000000000FF00U)<<40U) \
                          | (((VAL)&0x0000FF0000000000U)>>24U) | (((VAL)&0x0000000000FF0000U)<<24U) \
                          | (((VAL)&0x000000FF00000000U)>>8U ) | (((VAL)&0x00000000FF000000U)<<8U ) \
                          )
/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef struct __attribute__((packed))
{
    union
    {
        Elf64_Ehdr r64;
        Elf32_Ehdr r32;
        uint8_t    e_ident[EI_NIDENT]; /* Direct access, same for both 64 and 32 */
    }          Header;
    Elf64_Phdr *arProgHead64;
    Elf64_Shdr *arSectHead64;
    Elf32_Phdr *arProgHead32;
    Elf32_Shdr *arSectHead32;
    int8_t     *acSectNames;
    uint32_t   u32ProgScanIdx;
    bool_t     bIs64Bit;
    const void __attribute__((aligned(4))) *pvData; /* Raw file */
} ELF_File_t;

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_START_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

extern bool_t ELF_Open(ELF_File_t *pElfFile, const void *pvFile);
extern void ELF_Close(ELF_File_t *pElfFile);

#if TRUE == ELF_CFG_PROGRAM_TABLE_USED
    extern bool_t ELF_ProgSectFindNext( ELF_File_t *pElfFile, uint32_t *pu32ProgIdx,
                                         uint64_t *pu64LoadVAddr, uint64_t *pu64LoadPAddr, uint64_t *pu64Length
                                       );
    extern bool_t ELF_ProgSectLoad( const ELF_File_t *pElfFile,
                                     uint32_t u32ProgIdx, addr_t AccessAddr, addr_t AllocSize
                                   );
#endif

#if TRUE == ELF_CFG_SECTION_TABLE_USED
    extern bool_t ELF_SectFindName( const ELF_File_t *pElfFile, const char_t *szSectionName,
                                     uint32_t *pu32SectIdx, uint64_t *pu64LoadAddr, uint64_t *pu64Length
                                   );
    extern bool_t ELF_SectLoad( const ELF_File_t *pElfFile,
                                 uint32_t u32SectIdx, addr_t AccessAddr, addr_t AllocSize
                               );
#endif

#if TRUE == ELF_CFG_SECTION_PRINT_ENABLED
    extern void ELF_PrintSections(const ELF_File_t *pElfFile);
#endif

/*==================================================================================================
                                    GLOBAL INLINE FUNCTIONS
==================================================================================================*/
/**
* @brief        Provides entry point memory address.
* @param[in]    pElfFile Structure holding all informations about opened ELF file.
* @return       The entry point address
*/
static inline uint64_t ELF_GetEntryPoint(const ELF_File_t *pElfFile)
{
    uint64_t u64Addr;
    if(TRUE == pElfFile->bIs64Bit)
    {
        u64Addr = pElfFile->Header.r64.e_entry;
    }
    else
    {
        u64Addr = pElfFile->Header.r32.e_entry;
    }
    return u64Addr;
}

/**
* @brief        Makes function ELF_ProgSectFindNext search again from beginning.
* @details      It is not needed to call this function after the ELF is opened.
* @param[out]   pElfFile Structure holding all informations about opened ELF file.
*/
static inline void ELF_ProgSectSearchReset(ELF_File_t *pElfFile)
{
    pElfFile->u32ProgScanIdx = 0U;
}

/**
* @brief        Use to get ELF format if needed.
* @param[in]    pElfFile Structure holding all informations about (partially) opened ELF file.
* @retval       TRUE It is 64bit ELF
* @retval       FALSE It is 32bit ELF
*/
static inline bool_t ELF_Is64bit(const ELF_File_t *pElfFile)
{
    return (2U == pElfFile->Header.e_ident[EI_CLASS]) ? TRUE : FALSE;
}
/**
* @brief        Use to get ELF format if needed.
* @param[in]    pElfFile Structure holding all informations about (partially) opened ELF file.
* @retval       TRUE It is 32bit ELF
* @retval       FALSE It is 64bit ELF
*/
static inline bool_t ELF_Is32bit(const ELF_File_t *pElfFile)
{
    return (1U == pElfFile->Header.e_ident[EI_CLASS]) ? TRUE : FALSE;
}
/**
* @brief        Use to get ELF endianness if needed.
* @param[in]    pElfFile Structure holding all informations about (partially) opened ELF file.
* @retval       TRUE It is BIG endian ELF
* @retval       FALSE It is LITTLE endian ELF
*/
static inline bool_t ELF_IsBigEndian(const ELF_File_t *pElfFile)
{
    return (2U == pElfFile->Header.e_ident[EI_DATA]) ? TRUE : FALSE;
}
/**
* @brief        Use to get ELF endianness if needed.
* @param[in]    pElfFile Structure holding all informations about (partially) opened ELF file.
* @retval       TRUE It is LITTLE endian ELF
* @retval       FALSE It is BIG endian ELF
*/
static inline bool_t ELF_IsLittleEndian(const ELF_File_t *pElfFile)
{
    return (1U == pElfFile->Header.e_ident[EI_DATA]) ? TRUE : FALSE;
}
#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* ELF_H */

/** @}*/
