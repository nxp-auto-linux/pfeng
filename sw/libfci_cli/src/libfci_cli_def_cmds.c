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

#ifdef CMD_100_ENUM_NAME
   int CMD_100_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_101_ENUM_NAME
   int CMD_101_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_102_ENUM_NAME
   int CMD_102_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_103_ENUM_NAME
   int CMD_103_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_104_ENUM_NAME
   int CMD_104_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_105_ENUM_NAME
   int CMD_105_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_106_ENUM_NAME
   int CMD_106_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_107_ENUM_NAME
   int CMD_107_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_108_ENUM_NAME
   int CMD_108_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_109_ENUM_NAME
   int CMD_109_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_110_ENUM_NAME
   int CMD_110_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_111_ENUM_NAME
   int CMD_111_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_112_ENUM_NAME
   int CMD_112_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_113_ENUM_NAME
   int CMD_113_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_114_ENUM_NAME
   int CMD_114_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_115_ENUM_NAME
   int CMD_115_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_116_ENUM_NAME
   int CMD_116_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_117_ENUM_NAME
   int CMD_117_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_118_ENUM_NAME
   int CMD_118_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_119_ENUM_NAME
   int CMD_119_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_120_ENUM_NAME
   int CMD_120_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_121_ENUM_NAME
   int CMD_121_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_122_ENUM_NAME
   int CMD_122_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_123_ENUM_NAME
   int CMD_123_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_124_ENUM_NAME
   int CMD_124_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_125_ENUM_NAME
   int CMD_125_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_126_ENUM_NAME
   int CMD_126_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_127_ENUM_NAME
   int CMD_127_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_128_ENUM_NAME
   int CMD_128_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_129_ENUM_NAME
   int CMD_129_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_130_ENUM_NAME
   int CMD_130_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_131_ENUM_NAME
   int CMD_131_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_132_ENUM_NAME
   int CMD_132_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_133_ENUM_NAME
   int CMD_133_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_134_ENUM_NAME
   int CMD_134_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_135_ENUM_NAME
   int CMD_135_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_136_ENUM_NAME
   int CMD_136_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_137_ENUM_NAME
   int CMD_137_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_138_ENUM_NAME
   int CMD_138_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_139_ENUM_NAME
   int CMD_139_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_140_ENUM_NAME
   int CMD_140_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_141_ENUM_NAME
   int CMD_141_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_142_ENUM_NAME
   int CMD_142_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_143_ENUM_NAME
   int CMD_143_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_144_ENUM_NAME
   int CMD_144_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_145_ENUM_NAME
   int CMD_145_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_146_ENUM_NAME
   int CMD_146_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_147_ENUM_NAME
   int CMD_147_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_148_ENUM_NAME
   int CMD_148_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_149_ENUM_NAME
   int CMD_149_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_150_ENUM_NAME
   int CMD_150_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_151_ENUM_NAME
   int CMD_151_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_152_ENUM_NAME
   int CMD_152_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_153_ENUM_NAME
   int CMD_153_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_154_ENUM_NAME
   int CMD_154_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_155_ENUM_NAME
   int CMD_155_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_156_ENUM_NAME
   int CMD_156_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_157_ENUM_NAME
   int CMD_157_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_158_ENUM_NAME
   int CMD_158_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_159_ENUM_NAME
   int CMD_159_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_160_ENUM_NAME
   int CMD_160_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_161_ENUM_NAME
   int CMD_161_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_162_ENUM_NAME
   int CMD_162_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_163_ENUM_NAME
   int CMD_163_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_164_ENUM_NAME
   int CMD_164_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_165_ENUM_NAME
   int CMD_165_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_166_ENUM_NAME
   int CMD_166_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_167_ENUM_NAME
   int CMD_167_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_168_ENUM_NAME
   int CMD_168_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_169_ENUM_NAME
   int CMD_169_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_170_ENUM_NAME
   int CMD_170_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_171_ENUM_NAME
   int CMD_171_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_172_ENUM_NAME
   int CMD_172_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_173_ENUM_NAME
   int CMD_173_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_174_ENUM_NAME
   int CMD_174_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_175_ENUM_NAME
   int CMD_175_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_176_ENUM_NAME
   int CMD_176_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_177_ENUM_NAME
   int CMD_177_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_178_ENUM_NAME
   int CMD_178_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_179_ENUM_NAME
   int CMD_179_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_180_ENUM_NAME
   int CMD_180_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_181_ENUM_NAME
   int CMD_181_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_182_ENUM_NAME
   int CMD_182_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_183_ENUM_NAME
   int CMD_183_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_184_ENUM_NAME
   int CMD_184_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_185_ENUM_NAME
   int CMD_185_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_186_ENUM_NAME
   int CMD_186_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_187_ENUM_NAME
   int CMD_187_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_188_ENUM_NAME
   int CMD_188_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_189_ENUM_NAME
   int CMD_189_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif

