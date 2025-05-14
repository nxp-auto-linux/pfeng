/*
 * Original code: https://www.pixelbeat.org/programming/gcc/static_assert.html
 * Licensed under the GNU All-Permissive License.
 *
 * Modifications Copyright 2018-2022 NXP
 *
 * SPDX-License-Identifier: FSFAP
 *
 */
 
#ifndef CT_ASSERT_H
#define CT_ASSERT_H

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)

#define ct_assert(e) enum { ASSERT_CONCAT(precompile_assert_, __COUNTER__) = (1/(!!(e))) }

#ifdef PFE_CFG_TARGET_OS_AUTOSAR
    /* The compiler options for MCAL driver generate errors or warnings when offsetof is used inside of ct_assert.
    So it is not possible to assert offset of structures at compile time for MCAL driver.
    This is done at runtime in test case Eth_43_PFE_TC_CT_ASSERT in test suite Eth_43_PFE_TS_017.
    Therefore ct_assert_offsetof is a dummy implementation on MCAL driver.
    */
    #define ct_assert_offsetof(e) enum { ASSERT_CONCAT(precompile_assert_, __COUNTER__) = 1 } 
#else
    #define ct_assert_offsetof(e) enum { ASSERT_CONCAT(precompile_assert_, __COUNTER__) = 1/(!!(e)) }
#endif /* PFE_CFG_TARGET_OS_AUTOSAR */

#endif /* CT_ASSERT_H */
