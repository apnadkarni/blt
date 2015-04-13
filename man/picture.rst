
===============
picture
===============

----------------------------------------------------------------
Full color image type
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

**image create picture** ?\ *imageName*\ ? ?\ *options* ... ? 

DESCRIPTION
-----------

The **picture** is an image type for Tk. It is for full color images
(32-bit pixels) with or without transparency.  Each color component in a
picture is eight bits and there is an 8-bit alpha channel.  Image data for
a picture image can be obtained from a file or a string, or it can be
supplied from C code through a procedural interface.  Many image formats
are supported (JPG, GIF, TGA, BMP, TIF, ICO, PDF, PS, etc) as well as a
number of operations that can be performed on the image such as resizing
(through resampling).

SYNTAX
------

**image create picture** ?\ *imageName*\ ? ?\ *options* ... ? 

Creates a new *picture* image.  The name of the new *picture* is returned.
If no *imageName* argument is present, then the name of the picture is
automatically generated in the form ""image0"", ""image1"", etc.

A new TCL command (by the same name as the picture) is also created.
Another TCL command or picture object can not already exist as *imageName*.
If the TCL command is deleted, the picture will also be freed.  Additional
options may be specified on the command line or in the option database to
configure aspects of the widget such as its colors and font.  See the
**configure** operation below for the exact details about what *option* and
*value* pairs are valid.

COLOR SPECIFICATIONS
--------------------

Pixel color values can be described in any of the following forms:

  *colorName*		
    Any of the valid textual names for a color defined in the 
    server's color database file, such as red or "PeachPuff".

  #\ *RGB*  #\ *RRGGBB* #\ *RRRGGGBBB*  #\ *RRRRGGGGBBBB*		

    A numeric specification of the red, green, and blue intensities to use
    to display the color. Each R, G, or B represents a single hexadecimal
    digit.  The four forms permit colors to be specified with 4-bit, 8-bit,
    12-bit or 16-bit values.  When fewer than 16 bits are provided for each
    color, they represent the most significant bits of the color.  For
    example, #3a7 is the same as #3000a0007000.

  0x\ *AARRGGBB*		

    A numeric specification of the alpha, red, green, and blue intensities
    to use to display the color. Each A, R, G, or B represents a single
    hexadecimal digit. The alpha value represents the transparency of the
    pixel: "FF" is fully opaque, "00" is fully transparent.

PICTURE OPERATIONS
------------------

Once you create a picture image, you can use its associated TCL command to
query or modify it.  The general form is

  *imageName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for pictures are listed below.

*imageName* **add** *pictOrColor*

  Performs an arithmetic-add with the picture or color and *imageName*.
  *PictOrColor* is either the name of a *picture* image or a color
  specification. The resulting image is saved in *imageName*.
  
*imageName* **and** *pictOrColor* 

  Performs a bitwise-and of the picture or color and *imageName*.
  *PictOrColor* is either the name of a *picture* image or a color
  specification.  The resulting image is saved in *imageName*.
  
*imageName* **blank** ?\ *colorSpec*\ ?

  Blanks the image. By default, the entire image is set to be transparent
  and the background of whatever window it is displayed in will show
  through.  If a *colorSpec* argument is given then the image will be
  filled with that color.

*imageName* **blend** *bgName* *fgName*  ?\ *switches* ... ?

*imageName* **blur** *srcName* *width* 

*imageName* **cget** *option* 

  Returns the current value of the configuration option given by
  *option*.  *Option* may have any of the values accepted by the
  **configure** operation.

