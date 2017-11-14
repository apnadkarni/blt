/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltMath.h --
 *
 * Copyright 2015 George A. Howlett. All rights reserved.  
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1) Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2) Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the
 *      distribution.
 *   3) Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *   4) Products derived from this software may not be called "BLT" nor may
 *      "BLT" appear in their names without specific prior written
 *      permission from the author.
 *
 *   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _BLT_MATH_H
#define _BLT_MATH_H

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <math.h>

#ifdef HAVE_FLOAT_H
#include <float.h>
#endif /* HAVE_FLOAT_H */

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif /* HAVE_IEEEFP_H */

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif /* M_PI */

#ifndef M_PI_2
#define M_PI_2          1.57079632679489661923
#endif /* M_PI_2 */

#ifndef M_SQRT2
#define M_SQRT2         1.41421356237309504880
#endif /* M_SQRT2 */

#ifndef M_SQRT1_2
#define M_SQRT1_2       0.70710678118654752440
#endif /* M_SQRT1_2 */

#ifndef SHRT_MAX
#define SHRT_MAX        0x7FFF
#endif /* SHRT_MAX */

#ifndef SHRT_MIN
#define SHRT_MIN        -(SHRT_MAX)
#endif /* SHRT_MAX */

#ifndef USHRT_MAX
#define USHRT_MAX       0xFFFF
#endif /* USHRT_MAX */

#ifndef INT_MAX
#define INT_MAX         2147483647
#endif /* INT_MAX */

#ifndef HAVE_FLOAT_H
/*
 *---------------------------------------------------------------------------
 *
 * DBL_MIN, DBL_MAX --
 *
 *      DBL_MAX and DBL_MIN are the largest and smaller double
 *      precision numbers that can be represented by the floating
 *      point hardware. If the compiler is ANSI, they can be found in
 *      float.h.  Otherwise, we use HUGE_VAL or HUGE to determine
 *      them.
 *
 *---------------------------------------------------------------------------
 */
/*
 * Don't want to include __infinity (definition of HUGE_VAL (SC1.x))
 */
#ifdef sun
#define DBL_MAX         1.7976931348623157E+308
#define DBL_MIN         2.2250738585072014E-308
#define DBL_EPSILON     2.2204460492503131e-16
#else
#ifndef DBL_EPSILON
#define DBL_EPSILON     BLT_DBL_EPSILON
#endif
#ifdef HUGE_VAL
#define DBL_MAX         HUGE_VAL
#define DBL_MIN         (1/HUGE_VAL)
#else
#ifdef HUGE
#define DBL_MAX         HUGE
#define DBL_MIN         (1/HUGE)
#else
/*
 * Punt: Assume relatively small values
 */
#define DBL_MAX         3.40282347E+38
#define DBL_MIN         1.17549435E-38
#endif /*HUGE*/
#endif /*HUGE_VAL*/
#endif /*sun*/
#endif /*!HAVE_FLOAT_H*/

/* #define DEG2RAD      (M_PI / 180.0) */
#define DEG2RAD         0.017453292519943295
#define RAD2DEG         57.29577951308232

/*
 *---------------------------------------------------------------------------
 *
 *      The following are macros replacing math library functions:
 *      "fabs", "fmod", "abs", "rint", and "exp10".
 *
 *      Although many of these routines may exist in your math
 *      library, they aren't used in libtcl.a or libtk.a.  This makes
 *      it difficult to dynamically load the BLT library as a shared
 *      object unless the math library is also shared (which isn't
 *      true on several systems).  We can avoid the problem by
 *      replacing the "exotic" math routines with macros.
 *
 *---------------------------------------------------------------------------
 */
#undef ABS
#define ABS(x)          (((x)<0)?(-(x)):(x))

#undef EXP10
#define EXP10(x)        (pow(10.0,(x)))

#undef FABS
#define FABS(x)         (((x)<0.0)?(-(x)):(x))

#undef SIGN
#define SIGN(x)         (((x) < 0.0) ? -1 : 1)

#undef FMOD
#define FMOD(x,y)       ((x)-(((int64_t)((x)/(y)))*(y)))

#undef ROUND
#define ROUND(x)        (((int64_t)((x) + 1.0)) - 1)

#ifdef HAVE_FINITE
#define FINITE(x)       (finite(x))
#else
#ifdef HAVE_ISFINITE
#define FINITE(x)       (isfinite(x))
#else
#ifdef HAVE_ISNAN
#define FINITE(x)       (!isnan(x))
#else
#define FINITE(x)       (TRUE)
#endif /* HAVE_ISNAN */
#endif /* HAVE_ISFINITE */
#endif /* HAVE_FINITE */

#define DEFINED(x)      (!isnan(x))
#define UNDEFINED(x)    (isnan(x))

#if !HAVE_DECL_DRAND48
BLT_EXTERN double drand48(void);
#endif /* !HAVE_DECL_DRAND48 */

#if !HAVE_DECL_SRAND48
BLT_EXTERN void srand48(long seed);
#endif /* !HAVE_DECL_SRAND48 */

#if !HAVE_DECL_J1
extern double j1(double x);
#endif /* !HAVE_DECL_J1 */

#if !HAVE_DECL_HYPOT
extern double hypot(double x, double y);
#endif /* !HAVE_DECL_HYPOT */

#ifdef HAVE_ISNAN
#if !HAVE_DECL_ISNAN
extern int isnan(double x);
#endif /* !HAVE_DECL_ISNAN */
#endif /* HAVE_ISNAN */

#ifdef HAVE_FINITE
#if !HAVE_DECL_FINITE
BLT_EXTERN int finite(double x);
#endif /* !HAVE_DECL_FINITE */
#endif /* HAVE_FINITE */

typedef struct {
    unsigned int value;
} Blt_Random;

#endif /* BLT_MATH_H */

