===============
blt::listview
===============

----------------------------------------
Create and manipulate listview widgets.
----------------------------------------

:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::listview** *pathName* ?\ *option value* ... ?

DESCRIPTION
-----------

The **blt::listview** command creates and manages *listview* widgets.
The *listview* widget contains a list of items displayed either
vertically, horizontally, menu and optional scrollbars (vertical
and/or horizontal).  The scrollbars are automatically exposed or hidden as
necessary when the *listview* widget is resized.  Whenever the
*listview* window is smaller horizontally and/or vertically than the
actual menu, the appropiate scrollbar is exposed.

A *listview* is a widget that displays a list items.  There exist several
different types of items, each with different properties.  Items of
different types may be combined in a single menu.  List items are not
distinct widgets; the entire *listview* is one widget.

SYNTAX
------

The **blt::listview** command creates a new window using the *pathName*
argument and makes it into a *listview* widget.  The command has the
following form.

  **blt::listview** *pathName* ?\ *option value* ... ?

Additional options may be specified on the command line or in the option
database to configure aspects of the listview such as its background color
or scrollbars. The **blt::listview** command returns its *pathName*
argument.  There must not already exist a window named *pathName*, but
*pathName*'s parent must exist.

The normal size of the *listview* is the computed size to display all its
items. You can specify the maximum and minimum width and height of the
*listview* window using the **-width** and **-height** widget options.

ITEM LAYOUT
-----------