*imageName* **colorblend** *bgImage* *fgImage* ?\ *switches* ... ?

  Blends *fgImage* into the *bgImage* using one the **Photoshop** blending
  modes. *BgImage* and *fgImage* must be *picture* images, and one of them
  may be the same as *imageName*.  The following switches are available.

  **-mode** *mode*

    Specifies the blend mode to use. The default is "normal".  *Mode* can
    be one of the following:

    **colorburn**

      Selects the color component and darkens the *bgImage* color to reflect
      the *fgImage* color by increasing the contrast between the two. 
    
    **colordodge**

      Selects the color component and brightens the *bgImage* color to
      reflect the *fgColor* color by decreasing contrast. Blending with
      black produces no change.

    **darken**

      Selects the darker color components between *bgImage* and *fgImage*.
      Pixels lighter than the *fgImage* color are replaced, and pixels
      darker than the *fgImage* color do not change.
    
    **difference**

      Subtracts the color components; either the *fgImage* color from the
      *bgImage* color or the *bgImage* color from the *fgImage* color,
      depending on which has the greater brightness value. Blending with
      white inverts the *bgImage* color values; blending with black
      produces no change.

    **exclusion**

      Creates an effect similar to but lower in contrast than the
      **difference** mode. Blending with white inverts the base color
      values. Blending with black produces no change.

    **hardlight**

      Multiplies or screens the colors, depending on the *fgImage*
      color. The effect is similar to shining a harsh spotlight on the
      image. If the *fgImage* color (light source) is lighter than 50%
      gray, the image is lightened, as if it were screened. This is useful
      for adding highlights to an image. If the *bgImage* color is darker
      than 50% gray, the image is darkened, as if it were multiplied. This
      is useful for adding shadows to an image. Painting with pure black or
      white results in pure black or white.

    **hardmix**

      Adds the color components of the *fgImage* color to the RGB values of
      the *bgImage* color. If the resulting sum for a channel is 255 or
      greater, it receives a value of 255; if less than 255, a value
      of 0. Therefore, all blended pixels have red, green, and blue channel
      values of either 0 or 255. This changes all pixels to primary
      additive colors (red, green, or blue), white, or black.

    **lighten**

      Selects the lighter color components between *bgImage* and *fgImage*.
      Pixels darker than the *fgImage* color are replaced, and pixels
      lighter than the *fgImage* color do not change.
    
    **linearburn**

      Darkens the *bgImage* color to reflect the *fgImage* color by decreasing
      the brightness.  Blending with white produces no change.
    
    **lineardodge**

      Lightens the *bgImage* color to reflect the *fgImage* color by
      increasing the brightness.  Blending with black produces no change.

    **linearlight**

      Burns or dodges the colors by decreasing or increasing the
      brightness, depending on the *fgImage* color. If the *fgImage* color
      (light source) is lighter than 50% gray, the image is lightened by
      increasing the brightness. If the *fgImage* color is darker than 50%
      gray, the image is darkened by decreasing the brightness.

    **normal**

    **multiply**

      Multiplies each color component in *bgImage* with *fgImage*. The
      resulting color is always a darker color. Pixels lighter than the
      *fgImage* color are replaced, and pixels darker than the *fgImage*
      color do not change.
    
    **screen**

      Multiplies the inverse of each color component of *bgImage* and
      *fgImage*.  The resulting color is always a lighter color. Screening
      with black leaves the color unchanged. Screening with white produces
      white.  The effect is similar to projecting multiple photographic
      slides on top of each other.

    **softlight**

      Darkens or lightens the colors, depending on the *fgImage* color. The
      effect is similar to shining a diffused spotlight on the image. If
      the *fgImage* color (light source) is lighter than 50% gray, the
      image is lightened as if it were dodged. If the *fgImage* color is
      darker than 50% gray, the image is darkened as if it were burned
      in. Painting with pure black or white produces a distinctly darker or
      lighter area, but does not result in pure black or white.
    
    **subtract**

      Subtracts the *fgImage* color from the *bgImage* color.  Any
      resulting negative values are clipped to zero.
    
    **overlay**

      Multiplies or screens the colors, depending on the *bgImage*
      color. Patterns or colors overlay the existing pixels while
      preserving the highlights and shadows of the *bgImage* color. The
      *bgImage* color is not replaced, but mixed with the *fgImage* color
      to reflect the lightness or darkness of the original color.

    **pinlight**

      Replaces the colors, depending on the *fgImage* color. If the
      *fgImage* color (light source) is lighter than 50% gray, pixels
      darker than the *fgImage* color are replaced, and pixels lighter than
      the *fgImage* color do not change. If the *fgImage* color is darker
      than 50% gray, pixels lighter than the *fgImage* color are replaced,
      and pixels darker than the *fgImage* color do not change. This is
      useful for adding special effects to an image.

    **vividlight**

      Burns or dodges the colors by increasing or decreasing the contrast,
      depending on the *fgImage* color. If the *fgImage* color (light
      source) is lighter than 50% gray, the image is lightened by
      decreasing the contrast. If the *fgImage* color is darker than 50%
      gray, the image is darkened by increasing the contrast.

  **-from** *bbox*

    Specifies the region in the *srcName* image to be copied. *Bbox* is
    a list in the form "*x1* *y1* *x2* *y2*" or "*x1*
    *y1*".  The first describes the subregion to be copied.  The second
    says to copy the subimage starting at *x1*,*y1* of the
    foreground image and copying region extending to the lower right corner
    of *fgImage*.

  **-to** *bbox*

    Specifies the region in the *bgImage* image to be
    blended. *Bbox* is a list in the form "*x1* *y1* *x2*
    *y2*" or "*x1* *y1*".  The first describes the subregion to
    be blended.  The second says to copy the subimage starting at
    *x1*,*y1* of the background image and copying region extending
    to the lower right corner of *bgImage*.