#ifdef CMD_190_ENUM_NAME
   int CMD_190_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_191_ENUM_NAME
   int CMD_191_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_192_ENUM_NAME
   int CMD_192_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_193_ENUM_NAME
   int CMD_193_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_194_ENUM_NAME
   int CMD_194_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_195_ENUM_NAME
   int CMD_195_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_196_ENUM_NAME
   int CMD_196_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_197_ENUM_NAME
   int CMD_197_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_198_ENUM_NAME
   int CMD_198_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
#endif
#ifdef CMD_199_ENUM_NAME
   int CMD_199_CMDEXEC (const cli_cmdargs_t* p_cmdargs);
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

#ifdef CMD_100_ENUM_NAME
       CMD_100_CMDEXEC,
#endif
#ifdef CMD_101_ENUM_NAME
       CMD_101_CMDEXEC,
#endif
#ifdef CMD_102_ENUM_NAME
       CMD_102_CMDEXEC,
#endif
#ifdef CMD_103_ENUM_NAME
       CMD_103_CMDEXEC,
#endif
#ifdef CMD_104_ENUM_NAME
       CMD_104_CMDEXEC,
#endif
#ifdef CMD_105_ENUM_NAME
       CMD_105_CMDEXEC,
#endif
#ifdef CMD_106_ENUM_NAME
       CMD_106_CMDEXEC,
#endif
#ifdef CMD_107_ENUM_NAME
       CMD_107_CMDEXEC,
#endif
#ifdef CMD_108_ENUM_NAME
       CMD_108_CMDEXEC,
#endif
#ifdef CMD_109_ENUM_NAME
       CMD_109_CMDEXEC,
#endif

#ifdef CMD_110_ENUM_NAME
       CMD_110_CMDEXEC,
#endif
#ifdef CMD_111_ENUM_NAME
       CMD_111_CMDEXEC,
#endif
#ifdef CMD_112_ENUM_NAME
       CMD_112_CMDEXEC,
#endif
#ifdef CMD_113_ENUM_NAME
       CMD_113_CMDEXEC,
#endif
#ifdef CMD_114_ENUM_NAME
       CMD_114_CMDEXEC,
#endif
#ifdef CMD_115_ENUM_NAME
       CMD_115_CMDEXEC,
#endif
#ifdef CMD_116_ENUM_NAME
       CMD_116_CMDEXEC,
#endif
#ifdef CMD_117_ENUM_NAME
       CMD_117_CMDEXEC,
#endif
#ifdef CMD_118_ENUM_NAME
       CMD_118_CMDEXEC,
#endif
#ifdef CMD_119_ENUM_NAME
       CMD_119_CMDEXEC,
#endif

#ifdef CMD_120_ENUM_NAME
       CMD_120_CMDEXEC,
#endif
#ifdef CMD_121_ENUM_NAME
       CMD_121_CMDEXEC,
#endif
#ifdef CMD_122_ENUM_NAME
       CMD_122_CMDEXEC,
#endif
#ifdef CMD_123_ENUM_NAME
       CMD_123_CMDEXEC,
#endif
#ifdef CMD_124_ENUM_NAME
       CMD_124_CMDEXEC,
#endif
#ifdef CMD_125_ENUM_NAME
       CMD_125_CMDEXEC,
#endif
#ifdef CMD_126_ENUM_NAME
       CMD_126_CMDEXEC,
#endif
#ifdef CMD_127_ENUM_NAME
       CMD_127_CMDEXEC,
#endif
#ifdef CMD_128_ENUM_NAME
       CMD_128_CMDEXEC,
#endif
#ifdef CMD_129_ENUM_NAME
       CMD_129_CMDEXEC,
#endif

#ifdef CMD_130_ENUM_NAME
       CMD_130_CMDEXEC,
#endif
#ifdef CMD_131_ENUM_NAME
       CMD_131_CMDEXEC,
