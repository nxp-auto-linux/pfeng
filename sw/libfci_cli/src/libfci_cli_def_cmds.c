/* =========================================================================
 *  (c) Copyright 2006-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017, 2019-2021 NXP
 *
 *  NXP Confidential. This software is owned or controlled by NXP and may only
 *  be used strictly in accordance with the applicable license terms. By
 *  expressly accepting such terms or by downloading, installing, activating
 *  and/or otherwise using the software, you are agreeing that you have read,
 *  and that you agree to comply with and are bound by, such license terms. If
 *  you do not agree to be bound by the applicable license terms, then you may
 *  not retain, install, activate or otherwise use the software.
 *
 *  This file contains sample code only. It is not part of the production code deliverables.
 * ========================================================================= */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "libfci_cli_common.h"
#include "libfci_cli_def_cmds.h"

/* ==== TESTMODE vars ====================================================== */

#if !defined(NDEBUG)
cli_cmdargs_t TEST_cmdargs;
#endif

/* ==== TYPEDEFS & DATA ==================================================== */

/* declarations of extern functions */
#ifdef CMD_01_ENUM_NAME
   int CMD_01_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_02_ENUM_NAME
   int CMD_02_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_03_ENUM_NAME
   int CMD_03_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_04_ENUM_NAME
   int CMD_04_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_05_ENUM_NAME
   int CMD_05_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_06_ENUM_NAME
   int CMD_06_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_07_ENUM_NAME
   int CMD_07_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_08_ENUM_NAME
   int CMD_08_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_09_ENUM_NAME
   int CMD_09_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_10_ENUM_NAME
   int CMD_10_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_11_ENUM_NAME
   int CMD_11_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_12_ENUM_NAME
   int CMD_12_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_13_ENUM_NAME
   int CMD_13_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_14_ENUM_NAME
   int CMD_14_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_15_ENUM_NAME
   int CMD_15_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_16_ENUM_NAME
   int CMD_16_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_17_ENUM_NAME
   int CMD_17_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_18_ENUM_NAME
   int CMD_18_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_19_ENUM_NAME
   int CMD_19_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_20_ENUM_NAME
   int CMD_20_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_21_ENUM_NAME
   int CMD_21_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_22_ENUM_NAME
   int CMD_22_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_23_ENUM_NAME
   int CMD_23_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_24_ENUM_NAME
   int CMD_24_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_25_ENUM_NAME
   int CMD_25_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_26_ENUM_NAME
   int CMD_26_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_27_ENUM_NAME
   int CMD_27_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_28_ENUM_NAME
   int CMD_28_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_29_ENUM_NAME
   int CMD_29_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_30_ENUM_NAME
   int CMD_30_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_31_ENUM_NAME
   int CMD_31_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_32_ENUM_NAME
   int CMD_32_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_33_ENUM_NAME
   int CMD_33_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_34_ENUM_NAME
   int CMD_34_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_35_ENUM_NAME
   int CMD_35_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_36_ENUM_NAME
   int CMD_36_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_37_ENUM_NAME
   int CMD_37_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_38_ENUM_NAME
   int CMD_38_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_39_ENUM_NAME
   int CMD_39_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_40_ENUM_NAME
   int CMD_40_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_41_ENUM_NAME
   int CMD_41_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_42_ENUM_NAME
   int CMD_42_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_43_ENUM_NAME
   int CMD_43_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_44_ENUM_NAME
   int CMD_44_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_45_ENUM_NAME
   int CMD_45_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_46_ENUM_NAME
   int CMD_46_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_47_ENUM_NAME
   int CMD_47_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_48_ENUM_NAME
   int CMD_48_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_49_ENUM_NAME
   int CMD_49_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_50_ENUM_NAME
   int CMD_50_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_51_ENUM_NAME
   int CMD_51_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_52_ENUM_NAME
   int CMD_52_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_53_ENUM_NAME
   int CMD_53_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_54_ENUM_NAME
   int CMD_54_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_55_ENUM_NAME
   int CMD_55_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_56_ENUM_NAME
   int CMD_56_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_57_ENUM_NAME
   int CMD_57_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_58_ENUM_NAME
   int CMD_58_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_59_ENUM_NAME
   int CMD_59_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_60_ENUM_NAME
   int CMD_60_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_61_ENUM_NAME
   int CMD_61_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_62_ENUM_NAME
   int CMD_62_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_63_ENUM_NAME
   int CMD_63_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_64_ENUM_NAME
   int CMD_64_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_65_ENUM_NAME
   int CMD_65_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_66_ENUM_NAME
   int CMD_66_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_67_ENUM_NAME
   int CMD_67_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_68_ENUM_NAME
   int CMD_68_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_69_ENUM_NAME
   int CMD_69_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_70_ENUM_NAME
   int CMD_70_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_71_ENUM_NAME
   int CMD_71_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_72_ENUM_NAME
   int CMD_72_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_73_ENUM_NAME
   int CMD_73_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_74_ENUM_NAME
   int CMD_74_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_75_ENUM_NAME
   int CMD_75_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_76_ENUM_NAME
   int CMD_76_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_77_ENUM_NAME
   int CMD_77_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_78_ENUM_NAME
   int CMD_78_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_79_ENUM_NAME
   int CMD_79_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_80_ENUM_NAME
   int CMD_80_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_81_ENUM_NAME
   int CMD_81_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_82_ENUM_NAME
   int CMD_82_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_83_ENUM_NAME
   int CMD_83_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_84_ENUM_NAME
   int CMD_84_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_85_ENUM_NAME
   int CMD_85_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_86_ENUM_NAME
   int CMD_86_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_87_ENUM_NAME
   int CMD_87_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_88_ENUM_NAME
   int CMD_88_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_89_ENUM_NAME
   int CMD_89_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_90_ENUM_NAME
   int CMD_90_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_91_ENUM_NAME
   int CMD_91_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_92_ENUM_NAME
   int CMD_92_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_93_ENUM_NAME
   int CMD_93_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_94_ENUM_NAME
   int CMD_94_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_95_ENUM_NAME
   int CMD_95_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_96_ENUM_NAME
   int CMD_96_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_97_ENUM_NAME
   int CMD_97_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_98_ENUM_NAME
   int CMD_98_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_99_ENUM_NAME
   int CMD_99_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif


