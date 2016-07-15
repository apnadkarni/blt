
===============
blt::paintbrush
===============

--------------------------------
Create and manage paintbrushes.
--------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::paintbrush cget** *brushName* ?\ *option*\ ?

**blt::paintbrush configure** *window* ?\ *option* *value* ... ?

**blt::paintbrush create** *type* ?\ *brushName*\ ? ?\ *option* *value* ... ?

**blt::paintbrush delete**  ?\ *brushName* ... ?

**blt::paintbrush names** ?\ *pattern* ... ?

**blt::paintbrush type** *brushName* 

DESCRIPTION
-----------

The **blt::paintbrush** creates a paintbrush that can be used
with the **-brush** options of the BLT widgets.  

INTRODUCTION
------------

A paintbrush can have one of the following types: 

  **checkers**
    This *paintbrush* object draws a checkered pattern.
  **color**
    A color *paintbrush* object draws a single color.
  **conical**
    A conical gradient *paintbrush* object draws a linear conical gradient
    as a paintbrush. Conical gradients are specified by a gradient
    circle. Colors are interpolated along its circumference. The image is
    constructed by creating an infinite canvas and painting it with rays
    rotated around a fixed endpoint which is anchored at the center of the
    gradient circle. The color of the painted ray is the color of the
    gradient circle where the two intersect. This produces a smooth fade
    from each color to the next, progressing clockwise. With color
    selections that significantly differ in lightness, the visual result is
    reminiscent of a cone observed from above, hence the name "conical"
    gradient. 
  **linear**
    A linear gradient *paintbrush* object draws a linear gradient as a
    paintbrush. Linear gradients are defined by an axis (the gradient line
    segment) with each point on it interpolated to a specific color. The
    lines perpendicular to the gradient line have the same color as the
    point is crosses the gradient line.  If a perpendicular line is outside
    the line segment, its color may be the low or high color of the
    gradient, it may be repeated, or reversed (depending on the **-repeat**
    option).
  **radial** 
    A radial gradient *paintbrush* object draws a linear radial gradient as
    a paintbrush. Radial gradients are defined by a gradent ellipse. Each
    point is interpolated according to its distance from the center of the
    ellipse and the shape of the ellipse.  
  **stripes**
    This *paintbrush* object draws a striped pattern.  The stripes may
    run horizontally or vertically depending upon the **-orient** option.
  **tile**
    A tile *paintbrush* object draws a tiled paintbrush.  The tile is an
    image that it repeated to cover the entire paintbrush.  

OPERATIONS
----------

The following operations are available for the **blt::paintbrush** command:

**blt::paintbrush cget** *brushName* *option*
  Returns the current value of the *paintbrush* configuration option given
  by *option*. *BrushName* is the name of *paintbrush* object returned by the
  **create** operation. *Option* and may have any of the values accepted by
  the **configure** operation. They are specific to the type of paintbrush
  for *brushName*. They are described in the **create** operations below.

**blt::paintbrush configure** *brushName* ?\ *option* *value* ... ?
  Queries or modifies the *paintbrush* configuration options for
  *brushName*. *BrushName* is the name of *paintbrush* object returned by the
  **create** operation.  *Option* and *value* are specific to the type
  of *brushName*.  If no options are specified, a list describing all of the
  available options for *brushName* (see **Tk_ConfigureInfo** for information
  on the format of this list) is returned.  If *option* is specified with
  no *value*, then this command returns a list describing the one named
  option (this list will be identical to the corresponding sublist of the
  value returned if no *option* is specified).  If one or more *option*\
  -*value* pairs are specified, then this command modifies the given widget
  option(s) to have the given value(s); in this case the command returns
  the empty string.  *Option* and *value* can any of the values accepted by
  the **create** operation.