*imageName* **configure** ?\ *option* *value* ... ?

  Query or modify the configuration options for the image.  If no
  *option* is specified, returns a list describing all of the available
  options for *imageName* (see **Tk_ConfigureInfo** for information
  on the format of this list).  If *option* is specified with no
  *value*, then the command returns a list describing the one named
  option (this list will be identical to the corresponding sublist of the
  value returned if no *option* is specified).  If one or more
  *option-value* pairs are specified, then the command modifies the
  given option(s) to have the given value(s); in this case the command
  returns an empty string.  The valid option-value pairs are described
  below.

  **-autoscale** *bool*

    When the dimensions of the image change, automatically resize the
    image to match the new dimensions.  The **-filter** and **-maxpect**
    also control how the image is resized.

  **-data** *string*

    Specifies the contents of the image as a string.  The string should
    contain binary data or base64-encoded data.  The format of the string
    must be one of those for which there is an image file format handler
    that will accept string data.  It is an error if both the **-data** and
    **-file** options are specified.

  **-dither** *bool*

    Indicates to dither the picture.  Dithering scatters different colored
    pixels in an image to make it appear as though there are intermediate
    colors in images with a limited color palette. Dithering propagates
    quantization errors from one pixel to its neighbors.

    Reference: Victor Ostromoukhov, "A Simple and Efficient Error-Diffusion
    Algorithm" in SIGGRAPH'01.

  **-file** *fileName*

    *FileName* gives the name of a file that is to be read to supply
    data for the picture image.  The file format must be one of those for
    which there is an image file format handler that can read data.

  **-filter** *filterName*

    Specifies the use *filterName* when resizing the image.  This option
    matters only when **-autoscale** is on. The valid filter names are
    "bell", "box", "bspline", "catrom", "default", "dummy", "gauss8",
    "gaussian", "gi8", "lanczos3", "mitchell", "none", "sinc", "sinc8",
    "sinc12", "tent", or "triangle".

  **-gamma** *number*

    Specifies that the colors allocated for displaying this image in a
    window should be corrected for a non-linear display with the specified
    gamma exponent value.  (The intensity produced by most CRT displays is
    a power function of the input value, to a good approximation; gamma is
    the exponent and is typically around 2).  The value specified must be
    greater than zero.  The default value is one (no correction).  In
    general, values greater than one will make the image lighter, and
    values less than one will make it darker.

  **-height** *pixels*

    Specifies the height of the image, in pixels.  This option is useful
    primarily in situations where the user wishes to build up the contents
    of the image piece by piece.  A value of zero (the default) allows the
    image to expand or shrink vertically to fit the data stored in it.

  **-maxpect** *bool*

    When resizing the image, maintain the aspect ratio of the original picture.

  **-rotate** *angle*

    Rotate the image by *angle*. *Angle* is the number of degrees
    to rotate the image.

  **-sharpen** *bool*

    Automatically sharpen the image.

  **-width** *pixels*

    Specifies the width of the image, in pixels.  This option is useful
    primarily in situations where the user wishes to build up the contents
    of the image piece by piece.  A value of zero (the default) allows the
    image to expand or shrink horizontally to fit the data stored in it.

  **-window** *windowName*

    Specifies a window of a file that is to be read to supply data for the
    picture image.  The format *windowName* is either a Tk window name
    or a hexadecimal number (e.g. "0x000000002100") if the window is
    an external window.  It is an error if *windowName* is obscured.
    You should raise it beforehand.

