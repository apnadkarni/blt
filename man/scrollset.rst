===============
scrollset
===============

-------------------------------------------------
Create and manipulate scrollset widgets
-------------------------------------------------

:Author: gahowlett@gmail.com
:Date:   2012-11-28
:Copyright: George A. Howlett.
    See the file "license.terms" for information on usage and redistribution
    of this file, and for a DISCLAIMER OF ALL WARRANTIES.
:Version: BLT 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

.. TODO: authors and author with name <email>

SYNOPSIS
========

**blt::scrollset** *pathName* ?\ *\ option value*\ ?...

DESCRIPTION
===========

The **scrollset** manages an embedded widget and optionally 
vertical and hortizontal scrollbars.  The scrollbars are automatically 
exposed and hidden as the **scrollset** widget is resized.
Whenever the **scrollset** window is smaller horizontally and/or vertically
than the 
embedded window, the appropiate scrollbar is exposed.
The embedded widget can be any Tk widget.  If the widget
doesn't support standard Tk scrolling capabilities 
(i.e. a **xview** or **yview** operation) 
the widget is scrolled by using 
the scrollset window as a viewport over the embedded widget. 

SYNTAX
======

The **scrollset** command creates a new window named *pathName*
and makes it into a **scrollset** widget.
Additional options may be specified on the command line or in the
option database to configure aspects of the scrollset such as its background
color, the scrollbars, or the embedded widget. The **scrollset** 
command returns its *pathName* argument.  At the time this command is 
invoked, there
must not exist a window named *pathName*, but *pathName*'s
parent must exist.

The scrollbars and embedded widget must be children of the scrollset 
widget.  The scrollbars and embedded widget may be designated 
in the **scrollset** widget before they exist.  The normal size of
of **scrollset** is the requested width and height of the embedded widget
(where no scrollbars are necessary).  You can override the embedded widget's
requested size by setting the scrollset's **-reqwidth** and 
**-reqheight** options respectively.

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
  **configure** operation. They are described in the section 
  `WIDGET OPTIONS`_ below.

*pathName* **configure** ?\ *option*\ ? ?\ *value option value ...*\ ?

  Query or modify the configuration options of the widget.  If no *option* is
  specified, returns a list describing all the available options for
  *pathName* (see **Tk_ConfigureInfo** for information on the format of this
  list).  If *option* is specified with no *value*, then the command returns a
  list describing the one named option (this list will be identical to the
  corresponding sublist of the value returned if no *option* is specified).
  If one or more *option-value* pairs are specified, then the command
  modifies the given widget option(s) to have the given value(s); in this case
  the command returns an empty string.  *Option* and *value* are described
  in the section `WIDGET OPTIONS`_ below.

*pathName* **xset** *first* *last*
 
   Scrolls the scrollset so that the specified portion of the embedded 
   widget is visible in the scrollset window.

*pathName* **xview** *args*

  This command queries or changes the horizontal position of the
  embedded widget in the scrollset's window.  It can take any of the 
  following forms:

  *pathName* **xview**

     Returns a list of two numbers between 0.0 and 1.0 that describe the
     amount and position of the embedded widget that is visible in the
     **scrollset** window.  For example, if *view* is "``0.2 0.6``",
     twenty percent of the embedded widget is off-screen to the left, forty
     percent is visible in the window, and 40 percent of the embedded widget
     is off-screen to the right.  These are the same values passed to
     scrollbars via the **-xscrollcommand** option.

   *pathName* **xview moveto** *fraction*

     Adjusts the view in the window so that *fraction* of the
     total width of the scrollset text is off-screen to the left.
     *fraction* must be a number between 0.0 and 1.0.

   *pathName* **xview scroll** *number what*

     This command shifts the view in the window (left/top or right/bottom)
     according to *number* and *what*.  *Number* must be an
     integer. *What* must be either ``units`` or ``pages`` or an
     abbreviation of these.  If *what* is ``units``, the view adjusts left
     or right by *number* scroll units (see the **-xscrollincrement**
     option); if it is ``pages`` then the view adjusts by *number* widget
     windows.  If *number* is negative then tabs farther to the left become
     visible; if it is positive then tabs farther to the right become visible.


  *pathName* **yset**  *first* *last*

    Scrolls the embedded window vertically so that the tab
    *tab* is visible in the widget's window.

  *pathName* **yview** *args*

    This command queries or changes the vertical position of the
    embedded widget in the scrollset's window.  It can take any of the 
    following

    *pathName* **yview**

       Returns a list of two numbers between 0.0 and 1.0 that describe the
       amount and position of the embedded widget that is visible in the
       **scrollset** window.  For example, if the result is "``0.2 0.6``",
       twenty percent of the embedded widget is off-screen to the top, forth
       percent is visible in the window, and forty percent of the embedded
       widget is off-screen to the bottom.  These are the same values passed
       to scrollbars via the **-yscrollcommand** option.

     *pathName* **yview moveto** *fraction*

	Adjusts the view in the window so that *fraction* of the
	total width of the embedded widget is off-screen to the top.
	*fraction* must be a number between 0.0 and 1.0.

     *pathName* **yview scroll** *number what*

	This command shifts the view in the window (top or bottom) according
	to *number* and *what*.  *Number* must be an integer. *What*
	must be either ``units`` or ``pages`` or an abbreviation of these.
	If *what* is ``units``, the view adjusts left or right by *number*
	scroll units (see the **-yscrollincrement** option); if it is
	``pages`` then the view adjusts by *number* widget windows.  If
	*number* is negative then tabs farther to the left become visible;
	if it is positive then tabs farther to the right become visible.


