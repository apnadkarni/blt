===============
blt::scrollset
===============

----------------------------------------
Create and manipulate scrollset widgets.
----------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::scrollset** *pathName* ?\ *option value* ... ?

DESCRIPTION
===========

The **blt::scrollset** widget manages a child widget and optionally
vertical and hortizontal scrollbars.  The scrollbars are automatically
exposed and hidden as the *scrollset* widget is resized.  Whenever the
*scrollset* window is smaller horizontally and/or vertically than the
child window, the appropiate scrollbar is exposed.  The child widget can be
any Tk widget.  If the widget doesn't support standard Tk scrolling
capabilities (i.e. a **xview** or **yview** operation) the widget is
scrolled by using the scrollset window as a viewport over the child widget.

SYNTAX
======

  **blt::scrollset** *pathName* ?\ *option value* ... ?

The **blt::scrollset** command creates a new window named *pathName* and
makes it into a *scrollset* widget.  Additional options may be specified on
the command line or in the option database to configure aspects of the
scrollset such as its background color, the scrollbars, or the child
widget. The **blt::scrollset** command returns its *pathName* argument.  At
the time this command is invoked, there must not exist a window named
*pathName*, but *pathName*'s parent must exist.

The scrollbars and child widget must be children of the scrollset widget.
The scrollbars and child widget may be designated in the *scrollset*
widget before they exist.  The normal size of of *scrollset* is the
requested width and height of the child widget (where no scrollbars are
necessary).  You can override the child widget's requested size by setting
the scrollset's **-reqwidth** and **-reqheight** options respectively.

OPERATIONS
==========

All *scrollset* operations are invoked by specifying the widget's pathname,
the operation, and any arguments that pertain to that operation.  The
general form is:

  *pathName* *operation* ?\ *arg* *arg* ... ?

*Operation* and the *arg*\ s determine the exact behavior of the command.
The following operations are available for scrollset widgets:

*pathName* **cget** *option*  
  Returns the current value of the configuration option given
  by *option*. *Option* may have any of the values accepted by the 
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **configure** ?\ *option*\ ? ?\ *value*\ ? ?\ *option* *value* ... ?
  Query or modify the configuration options of the widget.  If no *option* is
  specified, returns a list describing all the available options for
  *pathName* (see **Tk_ConfigureInfo** for information on the format of this
  list).  If *option* is specified with no *value*, then the command returns a
  list describing the one named option (this list will be identical to the
  corresponding sublist of the value returned if no *option* is specified).
  If one or more *option-value* pairs are specified, then the command
  modifies the given widget option(s) to have the given value(s); in this case
  the command returns an empty string.  *Option* and *value* are described
  below.

  Widget configuration options may be set either by the **configure** 
  operation or the Tk **option** command.  The resource class
  is **BltScrollset**.  The resource name is the name of the widget::

    option add *BltScrollset.anchor n
    option add *BltScrollset.Anchor e

  The following widget options are available:

  **-anchor** *anchorName* 
    It the **scrollset** window is bigger than the child widget, this
    option specifies how the child widget should be positioned within the
    scrollset. The default is "center".

  **-background** *bgName*  
    Sets the default background for the scrollset widget.  The background is
    normally completely obscurred by the child widget.  But if both
    scrollbars are exposed, there will be a square region in lower right
    corner.  This option defines the color of that region.  The default is
    "grey85".
    FIXME
    
  **-cursor** *cursor*  
    Specifies the widget's cursor.  The default cursor is "".

  **-fill** *fillMode*  
    If the **scrollset** window is bigger than the child widget,
    this option specifies how the child widget is to be stretched to
    fill the window. *FillMode* can be either "none", "x", "y", or
    "both".  For example, if *fillMode* is "x", then the widget is stretched
    horizontally.  If *fillMode* is "both", the widget is stretched both
    horizontally and vertically.  The  default is "both".

  **-height** *numPixels*  
    Specifies the requested height of scrollset widget.  If *numPixels* is
    0, then the height of the widget will be calculated based on the size
    the child widget.  The default is "0".

  **-ipadx** *numPixels*  
    Sets the padding to the left and right of the child widget.
    *NumPixels* can be a list of one or two screen distances.  If
    *numPixels* has two elements, the left side of the widget is padded by
    the first distance and the right side by the second.  If *numPixels*
    has just one distance, both the left and right sides are padded evenly.
    The default value is "0".

  **-ipady** *numPixels*  
    Sets the padding to the top and bottom of the child widget.
    *NumPixels* can be a list of one or two screen distances.  If
    *numPixels* has two elements, the top of the child widget is padded by
    the first distance and the bottom by the second.  If *numPixels* has
    just one distance, both the top and bottom are padded evenly.  The
    default value is "0".

  **-padx** *numPixels*  
    Sets the padding around the left and right of the child widget, if one
    exists.  *NumPixels* can be a list of one or two screen distances.  If
    *numPixels* has two elements, the left side of the widget is padded by
    the first distance and the right side by the second.  If *numPixels*
    has just one distance, both the left and right sides are padded evenly.
    The default value is "0".

  **-pady** *numPixels*  
    Sets the padding around the top and bottom of the child widget, if one
    exists.  *NumPixels* can be a list of one or two screen distances.  If
    *numPixels* has two elements, the top of the widget is padded by the
    first distance and the bottom by the second.  If *numPixels* has just
    one distance, both the top and bottom sides are padded evenly.  The
    default value is "0".

  **-reqheight** *numPixels*  
    If *numPixels* is not zero, it specifies the requested height of the
    child widget, overriding its the child widget's requested height.   
    The default is "0".

  **-reqwidth** *numPixels*  
    If *pixels* is not zero, it specifies the requested width of the
    child widget, overriding the child widget's own requested width.  
    The default is "0".

  **-xscrollbar** *scrollbarName*  
    Specifies the horizontal scrollbar.  If *scrollbarName* is the empty
    string, no horizontal scrollbar will be used.  The default is "".

  **-xscrollcommand** *cmdPrefix*  
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window changes,
    the widget will generate a TCL command by concatenating the scroll command
    and two numbers.  If this option is not specified, then no command will be
    executed.

  **-xscrollincrement** *numPixels*  
    Sets the horizontal scrolling distance. The default is 20 pixels.

  **-xviewcommand** *cmdPrefix*  
    Sets the width of the 3-D border around the outside edge of the widget.
    The **-relief** option determines how the border is to be drawn.  The
    default is "0".

  **-yscrollbar** *scrollbarName*  
    Specifies the vertical scrollbar.  If *scrollbarName* is the empty string,
    no scrollbar will be used.  The default is "".

  **-yscrollcommand** *cmdPrefix*  
    Specifies the prefix for a command used to communicate with vertical
    scrollbars.  Whenever the vertical view in the widget's window changes, the
    widget will generate a TCL command by concatenating the scroll command and
    two numbers.  If this option is not specified, then no command will be
    executed.

  **-yscrollincrement** *numPixels*  
    Sets the vertical scrolling distance. The default is 20 pixels.

  **-yviewcommand** *cmdPrefix*  
    Sets the width of the 3-D border around the outside edge of the widget.  The
    **-relief** option determines how the border is to be drawn.  The default is
    "0".

  **-width** *numPixels*  
    Specifies the requested width of the scrollset widget.  If *numPixels* is 0,
    then the width of the widget will be calculated based on the request size
    child widget.  The default is "0".

  **-window** *pathName*  
    Specifies the widget to be embedded into the scrollset.  *PathName* must
    be a child of the **scrollset** widget.  The scrollset will "pack" and
    manage the size and placement of *pathName*.  The default value is "".