*imageName* **convolve** *srcName* ?\ *switches* ... ?

*imageName* **copy** *srcName* ?\ *switches* ... ?

  Copies a region from the image called *srcName* (which must be a picture
  image) to the image called *imageName*.  If no options are specified,
  this command copies the whole of *srcName* into *imageName*, starting at
  coordinates (0,0) in *imageName*.  The following options may be
  specified:

  **-blend** *bool*

    The contents of the *srcName* are blended with the background or
    *imageName*.  The is only useful when *srcName* contains
    transparent pixels.

  **-from** *bbox*

    Specifies the region in the *srcName* image to be copied. *Bbox* is a
    list in the form "*x1* *y1* *x2* *y2*" or "*x1* *y1*".  The first form
    describes the subregion to be copied.  The second indicates to copy the
    subimage starting at *x1*,*y1* of the source image and copying region
    extending to the lower right corner of *srcName*.

  **-to** *bbox*

    Specifies the region in the *imageName* image to be copied. *Bbox* is a
    list in the form "*x1* *y1* *x2* *y2*" or "*x1* *y1*".  The first form
    describes the subregion to be copied.  The second indicates to copy the
    subimage starting at *x1*,*y1* of the destination image and copying
    region extending to the lower right corner of *imageName*.

*imageName* **convolve** *srcName* ?\ *switches* ... ?

*imageName* **copy** *srcName* ?\ *switches* ... ?

*imageName* **crop** *x1* *y1* ?\ *x2 *y2*\ ?

  Crops *imageName* to specified rectangular region.  The region is defined
  by the coordinates *x1*, *y1*, *x2*, *y2* (where *x1, *y1* and *x2*, *y2*
  describe opposite corners of a rectangle) is cut out and saved in
  *imageName*. If no *x2* and *y2* coordinates are specified, then the
  region is from the point *x1*, *y1* to the lower right corner of
  *imageName*. *ImageName* will be resized to the new size.  All the
  coordinates are clamped to reside within the image.  For example if *x2*
  is "10000" and the image width is 50, the value will be clamped to 49.

*imageName* **crossfade** *from* *to* ?\ *switches* ... ?

   Cross fades *to* into *from*, saving the result in *imageName*. *From*
   and *to* can be either the name of a picture (it can not be *imageName*)
   or a color specification.  For example if *to* is "black", this image
   will fade to black.  *From* and *to* cannot both be colors. *ImageName*
   will first be a copy of *from*.  It is progressively changed by fading
   the *from* and adding *to* until *imageName* is a copy of *to*.

   If **-delay** is greater than zero, the transition automatically starts
   after this command completes at an idle point. Care must be taken not to
   the change *imageName* while the transition is occurring. The results
   will be unexpected. You can specify a TCL variable that is automatically
   set when the transition has completed. See the **-variable** switch.
   The rate of transition is determined by both the **-delay** and
   **-steps** switches.  *Switches* may be any of the following.

   **-goto** *step*
     Specifies the current step of the transition.  The default is 1.

   **-delay** *milliseconds*
     Specifies the delay between steps in the transition in milliseconds.
     If *milliseconds* is 0, then no automatic changes will occur.
     The default is "0".

   **-steps** *numSteps*
     Specifies how may steps the transition should take.  The default is
     "10".

   **-variable** *varName*
      Specifies the name of a TCL variable that will be set when the
      transition has completed.

*imageName* **dissolve** *from* *to* ?\ *switches* ... ?

   Transitions from *from* to *to* using by dissolving *to* into *from* and
   saving the result in *imageName*. *From* and *to* can be either the name
   of a picture (it can not be *imageName*) or a color specification.
   *From* and *to* cannot both be colors. *ImageName* starts as a copy of
   *from*.  It is progressively changed by randomly copying pixels from
   *to* into it. 

   Reference: "A Digital "Dissolve" Effect" by Mike Morton in "Graphics
   Gems V", pp. 221-232, Academic Press, 1994.


   This transition will start after this command completes, when an idle
   point is reached. Care must be taken not to change *imageName* while the
   transition is occurring. The results may be unexpected. You can specify a
   TCL variable that will be automatically set when the transition has
   completed. See the **-variable** switch.  The rate of transition is
   determined by both the **-delay** and **-steps** switches.
   *Switches* may be any of the following.

   **-delay** *milliseconds*
     Specifies the delay between steps in the transition in milliseconds.
     The default is "0". 

   **-steps** *numSteps*
     Specifies how may steps the transition should take.  The default is
     "10".
     
   **-variable** *varName*
      Specifies the name of a TCL variable that will be set when the
      transition has completed.

