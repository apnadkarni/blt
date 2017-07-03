===============
blt::comboframe
===============

----------------------------------------
Create and manipulate comboframe widgets.
----------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::comboframe** *pathName* ?\ *option value* ... ?

DESCRIPTION
-----------

The **blt::comboframe** command creates and manages *comboframe* widgets.
A *comboframe* widget provides a shell for custom drop-down menus. The
**-window** option lets you specify a widget to use as the menu. The widget
can be a Tk **frame** widget that you pack other widgets into.

Like the **blt::combomenu**, the *comboframe* works with the
**blt::comboentry** or **blt::combobutton** widgets to expose and hide the
menu as needed. Unlike the *combomenu*, the *comboframe* items are district
widgets create and assembled by user code.

SYNTAX
------

The **blt::comboframe** command creates a new window using the *pathName*
argument and makes it into a *comboframe* widget.  The command has the
following form.

  **blt::comboframe** *pathName* ?\ *option value* ... ?

Additional options may be specified on the command line or in the option
database to configure aspects of the comboframe such as its background color
or scrollbars. The **blt::comboframe** command returns its *pathName*
argument.  There must not already exist a window named *pathName*, but
*pathName*'s parent must exist.

The *comboframe* embeds a Tk widget that must be a child of the
*comboframe* widget.  Typically this is a Tk frame that the user populates
with other widgets.  The normal size of the *comboframe* is the requested
size of the embedded widget. You can override this size by specifying the
maximum and minimum width and height of the *comboframe* window using the
**-width** and **-height** widget options.

MENU ITEMS
----------

