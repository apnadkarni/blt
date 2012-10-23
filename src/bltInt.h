
/*
 * bltInt.h --
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
#endif	/* USE_TCL_STUBS */

#include <stdio.h>

#if defined(__GNUC__) && defined(HAVE_X86) && defined(__OPTIMIZE__)
#  define HAVE_X86_ASM
#endif

#undef MIN
#define MIN(a,b)	(((a)<(b))?(a):(b))
#undef MAX
#define MAX(a,b)	(((a)>(b))?(a):(b))

#define MIN3(a,b,c)	\
    (((a)<(b))?((a)<(c))?(a):((b)<(c))?(b):(c):((b)<(c))?(b):(c))
#define MAX3(a,b,c)	\
    (((a)>(b))?((a)>(c))?(a):((b)>(c))?(b):(c):((b)>(c))?(b):(c))

#define TRUE 	1
#define FALSE 	0

#define PKG_ANY		0
#define PKG_EXACT	1

/*
 * The macro below is used to modify a "char" value (e.g. by casting
 * it to an unsigned character) so that it can be used safely with
 * macros such as isspace.
 */
#define UCHAR(c) ((unsigned char) (c))

#ifdef TCL_UTF_MAX
#  define HAVE_UTF	1
#else
#  define HAVE_UTF	0
#endif /* TCL_UTF_MAX */

#include <bltAssert.h>
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
#endif	/* USE_TCL_STUBS */

#include "bltTclInit.h"
#include "bltTkInit.h"

#endif /*_BLT_INT_H*/
