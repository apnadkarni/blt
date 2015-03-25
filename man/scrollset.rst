===============
blt::scrollset
===============

-------------------------------------------------
Create and manipulate scrollset widgets
-------------------------------------------------

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

**blt::scrollset** *pathName* ?\ *option value* ...\ ?

DESCRIPTION
===========

The **scrollset** manages a child widget and optionally vertical and
hortizontal scrollbars.  The scrollbars are automatically exposed and
hidden as the **scrollset** widget is resized.  Whenever the **scrollset**
window is smaller horizontally and/or vertically than the child window, the
appropiate scrollbar is exposed.  The child widget can be any Tk widget.
If the widget doesn't support standard Tk scrolling capabilities (i.e. a
**xview** or **yview** operation) the widget is scrolled by using the
scrollset window as a viewport over the child widget.

SYNTAX
======

**blt::scrollset** *pathName* ?\ *option value* ...\ ?

The **scrollset** command creates a new window named *pathName*
and makes it into a **scrollset** widget.
Additional options may be specified on the command line or in the
option database to configure aspects of the scrollset such as its background
color, the scrollbars, or the child widget. The **scrollset** 
command returns its *pathName* argument.  At the time this command is 
invoked, there
must not exist a window named *pathName*, but *pathName*'s
parent must exist.

The scrollbars and child widget must be children of the scrollset widget.
The scrollbars and child widget may be designated in the **scrollset**
widget before they exist.  The normal size of of **scrollset** is the
requested width and height of the child widget (where no scrollbars are
necessary).  You can override the child widget's requested size by
setting the scrollset's **-reqwidth** and **-reqheight** options
respectively.

OPERATIONS
==========

All **scrollset** operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for scrollset widgets:

*pathName* **cget** *option*  

  Returns the current value of the configuration option given
  by *option*. *Option* may have any of the values accepted by the 
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **configure** ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?

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
  is "BltScrollset".  The resource name is the name of the widget::

    option add *BltScrollset.anchor n
    option add *BltScrollset.Anchor e

  The following widget options are available\:

  **-anchor** *anchor* 

    It the **scrollset** window is bigger than the child widget, this
    option specifies how the child widget should be positioned within the
    scrollset. The default is "center".

  **-background** *color*  

    Sets the default background for the scrollset widget.  The background is
    normally completely obscurred by the child widget.  But if both
    scrollbars are exposed, there will be a square region in lower right
    corner.  This option defines the color of that region.  The default is
    "grey85".

  **-cursor** *cursor*  

    Specifies the widget's cursor.  The default cursor is "".

  **-fill** *fill*  

    If the **scrollset** window is bigger than the child widget,
    this option specifies how the child widget is to be stretched to
    fill the window. *Fill* can be either "none", "x", "y", or
    "both".  For example, if *fill* is "x", then the widget is stretched
    horizontally.  If *fill* is "both", the widget is stretched both
    horizontally and vertically.  The  default is "both".

  **-height** *pixels*  

    Specifies the requested height of scrollset widget.  If *pixels* is 0, then
    the height of the widget will be calculated based on the size the child
    widget.  The default is "0".

  **-ipadx** *pad*  

    Sets the padding to the left and right of the child widget.  *Pad* can be
    a list of one or two screen distances.  If *pad* has two elements, the left
    side of the widget is padded by the first distance and the right side by the
    second.  If *pad* has just one distance, both the left and right sides are
    padded evenly.  The default value is "0".

  **-ipady** *pad*  

    Sets the padding to the top and bottom of the child widget.  *Pad* can be
    a list of one or two screen distances.  If *pad* has two elements, the top
    of the child widget is padded by the first distance and the bottom by the
    second.  If *pad* has just one distance, both the top and bottom are padded
    evenly.  The default value is "0".

  **-padx** *pad*  

    Sets the padding around the left and right of the child widget, if one
    exists.  *Pad* can be a list of one or two screen distances.  If *pad* has
    two elements, the left side of the widget is padded by the first distance
    and the right side by the second.  If *pad* has just one distance, both the
    left and right sides are padded evenly.  The default value is "0".

  **-pady** *pad*  

    Sets the padding around the top and bottom of the child widget, if one
    exists.  *Pad* can be a list of one or two screen distances.  If *pad* has
    two elements, the top of the widget is padded by the first distance and the
    bottom by the second.  If *pad* has just one distance, both the top and
    bottom sides are padded evenly.  The default value is "0".

  **-reqheight** *pixels*  

    If *pixels* is not zero, it specifies the requested height of the
    child widget, overriding its the child widget's requested height.   
    The default is "0".

  **-reqwidth** *pixels*  

    If *pixels* is not zero, it specifies the requested width of the
    child widget, overriding the child widget's own requested width.  
    The default is "0".

  **-xscrollbar** *scrollbar*  

    Specifies the horizontal scrollbar.  If *scrollbar* is the empty string, no
    horizontal scrollbar will be used.  The default is "".

  **-xscrollcommand** *string*  

    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window changes,
    the widget will generate a Tcl command by concatenating the scroll command
    and two numbers.  If this option is not specified, then no command will be
    executed.

  **-xscrollincrement** *pixels*  

    Sets the horizontal scrolling distance. The default is 20 pixels.

  **-xviewcommand** *command*  

    Sets the width of the 3-D border around the outside edge of the widget.  The
    **-relief** option determines how the border is to be drawn.  The default is
    "0".

  **-yscrollbar** *scrollbar*  

    Specifies the vertical scrollbar.  If *scrollbar* is the empty string, no
    scrollbar will be used.  The default is "".

  **-yscrollcommand** *string*  

    Specifies the prefix for a command used to communicate with vertical
    scrollbars.  Whenever the vertical view in the widget's window changes, the
    widget will generate a Tcl command by concatenating the scroll command and
    two numbers.  If this option is not specified, then no command will be
    executed.

  **-yscrollincrement** *pixels*  

    Sets the vertical scrolling distance. The default is 20 pixels.

  **-yviewcommand** *command*  

    Sets the width of the 3-D border around the outside edge of the widget.  The
    **-relief** option determines how the border is to be drawn.  The default is
    "0".

  **-width** *pixels*  

    Specifies the requested width of the scrollset widget.  If *pixels* is 0,
    then the width of the widget will be calculated based on the request size
    child widget.  The default is "0".

  **-window** *pathName*  

    Specifies the widget to be child into the scrollset.  *PathName* must
    be a child of the **scrollset** widget.  The scrollset will "pack" and
    manage the size and placement of *pathName*.  The default value is "".