**blt::paintbrush create checkers** ?\ *option* *value* ... ?
  Creates a checkers *paintbrush* object. It paints a checkered pattern
  using two colors.  This command returns the name of the paintbrush.  The
  name is automatically generated in the form "paintbrush0", "paintbrush1",
  etc.  *Option* and *value* are specific to checkers paintbrushes and are
  listed below.

  **-jitter** *percent*
    Specifies the amount of randomness to add to the interpolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary. The default is "0".
     
  **-offcolor** *colorName*
    Specifies the color of odd checkers.  The default is "grey97".

  **-oncolor** *colorName*
    Specifies the color of even checkers. The default is "grey90".

  **-stride** *numPixels*
    Specifies the width of the checkers. *NumPixels* is a positive integer
    value indicating the width of the checkers.  The value may have any of
    the forms accept able to **Tk_GetPixels**, such as "1.2i".  The default
    is "10".

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    **Tk_GetPixels**, such as "1.2i".  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to **Tk_GetPixels**, such as
    "1.2i".  The default is "0".

**blt::paintbrush create conical** ?\ *option* *value* ... ?
  Creates a new conical gradient *paintbrush* object. Conical gradients are
  defined by a circle with each point in it interpolated to a specific
  color. The distance from the center of the circle determines how the
  color is interpolated.
  
  This command returns the name of *paintbrush* object.  The name of the
  *paintbrush* is automatically generated in the form "paintbrush0",
  "paintbrush1", etc.  The name of the new *paintbrush* is
  returned. *Option* and *value* are specific to conical paintbrushes and
  are listed below.

  **-center** *position*
    Specifies the center of the conical gradient.  The center
    position is a relative location in the reference window.  *Position*
    can be one of the following forms.

    *anchorName*
      The position is an anchor position: **nw**, **n**, **ne**,
      **w**, **c**, **e**, **sw**, **s**, or **sw**.  *AnchorName*
      represents a location in the reference window.  For example "nw"
      is the upper left corner of the reference window.

    "*sideName sideName*"
      The position is a 2 element list. The first element can be **top**,
      **bottom**, or **center**. The second element can be **left**,
      **right**, or **center**.  The combination of the two sides
      represent the locations in the reference window. For example "top
      left" is the upper left corner of the reference window.

    "*relNumber relNumber*"
      The position is a list of 2 numbers. *RelNumber* is a real number from 0
      to 1. The number represent relative x and y positions in the reference
      window.  For example "0 0" is the upper left corner of the reference
      window.

  **-colorscale** *scaleName*
    Specifies how colors are interpolated from values. *ScaleName* can be
    any of the following.

    **linear**
      Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
      Colors are interpolated using the log of the value.
    
    The default is "linear".

  **-decreasing**
    Indicates that the colors are interpolated from high to low.  By
    default colors are interpolated from low to high.

  **-diameter** *relNumber*
    Specifies the diameter of the cone. The default is "0.0".

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
    that colors may vary. The default is "0".
     
  **-palette** *paletteName*
    Specifies a color palette to use when interpolating color values.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *how*
    Specifies if the gradient should be repeated outside of the
    circle.  *How* can be any of the following.

    **yes**
      Repeat the gradient for starting from the low color.
    
    **no**
      Do not repeat the gradient.  All colors outside the gradient are
      transparent.

    **reversing**
      Repeat the gradient but alternate low-to-high, high-to-low
      interpolation.

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to **Tk_GetPixels**.  The default
    is "0".

