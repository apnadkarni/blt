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
-----------

The **blt::combomenu** command creates and manages *combomenu* widgets.
The *combomenu* widget contains a menu and optional scrollbars (vertical
and/or horizontal).  The scrollbars are automatically exposed or hidden as
necessary when the **combomenu** widget is resized.  Whenever the
**combomenu** window is smaller horizontally and/or vertically than the
actual menu, the appropiate scrollbar is exposed.

A *combomenu* is a widget that displays a menu items.  There exist several
different types of items, each with different properties.  Items of
different types may be combined in a single menu.  Menu items are not
distinct widgets; the entire *combomenu* is one widget.

SYNTAX
------

**blt::combomenu** *pathName* ?\ *option value* ...\ ?

The **blt::combomenu** command creates a new window named *pathName* and
makes it into a *combomenu* widget.  Additional options may be specified on
the command line or in the option database to configure aspects of the
combomenu such as its background color or scrollbars. The
**blt::combomenu** command returns its *pathName* argument.  There must not
already exist a window named *pathName*, but *pathName*'s parent must
exist.

The normal size of the *combomenu* is the computed size to display all menu
items. You can specify the maximum and minimum width and height of the
*combomenu* window using the **-width** and **-height** widget options.
Scrollbars are specified for *combomenu* by the **-xscrollbar** and
**-yscrollbar** widget options.  The scrollbars must be children of the
*pathName*.  The scrollbars may be specified before they exist.  If the
specified width or height is less than the computed size of the combomenu,
the appropiate scrollbar is automatically displayed.

MENU ITEMS
----------

