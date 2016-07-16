
==============
blt::filmstrip
==============

----------------------------------------
Create and manipulate filmstrip widgets.
----------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::filmstrip** *pathName* ?\ *option* *value* ... ?

DESCRIPTION
-----------

The *filmstrip* widget displays a scroll-able vertical or horizontal strip
of embedded Tk widgets.  Each child widget is contained in a *filmstrip*
frame (different from the Tk frame widget). Frames are optionally separated
by grips (handles) that appear as a border between frames.  It is
positioned on the left (horizontal arrangement) or bottom (vertical
arrangement) of the frame.

INTRODUCTION
------------

The *filmstrip* widget displays embedded Tk widgets as a strip of
*filmstrip* frames.  The frames may be arranged horizontally or vertically.
Frames have optional grips (handles) that appear as a border between
frames.  They can be used to slide frames left-to-right (horizontal
arrangement) or top-to-bottom (vertical arrangement).  The user can adjust
the size of a frame by using the mouse or keyboard on the handle.

The *filmstrip* widget can also be used to create a wizard-like interface
where frames contain are a sequence of dialogs that lead the user through a
series of well-defined tasks.  In this case the size of the *filmstrip* widget
is the size of a frame.  If the *filmstrip* widget is resized the frame
containing the dialog is also resized.

The embedded Tk widgets of a *filmstrip* must be children of the
*filmstrip* widget.

SYNTAX
------

  **blt::filmstrip** *pathName* ?\ *option* *value* ... ?

The **blt::filmstrip** command creates a new window *pathName* and makes it
into a *filmstrip* widget.  At the time this command is invoked, there must
not exist a window named *pathName*, but *pathName*'s parent must exist.
Additional options may be specified on the command line or in the option
database to configure aspects of the widget such as its colors and font.
See the widget's **configure** operation below for the exact details about
what *option* and *value* pairs are valid.

If successful, **filmstrip** returns the path name of the widget.  It also
creates a new TCL command by the same name.  You can use this command to
invoke various operations that query or modify the widget.  The general
form is:

  *pathName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available are described in the
`OPERATIONS`_ section below.

REFERENCING FRAMES
------------------

Frames can be referenced either by index, label, or by tag.

*index*
  The number of the frame.  Indices start from 0.  The index number of a
  frame can change as other frames are added, deleted, or moved.  There are
  also the following special non-numeric indices.

  **active**
    This is the frame whose grip is where the mouse pointer is currently
    located.  When a grip is active, it is drawn using its active colors.
    The **active** index is changes when you move the mouse pointer over
    another frame's grip or by using the **activate** operation. Note
    that there can be only one active frame at a time.

  **current**
    The index of the current frame. This is set by the **see** operation.

  **end**
    The index of the last frame.
    
  **first**
    The index of the first frame that is not hidden or disabled.

  **last**
    The index of the last frame that is not hidden or disabled.

  **previous**
    The frame previous to the current frame. In horizontal mode, this is
    the frame to the left, in vertical mode this is the frame above.  If
    no previous frame exists or the preceding frame are hidden or
    disabled, an index of "-1" is returned.

  **next**
    The frame next to the current frame. In horizontal mode, this is the
    frame to the right, in vertical mode this is the frame below.  If no
    next frame exists or the following frames are hidden or disabled, an
    index of "-1" is returned.

  **@**\ *x*\ ,\ *y*
    The index of the frame that is located at the *x* and *y*
    screen coordinates.  If no frame is at that point, then the
    index returned is "-1".

*label*
  The name of the frame.  This is usually in the form "frame0", "frame1",
  etc., although you can specify the name of the frame.

*tag*
  A tag is a string associated with an frame.  They are a useful for
  referring to groups of frames. Frames can have any number of tags
  associated with them (specified by the **-tags** item option).  A
  tag may refer to multiple frames.  There is one built-in tag: "all".
  Every frame has the tag "all".  

