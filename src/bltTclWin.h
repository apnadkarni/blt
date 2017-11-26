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
#define isnan(x)                _isnan(x)
#define strcasecmp(s1,s2)       _stricmp(s1,s2)
#define strncasecmp(s1,s2,n)    _strnicmp(s1,s2,n)
#define vsnprintf               _vsnprintf
#define isascii(c)              __isascii(c)
#endif /* _MSC_VER || __BORLANDC__ */

#ifdef __BORLANDC__
#define isnan(x)                _isnan(x)
#endif

BLT_EXTERN double hypot(double x, double y);

#if defined(__BORLANDC__) || defined(_MSC_VER)
#ifdef FINITE
#undef FINITE
#define FINITE(x)               _finite(x)
#endif
#endif /* __BORLANDC__ || _MSC_VER */

BLT_EXTERN ssize_t Blt_AsyncRead(HANDLE hFile, char *buffer, size_t size);
BLT_EXTERN ssize_t Blt_AsyncWrite(HANDLE hFile, const char *buffer, 
        size_t size);
BLT_EXTERN void Blt_CreateFileHandler(HANDLE hFile, int flags,
        Tcl_FileProc *proc, ClientData clientData);
BLT_EXTERN void Blt_DeleteFileHandler(HANDLE hFile);

BLT_EXTERN char *strcasestr(const char *s, const char *find);

#endif /*_BLT_TCL_WIN_H*/