static int cmdexec_dummy(const cli_cmdargs_t* p_cmdargs);

/* indexed by 'cmd_t' */
typedef int (*cb_cmdexec_t)(const cli_cmdargs_t* p_cmdargs);
static const cb_cmdexec_t cmdexecs[CMD_LN] = 
{
    cmdexec_dummy,  /* CMD_00_NO_COMMAND_CMD */
    
#ifdef CMD_01_ENUM_NAME
       CMD_01_CMDEXEC,
#endif
#ifdef CMD_02_ENUM_NAME
       CMD_02_CMDEXEC,
#endif
#ifdef CMD_03_ENUM_NAME
       CMD_03_CMDEXEC,
#endif
#ifdef CMD_04_ENUM_NAME
       CMD_04_CMDEXEC,
#endif
#ifdef CMD_05_ENUM_NAME
       CMD_05_CMDEXEC,
#endif
#ifdef CMD_06_ENUM_NAME
       CMD_06_CMDEXEC,
#endif
#ifdef CMD_07_ENUM_NAME
       CMD_07_CMDEXEC,
#endif
#ifdef CMD_08_ENUM_NAME
       CMD_08_CMDEXEC,
#endif
#ifdef CMD_09_ENUM_NAME
       CMD_09_CMDEXEC,
#endif

#ifdef CMD_10_ENUM_NAME
       CMD_10_CMDEXEC,
#endif
#ifdef CMD_11_ENUM_NAME
       CMD_11_CMDEXEC,
#endif
#ifdef CMD_12_ENUM_NAME
       CMD_12_CMDEXEC,
#endif
#ifdef CMD_13_ENUM_NAME
       CMD_13_CMDEXEC,
#endif
#ifdef CMD_14_ENUM_NAME
       CMD_14_CMDEXEC,
#endif
#ifdef CMD_15_ENUM_NAME
       CMD_15_CMDEXEC,
#endif
#ifdef CMD_16_ENUM_NAME
       CMD_16_CMDEXEC,
#endif
#ifdef CMD_17_ENUM_NAME
       CMD_17_CMDEXEC,
#endif
#ifdef CMD_18_ENUM_NAME
       CMD_18_CMDEXEC,
#endif
#ifdef CMD_19_ENUM_NAME
       CMD_19_CMDEXEC,
#endif

#ifdef CMD_20_ENUM_NAME
       CMD_20_CMDEXEC,
#endif
#ifdef CMD_21_ENUM_NAME
       CMD_21_CMDEXEC,
#endif
#ifdef CMD_22_ENUM_NAME
       CMD_22_CMDEXEC,
#endif
#ifdef CMD_23_ENUM_NAME
       CMD_23_CMDEXEC,
#endif
#ifdef CMD_24_ENUM_NAME
       CMD_24_CMDEXEC,
#endif
#ifdef CMD_25_ENUM_NAME
       CMD_25_CMDEXEC,
#endif
#ifdef CMD_26_ENUM_NAME
       CMD_26_CMDEXEC,
#endif
#ifdef CMD_27_ENUM_NAME
       CMD_27_CMDEXEC,
#endif
#ifdef CMD_28_ENUM_NAME
       CMD_28_CMDEXEC,
#endif
#ifdef CMD_29_ENUM_NAME
       CMD_29_CMDEXEC,
#endif

#ifdef CMD_30_ENUM_NAME
       CMD_30_CMDEXEC,
#endif
#ifdef CMD_31_ENUM_NAME
       CMD_31_CMDEXEC,
#endif
#ifdef CMD_32_ENUM_NAME
       CMD_32_CMDEXEC,
#endif
#ifdef CMD_33_ENUM_NAME
       CMD_33_CMDEXEC,
#endif
#ifdef CMD_34_ENUM_NAME
       CMD_34_CMDEXEC,
#endif
#ifdef CMD_35_ENUM_NAME
       CMD_35_CMDEXEC,
#endif
#ifdef CMD_36_ENUM_NAME
       CMD_36_CMDEXEC,
#endif
#ifdef CMD_37_ENUM_NAME
       CMD_37_CMDEXEC,
#endif
#ifdef CMD_38_ENUM_NAME
       CMD_38_CMDEXEC,
#endif
#ifdef CMD_39_ENUM_NAME
       CMD_39_CMDEXEC,
#endif

#ifdef CMD_40_ENUM_NAME
       CMD_40_CMDEXEC,
#endif
#ifdef CMD_41_ENUM_NAME
       CMD_41_CMDEXEC,
#endif
#ifdef CMD_42_ENUM_NAME
       CMD_42_CMDEXEC,
#endif
#ifdef CMD_43_ENUM_NAME
       CMD_43_CMDEXEC,
#endif
#ifdef CMD_44_ENUM_NAME
       CMD_44_CMDEXEC,
#endif
#ifdef CMD_45_ENUM_NAME
       CMD_45_CMDEXEC,
#endif
#ifdef CMD_46_ENUM_NAME
       CMD_46_CMDEXEC,
#endif
#ifdef CMD_47_ENUM_NAME
       CMD_47_CMDEXEC,
#endif
#ifdef CMD_48_ENUM_NAME
       CMD_48_CMDEXEC,
#endif
#ifdef CMD_49_ENUM_NAME
       CMD_49_CMDEXEC,
#endif

#ifdef CMD_50_ENUM_NAME
       CMD_50_CMDEXEC,
#endif
#ifdef CMD_51_ENUM_NAME
       CMD_51_CMDEXEC,
#endif
#ifdef CMD_52_ENUM_NAME
       CMD_52_CMDEXEC,
#endif
#ifdef CMD_53_ENUM_NAME
       CMD_53_CMDEXEC,
#endif
#ifdef CMD_54_ENUM_NAME
       CMD_54_CMDEXEC,
#endif
#ifdef CMD_55_ENUM_NAME
       CMD_55_CMDEXEC,
#endif
#ifdef CMD_56_ENUM_NAME
       CMD_56_CMDEXEC,
#endif
#ifdef CMD_57_ENUM_NAME
       CMD_57_CMDEXEC,
#endif
#ifdef CMD_58_ENUM_NAME
       CMD_58_CMDEXEC,
#endif
#ifdef CMD_59_ENUM_NAME
       CMD_59_CMDEXEC,
#endif

#ifdef CMD_60_ENUM_NAME
       CMD_60_CMDEXEC,
#endif
#ifdef CMD_61_ENUM_NAME
       CMD_61_CMDEXEC,
#endif
#ifdef CMD_62_ENUM_NAME
       CMD_62_CMDEXEC,
#endif
#ifdef CMD_63_ENUM_NAME
       CMD_63_CMDEXEC,
#endif
#ifdef CMD_64_ENUM_NAME
       CMD_64_CMDEXEC,
#endif
#ifdef CMD_65_ENUM_NAME
       CMD_65_CMDEXEC,
#endif
#ifdef CMD_66_ENUM_NAME
       CMD_66_CMDEXEC,
#endif
#ifdef CMD_67_ENUM_NAME
       CMD_67_CMDEXEC,
#endif
#ifdef CMD_68_ENUM_NAME
       CMD_68_CMDEXEC,
#endif
#ifdef CMD_69_ENUM_NAME
       CMD_69_CMDEXEC,
#endif

#ifdef CMD_70_ENUM_NAME
       CMD_70_CMDEXEC,
#endif
#ifdef CMD_71_ENUM_NAME
       CMD_71_CMDEXEC,
#endif
#ifdef CMD_72_ENUM_NAME
       CMD_72_CMDEXEC,
#endif
#ifdef CMD_73_ENUM_NAME
       CMD_73_CMDEXEC,
#endif
#ifdef CMD_74_ENUM_NAME
       CMD_74_CMDEXEC,
#endif
#ifdef CMD_75_ENUM_NAME
       CMD_75_CMDEXEC,
#endif
#ifdef CMD_76_ENUM_NAME
       CMD_76_CMDEXEC,
#endif
#ifdef CMD_77_ENUM_NAME
       CMD_77_CMDEXEC,
#endif
#ifdef CMD_78_ENUM_NAME
       CMD_78_CMDEXEC,
#endif
#ifdef CMD_79_ENUM_NAME
       CMD_79_CMDEXEC,
#endif

#ifdef CMD_80_ENUM_NAME
       CMD_80_CMDEXEC,
#endif
#ifdef CMD_81_ENUM_NAME
       CMD_81_CMDEXEC,
#endif
#ifdef CMD_82_ENUM_NAME
       CMD_82_CMDEXEC,
#endif
#ifdef CMD_83_ENUM_NAME
       CMD_83_CMDEXEC,
#endif
#ifdef CMD_84_ENUM_NAME
       CMD_84_CMDEXEC,
#endif
#ifdef CMD_85_ENUM_NAME
       CMD_85_CMDEXEC,
#endif
#ifdef CMD_86_ENUM_NAME
       CMD_86_CMDEXEC,
#endif
#ifdef CMD_87_ENUM_NAME
       CMD_87_CMDEXEC,
#endif
#ifdef CMD_88_ENUM_NAME
       CMD_88_CMDEXEC,
#endif
#ifdef CMD_89_ENUM_NAME
       CMD_89_CMDEXEC,
#endif

#ifdef CMD_90_ENUM_NAME
       CMD_90_CMDEXEC,
#endif
#ifdef CMD_91_ENUM_NAME
       CMD_91_CMDEXEC,
#endif
#ifdef CMD_92_ENUM_NAME
       CMD_92_CMDEXEC,
#endif
#ifdef CMD_93_ENUM_NAME
       CMD_93_CMDEXEC,
#endif
#ifdef CMD_94_ENUM_NAME
       CMD_94_CMDEXEC,
#endif
#ifdef CMD_95_ENUM_NAME
       CMD_95_CMDEXEC,
#endif
#ifdef CMD_96_ENUM_NAME
       CMD_96_CMDEXEC,
#endif
#ifdef CMD_97_ENUM_NAME
       CMD_97_CMDEXEC,
#endif
#ifdef CMD_98_ENUM_NAME
       CMD_98_CMDEXEC,
#endif
#ifdef CMD_99_ENUM_NAME
       CMD_99_CMDEXEC,
#endif
};

