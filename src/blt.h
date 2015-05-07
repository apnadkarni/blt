/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * blt.h --
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

#ifndef _BLT_H
#define _BLT_H

#define BLT_MAJOR_VERSION       3
#define BLT_MINOR_VERSION       0
#define BLT_VERSION             "3.0"
#define BLT_PATCH_LEVEL         "3.0a"
#define BLT_RELEASE_SERIAL      0

#ifndef BLT_STORAGE_CLASS
#define BLT_STORAGE_CLASS       
#endif
#undef INLINE

#ifdef __GNUC__
#  define INLINE __inline__
#else
#  define INLINE
#endif

#ifdef __cplusplus
#  define BLT_EXTERN BLT_STORAGE_CLASS extern "C" 
#else
#  define BLT_EXTERN BLT_STORAGE_CLASS extern 
#endif  /* __cplusplus */

#define _VERSION(a,b,c)     (((a) << 16) + ((b) << 8) + (c))

#define _TCL_VERSION _VERSION(TCL_MAJOR_VERSION, TCL_MINOR_VERSION, TCL_RELEASE_SERIAL)
#define _TK_VERSION _VERSION(TK_MAJOR_VERSION, TK_MINOR_VERSION, TK_RELEASE_SERIAL)

/* 
 * Tcl 8.5 breaks backward compatibility with Tcl_PkgRequire by including the
 * patchlevel in exact matches of the version number.  As a result, we have to
 * request the version as either TCL_PATCH_LEVEL or TCL_VERSION, depending
 * upon which version of Tcl.
 */
#if (_TCL_VERSION >= _VERSION(8,5,0)) 
#  define TCL_VERSION_COMPILED          TCL_PATCH_LEVEL
#else 
#  define TCL_VERSION_COMPILED          TCL_VERSION
#endif

#ifdef _TK
#if (_TK_VERSION >= _VERSION(8,5,0)) 
#  define TK_VERSION_COMPILED           TK_PATCH_LEVEL
#else 
#  define TK_VERSION_COMPILED           TK_VERSION
#endif 
#endif /*_TK*/

#define PKG_ANY         0
#define PKG_EXACT       1

#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__BORLANDC__)
#       define __WIN32__
#       ifndef WIN32
#           define WIN32
#       endif
#   endif
#endif

#ifdef USE_BLT_STUBS
BLT_EXTERN const char *Blt_InitTclStubs(Tcl_Interp *interp, const char *version,
        int exact);
#ifdef _TK
BLT_EXTERN const char *Blt_InitTkStubs(Tcl_Interp *interp, const char *version,
        int exact);
#endif
#endif

#endif /*_BLT_H*/
