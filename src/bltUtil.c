/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltUtil.c --
 *
 * This module implements utility procedures for the BLT toolkit.
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
 * The routines for testing equality of real numbers are from Google.
 *
 * Copyright 2005, Google Inc.  All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *   * Neither the name of Google Inc. nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *   Authors: Zhanyong Wan, Sean Mcafee
 *
 *   Taken from The Google C++ Testing Framework (Google Test).
 *   Modified for this discussion by Fred Richards.
 *
 */

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"
#define _XOPEN_SOURCE 500      /* See feature_test_macros(7) */
#include <stdio.h>

#ifdef HAVE_STDARG_H
  #include <stdarg.h>
#endif /* HAVE_STDARG_H */

#ifdef HAVE_TIME_H
  #include <time.h>
#endif  /* HAVE_TIME_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_ERRNO_H
  #include <errno.h>
#endif /* HAVE_ERRNO_H */

#include "bltMath.h"
#include "bltString.h"
#include <bltHash.h>
#include <bltDBuffer.h>
#include "bltOp.h"

#if !HAVE_DECL_VSNPRINTF
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#endif

#ifdef WIN32

#include <shlwapi.h>

char *
strcasestr(const char *s, const char *find)
{
    return StrStrIA(s, find);
}
#endif  /* WIN32 */

#ifndef WIN32
#include <errno.h>

const char *
Blt_LastError(void)
{
    return strerror(errno);
}

int
Blt_GetPlatformId(void)
{
    return 0;
}

#endif

void
Blt_LowerCase(char *s)
{
    while (*s != '\0') {
        *s = tolower(UCHAR(*s));
        s++;
    }
}

void
Blt_UpperCase(char *s)
{
    while (*s != '\0') {
        *s = toupper(UCHAR(*s));
        s++;
    }
}


#ifndef HAVE_STRCASECMP
static unsigned char caseTable[] =
{
    (unsigned char)'\000', (unsigned char)'\001', 
    (unsigned char)'\002', (unsigned char)'\003', 
    (unsigned char)'\004', (unsigned char)'\005', 
    (unsigned char)'\006', (unsigned char)'\007',
    (unsigned char)'\010', (unsigned char)'\011', 
    (unsigned char)'\012', (unsigned char)'\013', 
    (unsigned char)'\014', (unsigned char)'\015', 
    (unsigned char)'\016', (unsigned char)'\017',
    (unsigned char)'\020', (unsigned char)'\021', 
    (unsigned char)'\022', (unsigned char)'\023', 
    (unsigned char)'\024', (unsigned char)'\025', 
    (unsigned char)'\026', (unsigned char)'\027',
    (unsigned char)'\030', (unsigned char)'\031', 
    (unsigned char)'\032', (unsigned char)'\033', 
    (unsigned char)'\034', (unsigned char)'\035', 
    (unsigned char)'\036', (unsigned char)'\037',
    (unsigned char)'\040', (unsigned char)'\041', 
    (unsigned char)'\042', (unsigned char)'\043', 
    (unsigned char)'\044', (unsigned char)'\045', 
    (unsigned char)'\046', (unsigned char)'\047',
    (unsigned char)'\050', (unsigned char)'\051', 
    (unsigned char)'\052', (unsigned char)'\053', 
    (unsigned char)'\054', (unsigned char)'\055', 
    (unsigned char)'\056', (unsigned char)'\057',
    (unsigned char)'\060', (unsigned char)'\061', 
    (unsigned char)'\062', (unsigned char)'\063', 
    (unsigned char)'\064', (unsigned char)'\065', 
    (unsigned char)'\066', (unsigned char)'\067',
    (unsigned char)'\070', (unsigned char)'\071', 
    (unsigned char)'\072', (unsigned char)'\073', 
    (unsigned char)'\074', (unsigned char)'\075', 
    (unsigned char)'\076', (unsigned char)'\077',
    (unsigned char)'\100', (unsigned char)'\141', 
    (unsigned char)'\142', (unsigned char)'\143', 
    (unsigned char)'\144', (unsigned char)'\145', 
    (unsigned char)'\146', (unsigned char)'\147',
    (unsigned char)'\150', (unsigned char)'\151', 
    (unsigned char)'\152', (unsigned char)'\153', 
    (unsigned char)'\154', (unsigned char)'\155', 
    (unsigned char)'\156', (unsigned char)'\157',
    (unsigned char)'\160', (unsigned char)'\161', 
    (unsigned char)'\162', (unsigned char)'\163', 
    (unsigned char)'\164', (unsigned char)'\165', 
    (unsigned char)'\166', (unsigned char)'\167',
    (unsigned char)'\170', (unsigned char)'\171', 
    (unsigned char)'\172', (unsigned char)'\133', 
    (unsigned char)'\134', (unsigned char)'\135', 
    (unsigned char)'\136', (unsigned char)'\137',
    (unsigned char)'\140', (unsigned char)'\141', 
    (unsigned char)'\142', (unsigned char)'\143', 
    (unsigned char)'\144', (unsigned char)'\145', 
    (unsigned char)'\146', (unsigned char)'\147',
    (unsigned char)'\150', (unsigned char)'\151', 
    (unsigned char)'\152', (unsigned char)'\153', 
    (unsigned char)'\154', (unsigned char)'\155', 
    (unsigned char)'\156', (unsigned char)'\157',
    (unsigned char)'\160', (unsigned char)'\161',
    (unsigned char)'\162', (unsigned char)'\163', 
    (unsigned char)'\164', (unsigned char)'\165', 
    (unsigned char)'\166', (unsigned char)'\167',
    (unsigned char)'\170', (unsigned char)'\171', 
    (unsigned char)'\172', (unsigned char)'\173', 
    (unsigned char)'\174', (unsigned char)'\175', 
    (unsigned char)'\176', (unsigned char)'\177',
    (unsigned char)'\200', (unsigned char)'\201', 
    (unsigned char)'\202', (unsigned char)'\203', 
    (unsigned char)'\204', (unsigned char)'\205', 
    (unsigned char)'\206', (unsigned char)'\207',
    (unsigned char)'\210', (unsigned char)'\211', 
    (unsigned char)'\212', (unsigned char)'\213', 
    (unsigned char)'\214', (unsigned char)'\215', 
    (unsigned char)'\216', (unsigned char)'\217',
    (unsigned char)'\220', (unsigned char)'\221', 
    (unsigned char)'\222', (unsigned char)'\223', 
    (unsigned char)'\224', (unsigned char)'\225', 
    (unsigned char)'\226', (unsigned char)'\227',
    (unsigned char)'\230', (unsigned char)'\231', 
    (unsigned char)'\232', (unsigned char)'\233', 
    (unsigned char)'\234', (unsigned char)'\235', 
    (unsigned char)'\236', (unsigned char)'\237',
    (unsigned char)'\240', (unsigned char)'\241', 
    (unsigned char)'\242', (unsigned char)'\243', 
    (unsigned char)'\244', (unsigned char)'\245', 
    (unsigned char)'\246', (unsigned char)'\247',
    (unsigned char)'\250', (unsigned char)'\251', 
    (unsigned char)'\252', (unsigned char)'\253', 
    (unsigned char)'\254', (unsigned char)'\255', 
    (unsigned char)'\256', (unsigned char)'\257',
    (unsigned char)'\260', (unsigned char)'\261', 
    (unsigned char)'\262', (unsigned char)'\263', 
    (unsigned char)'\264', (unsigned char)'\265', 
    (unsigned char)'\266', (unsigned char)'\267',
    (unsigned char)'\270', (unsigned char)'\271', 
    (unsigned char)'\272', (unsigned char)'\273', 
    (unsigned char)'\274', (unsigned char)'\275', 
    (unsigned char)'\276', (unsigned char)'\277',
    (unsigned char)'\300', (unsigned char)'\341', 
    (unsigned char)'\342', (unsigned char)'\343', 
    (unsigned char)'\344', (unsigned char)'\345', 
    (unsigned char)'\346', (unsigned char)'\347',
    (unsigned char)'\350', (unsigned char)'\351', 
    (unsigned char)'\352', (unsigned char)'\353', 
    (unsigned char)'\354', (unsigned char)'\355', 
    (unsigned char)'\356', (unsigned char)'\357',
    (unsigned char)'\360', (unsigned char)'\361', 
    (unsigned char)'\362', (unsigned char)'\363', 
    (unsigned char)'\364', (unsigned char)'\365', 
    (unsigned char)'\366', (unsigned char)'\367',
    (unsigned char)'\370', (unsigned char)'\371', 
    (unsigned char)'\372', (unsigned char)'\333', 
    (unsigned char)'\334', (unsigned char)'\335', 
    (unsigned char)'\336', (unsigned char)'\337',
    (unsigned char)'\340', (unsigned char)'\341', 
    (unsigned char)'\342', (unsigned char)'\343', 
    (unsigned char)'\344', (unsigned char)'\345', 
    (unsigned char)'\346', (unsigned char)'\347',
    (unsigned char)'\350', (unsigned char)'\351', 
    (unsigned char)'\352', (unsigned char)'\353', 
    (unsigned char)'\354', (unsigned char)'\355', 
    (unsigned char)'\356', (unsigned char)'\357',
    (unsigned char)'\360', (unsigned char)'\361', 
    (unsigned char)'\362', (unsigned char)'\363', 
    (unsigned char)'\364', (unsigned char)'\365', 
    (unsigned char)'\366', (unsigned char)'\367',
    (unsigned char)'\370', (unsigned char)'\371', 
    (unsigned char)'\372', (unsigned char)'\373', 
    (unsigned char)'\374', (unsigned char)'\375', 
    (unsigned char)'\376', (unsigned char)'\377',
};