#endif
#ifdef CMD_132_ENUM_NAME
       CMD_132_CMDEXEC,
#endif
#ifdef CMD_133_ENUM_NAME
       CMD_133_CMDEXEC,
#endif
#ifdef CMD_134_ENUM_NAME
       CMD_134_CMDEXEC,
#endif
#ifdef CMD_135_ENUM_NAME
       CMD_135_CMDEXEC,
#endif
#ifdef CMD_136_ENUM_NAME
       CMD_136_CMDEXEC,
#endif
#ifdef CMD_137_ENUM_NAME
       CMD_137_CMDEXEC,
#endif
#ifdef CMD_138_ENUM_NAME
       CMD_138_CMDEXEC,
#endif
#ifdef CMD_139_ENUM_NAME
       CMD_139_CMDEXEC,
#endif

#ifdef CMD_140_ENUM_NAME
       CMD_140_CMDEXEC,
#endif
#ifdef CMD_141_ENUM_NAME
       CMD_141_CMDEXEC,
#endif
#ifdef CMD_142_ENUM_NAME
       CMD_142_CMDEXEC,
#endif
#ifdef CMD_143_ENUM_NAME
       CMD_143_CMDEXEC,
#endif
#ifdef CMD_144_ENUM_NAME
       CMD_144_CMDEXEC,
#endif
#ifdef CMD_145_ENUM_NAME
       CMD_145_CMDEXEC,
#endif
#ifdef CMD_146_ENUM_NAME
       CMD_146_CMDEXEC,
#endif
#ifdef CMD_147_ENUM_NAME
       CMD_147_CMDEXEC,
#endif
#ifdef CMD_148_ENUM_NAME
       CMD_148_CMDEXEC,
#endif
#ifdef CMD_149_ENUM_NAME
       CMD_149_CMDEXEC,
#endif

#ifdef CMD_150_ENUM_NAME
       CMD_150_CMDEXEC,
#endif
#ifdef CMD_151_ENUM_NAME
       CMD_151_CMDEXEC,
#endif
#ifdef CMD_152_ENUM_NAME
       CMD_152_CMDEXEC,
#endif
#ifdef CMD_153_ENUM_NAME
       CMD_153_CMDEXEC,
#endif
#ifdef CMD_154_ENUM_NAME
       CMD_154_CMDEXEC,
#endif
#ifdef CMD_155_ENUM_NAME
       CMD_155_CMDEXEC,
#endif
#ifdef CMD_156_ENUM_NAME
       CMD_156_CMDEXEC,
#endif
#ifdef CMD_157_ENUM_NAME
       CMD_157_CMDEXEC,
#endif
#ifdef CMD_158_ENUM_NAME
       CMD_158_CMDEXEC,
#endif
#ifdef CMD_159_ENUM_NAME
       CMD_159_CMDEXEC,
#endif

#ifdef CMD_160_ENUM_NAME
       CMD_160_CMDEXEC,
#endif
#ifdef CMD_161_ENUM_NAME
       CMD_161_CMDEXEC,
#endif
#ifdef CMD_162_ENUM_NAME
       CMD_162_CMDEXEC,
#endif
#ifdef CMD_163_ENUM_NAME
       CMD_163_CMDEXEC,
#endif
#ifdef CMD_164_ENUM_NAME
       CMD_164_CMDEXEC,
#endif
#ifdef CMD_165_ENUM_NAME
       CMD_165_CMDEXEC,
#endif
#ifdef CMD_166_ENUM_NAME
       CMD_166_CMDEXEC,
#endif
#ifdef CMD_167_ENUM_NAME
       CMD_167_CMDEXEC,
#endif
#ifdef CMD_168_ENUM_NAME
       CMD_168_CMDEXEC,
#endif
#ifdef CMD_169_ENUM_NAME
       CMD_169_CMDEXEC,
#endif

#ifdef CMD_170_ENUM_NAME
       CMD_170_CMDEXEC,
#endif
#ifdef CMD_171_ENUM_NAME
       CMD_171_CMDEXEC,
#endif
#ifdef CMD_172_ENUM_NAME
       CMD_172_CMDEXEC,
#endif
#ifdef CMD_173_ENUM_NAME
       CMD_173_CMDEXEC,
#endif
#ifdef CMD_174_ENUM_NAME
       CMD_174_CMDEXEC,
