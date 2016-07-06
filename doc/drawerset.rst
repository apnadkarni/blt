
==============
blt::drawerset
==============

----------------------------------------
Create and manipulate drawerset widgets.
----------------------------------------

:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::drawerset** *pathName* ?\ *option* *value* ... ?

DESCRIPTION
-----------

The **blt::drawerset** command is a geometry manager for Tk.  It manages
embedded Tk widgets as drawers that slide in and out from a selected side
of a parent Tk widget.  The drawers (embedded widgets) may have a handle
attached to them that allows the user to adjust the amount the drawer is
pulled out from the side of the parent widget.

INTRODUCTION
------------

The *drawerset* widget displays embedded Tk widgets as a strip of *drawerset*
drawers.  The drawers may be arranged horizontally or vertically.  Drawers
have optional grips (handles) that appear as a border between drawers.  They
can be used to slide drawers left-to-right (horizontal arrangement) or
top-to-bottom (vertical arrangement).  The user can adjust the size of a
drawer by using the mouse or keyboard on the handle.

The *drawerset* widget can also be used to create a wizard-like interface
where drawers contain are a sequence of dialogs that lead the user through a
series of well-defined tasks.  In this case the size of the *drawerset* widget
is the size of a drawer.  If the *drawerset* widget is resized the drawer
containing the dialog is also resized.

The embedded Tk widgets of a *drawerset* must be children of the *drawerset*
widget.

SYNTAX
------

  **blt::drawerset** *pathName* ?\ *option* *value* ... ?

The **blt::drawerset** manages the geometry for embedded Tk widget of
*pathName*.  *PathName* can be any Tk widget. The window *pathName*
must already exist.
Additional options may be specified on the command line or in the option
database to configure aspects of the widget such as its colors and font.
See the widget's **configure** operation below for the exact details about
what *option* and *value* pairs are valid.

If successful, **blt::drawerset** returns the path name of the widget.  It also
creates a new TCL command by the same name.  You can use this command to
invoke various operations that query or modify the widget.  The general
form is:

  *pathName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available are described in the
`OPERATIONS`_ section below.

REFERENCING DRAWERS
------------------

Drawers can be referenced either by index, label, or by tag.

*index*
  The number of the drawer.  Indices start from 0.  The index number of a
  drawer can change as other drawers are added, deleted, or moved.  There are
  also the following special non-numeric indices.

  **active**
    This is the drawer whose grip is where the mouse pointer is currently
    located.  When a grip is active, it is drawn using its active colors.
    The **active** index is changes when you move the mouse pointer over
    another drawer's grip or by using the **activate** operation. Note
    that there can be only one active drawer at a time.

  **current**
    The index of the current drawer. This is set by the **see** operation.

  **end**
    The index of the last drawer.
    
  **first**
    The index of the first drawer that is not hidden or disabled.

  **last**
    The index of the last drawer that is not hidden or disabled.

  **previous**
    The drawer previous to the current drawer. In horizontal mode, this is
    the drawer to the left, in vertical mode this is the drawer above.  If
    no previous drawer exists or the preceding drawer are hidden or
    disabled, an index of "-1" is returned.

  **next**
    The drawer next to the current drawer. In horizontal mode, this is the
    drawer to the right, in vertical mode this is the drawer below.  If no
    next drawer exists or the following drawers are hidden or disabled, an
    index of "-1" is returned.

  **@**\ *x*\ ,\ *y*
    The index of the drawer that is located at the *x* and *y*
    screen coordinates.  If no drawer is at that point, then the
    index returned is "-1".

*label*
  The name of the drawer.  This is usually in the form "drawer0", "drawer1",
  etc., although you can specify the name of the drawer.

*tag*
  A tag is a string associated with an drawer.  They are a useful for
  referring to groups of drawers. Drawers can have any number of tags
  associated with them (specified by the **-tags** item option).  A
  tag may refer to multiple drawers.  There is one built-in tag: "all".
  Every drawer has the tag "all".  

