/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTclWin.h --
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

#ifndef _BLT_TCL_WIN_H
#define _BLT_TCL_WIN_H

#ifdef CHECK_UNICODE_CALLS
#define _UNICODE
#define UNICODE
#define __TCHAR_DEFINED
typedef float *_TCHAR;
#define _TCHAR_DEFINED
typedef float *TCHAR;
#endif /* CHECK_UNICODE_CALLS */

#undef Blt_Export
#define Blt_Export __declspec(dllexport)

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define fstat    _fstat
#define stat     _stat
#ifdef _MSC_VER
#define fileno   _fileno
#endif
#if 0 /* APN TBD */
#define isnan(x)                _isnan(x)
#define isascii(c)              __isascii(c)
#endif
#define strcasecmp              _stricmp
#define strncasecmp             _strnicmp
#define vsnprintf               _vsnprintf
#endif /* _MSC_VER || __BORLANDC__ */

#ifdef __BORLANDC__
#define isnan(x)                _isnan(x)
#endif

#if 0
BLT_EXTERN double hypot(double x, double y);
#endif

#if defined(__BORLANDC__) || defined(_MSC_VER)
#ifdef FINITE
#undef FINITE
#define FINITE(x)               _finite(x)
#endif
#endif /* __BORLANDC__ || _MSC_VER */

#ifdef _MSC_VER
typedef SSIZE_T ssize_t;
#define S_IS(mode_, s_flags_) (((mode_) & S_IFMT) == (s_flags_))
#define S_ISDIR(mode_) S_IS(mode_, _S_IFDIR)
#define S_ISREG(mode_) S_IS(mode_, _S_IFREG)
#define S_ISCHR(mode_) S_IS(mode_, _S_IFCHR)
#define S_ISBLK(mode_) (0) /* APN TBD - Win32 equivalent */
#define S_ISFIFO(mode_) S_IS(mode_, _S_IFIFO)

#define R_OK 04
#define F_OK 00
#define W_OK 02

#endif

BLT_EXTERN int Blt_AsyncRead(HANDLE hFile, char *buffer, size_t count);
BLT_EXTERN int Blt_AsyncWrite(HANDLE hFile, const char *buffer, size_t count);
BLT_EXTERN void Blt_CreateFileHandler(HANDLE hFile, int flags,
        Tcl_FileProc *proc, ClientData clientData);
BLT_EXTERN void Blt_DeleteFileHandler(HANDLE hFile);

BLT_EXTERN char *strcasestr(const char *s, const char *find);

#endif /*_BLT_TCL_WIN_H*/