#endif
#ifdef CMD_175_ENUM_NAME
       CMD_175_CMDEXEC,
#endif
#ifdef CMD_176_ENUM_NAME
       CMD_176_CMDEXEC,
#endif
#ifdef CMD_177_ENUM_NAME
       CMD_177_CMDEXEC,
#endif
#ifdef CMD_178_ENUM_NAME
       CMD_178_CMDEXEC,
#endif
#ifdef CMD_179_ENUM_NAME
       CMD_179_CMDEXEC,
#endif

#ifdef CMD_180_ENUM_NAME
       CMD_180_CMDEXEC,
#endif
#ifdef CMD_181_ENUM_NAME
       CMD_181_CMDEXEC,
#endif
#ifdef CMD_182_ENUM_NAME
       CMD_182_CMDEXEC,
#endif
#ifdef CMD_183_ENUM_NAME
       CMD_183_CMDEXEC,
#endif
#ifdef CMD_184_ENUM_NAME
       CMD_184_CMDEXEC,
#endif
#ifdef CMD_185_ENUM_NAME
       CMD_185_CMDEXEC,
#endif
#ifdef CMD_186_ENUM_NAME
       CMD_186_CMDEXEC,
#endif
#ifdef CMD_187_ENUM_NAME
       CMD_187_CMDEXEC,
#endif
#ifdef CMD_188_ENUM_NAME
       CMD_188_CMDEXEC,
#endif
#ifdef CMD_189_ENUM_NAME
       CMD_189_CMDEXEC,
#endif

#ifdef CMD_190_ENUM_NAME
       CMD_190_CMDEXEC,
#endif
#ifdef CMD_191_ENUM_NAME
       CMD_191_CMDEXEC,
#endif
#ifdef CMD_192_ENUM_NAME
       CMD_192_CMDEXEC,
#endif
#ifdef CMD_193_ENUM_NAME
       CMD_193_CMDEXEC,
#endif
#ifdef CMD_194_ENUM_NAME
       CMD_194_CMDEXEC,
#endif
#ifdef CMD_195_ENUM_NAME
       CMD_195_CMDEXEC,
#endif
#ifdef CMD_196_ENUM_NAME
       CMD_196_CMDEXEC,
#endif
#ifdef CMD_197_ENUM_NAME
       CMD_197_CMDEXEC,
#endif
#ifdef CMD_198_ENUM_NAME
       CMD_198_CMDEXEC,
#endif
#ifdef CMD_199_ENUM_NAME
       CMD_199_CMDEXEC,
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

#ifdef CMD_100_ENUM_NAME
       CMD_100_CLI_TXT,
#endif
#ifdef CMD_101_ENUM_NAME
       CMD_101_CLI_TXT,
#endif
#ifdef CMD_102_ENUM_NAME
       CMD_102_CLI_TXT,
#endif
#ifdef CMD_103_ENUM_NAME
       CMD_103_CLI_TXT,
#endif
#ifdef CMD_104_ENUM_NAME
       CMD_104_CLI_TXT,
#endif
#ifdef CMD_105_ENUM_NAME
       CMD_105_CLI_TXT,
#endif
#ifdef CMD_106_ENUM_NAME
       CMD_106_CLI_TXT,
#endif
#ifdef CMD_107_ENUM_NAME
       CMD_107_CLI_TXT,
#endif
#ifdef CMD_108_ENUM_NAME
       CMD_108_CLI_TXT,
#endif
#ifdef CMD_109_ENUM_NAME
       CMD_109_CLI_TXT,
#endif

#ifdef CMD_110_ENUM_NAME
       CMD_110_CLI_TXT,
#endif
#ifdef CMD_111_ENUM_NAME
       CMD_111_CLI_TXT,
#endif
#ifdef CMD_112_ENUM_NAME
       CMD_112_CLI_TXT,
#endif
#ifdef CMD_113_ENUM_NAME
       CMD_113_CLI_TXT,
#endif
#ifdef CMD_114_ENUM_NAME
       CMD_114_CLI_TXT,
#endif
#ifdef CMD_115_ENUM_NAME
       CMD_115_CLI_TXT,
#endif
#ifdef CMD_116_ENUM_NAME
       CMD_116_CLI_TXT,
#endif
#ifdef CMD_117_ENUM_NAME
       CMD_117_CLI_TXT,
