
/*
 * bltRound.h --
 *
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _BLT_ROUND_H
#define _BLT_ROUND_H


#if (SIZEOF_FLOAT == 8)
#define REAL64	float
#else
#define REAL64	double
#endif	/* SIZE_VOID_P == 8 */

#define DOUBLE_MAGIC		6755399441055744.0
#define DEFAULT_CONVERSION	1
#ifdef WORDS_BIGENDIAN
#define IMAN 1
#define IEXP 0
#else
#define IMAN 0
#define IEXP 1
#endif	/* WORDS_BIGENDIAN */

static INLINE int 
CRoundToInt(REAL64 val)
{
#if DEFAULT_CONVERSION==0
    val += DOUBLE_MAGIC;
#ifdef WORDS_BIGENDIAN
    return ((int *)&val)[1];
#else
    return ((int *)&val)[0];
#endif	/* WORD_BIGENDIAN */
#else
    return (int)(floor(val+.5));
#endif	/* DEFAULT_CONVERSION */
}

#endif	/*_BLT_ROUND_H*/