A menu is a widget that displays a collection of item arranged in one or
more columns.  There exist several different types of items (specified by
the item's **-type** option), each with different properties.  Items of
different types may be combined in a single menu.  Menu items are not
distinct widgets; the entire **combomenu** is one widget.

A menu item can be one of the following types: 

  *button*

    The *button* is the most common type of menu item that behaves much
    like a Tk **button** widget.  When it isinvoked, a TCL command is
    executed.  The TCL command is specified with the **-command** option.

  *cascade*

    A *cascade* item has an associated *combomenu* (specified by the item's
    **-menu** option) that is posted to the right of the *cascade* item
    when the item is active.  The **postcascade** operation also posts the
    associated *combomenu*. *Cascade* items allow the construction of
    cascading menus.  The associated menu must be a child of the
    *combomenu* containing the *cascade* item.

  *checkbutton*

    A *checkbutton* menu item behaves much like a Tk **checkbutton** widget.
    When it is invoked it toggles back and forth between the selected and
    deselected states.  When the item is selected, a particular value is
    stored in a particular global variable (as determined by the
    **-onvalue** and **-variable** options for the item); when the item is
    deselected another value (determined by the **-offvalue** option) is
    stored in the global variable.  An indicator box is displayed to the
    left of the label.  When the item is selected, a check is displayed in
    the indicator. If a **-command** option is specified, its value is
    evaluated as a TCL command each time the item is invoked; this happens
    after toggling the item's selected state.

  *radiobutton* 

    A *radiobutton* menu item behaves much like a Tk **radiobutton**
    widget.  *Radiobutton* items are organized in groups of which only one
    item may be selected at a time.  Whenever a particular item becomes
    selected it stores a particular value into a particular global variable
    (as determined by the **-value** and **-variable** options for the
    item).  This action causes any previously selected item in the same
    group to deselect itself.  Once an item has become selected, any change
    to the item's associated variable will cause the item to deselect
    itself.  Grouping of *radiobutton* items is determined by their
    associated variables: if two items have the same associated variable
    then they are in the same group.  An indicator circle is displayed to
    the left of the label.  If the item is selected then the indicator's
    center is filled with a solid color.  If a **-command** option is
    specified then its value is evaluated as a Tcl command when the item is
    invoked; this happens after selecting the item.

  *separator*

    A *separator* is an item that is displayed as a horizontal dividing
    line.  A *separator* can't be activated or invoked, and it has no
    behavior other than its display appearance.

Menu items are displayed with up to four separate fields.

  *label*

    The main field is a label in the form of a text string, or an image,
    controlled by the **-text** and **-image** options for the item.

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
   The label of the item (specified by the **-text** menu item option).
   Labels should not be numbers (to distinguish them from indices) or tags.

 **tag**
   A tag is a string associated with an item.  They are a useful for
   referring to groups of items. Items can have any number of tags
   associated with them (specified by the **-tags** menu item option).  A
   tag may refer to multiple items.  There are two built-in tags: "all" and
   "end".  Every item has the tag "all".  The last item in the menu will
   have the tag "end".
     
If an item is specified by an integer it is assumed to be an index.  If it
is specified by a string, it is first tested if it's a valid label and then
a tag.  This means that you shouldn't have tags and labels that are the
same.  They will always be interpreted as labels.  Unlike labels, tags
aren't seen by the user, so you can do whatever you want to make them
unique (example: "mytag::fred").

If an item's **-command** option is specified, a TCL command will be invoke
whenever the item is selected (typically by clicking on the item).
  
OPERATIONS
----------

All *combomenu* operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *combomenu* widgets:

*pathName* **activate** *item* 
 
  Redisplays *item* using its active colors and relief.  This typically is
  used by widget bindings to highlight menu items when the pointer is moved
  over items in the menu. Any previously active item is deactivated.
  *Item* may be a label, index, or tag, but may not represent more than one
  menu item.

*pathName* **add** ?\ *option* *value* ...?
 
  Creates a new menu item, adding it to the end of the menu.  If one or
  more *option-value* pairs are specified, they modify the given menu item
  option(s) to have the given value(s).  *Option* and *value* are described
  in the **item configure** operation.

*pathName* **bbox** *item* 
 
  Returns of list of four numbers describing the bounding box of *item*.
  The numbers represent the x and y root coordinates of two opposite
  corners of the box. *Item* may be a label, index, or tag, but may not
  represent more than one menu item.

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
  "BltCombomenu".  The resource name is the name of the widget::

    option add *BltCombomenu.anchor n
    option add *BltCombomenu.Anchor e

  The following widget options are available\:

  **-acceleratorfont** *fontName* 

    Specifies the font for the accelerator.  The default is "{Sans Serif}
    9".

  **-acceleratorforeground** *colorName* 

    Specifies the color of the accelerator.  The default is "black".

  **-activeacceleratorforeground** *colorName* 

    Specifies the active color of the accelerator.  The default is "white".

  **-activeforeground** *colorName* 

    Specifies the color of the label when the menu item is active.  The
    default is "white".

  **-activerelief** *relief* 

    Specifies the relief of active menu items.  This determines the 3-D
    effect for the menu item.  *Relief* indicates how the item should
    appear relative to the menu window; for example, "raised" means the
    item should appear to protrude.  The default is "flat".
    
  **-background** *background* 

    Specifies the background of the menu items.  *Background* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 

    Specifies the borderwidth of the menu.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the menu. The
    value may have any of the forms accept able to **Tk_GetPixels**.  The
    default is "1".

  **-checkbuttoncolor** *colorName*

    Specifies the color of the check for checkbutton items.  The default is
    "red3".

  **-checkbuttonfillcolor** *colorName*

    Specifies the fill color of the box for checkbutton items. If
    *colorName* is "", then the box color is the background color of
    the menu item.  The default is "".

  **-checkbuttonoutlinecolor** *colorName*

    Specifies the outline color of the box for checkbutton items.  If
    *colorName* is "", then the no outline is drawn. The default is "".

  **-checkbuttonsize** *numPixels*

    Specifies the size of the box of for checkbutton items.  *NumPixels* is
    a non-negative value indicating the width and height of the check
    box. The value may have any of the forms accept able to Tk_GetPixels.
    The default is "12".

  **-command** *string* 

    Specifies a TCL command to be invoked when a menu item is selected:
    either by clicking on the menu item or using the **select** operation.
    If *string* is "", then no command is invoked. The default is "".

  **-cursor** *cursorName* 

    Specifies the cursor to be used for the widget. *CursorName* may have
    any of the forms acceptable to **Tk_GetCursor**.  If *cursorName* is "",
    this indicates that the widget should defer to its parent for cursor
    specification.  The default is "".

  **-disabledacceleratorforeground** *colorName* 

    Specifies the color of the accelerator of menu items that are
    disabled. The default is "grey90".

  **-disabledbackground** *background* 

    Specifies the background of menu items that are disabled.  *Background*
    may be a color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-disabledforeground** *colorName* 

    Specifies the color of the label for menu items that are disabled.  The
    default is "grey70".

  **-font** *colorName* 

    Specifies the font of labels in menu items.  The default is "{Sans
    Serif} 11".

  **-foreground** *colorName* 

    Specifies the color of labels in menu items.  The default is "black".

  **-height** *numPixels* 

    Specifies the height in the *combomenu*.  *NumPixels* can be single
    value or a list.  If *numPixels* is a single value it is a non-negative
    value indicating the height the menu. The value may have any of the
    forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is a 2 element list, then this sets the minimum and maximum
    limits for the height of the menu. The menu will be at least the
    minimum height and less than or equal to the maximum. If *numPixels* is
    a 3 element list, then this specifies minimum, maximum, and nominal
    height or the menu.  The nominal size overrides the calculated height
    of the menu.  If *numPixels* is "", then the height of the menu is
    calculated based on all the menu items.  The default is "".

  **-iconvariable** *varName* 

    Specifies the name of a global TCL variable that will be set to the
    name of the image representing the icon of the selected menu item.  If
    *varName* is "", no variable is used. The default is "".

  **-itemborderwidth** *numPixels* 

    Specifies the borderwidth of menu items in the menu.  *NumPixels* is a
    non-negative value indicating the width of the 3-D border drawn around
    the item. The value may have any of the forms acceptable to
    **Tk_GetPixels**.  The default is "0". 

  **-postcommand** *string* 

    Specifies a TCL command to invoked when the menu is posted.  The
    command will be invoked before the menu is displayed onscreen.  For
    example, this may be used to disable menu items that may not be valid
    when the menu is posted. If *string* is "", no command is invoked.  The
    default is "".

  **-radiobuttoncolor** *colorName*

    Specifies the color of the indicator circle for radiobutton items. The
    indicator circle is displayed when the radiobutton item is
    selected. The default is "red3".

  **-radiobuttonfillcolor** *colorName*

    Specifies the fill color of the circle for radiobutton items.  The
    default is "white".

  **-radiobuttonsize** *numPixels*

    Specifies the size of the circle for radiobutton items.  *NumPixels* is
    a non-negative value indicating the width and height of the radiobutton
    circle. The value may have any of the forms acceptable to
    *Tk_GetPixels*, such as "1.2i".  The default is "12".

  **-relief** *relief* 

     Specifies the 3-D effect for the menu.  *Relief* indicates how the
     menu should appear relative to the root window; for example, "raised"
     means the menu should appear to protrude.  The default is "raised".

  **-restrictwidth** *option* 

     Specifies how the menu width should be restricted according to the
     parent widget that posted it. *Option* can be one of the following
     "none".

     max
       The menu width will be the maximum of the calculated menu width and
       the parent widget width.

     min
       The menu width will be the minimum of the calculated menu width and
       the parent widget width.

     both
       The menu width will the same as the parent widget width.

     none
       Don't restrict the menu width. This is the default.
       
  **-takefocus** *bool*

     Provides information used when moving the focus from window to window
     via keyboard traversal (e.g., Tab and Shift-Tab).  If *bool* is "0",
     this means that this window should be skipped entirely during keyboard
     traversal.  "1" means that the this window should always receive the
     input focus.  An empty value means that the traversal scripts make the
     decision whether to focus on the window.  The default is "".

  **-textvariable** *varName* 

     Specifies the name of a global TCL variable that will be set to the
     label of the selected item.  If *varName* is "", no variable is
     used. The default is "".

  **-unpostcommand** *string*

     Specifies the TCL command to be invoked when the menu is unposted.  If
     *string* is "", no command is invoked. The default is "".

  **-width** *numPixels*

    Specifies the width in the *combomenu*.  *NumPixels* can be single
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

  **-xscrollbar** *widget*

     Specifies the name of a scrollbar widget to use as the horizontal
     scrollbar for this menu.  The scrollbar widget must be a child of the
     combomenu and doesn't have to exist yet.  At an idle point later, the
     combomenu will attach the scrollbar to widget, effectively packing the
     scrollbar into the menu.

  **-xscrollcommand** *string*

     Specifies the prefix for a command used to communicate with horizontal
     scrollbars.  Whenever the horizontal view in the widget's window
     changes, the widget will generate a Tcl command by concatenating the
     scroll command and two numbers. If this option is not specified, then
     no command will be executed.  The widget's initialization script
     will automatically set this for you.

  **-xscrollincrement** *numPixels*

     Sets the horizontal scrolling unit. This is the distance the menu is
     scrolled horizontally by one unit. *NumPixels* is a non-negative value
     indicating the width of the 3-D border drawn around the menu. The
     value may have any of the forms accept able to **Tk_GetPixels**.  The
     default is "20".

  **-yscrollbar** *widget*

     Specifies the name of a scrollbar widget to use as the vertical
     scrollbar for this menu.  The scrollbar widget must be a child of the
     combomenu and doesn't have to exist yet.  At an idle point later, the
     combomenu will attach the scrollbar to widget, effectively packing the
     scrollbar into the menu.

  **-yscrollcommand** *string*

     Specifies the prefix for a command used to communicate with vertical
     scrollbars.  Whenever the vertical view in the widget's window
     changes, the widget will generate a Tcl command by concatenating the
     scroll command and two numbers.  If this option is not specified, then
     no command will be executed.  The widget's initialization script
     will automatically set this for you.

  **-yscrollincrement** *numPixels*

     Sets the vertical scrolling unit.  This is the distance the menu is
     scrolled vertically by one unit. *NumPixels* is a non-negative value
     indicating the width of the 3-D border drawn around the menu. The
     value may have any of the forms accept able to **Tk_GetPixels**.  The
     default is "20".

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
 
   Searches for the next menu item that matches *string*.  Returns the
   index of the matching item or "-1" if no match is found.  *Switches* can
   be one of the following:

  **-decreasing** 

    Search menu items in order of the highest to lowest index.

  **-from** *item* 

    Specifies the menu item frow where to start searching.  *Item* may be a
    label, index, or tag, but may not represent more than one menu item.

  **-glob** 
  
     Indicates that *string* is glob-style pattern.  Matching is done in a
     fashion similar to that used by the TCL **glob** command.

  **-regexp** 

     Indicates that *string* is regular expression.  Matching is done in a
     fashion similar to that used by the TCL *regexp* command.

  **-type** *itemType*

     Specifies the type of menu items to search.  *ItemType* may be
     and of the types described in the **-type** option. 

  **-underline** 

     Specifies to match the items underlined character instead of its
     entire label.