#endif
#ifdef CMD_118_ENUM_NAME
       CMD_118_CLI_TXT,
#endif
#ifdef CMD_119_ENUM_NAME
       CMD_119_CLI_TXT,
#endif

#ifdef CMD_120_ENUM_NAME
       CMD_120_CLI_TXT,
#endif
#ifdef CMD_121_ENUM_NAME
       CMD_121_CLI_TXT,
#endif
#ifdef CMD_122_ENUM_NAME
       CMD_122_CLI_TXT,
#endif
#ifdef CMD_123_ENUM_NAME
       CMD_123_CLI_TXT,
#endif
#ifdef CMD_124_ENUM_NAME
       CMD_124_CLI_TXT,
#endif
#ifdef CMD_125_ENUM_NAME
       CMD_125_CLI_TXT,
#endif
#ifdef CMD_126_ENUM_NAME
       CMD_126_CLI_TXT,
#endif
#ifdef CMD_127_ENUM_NAME
       CMD_127_CLI_TXT,
#endif
#ifdef CMD_128_ENUM_NAME
       CMD_128_CLI_TXT,
#endif
#ifdef CMD_129_ENUM_NAME
       CMD_129_CLI_TXT,
#endif

#ifdef CMD_130_ENUM_NAME
       CMD_130_CLI_TXT,
#endif
#ifdef CMD_131_ENUM_NAME
       CMD_131_CLI_TXT,
#endif
#ifdef CMD_132_ENUM_NAME
       CMD_132_CLI_TXT,
#endif
#ifdef CMD_133_ENUM_NAME
       CMD_133_CLI_TXT,
#endif
#ifdef CMD_134_ENUM_NAME
       CMD_134_CLI_TXT,
#endif
#ifdef CMD_135_ENUM_NAME
       CMD_135_CLI_TXT,
#endif
#ifdef CMD_136_ENUM_NAME
       CMD_136_CLI_TXT,
#endif
#ifdef CMD_137_ENUM_NAME
       CMD_137_CLI_TXT,
#endif
#ifdef CMD_138_ENUM_NAME
       CMD_138_CLI_TXT,
#endif
#ifdef CMD_139_ENUM_NAME
       CMD_139_CLI_TXT,
#endif

#ifdef CMD_140_ENUM_NAME
       CMD_140_CLI_TXT,
#endif
#ifdef CMD_141_ENUM_NAME
       CMD_141_CLI_TXT,
#endif
#ifdef CMD_142_ENUM_NAME
       CMD_142_CLI_TXT,
#endif
#ifdef CMD_143_ENUM_NAME
       CMD_143_CLI_TXT,
#endif
#ifdef CMD_144_ENUM_NAME
       CMD_144_CLI_TXT,
#endif
#ifdef CMD_145_ENUM_NAME
       CMD_145_CLI_TXT,
#endif
#ifdef CMD_146_ENUM_NAME
       CMD_146_CLI_TXT,
#endif
#ifdef CMD_147_ENUM_NAME
       CMD_147_CLI_TXT,
#endif
#ifdef CMD_148_ENUM_NAME
       CMD_148_CLI_TXT,
#endif
#ifdef CMD_149_ENUM_NAME
       CMD_149_CLI_TXT,
#endif

#ifdef CMD_150_ENUM_NAME
       CMD_150_CLI_TXT,
#endif
#ifdef CMD_151_ENUM_NAME
       CMD_151_CLI_TXT,
#endif
#ifdef CMD_152_ENUM_NAME
       CMD_152_CLI_TXT,
#endif
#ifdef CMD_153_ENUM_NAME
       CMD_153_CLI_TXT,
#endif
#ifdef CMD_154_ENUM_NAME
       CMD_154_CLI_TXT,
#endif
#ifdef CMD_155_ENUM_NAME
       CMD_155_CLI_TXT,
#endif
#ifdef CMD_156_ENUM_NAME
       CMD_156_CLI_TXT,
#endif
#ifdef CMD_157_ENUM_NAME
       CMD_157_CLI_TXT,
#endif
#ifdef CMD_158_ENUM_NAME
       CMD_158_CLI_TXT,
#endif
#ifdef CMD_159_ENUM_NAME
       CMD_159_CLI_TXT,
#endif

#ifdef CMD_160_ENUM_NAME
       CMD_160_CLI_TXT,
#endif
#ifdef CMD_161_ENUM_NAME
       CMD_161_CLI_TXT,