If a frame is specified by an integer (or one of the non-numeric indices)
it is assumed to be an index.  If it is specified by a string, it is first
tested if it's a valid label and then a tag.  Ideally you shouldn't have
tags, labels, or, indices that are the same.  They will always be
interpreted as indices or labels.  But you can also distinguish indices,
names and tables by prefixing them with "index:", "label:", or "tag:"
(such as "label:12").

OPERATIONS
----------

All *filmstrip* operations are invoked by specifying the widget's pathname,
the operation, and any arguments that pertain to that operation.  The
general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *filmstrip* widgets:

*pathName* **add** ?\ *label*\ ? ?\ *option* *value* ...?
  Creates a new frame, appending it to the end of the list of frames in the
  *filmstrip* widget. If no *label* argument is present, then the name of
  the frame is automatically generated in the form "frame0", "frame1", etc.
  If a *label* argument is present, then this is the name of the new frame.
  *Label* can not start with a dash "-" or be the name of another frame.
  The name of the new frame is returned.

  If one or more *option-value* pairs are specified, they modify the given
  frame option(s) to have the given value(s).  *Option* and *value* are
  described in the **frame configure** operation.

*pathName* **bbox** *frameName*  ?\ *switches* ... ?
  Returns the bounding box of *frameName*.  *FrameName* may be a label,
  index, or tag, but may not represent more than one frame. The returned
  list contains 4 numbers: two sets of x,y coordinates that represent the
  opposite corners of the bounding box. *Switches* can be one of the 
  following:

  **-root** 
    Return the bounding box coordinates in root screen coordinates instead
    of relative to the *filmstrip* window.
    
*pathName* **cget** *option*
  Returns the current value of the widget configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **configure** ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of the *filmstrip* widget.
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
  **BltFilmstrip**.  The resource name is the name of the widget::

    option add *BltFilmstrip.anchor n
    option add *BltFilmstrip.Anchor e

  The following widget options are available\:

  **-activegripcolor** *colorName* 
    Specifies the background color of the frame's grip when it is active.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command.  
    The default is "grey90". 

  **-activegriprelief** *reliefName* 
    Specifies the default relief when a frame's grip is active.  This
    determines the 3-D effect for the grip.  *ReliefName* indicates how the
    grip should appear relative to the *filmstrip* window. Acceptable
    values are **raised**, **sunken**, **flat**, **ridge**, **solid**, and
    **groove**. For example, "raised" means the grip should appear to
    protrude.  The default is "raised".
    
  **-anchor** *anchorName* 
    Specifies how to position the set of frames if extra space is available
    in the *filmstrip*. For example, if *anchorName* is "center" then the
    widget is centered in the *filmstrip*; if *anchorName* is "n" then the
    widget will be drawn such that the top center point of the widget will
    be the top center point of the frame.  This option defaults to "c".

  **-animate** *boolean*
    Indicates to animate the movement of frames.  The **-scrolldelay** and
    **-scrollincrement** options determine how the animation is
    performed. The default is "0".

  **-background** *colorName* 
    Specifies the default background of the widget including its frames.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "grey85".
    
  **-gripactiverelief** *reliefName* 
    Specifies the relief of grips when they are active.  This determines
    the 3-D effect for the grip.  *ReliefName* indicates how the grip
    should appear relative to the *filmstrip* window.  Acceptable values
    are **raised**, **sunken**, **flat**, **ridge**, **solid**, and
    **groove**. For example, "raised" means the grip should appear to
    protrude.  The default is "raised".

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
    Specifies the default relief of grips.  This determines the 3-D effect
    for the grip.  *ReliefName* indicates how the grip should appear
    relative to the *filmstrip* window. Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means the grip should appear to protrude.  The
    default is "flat".
    
  **-gripthickness** *numPixels*
    Specifies a non-negative value for the thickness in pixels of the grip
    rectangle.  This doesn't include any extra padding (see the
    **-grippad** option).  *NumPixels* may have any of the forms acceptable
    to **Tk_GetPixels**.  The default is "3".

  **-height** *numPixels*
    Specifies the height of the *filmstrip* window.  *NumPixels* is a
    non-negative value indicating the height the widget. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is "0" and the **-orient** option is
    "horizontal", then the height calculated to display all the frames.
    The default is "0".

  **-orient** *orientation*
    Specifies the orientation of the *filmstrip*.  *Orientation* may be
    "vertical" (frames run left to right) or "horizontal" (frames run top
    to bottom).  The default is "horizontal".

  **-relheight** *number*
    Specifies the relative height of frames to the *filmstrip* window.
    *Number* is a number between 0.0 and 1.0.  If *number* is "1.0", then
    each frame will take up the entire *filmstrip* window. If *number* is
    0.0, and **-orient** is "vertical", then the height of each frame is
    computed from the requested height of its embedded child widget.  The
    default is "0.0".

  **-relwidth** *number*
    Specifies the relative width of frames to the *filmstrip* window.
    *Number* is a number between 0.0 and 1.0.  If *number* is "1.0", then
    each frame will take up the entire *filmstrip* window. If *number* is
    0.0, and **-orient** is "horizontal", then the width of each frame is
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
    Sets the smallest number of pixels to scroll the frames.  If
    *numPixels* is greater than 0, this sets the units for scrolling (e.g.,
    when you the change the view by clicking on the left and right arrows
    of a scrollbar). The default is "10".

  **-width** *numPixels*
    Specifies the width of the *filmstrip* window.  *NumPixels* is a
    non-negative value indicating the width the widget. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is "0" and the **-orient** option is
    "vertical", then the width is calculated to display all the frames.
    The default is "0".

