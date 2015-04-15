
===============
blt::busy
===============

-------------------------------------------------------------
Make Tk widgets busy, temporarily blocking user interactions.
-------------------------------------------------------------

:Author: gahowlett@gmail.com
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

.. TODO: authors and author with name <email>

SYNOPSIS
--------

**blt::busy hold** *window* ?\ *option* *value* ...\ ?

**blt::busy release**  ?\ *window* ...\ ?

**blt::busy configure** *window* ?\ *option* *value* ...\ ?

**blt::busy forget**  ?\ *window* ...\ ?

**blt::busy isbusy** *window* 

**blt::busy names** ?\ *pattern* ...\ ?

**blt::busy status** *window* 

DESCRIPTION
-----------

The **blt::busy** command provides a simple means to block
keyboard, button, and pointer events from Tk widgets, while overriding
the widget's cursor with a configurable busy cursor.

INTRODUCTION
------------

There are many times in applications where you want to temporarily
restrict what actions the user can take.  For example, an application
could have a "run" button that when pressed causes some processing to
occur.  But while the application is busy processing, you probably don't
want the the user to be able to click the "run" button again.  You
may also want restrict the user from other tasks such as clicking a
"print" button.

The **blt::busy** command lets you make Tk widgets busy. This means
that user interactions such as button clicks, moving the mouse, typing
at the keyboard, etc. are ignored by the widget.  You can set a
special cursor (like a watch) that overrides the widget's normal
cursor, providing feedback that the application (widget) is
temporarily busy.

When a widget is made busy, the widget and all of its descendents will
ignore events.  It's easy to make an entire panel of widgets busy. You
can simply make the toplevel widget (such as ".") busy.  This is
easier and far much more efficient than recursively traversing the
widget hierarchy, disabling each widget and re-configuring its cursor.

The **blt::busy** command isn't a replacement for the Tk **grab** command.
There are times where the **blt::busy** command can be used instead of Tk's
**grab** command.  Unlike **grab** which restricts all user interactions to
one widget, with the busy command you can have more than one widget active
(for example, a "cancel" dialog and a "help" button).


OPERATIONS
----------

The following operations are available for the **blt::busy** command:

**blt::busy hold** *window* ?\ *option* *value* ...\ ?

  Makes the widget *window* (and its descendants in the Tk window
  hierarchy) busy.  *Window* must be a valid path name of a Tk widget.  The
  busy window is mapped the next time idle tasks are processed, and the
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

**blt::busy configure** *window* ?\ *option* *value* ...\ ?

  Queries or modifies the **blt::busy** command configuration options for
  *window*. *Window* must be the path name of a widget previously made busy
  by the **hold** operation.  If no options are specified, a list
  describing all of the available options for *window* (see
  **Tk_ConfigureInfo** for information on the format of this list) is
  returned.  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*\ -*value* pairs are
  specified, then the command modifies the given widget option(s) to have
  the given value(s); in this case the command returns the empty string.
  *Option* may have any of the values accepted by the **hold** operation.

  Please note that the option database is referenced through *window*.  For
  example, if the widget ".frame" is to be made busy, the busy cursor can
  be specified for it by either **option** command:

  ::

	"option add *frame.busyCursor gumby"
	"option add *Frame.BusyCursor gumby"

**blt::busy active**  ?\ *pattern* ...\ ?

  Returns the pathnames of all widgets that are currently busy (active).
  If a *pattern* is given, the path names of busy widgets matching
  *pattern* are returned.  This differs from the **names** operation in that
  **active** only returns the names of windows where the busy window is
  currently active (**names** returns all busy windows).

**blt::busy forget** ?\ *window* ...\ ?

  Releases resources allocated by the busy command for *window*, including
  the busy window.  User events will again be received again by *window*.
  Resources are also released when *window* is destroyed. *Window* must be
  the name of a widget specified in the **hold** operation, otherwise an
  error is reported.

**blt::busy check** *window*

  Checks if *window* or any of its ancestors are currently busy.  If
  *window* is presently busy (it can not receive user interactions) "1" is
  returned, otherwise "0".

**blt::busy isbusy** *window*

  Indicates whether *window* is currently busy.  *Window* is the name of a
  Tk widget. Returns "1" the window is busy and "0" otherwise.  If *window*
  doesn't exist, then "0" is returned.

**blt::busy names** ?\ *pattern* ...\ ?

  Returns the pathnames of all widgets that have previously been made busy
  (i.e. a busy window is allocated and associated with the widget).  It
  makes no difference if the window is currently busy or not.  If a
  *pattern* is given, the path names of busy widgets matching *pattern* are
  returned.

**blt::busy release** ?\ *window* ...\ ?

  Makes the *window* un-busy. Restores user interactions to the widget
  *window* again.  This differs from the **forget** operation in that the
  busy window is not destroyed, but simply unmapped.  *Window* must be the
  name of a widget specified in a **hold** operation, otherwise an error is
  reported.

**blt::busy status** *window*

  Indicates the busy status of *window*.  If *window* has a busy window and
  the busy window is currently active (user interactions are blocked),
  "active" is returned.  If *window* has a busy window, but is not active,
  then "inactive" is returned.  Otherwise "none" is returned.

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
         if { [blt::busy check $w] } {
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

KEYWORDS
--------

busy, keyboard events, pointer events, window, cursor