*pathName* **xset** *first* *last*
 
   Scrolls the scrollset so that the specified portion of the child 
   widget is visible in the scrollset window.

*pathName* **xview** *args*

  This command queries or changes the horizontal position of the
  child widget in the scrollset's window.  It can take any of the 
  following forms:

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
    according to *number* and *what*.  *Number* must be an
    integer. *What* must be either "units" or "pages" or an abbreviation
    of these.  If *what* is "units", the view adjusts left or right by
    *number* scroll units (see the **-xscrollincrement** option); if it
    is "pages" then the view adjusts by *number* widget windows.  If
    *number* is negative then tabs farther to the left become visible; if
    it is positive then tabs farther to the right become visible.


*pathName* **yset**  *first* *last*

  Scrolls the child window vertically so that the tab *tab* is visible in
  the widget's window.

*pathName* **yview** *args*

  This command queries or changes the vertical position of the child
  widget in the scrollset's window.  It can take any of the following

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
================

There are no default class bindings for **scrollset** widgets.

EXAMPLE
=======

You create a scrollset widget with the **scrollset** command.

  ::

    package require BLT

    # Create a new scrollset
    blt::scrollset .ss  

A new Tcl command ".ss" is also created.  This command can be
used to query and modify the scrollset.  For example, you can specify
the scrollbars and child widget to use with the scrollset's 
**configure** operation.

  ::

    .ss configure -xscrollbar .ss.xsbar -yscrollbar .ss.ysbar -window .ss.g 
    blt::tk::scrollbar .ss.ysbar 
    blt::tk::scrollbar .ss.xsbar 
    blt::graph .ss.g 

Note that

  - The scrollbars and child widget are children of the
    scrollset widget.  

  - The scrollbars and child widget do not have to exist before you create 
    the scrollset instance.

  - You don't have to specify the orientation of the scrollbars 
    (the scrollbar's **-orient** option is set for you).

You can then pack the scrollset as usual.

KEYWORDS
========
scrollset, widget