/*
 *---------------------------------------------------------------------------
 *
 * strcasecmp --
 *
 *      Compare two strings, disregarding case.
 *
 * Results:
 *      Returns a signed integer representing the following:
 *
 *      zero      - two strings are equal
 *      negative  - first string is less than second
 *      positive  - first string is greater than second
 *
 *---------------------------------------------------------------------------
 */
int
strcasecmp(const char *s1, const char *s2)
{
    unsigned char *s = (unsigned char *)s1;
    unsigned char *t = (unsigned char *)s2;

    for ( /* empty */ ; (caseTable[*s] == caseTable[*t]); s++, t++) {
        if (*s == '\0') {
            return 0;
        }
    }
    return (caseTable[*s] - caseTable[*t]);
}

/*
 *---------------------------------------------------------------------------
 *
 * strncasecmp --
 *
 *      Compare two strings, disregarding case, up to a given length.
 *
 * Results:
 *      Returns a signed integer representing the following:
 *
 *      zero      - two strings are equal
 *      negative  - first string is less than second
 *      positive  - first string is greater than second
 *
 *---------------------------------------------------------------------------
 */
int
strncasecmp(const char *s1, const char *s2, size_t length)
{
    unsigned char *s = (unsigned char *)s1;
    unsigned char *t = (unsigned char *)s2;

    for ( /* empty */ ; (length > 0); s++, t++, length--) {
        if (caseTable[*s] != caseTable[*t]) {
            return (caseTable[*s] - caseTable[*t]);
        }
        if (*s == '\0') {
            return 0;
        }
    }
    return 0;
}

#endif /* !HAVE_STRCASECMP */


#ifndef HAVE_DRAND48
#ifdef _MSC_VER
#define XDR48   0x000100010001
#define MNDR48  0x0005deece66d
#define DODDR48 0xb