*pathName* **index** *item* 
 
  Returns the index of *item*. *Item* may be a label, index, or tag, but
  may not represent more than one menu item.  If the item does not
  exist, "-1" is returned.
  
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
 
  Selects the *item and invokes the TCL command specified by *item*'s
  **-command** option. *Item* may be a label, index, or tag, but may not
  represent more than one menu item.
  
*pathName* **item cget** *item* *option*
 
  Returns the current value of the configuration option for *item* given by
  *option*.  *Option* may be any option described below for the **item
  configure** operation below. *Item* may be a label, index, or tag, but
  may not represent more than one menu item.

*pathName* **item configure** *item* ?\ *option* *value* ...\ ?
 
  Queries or modifies the configuration options of *item*.  *Item* may be a
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

    Specifies a textual field to be displayed to the right of the label.
    The accelerator typically describes a keystroke sequence that may be
    typed in the application to cause the same result as invoking the menu
    item.  The default is "".

  **-command** *string* 

    Specifies a TCL command to be invoked when *item* is selected. If
    *string* is "", then no command is executed.  The default is "".

  **-data** *string* 

    Specifies data to be associated with the menu item. *String* can be an
    arbitrary.  It is not used by the *combomenu* widget. The default is
    "".

  **-icon** *imageName* 

    Specifies the name of an image to be displayed as the icon for the
    menu item.  The icon is displayed to the left of the label.  If
    *imageName* is "", then no icon is display. The default is "".

  **-image** *imageName* 

    Specifies the name of an image to be displayed as the label for the
    menu item.  If *imageName* is "", then no image is displayed and the
    label text specified by the **-text** option is displayed. The default
    is "".
    
  **-indent** *numPixels* 

    Specifies the amount to indent the menu entry. *NumPixels* is a
    non-negative value indicating the how far to the right to indent the
    menu item. The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "0".
    
  **-menu** *menuName* 

    Specifies the path name of the submenu associated with this item.
    *MenuName* must be a *combomenu* widget and a child of *pathName*.
    This option is only used for *cascade* items.  The default is "".

  **-offvalue** *string*

    Specifies the value to store in the items's associated variable when
    the item is deselected.  This option only affects *checkbutton* items.
    The default is "".

  **-onvalue** *string*

    Specifies the value to store in the items's associated variable when
    the item is selected.  This option only affects *checkbutton* items.
    The default is "".

  **-state** *state*

    Specifies one of three states for the item: 

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

    Specifies the size of the check box of *checkbutton* items.
    *NumPixels* is a valid screen distance, such as \f(CW2\fR or \f(CW1.2i\fR.
    If this option isn't specified, then it defaults to "1".

  **-tags** *tagList* 

    Specifies a list of tags to associate with the menu item.  *TagList* is
    a list of tags.  Tags are a useful for referring to groups of menu
    items. Items can have any number of tags associated with them. Tags may
    refer to more than one menu item.  Tags should not be the same as
    labels or the two built-in tags: "all" and "end".  The default is "".

  **-text** *string* 

    Specifies the text to be displayed as the menu item's label. *String*
    can not be a number.  The default is "".

  **-tooltip** *string* 

    Specifies a string to be associated with the menu item. *String* can be
    an arbitrary.  The purpose of this option is to associate a tooltip
    description with the menu item. It is not used by the *combomenu*
    widget.  The default is "".

  **-type** *itemType* 

    Specifies the type of the menu item.  *Itemtype* can be "button",
    "cascade", "checkbutton", "radiobutton", or "separator". These
    menu types are described in the section MENU ITEMS.
    The default is "button".

  **-underline** *charIndex* 

    Specifies the index of the character to be underlined when displaying
    menus item.  In addition the underlined character is used in the
    *combomenu* widget's bindings.  When the menu is posted and the key
    associated with the underlined character is pressed, the item is
    selected.  *CharIndex* is the index of the character in the label,
    starting from zero.  If *charIndex* is not a valid index, no character
    is underlined. The default is -1.

  **-value** *string* 

    Specifies the value to be stored in the radiobutton item's associated
    global TCL variable (see the **-variable** option) when the item is
    selected.  *String* is a arbitrary string but should be unique among
    radiobutton items using the same TCL variable.  The default is "".

  **-variable** *varName* 

    Specifies the name of a global TCL variable to set whenever this
    radionbutton item is selected.  Changes in *varName* also cause the
    item to select or deselect itself.  The default value is "".

*pathName* **listadd** *labelList*  ?\ *option* *value* ...\ ?
 
  Adds one or more menu items to the menu from *labelList*.  For each label
  in *labelList* a new menu item is created with that label.  A menu item
  can not already exist with the label.  If one or more *option-value*
  pairs are specified, they modify each created menu item with the given
  option(s) to have the given value(s).  *Option* and *value* are described
  in the **item configure** operation.

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
 
  Indicates if the x and y coordinates specified are over the button region
  for this menu.  *X* and *y* are root coordinates.  This command uses the
  information set by the **post** operation to determine where the button
  region is.  Returns "1" if the coordinate is in the button region, "0"
  otherwise.

*pathName* **post** ?\ *switches* ...\ ? 
 
  Arranges for the *pathName* to be displayed on the screen. The position
  of *pathName* depends upon *switches*.

  The position of the *combomenu* may be adjusted to guarantee that the
  entire widget is visible on the screen.  This command normally returns an
  empty string.  If the **-postcommand** option has been specified, then
  its value is executed as a Tcl script before posting the menu and the
  result of that script is returned as the result of the post widget
  command.  If an error returns while executing the command, then the error
  is returned without posting the menu.

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
    Specifies the name of window to align the menu to.  Normally *combomenu*s
    are aligned to its parent window.  *Window* is the name of another
    widget.

*pathName* **postcascade** ?\ *item* ? 
 
  Posts the the *combomenu* associated with *item* (the menu is specified
  by the **-menu** option for menu items). This command is only affects
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
 
  Selects *item* in the menu. The item is drawn in its selected colors and
  its TCL command is invoked (see the **-command** menu item option).
  *Item* may be a label, index, or tag, but may not represent more than one
  menu item.
  
*pathName* **size**
 
  Returns the number of items in the menu.  
   
*pathName* **sort cget** *option*

  Returns the current value of the sort configuration option given by
  *option*. *Option* may have any of the values accepted by the **sort
  configure** operation. They are described below.

*pathName* **sort configure** ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?

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
  *option* for *styleName*.  *StyleName* is the name of a style created by
  the **style create** operaton.  *Option* may be any option described
  below for the **style configure** operation.
   
*pathName* **style configure** *styleName* ?\ *option* *value* ...\ ?
   
  Queries or modifies the configuration options for the style *styleName*.
  *StyleName* is the name of a style created by the **style create**
  operaton.  If no *option* argument is specified, this command returns a
  list describing all the available options for *pathName* (see
  **Tk_ConfigureInfo** for information on the format of this list).  If
  *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sublist of the value returned if no *option* is specified).
  If one or more *option-value* pairs are specified, then this command
  modifies the given widget option(s) to have the given value(s); in this
  case the command returns an empty string.  *Option* and *value* are
  described below.

  **-acceleratorfont** *fontName* 

    Specifies the font for the accelerator.  The default is "{Sans Serif}
    9".

  **-acceleratorforeground** *colorName* 

    Specifies the color of the accelerator.  The default is "black".

  **-activeacceleratorforeground** *colorName* 

    Specifies the color of the label when the menu item is active.  The
    default is "white".

  **-activeforeground** *colorName* 

    Specifies the active color of the label.  The default is
    "black".

  **-activerelief** *relief* 

    Specifies the relief of active menu items.  This determines the 3-D
    effect for the menu item.  *Relief* indicates how the item should
    appear relative to the menu window; for example, "raised" means the
    item should appear to protrude.  The default is "flat".
    
  **-background** *background* 

    Specifies the background of the menu item.  *Background* may be a color
    name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 

    Specifies the borderwidth of the menu item.  *NumPixels* is a
    non-negative value indicating the width of the 3-D border drawn around
    the menu item. The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "1".

  **-checkbuttoncolor** *colorName*

    Specifies the color of the check for *checkbutton* items.  The default is
    "red3".

  **-checkbuttonfillcolor** *colorName*

    Specifies the fill color of the box for *checkbutton* items. If
    *colorName* is "", then the box color is the background color of
    the menu item.  The default is "".

  **-checkbuttonoutlinecolor** *colorName*

    Specifies the outline color of the box for *checkbutton* items.  If
    *colorName* is "", then the no outline is drawn. The default is "".

  **-checkbuttonsize** *numPixels*

    Specifies the size of the box of for *checkbutton* items.  *NumPixels* is
    a non-negative value indicating the width and height of the check
    box. The value may have any of the forms accept able to Tk_GetPixels.
    The default is "12".

  **-disabledacceleratorforeground** *colorName* 

    Specifies the color of the accelerator of menu items that are
    disabled. The default is "grey90".

  **-disabledbackground** *background* 

    Specifies the background of menu items that are disabled.  *Background*
    may be a color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-disabledforeground** *colorName* 

    Specifies the color of the label for menu items that are disabled.  The
    default is "grey70".

  **-font** *colorName* 

    Specifies the font of labels in menu items.  The default is "{Sans
    Serif} 11".

  **-foreground** *colorName* 

    Specifies the color of labels in menu items.  The default is "black".

  **-radiobuttoncolor** *colorName*

    Specifies the color of the indicator circle for radiobutton items. The
    indicator circle is displayed when the radiobutton item is
    selected. The default is "red3".

  **-radiobuttonfillcolor** *colorName*

    Specifies the fill color of the circle for radiobutton items.  The
    default is "white".

  **-radiobuttonsize** *numPixels*

    Specifies the size of the circle for radiobutton items.  *NumPixels* is
    a non-negative value indicating the width and height of the radiobutton
    circle. The value may have any of the forms acceptable to
    *Tk_GetPixels*, such as "1.2i".  The default is "12".

  **-relief** *relief* 

    Specifies the 3-D effect for the border around the menu item.
    *Relief* specifies how the interior of the legend should appear
    relative to the menu; for example, "raised" means the item
    should appear to protrude from the menu, relative to the surface of
    the menu.  The default is "flat".

