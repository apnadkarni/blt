
============
blt::paneset
============

--------------------------------------
Create and manipulate paneset widgets.
--------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::paneset** *pathName* ?\ *option* *value* ... ?

DESCRIPTION
-----------

The *paneset* widget displays a set of panes that contain embedded Tk
widgets divided by sashes. *Paneset* widgets are useful when you want to
divide an area into two or more parts, with the relative size of the parts
controlled by the user. A sash (groove) is drawn between the portions with
a handle that the user can drag to change the ratio. The division can
either be horizontal or vertical.

INTRODUCTION
------------

The *paneset* widget displays embedded Tk widgets as panes divided by one
or more sashes that separate the panes.  The panes act as containers for
the embedded Tk widgets.  The relative size of the panes can be changed by
moving the sash.  *Paneset* widgets are useful when you want to divide an
area into two or more parts, with the relative size of the parts controlled
by the user. A sash (groove) is drawn between the portions with a handle
that the user can drag to change the ratio. The division can either be
horizontal or vertical.

The embedded Tk widgets of a *paneset* must be children of the *paneset*
widget.

SYNTAX
------

  **blt::paneset** *pathName* ?\ *option* *value* ... ?

The **blt::paneset** command creates a new window *pathName* and makes it
into a *paneset* widget.  At the time this command is invoked, there must
not exist a window named *pathName*, but *pathName*'s parent must exist.
Additional options may be specified on the command line or in the option
database to configure aspects of the widget such as its colors and font.
See the widget's **configure** operation below for the exact details about
what *option* and *value* pairs are valid.

If successful, **paneset** returns the path name of the widget.  It also
creates a new TCL command by the same name.  You can use this command to
invoke various operations that query or modify the widget.  The general
form is:

  *pathName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available are described in the
`OPERATIONS`_ section below.

REFERENCING PANES
------------------

Panes can be referenced by using either an index, label, or tag.

*index*
  The number of the pane.  Indices start from 0.  The index number of a
  pane can change as other panes are added, deleted, or moved.  There are
  also special non-numeric indices that can be used.

  **active**
    This is the pane whose sash is where the mouse pointer is currently
    located.  When a sash is active, it is drawn using its active colors.
    The **active** index is changes when you move the mouse pointer over
    another pane's sash or by using the **activate** operation. Note
    that there can be only one active pane at a time.

  **end**
    The index of the last pane.
    
  **first**
    The index of the first pane that is not hidden or disabled.

  **last**
    The index of the last pane that is not hidden or disabled.

*label*
  The name of the pane.  This is usually in the form "pane0", "pane1",
  etc., although you can specify the name of the pane.

*tag*
  A tag is a string associated with an pane.  They are a useful for
  referring to groups of panes. Panes can have any number of tags
  associated with them (specified by the **-tags** item option).  A
  tag may refer to multiple panes.  There is one built-in tag: "all".
  Every pane has the tag "all".  

If a pane is specified by an integer (or one of the non-numeric indices)
it is assumed to be an index.  If it is specified by a string, it is first
tested if it's a valid label and then a tag.  Ideally you shouldn't have
tags, labels, or, indices that are the same.  They will always be
interpreted as indices or labels.  But you can also distinguish indices,
names and tables by prefixing them with "index:", "label:", or "tag:"
(such as "label:12").

OPERATIONS
----------

All *paneset* operations are invoked by specifying the widget's pathname,
the operation, and any arguments that pertain to that operation.  The
general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *paneset* widgets:

*pathName* **add** ?\ *label*\ ? ?\ *option* *value* ...?
  Creates a new pane, appending it to the end of the list of panes in the
  *paneset* widget. If no *label* argument is present, then the name of
  the pane is automatically generated in the form "pane0", "pane1", etc.
  If a *label* argument is present, then this is the name of the new pane.
  *Label* can not start with a dash "-" or be the name of another pane.
  The name of the new pane is returned.

  If one or more *option-value* pairs are specified, they modify the given
  pane option(s) to have the given value(s).  *Option* and *value* are
  described in the **pane configure** operation.

