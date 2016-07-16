
=========
blt::grab
=========

-------------------------------------------------------------
Confine pointer and keyboard events to a window sub-tree.
-------------------------------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
========

**blt::grab current** ?\ *windowName*\ ?

**blt::grab debug**  *boolean*

**blt::grab empty** 

**blt::grab list**  

**blt::grab pop** ?\ *windowName*\ ?

**blt::grab push** *windowName* ?\ *switches* ... ?

**blt::grab release** *windowName* 

**blt::grab set** ?\ **-global**\ ? *windowName* 

**blt::grab status** *windowName* 

**blt::grab top** 

DESCRIPTION
===========

The **blt::grab** command adds a window stack to the Tk **grab**
command.  It lets you push and pop grabbed windows.  When you pop a
grabbed window, the previous grab is restored.  

INTRODUCTION
============

This command implements simple pointer and keyboard grabs using the same
mechanism as Tk's **grab** command.  The only difference is that the
**blt::grab** command creates a stack of grabbed window so that the grab
can be restored to the previous window on the stack when the grab from top
window is released. This can be useful, for example, when a modal dialog
box pops up and menu or another modal dialog.

OPERATIONS
==========

The following operations are available for the **blt::grab** command:

**blt::grab** ?\ **-global**\ ? *windowName*
  Same as grab set, described below.

**blt::grab current** ?\ *windowName*\ ?
   If *windowName* is specified, returns the name of the current grab
   window in this application for window's display, or an empty string if
   there is no such window.  If *windowName* is omitted, the command
   returns a list whose elements are all of the windows grabbed by this
   application for all displays, or an empty string if the application has
   no grabs.

**blt::grab debug**  *boolean*
  Indicates to turn on/off debugging of the grab stack.  
  
**blt::grab empty** 
  Indicates if the stack of windows is empty.  Returns "1" if there
  are no grabbed windows, and "0" otherwise.
  
**blt::grab list**  
  Returns a list of the windows on the stack.

**blt::grab pop** ?\ *windowName*\ ?
  Pops the top window on the grab stack and restores the grab to previous
  window in the stack (if there is one). If there is no previous window
  on stack, the grab is released.
  
**blt::grab push** *windowName* ?\ *switches* ... ?
  Pushes *windowName* on the grabbed window stack and sets the current grab
  to *windowName*.
  
**blt::grab release** *windowName* 
  Releases the grab on *windowName* if there is one, otherwise does nothing.
  Returns an empty string.  

  This operation exists for compatibility with Tk's **grab** command
  only. The **pop** operation should be used instead. The **release**
  operation defeats the purpose of stacking grab windows.  The entire
  grab stack is popped.

**blt::grab set** ?\ **-global**\ ? *windowName* 
  Sets a grab on *windowName*.  If **-global** is specified then the grab
  is global, otherwise it is local.  If a grab was already in effect for
  this application on window's display then it is automatically released.
  If there is already a grab on window and it has the same global/local
  form as the requested grab, then the command does nothing.  Returns an
  empty string.

  This operation exists for compatibility with Tk's **grab** command
  only. The **push** operation should be used instead. The **set**
  operation defeats the purpose of stacking grab windows.  The grab stack
  is emptied and *windowName* is pushed on top.

**blt::grab status** *windowName* 
  Returns none if no grab is currently set on *windowName*, local if a
  local grab is set on *windowName*, and global if a global grab is set.

**blt::grab top** 
  Returns the name of the window on the top of the grab stack.  This is the
  window that is currently grabbed.  If the stack is empty, "" is returned.

EXAMPLE
=======

You can make several widgets grab by simply making its ancestor widget
grab using the **push** operation.

  ::

     package require BLT

     frame .top
     button .top.button; canvas .top.canvas
     pack .top.button .top.canvas
     pack .top
       . . .
     blt::grab push .top

All the widgets within ".top" (including ".top") are now grabbed.  

When the application is no longer grab processing, you can allow user
interactions again by the **pop** operation.

 ::

     blt::grab pop .top 

KEYWORDS
========

grab, keyboard events, pointer events, window, cursor

COPYRIGHT
=========

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


