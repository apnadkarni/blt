===============
blt::winop
===============

----------------------------------
Perform assorted window operations
----------------------------------


:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::winop geometry** *windowName*

**blt::winop lower** ? *windowName* ... ?

**blt::winop map** ? *windowName* ... ?

**blt::winop move** *windowName* *x* *y*

**blt::winop query** 

**blt::winop raise** ?\ *windowName* ... ?

**blt::winop top** *x* *y*

**blt::winop tree** *windowName* *treeName*

**blt::winop unmap** ?\ *windowName* ... ?

**blt::winop warpto** ?\ *windowName*\ ?

DESCRIPTION
-----------

The **blt::winop** command performs various window operations on Tk
windows using low-level Xlib function calls to work around window
manager pecularities.

INTRODUCTION
------------

Tk has several commands for manipulating its windows: **raise**, **lower**,
**wm**, etc.  These commands ask the window manager to perform operations
on Tk windows.  In some cases, a particular window manager won't perform
the operation as expected.

For example, if you positioned a toplevel window using **wm geometry**, the
window may not actually be at those particular coordinates.  The position
of the window may be offset by dimensions of the title bar added by the
window manager.

In situations like these, the **blt::winop** command can be used to
workaround these difficulties.  Instead, it makes low-level Xlib (such
**XRaiseWindow** and **XMapWindow**) calls to perform these operations.

REFERENCING WINDOWS
-------------------

Windows can be referenced by their path name if they are a Tk window, or
their Window ID if they are a window belonging to another application.
Tk window path names always start with a period (such as ".top"), while
Window IDs are prefixed by "0x" (such as "0xc00051").
  

OPERATIONS
----------

The following operations are available for the **blt::winop** command:

**blt::winop geometry** *windowName*
  Returns the geometry of *windowName*.  The list returned will contain 4
  numbers: the x, y screen coordinates of the upper-left position of the
  window relative to the root window and the width and height of the
  window.

**blt::winop lower** ?\ *windowName* ...  ?
  Lowers *windowName* to the bottom of the window stacking order.  

**blt::winop map** ?\ *windowName* ... ?
  Maps *windowName* on the screen.  When a window is mapped it is drawn on
  the screen.  If *windowName* is already mapped, this command has no
  effect.

**blt::winop move** *windowName* *x* *y*
  Move *windowName* to the screen location specified by *x* and *y*.  *X*
  and *y* are screen distances. They may be in any of the forms acceptable
  to **Tk_GetPixels**.  Negative *x* values move the window left. Negative
  *y* values move the window up.

**blt::winop query** 
  Returns the position of the mouse pointer.  This command returns a list
  of the x and y screen coordinates where the mouse is currently located.
  The coordinates are relative to the root window.

**blt::winop raise** ?\ *windowName*\ ... ?
  Raises one of more windows to the top of the window stack.
  *WindowName* must be the path name of a Tk window or a numeric
  window Id.  This command returns the empty string.

**blt::winop top** *x* *y*
  Returns the pathname or window ID of the topmost window at the given
  screen location. *X* and *y* are screen coordinates relative to the the
  root window.  If no window is at the location, "" is returned.

**blt::winop tree** *windowName* *treeName*
  Fills the BLT tree object *treeName* with the window hierarchy starting
  from *windowName*. *TreeName* is the name of a tree object created by
  the **blt::tree** command.
  
**blt::winop unmap** ?\ *windowName*  ... ?
  Unmaps one or more windows from the screen. When a window is unmapped it
  is hidden.  *WindowName* is the path name of a Tk window or a numeric
  window ID.

**blt::winop warpto** ?\ *windowName*\ ?
  Warps the mouse pointer to *windowName*. *WindowName* is the path name
  of a Tk window, a numeric window ID, in the form "**@**\ *x*,*y*"
  where *x* and *y* are root screen coordinates. In the last case,
  the pointer is warped to that location on the screen.

   [I've never heard a good case for warping the pointer in an application.
   It can be useful for testing, but in applications, it's always a bad
   idea.  Simply stated, the user owns the pointer, not the application.
   If you have an application that needs it, I'd like to hear about it.]

  If no *windowName* argument is present the current location of the pointer is
  returned. The location is returned as a list in the form "*x y*", where
  *x* and *y* are the current coordinates of the pointer.

EXAMPLE
-------

 ::

    toplevel .top
    wm withdraw .top

    # Set the geometry to make the window manager 
    # place the window.
    wm geometry .top +100+100

    # Move the window to the desired location
    # and "update" to force the window manager
    # to recognize it.
    blt::winop move .top 100 100
    update 

    wm deiconify .top
    blt::winop move .top 100 100

DIFFERENCES WITH PREVIOUS VERSIONS
----------------------------------

1. The **snap** operation has been moved to the BLT picture image.


KEYWORDS
--------

window, map, raise, lower, pointer, warp