A listview is a widget that displays a collection of items arranged in rows
or columns.  There exist several different types of layout modes (specified
by the item's **-layout** option), each with different properties.  

A listview widget as the following layout modes: 

  **columns**
    Items are positioned in a column-by-column layout. The number of rows
    is determined from the height of the *listview* window.  Vertical
    scrolling has no effect.

  **icons**
    Items are positioned in a row-by-row layout.  The **-bigicon** image is
    used instead of the **-icon** image as the item's icon. Horizontal
    scrolling has no effect. The width of the item is determined from the
    icon and the text unless overriden by the **-maxwidth** option.

  **row**
    Items are positioned in a single row layout.  Horizontal scrolling
    has no effect.

  **rows**
    Items are positioned in a row-by-row layout. The number of rows
    is determined from the width of the *listview* window.  Horizontal
    scrolling has no effect.


REFERENCING LIST ITEMS
----------------------

List items may be referred to by either their index, label, or tag.

  **index**
    The number of the item.  Indices start from 0.  The index of an
    item may change as other items are added, deleted, moved, or sorted.
    There are also special non-numeric indices that can be used.

    **active**
      The index of the active item.

    **anchor**
       The index of the selection anchor.

    **end**
      The index of the last item.
      
    **first**
      The index of the first item that is not hidden or disabled.

    **focus**
      The index of the item that has focus.

    **last**
      The index of the last item that is not hidden or disabled.

    **mark**
       The index of the selection mark.

    **next**
      The next item that is not hidden or disabled.

    **previous**
      The previous item that is not hidden or disabled.
      
    **@**\ *x*\ ,\ *y*
      The index of the item that is located at the *x* and *y* screen
      coordinates.  If no item is at that point, then the index is "-1".

  **label**
    The label of the item (specified by the **-text** item option).

  **tag**
    A tag is a string associated with an item.  They are a useful for
    referring to groups of items. Items can have any number of tags
    associated with them (specified by the **-tags** item option).  A
    tag may refer to multiple items.  There are two built-in tags: "all" and
    "end".  Every item has the tag "all".  The last item in the list will
    have the tag "end".
     
If an item is specified by an integer (or one of the non-numeric indices)
it is assumed to be an index.  If it is specified by a string, it is first
tested if it's a valid label and then a tag.  Ideally you shouldn't have
tags, labels, or, indices that are the same.  They will always be
interpreted as indices or labels.  But you can also distinguish indices,
labels and tables by prefixing them with "index:", "label:", or "tag:"
(such as "label:12").

OPERATIONS
----------

All *listview* operations are invoked by specifying the widget's pathname,
the operation, and any arguments that pertain to that operation.  The
general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *listview* widgets:

*pathName* **activate** *itemName* 
  Redisplays *itemName* using its active colors and relief.  This typically
  is used by widget bindings to highlight list items when the pointer is
  moved over items in the list. Any previously active item is deactivated.
  *ItemName* may be a label, index, or tag, but may not represent more than
  one list item.

*pathName* **add** ?\ *option* *value* ...?
  Creates a new list item, adding it to the end of the list.  If one or
  more *option-value* pairs are specified, they modify the given list item
  option(s) to have the given value(s).  *Option* and *value* are described
  in the **item configure** operation.

*pathName* **bbox** *itemName* 
  Returns of list of four numbers describing the bounding box of *itemName*.
  The numbers represent the x and y root coordinates of two opposite
  corners of the box. *Item* may be a label, index, or tag, but may not
  represent more than one list item.

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
  "BltListview".  The resource name is the name of the widget::

    option add *BltListview.anchor n
    option add *BltListview.Anchor e

  The following widget options are available\:

  **-activebackground** *colorName* 
    Specifies the default color of the text when the item is active.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command.  This option may be
    overridden by the style's **-activebackground** option.
    The default is "white". 

  **-activeforeground** *colorName* 
    Specifies the default color of the label when the list item is active.
    This option may be overridden the style's **-activeforeground** option.
    The default is "white".

  **-activerelief** *relief* 
    Specifies the default relief of active list items.  This determines the
    3-D effect for the list item.  *Relief* indicates how the item should
    appear relative to the window; for example, "raised" means the
    item should appear to protrude.  This option may be overridden by 
    the style's **-activerelief** option. The default is "flat".
    
  **-background** *colorName* 
    Specifies the default background of the list items.  *ColorName* may be
    a color name or the name of a background object created by the
    **blt::background** command.  This option may be overridden the
    style's **-background** option. The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of *pathName*.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the window.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "0".

  **-command** *cmdString* 
    Specifies a TCL command to be invoked when a list item is selected:
    either by clicking on the list item or using the **select** operation.
    If *cmdString* is "", then no command is invoked. The default is "".

  **-disabledbackground** *colorName* 
    Specifies the default background of list items that are disabled.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command.  This option may be
    overridden the style's **-disabledbackground** option.  The
    default is "grey90".

  **-disabledforeground** *colorName* 
    Specifies the default color of the text of items that are
    disabled.  This option may be overridden by the style's
    **-disabledforeground** option. The default is "grey70".

  **-exportselection** *boolean* 
    Indicates if the selections are to be exported and copied to the
    clipboard.  The default is "0".

  **-focuscolor** *colorName* 
    Specifies the default color of the text for items that are
    disabled.  This option may be overridden the style's
    **-focuscolor** option. The default is "grey70".

  **-font** *fontName* 
    Specifies the default font of items.  The default is "{Sans Serif} 9".

  **-foreground** *colorName* 
    Specifies the default color of the text of items.  This option may
    be overridden by the style's **-foreground** option.  The
    default is "black".

  **-height** *numPixels* 
    Specifies the height in the *listview* window.  *NumPixels* can be
    single value or a list.  If *numPixels* is a single value it is a
    non-negative value indicating the height the list. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is a 2 element list, then this sets the minimum
    and maximum limits for the height of the list. The list will be at
    least the minimum height and less than or equal to the maximum. If
    *numPixels* is a 3 element list, then this specifies minimum, maximum,
    and nominal height or the list.  The nominal size overrides the
    calculated height of the list.  If *numPixels* is "", then the height
    of the list is calculated based on all the list items.  The default is
    "".

  **-highlightbackground** *colorName*
    Specifies the color of the traversal highlight region when *pathName*
    does not have the input focus.  *ColorName* may be a color name or the
    name of a background object created by the **blt::background** command.
    The default is "grey85".

  **-highlightcolor** *colorName*
    Specifies the color of the traversal highlight region when *pathName*
    has input focus.  *ColorName* may be a color name or the name of a
    background object created by the **blt::background** command. The
    default is "black".

  **-highlightthickness** *numPixels*
    Specifies a non-negative value for the width of the highlight rectangle
    to drawn around the outside of the widget.  *NumPixels* may have any of
    the forms acceptable to **Tk_GetPixels**.  If *numPixels* is "0", no
    focus highlight is drawn around the widget.  The default is "2".

  **-iconvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    name of the image representing the icon of the last selected list item.
    If *varName* is "", no variable is used. The default is "".

  **-itemborderwidth** *numPixels* 
    Specifies the default borderwidth of list items in the list.
    *NumPixels* is a non-negative value indicating the width of the 3-D
    border drawn around the item. The value may have any of the forms
    acceptable to **Tk_GetPixels**.  This option may be overridden by the
    style's **-borderwidth** option.  The default is "0".

  **-layoutmode** *modeName* 
    Specifies how items are positioned in the widget. *ModeName* can be
    one of the following.

    **column**
      Items are positioned in a column-by-column layout. The number
      of rows is dependent upon the height of the *listview* window.
      Vertical scrolling has no effect.
       
    **row**
      Items are positioned in a single row layout.  Horizontal scrolling
      has no effect.

    **icons**
      Items are positioned in a row-by-row layout.  The **-bigicon**
      value is used instead of the **-icon** value. Horizontal scrolling
      has no effect. The width of the item is determined from the
      icon and the text unless overriden by the **-maxwidth** option.

  **-maxwidth** *numPixels* 
    Specifies the maximum width of an item in **icons** layout mode.
    *NumPixels* is a non-negative value indicating the maximum width of any
    item in the list.  *NumPixels* may have any of the forms acceptable to
    **Tk_GetPixels**.  If *numPixels* is 0, the width of an item is the
    maximum width of its icon and label. The default is "1i".

  **-relief** *relief* 
    Specifies the 3-D effect for *pathName*.  *Relief* indicates how the
    window should appear relative to the root window; for example, "raised"
    means the window should appear to protrude.  The default is "raised".

  **-selectbackground** *colorName* 
    Specifies the default background of list items that are selected.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command.  This option may be
    overridden by the style's **-selectbackground** option.  The
    default is "grey90".

  **-selectcommand** *cmdString* 
    Specifies a TCL command to be invoked when an list item is selected:
    either by clicking on the list item or using the **select** operation.
    If *cmdString* is "", then no command is invoked. The default is "".

  **-selectforeground** *colorName* 
    Specifies the default color of the text of items that are
    selected.  This option may be overridden the style's
    **-selectforeground** option. The default is "grey70".

  **-selectmode** *mode*
    Specifies the selection mode. *Mode* can be any of the following.

    **single**
      Only one item can be selected at a time.
    **multiple**
      More than one item can be selected.

    The default is "single".

  **-selectordered** *boolean* 
    Indicates whether to return the list of selected items in the order
    they are found in the list or as they were selected.  This option only
    matters when more than one item is selected (**-selectmode** option is
    "multiple").  If *boolean* is true, then items are returned in the
    list's order. If false, items will be returned in the order that they
    were selected. The default is "0".

  **-selectrelief** *relief* 
    Specifies the default relief of selected list items.  This determines
    the 3-D effect for the list item.  *Relief* indicates how the item
    should appear relative to the widget window; for example, "raised"
    means the item should appear to protrude.  This option may be
    overridden by the style's **-selectrelief** option. The
    default is "flat".

  **-takefocus** *bool*
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *bool* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts make the
    decision whether to focus on the window.  The default is "".

  **-textvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    label of the last selected item.  If *varName* is "", no variable is
    used. The default is "".

  **-width** *numPixels*
   Specifies the width in the *listview*.  *NumPixels* can be single
   value or a list.  If *numPixels* is a single value it is a non-negative
   value indicating the width the window. The value may have any of the
   forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
   *numPixels* is a 2 element list, then this sets the minimum and maximum
   limits for the width of the window. The indow will be at least the minimum
   width and less than or equal to the maximum. If *numPixels* is a 3
   element list, then this specifies minimum, maximum, and nominal width
   or the window.  The nominal size overrides the calculated width of the
   window.  If *numPixels* is "", then the width of the window is calculated
   based on the widths of all the list items.  The default is "".

  **-xscrollcommand** *cmdPrefix*
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window
    changes, the widget will generate a TCL command by concatenating
    *cmdPrefix* with two numbers. If this option is not specified, then no
    command will be executed.  

  **-xscrollincrement** *numPixels*
    Sets the horizontal scrolling unit. This is the distance the menu is
    scrolled horizontally by one unit. *NumPixels* is a non-negative value
    indicating the width of the 3-D border drawn around the menu. The
    value may have any of the forms accept able to **Tk_GetPixels**.  The
    default is "20".

  **-yscrollcommand** *cmdPrefix*
    Specifies the prefix for a command used to communicate with vertical
    scrollbars.  Whenever the vertical view in the widget's window changes,
    the widget will generate a TCL command by concatenating *cmdPrefix*
    with two numbers.  If this option is not specified, then no command
    will be executed.  

  **-yscrollincrement** *numPixels*
    Sets the vertical scrolling unit.  This is the distance the menu is
    scrolled vertically by one unit. *NumPixels* is a non-negative value
    indicating the width of the 3-D border drawn around the menu. The
    value may have any of the forms accept able to **Tk_GetPixels**.  The
    default is "20".

*pathName* **curselection** 
  Returns a list containing the indices of all of the items that are
  currently selected.  If there are no items are selected, then the empty
  string is returned.

*pathName* **deactivate** 
  Redisplays all list items using their normal colors.  This typically is
  used by widget bindings to un-highlight list items as the pointer is
  moved over the menu. 

*pathName* **delete** *itemName*\ ...
  Deletes one or more items from the menu. *ItemName* may be a label, index, or
  tag and may refer to multiple items (example: "all"). 

*pathName* **exists** *itemName*
  Returns *itemName* exists in the widget. *ItemName* may be a label,
  index, or tag, but may not represent more than one list item.  Returns
  "1" is the item exists, "0" otherwise.
  
*pathName* **find** *pattern* ?\ *switches* ... ?
  Searches for the next list item that matches *string*.  Returns the
  index of the matching item or "-1" if no match is found.  *Switches* can
  be one of the following:

  **-any** 
    Search all items: hidden, disabled, etc.

  **-count** *number*
    Stop searching after locating *number* of items.

  **-disabled** 
    Search disabled items.

  **-from** *itemName* 
    Specifies the first item from where to start searching.  *ItemName* may
    be a label, index, or tag, but may not represent more than one list
    item. The default is the first item.

  **-hidden** 
    Search hidden items.

  **-reverse** 
    Reverses the order of the search.  Normally items are search from
    low index to high index.  If this switch is set, items are searched
    from high index to low index.

  **-to** *itemName* 
    Specifies the last item to search.  *ItemName* may be a label, index,
    or tag, but may not represent more than one list item.  The default
    is the last item.

  **-type** *searchType*
    Specifies the type of matching to perform.  *SearchType* may be
    any of the following.

    **exact**
      Indicates that *pattern* should be matched exactly. 

    **glob**
      Indicates that *pattern* is glob-style pattern.  Matching is done in a
      fashion similar to that used by the TCL **glob** command.

    **regexp** 
      Indicates that *pattern* is a regular expression.  Matching is done
      in a fashion similar to that used by the TCL **regexp** command.

  **-wrap** 
    Allow the search to wrap for the end of the list back to the beginning.

*pathName* **index** *itemName* 
  Returns the index of *itemName*. *ItemName* may be a label, index, or
  tag, but may not represent more than one list item.  If the item does not
  exist, "-1" is returned.
  
*pathName* **insert after** *itemName* ?\ *option *value* ... ? 
  Creates a new list item and inserts it after *itemName*.  Normally list items
  are appended to the end of the menu, but this command allows you to
  specify its location. Note that this may change the indices of previously
  created list items. *Item* may be a label, index, or tag, but may not
  represent more than one list item. If one or more *option-value* pairs
  are specified, they modifies the given list item option(s) to have the
  given value(s).  *Option* and *value* are described in the **item
  configure** operation.
  
*pathName* **insert at** *itemName* ?\ *option *value* ... ? 
  Creates a new list item and inserts it at the index specified by *itemName*.
  Normally list items are appended to the end of the menu, but this command
  allows you to specify its location. Note that this may change the indices
  of previously created list items. *Item* may be a label, index, or tag,
  but may not represent more than one list item. If one or more
  *option-value* pairs are specified, they modifies the given list item
  option(s) to have the given value(s).  *Option* and *value* are described
  in the **item configure** operation.
  
*pathName* **insert before** *itemName* ?\ *option *value* ... ? 
  Creates a new list item and inserts it before *itemName*.  Normally menu
  items are appended to the end of the menu, but this command allows you to
  specify its location. Note that this may change the indices of previously
  created list items. *Item* may be a label, index, or tag, but may not
  represent more than one list item. If one or more *option-value* pairs
  are specified, they modifies the given list item option(s) to have the
  given value(s).  *Option* and *value* are described in the **item
  configure** operation.
  
*pathName* **invoke** *itemName* 
  Selects the *item and invokes the TCL command specified by *item*'s
  **-command** option. *Item* may be a label, index, or tag, but may not
  represent more than one list item.
  
*pathName* **item cget** *itemName* *option*
  Returns the current value of the configuration option for *item* given by
  *option*.  *Option* may be any option described below for the **item
  configure** operation below. *ItemName* may be a label, index, or tag, but
  may not represent more than one list item.

*pathName* **item configure** *itemName* ?\ *option* *value* ... ?
  Queries or modifies the configuration options of *itemName*.  *ItemName*
  may be a label, index, or tag.  If no *option* is specified, returns a
  list describing all the available options for *itemName* (see
  **Tk_ConfigureInfo** for information on the format of this list).  If
  *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sublist of the value returned if no *option* is specified).
  In both cases, *itemName* may not represent more than one list item.
  
  If one or more *option-value* pairs are specified, then this command
  modifies the given option(s) to have the given value(s); in this case
  *itemName* may refer to mulitple items (example: "all").  *Option* and
  *value* are described below.

  **-bigicon** *imageName* 
    Specifies the name of an image to be displayed as the icon for the item
    when in **icons** layout mode.  The icon is displayed above the label.
    If *imageName* is "", then the **-icon** option is used.  If no icon
    is set, none is displayed. The default is "".

  **-command** *cmdPrefix* 
    Specifies a TCL command to be invoked when an item is selected. If
    *cmdPrefix* is "", then no command is executed.  The default is "".

  **-data** *string* 
    Specifies data to be associated with the list item. *String* can be an
    arbitrary.  It is not used by the *listview* widget. The default is
    "".

  **-icon** *imageName* 
    Specifies the name of an image to be displayed as the icon for the item
    in both **row** and **column** layout modes.  The icon is displayed to
    the left of the label.  If *imageName* is "", then no icon is
    display. The default is "".

  **-image** *imageName* 
    Specifies the name of an image to be displayed as the label for the
    item.  If *imageName* is "", then no image is displayed and the text
    specified by the **-text** option is displayed. The default is "".
    
  **-indent** *numPixels* 
    Specifies the amount to indent the list item. *NumPixels* is a
    non-negative value indicating the how far to the right to indent the
    list item. The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "0".
    
  **-state** *state*
    Specifies one of two states for the item: 

    **normal**
      In normal state the item is displayed using the **-foreground**
      and **-background** options.

    **disabled**
      Disabled state means that the item should be insensitive: the default
      bindings will not activate or invoke the item.  In this state
      the item is displayed according to the **-disabledforeground** 
      and the **-disabledbackground** options.

    The default is "normal".

  **-style** *styleName*
    Specifies the name of a style to use for *itemName*.  This style will
    override the global widget options for the item.  *StyleName* is the
    name of a style returned by the **style create** operation. If
    *styleName* is "", then the global options are used. The default is "".

  **-tags** *tagList* 
    Specifies a list of tags to associate with the item.  *TagList* is a
    list of tags.  Tags are a useful for referring to groups of
    items. Items can have any number of tags associated with them. Tags may
    refer to more than one list item.  Tags should not be the same as
    labels or the two built-in tags: "all" and "end".  The default is "".

  **-text** *string* 
    Specifies the text to be displayed as the item's label.  The default is
    "".

  **-tooltip** *string*
    Specifies a string to be associated with the item. *String* can be any
    character string.  This option isn't used by the widget.  Its purpose
    is to associate text (such as a tooltip description) with the item.
    The default is "".

  **-type** *string* 
    Specifies a string to be used to sort the item.  *String* is an
    arbitrary string that can be used describe the type of an item.  This
    field is then used when sorting items.  The default is "".

