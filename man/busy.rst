
===============
blt::busy
===============

-------------------------------------------------------------
Make Tk widgets busy, temporarily blocking user interactions.
-------------------------------------------------------------

:Author: George A. Howlettt <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::busy hold** *windowName* ?\ *option* *value* ... ?

**blt::busy release**  ?\ *windowName* ... ?

**blt::busy configure** *windowName* ?\ *option* *value* ... ?

**blt::busy forget**  ?\ *windowName* ... ?

**blt::busy isbusy** *windowName* 

**blt::busy names** ?\ *pattern* ... ?

**blt::busy status** *windowName* 

DESCRIPTION
-----------

The **blt::busy** command provides a simple means to block keyboard,
button, and pointer events from Tk widgets, while overriding the widget's
cursor with a configurable busy cursor.

INTRODUCTION
------------

There are many times in applications where you want to temporarily restrict
what actions the user can take.  For example, an application could have a
"run" button that when pressed causes some processing to occur.  But while
the application is busy processing, you probably don't want the the user to
be able to click the "run" button again.  You may also want restrict the
user from other tasks such as clicking a "print" button.

The **blt::busy** command lets you make Tk widgets busy. This means that
user interactions such as button clicks, moving the mouse, typing at the
keyboard, etc. are ignored by the widget.  You can set a special cursor
(like a watch) that overrides the widget's normal cursor, providing
feedback that the application (widget) is temporarily busy.

When a widget is made busy, the widget and all of its descendents will
ignore events.  It's easy to make an entire panel of widgets busy. You can
simply make the toplevel widget (such as ".") busy.  This is easier and far
much more efficient than recursively traversing the widget hierarchy,
disabling each widget and re-configuring its cursor.

The **blt::busy** command isn't a replacement for the Tk **grab** command.
There are times where the **blt::busy** command can be better used instead
of Tk's **grab** command.  Unlike **grab** which restricts all user
interactions to one widget, with the busy command you can have more than
one widget active (for example, a "cancel" dialog and a "help" button).


OPERATIONS
----------

The following operations are available for the **blt::busy** command:

**blt::busy hold** *windowName* ?\ *option* *value* ... ?
  Makes the widget *windowName* (and its descendants in the Tk window
  hierarchy) busy.  *windowName* must be a valid path name of a Tk widget.
  The busy window is mapped the next time idle tasks are processed, and the
  widget and its descendants will be blocked from user interactions. All
  events in the widget window and its descendants are ignored.  Normally
  **update** should be called immediately afterward to insure that the
  **hold** operation is in effect *before* the application starts its
  processing. The following configuration options are valid:

  **-cursor** *cursorName*
    Specifies the cursor to be displayed when the widget is made busy.
    *CursorName* can be in any form accepted by **Tk_GetCursor**.  The
    default cursor is "watch".

  **-darken** *percent*
    Specifies the amount darken or lighten the opaque busy window. The
    **-opaque** option must be set.  The default is "30".

  **-image** *imageName*
    Draws the given image centered over the opaque busy window.

  **-opaque** *boolean*
    Indicates if an opaque busy window should be used.  This window is a
    snapshot of the current window lightened or darkened by the specified
    amount (see the **-darken** option). The default is "0".

**blt::busy configure** *windowName* ?\ *option* *value* ... ?
  Queries or modifies the **blt::busy** command configuration options for
  *windowName*. *WindowName* must be the path name of a widget previously
  made busy by the **hold** operation.  If no options are specified, a list
  describing all of the available options for *windowName* (see
  **Tk_ConfigureInfo** for information on the format of this list) is
  returned.  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*\ -*value* pairs are
  specified, then the command modifies the given widget option(s) to have
  the given value(s); in this case the command returns the empty string.
  *Option* may have any of the values accepted by the **hold** operation.

  Please note that the option database is referenced through *windowName*.
  For example, if the widget ".frame" is to be made busy, the busy cursor
  can be specified for it by either **option** command:

  ::

        "option add *frame.busyCursor gumby"
        "option add *Frame.BusyCursor gumby"

**blt::busy active**  ?\ *pattern* ... ?
  Returns the pathnames of all widgets that are currently busy (active).
  If a *pattern* is given, the path names of busy widgets matching
  *pattern* are returned.  This differs from the **names** operation in
  that **active** only returns the names of windows where the busy window
  is currently active (**names** returns all busy windows).

**blt::busy forget** ?\ *windowName* ... ?
  Releases resources allocated by the busy command for *windowName*,
  including the busy window.  User events will again be received again by
  *windowName*.  Resources are also released when *windowName* is
  destroyed. *WindowName* must be the name of a widget specified in the
  **hold** operation, otherwise an error is reported.

