/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWinUtil.c --
 *
 * This module contains WIN32 routines not included in the Tcl/Tk
 * libraries.
 *
 *	Copyright 1998-2004 George A Howlett.
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

#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

#ifdef notdef
double
drand48(void)
{
    return (double) rand() / (double)RAND_MAX;
}

void
srand48(long int seed)
{
    srand(seed);
}
#endif

int
Blt_GetPlatformId(void)
{
    static int platformId = 0;

    if (platformId == 0) {
	OSVERSIONINFO opsysInfo;

	opsysInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&opsysInfo)) {
	    platformId = opsysInfo.dwPlatformId;
	}
    }
    return platformId;
}

const char *
Blt_PrintError(int error)
{
    static char buffer[1024];
    int length;

    FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM,
	NULL,
	error,
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	/* Default language */
	buffer,
	1024,
	NULL);
    length = strlen(buffer);
    if (buffer[length - 2] == '\r') {
	buffer[length - 2] = '\0';
    }
    return buffer;
}

const char *
Blt_LastError(void)
{
    return Blt_PrintError(GetLastError());
}