*imageName* **draw** ?\ *args* ... ?

*imageName* **dup** ?\ *switches* ... ?

  The following switches are available.

  **-region** *bbox*

    Specifies a subregion in the picture to be copied. *Bbox* is a list in
    the form "*x1* *y1* *x2* *y2*". 

*imageName* **emboss** *srcName*

  Embosses *srcName* and saves the result in *imageName*.  *SrcName* is the
  name of picture image, but can't the same as *imageName*.  The image
  is embossed by shading the RGB pixels using a single distant light source.

  Reference: "Fast Embossing Effects on Raster Image Data" by John Schlag,
  in "Graphics Gems IV", Academic Press, 1994.
  
*imageName* **export** 

  Reports the currently available image formats.

*imageName* **export** *format* ?\ *switches* ... ?

  Exports the image in the specified format. *Format* can be any
  registered format. *Switches* are optional flags, specific the
  format, that can be used. See the section **EXPORT FORMATS** below that
  describes the switches available for each format.

*imageName* **fade** *srcName* *factor*

*imageName* **flip x**

  Flips the image horizontally and saves the result in *imageName*.

*imageName* **flip y**

  Flips the image vertically and saves the result in *imageName*.

*imageName* **gamma** *value* 

*imageName* **get** *x* *y* 

  Returns the pixel value at the designated image coordinates. Both *x* and
  *y* must reside within the image.  The upper left corner of the image is
  0,0.  The lower right corner is width-1, height-1.
  
*imageName* **greyscale** *srcName*

  Converts *srcName* to greyscale and saves the result in *imageName*
  *SrcName* is the name of picture image. It can be the same as
  *imageName*.

  Luminosity is computed using the formula

    Y = 0.212671 * R + 0.715160 * G + 0.072169 * B

  where Y is the luminosity and R, G, and B are color components.
  
*imageName* **height** ?\ *numPixels*\ ?

  Gets or sets the height of the picture.  If no *numPixels* argument is
  present, the height of the picture in pixels is returned.  *NumPixels* is
  a non-zero, positive integer specifying the new height of the image.

*imageName* **import** *format* ?\ *switches* ... ?

*imageName* **info** 

*imageName* **list** ?\ *args* ... ?

*imageName* **list animate** ?\ *args* ... ?

*imageName* **list append** ?\ *image* ... ?

*imageName* **list current** ?\ *index*\ ?

*imageName* **list delete** *firstIndex* ?\ *lastIndex*\ ?

*imageName* **list length** 

*imageName* **list next** 

*imageName* **list previous** 

*imageName* **list replace** *firstIndex*  *lastIndex* ?\ *image* ... ?

*imageName* **max** *pictOrColor*

  Computes the maximum of the picture or color and *imageName*.  The
  maximum of each color component is computed.  *PictOrColor* is either the
  name of a *picture* image or a color specification.  The resulting image
  is saved in *imageName*.

*imageName* **min** *pictOrColor*

  Computes the minimum of the picture or color and *imageName*.  The
  minimum of each color component is computed.  *PictOrColor* is either the
  name of a *picture* image or a color specification.  The resulting image
  is saved in *imageName*.

*imageName* **multiply** *number*

  Performs an arithmetic-multiplication of the picture or color and
  *imageName*.  Each color component is multiplied. *PictOrColor* is either
  the name of a *picture* image or a color specification.  The resulting
  image is saved in *imageName*.

*imageName* **nand** *pictOrColor*

  Performs a bitwise-nand with the picture or color and *imageName*.  Each
  color component is and-ed and negated.  *PictOrColor* is either the name
  of a *picture* image or a color specification.  The resulting image is
  saved in *imageName*.