**blt::busy check** *windowName*
  Checks if *windowName* or any of its ancestors are currently busy.  If
  *windowName* is presently busy (it can not receive user interactions) "1"
  is returned, otherwise "0".

**blt::busy isbusy** *windowName*
  Indicates whether *windowName* is currently busy.  *WindowName* is the
  name of a Tk widget. Returns "1" the window is busy and "0" otherwise.
  If *windowName* doesn't exist, then "0" is returned.

**blt::busy names** ?\ *pattern* ... ?
  Returns the pathnames of all widgets that have previously been made busy
  (i.e. a busy window is allocated and associated with the widget).  It
  makes no difference if the window is currently busy or not.  If one or
  more *pattern* arguments are present, then the pathname of any busy window
  matching *pattern* will be returned. *Pattern* is a **glob**\-style pattern.

**blt::busy release** ?\ *windowName* ... ?
  Makes *windowName* un-busy. Restores user interactions to the widget
  *windowName* again.  This differs from the **forget** operation in that
  the busy window is not destroyed, but simply unmapped.  *WindowName* must
  be the name of a widget specified in a **hold** operation, otherwise an
  error is reported.

**blt::busy status** *windowName*
  Indicates the busy status of *windowName*.  If *windowName* has a busy
  window and the busy window is currently active (user interactions are
  blocked), "active" is returned.  If *windowName* has a busy window, but
  is not active, then "inactive" is returned.  Otherwise "none" is
  returned.

BINDINGS
--------

The event blocking feature is implemented by creating and mapping a
transparent window that completely covers the widget.  When the busy window
is mapped, it invisibly shields the widget and its hierarchy from all
events that may be sent.  Like Tk widgets, busy windows have widget names
in the Tk window hierarchy.  This means that you can use the **bind**
command, to handle events in the busy window.

 ::

    blt::busy hold .frame.canvas
    bind .frame.canvas_Busy <Enter> { ... } 

Normally the busy window is a sibling of the widget.  The name of the busy
window is "*widget*\ _Busy" where *widget* is the name of the widget to be
made busy.  In the previous example, the pathname of the busy window is
".frame.canvas_Busy". The exception is when the widget is a toplevel
widget (such as ".")  where the busy window can't be made a sibling.  The
busy window is then a child of the widget named "*widget*\ ._Busy" where
*widget* is the name of the toplevel widget.  In the following example, the
pathname of the busy window is "._Busy".

  ::

     blt::busy hold .
     bind ._Busy <Enter> { ... } 

ENTER/LEAVE EVENTS
------------------

Mapping and unmapping busy windows generates Enter/Leave events for all
widgets they cover.  Please note this if you are tracking Enter/Leave
events in widgets.

KEYBOARD EVENTS
---------------

When a widget is made busy, the widget is prevented from gaining the
keyboard focus by the busy window. But if the widget already had focus, it
still may received keyboard events.  To prevent this, you must move focus
to another window.

  ::

     blt::busy hold .frame
     label .dummy
     focus .dummy
     update

The above example moves the focus from .frame immediately after invoking
the **hold** so that no keyboard events will be sent to ".frame" or any of
its descendants.

Tk's tab traversal mechanism should be also modified to check for busy
windows. You can do this by adding a simple test to "tkFocusOK".  Here's
an example.

  ::
     
     tk_focusNext .
     rename tkFocusOK tkFocusOK.orig
     proc tkFocusOK { w }  {
         if { [blt::busy isbusy $w] } {
             return 0
         }
         return [tkFocusOK.orig $w]
     }

EXAMPLE
-------

You can make several widgets busy by simply making its ancestor widget
busy using the **hold** operation.

  ::

        frame .top
        button .top.button; canvas .top.canvas
        pack .top.button .top.canvas
        pack .top
          . . .
        blt::busy hold .top
        update

All the widgets within ".top" (including ".top") are now busy.  
Using **update** insures that **blt::busy** command will take effect before
any other user events can occur.

When the application is no longer busy processing, you can allow user
interactions again by the **release** operation.

 ::

        blt::busy release .top 

The busy window has a configurable cursor.  You can change the busy
cursor using the **configure** operation.

  ::

        blt::busy configure .top -cursor "watch

Finally, when you no longer need to the busy window, 
invoke the **forget** operation to free any resources it allocated.

 ::

        blt::busy forget .top 

Destroying the widget will also clean up any resources allocated by
the busy command.

DIFFERENCES WITH PREVIOUS VERSIONS
----------------------------------

1. The **isbusy** operation no longer takes a pattern argument and returns
   a list of matches window names.  It now takes a single busy window
   argument and returns 1 if busy, 0 otherwise.  This is the same as the
   previous **check** operation.

KEYWORDS
--------

busy, keyboard events, pointer events, window, cursor

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