A menu is a widget that displays a collection of item arranged in one or
more columns.  There exist several different types of items (specified by
the item's **-type** option), each with different properties.  Items of
different types may be combined in a single menu.  Menu items are not
distinct widgets; the entire *comboframe* is one widget.


If an item's **-command** option is specified, a TCL command will be invoke
whenever the item is selected (typically by clicking on the item).
  
OPERATIONS
----------

All *comboframe* operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *comboframe* widgets:

*pathName* **activate** *string* 
  Invokes the **-activatecommand** command appending *string* as an
  argument.  The string is normally the text from the **blt::combobutton**
  or **blt::comboentry** widgets.  This is used by user code initialize the
  widgets insided of the drop-down menu to the desired values.  It is the
  the same string as passed via the **invoke** operation.

*pathName* **cget** *option*  
  Returns the current value of the widget configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **configure** ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of the widget.  If no
  *option* is specified, this command returns a list describing all the
  available options for *pathName* (see **Tk_ConfigureInfo** for
  information on the format of this list).  If *option* is specified with
  no *value*, then a list describing the one named option (this list will
  be identical to the corresponding sublist of the value returned if no
  *option* is specified) is returned.  If one or more *option-value* pairs
  are specified, then this command modifies the given widget option(s) to
  have the given value(s); in this case the command returns an empty
  string.  *Option* and *value* are described below.

  Widget configuration options may be set either by the **configure**
  operation or the Tk **option** command.  The resource class is
  **BltComboframe**.  The resource name is the name of the widget::

    option add *BltComboframe.anchor n
    option add *BltComboframe.Anchor e

  The following widget options are available\:

  **-activatecommand** *cmdPrefix* 
    Specifies the prefix for a TCL command used to communicate with the
    user defined widgets.  After the menu is posted by the
    **blt::combobutton** or **blt::comboentry** the **activate** command is
    called. It will have as an argument a string indicating how to
    initialize the widget.  This is the same string given to the **invoke**
    operation.  If *cmdString* is "", then no command is invoked. The
    default is "".

  **-anchor** *anchorName* 
    Specifies how to position the embedded Tk widget in *pathName* if extra
    space is available. For example, if *anchorName* is "center" then the
    widget is centered in the *pathName*; if *anchorName* is "n" then the
    widget will be drawn such that the top center point of the pane will be
    the top center point of the pane.  This option defaults to "c".
    
  **-background** *bgName* 
    Specifies the background of the menu items.  *BgName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of the menu.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the menu.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-class** *className* 

  **-command** *cmdString* 
    Specifies a TCL command to be invoked when a menu item is selected:
    either by clicking on the menu item or using the **select** operation.
    If *cmdString* is "", then no command is invoked. The default is "".

  **-cursor** *cursorName* 
    Specifies the cursor to be used for the widget. *CursorName* may have
    any of the forms acceptable to **Tk_GetCursor**.  If *cursorName* is "",
    this indicates that the widget should defer to its parent for cursor
    specification.  The default is "".

  **-fill** *fillName* 
    If the *comboframe* widget is bigger than the menu (the embedded child
    widget), then *fillName* specifies if the child widget should be
    stretched to occupy the extra space.  *FillName* can be one of the
    following:

    **x**
      The width of the embedded widget is expanded to fill the window.
    **y**
      The height of the embedded widget is expanded to fill the window.
    **both**
      Both the width and height of the embedded widget are expanded.
    **none**
      The embedded widget not resized.

    The default is "both".

  **-height** *numPixels* 
    Specifies the height in the *comboframe*.  *NumPixels* can be single
    value or a list.  If *numPixels* is a single value it is a non-negative
    value indicating the height the menu. The value may have any of the
    forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is a 2 element list, then this sets the minimum and maximum
    limits for the height of the menu. The menu will be at least the
    minimum height and less than or equal to the maximum. If *numPixels* is
    a 3 element list, then this specifies minimum, maximum, and nominal
    height or the menu.  The nominal size overrides the calculated height
    of the menu.  If *numPixels* is "", then the height of the menu is
    the requested size of the embedded widget.  The default is "".

  **-highlightbackground** *bgName*
    Specifies the color of the traversal highlight region when the
    *comboframe* does not have the input focus.  *BgName* may be a color
    name or the name of a background object created by the
    **blt::background** command.  The default is "grey85".

  **-highlightcolor** *bgName*
    Specifies the color of the traversal highlight region when the graph
    has input focus.  *BgName* may be a color name or the name of a
    background object created by the **blt::background** command. The
    default is "black".
    
  **-highlightthickness** *numPixels*
    Specifies a non-negative value for the width of the highlight rectangle
    to drawn around the outside of the widget.  *NumPixels* may have any of
    the forms acceptable to **Tk_GetPixels**.  If *numPixels* is "0.0", no
    focus highlight is drawn around the widget.  The default is "2".

  **-padx**  *numPixels*
    Sets the padding to the left and right of the embedded child widget.
    *NumPixels* can be a list of one or two screen distances.  If
    *numPixels* has two elements, the left side of the child is padded by
    the first distance and the right side by the second.  If *numPixels*
    has just one distance, both the left and right sides are padded evenly.
    The default is "0".

  **-pady**  *numPixels*
    Sets the padding above and below the embedded child widget.
    *NumPixels* can be a list of one or two screen distances.  If
    *numPixels* has two elements, the area above the child is padded by the
    first distance and the area below by the second.  If *numPixels* is
    just one distance, both the top and bottom areas are padded evenly.
    The default is "4".

  **-postcommand** *cmdString* 
    Specifies a TCL command to invoked when the menu is posted.  The
    command will be invoked before the menu is displayed onscreen.  For
    example, this may be used to create and initialize widgets inside the
    embedded child widget. If *cmdString* is "", no command is invoked.
    The default is "".

  **-relief** *reliefName* 
    Specifies the 3-D effect for the menu.  *ReliefName* indicates how the
    widget should appear relative to the root window. Acceptable values are
    **raised**, **sunken**, **flat**, **ridge**, **solid**, and
    **groove**. For example, "raised" means the menu should appear to
    protrude.  The default is "raised".

  **-restrictwidth** *optionName* 
    Specifies how the menu width should be restricted according to the
    parent widget that posted it. *OptionName* can be one of the following
    "none".

    **max**
      The menu width will be the maximum of the calculated menu width and
      the parent widget width.

    **min**
      The menu width will be the minimum of the calculated menu width and
      the parent widget width.

    **both**
      The menu width will the same as the parent widget width.

    **none**
      Don't restrict the menu width. This is the default.
       
  **-takefocus** *boolean*
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *boolean* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts make the
    decision whether to focus on the window.  The default is "".

  **-textvariable** *varName* 
    Specifies the name of a global TCL variable that contains the label of
    the selected item.  *VarName* is the name of a TCL variable. When an
    item is selected, *varName* is set to its label.  If *varName* is
    "", no variable is set. The default is "".

  **-unpostcommand** *cmdString*
    Specifies the TCL command to be invoked when the menu is unposted.  If
    *cmdString* is "", no command is invoked. The default is "".

  **-valuevariable** *varName* 
    Specifies the name of a global TCL variable that contains the value of
    the selected item.  *VarName* is the name of a TCL variable. When an
    item is selected, *varName* is set to its value. If *varName* is "",
    no variable is set. The default is "".

  **-width** *numPixels*
    Specifies the width in the *comboframe*.  *NumPixels* can be single
    value or a list.  If *numPixels* is a single value it is a non-negative
    value indicating the width the menu. The value may have any of the
    forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is a 2 element list, then this sets the minimum and maximum
    limits for the width of the menu. The menu will be at least the minimum
    width and less than or equal to the maximum. If *numPixels* is a 3
    element list, then this specifies minimum, maximum, and nominal width
    or the menu.  The nominal size overrides the calculated width of the
    menu.  If *numPixels* is "", then the width of the menu is calculated
    based on the widths of all the menu items.  The default is "".

*pathName* **deactivate** 
  Redisplays all menu items using their normal colors.  This typically is
  used by widget bindings to un-highlight menu items as the pointer is
  moved over the menu. 

*pathName* **delete** *itemName*\ ...
  Deletes one or more items from the menu. *ItemName* may be a label,
  index, or tag and may refer to multiple items (example: "all").

*pathName* **deselect** *itemName*
  Deselects *itemName* and sets the associated variables to their off values.
  *ItemName* may be a label, index, or tag, but may not represent more than one
  menu item.  If this item was not currently selected, the command has no
  effect.

*pathName* **exists** *itemName*
  Returns the *itemName* exists in the menu. *ItemName* may be a label,
  index, or tag, but may not represent more than one menu item.  Returns
  "1" is the item exists, "0" otherwise.
  
*pathName* **index** *itemName* 
  Returns the index of *itemName*. *ItemName* may be a label, index, or
  tag, but may not represent more than one menu item.  If the item does not
  exist, "-1" is returned.
  
*pathName* **invoke** *itemName* 
  Selects the *item and invokes the TCL command specified by *itemName*'s
  **-command** option. *ItemName* may be a label, index, or tag, but may
  not represent more than one menu item.
  
*pathName* **overbutton** *x* *y* 
  Indicates if the x and y coordinates specified are over the button region
  for this menu.  *X* and *y* are root coordinates.  This command uses the
  information set by the **post** operation to determine where the button
  region is.  Returns "1" if the coordinate is in the button region, "0"
  otherwise.

*pathName* **post** ?\ *switches* ... ? 
  Arranges for the *pathName* to be displayed on the screen. The position
  of *pathName* depends upon *switches*.

  The position of the *comboframe* may be adjusted to guarantee that the
  entire widget is visible on the screen.  This command normally returns an
  empty string.  If the **-postcommand** option has been specified, then
  its value is executed as a TCL script before posting the menu and the
  result of that script is returned as the result of the post widget
  command.  If an error returns while executing the command, then the error
  is returned without posting the menu.

  *Switches* can be one of the following:

  **-align** *how*
    Aligns the menu horizontally to its parent according to *how*.  *How*
    can be "left", "center", or "right".

  **-box** *coordList*
    Specifies the region of the parent window that represent the button.
    Normally comboframes are aligned to the parent window.  This allows you
    to align the menu a specific screen region.  *CoordList* is a list of
    two x,y coordinates pairs representing the two corners of the box.

  **-window** *windowName*
    Specifies the name of window to align the menu to.  Normally
    *comboframe*s are aligned to its parent window.  *WindowName* is the
    name of another widget.

*pathName* **see** *itemName* 
  Scrolls the menu so that *itemName* is visible in the widget's window.
  *ItemName* may be a label, index, or tag, but may not represent more than
  one menu item.
  
*pathName* **select** *itemName* 
  Selects *itemName* in the menu. The item is drawn in its selected colors
  and its TCL command is invoked (see the **-command** menu item option).
  *ItemName* may be a label, index, or tag, but may not represent more than
  one menu item.
  
*pathName* **unpost**
  Unposts the *comboframe* window so it is no longer displayed onscreen.
  This is used by user code to unmap the menu.

*pathName* **value** *itemName*
  Returns the value associated with *itemName*.  The value is specified by
  the menu item's **-value** option.  *ItemName* may be a label, index, or
  tag, but may not represent more than one menu item.
   
DEFAULT BINDINGS
----------------

There are many default class bindings for *comboframe* widgets.

EXAMPLE
-------

Create a *comboframe* widget with the **blt::comboframe** command.

 ::

    package require BLT

    # Create a new comboframe and add menu items to it.

    blt::combobutton .file -text "File" -menu .file.m 
    blt::comboframe .file.m \
	-window .file.m.frame 

Please note the following:

1. You can't use a Tk **menubutton** with *comboframe*\ s.  The menu is
   posted by either a **blt::combobutton** or **blt::comboentry**
   widget.

3. You create menu items with the **add** operation.  The type of item is
   specified by the **-type** option.  The default type is "button".

4. You don't pack the scrollbars.  This is done for you.

5. You don't have to specify the **-orient** or **-command** options to
   the scrollbars. This is done for you.


DIFFERENCES WITH TK MENUS
-------------------------

The **blt::comboframe** widget has several differences with the Tk **menu**
widget.

1. *Comboframe* item types are specified by the **-type** option.

2. *Comboframes* can not be torn off.

3. *Comboframes* can not be invoked by a Tk **menubutton**.

4. *Comboframes* are a single column.
   
KEYWORDS
--------

comboframe, widget

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