*pathName* **listadd** *itemsList*  ?\ *option* *value* ... ?
  Adds one or more list items to the list from *itemsList*.  For each label
  in *itemsList* a new list item is created with that label.  A list item
  can not already exist with the label.  If one or more *option-value*
  pairs are specified, they modify each created list item with the given
  option(s) to have the given value(s).  *Option* and *value* are described
  in the **item configure** operation.

*pathName* **names** ?\ *pattern* ... ?
  Returns the labels of all the items in the list.  If one or more
  *pattern* arguments are provided, then the label of any item matching
  *pattern* will be returned. *Pattern* is a glob-style pattern.

*pathName* **nearest** *x* *y*
  Returns the index of the list item closest to the coordinates specified.
  *X* and *y* are root coordinates.

*pathName* **next** *itemName* 
  Moves the focus to the next list item from *itemName*.  *ItemName* may be
  a label, index, or tag, but may not represent more than one list item.

*pathName* **previous** *itemName*
  Moves the focus to the previous list item from *itemName*.  *ItemName*
  may be a label, index, or tag, but may not represent more than one list
  item.

*pathName* **scan dragto** *x* *y* This command computes the difference
  between *x* and *y* and the coordinates to the last **scan mark** command
  for the widget.  It then adjusts the view by 10 times the difference in
  coordinates.  This command is typically associated with mouse motion
  events in the widget, to produce the effect of dragging the item list at
  high speed through the window.  The return value is an empty string.
   