*imageName* **nor** *pictOrColor*

  Performs a bitwise-nor with the picture or color and *imageName*.  Each
  color component is or-ed and negated.  *PictOrColor* is either the name
  of a *picture* image or a color specification.  The resulting image is
  saved in *imageName*.

*imageName* **or** *pictOrColor*

  Performs a bitwise-or with the picture or color and *imageName*.  Each
  color component is or-ed.  *PictOrColor* is either the name of a
  *picture* image or a color specification.  The resulting image is saved
  in *imageName*.

*imageName* **project** *srcName* *coords* *coords* ?\ *switches* ... ?

*imageName* **put** *x* *y* *colorSpec* 

  Sets the named color at the specified coordinates in *imageName*.  Both
  *x* and *y* must reside within the image.  The upper left corner of the
  image is 0,0.  The lower right corner is width-1, height-1.  *ColorSpec*
  is a color specification that can be in any of the forms described
  above in the section PICTURE COLOR VALUES.
  
*imageName* **quantize** *srcName* *numColors*

  Reduces the number of colors in *srcName* to be less than or
  equal to *numColors*. The resulting image is saved in *imageName*.
  *NumColors* must be a positive number greater than 1.
   
  Reference: "Efficient Statistical Computations for Optimal Color
  Quantization"by Wu, Xiaolin in "Graphics Gems II", p. 126-133, Academic
  Press, 1995.
   
*imageName* **reflect** *srcName* ?\ *switches* ... ?

  Reflects *srcName* with the resulting image saved in *imageName*.
  *SrcName* is the name of another image created by the **image create
  picture** command.    *Switches* may be any of the following.

  **-background** *colorSpec**

  **-blur** *blurLevel*
  
  **-colorscale** *scale*

    Specifies the scale when interpolating values. *Scale* can be "linear",
    or "logarithmic"".

    **linear**
	Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
	Colors are interpolated using the log of the value.
    
  **-low** *opacity*

  **-high** *opacity*

  **-jitter** *percent*

    Specifies the amount of randomness to add to the interpolated colors.
    *Percent* is a real number between 0 and 100.  It is the percentage
    that colors may vary.
  
  **-ratio** *number*
  
  **-side** *side*

    Specifies the side of *srcName* to be reflected.  Side can be "bottom",
    "top".  "Left" and "right" are not implemented yet.


*imageName* **resample** *srcName* ?\ *switches* ... ?

  Resizes *srcName* with the resulting image saved in *imageName*.
  *SrcName* is the name of another image created by the **image create
  picture** command.  Resizing is done by filtered resampling the source
  picture. Filters have a time/quality trade-off. The fastest filters give
  the poorest results.  The best quality filters are slower.

  Reference: "Fundamentals of Texture Mapping and Image Warping" by
  Paul S. Heckbert, M.Sc. Thesis, Department of Electrical Engineering and
  Computer Science, University of California, Berkeley, June, 1989.

  Reference: “General Filtered Image Rescaling” by Dale Schumacher,
  Graphics Gems III, pp. 8–16, Academic Press, 1992.
  
  *Switches* may be any of the following.
  
  **-filter** *filterName*

    Specifies the image filter to use for both the horizontal and
    vertical resampling.  *FilterName* can be any one of the following.

    **bell**
      BellFilter (support is 1.5).

    **bessel**
      BesselFilter (support is 3.2383)

    **box**
      This filter sums up all the samples in the filter area with an equal
      weight. Box is the fastest filtering method. 

    **bspline**
      BSplineFilter,		     2.0	 },

    **catrom**
      CatRomFilter,		     2.0	 },

    **default**
      DefaultFilter,		     1.0	 },

    **dummy**
      DummyFilter,		     0.5	 },

    **gauss8**
      GaussianFilter,	     8.0	 },

    **gaussian**
      GaussianFilter,	     1.25	 },

    **gi**
      GiFilter,		     4.0	 },

    **gi8**
      GiFilter,		     8.0	 },

    **lanczos3**
      Lanczos3Filter,	     3.0	 },

    **mitchell**
      MitchellFilter,	     2.0	 },

    **none**
     NULL, 0.0	 },

    **sinc**
      SincFilter,		     4.0	 },

    **sinc8**
      SincFilter,		     8.0	 },

    **sinc12**
      SincFilter,		     12.0	 }, 

    **tent**
      TriangleFilter,	     1.0	 },

    **triangle**
      TriangleFilter,	     1.0	 },

  **-from** *bbox*

    Specifies a region in the *srcName* to be resampled.  By default
    the all of *srcName* is resampled.

  **-height** *numPixels*

    Specifies the height of the resampled image.  *NumPixels* may have any
    of the forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".
    If *numPixels* is "0", then the height of *imageName* will not change.
    
  **-hfilter** *filterName*

    Specifies the image filter to use for horizontal resampling. 
    *FilterName* can be any of the filter described in **-filter**
    switch.
     
  **-maxpect** 

    Forces the *imageName* to retain the aspect ratio as *srcName*.
    The maximum of **-width** and **-height** is used.

  **-vfilter** *filterName*

    Specifies the image filter to use for vertical resampling.
    *FilterName* can be any of the filter described in **-filter** switch.


  **-width** *numPixels*

    Specifies the width of the resampled image.  *NumPixels* may have any
    of the forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".
    If *numPixels* is "0", then the width of *imageName* will not change.


