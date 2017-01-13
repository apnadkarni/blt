/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltInt.h --
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

#ifndef _BLT_INT_H
#define _BLT_INT_H

#undef SIZEOF_LONG
#include "config.h"
#ifdef _MSC_VER
#  define _CRT_SECURE_NO_DEPRECATE
#  define _CRT_NONSTDC_NO_DEPRECATE
#endif
#ifdef WIN32
#  define STRICT
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef STRICT
#  undef WIN32_LEAN_AND_MEAN
#  include <windowsx.h>
#endif /* WIN32 */

/* Need off_t typedef before including tk.h (mingw32) */
#ifdef HAVE_SYS_TYPES_H
  #include <sys/types.h>
#endif  /* HAVE_SYS_TYPES_H */

#ifndef __STDC_VERSION__
#  define __STDC_VERSION__ 0L
#endif

#include <tcl.h>
#include <tk.h>
#include "blt.h"

#undef  BLT_STORAGE_CLASS
#define BLT_STORAGE_CLASS DLLEXPORT


#ifdef USE_TCL_STUBS
#  include "tclIntDecls.h"
#  ifdef WIN32
#    include "tclIntPlatDecls.h"
#  endif
#endif  /* USE_TCL_STUBS */

#include <stdio.h>

#if defined(__GNUC__) && defined(HAVE_X86) && defined(__OPTIMIZE__)
#  define HAVE_X86_ASM
#endif

#undef MIN
#define MIN(a,b)        (((a)<(b))?(a):(b))
#undef MAX
#define MAX(a,b)        (((a)>(b))?(a):(b))

#define MIN3(a,b,c)     \
    (((a)<(b))?((a)<(c))?(a):((b)<(c))?(b):(c):((b)<(c))?(b):(c))
#define MAX3(a,b,c)     \
    (((a)>(b))?((a)>(c))?(a):((b)>(c))?(b):(c):((b)>(c))?(b):(c))

#define TRUE    1
#define FALSE   0

#define PKG_ANY         0
#define PKG_EXACT       1

/*
 * The macro below is used to modify a "char" value (e.g. by casting
it to an unsigned character) so that it can be used safely with
 * macros such as isspace.
 */
#define UCHAR(c) ((unsigned char) (c))

#ifdef TCL_UTF_MAX
#  define HAVE_UTF      1
#else
#  define HAVE_UTF      0
#endif /* TCL_UTF_MAX */

#include <bltAssert.h>
#include <bltTypes.h>
#include "bltTclProcs.h"
#include "bltTclIntProcs.h"
#include "bltTkProcs.h"
#include "bltTkIntProcs.h"

/*
 * Define this if you want to be able to tile to the main window "."  This
 * will cause a conflict with Tk if you try to compile and link statically.
 */
#undef TK_MAINWINDOW

#include "bltTclInt.h"
#include "bltTkInt.h"

#ifdef WIN32
#  include "bltWin.h"
#else 

#ifdef MACOSX
#  include "bltMacOSX.h"
#endif /* MACOSX */

#endif /* WIN32 */

#ifdef USE_TK_STUBS
#  include "tkIntDecls.h"
#  include "tkIntPlatDecls.h"
#endif  /* USE_TCL_STUBS */

#include "bltTclInit.h"
#include "bltTkInit.h"

#endif /*_BLT_INT_H*/