*pathName* **scan mark** *x* *y*
  Records *x* and *y* and the current view in the menu window; to be used
  with later **scan dragto** commands. *X* and *y* are window coordinates
  (i.e. relative to menu window).  Typically this command is associated
  with a mouse button press in the widget.  It returns an empty string.

*pathName* **see** *itemName* 
  Scrolls the menu so that *itemName* is visible in the widget's window.
  *ItemName* may be a label, index, or tag, but may not represent more than
  one list item.
  
*pathName* **selection anchor** *itemName*
  Sets the selection anchor to the item given by *itemName*.  If *itemName*
  refers to a non-existent item, then the closest item is used.  The
  selection anchor is the end of the selection that is fixed while dragging
  out a selection with the mouse.  The special id **anchor** may be used to
  refer to the anchor item.

*pathName* **selection clear** *firstItem* ?\ *lastItem*\ ?
  Removes the items between *firstItem* and *lastItem* (inclusive) from the
  selection.  Both *firstItem* and *lastItem* are ids representing a range of
  items.  If *lastItem* isn't given, then only *firstItem* is deselected.
  Items outside the selection are not affected.

*pathName* **selection clearall**
  Clears the entire selection.  

*pathName* **selection includes** *itemName*
  Returns 1 if the item given by *itemName* is currently selected, 0 if it
  isn't.

