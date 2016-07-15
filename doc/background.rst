
===============
blt::background
===============

------------------------------------------------
Create and manage background styles for widgets.
------------------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::background cget** *bgName* ?\ *option*\ ?

**blt::background configure** *windowName* ?\ *option* *value* ... ?

**blt::background create** *type* ?\ *bgName*\ ? ?\ *option* *value* ... ?

**blt::background delete**  ?\ *bgName* ... ?

**blt::background exists** *bgName*

**blt::background names** ?\ *pattern* ... ?

**blt::background type** *bgName* 

DESCRIPTION
-----------

The **blt::background** creates gradient, tiled, or textured backgrounds
that can be used with the **-background** options of the BLT widgets.

INTRODUCTION
------------

Normally the background of a Tk widget is specified by color name that
specifies a solid color for the background.  The **blt::background**
command lets you defined different types of background (for example a
gradient), that you can use with the BLT widgets.  This includes copies of
the Tk widgets: **blt::tk::button**, **blt::tk::checkbutton**,
**blt::tk::frame**, **blt::tk::label**, **blt::tk::radiobutton**, and
**blt::tk::toplevel**.

A background can have one of the following types: 

  **checkers**
    A checkers *background* object draws a checkered pattern background.
    
  **conical**
    A conical gradient *background* object draws a linear conical gradient
    as a background. Conical gradients are specified by a gradient
    circle. Colors are interpolated along its circumference. The image is
    constructed by creating an infinite canvas and painting it with rays
    rotated around a fixed endpoint which is anchored at the center of the
    gradient circle. The color of the painted ray is the color of the
    gradient circle where the two intersect. This produces a smooth fade
    from each color to the next, progressing clockwise. With color
    selections that significantly differ in lightness, the visual result is
    reminiscent of a cone observed from above, hence the name "conical"
    gradient. The center of the gradient circle is relative to the window
    that it refers to (see the **-relativeto** option).

  **linear**
    A linear gradient *background* object draws a linear gradient as a
    background. Linear gradients are defined by an axis (the gradient line
    segment) with each point on it interpolated to a specific color. The
    lines perpendicular to the gradient line have the same color as the
    point is crosses the gradient line.  The position and length of the
    line segment is relative to the window that it refers to (see the
    **-relativeto** option).  If a perpendicular line is outside the line
    segment, its color may be the low or high color of the gradient, it
    may be repeated, or reversed (depending on the **-repeat** option).

  **radial** 
    A radial gradient *background* object draws a linear radial gradient as
    a background. Radial gradients are defined by a ellipse. Each point is
    interpolated according to its distance from the center of the ellipse
    and the shape of the ellipse.  The position, width, and height of the
    ellipse is relative to the window that it refers to (see the
    **-relativeto** option).
    
  **stripes**
    A stripes *background* object draws a striped pattern background.  The
    stripes may run horizontally or vertically depending upon the
    **-orient** option.

  **tile**
    A tile *background* object draws a tiled background.  The tile is an
    image that it repeated to cover the entire background.  The starting
    position of tiles (i.e the origin) is the upper, left corner of the
    window that it refers to (see the **-relativeto** option).

OPERATIONS
----------

The following operations are available for the **blt::background** command:

**blt::background cget** *bgName* *option*
  Returns the current value of the *background* configuration option given
  by *option*. *BgName* is the name of *background* object returned by the
  **create** operation. *Option* and may have any of the values accepted by
  the **configure** operation. They are specific to the type of background
  for *bgName*. They are described in the **create** operations below.

**blt::background configure** *bgName* ?\ *option* *value* ... ?
  Queries or modifies the *background* configuration options for
  *bgName*. *BgName* is the name of *background* object returned by the
  **create** operation.  *Option* and *value* are specific to the type
  of *bgName*.  If no options are specified, a list describing all of the
  available options for *bgName* (see **Tk_ConfigureInfo** for information
  on the format of this list) is returned.  If *option* is specified with
  no *value*, then this command returns a list describing the one named
  option (this list will be identical to the corresponding sublist of the
  value returned if no *option* is specified).  If one or more *option*\
  -*value* pairs are specified, then this command modifies the given widget
  option(s) to have the given value(s); in this case the command returns
  the empty string.  *Option* and *value* can any of the values accepted by
  the **create** operation.