static __int64 xdr48 = XDR48;
double 
drand48(void)             /* Works only on compilers with 64-bit long. */
{
    xdr48 = MNDR48 * xdr48 + DODDR48; 
    xdr48 &= 0xffffffffffff;
    return xdr48 / 281474976710656.0;
}
#else 
#define XDR48   0x000100010001LL
#define MNDR48  0x0005deece66dLL
#define DODDR48 0xbLL
static long long xdr48 = XDR48;
double 
drand48(void)             /* Works only on compilers with long long int! */
{
    xdr48 = MNDR48 * xdr48 + DODDR48; 
    xdr48 &= 0xffffffffffffLL;
    return xdr48 / 281474976710656.0;
}
#endif

/*
 * Random number generator initialization parameters:
 *
 *              s - seed
 * return:
 */
void 
srand48(long x0)
{
    xdr48 = ((unsigned long)x0<<16) + 0x330e; 
}
#endif /*HAVE_DRAND48*/

#if (_TCL_VERSION < _VERSION(8,1,0)) 

char *
Tcl_GetString(Tcl_Obj *objPtr)
{
    unsigned int dummy;

    return Tcl_GetStringFromObj(objPtr, &dummy);
}

int 
Tcl_EvalObjv(Tcl_Interp *interp, int objc, Tcl_Obj **objv, int flags)
{
    Tcl_DString ds;
    int i;
    int result;

    Tcl_DStringInit(&ds);
    for (i = 0; i < objc; i++) {
        Tcl_DStringAppendElement(&ds, Tcl_GetString(objv[i]));
    }
    result = Tcl_Eval(interp, Tcl_DStringValue(&ds)); 
    Tcl_DStringFree(&ds);
    return result;
}

int 
Tcl_WriteObj(Tcl_Channel channel, Tcl_Obj *objPtr)
{
    char *string;
    int numBytes;

    string = Tcl_GetStringFromObj(objPtr, &nBytes);
    return Tcl_Write(channel, string, numBytes);
}

char *
Tcl_SetVar2Ex(
    Tcl_Interp *interp, 
    char *part1, 
    char *part2, 
    Tcl_Obj *objPtr, 
    int flags)
{
    return Tcl_SetVar2(interp, part1, part2, Tcl_GetString(objPtr), flags);
}

Tcl_Obj *
Tcl_GetVar2Ex(
    Tcl_Interp *interp,
    char *part1, 
    char *part2,
    int flags)
{
    char *result;
    
    result = Tcl_GetVar2(interp, part1, part2, flags);
    if (result == NULL) {
        return NULL;
    }
    return Tcl_NewStringObj(result, -1);
}
#endif

