===============
blt::combomenu
===============

-------------------------------------------------
Create and manipulate combomenu widgets
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

**blt::combomenu** *pathName* ?\ *option value* ...\ ?

DESCRIPTION
===========

The **combomenu** manages a menu and optionally vertical and hortizontal
scrollbars.  The scrollbars are automatically exposed and hidden as needed
with the **combomenu** widget is resized.  Whenever the **combomenu**
window is smaller horizontally and/or vertically than the menu, the
appropiate scrollbar is exposed.  

A menu is a widget that displays a collection of item arranged in one or
more columns.  There exist several different types of items, each with
different properties.  Items of different types may be combined in a single
menu.  Menu items are not distinct widgets; the entire *combomenu* is one
widget.

SYNTAX
======

**blt::combomenu** *pathName* ?\ *option value* ...\ ?

The **combomenu** command creates a new window named *pathName* and makes
it into a **combomenu** widget.  Additional options may be specified on the
command line or in the option database to configure aspects of the
combomenu such as its background color, the scrollbars, or the child
widget. The **combomenu** command returns its *pathName* argument.  At the
time this command is invoked, there must not exist a window named
*pathName*, but *pathName*'s parent must exist.

The scrollbars must be children of the combomenu widget.  The scrollbars
may be specified in the **combomenu** widget before they exist.  The normal
size of of **combomenu** is the requested width and height of the child
widget (where no scrollbars are necessary).  You can override the child
widget's requested size by setting the combomenu's **-reqwidth** and
**-reqheight** options respectively.

MENU ITEMS
==========

