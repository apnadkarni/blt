
==========
blt::debug
==========

------------------------------------------------
Print Tcl commands before execution
------------------------------------------------

:Author: George A. Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
        Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::debug** ?\ *debugLevel*\ ?

DESCRIPTION
-----------

The **blt::debug** command provides a simple tracing facility for TCL
commands.  Each TCL command line is printed on standard error before it is
executed. The output consists of the command line both before and after
substitutions have occurred.

*DebugLevel* is an integer then it gives a distance (down the procedure
calling stack) before stopping tracing of commands.  If *debugLevel* is
"0", no tracing is performed. This is the default.

If no *debugLevel* argument is given, the current debug level is printed.


KEYWORDS
--------

debug