int
Blt_ObjIsInteger(Tcl_Obj *objPtr)
{
    int64_t value;
    
    if (Blt_GetInt64FromObj(NULL, objPtr, &value) == TCL_OK) {
        return TRUE;
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CompareDictionary
 *
 *      This function compares two strings as if they were being used in an
 *      index or card catalog.  The case of alphabetic characters is
 *      ignored, except to break ties.  Thus "B" comes before "b" but after
 *      "a".  Also, integers embedded in the strings compare in numerical
 *      order.  In other words, "x10y" comes after "x9y", not before it as
 *      it would when using strcmp().
 *
 * Results:
 *      A negative result means that the first element comes before the
 *      second, and a positive result means that the second element should
 *      come first.  A result of zero means the two elements are equal and
 *      it doesn't matter which comes first.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

#if HAVE_UTF
int
Blt_DictionaryCompare(const char *left, const char *right)
{
    Tcl_UniChar uniLeft, uniRight, uniLeftLower, uniRightLower;
    int diff, zeros;
    int secondaryDiff = 0;

    for(;;) {
        if ((isdigit(UCHAR(*right))) && (isdigit(UCHAR(*left)))) { 
            /*
             * There are decimal numbers embedded in the two strings.
             * Compare them as numbers, rather than strings.  If one number
             * has more leading zeros than the other, the number with more
             * leading zeros sorts later, but only as a secondary choice.
             */

            zeros = 0;
            while ((*right == '0') && (isdigit(UCHAR(right[1])))) {
                right++;
                zeros--;
            }
            while ((*left == '0') && (isdigit(UCHAR(left[1])))) {
                left++;
                zeros++;
            }
            if (secondaryDiff == 0) {
                secondaryDiff = zeros;
            }

            /*
             * The code below compares the numbers in the two strings
             * without ever converting them to integers.  It does this by
             * first comparing the lengths of the numbers and then
             * comparing the digit values.
             */

            diff = 0;
            for (;;) {
                if (diff == 0) {
                    diff = UCHAR(*left) - UCHAR(*right);
                }
                right++;
                left++;

                /* Ignore commas in numbers. */
                if (*left == ',') {
                    left++;
                }
                if (*right == ',') {
                    right++;
                }

                if (!isdigit(UCHAR(*right))) { /* INTL: digit */
                    if (isdigit(UCHAR(*left))) { /* INTL: digit */
                        return 1;
                    } else {
                        /*
                         * The two numbers have the same length. See if
                         * their values are different.
                         */

                        if (diff != 0) {
                            return diff;
                        }
                        break;
                    }
                } else if (!isdigit(UCHAR(*left))) { /* INTL: digit */
                    return -1;
                }
            }
            continue;
        }

        /*
         * Convert character to Unicode for comparison purposes.  If either
         * string is at the terminating null, do a byte-wise comparison and
         * bail out immediately.
         */
        if ((*left != '\0') && (*right != '\0')) {
            left += Tcl_UtfToUniChar(left, &uniLeft);
            right += Tcl_UtfToUniChar(right, &uniRight);
            /*
             * Convert both chars to lower for the comparison, because
             * dictionary sorts are case insensitve.  Convert to lower, not
             * upper, so chars between Z and a will sort before A (where
             * most other interesting punctuations occur)
             */
            uniLeftLower = Tcl_UniCharToLower(uniLeft);
            uniRightLower = Tcl_UniCharToLower(uniRight);
        } else {
            diff = UCHAR(*left) - UCHAR(*right);
            break;
        }

        diff = uniLeftLower - uniRightLower;
        if (diff) {
            return diff;
        } else if (secondaryDiff == 0) {
            if (Tcl_UniCharIsUpper(uniLeft) &&
                    Tcl_UniCharIsLower(uniRight)) {
                secondaryDiff = -1;
            } else if (Tcl_UniCharIsUpper(uniRight)
                    && Tcl_UniCharIsLower(uniLeft)) {
                secondaryDiff = 1;
            }
        }
    }
    if (diff == 0) {
        diff = secondaryDiff;
    }
    return diff;
}

#else 

int
Blt_DictionaryCompare(const char *left, const char *right) 
{
    int diff, zeros;
    int secondaryDiff = 0;

    while (1) {
        if (isdigit(UCHAR(*right)) && isdigit(UCHAR(*left))) {
            /*
             * There are decimal numbers embedded in the two strings.
             * Compare them as numbers, rather than strings.  If one number
             * has more leading zeros than the other, the number with more
             * leading zeros sorts later, but only as a secondary choice.
             */
            zeros = 0;
            while ((*right == '0') && (isdigit(UCHAR(right[1])))) {
                right++;
                zeros--;
            }
            while ((*left == '0') && (isdigit(UCHAR(left[1])))) {
                left++;
                zeros++;
            }
            if (secondaryDiff == 0) {
                secondaryDiff = zeros;
            }

            /*
             * The code below compares the numbers in the two strings
             * without ever converting them to integers.  It does this by
             * first comparing the lengths of the numbers and then
             * comparing the digit values.
             */

            diff = 0;
            while (1) {
                if (diff == 0) {
                    diff = UCHAR(*left) - UCHAR(*right);
                }
                right++;
                left++;
                /* Ignore commas in numbers. */
                if (*left == ',') {
                    left++;
                }
                if (*right == ',') {
                    right++;
                }
                if (!isdigit(UCHAR(*right))) {
                    if (isdigit(UCHAR(*left))) {
                        return 1;
                    } else {
                        /*
                         * The two numbers have the same length. See if
                         * their values are different.
                         */

                        if (diff != 0) {
                            return diff;
                        }
                        break;
                    }
                } else if (!isdigit(UCHAR(*left))) {
                    return -1;
                }
            }
            continue;
        }
        diff = UCHAR(*left) - UCHAR(*right);
        if (diff) {
            if (isupper(UCHAR(*left)) && islower(UCHAR(*right))) {
                diff = UCHAR(tolower(*left)) - UCHAR(*right);
                if (diff) {
                    return diff;
                } else if (secondaryDiff == 0) {
                    secondaryDiff = -1;
                }
            } else if (isupper(UCHAR(*right)) && islower(UCHAR(*left))) {
                diff = UCHAR(*left) - UCHAR(tolower(UCHAR(*right)));
                if (diff) {
                    return diff;
                } else if (secondaryDiff == 0) {
                    secondaryDiff = 1;
                }
            } else {
                return diff;
            }
        }
        if (*left == 0) {
            break;
        }
        left++;
        right++;
    }
    if (diff == 0) {
        diff = secondaryDiff;
    }
    return diff;
}
#endif

/*
 *  Note - this function is needed even when NDEBUG is defined as it
 * is referenced from the BLT stubs table.
 */
void
Blt_Assert(const char *testExpr, const char *fileName, int lineNumber)
{
    fprintf(stderr, "line %d of %s: Assert \"%s\" failed\n",
        lineNumber, fileName, testExpr);
    fflush(stderr);
    abort();
}

/*ARGSUSED*/
void
Blt_Panic(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(args);
    abort();
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Warn --
 *
 *      Display a message and exit.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Exits the program.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Warn(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
#ifdef WIN32
    {
        char buf[1024];

        vsnprintf(buf, 1024, fmt, args);
        buf[1023] = '\0';
        MessageBeep(MB_ICONEXCLAMATION);
        MessageBoxA(NULL, buf, "Warning from BLT",
                   MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
    }
#else
    fprintf(stderr, "BLT Warning: ");
    vfprintf(stderr, fmt, args);
#endif /* WIN32 */
    va_end(args);
}

void 
Blt_DStringAppendElements(Tcl_DString *dsPtr, ...)
{
    const char *elem;
    va_list args;

    va_start(args, dsPtr);
    while ((elem = va_arg(args, const char *)) != NULL) {
        Tcl_DStringAppendElement(dsPtr, elem);
    }
    va_end(args);
}

/*ARGSUSED*/
int 
Blt_FmtString(char *s, size_t size, const char *fmt, /*args*/ ...) 
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(s, size, fmt, ap);
    if ((n != (int)size) && (size > 0)) {
        s[size-1] = '\0';
    }
    va_end(ap);
    return n;
}

static char stringRep[200];

const char *
Blt_Itoa(int value)
{
    Blt_FmtString(stringRep, 200, "%d", value);
    return stringRep;
}

const char *
Blt_Ltoa(long value)
{
    Blt_FmtString(stringRep, 200, "%ld", value);
    return stringRep;
}

const char *
Blt_Utoa(unsigned int value)
{
    Blt_FmtString(stringRep, 200, "%u", value);
    return stringRep;
}

const char *
Blt_Dtoa(Tcl_Interp *interp, double value)
{
    Tcl_PrintDouble(interp, value, stringRep);
    return stringRep;
}

FILE *
Blt_OpenFile(Tcl_Interp *interp, const char *fileName, const char *mode)
{
    FILE *f;
#if (_TCL_VERSION >= _VERSION(8,1,0)) 
    Tcl_DString ds, nativeds;
    char *native;

    native = Tcl_TranslateFileName(interp, fileName, &nativeds);
    if (native == NULL) {
        return NULL;
    }
    fileName = Tcl_UtfToExternalDString(NULL, native, -1, &ds);
    if (fileName == NULL) {
        Tcl_AppendResult(interp, "can't convert filename \"", native, 
                 "\" to system encoding", (char *)NULL);
        Tcl_DStringFree(&nativeds);
        return NULL;
    }
    f = fopen(fileName, mode);
#else
    f = fopen(fileName, mode);
#endif
    if (f == NULL) {
        Tcl_AppendResult(interp, "can't open \"", fileName, "\": ",
                         Tcl_PosixError(interp), (char *)NULL);
    }
    Tcl_DStringFree(&ds);
    Tcl_DStringFree(&nativeds);
    return f;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_InitHexTable --
 *
 *      Table index for the hex values. Initialized once, first time.  Used
 *      for translation value or delimiter significance lookup.
 *
 *      We build the table at run time for several reasons:
 *
 *        1.  portable to non-ASCII machines.
 *        2.  still reentrant since we set the init flag after setting
 *            table.
 *        3.  easier to extend.
 *        4.  less prone to bugs.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_InitHexTable(unsigned char *hexTable)
{
    memset(hexTable, 0xFF, 256);
    hexTable['0'] = 0;
    hexTable['1'] = 1;
    hexTable['2'] = 2;
    hexTable['3'] = 3;
    hexTable['4'] = 4;
    hexTable['5'] = 5;
    hexTable['6'] = 6;
    hexTable['7'] = 7;
    hexTable['8'] = 8;
    hexTable['9'] = 9;
    hexTable['a'] = hexTable['A'] = 10;
    hexTable['b'] = hexTable['B'] = 11;
    hexTable['c'] = hexTable['C'] = 12;
    hexTable['d'] = hexTable['D'] = 13;
    hexTable['e'] = hexTable['E'] = 14;
    hexTable['f'] = hexTable['F'] = 15;
}


#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPosition --
 *
 *      Convert a string representing a numeric position.  A position can
 *      be in one of the following forms.
 *
 *        number        - number of the item in the hierarchy, indexed
 *                        from zero.
 *        "end"         - last position in the hierarchy.
 *
 * Results:
 *      A standard TCL result.  If "string" is a valid index, then
 *      *indexPtr is filled with the corresponding numeric index.  If "end"
 *      was selected then *indexPtr is set to -1.  Otherwise an error
 *      message is left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPosition(
    Tcl_Interp *interp,                 /* Interpreter to report results
                                         * back to. */
    const char *string,                 /* String representation of the
                                         * index.  Can be an integer or
                                         * "end" to refer to the last
                                         * index. */
    size_t *indexPtr)                   /* Holds the converted index. */
{
    if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
        *indexPtr = -1;                 /* Indicates last position in
                                         * hierarchy. */
    } else {
        int64_t position;

        if (Blt_GetInt64(interp, string, &position) != TCL_OK) {
            return TCL_ERROR;
        }
        if (position < 0) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "bad position \"", string, "\"",
                                 (char *)NULL);
            }
            return TCL_ERROR;
        }
        *indexPtr = (size_t)position;
    }
    return TCL_OK;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPositionFromObj --
 *
 *      Convert a string representing a numeric position.  A position can
 *      be in one of the following forms.
 *
 *        number        - number of the item in the hierarchy, indexed
 *                        from zero.
 *        "end"         - last position in the hierarchy.
 *
 * Results:
 *      A standard TCL result.  If "string" is a valid index, then
 *      *indexPtr is filled with the corresponding numeric index.  If "end"
 *      was selected then *indexPtr is set to -1.  Otherwise an error
 *      message is left in interp->result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPositionFromObj(
    Tcl_Interp *interp,                 /* Interpreter to report results
                                         * back to. */
    Tcl_Obj *objPtr,                    /* Tcl_Obj representation of the
                                         * index.  Can be an integer or
                                         * "end" to refer to the last
                                         * index. */
    long *indexPtr)                     /* Holds the converted index. */
{
    const char *string;

    string = Tcl_GetString(objPtr);
    if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
        *indexPtr = -1;                 /* Indicates last position in
                                         * hierarchy. */
    } else {
        int64_t position;

        if (Blt_GetInt64FromObj(interp, objPtr, &position) != TCL_OK) {
            return TCL_ERROR;
        }
        if (position < 0) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "bad position \"", string, "\"",
                                 (char *)NULL);
            }
            return TCL_ERROR;
        }
        *indexPtr = position;
    }
    return TCL_OK;
}

