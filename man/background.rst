
===============
blt::background
===============

----------------------------------------------------------------
Create and manage background styles for widgets.
----------------------------------------------------------------

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

**blt::background cget** *bgName* ?\ *option*\ ?

**blt::background configure** *window* ?\ *option* *value* ...\ ?

**blt::background create** *type* ?\ *bgName*\ ? ?\ *option* *value* ...\ ?

**blt::background delete**  *bgName* ?\ *bgName* ...\ ?

**blt::background names** ?\ *pattern* ...\ ?

**blt::background type** *bgName* 

DESCRIPTION
-----------

The **blt::background** command provides a simple means to block
keyboard, button, and pointer events from Tk widgets, while overriding
the widget's cursor with a configurable background cursor.

INTRODUCTION
------------

There are many times in applications where you want to temporarily
restrict what actions the user can take.  For example, an application
could have a "run" button that when pressed causes some processing to
occur.  But while the application is background processing, you probably don't
want the the user to be able to click the "run" button again.  You
may also want restrict the user from other tasks such as clicking a
"print" button.

The **blt::background** command lets you make Tk widgets background. This means
that user interactions such as button clicks, moving the mouse, typing
at the keyboard, etc. are ignored by the widget.  You can set a
special cursor (like a watch) that overrides the widget's normal
cursor, providing feedback that the application (widget) is
temporarily background.

When a widget is made background, the widget and all of its descendents will
ignore events.  It's easy to make an entire panel of widgets background. You
can simply make the toplevel widget (such as ".") background.  This is
easier and far much more efficient than recursively traversing the
widget hierarchy, disabling each widget and re-configuring its cursor.

The **blt::background** command isn't a replacement for the Tk **grab** command.
There are times where the **blt::background** command can be used instead of Tk's
**grab** command.  Unlike **grab** which restricts all user interactions to
one widget, with the background command you can have more than one widget active
(for example, a "cancel" dialog and a "help" button).


OPERATIONS
----------

The following operations are available for the **blt::background** command:

**blt::background cget** *bgName* ?\ *option*\ ?

  Returns the current value of the *background* configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described in the **configure**
  operation below.

**blt::background configure** *bgName* ?\ *option* *value* ...\ ?

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

