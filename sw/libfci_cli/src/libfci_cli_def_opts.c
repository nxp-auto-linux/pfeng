/* =========================================================================
 *  Copyright 2020-2022 NXP
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

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "libfci_cli_common.h"
#include "libfci_cli_def_opts.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
/* empty */
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

/* shortopt - mapping defined items into optstring */
static const char txt_shortopts[] = 
{  
    ':',  /* special flag: if set, then 'getopt()' fnc family will return ':' if opt argument is missing */
    
    /* OPT_00_NO_OPTION does not have a shortopt version */
    
#ifdef OPT_01_ENUM_NAME
  #if (OPT_01_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_01_CLI_SHORT_CODE_CHR,
    #if (OPT_01_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_02_ENUM_NAME
  #if (OPT_02_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_02_CLI_SHORT_CODE_CHR,
    #if (OPT_02_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_03_ENUM_NAME
  #if (OPT_03_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_03_CLI_SHORT_CODE_CHR,
    #if (OPT_03_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_04_ENUM_NAME
  #if (OPT_04_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_04_CLI_SHORT_CODE_CHR,
    #if (OPT_04_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_05_ENUM_NAME
  #if (OPT_05_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_05_CLI_SHORT_CODE_CHR,
    #if (OPT_05_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_06_ENUM_NAME
  #if (OPT_06_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_06_CLI_SHORT_CODE_CHR,
    #if (OPT_06_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_07_ENUM_NAME
  #if (OPT_07_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_07_CLI_SHORT_CODE_CHR,
    #if (OPT_07_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_08_ENUM_NAME
  #if (OPT_08_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_08_CLI_SHORT_CODE_CHR,
    #if (OPT_08_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_09_ENUM_NAME
  #if (OPT_09_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_09_CLI_SHORT_CODE_CHR,
    #if (OPT_09_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_10_ENUM_NAME
  #if (OPT_10_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_10_CLI_SHORT_CODE_CHR,
    #if (OPT_10_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_11_ENUM_NAME
  #if (OPT_11_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_11_CLI_SHORT_CODE_CHR,
    #if (OPT_11_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_12_ENUM_NAME
  #if (OPT_12_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_12_CLI_SHORT_CODE_CHR,
    #if (OPT_12_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_13_ENUM_NAME
  #if (OPT_13_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_13_CLI_SHORT_CODE_CHR,
    #if (OPT_13_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_14_ENUM_NAME
  #if (OPT_14_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_14_CLI_SHORT_CODE_CHR,
    #if (OPT_14_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_15_ENUM_NAME
  #if (OPT_15_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_15_CLI_SHORT_CODE_CHR,
    #if (OPT_15_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_16_ENUM_NAME
  #if (OPT_16_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_16_CLI_SHORT_CODE_CHR,
    #if (OPT_16_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_17_ENUM_NAME
  #if (OPT_17_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_17_CLI_SHORT_CODE_CHR,
    #if (OPT_17_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_18_ENUM_NAME
  #if (OPT_18_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_18_CLI_SHORT_CODE_CHR,
    #if (OPT_18_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_19_ENUM_NAME
  #if (OPT_19_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_19_CLI_SHORT_CODE_CHR,
    #if (OPT_19_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_20_ENUM_NAME
  #if (OPT_20_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_20_CLI_SHORT_CODE_CHR,
    #if (OPT_20_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_21_ENUM_NAME
  #if (OPT_21_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_21_CLI_SHORT_CODE_CHR,
    #if (OPT_21_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_22_ENUM_NAME
  #if (OPT_22_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_22_CLI_SHORT_CODE_CHR,
    #if (OPT_22_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_23_ENUM_NAME
  #if (OPT_23_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_23_CLI_SHORT_CODE_CHR,
    #if (OPT_23_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_24_ENUM_NAME
  #if (OPT_24_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_24_CLI_SHORT_CODE_CHR,
    #if (OPT_24_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_25_ENUM_NAME
  #if (OPT_25_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_25_CLI_SHORT_CODE_CHR,
    #if (OPT_25_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_26_ENUM_NAME
  #if (OPT_26_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_26_CLI_SHORT_CODE_CHR,
    #if (OPT_26_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_27_ENUM_NAME
  #if (OPT_27_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_27_CLI_SHORT_CODE_CHR,
    #if (OPT_27_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_28_ENUM_NAME
  #if (OPT_28_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_28_CLI_SHORT_CODE_CHR,
    #if (OPT_28_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_29_ENUM_NAME
  #if (OPT_29_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_29_CLI_SHORT_CODE_CHR,
    #if (OPT_29_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_30_ENUM_NAME
  #if (OPT_30_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_30_CLI_SHORT_CODE_CHR,
    #if (OPT_30_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_31_ENUM_NAME
  #if (OPT_31_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_31_CLI_SHORT_CODE_CHR,
    #if (OPT_31_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_32_ENUM_NAME
  #if (OPT_32_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_32_CLI_SHORT_CODE_CHR,
    #if (OPT_32_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_33_ENUM_NAME
  #if (OPT_33_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_33_CLI_SHORT_CODE_CHR,
    #if (OPT_33_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_34_ENUM_NAME
  #if (OPT_34_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_34_CLI_SHORT_CODE_CHR,
    #if (OPT_34_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_35_ENUM_NAME
  #if (OPT_35_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_35_CLI_SHORT_CODE_CHR,
    #if (OPT_35_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_36_ENUM_NAME
  #if (OPT_36_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_36_CLI_SHORT_CODE_CHR,
    #if (OPT_36_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_37_ENUM_NAME
  #if (OPT_37_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_37_CLI_SHORT_CODE_CHR,
    #if (OPT_37_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_38_ENUM_NAME
  #if (OPT_38_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_38_CLI_SHORT_CODE_CHR,
    #if (OPT_38_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_39_ENUM_NAME
  #if (OPT_39_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_39_CLI_SHORT_CODE_CHR,
    #if (OPT_39_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_40_ENUM_NAME
  #if (OPT_40_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_40_CLI_SHORT_CODE_CHR,
    #if (OPT_40_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_41_ENUM_NAME
  #if (OPT_41_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_41_CLI_SHORT_CODE_CHR,
    #if (OPT_41_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_42_ENUM_NAME
  #if (OPT_42_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_42_CLI_SHORT_CODE_CHR,
    #if (OPT_42_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_43_ENUM_NAME
  #if (OPT_43_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_43_CLI_SHORT_CODE_CHR,
    #if (OPT_43_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_44_ENUM_NAME
  #if (OPT_44_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_44_CLI_SHORT_CODE_CHR,
    #if (OPT_44_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_45_ENUM_NAME
  #if (OPT_45_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_45_CLI_SHORT_CODE_CHR,
    #if (OPT_45_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_46_ENUM_NAME
  #if (OPT_46_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_46_CLI_SHORT_CODE_CHR,
    #if (OPT_46_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_47_ENUM_NAME
  #if (OPT_47_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_47_CLI_SHORT_CODE_CHR,
    #if (OPT_47_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_48_ENUM_NAME
  #if (OPT_48_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_48_CLI_SHORT_CODE_CHR,
    #if (OPT_48_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_49_ENUM_NAME
  #if (OPT_49_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_49_CLI_SHORT_CODE_CHR,
    #if (OPT_49_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_50_ENUM_NAME
  #if (OPT_50_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_50_CLI_SHORT_CODE_CHR,
    #if (OPT_50_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_51_ENUM_NAME
  #if (OPT_51_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_51_CLI_SHORT_CODE_CHR,
    #if (OPT_51_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_52_ENUM_NAME
  #if (OPT_52_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_52_CLI_SHORT_CODE_CHR,
    #if (OPT_52_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_53_ENUM_NAME
  #if (OPT_53_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_53_CLI_SHORT_CODE_CHR,
    #if (OPT_53_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_54_ENUM_NAME
  #if (OPT_54_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_54_CLI_SHORT_CODE_CHR,
    #if (OPT_54_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_55_ENUM_NAME
  #if (OPT_55_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_55_CLI_SHORT_CODE_CHR,
    #if (OPT_55_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_56_ENUM_NAME
  #if (OPT_56_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_56_CLI_SHORT_CODE_CHR,
    #if (OPT_56_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_57_ENUM_NAME
  #if (OPT_57_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_57_CLI_SHORT_CODE_CHR,
    #if (OPT_57_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_58_ENUM_NAME
  #if (OPT_58_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_58_CLI_SHORT_CODE_CHR,
    #if (OPT_58_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_59_ENUM_NAME
  #if (OPT_59_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_59_CLI_SHORT_CODE_CHR,
    #if (OPT_59_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_60_ENUM_NAME
  #if (OPT_60_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_60_CLI_SHORT_CODE_CHR,
    #if (OPT_60_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_61_ENUM_NAME
  #if (OPT_61_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_61_CLI_SHORT_CODE_CHR,
    #if (OPT_61_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_62_ENUM_NAME
  #if (OPT_62_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_62_CLI_SHORT_CODE_CHR,
    #if (OPT_62_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_63_ENUM_NAME
  #if (OPT_63_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_63_CLI_SHORT_CODE_CHR,
    #if (OPT_63_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_64_ENUM_NAME
  #if (OPT_64_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_64_CLI_SHORT_CODE_CHR,
    #if (OPT_64_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_65_ENUM_NAME
  #if (OPT_65_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_65_CLI_SHORT_CODE_CHR,
    #if (OPT_65_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_66_ENUM_NAME
  #if (OPT_66_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_66_CLI_SHORT_CODE_CHR,
    #if (OPT_66_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_67_ENUM_NAME
  #if (OPT_67_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_67_CLI_SHORT_CODE_CHR,
    #if (OPT_67_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_68_ENUM_NAME
  #if (OPT_68_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_68_CLI_SHORT_CODE_CHR,
    #if (OPT_68_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_69_ENUM_NAME
  #if (OPT_69_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_69_CLI_SHORT_CODE_CHR,
    #if (OPT_69_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_70_ENUM_NAME
  #if (OPT_70_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_70_CLI_SHORT_CODE_CHR,
    #if (OPT_70_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_71_ENUM_NAME
  #if (OPT_71_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_71_CLI_SHORT_CODE_CHR,
    #if (OPT_71_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_72_ENUM_NAME
  #if (OPT_72_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_72_CLI_SHORT_CODE_CHR,
    #if (OPT_72_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_73_ENUM_NAME
  #if (OPT_73_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_73_CLI_SHORT_CODE_CHR,
    #if (OPT_73_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_74_ENUM_NAME
  #if (OPT_74_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_74_CLI_SHORT_CODE_CHR,
    #if (OPT_74_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_75_ENUM_NAME
  #if (OPT_75_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_75_CLI_SHORT_CODE_CHR,
    #if (OPT_75_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_76_ENUM_NAME
  #if (OPT_76_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_76_CLI_SHORT_CODE_CHR,
    #if (OPT_76_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_77_ENUM_NAME
  #if (OPT_77_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_77_CLI_SHORT_CODE_CHR,
    #if (OPT_77_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_78_ENUM_NAME
  #if (OPT_78_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_78_CLI_SHORT_CODE_CHR,
    #if (OPT_78_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_79_ENUM_NAME
  #if (OPT_79_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_79_CLI_SHORT_CODE_CHR,
    #if (OPT_79_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_80_ENUM_NAME
  #if (OPT_80_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_80_CLI_SHORT_CODE_CHR,
    #if (OPT_80_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_81_ENUM_NAME
  #if (OPT_81_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_81_CLI_SHORT_CODE_CHR,
    #if (OPT_81_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_82_ENUM_NAME
  #if (OPT_82_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_82_CLI_SHORT_CODE_CHR,
    #if (OPT_82_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_83_ENUM_NAME
  #if (OPT_83_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_83_CLI_SHORT_CODE_CHR,
    #if (OPT_83_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_84_ENUM_NAME
  #if (OPT_84_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_84_CLI_SHORT_CODE_CHR,
    #if (OPT_84_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_85_ENUM_NAME
  #if (OPT_85_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_85_CLI_SHORT_CODE_CHR,
    #if (OPT_85_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_86_ENUM_NAME
  #if (OPT_86_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_86_CLI_SHORT_CODE_CHR,
    #if (OPT_86_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_87_ENUM_NAME
  #if (OPT_87_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_87_CLI_SHORT_CODE_CHR,
    #if (OPT_87_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_88_ENUM_NAME
  #if (OPT_88_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_88_CLI_SHORT_CODE_CHR,
    #if (OPT_88_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_89_ENUM_NAME
  #if (OPT_89_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_89_CLI_SHORT_CODE_CHR,
    #if (OPT_89_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_90_ENUM_NAME
  #if (OPT_90_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_90_CLI_SHORT_CODE_CHR,
    #if (OPT_90_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_91_ENUM_NAME
  #if (OPT_91_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_91_CLI_SHORT_CODE_CHR,
    #if (OPT_91_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_92_ENUM_NAME
  #if (OPT_92_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_92_CLI_SHORT_CODE_CHR,
    #if (OPT_92_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_93_ENUM_NAME
  #if (OPT_93_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_93_CLI_SHORT_CODE_CHR,
    #if (OPT_93_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_94_ENUM_NAME
  #if (OPT_94_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_94_CLI_SHORT_CODE_CHR,
    #if (OPT_94_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_95_ENUM_NAME
  #if (OPT_95_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_95_CLI_SHORT_CODE_CHR,
    #if (OPT_95_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_96_ENUM_NAME
  #if (OPT_96_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_96_CLI_SHORT_CODE_CHR,
    #if (OPT_96_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_97_ENUM_NAME
  #if (OPT_97_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_97_CLI_SHORT_CODE_CHR,
    #if (OPT_97_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_98_ENUM_NAME
  #if (OPT_98_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_98_CLI_SHORT_CODE_CHR,
    #if (OPT_98_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_99_ENUM_NAME
  #if (OPT_99_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_99_CLI_SHORT_CODE_CHR,
    #if (OPT_99_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_100_ENUM_NAME
  #if (OPT_100_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_100_CLI_SHORT_CODE_CHR,
    #if (OPT_100_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_101_ENUM_NAME
  #if (OPT_101_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_101_CLI_SHORT_CODE_CHR,
    #if (OPT_101_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_102_ENUM_NAME
  #if (OPT_102_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_102_CLI_SHORT_CODE_CHR,
    #if (OPT_102_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_103_ENUM_NAME
  #if (OPT_103_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_103_CLI_SHORT_CODE_CHR,
    #if (OPT_103_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_104_ENUM_NAME
  #if (OPT_104_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_104_CLI_SHORT_CODE_CHR,
    #if (OPT_104_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_105_ENUM_NAME
  #if (OPT_105_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_105_CLI_SHORT_CODE_CHR,
    #if (OPT_105_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_106_ENUM_NAME
  #if (OPT_106_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_106_CLI_SHORT_CODE_CHR,
    #if (OPT_106_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_107_ENUM_NAME
  #if (OPT_107_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_107_CLI_SHORT_CODE_CHR,
    #if (OPT_107_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_108_ENUM_NAME
  #if (OPT_108_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_108_CLI_SHORT_CODE_CHR,
    #if (OPT_108_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_109_ENUM_NAME
  #if (OPT_109_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_109_CLI_SHORT_CODE_CHR,
    #if (OPT_109_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_110_ENUM_NAME
  #if (OPT_110_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_110_CLI_SHORT_CODE_CHR,
    #if (OPT_110_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_111_ENUM_NAME
  #if (OPT_111_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_111_CLI_SHORT_CODE_CHR,
    #if (OPT_111_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_112_ENUM_NAME
  #if (OPT_112_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_112_CLI_SHORT_CODE_CHR,
    #if (OPT_112_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_113_ENUM_NAME
  #if (OPT_113_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_113_CLI_SHORT_CODE_CHR,
    #if (OPT_113_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_114_ENUM_NAME
  #if (OPT_114_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_114_CLI_SHORT_CODE_CHR,
    #if (OPT_114_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_115_ENUM_NAME
  #if (OPT_115_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_115_CLI_SHORT_CODE_CHR,
    #if (OPT_115_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_116_ENUM_NAME
  #if (OPT_116_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_116_CLI_SHORT_CODE_CHR,
    #if (OPT_116_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_117_ENUM_NAME
  #if (OPT_117_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_117_CLI_SHORT_CODE_CHR,
    #if (OPT_117_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_118_ENUM_NAME
  #if (OPT_118_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_118_CLI_SHORT_CODE_CHR,
    #if (OPT_118_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_119_ENUM_NAME
  #if (OPT_119_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_119_CLI_SHORT_CODE_CHR,
    #if (OPT_119_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_120_ENUM_NAME
  #if (OPT_120_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_120_CLI_SHORT_CODE_CHR,
    #if (OPT_120_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_121_ENUM_NAME
  #if (OPT_121_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_121_CLI_SHORT_CODE_CHR,
    #if (OPT_121_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_122_ENUM_NAME
  #if (OPT_122_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_122_CLI_SHORT_CODE_CHR,
    #if (OPT_122_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_123_ENUM_NAME
  #if (OPT_123_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_123_CLI_SHORT_CODE_CHR,
    #if (OPT_123_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_124_ENUM_NAME
  #if (OPT_124_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_124_CLI_SHORT_CODE_CHR,
    #if (OPT_124_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_125_ENUM_NAME
  #if (OPT_125_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_125_CLI_SHORT_CODE_CHR,
    #if (OPT_125_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_126_ENUM_NAME
  #if (OPT_126_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_126_CLI_SHORT_CODE_CHR,
    #if (OPT_126_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_127_ENUM_NAME
  #if (OPT_127_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_127_CLI_SHORT_CODE_CHR,
    #if (OPT_127_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_128_ENUM_NAME
  #if (OPT_128_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_128_CLI_SHORT_CODE_CHR,
    #if (OPT_128_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_129_ENUM_NAME
  #if (OPT_129_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_129_CLI_SHORT_CODE_CHR,
    #if (OPT_129_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_130_ENUM_NAME
  #if (OPT_130_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_130_CLI_SHORT_CODE_CHR,
    #if (OPT_130_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_131_ENUM_NAME
  #if (OPT_131_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_131_CLI_SHORT_CODE_CHR,
    #if (OPT_131_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_132_ENUM_NAME
  #if (OPT_132_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_132_CLI_SHORT_CODE_CHR,
    #if (OPT_132_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_133_ENUM_NAME
  #if (OPT_133_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_133_CLI_SHORT_CODE_CHR,
    #if (OPT_133_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_134_ENUM_NAME
  #if (OPT_134_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_134_CLI_SHORT_CODE_CHR,
    #if (OPT_134_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_135_ENUM_NAME
  #if (OPT_135_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_135_CLI_SHORT_CODE_CHR,
    #if (OPT_135_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_136_ENUM_NAME
  #if (OPT_136_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_136_CLI_SHORT_CODE_CHR,
    #if (OPT_136_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_137_ENUM_NAME
  #if (OPT_137_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_137_CLI_SHORT_CODE_CHR,
    #if (OPT_137_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_138_ENUM_NAME
  #if (OPT_138_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_138_CLI_SHORT_CODE_CHR,
    #if (OPT_138_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_139_ENUM_NAME
  #if (OPT_139_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_139_CLI_SHORT_CODE_CHR,
    #if (OPT_139_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_140_ENUM_NAME
  #if (OPT_140_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_140_CLI_SHORT_CODE_CHR,
    #if (OPT_140_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_141_ENUM_NAME
  #if (OPT_141_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_141_CLI_SHORT_CODE_CHR,
    #if (OPT_141_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_142_ENUM_NAME
  #if (OPT_142_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_142_CLI_SHORT_CODE_CHR,
    #if (OPT_142_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_143_ENUM_NAME
  #if (OPT_143_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_143_CLI_SHORT_CODE_CHR,
    #if (OPT_143_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_144_ENUM_NAME
  #if (OPT_144_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_144_CLI_SHORT_CODE_CHR,
    #if (OPT_144_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_145_ENUM_NAME
  #if (OPT_145_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_145_CLI_SHORT_CODE_CHR,
    #if (OPT_145_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_146_ENUM_NAME
  #if (OPT_146_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_146_CLI_SHORT_CODE_CHR,
    #if (OPT_146_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_147_ENUM_NAME
  #if (OPT_147_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_147_CLI_SHORT_CODE_CHR,
    #if (OPT_147_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_148_ENUM_NAME
  #if (OPT_148_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_148_CLI_SHORT_CODE_CHR,
    #if (OPT_148_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_149_ENUM_NAME
  #if (OPT_149_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_149_CLI_SHORT_CODE_CHR,
    #if (OPT_149_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_150_ENUM_NAME
  #if (OPT_150_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_150_CLI_SHORT_CODE_CHR,
    #if (OPT_150_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_151_ENUM_NAME
  #if (OPT_151_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_151_CLI_SHORT_CODE_CHR,
    #if (OPT_151_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_152_ENUM_NAME
  #if (OPT_152_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_152_CLI_SHORT_CODE_CHR,
    #if (OPT_152_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_153_ENUM_NAME
  #if (OPT_153_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_153_CLI_SHORT_CODE_CHR,
    #if (OPT_153_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_154_ENUM_NAME
  #if (OPT_154_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_154_CLI_SHORT_CODE_CHR,
    #if (OPT_154_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_155_ENUM_NAME
  #if (OPT_155_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_155_CLI_SHORT_CODE_CHR,
    #if (OPT_155_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_156_ENUM_NAME
  #if (OPT_156_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_156_CLI_SHORT_CODE_CHR,
    #if (OPT_156_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_157_ENUM_NAME
  #if (OPT_157_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_157_CLI_SHORT_CODE_CHR,
    #if (OPT_157_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_158_ENUM_NAME
  #if (OPT_158_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_158_CLI_SHORT_CODE_CHR,
    #if (OPT_158_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_159_ENUM_NAME
  #if (OPT_159_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_159_CLI_SHORT_CODE_CHR,
    #if (OPT_159_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_160_ENUM_NAME
  #if (OPT_160_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_160_CLI_SHORT_CODE_CHR,
    #if (OPT_160_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_161_ENUM_NAME
  #if (OPT_161_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_161_CLI_SHORT_CODE_CHR,
    #if (OPT_161_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_162_ENUM_NAME
  #if (OPT_162_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_162_CLI_SHORT_CODE_CHR,
    #if (OPT_162_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_163_ENUM_NAME
  #if (OPT_163_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_163_CLI_SHORT_CODE_CHR,
    #if (OPT_163_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_164_ENUM_NAME
  #if (OPT_164_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_164_CLI_SHORT_CODE_CHR,
    #if (OPT_164_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_165_ENUM_NAME
  #if (OPT_165_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_165_CLI_SHORT_CODE_CHR,
    #if (OPT_165_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_166_ENUM_NAME
  #if (OPT_166_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_166_CLI_SHORT_CODE_CHR,
    #if (OPT_166_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_167_ENUM_NAME
  #if (OPT_167_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_167_CLI_SHORT_CODE_CHR,
    #if (OPT_167_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_168_ENUM_NAME
  #if (OPT_168_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_168_CLI_SHORT_CODE_CHR,
    #if (OPT_168_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_169_ENUM_NAME
  #if (OPT_169_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_169_CLI_SHORT_CODE_CHR,
    #if (OPT_169_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_170_ENUM_NAME
  #if (OPT_170_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_170_CLI_SHORT_CODE_CHR,
    #if (OPT_170_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_171_ENUM_NAME
  #if (OPT_171_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_171_CLI_SHORT_CODE_CHR,
    #if (OPT_171_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_172_ENUM_NAME
  #if (OPT_172_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_172_CLI_SHORT_CODE_CHR,
    #if (OPT_172_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_173_ENUM_NAME
  #if (OPT_173_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_173_CLI_SHORT_CODE_CHR,
    #if (OPT_173_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_174_ENUM_NAME
  #if (OPT_174_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_174_CLI_SHORT_CODE_CHR,
    #if (OPT_174_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_175_ENUM_NAME
  #if (OPT_175_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_175_CLI_SHORT_CODE_CHR,
    #if (OPT_175_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_176_ENUM_NAME
  #if (OPT_176_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_176_CLI_SHORT_CODE_CHR,
    #if (OPT_176_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_177_ENUM_NAME
  #if (OPT_177_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_177_CLI_SHORT_CODE_CHR,
    #if (OPT_177_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_178_ENUM_NAME
  #if (OPT_178_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_178_CLI_SHORT_CODE_CHR,
    #if (OPT_178_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_179_ENUM_NAME
  #if (OPT_179_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_179_CLI_SHORT_CODE_CHR,
    #if (OPT_179_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_180_ENUM_NAME
  #if (OPT_180_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_180_CLI_SHORT_CODE_CHR,
    #if (OPT_180_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_181_ENUM_NAME
  #if (OPT_181_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_181_CLI_SHORT_CODE_CHR,
    #if (OPT_181_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_182_ENUM_NAME
  #if (OPT_182_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_182_CLI_SHORT_CODE_CHR,
    #if (OPT_182_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_183_ENUM_NAME
  #if (OPT_183_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_183_CLI_SHORT_CODE_CHR,
    #if (OPT_183_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_184_ENUM_NAME
  #if (OPT_184_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_184_CLI_SHORT_CODE_CHR,
    #if (OPT_184_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_185_ENUM_NAME
  #if (OPT_185_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_185_CLI_SHORT_CODE_CHR,
    #if (OPT_185_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_186_ENUM_NAME
  #if (OPT_186_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_186_CLI_SHORT_CODE_CHR,
    #if (OPT_186_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_187_ENUM_NAME
  #if (OPT_187_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_187_CLI_SHORT_CODE_CHR,
    #if (OPT_187_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_188_ENUM_NAME
  #if (OPT_188_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_188_CLI_SHORT_CODE_CHR,
    #if (OPT_188_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_189_ENUM_NAME
  #if (OPT_189_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_189_CLI_SHORT_CODE_CHR,
    #if (OPT_189_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
    
#ifdef OPT_190_ENUM_NAME
  #if (OPT_190_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_190_CLI_SHORT_CODE_CHR,
    #if (OPT_190_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_191_ENUM_NAME
  #if (OPT_191_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_191_CLI_SHORT_CODE_CHR,
    #if (OPT_191_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_192_ENUM_NAME
  #if (OPT_192_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_192_CLI_SHORT_CODE_CHR,
    #if (OPT_192_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_193_ENUM_NAME
  #if (OPT_193_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_193_CLI_SHORT_CODE_CHR,
    #if (OPT_193_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_194_ENUM_NAME
  #if (OPT_194_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_194_CLI_SHORT_CODE_CHR,
    #if (OPT_194_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_195_ENUM_NAME
  #if (OPT_195_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_195_CLI_SHORT_CODE_CHR,
    #if (OPT_195_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_196_ENUM_NAME
  #if (OPT_196_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_196_CLI_SHORT_CODE_CHR,
    #if (OPT_196_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_197_ENUM_NAME
  #if (OPT_197_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_197_CLI_SHORT_CODE_CHR,
    #if (OPT_197_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_198_ENUM_NAME
  #if (OPT_198_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_198_CLI_SHORT_CODE_CHR,
    #if (OPT_198_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif
#ifdef OPT_199_ENUM_NAME
  #if (OPT_199_CLI_SHORT_CODE_CHR != OPT_AUTO_CODE)
       OPT_199_CLI_SHORT_CODE_CHR,
    #if (OPT_199_HAS_ARG_CHR == 'y')
       ':',
    #endif
  #endif
#endif


    0  /* terminator for 'txt_shortopts[]' */
};


/* longopt - auxiliary preprocessor symbols for REQ_ARG */
/* OPT_00_NO_OPTION is hardcoded with REQ_ARG symbol '(no_argument)' */
#ifdef OPT_01_ENUM_NAME
  #if (OPT_01_HAS_ARG_CHR == 'y')
    #define OPT_01_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_01_HAS_ARG_CHR == 'n')
    #define OPT_01_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_02_ENUM_NAME
  #if (OPT_02_HAS_ARG_CHR == 'y')
    #define OPT_02_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_02_HAS_ARG_CHR == 'n')
    #define OPT_02_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_03_ENUM_NAME
  #if (OPT_03_HAS_ARG_CHR == 'y')
    #define OPT_03_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_03_HAS_ARG_CHR == 'n')
    #define OPT_03_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_04_ENUM_NAME
  #if (OPT_04_HAS_ARG_CHR == 'y')
    #define OPT_04_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_04_HAS_ARG_CHR == 'n')
    #define OPT_04_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_05_ENUM_NAME
  #if (OPT_05_HAS_ARG_CHR == 'y')
    #define OPT_05_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_05_HAS_ARG_CHR == 'n')
    #define OPT_05_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_06_ENUM_NAME
  #if (OPT_06_HAS_ARG_CHR == 'y')
    #define OPT_06_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_06_HAS_ARG_CHR == 'n')
    #define OPT_06_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_07_ENUM_NAME
  #if (OPT_07_HAS_ARG_CHR == 'y')
    #define OPT_07_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_07_HAS_ARG_CHR == 'n')
    #define OPT_07_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_08_ENUM_NAME
  #if (OPT_08_HAS_ARG_CHR == 'y')
    #define OPT_08_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_08_HAS_ARG_CHR == 'n')
    #define OPT_08_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_09_ENUM_NAME
  #if (OPT_09_HAS_ARG_CHR == 'y')
    #define OPT_09_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_09_HAS_ARG_CHR == 'n')
    #define OPT_09_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_10_ENUM_NAME
  #if (OPT_10_HAS_ARG_CHR == 'y')
    #define OPT_10_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_10_HAS_ARG_CHR == 'n')
    #define OPT_10_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_11_ENUM_NAME
  #if (OPT_11_HAS_ARG_CHR == 'y')
    #define OPT_11_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_11_HAS_ARG_CHR == 'n')
    #define OPT_11_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_12_ENUM_NAME
  #if (OPT_12_HAS_ARG_CHR == 'y')
    #define OPT_12_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_12_HAS_ARG_CHR == 'n')
    #define OPT_12_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_13_ENUM_NAME
  #if (OPT_13_HAS_ARG_CHR == 'y')
    #define OPT_13_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_13_HAS_ARG_CHR == 'n')
    #define OPT_13_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_14_ENUM_NAME
  #if (OPT_14_HAS_ARG_CHR == 'y')
    #define OPT_14_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_14_HAS_ARG_CHR == 'n')
    #define OPT_14_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_15_ENUM_NAME
  #if (OPT_15_HAS_ARG_CHR == 'y')
    #define OPT_15_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_15_HAS_ARG_CHR == 'n')
    #define OPT_15_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_16_ENUM_NAME
  #if (OPT_16_HAS_ARG_CHR == 'y')
    #define OPT_16_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_16_HAS_ARG_CHR == 'n')
    #define OPT_16_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_17_ENUM_NAME
  #if (OPT_17_HAS_ARG_CHR == 'y')
    #define OPT_17_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_17_HAS_ARG_CHR == 'n')
    #define OPT_17_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_18_ENUM_NAME
  #if (OPT_18_HAS_ARG_CHR == 'y')
    #define OPT_18_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_18_HAS_ARG_CHR == 'n')
    #define OPT_18_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_19_ENUM_NAME
  #if (OPT_19_HAS_ARG_CHR == 'y')
    #define OPT_19_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_19_HAS_ARG_CHR == 'n')
    #define OPT_19_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_20_ENUM_NAME
  #if (OPT_20_HAS_ARG_CHR == 'y')
    #define OPT_20_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_20_HAS_ARG_CHR == 'n')
    #define OPT_20_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_21_ENUM_NAME
  #if (OPT_21_HAS_ARG_CHR == 'y')
    #define OPT_21_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_21_HAS_ARG_CHR == 'n')
    #define OPT_21_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_22_ENUM_NAME
  #if (OPT_22_HAS_ARG_CHR == 'y')
    #define OPT_22_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_22_HAS_ARG_CHR == 'n')
    #define OPT_22_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_23_ENUM_NAME
  #if (OPT_23_HAS_ARG_CHR == 'y')
    #define OPT_23_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_23_HAS_ARG_CHR == 'n')
    #define OPT_23_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_24_ENUM_NAME
  #if (OPT_24_HAS_ARG_CHR == 'y')
    #define OPT_24_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_24_HAS_ARG_CHR == 'n')
    #define OPT_24_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_25_ENUM_NAME
  #if (OPT_25_HAS_ARG_CHR == 'y')
    #define OPT_25_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_25_HAS_ARG_CHR == 'n')
    #define OPT_25_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_26_ENUM_NAME
  #if (OPT_26_HAS_ARG_CHR == 'y')
    #define OPT_26_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_26_HAS_ARG_CHR == 'n')
    #define OPT_26_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_27_ENUM_NAME
  #if (OPT_27_HAS_ARG_CHR == 'y')
    #define OPT_27_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_27_HAS_ARG_CHR == 'n')
    #define OPT_27_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_28_ENUM_NAME
  #if (OPT_28_HAS_ARG_CHR == 'y')
    #define OPT_28_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_28_HAS_ARG_CHR == 'n')
    #define OPT_28_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_29_ENUM_NAME
  #if (OPT_29_HAS_ARG_CHR == 'y')
    #define OPT_29_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_29_HAS_ARG_CHR == 'n')
    #define OPT_29_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_30_ENUM_NAME
  #if (OPT_30_HAS_ARG_CHR == 'y')
    #define OPT_30_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_30_HAS_ARG_CHR == 'n')
    #define OPT_30_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_31_ENUM_NAME
  #if (OPT_31_HAS_ARG_CHR == 'y')
    #define OPT_31_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_31_HAS_ARG_CHR == 'n')
    #define OPT_31_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_32_ENUM_NAME
  #if (OPT_32_HAS_ARG_CHR == 'y')
    #define OPT_32_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_32_HAS_ARG_CHR == 'n')
    #define OPT_32_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_33_ENUM_NAME
  #if (OPT_33_HAS_ARG_CHR == 'y')
    #define OPT_33_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_33_HAS_ARG_CHR == 'n')
    #define OPT_33_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_34_ENUM_NAME
  #if (OPT_34_HAS_ARG_CHR == 'y')
    #define OPT_34_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_34_HAS_ARG_CHR == 'n')
    #define OPT_34_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_35_ENUM_NAME
  #if (OPT_35_HAS_ARG_CHR == 'y')
    #define OPT_35_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_35_HAS_ARG_CHR == 'n')
    #define OPT_35_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_36_ENUM_NAME
  #if (OPT_36_HAS_ARG_CHR == 'y')
    #define OPT_36_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_36_HAS_ARG_CHR == 'n')
    #define OPT_36_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_37_ENUM_NAME
  #if (OPT_37_HAS_ARG_CHR == 'y')
    #define OPT_37_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_37_HAS_ARG_CHR == 'n')
    #define OPT_37_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_38_ENUM_NAME
  #if (OPT_38_HAS_ARG_CHR == 'y')
    #define OPT_38_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_38_HAS_ARG_CHR == 'n')
    #define OPT_38_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_39_ENUM_NAME
  #if (OPT_39_HAS_ARG_CHR == 'y')
    #define OPT_39_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_39_HAS_ARG_CHR == 'n')
    #define OPT_39_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_40_ENUM_NAME
  #if (OPT_40_HAS_ARG_CHR == 'y')
    #define OPT_40_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_40_HAS_ARG_CHR == 'n')
    #define OPT_40_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_41_ENUM_NAME
  #if (OPT_41_HAS_ARG_CHR == 'y')
    #define OPT_41_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_41_HAS_ARG_CHR == 'n')
    #define OPT_41_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_42_ENUM_NAME
  #if (OPT_42_HAS_ARG_CHR == 'y')
    #define OPT_42_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_42_HAS_ARG_CHR == 'n')
    #define OPT_42_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_43_ENUM_NAME
  #if (OPT_43_HAS_ARG_CHR == 'y')
    #define OPT_43_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_43_HAS_ARG_CHR == 'n')
    #define OPT_43_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_44_ENUM_NAME
  #if (OPT_44_HAS_ARG_CHR == 'y')
    #define OPT_44_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_44_HAS_ARG_CHR == 'n')
    #define OPT_44_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_45_ENUM_NAME
  #if (OPT_45_HAS_ARG_CHR == 'y')
    #define OPT_45_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_45_HAS_ARG_CHR == 'n')
    #define OPT_45_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_46_ENUM_NAME
  #if (OPT_46_HAS_ARG_CHR == 'y')
    #define OPT_46_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_46_HAS_ARG_CHR == 'n')
    #define OPT_46_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_47_ENUM_NAME
  #if (OPT_47_HAS_ARG_CHR == 'y')
    #define OPT_47_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_47_HAS_ARG_CHR == 'n')
    #define OPT_47_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_48_ENUM_NAME
  #if (OPT_48_HAS_ARG_CHR == 'y')
    #define OPT_48_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_48_HAS_ARG_CHR == 'n')
    #define OPT_48_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_49_ENUM_NAME
  #if (OPT_49_HAS_ARG_CHR == 'y')
    #define OPT_49_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_49_HAS_ARG_CHR == 'n')
    #define OPT_49_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_50_ENUM_NAME
  #if (OPT_50_HAS_ARG_CHR == 'y')
    #define OPT_50_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_50_HAS_ARG_CHR == 'n')
    #define OPT_50_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_51_ENUM_NAME
  #if (OPT_51_HAS_ARG_CHR == 'y')
    #define OPT_51_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_51_HAS_ARG_CHR == 'n')
    #define OPT_51_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_52_ENUM_NAME
  #if (OPT_52_HAS_ARG_CHR == 'y')
    #define OPT_52_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_52_HAS_ARG_CHR == 'n')
    #define OPT_52_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_53_ENUM_NAME
  #if (OPT_53_HAS_ARG_CHR == 'y')
    #define OPT_53_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_53_HAS_ARG_CHR == 'n')
    #define OPT_53_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_54_ENUM_NAME
  #if (OPT_54_HAS_ARG_CHR == 'y')
    #define OPT_54_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_54_HAS_ARG_CHR == 'n')
    #define OPT_54_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_55_ENUM_NAME
  #if (OPT_55_HAS_ARG_CHR == 'y')
    #define OPT_55_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_55_HAS_ARG_CHR == 'n')
    #define OPT_55_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_56_ENUM_NAME
  #if (OPT_56_HAS_ARG_CHR == 'y')
    #define OPT_56_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_56_HAS_ARG_CHR == 'n')
    #define OPT_56_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_57_ENUM_NAME
  #if (OPT_57_HAS_ARG_CHR == 'y')
    #define OPT_57_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_57_HAS_ARG_CHR == 'n')
    #define OPT_57_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_58_ENUM_NAME
  #if (OPT_58_HAS_ARG_CHR == 'y')
    #define OPT_58_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_58_HAS_ARG_CHR == 'n')
    #define OPT_58_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_59_ENUM_NAME
  #if (OPT_59_HAS_ARG_CHR == 'y')
    #define OPT_59_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_59_HAS_ARG_CHR == 'n')
    #define OPT_59_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_60_ENUM_NAME
  #if (OPT_60_HAS_ARG_CHR == 'y')
    #define OPT_60_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_60_HAS_ARG_CHR == 'n')
    #define OPT_60_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_61_ENUM_NAME
  #if (OPT_61_HAS_ARG_CHR == 'y')
    #define OPT_61_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_61_HAS_ARG_CHR == 'n')
    #define OPT_61_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_62_ENUM_NAME
  #if (OPT_62_HAS_ARG_CHR == 'y')
    #define OPT_62_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_62_HAS_ARG_CHR == 'n')
    #define OPT_62_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_63_ENUM_NAME
  #if (OPT_63_HAS_ARG_CHR == 'y')
    #define OPT_63_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_63_HAS_ARG_CHR == 'n')
    #define OPT_63_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_64_ENUM_NAME
  #if (OPT_64_HAS_ARG_CHR == 'y')
    #define OPT_64_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_64_HAS_ARG_CHR == 'n')
    #define OPT_64_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_65_ENUM_NAME
  #if (OPT_65_HAS_ARG_CHR == 'y')
    #define OPT_65_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_65_HAS_ARG_CHR == 'n')
    #define OPT_65_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_66_ENUM_NAME
  #if (OPT_66_HAS_ARG_CHR == 'y')
    #define OPT_66_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_66_HAS_ARG_CHR == 'n')
    #define OPT_66_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_67_ENUM_NAME
  #if (OPT_67_HAS_ARG_CHR == 'y')
    #define OPT_67_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_67_HAS_ARG_CHR == 'n')
    #define OPT_67_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_68_ENUM_NAME
  #if (OPT_68_HAS_ARG_CHR == 'y')
    #define OPT_68_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_68_HAS_ARG_CHR == 'n')
    #define OPT_68_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_69_ENUM_NAME
  #if (OPT_69_HAS_ARG_CHR == 'y')
    #define OPT_69_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_69_HAS_ARG_CHR == 'n')
    #define OPT_69_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_70_ENUM_NAME
  #if (OPT_70_HAS_ARG_CHR == 'y')
    #define OPT_70_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_70_HAS_ARG_CHR == 'n')
    #define OPT_70_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_71_ENUM_NAME
  #if (OPT_71_HAS_ARG_CHR == 'y')
    #define OPT_71_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_71_HAS_ARG_CHR == 'n')
    #define OPT_71_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_72_ENUM_NAME
  #if (OPT_72_HAS_ARG_CHR == 'y')
    #define OPT_72_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_72_HAS_ARG_CHR == 'n')
    #define OPT_72_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_73_ENUM_NAME
  #if (OPT_73_HAS_ARG_CHR == 'y')
    #define OPT_73_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_73_HAS_ARG_CHR == 'n')
    #define OPT_73_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_74_ENUM_NAME
  #if (OPT_74_HAS_ARG_CHR == 'y')
    #define OPT_74_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_74_HAS_ARG_CHR == 'n')
    #define OPT_74_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_75_ENUM_NAME
  #if (OPT_75_HAS_ARG_CHR == 'y')
    #define OPT_75_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_75_HAS_ARG_CHR == 'n')
    #define OPT_75_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_76_ENUM_NAME
  #if (OPT_76_HAS_ARG_CHR == 'y')
    #define OPT_76_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_76_HAS_ARG_CHR == 'n')
    #define OPT_76_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_77_ENUM_NAME
  #if (OPT_77_HAS_ARG_CHR == 'y')
    #define OPT_77_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_77_HAS_ARG_CHR == 'n')
    #define OPT_77_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_78_ENUM_NAME
  #if (OPT_78_HAS_ARG_CHR == 'y')
    #define OPT_78_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_78_HAS_ARG_CHR == 'n')
    #define OPT_78_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_79_ENUM_NAME
  #if (OPT_79_HAS_ARG_CHR == 'y')
    #define OPT_79_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_79_HAS_ARG_CHR == 'n')
    #define OPT_79_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_80_ENUM_NAME
  #if (OPT_80_HAS_ARG_CHR == 'y')
    #define OPT_80_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_80_HAS_ARG_CHR == 'n')
    #define OPT_80_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_81_ENUM_NAME
  #if (OPT_81_HAS_ARG_CHR == 'y')
    #define OPT_81_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_81_HAS_ARG_CHR == 'n')
    #define OPT_81_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_82_ENUM_NAME
  #if (OPT_82_HAS_ARG_CHR == 'y')
    #define OPT_82_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_82_HAS_ARG_CHR == 'n')
    #define OPT_82_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_83_ENUM_NAME
  #if (OPT_83_HAS_ARG_CHR == 'y')
    #define OPT_83_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_83_HAS_ARG_CHR == 'n')
    #define OPT_83_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_84_ENUM_NAME
  #if (OPT_84_HAS_ARG_CHR == 'y')
    #define OPT_84_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_84_HAS_ARG_CHR == 'n')
    #define OPT_84_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_85_ENUM_NAME
  #if (OPT_85_HAS_ARG_CHR == 'y')
    #define OPT_85_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_85_HAS_ARG_CHR == 'n')
    #define OPT_85_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_86_ENUM_NAME
  #if (OPT_86_HAS_ARG_CHR == 'y')
    #define OPT_86_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_86_HAS_ARG_CHR == 'n')
    #define OPT_86_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_87_ENUM_NAME
  #if (OPT_87_HAS_ARG_CHR == 'y')
    #define OPT_87_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_87_HAS_ARG_CHR == 'n')
    #define OPT_87_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_88_ENUM_NAME
  #if (OPT_88_HAS_ARG_CHR == 'y')
    #define OPT_88_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_88_HAS_ARG_CHR == 'n')
    #define OPT_88_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_89_ENUM_NAME
  #if (OPT_89_HAS_ARG_CHR == 'y')
    #define OPT_89_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_89_HAS_ARG_CHR == 'n')
    #define OPT_89_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_90_ENUM_NAME
  #if (OPT_90_HAS_ARG_CHR == 'y')
    #define OPT_90_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_90_HAS_ARG_CHR == 'n')
    #define OPT_90_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_91_ENUM_NAME
  #if (OPT_91_HAS_ARG_CHR == 'y')
    #define OPT_91_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_91_HAS_ARG_CHR == 'n')
    #define OPT_91_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_92_ENUM_NAME
  #if (OPT_92_HAS_ARG_CHR == 'y')
    #define OPT_92_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_92_HAS_ARG_CHR == 'n')
    #define OPT_92_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_93_ENUM_NAME
  #if (OPT_93_HAS_ARG_CHR == 'y')
    #define OPT_93_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_93_HAS_ARG_CHR == 'n')
    #define OPT_93_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_94_ENUM_NAME
  #if (OPT_94_HAS_ARG_CHR == 'y')
    #define OPT_94_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_94_HAS_ARG_CHR == 'n')
    #define OPT_94_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_95_ENUM_NAME
  #if (OPT_95_HAS_ARG_CHR == 'y')
    #define OPT_95_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_95_HAS_ARG_CHR == 'n')
    #define OPT_95_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_96_ENUM_NAME
  #if (OPT_96_HAS_ARG_CHR == 'y')
    #define OPT_96_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_96_HAS_ARG_CHR == 'n')
    #define OPT_96_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_97_ENUM_NAME
  #if (OPT_97_HAS_ARG_CHR == 'y')
    #define OPT_97_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_97_HAS_ARG_CHR == 'n')
    #define OPT_97_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_98_ENUM_NAME
  #if (OPT_98_HAS_ARG_CHR == 'y')
    #define OPT_98_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_98_HAS_ARG_CHR == 'n')
    #define OPT_98_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_99_ENUM_NAME
  #if (OPT_99_HAS_ARG_CHR == 'y')
    #define OPT_99_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_99_HAS_ARG_CHR == 'n')
    #define OPT_99_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_100_ENUM_NAME
  #if (OPT_100_HAS_ARG_CHR == 'y')
    #define OPT_100_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_100_HAS_ARG_CHR == 'n')
    #define OPT_100_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_101_ENUM_NAME
  #if (OPT_101_HAS_ARG_CHR == 'y')
    #define OPT_101_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_101_HAS_ARG_CHR == 'n')
    #define OPT_101_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_102_ENUM_NAME
  #if (OPT_102_HAS_ARG_CHR == 'y')
    #define OPT_102_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_102_HAS_ARG_CHR == 'n')
    #define OPT_102_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_103_ENUM_NAME
  #if (OPT_103_HAS_ARG_CHR == 'y')
    #define OPT_103_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_103_HAS_ARG_CHR == 'n')
    #define OPT_103_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_104_ENUM_NAME
  #if (OPT_104_HAS_ARG_CHR == 'y')
    #define OPT_104_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_104_HAS_ARG_CHR == 'n')
    #define OPT_104_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_105_ENUM_NAME
  #if (OPT_105_HAS_ARG_CHR == 'y')
    #define OPT_105_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_105_HAS_ARG_CHR == 'n')
    #define OPT_105_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_106_ENUM_NAME
  #if (OPT_106_HAS_ARG_CHR == 'y')
    #define OPT_106_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_106_HAS_ARG_CHR == 'n')
    #define OPT_106_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_107_ENUM_NAME
  #if (OPT_107_HAS_ARG_CHR == 'y')
    #define OPT_107_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_107_HAS_ARG_CHR == 'n')
    #define OPT_107_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_108_ENUM_NAME
  #if (OPT_108_HAS_ARG_CHR == 'y')
    #define OPT_108_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_108_HAS_ARG_CHR == 'n')
    #define OPT_108_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_109_ENUM_NAME
  #if (OPT_109_HAS_ARG_CHR == 'y')
    #define OPT_109_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_109_HAS_ARG_CHR == 'n')
    #define OPT_109_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_110_ENUM_NAME
  #if (OPT_110_HAS_ARG_CHR == 'y')
    #define OPT_110_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_110_HAS_ARG_CHR == 'n')
    #define OPT_110_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_111_ENUM_NAME
  #if (OPT_111_HAS_ARG_CHR == 'y')
    #define OPT_111_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_111_HAS_ARG_CHR == 'n')
    #define OPT_111_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_112_ENUM_NAME
  #if (OPT_112_HAS_ARG_CHR == 'y')
    #define OPT_112_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_112_HAS_ARG_CHR == 'n')
    #define OPT_112_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_113_ENUM_NAME
  #if (OPT_113_HAS_ARG_CHR == 'y')
    #define OPT_113_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_113_HAS_ARG_CHR == 'n')
    #define OPT_113_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_114_ENUM_NAME
  #if (OPT_114_HAS_ARG_CHR == 'y')
    #define OPT_114_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_114_HAS_ARG_CHR == 'n')
    #define OPT_114_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_115_ENUM_NAME
  #if (OPT_115_HAS_ARG_CHR == 'y')
    #define OPT_115_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_115_HAS_ARG_CHR == 'n')
    #define OPT_115_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_116_ENUM_NAME
  #if (OPT_116_HAS_ARG_CHR == 'y')
    #define OPT_116_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_116_HAS_ARG_CHR == 'n')
    #define OPT_116_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_117_ENUM_NAME
  #if (OPT_117_HAS_ARG_CHR == 'y')
    #define OPT_117_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_117_HAS_ARG_CHR == 'n')
    #define OPT_117_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_118_ENUM_NAME
  #if (OPT_118_HAS_ARG_CHR == 'y')
    #define OPT_118_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_118_HAS_ARG_CHR == 'n')
    #define OPT_118_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_119_ENUM_NAME
  #if (OPT_119_HAS_ARG_CHR == 'y')
    #define OPT_119_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_119_HAS_ARG_CHR == 'n')
    #define OPT_119_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_120_ENUM_NAME
  #if (OPT_120_HAS_ARG_CHR == 'y')
    #define OPT_120_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_120_HAS_ARG_CHR == 'n')
    #define OPT_120_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_121_ENUM_NAME
  #if (OPT_121_HAS_ARG_CHR == 'y')
    #define OPT_121_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_121_HAS_ARG_CHR == 'n')
    #define OPT_121_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_122_ENUM_NAME
  #if (OPT_122_HAS_ARG_CHR == 'y')
    #define OPT_122_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_122_HAS_ARG_CHR == 'n')
    #define OPT_122_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_123_ENUM_NAME
  #if (OPT_123_HAS_ARG_CHR == 'y')
    #define OPT_123_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_123_HAS_ARG_CHR == 'n')
    #define OPT_123_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_124_ENUM_NAME
  #if (OPT_124_HAS_ARG_CHR == 'y')
    #define OPT_124_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_124_HAS_ARG_CHR == 'n')
    #define OPT_124_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_125_ENUM_NAME
  #if (OPT_125_HAS_ARG_CHR == 'y')
    #define OPT_125_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_125_HAS_ARG_CHR == 'n')
    #define OPT_125_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_126_ENUM_NAME
  #if (OPT_126_HAS_ARG_CHR == 'y')
    #define OPT_126_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_126_HAS_ARG_CHR == 'n')
    #define OPT_126_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_127_ENUM_NAME
  #if (OPT_127_HAS_ARG_CHR == 'y')
    #define OPT_127_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_127_HAS_ARG_CHR == 'n')
    #define OPT_127_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_128_ENUM_NAME
  #if (OPT_128_HAS_ARG_CHR == 'y')
    #define OPT_128_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_128_HAS_ARG_CHR == 'n')
    #define OPT_128_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_129_ENUM_NAME
  #if (OPT_129_HAS_ARG_CHR == 'y')
    #define OPT_129_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_129_HAS_ARG_CHR == 'n')
    #define OPT_129_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_130_ENUM_NAME
  #if (OPT_130_HAS_ARG_CHR == 'y')
    #define OPT_130_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_130_HAS_ARG_CHR == 'n')
    #define OPT_130_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_131_ENUM_NAME
  #if (OPT_131_HAS_ARG_CHR == 'y')
    #define OPT_131_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_131_HAS_ARG_CHR == 'n')
    #define OPT_131_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_132_ENUM_NAME
  #if (OPT_132_HAS_ARG_CHR == 'y')
    #define OPT_132_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_132_HAS_ARG_CHR == 'n')
    #define OPT_132_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_133_ENUM_NAME
  #if (OPT_133_HAS_ARG_CHR == 'y')
    #define OPT_133_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_133_HAS_ARG_CHR == 'n')
    #define OPT_133_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_134_ENUM_NAME
  #if (OPT_134_HAS_ARG_CHR == 'y')
    #define OPT_134_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_134_HAS_ARG_CHR == 'n')
    #define OPT_134_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_135_ENUM_NAME
  #if (OPT_135_HAS_ARG_CHR == 'y')
    #define OPT_135_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_135_HAS_ARG_CHR == 'n')
    #define OPT_135_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_136_ENUM_NAME
  #if (OPT_136_HAS_ARG_CHR == 'y')
    #define OPT_136_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_136_HAS_ARG_CHR == 'n')
    #define OPT_136_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_137_ENUM_NAME
  #if (OPT_137_HAS_ARG_CHR == 'y')
    #define OPT_137_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_137_HAS_ARG_CHR == 'n')
    #define OPT_137_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_138_ENUM_NAME
  #if (OPT_138_HAS_ARG_CHR == 'y')
    #define OPT_138_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_138_HAS_ARG_CHR == 'n')
    #define OPT_138_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_139_ENUM_NAME
  #if (OPT_139_HAS_ARG_CHR == 'y')
    #define OPT_139_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_139_HAS_ARG_CHR == 'n')
    #define OPT_139_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_140_ENUM_NAME
  #if (OPT_140_HAS_ARG_CHR == 'y')
    #define OPT_140_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_140_HAS_ARG_CHR == 'n')
    #define OPT_140_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_141_ENUM_NAME
  #if (OPT_141_HAS_ARG_CHR == 'y')
    #define OPT_141_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_141_HAS_ARG_CHR == 'n')
    #define OPT_141_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_142_ENUM_NAME
  #if (OPT_142_HAS_ARG_CHR == 'y')
    #define OPT_142_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_142_HAS_ARG_CHR == 'n')
    #define OPT_142_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_143_ENUM_NAME
  #if (OPT_143_HAS_ARG_CHR == 'y')
    #define OPT_143_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_143_HAS_ARG_CHR == 'n')
    #define OPT_143_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_144_ENUM_NAME
  #if (OPT_144_HAS_ARG_CHR == 'y')
    #define OPT_144_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_144_HAS_ARG_CHR == 'n')
    #define OPT_144_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_145_ENUM_NAME
  #if (OPT_145_HAS_ARG_CHR == 'y')
    #define OPT_145_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_145_HAS_ARG_CHR == 'n')
    #define OPT_145_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_146_ENUM_NAME
  #if (OPT_146_HAS_ARG_CHR == 'y')
    #define OPT_146_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_146_HAS_ARG_CHR == 'n')
    #define OPT_146_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_147_ENUM_NAME
  #if (OPT_147_HAS_ARG_CHR == 'y')
    #define OPT_147_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_147_HAS_ARG_CHR == 'n')
    #define OPT_147_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_148_ENUM_NAME
  #if (OPT_148_HAS_ARG_CHR == 'y')
    #define OPT_148_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_148_HAS_ARG_CHR == 'n')
    #define OPT_148_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_149_ENUM_NAME
  #if (OPT_149_HAS_ARG_CHR == 'y')
    #define OPT_149_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_149_HAS_ARG_CHR == 'n')
    #define OPT_149_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_150_ENUM_NAME
  #if (OPT_150_HAS_ARG_CHR == 'y')
    #define OPT_150_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_150_HAS_ARG_CHR == 'n')
    #define OPT_150_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_151_ENUM_NAME
  #if (OPT_151_HAS_ARG_CHR == 'y')
    #define OPT_151_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_151_HAS_ARG_CHR == 'n')
    #define OPT_151_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_152_ENUM_NAME
  #if (OPT_152_HAS_ARG_CHR == 'y')
    #define OPT_152_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_152_HAS_ARG_CHR == 'n')
    #define OPT_152_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_153_ENUM_NAME
  #if (OPT_153_HAS_ARG_CHR == 'y')
    #define OPT_153_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_153_HAS_ARG_CHR == 'n')
    #define OPT_153_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_154_ENUM_NAME
  #if (OPT_154_HAS_ARG_CHR == 'y')
    #define OPT_154_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_154_HAS_ARG_CHR == 'n')
    #define OPT_154_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_155_ENUM_NAME
  #if (OPT_155_HAS_ARG_CHR == 'y')
    #define OPT_155_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_155_HAS_ARG_CHR == 'n')
    #define OPT_155_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_156_ENUM_NAME
  #if (OPT_156_HAS_ARG_CHR == 'y')
    #define OPT_156_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_156_HAS_ARG_CHR == 'n')
    #define OPT_156_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_157_ENUM_NAME
  #if (OPT_157_HAS_ARG_CHR == 'y')
    #define OPT_157_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_157_HAS_ARG_CHR == 'n')
    #define OPT_157_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_158_ENUM_NAME
  #if (OPT_158_HAS_ARG_CHR == 'y')
    #define OPT_158_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_158_HAS_ARG_CHR == 'n')
    #define OPT_158_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_159_ENUM_NAME
  #if (OPT_159_HAS_ARG_CHR == 'y')
    #define OPT_159_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_159_HAS_ARG_CHR == 'n')
    #define OPT_159_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_160_ENUM_NAME
  #if (OPT_160_HAS_ARG_CHR == 'y')
    #define OPT_160_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_160_HAS_ARG_CHR == 'n')
    #define OPT_160_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_161_ENUM_NAME
  #if (OPT_161_HAS_ARG_CHR == 'y')
    #define OPT_161_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_161_HAS_ARG_CHR == 'n')
    #define OPT_161_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_162_ENUM_NAME
  #if (OPT_162_HAS_ARG_CHR == 'y')
    #define OPT_162_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_162_HAS_ARG_CHR == 'n')
    #define OPT_162_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_163_ENUM_NAME
  #if (OPT_163_HAS_ARG_CHR == 'y')
    #define OPT_163_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_163_HAS_ARG_CHR == 'n')
    #define OPT_163_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_164_ENUM_NAME
  #if (OPT_164_HAS_ARG_CHR == 'y')
    #define OPT_164_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_164_HAS_ARG_CHR == 'n')
    #define OPT_164_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_165_ENUM_NAME
  #if (OPT_165_HAS_ARG_CHR == 'y')
    #define OPT_165_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_165_HAS_ARG_CHR == 'n')
    #define OPT_165_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_166_ENUM_NAME
  #if (OPT_166_HAS_ARG_CHR == 'y')
    #define OPT_166_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_166_HAS_ARG_CHR == 'n')
    #define OPT_166_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_167_ENUM_NAME
  #if (OPT_167_HAS_ARG_CHR == 'y')
    #define OPT_167_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_167_HAS_ARG_CHR == 'n')
    #define OPT_167_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_168_ENUM_NAME
  #if (OPT_168_HAS_ARG_CHR == 'y')
    #define OPT_168_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_168_HAS_ARG_CHR == 'n')
    #define OPT_168_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_169_ENUM_NAME
  #if (OPT_169_HAS_ARG_CHR == 'y')
    #define OPT_169_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_169_HAS_ARG_CHR == 'n')
    #define OPT_169_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_170_ENUM_NAME
  #if (OPT_170_HAS_ARG_CHR == 'y')
    #define OPT_170_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_170_HAS_ARG_CHR == 'n')
    #define OPT_170_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_171_ENUM_NAME
  #if (OPT_171_HAS_ARG_CHR == 'y')
    #define OPT_171_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_171_HAS_ARG_CHR == 'n')
    #define OPT_171_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_172_ENUM_NAME
  #if (OPT_172_HAS_ARG_CHR == 'y')
    #define OPT_172_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_172_HAS_ARG_CHR == 'n')
    #define OPT_172_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_173_ENUM_NAME
  #if (OPT_173_HAS_ARG_CHR == 'y')
    #define OPT_173_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_173_HAS_ARG_CHR == 'n')
    #define OPT_173_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_174_ENUM_NAME
  #if (OPT_174_HAS_ARG_CHR == 'y')
    #define OPT_174_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_174_HAS_ARG_CHR == 'n')
    #define OPT_174_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_175_ENUM_NAME
  #if (OPT_175_HAS_ARG_CHR == 'y')
    #define OPT_175_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_175_HAS_ARG_CHR == 'n')
    #define OPT_175_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_176_ENUM_NAME
  #if (OPT_176_HAS_ARG_CHR == 'y')
    #define OPT_176_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_176_HAS_ARG_CHR == 'n')
    #define OPT_176_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_177_ENUM_NAME
  #if (OPT_177_HAS_ARG_CHR == 'y')
    #define OPT_177_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_177_HAS_ARG_CHR == 'n')
    #define OPT_177_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_178_ENUM_NAME
  #if (OPT_178_HAS_ARG_CHR == 'y')
    #define OPT_178_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_178_HAS_ARG_CHR == 'n')
    #define OPT_178_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_179_ENUM_NAME
  #if (OPT_179_HAS_ARG_CHR == 'y')
    #define OPT_179_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_179_HAS_ARG_CHR == 'n')
    #define OPT_179_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_180_ENUM_NAME
  #if (OPT_180_HAS_ARG_CHR == 'y')
    #define OPT_180_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_180_HAS_ARG_CHR == 'n')
    #define OPT_180_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_181_ENUM_NAME
  #if (OPT_181_HAS_ARG_CHR == 'y')
    #define OPT_181_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_181_HAS_ARG_CHR == 'n')
    #define OPT_181_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_182_ENUM_NAME
  #if (OPT_182_HAS_ARG_CHR == 'y')
    #define OPT_182_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_182_HAS_ARG_CHR == 'n')
    #define OPT_182_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_183_ENUM_NAME
  #if (OPT_183_HAS_ARG_CHR == 'y')
    #define OPT_183_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_183_HAS_ARG_CHR == 'n')
    #define OPT_183_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_184_ENUM_NAME
  #if (OPT_184_HAS_ARG_CHR == 'y')
    #define OPT_184_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_184_HAS_ARG_CHR == 'n')
    #define OPT_184_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_185_ENUM_NAME
  #if (OPT_185_HAS_ARG_CHR == 'y')
    #define OPT_185_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_185_HAS_ARG_CHR == 'n')
    #define OPT_185_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_186_ENUM_NAME
  #if (OPT_186_HAS_ARG_CHR == 'y')
    #define OPT_186_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_186_HAS_ARG_CHR == 'n')
    #define OPT_186_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_187_ENUM_NAME
  #if (OPT_187_HAS_ARG_CHR == 'y')
    #define OPT_187_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_187_HAS_ARG_CHR == 'n')
    #define OPT_187_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_188_ENUM_NAME
  #if (OPT_188_HAS_ARG_CHR == 'y')
    #define OPT_188_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_188_HAS_ARG_CHR == 'n')
    #define OPT_188_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_189_ENUM_NAME
  #if (OPT_189_HAS_ARG_CHR == 'y')
    #define OPT_189_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_189_HAS_ARG_CHR == 'n')
    #define OPT_189_HAS_ARG_LONG  (no_argument)
  #endif
#endif

#ifdef OPT_190_ENUM_NAME
  #if (OPT_190_HAS_ARG_CHR == 'y')
    #define OPT_190_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_190_HAS_ARG_CHR == 'n')
    #define OPT_190_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_191_ENUM_NAME
  #if (OPT_191_HAS_ARG_CHR == 'y')
    #define OPT_191_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_191_HAS_ARG_CHR == 'n')
    #define OPT_191_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_192_ENUM_NAME
  #if (OPT_192_HAS_ARG_CHR == 'y')
    #define OPT_192_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_192_HAS_ARG_CHR == 'n')
    #define OPT_192_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_193_ENUM_NAME
  #if (OPT_193_HAS_ARG_CHR == 'y')
    #define OPT_193_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_193_HAS_ARG_CHR == 'n')
    #define OPT_193_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_194_ENUM_NAME
  #if (OPT_194_HAS_ARG_CHR == 'y')
    #define OPT_194_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_194_HAS_ARG_CHR == 'n')
    #define OPT_194_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_195_ENUM_NAME
  #if (OPT_195_HAS_ARG_CHR == 'y')
    #define OPT_195_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_195_HAS_ARG_CHR == 'n')
    #define OPT_195_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_196_ENUM_NAME
  #if (OPT_196_HAS_ARG_CHR == 'y')
    #define OPT_196_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_196_HAS_ARG_CHR == 'n')
    #define OPT_196_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_197_ENUM_NAME
  #if (OPT_197_HAS_ARG_CHR == 'y')
    #define OPT_197_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_197_HAS_ARG_CHR == 'n')
    #define OPT_197_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_198_ENUM_NAME
  #if (OPT_198_HAS_ARG_CHR == 'y')
    #define OPT_198_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_198_HAS_ARG_CHR == 'n')
    #define OPT_198_HAS_ARG_LONG  (no_argument)
  #endif
#endif
#ifdef OPT_199_ENUM_NAME
  #if (OPT_199_HAS_ARG_CHR == 'y')
    #define OPT_199_HAS_ARG_LONG  (required_argument)
  #endif
  #if (OPT_199_HAS_ARG_CHR == 'n')
    #define OPT_199_HAS_ARG_LONG  (no_argument)
  #endif
#endif

/* longopts - mapping defined items into longopt array */
/* NOTE: remember, array is NOT INDEXED by 'cli_opt_t', because 'cli_opt_t' is NOT A CONSECUTIVE LIST */
static const struct option longopts[] =
{
    {"no-option", no_argument, 0, OPT_00_NO_OPTION},

#ifdef OPT_01_ENUM_NAME
    #ifdef OPT_01_CLI_LONG_TXT_A
          {OPT_01_CLI_LONG_TXT_A, OPT_01_HAS_ARG_LONG, 0, OPT_01_ENUM_NAME},
    #endif
    #ifdef OPT_01_CLI_LONG_TXT_B
          {OPT_01_CLI_LONG_TXT_B, OPT_01_HAS_ARG_LONG, 0, OPT_01_ENUM_NAME},
    #endif
    #ifdef OPT_01_CLI_LONG_TXT_C
          {OPT_01_CLI_LONG_TXT_C, OPT_01_HAS_ARG_LONG, 0, OPT_01_ENUM_NAME},
    #endif
    #ifdef OPT_01_CLI_LONG_TXT_D
          {OPT_01_CLI_LONG_TXT_D, OPT_01_HAS_ARG_LONG, 0, OPT_01_ENUM_NAME},
    #endif
    #ifdef OPT_01_CLI_SHORT_CODE_TXT
          {OPT_01_CLI_SHORT_CODE_TXT, OPT_01_HAS_ARG_LONG, 0, OPT_01_ENUM_NAME},
    #endif
#endif
#ifdef OPT_02_ENUM_NAME
    #ifdef OPT_02_CLI_LONG_TXT_A
          {OPT_02_CLI_LONG_TXT_A, OPT_02_HAS_ARG_LONG, 0, OPT_02_ENUM_NAME},
    #endif
    #ifdef OPT_02_CLI_LONG_TXT_B
          {OPT_02_CLI_LONG_TXT_B, OPT_02_HAS_ARG_LONG, 0, OPT_02_ENUM_NAME},
    #endif
    #ifdef OPT_02_CLI_LONG_TXT_C
          {OPT_02_CLI_LONG_TXT_C, OPT_02_HAS_ARG_LONG, 0, OPT_02_ENUM_NAME},
    #endif
    #ifdef OPT_02_CLI_LONG_TXT_D
          {OPT_02_CLI_LONG_TXT_D, OPT_02_HAS_ARG_LONG, 0, OPT_02_ENUM_NAME},
    #endif
    #ifdef OPT_02_CLI_SHORT_CODE_TXT
          {OPT_02_CLI_SHORT_CODE_TXT, OPT_02_HAS_ARG_LONG, 0, OPT_02_ENUM_NAME},
    #endif
#endif
#ifdef OPT_03_ENUM_NAME
    #ifdef OPT_03_CLI_LONG_TXT_A
          {OPT_03_CLI_LONG_TXT_A, OPT_03_HAS_ARG_LONG, 0, OPT_03_ENUM_NAME},
    #endif
    #ifdef OPT_03_CLI_LONG_TXT_B
          {OPT_03_CLI_LONG_TXT_B, OPT_03_HAS_ARG_LONG, 0, OPT_03_ENUM_NAME},
    #endif
    #ifdef OPT_03_CLI_LONG_TXT_C
          {OPT_03_CLI_LONG_TXT_C, OPT_03_HAS_ARG_LONG, 0, OPT_03_ENUM_NAME},
    #endif
    #ifdef OPT_03_CLI_LONG_TXT_D
          {OPT_03_CLI_LONG_TXT_D, OPT_03_HAS_ARG_LONG, 0, OPT_03_ENUM_NAME},
    #endif
    #ifdef OPT_03_CLI_SHORT_CODE_TXT
          {OPT_03_CLI_SHORT_CODE_TXT, OPT_03_HAS_ARG_LONG, 0, OPT_03_ENUM_NAME},
    #endif
#endif
#ifdef OPT_04_ENUM_NAME
    #ifdef OPT_04_CLI_LONG_TXT_A
          {OPT_04_CLI_LONG_TXT_A, OPT_04_HAS_ARG_LONG, 0, OPT_04_ENUM_NAME},
    #endif
    #ifdef OPT_04_CLI_LONG_TXT_B
          {OPT_04_CLI_LONG_TXT_B, OPT_04_HAS_ARG_LONG, 0, OPT_04_ENUM_NAME},
    #endif
    #ifdef OPT_04_CLI_LONG_TXT_C
          {OPT_04_CLI_LONG_TXT_C, OPT_04_HAS_ARG_LONG, 0, OPT_04_ENUM_NAME},
    #endif
    #ifdef OPT_04_CLI_LONG_TXT_D
          {OPT_04_CLI_LONG_TXT_D, OPT_04_HAS_ARG_LONG, 0, OPT_04_ENUM_NAME},
    #endif
    #ifdef OPT_04_CLI_SHORT_CODE_TXT
          {OPT_04_CLI_SHORT_CODE_TXT, OPT_04_HAS_ARG_LONG, 0, OPT_04_ENUM_NAME},
    #endif
#endif
#ifdef OPT_05_ENUM_NAME
    #ifdef OPT_05_CLI_LONG_TXT_A
          {OPT_05_CLI_LONG_TXT_A, OPT_05_HAS_ARG_LONG, 0, OPT_05_ENUM_NAME},
    #endif
    #ifdef OPT_05_CLI_LONG_TXT_B
          {OPT_05_CLI_LONG_TXT_B, OPT_05_HAS_ARG_LONG, 0, OPT_05_ENUM_NAME},
    #endif
    #ifdef OPT_05_CLI_LONG_TXT_C
          {OPT_05_CLI_LONG_TXT_C, OPT_05_HAS_ARG_LONG, 0, OPT_05_ENUM_NAME},
    #endif
    #ifdef OPT_05_CLI_LONG_TXT_D
          {OPT_05_CLI_LONG_TXT_D, OPT_05_HAS_ARG_LONG, 0, OPT_05_ENUM_NAME},
    #endif
    #ifdef OPT_05_CLI_SHORT_CODE_TXT
          {OPT_05_CLI_SHORT_CODE_TXT, OPT_05_HAS_ARG_LONG, 0, OPT_05_ENUM_NAME},
    #endif
#endif
#ifdef OPT_06_ENUM_NAME
    #ifdef OPT_06_CLI_LONG_TXT_A
          {OPT_06_CLI_LONG_TXT_A, OPT_06_HAS_ARG_LONG, 0, OPT_06_ENUM_NAME},
    #endif
    #ifdef OPT_06_CLI_LONG_TXT_B
          {OPT_06_CLI_LONG_TXT_B, OPT_06_HAS_ARG_LONG, 0, OPT_06_ENUM_NAME},
    #endif
    #ifdef OPT_06_CLI_LONG_TXT_C
          {OPT_06_CLI_LONG_TXT_C, OPT_06_HAS_ARG_LONG, 0, OPT_06_ENUM_NAME},
    #endif
    #ifdef OPT_06_CLI_LONG_TXT_D
          {OPT_06_CLI_LONG_TXT_D, OPT_06_HAS_ARG_LONG, 0, OPT_06_ENUM_NAME},
    #endif
    #ifdef OPT_06_CLI_SHORT_CODE_TXT
          {OPT_06_CLI_SHORT_CODE_TXT, OPT_06_HAS_ARG_LONG, 0, OPT_06_ENUM_NAME},
    #endif
#endif
#ifdef OPT_07_ENUM_NAME
    #ifdef OPT_07_CLI_LONG_TXT_A
          {OPT_07_CLI_LONG_TXT_A, OPT_07_HAS_ARG_LONG, 0, OPT_07_ENUM_NAME},
    #endif
    #ifdef OPT_07_CLI_LONG_TXT_B
          {OPT_07_CLI_LONG_TXT_B, OPT_07_HAS_ARG_LONG, 0, OPT_07_ENUM_NAME},
    #endif
    #ifdef OPT_07_CLI_LONG_TXT_C
          {OPT_07_CLI_LONG_TXT_C, OPT_07_HAS_ARG_LONG, 0, OPT_07_ENUM_NAME},
    #endif
    #ifdef OPT_07_CLI_LONG_TXT_D
          {OPT_07_CLI_LONG_TXT_D, OPT_07_HAS_ARG_LONG, 0, OPT_07_ENUM_NAME},
    #endif
    #ifdef OPT_07_CLI_SHORT_CODE_TXT
          {OPT_07_CLI_SHORT_CODE_TXT, OPT_07_HAS_ARG_LONG, 0, OPT_07_ENUM_NAME},
    #endif
#endif
#ifdef OPT_08_ENUM_NAME
    #ifdef OPT_08_CLI_LONG_TXT_A
          {OPT_08_CLI_LONG_TXT_A, OPT_08_HAS_ARG_LONG, 0, OPT_08_ENUM_NAME},
    #endif
    #ifdef OPT_08_CLI_LONG_TXT_B
          {OPT_08_CLI_LONG_TXT_B, OPT_08_HAS_ARG_LONG, 0, OPT_08_ENUM_NAME},
    #endif
    #ifdef OPT_08_CLI_LONG_TXT_C
          {OPT_08_CLI_LONG_TXT_C, OPT_08_HAS_ARG_LONG, 0, OPT_08_ENUM_NAME},
    #endif
    #ifdef OPT_08_CLI_LONG_TXT_D
          {OPT_08_CLI_LONG_TXT_D, OPT_08_HAS_ARG_LONG, 0, OPT_08_ENUM_NAME},
    #endif
    #ifdef OPT_08_CLI_SHORT_CODE_TXT
          {OPT_08_CLI_SHORT_CODE_TXT, OPT_08_HAS_ARG_LONG, 0, OPT_08_ENUM_NAME},
    #endif
#endif
#ifdef OPT_09_ENUM_NAME
    #ifdef OPT_09_CLI_LONG_TXT_A
          {OPT_09_CLI_LONG_TXT_A, OPT_09_HAS_ARG_LONG, 0, OPT_09_ENUM_NAME},
    #endif
    #ifdef OPT_09_CLI_LONG_TXT_B
          {OPT_09_CLI_LONG_TXT_B, OPT_09_HAS_ARG_LONG, 0, OPT_09_ENUM_NAME},
    #endif
    #ifdef OPT_09_CLI_LONG_TXT_C
          {OPT_09_CLI_LONG_TXT_C, OPT_09_HAS_ARG_LONG, 0, OPT_09_ENUM_NAME},
    #endif
    #ifdef OPT_09_CLI_LONG_TXT_D
          {OPT_09_CLI_LONG_TXT_D, OPT_09_HAS_ARG_LONG, 0, OPT_09_ENUM_NAME},
    #endif
    #ifdef OPT_09_CLI_SHORT_CODE_TXT
          {OPT_09_CLI_SHORT_CODE_TXT, OPT_09_HAS_ARG_LONG, 0, OPT_09_ENUM_NAME},
    #endif
#endif

#ifdef OPT_10_ENUM_NAME
    #ifdef OPT_10_CLI_LONG_TXT_A
          {OPT_10_CLI_LONG_TXT_A, OPT_10_HAS_ARG_LONG, 0, OPT_10_ENUM_NAME},
    #endif
    #ifdef OPT_10_CLI_LONG_TXT_B
          {OPT_10_CLI_LONG_TXT_B, OPT_10_HAS_ARG_LONG, 0, OPT_10_ENUM_NAME},
    #endif
    #ifdef OPT_10_CLI_LONG_TXT_C
          {OPT_10_CLI_LONG_TXT_C, OPT_10_HAS_ARG_LONG, 0, OPT_10_ENUM_NAME},
    #endif
    #ifdef OPT_10_CLI_LONG_TXT_D
          {OPT_10_CLI_LONG_TXT_D, OPT_10_HAS_ARG_LONG, 0, OPT_10_ENUM_NAME},
    #endif
    #ifdef OPT_10_CLI_SHORT_CODE_TXT
          {OPT_10_CLI_SHORT_CODE_TXT, OPT_10_HAS_ARG_LONG, 0, OPT_10_ENUM_NAME},
    #endif
#endif
#ifdef OPT_11_ENUM_NAME
    #ifdef OPT_11_CLI_LONG_TXT_A
          {OPT_11_CLI_LONG_TXT_A, OPT_11_HAS_ARG_LONG, 0, OPT_11_ENUM_NAME},
    #endif
    #ifdef OPT_11_CLI_LONG_TXT_B
          {OPT_11_CLI_LONG_TXT_B, OPT_11_HAS_ARG_LONG, 0, OPT_11_ENUM_NAME},
    #endif
    #ifdef OPT_11_CLI_LONG_TXT_C
          {OPT_11_CLI_LONG_TXT_C, OPT_11_HAS_ARG_LONG, 0, OPT_11_ENUM_NAME},
    #endif
    #ifdef OPT_11_CLI_LONG_TXT_D
          {OPT_11_CLI_LONG_TXT_D, OPT_11_HAS_ARG_LONG, 0, OPT_11_ENUM_NAME},
    #endif
    #ifdef OPT_11_CLI_SHORT_CODE_TXT
          {OPT_11_CLI_SHORT_CODE_TXT, OPT_11_HAS_ARG_LONG, 0, OPT_11_ENUM_NAME},
    #endif
#endif
#ifdef OPT_12_ENUM_NAME
    #ifdef OPT_12_CLI_LONG_TXT_A
          {OPT_12_CLI_LONG_TXT_A, OPT_12_HAS_ARG_LONG, 0, OPT_12_ENUM_NAME},
    #endif
    #ifdef OPT_12_CLI_LONG_TXT_B
          {OPT_12_CLI_LONG_TXT_B, OPT_12_HAS_ARG_LONG, 0, OPT_12_ENUM_NAME},
    #endif
    #ifdef OPT_12_CLI_LONG_TXT_C
          {OPT_12_CLI_LONG_TXT_C, OPT_12_HAS_ARG_LONG, 0, OPT_12_ENUM_NAME},
    #endif
    #ifdef OPT_12_CLI_LONG_TXT_D
          {OPT_12_CLI_LONG_TXT_D, OPT_12_HAS_ARG_LONG, 0, OPT_12_ENUM_NAME},
    #endif
    #ifdef OPT_12_CLI_SHORT_CODE_TXT
          {OPT_12_CLI_SHORT_CODE_TXT, OPT_12_HAS_ARG_LONG, 0, OPT_12_ENUM_NAME},
    #endif
#endif
#ifdef OPT_13_ENUM_NAME
    #ifdef OPT_13_CLI_LONG_TXT_A
          {OPT_13_CLI_LONG_TXT_A, OPT_13_HAS_ARG_LONG, 0, OPT_13_ENUM_NAME},
    #endif
    #ifdef OPT_13_CLI_LONG_TXT_B
          {OPT_13_CLI_LONG_TXT_B, OPT_13_HAS_ARG_LONG, 0, OPT_13_ENUM_NAME},
    #endif
    #ifdef OPT_13_CLI_LONG_TXT_C
          {OPT_13_CLI_LONG_TXT_C, OPT_13_HAS_ARG_LONG, 0, OPT_13_ENUM_NAME},
    #endif
    #ifdef OPT_13_CLI_LONG_TXT_D
          {OPT_13_CLI_LONG_TXT_D, OPT_13_HAS_ARG_LONG, 0, OPT_13_ENUM_NAME},
    #endif
    #ifdef OPT_13_CLI_SHORT_CODE_TXT
          {OPT_13_CLI_SHORT_CODE_TXT, OPT_13_HAS_ARG_LONG, 0, OPT_13_ENUM_NAME},
    #endif
#endif
#ifdef OPT_14_ENUM_NAME
    #ifdef OPT_14_CLI_LONG_TXT_A
          {OPT_14_CLI_LONG_TXT_A, OPT_14_HAS_ARG_LONG, 0, OPT_14_ENUM_NAME},
    #endif
    #ifdef OPT_14_CLI_LONG_TXT_B
          {OPT_14_CLI_LONG_TXT_B, OPT_14_HAS_ARG_LONG, 0, OPT_14_ENUM_NAME},
    #endif
    #ifdef OPT_14_CLI_LONG_TXT_C
          {OPT_14_CLI_LONG_TXT_C, OPT_14_HAS_ARG_LONG, 0, OPT_14_ENUM_NAME},
    #endif
    #ifdef OPT_14_CLI_LONG_TXT_D
          {OPT_14_CLI_LONG_TXT_D, OPT_14_HAS_ARG_LONG, 0, OPT_14_ENUM_NAME},
    #endif
    #ifdef OPT_14_CLI_SHORT_CODE_TXT
          {OPT_14_CLI_SHORT_CODE_TXT, OPT_14_HAS_ARG_LONG, 0, OPT_14_ENUM_NAME},
    #endif
#endif
#ifdef OPT_15_ENUM_NAME
    #ifdef OPT_15_CLI_LONG_TXT_A
          {OPT_15_CLI_LONG_TXT_A, OPT_15_HAS_ARG_LONG, 0, OPT_15_ENUM_NAME},
    #endif
    #ifdef OPT_15_CLI_LONG_TXT_B
          {OPT_15_CLI_LONG_TXT_B, OPT_15_HAS_ARG_LONG, 0, OPT_15_ENUM_NAME},
    #endif
    #ifdef OPT_15_CLI_LONG_TXT_C
          {OPT_15_CLI_LONG_TXT_C, OPT_15_HAS_ARG_LONG, 0, OPT_15_ENUM_NAME},
    #endif
    #ifdef OPT_15_CLI_LONG_TXT_D
          {OPT_15_CLI_LONG_TXT_D, OPT_15_HAS_ARG_LONG, 0, OPT_15_ENUM_NAME},
    #endif
    #ifdef OPT_15_CLI_SHORT_CODE_TXT
          {OPT_15_CLI_SHORT_CODE_TXT, OPT_15_HAS_ARG_LONG, 0, OPT_15_ENUM_NAME},
    #endif
#endif
#ifdef OPT_16_ENUM_NAME
    #ifdef OPT_16_CLI_LONG_TXT_A
          {OPT_16_CLI_LONG_TXT_A, OPT_16_HAS_ARG_LONG, 0, OPT_16_ENUM_NAME},
    #endif
    #ifdef OPT_16_CLI_LONG_TXT_B
          {OPT_16_CLI_LONG_TXT_B, OPT_16_HAS_ARG_LONG, 0, OPT_16_ENUM_NAME},
    #endif
    #ifdef OPT_16_CLI_LONG_TXT_C
          {OPT_16_CLI_LONG_TXT_C, OPT_16_HAS_ARG_LONG, 0, OPT_16_ENUM_NAME},
    #endif
    #ifdef OPT_16_CLI_LONG_TXT_D
          {OPT_16_CLI_LONG_TXT_D, OPT_16_HAS_ARG_LONG, 0, OPT_16_ENUM_NAME},
    #endif
    #ifdef OPT_16_CLI_SHORT_CODE_TXT
          {OPT_16_CLI_SHORT_CODE_TXT, OPT_16_HAS_ARG_LONG, 0, OPT_16_ENUM_NAME},
    #endif
#endif
#ifdef OPT_17_ENUM_NAME
    #ifdef OPT_17_CLI_LONG_TXT_A
          {OPT_17_CLI_LONG_TXT_A, OPT_17_HAS_ARG_LONG, 0, OPT_17_ENUM_NAME},
    #endif
    #ifdef OPT_17_CLI_LONG_TXT_B
          {OPT_17_CLI_LONG_TXT_B, OPT_17_HAS_ARG_LONG, 0, OPT_17_ENUM_NAME},
    #endif
    #ifdef OPT_17_CLI_LONG_TXT_C
          {OPT_17_CLI_LONG_TXT_C, OPT_17_HAS_ARG_LONG, 0, OPT_17_ENUM_NAME},
    #endif
    #ifdef OPT_17_CLI_LONG_TXT_D
          {OPT_17_CLI_LONG_TXT_D, OPT_17_HAS_ARG_LONG, 0, OPT_17_ENUM_NAME},
    #endif
    #ifdef OPT_17_CLI_SHORT_CODE_TXT
          {OPT_17_CLI_SHORT_CODE_TXT, OPT_17_HAS_ARG_LONG, 0, OPT_17_ENUM_NAME},
    #endif
#endif
#ifdef OPT_18_ENUM_NAME
    #ifdef OPT_18_CLI_LONG_TXT_A
          {OPT_18_CLI_LONG_TXT_A, OPT_18_HAS_ARG_LONG, 0, OPT_18_ENUM_NAME},
    #endif
    #ifdef OPT_18_CLI_LONG_TXT_B
          {OPT_18_CLI_LONG_TXT_B, OPT_18_HAS_ARG_LONG, 0, OPT_18_ENUM_NAME},
    #endif
    #ifdef OPT_18_CLI_LONG_TXT_C
          {OPT_18_CLI_LONG_TXT_C, OPT_18_HAS_ARG_LONG, 0, OPT_18_ENUM_NAME},
    #endif
    #ifdef OPT_18_CLI_LONG_TXT_D
          {OPT_18_CLI_LONG_TXT_D, OPT_18_HAS_ARG_LONG, 0, OPT_18_ENUM_NAME},
    #endif
    #ifdef OPT_18_CLI_SHORT_CODE_TXT
          {OPT_18_CLI_SHORT_CODE_TXT, OPT_18_HAS_ARG_LONG, 0, OPT_18_ENUM_NAME},
    #endif
#endif
#ifdef OPT_19_ENUM_NAME
    #ifdef OPT_19_CLI_LONG_TXT_A
          {OPT_19_CLI_LONG_TXT_A, OPT_19_HAS_ARG_LONG, 0, OPT_19_ENUM_NAME},
    #endif
    #ifdef OPT_19_CLI_LONG_TXT_B
          {OPT_19_CLI_LONG_TXT_B, OPT_19_HAS_ARG_LONG, 0, OPT_19_ENUM_NAME},
    #endif
    #ifdef OPT_19_CLI_LONG_TXT_C
          {OPT_19_CLI_LONG_TXT_C, OPT_19_HAS_ARG_LONG, 0, OPT_19_ENUM_NAME},
    #endif
    #ifdef OPT_19_CLI_LONG_TXT_D
          {OPT_19_CLI_LONG_TXT_D, OPT_19_HAS_ARG_LONG, 0, OPT_19_ENUM_NAME},
    #endif
    #ifdef OPT_19_CLI_SHORT_CODE_TXT
          {OPT_19_CLI_SHORT_CODE_TXT, OPT_19_HAS_ARG_LONG, 0, OPT_19_ENUM_NAME},
    #endif
#endif

#ifdef OPT_20_ENUM_NAME
    #ifdef OPT_20_CLI_LONG_TXT_A
          {OPT_20_CLI_LONG_TXT_A, OPT_20_HAS_ARG_LONG, 0, OPT_20_ENUM_NAME},
    #endif
    #ifdef OPT_20_CLI_LONG_TXT_B
          {OPT_20_CLI_LONG_TXT_B, OPT_20_HAS_ARG_LONG, 0, OPT_20_ENUM_NAME},
    #endif
    #ifdef OPT_20_CLI_LONG_TXT_C
          {OPT_20_CLI_LONG_TXT_C, OPT_20_HAS_ARG_LONG, 0, OPT_20_ENUM_NAME},
    #endif
    #ifdef OPT_20_CLI_LONG_TXT_D
          {OPT_20_CLI_LONG_TXT_D, OPT_20_HAS_ARG_LONG, 0, OPT_20_ENUM_NAME},
    #endif
    #ifdef OPT_20_CLI_SHORT_CODE_TXT
          {OPT_20_CLI_SHORT_CODE_TXT, OPT_20_HAS_ARG_LONG, 0, OPT_20_ENUM_NAME},
    #endif
#endif
#ifdef OPT_21_ENUM_NAME
    #ifdef OPT_21_CLI_LONG_TXT_A
          {OPT_21_CLI_LONG_TXT_A, OPT_21_HAS_ARG_LONG, 0, OPT_21_ENUM_NAME},
    #endif
    #ifdef OPT_21_CLI_LONG_TXT_B
          {OPT_21_CLI_LONG_TXT_B, OPT_21_HAS_ARG_LONG, 0, OPT_21_ENUM_NAME},
    #endif
    #ifdef OPT_21_CLI_LONG_TXT_C
          {OPT_21_CLI_LONG_TXT_C, OPT_21_HAS_ARG_LONG, 0, OPT_21_ENUM_NAME},
    #endif
    #ifdef OPT_21_CLI_LONG_TXT_D
          {OPT_21_CLI_LONG_TXT_D, OPT_21_HAS_ARG_LONG, 0, OPT_21_ENUM_NAME},
    #endif
    #ifdef OPT_21_CLI_SHORT_CODE_TXT
          {OPT_21_CLI_SHORT_CODE_TXT, OPT_21_HAS_ARG_LONG, 0, OPT_21_ENUM_NAME},
    #endif
#endif
#ifdef OPT_22_ENUM_NAME
    #ifdef OPT_22_CLI_LONG_TXT_A
          {OPT_22_CLI_LONG_TXT_A, OPT_22_HAS_ARG_LONG, 0, OPT_22_ENUM_NAME},
    #endif
    #ifdef OPT_22_CLI_LONG_TXT_B
          {OPT_22_CLI_LONG_TXT_B, OPT_22_HAS_ARG_LONG, 0, OPT_22_ENUM_NAME},
    #endif
    #ifdef OPT_22_CLI_LONG_TXT_C
          {OPT_22_CLI_LONG_TXT_C, OPT_22_HAS_ARG_LONG, 0, OPT_22_ENUM_NAME},
    #endif
    #ifdef OPT_22_CLI_LONG_TXT_D
          {OPT_22_CLI_LONG_TXT_D, OPT_22_HAS_ARG_LONG, 0, OPT_22_ENUM_NAME},
    #endif
    #ifdef OPT_22_CLI_SHORT_CODE_TXT
          {OPT_22_CLI_SHORT_CODE_TXT, OPT_22_HAS_ARG_LONG, 0, OPT_22_ENUM_NAME},
    #endif
#endif
#ifdef OPT_23_ENUM_NAME
    #ifdef OPT_23_CLI_LONG_TXT_A
          {OPT_23_CLI_LONG_TXT_A, OPT_23_HAS_ARG_LONG, 0, OPT_23_ENUM_NAME},
    #endif
    #ifdef OPT_23_CLI_LONG_TXT_B
          {OPT_23_CLI_LONG_TXT_B, OPT_23_HAS_ARG_LONG, 0, OPT_23_ENUM_NAME},
    #endif
    #ifdef OPT_23_CLI_LONG_TXT_C
          {OPT_23_CLI_LONG_TXT_C, OPT_23_HAS_ARG_LONG, 0, OPT_23_ENUM_NAME},
    #endif
    #ifdef OPT_23_CLI_LONG_TXT_D
          {OPT_23_CLI_LONG_TXT_D, OPT_23_HAS_ARG_LONG, 0, OPT_23_ENUM_NAME},
    #endif
    #ifdef OPT_23_CLI_SHORT_CODE_TXT
          {OPT_23_CLI_SHORT_CODE_TXT, OPT_23_HAS_ARG_LONG, 0, OPT_23_ENUM_NAME},
    #endif
#endif
#ifdef OPT_24_ENUM_NAME
    #ifdef OPT_24_CLI_LONG_TXT_A
          {OPT_24_CLI_LONG_TXT_A, OPT_24_HAS_ARG_LONG, 0, OPT_24_ENUM_NAME},
    #endif
    #ifdef OPT_24_CLI_LONG_TXT_B
          {OPT_24_CLI_LONG_TXT_B, OPT_24_HAS_ARG_LONG, 0, OPT_24_ENUM_NAME},
    #endif
    #ifdef OPT_24_CLI_LONG_TXT_C
          {OPT_24_CLI_LONG_TXT_C, OPT_24_HAS_ARG_LONG, 0, OPT_24_ENUM_NAME},
    #endif
    #ifdef OPT_24_CLI_LONG_TXT_D
          {OPT_24_CLI_LONG_TXT_D, OPT_24_HAS_ARG_LONG, 0, OPT_24_ENUM_NAME},
    #endif
    #ifdef OPT_24_CLI_SHORT_CODE_TXT
          {OPT_24_CLI_SHORT_CODE_TXT, OPT_24_HAS_ARG_LONG, 0, OPT_24_ENUM_NAME},
    #endif
#endif
#ifdef OPT_25_ENUM_NAME
    #ifdef OPT_25_CLI_LONG_TXT_A
          {OPT_25_CLI_LONG_TXT_A, OPT_25_HAS_ARG_LONG, 0, OPT_25_ENUM_NAME},
    #endif
    #ifdef OPT_25_CLI_LONG_TXT_B
          {OPT_25_CLI_LONG_TXT_B, OPT_25_HAS_ARG_LONG, 0, OPT_25_ENUM_NAME},
    #endif
    #ifdef OPT_25_CLI_LONG_TXT_C
          {OPT_25_CLI_LONG_TXT_C, OPT_25_HAS_ARG_LONG, 0, OPT_25_ENUM_NAME},
    #endif
    #ifdef OPT_25_CLI_LONG_TXT_D
          {OPT_25_CLI_LONG_TXT_D, OPT_25_HAS_ARG_LONG, 0, OPT_25_ENUM_NAME},
    #endif
    #ifdef OPT_25_CLI_SHORT_CODE_TXT
          {OPT_25_CLI_SHORT_CODE_TXT, OPT_25_HAS_ARG_LONG, 0, OPT_25_ENUM_NAME},
    #endif
#endif
#ifdef OPT_26_ENUM_NAME
    #ifdef OPT_26_CLI_LONG_TXT_A
          {OPT_26_CLI_LONG_TXT_A, OPT_26_HAS_ARG_LONG, 0, OPT_26_ENUM_NAME},
    #endif
    #ifdef OPT_26_CLI_LONG_TXT_B
          {OPT_26_CLI_LONG_TXT_B, OPT_26_HAS_ARG_LONG, 0, OPT_26_ENUM_NAME},
    #endif
    #ifdef OPT_26_CLI_LONG_TXT_C
          {OPT_26_CLI_LONG_TXT_C, OPT_26_HAS_ARG_LONG, 0, OPT_26_ENUM_NAME},
    #endif
    #ifdef OPT_26_CLI_LONG_TXT_D
          {OPT_26_CLI_LONG_TXT_D, OPT_26_HAS_ARG_LONG, 0, OPT_26_ENUM_NAME},
    #endif
    #ifdef OPT_26_CLI_SHORT_CODE_TXT
          {OPT_26_CLI_SHORT_CODE_TXT, OPT_26_HAS_ARG_LONG, 0, OPT_26_ENUM_NAME},
    #endif
#endif
#ifdef OPT_27_ENUM_NAME
    #ifdef OPT_27_CLI_LONG_TXT_A
          {OPT_27_CLI_LONG_TXT_A, OPT_27_HAS_ARG_LONG, 0, OPT_27_ENUM_NAME},
    #endif
    #ifdef OPT_27_CLI_LONG_TXT_B
          {OPT_27_CLI_LONG_TXT_B, OPT_27_HAS_ARG_LONG, 0, OPT_27_ENUM_NAME},
    #endif
    #ifdef OPT_27_CLI_LONG_TXT_C
          {OPT_27_CLI_LONG_TXT_C, OPT_27_HAS_ARG_LONG, 0, OPT_27_ENUM_NAME},
    #endif
    #ifdef OPT_27_CLI_LONG_TXT_D
          {OPT_27_CLI_LONG_TXT_D, OPT_27_HAS_ARG_LONG, 0, OPT_27_ENUM_NAME},
    #endif
    #ifdef OPT_27_CLI_SHORT_CODE_TXT
          {OPT_27_CLI_SHORT_CODE_TXT, OPT_27_HAS_ARG_LONG, 0, OPT_27_ENUM_NAME},
    #endif
#endif
#ifdef OPT_28_ENUM_NAME
    #ifdef OPT_28_CLI_LONG_TXT_A
          {OPT_28_CLI_LONG_TXT_A, OPT_28_HAS_ARG_LONG, 0, OPT_28_ENUM_NAME},
    #endif
    #ifdef OPT_28_CLI_LONG_TXT_B
          {OPT_28_CLI_LONG_TXT_B, OPT_28_HAS_ARG_LONG, 0, OPT_28_ENUM_NAME},
    #endif
    #ifdef OPT_28_CLI_LONG_TXT_C
          {OPT_28_CLI_LONG_TXT_C, OPT_28_HAS_ARG_LONG, 0, OPT_28_ENUM_NAME},
    #endif
    #ifdef OPT_28_CLI_LONG_TXT_D
          {OPT_28_CLI_LONG_TXT_D, OPT_28_HAS_ARG_LONG, 0, OPT_28_ENUM_NAME},
    #endif
    #ifdef OPT_28_CLI_SHORT_CODE_TXT
          {OPT_28_CLI_SHORT_CODE_TXT, OPT_28_HAS_ARG_LONG, 0, OPT_28_ENUM_NAME},
    #endif
#endif
#ifdef OPT_29_ENUM_NAME
    #ifdef OPT_29_CLI_LONG_TXT_A
          {OPT_29_CLI_LONG_TXT_A, OPT_29_HAS_ARG_LONG, 0, OPT_29_ENUM_NAME},
    #endif
    #ifdef OPT_29_CLI_LONG_TXT_B
          {OPT_29_CLI_LONG_TXT_B, OPT_29_HAS_ARG_LONG, 0, OPT_29_ENUM_NAME},
    #endif
    #ifdef OPT_29_CLI_LONG_TXT_C
          {OPT_29_CLI_LONG_TXT_C, OPT_29_HAS_ARG_LONG, 0, OPT_29_ENUM_NAME},
    #endif
    #ifdef OPT_29_CLI_LONG_TXT_D
          {OPT_29_CLI_LONG_TXT_D, OPT_29_HAS_ARG_LONG, 0, OPT_29_ENUM_NAME},
    #endif
    #ifdef OPT_29_CLI_SHORT_CODE_TXT
          {OPT_29_CLI_SHORT_CODE_TXT, OPT_29_HAS_ARG_LONG, 0, OPT_29_ENUM_NAME},
    #endif
#endif

#ifdef OPT_30_ENUM_NAME
    #ifdef OPT_30_CLI_LONG_TXT_A
          {OPT_30_CLI_LONG_TXT_A, OPT_30_HAS_ARG_LONG, 0, OPT_30_ENUM_NAME},
    #endif
    #ifdef OPT_30_CLI_LONG_TXT_B
          {OPT_30_CLI_LONG_TXT_B, OPT_30_HAS_ARG_LONG, 0, OPT_30_ENUM_NAME},
    #endif
    #ifdef OPT_30_CLI_LONG_TXT_C
          {OPT_30_CLI_LONG_TXT_C, OPT_30_HAS_ARG_LONG, 0, OPT_30_ENUM_NAME},
    #endif
    #ifdef OPT_30_CLI_LONG_TXT_D
          {OPT_30_CLI_LONG_TXT_D, OPT_30_HAS_ARG_LONG, 0, OPT_30_ENUM_NAME},
    #endif
    #ifdef OPT_30_CLI_SHORT_CODE_TXT
          {OPT_30_CLI_SHORT_CODE_TXT, OPT_30_HAS_ARG_LONG, 0, OPT_30_ENUM_NAME},
    #endif
#endif
#ifdef OPT_31_ENUM_NAME
    #ifdef OPT_31_CLI_LONG_TXT_A
          {OPT_31_CLI_LONG_TXT_A, OPT_31_HAS_ARG_LONG, 0, OPT_31_ENUM_NAME},
    #endif
    #ifdef OPT_31_CLI_LONG_TXT_B
          {OPT_31_CLI_LONG_TXT_B, OPT_31_HAS_ARG_LONG, 0, OPT_31_ENUM_NAME},
    #endif
    #ifdef OPT_31_CLI_LONG_TXT_C
          {OPT_31_CLI_LONG_TXT_C, OPT_31_HAS_ARG_LONG, 0, OPT_31_ENUM_NAME},
    #endif
    #ifdef OPT_31_CLI_LONG_TXT_D
          {OPT_31_CLI_LONG_TXT_D, OPT_31_HAS_ARG_LONG, 0, OPT_31_ENUM_NAME},
    #endif
    #ifdef OPT_31_CLI_SHORT_CODE_TXT
          {OPT_31_CLI_SHORT_CODE_TXT, OPT_31_HAS_ARG_LONG, 0, OPT_31_ENUM_NAME},
    #endif
#endif
#ifdef OPT_32_ENUM_NAME
    #ifdef OPT_32_CLI_LONG_TXT_A
          {OPT_32_CLI_LONG_TXT_A, OPT_32_HAS_ARG_LONG, 0, OPT_32_ENUM_NAME},
    #endif
    #ifdef OPT_32_CLI_LONG_TXT_B
          {OPT_32_CLI_LONG_TXT_B, OPT_32_HAS_ARG_LONG, 0, OPT_32_ENUM_NAME},
    #endif
    #ifdef OPT_32_CLI_LONG_TXT_C
          {OPT_32_CLI_LONG_TXT_C, OPT_32_HAS_ARG_LONG, 0, OPT_32_ENUM_NAME},
    #endif
    #ifdef OPT_32_CLI_LONG_TXT_D
          {OPT_32_CLI_LONG_TXT_D, OPT_32_HAS_ARG_LONG, 0, OPT_32_ENUM_NAME},
    #endif
    #ifdef OPT_32_CLI_SHORT_CODE_TXT
          {OPT_32_CLI_SHORT_CODE_TXT, OPT_32_HAS_ARG_LONG, 0, OPT_32_ENUM_NAME},
    #endif
#endif
#ifdef OPT_33_ENUM_NAME
    #ifdef OPT_33_CLI_LONG_TXT_A
          {OPT_33_CLI_LONG_TXT_A, OPT_33_HAS_ARG_LONG, 0, OPT_33_ENUM_NAME},
    #endif
    #ifdef OPT_33_CLI_LONG_TXT_B
          {OPT_33_CLI_LONG_TXT_B, OPT_33_HAS_ARG_LONG, 0, OPT_33_ENUM_NAME},
    #endif
    #ifdef OPT_33_CLI_LONG_TXT_C
          {OPT_33_CLI_LONG_TXT_C, OPT_33_HAS_ARG_LONG, 0, OPT_33_ENUM_NAME},
    #endif
    #ifdef OPT_33_CLI_LONG_TXT_D
          {OPT_33_CLI_LONG_TXT_D, OPT_33_HAS_ARG_LONG, 0, OPT_33_ENUM_NAME},
    #endif
    #ifdef OPT_33_CLI_SHORT_CODE_TXT
          {OPT_33_CLI_SHORT_CODE_TXT, OPT_33_HAS_ARG_LONG, 0, OPT_33_ENUM_NAME},
    #endif
#endif
#ifdef OPT_34_ENUM_NAME
    #ifdef OPT_34_CLI_LONG_TXT_A
          {OPT_34_CLI_LONG_TXT_A, OPT_34_HAS_ARG_LONG, 0, OPT_34_ENUM_NAME},
    #endif
    #ifdef OPT_34_CLI_LONG_TXT_B
          {OPT_34_CLI_LONG_TXT_B, OPT_34_HAS_ARG_LONG, 0, OPT_34_ENUM_NAME},
    #endif
    #ifdef OPT_34_CLI_LONG_TXT_C
          {OPT_34_CLI_LONG_TXT_C, OPT_34_HAS_ARG_LONG, 0, OPT_34_ENUM_NAME},
    #endif
    #ifdef OPT_34_CLI_LONG_TXT_D
          {OPT_34_CLI_LONG_TXT_D, OPT_34_HAS_ARG_LONG, 0, OPT_34_ENUM_NAME},
    #endif
    #ifdef OPT_34_CLI_SHORT_CODE_TXT
          {OPT_34_CLI_SHORT_CODE_TXT, OPT_34_HAS_ARG_LONG, 0, OPT_34_ENUM_NAME},
    #endif
#endif
#ifdef OPT_35_ENUM_NAME
    #ifdef OPT_35_CLI_LONG_TXT_A
          {OPT_35_CLI_LONG_TXT_A, OPT_35_HAS_ARG_LONG, 0, OPT_35_ENUM_NAME},
    #endif
    #ifdef OPT_35_CLI_LONG_TXT_B
          {OPT_35_CLI_LONG_TXT_B, OPT_35_HAS_ARG_LONG, 0, OPT_35_ENUM_NAME},
    #endif
    #ifdef OPT_35_CLI_LONG_TXT_C
          {OPT_35_CLI_LONG_TXT_C, OPT_35_HAS_ARG_LONG, 0, OPT_35_ENUM_NAME},
    #endif
    #ifdef OPT_35_CLI_LONG_TXT_D
          {OPT_35_CLI_LONG_TXT_D, OPT_35_HAS_ARG_LONG, 0, OPT_35_ENUM_NAME},
    #endif
    #ifdef OPT_35_CLI_SHORT_CODE_TXT
          {OPT_35_CLI_SHORT_CODE_TXT, OPT_35_HAS_ARG_LONG, 0, OPT_35_ENUM_NAME},
    #endif
#endif
#ifdef OPT_36_ENUM_NAME
    #ifdef OPT_36_CLI_LONG_TXT_A
          {OPT_36_CLI_LONG_TXT_A, OPT_36_HAS_ARG_LONG, 0, OPT_36_ENUM_NAME},
    #endif
    #ifdef OPT_36_CLI_LONG_TXT_B
          {OPT_36_CLI_LONG_TXT_B, OPT_36_HAS_ARG_LONG, 0, OPT_36_ENUM_NAME},
    #endif
    #ifdef OPT_36_CLI_LONG_TXT_C
          {OPT_36_CLI_LONG_TXT_C, OPT_36_HAS_ARG_LONG, 0, OPT_36_ENUM_NAME},
    #endif
    #ifdef OPT_36_CLI_LONG_TXT_D
          {OPT_36_CLI_LONG_TXT_D, OPT_36_HAS_ARG_LONG, 0, OPT_36_ENUM_NAME},
    #endif
    #ifdef OPT_36_CLI_SHORT_CODE_TXT
          {OPT_36_CLI_SHORT_CODE_TXT, OPT_36_HAS_ARG_LONG, 0, OPT_36_ENUM_NAME},
    #endif
#endif
#ifdef OPT_37_ENUM_NAME
    #ifdef OPT_37_CLI_LONG_TXT_A
          {OPT_37_CLI_LONG_TXT_A, OPT_37_HAS_ARG_LONG, 0, OPT_37_ENUM_NAME},
    #endif
    #ifdef OPT_37_CLI_LONG_TXT_B
          {OPT_37_CLI_LONG_TXT_B, OPT_37_HAS_ARG_LONG, 0, OPT_37_ENUM_NAME},
    #endif
    #ifdef OPT_37_CLI_LONG_TXT_C
          {OPT_37_CLI_LONG_TXT_C, OPT_37_HAS_ARG_LONG, 0, OPT_37_ENUM_NAME},
    #endif
    #ifdef OPT_37_CLI_LONG_TXT_D
          {OPT_37_CLI_LONG_TXT_D, OPT_37_HAS_ARG_LONG, 0, OPT_37_ENUM_NAME},
    #endif
    #ifdef OPT_37_CLI_SHORT_CODE_TXT
          {OPT_37_CLI_SHORT_CODE_TXT, OPT_37_HAS_ARG_LONG, 0, OPT_37_ENUM_NAME},
    #endif
#endif
#ifdef OPT_38_ENUM_NAME
    #ifdef OPT_38_CLI_LONG_TXT_A
          {OPT_38_CLI_LONG_TXT_A, OPT_38_HAS_ARG_LONG, 0, OPT_38_ENUM_NAME},
    #endif
    #ifdef OPT_38_CLI_LONG_TXT_B
          {OPT_38_CLI_LONG_TXT_B, OPT_38_HAS_ARG_LONG, 0, OPT_38_ENUM_NAME},
    #endif
    #ifdef OPT_38_CLI_LONG_TXT_C
          {OPT_38_CLI_LONG_TXT_C, OPT_38_HAS_ARG_LONG, 0, OPT_38_ENUM_NAME},
    #endif
    #ifdef OPT_38_CLI_LONG_TXT_D
          {OPT_38_CLI_LONG_TXT_D, OPT_38_HAS_ARG_LONG, 0, OPT_38_ENUM_NAME},
    #endif
    #ifdef OPT_38_CLI_SHORT_CODE_TXT
          {OPT_38_CLI_SHORT_CODE_TXT, OPT_38_HAS_ARG_LONG, 0, OPT_38_ENUM_NAME},
    #endif
#endif
#ifdef OPT_39_ENUM_NAME
    #ifdef OPT_39_CLI_LONG_TXT_A
          {OPT_39_CLI_LONG_TXT_A, OPT_39_HAS_ARG_LONG, 0, OPT_39_ENUM_NAME},
    #endif
    #ifdef OPT_39_CLI_LONG_TXT_B
          {OPT_39_CLI_LONG_TXT_B, OPT_39_HAS_ARG_LONG, 0, OPT_39_ENUM_NAME},
    #endif
    #ifdef OPT_39_CLI_LONG_TXT_C
          {OPT_39_CLI_LONG_TXT_C, OPT_39_HAS_ARG_LONG, 0, OPT_39_ENUM_NAME},
    #endif
    #ifdef OPT_39_CLI_LONG_TXT_D
          {OPT_39_CLI_LONG_TXT_D, OPT_39_HAS_ARG_LONG, 0, OPT_39_ENUM_NAME},
    #endif
    #ifdef OPT_39_CLI_SHORT_CODE_TXT
          {OPT_39_CLI_SHORT_CODE_TXT, OPT_39_HAS_ARG_LONG, 0, OPT_39_ENUM_NAME},
    #endif
#endif

#ifdef OPT_40_ENUM_NAME
    #ifdef OPT_40_CLI_LONG_TXT_A
          {OPT_40_CLI_LONG_TXT_A, OPT_40_HAS_ARG_LONG, 0, OPT_40_ENUM_NAME},
    #endif
    #ifdef OPT_40_CLI_LONG_TXT_B
          {OPT_40_CLI_LONG_TXT_B, OPT_40_HAS_ARG_LONG, 0, OPT_40_ENUM_NAME},
    #endif
    #ifdef OPT_40_CLI_LONG_TXT_C
          {OPT_40_CLI_LONG_TXT_C, OPT_40_HAS_ARG_LONG, 0, OPT_40_ENUM_NAME},
    #endif
    #ifdef OPT_40_CLI_LONG_TXT_D
          {OPT_40_CLI_LONG_TXT_D, OPT_40_HAS_ARG_LONG, 0, OPT_40_ENUM_NAME},
    #endif
    #ifdef OPT_40_CLI_SHORT_CODE_TXT
          {OPT_40_CLI_SHORT_CODE_TXT, OPT_40_HAS_ARG_LONG, 0, OPT_40_ENUM_NAME},
    #endif
#endif
#ifdef OPT_41_ENUM_NAME
    #ifdef OPT_41_CLI_LONG_TXT_A
          {OPT_41_CLI_LONG_TXT_A, OPT_41_HAS_ARG_LONG, 0, OPT_41_ENUM_NAME},
    #endif
    #ifdef OPT_41_CLI_LONG_TXT_B
          {OPT_41_CLI_LONG_TXT_B, OPT_41_HAS_ARG_LONG, 0, OPT_41_ENUM_NAME},
    #endif
    #ifdef OPT_41_CLI_LONG_TXT_C
          {OPT_41_CLI_LONG_TXT_C, OPT_41_HAS_ARG_LONG, 0, OPT_41_ENUM_NAME},
    #endif
    #ifdef OPT_41_CLI_LONG_TXT_D
          {OPT_41_CLI_LONG_TXT_D, OPT_41_HAS_ARG_LONG, 0, OPT_41_ENUM_NAME},
    #endif
    #ifdef OPT_41_CLI_SHORT_CODE_TXT
          {OPT_41_CLI_SHORT_CODE_TXT, OPT_41_HAS_ARG_LONG, 0, OPT_41_ENUM_NAME},
    #endif
#endif
#ifdef OPT_42_ENUM_NAME
    #ifdef OPT_42_CLI_LONG_TXT_A
          {OPT_42_CLI_LONG_TXT_A, OPT_42_HAS_ARG_LONG, 0, OPT_42_ENUM_NAME},
    #endif
    #ifdef OPT_42_CLI_LONG_TXT_B
          {OPT_42_CLI_LONG_TXT_B, OPT_42_HAS_ARG_LONG, 0, OPT_42_ENUM_NAME},
    #endif
    #ifdef OPT_42_CLI_LONG_TXT_C
          {OPT_42_CLI_LONG_TXT_C, OPT_42_HAS_ARG_LONG, 0, OPT_42_ENUM_NAME},
    #endif
    #ifdef OPT_42_CLI_LONG_TXT_D
          {OPT_42_CLI_LONG_TXT_D, OPT_42_HAS_ARG_LONG, 0, OPT_42_ENUM_NAME},
    #endif
    #ifdef OPT_42_CLI_SHORT_CODE_TXT
          {OPT_42_CLI_SHORT_CODE_TXT, OPT_42_HAS_ARG_LONG, 0, OPT_42_ENUM_NAME},
    #endif
#endif
#ifdef OPT_43_ENUM_NAME
    #ifdef OPT_43_CLI_LONG_TXT_A
          {OPT_43_CLI_LONG_TXT_A, OPT_43_HAS_ARG_LONG, 0, OPT_43_ENUM_NAME},
    #endif
    #ifdef OPT_43_CLI_LONG_TXT_B
          {OPT_43_CLI_LONG_TXT_B, OPT_43_HAS_ARG_LONG, 0, OPT_43_ENUM_NAME},
    #endif
    #ifdef OPT_43_CLI_LONG_TXT_C
          {OPT_43_CLI_LONG_TXT_C, OPT_43_HAS_ARG_LONG, 0, OPT_43_ENUM_NAME},
    #endif
    #ifdef OPT_43_CLI_LONG_TXT_D
          {OPT_43_CLI_LONG_TXT_D, OPT_43_HAS_ARG_LONG, 0, OPT_43_ENUM_NAME},
    #endif
    #ifdef OPT_43_CLI_SHORT_CODE_TXT
          {OPT_43_CLI_SHORT_CODE_TXT, OPT_43_HAS_ARG_LONG, 0, OPT_43_ENUM_NAME},
    #endif
#endif
#ifdef OPT_44_ENUM_NAME
    #ifdef OPT_44_CLI_LONG_TXT_A
          {OPT_44_CLI_LONG_TXT_A, OPT_44_HAS_ARG_LONG, 0, OPT_44_ENUM_NAME},
    #endif
    #ifdef OPT_44_CLI_LONG_TXT_B
          {OPT_44_CLI_LONG_TXT_B, OPT_44_HAS_ARG_LONG, 0, OPT_44_ENUM_NAME},
    #endif
    #ifdef OPT_44_CLI_LONG_TXT_C
          {OPT_44_CLI_LONG_TXT_C, OPT_44_HAS_ARG_LONG, 0, OPT_44_ENUM_NAME},
    #endif
    #ifdef OPT_44_CLI_LONG_TXT_D
          {OPT_44_CLI_LONG_TXT_D, OPT_44_HAS_ARG_LONG, 0, OPT_44_ENUM_NAME},
    #endif
    #ifdef OPT_44_CLI_SHORT_CODE_TXT
          {OPT_44_CLI_SHORT_CODE_TXT, OPT_44_HAS_ARG_LONG, 0, OPT_44_ENUM_NAME},
    #endif
#endif
#ifdef OPT_45_ENUM_NAME
    #ifdef OPT_45_CLI_LONG_TXT_A
          {OPT_45_CLI_LONG_TXT_A, OPT_45_HAS_ARG_LONG, 0, OPT_45_ENUM_NAME},
    #endif
    #ifdef OPT_45_CLI_LONG_TXT_B
          {OPT_45_CLI_LONG_TXT_B, OPT_45_HAS_ARG_LONG, 0, OPT_45_ENUM_NAME},
    #endif
    #ifdef OPT_45_CLI_LONG_TXT_C
          {OPT_45_CLI_LONG_TXT_C, OPT_45_HAS_ARG_LONG, 0, OPT_45_ENUM_NAME},
    #endif
    #ifdef OPT_45_CLI_LONG_TXT_D
          {OPT_45_CLI_LONG_TXT_D, OPT_45_HAS_ARG_LONG, 0, OPT_45_ENUM_NAME},
    #endif
    #ifdef OPT_45_CLI_SHORT_CODE_TXT
          {OPT_45_CLI_SHORT_CODE_TXT, OPT_45_HAS_ARG_LONG, 0, OPT_45_ENUM_NAME},
    #endif
#endif
#ifdef OPT_46_ENUM_NAME
    #ifdef OPT_46_CLI_LONG_TXT_A
          {OPT_46_CLI_LONG_TXT_A, OPT_46_HAS_ARG_LONG, 0, OPT_46_ENUM_NAME},
    #endif
    #ifdef OPT_46_CLI_LONG_TXT_B
          {OPT_46_CLI_LONG_TXT_B, OPT_46_HAS_ARG_LONG, 0, OPT_46_ENUM_NAME},
    #endif
    #ifdef OPT_46_CLI_LONG_TXT_C
          {OPT_46_CLI_LONG_TXT_C, OPT_46_HAS_ARG_LONG, 0, OPT_46_ENUM_NAME},
    #endif
    #ifdef OPT_46_CLI_LONG_TXT_D
          {OPT_46_CLI_LONG_TXT_D, OPT_46_HAS_ARG_LONG, 0, OPT_46_ENUM_NAME},
    #endif
    #ifdef OPT_46_CLI_SHORT_CODE_TXT
          {OPT_46_CLI_SHORT_CODE_TXT, OPT_46_HAS_ARG_LONG, 0, OPT_46_ENUM_NAME},
    #endif
#endif
#ifdef OPT_47_ENUM_NAME
    #ifdef OPT_47_CLI_LONG_TXT_A
          {OPT_47_CLI_LONG_TXT_A, OPT_47_HAS_ARG_LONG, 0, OPT_47_ENUM_NAME},
    #endif
    #ifdef OPT_47_CLI_LONG_TXT_B
          {OPT_47_CLI_LONG_TXT_B, OPT_47_HAS_ARG_LONG, 0, OPT_47_ENUM_NAME},
    #endif
    #ifdef OPT_47_CLI_LONG_TXT_C
          {OPT_47_CLI_LONG_TXT_C, OPT_47_HAS_ARG_LONG, 0, OPT_47_ENUM_NAME},
    #endif
    #ifdef OPT_47_CLI_LONG_TXT_D
          {OPT_47_CLI_LONG_TXT_D, OPT_47_HAS_ARG_LONG, 0, OPT_47_ENUM_NAME},
    #endif
    #ifdef OPT_47_CLI_SHORT_CODE_TXT
          {OPT_47_CLI_SHORT_CODE_TXT, OPT_47_HAS_ARG_LONG, 0, OPT_47_ENUM_NAME},
    #endif
#endif
#ifdef OPT_48_ENUM_NAME
    #ifdef OPT_48_CLI_LONG_TXT_A
          {OPT_48_CLI_LONG_TXT_A, OPT_48_HAS_ARG_LONG, 0, OPT_48_ENUM_NAME},
    #endif
    #ifdef OPT_48_CLI_LONG_TXT_B
          {OPT_48_CLI_LONG_TXT_B, OPT_48_HAS_ARG_LONG, 0, OPT_48_ENUM_NAME},
    #endif
    #ifdef OPT_48_CLI_LONG_TXT_C
          {OPT_48_CLI_LONG_TXT_C, OPT_48_HAS_ARG_LONG, 0, OPT_48_ENUM_NAME},
    #endif
    #ifdef OPT_48_CLI_LONG_TXT_D
          {OPT_48_CLI_LONG_TXT_D, OPT_48_HAS_ARG_LONG, 0, OPT_48_ENUM_NAME},
    #endif
    #ifdef OPT_48_CLI_SHORT_CODE_TXT
          {OPT_48_CLI_SHORT_CODE_TXT, OPT_48_HAS_ARG_LONG, 0, OPT_48_ENUM_NAME},
    #endif
#endif
#ifdef OPT_49_ENUM_NAME
    #ifdef OPT_49_CLI_LONG_TXT_A
          {OPT_49_CLI_LONG_TXT_A, OPT_49_HAS_ARG_LONG, 0, OPT_49_ENUM_NAME},
    #endif
    #ifdef OPT_49_CLI_LONG_TXT_B
          {OPT_49_CLI_LONG_TXT_B, OPT_49_HAS_ARG_LONG, 0, OPT_49_ENUM_NAME},
    #endif
    #ifdef OPT_49_CLI_LONG_TXT_C
          {OPT_49_CLI_LONG_TXT_C, OPT_49_HAS_ARG_LONG, 0, OPT_49_ENUM_NAME},
    #endif
    #ifdef OPT_49_CLI_LONG_TXT_D
          {OPT_49_CLI_LONG_TXT_D, OPT_49_HAS_ARG_LONG, 0, OPT_49_ENUM_NAME},
    #endif
    #ifdef OPT_49_CLI_SHORT_CODE_TXT
          {OPT_49_CLI_SHORT_CODE_TXT, OPT_49_HAS_ARG_LONG, 0, OPT_49_ENUM_NAME},
    #endif
#endif

#ifdef OPT_50_ENUM_NAME
    #ifdef OPT_50_CLI_LONG_TXT_A
          {OPT_50_CLI_LONG_TXT_A, OPT_50_HAS_ARG_LONG, 0, OPT_50_ENUM_NAME},
    #endif
    #ifdef OPT_50_CLI_LONG_TXT_B
          {OPT_50_CLI_LONG_TXT_B, OPT_50_HAS_ARG_LONG, 0, OPT_50_ENUM_NAME},
    #endif
    #ifdef OPT_50_CLI_LONG_TXT_C
          {OPT_50_CLI_LONG_TXT_C, OPT_50_HAS_ARG_LONG, 0, OPT_50_ENUM_NAME},
    #endif
    #ifdef OPT_50_CLI_LONG_TXT_D
          {OPT_50_CLI_LONG_TXT_D, OPT_50_HAS_ARG_LONG, 0, OPT_50_ENUM_NAME},
    #endif
    #ifdef OPT_50_CLI_SHORT_CODE_TXT
          {OPT_50_CLI_SHORT_CODE_TXT, OPT_50_HAS_ARG_LONG, 0, OPT_50_ENUM_NAME},
    #endif
#endif
#ifdef OPT_51_ENUM_NAME
    #ifdef OPT_51_CLI_LONG_TXT_A
          {OPT_51_CLI_LONG_TXT_A, OPT_51_HAS_ARG_LONG, 0, OPT_51_ENUM_NAME},
    #endif
    #ifdef OPT_51_CLI_LONG_TXT_B
          {OPT_51_CLI_LONG_TXT_B, OPT_51_HAS_ARG_LONG, 0, OPT_51_ENUM_NAME},
    #endif
    #ifdef OPT_51_CLI_LONG_TXT_C
          {OPT_51_CLI_LONG_TXT_C, OPT_51_HAS_ARG_LONG, 0, OPT_51_ENUM_NAME},
    #endif
    #ifdef OPT_51_CLI_LONG_TXT_D
          {OPT_51_CLI_LONG_TXT_D, OPT_51_HAS_ARG_LONG, 0, OPT_51_ENUM_NAME},
    #endif
    #ifdef OPT_51_CLI_SHORT_CODE_TXT
          {OPT_51_CLI_SHORT_CODE_TXT, OPT_51_HAS_ARG_LONG, 0, OPT_51_ENUM_NAME},
    #endif
#endif
#ifdef OPT_52_ENUM_NAME
    #ifdef OPT_52_CLI_LONG_TXT_A
          {OPT_52_CLI_LONG_TXT_A, OPT_52_HAS_ARG_LONG, 0, OPT_52_ENUM_NAME},
    #endif
    #ifdef OPT_52_CLI_LONG_TXT_B
          {OPT_52_CLI_LONG_TXT_B, OPT_52_HAS_ARG_LONG, 0, OPT_52_ENUM_NAME},
    #endif
    #ifdef OPT_52_CLI_LONG_TXT_C
          {OPT_52_CLI_LONG_TXT_C, OPT_52_HAS_ARG_LONG, 0, OPT_52_ENUM_NAME},
    #endif
    #ifdef OPT_52_CLI_LONG_TXT_D
          {OPT_52_CLI_LONG_TXT_D, OPT_52_HAS_ARG_LONG, 0, OPT_52_ENUM_NAME},
    #endif
    #ifdef OPT_52_CLI_SHORT_CODE_TXT
          {OPT_52_CLI_SHORT_CODE_TXT, OPT_52_HAS_ARG_LONG, 0, OPT_52_ENUM_NAME},
    #endif
#endif
#ifdef OPT_53_ENUM_NAME
    #ifdef OPT_53_CLI_LONG_TXT_A
          {OPT_53_CLI_LONG_TXT_A, OPT_53_HAS_ARG_LONG, 0, OPT_53_ENUM_NAME},
    #endif
    #ifdef OPT_53_CLI_LONG_TXT_B
          {OPT_53_CLI_LONG_TXT_B, OPT_53_HAS_ARG_LONG, 0, OPT_53_ENUM_NAME},
    #endif
    #ifdef OPT_53_CLI_LONG_TXT_C
          {OPT_53_CLI_LONG_TXT_C, OPT_53_HAS_ARG_LONG, 0, OPT_53_ENUM_NAME},
    #endif
    #ifdef OPT_53_CLI_LONG_TXT_D
          {OPT_53_CLI_LONG_TXT_D, OPT_53_HAS_ARG_LONG, 0, OPT_53_ENUM_NAME},
    #endif
    #ifdef OPT_53_CLI_SHORT_CODE_TXT
          {OPT_53_CLI_SHORT_CODE_TXT, OPT_53_HAS_ARG_LONG, 0, OPT_53_ENUM_NAME},
    #endif
#endif
#ifdef OPT_54_ENUM_NAME
    #ifdef OPT_54_CLI_LONG_TXT_A
          {OPT_54_CLI_LONG_TXT_A, OPT_54_HAS_ARG_LONG, 0, OPT_54_ENUM_NAME},
    #endif
    #ifdef OPT_54_CLI_LONG_TXT_B
          {OPT_54_CLI_LONG_TXT_B, OPT_54_HAS_ARG_LONG, 0, OPT_54_ENUM_NAME},
    #endif
    #ifdef OPT_54_CLI_LONG_TXT_C
          {OPT_54_CLI_LONG_TXT_C, OPT_54_HAS_ARG_LONG, 0, OPT_54_ENUM_NAME},
    #endif
    #ifdef OPT_54_CLI_LONG_TXT_D
          {OPT_54_CLI_LONG_TXT_D, OPT_54_HAS_ARG_LONG, 0, OPT_54_ENUM_NAME},
    #endif
    #ifdef OPT_54_CLI_SHORT_CODE_TXT
          {OPT_54_CLI_SHORT_CODE_TXT, OPT_54_HAS_ARG_LONG, 0, OPT_54_ENUM_NAME},
    #endif
#endif
#ifdef OPT_55_ENUM_NAME
    #ifdef OPT_55_CLI_LONG_TXT_A
          {OPT_55_CLI_LONG_TXT_A, OPT_55_HAS_ARG_LONG, 0, OPT_55_ENUM_NAME},
    #endif
    #ifdef OPT_55_CLI_LONG_TXT_B
          {OPT_55_CLI_LONG_TXT_B, OPT_55_HAS_ARG_LONG, 0, OPT_55_ENUM_NAME},
    #endif
    #ifdef OPT_55_CLI_LONG_TXT_C
          {OPT_55_CLI_LONG_TXT_C, OPT_55_HAS_ARG_LONG, 0, OPT_55_ENUM_NAME},
    #endif
    #ifdef OPT_55_CLI_LONG_TXT_D
          {OPT_55_CLI_LONG_TXT_D, OPT_55_HAS_ARG_LONG, 0, OPT_55_ENUM_NAME},
    #endif
    #ifdef OPT_55_CLI_SHORT_CODE_TXT
          {OPT_55_CLI_SHORT_CODE_TXT, OPT_55_HAS_ARG_LONG, 0, OPT_55_ENUM_NAME},
    #endif
#endif
#ifdef OPT_56_ENUM_NAME
    #ifdef OPT_56_CLI_LONG_TXT_A
          {OPT_56_CLI_LONG_TXT_A, OPT_56_HAS_ARG_LONG, 0, OPT_56_ENUM_NAME},
    #endif
    #ifdef OPT_56_CLI_LONG_TXT_B
          {OPT_56_CLI_LONG_TXT_B, OPT_56_HAS_ARG_LONG, 0, OPT_56_ENUM_NAME},
    #endif
    #ifdef OPT_56_CLI_LONG_TXT_C
          {OPT_56_CLI_LONG_TXT_C, OPT_56_HAS_ARG_LONG, 0, OPT_56_ENUM_NAME},
    #endif
    #ifdef OPT_56_CLI_LONG_TXT_D
          {OPT_56_CLI_LONG_TXT_D, OPT_56_HAS_ARG_LONG, 0, OPT_56_ENUM_NAME},
    #endif
    #ifdef OPT_56_CLI_SHORT_CODE_TXT
          {OPT_56_CLI_SHORT_CODE_TXT, OPT_56_HAS_ARG_LONG, 0, OPT_56_ENUM_NAME},
    #endif
#endif
#ifdef OPT_57_ENUM_NAME
    #ifdef OPT_57_CLI_LONG_TXT_A
          {OPT_57_CLI_LONG_TXT_A, OPT_57_HAS_ARG_LONG, 0, OPT_57_ENUM_NAME},
    #endif
    #ifdef OPT_57_CLI_LONG_TXT_B
          {OPT_57_CLI_LONG_TXT_B, OPT_57_HAS_ARG_LONG, 0, OPT_57_ENUM_NAME},
    #endif
    #ifdef OPT_57_CLI_LONG_TXT_C
          {OPT_57_CLI_LONG_TXT_C, OPT_57_HAS_ARG_LONG, 0, OPT_57_ENUM_NAME},
    #endif
    #ifdef OPT_57_CLI_LONG_TXT_D
          {OPT_57_CLI_LONG_TXT_D, OPT_57_HAS_ARG_LONG, 0, OPT_57_ENUM_NAME},
    #endif
    #ifdef OPT_57_CLI_SHORT_CODE_TXT
          {OPT_57_CLI_SHORT_CODE_TXT, OPT_57_HAS_ARG_LONG, 0, OPT_57_ENUM_NAME},
    #endif
#endif
#ifdef OPT_58_ENUM_NAME
    #ifdef OPT_58_CLI_LONG_TXT_A
          {OPT_58_CLI_LONG_TXT_A, OPT_58_HAS_ARG_LONG, 0, OPT_58_ENUM_NAME},
    #endif
    #ifdef OPT_58_CLI_LONG_TXT_B
          {OPT_58_CLI_LONG_TXT_B, OPT_58_HAS_ARG_LONG, 0, OPT_58_ENUM_NAME},
    #endif
    #ifdef OPT_58_CLI_LONG_TXT_C
          {OPT_58_CLI_LONG_TXT_C, OPT_58_HAS_ARG_LONG, 0, OPT_58_ENUM_NAME},
    #endif
    #ifdef OPT_58_CLI_LONG_TXT_D
          {OPT_58_CLI_LONG_TXT_D, OPT_58_HAS_ARG_LONG, 0, OPT_58_ENUM_NAME},
    #endif
    #ifdef OPT_58_CLI_SHORT_CODE_TXT
          {OPT_58_CLI_SHORT_CODE_TXT, OPT_58_HAS_ARG_LONG, 0, OPT_58_ENUM_NAME},
    #endif
#endif
#ifdef OPT_59_ENUM_NAME
    #ifdef OPT_59_CLI_LONG_TXT_A
          {OPT_59_CLI_LONG_TXT_A, OPT_59_HAS_ARG_LONG, 0, OPT_59_ENUM_NAME},
    #endif
    #ifdef OPT_59_CLI_LONG_TXT_B
          {OPT_59_CLI_LONG_TXT_B, OPT_59_HAS_ARG_LONG, 0, OPT_59_ENUM_NAME},
    #endif
    #ifdef OPT_59_CLI_LONG_TXT_C
          {OPT_59_CLI_LONG_TXT_C, OPT_59_HAS_ARG_LONG, 0, OPT_59_ENUM_NAME},
    #endif
    #ifdef OPT_59_CLI_LONG_TXT_D
          {OPT_59_CLI_LONG_TXT_D, OPT_59_HAS_ARG_LONG, 0, OPT_59_ENUM_NAME},
    #endif
    #ifdef OPT_59_CLI_SHORT_CODE_TXT
          {OPT_59_CLI_SHORT_CODE_TXT, OPT_59_HAS_ARG_LONG, 0, OPT_59_ENUM_NAME},
    #endif
#endif

#ifdef OPT_60_ENUM_NAME
    #ifdef OPT_60_CLI_LONG_TXT_A
          {OPT_60_CLI_LONG_TXT_A, OPT_60_HAS_ARG_LONG, 0, OPT_60_ENUM_NAME},
    #endif
    #ifdef OPT_60_CLI_LONG_TXT_B
          {OPT_60_CLI_LONG_TXT_B, OPT_60_HAS_ARG_LONG, 0, OPT_60_ENUM_NAME},
    #endif
    #ifdef OPT_60_CLI_LONG_TXT_C
          {OPT_60_CLI_LONG_TXT_C, OPT_60_HAS_ARG_LONG, 0, OPT_60_ENUM_NAME},
    #endif
    #ifdef OPT_60_CLI_LONG_TXT_D
          {OPT_60_CLI_LONG_TXT_D, OPT_60_HAS_ARG_LONG, 0, OPT_60_ENUM_NAME},
    #endif
    #ifdef OPT_60_CLI_SHORT_CODE_TXT
          {OPT_60_CLI_SHORT_CODE_TXT, OPT_60_HAS_ARG_LONG, 0, OPT_60_ENUM_NAME},
    #endif
#endif
#ifdef OPT_61_ENUM_NAME
    #ifdef OPT_61_CLI_LONG_TXT_A
          {OPT_61_CLI_LONG_TXT_A, OPT_61_HAS_ARG_LONG, 0, OPT_61_ENUM_NAME},
    #endif
    #ifdef OPT_61_CLI_LONG_TXT_B
          {OPT_61_CLI_LONG_TXT_B, OPT_61_HAS_ARG_LONG, 0, OPT_61_ENUM_NAME},
    #endif
    #ifdef OPT_61_CLI_LONG_TXT_C
          {OPT_61_CLI_LONG_TXT_C, OPT_61_HAS_ARG_LONG, 0, OPT_61_ENUM_NAME},
    #endif
    #ifdef OPT_61_CLI_LONG_TXT_D
          {OPT_61_CLI_LONG_TXT_D, OPT_61_HAS_ARG_LONG, 0, OPT_61_ENUM_NAME},
    #endif
    #ifdef OPT_61_CLI_SHORT_CODE_TXT
          {OPT_61_CLI_SHORT_CODE_TXT, OPT_61_HAS_ARG_LONG, 0, OPT_61_ENUM_NAME},
    #endif
#endif
#ifdef OPT_62_ENUM_NAME
    #ifdef OPT_62_CLI_LONG_TXT_A
          {OPT_62_CLI_LONG_TXT_A, OPT_62_HAS_ARG_LONG, 0, OPT_62_ENUM_NAME},
    #endif
    #ifdef OPT_62_CLI_LONG_TXT_B
          {OPT_62_CLI_LONG_TXT_B, OPT_62_HAS_ARG_LONG, 0, OPT_62_ENUM_NAME},
    #endif
    #ifdef OPT_62_CLI_LONG_TXT_C
          {OPT_62_CLI_LONG_TXT_C, OPT_62_HAS_ARG_LONG, 0, OPT_62_ENUM_NAME},
    #endif
    #ifdef OPT_62_CLI_LONG_TXT_D
          {OPT_62_CLI_LONG_TXT_D, OPT_62_HAS_ARG_LONG, 0, OPT_62_ENUM_NAME},
    #endif
    #ifdef OPT_62_CLI_SHORT_CODE_TXT
          {OPT_62_CLI_SHORT_CODE_TXT, OPT_62_HAS_ARG_LONG, 0, OPT_62_ENUM_NAME},
    #endif
#endif
#ifdef OPT_63_ENUM_NAME
    #ifdef OPT_63_CLI_LONG_TXT_A
          {OPT_63_CLI_LONG_TXT_A, OPT_63_HAS_ARG_LONG, 0, OPT_63_ENUM_NAME},
    #endif
    #ifdef OPT_63_CLI_LONG_TXT_B
          {OPT_63_CLI_LONG_TXT_B, OPT_63_HAS_ARG_LONG, 0, OPT_63_ENUM_NAME},
    #endif
    #ifdef OPT_63_CLI_LONG_TXT_C
          {OPT_63_CLI_LONG_TXT_C, OPT_63_HAS_ARG_LONG, 0, OPT_63_ENUM_NAME},
    #endif
    #ifdef OPT_63_CLI_LONG_TXT_D
          {OPT_63_CLI_LONG_TXT_D, OPT_63_HAS_ARG_LONG, 0, OPT_63_ENUM_NAME},
    #endif
    #ifdef OPT_63_CLI_SHORT_CODE_TXT
          {OPT_63_CLI_SHORT_CODE_TXT, OPT_63_HAS_ARG_LONG, 0, OPT_63_ENUM_NAME},
    #endif
#endif
#ifdef OPT_64_ENUM_NAME
    #ifdef OPT_64_CLI_LONG_TXT_A
          {OPT_64_CLI_LONG_TXT_A, OPT_64_HAS_ARG_LONG, 0, OPT_64_ENUM_NAME},
    #endif
    #ifdef OPT_64_CLI_LONG_TXT_B
          {OPT_64_CLI_LONG_TXT_B, OPT_64_HAS_ARG_LONG, 0, OPT_64_ENUM_NAME},
    #endif
    #ifdef OPT_64_CLI_LONG_TXT_C
          {OPT_64_CLI_LONG_TXT_C, OPT_64_HAS_ARG_LONG, 0, OPT_64_ENUM_NAME},
    #endif
    #ifdef OPT_64_CLI_LONG_TXT_D
          {OPT_64_CLI_LONG_TXT_D, OPT_64_HAS_ARG_LONG, 0, OPT_64_ENUM_NAME},
    #endif
    #ifdef OPT_64_CLI_SHORT_CODE_TXT
          {OPT_64_CLI_SHORT_CODE_TXT, OPT_64_HAS_ARG_LONG, 0, OPT_64_ENUM_NAME},
    #endif
#endif
#ifdef OPT_65_ENUM_NAME
    #ifdef OPT_65_CLI_LONG_TXT_A
          {OPT_65_CLI_LONG_TXT_A, OPT_65_HAS_ARG_LONG, 0, OPT_65_ENUM_NAME},
    #endif
    #ifdef OPT_65_CLI_LONG_TXT_B
          {OPT_65_CLI_LONG_TXT_B, OPT_65_HAS_ARG_LONG, 0, OPT_65_ENUM_NAME},
    #endif
    #ifdef OPT_65_CLI_LONG_TXT_C
          {OPT_65_CLI_LONG_TXT_C, OPT_65_HAS_ARG_LONG, 0, OPT_65_ENUM_NAME},
    #endif
    #ifdef OPT_65_CLI_LONG_TXT_D
          {OPT_65_CLI_LONG_TXT_D, OPT_65_HAS_ARG_LONG, 0, OPT_65_ENUM_NAME},
    #endif
    #ifdef OPT_65_CLI_SHORT_CODE_TXT
          {OPT_65_CLI_SHORT_CODE_TXT, OPT_65_HAS_ARG_LONG, 0, OPT_65_ENUM_NAME},
    #endif
#endif
#ifdef OPT_66_ENUM_NAME
    #ifdef OPT_66_CLI_LONG_TXT_A
          {OPT_66_CLI_LONG_TXT_A, OPT_66_HAS_ARG_LONG, 0, OPT_66_ENUM_NAME},
    #endif
    #ifdef OPT_66_CLI_LONG_TXT_B
          {OPT_66_CLI_LONG_TXT_B, OPT_66_HAS_ARG_LONG, 0, OPT_66_ENUM_NAME},
    #endif
    #ifdef OPT_66_CLI_LONG_TXT_C
          {OPT_66_CLI_LONG_TXT_C, OPT_66_HAS_ARG_LONG, 0, OPT_66_ENUM_NAME},
    #endif
    #ifdef OPT_66_CLI_LONG_TXT_D
          {OPT_66_CLI_LONG_TXT_D, OPT_66_HAS_ARG_LONG, 0, OPT_66_ENUM_NAME},
    #endif
    #ifdef OPT_66_CLI_SHORT_CODE_TXT
          {OPT_66_CLI_SHORT_CODE_TXT, OPT_66_HAS_ARG_LONG, 0, OPT_66_ENUM_NAME},
    #endif
#endif
#ifdef OPT_67_ENUM_NAME
    #ifdef OPT_67_CLI_LONG_TXT_A
          {OPT_67_CLI_LONG_TXT_A, OPT_67_HAS_ARG_LONG, 0, OPT_67_ENUM_NAME},
    #endif
    #ifdef OPT_67_CLI_LONG_TXT_B
          {OPT_67_CLI_LONG_TXT_B, OPT_67_HAS_ARG_LONG, 0, OPT_67_ENUM_NAME},
    #endif
    #ifdef OPT_67_CLI_LONG_TXT_C
          {OPT_67_CLI_LONG_TXT_C, OPT_67_HAS_ARG_LONG, 0, OPT_67_ENUM_NAME},
    #endif
    #ifdef OPT_67_CLI_LONG_TXT_D
          {OPT_67_CLI_LONG_TXT_D, OPT_67_HAS_ARG_LONG, 0, OPT_67_ENUM_NAME},
    #endif
    #ifdef OPT_67_CLI_SHORT_CODE_TXT
          {OPT_67_CLI_SHORT_CODE_TXT, OPT_67_HAS_ARG_LONG, 0, OPT_67_ENUM_NAME},
    #endif
#endif
#ifdef OPT_68_ENUM_NAME
    #ifdef OPT_68_CLI_LONG_TXT_A
          {OPT_68_CLI_LONG_TXT_A, OPT_68_HAS_ARG_LONG, 0, OPT_68_ENUM_NAME},
    #endif
    #ifdef OPT_68_CLI_LONG_TXT_B
          {OPT_68_CLI_LONG_TXT_B, OPT_68_HAS_ARG_LONG, 0, OPT_68_ENUM_NAME},
    #endif
    #ifdef OPT_68_CLI_LONG_TXT_C
          {OPT_68_CLI_LONG_TXT_C, OPT_68_HAS_ARG_LONG, 0, OPT_68_ENUM_NAME},
    #endif
    #ifdef OPT_68_CLI_LONG_TXT_D
          {OPT_68_CLI_LONG_TXT_D, OPT_68_HAS_ARG_LONG, 0, OPT_68_ENUM_NAME},
    #endif
    #ifdef OPT_68_CLI_SHORT_CODE_TXT
          {OPT_68_CLI_SHORT_CODE_TXT, OPT_68_HAS_ARG_LONG, 0, OPT_68_ENUM_NAME},
    #endif
#endif
#ifdef OPT_69_ENUM_NAME
    #ifdef OPT_69_CLI_LONG_TXT_A
          {OPT_69_CLI_LONG_TXT_A, OPT_69_HAS_ARG_LONG, 0, OPT_69_ENUM_NAME},
    #endif
    #ifdef OPT_69_CLI_LONG_TXT_B
          {OPT_69_CLI_LONG_TXT_B, OPT_69_HAS_ARG_LONG, 0, OPT_69_ENUM_NAME},
    #endif
    #ifdef OPT_69_CLI_LONG_TXT_C
          {OPT_69_CLI_LONG_TXT_C, OPT_69_HAS_ARG_LONG, 0, OPT_69_ENUM_NAME},
    #endif
    #ifdef OPT_69_CLI_LONG_TXT_D
          {OPT_69_CLI_LONG_TXT_D, OPT_69_HAS_ARG_LONG, 0, OPT_69_ENUM_NAME},
    #endif
    #ifdef OPT_69_CLI_SHORT_CODE_TXT
          {OPT_69_CLI_SHORT_CODE_TXT, OPT_69_HAS_ARG_LONG, 0, OPT_69_ENUM_NAME},
    #endif
#endif

#ifdef OPT_70_ENUM_NAME
    #ifdef OPT_70_CLI_LONG_TXT_A
          {OPT_70_CLI_LONG_TXT_A, OPT_70_HAS_ARG_LONG, 0, OPT_70_ENUM_NAME},
    #endif
    #ifdef OPT_70_CLI_LONG_TXT_B
          {OPT_70_CLI_LONG_TXT_B, OPT_70_HAS_ARG_LONG, 0, OPT_70_ENUM_NAME},
    #endif
    #ifdef OPT_70_CLI_LONG_TXT_C
          {OPT_70_CLI_LONG_TXT_C, OPT_70_HAS_ARG_LONG, 0, OPT_70_ENUM_NAME},
    #endif
    #ifdef OPT_70_CLI_LONG_TXT_D
          {OPT_70_CLI_LONG_TXT_D, OPT_70_HAS_ARG_LONG, 0, OPT_70_ENUM_NAME},
    #endif
    #ifdef OPT_70_CLI_SHORT_CODE_TXT
          {OPT_70_CLI_SHORT_CODE_TXT, OPT_70_HAS_ARG_LONG, 0, OPT_70_ENUM_NAME},
    #endif
#endif
#ifdef OPT_71_ENUM_NAME
    #ifdef OPT_71_CLI_LONG_TXT_A
          {OPT_71_CLI_LONG_TXT_A, OPT_71_HAS_ARG_LONG, 0, OPT_71_ENUM_NAME},
    #endif
    #ifdef OPT_71_CLI_LONG_TXT_B
          {OPT_71_CLI_LONG_TXT_B, OPT_71_HAS_ARG_LONG, 0, OPT_71_ENUM_NAME},
    #endif
    #ifdef OPT_71_CLI_LONG_TXT_C
          {OPT_71_CLI_LONG_TXT_C, OPT_71_HAS_ARG_LONG, 0, OPT_71_ENUM_NAME},
    #endif
    #ifdef OPT_71_CLI_LONG_TXT_D
          {OPT_71_CLI_LONG_TXT_D, OPT_71_HAS_ARG_LONG, 0, OPT_71_ENUM_NAME},
    #endif
    #ifdef OPT_71_CLI_SHORT_CODE_TXT
          {OPT_71_CLI_SHORT_CODE_TXT, OPT_71_HAS_ARG_LONG, 0, OPT_71_ENUM_NAME},
    #endif
#endif
#ifdef OPT_72_ENUM_NAME
    #ifdef OPT_72_CLI_LONG_TXT_A
          {OPT_72_CLI_LONG_TXT_A, OPT_72_HAS_ARG_LONG, 0, OPT_72_ENUM_NAME},
    #endif
    #ifdef OPT_72_CLI_LONG_TXT_B
          {OPT_72_CLI_LONG_TXT_B, OPT_72_HAS_ARG_LONG, 0, OPT_72_ENUM_NAME},
    #endif
    #ifdef OPT_72_CLI_LONG_TXT_C
          {OPT_72_CLI_LONG_TXT_C, OPT_72_HAS_ARG_LONG, 0, OPT_72_ENUM_NAME},
    #endif
    #ifdef OPT_72_CLI_LONG_TXT_D
          {OPT_72_CLI_LONG_TXT_D, OPT_72_HAS_ARG_LONG, 0, OPT_72_ENUM_NAME},
    #endif
    #ifdef OPT_72_CLI_SHORT_CODE_TXT
          {OPT_72_CLI_SHORT_CODE_TXT, OPT_72_HAS_ARG_LONG, 0, OPT_72_ENUM_NAME},
    #endif
#endif
#ifdef OPT_73_ENUM_NAME
    #ifdef OPT_73_CLI_LONG_TXT_A
          {OPT_73_CLI_LONG_TXT_A, OPT_73_HAS_ARG_LONG, 0, OPT_73_ENUM_NAME},
    #endif
    #ifdef OPT_73_CLI_LONG_TXT_B
          {OPT_73_CLI_LONG_TXT_B, OPT_73_HAS_ARG_LONG, 0, OPT_73_ENUM_NAME},
    #endif
    #ifdef OPT_73_CLI_LONG_TXT_C
          {OPT_73_CLI_LONG_TXT_C, OPT_73_HAS_ARG_LONG, 0, OPT_73_ENUM_NAME},
    #endif
    #ifdef OPT_73_CLI_LONG_TXT_D
          {OPT_73_CLI_LONG_TXT_D, OPT_73_HAS_ARG_LONG, 0, OPT_73_ENUM_NAME},
    #endif
    #ifdef OPT_73_CLI_SHORT_CODE_TXT
          {OPT_73_CLI_SHORT_CODE_TXT, OPT_73_HAS_ARG_LONG, 0, OPT_73_ENUM_NAME},
    #endif
#endif
#ifdef OPT_74_ENUM_NAME
    #ifdef OPT_74_CLI_LONG_TXT_A
          {OPT_74_CLI_LONG_TXT_A, OPT_74_HAS_ARG_LONG, 0, OPT_74_ENUM_NAME},
    #endif
    #ifdef OPT_74_CLI_LONG_TXT_B
          {OPT_74_CLI_LONG_TXT_B, OPT_74_HAS_ARG_LONG, 0, OPT_74_ENUM_NAME},
    #endif
    #ifdef OPT_74_CLI_LONG_TXT_C
          {OPT_74_CLI_LONG_TXT_C, OPT_74_HAS_ARG_LONG, 0, OPT_74_ENUM_NAME},
    #endif
    #ifdef OPT_74_CLI_LONG_TXT_D
          {OPT_74_CLI_LONG_TXT_D, OPT_74_HAS_ARG_LONG, 0, OPT_74_ENUM_NAME},
    #endif
    #ifdef OPT_74_CLI_SHORT_CODE_TXT
          {OPT_74_CLI_SHORT_CODE_TXT, OPT_74_HAS_ARG_LONG, 0, OPT_74_ENUM_NAME},
    #endif
#endif
#ifdef OPT_75_ENUM_NAME
    #ifdef OPT_75_CLI_LONG_TXT_A
          {OPT_75_CLI_LONG_TXT_A, OPT_75_HAS_ARG_LONG, 0, OPT_75_ENUM_NAME},
    #endif
    #ifdef OPT_75_CLI_LONG_TXT_B
          {OPT_75_CLI_LONG_TXT_B, OPT_75_HAS_ARG_LONG, 0, OPT_75_ENUM_NAME},
    #endif
    #ifdef OPT_75_CLI_LONG_TXT_C
          {OPT_75_CLI_LONG_TXT_C, OPT_75_HAS_ARG_LONG, 0, OPT_75_ENUM_NAME},
    #endif
    #ifdef OPT_75_CLI_LONG_TXT_D
          {OPT_75_CLI_LONG_TXT_D, OPT_75_HAS_ARG_LONG, 0, OPT_75_ENUM_NAME},
    #endif
    #ifdef OPT_75_CLI_SHORT_CODE_TXT
          {OPT_75_CLI_SHORT_CODE_TXT, OPT_75_HAS_ARG_LONG, 0, OPT_75_ENUM_NAME},
    #endif
#endif
#ifdef OPT_76_ENUM_NAME
    #ifdef OPT_76_CLI_LONG_TXT_A
          {OPT_76_CLI_LONG_TXT_A, OPT_76_HAS_ARG_LONG, 0, OPT_76_ENUM_NAME},
    #endif
    #ifdef OPT_76_CLI_LONG_TXT_B
          {OPT_76_CLI_LONG_TXT_B, OPT_76_HAS_ARG_LONG, 0, OPT_76_ENUM_NAME},
    #endif
    #ifdef OPT_76_CLI_LONG_TXT_C
          {OPT_76_CLI_LONG_TXT_C, OPT_76_HAS_ARG_LONG, 0, OPT_76_ENUM_NAME},
    #endif
    #ifdef OPT_76_CLI_LONG_TXT_D
          {OPT_76_CLI_LONG_TXT_D, OPT_76_HAS_ARG_LONG, 0, OPT_76_ENUM_NAME},
    #endif
    #ifdef OPT_76_CLI_SHORT_CODE_TXT
          {OPT_76_CLI_SHORT_CODE_TXT, OPT_76_HAS_ARG_LONG, 0, OPT_76_ENUM_NAME},
    #endif
#endif
#ifdef OPT_77_ENUM_NAME
    #ifdef OPT_77_CLI_LONG_TXT_A
          {OPT_77_CLI_LONG_TXT_A, OPT_77_HAS_ARG_LONG, 0, OPT_77_ENUM_NAME},
    #endif
    #ifdef OPT_77_CLI_LONG_TXT_B
          {OPT_77_CLI_LONG_TXT_B, OPT_77_HAS_ARG_LONG, 0, OPT_77_ENUM_NAME},
    #endif
    #ifdef OPT_77_CLI_LONG_TXT_C
          {OPT_77_CLI_LONG_TXT_C, OPT_77_HAS_ARG_LONG, 0, OPT_77_ENUM_NAME},
    #endif
    #ifdef OPT_77_CLI_LONG_TXT_D
          {OPT_77_CLI_LONG_TXT_D, OPT_77_HAS_ARG_LONG, 0, OPT_77_ENUM_NAME},
    #endif
    #ifdef OPT_77_CLI_SHORT_CODE_TXT
          {OPT_77_CLI_SHORT_CODE_TXT, OPT_77_HAS_ARG_LONG, 0, OPT_77_ENUM_NAME},
    #endif
#endif
#ifdef OPT_78_ENUM_NAME
    #ifdef OPT_78_CLI_LONG_TXT_A
          {OPT_78_CLI_LONG_TXT_A, OPT_78_HAS_ARG_LONG, 0, OPT_78_ENUM_NAME},
    #endif
    #ifdef OPT_78_CLI_LONG_TXT_B
          {OPT_78_CLI_LONG_TXT_B, OPT_78_HAS_ARG_LONG, 0, OPT_78_ENUM_NAME},
    #endif
    #ifdef OPT_78_CLI_LONG_TXT_C
          {OPT_78_CLI_LONG_TXT_C, OPT_78_HAS_ARG_LONG, 0, OPT_78_ENUM_NAME},
    #endif
    #ifdef OPT_78_CLI_LONG_TXT_D
          {OPT_78_CLI_LONG_TXT_D, OPT_78_HAS_ARG_LONG, 0, OPT_78_ENUM_NAME},
    #endif
    #ifdef OPT_78_CLI_SHORT_CODE_TXT
          {OPT_78_CLI_SHORT_CODE_TXT, OPT_78_HAS_ARG_LONG, 0, OPT_78_ENUM_NAME},
    #endif
#endif
#ifdef OPT_79_ENUM_NAME
    #ifdef OPT_79_CLI_LONG_TXT_A
          {OPT_79_CLI_LONG_TXT_A, OPT_79_HAS_ARG_LONG, 0, OPT_79_ENUM_NAME},
    #endif
    #ifdef OPT_79_CLI_LONG_TXT_B
          {OPT_79_CLI_LONG_TXT_B, OPT_79_HAS_ARG_LONG, 0, OPT_79_ENUM_NAME},
    #endif
    #ifdef OPT_79_CLI_LONG_TXT_C
          {OPT_79_CLI_LONG_TXT_C, OPT_79_HAS_ARG_LONG, 0, OPT_79_ENUM_NAME},
    #endif
    #ifdef OPT_79_CLI_LONG_TXT_D
          {OPT_79_CLI_LONG_TXT_D, OPT_79_HAS_ARG_LONG, 0, OPT_79_ENUM_NAME},
    #endif
    #ifdef OPT_79_CLI_SHORT_CODE_TXT
          {OPT_79_CLI_SHORT_CODE_TXT, OPT_79_HAS_ARG_LONG, 0, OPT_79_ENUM_NAME},
    #endif
#endif

#ifdef OPT_80_ENUM_NAME
    #ifdef OPT_80_CLI_LONG_TXT_A
          {OPT_80_CLI_LONG_TXT_A, OPT_80_HAS_ARG_LONG, 0, OPT_80_ENUM_NAME},
    #endif
    #ifdef OPT_80_CLI_LONG_TXT_B
          {OPT_80_CLI_LONG_TXT_B, OPT_80_HAS_ARG_LONG, 0, OPT_80_ENUM_NAME},
    #endif
    #ifdef OPT_80_CLI_LONG_TXT_C
          {OPT_80_CLI_LONG_TXT_C, OPT_80_HAS_ARG_LONG, 0, OPT_80_ENUM_NAME},
    #endif
    #ifdef OPT_80_CLI_LONG_TXT_D
          {OPT_80_CLI_LONG_TXT_D, OPT_80_HAS_ARG_LONG, 0, OPT_80_ENUM_NAME},
    #endif
    #ifdef OPT_80_CLI_SHORT_CODE_TXT
          {OPT_80_CLI_SHORT_CODE_TXT, OPT_80_HAS_ARG_LONG, 0, OPT_80_ENUM_NAME},
    #endif
#endif
#ifdef OPT_81_ENUM_NAME
    #ifdef OPT_81_CLI_LONG_TXT_A
          {OPT_81_CLI_LONG_TXT_A, OPT_81_HAS_ARG_LONG, 0, OPT_81_ENUM_NAME},
    #endif
    #ifdef OPT_81_CLI_LONG_TXT_B
          {OPT_81_CLI_LONG_TXT_B, OPT_81_HAS_ARG_LONG, 0, OPT_81_ENUM_NAME},
    #endif
    #ifdef OPT_81_CLI_LONG_TXT_C
          {OPT_81_CLI_LONG_TXT_C, OPT_81_HAS_ARG_LONG, 0, OPT_81_ENUM_NAME},
    #endif
    #ifdef OPT_81_CLI_LONG_TXT_D
          {OPT_81_CLI_LONG_TXT_D, OPT_81_HAS_ARG_LONG, 0, OPT_81_ENUM_NAME},
    #endif
    #ifdef OPT_81_CLI_SHORT_CODE_TXT
          {OPT_81_CLI_SHORT_CODE_TXT, OPT_81_HAS_ARG_LONG, 0, OPT_81_ENUM_NAME},
    #endif
#endif
#ifdef OPT_82_ENUM_NAME
    #ifdef OPT_82_CLI_LONG_TXT_A
          {OPT_82_CLI_LONG_TXT_A, OPT_82_HAS_ARG_LONG, 0, OPT_82_ENUM_NAME},
    #endif
    #ifdef OPT_82_CLI_LONG_TXT_B
          {OPT_82_CLI_LONG_TXT_B, OPT_82_HAS_ARG_LONG, 0, OPT_82_ENUM_NAME},
    #endif
    #ifdef OPT_82_CLI_LONG_TXT_C
          {OPT_82_CLI_LONG_TXT_C, OPT_82_HAS_ARG_LONG, 0, OPT_82_ENUM_NAME},
    #endif
    #ifdef OPT_82_CLI_LONG_TXT_D
          {OPT_82_CLI_LONG_TXT_D, OPT_82_HAS_ARG_LONG, 0, OPT_82_ENUM_NAME},
    #endif
    #ifdef OPT_82_CLI_SHORT_CODE_TXT
          {OPT_82_CLI_SHORT_CODE_TXT, OPT_82_HAS_ARG_LONG, 0, OPT_82_ENUM_NAME},
    #endif
#endif
#ifdef OPT_83_ENUM_NAME
    #ifdef OPT_83_CLI_LONG_TXT_A
          {OPT_83_CLI_LONG_TXT_A, OPT_83_HAS_ARG_LONG, 0, OPT_83_ENUM_NAME},
    #endif
    #ifdef OPT_83_CLI_LONG_TXT_B
          {OPT_83_CLI_LONG_TXT_B, OPT_83_HAS_ARG_LONG, 0, OPT_83_ENUM_NAME},
    #endif
    #ifdef OPT_83_CLI_LONG_TXT_C
          {OPT_83_CLI_LONG_TXT_C, OPT_83_HAS_ARG_LONG, 0, OPT_83_ENUM_NAME},
    #endif
    #ifdef OPT_83_CLI_LONG_TXT_D
          {OPT_83_CLI_LONG_TXT_D, OPT_83_HAS_ARG_LONG, 0, OPT_83_ENUM_NAME},
    #endif
    #ifdef OPT_83_CLI_SHORT_CODE_TXT
          {OPT_83_CLI_SHORT_CODE_TXT, OPT_83_HAS_ARG_LONG, 0, OPT_83_ENUM_NAME},
    #endif
#endif
#ifdef OPT_84_ENUM_NAME
    #ifdef OPT_84_CLI_LONG_TXT_A
          {OPT_84_CLI_LONG_TXT_A, OPT_84_HAS_ARG_LONG, 0, OPT_84_ENUM_NAME},
    #endif
    #ifdef OPT_84_CLI_LONG_TXT_B
          {OPT_84_CLI_LONG_TXT_B, OPT_84_HAS_ARG_LONG, 0, OPT_84_ENUM_NAME},
    #endif
    #ifdef OPT_84_CLI_LONG_TXT_C
          {OPT_84_CLI_LONG_TXT_C, OPT_84_HAS_ARG_LONG, 0, OPT_84_ENUM_NAME},
    #endif
    #ifdef OPT_84_CLI_LONG_TXT_D
          {OPT_84_CLI_LONG_TXT_D, OPT_84_HAS_ARG_LONG, 0, OPT_84_ENUM_NAME},
    #endif
    #ifdef OPT_84_CLI_SHORT_CODE_TXT
          {OPT_84_CLI_SHORT_CODE_TXT, OPT_84_HAS_ARG_LONG, 0, OPT_84_ENUM_NAME},
    #endif
#endif
#ifdef OPT_85_ENUM_NAME
    #ifdef OPT_85_CLI_LONG_TXT_A
          {OPT_85_CLI_LONG_TXT_A, OPT_85_HAS_ARG_LONG, 0, OPT_85_ENUM_NAME},
    #endif
    #ifdef OPT_85_CLI_LONG_TXT_B
          {OPT_85_CLI_LONG_TXT_B, OPT_85_HAS_ARG_LONG, 0, OPT_85_ENUM_NAME},
    #endif
    #ifdef OPT_85_CLI_LONG_TXT_C
          {OPT_85_CLI_LONG_TXT_C, OPT_85_HAS_ARG_LONG, 0, OPT_85_ENUM_NAME},
    #endif
    #ifdef OPT_85_CLI_LONG_TXT_D
          {OPT_85_CLI_LONG_TXT_D, OPT_85_HAS_ARG_LONG, 0, OPT_85_ENUM_NAME},
    #endif
    #ifdef OPT_85_CLI_SHORT_CODE_TXT
          {OPT_85_CLI_SHORT_CODE_TXT, OPT_85_HAS_ARG_LONG, 0, OPT_85_ENUM_NAME},
    #endif
#endif
#ifdef OPT_86_ENUM_NAME
    #ifdef OPT_86_CLI_LONG_TXT_A
          {OPT_86_CLI_LONG_TXT_A, OPT_86_HAS_ARG_LONG, 0, OPT_86_ENUM_NAME},
    #endif
    #ifdef OPT_86_CLI_LONG_TXT_B
          {OPT_86_CLI_LONG_TXT_B, OPT_86_HAS_ARG_LONG, 0, OPT_86_ENUM_NAME},
    #endif
    #ifdef OPT_86_CLI_LONG_TXT_C
          {OPT_86_CLI_LONG_TXT_C, OPT_86_HAS_ARG_LONG, 0, OPT_86_ENUM_NAME},
    #endif
    #ifdef OPT_86_CLI_LONG_TXT_D
          {OPT_86_CLI_LONG_TXT_D, OPT_86_HAS_ARG_LONG, 0, OPT_86_ENUM_NAME},
    #endif
    #ifdef OPT_86_CLI_SHORT_CODE_TXT
          {OPT_86_CLI_SHORT_CODE_TXT, OPT_86_HAS_ARG_LONG, 0, OPT_86_ENUM_NAME},
    #endif
#endif
#ifdef OPT_87_ENUM_NAME
    #ifdef OPT_87_CLI_LONG_TXT_A
          {OPT_87_CLI_LONG_TXT_A, OPT_87_HAS_ARG_LONG, 0, OPT_87_ENUM_NAME},
    #endif
    #ifdef OPT_87_CLI_LONG_TXT_B
          {OPT_87_CLI_LONG_TXT_B, OPT_87_HAS_ARG_LONG, 0, OPT_87_ENUM_NAME},
    #endif
    #ifdef OPT_87_CLI_LONG_TXT_C
          {OPT_87_CLI_LONG_TXT_C, OPT_87_HAS_ARG_LONG, 0, OPT_87_ENUM_NAME},
    #endif
    #ifdef OPT_87_CLI_LONG_TXT_D
          {OPT_87_CLI_LONG_TXT_D, OPT_87_HAS_ARG_LONG, 0, OPT_87_ENUM_NAME},
    #endif
    #ifdef OPT_87_CLI_SHORT_CODE_TXT
          {OPT_87_CLI_SHORT_CODE_TXT, OPT_87_HAS_ARG_LONG, 0, OPT_87_ENUM_NAME},
    #endif
#endif
#ifdef OPT_88_ENUM_NAME
    #ifdef OPT_88_CLI_LONG_TXT_A
          {OPT_88_CLI_LONG_TXT_A, OPT_88_HAS_ARG_LONG, 0, OPT_88_ENUM_NAME},
    #endif
    #ifdef OPT_88_CLI_LONG_TXT_B
          {OPT_88_CLI_LONG_TXT_B, OPT_88_HAS_ARG_LONG, 0, OPT_88_ENUM_NAME},
    #endif
    #ifdef OPT_88_CLI_LONG_TXT_C
          {OPT_88_CLI_LONG_TXT_C, OPT_88_HAS_ARG_LONG, 0, OPT_88_ENUM_NAME},
    #endif
    #ifdef OPT_88_CLI_LONG_TXT_D
          {OPT_88_CLI_LONG_TXT_D, OPT_88_HAS_ARG_LONG, 0, OPT_88_ENUM_NAME},
    #endif
    #ifdef OPT_88_CLI_SHORT_CODE_TXT
          {OPT_88_CLI_SHORT_CODE_TXT, OPT_88_HAS_ARG_LONG, 0, OPT_88_ENUM_NAME},
    #endif
#endif
#ifdef OPT_89_ENUM_NAME
    #ifdef OPT_89_CLI_LONG_TXT_A
          {OPT_89_CLI_LONG_TXT_A, OPT_89_HAS_ARG_LONG, 0, OPT_89_ENUM_NAME},
    #endif
    #ifdef OPT_89_CLI_LONG_TXT_B
          {OPT_89_CLI_LONG_TXT_B, OPT_89_HAS_ARG_LONG, 0, OPT_89_ENUM_NAME},
    #endif
    #ifdef OPT_89_CLI_LONG_TXT_C
          {OPT_89_CLI_LONG_TXT_C, OPT_89_HAS_ARG_LONG, 0, OPT_89_ENUM_NAME},
    #endif
    #ifdef OPT_89_CLI_LONG_TXT_D
          {OPT_89_CLI_LONG_TXT_D, OPT_89_HAS_ARG_LONG, 0, OPT_89_ENUM_NAME},
    #endif
    #ifdef OPT_89_CLI_SHORT_CODE_TXT
          {OPT_89_CLI_SHORT_CODE_TXT, OPT_89_HAS_ARG_LONG, 0, OPT_89_ENUM_NAME},
    #endif
#endif

#ifdef OPT_90_ENUM_NAME
    #ifdef OPT_90_CLI_LONG_TXT_A
          {OPT_90_CLI_LONG_TXT_A, OPT_90_HAS_ARG_LONG, 0, OPT_90_ENUM_NAME},
    #endif
    #ifdef OPT_90_CLI_LONG_TXT_B
          {OPT_90_CLI_LONG_TXT_B, OPT_90_HAS_ARG_LONG, 0, OPT_90_ENUM_NAME},
    #endif
    #ifdef OPT_90_CLI_LONG_TXT_C
          {OPT_90_CLI_LONG_TXT_C, OPT_90_HAS_ARG_LONG, 0, OPT_90_ENUM_NAME},
    #endif
    #ifdef OPT_90_CLI_LONG_TXT_D
          {OPT_90_CLI_LONG_TXT_D, OPT_90_HAS_ARG_LONG, 0, OPT_90_ENUM_NAME},
    #endif
    #ifdef OPT_90_CLI_SHORT_CODE_TXT
          {OPT_90_CLI_SHORT_CODE_TXT, OPT_90_HAS_ARG_LONG, 0, OPT_90_ENUM_NAME},
    #endif
#endif
#ifdef OPT_91_ENUM_NAME
    #ifdef OPT_91_CLI_LONG_TXT_A
          {OPT_91_CLI_LONG_TXT_A, OPT_91_HAS_ARG_LONG, 0, OPT_91_ENUM_NAME},
    #endif
    #ifdef OPT_91_CLI_LONG_TXT_B
          {OPT_91_CLI_LONG_TXT_B, OPT_91_HAS_ARG_LONG, 0, OPT_91_ENUM_NAME},
    #endif
    #ifdef OPT_91_CLI_LONG_TXT_C
          {OPT_91_CLI_LONG_TXT_C, OPT_91_HAS_ARG_LONG, 0, OPT_91_ENUM_NAME},
    #endif
    #ifdef OPT_91_CLI_LONG_TXT_D
          {OPT_91_CLI_LONG_TXT_D, OPT_91_HAS_ARG_LONG, 0, OPT_91_ENUM_NAME},
    #endif
    #ifdef OPT_91_CLI_SHORT_CODE_TXT
          {OPT_91_CLI_SHORT_CODE_TXT, OPT_91_HAS_ARG_LONG, 0, OPT_91_ENUM_NAME},
    #endif
#endif
#ifdef OPT_92_ENUM_NAME
    #ifdef OPT_92_CLI_LONG_TXT_A
          {OPT_92_CLI_LONG_TXT_A, OPT_92_HAS_ARG_LONG, 0, OPT_92_ENUM_NAME},
    #endif
    #ifdef OPT_92_CLI_LONG_TXT_B
          {OPT_92_CLI_LONG_TXT_B, OPT_92_HAS_ARG_LONG, 0, OPT_92_ENUM_NAME},
    #endif
    #ifdef OPT_92_CLI_LONG_TXT_C
          {OPT_92_CLI_LONG_TXT_C, OPT_92_HAS_ARG_LONG, 0, OPT_92_ENUM_NAME},
    #endif
    #ifdef OPT_92_CLI_LONG_TXT_D
          {OPT_92_CLI_LONG_TXT_D, OPT_92_HAS_ARG_LONG, 0, OPT_92_ENUM_NAME},
    #endif
    #ifdef OPT_92_CLI_SHORT_CODE_TXT
          {OPT_92_CLI_SHORT_CODE_TXT, OPT_92_HAS_ARG_LONG, 0, OPT_92_ENUM_NAME},
    #endif
#endif
#ifdef OPT_93_ENUM_NAME
    #ifdef OPT_93_CLI_LONG_TXT_A
          {OPT_93_CLI_LONG_TXT_A, OPT_93_HAS_ARG_LONG, 0, OPT_93_ENUM_NAME},
    #endif
    #ifdef OPT_93_CLI_LONG_TXT_B
          {OPT_93_CLI_LONG_TXT_B, OPT_93_HAS_ARG_LONG, 0, OPT_93_ENUM_NAME},
    #endif
    #ifdef OPT_93_CLI_LONG_TXT_C
          {OPT_93_CLI_LONG_TXT_C, OPT_93_HAS_ARG_LONG, 0, OPT_93_ENUM_NAME},
    #endif
    #ifdef OPT_93_CLI_LONG_TXT_D
          {OPT_93_CLI_LONG_TXT_D, OPT_93_HAS_ARG_LONG, 0, OPT_93_ENUM_NAME},
    #endif
    #ifdef OPT_93_CLI_SHORT_CODE_TXT
          {OPT_93_CLI_SHORT_CODE_TXT, OPT_93_HAS_ARG_LONG, 0, OPT_93_ENUM_NAME},
    #endif
#endif
#ifdef OPT_94_ENUM_NAME
    #ifdef OPT_94_CLI_LONG_TXT_A
          {OPT_94_CLI_LONG_TXT_A, OPT_94_HAS_ARG_LONG, 0, OPT_94_ENUM_NAME},
    #endif
    #ifdef OPT_94_CLI_LONG_TXT_B
          {OPT_94_CLI_LONG_TXT_B, OPT_94_HAS_ARG_LONG, 0, OPT_94_ENUM_NAME},
    #endif
    #ifdef OPT_94_CLI_LONG_TXT_C
          {OPT_94_CLI_LONG_TXT_C, OPT_94_HAS_ARG_LONG, 0, OPT_94_ENUM_NAME},
    #endif
    #ifdef OPT_94_CLI_LONG_TXT_D
          {OPT_94_CLI_LONG_TXT_D, OPT_94_HAS_ARG_LONG, 0, OPT_94_ENUM_NAME},
    #endif
    #ifdef OPT_94_CLI_SHORT_CODE_TXT
          {OPT_94_CLI_SHORT_CODE_TXT, OPT_94_HAS_ARG_LONG, 0, OPT_94_ENUM_NAME},
    #endif
#endif
#ifdef OPT_95_ENUM_NAME
    #ifdef OPT_95_CLI_LONG_TXT_A
          {OPT_95_CLI_LONG_TXT_A, OPT_95_HAS_ARG_LONG, 0, OPT_95_ENUM_NAME},
    #endif
    #ifdef OPT_95_CLI_LONG_TXT_B
          {OPT_95_CLI_LONG_TXT_B, OPT_95_HAS_ARG_LONG, 0, OPT_95_ENUM_NAME},
    #endif
    #ifdef OPT_95_CLI_LONG_TXT_C
          {OPT_95_CLI_LONG_TXT_C, OPT_95_HAS_ARG_LONG, 0, OPT_95_ENUM_NAME},
    #endif
    #ifdef OPT_95_CLI_LONG_TXT_D
          {OPT_95_CLI_LONG_TXT_D, OPT_95_HAS_ARG_LONG, 0, OPT_95_ENUM_NAME},
    #endif
    #ifdef OPT_95_CLI_SHORT_CODE_TXT
          {OPT_95_CLI_SHORT_CODE_TXT, OPT_95_HAS_ARG_LONG, 0, OPT_95_ENUM_NAME},
    #endif
#endif
#ifdef OPT_96_ENUM_NAME
    #ifdef OPT_96_CLI_LONG_TXT_A
          {OPT_96_CLI_LONG_TXT_A, OPT_96_HAS_ARG_LONG, 0, OPT_96_ENUM_NAME},
    #endif
    #ifdef OPT_96_CLI_LONG_TXT_B
          {OPT_96_CLI_LONG_TXT_B, OPT_96_HAS_ARG_LONG, 0, OPT_96_ENUM_NAME},
    #endif
    #ifdef OPT_96_CLI_LONG_TXT_C
          {OPT_96_CLI_LONG_TXT_C, OPT_96_HAS_ARG_LONG, 0, OPT_96_ENUM_NAME},
    #endif
    #ifdef OPT_96_CLI_LONG_TXT_D
          {OPT_96_CLI_LONG_TXT_D, OPT_96_HAS_ARG_LONG, 0, OPT_96_ENUM_NAME},
    #endif
    #ifdef OPT_96_CLI_SHORT_CODE_TXT
          {OPT_96_CLI_SHORT_CODE_TXT, OPT_96_HAS_ARG_LONG, 0, OPT_96_ENUM_NAME},
    #endif
#endif
#ifdef OPT_97_ENUM_NAME
    #ifdef OPT_97_CLI_LONG_TXT_A
          {OPT_97_CLI_LONG_TXT_A, OPT_97_HAS_ARG_LONG, 0, OPT_97_ENUM_NAME},
    #endif
    #ifdef OPT_97_CLI_LONG_TXT_B
          {OPT_97_CLI_LONG_TXT_B, OPT_97_HAS_ARG_LONG, 0, OPT_97_ENUM_NAME},
    #endif
    #ifdef OPT_97_CLI_LONG_TXT_C
          {OPT_97_CLI_LONG_TXT_C, OPT_97_HAS_ARG_LONG, 0, OPT_97_ENUM_NAME},
    #endif
    #ifdef OPT_97_CLI_LONG_TXT_D
          {OPT_97_CLI_LONG_TXT_D, OPT_97_HAS_ARG_LONG, 0, OPT_97_ENUM_NAME},
    #endif
    #ifdef OPT_97_CLI_SHORT_CODE_TXT
          {OPT_97_CLI_SHORT_CODE_TXT, OPT_97_HAS_ARG_LONG, 0, OPT_97_ENUM_NAME},
    #endif
#endif
#ifdef OPT_98_ENUM_NAME
    #ifdef OPT_98_CLI_LONG_TXT_A
          {OPT_98_CLI_LONG_TXT_A, OPT_98_HAS_ARG_LONG, 0, OPT_98_ENUM_NAME},
    #endif
    #ifdef OPT_98_CLI_LONG_TXT_B
          {OPT_98_CLI_LONG_TXT_B, OPT_98_HAS_ARG_LONG, 0, OPT_98_ENUM_NAME},
    #endif
    #ifdef OPT_98_CLI_LONG_TXT_C
          {OPT_98_CLI_LONG_TXT_C, OPT_98_HAS_ARG_LONG, 0, OPT_98_ENUM_NAME},
    #endif
    #ifdef OPT_98_CLI_LONG_TXT_D
          {OPT_98_CLI_LONG_TXT_D, OPT_98_HAS_ARG_LONG, 0, OPT_98_ENUM_NAME},
    #endif
    #ifdef OPT_98_CLI_SHORT_CODE_TXT
          {OPT_98_CLI_SHORT_CODE_TXT, OPT_98_HAS_ARG_LONG, 0, OPT_98_ENUM_NAME},
    #endif
#endif
#ifdef OPT_99_ENUM_NAME
    #ifdef OPT_99_CLI_LONG_TXT_A
          {OPT_99_CLI_LONG_TXT_A, OPT_99_HAS_ARG_LONG, 0, OPT_99_ENUM_NAME},
    #endif
    #ifdef OPT_99_CLI_LONG_TXT_B
          {OPT_99_CLI_LONG_TXT_B, OPT_99_HAS_ARG_LONG, 0, OPT_99_ENUM_NAME},
    #endif
    #ifdef OPT_99_CLI_LONG_TXT_C
          {OPT_99_CLI_LONG_TXT_C, OPT_99_HAS_ARG_LONG, 0, OPT_99_ENUM_NAME},
    #endif
    #ifdef OPT_99_CLI_LONG_TXT_D
          {OPT_99_CLI_LONG_TXT_D, OPT_99_HAS_ARG_LONG, 0, OPT_99_ENUM_NAME},
    #endif
    #ifdef OPT_99_CLI_SHORT_CODE_TXT
          {OPT_99_CLI_SHORT_CODE_TXT, OPT_99_HAS_ARG_LONG, 0, OPT_99_ENUM_NAME},
    #endif
#endif

#ifdef OPT_100_ENUM_NAME
    #ifdef OPT_100_CLI_LONG_TXT_A
          {OPT_100_CLI_LONG_TXT_A, OPT_100_HAS_ARG_LONG, 0, OPT_100_ENUM_NAME},
    #endif
    #ifdef OPT_100_CLI_LONG_TXT_B
          {OPT_100_CLI_LONG_TXT_B, OPT_100_HAS_ARG_LONG, 0, OPT_100_ENUM_NAME},
    #endif
    #ifdef OPT_100_CLI_LONG_TXT_C
          {OPT_100_CLI_LONG_TXT_C, OPT_100_HAS_ARG_LONG, 0, OPT_100_ENUM_NAME},
    #endif
    #ifdef OPT_100_CLI_LONG_TXT_D
          {OPT_100_CLI_LONG_TXT_D, OPT_100_HAS_ARG_LONG, 0, OPT_100_ENUM_NAME},
    #endif
    #ifdef OPT_100_CLI_SHORT_CODE_TXT
          {OPT_100_CLI_SHORT_CODE_TXT, OPT_100_HAS_ARG_LONG, 0, OPT_100_ENUM_NAME},
    #endif
#endif
#ifdef OPT_101_ENUM_NAME
    #ifdef OPT_101_CLI_LONG_TXT_A
          {OPT_101_CLI_LONG_TXT_A, OPT_101_HAS_ARG_LONG, 0, OPT_101_ENUM_NAME},
    #endif
    #ifdef OPT_101_CLI_LONG_TXT_B
          {OPT_101_CLI_LONG_TXT_B, OPT_101_HAS_ARG_LONG, 0, OPT_101_ENUM_NAME},
    #endif
    #ifdef OPT_101_CLI_LONG_TXT_C
          {OPT_101_CLI_LONG_TXT_C, OPT_101_HAS_ARG_LONG, 0, OPT_101_ENUM_NAME},
    #endif
    #ifdef OPT_101_CLI_LONG_TXT_D
          {OPT_101_CLI_LONG_TXT_D, OPT_101_HAS_ARG_LONG, 0, OPT_101_ENUM_NAME},
    #endif
    #ifdef OPT_101_CLI_SHORT_CODE_TXT
          {OPT_101_CLI_SHORT_CODE_TXT, OPT_101_HAS_ARG_LONG, 0, OPT_101_ENUM_NAME},
    #endif
#endif
#ifdef OPT_102_ENUM_NAME
    #ifdef OPT_102_CLI_LONG_TXT_A
          {OPT_102_CLI_LONG_TXT_A, OPT_102_HAS_ARG_LONG, 0, OPT_102_ENUM_NAME},
    #endif
    #ifdef OPT_102_CLI_LONG_TXT_B
          {OPT_102_CLI_LONG_TXT_B, OPT_102_HAS_ARG_LONG, 0, OPT_102_ENUM_NAME},
    #endif
    #ifdef OPT_102_CLI_LONG_TXT_C
          {OPT_102_CLI_LONG_TXT_C, OPT_102_HAS_ARG_LONG, 0, OPT_102_ENUM_NAME},
    #endif
    #ifdef OPT_102_CLI_LONG_TXT_D
          {OPT_102_CLI_LONG_TXT_D, OPT_102_HAS_ARG_LONG, 0, OPT_102_ENUM_NAME},
    #endif
    #ifdef OPT_102_CLI_SHORT_CODE_TXT
          {OPT_102_CLI_SHORT_CODE_TXT, OPT_102_HAS_ARG_LONG, 0, OPT_102_ENUM_NAME},
    #endif
#endif
#ifdef OPT_103_ENUM_NAME
    #ifdef OPT_103_CLI_LONG_TXT_A
          {OPT_103_CLI_LONG_TXT_A, OPT_103_HAS_ARG_LONG, 0, OPT_103_ENUM_NAME},
    #endif
    #ifdef OPT_103_CLI_LONG_TXT_B
          {OPT_103_CLI_LONG_TXT_B, OPT_103_HAS_ARG_LONG, 0, OPT_103_ENUM_NAME},
    #endif
    #ifdef OPT_103_CLI_LONG_TXT_C
          {OPT_103_CLI_LONG_TXT_C, OPT_103_HAS_ARG_LONG, 0, OPT_103_ENUM_NAME},
    #endif
    #ifdef OPT_103_CLI_LONG_TXT_D
          {OPT_103_CLI_LONG_TXT_D, OPT_103_HAS_ARG_LONG, 0, OPT_103_ENUM_NAME},
    #endif
    #ifdef OPT_103_CLI_SHORT_CODE_TXT
          {OPT_103_CLI_SHORT_CODE_TXT, OPT_103_HAS_ARG_LONG, 0, OPT_103_ENUM_NAME},
    #endif
#endif
#ifdef OPT_104_ENUM_NAME
    #ifdef OPT_104_CLI_LONG_TXT_A
          {OPT_104_CLI_LONG_TXT_A, OPT_104_HAS_ARG_LONG, 0, OPT_104_ENUM_NAME},
    #endif
    #ifdef OPT_104_CLI_LONG_TXT_B
          {OPT_104_CLI_LONG_TXT_B, OPT_104_HAS_ARG_LONG, 0, OPT_104_ENUM_NAME},
    #endif
    #ifdef OPT_104_CLI_LONG_TXT_C
          {OPT_104_CLI_LONG_TXT_C, OPT_104_HAS_ARG_LONG, 0, OPT_104_ENUM_NAME},
    #endif
    #ifdef OPT_104_CLI_LONG_TXT_D
          {OPT_104_CLI_LONG_TXT_D, OPT_104_HAS_ARG_LONG, 0, OPT_104_ENUM_NAME},
    #endif
    #ifdef OPT_104_CLI_SHORT_CODE_TXT
          {OPT_104_CLI_SHORT_CODE_TXT, OPT_104_HAS_ARG_LONG, 0, OPT_104_ENUM_NAME},
    #endif
#endif
#ifdef OPT_105_ENUM_NAME
    #ifdef OPT_105_CLI_LONG_TXT_A
          {OPT_105_CLI_LONG_TXT_A, OPT_105_HAS_ARG_LONG, 0, OPT_105_ENUM_NAME},
    #endif
    #ifdef OPT_105_CLI_LONG_TXT_B
          {OPT_105_CLI_LONG_TXT_B, OPT_105_HAS_ARG_LONG, 0, OPT_105_ENUM_NAME},
    #endif
    #ifdef OPT_105_CLI_LONG_TXT_C
          {OPT_105_CLI_LONG_TXT_C, OPT_105_HAS_ARG_LONG, 0, OPT_105_ENUM_NAME},
    #endif
    #ifdef OPT_105_CLI_LONG_TXT_D
          {OPT_105_CLI_LONG_TXT_D, OPT_105_HAS_ARG_LONG, 0, OPT_105_ENUM_NAME},
    #endif
    #ifdef OPT_105_CLI_SHORT_CODE_TXT
          {OPT_105_CLI_SHORT_CODE_TXT, OPT_105_HAS_ARG_LONG, 0, OPT_105_ENUM_NAME},
    #endif
#endif
#ifdef OPT_106_ENUM_NAME
    #ifdef OPT_106_CLI_LONG_TXT_A
          {OPT_106_CLI_LONG_TXT_A, OPT_106_HAS_ARG_LONG, 0, OPT_106_ENUM_NAME},
    #endif
    #ifdef OPT_106_CLI_LONG_TXT_B
          {OPT_106_CLI_LONG_TXT_B, OPT_106_HAS_ARG_LONG, 0, OPT_106_ENUM_NAME},
    #endif
    #ifdef OPT_106_CLI_LONG_TXT_C
          {OPT_106_CLI_LONG_TXT_C, OPT_106_HAS_ARG_LONG, 0, OPT_106_ENUM_NAME},
    #endif
    #ifdef OPT_106_CLI_LONG_TXT_D
          {OPT_106_CLI_LONG_TXT_D, OPT_106_HAS_ARG_LONG, 0, OPT_106_ENUM_NAME},
    #endif
    #ifdef OPT_106_CLI_SHORT_CODE_TXT
          {OPT_106_CLI_SHORT_CODE_TXT, OPT_106_HAS_ARG_LONG, 0, OPT_106_ENUM_NAME},
    #endif
#endif
#ifdef OPT_107_ENUM_NAME
    #ifdef OPT_107_CLI_LONG_TXT_A
          {OPT_107_CLI_LONG_TXT_A, OPT_107_HAS_ARG_LONG, 0, OPT_107_ENUM_NAME},
    #endif
    #ifdef OPT_107_CLI_LONG_TXT_B
          {OPT_107_CLI_LONG_TXT_B, OPT_107_HAS_ARG_LONG, 0, OPT_107_ENUM_NAME},
    #endif
    #ifdef OPT_107_CLI_LONG_TXT_C
          {OPT_107_CLI_LONG_TXT_C, OPT_107_HAS_ARG_LONG, 0, OPT_107_ENUM_NAME},
    #endif
    #ifdef OPT_107_CLI_LONG_TXT_D
          {OPT_107_CLI_LONG_TXT_D, OPT_107_HAS_ARG_LONG, 0, OPT_107_ENUM_NAME},
    #endif
    #ifdef OPT_107_CLI_SHORT_CODE_TXT
          {OPT_107_CLI_SHORT_CODE_TXT, OPT_107_HAS_ARG_LONG, 0, OPT_107_ENUM_NAME},
    #endif
#endif
#ifdef OPT_108_ENUM_NAME
    #ifdef OPT_108_CLI_LONG_TXT_A
          {OPT_108_CLI_LONG_TXT_A, OPT_108_HAS_ARG_LONG, 0, OPT_108_ENUM_NAME},
    #endif
    #ifdef OPT_108_CLI_LONG_TXT_B
          {OPT_108_CLI_LONG_TXT_B, OPT_108_HAS_ARG_LONG, 0, OPT_108_ENUM_NAME},
    #endif
    #ifdef OPT_108_CLI_LONG_TXT_C
          {OPT_108_CLI_LONG_TXT_C, OPT_108_HAS_ARG_LONG, 0, OPT_108_ENUM_NAME},
    #endif
    #ifdef OPT_108_CLI_LONG_TXT_D
          {OPT_108_CLI_LONG_TXT_D, OPT_108_HAS_ARG_LONG, 0, OPT_108_ENUM_NAME},
    #endif
    #ifdef OPT_108_CLI_SHORT_CODE_TXT
          {OPT_108_CLI_SHORT_CODE_TXT, OPT_108_HAS_ARG_LONG, 0, OPT_108_ENUM_NAME},
    #endif
#endif
#ifdef OPT_109_ENUM_NAME
    #ifdef OPT_109_CLI_LONG_TXT_A
          {OPT_109_CLI_LONG_TXT_A, OPT_109_HAS_ARG_LONG, 0, OPT_109_ENUM_NAME},
    #endif
    #ifdef OPT_109_CLI_LONG_TXT_B
          {OPT_109_CLI_LONG_TXT_B, OPT_109_HAS_ARG_LONG, 0, OPT_109_ENUM_NAME},
    #endif
    #ifdef OPT_109_CLI_LONG_TXT_C
          {OPT_109_CLI_LONG_TXT_C, OPT_109_HAS_ARG_LONG, 0, OPT_109_ENUM_NAME},
    #endif
    #ifdef OPT_109_CLI_LONG_TXT_D
          {OPT_109_CLI_LONG_TXT_D, OPT_109_HAS_ARG_LONG, 0, OPT_109_ENUM_NAME},
    #endif
    #ifdef OPT_109_CLI_SHORT_CODE_TXT
          {OPT_109_CLI_SHORT_CODE_TXT, OPT_109_HAS_ARG_LONG, 0, OPT_109_ENUM_NAME},
    #endif
#endif

#ifdef OPT_110_ENUM_NAME
    #ifdef OPT_110_CLI_LONG_TXT_A
          {OPT_110_CLI_LONG_TXT_A, OPT_110_HAS_ARG_LONG, 0, OPT_110_ENUM_NAME},
    #endif
    #ifdef OPT_110_CLI_LONG_TXT_B
          {OPT_110_CLI_LONG_TXT_B, OPT_110_HAS_ARG_LONG, 0, OPT_110_ENUM_NAME},
    #endif
    #ifdef OPT_110_CLI_LONG_TXT_C
          {OPT_110_CLI_LONG_TXT_C, OPT_110_HAS_ARG_LONG, 0, OPT_110_ENUM_NAME},
    #endif
    #ifdef OPT_110_CLI_LONG_TXT_D
          {OPT_110_CLI_LONG_TXT_D, OPT_110_HAS_ARG_LONG, 0, OPT_110_ENUM_NAME},
    #endif
    #ifdef OPT_110_CLI_SHORT_CODE_TXT
          {OPT_110_CLI_SHORT_CODE_TXT, OPT_110_HAS_ARG_LONG, 0, OPT_110_ENUM_NAME},
    #endif
#endif
#ifdef OPT_111_ENUM_NAME
    #ifdef OPT_111_CLI_LONG_TXT_A
          {OPT_111_CLI_LONG_TXT_A, OPT_111_HAS_ARG_LONG, 0, OPT_111_ENUM_NAME},
    #endif
    #ifdef OPT_111_CLI_LONG_TXT_B
          {OPT_111_CLI_LONG_TXT_B, OPT_111_HAS_ARG_LONG, 0, OPT_111_ENUM_NAME},
    #endif
    #ifdef OPT_111_CLI_LONG_TXT_C
          {OPT_111_CLI_LONG_TXT_C, OPT_111_HAS_ARG_LONG, 0, OPT_111_ENUM_NAME},
    #endif
    #ifdef OPT_111_CLI_LONG_TXT_D
          {OPT_111_CLI_LONG_TXT_D, OPT_111_HAS_ARG_LONG, 0, OPT_111_ENUM_NAME},
    #endif
    #ifdef OPT_111_CLI_SHORT_CODE_TXT
          {OPT_111_CLI_SHORT_CODE_TXT, OPT_111_HAS_ARG_LONG, 0, OPT_111_ENUM_NAME},
    #endif
#endif
#ifdef OPT_112_ENUM_NAME
    #ifdef OPT_112_CLI_LONG_TXT_A
          {OPT_112_CLI_LONG_TXT_A, OPT_112_HAS_ARG_LONG, 0, OPT_112_ENUM_NAME},
    #endif
    #ifdef OPT_112_CLI_LONG_TXT_B
          {OPT_112_CLI_LONG_TXT_B, OPT_112_HAS_ARG_LONG, 0, OPT_112_ENUM_NAME},
    #endif
    #ifdef OPT_112_CLI_LONG_TXT_C
          {OPT_112_CLI_LONG_TXT_C, OPT_112_HAS_ARG_LONG, 0, OPT_112_ENUM_NAME},
    #endif
    #ifdef OPT_112_CLI_LONG_TXT_D
          {OPT_112_CLI_LONG_TXT_D, OPT_112_HAS_ARG_LONG, 0, OPT_112_ENUM_NAME},
    #endif
    #ifdef OPT_112_CLI_SHORT_CODE_TXT
          {OPT_112_CLI_SHORT_CODE_TXT, OPT_112_HAS_ARG_LONG, 0, OPT_112_ENUM_NAME},
    #endif
#endif
#ifdef OPT_113_ENUM_NAME
    #ifdef OPT_113_CLI_LONG_TXT_A
          {OPT_113_CLI_LONG_TXT_A, OPT_113_HAS_ARG_LONG, 0, OPT_113_ENUM_NAME},
    #endif
    #ifdef OPT_113_CLI_LONG_TXT_B
          {OPT_113_CLI_LONG_TXT_B, OPT_113_HAS_ARG_LONG, 0, OPT_113_ENUM_NAME},
    #endif
    #ifdef OPT_113_CLI_LONG_TXT_C
          {OPT_113_CLI_LONG_TXT_C, OPT_113_HAS_ARG_LONG, 0, OPT_113_ENUM_NAME},
    #endif
    #ifdef OPT_113_CLI_LONG_TXT_D
          {OPT_113_CLI_LONG_TXT_D, OPT_113_HAS_ARG_LONG, 0, OPT_113_ENUM_NAME},
    #endif
    #ifdef OPT_113_CLI_SHORT_CODE_TXT
          {OPT_113_CLI_SHORT_CODE_TXT, OPT_113_HAS_ARG_LONG, 0, OPT_113_ENUM_NAME},
    #endif
#endif
#ifdef OPT_114_ENUM_NAME
    #ifdef OPT_114_CLI_LONG_TXT_A
          {OPT_114_CLI_LONG_TXT_A, OPT_114_HAS_ARG_LONG, 0, OPT_114_ENUM_NAME},
    #endif
    #ifdef OPT_114_CLI_LONG_TXT_B
          {OPT_114_CLI_LONG_TXT_B, OPT_114_HAS_ARG_LONG, 0, OPT_114_ENUM_NAME},
    #endif
    #ifdef OPT_114_CLI_LONG_TXT_C
          {OPT_114_CLI_LONG_TXT_C, OPT_114_HAS_ARG_LONG, 0, OPT_114_ENUM_NAME},
    #endif
    #ifdef OPT_114_CLI_LONG_TXT_D
          {OPT_114_CLI_LONG_TXT_D, OPT_114_HAS_ARG_LONG, 0, OPT_114_ENUM_NAME},
    #endif
    #ifdef OPT_114_CLI_SHORT_CODE_TXT
          {OPT_114_CLI_SHORT_CODE_TXT, OPT_114_HAS_ARG_LONG, 0, OPT_114_ENUM_NAME},
    #endif
#endif
#ifdef OPT_115_ENUM_NAME
    #ifdef OPT_115_CLI_LONG_TXT_A
          {OPT_115_CLI_LONG_TXT_A, OPT_115_HAS_ARG_LONG, 0, OPT_115_ENUM_NAME},
    #endif
    #ifdef OPT_115_CLI_LONG_TXT_B
          {OPT_115_CLI_LONG_TXT_B, OPT_115_HAS_ARG_LONG, 0, OPT_115_ENUM_NAME},
    #endif
    #ifdef OPT_115_CLI_LONG_TXT_C
          {OPT_115_CLI_LONG_TXT_C, OPT_115_HAS_ARG_LONG, 0, OPT_115_ENUM_NAME},
    #endif
    #ifdef OPT_115_CLI_LONG_TXT_D
          {OPT_115_CLI_LONG_TXT_D, OPT_115_HAS_ARG_LONG, 0, OPT_115_ENUM_NAME},
    #endif
    #ifdef OPT_115_CLI_SHORT_CODE_TXT
          {OPT_115_CLI_SHORT_CODE_TXT, OPT_115_HAS_ARG_LONG, 0, OPT_115_ENUM_NAME},
    #endif
#endif
#ifdef OPT_116_ENUM_NAME
    #ifdef OPT_116_CLI_LONG_TXT_A
          {OPT_116_CLI_LONG_TXT_A, OPT_116_HAS_ARG_LONG, 0, OPT_116_ENUM_NAME},
    #endif
    #ifdef OPT_116_CLI_LONG_TXT_B
          {OPT_116_CLI_LONG_TXT_B, OPT_116_HAS_ARG_LONG, 0, OPT_116_ENUM_NAME},
    #endif
    #ifdef OPT_116_CLI_LONG_TXT_C
          {OPT_116_CLI_LONG_TXT_C, OPT_116_HAS_ARG_LONG, 0, OPT_116_ENUM_NAME},
    #endif
    #ifdef OPT_116_CLI_LONG_TXT_D
          {OPT_116_CLI_LONG_TXT_D, OPT_116_HAS_ARG_LONG, 0, OPT_116_ENUM_NAME},
    #endif
    #ifdef OPT_116_CLI_SHORT_CODE_TXT
          {OPT_116_CLI_SHORT_CODE_TXT, OPT_116_HAS_ARG_LONG, 0, OPT_116_ENUM_NAME},
    #endif
#endif
#ifdef OPT_117_ENUM_NAME
    #ifdef OPT_117_CLI_LONG_TXT_A
          {OPT_117_CLI_LONG_TXT_A, OPT_117_HAS_ARG_LONG, 0, OPT_117_ENUM_NAME},
    #endif
    #ifdef OPT_117_CLI_LONG_TXT_B
          {OPT_117_CLI_LONG_TXT_B, OPT_117_HAS_ARG_LONG, 0, OPT_117_ENUM_NAME},
    #endif
    #ifdef OPT_117_CLI_LONG_TXT_C
          {OPT_117_CLI_LONG_TXT_C, OPT_117_HAS_ARG_LONG, 0, OPT_117_ENUM_NAME},
    #endif
    #ifdef OPT_117_CLI_LONG_TXT_D
          {OPT_117_CLI_LONG_TXT_D, OPT_117_HAS_ARG_LONG, 0, OPT_117_ENUM_NAME},
    #endif
    #ifdef OPT_117_CLI_SHORT_CODE_TXT
          {OPT_117_CLI_SHORT_CODE_TXT, OPT_117_HAS_ARG_LONG, 0, OPT_117_ENUM_NAME},
    #endif
#endif
#ifdef OPT_118_ENUM_NAME
    #ifdef OPT_118_CLI_LONG_TXT_A
          {OPT_118_CLI_LONG_TXT_A, OPT_118_HAS_ARG_LONG, 0, OPT_118_ENUM_NAME},
    #endif
    #ifdef OPT_118_CLI_LONG_TXT_B
          {OPT_118_CLI_LONG_TXT_B, OPT_118_HAS_ARG_LONG, 0, OPT_118_ENUM_NAME},
    #endif
    #ifdef OPT_118_CLI_LONG_TXT_C
          {OPT_118_CLI_LONG_TXT_C, OPT_118_HAS_ARG_LONG, 0, OPT_118_ENUM_NAME},
    #endif
    #ifdef OPT_118_CLI_LONG_TXT_D
          {OPT_118_CLI_LONG_TXT_D, OPT_118_HAS_ARG_LONG, 0, OPT_118_ENUM_NAME},
    #endif
    #ifdef OPT_118_CLI_SHORT_CODE_TXT
          {OPT_118_CLI_SHORT_CODE_TXT, OPT_118_HAS_ARG_LONG, 0, OPT_118_ENUM_NAME},
    #endif
#endif
#ifdef OPT_119_ENUM_NAME
    #ifdef OPT_119_CLI_LONG_TXT_A
          {OPT_119_CLI_LONG_TXT_A, OPT_119_HAS_ARG_LONG, 0, OPT_119_ENUM_NAME},
    #endif
    #ifdef OPT_119_CLI_LONG_TXT_B
          {OPT_119_CLI_LONG_TXT_B, OPT_119_HAS_ARG_LONG, 0, OPT_119_ENUM_NAME},
    #endif
    #ifdef OPT_119_CLI_LONG_TXT_C
          {OPT_119_CLI_LONG_TXT_C, OPT_119_HAS_ARG_LONG, 0, OPT_119_ENUM_NAME},
    #endif
    #ifdef OPT_119_CLI_LONG_TXT_D
          {OPT_119_CLI_LONG_TXT_D, OPT_119_HAS_ARG_LONG, 0, OPT_119_ENUM_NAME},
    #endif
    #ifdef OPT_119_CLI_SHORT_CODE_TXT
          {OPT_119_CLI_SHORT_CODE_TXT, OPT_119_HAS_ARG_LONG, 0, OPT_119_ENUM_NAME},
    #endif
#endif

#ifdef OPT_120_ENUM_NAME
    #ifdef OPT_120_CLI_LONG_TXT_A
          {OPT_120_CLI_LONG_TXT_A, OPT_120_HAS_ARG_LONG, 0, OPT_120_ENUM_NAME},
    #endif
    #ifdef OPT_120_CLI_LONG_TXT_B
          {OPT_120_CLI_LONG_TXT_B, OPT_120_HAS_ARG_LONG, 0, OPT_120_ENUM_NAME},
    #endif
    #ifdef OPT_120_CLI_LONG_TXT_C
          {OPT_120_CLI_LONG_TXT_C, OPT_120_HAS_ARG_LONG, 0, OPT_120_ENUM_NAME},
    #endif
    #ifdef OPT_120_CLI_LONG_TXT_D
          {OPT_120_CLI_LONG_TXT_D, OPT_120_HAS_ARG_LONG, 0, OPT_120_ENUM_NAME},
    #endif
    #ifdef OPT_120_CLI_SHORT_CODE_TXT
          {OPT_120_CLI_SHORT_CODE_TXT, OPT_120_HAS_ARG_LONG, 0, OPT_120_ENUM_NAME},
    #endif
#endif
#ifdef OPT_121_ENUM_NAME
    #ifdef OPT_121_CLI_LONG_TXT_A
          {OPT_121_CLI_LONG_TXT_A, OPT_121_HAS_ARG_LONG, 0, OPT_121_ENUM_NAME},
    #endif
    #ifdef OPT_121_CLI_LONG_TXT_B
          {OPT_121_CLI_LONG_TXT_B, OPT_121_HAS_ARG_LONG, 0, OPT_121_ENUM_NAME},
    #endif
    #ifdef OPT_121_CLI_LONG_TXT_C
          {OPT_121_CLI_LONG_TXT_C, OPT_121_HAS_ARG_LONG, 0, OPT_121_ENUM_NAME},
    #endif
    #ifdef OPT_121_CLI_LONG_TXT_D
          {OPT_121_CLI_LONG_TXT_D, OPT_121_HAS_ARG_LONG, 0, OPT_121_ENUM_NAME},
    #endif
    #ifdef OPT_121_CLI_SHORT_CODE_TXT
          {OPT_121_CLI_SHORT_CODE_TXT, OPT_121_HAS_ARG_LONG, 0, OPT_121_ENUM_NAME},
    #endif
#endif
#ifdef OPT_122_ENUM_NAME
    #ifdef OPT_122_CLI_LONG_TXT_A
          {OPT_122_CLI_LONG_TXT_A, OPT_122_HAS_ARG_LONG, 0, OPT_122_ENUM_NAME},
    #endif
    #ifdef OPT_122_CLI_LONG_TXT_B
          {OPT_122_CLI_LONG_TXT_B, OPT_122_HAS_ARG_LONG, 0, OPT_122_ENUM_NAME},
    #endif
    #ifdef OPT_122_CLI_LONG_TXT_C
          {OPT_122_CLI_LONG_TXT_C, OPT_122_HAS_ARG_LONG, 0, OPT_122_ENUM_NAME},
    #endif
    #ifdef OPT_122_CLI_LONG_TXT_D
          {OPT_122_CLI_LONG_TXT_D, OPT_122_HAS_ARG_LONG, 0, OPT_122_ENUM_NAME},
    #endif
    #ifdef OPT_122_CLI_SHORT_CODE_TXT
          {OPT_122_CLI_SHORT_CODE_TXT, OPT_122_HAS_ARG_LONG, 0, OPT_122_ENUM_NAME},
    #endif
#endif
#ifdef OPT_123_ENUM_NAME
    #ifdef OPT_123_CLI_LONG_TXT_A
          {OPT_123_CLI_LONG_TXT_A, OPT_123_HAS_ARG_LONG, 0, OPT_123_ENUM_NAME},
    #endif
    #ifdef OPT_123_CLI_LONG_TXT_B
          {OPT_123_CLI_LONG_TXT_B, OPT_123_HAS_ARG_LONG, 0, OPT_123_ENUM_NAME},
    #endif
    #ifdef OPT_123_CLI_LONG_TXT_C
          {OPT_123_CLI_LONG_TXT_C, OPT_123_HAS_ARG_LONG, 0, OPT_123_ENUM_NAME},
    #endif
    #ifdef OPT_123_CLI_LONG_TXT_D
          {OPT_123_CLI_LONG_TXT_D, OPT_123_HAS_ARG_LONG, 0, OPT_123_ENUM_NAME},
    #endif
    #ifdef OPT_123_CLI_SHORT_CODE_TXT
          {OPT_123_CLI_SHORT_CODE_TXT, OPT_123_HAS_ARG_LONG, 0, OPT_123_ENUM_NAME},
    #endif
#endif
#ifdef OPT_124_ENUM_NAME
    #ifdef OPT_124_CLI_LONG_TXT_A
          {OPT_124_CLI_LONG_TXT_A, OPT_124_HAS_ARG_LONG, 0, OPT_124_ENUM_NAME},
    #endif
    #ifdef OPT_124_CLI_LONG_TXT_B
          {OPT_124_CLI_LONG_TXT_B, OPT_124_HAS_ARG_LONG, 0, OPT_124_ENUM_NAME},
    #endif
    #ifdef OPT_124_CLI_LONG_TXT_C
          {OPT_124_CLI_LONG_TXT_C, OPT_124_HAS_ARG_LONG, 0, OPT_124_ENUM_NAME},
    #endif
    #ifdef OPT_124_CLI_LONG_TXT_D
          {OPT_124_CLI_LONG_TXT_D, OPT_124_HAS_ARG_LONG, 0, OPT_124_ENUM_NAME},
    #endif
    #ifdef OPT_124_CLI_SHORT_CODE_TXT
          {OPT_124_CLI_SHORT_CODE_TXT, OPT_124_HAS_ARG_LONG, 0, OPT_124_ENUM_NAME},
    #endif
#endif
#ifdef OPT_125_ENUM_NAME
    #ifdef OPT_125_CLI_LONG_TXT_A
          {OPT_125_CLI_LONG_TXT_A, OPT_125_HAS_ARG_LONG, 0, OPT_125_ENUM_NAME},
    #endif
    #ifdef OPT_125_CLI_LONG_TXT_B
          {OPT_125_CLI_LONG_TXT_B, OPT_125_HAS_ARG_LONG, 0, OPT_125_ENUM_NAME},
    #endif
    #ifdef OPT_125_CLI_LONG_TXT_C
          {OPT_125_CLI_LONG_TXT_C, OPT_125_HAS_ARG_LONG, 0, OPT_125_ENUM_NAME},
    #endif
    #ifdef OPT_125_CLI_LONG_TXT_D
          {OPT_125_CLI_LONG_TXT_D, OPT_125_HAS_ARG_LONG, 0, OPT_125_ENUM_NAME},
    #endif
    #ifdef OPT_125_CLI_SHORT_CODE_TXT
          {OPT_125_CLI_SHORT_CODE_TXT, OPT_125_HAS_ARG_LONG, 0, OPT_125_ENUM_NAME},
    #endif
#endif
#ifdef OPT_126_ENUM_NAME
    #ifdef OPT_126_CLI_LONG_TXT_A
          {OPT_126_CLI_LONG_TXT_A, OPT_126_HAS_ARG_LONG, 0, OPT_126_ENUM_NAME},
    #endif
    #ifdef OPT_126_CLI_LONG_TXT_B
          {OPT_126_CLI_LONG_TXT_B, OPT_126_HAS_ARG_LONG, 0, OPT_126_ENUM_NAME},
    #endif
    #ifdef OPT_126_CLI_LONG_TXT_C
          {OPT_126_CLI_LONG_TXT_C, OPT_126_HAS_ARG_LONG, 0, OPT_126_ENUM_NAME},
    #endif
    #ifdef OPT_126_CLI_LONG_TXT_D
          {OPT_126_CLI_LONG_TXT_D, OPT_126_HAS_ARG_LONG, 0, OPT_126_ENUM_NAME},
    #endif
    #ifdef OPT_126_CLI_SHORT_CODE_TXT
          {OPT_126_CLI_SHORT_CODE_TXT, OPT_126_HAS_ARG_LONG, 0, OPT_126_ENUM_NAME},
    #endif
#endif
#ifdef OPT_127_ENUM_NAME
    #ifdef OPT_127_CLI_LONG_TXT_A
          {OPT_127_CLI_LONG_TXT_A, OPT_127_HAS_ARG_LONG, 0, OPT_127_ENUM_NAME},
    #endif
    #ifdef OPT_127_CLI_LONG_TXT_B
          {OPT_127_CLI_LONG_TXT_B, OPT_127_HAS_ARG_LONG, 0, OPT_127_ENUM_NAME},
    #endif
    #ifdef OPT_127_CLI_LONG_TXT_C
          {OPT_127_CLI_LONG_TXT_C, OPT_127_HAS_ARG_LONG, 0, OPT_127_ENUM_NAME},
    #endif
    #ifdef OPT_127_CLI_LONG_TXT_D
          {OPT_127_CLI_LONG_TXT_D, OPT_127_HAS_ARG_LONG, 0, OPT_127_ENUM_NAME},
    #endif
    #ifdef OPT_127_CLI_SHORT_CODE_TXT
          {OPT_127_CLI_SHORT_CODE_TXT, OPT_127_HAS_ARG_LONG, 0, OPT_127_ENUM_NAME},
    #endif
#endif
#ifdef OPT_128_ENUM_NAME
    #ifdef OPT_128_CLI_LONG_TXT_A
          {OPT_128_CLI_LONG_TXT_A, OPT_128_HAS_ARG_LONG, 0, OPT_128_ENUM_NAME},
    #endif
    #ifdef OPT_128_CLI_LONG_TXT_B
          {OPT_128_CLI_LONG_TXT_B, OPT_128_HAS_ARG_LONG, 0, OPT_128_ENUM_NAME},
    #endif
    #ifdef OPT_128_CLI_LONG_TXT_C
          {OPT_128_CLI_LONG_TXT_C, OPT_128_HAS_ARG_LONG, 0, OPT_128_ENUM_NAME},
    #endif
    #ifdef OPT_128_CLI_LONG_TXT_D
          {OPT_128_CLI_LONG_TXT_D, OPT_128_HAS_ARG_LONG, 0, OPT_128_ENUM_NAME},
    #endif
    #ifdef OPT_128_CLI_SHORT_CODE_TXT
          {OPT_128_CLI_SHORT_CODE_TXT, OPT_128_HAS_ARG_LONG, 0, OPT_128_ENUM_NAME},
    #endif
#endif
#ifdef OPT_129_ENUM_NAME
    #ifdef OPT_129_CLI_LONG_TXT_A
          {OPT_129_CLI_LONG_TXT_A, OPT_129_HAS_ARG_LONG, 0, OPT_129_ENUM_NAME},
    #endif
    #ifdef OPT_129_CLI_LONG_TXT_B
          {OPT_129_CLI_LONG_TXT_B, OPT_129_HAS_ARG_LONG, 0, OPT_129_ENUM_NAME},
    #endif
    #ifdef OPT_129_CLI_LONG_TXT_C
          {OPT_129_CLI_LONG_TXT_C, OPT_129_HAS_ARG_LONG, 0, OPT_129_ENUM_NAME},
    #endif
    #ifdef OPT_129_CLI_LONG_TXT_D
          {OPT_129_CLI_LONG_TXT_D, OPT_129_HAS_ARG_LONG, 0, OPT_129_ENUM_NAME},
    #endif
    #ifdef OPT_129_CLI_SHORT_CODE_TXT
          {OPT_129_CLI_SHORT_CODE_TXT, OPT_129_HAS_ARG_LONG, 0, OPT_129_ENUM_NAME},
    #endif
#endif

#ifdef OPT_130_ENUM_NAME
    #ifdef OPT_130_CLI_LONG_TXT_A
          {OPT_130_CLI_LONG_TXT_A, OPT_130_HAS_ARG_LONG, 0, OPT_130_ENUM_NAME},
    #endif
    #ifdef OPT_130_CLI_LONG_TXT_B
          {OPT_130_CLI_LONG_TXT_B, OPT_130_HAS_ARG_LONG, 0, OPT_130_ENUM_NAME},
    #endif
    #ifdef OPT_130_CLI_LONG_TXT_C
          {OPT_130_CLI_LONG_TXT_C, OPT_130_HAS_ARG_LONG, 0, OPT_130_ENUM_NAME},
    #endif
    #ifdef OPT_130_CLI_LONG_TXT_D
          {OPT_130_CLI_LONG_TXT_D, OPT_130_HAS_ARG_LONG, 0, OPT_130_ENUM_NAME},
    #endif
    #ifdef OPT_130_CLI_SHORT_CODE_TXT
          {OPT_130_CLI_SHORT_CODE_TXT, OPT_130_HAS_ARG_LONG, 0, OPT_130_ENUM_NAME},
    #endif
#endif
#ifdef OPT_131_ENUM_NAME
    #ifdef OPT_131_CLI_LONG_TXT_A
          {OPT_131_CLI_LONG_TXT_A, OPT_131_HAS_ARG_LONG, 0, OPT_131_ENUM_NAME},
    #endif
    #ifdef OPT_131_CLI_LONG_TXT_B
          {OPT_131_CLI_LONG_TXT_B, OPT_131_HAS_ARG_LONG, 0, OPT_131_ENUM_NAME},
    #endif
    #ifdef OPT_131_CLI_LONG_TXT_C
          {OPT_131_CLI_LONG_TXT_C, OPT_131_HAS_ARG_LONG, 0, OPT_131_ENUM_NAME},
    #endif
    #ifdef OPT_131_CLI_LONG_TXT_D
          {OPT_131_CLI_LONG_TXT_D, OPT_131_HAS_ARG_LONG, 0, OPT_131_ENUM_NAME},
    #endif
    #ifdef OPT_131_CLI_SHORT_CODE_TXT
          {OPT_131_CLI_SHORT_CODE_TXT, OPT_131_HAS_ARG_LONG, 0, OPT_131_ENUM_NAME},
    #endif
#endif
#ifdef OPT_132_ENUM_NAME
    #ifdef OPT_132_CLI_LONG_TXT_A
          {OPT_132_CLI_LONG_TXT_A, OPT_132_HAS_ARG_LONG, 0, OPT_132_ENUM_NAME},
    #endif
    #ifdef OPT_132_CLI_LONG_TXT_B
          {OPT_132_CLI_LONG_TXT_B, OPT_132_HAS_ARG_LONG, 0, OPT_132_ENUM_NAME},
    #endif
    #ifdef OPT_132_CLI_LONG_TXT_C
          {OPT_132_CLI_LONG_TXT_C, OPT_132_HAS_ARG_LONG, 0, OPT_132_ENUM_NAME},
    #endif
    #ifdef OPT_132_CLI_LONG_TXT_D
          {OPT_132_CLI_LONG_TXT_D, OPT_132_HAS_ARG_LONG, 0, OPT_132_ENUM_NAME},
    #endif
    #ifdef OPT_132_CLI_SHORT_CODE_TXT
          {OPT_132_CLI_SHORT_CODE_TXT, OPT_132_HAS_ARG_LONG, 0, OPT_132_ENUM_NAME},
    #endif
#endif
#ifdef OPT_133_ENUM_NAME
    #ifdef OPT_133_CLI_LONG_TXT_A
          {OPT_133_CLI_LONG_TXT_A, OPT_133_HAS_ARG_LONG, 0, OPT_133_ENUM_NAME},
    #endif
    #ifdef OPT_133_CLI_LONG_TXT_B
          {OPT_133_CLI_LONG_TXT_B, OPT_133_HAS_ARG_LONG, 0, OPT_133_ENUM_NAME},
    #endif
    #ifdef OPT_133_CLI_LONG_TXT_C
          {OPT_133_CLI_LONG_TXT_C, OPT_133_HAS_ARG_LONG, 0, OPT_133_ENUM_NAME},
    #endif
    #ifdef OPT_133_CLI_LONG_TXT_D
          {OPT_133_CLI_LONG_TXT_D, OPT_133_HAS_ARG_LONG, 0, OPT_133_ENUM_NAME},
    #endif
    #ifdef OPT_133_CLI_SHORT_CODE_TXT
          {OPT_133_CLI_SHORT_CODE_TXT, OPT_133_HAS_ARG_LONG, 0, OPT_133_ENUM_NAME},
    #endif
#endif
#ifdef OPT_134_ENUM_NAME
    #ifdef OPT_134_CLI_LONG_TXT_A
          {OPT_134_CLI_LONG_TXT_A, OPT_134_HAS_ARG_LONG, 0, OPT_134_ENUM_NAME},
    #endif
    #ifdef OPT_134_CLI_LONG_TXT_B
          {OPT_134_CLI_LONG_TXT_B, OPT_134_HAS_ARG_LONG, 0, OPT_134_ENUM_NAME},
    #endif
    #ifdef OPT_134_CLI_LONG_TXT_C
          {OPT_134_CLI_LONG_TXT_C, OPT_134_HAS_ARG_LONG, 0, OPT_134_ENUM_NAME},
    #endif
    #ifdef OPT_134_CLI_LONG_TXT_D
          {OPT_134_CLI_LONG_TXT_D, OPT_134_HAS_ARG_LONG, 0, OPT_134_ENUM_NAME},
    #endif
    #ifdef OPT_134_CLI_SHORT_CODE_TXT
          {OPT_134_CLI_SHORT_CODE_TXT, OPT_134_HAS_ARG_LONG, 0, OPT_134_ENUM_NAME},
    #endif
#endif
#ifdef OPT_135_ENUM_NAME
    #ifdef OPT_135_CLI_LONG_TXT_A
          {OPT_135_CLI_LONG_TXT_A, OPT_135_HAS_ARG_LONG, 0, OPT_135_ENUM_NAME},
    #endif
    #ifdef OPT_135_CLI_LONG_TXT_B
          {OPT_135_CLI_LONG_TXT_B, OPT_135_HAS_ARG_LONG, 0, OPT_135_ENUM_NAME},
    #endif
    #ifdef OPT_135_CLI_LONG_TXT_C
          {OPT_135_CLI_LONG_TXT_C, OPT_135_HAS_ARG_LONG, 0, OPT_135_ENUM_NAME},
    #endif
    #ifdef OPT_135_CLI_LONG_TXT_D
          {OPT_135_CLI_LONG_TXT_D, OPT_135_HAS_ARG_LONG, 0, OPT_135_ENUM_NAME},
    #endif
    #ifdef OPT_135_CLI_SHORT_CODE_TXT
          {OPT_135_CLI_SHORT_CODE_TXT, OPT_135_HAS_ARG_LONG, 0, OPT_135_ENUM_NAME},
    #endif
#endif
#ifdef OPT_136_ENUM_NAME
    #ifdef OPT_136_CLI_LONG_TXT_A
          {OPT_136_CLI_LONG_TXT_A, OPT_136_HAS_ARG_LONG, 0, OPT_136_ENUM_NAME},
    #endif
    #ifdef OPT_136_CLI_LONG_TXT_B
          {OPT_136_CLI_LONG_TXT_B, OPT_136_HAS_ARG_LONG, 0, OPT_136_ENUM_NAME},
    #endif
    #ifdef OPT_136_CLI_LONG_TXT_C
          {OPT_136_CLI_LONG_TXT_C, OPT_136_HAS_ARG_LONG, 0, OPT_136_ENUM_NAME},
    #endif
    #ifdef OPT_136_CLI_LONG_TXT_D
          {OPT_136_CLI_LONG_TXT_D, OPT_136_HAS_ARG_LONG, 0, OPT_136_ENUM_NAME},
    #endif
    #ifdef OPT_136_CLI_SHORT_CODE_TXT
          {OPT_136_CLI_SHORT_CODE_TXT, OPT_136_HAS_ARG_LONG, 0, OPT_136_ENUM_NAME},
    #endif
#endif
#ifdef OPT_137_ENUM_NAME
    #ifdef OPT_137_CLI_LONG_TXT_A
          {OPT_137_CLI_LONG_TXT_A, OPT_137_HAS_ARG_LONG, 0, OPT_137_ENUM_NAME},
    #endif
    #ifdef OPT_137_CLI_LONG_TXT_B
          {OPT_137_CLI_LONG_TXT_B, OPT_137_HAS_ARG_LONG, 0, OPT_137_ENUM_NAME},
    #endif
    #ifdef OPT_137_CLI_LONG_TXT_C
          {OPT_137_CLI_LONG_TXT_C, OPT_137_HAS_ARG_LONG, 0, OPT_137_ENUM_NAME},
    #endif
    #ifdef OPT_137_CLI_LONG_TXT_D
          {OPT_137_CLI_LONG_TXT_D, OPT_137_HAS_ARG_LONG, 0, OPT_137_ENUM_NAME},
    #endif
    #ifdef OPT_137_CLI_SHORT_CODE_TXT
          {OPT_137_CLI_SHORT_CODE_TXT, OPT_137_HAS_ARG_LONG, 0, OPT_137_ENUM_NAME},
    #endif
#endif
#ifdef OPT_138_ENUM_NAME
    #ifdef OPT_138_CLI_LONG_TXT_A
          {OPT_138_CLI_LONG_TXT_A, OPT_138_HAS_ARG_LONG, 0, OPT_138_ENUM_NAME},
    #endif
    #ifdef OPT_138_CLI_LONG_TXT_B
          {OPT_138_CLI_LONG_TXT_B, OPT_138_HAS_ARG_LONG, 0, OPT_138_ENUM_NAME},
    #endif
    #ifdef OPT_138_CLI_LONG_TXT_C
          {OPT_138_CLI_LONG_TXT_C, OPT_138_HAS_ARG_LONG, 0, OPT_138_ENUM_NAME},
    #endif
    #ifdef OPT_138_CLI_LONG_TXT_D
          {OPT_138_CLI_LONG_TXT_D, OPT_138_HAS_ARG_LONG, 0, OPT_138_ENUM_NAME},
    #endif
    #ifdef OPT_138_CLI_SHORT_CODE_TXT
          {OPT_138_CLI_SHORT_CODE_TXT, OPT_138_HAS_ARG_LONG, 0, OPT_138_ENUM_NAME},
    #endif
#endif
#ifdef OPT_139_ENUM_NAME
    #ifdef OPT_139_CLI_LONG_TXT_A
          {OPT_139_CLI_LONG_TXT_A, OPT_139_HAS_ARG_LONG, 0, OPT_139_ENUM_NAME},
    #endif
    #ifdef OPT_139_CLI_LONG_TXT_B
          {OPT_139_CLI_LONG_TXT_B, OPT_139_HAS_ARG_LONG, 0, OPT_139_ENUM_NAME},
    #endif
    #ifdef OPT_139_CLI_LONG_TXT_C
          {OPT_139_CLI_LONG_TXT_C, OPT_139_HAS_ARG_LONG, 0, OPT_139_ENUM_NAME},
    #endif
    #ifdef OPT_139_CLI_LONG_TXT_D
          {OPT_139_CLI_LONG_TXT_D, OPT_139_HAS_ARG_LONG, 0, OPT_139_ENUM_NAME},
    #endif
    #ifdef OPT_139_CLI_SHORT_CODE_TXT
          {OPT_139_CLI_SHORT_CODE_TXT, OPT_139_HAS_ARG_LONG, 0, OPT_139_ENUM_NAME},
    #endif
#endif

#ifdef OPT_140_ENUM_NAME
    #ifdef OPT_140_CLI_LONG_TXT_A
          {OPT_140_CLI_LONG_TXT_A, OPT_140_HAS_ARG_LONG, 0, OPT_140_ENUM_NAME},
    #endif
    #ifdef OPT_140_CLI_LONG_TXT_B
          {OPT_140_CLI_LONG_TXT_B, OPT_140_HAS_ARG_LONG, 0, OPT_140_ENUM_NAME},
    #endif
    #ifdef OPT_140_CLI_LONG_TXT_C
          {OPT_140_CLI_LONG_TXT_C, OPT_140_HAS_ARG_LONG, 0, OPT_140_ENUM_NAME},
    #endif
    #ifdef OPT_140_CLI_LONG_TXT_D
          {OPT_140_CLI_LONG_TXT_D, OPT_140_HAS_ARG_LONG, 0, OPT_140_ENUM_NAME},
    #endif
    #ifdef OPT_140_CLI_SHORT_CODE_TXT
          {OPT_140_CLI_SHORT_CODE_TXT, OPT_140_HAS_ARG_LONG, 0, OPT_140_ENUM_NAME},
    #endif
#endif
#ifdef OPT_141_ENUM_NAME
    #ifdef OPT_141_CLI_LONG_TXT_A
          {OPT_141_CLI_LONG_TXT_A, OPT_141_HAS_ARG_LONG, 0, OPT_141_ENUM_NAME},
    #endif
    #ifdef OPT_141_CLI_LONG_TXT_B
          {OPT_141_CLI_LONG_TXT_B, OPT_141_HAS_ARG_LONG, 0, OPT_141_ENUM_NAME},
    #endif
    #ifdef OPT_141_CLI_LONG_TXT_C
          {OPT_141_CLI_LONG_TXT_C, OPT_141_HAS_ARG_LONG, 0, OPT_141_ENUM_NAME},
    #endif
    #ifdef OPT_141_CLI_LONG_TXT_D
          {OPT_141_CLI_LONG_TXT_D, OPT_141_HAS_ARG_LONG, 0, OPT_141_ENUM_NAME},
    #endif
    #ifdef OPT_141_CLI_SHORT_CODE_TXT
          {OPT_141_CLI_SHORT_CODE_TXT, OPT_141_HAS_ARG_LONG, 0, OPT_141_ENUM_NAME},
    #endif
#endif
#ifdef OPT_142_ENUM_NAME
    #ifdef OPT_142_CLI_LONG_TXT_A
          {OPT_142_CLI_LONG_TXT_A, OPT_142_HAS_ARG_LONG, 0, OPT_142_ENUM_NAME},
    #endif
    #ifdef OPT_142_CLI_LONG_TXT_B
          {OPT_142_CLI_LONG_TXT_B, OPT_142_HAS_ARG_LONG, 0, OPT_142_ENUM_NAME},
    #endif
    #ifdef OPT_142_CLI_LONG_TXT_C
          {OPT_142_CLI_LONG_TXT_C, OPT_142_HAS_ARG_LONG, 0, OPT_142_ENUM_NAME},
    #endif
    #ifdef OPT_142_CLI_LONG_TXT_D
          {OPT_142_CLI_LONG_TXT_D, OPT_142_HAS_ARG_LONG, 0, OPT_142_ENUM_NAME},
    #endif
    #ifdef OPT_142_CLI_SHORT_CODE_TXT
          {OPT_142_CLI_SHORT_CODE_TXT, OPT_142_HAS_ARG_LONG, 0, OPT_142_ENUM_NAME},
    #endif
#endif
#ifdef OPT_143_ENUM_NAME
    #ifdef OPT_143_CLI_LONG_TXT_A
          {OPT_143_CLI_LONG_TXT_A, OPT_143_HAS_ARG_LONG, 0, OPT_143_ENUM_NAME},
    #endif
    #ifdef OPT_143_CLI_LONG_TXT_B
          {OPT_143_CLI_LONG_TXT_B, OPT_143_HAS_ARG_LONG, 0, OPT_143_ENUM_NAME},
    #endif
    #ifdef OPT_143_CLI_LONG_TXT_C
          {OPT_143_CLI_LONG_TXT_C, OPT_143_HAS_ARG_LONG, 0, OPT_143_ENUM_NAME},
    #endif
    #ifdef OPT_143_CLI_LONG_TXT_D
          {OPT_143_CLI_LONG_TXT_D, OPT_143_HAS_ARG_LONG, 0, OPT_143_ENUM_NAME},
    #endif
    #ifdef OPT_143_CLI_SHORT_CODE_TXT
          {OPT_143_CLI_SHORT_CODE_TXT, OPT_143_HAS_ARG_LONG, 0, OPT_143_ENUM_NAME},
    #endif
#endif
#ifdef OPT_144_ENUM_NAME
    #ifdef OPT_144_CLI_LONG_TXT_A
          {OPT_144_CLI_LONG_TXT_A, OPT_144_HAS_ARG_LONG, 0, OPT_144_ENUM_NAME},
    #endif
    #ifdef OPT_144_CLI_LONG_TXT_B
          {OPT_144_CLI_LONG_TXT_B, OPT_144_HAS_ARG_LONG, 0, OPT_144_ENUM_NAME},
    #endif
    #ifdef OPT_144_CLI_LONG_TXT_C
          {OPT_144_CLI_LONG_TXT_C, OPT_144_HAS_ARG_LONG, 0, OPT_144_ENUM_NAME},
    #endif
    #ifdef OPT_144_CLI_LONG_TXT_D
          {OPT_144_CLI_LONG_TXT_D, OPT_144_HAS_ARG_LONG, 0, OPT_144_ENUM_NAME},
    #endif
    #ifdef OPT_144_CLI_SHORT_CODE_TXT
          {OPT_144_CLI_SHORT_CODE_TXT, OPT_144_HAS_ARG_LONG, 0, OPT_144_ENUM_NAME},
    #endif
#endif
#ifdef OPT_145_ENUM_NAME
    #ifdef OPT_145_CLI_LONG_TXT_A
          {OPT_145_CLI_LONG_TXT_A, OPT_145_HAS_ARG_LONG, 0, OPT_145_ENUM_NAME},
    #endif
    #ifdef OPT_145_CLI_LONG_TXT_B
          {OPT_145_CLI_LONG_TXT_B, OPT_145_HAS_ARG_LONG, 0, OPT_145_ENUM_NAME},
    #endif
    #ifdef OPT_145_CLI_LONG_TXT_C
          {OPT_145_CLI_LONG_TXT_C, OPT_145_HAS_ARG_LONG, 0, OPT_145_ENUM_NAME},
    #endif
    #ifdef OPT_145_CLI_LONG_TXT_D
          {OPT_145_CLI_LONG_TXT_D, OPT_145_HAS_ARG_LONG, 0, OPT_145_ENUM_NAME},
    #endif
    #ifdef OPT_145_CLI_SHORT_CODE_TXT
          {OPT_145_CLI_SHORT_CODE_TXT, OPT_145_HAS_ARG_LONG, 0, OPT_145_ENUM_NAME},
    #endif
#endif
#ifdef OPT_146_ENUM_NAME
    #ifdef OPT_146_CLI_LONG_TXT_A
          {OPT_146_CLI_LONG_TXT_A, OPT_146_HAS_ARG_LONG, 0, OPT_146_ENUM_NAME},
    #endif
    #ifdef OPT_146_CLI_LONG_TXT_B
          {OPT_146_CLI_LONG_TXT_B, OPT_146_HAS_ARG_LONG, 0, OPT_146_ENUM_NAME},
    #endif
    #ifdef OPT_146_CLI_LONG_TXT_C
          {OPT_146_CLI_LONG_TXT_C, OPT_146_HAS_ARG_LONG, 0, OPT_146_ENUM_NAME},
    #endif
    #ifdef OPT_146_CLI_LONG_TXT_D
          {OPT_146_CLI_LONG_TXT_D, OPT_146_HAS_ARG_LONG, 0, OPT_146_ENUM_NAME},
    #endif
    #ifdef OPT_146_CLI_SHORT_CODE_TXT
          {OPT_146_CLI_SHORT_CODE_TXT, OPT_146_HAS_ARG_LONG, 0, OPT_146_ENUM_NAME},
    #endif
#endif
#ifdef OPT_147_ENUM_NAME
    #ifdef OPT_147_CLI_LONG_TXT_A
          {OPT_147_CLI_LONG_TXT_A, OPT_147_HAS_ARG_LONG, 0, OPT_147_ENUM_NAME},
    #endif
    #ifdef OPT_147_CLI_LONG_TXT_B
          {OPT_147_CLI_LONG_TXT_B, OPT_147_HAS_ARG_LONG, 0, OPT_147_ENUM_NAME},
    #endif
    #ifdef OPT_147_CLI_LONG_TXT_C
          {OPT_147_CLI_LONG_TXT_C, OPT_147_HAS_ARG_LONG, 0, OPT_147_ENUM_NAME},
    #endif
    #ifdef OPT_147_CLI_LONG_TXT_D
          {OPT_147_CLI_LONG_TXT_D, OPT_147_HAS_ARG_LONG, 0, OPT_147_ENUM_NAME},
    #endif
    #ifdef OPT_147_CLI_SHORT_CODE_TXT
          {OPT_147_CLI_SHORT_CODE_TXT, OPT_147_HAS_ARG_LONG, 0, OPT_147_ENUM_NAME},
    #endif
#endif
#ifdef OPT_148_ENUM_NAME
    #ifdef OPT_148_CLI_LONG_TXT_A
          {OPT_148_CLI_LONG_TXT_A, OPT_148_HAS_ARG_LONG, 0, OPT_148_ENUM_NAME},
    #endif
    #ifdef OPT_148_CLI_LONG_TXT_B
          {OPT_148_CLI_LONG_TXT_B, OPT_148_HAS_ARG_LONG, 0, OPT_148_ENUM_NAME},
    #endif
    #ifdef OPT_148_CLI_LONG_TXT_C
          {OPT_148_CLI_LONG_TXT_C, OPT_148_HAS_ARG_LONG, 0, OPT_148_ENUM_NAME},
    #endif
    #ifdef OPT_148_CLI_LONG_TXT_D
          {OPT_148_CLI_LONG_TXT_D, OPT_148_HAS_ARG_LONG, 0, OPT_148_ENUM_NAME},
    #endif
    #ifdef OPT_148_CLI_SHORT_CODE_TXT
          {OPT_148_CLI_SHORT_CODE_TXT, OPT_148_HAS_ARG_LONG, 0, OPT_148_ENUM_NAME},
    #endif
#endif
#ifdef OPT_149_ENUM_NAME
    #ifdef OPT_149_CLI_LONG_TXT_A
          {OPT_149_CLI_LONG_TXT_A, OPT_149_HAS_ARG_LONG, 0, OPT_149_ENUM_NAME},
    #endif
    #ifdef OPT_149_CLI_LONG_TXT_B
          {OPT_149_CLI_LONG_TXT_B, OPT_149_HAS_ARG_LONG, 0, OPT_149_ENUM_NAME},
    #endif
    #ifdef OPT_149_CLI_LONG_TXT_C
          {OPT_149_CLI_LONG_TXT_C, OPT_149_HAS_ARG_LONG, 0, OPT_149_ENUM_NAME},
    #endif
    #ifdef OPT_149_CLI_LONG_TXT_D
          {OPT_149_CLI_LONG_TXT_D, OPT_149_HAS_ARG_LONG, 0, OPT_149_ENUM_NAME},
    #endif
    #ifdef OPT_149_CLI_SHORT_CODE_TXT
          {OPT_149_CLI_SHORT_CODE_TXT, OPT_149_HAS_ARG_LONG, 0, OPT_149_ENUM_NAME},
    #endif
#endif

#ifdef OPT_150_ENUM_NAME
    #ifdef OPT_150_CLI_LONG_TXT_A
          {OPT_150_CLI_LONG_TXT_A, OPT_150_HAS_ARG_LONG, 0, OPT_150_ENUM_NAME},
    #endif
    #ifdef OPT_150_CLI_LONG_TXT_B
          {OPT_150_CLI_LONG_TXT_B, OPT_150_HAS_ARG_LONG, 0, OPT_150_ENUM_NAME},
    #endif
    #ifdef OPT_150_CLI_LONG_TXT_C
          {OPT_150_CLI_LONG_TXT_C, OPT_150_HAS_ARG_LONG, 0, OPT_150_ENUM_NAME},
    #endif
    #ifdef OPT_150_CLI_LONG_TXT_D
          {OPT_150_CLI_LONG_TXT_D, OPT_150_HAS_ARG_LONG, 0, OPT_150_ENUM_NAME},
    #endif
    #ifdef OPT_150_CLI_SHORT_CODE_TXT
          {OPT_150_CLI_SHORT_CODE_TXT, OPT_150_HAS_ARG_LONG, 0, OPT_150_ENUM_NAME},
    #endif
#endif
#ifdef OPT_151_ENUM_NAME
    #ifdef OPT_151_CLI_LONG_TXT_A
          {OPT_151_CLI_LONG_TXT_A, OPT_151_HAS_ARG_LONG, 0, OPT_151_ENUM_NAME},
    #endif
    #ifdef OPT_151_CLI_LONG_TXT_B
          {OPT_151_CLI_LONG_TXT_B, OPT_151_HAS_ARG_LONG, 0, OPT_151_ENUM_NAME},
    #endif
    #ifdef OPT_151_CLI_LONG_TXT_C
          {OPT_151_CLI_LONG_TXT_C, OPT_151_HAS_ARG_LONG, 0, OPT_151_ENUM_NAME},
    #endif
    #ifdef OPT_151_CLI_LONG_TXT_D
          {OPT_151_CLI_LONG_TXT_D, OPT_151_HAS_ARG_LONG, 0, OPT_151_ENUM_NAME},
    #endif
    #ifdef OPT_151_CLI_SHORT_CODE_TXT
          {OPT_151_CLI_SHORT_CODE_TXT, OPT_151_HAS_ARG_LONG, 0, OPT_151_ENUM_NAME},
    #endif
#endif
#ifdef OPT_152_ENUM_NAME
    #ifdef OPT_152_CLI_LONG_TXT_A
          {OPT_152_CLI_LONG_TXT_A, OPT_152_HAS_ARG_LONG, 0, OPT_152_ENUM_NAME},
    #endif
    #ifdef OPT_152_CLI_LONG_TXT_B
          {OPT_152_CLI_LONG_TXT_B, OPT_152_HAS_ARG_LONG, 0, OPT_152_ENUM_NAME},
    #endif
    #ifdef OPT_152_CLI_LONG_TXT_C
          {OPT_152_CLI_LONG_TXT_C, OPT_152_HAS_ARG_LONG, 0, OPT_152_ENUM_NAME},
    #endif
    #ifdef OPT_152_CLI_LONG_TXT_D
          {OPT_152_CLI_LONG_TXT_D, OPT_152_HAS_ARG_LONG, 0, OPT_152_ENUM_NAME},
    #endif
    #ifdef OPT_152_CLI_SHORT_CODE_TXT
          {OPT_152_CLI_SHORT_CODE_TXT, OPT_152_HAS_ARG_LONG, 0, OPT_152_ENUM_NAME},
    #endif
#endif
#ifdef OPT_153_ENUM_NAME
    #ifdef OPT_153_CLI_LONG_TXT_A
          {OPT_153_CLI_LONG_TXT_A, OPT_153_HAS_ARG_LONG, 0, OPT_153_ENUM_NAME},
    #endif
    #ifdef OPT_153_CLI_LONG_TXT_B
          {OPT_153_CLI_LONG_TXT_B, OPT_153_HAS_ARG_LONG, 0, OPT_153_ENUM_NAME},
    #endif
    #ifdef OPT_153_CLI_LONG_TXT_C
          {OPT_153_CLI_LONG_TXT_C, OPT_153_HAS_ARG_LONG, 0, OPT_153_ENUM_NAME},
    #endif
    #ifdef OPT_153_CLI_LONG_TXT_D
          {OPT_153_CLI_LONG_TXT_D, OPT_153_HAS_ARG_LONG, 0, OPT_153_ENUM_NAME},
    #endif
    #ifdef OPT_153_CLI_SHORT_CODE_TXT
          {OPT_153_CLI_SHORT_CODE_TXT, OPT_153_HAS_ARG_LONG, 0, OPT_153_ENUM_NAME},
    #endif
#endif
#ifdef OPT_154_ENUM_NAME
    #ifdef OPT_154_CLI_LONG_TXT_A
          {OPT_154_CLI_LONG_TXT_A, OPT_154_HAS_ARG_LONG, 0, OPT_154_ENUM_NAME},
    #endif
    #ifdef OPT_154_CLI_LONG_TXT_B
          {OPT_154_CLI_LONG_TXT_B, OPT_154_HAS_ARG_LONG, 0, OPT_154_ENUM_NAME},
    #endif
    #ifdef OPT_154_CLI_LONG_TXT_C
          {OPT_154_CLI_LONG_TXT_C, OPT_154_HAS_ARG_LONG, 0, OPT_154_ENUM_NAME},
    #endif
    #ifdef OPT_154_CLI_LONG_TXT_D
          {OPT_154_CLI_LONG_TXT_D, OPT_154_HAS_ARG_LONG, 0, OPT_154_ENUM_NAME},
    #endif
    #ifdef OPT_154_CLI_SHORT_CODE_TXT
          {OPT_154_CLI_SHORT_CODE_TXT, OPT_154_HAS_ARG_LONG, 0, OPT_154_ENUM_NAME},
    #endif
#endif
#ifdef OPT_155_ENUM_NAME
    #ifdef OPT_155_CLI_LONG_TXT_A
          {OPT_155_CLI_LONG_TXT_A, OPT_155_HAS_ARG_LONG, 0, OPT_155_ENUM_NAME},
    #endif
    #ifdef OPT_155_CLI_LONG_TXT_B
          {OPT_155_CLI_LONG_TXT_B, OPT_155_HAS_ARG_LONG, 0, OPT_155_ENUM_NAME},
    #endif
    #ifdef OPT_155_CLI_LONG_TXT_C
          {OPT_155_CLI_LONG_TXT_C, OPT_155_HAS_ARG_LONG, 0, OPT_155_ENUM_NAME},
    #endif
    #ifdef OPT_155_CLI_LONG_TXT_D
          {OPT_155_CLI_LONG_TXT_D, OPT_155_HAS_ARG_LONG, 0, OPT_155_ENUM_NAME},
    #endif
    #ifdef OPT_155_CLI_SHORT_CODE_TXT
          {OPT_155_CLI_SHORT_CODE_TXT, OPT_155_HAS_ARG_LONG, 0, OPT_155_ENUM_NAME},
    #endif
#endif
#ifdef OPT_156_ENUM_NAME
    #ifdef OPT_156_CLI_LONG_TXT_A
          {OPT_156_CLI_LONG_TXT_A, OPT_156_HAS_ARG_LONG, 0, OPT_156_ENUM_NAME},
    #endif
    #ifdef OPT_156_CLI_LONG_TXT_B
          {OPT_156_CLI_LONG_TXT_B, OPT_156_HAS_ARG_LONG, 0, OPT_156_ENUM_NAME},
    #endif
    #ifdef OPT_156_CLI_LONG_TXT_C
          {OPT_156_CLI_LONG_TXT_C, OPT_156_HAS_ARG_LONG, 0, OPT_156_ENUM_NAME},
    #endif
    #ifdef OPT_156_CLI_LONG_TXT_D
          {OPT_156_CLI_LONG_TXT_D, OPT_156_HAS_ARG_LONG, 0, OPT_156_ENUM_NAME},
    #endif
    #ifdef OPT_156_CLI_SHORT_CODE_TXT
          {OPT_156_CLI_SHORT_CODE_TXT, OPT_156_HAS_ARG_LONG, 0, OPT_156_ENUM_NAME},
    #endif
#endif
#ifdef OPT_157_ENUM_NAME
    #ifdef OPT_157_CLI_LONG_TXT_A
          {OPT_157_CLI_LONG_TXT_A, OPT_157_HAS_ARG_LONG, 0, OPT_157_ENUM_NAME},
    #endif
    #ifdef OPT_157_CLI_LONG_TXT_B
          {OPT_157_CLI_LONG_TXT_B, OPT_157_HAS_ARG_LONG, 0, OPT_157_ENUM_NAME},
    #endif
    #ifdef OPT_157_CLI_LONG_TXT_C
          {OPT_157_CLI_LONG_TXT_C, OPT_157_HAS_ARG_LONG, 0, OPT_157_ENUM_NAME},
    #endif
    #ifdef OPT_157_CLI_LONG_TXT_D
          {OPT_157_CLI_LONG_TXT_D, OPT_157_HAS_ARG_LONG, 0, OPT_157_ENUM_NAME},
    #endif
    #ifdef OPT_157_CLI_SHORT_CODE_TXT
          {OPT_157_CLI_SHORT_CODE_TXT, OPT_157_HAS_ARG_LONG, 0, OPT_157_ENUM_NAME},
    #endif
#endif
#ifdef OPT_158_ENUM_NAME
    #ifdef OPT_158_CLI_LONG_TXT_A
          {OPT_158_CLI_LONG_TXT_A, OPT_158_HAS_ARG_LONG, 0, OPT_158_ENUM_NAME},
    #endif
    #ifdef OPT_158_CLI_LONG_TXT_B
          {OPT_158_CLI_LONG_TXT_B, OPT_158_HAS_ARG_LONG, 0, OPT_158_ENUM_NAME},
    #endif
    #ifdef OPT_158_CLI_LONG_TXT_C
          {OPT_158_CLI_LONG_TXT_C, OPT_158_HAS_ARG_LONG, 0, OPT_158_ENUM_NAME},
    #endif
    #ifdef OPT_158_CLI_LONG_TXT_D
          {OPT_158_CLI_LONG_TXT_D, OPT_158_HAS_ARG_LONG, 0, OPT_158_ENUM_NAME},
    #endif
    #ifdef OPT_158_CLI_SHORT_CODE_TXT
          {OPT_158_CLI_SHORT_CODE_TXT, OPT_158_HAS_ARG_LONG, 0, OPT_158_ENUM_NAME},
    #endif
#endif
#ifdef OPT_159_ENUM_NAME
    #ifdef OPT_159_CLI_LONG_TXT_A
          {OPT_159_CLI_LONG_TXT_A, OPT_159_HAS_ARG_LONG, 0, OPT_159_ENUM_NAME},
    #endif
    #ifdef OPT_159_CLI_LONG_TXT_B
          {OPT_159_CLI_LONG_TXT_B, OPT_159_HAS_ARG_LONG, 0, OPT_159_ENUM_NAME},
    #endif
    #ifdef OPT_159_CLI_LONG_TXT_C
          {OPT_159_CLI_LONG_TXT_C, OPT_159_HAS_ARG_LONG, 0, OPT_159_ENUM_NAME},
    #endif
    #ifdef OPT_159_CLI_LONG_TXT_D
          {OPT_159_CLI_LONG_TXT_D, OPT_159_HAS_ARG_LONG, 0, OPT_159_ENUM_NAME},
    #endif
    #ifdef OPT_159_CLI_SHORT_CODE_TXT
          {OPT_159_CLI_SHORT_CODE_TXT, OPT_159_HAS_ARG_LONG, 0, OPT_159_ENUM_NAME},
    #endif
#endif

#ifdef OPT_160_ENUM_NAME
    #ifdef OPT_160_CLI_LONG_TXT_A
          {OPT_160_CLI_LONG_TXT_A, OPT_160_HAS_ARG_LONG, 0, OPT_160_ENUM_NAME},
    #endif
    #ifdef OPT_160_CLI_LONG_TXT_B
          {OPT_160_CLI_LONG_TXT_B, OPT_160_HAS_ARG_LONG, 0, OPT_160_ENUM_NAME},
    #endif
    #ifdef OPT_160_CLI_LONG_TXT_C
          {OPT_160_CLI_LONG_TXT_C, OPT_160_HAS_ARG_LONG, 0, OPT_160_ENUM_NAME},
    #endif
    #ifdef OPT_160_CLI_LONG_TXT_D
          {OPT_160_CLI_LONG_TXT_D, OPT_160_HAS_ARG_LONG, 0, OPT_160_ENUM_NAME},
    #endif
    #ifdef OPT_160_CLI_SHORT_CODE_TXT
          {OPT_160_CLI_SHORT_CODE_TXT, OPT_160_HAS_ARG_LONG, 0, OPT_160_ENUM_NAME},
    #endif
#endif
#ifdef OPT_161_ENUM_NAME
    #ifdef OPT_161_CLI_LONG_TXT_A
          {OPT_161_CLI_LONG_TXT_A, OPT_161_HAS_ARG_LONG, 0, OPT_161_ENUM_NAME},
    #endif
    #ifdef OPT_161_CLI_LONG_TXT_B
          {OPT_161_CLI_LONG_TXT_B, OPT_161_HAS_ARG_LONG, 0, OPT_161_ENUM_NAME},
    #endif
    #ifdef OPT_161_CLI_LONG_TXT_C
          {OPT_161_CLI_LONG_TXT_C, OPT_161_HAS_ARG_LONG, 0, OPT_161_ENUM_NAME},
    #endif
    #ifdef OPT_161_CLI_LONG_TXT_D
          {OPT_161_CLI_LONG_TXT_D, OPT_161_HAS_ARG_LONG, 0, OPT_161_ENUM_NAME},
    #endif
    #ifdef OPT_161_CLI_SHORT_CODE_TXT
          {OPT_161_CLI_SHORT_CODE_TXT, OPT_161_HAS_ARG_LONG, 0, OPT_161_ENUM_NAME},
    #endif
#endif
#ifdef OPT_162_ENUM_NAME
    #ifdef OPT_162_CLI_LONG_TXT_A
          {OPT_162_CLI_LONG_TXT_A, OPT_162_HAS_ARG_LONG, 0, OPT_162_ENUM_NAME},
    #endif
    #ifdef OPT_162_CLI_LONG_TXT_B
          {OPT_162_CLI_LONG_TXT_B, OPT_162_HAS_ARG_LONG, 0, OPT_162_ENUM_NAME},
    #endif
    #ifdef OPT_162_CLI_LONG_TXT_C
          {OPT_162_CLI_LONG_TXT_C, OPT_162_HAS_ARG_LONG, 0, OPT_162_ENUM_NAME},
    #endif
    #ifdef OPT_162_CLI_LONG_TXT_D
          {OPT_162_CLI_LONG_TXT_D, OPT_162_HAS_ARG_LONG, 0, OPT_162_ENUM_NAME},
    #endif
    #ifdef OPT_162_CLI_SHORT_CODE_TXT
          {OPT_162_CLI_SHORT_CODE_TXT, OPT_162_HAS_ARG_LONG, 0, OPT_162_ENUM_NAME},
    #endif
#endif
#ifdef OPT_163_ENUM_NAME
    #ifdef OPT_163_CLI_LONG_TXT_A
          {OPT_163_CLI_LONG_TXT_A, OPT_163_HAS_ARG_LONG, 0, OPT_163_ENUM_NAME},
    #endif
    #ifdef OPT_163_CLI_LONG_TXT_B
          {OPT_163_CLI_LONG_TXT_B, OPT_163_HAS_ARG_LONG, 0, OPT_163_ENUM_NAME},
    #endif
    #ifdef OPT_163_CLI_LONG_TXT_C
          {OPT_163_CLI_LONG_TXT_C, OPT_163_HAS_ARG_LONG, 0, OPT_163_ENUM_NAME},
    #endif
    #ifdef OPT_163_CLI_LONG_TXT_D
          {OPT_163_CLI_LONG_TXT_D, OPT_163_HAS_ARG_LONG, 0, OPT_163_ENUM_NAME},
    #endif
    #ifdef OPT_163_CLI_SHORT_CODE_TXT
          {OPT_163_CLI_SHORT_CODE_TXT, OPT_163_HAS_ARG_LONG, 0, OPT_163_ENUM_NAME},
    #endif
#endif
#ifdef OPT_164_ENUM_NAME
    #ifdef OPT_164_CLI_LONG_TXT_A
          {OPT_164_CLI_LONG_TXT_A, OPT_164_HAS_ARG_LONG, 0, OPT_164_ENUM_NAME},
    #endif
    #ifdef OPT_164_CLI_LONG_TXT_B
          {OPT_164_CLI_LONG_TXT_B, OPT_164_HAS_ARG_LONG, 0, OPT_164_ENUM_NAME},
    #endif
    #ifdef OPT_164_CLI_LONG_TXT_C
          {OPT_164_CLI_LONG_TXT_C, OPT_164_HAS_ARG_LONG, 0, OPT_164_ENUM_NAME},
    #endif
    #ifdef OPT_164_CLI_LONG_TXT_D
          {OPT_164_CLI_LONG_TXT_D, OPT_164_HAS_ARG_LONG, 0, OPT_164_ENUM_NAME},
    #endif
    #ifdef OPT_164_CLI_SHORT_CODE_TXT
          {OPT_164_CLI_SHORT_CODE_TXT, OPT_164_HAS_ARG_LONG, 0, OPT_164_ENUM_NAME},
    #endif
#endif
#ifdef OPT_165_ENUM_NAME
    #ifdef OPT_165_CLI_LONG_TXT_A
          {OPT_165_CLI_LONG_TXT_A, OPT_165_HAS_ARG_LONG, 0, OPT_165_ENUM_NAME},
    #endif
    #ifdef OPT_165_CLI_LONG_TXT_B
          {OPT_165_CLI_LONG_TXT_B, OPT_165_HAS_ARG_LONG, 0, OPT_165_ENUM_NAME},
    #endif
    #ifdef OPT_165_CLI_LONG_TXT_C
          {OPT_165_CLI_LONG_TXT_C, OPT_165_HAS_ARG_LONG, 0, OPT_165_ENUM_NAME},
    #endif
    #ifdef OPT_165_CLI_LONG_TXT_D
          {OPT_165_CLI_LONG_TXT_D, OPT_165_HAS_ARG_LONG, 0, OPT_165_ENUM_NAME},
    #endif
    #ifdef OPT_165_CLI_SHORT_CODE_TXT
          {OPT_165_CLI_SHORT_CODE_TXT, OPT_165_HAS_ARG_LONG, 0, OPT_165_ENUM_NAME},
    #endif
#endif
#ifdef OPT_166_ENUM_NAME
    #ifdef OPT_166_CLI_LONG_TXT_A
          {OPT_166_CLI_LONG_TXT_A, OPT_166_HAS_ARG_LONG, 0, OPT_166_ENUM_NAME},
    #endif
    #ifdef OPT_166_CLI_LONG_TXT_B
          {OPT_166_CLI_LONG_TXT_B, OPT_166_HAS_ARG_LONG, 0, OPT_166_ENUM_NAME},
    #endif
    #ifdef OPT_166_CLI_LONG_TXT_C
          {OPT_166_CLI_LONG_TXT_C, OPT_166_HAS_ARG_LONG, 0, OPT_166_ENUM_NAME},
    #endif
    #ifdef OPT_166_CLI_LONG_TXT_D
          {OPT_166_CLI_LONG_TXT_D, OPT_166_HAS_ARG_LONG, 0, OPT_166_ENUM_NAME},
    #endif
    #ifdef OPT_166_CLI_SHORT_CODE_TXT
          {OPT_166_CLI_SHORT_CODE_TXT, OPT_166_HAS_ARG_LONG, 0, OPT_166_ENUM_NAME},
    #endif
#endif
#ifdef OPT_167_ENUM_NAME
    #ifdef OPT_167_CLI_LONG_TXT_A
          {OPT_167_CLI_LONG_TXT_A, OPT_167_HAS_ARG_LONG, 0, OPT_167_ENUM_NAME},
    #endif
    #ifdef OPT_167_CLI_LONG_TXT_B
          {OPT_167_CLI_LONG_TXT_B, OPT_167_HAS_ARG_LONG, 0, OPT_167_ENUM_NAME},
    #endif
    #ifdef OPT_167_CLI_LONG_TXT_C
          {OPT_167_CLI_LONG_TXT_C, OPT_167_HAS_ARG_LONG, 0, OPT_167_ENUM_NAME},
    #endif
    #ifdef OPT_167_CLI_LONG_TXT_D
          {OPT_167_CLI_LONG_TXT_D, OPT_167_HAS_ARG_LONG, 0, OPT_167_ENUM_NAME},
    #endif
    #ifdef OPT_167_CLI_SHORT_CODE_TXT
          {OPT_167_CLI_SHORT_CODE_TXT, OPT_167_HAS_ARG_LONG, 0, OPT_167_ENUM_NAME},
    #endif
#endif
#ifdef OPT_168_ENUM_NAME
    #ifdef OPT_168_CLI_LONG_TXT_A
          {OPT_168_CLI_LONG_TXT_A, OPT_168_HAS_ARG_LONG, 0, OPT_168_ENUM_NAME},
    #endif
    #ifdef OPT_168_CLI_LONG_TXT_B
          {OPT_168_CLI_LONG_TXT_B, OPT_168_HAS_ARG_LONG, 0, OPT_168_ENUM_NAME},
    #endif
    #ifdef OPT_168_CLI_LONG_TXT_C
          {OPT_168_CLI_LONG_TXT_C, OPT_168_HAS_ARG_LONG, 0, OPT_168_ENUM_NAME},
    #endif
    #ifdef OPT_168_CLI_LONG_TXT_D
          {OPT_168_CLI_LONG_TXT_D, OPT_168_HAS_ARG_LONG, 0, OPT_168_ENUM_NAME},
    #endif
    #ifdef OPT_168_CLI_SHORT_CODE_TXT
          {OPT_168_CLI_SHORT_CODE_TXT, OPT_168_HAS_ARG_LONG, 0, OPT_168_ENUM_NAME},
    #endif
#endif
#ifdef OPT_169_ENUM_NAME
    #ifdef OPT_169_CLI_LONG_TXT_A
          {OPT_169_CLI_LONG_TXT_A, OPT_169_HAS_ARG_LONG, 0, OPT_169_ENUM_NAME},
    #endif
    #ifdef OPT_169_CLI_LONG_TXT_B
          {OPT_169_CLI_LONG_TXT_B, OPT_169_HAS_ARG_LONG, 0, OPT_169_ENUM_NAME},
    #endif
    #ifdef OPT_169_CLI_LONG_TXT_C
          {OPT_169_CLI_LONG_TXT_C, OPT_169_HAS_ARG_LONG, 0, OPT_169_ENUM_NAME},
    #endif
    #ifdef OPT_169_CLI_LONG_TXT_D
          {OPT_169_CLI_LONG_TXT_D, OPT_169_HAS_ARG_LONG, 0, OPT_169_ENUM_NAME},
    #endif
    #ifdef OPT_169_CLI_SHORT_CODE_TXT
          {OPT_169_CLI_SHORT_CODE_TXT, OPT_169_HAS_ARG_LONG, 0, OPT_169_ENUM_NAME},
    #endif
#endif

#ifdef OPT_170_ENUM_NAME
    #ifdef OPT_170_CLI_LONG_TXT_A
          {OPT_170_CLI_LONG_TXT_A, OPT_170_HAS_ARG_LONG, 0, OPT_170_ENUM_NAME},
    #endif
    #ifdef OPT_170_CLI_LONG_TXT_B
          {OPT_170_CLI_LONG_TXT_B, OPT_170_HAS_ARG_LONG, 0, OPT_170_ENUM_NAME},
    #endif
    #ifdef OPT_170_CLI_LONG_TXT_C
          {OPT_170_CLI_LONG_TXT_C, OPT_170_HAS_ARG_LONG, 0, OPT_170_ENUM_NAME},
    #endif
    #ifdef OPT_170_CLI_LONG_TXT_D
          {OPT_170_CLI_LONG_TXT_D, OPT_170_HAS_ARG_LONG, 0, OPT_170_ENUM_NAME},
    #endif
    #ifdef OPT_170_CLI_SHORT_CODE_TXT
          {OPT_170_CLI_SHORT_CODE_TXT, OPT_170_HAS_ARG_LONG, 0, OPT_170_ENUM_NAME},
    #endif
#endif
#ifdef OPT_171_ENUM_NAME
    #ifdef OPT_171_CLI_LONG_TXT_A
          {OPT_171_CLI_LONG_TXT_A, OPT_171_HAS_ARG_LONG, 0, OPT_171_ENUM_NAME},
    #endif
    #ifdef OPT_171_CLI_LONG_TXT_B
          {OPT_171_CLI_LONG_TXT_B, OPT_171_HAS_ARG_LONG, 0, OPT_171_ENUM_NAME},
    #endif
    #ifdef OPT_171_CLI_LONG_TXT_C
          {OPT_171_CLI_LONG_TXT_C, OPT_171_HAS_ARG_LONG, 0, OPT_171_ENUM_NAME},
    #endif
    #ifdef OPT_171_CLI_LONG_TXT_D
          {OPT_171_CLI_LONG_TXT_D, OPT_171_HAS_ARG_LONG, 0, OPT_171_ENUM_NAME},
    #endif
    #ifdef OPT_171_CLI_SHORT_CODE_TXT
          {OPT_171_CLI_SHORT_CODE_TXT, OPT_171_HAS_ARG_LONG, 0, OPT_171_ENUM_NAME},
    #endif
#endif
#ifdef OPT_172_ENUM_NAME
    #ifdef OPT_172_CLI_LONG_TXT_A
          {OPT_172_CLI_LONG_TXT_A, OPT_172_HAS_ARG_LONG, 0, OPT_172_ENUM_NAME},
    #endif
    #ifdef OPT_172_CLI_LONG_TXT_B
          {OPT_172_CLI_LONG_TXT_B, OPT_172_HAS_ARG_LONG, 0, OPT_172_ENUM_NAME},
    #endif
    #ifdef OPT_172_CLI_LONG_TXT_C
          {OPT_172_CLI_LONG_TXT_C, OPT_172_HAS_ARG_LONG, 0, OPT_172_ENUM_NAME},
    #endif
    #ifdef OPT_172_CLI_LONG_TXT_D
          {OPT_172_CLI_LONG_TXT_D, OPT_172_HAS_ARG_LONG, 0, OPT_172_ENUM_NAME},
    #endif
    #ifdef OPT_172_CLI_SHORT_CODE_TXT
          {OPT_172_CLI_SHORT_CODE_TXT, OPT_172_HAS_ARG_LONG, 0, OPT_172_ENUM_NAME},
    #endif
#endif
#ifdef OPT_173_ENUM_NAME
    #ifdef OPT_173_CLI_LONG_TXT_A
          {OPT_173_CLI_LONG_TXT_A, OPT_173_HAS_ARG_LONG, 0, OPT_173_ENUM_NAME},
    #endif
    #ifdef OPT_173_CLI_LONG_TXT_B
          {OPT_173_CLI_LONG_TXT_B, OPT_173_HAS_ARG_LONG, 0, OPT_173_ENUM_NAME},
    #endif
    #ifdef OPT_173_CLI_LONG_TXT_C
          {OPT_173_CLI_LONG_TXT_C, OPT_173_HAS_ARG_LONG, 0, OPT_173_ENUM_NAME},
    #endif
    #ifdef OPT_173_CLI_LONG_TXT_D
          {OPT_173_CLI_LONG_TXT_D, OPT_173_HAS_ARG_LONG, 0, OPT_173_ENUM_NAME},
    #endif
    #ifdef OPT_173_CLI_SHORT_CODE_TXT
          {OPT_173_CLI_SHORT_CODE_TXT, OPT_173_HAS_ARG_LONG, 0, OPT_173_ENUM_NAME},
    #endif
#endif
#ifdef OPT_174_ENUM_NAME
    #ifdef OPT_174_CLI_LONG_TXT_A
          {OPT_174_CLI_LONG_TXT_A, OPT_174_HAS_ARG_LONG, 0, OPT_174_ENUM_NAME},
    #endif
    #ifdef OPT_174_CLI_LONG_TXT_B
          {OPT_174_CLI_LONG_TXT_B, OPT_174_HAS_ARG_LONG, 0, OPT_174_ENUM_NAME},
    #endif
    #ifdef OPT_174_CLI_LONG_TXT_C
          {OPT_174_CLI_LONG_TXT_C, OPT_174_HAS_ARG_LONG, 0, OPT_174_ENUM_NAME},
    #endif
    #ifdef OPT_174_CLI_LONG_TXT_D
          {OPT_174_CLI_LONG_TXT_D, OPT_174_HAS_ARG_LONG, 0, OPT_174_ENUM_NAME},
    #endif
    #ifdef OPT_174_CLI_SHORT_CODE_TXT
          {OPT_174_CLI_SHORT_CODE_TXT, OPT_174_HAS_ARG_LONG, 0, OPT_174_ENUM_NAME},
    #endif
#endif
#ifdef OPT_175_ENUM_NAME
    #ifdef OPT_175_CLI_LONG_TXT_A
          {OPT_175_CLI_LONG_TXT_A, OPT_175_HAS_ARG_LONG, 0, OPT_175_ENUM_NAME},
    #endif
    #ifdef OPT_175_CLI_LONG_TXT_B
          {OPT_175_CLI_LONG_TXT_B, OPT_175_HAS_ARG_LONG, 0, OPT_175_ENUM_NAME},
    #endif
    #ifdef OPT_175_CLI_LONG_TXT_C
          {OPT_175_CLI_LONG_TXT_C, OPT_175_HAS_ARG_LONG, 0, OPT_175_ENUM_NAME},
    #endif
    #ifdef OPT_175_CLI_LONG_TXT_D
          {OPT_175_CLI_LONG_TXT_D, OPT_175_HAS_ARG_LONG, 0, OPT_175_ENUM_NAME},
    #endif
    #ifdef OPT_175_CLI_SHORT_CODE_TXT
          {OPT_175_CLI_SHORT_CODE_TXT, OPT_175_HAS_ARG_LONG, 0, OPT_175_ENUM_NAME},
    #endif
#endif
#ifdef OPT_176_ENUM_NAME
    #ifdef OPT_176_CLI_LONG_TXT_A
          {OPT_176_CLI_LONG_TXT_A, OPT_176_HAS_ARG_LONG, 0, OPT_176_ENUM_NAME},
    #endif
    #ifdef OPT_176_CLI_LONG_TXT_B
          {OPT_176_CLI_LONG_TXT_B, OPT_176_HAS_ARG_LONG, 0, OPT_176_ENUM_NAME},
    #endif
    #ifdef OPT_176_CLI_LONG_TXT_C
          {OPT_176_CLI_LONG_TXT_C, OPT_176_HAS_ARG_LONG, 0, OPT_176_ENUM_NAME},
    #endif
    #ifdef OPT_176_CLI_LONG_TXT_D
          {OPT_176_CLI_LONG_TXT_D, OPT_176_HAS_ARG_LONG, 0, OPT_176_ENUM_NAME},
    #endif
    #ifdef OPT_176_CLI_SHORT_CODE_TXT
          {OPT_176_CLI_SHORT_CODE_TXT, OPT_176_HAS_ARG_LONG, 0, OPT_176_ENUM_NAME},
    #endif
#endif
#ifdef OPT_177_ENUM_NAME
    #ifdef OPT_177_CLI_LONG_TXT_A
          {OPT_177_CLI_LONG_TXT_A, OPT_177_HAS_ARG_LONG, 0, OPT_177_ENUM_NAME},
    #endif
    #ifdef OPT_177_CLI_LONG_TXT_B
          {OPT_177_CLI_LONG_TXT_B, OPT_177_HAS_ARG_LONG, 0, OPT_177_ENUM_NAME},
    #endif
    #ifdef OPT_177_CLI_LONG_TXT_C
          {OPT_177_CLI_LONG_TXT_C, OPT_177_HAS_ARG_LONG, 0, OPT_177_ENUM_NAME},
    #endif
    #ifdef OPT_177_CLI_LONG_TXT_D
          {OPT_177_CLI_LONG_TXT_D, OPT_177_HAS_ARG_LONG, 0, OPT_177_ENUM_NAME},
    #endif
    #ifdef OPT_177_CLI_SHORT_CODE_TXT
          {OPT_177_CLI_SHORT_CODE_TXT, OPT_177_HAS_ARG_LONG, 0, OPT_177_ENUM_NAME},
    #endif
#endif
#ifdef OPT_178_ENUM_NAME
    #ifdef OPT_178_CLI_LONG_TXT_A
          {OPT_178_CLI_LONG_TXT_A, OPT_178_HAS_ARG_LONG, 0, OPT_178_ENUM_NAME},
    #endif
    #ifdef OPT_178_CLI_LONG_TXT_B
          {OPT_178_CLI_LONG_TXT_B, OPT_178_HAS_ARG_LONG, 0, OPT_178_ENUM_NAME},
    #endif
    #ifdef OPT_178_CLI_LONG_TXT_C
          {OPT_178_CLI_LONG_TXT_C, OPT_178_HAS_ARG_LONG, 0, OPT_178_ENUM_NAME},
    #endif
    #ifdef OPT_178_CLI_LONG_TXT_D
          {OPT_178_CLI_LONG_TXT_D, OPT_178_HAS_ARG_LONG, 0, OPT_178_ENUM_NAME},
    #endif
    #ifdef OPT_178_CLI_SHORT_CODE_TXT
          {OPT_178_CLI_SHORT_CODE_TXT, OPT_178_HAS_ARG_LONG, 0, OPT_178_ENUM_NAME},
    #endif
#endif
#ifdef OPT_179_ENUM_NAME
    #ifdef OPT_179_CLI_LONG_TXT_A
          {OPT_179_CLI_LONG_TXT_A, OPT_179_HAS_ARG_LONG, 0, OPT_179_ENUM_NAME},
    #endif
    #ifdef OPT_179_CLI_LONG_TXT_B
          {OPT_179_CLI_LONG_TXT_B, OPT_179_HAS_ARG_LONG, 0, OPT_179_ENUM_NAME},
    #endif
    #ifdef OPT_179_CLI_LONG_TXT_C
          {OPT_179_CLI_LONG_TXT_C, OPT_179_HAS_ARG_LONG, 0, OPT_179_ENUM_NAME},
    #endif
    #ifdef OPT_179_CLI_LONG_TXT_D
          {OPT_179_CLI_LONG_TXT_D, OPT_179_HAS_ARG_LONG, 0, OPT_179_ENUM_NAME},
    #endif
    #ifdef OPT_179_CLI_SHORT_CODE_TXT
          {OPT_179_CLI_SHORT_CODE_TXT, OPT_179_HAS_ARG_LONG, 0, OPT_179_ENUM_NAME},
    #endif
#endif

#ifdef OPT_180_ENUM_NAME
    #ifdef OPT_180_CLI_LONG_TXT_A
          {OPT_180_CLI_LONG_TXT_A, OPT_180_HAS_ARG_LONG, 0, OPT_180_ENUM_NAME},
    #endif
    #ifdef OPT_180_CLI_LONG_TXT_B
          {OPT_180_CLI_LONG_TXT_B, OPT_180_HAS_ARG_LONG, 0, OPT_180_ENUM_NAME},
    #endif
    #ifdef OPT_180_CLI_LONG_TXT_C
          {OPT_180_CLI_LONG_TXT_C, OPT_180_HAS_ARG_LONG, 0, OPT_180_ENUM_NAME},
    #endif
    #ifdef OPT_180_CLI_LONG_TXT_D
          {OPT_180_CLI_LONG_TXT_D, OPT_180_HAS_ARG_LONG, 0, OPT_180_ENUM_NAME},
    #endif
    #ifdef OPT_180_CLI_SHORT_CODE_TXT
          {OPT_180_CLI_SHORT_CODE_TXT, OPT_180_HAS_ARG_LONG, 0, OPT_180_ENUM_NAME},
    #endif
#endif
#ifdef OPT_181_ENUM_NAME
    #ifdef OPT_181_CLI_LONG_TXT_A
          {OPT_181_CLI_LONG_TXT_A, OPT_181_HAS_ARG_LONG, 0, OPT_181_ENUM_NAME},
    #endif
    #ifdef OPT_181_CLI_LONG_TXT_B
          {OPT_181_CLI_LONG_TXT_B, OPT_181_HAS_ARG_LONG, 0, OPT_181_ENUM_NAME},
    #endif
    #ifdef OPT_181_CLI_LONG_TXT_C
          {OPT_181_CLI_LONG_TXT_C, OPT_181_HAS_ARG_LONG, 0, OPT_181_ENUM_NAME},
    #endif
    #ifdef OPT_181_CLI_LONG_TXT_D
          {OPT_181_CLI_LONG_TXT_D, OPT_181_HAS_ARG_LONG, 0, OPT_181_ENUM_NAME},
    #endif
    #ifdef OPT_181_CLI_SHORT_CODE_TXT
          {OPT_181_CLI_SHORT_CODE_TXT, OPT_181_HAS_ARG_LONG, 0, OPT_181_ENUM_NAME},
    #endif
#endif
#ifdef OPT_182_ENUM_NAME
    #ifdef OPT_182_CLI_LONG_TXT_A
          {OPT_182_CLI_LONG_TXT_A, OPT_182_HAS_ARG_LONG, 0, OPT_182_ENUM_NAME},
    #endif
    #ifdef OPT_182_CLI_LONG_TXT_B
          {OPT_182_CLI_LONG_TXT_B, OPT_182_HAS_ARG_LONG, 0, OPT_182_ENUM_NAME},
    #endif
    #ifdef OPT_182_CLI_LONG_TXT_C
          {OPT_182_CLI_LONG_TXT_C, OPT_182_HAS_ARG_LONG, 0, OPT_182_ENUM_NAME},
    #endif
    #ifdef OPT_182_CLI_LONG_TXT_D
          {OPT_182_CLI_LONG_TXT_D, OPT_182_HAS_ARG_LONG, 0, OPT_182_ENUM_NAME},
    #endif
    #ifdef OPT_182_CLI_SHORT_CODE_TXT
          {OPT_182_CLI_SHORT_CODE_TXT, OPT_182_HAS_ARG_LONG, 0, OPT_182_ENUM_NAME},
    #endif
#endif
#ifdef OPT_183_ENUM_NAME
    #ifdef OPT_183_CLI_LONG_TXT_A
          {OPT_183_CLI_LONG_TXT_A, OPT_183_HAS_ARG_LONG, 0, OPT_183_ENUM_NAME},
    #endif
    #ifdef OPT_183_CLI_LONG_TXT_B
          {OPT_183_CLI_LONG_TXT_B, OPT_183_HAS_ARG_LONG, 0, OPT_183_ENUM_NAME},
    #endif
    #ifdef OPT_183_CLI_LONG_TXT_C
          {OPT_183_CLI_LONG_TXT_C, OPT_183_HAS_ARG_LONG, 0, OPT_183_ENUM_NAME},
    #endif
    #ifdef OPT_183_CLI_LONG_TXT_D
          {OPT_183_CLI_LONG_TXT_D, OPT_183_HAS_ARG_LONG, 0, OPT_183_ENUM_NAME},
    #endif
    #ifdef OPT_183_CLI_SHORT_CODE_TXT
          {OPT_183_CLI_SHORT_CODE_TXT, OPT_183_HAS_ARG_LONG, 0, OPT_183_ENUM_NAME},
    #endif
#endif
#ifdef OPT_184_ENUM_NAME
    #ifdef OPT_184_CLI_LONG_TXT_A
          {OPT_184_CLI_LONG_TXT_A, OPT_184_HAS_ARG_LONG, 0, OPT_184_ENUM_NAME},
    #endif
    #ifdef OPT_184_CLI_LONG_TXT_B
          {OPT_184_CLI_LONG_TXT_B, OPT_184_HAS_ARG_LONG, 0, OPT_184_ENUM_NAME},
    #endif
    #ifdef OPT_184_CLI_LONG_TXT_C
          {OPT_184_CLI_LONG_TXT_C, OPT_184_HAS_ARG_LONG, 0, OPT_184_ENUM_NAME},
    #endif
    #ifdef OPT_184_CLI_LONG_TXT_D
          {OPT_184_CLI_LONG_TXT_D, OPT_184_HAS_ARG_LONG, 0, OPT_184_ENUM_NAME},
    #endif
    #ifdef OPT_184_CLI_SHORT_CODE_TXT
          {OPT_184_CLI_SHORT_CODE_TXT, OPT_184_HAS_ARG_LONG, 0, OPT_184_ENUM_NAME},
    #endif
#endif
#ifdef OPT_185_ENUM_NAME
    #ifdef OPT_185_CLI_LONG_TXT_A
          {OPT_185_CLI_LONG_TXT_A, OPT_185_HAS_ARG_LONG, 0, OPT_185_ENUM_NAME},
    #endif
    #ifdef OPT_185_CLI_LONG_TXT_B
          {OPT_185_CLI_LONG_TXT_B, OPT_185_HAS_ARG_LONG, 0, OPT_185_ENUM_NAME},
    #endif
    #ifdef OPT_185_CLI_LONG_TXT_C
          {OPT_185_CLI_LONG_TXT_C, OPT_185_HAS_ARG_LONG, 0, OPT_185_ENUM_NAME},
    #endif
    #ifdef OPT_185_CLI_LONG_TXT_D
          {OPT_185_CLI_LONG_TXT_D, OPT_185_HAS_ARG_LONG, 0, OPT_185_ENUM_NAME},
    #endif
    #ifdef OPT_185_CLI_SHORT_CODE_TXT
          {OPT_185_CLI_SHORT_CODE_TXT, OPT_185_HAS_ARG_LONG, 0, OPT_185_ENUM_NAME},
    #endif
#endif
#ifdef OPT_186_ENUM_NAME
    #ifdef OPT_186_CLI_LONG_TXT_A
          {OPT_186_CLI_LONG_TXT_A, OPT_186_HAS_ARG_LONG, 0, OPT_186_ENUM_NAME},
    #endif
    #ifdef OPT_186_CLI_LONG_TXT_B
          {OPT_186_CLI_LONG_TXT_B, OPT_186_HAS_ARG_LONG, 0, OPT_186_ENUM_NAME},
    #endif
    #ifdef OPT_186_CLI_LONG_TXT_C
          {OPT_186_CLI_LONG_TXT_C, OPT_186_HAS_ARG_LONG, 0, OPT_186_ENUM_NAME},
    #endif
    #ifdef OPT_186_CLI_LONG_TXT_D
          {OPT_186_CLI_LONG_TXT_D, OPT_186_HAS_ARG_LONG, 0, OPT_186_ENUM_NAME},
    #endif
    #ifdef OPT_186_CLI_SHORT_CODE_TXT
          {OPT_186_CLI_SHORT_CODE_TXT, OPT_186_HAS_ARG_LONG, 0, OPT_186_ENUM_NAME},
    #endif
#endif
#ifdef OPT_187_ENUM_NAME
    #ifdef OPT_187_CLI_LONG_TXT_A
          {OPT_187_CLI_LONG_TXT_A, OPT_187_HAS_ARG_LONG, 0, OPT_187_ENUM_NAME},
    #endif
    #ifdef OPT_187_CLI_LONG_TXT_B
          {OPT_187_CLI_LONG_TXT_B, OPT_187_HAS_ARG_LONG, 0, OPT_187_ENUM_NAME},
    #endif
    #ifdef OPT_187_CLI_LONG_TXT_C
          {OPT_187_CLI_LONG_TXT_C, OPT_187_HAS_ARG_LONG, 0, OPT_187_ENUM_NAME},
    #endif
    #ifdef OPT_187_CLI_LONG_TXT_D
          {OPT_187_CLI_LONG_TXT_D, OPT_187_HAS_ARG_LONG, 0, OPT_187_ENUM_NAME},
    #endif
    #ifdef OPT_187_CLI_SHORT_CODE_TXT
          {OPT_187_CLI_SHORT_CODE_TXT, OPT_187_HAS_ARG_LONG, 0, OPT_187_ENUM_NAME},
    #endif
#endif
#ifdef OPT_188_ENUM_NAME
    #ifdef OPT_188_CLI_LONG_TXT_A
          {OPT_188_CLI_LONG_TXT_A, OPT_188_HAS_ARG_LONG, 0, OPT_188_ENUM_NAME},
    #endif
    #ifdef OPT_188_CLI_LONG_TXT_B
          {OPT_188_CLI_LONG_TXT_B, OPT_188_HAS_ARG_LONG, 0, OPT_188_ENUM_NAME},
    #endif
    #ifdef OPT_188_CLI_LONG_TXT_C
          {OPT_188_CLI_LONG_TXT_C, OPT_188_HAS_ARG_LONG, 0, OPT_188_ENUM_NAME},
    #endif
    #ifdef OPT_188_CLI_LONG_TXT_D
          {OPT_188_CLI_LONG_TXT_D, OPT_188_HAS_ARG_LONG, 0, OPT_188_ENUM_NAME},
    #endif
    #ifdef OPT_188_CLI_SHORT_CODE_TXT
          {OPT_188_CLI_SHORT_CODE_TXT, OPT_188_HAS_ARG_LONG, 0, OPT_188_ENUM_NAME},
    #endif
#endif
#ifdef OPT_189_ENUM_NAME
    #ifdef OPT_189_CLI_LONG_TXT_A
          {OPT_189_CLI_LONG_TXT_A, OPT_189_HAS_ARG_LONG, 0, OPT_189_ENUM_NAME},
    #endif
    #ifdef OPT_189_CLI_LONG_TXT_B
          {OPT_189_CLI_LONG_TXT_B, OPT_189_HAS_ARG_LONG, 0, OPT_189_ENUM_NAME},
    #endif
    #ifdef OPT_189_CLI_LONG_TXT_C
          {OPT_189_CLI_LONG_TXT_C, OPT_189_HAS_ARG_LONG, 0, OPT_189_ENUM_NAME},
    #endif
    #ifdef OPT_189_CLI_LONG_TXT_D
          {OPT_189_CLI_LONG_TXT_D, OPT_189_HAS_ARG_LONG, 0, OPT_189_ENUM_NAME},
    #endif
    #ifdef OPT_189_CLI_SHORT_CODE_TXT
          {OPT_189_CLI_SHORT_CODE_TXT, OPT_189_HAS_ARG_LONG, 0, OPT_189_ENUM_NAME},
    #endif
#endif

#ifdef OPT_190_ENUM_NAME
    #ifdef OPT_190_CLI_LONG_TXT_A
          {OPT_190_CLI_LONG_TXT_A, OPT_190_HAS_ARG_LONG, 0, OPT_190_ENUM_NAME},
    #endif
    #ifdef OPT_190_CLI_LONG_TXT_B
          {OPT_190_CLI_LONG_TXT_B, OPT_190_HAS_ARG_LONG, 0, OPT_190_ENUM_NAME},
    #endif
    #ifdef OPT_190_CLI_LONG_TXT_C
          {OPT_190_CLI_LONG_TXT_C, OPT_190_HAS_ARG_LONG, 0, OPT_190_ENUM_NAME},
    #endif
    #ifdef OPT_190_CLI_LONG_TXT_D
          {OPT_190_CLI_LONG_TXT_D, OPT_190_HAS_ARG_LONG, 0, OPT_190_ENUM_NAME},
    #endif
    #ifdef OPT_190_CLI_SHORT_CODE_TXT
          {OPT_190_CLI_SHORT_CODE_TXT, OPT_190_HAS_ARG_LONG, 0, OPT_190_ENUM_NAME},
    #endif
#endif
#ifdef OPT_191_ENUM_NAME
    #ifdef OPT_191_CLI_LONG_TXT_A
          {OPT_191_CLI_LONG_TXT_A, OPT_191_HAS_ARG_LONG, 0, OPT_191_ENUM_NAME},
    #endif
    #ifdef OPT_191_CLI_LONG_TXT_B
          {OPT_191_CLI_LONG_TXT_B, OPT_191_HAS_ARG_LONG, 0, OPT_191_ENUM_NAME},
    #endif
    #ifdef OPT_191_CLI_LONG_TXT_C
          {OPT_191_CLI_LONG_TXT_C, OPT_191_HAS_ARG_LONG, 0, OPT_191_ENUM_NAME},
    #endif
    #ifdef OPT_191_CLI_LONG_TXT_D
          {OPT_191_CLI_LONG_TXT_D, OPT_191_HAS_ARG_LONG, 0, OPT_191_ENUM_NAME},
    #endif
    #ifdef OPT_191_CLI_SHORT_CODE_TXT
          {OPT_191_CLI_SHORT_CODE_TXT, OPT_191_HAS_ARG_LONG, 0, OPT_191_ENUM_NAME},
    #endif
#endif
#ifdef OPT_192_ENUM_NAME
    #ifdef OPT_192_CLI_LONG_TXT_A
          {OPT_192_CLI_LONG_TXT_A, OPT_192_HAS_ARG_LONG, 0, OPT_192_ENUM_NAME},
    #endif
    #ifdef OPT_192_CLI_LONG_TXT_B
          {OPT_192_CLI_LONG_TXT_B, OPT_192_HAS_ARG_LONG, 0, OPT_192_ENUM_NAME},
    #endif
    #ifdef OPT_192_CLI_LONG_TXT_C
          {OPT_192_CLI_LONG_TXT_C, OPT_192_HAS_ARG_LONG, 0, OPT_192_ENUM_NAME},
    #endif
    #ifdef OPT_192_CLI_LONG_TXT_D
          {OPT_192_CLI_LONG_TXT_D, OPT_192_HAS_ARG_LONG, 0, OPT_192_ENUM_NAME},
    #endif
    #ifdef OPT_192_CLI_SHORT_CODE_TXT
          {OPT_192_CLI_SHORT_CODE_TXT, OPT_192_HAS_ARG_LONG, 0, OPT_192_ENUM_NAME},
    #endif
#endif
#ifdef OPT_193_ENUM_NAME
    #ifdef OPT_193_CLI_LONG_TXT_A
          {OPT_193_CLI_LONG_TXT_A, OPT_193_HAS_ARG_LONG, 0, OPT_193_ENUM_NAME},
    #endif
    #ifdef OPT_193_CLI_LONG_TXT_B
          {OPT_193_CLI_LONG_TXT_B, OPT_193_HAS_ARG_LONG, 0, OPT_193_ENUM_NAME},
    #endif
    #ifdef OPT_193_CLI_LONG_TXT_C
          {OPT_193_CLI_LONG_TXT_C, OPT_193_HAS_ARG_LONG, 0, OPT_193_ENUM_NAME},
    #endif
    #ifdef OPT_193_CLI_LONG_TXT_D
          {OPT_193_CLI_LONG_TXT_D, OPT_193_HAS_ARG_LONG, 0, OPT_193_ENUM_NAME},
    #endif
    #ifdef OPT_193_CLI_SHORT_CODE_TXT
          {OPT_193_CLI_SHORT_CODE_TXT, OPT_193_HAS_ARG_LONG, 0, OPT_193_ENUM_NAME},
    #endif
#endif
#ifdef OPT_194_ENUM_NAME
    #ifdef OPT_194_CLI_LONG_TXT_A
          {OPT_194_CLI_LONG_TXT_A, OPT_194_HAS_ARG_LONG, 0, OPT_194_ENUM_NAME},
    #endif
    #ifdef OPT_194_CLI_LONG_TXT_B
          {OPT_194_CLI_LONG_TXT_B, OPT_194_HAS_ARG_LONG, 0, OPT_194_ENUM_NAME},
    #endif
    #ifdef OPT_194_CLI_LONG_TXT_C
          {OPT_194_CLI_LONG_TXT_C, OPT_194_HAS_ARG_LONG, 0, OPT_194_ENUM_NAME},
    #endif
    #ifdef OPT_194_CLI_LONG_TXT_D
          {OPT_194_CLI_LONG_TXT_D, OPT_194_HAS_ARG_LONG, 0, OPT_194_ENUM_NAME},
    #endif
    #ifdef OPT_194_CLI_SHORT_CODE_TXT
          {OPT_194_CLI_SHORT_CODE_TXT, OPT_194_HAS_ARG_LONG, 0, OPT_194_ENUM_NAME},
    #endif
#endif
#ifdef OPT_195_ENUM_NAME
    #ifdef OPT_195_CLI_LONG_TXT_A
          {OPT_195_CLI_LONG_TXT_A, OPT_195_HAS_ARG_LONG, 0, OPT_195_ENUM_NAME},
    #endif
    #ifdef OPT_195_CLI_LONG_TXT_B
          {OPT_195_CLI_LONG_TXT_B, OPT_195_HAS_ARG_LONG, 0, OPT_195_ENUM_NAME},
    #endif
    #ifdef OPT_195_CLI_LONG_TXT_C
          {OPT_195_CLI_LONG_TXT_C, OPT_195_HAS_ARG_LONG, 0, OPT_195_ENUM_NAME},
    #endif
    #ifdef OPT_195_CLI_LONG_TXT_D
          {OPT_195_CLI_LONG_TXT_D, OPT_195_HAS_ARG_LONG, 0, OPT_195_ENUM_NAME},
    #endif
    #ifdef OPT_195_CLI_SHORT_CODE_TXT
          {OPT_195_CLI_SHORT_CODE_TXT, OPT_195_HAS_ARG_LONG, 0, OPT_195_ENUM_NAME},
    #endif
#endif
#ifdef OPT_196_ENUM_NAME
    #ifdef OPT_196_CLI_LONG_TXT_A
          {OPT_196_CLI_LONG_TXT_A, OPT_196_HAS_ARG_LONG, 0, OPT_196_ENUM_NAME},
    #endif
    #ifdef OPT_196_CLI_LONG_TXT_B
          {OPT_196_CLI_LONG_TXT_B, OPT_196_HAS_ARG_LONG, 0, OPT_196_ENUM_NAME},
    #endif
    #ifdef OPT_196_CLI_LONG_TXT_C
          {OPT_196_CLI_LONG_TXT_C, OPT_196_HAS_ARG_LONG, 0, OPT_196_ENUM_NAME},
    #endif
    #ifdef OPT_196_CLI_LONG_TXT_D
          {OPT_196_CLI_LONG_TXT_D, OPT_196_HAS_ARG_LONG, 0, OPT_196_ENUM_NAME},
    #endif
    #ifdef OPT_196_CLI_SHORT_CODE_TXT
          {OPT_196_CLI_SHORT_CODE_TXT, OPT_196_HAS_ARG_LONG, 0, OPT_196_ENUM_NAME},
    #endif
#endif
#ifdef OPT_197_ENUM_NAME
    #ifdef OPT_197_CLI_LONG_TXT_A
          {OPT_197_CLI_LONG_TXT_A, OPT_197_HAS_ARG_LONG, 0, OPT_197_ENUM_NAME},
    #endif
    #ifdef OPT_197_CLI_LONG_TXT_B
          {OPT_197_CLI_LONG_TXT_B, OPT_197_HAS_ARG_LONG, 0, OPT_197_ENUM_NAME},
    #endif
    #ifdef OPT_197_CLI_LONG_TXT_C
          {OPT_197_CLI_LONG_TXT_C, OPT_197_HAS_ARG_LONG, 0, OPT_197_ENUM_NAME},
    #endif
    #ifdef OPT_197_CLI_LONG_TXT_D
          {OPT_197_CLI_LONG_TXT_D, OPT_197_HAS_ARG_LONG, 0, OPT_197_ENUM_NAME},
    #endif
    #ifdef OPT_197_CLI_SHORT_CODE_TXT
          {OPT_197_CLI_SHORT_CODE_TXT, OPT_197_HAS_ARG_LONG, 0, OPT_197_ENUM_NAME},
    #endif
#endif
#ifdef OPT_198_ENUM_NAME
    #ifdef OPT_198_CLI_LONG_TXT_A
          {OPT_198_CLI_LONG_TXT_A, OPT_198_HAS_ARG_LONG, 0, OPT_198_ENUM_NAME},
    #endif
    #ifdef OPT_198_CLI_LONG_TXT_B
          {OPT_198_CLI_LONG_TXT_B, OPT_198_HAS_ARG_LONG, 0, OPT_198_ENUM_NAME},
    #endif
    #ifdef OPT_198_CLI_LONG_TXT_C
          {OPT_198_CLI_LONG_TXT_C, OPT_198_HAS_ARG_LONG, 0, OPT_198_ENUM_NAME},
    #endif
    #ifdef OPT_198_CLI_LONG_TXT_D
          {OPT_198_CLI_LONG_TXT_D, OPT_198_HAS_ARG_LONG, 0, OPT_198_ENUM_NAME},
    #endif
    #ifdef OPT_198_CLI_SHORT_CODE_TXT
          {OPT_198_CLI_SHORT_CODE_TXT, OPT_198_HAS_ARG_LONG, 0, OPT_198_ENUM_NAME},
    #endif
#endif
#ifdef OPT_199_ENUM_NAME
    #ifdef OPT_199_CLI_LONG_TXT_A
          {OPT_199_CLI_LONG_TXT_A, OPT_199_HAS_ARG_LONG, 0, OPT_199_ENUM_NAME},
    #endif
    #ifdef OPT_199_CLI_LONG_TXT_B
          {OPT_199_CLI_LONG_TXT_B, OPT_199_HAS_ARG_LONG, 0, OPT_199_ENUM_NAME},
    #endif
    #ifdef OPT_199_CLI_LONG_TXT_C
          {OPT_199_CLI_LONG_TXT_C, OPT_199_HAS_ARG_LONG, 0, OPT_199_ENUM_NAME},
    #endif
    #ifdef OPT_199_CLI_LONG_TXT_D
          {OPT_199_CLI_LONG_TXT_D, OPT_199_HAS_ARG_LONG, 0, OPT_199_ENUM_NAME},
    #endif
    #ifdef OPT_199_CLI_SHORT_CODE_TXT
          {OPT_199_CLI_SHORT_CODE_TXT, OPT_199_HAS_ARG_LONG, 0, OPT_199_ENUM_NAME},
    #endif
#endif

    {0, 0, 0, 0}  /* required per 'getopt()' fnc family specifications */
};


typedef struct opt_props_tt {
    const char*  p_txt_help;
    uint32_t     incompat_grps;  /* bitset */
} opt_props_t;

#define TXT_HELP_INVALID_ITEM  "__INVALID_ITEM__"

/* ==== PRIVATE FUNCTIONS ================================================== */

static opt_props_t opt_get_props(cli_opt_t opt)
{
    opt_props_t props = {TXT_HELP_INVALID_ITEM, OPT_GRP_NONE};
    
    /* cannot be done done via array indexing, because cli_opt_t IS NOT A CONSECUTIVE LIST */
    switch (opt)
    {
        case OPT_00_NO_OPTION:
            props.p_txt_help    = "";
            props.incompat_grps = OPT_GRP_NONE;
        break;
        
      #ifdef OPT_01_ENUM_NAME
        case OPT_01_ENUM_NAME:
            props.p_txt_help    = OPT_01_TXT_HELP;
            props.incompat_grps = OPT_01_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_02_ENUM_NAME
        case OPT_02_ENUM_NAME:
            props.p_txt_help    = OPT_02_TXT_HELP;
            props.incompat_grps = OPT_02_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_03_ENUM_NAME
        case OPT_03_ENUM_NAME:
            props.p_txt_help    = OPT_03_TXT_HELP;
            props.incompat_grps = OPT_03_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_04_ENUM_NAME
        case OPT_04_ENUM_NAME:
            props.p_txt_help    = OPT_04_TXT_HELP;
            props.incompat_grps = OPT_04_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_05_ENUM_NAME
        case OPT_05_ENUM_NAME:
            props.p_txt_help    = OPT_05_TXT_HELP;
            props.incompat_grps = OPT_05_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_06_ENUM_NAME
        case OPT_06_ENUM_NAME:
            props.p_txt_help    = OPT_06_TXT_HELP;
            props.incompat_grps = OPT_06_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_07_ENUM_NAME
        case OPT_07_ENUM_NAME:
            props.p_txt_help    = OPT_07_TXT_HELP;
            props.incompat_grps = OPT_07_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_08_ENUM_NAME
        case OPT_08_ENUM_NAME:
            props.p_txt_help    = OPT_08_TXT_HELP;
            props.incompat_grps = OPT_08_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_09_ENUM_NAME
        case OPT_09_ENUM_NAME:
            props.p_txt_help    = OPT_09_TXT_HELP;
            props.incompat_grps = OPT_09_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_10_ENUM_NAME
        case OPT_10_ENUM_NAME:
            props.p_txt_help    = OPT_10_TXT_HELP;
            props.incompat_grps = OPT_10_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_11_ENUM_NAME
        case OPT_11_ENUM_NAME:
            props.p_txt_help    = OPT_11_TXT_HELP;
            props.incompat_grps = OPT_11_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_12_ENUM_NAME
        case OPT_12_ENUM_NAME:
            props.p_txt_help    = OPT_12_TXT_HELP;
            props.incompat_grps = OPT_12_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_13_ENUM_NAME
        case OPT_13_ENUM_NAME:
            props.p_txt_help    = OPT_13_TXT_HELP;
            props.incompat_grps = OPT_13_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_14_ENUM_NAME
        case OPT_14_ENUM_NAME:
            props.p_txt_help    = OPT_14_TXT_HELP;
            props.incompat_grps = OPT_14_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_15_ENUM_NAME
        case OPT_15_ENUM_NAME:
            props.p_txt_help    = OPT_15_TXT_HELP;
            props.incompat_grps = OPT_15_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_16_ENUM_NAME
        case OPT_16_ENUM_NAME:
            props.p_txt_help    = OPT_16_TXT_HELP;
            props.incompat_grps = OPT_16_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_17_ENUM_NAME
        case OPT_17_ENUM_NAME:
            props.p_txt_help    = OPT_17_TXT_HELP;
            props.incompat_grps = OPT_17_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_18_ENUM_NAME
        case OPT_18_ENUM_NAME:
            props.p_txt_help    = OPT_18_TXT_HELP;
            props.incompat_grps = OPT_18_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_19_ENUM_NAME
        case OPT_19_ENUM_NAME:
            props.p_txt_help    = OPT_19_TXT_HELP;
            props.incompat_grps = OPT_19_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_20_ENUM_NAME
        case OPT_20_ENUM_NAME:
            props.p_txt_help    = OPT_20_TXT_HELP;
            props.incompat_grps = OPT_20_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_21_ENUM_NAME
        case OPT_21_ENUM_NAME:
            props.p_txt_help    = OPT_21_TXT_HELP;
            props.incompat_grps = OPT_21_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_22_ENUM_NAME
        case OPT_22_ENUM_NAME:
            props.p_txt_help    = OPT_22_TXT_HELP;
            props.incompat_grps = OPT_22_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_23_ENUM_NAME
        case OPT_23_ENUM_NAME:
            props.p_txt_help    = OPT_23_TXT_HELP;
            props.incompat_grps = OPT_23_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_24_ENUM_NAME
        case OPT_24_ENUM_NAME:
            props.p_txt_help    = OPT_24_TXT_HELP;
            props.incompat_grps = OPT_24_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_25_ENUM_NAME
        case OPT_25_ENUM_NAME:
            props.p_txt_help    = OPT_25_TXT_HELP;
            props.incompat_grps = OPT_25_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_26_ENUM_NAME
        case OPT_26_ENUM_NAME:
            props.p_txt_help    = OPT_26_TXT_HELP;
            props.incompat_grps = OPT_26_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_27_ENUM_NAME
        case OPT_27_ENUM_NAME:
            props.p_txt_help    = OPT_27_TXT_HELP;
            props.incompat_grps = OPT_27_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_28_ENUM_NAME
        case OPT_28_ENUM_NAME:
            props.p_txt_help    = OPT_28_TXT_HELP;
            props.incompat_grps = OPT_28_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_29_ENUM_NAME
        case OPT_29_ENUM_NAME:
            props.p_txt_help    = OPT_29_TXT_HELP;
            props.incompat_grps = OPT_29_INCOMPAT_GRPS;
        break;
      #endif
      
      #ifdef OPT_30_ENUM_NAME
        case OPT_30_ENUM_NAME:
            props.p_txt_help    = OPT_30_TXT_HELP;
            props.incompat_grps = OPT_30_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_31_ENUM_NAME
        case OPT_31_ENUM_NAME:
            props.p_txt_help    = OPT_31_TXT_HELP;
            props.incompat_grps = OPT_31_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_32_ENUM_NAME
        case OPT_32_ENUM_NAME:
            props.p_txt_help    = OPT_32_TXT_HELP;
            props.incompat_grps = OPT_32_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_33_ENUM_NAME
        case OPT_33_ENUM_NAME:
            props.p_txt_help    = OPT_33_TXT_HELP;
            props.incompat_grps = OPT_33_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_34_ENUM_NAME
        case OPT_34_ENUM_NAME:
            props.p_txt_help    = OPT_34_TXT_HELP;
            props.incompat_grps = OPT_34_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_35_ENUM_NAME
        case OPT_35_ENUM_NAME:
            props.p_txt_help    = OPT_35_TXT_HELP;
            props.incompat_grps = OPT_35_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_36_ENUM_NAME
        case OPT_36_ENUM_NAME:
            props.p_txt_help    = OPT_36_TXT_HELP;
            props.incompat_grps = OPT_36_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_37_ENUM_NAME
        case OPT_37_ENUM_NAME:
            props.p_txt_help    = OPT_37_TXT_HELP;
            props.incompat_grps = OPT_37_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_38_ENUM_NAME
        case OPT_38_ENUM_NAME:
            props.p_txt_help    = OPT_38_TXT_HELP;
            props.incompat_grps = OPT_38_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_39_ENUM_NAME
        case OPT_39_ENUM_NAME:
            props.p_txt_help    = OPT_39_TXT_HELP;
            props.incompat_grps = OPT_39_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_40_ENUM_NAME
        case OPT_40_ENUM_NAME:
            props.p_txt_help    = OPT_40_TXT_HELP;
            props.incompat_grps = OPT_40_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_41_ENUM_NAME
        case OPT_41_ENUM_NAME:
            props.p_txt_help    = OPT_41_TXT_HELP;
            props.incompat_grps = OPT_41_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_42_ENUM_NAME
        case OPT_42_ENUM_NAME:
            props.p_txt_help    = OPT_42_TXT_HELP;
            props.incompat_grps = OPT_42_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_43_ENUM_NAME
        case OPT_43_ENUM_NAME:
            props.p_txt_help    = OPT_43_TXT_HELP;
            props.incompat_grps = OPT_43_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_44_ENUM_NAME
        case OPT_44_ENUM_NAME:
            props.p_txt_help    = OPT_44_TXT_HELP;
            props.incompat_grps = OPT_44_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_45_ENUM_NAME
        case OPT_45_ENUM_NAME:
            props.p_txt_help    = OPT_45_TXT_HELP;
            props.incompat_grps = OPT_45_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_46_ENUM_NAME
        case OPT_46_ENUM_NAME:
            props.p_txt_help    = OPT_46_TXT_HELP;
            props.incompat_grps = OPT_46_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_47_ENUM_NAME
        case OPT_47_ENUM_NAME:
            props.p_txt_help    = OPT_47_TXT_HELP;
            props.incompat_grps = OPT_47_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_48_ENUM_NAME
        case OPT_48_ENUM_NAME:
            props.p_txt_help    = OPT_48_TXT_HELP;
            props.incompat_grps = OPT_48_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_49_ENUM_NAME
        case OPT_49_ENUM_NAME:
            props.p_txt_help    = OPT_49_TXT_HELP;
            props.incompat_grps = OPT_49_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_50_ENUM_NAME
        case OPT_50_ENUM_NAME:
            props.p_txt_help    = OPT_50_TXT_HELP;
            props.incompat_grps = OPT_50_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_51_ENUM_NAME
        case OPT_51_ENUM_NAME:
            props.p_txt_help    = OPT_51_TXT_HELP;
            props.incompat_grps = OPT_51_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_52_ENUM_NAME
        case OPT_52_ENUM_NAME:
            props.p_txt_help    = OPT_52_TXT_HELP;
            props.incompat_grps = OPT_52_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_53_ENUM_NAME
        case OPT_53_ENUM_NAME:
            props.p_txt_help    = OPT_53_TXT_HELP;
            props.incompat_grps = OPT_53_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_54_ENUM_NAME
        case OPT_54_ENUM_NAME:
            props.p_txt_help    = OPT_54_TXT_HELP;
            props.incompat_grps = OPT_54_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_55_ENUM_NAME
        case OPT_55_ENUM_NAME:
            props.p_txt_help    = OPT_55_TXT_HELP;
            props.incompat_grps = OPT_55_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_56_ENUM_NAME
        case OPT_56_ENUM_NAME:
            props.p_txt_help    = OPT_56_TXT_HELP;
            props.incompat_grps = OPT_56_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_57_ENUM_NAME
        case OPT_57_ENUM_NAME:
            props.p_txt_help    = OPT_57_TXT_HELP;
            props.incompat_grps = OPT_57_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_58_ENUM_NAME
        case OPT_58_ENUM_NAME:
            props.p_txt_help    = OPT_58_TXT_HELP;
            props.incompat_grps = OPT_58_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_59_ENUM_NAME
        case OPT_59_ENUM_NAME:
            props.p_txt_help    = OPT_59_TXT_HELP;
            props.incompat_grps = OPT_59_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_60_ENUM_NAME
        case OPT_60_ENUM_NAME:
            props.p_txt_help    = OPT_60_TXT_HELP;
            props.incompat_grps = OPT_60_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_61_ENUM_NAME
        case OPT_61_ENUM_NAME:
            props.p_txt_help    = OPT_61_TXT_HELP;
            props.incompat_grps = OPT_61_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_62_ENUM_NAME
        case OPT_62_ENUM_NAME:
            props.p_txt_help    = OPT_62_TXT_HELP;
            props.incompat_grps = OPT_62_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_63_ENUM_NAME
        case OPT_63_ENUM_NAME:
            props.p_txt_help    = OPT_63_TXT_HELP;
            props.incompat_grps = OPT_63_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_64_ENUM_NAME
        case OPT_64_ENUM_NAME:
            props.p_txt_help    = OPT_64_TXT_HELP;
            props.incompat_grps = OPT_64_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_65_ENUM_NAME
        case OPT_65_ENUM_NAME:
            props.p_txt_help    = OPT_65_TXT_HELP;
            props.incompat_grps = OPT_65_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_66_ENUM_NAME
        case OPT_66_ENUM_NAME:
            props.p_txt_help    = OPT_66_TXT_HELP;
            props.incompat_grps = OPT_66_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_67_ENUM_NAME
        case OPT_67_ENUM_NAME:
            props.p_txt_help    = OPT_67_TXT_HELP;
            props.incompat_grps = OPT_67_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_68_ENUM_NAME
        case OPT_68_ENUM_NAME:
            props.p_txt_help    = OPT_68_TXT_HELP;
            props.incompat_grps = OPT_68_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_69_ENUM_NAME
        case OPT_69_ENUM_NAME:
            props.p_txt_help    = OPT_69_TXT_HELP;
            props.incompat_grps = OPT_69_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_70_ENUM_NAME
        case OPT_70_ENUM_NAME:
            props.p_txt_help    = OPT_70_TXT_HELP;
            props.incompat_grps = OPT_70_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_71_ENUM_NAME
        case OPT_71_ENUM_NAME:
            props.p_txt_help    = OPT_71_TXT_HELP;
            props.incompat_grps = OPT_71_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_72_ENUM_NAME
        case OPT_72_ENUM_NAME:
            props.p_txt_help    = OPT_72_TXT_HELP;
            props.incompat_grps = OPT_72_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_73_ENUM_NAME
        case OPT_73_ENUM_NAME:
            props.p_txt_help    = OPT_73_TXT_HELP;
            props.incompat_grps = OPT_73_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_74_ENUM_NAME
        case OPT_74_ENUM_NAME:
            props.p_txt_help    = OPT_74_TXT_HELP;
            props.incompat_grps = OPT_74_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_75_ENUM_NAME
        case OPT_75_ENUM_NAME:
            props.p_txt_help    = OPT_75_TXT_HELP;
            props.incompat_grps = OPT_75_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_76_ENUM_NAME
        case OPT_76_ENUM_NAME:
            props.p_txt_help    = OPT_76_TXT_HELP;
            props.incompat_grps = OPT_76_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_77_ENUM_NAME
        case OPT_77_ENUM_NAME:
            props.p_txt_help    = OPT_77_TXT_HELP;
            props.incompat_grps = OPT_77_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_78_ENUM_NAME
        case OPT_78_ENUM_NAME:
            props.p_txt_help    = OPT_78_TXT_HELP;
            props.incompat_grps = OPT_78_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_79_ENUM_NAME
        case OPT_79_ENUM_NAME:
            props.p_txt_help    = OPT_79_TXT_HELP;
            props.incompat_grps = OPT_79_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_80_ENUM_NAME
        case OPT_80_ENUM_NAME:
            props.p_txt_help    = OPT_80_TXT_HELP;
            props.incompat_grps = OPT_80_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_81_ENUM_NAME
        case OPT_81_ENUM_NAME:
            props.p_txt_help    = OPT_81_TXT_HELP;
            props.incompat_grps = OPT_81_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_82_ENUM_NAME
        case OPT_82_ENUM_NAME:
            props.p_txt_help    = OPT_82_TXT_HELP;
            props.incompat_grps = OPT_82_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_83_ENUM_NAME
        case OPT_83_ENUM_NAME:
            props.p_txt_help    = OPT_83_TXT_HELP;
            props.incompat_grps = OPT_83_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_84_ENUM_NAME
        case OPT_84_ENUM_NAME:
            props.p_txt_help    = OPT_84_TXT_HELP;
            props.incompat_grps = OPT_84_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_85_ENUM_NAME
        case OPT_85_ENUM_NAME:
            props.p_txt_help    = OPT_85_TXT_HELP;
            props.incompat_grps = OPT_85_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_86_ENUM_NAME
        case OPT_86_ENUM_NAME:
            props.p_txt_help    = OPT_86_TXT_HELP;
            props.incompat_grps = OPT_86_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_87_ENUM_NAME
        case OPT_87_ENUM_NAME:
            props.p_txt_help    = OPT_87_TXT_HELP;
            props.incompat_grps = OPT_87_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_88_ENUM_NAME
        case OPT_88_ENUM_NAME:
            props.p_txt_help    = OPT_88_TXT_HELP;
            props.incompat_grps = OPT_88_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_89_ENUM_NAME
        case OPT_89_ENUM_NAME:
            props.p_txt_help    = OPT_89_TXT_HELP;
            props.incompat_grps = OPT_89_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_90_ENUM_NAME
        case OPT_90_ENUM_NAME:
            props.p_txt_help    = OPT_90_TXT_HELP;
            props.incompat_grps = OPT_90_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_91_ENUM_NAME
        case OPT_91_ENUM_NAME:
            props.p_txt_help    = OPT_91_TXT_HELP;
            props.incompat_grps = OPT_91_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_92_ENUM_NAME
        case OPT_92_ENUM_NAME:
            props.p_txt_help    = OPT_92_TXT_HELP;
            props.incompat_grps = OPT_92_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_93_ENUM_NAME
        case OPT_93_ENUM_NAME:
            props.p_txt_help    = OPT_93_TXT_HELP;
            props.incompat_grps = OPT_93_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_94_ENUM_NAME
        case OPT_94_ENUM_NAME:
            props.p_txt_help    = OPT_94_TXT_HELP;
            props.incompat_grps = OPT_94_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_95_ENUM_NAME
        case OPT_95_ENUM_NAME:
            props.p_txt_help    = OPT_95_TXT_HELP;
            props.incompat_grps = OPT_95_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_96_ENUM_NAME
        case OPT_96_ENUM_NAME:
            props.p_txt_help    = OPT_96_TXT_HELP;
            props.incompat_grps = OPT_96_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_97_ENUM_NAME
        case OPT_97_ENUM_NAME:
            props.p_txt_help    = OPT_97_TXT_HELP;
            props.incompat_grps = OPT_97_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_98_ENUM_NAME
        case OPT_98_ENUM_NAME:
            props.p_txt_help    = OPT_98_TXT_HELP;
            props.incompat_grps = OPT_98_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_99_ENUM_NAME
        case OPT_99_ENUM_NAME:
            props.p_txt_help    = OPT_99_TXT_HELP;
            props.incompat_grps = OPT_99_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_100_ENUM_NAME
        case OPT_100_ENUM_NAME:
            props.p_txt_help    = OPT_100_TXT_HELP;
            props.incompat_grps = OPT_100_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_101_ENUM_NAME
        case OPT_101_ENUM_NAME:
            props.p_txt_help    = OPT_101_TXT_HELP;
            props.incompat_grps = OPT_101_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_102_ENUM_NAME
        case OPT_102_ENUM_NAME:
            props.p_txt_help    = OPT_102_TXT_HELP;
            props.incompat_grps = OPT_102_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_103_ENUM_NAME
        case OPT_103_ENUM_NAME:
            props.p_txt_help    = OPT_103_TXT_HELP;
            props.incompat_grps = OPT_103_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_104_ENUM_NAME
        case OPT_104_ENUM_NAME:
            props.p_txt_help    = OPT_104_TXT_HELP;
            props.incompat_grps = OPT_104_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_105_ENUM_NAME
        case OPT_105_ENUM_NAME:
            props.p_txt_help    = OPT_105_TXT_HELP;
            props.incompat_grps = OPT_105_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_106_ENUM_NAME
        case OPT_106_ENUM_NAME:
            props.p_txt_help    = OPT_106_TXT_HELP;
            props.incompat_grps = OPT_106_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_107_ENUM_NAME
        case OPT_107_ENUM_NAME:
            props.p_txt_help    = OPT_107_TXT_HELP;
            props.incompat_grps = OPT_107_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_108_ENUM_NAME
        case OPT_108_ENUM_NAME:
            props.p_txt_help    = OPT_108_TXT_HELP;
            props.incompat_grps = OPT_108_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_109_ENUM_NAME
        case OPT_109_ENUM_NAME:
            props.p_txt_help    = OPT_109_TXT_HELP;
            props.incompat_grps = OPT_109_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_110_ENUM_NAME
        case OPT_110_ENUM_NAME:
            props.p_txt_help    = OPT_110_TXT_HELP;
            props.incompat_grps = OPT_110_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_111_ENUM_NAME
        case OPT_111_ENUM_NAME:
            props.p_txt_help    = OPT_111_TXT_HELP;
            props.incompat_grps = OPT_111_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_112_ENUM_NAME
        case OPT_112_ENUM_NAME:
            props.p_txt_help    = OPT_112_TXT_HELP;
            props.incompat_grps = OPT_112_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_113_ENUM_NAME
        case OPT_113_ENUM_NAME:
            props.p_txt_help    = OPT_113_TXT_HELP;
            props.incompat_grps = OPT_113_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_114_ENUM_NAME
        case OPT_114_ENUM_NAME:
            props.p_txt_help    = OPT_114_TXT_HELP;
            props.incompat_grps = OPT_114_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_115_ENUM_NAME
        case OPT_115_ENUM_NAME:
            props.p_txt_help    = OPT_115_TXT_HELP;
            props.incompat_grps = OPT_115_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_116_ENUM_NAME
        case OPT_116_ENUM_NAME:
            props.p_txt_help    = OPT_116_TXT_HELP;
            props.incompat_grps = OPT_116_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_117_ENUM_NAME
        case OPT_117_ENUM_NAME:
            props.p_txt_help    = OPT_117_TXT_HELP;
            props.incompat_grps = OPT_117_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_118_ENUM_NAME
        case OPT_118_ENUM_NAME:
            props.p_txt_help    = OPT_118_TXT_HELP;
            props.incompat_grps = OPT_118_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_119_ENUM_NAME
        case OPT_119_ENUM_NAME:
            props.p_txt_help    = OPT_119_TXT_HELP;
            props.incompat_grps = OPT_119_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_120_ENUM_NAME
        case OPT_120_ENUM_NAME:
            props.p_txt_help    = OPT_120_TXT_HELP;
            props.incompat_grps = OPT_120_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_121_ENUM_NAME
        case OPT_121_ENUM_NAME:
            props.p_txt_help    = OPT_121_TXT_HELP;
            props.incompat_grps = OPT_121_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_122_ENUM_NAME
        case OPT_122_ENUM_NAME:
            props.p_txt_help    = OPT_122_TXT_HELP;
            props.incompat_grps = OPT_122_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_123_ENUM_NAME
        case OPT_123_ENUM_NAME:
            props.p_txt_help    = OPT_123_TXT_HELP;
            props.incompat_grps = OPT_123_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_124_ENUM_NAME
        case OPT_124_ENUM_NAME:
            props.p_txt_help    = OPT_124_TXT_HELP;
            props.incompat_grps = OPT_124_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_125_ENUM_NAME
        case OPT_125_ENUM_NAME:
            props.p_txt_help    = OPT_125_TXT_HELP;
            props.incompat_grps = OPT_125_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_126_ENUM_NAME
        case OPT_126_ENUM_NAME:
            props.p_txt_help    = OPT_126_TXT_HELP;
            props.incompat_grps = OPT_126_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_127_ENUM_NAME
        case OPT_127_ENUM_NAME:
            props.p_txt_help    = OPT_127_TXT_HELP;
            props.incompat_grps = OPT_127_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_128_ENUM_NAME
        case OPT_128_ENUM_NAME:
            props.p_txt_help    = OPT_128_TXT_HELP;
            props.incompat_grps = OPT_128_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_129_ENUM_NAME
        case OPT_129_ENUM_NAME:
            props.p_txt_help    = OPT_129_TXT_HELP;
            props.incompat_grps = OPT_129_INCOMPAT_GRPS;
        break;
      #endif
      
      #ifdef OPT_130_ENUM_NAME
        case OPT_130_ENUM_NAME:
            props.p_txt_help    = OPT_130_TXT_HELP;
            props.incompat_grps = OPT_130_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_131_ENUM_NAME
        case OPT_131_ENUM_NAME:
            props.p_txt_help    = OPT_131_TXT_HELP;
            props.incompat_grps = OPT_131_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_132_ENUM_NAME
        case OPT_132_ENUM_NAME:
            props.p_txt_help    = OPT_132_TXT_HELP;
            props.incompat_grps = OPT_132_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_133_ENUM_NAME
        case OPT_133_ENUM_NAME:
            props.p_txt_help    = OPT_133_TXT_HELP;
            props.incompat_grps = OPT_133_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_134_ENUM_NAME
        case OPT_134_ENUM_NAME:
            props.p_txt_help    = OPT_134_TXT_HELP;
            props.incompat_grps = OPT_134_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_135_ENUM_NAME
        case OPT_135_ENUM_NAME:
            props.p_txt_help    = OPT_135_TXT_HELP;
            props.incompat_grps = OPT_135_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_136_ENUM_NAME
        case OPT_136_ENUM_NAME:
            props.p_txt_help    = OPT_136_TXT_HELP;
            props.incompat_grps = OPT_136_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_137_ENUM_NAME
        case OPT_137_ENUM_NAME:
            props.p_txt_help    = OPT_137_TXT_HELP;
            props.incompat_grps = OPT_137_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_138_ENUM_NAME
        case OPT_138_ENUM_NAME:
            props.p_txt_help    = OPT_138_TXT_HELP;
            props.incompat_grps = OPT_138_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_139_ENUM_NAME
        case OPT_139_ENUM_NAME:
            props.p_txt_help    = OPT_139_TXT_HELP;
            props.incompat_grps = OPT_139_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_140_ENUM_NAME
        case OPT_140_ENUM_NAME:
            props.p_txt_help    = OPT_140_TXT_HELP;
            props.incompat_grps = OPT_140_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_141_ENUM_NAME
        case OPT_141_ENUM_NAME:
            props.p_txt_help    = OPT_141_TXT_HELP;
            props.incompat_grps = OPT_141_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_142_ENUM_NAME
        case OPT_142_ENUM_NAME:
            props.p_txt_help    = OPT_142_TXT_HELP;
            props.incompat_grps = OPT_142_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_143_ENUM_NAME
        case OPT_143_ENUM_NAME:
            props.p_txt_help    = OPT_143_TXT_HELP;
            props.incompat_grps = OPT_143_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_144_ENUM_NAME
        case OPT_144_ENUM_NAME:
            props.p_txt_help    = OPT_144_TXT_HELP;
            props.incompat_grps = OPT_144_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_145_ENUM_NAME
        case OPT_145_ENUM_NAME:
            props.p_txt_help    = OPT_145_TXT_HELP;
            props.incompat_grps = OPT_145_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_146_ENUM_NAME
        case OPT_146_ENUM_NAME:
            props.p_txt_help    = OPT_146_TXT_HELP;
            props.incompat_grps = OPT_146_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_147_ENUM_NAME
        case OPT_147_ENUM_NAME:
            props.p_txt_help    = OPT_147_TXT_HELP;
            props.incompat_grps = OPT_147_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_148_ENUM_NAME
        case OPT_148_ENUM_NAME:
            props.p_txt_help    = OPT_148_TXT_HELP;
            props.incompat_grps = OPT_148_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_149_ENUM_NAME
        case OPT_149_ENUM_NAME:
            props.p_txt_help    = OPT_149_TXT_HELP;
            props.incompat_grps = OPT_149_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_150_ENUM_NAME
        case OPT_150_ENUM_NAME:
            props.p_txt_help    = OPT_150_TXT_HELP;
            props.incompat_grps = OPT_150_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_151_ENUM_NAME
        case OPT_151_ENUM_NAME:
            props.p_txt_help    = OPT_151_TXT_HELP;
            props.incompat_grps = OPT_151_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_152_ENUM_NAME
        case OPT_152_ENUM_NAME:
            props.p_txt_help    = OPT_152_TXT_HELP;
            props.incompat_grps = OPT_152_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_153_ENUM_NAME
        case OPT_153_ENUM_NAME:
            props.p_txt_help    = OPT_153_TXT_HELP;
            props.incompat_grps = OPT_153_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_154_ENUM_NAME
        case OPT_154_ENUM_NAME:
            props.p_txt_help    = OPT_154_TXT_HELP;
            props.incompat_grps = OPT_154_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_155_ENUM_NAME
        case OPT_155_ENUM_NAME:
            props.p_txt_help    = OPT_155_TXT_HELP;
            props.incompat_grps = OPT_155_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_156_ENUM_NAME
        case OPT_156_ENUM_NAME:
            props.p_txt_help    = OPT_156_TXT_HELP;
            props.incompat_grps = OPT_156_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_157_ENUM_NAME
        case OPT_157_ENUM_NAME:
            props.p_txt_help    = OPT_157_TXT_HELP;
            props.incompat_grps = OPT_157_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_158_ENUM_NAME
        case OPT_158_ENUM_NAME:
            props.p_txt_help    = OPT_158_TXT_HELP;
            props.incompat_grps = OPT_158_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_159_ENUM_NAME
        case OPT_159_ENUM_NAME:
            props.p_txt_help    = OPT_159_TXT_HELP;
            props.incompat_grps = OPT_159_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_160_ENUM_NAME
        case OPT_160_ENUM_NAME:
            props.p_txt_help    = OPT_160_TXT_HELP;
            props.incompat_grps = OPT_160_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_161_ENUM_NAME
        case OPT_161_ENUM_NAME:
            props.p_txt_help    = OPT_161_TXT_HELP;
            props.incompat_grps = OPT_161_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_162_ENUM_NAME
        case OPT_162_ENUM_NAME:
            props.p_txt_help    = OPT_162_TXT_HELP;
            props.incompat_grps = OPT_162_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_163_ENUM_NAME
        case OPT_163_ENUM_NAME:
            props.p_txt_help    = OPT_163_TXT_HELP;
            props.incompat_grps = OPT_163_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_164_ENUM_NAME
        case OPT_164_ENUM_NAME:
            props.p_txt_help    = OPT_164_TXT_HELP;
            props.incompat_grps = OPT_164_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_165_ENUM_NAME
        case OPT_165_ENUM_NAME:
            props.p_txt_help    = OPT_165_TXT_HELP;
            props.incompat_grps = OPT_165_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_166_ENUM_NAME
        case OPT_166_ENUM_NAME:
            props.p_txt_help    = OPT_166_TXT_HELP;
            props.incompat_grps = OPT_166_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_167_ENUM_NAME
        case OPT_167_ENUM_NAME:
            props.p_txt_help    = OPT_167_TXT_HELP;
            props.incompat_grps = OPT_167_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_168_ENUM_NAME
        case OPT_168_ENUM_NAME:
            props.p_txt_help    = OPT_168_TXT_HELP;
            props.incompat_grps = OPT_168_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_169_ENUM_NAME
        case OPT_169_ENUM_NAME:
            props.p_txt_help    = OPT_169_TXT_HELP;
            props.incompat_grps = OPT_169_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_170_ENUM_NAME
        case OPT_170_ENUM_NAME:
            props.p_txt_help    = OPT_170_TXT_HELP;
            props.incompat_grps = OPT_170_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_171_ENUM_NAME
        case OPT_171_ENUM_NAME:
            props.p_txt_help    = OPT_171_TXT_HELP;
            props.incompat_grps = OPT_171_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_172_ENUM_NAME
        case OPT_172_ENUM_NAME:
            props.p_txt_help    = OPT_172_TXT_HELP;
            props.incompat_grps = OPT_172_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_173_ENUM_NAME
        case OPT_173_ENUM_NAME:
            props.p_txt_help    = OPT_173_TXT_HELP;
            props.incompat_grps = OPT_173_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_174_ENUM_NAME
        case OPT_174_ENUM_NAME:
            props.p_txt_help    = OPT_174_TXT_HELP;
            props.incompat_grps = OPT_174_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_175_ENUM_NAME
        case OPT_175_ENUM_NAME:
            props.p_txt_help    = OPT_175_TXT_HELP;
            props.incompat_grps = OPT_175_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_176_ENUM_NAME
        case OPT_176_ENUM_NAME:
            props.p_txt_help    = OPT_176_TXT_HELP;
            props.incompat_grps = OPT_176_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_177_ENUM_NAME
        case OPT_177_ENUM_NAME:
            props.p_txt_help    = OPT_177_TXT_HELP;
            props.incompat_grps = OPT_177_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_178_ENUM_NAME
        case OPT_178_ENUM_NAME:
            props.p_txt_help    = OPT_178_TXT_HELP;
            props.incompat_grps = OPT_178_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_179_ENUM_NAME
        case OPT_179_ENUM_NAME:
            props.p_txt_help    = OPT_179_TXT_HELP;
            props.incompat_grps = OPT_179_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_180_ENUM_NAME
        case OPT_180_ENUM_NAME:
            props.p_txt_help    = OPT_180_TXT_HELP;
            props.incompat_grps = OPT_180_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_181_ENUM_NAME
        case OPT_181_ENUM_NAME:
            props.p_txt_help    = OPT_181_TXT_HELP;
            props.incompat_grps = OPT_181_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_182_ENUM_NAME
        case OPT_182_ENUM_NAME:
            props.p_txt_help    = OPT_182_TXT_HELP;
            props.incompat_grps = OPT_182_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_183_ENUM_NAME
        case OPT_183_ENUM_NAME:
            props.p_txt_help    = OPT_183_TXT_HELP;
            props.incompat_grps = OPT_183_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_184_ENUM_NAME
        case OPT_184_ENUM_NAME:
            props.p_txt_help    = OPT_184_TXT_HELP;
            props.incompat_grps = OPT_184_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_185_ENUM_NAME
        case OPT_185_ENUM_NAME:
            props.p_txt_help    = OPT_185_TXT_HELP;
            props.incompat_grps = OPT_185_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_186_ENUM_NAME
        case OPT_186_ENUM_NAME:
            props.p_txt_help    = OPT_186_TXT_HELP;
            props.incompat_grps = OPT_186_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_187_ENUM_NAME
        case OPT_187_ENUM_NAME:
            props.p_txt_help    = OPT_187_TXT_HELP;
            props.incompat_grps = OPT_187_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_188_ENUM_NAME
        case OPT_188_ENUM_NAME:
            props.p_txt_help    = OPT_188_TXT_HELP;
            props.incompat_grps = OPT_188_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_189_ENUM_NAME
        case OPT_189_ENUM_NAME:
            props.p_txt_help    = OPT_189_TXT_HELP;
            props.incompat_grps = OPT_189_INCOMPAT_GRPS;
        break;
      #endif
        
      #ifdef OPT_190_ENUM_NAME
        case OPT_190_ENUM_NAME:
            props.p_txt_help    = OPT_190_TXT_HELP;
            props.incompat_grps = OPT_190_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_191_ENUM_NAME
        case OPT_191_ENUM_NAME:
            props.p_txt_help    = OPT_191_TXT_HELP;
            props.incompat_grps = OPT_191_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_192_ENUM_NAME
        case OPT_192_ENUM_NAME:
            props.p_txt_help    = OPT_192_TXT_HELP;
            props.incompat_grps = OPT_192_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_193_ENUM_NAME
        case OPT_193_ENUM_NAME:
            props.p_txt_help    = OPT_193_TXT_HELP;
            props.incompat_grps = OPT_193_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_194_ENUM_NAME
        case OPT_194_ENUM_NAME:
            props.p_txt_help    = OPT_194_TXT_HELP;
            props.incompat_grps = OPT_194_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_195_ENUM_NAME
        case OPT_195_ENUM_NAME:
            props.p_txt_help    = OPT_195_TXT_HELP;
            props.incompat_grps = OPT_195_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_196_ENUM_NAME
        case OPT_196_ENUM_NAME:
            props.p_txt_help    = OPT_196_TXT_HELP;
            props.incompat_grps = OPT_196_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_197_ENUM_NAME
        case OPT_197_ENUM_NAME:
            props.p_txt_help    = OPT_197_TXT_HELP;
            props.incompat_grps = OPT_197_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_198_ENUM_NAME
        case OPT_198_ENUM_NAME:
            props.p_txt_help    = OPT_198_TXT_HELP;
            props.incompat_grps = OPT_198_INCOMPAT_GRPS;
        break;
      #endif
      #ifdef OPT_199_ENUM_NAME
        case OPT_199_ENUM_NAME:
            props.p_txt_help    = OPT_199_TXT_HELP;
            props.incompat_grps = OPT_199_INCOMPAT_GRPS;
        break;
      #endif

        default:
            props.p_txt_help    = TXT_HELP_INVALID_ITEM;
            props.incompat_grps = OPT_GRP_NONE;
        break;
    }
    
    return (props);
}

/* ==== PUBLIC FUNCTIONS =================================================== */

const struct option* cli_get_longopts(void)
{
    return (longopts);
}

const char* cli_get_txt_shortopts(void)
{
    return (txt_shortopts);
}


inline const char* cli_opt_get_txt_help(cli_opt_t opt)
{
    return (opt_get_props(opt).p_txt_help);
}

inline uint32_t cli_opt_get_incompat_grps(cli_opt_t opt)
{
    return (opt_get_props(opt).incompat_grps);
}








/* ==== TYPEDEFS & DATA : MANDOPT ========================================== */

static mandopt_optbuf_t internal_optbuf = {{OPT_NONE}};

/* ==== PUBLIC FUNCTIONS : MANDOPT ========================================= */

void cli_mandopt_print(const char* p_txt_indent, const char* p_txt_delim)
{
    assert(NULL != p_txt_delim);
    
    
    const char* p_txt_tmp = p_txt_indent;
    for (uint8_t i = 0u; (MANDOPT_OPTS_LN > i); (++i))
    {
        const char* p_txt_opt = cli_opt_get_txt_help(internal_optbuf.opts[i]);
        if ((NULL != p_txt_opt) && ('\0' != (p_txt_opt[0])))
        {
            printf("%s%s", p_txt_tmp, p_txt_opt);
            p_txt_tmp = p_txt_delim;
        }
    }
    printf("\n");
}

void cli_mandopt_clear(void)
{
    memset(&internal_optbuf, 0, sizeof(internal_optbuf));
}

int cli_mandopt_check(const mandopt_t* p_mandopts, const uint8_t mandopts_ln)
{
    assert(NULL != p_mandopts);
    
    
    int rtn = CLI_ERR;
    
    uint8_t i = UINT8_MAX;  /* WARNING: intentional use of owf behavior */ 
    while ((mandopts_ln > (++i)) && (p_mandopts[i].is_valid)) { /* empty */ };
    if (mandopts_ln <= i)
    {
        /* all items are valid */
        rtn = CLI_OK;
    }
    else
    {
        /* invalid item encountered */
        const mandopt_t *const p_item = (p_mandopts + i);
        if (NULL == (p_item->p_mandopt_optbuf))
        {
            /* NULL optbuf == use single opt */
            internal_optbuf.opts[0] = p_item->opt;
        }
        else
        {
            /* non-NULL optbuf == use optbuf data */
            internal_optbuf = *(p_item->p_mandopt_optbuf);
        }
        rtn = CLI_ERR_MISSING_MANDOPT;
    }
    
    return (rtn);
}

/* only for daemon ; do not use casually */
void cli_mandopt_getinternal(mandopt_optbuf_t* p_rtn_optbuf)
{
    assert(NULL != p_rtn_optbuf);
    *p_rtn_optbuf = internal_optbuf;
}

/* only for daemon ; do not use casually */
void cli_mandopt_setinternal(mandopt_optbuf_t *const p_optbuf)
{
    assert(NULL != p_optbuf);
    internal_optbuf = *p_optbuf;
}

/* ==== TESTMODE constants ================================================= */

#if !defined(NDEBUG)
const char *const TEST_defopts__p_txt_shortopts = txt_shortopts;
const struct option *const TEST_defopts__p_longopts = longopts;

const char *const *const TEST_defopts__p_txt_mandopts = txt_mandopts;
#endif

/* ========================================================================= */