*pathName* **style create** *styleName* ?\ *option* *value* ...\ ?
 
  Creates a new style named *styleName*.  By default all menu items use the
  same set of global widget configuration options to specify the item's the
  color, font, borderwidth, etc.  Styles contain sets of configuration
  options that you can apply to a menu items (using the its **-style**
  option) to override their appearance. More than one item can use the same
  style. *StyleName* can not already exist.  If one or more
  *option*-*value* pairs are specified, they specify options valid for the
  **style configure** operation.  The name of the style is returned.
   
*pathName* **style delete** ? *styleName* ...\ ?
 
  Deletes one or more styles.  *StyleName* is the name of a style created
  by the **style create** operaton.  Styles are reference counted.  The
  resources used by *styleName* are not freed until no item is using it.
   
*pathName* **style exists** *styleName*
 
  Indicates if the style *styleName* exists in the widget. Returns "1" if
  it exists, "0" otherwise.
   
*pathName* **style names** ?\ *pattern* ...\ ?
 
  Returns the names of all the styles in the widget.  If one or more
  *pattern* arguments are provided, then the names of any style matching
  *pattern* will be returned. *Pattern* is a glob-style pattern.

*pathName* **type** *item*
 
  Returns the type of *item*.  The returned type is either "button",
  "cascade", "checkbutton", "radiobutton", or "separator".  *Item* may be a
  label, index, or tag, but may not represent more than one menu item.
   