*pathName* **cget** *option*  
  Returns the current value of the widget configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **configure** ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of the *paneset* widget.
  If no *option* is specified, this command returns a list describing all
  the available options for *pathName* (see **Tk_ConfigureInfo** for
  information on the format of this list).  If *option* is specified with
  no *value*, then a list describing the one named option (this list will
  be identical to the corresponding sub-list of the value returned if no
  *option* is specified) is returned.  If one or more *option-value* pairs
  are specified, then this command modifies the given widget option(s) to
  have the given value(s); in this case the command returns an empty
  string.  *Option* and *value* are described below.

  Widget configuration options may be set either by the **configure**
  operation or the Tk **option** command.  The resource class is
  **BltPaneset**.  The resource name is the name of the widget::

    option add *BltPaneset.anchor n
    option add *BltPaneset.Anchor e

  The following widget options are available\:

  **-activesashcolor** *bgName* 
    Specifies the background color of the pane's sash when it is active.
    *BgName* may be a color name or the name of a background object
    created by the **blt::background** command.  
    The default is "grey90". 

  **-activesashelief** *reliefName* 
    Specifies the default relief when a pane's sash is active.  This
    determines the 3-D effect for the sash.  *ReliefName* indicates how the
    pane should appear relative to the *paneset* window. Acceptable values
    are **raised**, **sunken**, **flat**, **ridge**, **solid**, and
    **groove**. For example, "raised" means the sash should appear to
    protrude.  The default is "flat".
    
  **-background** *bgName* 
    Specifies the default background of the widget including its panes.
    *BgName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "grey85".
    
  **-disabledsashcolor** *bgName* 
    Specifies the background color of the pane's sash when it is disabled.
    *BgName* may be a color name or the name of a background object
    created by the **blt::background** command.  
    The default is "grey90". 

  **-height** *numPixels*
    Specifies the height of the *paneset* window.  *NumPixels* is a
    non-negative value indicating the height the widget. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is "0" and the **-orient** option is
    "horizontal", then the height calculated to display all the panes.
    The default is "0".

  **-mode** *modeName*
    Specifies the mode to use to arrange panes in the *paneset*.
    *modeName* may be one of the following:

    **givetake**
       Adjust panes to the left/right or top/bottom (depending on the
       orientation of the widget) to the active sash.  The panes are
       adjusted portionally.  

    **slinky**
       Adjust panes to the immediate left/right or top/bottom of the
       active sash.  If there is more space than the panes occupy,
       the extra space is spread evenly to all the panes on that side.
       This mode only matters is there are more than two panes.

    **spreadsheet**
       Adjust only the pane to the left of the sash and the last pane in
       the *paneset*. This resembles adjusting the columns of a
       spreadsheet.  This mode only matters is there are more than two
       panes.

    The default mode is "givetake".
    
  **-orient** *orientation*
    Specifies the orientation of the *paneset*.  *Orientation* may be
    "vertical" (panes run left to right) or "horizontal" (panes run
    top to bottom).  The default is "horizontal".

  **-sashborderwidth** *numPixels* 
    Specifies the default border width of sashes in the widget.  *NumPixels*
    is a non-negative value indicating the width of the 3-D border drawn
    around the sash. The value may have any of the forms acceptable to
    **Tk_GetPixels**.  This option may be overridden by the style's
    **-borderwidth** option.  The default is "1".

  **-sashcolor** *bgName*
    Specifies the default color of sashes.  *BgName* may be a color name or
    the name of a background object created by the **blt::background**
    command. The default is "grey85".

  **-sashcursor** *cursorName*
    Specifies the cursor for sashes.  *CursorName* can be in any form
    accepted by **Tk_GetCursor**.  The default is "sb_h_double_arrow"
    if the **-orient** option is "horizontal", "sb_v_double_arrow" if it is
    "vertical".

  **-sashpad** *numPixels* 
    Specifies extra padding for sashes. *NumPixels* can be a list of one or
    two numbers.  If *numPixels* has two elements, the left/top side of the
    sash is padded by the first value and the right/bottom side by the
    second value.  If *numPixels* has just one value, both the left/top and
    right/bottom sides are padded evenly by the value.  The default is "0".
    The default is "0".

  **-sashrelief** *reliefName* 
    Specifies the default relief of sashes.  This determines the 3-D effect
    for the sash.  *ReliefName* indicates how the sash should appear
    relative to the *paneset* window. Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means the sash should appear to protrude.  The
    default is "flat".
    
  **-sashthickness** *numPixels*
    Specifies a non-negative value for the thickness in pixels of the sash
    rectangle.  This doesn't include any extra padding (see the
    **-sashpad** option).  *NumPixels* may have any of the forms acceptable
    to **Tk_GetPixels**.  The default is "3".

  **-width** *numPixels*
    Specifies the width of the *paneset* window.  *NumPixels* is a
    non-negative value indicating the width the widget. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is "0" and the **-orient** option is
    "vertical", then the width is calculated to display all the panes.
    The default is "0".