*pathName* **delete** *frameName*\ ...
  Deletes one or more frames from the widget. *FrameName* may be a label,
  index, or tag and may refer to multiple frames (for example "all").
  If there is a **-deletecommand** option specified a deleted frame, that
  command is invoke before the frame is deleted.

*pathName* **exists** *frameName*
  Indicates if *frameName* exists in the widget. *FrameName* may be a label,
  index, or tag, but may not represent more than one frame.  Returns "1" is
  the frame exists, "0" otherwise.
  
*pathName* **frame cget** *frameName* *option*
  Returns the current value of the frame configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **frame configure** operation. They are described in the **frame configure**
  operation below.

*pathName* **frame configure** *frameName*  ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of *frameName*.  *FrameName*
  may be a label, index, or tag.  If no *option* is specified, returns a
  list describing all the available options for *frameName* (see
  **Tk_ConfigureInfo** for information on the format of this list).  If
  *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sub-list of the value returned if no *option* is specified).
  In both cases, *frameName* may not represent more than one frame.
  
  If one or more *option-value* pairs are specified, then this command
  modifies the given option(s) to have the given value(s); in this case
  *frameName* may refer to multiple items (for example "all").  *Option* and
  *value* are described below.


  **-borderwidth** *numPixels* 
    Specifies the border width of *frameName*.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the frame.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "0".

  **-data** *string* 
    Specifies data to be associated with the frame. *String* can be an
    arbitrary string.  It is not used by the *filmstrip* widget. The
    default is "".

  **-deletecommand** *string*
    Specifies a TCL command to invoked when the frame is deleted (via the
    *filmstrip*\ 's **delete** operation, or destroying the *filmstrip*).  The
    command will be invoked before the frame is actually deleted.  If
    *string* is "", no command is invoked.  The default is "".

  **-fill** *fillName* 
    If the frame is bigger than its embedded child widget, then *fillName*
    specifies if the child widget should be stretched to occupy the extra
    space. *FillName* can be one of the following:

    **x**
      The width of the frame's embedded widget is expanded to fill the
      window.
    **y**
      The height of the frame's embedded widget is expanded to fill the
      window.
    **both**
      Both the width and height of the frame's embedded widget are
      expanded.
    **none**
      The frame's embedded widget not resized.

    The default is "none".

  **-height** *numPixels* 
    Specifies the height of *frameName*. *NumPixels* can be
    single value or a list.  If *numPixels* is a single value it is a
    non-negative value indicating the height the frame. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is a 2 element list, then this sets the minimum
    and maximum limits for the height of the frame. The frame will be at
    least the minimum height and less than or equal to the maximum. If
    *numPixels* is a 3 element list, then this specifies minimum, maximum,
    and nominal height or the frame.  The nominal size overrides the
    calculated height of the frame.  If *numPixels* is "", then the height
    of the requested height of the child widget is used. The default is "".

  **-hide** *boolean*
    If *boolean* is true, then *frameName* is not displayed.  The default
    is "0".

  **-ipadx** *numPixels* 
    Sets how much horizontal padding to add internally on the left and
    right sides of the embedded child widget of *frameName*.
    *NumPixels* must be a valid screen distance
    like "2" or "0.3i".  The default is "0".

  **-ipady** *numPixels*
    Sets how much vertical padding to add internally on the top and bottom
    of embedded child widget of *frameName*.  *NumPixels* must be a valid
    screen distance like "2" or "0.3i".  The default is "0".

  **-padx** *numPixels*
    Sets how much padding to add to the left and right exteriors of
    *frameName*.  *NumPixels* can be a list of one or two numbers.  If
    *numPixels* has two elements, the left side of the frame is padded by
    the first value and the right side by the second value.  If *numPixels*
    has just one value, both the left and right sides are padded evenly by
    the value.  The default is "0".

  **-pady** *numPixels*
    Sets how much padding to add to the top and bottom exteriors of
    *frameName*.  *NumPixels* can be a list of one or two elements where
    each element is a valid screen distance like "2" or "0.3i".  If
    *numPixels* is two elements, the area above *pathName* is padded by the
    first distance and the area below by the second.  If *numPixels* is
    just one element, both the top and bottom areas are padded by the same
    distance.  The default is "0".
  
  **-relief** *reliefName* 
    Specifies the 3-D effect for the border around the frame.  *ReliefName*
    specifies how the interior of the frame should appear relative to the
    *filmstrip* widget. Acceptable values are **raised**, **sunken**,
    **flat**, **ridge**, **solid**, and **groove**. For example, "raised"
    means the frame should appear to protrude.  The default is "flat".

  **-resize** *resizeMode*
    Indicates that the frame can expand or shrink from its requested width
    when the *filmstrip* is resized.  *ResizeMode* must be one of the
    following.

    **none**
      The size of the embedded child widget in *frameName* does not change
      as the frame is resized.
    **expand**
      The size of the embedded child widget in *frameName* is expanded if
      there is extra space in frame.
    **shrink**
      The size of the embedded child widget in *frameName* is reduced
      beyond its requested width if there is not enough space in the
      frame.
    **both**
      The size of the embedded child widget in *frameName* may grow or
      shrink depending on the size of the frame.

    The default is "none".

  **-showgrip** *boolean* 
    Indicates if the grip for *frameName* should be displayed. The default is
    "1".
    
  **-size** *numPixels* 

  **-tags** *tagList* 
    Specifies a list of tags to associate with the frame.  *TagList* is a
    list of tags.  Tags are a useful for referring to groups of
    frames. Frames can have any number of tags associated with them. Tags may
    refer to more than one frame.  Tags should not be the same as labels or
    the non-numeric indices.  The default is "".

  **-takefocus** *boolean* 
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *boolean* is "0",
    this means that this grip window should be skipped entirely during
    keyboard traversal.  "1" means that the this frame's grip window should
    always receive the input focus.  An empty value means that the
    traversal scripts make the decision whether to focus on the window.
    The default is "".

  **-width** *numPixels* 
    Specifies the width of *frameName*. *NumPixels* can be
    single value or a list.  If *numPixels* is a single value it is a
    non-negative value indicating the width the frame. The value may have
    any of the forms accept able to **Tk_GetPixels**, such as "200" or
    "2.4i".  If *numPixels* is a 2 element list, then this sets the minimum
    and maximum limits for the width of the frame. The frame will be at
    least the minimum width and less than or equal to the maximum. If
    *numPixels* is a 3 element list, then this specifies minimum, maximum,
    and nominal width or the frame.  The nominal size overrides the
    calculated height of the frame.  If *numPixels* is "", then the height
    of the requested height of the child widget is used. The default is "".

  **-window** *childName*  
    Specifies the widget to be embedded into *frameName*.  *ChildName* must
    be a child of the *filmstrip* widget.  The *filmstrip* will "pack" and
    manage the size and placement of *childName*.  The default value is "".

*pathName* **grip activate** *frameName* 
  Specifies to draw *frameName*\ 's grip with its active colors and relief
  (see the **-activegripcolor** and **-activegriprelief** options).
  *FrameName* is an index, label, or tag but may not refer to more than
  one tab.  Only one grip may be active at a time.  

*pathName* **grip anchor** *frameName* *x* *y*
   Sets the anchor for the resizing or moving *frameName*.  Either the x or
   y coordinate is used depending upon the orientation of the frame.

*pathName* **grip deactivate** 
  Specifies to draw all grips with its default colors and relief
  (see the **-gripcolor** and **-griprelief** options).

*pathName* **grip mark** *frameName* *x* *y*
  Records *x* or *y* coordinate in the filmstrip window; used with
  later **grip move** commands.  Typically this command is associated
  with a mouse button press in the widget.  It returns an empty string.

*pathName* **grip move** *frameName* *x* *y*
  Moves the grip of *frameName*.  The grip is moved the given distance
  from its previous location (anchor).

*pathName* **grip set** *frameName* *x* *y*
  Sets the location of the *frameName*\ 's grip to the given coordinate
  (*x* or *y*) specified.  The *filmstrip* frames are moved accordingly.

*pathName* **index** *frameName* 
  Returns the index of *frameName*. *FrameName* may be a label, index, or
  tag, but may not represent more than one frame.  If the frame does not
  exist, "-1" is returned.
  
*pathName* **insert after** *whereName* ?\ *label*\ ? ?\ *option *value* ... ? 
  Creates a new frame and inserts it after the frame
  *whereName*. *WhereName* may be a label, index, or tag, but may not
  represent more than one frame.  If a *label* argument is present, then
  this is the name of the new frame.  *Label* can not start with a dash "-"
  or be the name of another frame.  The name of the new frame is
  returned. Note that this operation may change the indices of previously
  created frames.

  If one or more *option*\ -\ *value* pairs are specified, they modify the
  given frame option(s) to have the given value(s).  *Option* and *value*
  are described in the **frame configure** operation.  
  
*pathName* **insert before** *whereName* ?\ *label*\ ? ?\ *option *value* ... ?
  Creates a new frame and inserts it before the frame
  *whereName*. *WhereName* may be a label, index, or tag, but may not
  represent more than one frame.  If a *label* argument is present, then
  this is the name of the new frame.  *Label* can not start with a dash "-"
  or be the name of another frame. The name of the new frame is
  returned. Note that this operation may change the indices of previously
  created frames.

  If one or more *option*\ -\ *value* pairs are specified, they modify the
  given frame option(s) to have the given value(s).  *Option* and *value*
  are described in the **frame configure** operation.  
  
*pathName* **move after** *whereName* *frameName*
  Moves *frameName* after the frame *whereName*.  Both *whereName* and
  *frameName* may be a label, index, or tag, but may not represent more than
  one frame.  The indices of frames may change.
  
*pathName* **move before** *whereName* *frameName*
  Moves *frameName* before the frame *whereName*.  Both *whereName* and
  *frameName* may be a label, index, or tag, but may not represent more than
  one frame. The indices of frames may change.

*pathName* **names** ?\ *pattern* ... ?
  Returns the labels of all the frames.  If one or more *pattern* arguments
  are provided, then the label of any frame matching *pattern* will be
  returned. *Pattern* is a **glob**\ -style pattern.

*pathName* **see** *framemName* 
  Scrolls the *filmstrip* so that *frameName* is visible in the widget's window.
  *FrameName* may be a label, index, or tag, but may not represent more than
  one item.
  
*pathName* **size** 
  Returns the number of frames in the *filmstrip*.

*pathName* **tag add** *tag* ?\ *frameName* ... ?
  Adds the tag to one of more frames. *Tag* is an arbitrary string that can
  not start with a number.  *FrameName* may be a label, index, or tag and
  may refer to multiple frames (for example "all").
  
*pathName* **tag delete** *tag* ?\ *frameName* ... ?
  Deletes the tag from one or more frames. *FrameName* may be a label, index,
  or tag and may refer to multiple frames (for example "all").
  
*pathName* **tag exists** *frameName* ?\ *tag* ... ?
  Indicates if the frame has any of the given tags.  Returns "1" if
  *frameName* has one or more of the named tags, "0" otherwise.  *FrameName*
  may be a label, index, or tag and may refer to multiple frames (for example
  "all").

*pathName* **tag forget** *tag*
  Removes the tag *tag* from all frames.  It's not an error if no
  frames are tagged as *tag*.

*pathName* **tag get** *frameName* ?\ *pattern* ... ?
  Returns the tag names for a given frame.  If one of more pattern
  arguments are provided, then only those matching tags are returned.

*pathName* **tag indices**  ?\ *tag* ... ?
  Returns a list of frames that have the tag.  If no frame is tagged as
  *tag*, then an empty string is returned.

*pathName* **tag names** ?\ *frameName*\ ... ?
  Returns a list of tags used by the *filmstrip* widget.  If one or more
  *frameName* arguments are present, any tag used by *frameName* is returned.

*pathName* **tag set** *frameName* ?\ *tag* ... ?
  Sets one or more tags for a given frame.  *FrameName* may be a label,
  index, or tag and may refer to multiple frames.  Tag names can't start
  with a digit (to distinguish them from indices) and can't be a reserved
  tag ("all").

*pathName* **tag unset** *frameName* ?\ *tag* ... ?
  Removes one or more tags from a given frame. *FrameName* may be a label,
  index, or tag and may refer to multiple frames.  Tag names that don't
  exist or are reserved ("all") are silently ignored.

*pathName* **view moveto** *fraction*
  Adjusts the view in the *filmstrip* window so the portion of
  the frames starting from *fraction* is displayed.  *Fraction* is a number
  between 0.0 and 1.0 representing the position where to
  start displaying frames.
   
*pathName* **view scroll** *number* *what*
  Adjusts the view in the *filmstrip* window according to *number* and
  *what*.  *Number* must be an integer.  *What* must be either "units" or
  "pages".  If *what* is "units", the view adjusts left or right by
  *number* units.  The number of pixel in a unit is specified by the
  **-xscrollincrement** option.  If *what* is "pages" then the view
  adjusts by *number* screenfuls.  If *number* is negative then the view
  if scrolled left; if it is positive then it is scrolled right.

GRIP BINDINGS
-------------

The follow behaviors are defined for the grip windows created for each
frame. The widget class name is BltFilmstripGrip. 

  **<Enter>** 
    Display the grip in its active colors and relief.
  **<Leave>** 
    Display the grip in its normal colors and relief.
  **<ButtonPress-1>** 
    Start scrolling the *filmstrip*.
  **<B1-Motion>**
    Continue scrolling the *filmstrip*.
  **<ButtonRelease-1>** 
    Stop scrolling the *filmstrip*.
  **<KeyPress-Up>**
    If orientation is vertical, then scroll the *filmstrip* upward by 10
    pixels.
  **<KeyPress-Down>**
    If orientation is vertical, then scroll the *filmstrip* downward by 10
    pixels.
  **<KeyPress-Left>**
    If orientation is horizontal, then scroll the *filmstrip* left by 10
    pixels.
  **<KeyPress-Right>**
    If orientation is horizontal, then scroll the *filmstrip* right by 10
    pixels.
  **<Shift-KeyPress-Up>**
    If orientation is vertical, then scroll the *filmstrip* upward by 100
    pixels.
  **<Shift-KeyPress-Down>**
    If orientation is vertical, then scroll the *filmstrip* downward by 100
    pixels.
  **<Shift-KeyPress-Left>**
    If orientation is horizontal, then scroll the *filmstrip* left by 100
    pixels.
  **<Shift-KeyPress-Right>**
    If orientation is horizontal, then scroll the *filmstrip* right by 100
    pixels.

EXAMPLE
-------

The **filmstrip** command creates a new widget.  

  ::

    package require BLT

    blt::filmstrip .fs 

A new TCL command ".fs" is also created.  This new command can be used to
query and modify the *filmstrip* widget.  The default orientation of the
filmstrip is horizontal.  If you want a vertical filmstrip, where frames
run top to bottom, you can set the **-orient** option.

  ::

    # Change the orientation of the filmstrip.
    .fs configure -orient "vertical"

You can then add frames to the widget.  A frame is the container for an
embedded Tk widget.  Note that the embedded Tk widget must be a child of
the filmstrip widget.

  ::
    
    # Add a button to the filmstrip. 
    button .fs.b1
    set frame [.fs add -window .fs.b1]

The variable "frame" now contains the label of the frame.  You can
use that label to set or query configuration options specific to the
frame. You can also use the frame's index or tag to refer to the  frame.

  ::

    # Make the button expand to the size of the frame.
    .fs frame configure $frame -fill both
    
The **-fill** frame option says to may the embedded widget as big as the
frame that contains it.

You can add as many frames as you want to the widget.

  ::

     button .fs.b2 -text "Second" 
     .fs add -window .fs.b2 -fill both
     button .fs.b3 -text "Third" 
     .fs add -window .fs.b3 -fill both
     button .fs.b4 -text "Fourth" 
     .fs add -window .fs.b4 -fill both
     button .fs.b5 -text "Fifth" 
     .fs add -window .fs.b5 -fill both

By default, the *filmstrip* widget's requested height will be the computed
height of all its frame (vertical orientation).  But you can set the
**-height** option to override it.

  ::

    .fs configure -height 1i

Now only a subset of frames is visible.  You can attach a scrollbar
to the filmstrip widget to see the rest.

  ::

    blt::tk::scrollbar .sbar -orient vertical -command { .fs view }
    .fs configure -scrollcommand { .sbar set }

    blt::table . \
	0,0 .fs -fill both \
	0,1 .sbar -fill y
    
If you wanted to flip the filmstrip to be horizontal you would need
to reconfigure the orientation of the filmstrip and scrollbar and
repack.

  ::

    .sbar configure -orient horizontal
    .fs configure -orient horizontal -height 0 -width 1i

    blt::table . \
	0,0 .fs -fill both \
	1,0 .sbar -fill x


If you want the size of all frames to be the size of the filmstrip
window you can configure the frames with the **-relwidth** option.

  ::

    .fs configure -relwidth 1.0

You can programmatically move to specific frames by the **see** operation.

  ::

     # See the third frame. Indices are numbered from 0.
    .fs see

To delete frames there is the **delete** operation.

  ::

     # Delete the first frame.
    .fs delete 0

Note that while the frame has been delete, the button previously
embedded in the frame still exists.  You can use the frame's 
**-deletecommand** option to supply a TCL script to be invoked
before the frame is deleted.

  ::

   .fs frame configure 0 -deletecommand { destroy [%W frame cget 0 -window] }

KEYWORDS
--------

filmstrip, widget

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
