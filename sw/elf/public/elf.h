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

/**
* @page misra_violations MISRA-C:2004 violations
*
* @section elf_h_REF_1
* Violates MISRA 2004 TODO Rule TODO,
*
*
*/

#ifndef ELF_H
    #define ELF_H

/*==================================================================================================
                                         INCLUDE FILES
 1) system and project includes
 2) needed interfaces from external units
 3) internal and external interfaces from this unit
==================================================================================================*/
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
#define ELF_NIDENT                  16U
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
                                             ENUMS
==================================================================================================*/
enum
{
    EI_MAG0         = 0, /* 0x7F */
    EI_MAG1         = 1, /* 'E' */
    EI_MAG2         = 2, /* 'L' */
    EI_MAG3         = 3, /* 'F' */
    EI_CLASS        = 4, /* Architecture 32-bit Architecture or 64-bit Architecture */
    EI_DATA         = 5, /* Byte Order */
    EI_VERSION      = 6, /* ELF Version */
    EI_OSABI        = 7, /* OS Specific */
    EI_ABIVERSION   = 8, /* OS Specific */
    EI_PAD          = 9  /* Padding */
};

/* any section that is of type SHT_NOBITS and has the attribute SHF_ALLOC should be allocated */
enum
{
    SHT_NULL      = 0U,   /* Null section */
    SHT_PROGBITS  = 1U,   /* Program information */
    SHT_SYMTAB    = 2U,   /* Symbol table */
    SHT_STRTAB    = 3U,   /* String table */
    SHT_RELA      = 4U,   /* Relocation with addend*/
    SHT_NOBITS    = 8U,   /* Not present in file */
    SHT_REL       = 9U,   /* Relocation (no addend) */
};

enum
{
    SHF_WRITE = 0x1, /* Writable */
    SHF_ALLOC = 0x2, /* Occupies memory during execution */
    SHF_EXECINSTR = 0x4, /* Executable */
    SHF_MERGE = 0x10, /* Might be merged */
    SHF_STRINGS = 0x20, /* Contains nul-terminated strings */
    SHF_INFO_LINK = 0x40, /* 'sh info' contains SHT index */
    SHF_LINK_ORDER = 0x80, /* Preserve order after combining */
    SHF_OS_NONCONFORMING = 0x100, /* Non-standard OS specific handling required */
    SHF_GROUP = 0x200, /* Section is member of a group */
    SHF_TLS = 0x400, /* Section hold thread-local data */
    SHF_MASKOS = 0x0ff00000, /* OS-specific */
    SHF_MASKPROC = (int32_t)0xf000000, /* Processor-specific *//* Cast to avoid warning on some compilers */
    SHF_ORDERED = 0x4000000, /* Special ordering requirement (Solaris) */
    SHF_EXCLUDE = 0x8000000, /* Section is excluded unless referenced or allocated (Solaris) */
};

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum
{
    ELF_Arch_None    = 0x00u,
    ELF_Arch_SPARC   = 0x02u,
    ELF_Arch_x86     = 0x03u,
    ELF_Arch_MIPS    = 0x08u,
    ELF_Arch_PowerPC = 0x14u,
    ELF_Arch_ARM     = 0x28u,
    ELF_Arch_SuperH  = 0x2Au,
    ELF_Arch_IA_64   = 0x32u,
    ELF_Arch_x86_64  = 0x3Eu,
    ELF_Arch_AArch64 = 0xB7u,
    ELF_Arch_eXcess  = 0x6Fu,
} ELF_Arch_t;

typedef uint32_t Elf32_Off;     /* Unsigned offset */
typedef uint32_t Elf32_Addr;    /* Unsigned address */
typedef uint64_t Elf64_Off;     /* Unsigned offset */
typedef uint64_t Elf64_Addr;    /* Unsigned address */

typedef struct __attribute__((packed))
{
    uint8_t     e_ident[ELF_NIDENT];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    Elf32_Addr  e_entry;
    Elf32_Off   e_phoff;
    Elf32_Off   e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
} Elf32_Ehdr;
typedef struct __attribute__((packed))
{
    uint8_t     e_ident[ELF_NIDENT];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    Elf64_Addr  e_entry;
    Elf64_Off   e_phoff;
    Elf64_Off   e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
} Elf64_Ehdr;

typedef struct __attribute__((packed))
{
    uint32_t   p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    uint32_t   p_filesz;
    uint32_t   p_memsz;
    uint32_t   p_flags;
    uint32_t   p_align;
} Elf32_Phdr;
typedef struct __attribute__((packed))
{
    uint32_t   p_type;
    uint32_t   p_flags;
    Elf64_Off  p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    uint64_t   p_filesz;
    uint64_t   p_memsz;
    uint64_t   p_align;
} Elf64_Phdr;

typedef struct __attribute__((packed))
{
    uint32_t   sh_name;
    uint32_t   sh_type;
    uint32_t   sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    uint32_t   sh_size;
    uint32_t   sh_link;
    uint32_t   sh_info;
    uint32_t   sh_addralign;
    uint32_t   sh_entsize;
} Elf32_Shdr;
typedef struct __attribute__((packed))
{
    uint32_t   sh_name;
    uint32_t   sh_type;
    uint64_t   sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off  sh_offset;
    uint64_t   sh_size;
    uint32_t   sh_link;
    uint32_t   sh_info;
    uint64_t   sh_addralign;
    uint64_t   sh_entsize;
} Elf64_Shdr;

typedef struct __attribute__((packed))
{
    union
    {
        Elf64_Ehdr r64;
        Elf32_Ehdr r32;
        uint8_t    e_ident[ELF_NIDENT]; /* Direct access, same for both 64 and 32 */
    }          Header;
    Elf64_Phdr *arProgHead64;
    Elf64_Shdr *arSectHead64;
    Elf32_Phdr *arProgHead32;
    Elf32_Shdr *arSectHead32;
    int8_t     *acSectNames;
    uint32_t   u32ProgScanIdx;
    uint32_t   u32FileSize;
    bool_t     bIs64Bit;
    void __attribute__((aligned(4))) *pvData; /* Raw file */
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

extern bool_t ELF_Open(ELF_File_t *pElfFile,void *pvFile, uint32_t u32FileSize);
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
/**
* @brief        Use to check target architecture of the ELF.
* @param[in]    pElfFile Structure holding all informations about opened ELF file.
* @param[in]    eArch Expected architecture specification.
* @retval       TRUE ELF architecture matches given value.
* @retval       FALSE ELF targets different architecture.
*/
static inline bool_t ELF_IsArchitecture(const ELF_File_t *pElfFile, ELF_Arch_t eArch)
{
    bool_t bRetVal;
    if(TRUE == pElfFile->bIs64Bit)
    {
        bRetVal = ((uint16_t)eArch == pElfFile->Header.r64.e_machine) ? TRUE : FALSE;
    }
    else
    {
        bRetVal = ((uint16_t)eArch == pElfFile->Header.r32.e_machine) ? TRUE : FALSE;
    }
    return bRetVal;
}

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
#define ETH_43_PFE_STOP_SEC_CODE
#include "Eth_43_PFE_MemMap.h"
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* ELF_H */

/** @}*/
