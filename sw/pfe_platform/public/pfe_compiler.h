/* =========================================================================
 *  Copyright 2020 NXP
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
#ifndef PFE_COMPILER_H
#define PFE_COMPILER_H

/*------------------------------------------------------------------------------------------------*/
/* Section describing behavior of supported compilers:
*  - do no modify this section
*  - macro values are intentionally not 0 and 1 to avoid confusion with undefined macros having
*    one of these values
*/

/** 
* @brief Order of bit fields in the structure 
* @details Either bit field corresponding to the highest bits in the memory is specified as the first
*          member or the last member of the structure.
*/
#define PFE_COMPILER_BITFIELD_HIGH_FIRST 3
#define PFE_COMPILER_BITFIELD_HIGH_LAST 2

/**
* @brief Result of the compilation - either driver or firmware
*/
#define PFE_COMPILER_RESULT_DRV 4
#define PFE_COMPILER_RESULT_FW 5

/*------------------------------------------------------------------------------------------------*/
/* Section describing result of the compilation: */
#define PFE_COMPILER_RESULT PFE_COMPILER_RESULT_DRV

/*------------------------------------------------------------------------------------------------*/
/* Section describing behavior of supported compilers regarding bit-fields position in structure:
*  - when adding a new compiler, just add it and define the macro PFE_COMPILER_BITFIELD_BEHAVIOR to one
*    of the variants
*/

/* Various supported GCC variants: */
#if (defined(__GNUC__))
	#if ((__GNUC__ == 5) && (__GNUC_MINOR__ == 4) && (__GNUC_PATCHLEVEL__ == 0))
		/* GCC version 5.4.0 */
		#if (defined(PFE_CFG_TARGET_ARCH_x86))
			/* Compiling driver */
			#define PFE_COMPILER_BITFIELD_BEHAVIOR PFE_COMPILER_BITFIELD_HIGH_LAST
		#elif (defined(PFE_CFG_TARGET_ARCH_x86_64))
			/* Compiling driver */
			#define PFE_COMPILER_BITFIELD_BEHAVIOR PFE_COMPILER_BITFIELD_HIGH_LAST
		#elif (defined(PFE_CFG_TARGET_ARCH_aarch64le))
			/* Compiling driver */
			#define PFE_COMPILER_BITFIELD_BEHAVIOR PFE_COMPILER_BITFIELD_HIGH_LAST
		#endif
	#elif ((__GNUC__ == 6) && (__GNUC_MINOR__ == 3) && (__GNUC_PATCHLEVEL__ == 1))
		/* GCC version 6.3.1 */
		#if (defined(PFE_CFG_TARGET_ARCH_aarch64))
			/* Compiling driver */
			#define PFE_COMPILER_BITFIELD_BEHAVIOR PFE_COMPILER_BITFIELD_HIGH_LAST
		#elif (defined(PFE_CFG_TARGET_ARCH_armv7le))
			/* Compiling driver */
			#define PFE_COMPILER_BITFIELD_BEHAVIOR PFE_COMPILER_BITFIELD_HIGH_LAST
		#endif
	#elif ((__GNUC__ == 9) && (__GNUC_MINOR__ == 2) && (__GNUC_PATCHLEVEL__ == 0))
		/* GCC version 9.2.0 */
		#if (defined(PFE_CFG_TARGET_ARCH_aarch64))
			/* Compiling driver */
			#define PFE_COMPILER_BITFIELD_BEHAVIOR PFE_COMPILER_BITFIELD_HIGH_LAST
		#endif
	#endif
#endif

/* Supported GHS variants */
#if (defined(__ghs__))
	#if ((__GHS_VERSION_NUMBER == 201814) || (__GHS_VERSION_NUMBER == 201914)) && defined(__LITTLE_ENDIAN__)
		/* Compiling MCAL driver */
		#define PFE_COMPILER_BITFIELD_BEHAVIOR PFE_COMPILER_BITFIELD_HIGH_LAST
	#endif
#endif

/* Supported DIAB variants */
#if (defined(__DCC__))
	#if ((__VERSION_NUMBER__ == 7020) && defined(__ORDER_LITTLE_ENDIAN__))
		/* Compiling MCAL driver */
		#define PFE_COMPILER_BITFIELD_BEHAVIOR PFE_COMPILER_BITFIELD_HIGH_LAST
	#endif
#endif

/*------------------------------------------------------------------------------------------------*/
/* Checks for correctness: */

#if(!defined(PFE_COMPILER_BITFIELD_BEHAVIOR))
	/* Required macro not defined */
	#error Please specify your compiler behavior by defining PFE_COMPILER_BITFIELD_BEHAVIOR.
#endif

#if ((PFE_COMPILER_BITFIELD_BEHAVIOR != PFE_COMPILER_BITFIELD_HIGH_LAST) && (PFE_COMPILER_BITFIELD_BEHAVIOR != PFE_COMPILER_BITFIELD_HIGH_FIRST))
	/* Wrong macro value */
	#error PFE_COMPILER_BITFIELD_BEHAVIOR shall be either PFE_COMPILER_BITFIELD_HIGH_LAST or PFE_COMPILER_BITFIELD_HIGH_FIRST
#endif

#if(!defined(PFE_COMPILER_RESULT))
	/* Required macro not defined */
	#error Please specify your compiler output by defining PFE_COMPILER_RESULT.
#endif

#if ((PFE_COMPILER_RESULT != PFE_COMPILER_RESULT_DRV) && (PFE_COMPILER_RESULT != PFE_COMPILER_RESULT_FW))
	/* Wrong macro value */
	#error PFE_COMPILER_RESULT shall be either PFE_COMPILER_RESULT_DRV or PFE_COMPILER_RESULT_FW
#endif

#endif /* PFE_COMPILER_H */
