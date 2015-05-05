
===============
blt::bgexec
===============

--------------------------------------------------------
Run programs in the background while handling Tk events.
--------------------------------------------------------

:Author: George A. Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::bgexec** *varName* ?\ *option value* ... ? *program* ?\ *arg*  ... ?

DESCRIPTION
-----------

The **blt::bgexec** command executes programs in the background, allowing
Tk to handle events.  A global TCL variable *varName* is set when the
program has completed.

INTRODUCTION
------------

TCL's **exec** command is very useful for gathering information from the
operating system.  It runs a program and returns the output as its result.
This works well for TCL-only applications. But for Tk applications, a
problem occurs when the program takes time to process.  Let's say we want
the get the disk usage of a directory.  We'll use the Unix program "du" to
get the summary.

 ::

    set out [exec du -s $dir]
    puts "Disk usage for $dir is $out"

While "du" is running, scrollbars won't respond.  None of the Tk widgets
will be redrawn properly.  The **send** command won't work.  And the worst
part is that the application appears hung up or dead.  The problem is that
while **exec** is waiting for *du* to finish, Tk is not able to handle X
events.

The **blt::bgexec** command performs the same functions as **exec**, but
also allows Tk to handle events.  You can execute a long-running program
and the Tk widgets will behave normally.  When the program finishes, its
output and the exit status are written to TCL variables.  This makes it
easy to monitor and save the output of a program.

SYNTAX
------

The **blt::bgexec** command takes the following form:

  **blt::bgexec** *varName* ?\ *option* *value* ... ? *program* ?\ *arg* ... ?

*VarName* is the name of a global variable which is set when program* has
finished executing.  The exit status of will be stored in *varName*.  The
exit status is a list of a status token, the process-id of the program, the
exit code, and a status message.  You can also prematurely terminate the
program by setting *varName*.  Under Unix, the program will be sent a
signal to terminate it (by default the signal is a SIGKILL; see the
**-killsignal** option).

*Program* is the name of the program to be executed and *args* are any
extra arguments for *program*.  The syntax of *program* and *args* is the
same as the **exec** command. So you can redirect I/O, execute pipelines,
etc. (see the **exec** manual for further information) just like **exec**.
If the last argument is an ampersand (&), the program will be run detached,
and **blt::bgexec** will return immediately.  *VarName* will still be set
with the return status when *program* completes.  *Option* refers to the
switch name that always beginning with a dash (-).  *Value* is the value of
the option.  Option-value pairs are terminated either by the program name,
or double dashes (--).  The following options are available for
**blt::bgexec**:

  **-decodeerror** *encodingName* 
    Specifies the encoding of the stderr channel.  This affects only data
    returned to the TCL interpreter.  No translation is done on file
    redirection.

    For example if data is to be converted from Unicode for use in TCL, you
    would use the "unicode" encoding. The default is that no tranlation is
    performed.

  **-decodeoutput** *encodingName* 
    Specifies the encoding of the stdout channels.  This affects only data
    returned to the TCL interpreter.  No translation is done on file
    redirection.

    For example if data is to be converted from Unicode for use in TCL, you
    would use the "unicode" encoding. The default is that no tranlation is
    performed.

  **-detach** *boolean*
    Indicates that the detached program should not be killed when the
    calling TCL interpreter exits.  By default all detached programs are
    killed when the TCL interpreter ends.

  **-echo** *boolean*
    Indicates that the program's stderr channel should be echoed to the
    terminal's stderr.
    
  **-error** *varName* 
    Specifies that a global variable *varName* is to be set with the contents
    of stderr after the program has completed.

  **-ignoreexitcode** *boolean*

  **-keepnewline** *boolean*
    Specifies that a trailing newline should be retained in the output. If
    *boolean* is true, the trailing newline is truncated from the output of
    the **-onoutput** and **-output** variables.  The default value is
    "true".

  **-killsignal** *signal*
    Specifies the signal to be sent to the program when terminating. This is
    available only under Unix.  *Signal* can either be a number (typically
    1-32) or a mnemonic (such as SIGINT). If *signal* is the empty string,
    then no signal is sent.  The default signal is "9" (SIGKILL).

  **-lasterror** *varName*
    Specifies a variable *varName* that is updated whenever data becomes
    available from standard error of the program.  *VarName* is a global
    variable. Unlike the **-error** option, data is available as soon as
    it arrives.

  **-lastoutput** *varName* 
    Specifies a variable *varName* that is updated whenever data becomes
    available from standard output of the program.  *VarName* is a global
    variable. Unlike the **-output** option, data is available as soon as
    it arrives.

  **-linebuffered** *boolean*
    Specifies that updates should be made on a line-by-line basis.  Normally
    when new data is available **blt::bgexec** will set the variable
    (**-lastoutput** and **-lasterror** options) or invoke the command
    (**-onoutput** and **-onerror** options) delivering all the new
    data currently available.  If *boolean* is true, only one line at a time
    will be delivered.  This can be useful when you want to process the
    output on a line-by-line basis.  The default value is "false".

  **-onerror** *command*
    Specifies the start of a TCL command that will be executed whenever new
    data is available from standard error. The data is appended to the
    command as an extra argument before it is executed.

  **-onoutput** *command* 
    Specifies the start of a TCL command that will be executed whenever new
    data is available from standard output. The data is appended to the
    command as an extra argument before it is executed.

  **-output** *varName*
    Specifies that a global variable *varName* is to be set with the output
    of the program, once it has completed.  If this option is not set, no
    output will be accumulated.

  **-poll** *milliseconds* 
    Specifies the time to wait before checking if the program has
    terminated.  Typically a program will close its stdout and stderr
    channels right before it terminates.  But for programs that close
    stdout early, **blt::bgexec** will wait for the program to finish.
    *Milliseconds* is the number of milliseconds to wait before checking if the
    program has terminated.  The default is "1000".

  **-pty** *boolean* 
    For Unix programs only, this flags indicates to use a pseudo-terminal
    and runs the program in a session (see **setsid**). The advantages
    are 1) output is not buffered and 2) child processes of the the program
    and killed when the program is terminated.
    
  **-update** *varName* 
    Deprecated. This option is replaced by **-lasterror**.

  **--**
    This marks the end of the options.  The following argument will
    be considered the name of a program even if it starts with 
    a dash "-".