**blt::background create checkers** ?\ *bgName*\ ? ?\ *option* *value* ... ?
  Creates a checkers *background* object. If no *bgName* argument is
  present, then the name of the *background* is automatically generated in
  the form "background0", "background1", etc. Another *background* object
  can not already exist as *bgName*. *Option* and *value* are specific to
  checkers backgrounds and are listed below.

  **-background** *colorName*

  **-border** *colorName*
    Specifies the border color of the background object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
  **-jitter** *percent*
    Specifies the amount of randomness to add to the interpolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-offcolor** *colorName*
    Specifies the color of odd checkers.  The default is "grey90".

  **-oncolor** *colorName*
    Specifies the color of even checkers. The default is "grey90".

**blt::background create conical** ?\ *bgName*\ ? ?\ *option* *value* ... ?
  Creates a new conical gradient *background* object. Conical gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  If no *bgName* argument is present, then the name of the *background* is
  automatically generated in the form "background0", "background1",
  etc. Another *background* object can not already exist as *bgName*. 
  *Option* and *value* are specific to conical backgrounds and are listed
  below.

  **-background** *colorName*

  **-border** *colorName*
    Specifies the border color of the background object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
  **-colorscale** *scaleName*
    Specifies the scale when interpolating values. *ScaleName* can be
    "linear", or "logarithmic"".

    **linear**
      Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
      Colors are interpolated using the log of the value.

    The default is "linear".
    
  **-decreasing**
    Indicates that the colors are interpolated from high to low.  By
    default colors are interpolated from low to high.

  **-center** *position*
    Specifies the center of the conical gradient.  The center
    position is a relative location in the reference window.  *Position*
    can be one of the following forms.

    *anchorName*
      The position is an anchor position: **nw**, **n**, **ne**,
      **w**, **c**, **e**, **sw**, **s**, or **sw**.  *AnchorName*
      represents a location in the reference window.  For example "nw"
      is the upper left corner of the reference window.

    *vertSide* *horzSide*
      The position is a 2 element list. The first element can be **top**,
      **bottom**, or **center**. The second element can be **left**,
      **right**, or **center**.  The combination of the two sides
      represent the locations in the reference window. For example "top
      left" is the upper left corner of the reference window.

    *number number*
      The position is a list of 2 numbers. *Number* is a real number from
      0 to 1. The number represent relative x and y positions in the
      reference window.  For example "0 0" is the upper left corner of
      the reference window.
        
  **-highcolor** *colorName*
    Specifies the high color of the gradient.  This is the color when the
    gradient value is 1.  This option can be overridden by the **-palette**
    option. The default is "grey90".

  **-lowcolor** *colorName*
    Specifies the low color of the gradient.  This is the color when the
    gradient value is 0.  This option can be overridden by the **-palette**
    option.  The default is "grey50".

  **-jitter** *percent*
    Specifies the amount of randomness to add to the interpolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-palette** *paletteName*
    Specifies a color palette to use when interpolating the background.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *string*

  **-relativeto** *windowName*
    Specifies a reference window for the linear gradient.  This is useful
    for creating seamless gradients with many widgets.  For example if a
    *windowName* is a frame then all the children packed in *window* can
    use the background seamlessly.  *WindowName* can be one of the
    following.

    **self**
      The reference window is the window whose background is being drawn.  

    **toplevel**
      The reference window is the toplevel window whose background is
      being drawn.  This is the default.
       
    *refWindowName*
      The reference window is *refWindowName*.  *RefWindowName* is the name
      of a Tk widget.  It must be an ancestor of the window whose
      background is being drawn. *RefWindowName* doesn't have to exist
      yet. At an idle point later, the background will check for the
      widget, If *refWindowName* is destroyed, the reference window reverts
      to **self**.
       
  **-xoffset** *numPixels*
    Specifies the horizontal offset of the background. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    background.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the background. *NumPixels* is integer
    value indicating amount up or down to offset the background.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::background create linear** ?\ *bgName*\ ? ?\ *option* *value* ... ?
  Creates a new linear gradient *background* object. Linear gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  If no *bgName* argument is present, then the name of the *background* is
  automatically generated in the form "background0", "background1",
  etc. Another *background* object can not already exist as *bgName*.
  *Option* and *value* are specific to linear backgrounds and are listed
  below.

  **-background** *colorName*

  **-border** *colorName*
    Specifies the border color of the background object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
  **-colorscale** *scaleName*
    Specifies the scale when interpolating values. *ScaleName* can be
    "linear", or "logarithmic"".

    **linear**
        Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
        Colors are interpolated using the log of the value.
    
    The default is "linear".
    
  **-decreasing**
    Indicates that the colors are interpolated from high to low.  By
    default colors are interpolated from low to high.

  **-from** *position*
    Specifies the starting position of linear gradient axis.  The starting
    position is a relative location in the reference window.  *Position*
    can be one of the following forms.

    *anchorName*
      The position is an anchor position: **nw**, **n**, **ne**,
      **w**, **c**, **e**, **sw**, **s**, or **sw**.  *AnchorName*
      represents a location in the reference window.  For example "nw"
      is the upper left corner of the reference window.

    *vertSide* *horzSide*
      The position is a 2 element list. The first element can be **top**,
      **bottom**, or **center**. The second element can be **left**,
      **right**, or **center**.  The combination of the two sides
      represent the locations in the reference window. For example "top
      left" is the upper left corner of the reference window.

    *number number*
      The position is a list of 2 numbers. *Number* is a real number from
      0 to 1. The number represent relative x and y positions in the
      reference window.  For example "0 0" is the upper left corner of
      the reference window.
        
  **-highcolor** *colorName*
    Specifies the high color of the gradient.  This is the color when the
    gradient value is 1.  This option can be overridden by the **-palette**
    option. The default is "grey90".

  **-lowcolor** *colorName*
    Specifies the low color of the gradient.  This is the color when the
    gradient value is 0.  This option can be overridden by the **-palette**
    option.  The default is "grey50".

  **-jitter** *percent*
    Specifies the amount of randomness to add to the interpolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-palette** *paletteName*
    Specifies a color palette to use when interpolating the background.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *string*

  **-relativeto** *windowName*
    Specifies a reference window for the linear gradient.  This is useful
    for creating seamless gradients with many widgets.  For example if a
    *windowName* is a frame then all the children packed in *window* can use
    the background seamlessly.  *WindowName* can be one of the following.

    **self**
       The reference window is the window whose background is being drawn.  

    **toplevel**
       The reference window is the toplevel window whose background is
       being drawn.  This is the default.
       
    *refWindowName*
      The reference window is *refWindowName*.  *RefWindowName* is the name
      of a Tk widget.  It must be an ancestor of the window whose
      background is being drawn. *RefWindowName* doesn't have to exist
      yet. At an idle point later, the background will check for the
      widget, If *refWindowName* is destroyed, the reference window reverts
      to **self**.
       
  **-to** *position*
    Specifies the ending position of linear gradient axis.  The ending
    position is a relative location in the reference window.  *Position*
    can be one of the following.

    *anchorName*
      The position is an anchor position: **nw**, **n**, **ne**,
      **w**, **c**, **e**, **sw**, **s**, or **sw**.  *Anchor*
      represents a location in the reference window.  For example "nw"
      is the upper left corner of the reference window.

    *vertSide* *horzSide*
      The position is a 2 element list. The first element can be **top**,
      **bottom**, or **center**. The second element can be **left**,
      **right**, or **center**.  The combination of the two sides
      represent a location in the reference window. For example "top
      left" is the upper left corner of the reference window.

    *number number*
      The position is a list of 2 numbers. *Number* is a real number from
      0 to 1. The number represent relative x and y positions in the
      reference window.  For example "0 0" is the upper left corner of
      the reference window.

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the background. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    background.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the background. *NumPixels* is integer
    value indicating amount up or down to offset the background.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::background create radial** ?\ *bgName*\ ? ?\ *option* *value* ... ?
  Creates a new radial gradient *background* object. Radial gradients are
  defined by an ellipse. Each point is interpolated according to its
  distance from the center of the ellipse and the shape of the ellipse.
  The position, width, and height of the ellipse is relative to the window
  that it refers to (see the **-relativeto** option).

  If no *bgName* argument is present, then the name of the *background* is
  automatically generated in the form "background0", "background1",
  etc. Another *background* object can not already exist as *bgName*.
  *Option* and *value* are specific to radial backgrounds and are listed
  below.

  **-background** *colorName*

  **-border** *colorName*
    Specifies the border color of the background object.  If a widget has a
    3D relief, this specifies the colors of the bevels.
    
  **-colorscale** *scaleName*
    Specifies the scale when interpolating values. *ScaleName* can be
    "linear", or "logarithmic".

    **linear**
      Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
      Colors are interpolated using the log of the value.
    
    The default is "linear".
    
  **-decreasing**
    Indicates that the colors are interpolated from high to low.  By
    default colors are interpolated from low to high.

  **-center** *position*
    Specifies the center of the conical gradient.  The center
    position is a relative location in the reference window.  *Position*
    can be one of the following forms.

    *anchorName*
      The position is an anchor position: **nw**, **n**, **ne**,
      **w**, **c**, **e**, **sw**, **s**, or **sw**.  *AnchorName*
      represents a location in the reference window.  For example "nw"
      is the upper left corner of the reference window.

    *vertSide*  *horzSide*
      The position is a 2 element list. The first element can be **top**,
      **bottom**, or **center**. The second element can be **left**,
      **right**, or **center**.  The combination of the two sides
      represent the locations in the reference window. For example "top
      left" is the upper left corner of the reference window.

    *number number*
      The position is a list of 2 numbers. *Number* is a real number from
      0 to 1. The number represent relative x and y positions in the
      reference window.  For example "0 0" is the upper left corner of
      the reference window.

  **-height** *number*
    Specifies the height of the gradient ellipse.  This is the color when
    the gradient value is 1.  This option can be overridden by the
    **-palette** option. The default is "grey90".

  **-highcolor** *colorName*
    Specifies the high color of the gradient.  This is the color when the
    gradient value is 1.  This option can be overridden by the **-palette**
    option. The default is "grey90".

  **-lowcolor** *colorName*
    Specifies the low color of the gradient.  This is the color when the
    gradient value is 0.  This option can be overridden by the **-palette**
    option.  The default is "grey50".

  **-jitter** *percent*
    Specifies the amount of randomness to add to the interpolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-palette** *paletteName*
    Specifies a color palette to use when interpolating the background.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *string*

  **-relativeto** *windowName*
    Specifies a reference window for the linear gradient.  This is useful
    for creating seamless gradients with many widgets.  For example if a
    *windowName* is a frame then all the children packed in *windowName*
    can use the background seamlessly.  *WindowName* can be one of the
    following.

    **self**
      The reference window is the window whose background is being drawn.  

    **toplevel**
      The reference window is the toplevel window whose background is being
      drawn.  This is the default.
       
    *refWindowName*
      The reference window is *refWindowName*.  *RefWindowName* is the name
      of a Tk widget.  It must be an ancestor of the window whose
      background is being drawn. *RefWindowName* doesn't have to exist
      yet. At an idle point later, the background will check for the
      widget, If *refWindowName* is destroyed, the reference window reverts
      to **self**.
       
  **-width** *number*
    Specifies the width of the gradient ellipse.  This is the color when
    the gradient value is 1.  This option can be overridden by the
    **-palette** option. FIXME The default is "grey90".

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the background. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    background.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the background. *NumPixels* is integer
    value indicating amount up or down to offset the background.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::background create stripes** ?\ *bgName*\ ? ?\ *option* *value* ... ?
  Creates a stripes *background* object.  If no *bgName* argument is
  present, then the name of the *background* is automatically generated in
  the form "background0", "background1", etc. Another *background* object
  can not already exist as *bgName*. *Option* and *value* are specific to
  stripes backgrounds and are listed below.

  **-background** *colorName*
    FIXME
    
  **-border** *colorName*
    Specifies the border color of the background object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
  **-jitter** *percent*
    Specifies the amount of randomness to add to the colors.  *Percent* is
    a real number between 0 and 100.  It is the percentage that colors may
    vary.
     
  **-offcolor** *colorName*
    Specifies the color of odd stripes.  The default is "grey90".

  **-oncolor** *colorName*
    Specifies the color of even stripes. The default is "grey90".

  **-orient** *orient*
    Specifies the orientation of the stripes.  *Orient* may be "vertical"
    of "horizontal".  The default is "vertical".

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the background. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    background.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the background. *NumPixels* is integer
    value indicating amount up or down to offset the background.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::background create tile** ?\ *bgName*\ ? ?\ *option* *value* ... ?
  Creates a tile *background* object.  If no *bgName* argument is present,
  then the name of the *background* is automatically generated in the form
  "background0", "background1", etc. Another *background* object can not
  already exist as *bgName*. *Option* and *value* are specific to tile
  backgrounds and are listed below.

  **-border** *colorName*
    Specifies the border color of the background object.  If a widget has a
    3D relief, this specifies the colors of the bevels and the background
    when there is no tiled image (see the **-image** option below).
    *ColorName* can be any name accepted by **Tk_GetColor**.  The default
    is "grey85".

  **-image** *imageName*
    Specifies the image to use as the tile for the background.  *ImageName*
    must be the name of a Tk **photo** or BLT **picture** image.

  **-jitter** *percent*
    Specifies the amount of randomness to add to the image's colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-relativeto** *refWindowName*
    Specifies a reference window to use of the origin the
    tile. *RefWindowName* is the name of a Tk widget.  This is useful for
    creating seamless tiles with many widgets.  For example is a frame is
    *refWindowName* then all the children packed in *refWindowName* can use
    the same tile seamlessly.  If *refWindowName* is "", then the origin is
    based on the widget using the tile.  The default is "".

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the background. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    background.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the background. *NumPixels* is integer
    value indicating amount up or down to offset the background.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::background delete** ?\ *bgName* ... ?
  Releases resources allocated by the background command for *window*,
  including the background window.  User events will again be received
  again by *window*.  Resources are also released when *window* is
  destroyed. *Window* must be the name of a widget specified in the
  **create** operation, otherwise an error is reported.

