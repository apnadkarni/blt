
/*
 * bltTclWin.h --
 *
 *	Copyright 1993-2004 George A Howlett.
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
#define fstat	 _fstat
#define stat	 _stat
#ifdef _MSC_VER
#define fileno	 _fileno
#endif
#define isnan(x)		_isnan(x)
#define strcasecmp(s1,s2)	_stricmp(s1,s2)
#define strncasecmp(s1,s2,n)	_strnicmp(s1,s2,n)
#define vsnprintf		_vsnprintf
#define isascii(c)		__isascii(c)
#endif /* _MSC_VER || __BORLANDC__ */

#ifdef __BORLANDC__
#define isnan(x)		_isnan(x)
#endif

BLT_EXTERN double hypot(double x, double y);

#if defined(__BORLANDC__) || defined(_MSC_VER)
#ifdef FINITE
#undef FINITE
#define FINITE(x)		_finite(x)
#endif
#endif /* __BORLANDC__ || _MSC_VER */

BLT_EXTERN ssize_t Blt_AsyncRead(int fd, unsigned char *buffer, size_t size);
BLT_EXTERN ssize_t Blt_AsyncWrite(int fd, const unsigned char *buffer, 
	size_t size);
BLT_EXTERN void Blt_CreateFileHandler(int fd, int flags, Tcl_FileProc *proc, 
	ClientData clientData);
BLT_EXTERN void Blt_DeleteFileHandler(int fd);

#endif /*_BLT_TCL_WIN_H*/