**blt::paintbrush create linear** ?\ *option* *value* ... ?
  Creates a new linear gradient *paintbrush* object. Linear gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *paintbrush* object.  The name of the
  *paintbrush* is automatically generated in the form "paintbrush0",
  "paintbrush1", etc.  The name of the new *paintbrush* is
  returned. *Option* and *value* are specific to linear paintbrushes and
  are listed below.

  **-colorscale** *scaleName*
    Specifies how colors are interpolated from values. *ScaleName* can be
    any of the following.

    **linear**
      Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
      Colors are interpolated using the log of the value.
    
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

    "*sideName sideName*"
      The position is a 2 element list. The first element can be **top**,
      **bottom**, or **center**. The second element can be **left**,
      **right**, or **center**.  The combination of the two sides
      represent the locations in the reference window. For example "top
      left" is the upper left corner of the reference window.

    "*relNumber relNumber*"
      The position is a list of 2 numbers. *RelNumber* is a real number
      from 0 to 1. The number represent relative x and y positions in the
      reference window.  For example "0 0" is the upper left corner of the
      reference window.
        
  **-highcolor** *colorName*
    Specifies the high color of the gradient.  This is the color
    when the gradient value is 1.  This option can be overridden
    by the **-palette** option. The default is "grey90".

  **-lowcolor** *colorName*
    Specifies the low color of the gradient.  This is the color 
    when the gradient value is 0.  This option can be overridden
    by the **-palette** option.  The default is "grey50".

  **-jitter** *percent*
    Specifies the amount of randomness to add to the interpolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary. The default is "0".
     
  **-palette** *paletteName*
    Specifies a color palette to use when interpolating color values.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *string*

  **-to** *position*
    Specifies the ending position of linear gradient axis.  The ending
    position is a relative location in the reference window.  *Position*
    can be one of the following.

    *anchorName*
      The position is an anchor position: **nw**, **n**, **ne**,
      **w**, **c**, **e**, **sw**, **s**, or **sw**.  *AnchorName*
      represents a location in the reference window.  For example "nw"
      is the upper left corner of the reference window.

    "*sideName sideName*"
      The position is a 2 element list. The first element can be **top**,
      **bottom**, or **center**. The second element can be **left**,
      **right**, or **center**.  The combination of the two sides
      represent a locationj in the reference window. For example "top
      left" is the upper left corner of the reference window.

    "*relNumber relNumber*"
      The position is a list of 2 numbers. *RelNumber* is a real number
      from 0 to 1. The number represent relative x and y positions in the
      reference window.  For example "0 0" is the upper left corner of the
      reference window.

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to **Tk_GetPixels**.  The default is
    "0".

**blt::paintbrush create radial** ?\ *option* *value* ... ?
  Creates a new radial gradient *paintbrush* object. Radial gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *paintbrush* object.  The name of the
  *paintbrush* is automatically generated in the form "paintbrush0",
  "paintbrush1", etc.  The name of the new *paintbrush* is
  returned. *Option* and *value* are specific to "linear" paintbrushes and
  are listed below.

  **-center** *position*
     Specifies the center of the radial gradient.  The center position is
     a relative location in the reference window.  *Position* can be one of
     the following forms.

     *anchorName*
       The position is an anchor position: **nw**, **n**, **ne**,
       **w**, **c**, **e**, **sw**, **s**, or **sw**.  *AnchorName*
       represents a location in the reference window.  For example "nw"
       is the upper left corner of the reference window.

     "*sideName sideName*"
       The position is a 2 element list. The first element can be **top**,
       **bottom**, or **center**. The second element can be **left**,
       **right**, or **center**.  The combination of the two sides
       represent the locations in the reference window. For example "top
       left" is the upper left corner of the reference window.

     "*relNumber relNumber*"
       The position is a list of 2 numbers. *RelNumber* is a real number
       from 0 to 1. The number represent relative x and y positions in the
       reference window.  For example "0 0" is the upper left corner of the
       reference window.
        
  **-colorscale** *scaleName*
    Specifies how colors are interpolated from values. *ScaleName* can be
    any of the following.

    **linear**
      Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
      Colors are interpolated using the log of the value.
    
  **-decreasing**
    Indicates that the colors are interpolated from high to low.  By
    default colors are interpolated from low to high.

  **-height** *relNumber*
    Specifies the height of the gradient ellipse.  *RelNumber* is a real
    number between 0 and 1 that the relative height of the ellipse. The
    default is "1.0".

  **-highcolor** *colorName*
    Specifies the high color of the gradient.  This is the color when the
    gradient value is 1.  This option can be overridden by the **-palette**
    option. The default is "grey90".

  **-lowcolor** *colorName*
    Specifies the low color of the gradient.  This is the color when the
    gradient value is 0.  This option can be overridden by the **-palette**
    option.  The default is "grey50".

  **-jitter** *percent*
    Specifies the amount of randomness to add to the interpolated pixel
    colors.  *Percent* is a real number between 0 and 100.  It is the
    percentage that colors may vary. The default is "0".
     
  **-palette** *paletteName*
    Specifies a color palette to use when interpolating color values.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *string*

  **-width** *relNumber*
    Specifies the width of the gradient ellipse.  *RelNumber* is a real
    number between 0 and 1 that the relative width of the ellipse. The
    default is "1.0".

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to **Tk_GetPixels**.  The default
    is "0".