**blt::background exists** *bgName*
  Indicates if the background *bgName* exists. *BgName* is the name of a
  background created by the **create** operation. Returns "1" if the named
  background exists, "0" otherwise.

**blt::background names** ?\ *pattern* ... ?
  Returns the names of all the backgrounds.  If one or more *pattern*
  arguments are provided, then the name of any background matching
  *pattern* will be returned. *Pattern* is a glob-style pattern.

**blt::background type** *bgName*
  Returns the type of the background for *bgName*.  *BgName* is the name of
  a background created by the **create** operation.

EXAMPLE
-------

Create a *background* object with the **blt::background** command.

 ::

    package require BLT

    # Create a new linear gradient background.
    blt::background create linear myBackground \
        -from n -to s -lowcolor grey80 -highcolor grey95 \
        -relativeto .frame -jitter 10
        
Now we can create widgets that use the background.

 ::

    blt::tk::frame .frame -bg myBackground
    blt::tk::label .frame.label -text "Label" -bg myBackground
    blt::tk::button .frame.label -text "Button" -bg myBackground
    blt::graph .frame.graph -bg myBackground

To remove the background, use the **delete** operation.

 ::

    blt::background delete myBackground
     
Please note the following:

1. The backgrounds created by the **blt::background** command are only
   recognized by BLT widgets.

2. The reference window designated with the **-relativeto** option doesn't
   have to already exist when you create the background.

3. If you change a background option (such as **-highcolor**) all the
   widgets using the background object will be notified and automatically
   redraw themselves.

4. Backgrounds are reference counted.  If you delete a background, its
   resources are not freed until no widget is using it.
   
KEYWORDS
--------
background, window


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