*pathName* **delete** ?\ *paneName*\ ... ?
  Deletes zero or more panes from the widget. *PaneName* may be a label,
  index, or tag and may refer to multiple panes (for example "all").
  If there is a **-deletecommand** option specified a deleted pane, that
  command is invoke before the pane is deleted.

*pathName* **exists** *paneName*
  Indicates if *paneName* exists in the widget. *PaneName* may be a label,
  index, or tag, but may not represent more than one pane.  Returns "1" is
  the pane exists, "0" otherwise.
  
*pathName* **index** *paneName* 
  Returns the index of *paneName*. *PaneName* may be a label, index, or
  tag, but may not represent more than one pane.  If the pane does not
  exist, "-1" is returned.
  
*pathName* **insert after** *whereName* ?\ *label*\ ? ?\ *option *value* ... ? 
  Creates a new pane and inserts it after the pane
  *whereName*. *WhereName* may be a label, index, or tag, but may not
  represent more than one pane.  If a *label* argument is present, then
  this is the name of the new pane.  *Label* can not start with a dash "-"
  or be the name of another pane.  The name of the new pane is
  returned. Note that this operation may change the indices of previously
  created panes.

  If one or more *option*\ -\ *value* pairs are specified, they modify the
  given pane option(s) to have the given value(s).  *Option* and *value*
  are described in the **pane configure** operation.  
  
*pathName* **insert before** *whereName* ?\ *label*\ ? ?\ *option *value* ... ?
  Creates a new pane and inserts it before the pane
  *whereName*. *WhereName* may be a label, index, or tag, but may not
  represent more than one pane.  If a *label* argument is present, then
  this is the name of the new pane.  *Label* can not start with a dash "-"
  or be the name of another pane. The name of the new pane is
  returned. Note that this operation may change the indices of previously
  created panes.

  If one or more *option*\ -\ *value* pairs are specified, they modify the
  given pane option(s) to have the given value(s).  *Option* and *value*
  are described in the **pane configure** operation.  
  
*pathName* **move after** *whereName* *paneName*
  Moves *paneName* after the pane *whereName*.  Both *whereName* and
  *paneName* may be a label, index, or tag, but may not represent more than
  one pane.  The indices of panes may change.
  
*pathName* **move before** *whereName* *paneName*
  Moves *paneName* before the pane *whereName*.  Both *whereName* and
  *paneName* may be a pane label, index, or tag, but may not represent more
  than one pane. The indices of panes may change.

*pathName* **names** ?\ *pattern* ... ?
  Returns the labels of all the panes.  If one or more *pattern* arguments
  are provided, then the label of any pane matching *pattern* will be
  returned. *Pattern* is a **glob**\ -style pattern.