If a drawer is specified by an integer (or one of the non-numeric indices)
it is assumed to be an index.  If it is specified by a string, it is first
tested if it's a valid label and then a tag.  Ideally you shouldn't have
tags, labels, or, indices that are the same.  They will always be
interpreted as indices or labels.  But you can also distinguish indices,
names and tables by prefixing them with "index:", "label:", or "tag:"
(such as "label:12").

OPERATIONS
----------

All *drawerset* operations are invoked by specifying the widget's pathname,
the operation, and any arguments that pertain to that operation.  The
general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *drawerset* widgets:

*pathName* **add** ?\ *label*\ ? ?\ *option* *value* ...?
  Creates a new drawer, appending it to the end of the list of drawers in the
  *drawerset* widget. If no *label* argument is present, then the name of
  the drawer is automatically generated in the form "drawer0", "drawer1", etc.
  If a *label* argument is present, then this is the name of the new drawer.
  *Label* can not start with a dash "-" or be the name of another drawer.
  The name of the new drawer is returned.

  If one or more *option-value* pairs are specified, they modify the given
  drawer option(s) to have the given value(s).  *Option* and *value* are
  described in the **drawer configure** operation.

*pathName* **bbox** *drawerName*  ?\ *switches* ... ?
  Returns the bounding box of *drawerName*.  *DrawerName* may be a label,
  index, or tag, but may not represent more than one drawer. The returned
  list contains 4 numbers: two sets of x,y coordinates that represent the
  opposite corners of the bounding box. *Switches* can be one of the 
  following:

  **-root** 
    Return the bounding box coordinates in root screen coordinates instead
    of relative to the *drawerset* window.
    
*pathName* **cget** *option* Returns the current value of the widget
  configuration option given by *option*. *Option* may have any of the
  values accepted by the **configure** operation. They are described in the
  **configure** operation below.