*pathName* **xset** *firstPos* *lastPos*
   Scrolls the scrollset so that the specified portion of the child 
   widget is visible in the scrollset window.

*pathName* **xview**
  Returns a list of two numbers between 0.0 and 1.0 that describe the
  amount and position of the child widget that is visible in the
  **scrollset** window.  For example, if *view* is "0.2 0.6", twenty
  percent of the child widget is off-screen to the left, forty percent is
  visible in the window, and 40 percent of the child widget is off-screen
  to the right.  These are the same values passed to scrollbars via the
  **-xscrollcommand** option.

*pathName* **xview moveto** *fraction*
  Adjusts the view in the window so that *fraction* of the
  total width of the scrollset text is off-screen to the left.
  *fraction* must be a number between 0.0 and 1.0.

*pathName* **xview scroll** *number what*
  This command shifts the view in the window (left/top or right/bottom)
  according to *number* and *what*.  *Number* must be an integer. *What*
  must be either "units" or "pages" or an abbreviation of these.  If *what*
  is "units", the view adjusts left or right by *number* scroll units (see
  the **-xscrollincrement** option); if it is "pages" then the view adjusts
  by *number* widget windows.  If *number* is negative then tabs farther to
  the left become visible; if it is positive then tabs farther to the right
  become visible.


*pathName* **yset**  *first* *last*
  Scrolls the child window vertically so that the tab *tab* is visible in
  the widget's window.

*pathName* **yview**
  Returns a list of two numbers between 0.0 and 1.0 that describe the
  amount and position of the child widget that is visible in the
  **scrollset** window.  For example, if the result is "0.2 0.6", twenty
  percent of the child is off-screen to the top, forth percent is visible
  in the window, and forty percent of the child widget is off-screen to
  the bottom.  These are the same values passed to scrollbars via the
  **-yscrollcommand** option.

*pathName* **yview moveto** *fraction*
  Adjusts the view in the window so that *fraction* of the
  total width of the child widget is off-screen to the top.
  *fraction* must be a number between 0.0 and 1.0.

*pathName* **yview scroll** *number what*
  This command shifts the view in the window (top or bottom) according to
  *number* and *what*.  *Number* must be an integer. *What* must be
  either "units" or "pages" or an abbreviation of these.  If *what* is
  "units", the view adjusts left or right by *number* scroll units (see
  the **-yscrollincrement** option); if it is "pages" then the view
  adjusts by *number* widget windows.  If *number* is negative then tabs
  farther to the left become visible; if it is positive then tabs farther
  to the right become visible.

DEFAULT BINDINGS
----------------

There are no default class bindings for **scrollset** widgets.

EXAMPLE
-------

You create a scrollset widget with the **scrollset** command.

  ::

    package require BLT

    # Create a new scrollset
    blt::scrollset .ss  

A new TCL command ".ss" is also created.  This command can be
used to query and modify the scrollset.  For example, you can specify
the scrollbars and child widget to use with the scrollset's 
**configure** operation.

  ::

    .ss configure -xscrollbar .ss.xsbar -yscrollbar .ss.ysbar -window .ss.g 
    blt::tk::scrollbar .ss.ysbar 
    blt::tk::scrollbar .ss.xsbar 
    blt::graph .ss.g 

Note that

  1. The scrollbars and child widget are children of the
     scrollset widget.  
  2. The scrollbars and child widget do not have to exist before you create 
     the scrollset instance.
  3. You don't have to specify the orientation of the scrollbars 
     (the scrollbar's **-orient** option is set for you).

You can then pack the scrollset as usual.

KEYWORDS
========
scrollset, widget

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