*pathName* **selection mark** ?\ *itemName*\ ?
  Sets the selection mark to the item given by *itemName*.  This causes the
  range of items between the anchor and the mark to be temporarily added
  to the selection.  The selection mark is the end of the selection that is
  fixed while dragging out a selection with the mouse.  The special id
  **mark** may be used to refer to the current mark item.  If *itemName*
  refers to a non-existent item, then the mark is ignored.  Resetting the
  mark will unselect the previous range.  Setting the anchor finalizes the
  range.

*pathName* **selection present**
  Returns 1 if any items currently selected and 0 otherwise.

*pathName* **selection set** *firstItem* ?\ *lastItem*\ ?
  Selects all of the items in the range between *firstItem* and *lastItem*,
  inclusive, without affecting the selection state of items outside that
  range. If *lastItem* isn't given, then only *firstItem* is set.

*pathName* **selection toggle** *firstItem* ?\ *lastItem*\ ?
  Selects/deselects items in the range between *firstItem* and *lastItem*,
  inclusive, from the selection.  If a item is currently selected, it
  becomes deselected, and visa versa. If *lastItem* isn't given,
  then only *firstItem* is toggled.

*pathName* **size**
  Returns the number of items in the list.  
   
*pathName* **sort cget** *option*
  Returns the current value of the sort configuration option given by
  *option*. *Option* may have any of the values accepted by the **sort
  configure** operation. They are described below.

