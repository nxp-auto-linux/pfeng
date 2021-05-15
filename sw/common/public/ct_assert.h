/*
 * Original code: https://www.pixelbeat.org/programming/gcc/static_assert.html
 * Licensed under the GNU All-Permissive License.
 *
 * Modifications Copyright 2018-2021 NXP
 *
 */
 
#ifndef CT_ASSERT_H
#define CT_ASSERT_H

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)

#ifdef __ghs__ /* AAVB-2386 */
	/* Dummy implementation with no check to avoid compile error */
	#define ct_assert(e) enum { ASSERT_CONCAT(precompile_assert_, __COUNTER__) = 1 } 
#else
	#define ASSERT_CONCAT_INNER(a, b) a##b
	#define ASSERT_CONCAT_OUTER(a, b) ASSERT_CONCAT_INNER(a, b)
	#define ct_assert(e) enum { ASSERT_CONCAT_OUTER(precompile_assert_, __COUNTER__) = 1/(!!(e)) }
#endif /* __ghs__ */

#endif /* CT_ASSERT_H */