*imageName* **rotate** *srcName* *angle*

*imageName* **select** *srcName* *color* ?*color*?

*imageName* **sharpen** 

*imageName* **snap** *window* ?\ *switches* ... ?

   Takes a snapshot of the *window* and saves the result in *imageName*.
   *Window* is the name of a window that is fully visible on the screen.
   It cannot be obscured by other window. *Window* can be one of the
   following.

      **.**\ *pathName*
	 The path of any Tk widget. Note that Tk **canvas** widgets are
	 treated specially.  The **canvas** window does not have to be viewable
	 on the screen to be snapped. It underlying pixmap is read directly.

      **root**
	 The root window.

      *number*
	 The ID of the window.  In X11 the number will be a hexadecimal number
	 such as "0x2e00004".

   *Switches* can be any of the following.

   **-bbox** *bbox*
     Specifies the time between steps in the transition. The default is
     "50". 

   **-raise** 
     Indicates to raise the window before snapping. The is sometimes
     required for non-Tk windows.  The default is not to raise *window*.
     
*imageName* **subtract** *pictOrColor*

  Performs an arithmetic-subtraction of the picture or color from Each color
  component is subtracted.  *imageName*.  *PictOrColor* is either the name
  of a *picture* image or a color specification.  The resulting image is
  saved in *imageName*.

*imageName* **transp** *bgcolor* 

*imageName* **width** *pixels* 

  Gets or sets the width of the picture.  If no *numPixels* argument is
  present, the width of the picture in pixels is returned.  *NumPixels* is
  a non-zero, positive integer specifying the new width of the image.

*imageName* **wipe** *from* *to* ?\ *switches* ... ?

   Transitions from *from* to *to* using by wiping. *To* is *to* into *from* and
   saving the result in *imageName*. *From* and *to* can be either the name
   of a picture (it can not be *imageName*) or a color specification.
   *From* and *to* cannot both be colors. *ImageName* starts as a copy of
   *from*.  It is progressively changed by randomly copying pixels from
   *to* into it. 

   This transition will start after this command completes, when an idle
   point is reached. Care must be taken not to change *imageName* while the
   transition is occurring. The results may be unexpected. You can specify a
   TCL variable that will be automatically set when the transition has
   completed. See the **-variable** switch.  The rate of transition is
   determined by both the **-interval** and **-steps** switches.
   *Switches* may be any of the following.

   **-interval** *milliseconds*
     Specifies the time between steps in the transition. The default is
     "50". 

   **-steps** *numSteps*
     Specifies how may steps the transition should take.  The default is
     "10".
     
   **-variable** *varName*
      Specifies the name of a TCL variable that will be set when the
      transition has completed.


*imageName* **xor** *pictOrColor* ?\ *switches* ... ?

  Performs a bitwise-xor with the picture or color and *imageName*.  Each
  color component is xor-ed.  *PictOrColor* is either the name of a
  *picture* image or a color specification.  The resulting image is saved
  in *imageName*. *Switches* can be one of the following.

  **-invert** *boolean*

     Indicates to invert the result.

  **-mask** *mask*

    *Mask* is either the name of a *picture* image or a color specification.

KEYWORDS
--------

picture, image