*pathName* **pane cget** *paneName* *option*
  Returns the current value of the pane configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **pane configure** operation. They are described in the **pane configure**
  operation below.

*pathName* **pane configure** *paneName*  ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of *paneName*.  *PaneName*
  may be a label, index, or tag.  If no *option* is specified, returns a
  list describing all the available options for *paneName* (see
  **Tk_ConfigureInfo** for information on the format of this list).  If
  *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sub-list of the value returned if no *option* is specified).
  In both cases, *paneName* may not represent more than one pane.
  
  If one or more *option-value* pairs are specified, then this command
  modifies the given option(s) to have the given value(s); in this case
  *paneName* may refer to multiple items (for example "all").  *Option* and
  *value* are described below.


  **-anchor** *anchorName* 
    Specifies how to position the embedded Tk widget in *paneName* if extra
    space is available. For example, if *anchorName* is "center" then the
    widget is centered in the *paneName*; if *anchorName* is "n" then the
    widget will be drawn such that the top center point of the pane will be
    the top center point of the pane.  This option defaults to "c".

  **-background** *bgName* 
    Specifies the background of *paneName*.  *BgName* may be a color name
    or the name of a background object created by the **blt::background**
    command.  If *colorName* is "", the widget's **-background** is
    used. The default is "".

  **-data** *dataString* 
    Specifies data to be associated with the pane. *DataString* can be an
    arbitrary string.  It is not used by the *paneset* widget. The default
    is "".

  **-deletecommand** *cmdString*
    Specifies a TCL command to invoked when the pane is deleted (via the
    *paneset*\ 's **delete** operation, or destroying the *paneset*).  The
    command will be invoked before the pane is actually deleted.  If
    *cmdString* is "", no command is invoked.  The default is "".

  **-fill** *fillName* 
    If the pane is bigger than its embedded child widget, then *fillName*
    specifies if the child widget should be stretched to occupy the extra
    space. *FillName* is one of the following.
  
    **x**
      The embedded widget can grow horizontally.  

    **y**
      The embedded widget can grow vertically.  

    **both**
      The embedded widget can grow both vertically and horizontally.  

    **none**
      The embedded widget does not grow along with the span.  

    The default is "none".

  **-height** *numPixels* 
    Specifies the height of *paneName*. *NumPixels* can be
    single value or a list.  If *numPixels* is a single value it is a
    non-negative value indicating the height the pane. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is a 2 element list, then this sets the minimum
    and maximum limits for the height of the pane. The pane will be at
    least the minimum height and less than or equal to the maximum. If
    *numPixels* is a 3 element list, then this specifies minimum, maximum,
    and nominal height or the pane.  The nominal size overrides the
    calculated height of the pane.  If *numPixels* is "", then the height
    of the requested height of the child widget is used. The default is "".

  **-hide** *boolean*
    If *boolean* is true, then *paneName* is not displayed.
    The default is "0".

  **-ipadx** *numPixels* 
    Sets how much horizontal padding to add internally on the left and
    right sides of the embedded child widget of *paneName*.
    *NumPixels* must be a valid screen distance
    like "2" or "0.3i".  The default is "0".

  **-ipady** *numPixels*
    Sets how much vertical padding to add internally on the top and bottom
    of embedded child widget of *paneName*.  *NumPixels* must be a valid
    screen distance like "2" or "0.3i".  The default is "0".

  **-padx** *numPixels*
    Sets how much padding to add to the left and right exteriors of
    *paneName*.  *NumPixels* can be a list of one or two numbers.  If
    *numPixels* has two elements, the left side of the pane is padded by
    the first value and the right side by the second value.  If *numPixels*
    has just one value, both the left and right sides are padded evenly by
    the value.  The default is "0".

  **-pady** *numPixels*
    Sets how much padding to add to the top and bottom exteriors of
    *paneName*.  *NumPixels* can be a list of one or two elements where
    each element is a valid screen distance like "2" or "0.3i".  If
    *numPixels* is two elements, the area above *pathName* is padded by the
    first distance and the area below by the second.  If *numPixels* is
    just one element, both the top and bottom areas are padded by the same
    distance.  The default is "0".
  
  **-relief** *reliefName* 
    Specifies the 3-D effect for the border around the pane.  *ReliefName*
    specifies how the interior of the pane should appear relative to the
    *paneset* window. Acceptable values are **raised**, **sunken**,
    **flat**, **ridge**, **solid**, and **groove**. For example, "raised"
    means the pane should appear to protrude.  The default is "flat".

  **-resize** *resizeMode*
    Indicates that the pane can expand or shrink from its requested width
    when the *paneset* is resized.  *ResizeMode* must be one of the
    following.

    **none**
      The size of the embedded child widget in *paneName* does not change
      as the pane is resized.
    **expand**
      The size of the embedded child widget in *paneName* is expanded if
      there is extra space in pane.
    **shrink**
      The size of the embedded child widget in *paneName* is reduced
      beyond its requested width if there is not enough space in the
      pane.
    **both**
      The size of the embedded child widget in *paneName* may grow or
      shrink depending on the size of the pane.

    The default is "none".

  **-showsash** *boolean* 
    Indicates if the sash for *paneName* should be displayed. The default is
    "1".
    
  **-size** *numPixels* 
     FIXME
     
  **-state** *stateName*
    Sets the state of the pane's sash. *StateName* can be one of the following.

    **normal**
      In normal state the sash for *paneName* is displayed using 
      its default background and relief.

    **disabled**
      Disabled state means that the sash for *paneName* should be
      insensitive: the default bindings will not activate the item, the
      sash can not be moved, etc.  In this state *paneName* is displayed
      according to the *paneset*\ 's **-disabledsashcolor** option.

    The default is "normal".

  **-tags** *tagList* 
    Specifies a list of tags to associate with the pane.  *TagList* is a
    list of tags.  Tags are a useful for referring to groups of
    panes. Panes can have any number of tags associated with them. Tags may
    refer to more than one pane.  Tags should not be the same as labels or
    the non-numeric indices.  The default is "".

  **-takefocus** *boolean* 
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *boolean* is "0",
    this means that this sash window should be skipped entirely during
    keyboard traversal.  "1" means that the this pane's sash window should
    always receive the input focus.  An empty value means that the
    traversal scripts make the decision whether to focus on the window.
    The default is "".

  **-width** *numPixels* 
    Specifies the width of *paneName*. *NumPixels* can be
    single value or a list.  If *numPixels* is a single value it is a
    non-negative value indicating the width the pane. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is a 2 element list, then this sets the minimum
    and maximum limits for the width of the pane. The pane will be at
    least the minimum width and less than or equal to the maximum. If
    *numPixels* is a 3 element list, then this specifies minimum, maximum,
    and nominal width or the pane.  The nominal size overrides the
    calculated height of the pane.  If *numPixels* is "", then the height
    of the requested height of the child widget is used. The default is "".

  **-window** *childName*  
    Specifies the widget to be embedded into *paneName*.  *ChildName* must
    be a child of the *paneset* widget.  The *paneset* will "pack" and
    manage the size and placement of *childName*.  The default value is "".

*pathName* **sash activate** *paneName* 
  Specifies to draw *paneName*\ 's sash with its active colors and relief
  (see the **-activesashcolor** and **-activesashrelief** options).
  *PaneName* is an index, label, or tag but may not refer to more than
  one tab.  Only one sash may be active at a time.  

*pathName* **sash anchor** *paneName* *x* *y*
   Sets the anchor for the resizing the sash of *paneName*.  Either the x
   or y coordinate is used depending upon the orientation of the pane.

*pathName* **sash deactivate** 
  Specifies to draw all sashes with its default colors and relief
  (see the **-sashcolor** and **-sashrelief** options).

*pathName* **sash mark** *paneName* *x* *y*
  Records *x* or *y* coordinate in the filmstrip window; used with
  later **sash move** commands.  Typically this command is associated
  with a mouse button press in the widget.  It returns an empty string.

*pathName* **sash move** *paneName* *x* *y*
  Moves the sash of *paneName*.  The sash is moved the given distance from
  its previous location (anchor).  The panes are rearranged according to
  the mode.

*pathName* **sash set** *paneName* *x* *y*
  Sets the location of the *paneName*\ 's sash to the given coordinate
  (*x* or *y*) specified.  The *paneset* panes are rearranged according
  to  the mode.

*pathName* **tag add** *tag* ?\ *paneName* ... ?
  Adds the tag to one of more panes. *Tag* is an arbitrary string that can
  not start with a number.  *PaneName* may be a label, index, or tag and
  may refer to multiple panes (for example "all").
  
*pathName* **tag delete** *tag* ?\ *paneName* ... ?
  Deletes the tag from one or more panes. *PaneName* may be a label, index,
  or tag and may refer to multiple panes (for example "all").
  
*pathName* **tag exists** *paneName* ?\ *tag* ... ?
  Indicates if the pane has any of the given tags.  Returns "1" if
  *paneName* has one or more of the named tags, "0" otherwise.  *PaneName*
  may be a label, index, or tag and may refer to multiple panes (for example
  "all").

*pathName* **tag forget** *tag*
  Removes the tag *tag* from all panes.  It's not an error if no
  panes are tagged as *tag*.

*pathName* **tag get** *paneName* ?\ *pattern* ... ?
  Returns the tag names for a given pane.  If one of more pattern
  arguments are provided, then only those matching tags are returned.

*pathName* **tag indices**  ?\ *tag* ... ?
  Returns a list of panes that have the tag.  If no pane is tagged as
  *tag*, then an empty string is returned.

*pathName* **tag names** ?\ *paneName*\ ... ?
  Returns a list of tags used by the *paneset* widget.  If one or more
  *paneName* arguments are present, any tag used by *paneName* is returned.

*pathName* **tag set** *paneName* ?\ *tag* ... ?
  Sets one or more tags for a given pane.  *PaneName* may be a label,
  index, or tag and may refer to multiple panes.  Tag names can't start
  with a digit (to distinguish them from indices) and can't be a reserved
  tag ("all").

*pathName* **tag unset** *paneName* ?\ *tag* ... ?
  Removes one or more tags from a given pane. *PaneName* may be a label,
  index, or tag and may refer to multiple panes.  Tag names that don't
  exist or are reserved ("all") are silently ignored.

*pathName* **view moveto** *fraction*
  Adjusts the view in the *paneset* window so the portion of
  the panes starting from *fraction* is displayed.  *Fraction* is a number
  between 0.0 and 1.0 representing the position where to
  start displaying panes.
   
*pathName* **view scroll** *number* *what*
  Adjusts the view in the *paneset* window according to *number* and
  *what*.  *Number* must be an integer.  *What* must be either "units" or
  "pages".  If *what* is "units", the view adjusts left or right by
  *number* units.  The number of pixel in a unit is specified by the
  **-xscrollincrement** option.  If *what* is "pages" then the view
  adjusts by *number* screenfuls.  If *number* is negative then the view
  if scrolled left; if it is positive then it is scrolled right.

SASH BINDINGS
-------------

The follow behaviors are defined for the sash windows created for each
pane. The widget class name is BltPanesetSash. 

  **<Enter>** 
    Display the sash in its active colors and relief.
  **<Leave>** 
    Display the sash in its normal colors and relief.
  **<ButtonPress-1>** 
    Start scrolling the *paneset*.
  **<B1-Motion>**
    Continue scrolling the *paneset*.
  **<ButtonRelease-1>** 
    Stop scrolling the *paneset*.
  **<KeyPress-Up>**
    If orientation is vertical, then scroll the *paneset* upward by 10
    pixels.
  **<KeyPress-Down>**
    If orientation is vertical, then scroll the *paneset* downward by 10
    pixels.
  **<KeyPress-Left>**
    If orientation is horizontal, then scroll the *paneset* left by 10
    pixels.
  **<KeyPress-Right>**
    If orientation is horizontal, then scroll the *paneset* right by 10
    pixels.
  **<Shift-KeyPress-Up>**
    If orientation is vertical, then scroll the *paneset* upward by 100
    pixels.
  **<Shift-KeyPress-Down>**
    If orientation is vertical, then scroll the *paneset* downward by 100
    pixels.
  **<Shift-KeyPress-Left>**
    If orientation is horizontal, then scroll the *paneset* left by 100
    pixels.
  **<Shift-KeyPress-Right>**
    If orientation is horizontal, then scroll the *paneset* right by 100
    pixels.

EXAMPLE
-------

The **paneset** command creates a new widget.  

  ::

    package require BLT

    blt::paneset .ps 

A new TCL command ".ps" is also created.  This new command can be used to
query and modify the *paneset* widget.  The default orientation of the
paneset is horizontal.  If you want a vertical paneset, where panes
run top to bottom, you can set the **-orient** option.

  ::

    # Change the orientation of the paneset.
    .ps configure -orient "vertical"

You can then add panes to the widget.  A pane is the container for an
embedded Tk widget.  Note that the embedded Tk widget must be a child of
the paneset widget.

  ::
    
    # Add a button to the paneset. 
    button .ps.b1
    set pane [.ps add -window .ps.b1]

The variable "pane" now contains the label of the pane.  You can
use that label to set or query configuration options specific to the
pane. You can also use the pane's index or tag to refer to the  pane.

  ::

    # Make the button expand to the size of the pane.
    .ps pane configure $pane -fill both
    
The **-fill** pane option says to may the embedded widget as big as the
pane that contains it.

You can add as many panes as you want to the widget.

  ::

     button .ps.b2 -text "Second" 
     .ps add -window .ps.b2 -fill both
     button .ps.b3 -text "Third" 
     .ps add -window .ps.b3 -fill both
     button .ps.b4 -text "Fourth" 
     .ps add -window .ps.b4 -fill both
     button .ps.b5 -text "Fifth" 
     .ps add -window .ps.b5 -fill both

By default, the *paneset* widget's requested height will be the computed
height of all its pane (vertical orientation).  But you can set the
**-height** option to override it.

  ::

    .ps configure -height 1i

Now only a subset of panes is visible.  You can attach a scrollbar
to the paneset widget to see the rest.

  ::

    blt::tk::scrollbar .sbar -orient vertical -command { .ps view }
    .ps configure -scrollcommand { .sbar set }

    blt::table . \
	0,0 .ps -fill both \
	0,1 .sbar -fill y
    
If you wanted to flip the paneset to be horizontal you would need
to reconfigure the orientation of the paneset and scrollbar and
repack.

  ::

    .sbar configure -orient horizontal
    .ps configure -orient horizontal -height 0 -width 1i

    blt::table . \
	0,0 .ps -fill both \
	1,0 .sbar -fill x


If you want the size of all panes to be the size of the paneset
window you can configure the panes with the **-relwidth** option.

  ::

    .ps configure -relwidth 1.0

You can programmatically move to specific panes by the **see** operation.

  ::

     # See the third pane. Indices are numbered from 0.
    .ps see

To delete panes there is the **delete** operation.

  ::

     # Delete the first pane.
    .ps delete 0

Note that while the pane has been delete, the button previously
embedded in the pane still exists.  You can use the pane's 
**-deletecommand** option to supply a TCL script to be invoked
before the pane is deleted.

  ::

   .ps pane configure 0 -deletecommand { destroy [%W pane cget 0 -window] }

KEYWORDS
--------

paneset, widget

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