*pathName* **sort configure** ?\ *option*\ ? ?\ *value*\ ? ?\ *option* *value* ... ?
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
    Automatically resort the list items anytime the items are added
    deleted, or changed.

  **-by** *what*
    Indicates to sort items either by their type or text label.
    *What* can be **text** or **type**. By default the items are sorted
    by their labels.

  **-command** *cmdPrefix*
    Specifies *cmdPrefix* as a TCL command to use for comparing items.  The
    items to compare are appended as additional arguments to *cmdPrefix*
    before evaluating the TCL command. The command should return an
    integer less than, equal to, or greater than zero if the first item
    is to be considered less than, equal to, or greater than the second,
    respectively.

  **-decreasing** 
    Sort the items highest to lowest. By default items are sorted
    lowest to highest.

  **-dictionary** *boolean*
     Use dictionary-style comparison. This is the same as *ascii*
     except (a) case is ignored except as a tie-breaker and (b) if two
     strings contain embedded numbers, the numbers compare as integers,
     not characters.  For example, in -dictionary mode, "bigBoy" sorts
     between "bigbang" and "bigboy", and "x10y" sorts between "x9y" and
     "x11y".  

*pathName* **sort once**  ?\ *option* *value* ... ?
  Sorts the list items using the current set of sort configuration values.
  *Option* and *value* are described above for the **sort configure**
  operation.
  
*pathName* **style cget** *styleName* *option*
  Returns the current value of the style configuration option given by
  *option* for *styleName*.  *StyleName* is the name of a style created by
  the **style create** operaton.  *Option* may be any option described
  below for the **style configure** operation.
   