*pathName* **unpost**
 
  Unposts the *combomenu* window so it is no longer displayed onscreen.  If
  one or more lower level cascaded menus are posted, they are unposted too.

*pathName* **value** *item*
 
   Returns the value associated with *item*.  The value is specified by the
   menu item's **-value** option.  *Item* may be a label, index, or tag,
   but may not represent more than one menu item.
   
*pathName* **xposition** *item*
 
   Returns the horizontal position of the item from left of the *combmenu*
   menu window.  The returned value is in pixels. Item* may be a label,
   index, or tag, but may not represent more than one menu item.
   
*pathName* **xview moveto** fraction
 
   Adjusts the horizontal view in the *combomenu* window so the portion of
   the menu starting from *fraction* is displayed.  *Fraction* is a number
   between 0.0 and 1.0 representing the position horizontally where to
   start displaying the menu.
   
*pathName* **xview scroll** *number* *what*
 
   Adjusts the view in the window horizontally according to *number* and
   *what*.  *Number* must be an integer.  *What* must be either "units" or
   "pages".  If *what* is "units", the view adjusts left or right by
   *number* units.  The number of pixel in a unit is specified by the
   **-xscrollincrement** option.  If *what* is "pages" then the view
   adjusts by *number* screenfuls.  If *number* is negative then the view
   if scrolled left; if it is positive then it is scrolled right.

