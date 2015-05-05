============
blt::watch
============

-------------------------------------------------
Call TCL procedures before and after each command
-------------------------------------------------

:Author: George A Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::watch create** *watchName* ?\ *option* *value* ... ?

**blt::watch activate** *watchName*

**blt::watch deactivate** *watchName*

**blt::watch delete** *watchName*

**blt::watch configure** *watchName* ?\ *option* *value* ... ?

**blt::watch info** *watchName*

**blt::watch names** 

DESCRIPTION
-----------

The **blt::watch** command arranges for TCL procedures to be invoked before
and after the execution of each TCL command.

INTRODUCTION
------------

When an error occurs in TCL, the global variable *errorInfo* will contain a
stack-trace of the active procedures when the error occured.  Sometimes,
however, the stack trace is insufficient.  You may need to know exactly
where in the program's execution the error occured.  In cases like this, a
more general tracing facility would be useful.

The **blt::watch** command lets you designate TCL procedures to be invoked
before and after the execution of each TCL command.  This means you can
display the command line and its results for each command as it executes.
Another use is to profile your TCL commands.  You can profile any TCL
command (like **if** and **set**), not just TCL procedures.

OPERATIONS
----------

The following operations are available for the **blt::watch** command:

**blt::watch activate** *watchName* 
  Activates the watch, causing TCL commands the be traced to the maximum
  depth selected.

**blt::watch create** *watchName* ?\ *option* *value* ... ?
  Creates a new watch *watchName*. It's an error if another watch
  *watchName* already exists and an error message will be returned.
  *Options* may have any of the values accepted by the **configure**
  operation.  This command returns the empty string.

**blt::watch configure** *watchName* ?\ *option* *value* ... ?
  Queries or modifies the configuration options of the watch *watchName*.
  *WatchName* is the name of a watch.  *Option* and *value* may be one
  of the following values:

  **-active** *boolean*
    Specifies if the watch is active.  By default, watches are active when
    created.

  **-postcmd** *cmdPrefix*
    Specifies a TCL procedure to be called immediately after each TCL
    command.  *CmdPrefix* is name of a TCL procedure and any extra
    arguments to be passed to it.  Before *cmdPrefix* is invoked, five more
    arguments are appended:

    1) The current level.
    2) The current command line.
    3) A list containing the command after substitutions and split into words.
    4) The return code of the command.
    5) The results of the command.

    The return status of the **-postcmd** procedure is always ignored.

  **-precmd** *cmdPrefix* 
    Specifies a TCL procedure to be called immediately before each TCL
    command.  *CmdPrefix* is name of a TCL procedure and any extra
    arguments to be passed to it.  Before *cmdPrefix* is invoked, three
    arguments are appended:

    1) The current level.
    2) The current command line.
    3) A list containing the command after substitutions and split into words.

    The return status of the **-precmd** procedure is always ignored.

  **-maxlevel** *number*
    Specifies the maximum evaluation depth to watch TCL commands.  The
    default maximum level is 10000.

**blt::watch deactivate** *watchName* 
  Deactivates the watch.  The **-precmd** and **-postcmd** procedures will
  no longer be invoked.

**blt::watch info** *watchName* 
  Returns the configuration information associated with the watch
  *watchName*.  *WatchName* is the name of a watch.

**blt::watch names** ?\ *state*\ ?
  Lists the names of the watches for a given state.  *State* may be one of
  the following: "active", "idle", or "ignore".  If a *state* argument
  isn't specified, all watches are listed.

EXAMPLE
-------

The following example use **blt::watch** to trace TCL commands 
(printing to standard error) both before and after they are executed. 

 ::

    proc preCmd { level command argv } {
	set name [lindex $argv 0]
	puts stderr "$level $name => $command"
    }

    proc postCmd { level command argv retcode results } {
	set name [lindex $argv 0]
	puts stderr "$level $name => $argv\n<= ($retcode) $results"
    }
    blt::watch create trace \\
	    -postcmd postCmd -precmd preCmd

KEYWORDS
--------

debug, profile

COPYRIGHT
---------

2015 George A. Howlett. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 1) Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2) Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the distribution.
 3) Neither the name of the authors nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.
 4) Products derived from this software may not be called "BLT" nor may
    "BLT" appear in their names without specific prior written permission
    from the author.

THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
