
===============
blt::paintbrush
===============

--------------------------------
Create and manage paintbrushes.
--------------------------------

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

**blt::paintbrush cget** *brushName* ?\ *option*\ ?

**blt::paintbrush configure** *window* ?\ *option* *value* ...\ ?

**blt::paintbrush create** *type* ?\ *brushName*\ ? ?\ *option* *value* ...\ ?

**blt::paintbrush delete**  ?\ *brushName* ...\ ?

**blt::paintbrush names** ?\ *pattern* ...\ ?

**blt::paintbrush type** *brushName* 

DESCRIPTION
-----------

The **blt::paintbrush** creates a paintbrush that can be used
with the **-brush** options of the BLT widgets.  

INTRODUCTION
------------

Normally the paintbrush of a Tk widget is specified by color name
that specifies a solid color for the paintbrush.  The **blt::paintbrush**
command lets you defined different types of paintbrush (for example a
gradient), that you can use with the BLT widgets.  This includes
copies of the Tk widgets: **blt::tk::button**, **blt::tk::checkbutton**,
**blt::tk::frame**, **blt::tk::label**, **blt::tk::radiobutton**, and
**blt::tk::toplevel**.

A paintbrush can have one of the following types: 

  **checker**

    A checker *paintbrush* object draws a checkered paintbrush.
    
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
    gradient. The center of the gradient circle is relative to the window
    that it refers to (see the **-relativeto** option).

  **linear**

    A linear gradient *paintbrush* object draws a linear gradient as a
    paintbrush. Linear gradients are defined by an axis (the gradient line
    segment) with each point on it interpolated to a specific color. The
    lines perpendicular to the gradient line have the same color as the
    point is crosses the gradient line.  The position and length of the
    line segment is relative to the window that it refers to (see the
    **-relativeto** option).  If a perpendicular line is outside the line
    segment, its color may be the low or high color of the gradient, it
    may be repeated, or reversed (depending on the **-repeat** option).

  **radial** 

    A radial gradient *paintbrush* object draws a linear radial gradient as
    a paintbrush. Radial gradients are defined by a gradent ellipse. Each
    point is interpolated according to its distance from the center of the
    ellipse and the shape of the ellipse.  The position, width, and height
    of the ellipse is relative to the window that it refers to (see the
    **-relativeto** option).
    
  **stripe**

    A stripe *paintbrush* object draws a striped paintbrush.  The stripes may
    run horizontally or vertically depending upon the **-orient** option.

  **tile**

    A tile *paintbrush* object draws a tiled paintbrush.  The tile is an
    image that it repeated to cover the entire paintbrush.  The starting
    position of tiles (i.e the origin) is the upper, left corner of the
    window that it refers to (see the **-relativeto** option).

OPERATIONS
----------

The following operations are available for the **blt::paintbrush** command:

**blt::paintbrush cget** *brushName* *option*

  Returns the current value of the *paintbrush* configuration option given
  by *option*. *BrushName* is the name of *paintbrush* object returned by the
  **create** operation. *Option* and may have any of the values accepted by
  the **configure** operation. They are specific to the type of paintbrush
  for *brushName*. They are described in the **create** operations below.

**blt::paintbrush configure** *brushName* ?\ *option* *value* ...\ ?

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

**blt::paintbrush create checker** ?\ *option* *value* ...\ ?

  Creates a checker *paintbrush* object. Radial gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *paintbrush* object.  The name of the
  *paintbrush* is automatically generated in the form "paintbrush0",
  "paintbrush1", etc.  The name of the new *paintbrush* is
  returned. *Option* and *value* are specific to "linear" paintbrushs and
  are listed below.

  **-paintbrush** *colorName*

  **-border** *colorName*

    Specifies the border color of the paintbrush object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
  **-jitter** *percent*

    Specifies the amount of randomness to add to the intepolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-offcolor** *colorName*

    Specifies the color of odd checkers.  The default is "grey90".

  **-oncolor** *colorName*

    Specifies the color of even checkers. The default is "grey90".