EXAMPLE
-------

Here is the disk usage example again, this time using **blt::bgexec**.  The
syntax to invoke "du" is exactly the same as the previous example, when we
used **exec**.

  ::

     global myStatus myOutput
     blt::bgexec myStatus -output myOutput du -s $dir
     puts "Disk usage for $dir is $myOutput"

Two global variables, "myStatus" and "myOutput", will be set by
**blt::bgexec** when "du" has completed. "MyStatus" will contain the
program's exit status.  "MyOutput", specified by the **-output** option,
will store the output of the program.

You can also terminate the program by setting the variable
"myStatus".  If "myStatus" is set before "du" has
completed, the process is killed. Under Unix, this is done sending by
a configurable signal (by default it's SIGKILL). Under Win32, this
is done by calling **TerminateProcess**. It makes no
difference what "myStatus" is set to.

  ::

     set myStatus {}

There are several **blt::bgexec** options to collect different types of
information.

  ::

     global myStatus myOutput myErrs
     blt::bgexec myStatus -output myOutput -error myErrs du -s $dir

The **-error** option is similar to **-output**.  It sets a global variable
when the program completes.  The variable will contain any data written to
stderr by the program.

The **-output** and **-error** variables are set only
after the program completes.  But if the program takes a long time, to
run you may want to receive its partial output.  You can gather data
as it becomes available using the **-onoutput** option.  It
specifies a TCL command prefix.  Whenever new data is available, this
command is executed, with the data appended as an argument to the
command.

  ::

     proc GetInfo { data } {
         puts $data
     }
     blt::bgexec myStatus -onoutput GetInfo du -s $dir

When output is available, the procedure "GetInfo" is called.  The
**-onerror** option performs a similar function for the stderr data stream.

Like **exec**, **blt::bgexec** returns an error if the exit code of the
program is not zero.  If you think you may get a non-zero exit
code, you might want to invoke **blt::bgexec** from within a **catch**.

  ::

     catch { blt::bgexec myStatus -output myOutput du -s $dir }

By default, **blt::bgexec** will wait for the program to finish.  But you
can detach the program making ampersand (&) the last argument on the
command line.

  ::

     global myStatus myOutput
     blt::bgexec myStatus -output myOutput du -s $dir &

**blt::bgexec** will return immediately and its result will be a list of
the spawned process ids.  If at some point you need to wait for the program
to finish up, you can use **tkwait**.  When the program finishes, the
variable "myStatus" will be written to, breaking out the **tkwait**
command.

  ::

     global myStatus myOutput
     blt::bgexec myStatus -output myOutput du -s $dir &
	...
     tkwait variable myStatus

PREEMPTION
----------

Because **blt::bgexec** allows Tk to handle events while a program is
running, it's possible for an application to preempt itself with further
user-interactions.  Let's say your application has a button that runs the
disk usage example.  And while the "du" program is running, the user
accidently presses the button again.  A second **blt::bgexec** program will
preempt the first.  What this means is that the first program can not
finish until the second program has completed.

Care must be taken to prevent an application from preempting itself by
blocking further user-interactions (such as button clicks).  The
**blt::busy** command is very useful for just these situations.  See the
**blt::busy** manual for details.


VERSUS TK FILEEVENT
-------------------

Since Tk 4.0, a subset of **blt::bgexec** can be also achieved using the
**fileevent** command.  The steps for running a program in the
background are:

Execute the program with the **open** command (using the "|" syntax) and
save the file handle.

  ::

     global fileId 
     set fileId [open "|du -s $dir" r]

Next register a TCL code snippet with **fileevent** to be run whenever
output is available on the file handle.  The code snippet will read from
the file handle and save the output in a variable.

  ::

     fileevent fileId readable { 
	if { [gets $fileId line] < 0 } {
            close $fileId
            set output $temp
	    unset fileId temp
        } else {
	    append temp $line
        }
     }

The biggest advantage of **blt::bgexec** is that, unlike **fileevent**, it
requires no additional TCL code to run a program.  It's simpler and less
error prone.  You don't have to worry about non-blocking I/O.  It's handled
tranparently for you.

**blt::bgexec** runs programs that **fileevent** can not.  **Fileevent**
assumes that the when stdout is closed the program has completed.  But some
programs, like the Unix "compress" program, reopen stdout, fooling
**fileevent** into thinking the program has terminated.  In the example
above, we assume that the program will write and flush its output
line-by-line.  However running another program, your application may block
in the **gets** command reading a partial line.

**blt::bgexec** lets you get back the exit status of the program. It also
allows you to collect data from both stdout and stderr simultaneously.
Finally, since data collection is handled in C code, **blt::bgexec** is
faster. You get back to the Tk event loop more quickly, making your
application seem more responsive.

DIFFERENCES WITH TK EXEC
------------------------

 1. The variable name argument must always by given to **blt::bgexec**.

 2. The presence of data on stderr does not return an error.  Only
    if the program returns a non-zero exit code, will **blt::bgexec**
    return an error.
    
 
SEE ALSO
--------

busy, exec, tkwait

KEYWORDS
--------

exec, background, busy

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