**blt::background create gradient** ?\ *option* *value* ...\ ?

  Creates a new gradient *background* object. The name of the *background* is
  automatically generated in the form "background0", "background1", etc.
  The name of the new *background* is returned. *Option* and *value* are
  specific to "tile" backgrounds and are listed below.

  **-background** *colorName*

    Specifies the color of the tile background.  The color is used for the
    drawing 3-D borders and when there is no tiled image (see the
    **-image** option below).  *ColorName* can be any name accepted by
    **Tk_GetColor**.  The default is "grey85".

  **-angle** *degrees*
    Specifies the angle of gradient to generate. *Dir* can be
    any of the following:

  **-start *list*

    bottom  = vertical bottom to top 
    top bottom = vertical bottom top top
    top bottom = top to bottom
    
    bottom =  vertical bottom to top
    center + bottom = vertical bottom to top
    center + bottom + left = diagonal bottom left to top right
    center + bottom + top = vertical spread bottom and top
    left + bottom + top = vertical spread bottom and top
    right + bottom + top = vertical spread bottom and top
    left + bottom + top = vertical spread bottom and top
    right + bottom + top = vertical spread bottom and top
    left + right + top = horizontal spread 
    center left right 
    center left right 
    
  **-direction** *center horizontal"
  **-direction** *center vertical"
  **-direction** *center bottom"
  **-direction** *center right" 

    *top left*   goes to "bottom right"  
    *top right*  goes to "bottom left"
    *top center* goes to "bottom center"
    *bottom center* goes to "top center"
    *left center* goes to "right center"
    *right center* goes to "left center"
    *center* goes left and right

  **-from** *left top colorName*
 
  **-fromcolor**
  **-fromposition**
  **-tocolor**
  **-toposition**
  
  **-to** "0.0 colorName"

    "position color"

    Specifies the stating point of gradient to generate. *Dir* can be
    any of the following:

    **left**
        The gradient starts at the left side   of the reference window and
	continues horizontally to the right.

    **right**
        The gradient starts at the right side of the reference window and
	continues horizontally to the left.

    **bottom**
        The gradient starts at the bottom of the reference window and
	continues vertically to the top.

    **top**
        The gradient starts at the top of the reference window and
	continues vertically to the bottom.

    **center**
        The gradient starts at the center of the reference window and
	continues vertically 

    *percent*
        The gradient starts at the position which is a percentage of the
	window size. For example "50" is the center of the window.
	
    The default is "".

  **-direction** *colorName*

    Specifies the direction of gradient to generate. *Dir* can be
    any of the following:

    *vertical*
        The gradient starts at the top of the reference window and
	continues vertically 

    *horizontal*
        The gradient starts at the top of the reference window and
	continues vertically 

    *diagonal*
        The gradient starts at the top of the reference window and
	continues vertically 

    *top left bottom right*
    *top right bottom left*
    *top center bottom center*
    *bottom center*
    *top center*
    *left center*
    *right center*

    The default is "".

  **-to** *colorName*

    Specifies the color of the high side of the interpolation.

  **-jitter** *percent*

    Specifies the amount of randomness to add to the gradient. *Percent*
    is to percentage.

  **-from** *colorName*

    Specifies the color of the low side of the interpolation.

  **-opacity** *percent*

    Specifies the percentage of opacity for of the background.  *Percent*
    must be is a number 0 and 100.  The default is "100".

  **-relativeto** *refName*

    Specifies a reference window to use of the origin the tile. *RefName*
    is the name of a Tk widget.  This is useful for creating seamless tiles
    with many widgets.  For example is a frame is *refName* then all the
    children packed in *refName* can use the same tile seamlessly.  If
    *refName* is "", then the origin is based on the widget using the tile.
    The default is "".

  **-scale** *scaleType*

    Specifies how to scale the gradient. *ScaleType* can be any of
    the following:

    *linear*
       Specifies the gradient is scaled linearly.

    *logarithmic*
       Specifies the gradient is scaled using logarithmic scale.

    *atan*
       Specifies the gradient is scaled using arctangent scale.

    The default is "linear".

  **-type** *gradType*

    Specifies the type of gradient to generate. *gradType* can be any of
    the following:

    *vertical*
        The gradient starts at the top of the reference window and
	continues vertically 

    *horizontal*
        The gradient starts at the top of the reference window and
	continues vertically 

    *radial*
         Two types of radial shapes "ellipse" and "circle"
	 
    *updiagonal*

    *downdiagonal*
    
    The default is "vertical".

  **-xorigin** *x*

    Specifies the x-coordinate of the origin of the gradient.  The gradient
    origin is the location from which to start the gradient.  For example,
    if *x* is "-10", then the gradient will be drawn offset 10 pixels to
    the left from the leftside the reference window.  The default is "0".

  **-yorigin** *y*

    Specifies the y-coordinate of the origin of the gradient.  The gradient
    origin is the location from which to start the gradient.  For example,
    if *y* is "-10", then the tiles will be drawn offset 10 pixels above
    the top the reference window.  The default is "0".

**blt::background create solid** ?\ *option* *value* ...\ ?

  Creates a new solid *background* object. The name of the *background* is
  automatically generated in the form "background0", "background1", etc.
  The name of the new *background* is returned. *Option* and *value* are
  specific to "solid" backgrounds and are listed below.

  **-background** *colorName*

    This is the same as the **-color** option below.

  **-color** *colorName*

    Specifies the color of the solid background.  *ColorName* can be any
    name accepted by **Tk_GetColor**.  The default is "grey85".

  **-opacity** *percent*

    Specifies the percentage of opacity for of the solid background.
    *Percent* must be is a number 0 and 100. 
    The default is "100".