*pathName* **style configure** *styleName* ?\ *option* *value* ... ?
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

  **-activebackground** *colorName* 
    Specifies the background of the item when it is active.  *ColorName*
    may be a color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-activeforeground** *colorName* 
    Specifies the text color of the item when it is active.  The default is
    "black".

  **-activerelief** *relief* 
    Specifies the relief of the item when it is active.  This determines
    the 3-D effect for the item.  *Relief* indicates how the item should
    appear relative to the widget window; for example, "raised" means the
    item should appear to protrude.  The default is "flat".
    
  **-background** *colorName* 
    Specifies the background of the item.  *ColorName* may be a color
    name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of the item.  *NumPixels* is a
    non-negative value indicating the width of the 3-D border drawn around
    the list item. The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "1".

  **-disabledbackground** *colorName* 
    Specifies the background of the item when it is disabled.  *ColorName*
    may be a color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-disabledforeground** *colorName* 
    Specifies the color of the text for the item when it is disabled.  The
    default is "grey70".

  **-font** *fontName* 
    Specifies the font of the text for the item.  The default is "{Sans
    Serif} 11".

  **-foreground** *colorName* 
    Specifies the color of the text for the item.  The default is "black".

  **-relief** *relief* 
    Specifies the 3-D effect for the border around the list item.
    *Relief* specifies how the interior of the legend should appear
    relative to the menu; for example, "raised" means the item
    should appear to protrude from the menu, relative to the surface of
    the menu.  The default is "flat".

  **-selectbackground** *colorName* 
    Specifies the background color of the item when it is selected.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "skyblue4".

  **-selectforeground** *colorName* 
    Specifies the color of the text of the item when it is selected.  The
    default is "white".

  **-selectrelief** *relief* 
    Specifies the relief of the item when it is selected.  This determines
    the 3-D effect for the item.  *Relief* indicates how the item should
    appear relative to the widget window; for example, "raised" means the
    item should appear to protrude.  The default is "flat".
    
*pathName* **style create** *styleName* ?\ *option* *value* ... ?
  Creates a new style named *styleName*.  By default all list items use the
  same set of global widget configuration options to specify the item's the
  color, font, borderwidth, etc.  Styles contain sets of configuration
  options that you can apply to a list items (using the its **-style**
  option) to override their appearance. More than one item can use the same
  style. *StyleName* can not already exist.  If one or more
  *option*-*value* pairs are specified, they specify options valid for the
  **style configure** operation.  The name of the style is returned.
   
*pathName* **style delete** ?\ *styleName* ... ?
  Deletes one or more styles.  *StyleName* is the name of a style created
  by the **style create** operaton.  Styles are reference counted.  The
  resources used by *styleName* are not freed until no item is using it.
   
*pathName* **style exists** *styleName*
  Indicates if the style named *styleName* exists in the widget. Returns
  "1" if it exists, "0" otherwise.
   
*pathName* **style names** ?\ *pattern* ... ?
  Returns the names of all the styles in the widget.  If one or more
  *pattern* arguments are provided, then the names of any style matching
  *pattern* will be returned. *Pattern* is a **glob**-style pattern.

*pathName* **table attach** *tableName* ?\ *option value* ... ?
  Attaches a BLT data table as the data source for the widget. *TableName*
  is the name of a data table created by the **blt::datatable** command.
  You must specify the columns in the table that contain specific
  information.  *Option* and *value* can be any of the following.
  
  **-bigicon** *columnName* 
    Specifies the name of the column in *tableName* to that holds the
    image names of the big icons used in **icons** layout mode.

  **-icon** *columnName* 
    Specifies the name of the column in *tableName* to that holds the image
    names of the small icons used in **row** and **column** layout modes.
   
  **-text** *columnName* 
    Specifies the name of the column in *tableName* to that holds the string
    to be used for the item text.

  **-type** *columnName* 
    Specifies the name of the column in *tableName* to that holds the string
    to be used for the item type.

*pathName* **table unattach** 
  Unlinks the current table.

*pathName* **tag add** *tag* ?\ *itemName* ... ?
  Adds the tag to one of more items. *Tag* is an arbitrary string that can
  not start with a number.  *ItemName* may be a label, index, or tag and
  may refer to multiple items (example: "all").
  
*pathName* **tag delete** *tag* ?\ *itemName* ... ?
  Deletes the tag from one or more items. *ItemName* may be a label, index,
  or tag and may refer to multiple items (example: "all").
  
*pathName* **tag exists** *itemName* ?\ *tag* ... ?
  Indicates if the item has any of the given tags.  Returns "1" if
  *itemName* has one or more of the named tags, "0" otherwise.  *ItemName*
  may be a label, index, or tag and may refer to multiple items (example:
  "all").

*pathName* **tag forget** *tag*
  Removes the tag *tag* from all items.  It's not an error if no
  items are tagged as *tag*.