*pathName* **yposition** *item*
 
   Returns the vertical position of the item from top of the *combmenu*
   menu window.  The returned value is in pixels. Item* may be a label,
   index, or tag, but may not represent more than one menu item.
   
*pathName* **yview moveto** fraction
 
   Adjusts the vertical view in the *combomenu* window so the portion of
   the menu starting from *fraction* is displayed.  *Fraction* is a number
   between 0.0 and 1.0 representing the position vertically where to start
   displaying the menu.
   
*pathName* **yview scroll** *number* *what*
 
   Adjusts the view in the window vertically according to *number* and
   *what*.  *Number* must be an integer.  *What* must be either "units" or
   "pages".  If *what* is "units", the view adjusts up or down by *number*
   units.  The number of pixels in a unit is specified by the
   **-yscrollincrement** option.  If *what* is "pages" then the view
   adjusts by *number* screenfuls.  If *number* is negative then earlier
   items become visible; if it is positive then later item becomes visible.
   
DEFAULT BINDINGS
----------------

There are many default class bindings for *combomenu* widgets.

EXAMPLE
-------

Create a *combomenu* widget with the **blt::combomenu** command.

 ::

    package require BLT

    # Create a new combomenu and add menu items to it.

    blt::combobutton .file -text "File" -menu .file.m \
      -xscrollbar .file.xs \
      -yscrollbar .file.ys 

    blt::combomenu .file.m 
    .file.m add -text "New Window" -accelerator "Ctrl+N" -underline 0 \
	-icon $image(new_window)
    .file.m add -text "New Tab" -accelerator "Ctrl+T" -underline 4 \
        -icon $icon(new_tab)
    .file.m add -text "Open Location..." -accelerator "Ctrl+L" -underline 5
    .file.m add -text "Open File..." -accelerator "Ctrl+O" -underline 0 \
       -icon $icon(open_file)
    .file.m add -text "Close Window" -accelerator "Ctrl+Shift+W" -underline 9
    .file.m add -text "Close Tab" -accelerator "Ctrl+W" -underline 0
    blt::tk::scrollbar .file.ysbar 
    blt::tk::scrollbar .file.xsbar 

Please note the following:

1. You can't use a Tk **menubutton** with *combomenu*\ s.  The menu is
   posted by either a **blt::combobutton** or **blt::comboentry**
   widget.

2. You specify scrollbar widgets with the **-xscrollbar** and
   **-yscrollbar** options.  The scrollbars do not already have to exist.

3. You create menu items with the **add** operation.  The type of item is
   specified by the **-type** option.  The default type is "button".

4. You don't pack the scrollbars.  This is done for you.

5. You don't have to specify the **-orient** or **-command** options to
   the scrollbars. This is done for you.


DIFFERENCES WITH TK MENUS
-------------------------

The **blt::combomenu** widget has several differences with the Tk **menu**
widget.

1. *Combomenu* item types are specified by the **-type** option.

2. *Combomenus* can not be torn off.

3. *Combomenus* can not be invoked by a Tk **menubutton**.

4. *Combomenus* are a single column.
   
KEYWORDS
--------

combomenu, widget