*pathName* **configure** ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of the *drawerset* widget.
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
  "BltDrawerset".  The resource name is the name of the widget::

    option add *BltDrawerset.anchor n
    option add *BltDrawerset.Anchor e

  The following widget options are available\:

  **-activegripcolor** *colorName* 
    Specifies the background color of the drawer's grip when it is active.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command.  
    The default is "grey90". 

  **-activegriprelief** *reliefName* 
    Specifies the default relief when a drawer's grip is active.  This
    determines the 3-D effect for the grip.  *ReliefName* indicates how
    the drawer should appear relative to the window; for example, "raised"
    means the item should appear to protrude.  The default is "flat".
    
  **-anchor** *anchorName* 
    Specifies how to position the set of drawers if extra space is available
    in the *drawerset*. For example, if *anchorName* is "center" then the
    widget is centered in the *drawerset*; if *anchorName* is "n" then the
    widget will be drawn such that the top center point of the widget will
    be the top center point of the drawer.  This option defaults to "c".

  **-animate** *boolean*
    Indicates to animate the movement of drawers.  The **-scrolldelay** and
    **--scrollincrement** options determine how the animation is
    performed. The default is "0".

  **-background** *colorName* 
    Specifies the default background of the widget including its drawers.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "grey85".
    
  **-gripactiverelief** *reliefName* 
    Specifies the relief of grips when they are active.  This determines
    the 3-D effect for the grip.  *Relief* indicates how the grip should
    appear relative to the window; for example, "raised" means the grip
    should appear to protrude.  The default is "raised".

  **-gripborderwidth** *numPixels* 
    Specifies the default border width of grips in the widget.  *NumPixels*
    is a non-negative value indicating the width of the 3-D border drawn
    around the grip. The value may have any of the forms acceptable to
    **Tk_GetPixels**.  This option may be overridden by the style's
    **-borderwidth** option.  The default is "1".

  **-gripcolor** *colorName*
    Specifies the default color of grips.  *ColorName* may be a color name or
    the name of a background object created by the **blt::background**
    command. The default is "grey85".

  **-grippad** *numPixels* 
    Specifies extra padding for grips.  *NumPixels* is a non-negative value
    indicating the width of the border drawn around the grip. The value may
    have any of the forms acceptable to **Tk_GetPixels**.  The default is
    "0".

  **-griprelief** *reliefName* 
    Specifies the default relief of grips.  This determines the 3-D
    effect for the grip.  *Relief* indicates how the grip should appear
    relative to the window; for example, "raised" means the item should
    appear to protrude.  The default is "flat".
    
  **-gripthickness** *numPixels*
    Specifies a non-negative value for the thickness in pixels of the grip
    rectangle.  This doesn't include any extra padding (see the
    **-grippad** option).  *NumPixels* may have any of the forms acceptable
    to **Tk_GetPixels**.  The default is "3".

  **-height** *numPixels*
    Specifies the height of the *drawerset* window.  *NumPixels* is a
    non-negative value indicating the height the widget. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is "0" and the **-orient** option is
    "horizontal", then the height calculated to display all the drawers.
    The default is "0".

  **-orient** *orientation*
    Specifies the orientation of the *drawerset*.  *Orientation* may be
    "vertical" (drawers run left to right) or "horizontal" (drawers run
    top to bottom).  The default is "horizontal".

  **-relheight** *number*
    Specifies the relative height of drawers to the *drawerset* window.
    *Number* is a number between 0.0 and 1.0.  If *number* is "1.0", then
    each drawer will take up the entire *drawerset* window. If *number* is
    0.0, and **-orient** is "vertical", then the height of each drawer is
    computed from the requested height of its embedded child widget.  The
    default is "0.0".

  **-relwidth** *number*
    Specifies the relative width of drawers to the *drawerset* window.
    *Number* is a number between 0.0 and 1.0.  If *number* is "1.0", then
    each drawer will take up the entire *drawerset* window. If *number* is
    0.0, and **-orient** is "horizontal", then the width of each drawer is
    computed from the requested width of its embedded child widget.  The
    default is "0.0".

  **-scrollcommand** *string*
    Specifies the prefix for a command for communicating with scrollbars.
    Whenever the view in the widget's window changes, the widget will
    generate a TCL command by concatenating the scroll command and two
    numbers.  If this option is not specified, then no command will be
    executed.

  **-scrolldelay** *milliseconds*
    Specifies the delay between steps in the scrolling in milliseconds.  If
    *milliseconds* is 0, then no automatic changes will occur.  The default
    is "0".

  **-scrollincrement** *numPixels*
    Sets the smallest number of pixels to scroll the drawers.  If
    *numPixels* is greater than 0, this sets the units for scrolling (e.g.,
    when you the change the view by clicking on the left and right arrows
    of a scrollbar). The default is "10".

  **-width** *numPixels*
    Specifies the width of the *drawerset* window.  *NumPixels* is a
    non-negative value indicating the width the widget. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is "0" and the **-orient** option is
    "vertical", then the width is calculated to display all the drawers.
    The default is "0".

*pathName* **delete** *drawerName*\ ...
  Deletes one or more drawers from the widget. *DrawerName* may be a label,
  index, or tag and may refer to multiple drawers (for example "all").
  If there is a **-deletecommand** option specified a deleted drawer, that
  command is invoke before the drawer is deleted.

*pathName* **exists** *drawerName*
  Indicates if *drawerName* exists in the widget. *DrawerName* may be a label,
  index, or tag, but may not represent more than one drawer.  Returns "1" is
  the drawer exists, "0" otherwise.
  
*pathName* **drawer cget** *drawerName* *option*
  Returns the current value of the drawer configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **drawer configure** operation. They are described in the **drawer configure**
  operation below.