*pathName* **tag get** *itemName* ?\ *pattern* ... ?
  Returns the tag names for a given item.  If one of more pattern
  arguments are provided, then only those matching tags are returned.

*pathName* **tag items** *tag*
  Returns a list of items that have the tag.  If no item is tagged as
  *tag*, then an empty string is returned.

*pathName* **tag names** ?\ *itemName*\ ... ?
  Returns a list of tags used by the *listview* widget.  If one or more
  *itemName* arguments are present, any tag used by *itemName* is returned.

*pathName* **tag set** *itemName* ?\ *tag* ... ?
  Sets one or more tags for a given item.  *ItemName* may be a label,
  index, or tag and may refer to multiple items (example: "all").  Tag
  names can't start with a digit (to distinquish them from indices) and
  can't be a reserved tag ("end" or "all").

*pathName* **tag unset** *itemName* ?\ *tag* ... ?
  Removes one or more tags from a given item. *ItemName* may be a label,
  index, or tag and may refer to multiple items (example: "all").  Tag
  names that don't exist or are reserved ("end" or "all") are silently
  ignored.

*pathName* **xposition** *itemName*
  Returns the horizontal position of the item from left of the *listview*
  window.  The returned value is in pixels. *ItemName* may be a label,
  index, or tag, but may not represent more than one list item.
   
*pathName* **xview moveto** fraction
  Adjusts the horizontal view in the *listview* window so the portion of
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

*pathName* **yposition** *itemName*
  Returns the vertical position of the item from top of the *listview*
  window.  The returned value is in pixels. *ItemName* may be a label,
  index, or tag, but may not represent more than one list item.
   
*pathName* **yview moveto** fraction
  Adjusts the vertical view in the *listview* window so the portion of
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

There are many default class bindings for *listview* widgets.

There are class bindings that supply listview widgets their default
behaviors. The following event sequences are set by default for listview
widgets. (via the class bind tag "BltListView"):

  **<ButtonPress-2>** 
    Starts scanning. 
  **<B2-Motion>** 
    Adjusts the scan.
  **<ButtonRelease-2>**
    Stops scanning.
  **<B1-Leave>** 
    Starts auto-scrolling.
  **<B1-Enter>**
    Starts auto-scrolling 
  **<KeyPress-Up>** 
    Moves the focus to the previous item.
  **<KeyPress-Down>** 
    Moves the focus to the next item.
  **<KeyPress-Prior>** 
    Moves the focus to first item.  Closed or hidden entries are ignored.
  **<KeyPress-Next>** 
    Move the focus to the last item. Closed or hidden entries are ignored.
  **<KeyPress-space>** 
    In "single" select mode this selects the item.  In "multiple" mode,
    it toggles the item (if it was previous selected, it is not
    deselected).
  **<KeyRelease-space>** 
    Turns off select mode.
  **<KeyPress-Return>** 
    Sets the focus to the current item.
  **<KeyRelease-Return>** 
    Turns off select mode.
  **<KeyPress>** 
    Moves to the next item whose label starts with the letter typed.
  **<KeyPress-Home>** 
    Moves the focus to first item.  Disabled or hidden entries
    are ignored.
  **<KeyPress-End>** 
    Move the focus to the last item. Disabled or hidden entries
    are ignored.

EXAMPLE
-------

Create a *listview* widget with the **blt::listview** command. 

::

    package require BLT

    # Create a scrollset to use with the listview widget.
    blt::listview .listview \
	-width 5i -height 2i \
        -layoutmode "columns" \
	-xscrollcommand { .xs set } 

Create two image to use as a big and small icon.  Typically the
small icon is 16x16 pixels, while the big icon is 64x64 pixels.

::

    image create picture smallIcon -file mySmallIcon.png
    image create picture bigIcon -file myBigIcon.png

Add items to the widget that are directory entries.  Use the
file extension as the item type.

::

    # Add items to the widget
    foreach f [lsort [glob -nocomplain ~/*]] {
	set name [file tail $f]
	set ext [file ext $name]
	set ext [string trimleft $ext .]
	if { [file isdir $f] } {
	    set ext .dir
	}
	.listview add -text $name -icon smallIcon -type $ext -bigicon bigIcon
    }

Connect a scroll bar and pack the widgets.

::

    blt::tk::scrollbar .xs -command { .listview xview } -orient horizontal

    blt::table . \
	0,0 .list -fill both  \
	1,0 .xs -fill x 


KEYWORDS
--------

listview, widget

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