**blt::paintbrush create stripes** ?\ *option* *value* ... ?
  Creates a stripes *paintbrush* object. It paints a striped pattern using
  two colors. This command returns the name of the paintbrush.  It is
  automatically generated in the form "paintbrush0", "paintbrush1", etc.
  *Option* and *value* are specific to stripes paintbrushes and are listed
  below.

  **-jitter** *percent*
    Specifies the amount of randomness to add to the pixel colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary. The default is "0".
     
  **-offcolor** *colorName*
    Specifies the color of the odd-numbered stripes.  The default is
    "grey90".

  **-oncolor** *colorName*
    Specifies the color of even-numbered stripes. The default is "grey90".

  **-orient** *orientation*
    Specifies the orientation of the stripes.  *Orientation* may be
    "vertical" of "horizontal".  The default is "vertical".

  **-stride** *numPixels*
    Specifies the width of the stripes. *NumPixels* is a positive integer
    value indicating the width of the stripes.  The value may have any of
    the forms accept able to **Tk_GetPixels**, such as "1.2i".  The default
    is "2".

  **-xoffset** *numPixels*
    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    **Tk_GetPixels**, such as "1.2i".  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to **Tk_GetPixels**, such as
    "1.2i".  The default is "0".

**blt::paintbrush create tile** ?\ *option* *value* ... ?
  Creates a tile *paintbrush* object. The name of the *paintbrush* is
  automatically generated in the form "paintbrush0", "paintbrush1", etc.
  The name of the new *paintbrush* is returned. *Option* and *value* are
  specific to "texture" paintbrushes and are listed below.

  **-image** *imageName*
    Specifies the image to use as the tile for the paintbrush.  *ImageName*
    must be the name of a Tk **photo** or BLT **picture** image.

  **-jitter** *percent*
    Specifies the amount of randomness to add to the image's colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary. The default is "0".
     
  **-xoffset** *numPixels*
    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "0".

  **-yoffset** *numPixels*
    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to **Tk_GetPixels**.  The default
    is "0".

**blt::paintbrush delete** ?\ *brushName* ... ?
  Releases resources allocated by the paintbrush command for *window*,
  including the paintbrush window.  User events will again be received
  again by *window*.  Resources are also released when *window* is
  destroyed. *Window* must be the name of a widget specified in the
  **create** operation, otherwise an error is reported.

**blt::paintbrush names** ?\ *pattern* ... ?
  Returns the names of all the paintbrushes currently created.  If one or
  more *pattern* arguments are provided, then the name of any paintbrush
  matching *pattern* will be returned. *Pattern* is a **glob**-style pattern.

**blt::paintbrush type** *brushName*
  Returns the type of the paintbrush for *brushName*.  *BrushName* is the
  name of a paintbrush created by the **create** operation.

EXAMPLE
-------

Create a *paintbrush* object with the **blt::paintbrush** command.

 ::

    package require BLT

    # Create a new linear gradient paintbrush.
    blt::paintbrush create linear myPaintbrush \
        -from n -to s -lowcolor grey80 -highcolor grey95 \
        -jitter 10
        
Now we can create widgets that use the paintbrush.

 ::

    blt::tk::frame .frame -bg myPaintbrush
    blt::tk::label .frame.label -text "Label" -bg myPaintbrush
    blt::tk::button .frame.label -text "Button" -bg myPaintbrush
    blt::graph .frame.graph -bg myPaintbrush

To remove the paintbrush, use the **delete** operation.

 ::

    blt::paintbrush delete myPaintbrush
     
Please note the following:

1. The paintbrushes created by the **blt::paintbrush** command are only
   recognized by BLT widgets.

3. If you change a paintbrush option (such as **-highcolor**) all the
   widgets using the paintbrush object will be notified and automatically
   redraw themselves.

4. Paintbrushes are reference counted.  If you delete a paintbrush, its
   resources are not freed until no widget is using it.
   
KEYWORDS
--------

paintbrush, window


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