.. _`WIDGET OPTIONS`:

WIDGET OPTIONS
==============

Widget configuration options may be set either by the **configure** 
operation or the Tk **option** command.  The resource class
is ``Scrollset``.  The resource name is the name of the widget::

  option add *Scrollset.anchor n
  option add *Scrollset.Anchor e


The following widget options are available\:

**-anchor** *anchor* \
  It the **scrollset** window is bigger than the embedded widget, 
  this option specifies how the embedded widget should be positioned 
  within the scrollset. The default is ``center``.

**-background** *color*  \
  Sets the default background for the scrollset widget.  The
  background is normally completely obscurred by the embedded widget.  
  But if both scrollbars are exposed, there will be a square region 
  in lower right corner.  This option defines the color of that region.
  The default is ``grey85``.

**-cursor** *cursor*  \
  Specifies the widget's cursor.  The default cursor is ``""``.

**-fill** *fill*  \
  If the **scrollset** window is bigger than the embedded widget,
  this option specifies how the embedded widget is to be stretched to
  fill the window. *Fill* can be either ``none``, ``x``, ``y``, or
  ``both``.  For example, if *fill* is ``x``, then the widget is stretched
  horizontally.  If *fill* is ``both``, the widget is stretched both
  horizontally and vertically.  The  default is ``both``.

**-height** *pixels*  \
  Specifies the requested height of scrollset widget.  If *pixels* is 0, then
  the height of the widget will be calculated based on the size the embedded
  widget.  The default is ``0``.

**-ipadx** *pad*  \
  Sets the padding to the left and right of the embedded widget.  *Pad* can be
  a list of one or two screen distances.  If *pad* has two elements, the left
  side of the widget is padded by the first distance and the right side by the
  second.  If *pad* has just one distance, both the left and right sides are
  padded evenly.  The default value is ``0``.

**-ipady** *pad*  \
  Sets the padding to the top and bottom of the embedded widget.  *Pad* can be
  a list of one or two screen distances.  If *pad* has two elements, the top
  of the embedded widget is padded by the first distance and the bottom by the
  second.  If *pad* has just one distance, both the top and bottom are padded
  evenly.  The default value is ``0``.

**-padx** *pad*  \
  Sets the padding around the left and right of the embedded widget, if one
  exists.  *Pad* can be a list of one or two screen distances.  If *pad* has
  two elements, the left side of the widget is padded by the first distance
  and the right side by the second.  If *pad* has just one distance, both the
  left and right sides are padded evenly.  The default value is ``0``.

**-pady** *pad*  \
  Sets the padding around the top and bottom of the embedded widget, if one
  exists.  *Pad* can be a list of one or two screen distances.  If *pad* has
  two elements, the top of the widget is padded by the first distance and the
  bottom by the second.  If *pad* has just one distance, both the top and
  bottom sides are padded evenly.  The default value is ``0``.

**-reqheight** *pixels*  \
  If *pixels* is not zero, it specifies the requested height of the
  embedded widget, overriding its the embedded widget's requested height.   
  The default is ``0``.

**-reqwidth** *pixels*  \
  If *pixels* is not zero, it specifies the requested width of the
  embedded widget, overriding the embedded widget's own requested width.  
  The default is ``0``.

**-xscrollbar** *scrollbar*  \
  Specifies the horizontal scrollbar.  If *scrollbar* is the empty string, no
  horizontal scrollbar will be used.  The default is ``""``.

**-xscrollcommand** *string*  \
  Specifies the prefix for a command used to communicate with horizontal
  scrollbars.  Whenever the horizontal view in the widget's window changes,
  the widget will generate a Tcl command by concatenating the scroll command
  and two numbers.  If this option is not specified, then no command will be
  executed.

**-xscrollincrement** *pixels*  \
  Sets the horizontal scrolling distance. The default is 20 pixels.

**-xviewcommand** *command*  \
  Sets the width of the 3-D border around the outside edge of the widget.  The
  **-relief** option determines how the border is to be drawn.  The default is
  ``0``.

**-yscrollbar** *scrollbar*  \
  Specifies the vertical scrollbar.  If *scrollbar* is the empty string, no
  scrollbar will be used.  The default is ``""``.