*pathName* **drawer configure** *drawerName*  ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of *drawerName*.  *DrawerName*
  may be a label, index, or tag.  If no *option* is specified, returns a
  list describing all the available options for *drawerName* (see
  **Tk_ConfigureInfo** for information on the format of this list).  If
  *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sub-list of the value returned if no *option* is specified).
  In both cases, *drawerName* may not represent more than one drawer.
  
  If one or more *option-value* pairs are specified, then this command
  modifies the given option(s) to have the given value(s); in this case
  *drawerName* may refer to multiple items (for example "all").  *Option* and
  *value* are described below.


  **-borderwidth** *numPixels* 
    Specifies the border width of *drawerName*.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the drawer.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "0".

  **-data** *string* 
    Specifies data to be associated with the drawer. *String* can be an
    arbitrary string.  It is not used by the *drawerset* widget. The
    default is "".

  **-deletecommand** *string*
    Specifies a TCL command to invoked when the drawer is deleted (via the
    *drawerset*\ 's **delete** operation, or destroying the *drawerset*).  The
    command will be invoked before the drawer is actually deleted.  If
    *string* is "", no command is invoked.  The default is "".

  **-fill** *fillName* 
    If the drawer is bigger than its embedded child widget, then *fillName*
    specifies if the child widget should be stretched to occupy the extra
    space.  *FillName* is either "none", "x", "y", "both".  For example, if
    *fillName* is "x", then the child widget is stretched horizontally.  If
    *fillName* is "y", the widget is stretched vertically.  The default is
    "none".

  **-height** *numPixels* 
    Specifies the height of *drawerName*. *NumPixels* can be
    single value or a list.  If *numPixels* is a single value it is a
    non-negative value indicating the height the drawer. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is a 2 element list, then this sets the minimum
    and maximum limits for the height of the drawer. The drawer will be at
    least the minimum height and less than or equal to the maximum. If
    *numPixels* is a 3 element list, then this specifies minimum, maximum,
    and nominal height or the drawer.  The nominal size overrides the
    calculated height of the drawer.  If *numPixels* is "", then the height
    of the requested height of the child widget is used. The default is "".

  **-hide** *boolean*
    If *boolean* is true, then *drawerName* is not displayed.
    The default is "yes".

  **-ipadx** *numPixels* 
    Sets how much horizontal padding to add internally on the left and
    right sides of the embedded child widget of *drawerName*.
    *NumPixels* must be a valid screen distance
    like "2" or "0.3i".  The default is "0".

  **-ipady** *numPixels*
    Sets how much vertical padding to add internally on the top and bottom
    of embedded child widget of *drawerName*.  *NumPixels* must be a valid
    screen distance like "2" or "0.3i".  The default is "0".

  **-padx** *numPixels*
    Sets how much padding to add to the left and right exteriors of
    *drawerName*.  *NumPixels* can be a list of one or two numbers.  If
    *numPixels* has two elements, the left side of the drawer is padded by
    the first value and the right side by the second value.  If *numPixels*
    has just one value, both the left and right sides are padded evenly by
    the value.  The default is "0".

  **-pady** *numPixels*
    Sets how much padding to add to the top and bottom exteriors of
    *drawerName*.  *NumPixels* can be a list of one or two elements where
    each element is a valid screen distance like "2" or "0.3i".  If
    *numPixels* is two elements, the area above *pathName* is padded by the
    first distance and the area below by the second.  If *numPixels* is
    just one element, both the top and bottom areas are padded by the same
    distance.  The default is "0".
  
  **-relief** *relief* 
    Specifies the 3-D effect for the border around the drawer.  *Relief*
    specifies how the interior of the drawer should appear relative to the
    *drawerset* widget; for example, "raised" means the item should appear to
    protrude from the window, relative to the surface of the window.  The
    default is "flat".

  **-resize** *resizeMode*
    Indicates that the drawer can expand or shrink from its requested width
    when the *drawerset* is resized.  *ResizeMode* must be one of the
    following.

    **none**
      The size of the embedded child widget in *drawerName* does not change
      as the drawer is resized.
    **expand**
      The size of the embedded child widget in *drawerName* is expanded if
      there is extra space in drawer.
    **shrink**
      The size of the embedded child widget in *drawerName* is reduced
      beyond its requested width if there is not enough space in the
      drawer.
    **both**
      The size of the embedded child widget in *drawerName* may grow or
      shrink depending on the size of the drawer.

    The default is "none".

  **-showgrip** *boolean* 
    Indicates if the grip for *drawerName* should be displayed. The default is
    "1".
    
  **-size** *numPixels* 

  **-tags** *tagList* 
    Specifies a list of tags to associate with the drawer.  *TagList* is a
    list of tags.  Tags are a useful for referring to groups of
    drawers. Drawers can have any number of tags associated with them. Tags may
    refer to more than one drawer.  Tags should not be the same as labels or
    the non-numeric indices.  The default is "".

  **-takefocus** *bool* 
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *bool* is "0",
    this means that this grip window should be skipped entirely during
    keyboard traversal.  "1" means that the this drawer's grip window should
    always receive the input focus.  An empty value means that the
    traversal scripts make the decision whether to focus on the window.
    The default is "".

  **-width** *numPixels* 
    Specifies the width of *drawerName*. *NumPixels* can be
    single value or a list.  If *numPixels* is a single value it is a
    non-negative value indicating the width the drawer. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is a 2 element list, then this sets the minimum
    and maximum limits for the width of the drawer. The drawer will be at
    least the minimum width and less than or equal to the maximum. If
    *numPixels* is a 3 element list, then this specifies minimum, maximum,
    and nominal width or the drawer.  The nominal size overrides the
    calculated height of the drawer.  If *numPixels* is "", then the height
    of the requested height of the child widget is used. The default is "".

  **-window** *childName*  
    Specifies the widget to be embedded into *drawerName*.  *ChildName* must
    be a child of the *drawerset* widget.  The *drawerset* will "pack" and
    manage the size and placement of *childName*.  The default value is "".

*pathName* **grip activate** *drawerName* 
  Specifies to draw *drawerName*\ 's grip with its active colors and relief
  (see the **-activegripcolor** and **-activegriprelief** options).
  *DrawerName* is an index, label, or tag but may not refer to more than
  one tab.  Only one grip may be active at a time.  

*pathName* **grip anchor** *drawerName* *x* *y*
   Sets the anchor for the resizing or moving *drawerName*.  Either the x or
   y coordinate is used depending upon the orientation of the drawer.

*pathName* **grip deactivate** 
  Specifies to draw all grips with its default colors and relief
  (see the **-gripcolor** and **-griprelief** options).

*pathName* **grip mark** *drawerName* *x* *y*
  Records *x* or *y* coordinate in the drawerset window; used with
  later **grip move** commands.  Typically this command is associated
  with a mouse button press in the widget.  It returns an empty string.

*pathName* **grip move** *drawerName* *x* *y*
  Moves the grip of *drawerName*.  The grip is moved the given distance
  from its previous location (anchor).

*pathName* **grip set** *drawerName* *x* *y*
  Sets the location of the *drawerName*\ 's grip to the given coordinate
  (*x* or *y*) specified.  The *drawerset* drawers are moved accordingly.

*pathName* **index** *drawerName* 
  Returns the index of *drawerName*. *DrawerName* may be a label, index, or
  tag, but may not represent more than one drawer.  If the drawer does not
  exist, "-1" is returned.
  
*pathName* **insert after** *whereName* ?\ *label*\ ? ?\ *option *value* ... ? 
  Creates a new drawer and inserts it after the drawer
  *whereName*. *WhereName* may be a label, index, or tag, but may not
  represent more than one drawer.  If a *label* argument is present, then
  this is the name of the new drawer.  *Label* can not start with a dash "-"
  or be the name of another drawer.  The name of the new drawer is
  returned. Note that this operation may change the indices of previously
  created drawers.

  If one or more *option*\ -\ *value* pairs are specified, they modify the
  given drawer option(s) to have the given value(s).  *Option* and *value*
  are described in the **drawer configure** operation.  
  
*pathName* **insert before** *whereName* ?\ *label*\ ? ?\ *option *value* ... ?
  Creates a new drawer and inserts it before the drawer
  *whereName*. *WhereName* may be a label, index, or tag, but may not
  represent more than one drawer.  If a *label* argument is present, then
  this is the name of the new drawer.  *Label* can not start with a dash "-"
  or be the name of another drawer. The name of the new drawer is
  returned. Note that this operation may change the indices of previously
  created drawers.

  If one or more *option*\ -\ *value* pairs are specified, they modify the
  given drawer option(s) to have the given value(s).  *Option* and *value*
  are described in the **drawer configure** operation.  
  
*pathName* **invoke** *drawerName* 
  Invokes the TCL command specified by drawer's **-command** option.
  *DrawerName* may be a label, index, or tag, but may not represent more
  than one drawer.  If *drawerName* is disabled, no command is invoked.
  
*pathName* **move after** *whereName* *drawerName*
  Moves *drawerName* after the drawer *whereName*.  Both *whereName* and
  *drawerName* may be a label, index, or tag, but may not represent more than
  one drawer.  The indices of drawers may change.
  
*pathName* **move before** *whereName* *drawerName*
  Moves *drawerName* before the drawer *whereName*.  Both *whereName* and
  *drawerName* may be a label, index, or tag, but may not represent more than
  one drawer. The indices of drawers may change.

*pathName* **names** ?\ *pattern* ... ?
  Returns the labels of all the drawers.  If one or more *pattern* arguments
  are provided, then the label of any drawer matching *pattern* will be
  returned. *Pattern* is a **glob**\ -style pattern.

*pathName* **see** *drawermName* 
  Scrolls the *drawerset* so that *drawerName* is visible in the widget's window.
  *DrawerName* may be a label, index, or tag, but may not represent more than
  one item.
  
*pathName* **size** 
  Returns the number of drawers in the *drawerset*.

*pathName* **tag add** *tag* ?\ *drawerName* ... ?
  Adds the tag to one of more drawers. *Tag* is an arbitrary string that can
  not start with a number.  *DrawerName* may be a label, index, or tag and
  may refer to multiple drawers (for example "all").
  
*pathName* **tag delete** *tag* ?\ *drawerName* ... ?
  Deletes the tag from one or more drawers. *DrawerName* may be a label, index,
  or tag and may refer to multiple drawers (for example "all").
  
*pathName* **tag exists** *drawerName* ?\ *tag* ... ?
  Indicates if the drawer has any of the given tags.  Returns "1" if
  *drawerName* has one or more of the named tags, "0" otherwise.  *DrawerName*
  may be a label, index, or tag and may refer to multiple drawers (for example
  "all").

*pathName* **tag forget** *tag*
  Removes the tag *tag* from all drawers.  It's not an error if no
  drawers are tagged as *tag*.

*pathName* **tag get** *drawerName* ?\ *pattern* ... ?
  Returns the tag names for a given drawer.  If one of more pattern
  arguments are provided, then only those matching tags are returned.

*pathName* **tag indices**  ?\ *tag* ... ?
  Returns a list of drawers that have the tag.  If no drawer is tagged as
  *tag*, then an empty string is returned.

*pathName* **tag names** ?\ *drawerName*\ ... ?
  Returns a list of tags used by the *drawerset* widget.  If one or more
  *drawerName* arguments are present, any tag used by *drawerName* is returned.

*pathName* **tag set** *drawerName* ?\ *tag* ... ?
  Sets one or more tags for a given drawer.  *DrawerName* may be a label,
  index, or tag and may refer to multiple drawers.  Tag names can't start
  with a digit (to distinguish them from indices) and can't be a reserved
  tag ("all").

*pathName* **tag unset** *drawerName* ?\ *tag* ... ?
  Removes one or more tags from a given drawer. *DrawerName* may be a label,
  index, or tag and may refer to multiple drawers.  Tag names that don't
  exist or are reserved ("all") are silently ignored.

*pathName* **view moveto** *fraction*
  Adjusts the view in the *drawerset* window so the portion of
  the drawers starting from *fraction* is displayed.  *Fraction* is a number
  between 0.0 and 1.0 representing the position where to
  start displaying drawers.
   
*pathName* **view scroll** *number* *what*
  Adjusts the view in the *drawerset* window according to *number* and
  *what*.  *Number* must be an integer.  *What* must be either "units" or
  "pages".  If *what* is "units", the view adjusts left or right by
  *number* units.  The number of pixel in a unit is specified by the
  **-xscrollincrement** option.  If *what* is "pages" then the view
  adjusts by *number* screenfuls.  If *number* is negative then the view
  if scrolled left; if it is positive then it is scrolled right.

GRIP BINDINGS
-------------

The follow behaviors are defined for the grip windows created for each
drawer. The widget class name is BltDrawersetGrip. 

  **<Enter>** 
    Display the grip in its active colors and relief.
  **<Leave>** 
    Display the grip in its normal colors and relief.
  **<ButtonPress-1>** 
    Start scrolling the *drawerset*.
  **<B1-Motion>**
    Continue scrolling the *drawerset*.
  **<ButtonRelease-1>** 
    Stop scrolling the *drawerset*.
  **<KeyPress-Up>**
    If orientation is vertical, then scroll the *drawerset* upward by 10
    pixels.
  **<KeyPress-Down>**
    If orientation is vertical, then scroll the *drawerset* downward by 10
    pixels.
  **<KeyPress-Left>**
    If orientation is horizontal, then scroll the *drawerset* left by 10
    pixels.
  **<KeyPress-Right>**
    If orientation is horizontal, then scroll the *drawerset* right by 10
    pixels.
  **<Shift-KeyPress-Up>**
    If orientation is vertical, then scroll the *drawerset* upward by 100
    pixels.
  **<Shift-KeyPress-Down>**
    If orientation is vertical, then scroll the *drawerset* downward by 100
    pixels.
  **<Shift-KeyPress-Left>**
    If orientation is horizontal, then scroll the *drawerset* left by 100
    pixels.
  **<Shift-KeyPress-Right>**
    If orientation is horizontal, then scroll the *drawerset* right by 100
    pixels.

EXAMPLE
-------

The **drawerset** command creates a new widget.  

  ::

    package require BLT

    blt::drawerset .fs 

A new TCL command ".fs" is also created.  This new command can be used to
query and modify the *drawerset* widget.  The default orientation of the
drawerset is horizontal.  If you want a vertical drawerset, where drawers
run top to bottom, you can set the **-orient** option.

  ::

    # Change the orientation of the drawerset.
    .fs configure -orient "vertical"

You can then add drawers to the widget.  A drawer is the container for an
embedded Tk widget.  Note that the embedded Tk widget must be a child of
the drawerset widget.

  ::
    
    # Add a button to the drawerset. 
    button .fs.b1
    set drawer [.fs add -window .fs.b1]

The variable "drawer" now contains the label of the drawer.  You can
use that label to set or query configuration options specific to the
drawer. You can also use the drawer's index or tag to refer to the  drawer.

  ::

    # Make the button expand to the size of the drawer.
    .fs drawer configure $drawer -fill both
    
The **-fill** drawer option says to may the embedded widget as big as the
drawer that contains it.

You can add as many drawers as you want to the widget.

  ::

     button .fs.b2 -text "Second" 
     .fs add -window .fs.b2 -fill both
     button .fs.b3 -text "Third" 
     .fs add -window .fs.b3 -fill both
     button .fs.b4 -text "Fourth" 
     .fs add -window .fs.b4 -fill both
     button .fs.b5 -text "Fifth" 
     .fs add -window .fs.b5 -fill both

By default, the *drawerset* widget's requested height will be the computed
height of all its drawer (vertical orientation).  But you can set the
**-height** option to override it.

  ::

    .fs configure -height 1i

Now only a subset of drawers is visible.  You can attach a scrollbar
to the drawerset widget to see the rest.

  ::

    blt::tk::scrollbar .sbar -orient vertical -command { .fs view }
    .fs configure -scrollcommand { .sbar set }

    blt::table . \
	0,0 .fs -fill both \
	0,1 .sbar -fill y
    
If you wanted to flip the drawerset to be horizontal you would need
to reconfigure the orientation of the drawerset and scrollbar and
repack.

  ::

    .sbar configure -orient horizontal
    .fs configure -orient horizontal -height 0 -width 1i

    blt::table . \
	0,0 .fs -fill both \
	1,0 .sbar -fill x


If you want the size of all drawers to be the size of the drawerset
window you can configure the drawers with the **-relwidth** option.

  ::

    .fs configure -relwidth 1.0

You can programmatically move to specific drawers by the **see** operation.

  ::

     # See the third drawer. Indices are numbered from 0.
    .fs see

To delete drawers there is the **delete** operation.

  ::

     # Delete the first drawer.
    .fs delete 0

Note that while the drawer has been delete, the button previously
embedded in the drawer still exists.  You can use the drawer's 
**-deletecommand** option to supply a TCL script to be invoked
before the drawer is deleted.

  ::

   .fs drawer configure 0 -deletecommand { destroy [%W drawer cget 0 -window] }

KEYWORDS
--------

drawerset, widget

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