/* indexed by 'cmd_t' */
static const char *const txt_cmdnames[CMD_LN] =
{
    "",  /*  CMD_00_NO_COMMAND  */
    
#ifdef CMD_01_ENUM_NAME
       CMD_01_CLI_TXT,
#endif
#ifdef CMD_02_ENUM_NAME
       CMD_02_CLI_TXT,
#endif
#ifdef CMD_03_ENUM_NAME
       CMD_03_CLI_TXT,
#endif
#ifdef CMD_04_ENUM_NAME
       CMD_04_CLI_TXT,
#endif
#ifdef CMD_05_ENUM_NAME
       CMD_05_CLI_TXT,
#endif
#ifdef CMD_06_ENUM_NAME
       CMD_06_CLI_TXT,
#endif
#ifdef CMD_07_ENUM_NAME
       CMD_07_CLI_TXT,
#endif
#ifdef CMD_08_ENUM_NAME
       CMD_08_CLI_TXT,
#endif
#ifdef CMD_09_ENUM_NAME
       CMD_09_CLI_TXT,
#endif

#ifdef CMD_10_ENUM_NAME
       CMD_10_CLI_TXT,
#endif
#ifdef CMD_11_ENUM_NAME
       CMD_11_CLI_TXT,
#endif
#ifdef CMD_12_ENUM_NAME
       CMD_12_CLI_TXT,
#endif
#ifdef CMD_13_ENUM_NAME
       CMD_13_CLI_TXT,
#endif
#ifdef CMD_14_ENUM_NAME
       CMD_14_CLI_TXT,
#endif
#ifdef CMD_15_ENUM_NAME
       CMD_15_CLI_TXT,
#endif
#ifdef CMD_16_ENUM_NAME
       CMD_16_CLI_TXT,
#endif
#ifdef CMD_17_ENUM_NAME
       CMD_17_CLI_TXT,
#endif
#ifdef CMD_18_ENUM_NAME
       CMD_18_CLI_TXT,
#endif
#ifdef CMD_19_ENUM_NAME
       CMD_19_CLI_TXT,
#endif

#ifdef CMD_20_ENUM_NAME
       CMD_20_CLI_TXT,
#endif
#ifdef CMD_21_ENUM_NAME
       CMD_21_CLI_TXT,
#endif
#ifdef CMD_22_ENUM_NAME
       CMD_22_CLI_TXT,
#endif
#ifdef CMD_23_ENUM_NAME
       CMD_23_CLI_TXT,
#endif
#ifdef CMD_24_ENUM_NAME
       CMD_24_CLI_TXT,
#endif
#ifdef CMD_25_ENUM_NAME
       CMD_25_CLI_TXT,
#endif
#ifdef CMD_26_ENUM_NAME
       CMD_26_CLI_TXT,
#endif
#ifdef CMD_27_ENUM_NAME
       CMD_27_CLI_TXT,
#endif
#ifdef CMD_28_ENUM_NAME
       CMD_28_CLI_TXT,
#endif
#ifdef CMD_29_ENUM_NAME
       CMD_29_CLI_TXT,
#endif

#ifdef CMD_30_ENUM_NAME
       CMD_30_CLI_TXT,
#endif
#ifdef CMD_31_ENUM_NAME
       CMD_31_CLI_TXT,
#endif
#ifdef CMD_32_ENUM_NAME
       CMD_32_CLI_TXT,
#endif
#ifdef CMD_33_ENUM_NAME
       CMD_33_CLI_TXT,
#endif
#ifdef CMD_34_ENUM_NAME
       CMD_34_CLI_TXT,
#endif
#ifdef CMD_35_ENUM_NAME
       CMD_35_CLI_TXT,
#endif
#ifdef CMD_36_ENUM_NAME
       CMD_36_CLI_TXT,
#endif
#ifdef CMD_37_ENUM_NAME
       CMD_37_CLI_TXT,
#endif
#ifdef CMD_38_ENUM_NAME
       CMD_38_CLI_TXT,
#endif
#ifdef CMD_39_ENUM_NAME
       CMD_39_CLI_TXT,
#endif

#ifdef CMD_40_ENUM_NAME
       CMD_40_CLI_TXT,
#endif
#ifdef CMD_41_ENUM_NAME
       CMD_41_CLI_TXT,
#endif
#ifdef CMD_42_ENUM_NAME
       CMD_42_CLI_TXT,
#endif
#ifdef CMD_43_ENUM_NAME
       CMD_43_CLI_TXT,
#endif
#ifdef CMD_44_ENUM_NAME
       CMD_44_CLI_TXT,
#endif
#ifdef CMD_45_ENUM_NAME
       CMD_45_CLI_TXT,
#endif
#ifdef CMD_46_ENUM_NAME
       CMD_46_CLI_TXT,
#endif
#ifdef CMD_47_ENUM_NAME
       CMD_47_CLI_TXT,
#endif
#ifdef CMD_48_ENUM_NAME
       CMD_48_CLI_TXT,
#endif
#ifdef CMD_49_ENUM_NAME
       CMD_49_CLI_TXT,
#endif

#ifdef CMD_50_ENUM_NAME
       CMD_50_CLI_TXT,
#endif
#ifdef CMD_51_ENUM_NAME
       CMD_51_CLI_TXT,
#endif
#ifdef CMD_52_ENUM_NAME
       CMD_52_CLI_TXT,
#endif
#ifdef CMD_53_ENUM_NAME
       CMD_53_CLI_TXT,
#endif
#ifdef CMD_54_ENUM_NAME
       CMD_54_CLI_TXT,
#endif
#ifdef CMD_55_ENUM_NAME
       CMD_55_CLI_TXT,
#endif
#ifdef CMD_56_ENUM_NAME
       CMD_56_CLI_TXT,
#endif
#ifdef CMD_57_ENUM_NAME
       CMD_57_CLI_TXT,
#endif
#ifdef CMD_58_ENUM_NAME
       CMD_58_CLI_TXT,
#endif
#ifdef CMD_59_ENUM_NAME
       CMD_59_CLI_TXT,
#endif

#ifdef CMD_60_ENUM_NAME
       CMD_60_CLI_TXT,
#endif
#ifdef CMD_61_ENUM_NAME
       CMD_61_CLI_TXT,
#endif
#ifdef CMD_62_ENUM_NAME
       CMD_62_CLI_TXT,
#endif
#ifdef CMD_63_ENUM_NAME
       CMD_63_CLI_TXT,
#endif
#ifdef CMD_64_ENUM_NAME
       CMD_64_CLI_TXT,
#endif
#ifdef CMD_65_ENUM_NAME
       CMD_65_CLI_TXT,
#endif
#ifdef CMD_66_ENUM_NAME
       CMD_66_CLI_TXT,
#endif
#ifdef CMD_67_ENUM_NAME
       CMD_67_CLI_TXT,
#endif
#ifdef CMD_68_ENUM_NAME
       CMD_68_CLI_TXT,
#endif
#ifdef CMD_69_ENUM_NAME
       CMD_69_CLI_TXT,
#endif

#ifdef CMD_70_ENUM_NAME
       CMD_70_CLI_TXT,
#endif
#ifdef CMD_71_ENUM_NAME
       CMD_71_CLI_TXT,
#endif
#ifdef CMD_72_ENUM_NAME
       CMD_72_CLI_TXT,
#endif
#ifdef CMD_73_ENUM_NAME
       CMD_73_CLI_TXT,
#endif
#ifdef CMD_74_ENUM_NAME
       CMD_74_CLI_TXT,
#endif
#ifdef CMD_75_ENUM_NAME
       CMD_75_CLI_TXT,
#endif
#ifdef CMD_76_ENUM_NAME
       CMD_76_CLI_TXT,
#endif
#ifdef CMD_77_ENUM_NAME
       CMD_77_CLI_TXT,
#endif
#ifdef CMD_78_ENUM_NAME
       CMD_78_CLI_TXT,
#endif
#ifdef CMD_79_ENUM_NAME
       CMD_79_CLI_TXT,
#endif

#ifdef CMD_80_ENUM_NAME
       CMD_80_CLI_TXT,
#endif
#ifdef CMD_81_ENUM_NAME
       CMD_81_CLI_TXT,
#endif
#ifdef CMD_82_ENUM_NAME
       CMD_82_CLI_TXT,
#endif
#ifdef CMD_83_ENUM_NAME
       CMD_83_CLI_TXT,
#endif
#ifdef CMD_84_ENUM_NAME
       CMD_84_CLI_TXT,
#endif
#ifdef CMD_85_ENUM_NAME
       CMD_85_CLI_TXT,
#endif
#ifdef CMD_86_ENUM_NAME
       CMD_86_CLI_TXT,
#endif
#ifdef CMD_87_ENUM_NAME
       CMD_87_CLI_TXT,
#endif
#ifdef CMD_88_ENUM_NAME
       CMD_88_CLI_TXT,
#endif
#ifdef CMD_89_ENUM_NAME
       CMD_89_CLI_TXT,
#endif

#ifdef CMD_90_ENUM_NAME
       CMD_90_CLI_TXT,
#endif
#ifdef CMD_91_ENUM_NAME
       CMD_91_CLI_TXT,
#endif
#ifdef CMD_92_ENUM_NAME
       CMD_92_CLI_TXT,
#endif
#ifdef CMD_93_ENUM_NAME
       CMD_93_CLI_TXT,
#endif
#ifdef CMD_94_ENUM_NAME
       CMD_94_CLI_TXT,
#endif
#ifdef CMD_95_ENUM_NAME
       CMD_95_CLI_TXT,
#endif
#ifdef CMD_96_ENUM_NAME
       CMD_96_CLI_TXT,
#endif
#ifdef CMD_97_ENUM_NAME
       CMD_97_CLI_TXT,
#endif
#ifdef CMD_98_ENUM_NAME
       CMD_98_CLI_TXT,
#endif
#ifdef CMD_99_ENUM_NAME
       CMD_99_CLI_TXT,
#endif
};

