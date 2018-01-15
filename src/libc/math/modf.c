/* $NetBSD: modf_ieee754.c,v 1.3 2010/01/27 14:10:41 drochner Exp $ */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

//#include <sys/types.h>
//#include <machine/ieee.h>
//#include <errno.h>
#include <math.h>
#include <stdint.h>

#define DBL_EXPBITS 11
#define DBL_FRACHBITS 20
#define DBL_FRACLBITS 32
#define DBL_FRACBITS (DBL_FRACHBITS + DBL_FRACLBITS)
#define DBL_EXP_INFNAN 2047
#define DBL_EXP_BIAS 1023
#define IEEE754_DOUBLE_BIAS DBL_EXP_BIAS

#define ieee_double_u ieee754_double
#define dblu_d d
#define dblu_dbl ieee
#define dbl_exp exponent
#define dbl_frach mantissa0
#define dbl_fracl mantissa1
#define dbl_sign negative

#define dblu_exp dblu_dbl.dbl_exp
#define dblu_sign dblu_dbl.dbl_sign
#define dblu_frach dblu_dbl.dbl_frach
#define dblu_fracl dblu_dbl.dbl_fracl


struct ieee754_float_double
{
    unsigned int mantissa1: DBL_FRACLBITS;
    unsigned int mantissa0: DBL_FRACHBITS;
    unsigned int exponent: DBL_EXPBITS;
    unsigned int negative: 1;
};

struct ieee754_float_double_nan
{
    unsigned int mantissa1: DBL_FRACLBITS;
    unsigned int mantissa0: DBL_FRACHBITS-1;
    unsigned int quiet_nan: 1;
    unsigned int exponent: DBL_EXPBITS;
    unsigned int negative: 1;
};

union ieee754_double
{
    double d;
    struct ieee754_float_double ieee;
    struct ieee754_float_double_nan ieee_nan;
};

#define ieee_double_u ieee754_double



/*
 * double modf(double val, double *iptr)
 * returns: f and i such that |f| < 1.0, (f + i) = val, and
 *  sign(f) == sign(i) == sign(val).
 *
 * Beware signedness when doing subtraction, and also operand size!
 */
double
modf(double val, double *iptr)
{
    union ieee_double_u u, v;
    uint64_t frac;

    /*
     * If input is +/-Inf or NaN, return +/-0 or NaN.
     */
    u.dblu_d = val;
    if (u.dblu_dbl.dbl_exp == DBL_EXP_INFNAN) {
        *iptr = u.dblu_d;
        return (0.0 / u.dblu_d);
    }

    /*
     * If input can't have a fractional part, return
     * (appropriately signed) zero, and make i be the input.
     */
    if ((int)u.dblu_dbl.dbl_exp - DBL_EXP_BIAS > DBL_FRACBITS - 1) {
        *iptr = u.dblu_d;
        v.dblu_d = 0.0;
        v.dblu_dbl.dbl_sign = u.dblu_dbl.dbl_sign;
        return (v.dblu_d);
    }

    /*
     * If |input| < 1.0, return it, and set i to the appropriately
     * signed zero.
     */
    if (u.dblu_dbl.dbl_exp < DBL_EXP_BIAS) {
        v.dblu_d = 0.0;
        v.dblu_dbl.dbl_sign = u.dblu_dbl.dbl_sign;
        *iptr = v.dblu_d;
        return (u.dblu_d);
    }

    /*
     * There can be a fractional part of the input.
     * If you look at the math involved for a few seconds, it's
     * plain to see that the integral part is the input, with the
     * low (DBL_FRACBITS - (exponent - DBL_EXP_BIAS)) bits zeroed,
     * the fractional part is the part with the rest of the
     * bits zeroed.  Just zeroing the high bits to get the
     * fractional part would yield a fraction in need of
     * normalization.  Therefore, we take the easy way out, and
     * just use subtraction to get the fractional part.
     */
    v.dblu_d = u.dblu_d;
    /* Zero the low bits of the fraction, the sleazy way. */
    frac = ((uint64_t)v.dblu_dbl.dbl_frach << 32) + v.dblu_dbl.dbl_fracl;
    frac >>= DBL_FRACBITS - (u.dblu_dbl.dbl_exp - DBL_EXP_BIAS);
    frac <<= DBL_FRACBITS - (u.dblu_dbl.dbl_exp - DBL_EXP_BIAS);
    v.dblu_dbl.dbl_fracl = frac & 0xffffffff;
    v.dblu_dbl.dbl_frach = frac >> 32;
    *iptr = v.dblu_d;

    u.dblu_d -= v.dblu_d;
    u.dblu_dbl.dbl_sign = v.dblu_dbl.dbl_sign;
    return (u.dblu_d);
}