**blt::background create texture** ?\ *option* *value* ...\ ?

  Creates a new textured *background* object. The name of the *background* is
  automatically generated in the form "background0", "background1", etc.
  The name of the new *background* is returned. *Option* and *value* are
  specific to "texture" backgrounds and are listed below.

  **-background** *colorName*

    Specifies the color of the tile background.  The color is used for the
    drawing 3-D borders and when there is no tiled image (see the
    **-image** option below).  *ColorName* can be any name accepted by
    **Tk_GetColor**.  The default is "grey85".

  **-to** *colorName*

    Specifies the color of the high side of the interpolation.

  **-direction** *colorName*

    Specifies the direction of texture to generate. *Dir* can be
    any of the following:

    *vertical*
        The texture starts at the top of the reference window and
	continues vertically 

    *horizontal*
        The texture starts at the top of the reference window and
	continues vertically 

    *diagonal*
        The texture starts at the top of the reference window and
	continues vertically 

    The default is "".

  **-jitter** *percent*

    Specifies the amount of randomness to add to the gradient. *Percent*
    is to percentage.

  **-from** *colorName*

    Specifies the color of the low side of the interpolation.

  **-opacity** *percent*

    Specifies the percentage of opacity for of the background.  *Percent*
    must be is a number 0 and 100.  The default is "100".

  **-palette** *paletteName*

    Specifies a color palette to use to draw the gradient.  *PaletteName*
    is the name of palette created by the **blt::palette** command.  If
    *paletteName" is "", then the color values from the **-from** and **-to**
    options are interpolated to draw the gradient.  The dafault is "".


  **-relativeto** *refName*

    Specifies a reference window to use of the origin the tile. *RefName*
    is the name of a Tk widget.  This is useful for creating seamless tiles
    with many widgets.  For example is a frame is *refName* then all the
    children packed in *refName* can use the same tile seamlessly.  If
    *refName* is "", then the origin is based on the widget using the tile.
    The default is "".

  **-type** *textureType*

    Specifies the type of texture to generate. *TextureType* can be any of
    the following:

    *stripes*
        The gradient starts at the top of the reference window and
	continues vertically 

    *checkers*
        The gradient starts at the top of the reference window and
	continues vertically 

    The default is "".

  **-xorigin** *x*

    Specifies the x-coordinate of the origin of the gradient.  The gradient
    origin is the location from which to start the gradient.  For example,
    if *x* is "-10", then the gradient will be drawn offset 10 pixels to
    the left from the leftside the reference window.  The default is "0".

  **-yorigin** *y*

    Specifies the y-coordinate of the origin of the gradient.  The gradient
    origin is the location from which to start the gradient.  For example,
    if *y* is "-10", then the tiles will be drawn offset 10 pixels above
    the top the reference window.  The default is "0".

**blt::background create tile** ?\ *option* *value* ...\ ?

  Creates a new tiled *background* object. The name of the *background* is
  automatically generated in the form "background0", "background1", etc.
  The name of the new *background* is returned. *Option* and *value* are
  specific to "tile" backgrounds and are listed below.

  **-color** *colorName*

    Specifies the color of the tile background.  The color is used for the
    drawing 3-D borders and when there is no tiled image (see the
    **-image** option below).  *ColorName* can be any name accepted by
    **Tk_GetColor**.  The default is "grey85".

  **-filter** *filterName*

    Specifies the resampling filter to use when scaling the tile pattern.
    See the **-scale** option below.  The default is "".

  **-image** *imageName*

    Specifies the Tk image to use as a tile.  *ImageName* can be a "photo*
    or "picture" image.  If *imageName* is "", then no tiled background is
    drawn. The background will be solid. The default is "".

  **-opacity** *percent*

    Specifies the percentage of opacity for of the background.  *Percent*
    must be is a number 0 and 100.  The default is "100".

  **-relativeto** *refName*

    Specifies a reference window to use of the origin the tile. *RefName*
    is the name of a Tk widget.  This is useful for creating seamless tiles
    with many widgets.  For example is a frame is *refName* then all the
    children packed in *refName* can use the same tile seamlessly.  If
    *refName* is "", then the origin is based on the widget using the tile.
    The default is "".

  **-scale** *bool*

    Indicates to scale the tile pattern when the reference window is resized
    instead of creating more tiles. The default is "0".

  **-xorigin** *x*

    Specifies the x-coordinate of the origin of the tile.  The tile origin
    is the tile location from which to all other tiles.  For example, if
    *x* is "-10", then the tiles will be drawn offset 10 pixels to the left
    from the leftside the reference window.  The default is "0".

  **-yorigin** *y*

    Specifies the y-coordinate of the origin of the tile.  The tile origin
    is the tile location from which to all other tiles.  For example, if
    *y* is "-10", then the tiles will be drawn offset 10 pixels above the
    top the reference window.  The default is "0".
    
**blt::background active**  ?\ *pattern* ...\ ?

  Returns the pathnames of all widgets that are currently background (active).
  If a *pattern* is given, the path names of background widgets matching
  *pattern* are returned.  This differs from the **names** operation in that
  **active** only returns the names of windows where the background window is
  currently active (**names** returns all background windows).

**blt::background delete** *bgName* ?\ *bgName* ...\ ?

  Releases resources allocated by the background command for *window*, including
  the background window.  User events will again be received again by *window*.
  Resources are also released when *window* is destroyed. *Window* must be
  the name of a widget specified in the **create** operation, otherwise an
  error is reported.

**blt::background names** ?\ *pattern* ...\ ?

  Returns the pathnames of all widgets that have previously been made background
  (i.e. a background window is allocated and associated with the widget).  It
  makes no difference if the window is currently background or not.  If a
  *pattern* is given, the path names of background widgets matching *pattern* are
  returned.

**blt::background type** *bgName*

  Returns the type of the background for *bgName*.

KEYWORDS
--------
background, keyboard events, pointer events, window, cursor