**blt::paintbrush create conical** ?\ *option* *value* ...\ ?

  Creates a new conical gradient *paintbrush* object. Conical gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *paintbrush* object.  The name of the
  *paintbrush* is automatically generated in the form "paintbrush0",
  "paintbrush1", etc.  The name of the new *paintbrush* is
  returned. *Option* and *value* are specific to "linear" paintbrushs and
  are listed below.

  **-colorscale** *scale*

    Specifies the scale when interpolating values. *Scale* can be "linear",
    or "logarithmic"".

    **linear**
	Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
	Colors are interpolated using the log of the value.
    
  **-decreasing**

    Indicates that the colors are interpolated from high to low.  By
    default colors are interpolated from low to high.

  **-center** *position*

     Specifies the center of the conical gradient.  The center
     position is a relative location in the reference window.  *Position*
     can be one of the following forms.

     *anchor*
        The position is an anchor position: **nw**, **n**, **ne**,
	**w**, **c**, **e**, **sw**, **s**, or **sw**.  *Anchor*
	represents a location in the reference window.  For example "nw"
	is the upper left corner of the reference window.

     *side side*
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

    Specifies the high color of the gradient.  This is the color
    when the gradient value is 1.  This option can be overridden
    by the **-palette** option. The default is "grey90".

  **-lowcolor** *colorName*

    Specifies the low color of the gradient.  This is the color 
    when the gradient value is 0.  This option can be overridden
    by the **-palette** option.  The default is "grey50".

  **-jitter** *percent*

    Specifies the amount of randomness to add to the intepolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-palette** *paletteName*

    Specifies a color palette to use when interpolating the paintbrush.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *string*

  **-xoffset** *numPixels*

    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*

    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::paintbrush create linear** ?\ *option* *value* ...\ ?

  Creates a new linear gradient *paintbrush* object. Linear gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *paintbrush* object.  The name of the
  *paintbrush* is automatically generated in the form "paintbrush0",
  "paintbrush1", etc.  The name of the new *paintbrush* is
  returned. *Option* and *value* are specific to "linear" paintbrushs and
  are listed below.

  **-paintbrush** *colorName*

  **-border** *colorName*

    Specifies the border color of the paintbrush object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
  **-colorscale** *scale*

    Specifies the scale when interpolating values. *Scale* can be "linear",
    or "logarithmic"".

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

     *anchor*
        The position is an anchor position: **nw**, **n**, **ne**,
	**w**, **c**, **e**, **sw**, **s**, or **sw**.  *Anchor*
	represents a location in the reference window.  For example "nw"
	is the upper left corner of the reference window.

     *side side*
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

    Specifies the high color of the gradient.  This is the color
    when the gradient value is 1.  This option can be overridden
    by the **-palette** option. The default is "grey90".

  **-lowcolor** *colorName*

    Specifies the low color of the gradient.  This is the color 
    when the gradient value is 0.  This option can be overridden
    by the **-palette** option.  The default is "grey50".

  **-jitter** *percent*

    Specifies the amount of randomness to add to the intepolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-palette** *paletteName*

    Specifies a color palette to use when interpolating the paintbrush.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *string*

  **-to** *position*

    Specifies the ending position of linear gradient axis.  The ending
    position is a relative location in the reference window.  *Position*
    can be one of the following.

    *anchor*
        The position is an anchor position: **nw**, **n**, **ne**,
	**w**, **c**, **e**, **sw**, **s**, or **sw**.  *Anchor*
	represents a location in the reference window.  For example "nw"
	is the upper left corner of the reference window.

    *side side*
        The position is a 2 element list. The first element can be **top**,
        **bottom**, or **center**. The second element can be **left**,
        **right**, or **center**.  The combination of the two sides
        represent a locationj in the reference window. For example "top
        left" is the upper left corner of the reference window.

    *number number*
        The position is a list of 2 numbers. *Number* is a real number from
	0 to 1. The number represent relative x and y positions in the
	reference window.  For example "0 0" is the upper left corner of
	the reference window.

  **-xoffset** *numPixels*

    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*

    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::paintbrush create radial** ?\ *option* *value* ...\ ?

  Creates a new radial gradient *paintbrush* object. Radial gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *paintbrush* object.  The name of the
  *paintbrush* is automatically generated in the form "paintbrush0",
  "paintbrush1", etc.  The name of the new *paintbrush* is
  returned. *Option* and *value* are specific to "linear" paintbrushs and
  are listed below.

  **-paintbrush** *colorName*

  **-border** *colorName*

    Specifies the border color of the paintbrush object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
  **-colorscale** *scale*

    Specifies the scale when interpolating values. *Scale* can be "linear",
    or "logarithmic"".

    **linear**
	Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
	Colors are interpolated using the log of the value.
    
  **-decreasing**

    Indicates that the colors are interpolated from high to low.  By
    default colors are interpolated from low to high.

  **-center** *position*

     Specifies the center of the conical gradient.  The center
     position is a relative location in the reference window.  *Position*
     can be one of the following forms.

     *anchor*
        The position is an anchor position: **nw**, **n**, **ne**,
	**w**, **c**, **e**, **sw**, **s**, or **sw**.  *Anchor*
	represents a location in the reference window.  For example "nw"
	is the upper left corner of the reference window.

     *side side*
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

    Specifies the height of the gradient ellipse.  This is the color
    when the gradient value is 1.  This option can be overridden
    by the **-palette** option. The default is "grey90".


  **-highcolor** *colorName*

    Specifies the high color of the gradient.  This is the color
    when the gradient value is 1.  This option can be overridden
    by the **-palette** option. The default is "grey90".

  **-lowcolor** *colorName*

    Specifies the low color of the gradient.  This is the color 
    when the gradient value is 0.  This option can be overridden
    by the **-palette** option.  The default is "grey50".

  **-jitter** *percent*

    Specifies the amount of randomness to add to the intepolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-palette** *paletteName*

    Specifies a color palette to use when interpolating the paintbrush.
    *PaletteName* is the name of a palette is created by the
    **blt::palette** command.  If *paletteName* is "", then the
    **-highcolor** and **-lowcolor** colors are interpolated.  The default
    is "".

  **-repeat** *string*

  **-width** *number*

    Specifies the width of the gradient ellipse.  This is the color
    when the gradient value is 1.  This option can be overridden
    by the **-palette** option. The default is "grey90".

  **-xoffset** *numPixels*

    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*

    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::paintbrush create stripe** ?\ *option* *value* ...\ ?

  Creates a stripe *paintbrush* object. Radial gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *paintbrush* object.  The name of the
  *paintbrush* is automatically generated in the form "paintbrush0",
  "paintbrush1", etc.  The name of the new *paintbrush* is
  returned. *Option* and *value* are specific to "linear" paintbrushs and
  are listed below.

  **-paintbrush** *colorName*

  **-border** *colorName*

    Specifies the border color of the paintbrush object.  If a widget
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

    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*

    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::paintbrush create tile** ?\ *option* *value* ...\ ?

  Creates a tile *paintbrush* object. The name of the *paintbrush* is
  automatically generated in the form "paintbrush0", "paintbrush1", etc.
  The name of the new *paintbrush* is returned. *Option* and *value* are
  specific to "texture" paintbrushs and are listed below.

  **-border** *colorName*

    Specifies the border color of the paintbrush object.  If a widget has a
    3D relief, this specifies the colors of the bevels and the paintbrush
    when there is no tiled image (see the **-image** option below).
    *ColorName* can be any name accepted by **Tk_GetColor**.  The default
    is "grey85".

  **-image** *imageName*

    Specifies the image to use as the tile for the paintbrush.  *ImageName*
    must be the name of a Tk **photo** or BLT **picture** image.

  **-jitter** *percent*

    Specifies the amount of randomness to add to the image's colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
     
  **-xoffset** *numPixels*

    Specifies the horizontal offset of the paintbrush. *NumPixels* is
    integer value indicating amount to the left or right to offset the
    paintbrush.  The value may have any of the forms accept able to
    Tk_GetPixels.  The default is "0".

  **-yoffset** *numPixels*

    Specifies the vertical offset of the paintbrush. *NumPixels* is integer
    value indicating amount up or down to offset the paintbrush.  The value
    may have any of the forms accept able to Tk_GetPixels.  The default is
    "0".

**blt::paintbrush delete** ?\ *brushName* ...\ ?

  Releases resources allocated by the paintbrush command for *window*, including
  the paintbrush window.  User events will again be received again by *window*.
  Resources are also released when *window* is destroyed. *Window* must be
  the name of a widget specified in the **create** operation, otherwise an
  error is reported.

**blt::paintbrush names** ?\ *pattern* ...\ ?

  Returns the names of all the paintbrushs currently created.  If one or
  more *pattern* arguments are provided, then the name of any paintbrush
  matching *pattern* will be returned. *Pattern* is a glob-style pattern.

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

1. The paintbrushs created by the **blt::paintbrush** command are only
   recognized by BLT widgets.

2. The reference window designated with the **-relativeto** option doesn't
   have to already exist when you create the paintbrush.

3. If you change a paintbrush option (such as **-highcolor**) all the
   widgets using the paintbrush object will be notified and automatically
   redraw themselves.

4. Paintbrushs are reference counted.  If you delete a paintbrush, its
   resources are not freed until no widget is using it.
   
KEYWORDS
--------

paintbrush, window