#endif
#ifdef CMD_162_ENUM_NAME
       CMD_162_CLI_TXT,
#endif
#ifdef CMD_163_ENUM_NAME
       CMD_163_CLI_TXT,
#endif
#ifdef CMD_164_ENUM_NAME
       CMD_164_CLI_TXT,
#endif
#ifdef CMD_165_ENUM_NAME
       CMD_165_CLI_TXT,
#endif
#ifdef CMD_166_ENUM_NAME
       CMD_166_CLI_TXT,
#endif
#ifdef CMD_167_ENUM_NAME
       CMD_167_CLI_TXT,
#endif
#ifdef CMD_168_ENUM_NAME
       CMD_168_CLI_TXT,
#endif
#ifdef CMD_169_ENUM_NAME
       CMD_169_CLI_TXT,
#endif

#ifdef CMD_170_ENUM_NAME
       CMD_170_CLI_TXT,
#endif
#ifdef CMD_171_ENUM_NAME
       CMD_171_CLI_TXT,
#endif
#ifdef CMD_172_ENUM_NAME
       CMD_172_CLI_TXT,
#endif
#ifdef CMD_173_ENUM_NAME
       CMD_173_CLI_TXT,
#endif
#ifdef CMD_174_ENUM_NAME
       CMD_174_CLI_TXT,
#endif
#ifdef CMD_175_ENUM_NAME
       CMD_175_CLI_TXT,
#endif
#ifdef CMD_176_ENUM_NAME
       CMD_176_CLI_TXT,
#endif
#ifdef CMD_177_ENUM_NAME
       CMD_177_CLI_TXT,
#endif
#ifdef CMD_178_ENUM_NAME
       CMD_178_CLI_TXT,
#endif
#ifdef CMD_179_ENUM_NAME
       CMD_179_CLI_TXT,
#endif

#ifdef CMD_180_ENUM_NAME
       CMD_180_CLI_TXT,
#endif
#ifdef CMD_181_ENUM_NAME
       CMD_181_CLI_TXT,
#endif
#ifdef CMD_182_ENUM_NAME
       CMD_182_CLI_TXT,
#endif
#ifdef CMD_183_ENUM_NAME
       CMD_183_CLI_TXT,
#endif
#ifdef CMD_184_ENUM_NAME
       CMD_184_CLI_TXT,
#endif
#ifdef CMD_185_ENUM_NAME
       CMD_185_CLI_TXT,
#endif
#ifdef CMD_186_ENUM_NAME
       CMD_186_CLI_TXT,
#endif
#ifdef CMD_187_ENUM_NAME
       CMD_187_CLI_TXT,
#endif
#ifdef CMD_188_ENUM_NAME
       CMD_188_CLI_TXT,
#endif
#ifdef CMD_189_ENUM_NAME
       CMD_189_CLI_TXT,
#endif

#ifdef CMD_190_ENUM_NAME
       CMD_190_CLI_TXT,
#endif
#ifdef CMD_191_ENUM_NAME
       CMD_191_CLI_TXT,
#endif
#ifdef CMD_192_ENUM_NAME
       CMD_192_CLI_TXT,
#endif
#ifdef CMD_193_ENUM_NAME
       CMD_193_CLI_TXT,
#endif
#ifdef CMD_194_ENUM_NAME
       CMD_194_CLI_TXT,
#endif
#ifdef CMD_195_ENUM_NAME
       CMD_195_CLI_TXT,
#endif
#ifdef CMD_196_ENUM_NAME
       CMD_196_CLI_TXT,
#endif
#ifdef CMD_197_ENUM_NAME
       CMD_197_CLI_TXT,
#endif
#ifdef CMD_198_ENUM_NAME
       CMD_198_CLI_TXT,
#endif
#ifdef CMD_199_ENUM_NAME
       CMD_199_CLI_TXT,
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

inline bool cli_cmd_is_daemon_related(uint16_t value)
{
    switch (value)
    {
        case CMD_DAEMON_PRINT:
        case CMD_DAEMON_UPDATE:
        case CMD_DAEMON_START:
        case CMD_DAEMON_STOP:
            return (true);
        break;
        
        default:
            return (false);
        break;
    }
}

inline bool cli_cmd_is_not_daemon_related(uint16_t value)
{
    return !(cli_cmd_is_daemon_related(value));
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
