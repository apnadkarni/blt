/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltRound.h --
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