/*
 * The hash table below is used to keep track of all the Blt_Uids created
 * so far.
 */
static Blt_HashTable uidTable;
static int uidInitialized = 0;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetUid --
 *
 *      Given a string, returns a unique identifier for the string.  A
 *      reference count is maintained, so that the identifier can be freed
 *      when it is not needed any more. This can be used in many places to
 *      replace Tcl_GetUid.
 *
 * Results:
 *      This procedure returns a Blt_Uid corresponding to the "string"
 *      argument.  The Blt_Uid has a string value identical to string
 *      (strcmp will return 0), but it's guaranteed that any other calls to
 *      this procedure with a string equal to "string" will return exactly
 *      the same result (i.e. can compare Blt_Uid *values* directly,
 *      without having to call strcmp on what they point to).
 *
 * Side effects:
 *      New information may be entered into the identifier table.
 *
 *---------------------------------------------------------------------------
 */
Blt_Uid
Blt_GetUid(const char *string)          /* String to convert. */
{
    Blt_HashEntry *hPtr;
    int isNew;
    size_t refCount;
    
    if (!uidInitialized) {
        Blt_InitHashTable(&uidTable, BLT_STRING_KEYS);
        uidInitialized = 1;
    }
    hPtr = Blt_CreateHashEntry(&uidTable, string, &isNew);
    if (isNew) {
        refCount = 0;
    } else {
        refCount = (size_t)Blt_GetHashValue(hPtr);
    }
    refCount++;
    Blt_SetHashValue(hPtr, (ClientData)refCount);
    return (Blt_Uid)Blt_GetHashKey(&uidTable, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeUid --
 *
 *      Frees the Blt_Uid if there are no more clients using this identifier.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The identifier may be deleted from the identifier table.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreeUid(Blt_Uid uid)                /* Identifier to release. */
{
    Blt_HashEntry *hPtr;

    if (!uidInitialized) {
        Blt_InitHashTable(&uidTable, BLT_STRING_KEYS);
        uidInitialized = 1;
    }
    hPtr = Blt_FindHashEntry(&uidTable, uid);
    if (hPtr) {
        size_t refCount;

        refCount = (size_t)Blt_GetHashValue(hPtr);
        refCount--;
        if (refCount == 0) {
            Blt_DeleteHashEntry(&uidTable, hPtr);
        } else {
            Blt_SetHashValue(hPtr, refCount);
        }
    } else {
        Blt_Warn("tried to release unknown identifier \"%s\"\n", uid);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FindUid --
 *
 *      Returns a Blt_Uid associated with a given string, if one exists.
 *
 * Results:
 *      A Blt_Uid for the string if one exists. Otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
Blt_Uid
Blt_FindUid(const char *string)         /* String to find. */
{
    Blt_HashEntry *hPtr;

    if (!uidInitialized) {
        Blt_InitHashTable(&uidTable, BLT_STRING_KEYS);
        uidInitialized = 1;
    }
    hPtr = Blt_FindHashEntry(&uidTable, string);
    if (hPtr == NULL) {
        return NULL;
    }
    return (Blt_Uid) Blt_GetHashKey(&uidTable, hPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * LinearOpSearch --
 *
 *      Performs a linear search on the array of command operation
 *      specifications to find a partial, anchored match for the given
 *      operation string.
 *
 * Results:
 *      If the string matches unambiguously the index of the specification
 *      in the array is returned.  If the string does not match, even as an
 *      abbreviation, any operation, -1 is returned.  If the string
 *      matches, but ambiguously -2 is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
LinearOpSearch(Blt_OpSpec *specs, int low, int high, const char *string, 
               int length)
{
    Blt_OpSpec *specPtr;
    char c;
    int numMatches, last;
    int i;

    c = string[0];
    numMatches = 0;
    last = -1;
    for (specPtr = specs+low, i = low; i <= high; i++, specPtr++) {
        if ((c == specPtr->name[0]) && 
            (strncmp(string, specPtr->name, length) == 0)) {
            last = i;
            numMatches++;
            if (length == specPtr->minChars) {
                break;
            }
        }
    }
    if (numMatches > 1) {
        return -2;                      /* Ambiguous operation name */
    } 
    if (numMatches == 0) {
        return -1;                      /* Can't find operation */
    } 
    return last;                        /* Op found. */
}

/*
 *---------------------------------------------------------------------------
 *
 * BinaryOpSearch --
 *
 *      Performs a binary search on the array of command operation
 *      specifications to find a partial, anchored match for the given
 *      operation string.  If we get to a point where the sample string
 *      partially matches an entry, we then revert to a linear search over
 *      the given range to check if it's an exact match or ambiguous
 *      (example: "next" with commands next and nextsibling).
 *
 * Results:
 *      If the string matches unambiguously the index of the specification
 *      in the array is returned.  If the string does not match, even as an
 *      abbreviation, any operation, -1 is returned.  If the string
 *      matches, but ambiguously -2 is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
BinaryOpSearch(Blt_OpSpec *specs, int low, int high, const char *string, 
               int length)
{
    char c;

    c = string[0];
    while (low <= high) {
        Blt_OpSpec *specPtr;
        int compare;
        int median;
        
        median = (low + high) >> 1;
        specPtr = specs + median;

        /* Test the first character */
        compare = c - specPtr->name[0];
        if (compare == 0) {
            /* Now test the entire string */
            compare = strncmp(string, specPtr->name, length);
        }
        if (compare < 0) {
            high = median - 1;
        } else if (compare > 0) {
            low = median + 1;
        } else {
            if (length < specPtr->minChars) {
                /* Verify that the string is either ambiguous or an exact
                 * match of another command by doing a linear search over
                 * the given interval. */
                return LinearOpSearch(specs, low, high, string, length);        
            }
            return median;              /* Op found. */
        }
    }
    return -1;                          /* Can't find operation */
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetOpFromObj --
 *
 *      Find the command operation given a string name.  This is useful
 *      where a group of command operations have the same argument
 *      signature.
 *
 * Results:
 *      If found, a pointer to the procedure (function pointer) is
 *      returned.  Otherwise NULL is returned and an error message
 *      containing a list of the possible commands is returned in
 *      interp->result.
 *
 *---------------------------------------------------------------------------
 */
void *
Blt_GetOpFromObj(
    Tcl_Interp *interp,                 /* Interpreter to report errors
                                         * to */
    int numSpecs,                       /* # of specifications in array */
    Blt_OpSpec *specs,                  /* Op specification array */
    int operPos,                        /* Position of operation in
                                         * argument list. */
    int objc,                           /* # of arguments in the argument *
                                         * vector.  This includes any
                                         * Prefixed arguments */
    Tcl_Obj *const *objv,               /* Argument vector */
    int flags)
{
    Blt_OpSpec *specPtr;
    const char *string;
    int length;
    int n;

    if (objc <= operPos) {              /* No operation argument */
        if (interp != NULL) {
            Tcl_AppendResult(interp, "wrong # args: ", (char *)NULL);
        usage:
            Tcl_AppendResult(interp, "should be one of...", (char *)NULL);
            for (n = 0; n < numSpecs; n++) {
                int i;
                
                Tcl_AppendResult(interp, "\n  ", (char *)NULL);
                for (i = 0; i < operPos; i++) {
                    Tcl_AppendResult(interp, Tcl_GetString(objv[i]), " ", 
                                     (char *)NULL);
                }
                specPtr = specs + n;
                Tcl_AppendResult(interp, specPtr->name, " ", specPtr->usage,
                                 (char *)NULL);
            }
        }
        return NULL;
    }
    string = Tcl_GetStringFromObj(objv[operPos], &length);
    if (flags & BLT_OP_LINEAR_SEARCH) {
        n = LinearOpSearch(specs, 0, numSpecs - 1, string, length);
    } else {
        n = BinaryOpSearch(specs, 0, numSpecs - 1, string, length);
    }
    if (n == -2) {
        if (interp != NULL) {
            char c;

            Tcl_AppendResult(interp, "ambiguous", (char *)NULL);
            if (operPos > 2) {
                Tcl_AppendResult(interp, " ", Tcl_GetString(objv[operPos - 1]), 
                                 (char *)NULL);
            }
            Tcl_AppendResult(interp, " operation \"", string, "\" matches: ",
                             (char *)NULL);
            
            c = string[0];
            for (n = 0; n < numSpecs; n++) {
                specPtr = specs + n;
                if ((c == specPtr->name[0]) &&
                    (strncmp(string, specPtr->name, length) == 0)) {
                    Tcl_AppendResult(interp, " ", specPtr->name, (char *)NULL);
                }
            }
        }
        return NULL;

    } else if (n == -1) {               /* Can't find operation, display
                                         * help */
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad", (char *)NULL);
            if (operPos > 2) {
                Tcl_AppendResult(interp, " ", Tcl_GetString(objv[operPos - 1]), 
                                 (char *)NULL);
            }
            Tcl_AppendResult(interp, " operation \"", string, "\": ", 
                             (char *)NULL);
            goto usage;
        }
        return NULL;
    }
    specPtr = specs + n;
    if ((objc < specPtr->minArgs) || 
        ((specPtr->maxArgs > 0) && (objc > specPtr->maxArgs))) {
        if (interp != NULL) {
            int i;

            Tcl_AppendResult(interp, "wrong # args: should be \"",(char *)NULL);
            for (i = 0; i < operPos; i++) {
                Tcl_AppendResult(interp, Tcl_GetString(objv[i]), " ", 
                                 (char *)NULL);
            }
            Tcl_AppendResult(interp, specPtr->name, " ", specPtr->usage, "\"",
                             (char *)NULL);
        }
        return NULL;
    }
    return specPtr->proc;
}

#if (_TCL_VERSION >= _VERSION(8,4,0)) 
/*ARGSUSED*/
int
Blt_LoadLibrary(Tcl_Interp *interp, const char *libPath, 
                const char *initProcName, const char *safeProcName)
{
    Tcl_FSUnloadFileProc *unLoadProcPtr = NULL;
    Tcl_LoadHandle loadHandle;
    Tcl_PackageInitProc *initProc, *safeProc;
    int result;
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewStringObj(libPath, -1);
    Tcl_IncrRefCount(objPtr);
    result = Tcl_FSLoadFile(interp, objPtr, initProcName, safeProcName, 
        &initProc, &safeProc, &loadHandle, &unLoadProcPtr);
    if (result != TCL_OK) {
        goto done;
    }
    if (initProc == NULL) {
        Tcl_AppendResult(interp, "couldn't find procedure ", initProcName, 
                (char *) NULL);
        result = TCL_ERROR;
        goto done;
    }
    if (Tcl_IsSafe(interp)) {
        if (safeProc == NULL) {
            Tcl_AppendResult(interp, 
                "can't use package in a safe interpreter: no ", safeProcName, 
                " procedure", (char *) NULL);
            result = TCL_ERROR;
            goto done;
        }
        result = (*safeProc)(interp);
    } else {
        result = (*initProc)(interp);
    }
 done:
    Tcl_DecrRefCount(objPtr);
    if (result != TCL_OK) {
        if (unLoadProcPtr != NULL) {
            (*unLoadProcPtr)(loadHandle);
        }
        return TCL_ERROR;
    }   
    return TCL_OK;
}
#else 
int
Blt_LoadLibrary(Tcl_Interp *interp, const char *libPath, 
                const char *initProcName, const char *safeProcName)
{
    ClientData loadData;
    Tcl_PackageInitProc *initProc, *safeProc;
    int result;

    result = TclpLoadFile(interp, libPath, initProcName, safeProcName, 
        &initProc, &safeProc, &loadData); 
    if (result != TCL_OK) {
        return TCL_ERROR;
    }
    if (initProc == NULL) {
        Tcl_AppendResult(interp, "couldn't find procedure ", initProcName, 
                (char *) NULL);
        result = TCL_ERROR;
        goto done;
    }
    if (Tcl_IsSafe(interp)) {
        if (safeProc == NULL) {
            Tcl_AppendResult(interp, 
                "can't use package in a safe interpreter: ", "no ", 
                safeProcName, " procedure", (char *) NULL);
            result = TCL_ERROR;
            goto done;
        }
        result = (*safeProc)(interp);
    } else {
        result = (*initProc)(interp);
    }
 done:
    if (result != TCL_OK) {
        TclpUnloadFile(loadData);
        return TCL_ERROR;
    }   
    return TCL_OK;
}
#endif


/*
 * Copyright 2005, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Zhanyong Wan, Sean Mcafee
 *
 * Taken from The Google C++ Testing Framework (Google Test).
 * Modified for this discussion by Fred Richards.
 *
 */

#define MAXULPS 4
#define SIGNBITMASK     0x8000000000000000ULL
#define FRACBITMASK     0x007FFFFFFFFFFFFFULL
#define EXPBITMASK      0x7F80000000000000ULL
#define INFBITMASK      0x7ff0000000000000ULL

typedef uint64_t Bits;

typedef union {
    double value;
    Bits bits;
} FloatingPoint;

static INLINE int
IsNaN(const Bits x)
{
    return (((EXPBITMASK & x) == EXPBITMASK) && ((FRACBITMASK & x) != 0));
}
 
#ifdef notdef
static INLINE int
IsInfinite(const Bits  x)
{
    return ((x & INFBITMASK) == EXPBITMASK);
}

static INLINE int
Signof(const Bits x)
{
    return (x & SIGNBITMASK);
}
#endif

static INLINE Bits 
SignAndMagnitudeToBiased(const Bits sam)
{
    if (SIGNBITMASK & sam) {
        return ~sam + 1UL;              /* two's complement */
    } else {
        return SIGNBITMASK | sam;       /* * 2 */
    }
}

static INLINE Bits 
DistanceBetweenSignAndMagnitudeNumber(const Bits sam1, const Bits sam2) 
{
    Bits biased1, biased2;

    biased1 = SignAndMagnitudeToBiased(sam1);
    biased2 = SignAndMagnitudeToBiased(sam2);
    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
}

/*
 * Now checking for NaN to match == behavior.
 */
int
Blt_AlmostEquals(double x, double y) 
{
    FloatingPoint a, b;
    Bits ulps;
    
    a.value = x;
    b.value = y;
    if (a.bits == b.bits) {
        return TRUE;
    }
    if (IsNaN(a.bits) || IsNaN(b.bits)) {
        return FALSE;
    }
    ulps = DistanceBetweenSignAndMagnitudeNumber(a.bits, b.bits);
#ifdef notdef
    if (ulps > MAXULPS) {
        fprintf(stderr, "AlmostEquals ulps=%u x=%.17g y=%.17g\n", 
                (Bits)ulps, x, y);
    }
#endif
    return (ulps <= MAXULPS) ? TRUE : FALSE;
}

#ifdef notdef
/* Support functions and conditional compilation directives for the master
 * AlmostEqual function. */
#define INFINITYCHECK
#define NANCHECK
#define SIGNCHECK

#ifdef  INFINITYCHECK
INLINE static int 
IsInfinite(double A)
{
    const long kInfAsInt = 0x7F80000000000000L;
    long *longPtr;

    longPtr = (long *)&A;
    /* An infinity has an exponent of 255 (shift left 23 positions) and a
     * zero mantissa. There are two infinities - positive and negative. */
    if ((*longPtr & 0x7FFFFFFFFFFFFFFFL) == kInfAsInt)
        return TRUE;
    return FALSE;
}
#endif /*INFINITYCHECK*/

INLINE static int 
IsNan(double A)
{
    long exp, mantissa;
    long *longPtr;

    longPtr = (long *)&A;
    /* A NAN has an exponent of 255 (shifted left 23 positions) and a
     * non-zero mantissa. */
    exp = *longPtr & 0x7F80000000000000L;
    mantissa = *longPtr & 0x007FFFFFFFFFFFFFL;
    if (exp == 0x7F80000000000000L && mantissa != 0) {
        return TRUE;
    }
    return FALSE;
}

INLINE static
long Sign(double A)
{
    long *longPtr;

    longPtr = (long *)&A;
    /* The sign bit of a number is the high bit. */
    return *longPtr & 0x8000000000000000;
}

/* This is the 'final' version of the AlmostEqualUlps function.  The
 * optional checks are included for completeness, but in many cases they
 * are not necessary, or even not desirable. */
int 
Blt_AlmostEquals2(double A, double B)
{
    long aInt, bInt, intDiff;
    long *longPtr;

    /* There are several optional checks that you can do, depending on what
     * behavior you want from your floating point comparisons.  These
     * checks should not be necessary and they are included mainly for
     * completeness. */

#ifdef  INFINITYCHECK
    /* If A or B are infinity (positive or negative) then only return true
     * if they are exactly equal to each other - that is, if they are both
     * infinities of the same sign.  This check is only needed if you will
     * be generating infinities and you don't want them 'close' to numbers
     * near FLT_MAX. */
    if (IsInfinite(A) || IsInfinite(B))
        return A == B;
#endif

#ifdef  NANCHECK
    /* If A or B are a NAN, return false. NANs are equal to nothing, not
     * even themselves.  This check is only needed if you will be
     * generating NANs and you use a maxUlps greater than 4 million or you
     * want to ensure that a NAN does not equal itself. */
    if (IsNan(A) || IsNan(B))
        return FALSE;
#endif

#ifdef  SIGNCHECK
    /* After adjusting floats so their representations are
     * lexicographically ordered as twos-complement integers a very small
     * positive number will compare as 'close' to a very small negative
     * number. If this is not desireable, and if you are on a platform that
     * supports subnormals (which is the only place the problem can show
     * up) then you need this check.  The check for A == B is because zero
     * and negative zero have different signs but are equal to each
     * other. */
    if (Sign(A) != Sign(B))
        return A == B;
#endif
    longPtr = (long *)&A;
    aInt = *longPtr;
    /* Make aInt lexicographically ordered as a twos-complement int */
    if (aInt < 0)
        aInt = 0x8000000000000000 - aInt;
    /* Make bInt lexicographically ordered as a twos-complement int */
    longPtr = (long *)&B;
    bInt = *longPtr;
    if (bInt < 0)
        bInt = 0x8000000000000000 - bInt;

    /* Now we can compare aInt and bInt to find out how far apart A and B
     * are. */
    intDiff = labs(aInt - bInt);
#ifndef notdef
    if (intDiff > MAXULPS) {
        fprintf(stderr, "AlmostEquals ulps=%ld x=%.17g y=%.17g\n", 
                (long)intDiff, A, B);
    }
#endif
    if (intDiff <= MAXULPS)
        return TRUE;
    return FALSE;
}
#endif
/* 
 * Converts the list from TCL memory into normal memory. This is
 * because the list could be either a split list or a (malloc-ed)
 * generated list (like below). 
*/
const char **
Blt_ConvertListToList(int argc, const char **argv)
{
    size_t listSize, needed, numBytes;
    const char **list;
    char *p;
    int i;
        
    listSize = sizeof(const char **) * (argc + 1);
    needed = 0;
    for (i = 0; i < argc; i++) {
        needed += (strlen(argv[i]) + 1);
    }
    numBytes = needed + listSize;
    list = Blt_AssertMalloc(numBytes);
    p = (char *)list + listSize;
    for (i = 0; i < argc; i++) {
        list[i] = p;
        strcpy(p, argv[i]);
        p += strlen(argv[i]) + 1;       /* Leave room for NUL byte. */
    }
    list[i] = NULL;
    return list;
}