/* ==== PRIVATE FUNCTIONS ================================================== */

inline static int cmdexec_dummy(const cli_cmdargs_t* p_cmdargs)
{
    #if !defined(NDEBUG)
    TEST_cmdargs = *p_cmdargs;
    #endif
    return ((p_cmdargs) ? (CLI_OK) : (CLI_OK));  /* ternary used just to suppress gcc warning */
}

/* ==== PUBLIC FUNCTIONS =================================================== */

inline bool cli_cmd_is_valid(uint16_t value)
{
    return (value < CMD_LN);
}

inline bool cli_cmd_is_not_valid(uint16_t value)
{
    return !(cli_cmd_is_valid(value));
}

int cli_cmd_execute(cli_cmd_t cmd, const cli_cmdargs_t* p_cmdargs)
{
    return ((cli_cmd_is_not_valid(cmd)) ? (CLI_ERR_INVCMD) : (cmdexecs[cmd](p_cmdargs)));
}

int cli_cmd_txt2cmd(cli_cmd_t* p_rtn_cmd, const char* p_txt)
{
    assert(NULL != p_rtn_cmd);
    assert(NULL != p_txt);
    
    
    int rtn = CLI_ERR;
    uint16_t i = 0u;  /* loop intentionally skips element [0] (it is NO_COMMAND) */
    while ((CMD_LN > (++i)) && (0 != strcmp(txt_cmdnames[i], p_txt))) { /* empty */ };
    if (CMD_LN <= i)
    {
        rtn = CLI_ERR_INVCMD;
    }
    else
    {
        *p_rtn_cmd = (cli_cmd_t)(i);  /* WARNING: cast to enum type (valid value is assumed) */
        rtn = CLI_OK;
    }
    
    return (rtn);
}

const char* cli_cmd_cmd2txt(cli_cmd_t cmd)
{
    return ((cli_cmd_is_not_valid(cmd)) ? ("__INVALID_ITEM__") : (txt_cmdnames[cmd]));
}

/* ==== TESTMODE constants ================================================= */

#if !defined(NDEBUG)
const uint16_t TEST_defcmds__cmdnames_ln = (sizeof(txt_cmdnames) / sizeof(char*));
#endif

/* ========================================================================= */