A menu is a widget that displays a collection of item arranged in one or
more columns.  There exist several different types of items (specified by
the item's **-type** option), each with different properties.  Items of
different types may be combined in a single menu.  Menu items are not
distinct widgets; the entire **combomenu** is one widget.

A menu item can be one of the following types: "button", "cascade",
"checkbutton", "radiobutton", and "separator".

Button Items

  The *button* is the most common type of menu item. It behaves much like a
  **button** widget. 

Cascade Items

  A *cascade* item is one with an associated menu (specified by the item's
  **-menu** option).  *Cascade* items allow the construction of cascading
  menus.  The **postcascade** widget command can be used to post and unpost
  the associated menu just next to of the *cascade* item.  The associated
  menu must be a child of the menu containing the *cascade* item.  

Checkbutton Items

  A *checkbutton* menu item behaves much like a **checkbutton** widget.
  When it is invoked it toggles back and forth between the selected and
  deselected states.  When the item is selected, a particular value is
  stored in a particular global variable (as determined by the **-onvalue**
  and **-variable** options for the item); when the item is deselected
  another value (determined by the **-offvalue** option) is stored in the
  global variable.  An indicator box is displayed to the left of the label.
  If the item is selected then the an arrow is display in the indicator. If
  a **-command** option is specified, then its value is evaluated as a TCL
  command each time the item is invoked; this happens after toggling the
  item's selected state.

Radiobutton Items

  A *radiobutton* menu item behaves much like a **radiobutton** widget.
  *Radiobutton* items are organized in groups of which only one item may be
  selected at a time.  Whenever a particular item becomes selected it
  stores a particular value into a particular global variable (as
  determined by the **-value** and **-variable** options for the item).
  This action causes any previously selected item in the same group to
  deselect itself.  Once an item has become selected, any change to the
  item's associated variable will cause the item to deselect itself.
  Grouping of *radiobutton* items is determined by their associated
  variables: if two items have the same associated variable then they are
  in the same group.  An indicator circle is displayed to the left of the
  label.  If the item is selected then the indicator's center is filled.
  If a **-command** option is specified then its value is evaluated as a
  Tcl command each time the item is invoked; this happens after selecting
  the item.

Separator items

  A *separator* is an item that is displayed as a horizontal dividing line.
  A *separator* can't be activated or invoked, and it has no behavior other
  than its display appearance.

Menu items are displayed with up to four separate fields.

  *label*

    The main field is a label in the form of a text string, or an image,
    controlled by the **-label** and **-image** options for the item.

  *icon*

    If the **-icon** option is specified, then a image is displayed to the
    left of the label.

  *accelerator*

    If the **-accelerator** option is specified for an item then a second
    textual field is displayed to the right of the label.  The accelerator
    typically describes a keystroke sequence that may be typed in the
    application to cause the same result as invoking the menu entry.

  *indicator*

    The indicator is present only for *checkbutton*, *radiobutton*, and
    *cascade* entries.  For *checkbutton* and *radiobutton* items it
    indicates whether the item is selected or not, and is displayed to the
    left of the entry's string.  For *cascade* items it indicates that
    clicking on item will post yet another menu and is displayed to the right
    of the accelerator.

Menu items may be referred to by either their index, label, or tag.

 **index**
   The number of the menu item.  Indices start from 0.  The index of an
   item as other items are added, deleted, moved, or sorted.

 **label**
   The label of the item.  Each item label.
   Labels should not be numbers (to distinguish them from indices). 

 **tag**
   A tag is a string associated with an item.  They are a useful for
   referring to groups of items. Items can have any number of tags
   associated with them using the **-tags** option.  "all" and "end".
   Every item has the tag "all".  The last item in the menu will have the
   tag "end".  A tag may refer to multiple items.
     
If an item is specified by an integer it is assumed to be an index.  It it
is specified by a strings it is first checked if it is a label and then a
tag.  This means that you shouldn't have tags and labels that are the same.
They will always be interpreted as labels.  Unlike labels, tags aren't seen
by the user, so you can do whatever you want to make them unique (example:
"mytag::fred").

If a **-command** option is specified for the item then it is evaluated as
a TCL command whenever the item is invoked.
  
OPERATIONS
==========

All **combomenu** operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for combomenu widgets:

*pathName* **activate** *item* 
 
  Redisplays *item* using its active colors.  This typically is used by
  widget bindings to highlight menu items when the pointer is moved over
  items in the menu. Any previously active item is deactivated.  *Item* may
  be a label, index, or tag, but may not represent more than one menu item.

*pathName* **add** ?\ *option* *value* ...?
 
  Creates a new menu item, appending it to the end of the menu.  If one or
  more *option-value* pairs are specified, they modifies the given menu
  item option(s) to have the given value(s).  *Option* and *value* are
  described in the **item configure** operation.

*pathName* **bbox** *item* 
 
  Returns of list of four numbers describing the bounding box *item*.  The
  numbers represent the x and y root coordinates to two corners of the
  box. *Item* may be a label, index, or tag, but may not represent more
  than one menu item.

*pathName* **cget** *option*  

  Returns the current value of the widget configuration option given by
  *option*. *Option* may have any of the values accepted by the
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
  is "BltCombomenu".  The resource name is the name of the widget::

    option add *BltCombomenu.anchor n
    option add *BltCombomenu.Anchor e

  The following widget options are available\:

  **-acceleratorfont** *fontName* 

    Specifies the font for the accelerator text.  The default is
    "\*-Helvetica-Bold-R-Normal-\*-18-180-\*"

  **-acceleratorforeground** *colorName* 

    Specifies the color of the accelerator text.  The default is "black".

  **-activeacceleratorforeground** *colorName* 

    Specifies the active color of the accelerator text.  The default is
    "white".

  **-activeforeground** *colorName* 

    Specifies the active color of the label text.  The default is
    "black".

  **-activerelief** *relief* 

    Specifies the active relief of menu items.  The default is
    "flat".
    
  **-background** *background* 

    Specifies the background of the menu.  The default is "white".
    *Background* can be a color name or background.
    
  **-borderwidth** *numPixels* 

    Specifies the borderwidth of the menu.  The default is "1".

  **-checkbuttoncolor** *colorName*

    Specifies the color of checkbutton items.  The default is "1".

  **-checkbuttonfillcolor** *colorName*

    Specifies the fill color of checkbutton items.  The default is "1".

  **-checkbuttonoutlinecolor** *colorName*

    Specifies the outline color of checkbutton items.  The default is "1".

  **-checkbuttonsize** *numPixels*

    Specifies the size of the check box of checkbutton items.
    *NumPixels* is a valid screen distance, such as \f(CW2\fR or \f(CW1.2i\fR.
    If this option isn't specified, then it defaults to "1".

  **-command** *string* 

    Specifies a command to invoke when menu is set.
    The default is "".

  **-cursor** *cursorName* 

    Specifies the cursor display in the menu.
    The default is "arrow".

  **-disabledacceleratorforeground** *colorName* 

    Specifies the disabled color of the accelerator text for items in the
    menu.  The default is "arrow".

  **-disabledbackground** *background* 

    Specifies the disabled color of the background for items in the menu.
    The default is "arrow".

  **-disabledforeground** *colorName* 

    Specifies the disabled color of the text for items in the menu.  The
    default is "arrow".

  **-font** *colorName* 

    Specifies the font of the text for items in the menu.  The
    default is "arrow".

  **-foreground** *colorName* 

    Specifies the color of the text for items in the menu.  The
    default is "arrow".

  **-height** *numPixels* 

    Specifies the height in the menu.  *NumPixels* is a valid screen
    distance, such as \f(CW2\fR or \f(CW1.2i\fR.  The default is "arrow".

  **-iconvariable** *varName* 

    Specifies the name of a variable that holds the name of the image
    representing the icon of the last selected item.  If *varName*
    is "", no variable is used. The default is "".

  **-itemborderwidth** *numPixels* 

    Specifies the borderwidth of items in the menu.  The default is "1".

  **-postcommand** *string* 

    Specifies the command to invoke when the menu is posted.  This
    is command is invoked before the menu is displayed.
    The default is "".

  **-radiobuttoncolor** *colorName*

  **-radiobuttonfillcolor** *colorName*

  **-radiobuttonoutlinecolor** *colorName*

  **-radiobuttonsize** *numPixels*

  **-relief** *relief* 

     Specifies the 3-D effect for the menu.  *Relief* indicates how the
     menu should appear relative to the root window; for example, "raised"
     means the menu should appear to protrude.  The default is "raised".

  **-restrictwidth** *option* 

  **-takefocus** *bool*

     Provides information used when moving the focus from window to window
     via keyboard traversal (e.g., Tab and Shift-Tab).  If *bool* is "0",
     this means that this window should be skipped entirely during keyboard
     traversal.  "1" means that the this window should always receive the
     input focus.  An empty value means that the traversal scripts make the
     decision whether to focus on the window.  The default is "".

  **-textvariable** *varName* 

     Specifies the name of a variable that holds the text of the last
     selected item.  If *varName* is "", no variable is used. The default
     is "".

  **-unpostcommand** *string*

     Specifies the command to invoke when the menu is unposted.  The
     default is "".

  **-width** *numPixels*

     Specifies the width in the menu.  If *numPixels* is 0, the width of
     the menu is computed from the size of its items. *NumPixels* must be a
     valid screen distance, such as "2" or "1.2i".  The default is "0".

  **-xscrollbar** *widget*

     Specifies the name of a scrollbar widget to use as the horizontal
     scrollbar for this menu.  The scrollbar widget must be a child of the
     combomenu and doesn't have to exist yet.  It at an idle point later,
     the combomenu will attach the scrollbar to widget, effectively
     packing the scrollbar into the menu.

  **-xscrollcommand** *string*

     Specifies the prefix for a command used to communicate with horizontal
     scrollbars.  Whenever the horizontal view in the widget's window
     changes, the widget will generate a Tcl command by concatenating the
     scroll command and two numbers. If this option is not specified, then
     no command will be executed.  The widget's initialization script
     will automatically set this for you.

  **-xscrollincrement** *numPixels*

     Sets the horizontal scrolling unit (distance). The default is 20
     pixels.

  **-yscrollbar** *widget*

     Specifies the name of a scrollbar widget to use as the vertical
     scrollbar for this menu.  The scrollbar widget must be a child of the
     combomenu and doesn't have to exist yet.  It at an idle point later,
     the combomenu will attach the scrollbar to widget, effectively
     packing the scrollbar into the menu.

  **-yscrollcommand** *string*

     Specifies the prefix for a command used to communicate with vertical
     scrollbars.  Whenever the vertical view in the widget's window
     changes, the widget will generate a Tcl command by concatenating the
     scroll command and two numbers.  If this option is not specified, then
     no command will be executed.  The widget's initialization script
     will automatically set this for you.

  **-yscrollincrement** *numPixels*

     Sets the vertical scrolling unit (distance). The default is 20 pixels.

*pathName* **deactivate** 

  Redisplays all menu items using their normal colors.  This typically is
  used by widget bindings to un-highlight menu items as the pointer is
  moved over the menu. 

*pathName* **delete** *item*...
 
  Deletes one or more items from the menu. *Item* may be a label, index, or
  tag and may refer to multiple items (example: "all"). 

*pathName* **deselect** *item*...
 
  Deselects *item* and sets the associated variables to their off values.
  *Item* may be a label, index, or tag, but may not represent more than one
  menu item.  If this item was not currently selected, the command has no
  effect.

*pathName* **exists** *item*...
 
  Returns the *item* exists in the menu. *Item* may be a label, index, or
  tag, but may not represent more than one menu item.  Returns "1" is
  the item exists, "0" otherwise.
  
*pathName* **find** *string* ?\ *switches* ...\ ?
 
   Scrolls the combomenu so that the specified portion of the child 
   widget is visible in the combomenu window.
  
*pathName* **index** *item* 
 
  Returns the index of *item*. *Item* may be a label, index, or tag, but
  may not represent more than one menu item.  
  
*pathName* **insert after** *item* ?\ *option *value* ...\ ? 
 
  Creates a new menu item and inserts it after *item*.  Normally menu items
  are appended to the end of the menu, but this command allows you to
  specify its location. Note that this may change the indices of previously
  created menu items. *Item* may be a label, index, or tag, but may not
  represent more than one menu item. If one or more *option-value* pairs
  are specified, they modifies the given menu item option(s) to have the
  given value(s).  *Option* and *value* are described in the **item
  configure** operation.
  
*pathName* **insert at** *item* ?\ *option *value* ...\ ? 
 
  Creates a new menu item and inserts it at the index specified by *item*.
  Normally menu items are appended to the end of the menu, but this command
  allows you to specify its location. Note that this may change the indices
  of previously created menu items. *Item* may be a label, index, or tag,
  but may not represent more than one menu item. If one or more
  *option-value* pairs are specified, they modifies the given menu item
  option(s) to have the given value(s).  *Option* and *value* are described
  in the **item configure** operation.
  
*pathName* **insert before** *item* ?\ *option *value* ...\ ? 
 
  Creates a new menu item and inserts it before *item*.  Normally menu
  items are appended to the end of the menu, but this command allows you to
  specify its location. Note that this may change the indices of previously
  created menu items. *Item* may be a label, index, or tag, but may not
  represent more than one menu item. If one or more *option-value* pairs
  are specified, they modifies the given menu item option(s) to have the
  given value(s).  *Option* and *value* are described in the **item
  configure** operation.
  
*pathName* **invoke** *item* 
 
  Invokes the command associated with *item*. *Item* may be a label, index,
  or tag, but may not represent more than one menu item. The command
  is specified by the **-command** item option.  
  
*pathName* **item cget** *item* *option*
 
  Returns the current value of the configuration option for *item* given by
  *option*.  *Option* may be any option described below for the **item
  configure** operation below. *Item* may be a label, index, or tag, but
  may not represent more than one menu item.

*pathName* **item configure** *item* ?\ *option* *value* ...\ ?
 
  Query or modify the configuration options of *item*.  *Item* may be a
  label, index, or tag.  If no *option* is specified, returns a list
  describing all the available options for *item* (see **Tk_ConfigureInfo**
  for information on the format of this list).  If *option* is specified
  with no *value*, then the command returns a list describing the one named
  option (this list will be identical to the corresponding sublist of the
  value returned if no *option* is specified).  In both cases, *item* may
  not represent more than one menu item.
  
  If one or more *option-value* pairs are specified, then this command
  modifies the given option(s) to have the given value(s); in this case
  *item* may refer to mulitple items (example: "all").  *Option* and
  *value* are described below.

  **-accelerator** *string* 

    Specifies a textual field is displayed to the right of the label.
    The accelerator typically describes a keystroke sequence  that
    may be typed in the application to cause the same result as invoking
    the menu item.  The default is "".

  **-command** *string* 

   Specifies the color of the accelerator text.  The default is "black".

  **-data** *string* 

    Specifies the active color of the accelerator text.  The default is
    "white".

  **-icon** *imageName* 

    Specifies the active color of the label text.  The default is
    "black".

  **-image** *imageName* 

    Specifies the active relief of menu items.  The default is
    "flat".
    
  **-indent** *numPixels* 

    Specifies the background of the menu.  The default is "white".
    *Background* can be a color name or background.
    
  **-menu** *menuName* 

    Specifies the path name of the submenu associated with this item.
    The submenu must be a child of the menu.  This option is only
    used for cascade items.  The default is "".

  **-offvalue** *string*

    Specifies the value to store in the items's associated variable when
    the item is deselected.  This option only affects checkbutton items.
    The default is "".

  **-onvalue** *string*

    Specifies the value to store in the items's associated variable when
    the item is selected.  This option only affects checkbutton items.
    The default is "".

  **-state** *state*

    Specifies one of three states for the item: "normal", "disabled", or
    "hidden". 

    *normal*
      In normal state the item is displayed using the **-foreground**
      option for the menu and the **-background** option from
      the item or the menu.

    *disabled*
      Disabled state means that the item should be insensitive: the default
      bindings will not activate or invoke the item.  In this state
      the item is displayed according to the **-disabledforeground** option
      for the menu and the **-disabledbackground** option from the item.

    *hidden*
      The item is not displayed.

    The default is "normal".

  **-style** *styleName*

    Specifies the size of the check box of checkbutton items.
    *NumPixels* is a valid screen distance, such as \f(CW2\fR or \f(CW1.2i\fR.
    If this option isn't specified, then it defaults to "1".

  **-tags** *tagList* 

    Specifies a command to invoke when menu is set.
    The default is "".

  **-text** *string* 

    Specifies the cursor display in the menu.
    The default is "arrow".

  **-tooltip** *string* 

    Specifies the disabled color of the accelerator text for items in the
    menu.  The default is "arrow".

  **-type** *itemType* 

    Specifies the disabled color of the background for items in the menu.
    The default is "arrow".

  **-underline** *numCharacters* 

    Specifies the disabled color of the text for items in the menu.  The
    default is "arrow".

  **-value** *string* 

    Specifies the font of the text for items in the menu.  The
    default is "arrow".

  **-variable** *varName* 

    Specifies the color of the text for items in the menu.  The
    default is "arrow".

*pathName* **listadd** *labelList*  ?\ *option* *value* ...\ ?
 
  Adds one or more menu items to the menu from *labelList*.  Each label in
  *labelList* is used to create a new menu item.  A menu item can not
  already exist with that label.  If one or more *option-value* pairs are
  specified, they modifies the given menu item option(s) to have the given
  value(s).  *Option* and *value* are described in the **item configure**
  operation.

*pathName* **names** ?\ *pattern* ...\ ?
 
  Returns the labels of all the items in the menu.  If one or more
  *pattern* arguments are provided, then the label of any item matching
  *pattern* will be returned. *Pattern* is a glob-style pattern.

*pathName* **nearest** *x* *y*
 
  Returns the index of the menu item closest to the coordinates specified.
  *X* and *y* are root coordinates.

*pathName* **next** *item* 
 
  Moves the focus to the next menu item from *item*.  *Item* may be a
  label, index, or tag, but may not represent more than one menu item.

*pathName* **overbutton** *x* *y* 
 
  Indicates it the coordinates specified are over the button region for
  this menu.  *X* and *y* are root coordinates.  This command used the
  information set by the **post** operation to determine where the button
  region is.  Returns "1" if the coordinate is in the button region, "0"
  otherwise.

*pathName* **post** ?\ *switches* ...\ ? 
 
  Arrange for the menu to be displayed on the screen. Where the
  window is displayed depends upon *switches*.

  The position of the menu may be adjusted to guarantee that the entire
  menu is visible on the screen.  This command normally returns an empty
  string.  If the **-postcommand** option has been specified, then its
  value is executed as a Tcl script before posting the menu and the result
  of that script is returned as the result of the post widget command.  If
  an error returns while executing the command, then the error is returned
  without posting the menu.

  *Switches* can be one of the following:

  **-align** *how*
    Aligns the menu horizontally to its parent according to *how*.  *How*
    can be "left", "center", or "right".

  **-box** *coordList*
    Specifies the region of the parent window that represent the button.
    Normally combomenus are aligned to the parent window.  This allows you
    to align the menu a specific screen region.  *CoordList* is a list of
    two x,y coordinates pairs representing the two corners of the box.

  **-cascade** *coordList*
    Specifies how to position the menu.  This option is for
    *cascade* menus. *CoordList* is a list of x and y coordinates
    representing the position of the cascade menu.

  **-popup** *coordList*
    Specifies how to position the menu.  This option is for
    *popup* menus. *CoordList* is a list of x and y coordinates
    representing the position of the popup menu.

  **-window** *window*
    Specifies the name of window to align the menu to.  Normally combomenus
    are aligned to the parent window.  *Window* is the name of another
    widget.

*pathName* **postcascade** ?\ *item* ? 
 
  Posts the the menu associated with *item*. This command is only affects
  *cascade* items.  *Item* may be a label, index, or tag, but may not
  represent more than one menu item.

*pathName* **previous** *item*
 
  Moves the focus to the previous menu item from *item*.  *Item* may be a
  label, index, or tag, but may not represent more than one menu item.

*pathName* **scan dragto** *x* *y*
 
  This command computes the difference between *x* and *y* and the
  coordinates to the last **scan mark** command for the widget.  It then
  adjusts the view by 10 times the difference in coordinates.  This command
  is typically associated with mouse motion events in the widget, to
  produce the effect of dragging the item list at high speed through the
  window.  The return value is an empty string.
   
*pathName* **scan mark** *x* *y*
 
   Records *x* and *y* and the current view in the menu window; to be used
   with later **scan dragto** commands. *X* and *y* are window coordinates
   (i.e. relative to menu window).  Typically this command is associated
   with a mouse button press in the widget.  It returns an empty string.

*pathName* **see** *item* 
 
  Scrolls the menu so that *item* is visible in the widget's window.
  *Item* may be a label, index, or tag, but may not represent more than one
  menu item.
  
*pathName* **select** *item* 
 
  Selects *item* in the menu. The item is drawn in its selected colors.
  *Item* may be a label, index, or tag, but may not represent more than one
  menu item.
  
*pathName* **size**
 
  Returns the number of items in the menu.
   
*pathName* **sort cget** *option*

  Returns the current value of the sort configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **sort configure** operation. They are described below.

*pathName* **sort configure** ?\ *option*\ ? ?\ *value*? ?\ *option value
  ...*\ ?

  Queries or modifies the sort configuration options.  If no *option* is
  specified, returns a list describing all the available options for
  *pathName* (see **Tk_ConfigureInfo** for information on the format of
  this list).  If *option* is specified with no *value*, then this command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option-value* pairs are
  specified, then this command modifies the given sort option(s) to have
  the given value(s); in this case the command returns an empty string.
  *Option* and *value* are described below.

  **-auto** 
    Automatically resort the menu items anytime the items are added
    deleted, or changed.

  **-byvalue** 
    Sort items using their values.  By default the items are sorted
    by their labels.

  **-command** *string*
    Specifies *string* as a TCL command to use for comparing items.  To
    compare two items, evaluate a Tcl script consisting of command with the
    two item appended as additional arguments.  The script should return an
    integer less than, equal to, or greater than zero if the first item
    is to be considered less than, equal to, or greater than the second,
    respectively.

  **-decreasing** 
    Sort the items highest to lowest. By default items are sorted
    lowest to highest.

  **-type** *sortType*
    Compare items based upon *sortType*.  *SortType* can be
    any of the following:

    *ascii*
      Use string comparison with Unicode code-point collation order (the name
      is for backward-compatibility reasons.)  The string representation of
      the values are compared.   

    *dictionary*
      Use dictionary-style comparison. This is the same as *ascii*
      except (a) case is ignored except as a tie-breaker and (b) if two
      strings contain embedded numbers, the numbers compare as integers,
      not characters.  For example, in -dictionary mode, "bigBoy" sorts
      between "bigbang" and "bigboy", and "x10y" sorts between "x9y" and
      "x11y".  

    *integer*
      Compare the items as integers.  

    *real*
      Compare the items as floating point numbers.  

    *command* 
      Use the command specified by **-command** option to compare items.

*pathName* **sort once** 

  Sorts the menu items using the current set of sort configuration values.  

*pathName* **style cget** *styleName* *option*
 
  Returns the current value of the style configuration option given by 
  *option* for *styleName*.  *Option* may be any option described below
  for the **style configure** operation.
   
*pathName* **style configure** *styleName* ?\ *option* *value* ...\ ?
   
  Query or modify the configuration options for the style *styleName*.
  *StyleName* is the name of a style created by the **style create**
  operaton.  If no *option* argument is specified, this command returns a
  list describing all the available options for *pathName* (see
  **Tk_ConfigureInfo** for information on the format of this list).  If
  *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sublist of the value returned if no *option* is specified).
  If one or more *option-value* pairs are specified, then the command
  modifies the given widget option(s) to have the given value(s); in this
  case the command returns an empty string.  *Option* and *value* are
  described below.

  **-acceleratorfont** *fontName* 

    Specifies the font for the accelerator text.  The default is
    "\*-Helvetica-Bold-R-Normal-\*-18-180-\*"

  **-acceleratorforeground** *colorName* 

    Specifies the color of the accelerator text.  The default is "black".

  **-activeacceleratorforeground** *colorName* 

    Specifies the active color of the accelerator text.  The default is
    "white".

  **-activeforeground** *colorName* 

    Specifies the active color of the label.  The default is
    "black".

  **-activerelief** *relief* 

    Specifies the active relief.  This is the relief when the menu item
    become active (normally when the pointer is over the item). The default
    is "flat".
    
  **-background** *background* 

    Specifies the background.  The default is "white".  *Background* can be
    a color name or background.
    
  **-borderwidth** *numPixels* 

    Specifies the borderwidth.  The default is "1".

  **-disabledacceleratorforeground** *colorName* 

    Specifies the disabled color of the accelerator text.
    The default is "arrow".

  **-disabledbackground** *background* 

    Specifies the disabled color of the background.
    The default is "arrow".

  **-disabledforeground** *colorName* 

    Specifies the disabled color of the label.  The
    default is "arrow".

  **-font** *colorName* 

    Specifies the font of the text for menu items.  The
    default is "arrow".

  **-foreground** *colorName* 

    Specifies the text color for menu items.  The default is "black".

  **-indicatorfillcolor** *colorName* 

    Specifies the fill color of the radiobutton or checkbutton
    indicators.  The default is "arrow".

  **-indicatoroutlinecolor** *colorName* 

    Specifies the outline color of the radiobutton or checkbutton
    indicators.  The default is "arrow".

  **-indicatorcolor** *colorName* 

    Specifies the color of the radiobutton or checkbutton.  The default is
    "arrow".

  **-indicatorsize** *numPixels* 

    Specifies the size in pixels of the radiobutton or checkbutton indicators.
    This is the checkbox for checkbuttons and the circle indicator for
    radiobuttons. The default is "20" pixels.

  **-relief** *relief* 

    Specifies the 3-D effect for the border around the menu item.
    *Relief* specifies how the interior of the legend should appear
    relative to the menu; for example, "raised" means the item
    should appear to protrude from the menu, relative to the surface of
    the menu.  The default is "flat".

*pathName* **style create** *styleName* ?\ *option* *value* ...\ ?
 
  Creates a new item style named *styleName*.  You can apply a style to
  menu item to change its appearance.  The style options override the
  general settings of the menu widget.  Styles are A style *styleName* can
  not already exist.  If one or more *option*-*value* pairs are specified,
  they specify options valid for the \fBstyle configure\fR operation.
   
*pathName* **style delete** ? *styleName* ...\ ?
 
  Deletes one or more styles.  *StyleName* is the name of a style created
  by the **style create** operaton.  Styles are reference counted.  The
  resources used by *styleName* are not freed until no item is using it.
   
*pathName* **style exists** *styleName*
 
  Indicates if the style *styleName* exists in the widget. Returns "1"
  if it exists, "0" otherwise.
   
*pathName* **style names** ?\ *pattern* ...\ ?
 
  Returns the names of all the styles in the widget.  If one or more
  *pattern* arguments are provided, then the names of any style matching
  *pattern* will be returned. *Pattern* is a glob-style pattern.

*pathName* **type** *item*
 
  Returns the type of *item*.  *Item* may be a label, index, or tag, but
  may not represent more than one menu item.
   
*pathName* **unpost**
 
  Unmap the menu window so that it is no longer displayed.  If one or
  more lower level cascaded menus are posted, they are unposted too. 

*pathName* **value** *item*
 
   Returns the value associated with *item*.  The value is stored by the
   **-value** option.  Item* may be a label, index, or tag, but may not
   represent more than one menu item.
   
*pathName* **xposition** *item*
 
   Scrolls the combomenu so that the specified portion of the child 
   widget is visible in the combomenu window.
   
*pathName* **xview moveto** fraction
 
   Scrolls the combomenu so that the specified portion of the child 
   widget is visible in the combomenu window.
   
*pathName* **xview scroll** *number* *what*
 
   Scrolls the combomenu so that the specified portion of the child 
   widget is visible in the combomenu window.
   
*pathName* **yposition** *item*
 
   Scrolls the combomenu so that the specified portion of the child 
   widget is visible in the combomenu window.
   
*pathName* **yview moveto** fraction
 
   Scrolls the combomenu so that the specified portion of the child 
   widget is visible in the combomenu window.
   
*pathName* **yview scroll** *number* *what*
 
   Scrolls the combomenu so that the specified portion of the child 
   widget is visible in the combomenu window.
   

  **-anchor** *anchor* 

    It the **combomenu** window is bigger than the child widget, this
    option specifies how the child widget should be positioned within the
    combomenu. The default is "center".

  **-background** *color*  

    Sets the default background for the combomenu widget.  The background is
    normally completely obscurred by the child widget.  But if both
    scrollbars are exposed, there will be a square region in lower right
    corner.  This option defines the color of that region.  The default is
    "grey85".

  **-cursor** *cursor*  

    Specifies the widget's cursor.  The default cursor is "".

  **-fill** *fill*  

    If the **combomenu** window is bigger than the child widget,
    this option specifies how the child widget is to be stretched to
    fill the window. *Fill* can be either "none", "x", "y", or
    "both".  For example, if *fill* is "x", then the widget is stretched
    horizontally.  If *fill* is "both", the widget is stretched both
    horizontally and vertically.  The  default is "both".

  **-height** *pixels*  

    Specifies the requested height of combomenu widget.  If *pixels* is 0, then
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

    Specifies the requested width of the combomenu widget.  If *pixels* is 0,
    then the width of the widget will be calculated based on the request size
    child widget.  The default is "0".

  **-window** *pathName*  

    Specifies the widget to be child into the combomenu.  *PathName* must
    be a child of the **combomenu** widget.  The combomenu will "pack" and
    manage the size and placement of *pathName*.  The default value is "".


*pathName* **xview** *args*

  This command queries or changes the horizontal position of the
  child widget in the combomenu's window.  It can take any of the 
  following forms:

  *pathName* **xview**

    Returns a list of two numbers between 0.0 and 1.0 that describe the
    amount and position of the child widget that is visible in the
    **combomenu** window.  For example, if *view* is "0.2 0.6", twenty
    percent of the child widget is off-screen to the left, forty percent is
    visible in the window, and 40 percent of the child widget is off-screen
    to the right.  These are the same values passed to scrollbars via the
    **-xscrollcommand** option.

  *pathName* **xview moveto** *fraction*

    Adjusts the view in the window so that *fraction* of the
    total width of the combomenu text is off-screen to the left.
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
  widget in the combomenu's window.  It can take any of the following

  *pathName* **yview**

    Returns a list of two numbers between 0.0 and 1.0 that describe the
    amount and position of the child widget that is visible in the
    **combomenu** window.  For example, if the result is "0.2 0.6", twenty
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

There are many default class bindings for **combomenu** widgets.

EXAMPLE
=======

You create a combomenu widget with the **combomenu** command.

  ::

    package require BLT

    # Create a new combomenu
    blt::combomenu .ss  

A new Tcl command ".ss" is also created.  This command can be
used to query and modify the combomenu.  For example, you can specify
the scrollbars and child widget to use with the combomenu's 
**configure** operation.

  ::

    .ss configure -xscrollbar .ss.xsbar -yscrollbar .ss.ysbar -window .ss.g 
    blt::tk::scrollbar .ss.ysbar 
    blt::tk::scrollbar .ss.xsbar 
    blt::graph .ss.g 

Note that

  - The scrollbars and child widget are children of the
    combomenu widget.  

  - The scrollbars and child widget do not have to exist before you create 
    the combomenu instance.

  - You don't have to specify the orientation of the scrollbars 
    (the scrollbar's **-orient** option is set for you).

You can then pack the combomenu as usual.

DIFFERENCES WITH TK MENUS
=========================

1. **Combomenu** item types are specified by the **-type** option.

2. **Combomenus** can not be torn off.

3. **Combomenus** can not be invoked by a Tk **menubutton**.


KEYWORDS
========
combomenu, widget
