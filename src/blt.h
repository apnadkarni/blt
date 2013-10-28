/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * blt.h --
 *
 *	Copyright 1991-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _BLT_H
#define _BLT_H

#define BLT_MAJOR_VERSION 	3
#define BLT_MINOR_VERSION 	0
#define BLT_VERSION		"3.0"
#define BLT_PATCH_LEVEL		"3.0a"
#define BLT_RELEASE_SERIAL	0

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
#endif	/* __cplusplus */

#define _VERSION(a,b,c)	    (((a) << 16) + ((b) << 8) + (c))

#define _TCL_VERSION _VERSION(TCL_MAJOR_VERSION, TCL_MINOR_VERSION, TCL_RELEASE_SERIAL)
#define _TK_VERSION _VERSION(TK_MAJOR_VERSION, TK_MINOR_VERSION, TK_RELEASE_SERIAL)

/* 
 * Tcl 8.5 breaks backward compatibility with Tcl_PkgRequire by including the
 * patchlevel in exact matches of the version number.  As a result, we have to
 * request the version as either TCL_PATCH_LEVEL or TCL_VERSION, depending
 * upon which version of Tcl.
 */
#if (_TCL_VERSION >= _VERSION(8,5,0)) 
#  define TCL_VERSION_COMPILED		TCL_PATCH_LEVEL
#else 
#  define TCL_VERSION_COMPILED		TCL_VERSION
#endif

#ifdef _TK
#if (_TK_VERSION >= _VERSION(8,5,0)) 
#  define TK_VERSION_COMPILED		TK_PATCH_LEVEL
#else 
#  define TK_VERSION_COMPILED		TK_VERSION
#endif 
#endif /*_TK*/

#define PKG_ANY		0
#define PKG_EXACT	1

#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(__BORLANDC__)
#	define __WIN32__
#	ifndef WIN32
#	    define WIN32
#	endif
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