**-yscrollcommand** *string*  \
  Specifies the prefix for a command used to communicate with vertical
  scrollbars.  Whenever the vertical view in the widget's window changes, the
  widget will generate a Tcl command by concatenating the scroll command and
  two numbers.  If this option is not specified, then no command will be
  executed.

**-yscrollincrement** *pixels*  \
  Sets the vertical scrolling distance. The default is 20 pixels.

**-yviewcommand** *command*  \
  Sets the width of the 3-D border around the outside edge of the widget.  The
  **-relief** option determines how the border is to be drawn.  The default is
  ``0``.

**-width** *pixels*  \
  Specifies the requested width of the scrollset widget.  If *pixels* is 0,
  then the width of the widget will be calculated based on the request size
  embedded widget.  The default is ``0``.

**-window** *pathName*  \
  Specifies the widget to be embedded into the scrollset.  *PathName* must be
  a child of the **scrollset** widget.  The scrollset will "pack" and manage
  the size and placement of *pathName*.  The default value is ``""``.

DEFAULT BINDINGS
================

There are no default class bindings for **scrollset** widgets.

EXAMPLE
=======

You create a scrollset widget with the **scrollset** command.

  ::

    # Create a new scrollset
    scrollset .ts -relief sunken -borderwidth 2 

A new Tcl command ``.ts`` is also created.  This command can be
used to query and modify the scrollset.  For example, to change the
default font used by all the tab labels, you use the new command and
the scrollset's **configure** operation.

  ::

    # Change the default font.
    .ts configure -font "fixed"

You can then add folders using the **insert** operation.

  ::

    # Create a new folder "f1"
    .ts insert 0 "f1"

This inserts the new tab named "f1" into the scrollset.  The index
``0`` indicates location to insert the new tab.  You can also use
the index ``end`` to append a tab to the end of the scrollset.  By
default, the text of the tab is the name of the tab.  You can change
this by configuring the **-text** option.

  ::

    # Change the label of "f1"
    .ts tab configure "f1" -text "Tab #1" 

The **insert** operation lets you add one or more folders at a time.

  ::

    .ts insert end "f2" -text "Tab #2" "f3" "f4" 

The tab on each folder contains a label.  A label may display both
an image and a text string.  You can reconfigure the tab's attributes
(foreground/background colors, font, rotation, etc) using the **tab
configure** operation.

  ::

    # Add an image to the label of "f1"
    set image [image create photo -file stopsign.gif]
    .ts tab configure "f1" -image $image
    .ts tab configure "f2" -rotate 90

Each folder may contain an embedded widget to represent its contents.
The widget to be embedded must be a child of the scrollset widget.  Using
the **-window** option, you specify the name of widget to be
embedded.  But don't pack the widget, the scrollset takes care of placing
and arranging the widget for you.

  ::

    graph .ts.graph
    .ts tab configure "f1" -window ".ts.graph" -fill both -padx 0.25i -pady 0.25i

The size of the folder is determined the sizes of the Tk widgets
embedded inside each folder.  The folder will be as wide as the widest
widget in any folder. The tallest determines the height.  You can use
the tab's **-pagewidth** and **-pageheight** options override this.

Other options control how the widget appears in the folder.  The
**-fill** option says that you wish to have the widget stretch to
fill the available space in the folder.

  ::

    .ts tab configure "f1" -fill both -padx 0.25i -pady 0.25i

Now when you click the left mouse button on "f1", the
graph will be displayed in the folder.  It will be automatically
hidden when another folder is selected.  If you click on the right
mouse button, the embedded widget will be moved into a toplevel widget 
of its own.  Clicking again on the right mouse button puts it back into 
the folder.

If you want to share a page between two different folders, the
**-command** option lets you specify a Tcl command to be invoked
whenever the folder is selected.  You can reset the **-window**
option for the tab whenever it's clicked.

  ::

    .ts tab configure "f2" -command { 
      .ts tab configure "f2" -window ".ts.graph"
    }
    .ts tab configure "f1" -command { 
      .ts tab configure "f1" -window ".ts.graph"
    }

If you have many folders, you may wish to stack tabs in multiple
tiers.  The scrollset's **-tiers** option requests a maximum
number of tiers.   The default is one tier.
 
  ::

    .ts configure -tiers 2

If the tabs can fit in less tiers, the widget will use that many.  
Whenever there are more tabs than can be displayed in the maximum number
of tiers, the scrollset will automatically let you scroll the tabs.  You
can even attach a scrollbar to the scrollset.

  ::
  
    .ts configure -scrollcommand { .sbar set }  -scrollincrement 20
    .sbar configure -orient horizontal -command { .ts view }

By default tabs are along the top of the scrollset from left to right.  
But tabs can be placed on any side of the scrollset using the **-side**
option.

  ::

    # Arrange tabs along the right side of the scrollset. 
    .ts configure -side right -rotate 270

KEYWORDS
========
scrollset, widget
