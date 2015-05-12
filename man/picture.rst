
=======
picture
=======

----------------------
Full color image type.
----------------------

:Author: George A Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

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
are supported (JPEG, GIF, TGA, BMP, TIFF, ICO, PDF, PS, etc.) as well as a
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
  Performs an arithmetic-add of each color component of *imageName* with
  the picture or color.  The resulting image is saved in *imageName*.
  *PictOrColor* is either the name of a *picture* image or a color
  specification.  Color components are clamped to 255.
       
  **-invert** *boolean*
     Indicates to invert mask.  This only has affect is the **-mask** switch
     is set.

  **-mask** *mask*
    Only change the non-zero pixels of *mask* in *imageName*.
    *Mask* is either the name of a *picture* image. 

*imageName* **and** *pictOrColor* 
  Performs a bitwise-and of each color component of *imageName* with the
  picture or color. *PictOrColor* is either the name of a *picture* image
  or a color specification.  The resulting image is saved in *imageName*.
  
  **-invert** *boolean*
     Indicates to invert mask.  This only has affect is the **-mask** switch
     is set.

  **-mask** *mask*
    Only change the non-zero pixels of *mask* in *imageName*.
    *Mask* is either the name of a *picture* image. 

*imageName* **blank** ?\ *colorSpec*\ ?
  Blanks the image. By default, the entire image is set to be transparent
  and the background of whatever window it is displayed in will show
  through.  If a *colorSpec* argument is given then the image will be
  filled with that color.  *ColorSpec* is a color name or the name
  of a paintbrush created by the **blt::paintbrush** command.

*imageName* **blend** *bgName* *fgName*  ?\ *switches* ... ?
  Blends or composites *fgImage* over *bgImage* using one the alpha
  channels of both images.  If *fgImage* is opaque, then this is same as
  copying *fgName*.  If *fgName* and *bgName* are different sizes, only the
  overlapping pixels are blended. The resulting image is saved in
  *imageName*.  *BgImage* and *fgImage* must be *picture* images, and one
  of them may be the same as *imageName*.  The following switches are
  available.

  **-from** *bbox*
    Specifies the region in the *bgName* image to be blended. *Bbox* is a
    list in the form "*x1* *y1* *x2* *y2*" or "*x1* *y1*".  The first form
    describes the subregion to be blended.  The second indicates to copy
    the subimage starting at *x1*,\ *y1* of *bgName* extending
    to the lower right corner.

  **-to** *bbox*
    Specifies the region in the *fgName* image to be blended. *Bbox* is a
    list in the form "*x1* *y1* *x2* *y2*" or "*x1* *y1*".  The first form
    describes the subregion to be blended.  The second indicates to copy
    the subimage starting at *x1*,\ *y1* of *fgName* and extending to the
    lower right corner.

*imageName* **blur** *srcName* *numPixels* 
  Blurs *srcName* performing a boxcar blur using *numPixels* as the radius
  of the blur.  The resulting image is saved in *imageName*.  *SrcName* is
  the name of a *picture* image and can be the same as *imageName*.
  *NumPixels* is a positive number indicating the width of the blur.

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
      Copies *fgImage* into *bgImage*.  The resulting color is always
      the same as *fgImage*.
      
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
    says to copy the subimage starting at *x1*,\ *y1* of the
    foreground image and copying region extending to the lower right corner
    of *fgImage*.

  **-to** *bbox*
    Specifies the region in the *bgImage* image to be
    blended. *Bbox* is a list in the form "*x1* *y1* *x2*
    *y2*" or "*x1* *y1*".  The first describes the subregion to
    be blended.  The second says to copy the subimage starting at
    *x1*,\ *y1* of the background image and copying region extending
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
    specified in the **resample** operation below.

  **-gamma** *number*
    Specifies that the colors allocated for displaying this image in a
    window should be corrected for a non-linear display with the specified
    gamma exponent value.  (The intensity produced by most CRT displays is
    a power function of the input value, to a good approximation; gamma is
    the exponent and is typically around 2).  The value specified must be
    greater than zero.  The default value is one (no correction).  In
    general, values greater than one will make the image lighter, and
    values less than one will make it darker.

  **-height** *numPixels*
    Specifies the height of the image, in pixels.  *NumPixels* may have any
    of the forms acceptable to **Tk_GetPixels**, such as "200" or
    "2.4i". This option let you specify an image a particular size and then
    draw into it. If *numPixels* is 0, the image will expand or shrink
    vertically to fit the data stored in it.  The default is 0.

  **-maxpect** *bool*
    When resizing the image, maintain the aspect ratio of the original picture.

  **-rotate** *angle*
    Rotates the image by *angle*. *Angle* is the number of degrees
    to rotate the image.

  **-sharpen** *bool*
    Automatically sharpens the image.

  **-width** *numPixels*
    Specifies the width of the image in pixels.  *NumPixels* may have any
    of the forms acceptable to **Tk_GetPixels**, such as "200" or
    "2.4i". This option let you specify an image of a particular size and
    then draw into it. If *numPixels* is 0, the image will expand or shrink
    horizontally to fit the data stored in it.  The default is 0.

  **-window** *windowName*
    Specifies a window of a file that is to be read to supply data for the
    picture image.  The format *windowName* is either a Tk window name
    or a hexadecimal number (e.g. "0x000000002100") if the window is
    an external window.  It is an error if *windowName* is obscured.
    You should raise it beforehand.

*imageName* **copy** *srcName* ?\ *switches* ... ?
  Copies a region from the image called *srcName* (which must be a picture
  image) to the image called *imageName*.  If no options are specified,
  this command copies the whole of *srcName* into *imageName*, starting at
  coordinates (0,0) in *imageName*.  *ImageName* is not resized.  If *srcName*
  is bigger than *imageName* then only the pixels that overlap are copied.
  *Switches* may be any of the following.

  **-blend** *bool*
    The contents of the *srcName* are blended with the background or
    *imageName*.  The is only useful when *srcName* contains
    transparent pixels.

  **-from** *bbox*
    Specifies the region in the *srcName* image to be copied. *Bbox* is a
    list in the form "*x1* *y1* *x2* *y2*" or "*x1* *y1*".  The first form
    describes the subregion to be copied.  The second indicates to copy the
    subimage starting at *x1*,\ *y1* of the source image and copying region
    extending to the lower right corner of *srcName*.

  **-to** *bbox*
    Specifies the region in the *imageName* image to be copied. *Bbox* is a
    list in the form "*x1* *y1* *x2* *y2*" or "*x1* *y1*".  The first form
    describes the subregion to be copied.  The second indicates to copy the
    subimage starting at *x1*,\ *y1* of the destination image and copying
    region extending to the lower right corner of *imageName*.


*imageName* **crop** *x1*  *y1* ?\ *x2*  *y2*\ ?
  Crops *imageName* to specified rectangular region.  The region is defined
  by the coordinates *x1*,  *y1*, *x2*, *y2* (where *x1*, *y1* and *x2*, *y2*
  describe opposite corners of a rectangle) is cut out and saved in
  *imageName*. If no *x2* and *y2* coordinates are specified, then the
  region is from the point *x1*, *y1* to the lower right corner of
  *imageName*. *ImageName* will be resized to the new size.  All the
  coordinates are clamped to reside within the image.  For example if *x2*
  is "10000" and the image width is 50, the value will be clamped to 49.

*imageName* **crossfade** *fromImage* *toImage* ?\ *switches* ... ?
  Cross fades *toImage* into *fromImage*, saving the result in
  *imageName*. *FromImage* and *toImage* can be either the name of a
  picture (it can not be *imageName*) or a color specification.  For
  example if *toImage* is "black", this image will fade to black.
  *FromImage* and *toImage* cannot both be colors. *ImageName* will first
  be a copy of *fromImage*.  It is progressively changed by fading the
  *fromImage* and adding *toImage* until *imageName* is a copy of
  *toImage*.

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

*imageName* **dissolve** *fromImage* *toImage* ?\ *switches* ... ?
  Transitions from *fromImage* to *toImage* by dissolving *toImage*
  into *fromImage* and saving the result in *imageName*. *FromImage* and
  *toImage* can be either the name of a picture (it can not be
  *imageName*) or a color specification.  *FromImage* and *toImage* cannot
  both be colors. *ImageName* starts as a copy of *fromImage*.  It is
  progressively changed by randomly copying pixels from *toImage* into it.

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
  Returns the name of a new picture image that is a duplicate of
  *imageName*. The following switches are available.

  **-region** *bbox*
    Instead of duplicating all of *imageName*, this specifies a sub-region
    to be duplicated. *Bbox* is a list in the form "*x1* *y1* *x2* *y2*" or
    "*x1* *y1*".  The first form describes two opposite corners of the
    sub-region to be copied.  The second form is where *x1*,\ *y1* is the
    upper left corner of the sub-region and the lower right corner of
    *imageName* is the other corner.

*imageName* **emboss** *srcName*
  Embosses *srcName* and saves the result in *imageName*.  *SrcName* is the
  name of picture image, but can't the same as *imageName*.  The image
  is embossed by shading the RGB pixels using a single distant light source.

  Reference: "Fast Embossing Effects on Raster Image Data" by John Schlag,
  in "Graphics Gems IV", Academic Press, 1994.
  
*imageName* **export** *imageFormat* ?\ *switches* ... ?
  Exports *imageName* into another format. *ImageFormat* is one of the
  different formats are described in the section `PICTURE FORMATS`_
  below. *Switches* are specific to *imageFormat*.

*imageName* **fade** *srcName* *percent*
  Decreases the opacity of *srcName* by *percent* (making it more
  transparent) and saves the result in *imageName*.  *Percent* is
  percentage (0 to 100) that specifies the amount to reduce the opacity.
  *SrcName* is the name of picture image and can the same as *imageName*.
  
*imageName* **flip x** ?\ *srcName*\ ?
  Flips the image in *srcName* horizontally and saves the result in
  *imageName*.  If no *srcName* argument is given, then *imageName*
  is flipped.

*imageName* **flip y** ?\ *srcName*\ ?
  Flips the image in *srcName* vertically and saves the result in
  *imageName*.  If no *srcName* argument is given, then *imageName*
  is flipped.

*imageName* **gamma** *gammaValue* 
  Gamma corrects *imageName* using *gammaValue*.  Specifies that the colors
  allocated for displaying this image in a window should be corrected for a
  non-linear display with the specified gamma exponent value.  (The
  intensity produced by most CRT displays is a power function of the input
  value, to a good approximation; gamma is the exponent and is typically
  around 2).  The value specified must be greater than zero.  The default
  value is "1.0" (no correction).  In general, values greater than one will
  make the image lighter, and values less than one will make it darker.

  *GammaValue* is used to compute the light intensity of the monitor as

     L = pow(v, gammaValue);

   where L is the radiance (light intensity) and v is the voltage applied.

*imageName* **get** *x* *y* 
  Returns the pixel value at the designated coordinates in *imageName*. The
  pixel at *x*,\ *y* must reside within the image.  The upper left corner
  of the image is 0,0.  The lower right corner is width-1, height-1.
  
*imageName* **greyscale** *srcName*
  Converts *srcName* to greyscale and saves the result in *imageName*
  *SrcName* is the name of picture image. It can be the same as
  *imageName*.

  Luminosity is computed using the formula

    Y = 0.212671 * R + 0.715160 * G + 0.072169 * B

  where Y is the luminosity and R, G, and B are color components.
  
*imageName* **height** ?\ *numPixels*\ ?
  Gets or sets the height of the picture.  If no *numPixels* argument is
  present, the height of the picture in pixels is returned.  *NumPixels*
  may have any of the forms acceptable to **Tk_GetPixels**, such as "200"
  or "2.4i".


*imageName* **import** *imageFormat* ?\ *switches* ... ?
  Import data into *imageName* from another format. *ImageFormat* is one of
  the different formats are described in the section `PICTURE FORMATS`_
  below. *Switches* are specific to *imageFormat*.

*imageName* **info** 
  Returns a key-value list of information about *imageName*. The
  keys are the following.

  **colors**
     The number of colors used.

  **count**
     The number of pictures in *imageName*.  

  **format**
     Indicates the original format *imageName*.  

  **height**
     The height of *imageName* in pixels.

  **index**
     The index of the current picture in *imageName*.

  **isassociated**
     Indicates if the alpha-values have been pre-multipled in RGB values
     of the images.

  **isgreyscale**
     Indicates if the *imageName* is greyscale (R = G = B for each pixel).

  **isopaque**
     Indicates if the *imageName* is opaque (all alpha-values are 0xFF).

  **width**
     The width of *imageName* in pixels.

*imageName* **list append** ?\ *srcName* ... ?
   Appends *srcName* to the list of pictures for *imageName*. The contents
   of *srcName* are copied and appended the list of pictures for
   *imageName*. *SrcName* is the name of a *picture* image and may not be
   the same as *imageName*.
   
*imageName* **list current** ?\ *numPicture*\ ?
   Sets or gets the index of the current picture displayed for *imageName*.
   If no *numPicture* argument is present, this command returns the current
   index.  Otherwise *numPicture* is the position in the list of the new
   current picture.  Picture indices start from 0.

*imageName* **list delay** *delay*
   Sets or gets the current delay between automatic picture changes.
   *Delay* is an integer representing the number of milliseconds to wait
   between picture changes.  See the **list start** operation for details
   how to automatically change pictures.

*imageName* **list delete** *firstIndex* ?\ *lastIndex*\ ?
   Deletes one or more pictures from *imageName*.  *FirstIndex* and
   *lastIndex* are picture indices.  The pictures from *firstIndex* to
   *lastIndex* will be deleted.  If no *lastIndex* argument is present,
   then only *firstIndex* is deleted.

*imageName* **list length** 
   Returns the number of pictures in *imageName*.
   
*imageName* **list next** 
   Moves the current index to the next picture of *imageName*.  The next
   picture will be displayed. If the current picture is at the end of the
   list, the next index will be the first picture.
   
*imageName* **list previous** 
   Moves the current index to the next picture of *imageName*.  The
   previous picture will be displayed. If the current picture is at the
   beginning of the list, the previous index will be the last picture.
   
*imageName* **list replace** *firstIndex*  *lastIndex* ?\ *srcName* ... ?
   Replaces one or pictures in the list of *imageName*.  The pictures
   in the range *firstIndex* to *lastIndex* are removed and replaced
   with the *srcName*.  *SrcName* is the name of a *picture* image.
   *FirstIndex* and *lastIndex* are picture indices.     

*imageName* **list start**
   Starts rotating pictures in *imageName*.  The time between picture
   changes is set by the **list delay** operation.

*imageName* **list stop**
   Stops the rotation of pictures. 

*imageName* **max** *pictOrColor*
  Computes the maximum of the picture or color and *imageName*.  The
  maximum of each color component is computed.  *PictOrColor* is either the
  name of a *picture* image or a color specification.  The resulting image
  is saved in *imageName*.

  **-invert** *boolean*
     Indicates to invert mask.  This only has affect is the **-mask** switch
     is set.

  **-mask** *mask*
    Only change the non-zero pixels of *mask* in *imageName*.
    *Mask* is either the name of a *picture* image. 

*imageName* **min** *pictOrColor*
  Computes the minimum of the picture or color and *imageName*.  The
  minimum of each color component is computed.  *PictOrColor* is either the
  name of a *picture* image or a color specification.  The resulting image
  is saved in *imageName*.

  **-invert** *boolean*
     Indicates to invert mask.  This only has affect is the **-mask** switch
     is set.

  **-mask** *mask*
    Only change the non-zero pixels of *mask* in *imageName*.
    *Mask* is either the name of a *picture* image. 

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

  **-invert** *boolean*
     Indicates to invert mask.  This only has affect is the **-mask** switch
     is set.

  **-mask** *mask*
    Only change the non-zero pixels of *mask* in *imageName*.
    *Mask* is either the name of a *picture* image. 

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

  **-invert** *boolean*
     Indicates to invert mask.  This only has affect is the **-mask** switch
     is set.

  **-mask** *mask*
    Only change the non-zero pixels of *mask* in *imageName*.
    *Mask* is either the name of a *picture* image. 

*imageName* **project** *srcName* *coords* *coords* ?\ *switches* ... ?
  Projects *srcName* into a quadrilateral.

*imageName* **put** *x* *y* *colorSpec* 
  Sets the named color at the specified coordinates in *imageName*.  Both
  *x* and *y* must reside within the image.  The upper left corner of the
  image is 0,0.  The lower right corner is width-1, height-1.  *ColorSpec*
  is a color specification that can be in any of the forms described
  above in the section `COLOR SPECIFICATIONS`_
  
*imageName* **quantize** *srcName* *numColors*
  Reduces the number of colors in *srcName* to be less than or
  equal to *numColors*. The resulting image is saved in *imageName*.
  *NumColors* is a number greater than 1.
   
  Reference: "Efficient Statistical Computations for Optimal Color
  Quantization"by Wu, Xiaolin in "Graphics Gems II", p. 126-133, Academic
  Press, 1995.
   
*imageName* **reflect** *srcName* ?\ *switches* ... ?
  Creates a reflection of *srcName* with the resulting image saved in
  *imageName*.  *SrcName* is the name of another *picture* image and may
  not be the same as *imageName*. *Switches* may be any of the following.

  **-background** *colorSpec*
    Specifies the background color of reflection.  If *colorSpec* is
    "", then the background is transparent.

  **-blur** *blurRadius*
    Specifies the radius of the blur.  If *blurRadius* is 0, no blurring
    of the reflection is performed.  The default is 0.

  **-colorscale** *scale*
    Specifies the scale when interpolating color values. *Scale* can be
    "linear", or "logarithmic"".

    **linear**
        Colors are interpolated on a linear scale between 0.0 and 1.0.
    **logarithmic**
        Colors are interpolated using the log of the value.
    
  **-low** *percentOpacity*
    Specifies the starting percent opacity of the reflection.
    *PercentOpacity* is a real number between 0 and 100.  It is the
    percentage that opacity may vary.

  **-high** *percentOpacity*
    Specifies the ending percent opacity of the reflection.
    *PercentOpacity* is a real number between 0 and 100.  It is the
    percentage that opacity may vary.

  **-jitter** *percentJitter*
    Specifies the amount of randomness to add to the interpolated colors.
    *PercentJitter* is a real number between 0 and 100.  It is the
    percentage that colors may vary.
  
  **-ratio** *number*
    Specifies the ratio between the *srcName* and the reflection.

  **-side** *side*
    Specifies the side of *srcName* to be reflected.  Side can be "bottom"
    "top", "left" or "right".

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
      BellFilter The support is 1.5.

    **bessel**
      BesselFilter The support is 3.2383.

    **box**
      This filter sums up all the samples in the filter area with an equal
      weight. Box is the fastest filtering method.  The support is 0.5.

    **bspline**
      BSplineFilter. The support is 2.0.

    **catrom**
      Samples are weighted by a hermite curve that has a negative lobe near
      its border. This filter will increase contrast at edges in the image,
      sharpening the image. The support is 2.0.

    **gaussian**
      The gauss filter uses a sloped curve, weighting the sampling gently
      at the top of the peak and toward the edge of the sampled area. This
      filtering method is often used to control the soft staircase artifact
      effect.  The support is 1.25.

    **lanczos3**
      The lanczos filter uses a narrower, less bell-shaped curve than the
      gaussian filter. The curve can go into negative values near the
      edges.  The support is 3.0.

    **mitchell**
      The mitchell filter uses a narrower bell-shaped curve than the
      Gaussian filter. The curve can go into negative values near the
      edges.  The support is 2.0.

    **sinc**
      Samples are weighted by a filter that looks similar to Catmull-Rom
      and has a negative lobe near its border. This filter will
      increase contrast at the edges in the image and give very sharp
      images.  The support is 4.0.

    **tent**
      Same as **triangle**.

    **triangle**
      The triangle filter uses a linear curve that affects the pixels so
      that the least filtering happens at the edges of the sampled area.
      The support is 1.0.

  **-from** *bbox*
    Specifies a region in the *srcName* to be resampled.  By default
    the all of *srcName* is resampled.

  **-height** *numPixels*
    Specifies the height of the resampled image.  *NumPixels* may have any
    of the forms acceptable to **Tk_GetPixels**, such as "200" or "2.4i".
    If *numPixels* is "0", then the height of *imageName* will not change.
    
  **-hfilter** *filterName*
    Specifies the image filter to use for horizontal resampling. 
    *FilterName* can be any of the filter described in **-filter**
    switch.
     
  **-maxpect** 
    Forces the *imageName* to retain the same aspect ratio as *srcName*.
    The maximum of **-width** and **-height** is used.

  **-vfilter** *filterName*
    Specifies the image filter to use for vertical resampling.
    *FilterName* can be any of the filter described in **-filter** switch.


  **-width** *numPixels*
    Specifies the width of the resampled image.  *NumPixels* may have any
    of the forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".
    If *numPixels* is "0", then the width of *imageName* will not change.

*imageName* **rotate** *srcName* *angle*
   Rotates *srcName* by *angle* and saves the result in *imageName*.
   *SrcName* is the name of a picture image and may be the same as
   *imageName*.  *Angle* is the number of degrees to rotate the picture.
   If the angel is not orthogonal, then the unpainted areas will be
   transparent (0x00).
   
*imageName* **select** *srcName* *firstColor* ?\ *lastColor*\ ?
   Creates a mask by selecting the pixels in *srcName* that are between two
   colors.  *SrcName* is the name of a picture image but may not be the
   same as *imageName*.  The resulting mask is saved in *imageName*. The
   pixels of *imageName* that represent selected pixels in *srcName* will
   be 1 (0xFFFFFFFF), otherwise 0 (0x00000000). *FirstColor* and
   *lastColor* are color specifications that represent a range of colors to
   be selected.
   
*imageName* **sharpen** 
   Sharpens *imageName*.  Sharpening is done by blurring *imageName* and
   subtracting the blur from it.  The result is saved in *imageName*.

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
     Specifies the sub-region in *window* to snap.  *Bbox* is a list
     in the form "*x1* *y1* *x2* *y2*" or "*x1* *y1*".  The first form
     describes the subregion to be snaped.  The second indicates to copy
     the subimage starting at *x1*,\ *y1* of *window* extending to the
     lower right corner.

   **-raise** 
     Indicates to raise the window before snapping. The is sometimes
     required for non-Tk windows.  The default is not to raise *window*.
     
*imageName* **subtract** *pictOrColor*
  Performs an arithmetic-subtraction of the picture or color from Each color
  component is subtracted.  *imageName*.  *PictOrColor* is either the name
  of a *picture* image or a color specification.  The resulting image is
  saved in *imageName*.

  **-invert** *boolean*
    Indicates to invert mask.  This only has affect is the **-mask** switch
    is set.

  **-mask** *mask*
    Only change the non-zero pixels of *mask* in *imageName*.
    *Mask* is either the name of a *picture* image. 

*imageName* **width** *pixels* 
  Gets or sets the width of the picture.  If no *numPixels* argument is
  present, the width of the picture in pixels is returned.  *NumPixels*
  may have any of the forms acceptable to **Tk_GetPixels**, such as "200"
  or "2.4i".

*imageName* **wipe** *fromImage* *toImage* ?\ *switches* ... ?
   Transitions from *fromImage* to *toImage* by wiping. *toImage* is
   *toImage* into *fromImage* and saving the result in
   *imageName*. *FromImage* and *toImage* can be either the name of a
   picture (it can not be *imageName*) or a color specification.
   *FromImage* and *toImage* cannot both be colors. *ImageName* starts as a
   copy of *fromImage*.  It is progressively changed by randomly copying
   pixels from *toImage* into it.

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
  Performs a bitwise-xor with each color component of *imageName* and the
  picture or color.  *PictOrColor* is either the name of a *picture* image
  or a color specification.  The resulting image is saved in
  *imageName*. *Switches* can be one of the following.

  **-invert** 
    Indicates to invert the mask.  This only has affect is the **-mask**
    switch is set.

  **-mask** *mask*
    Only change the non-zero pixels of *mask* in *imageName*.
    *Mask* is the name of a *picture* image. If the **-invert** switch
    is set, then the zero pixels of mask will be changed.

PICTURE FORMATS
---------------

Pictures can import and export their data into various formats.
They are loaded using the TCL **package** mechanism. Normally this
is done automatically for you when you invoke an **import** or
**export** operation on a picture.

The available formats are **bmp**, **gif**, **ico**, **jpg**, **pdf**,
**photo**, **png**, **pbm**, **ps**, **tga**, **tif**, **xbm**, and **xpm**
and are described below.

**bmp**
~~~~~~~

The *bmp* module reads and writes Device Independent Bitmap (BMP) data.
The BMP format supports 8, 15, 16, 24, and 32 bit pixels.
The 32-bit format supports 8-bit RGB components with an 8-bit alpha
channel.  The package can be manually loaded as follows.


    **package require blt_picture_bmp**

By default this package is automatically loaded when you use the *bmp*
format in the **import** or **export** operations.

*imageName* **import bmp** ?\ *switches* ... ?
    Imports BMP data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required. The following import switches are
    supported:

    **-data** *string*
     Read the BMP information from *string*.

    **-file** *fileName*
     Read the BMP file from *fileName*.

*imageName* **export bmp** ?\ *switches* ... ?
    Exports *imageName* into BMP data.  If no **-file** or **-data** switch
    is provided, this command returns the BMP output as a base64 string.  If
    *imageName* is greyscale, then the BMP output will be 1 8-bit component
    per pixel, otherwise it will contain 3 8-bit components per pixel.  If
    any pixel in *imageName* is not opaque, then an extra alpha component is
    output.

    The following switches are supported:

    **-alpha**
      Indicates to create BMP data with an 8-bit alpha channel.  This
      option affects only non-opaque pixels in *imageName*.  By default
      non-opaque pixels are blended with a background color (see the
      **-background** option).

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels and the **-alpha** switch is not set.
      *ColorSpec* is a color specification. The default background color
      is "white".

    **-data** *varName*
      Specifies the name of TCL variable to be set with the binary BMP
      data. *VarName* is the name of a global TCL variable.  It will
      contain a byte array object.

    **-file** *fileName*
      Write the BMP output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

**gif**
~~~~~~~~

The *gif* module reads and writes Graphic Interchange Format (GIF) data.
The package can be manually loaded as follows.

    **package require blt_picture_gif**

By default this package is automatically loaded when you use the *gif*
format in the **import** or **export** operations.

*imageName* **import gif** ?\ *switches* ... ?
    Imports GIF data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required.  The following import switches
    are supported:

    **-data** *string*
     Read the GIF information from *string*.

    **-file** *fileName*
     Read the GIF file from *fileName*.

*imageName* **export gif** ?\ *switches* ... ?
    Exports *imageName* into GIF data.  If no **-file** or **-data** switch
    is provided, this command returns the GIF output as a base64 string.
    The following switches are supported:

    **-animate** 
     Generates animated GIF output using the list of pictures in
     *imageName*. All the pictures in *imageName* should be the same size.

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains semi-transparent pixels.  *ColorSpec* is a color specification.

    **-comments** *string*
      Specifies comments to be included in the GIF data. *String* is a TCL list
      of key value pairs.

    **-data** *varName*
      Specifies the name of TCL variable to be set with the binary GIF
      data. *VarName* is the name of a global TCL variable.  It will
      contain a byte array object.

    **-delay** *milliseconds*
     Specifies the delay between images for the animated GIF.

    **-file** *fileName*
      Write the GIF output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

**ico**
~~~~~~~

The *ico* module reads and writes the image file format for computer icons
in Microsoft Windows (ICO). ICO files contain one or more small images at
multiple sizes and color depths, such that they may be scaled
appropriately. The package can be manually loaded as follows.

    **package require blt_picture_ico**

By default this package is automatically loaded when you use the *ico*
format in the **import** or **export** operations.

*imageName* **import ico** ?\ *switches* ... ?
    Imports ICO data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required. The following import switches are
    supported:

    **-data** *string*
     Read the ICO information from *string*.

    **-file** *fileName*
     Read the ICO file from *fileName*.

*imageName* **export ico** ?\ *switches* ... ?
    Exports *imageName* into ICO data.  If no **-file** or **-data** switch
    is provided, this command returns the ICO output as a base64 string.  If
    *imageName* is greyscale, then the ICO output will be 1 8-bit component
    per pixel, otherwise it will contain 3 8-bit components per pixel.  If
    any pixel in *imageName* is not opaque, then an extra alpha component is
    output.

    The following switches are supported:

    **-alpha**
      Indicates to create ICO data with an 8-bit alpha channel.  This
      option affects only non-opaque pixels in *imageName*.  By default
      non-opaque pixels are blended with a background color (see the
      **-background** option).

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels and the **-alpha** switch is not set.
      *ColorSpec* is a color specification. The default background color
      is "white".

    **-data** *varName*
      Specifies the name of TCL variable to be set with the binary ICO
      data. *VarName* is the name of a global TCL variable.  It will
      contain a byte array object.

    **-file** *fileName*
      Write the ICO output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

**jpg**
~~~~~~~

The *jpg* module reads and writes Joint Photographic Experts Group Format
(JPEG) data.  The package can be manually loaded as follows.

    **package require blt_picture_jpg**

By default this package is automatically loaded when you use the *jpg*
format in the **import** or **export** operations.

*imageName* **import jpg** ?\ *switches* ... ?
    Imports JPEG data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required.  The following import switches are
    supported:

    **-data** *string*
     Reads the JPEG information from *string*.

    **-dct** *method*
      Specifies the discrete cosine transform method. *Method* must be one
      of the following.

      **slow**
        Uses a slow but accurate integer algorithm. This is the default.

      **fast**
        Uses a faster but less accurate integer algorithm.

      **float**
        Uses floating-point. More accurate and faster depending on your
        hardware.

    **-file** *fileName*
      Reads the JPEG file from *fileName*.

*imageName* **export jpg** ?\ *switches* ... ?
    Exports *imageName* into JPEG data.  If no **-file** or **-data** switch
    is provided, this command returns the JPEG output as a base64 string.
    The following switches are supported:

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains transparent pixels.  *ColorSpec* is a color specification.

    **-data** *varName*
      Specifies the name of TCL variable to be set with the binary JPEG
      data. *VarName* is the name of a global TCL variable.  It will
      contain a byte array object.

    **-file** *fileName*
      Write the JPEG output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

    **-quality** *percent*
      Specifies the percent quality.  *Percent* must be a number between
      0 and 100.

    **-progressive** 
      Indicates to create a progressive JPEG.

    **-smooth** *percent*
      Specifies the percent of smoothing. *Percent* must be a number between
      0 and 100.

**photo**
~~~~~~~~~

The *photo* module reads and writes Tk photo data.
The package can be manually loaded as follows.

    **package require blt_picture_photo**

By default this package is automatically loaded when you use the *photo*
format in the **import** or **export** operations.

*imageName* **import photo** ?\ *switches* ... ?
    Imports Tk photo data into *imageName*.  The **-image** 
    switch is required.  The following import switches are supported:

    **-image** *photoName*
      Reads the photo information from image *photoName*. *PhotoName* must
      be the name of a Tk photo image.

*imageName* **export photo** ?\ *switches* ... ?
    Exports *imageName* into a Tk photo image.  The **-image** switch is
    required.  The following import switches are supported:

    **-image** *photoName*
      Write the picture information to the photo image *photoName*.
      *PhotoName* must be the name of a Tk photo image.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default, 0,
      is the first picture.

**pbm**
~~~~~~~

The *pbm* module reads and writes the NETPBM format.  These include the
Portable Pixmap (PPM), Portable Bitmap (PBM) and Portable Greymap (PGM)
data.  The NETPBM format supports multiple images in a single output.

The package can be manually loaded as follows.

    **package require blt_picture_pbm**

By default this package is automatically loaded when you use the *pbm*
format in the **import** or **export** operations.

*imageName* **import pbm** ?\ *switches* ... ?
    Imports NETPBM data into *imageName*.  Either the **-file** or
    **-data** switch (described below) is required. The following import
    switches are supported:

    **-data** *string*
     Read the NETPBM information from *string*.

    **-file** *fileName*
     Read the NETPBM file from *fileName*.

*imageName* **export pbm** ?\ *switches* ... ?
    Exports *imageName* into NETPBM data.  If no **-file** or **-data**
    switch is provided, this command returns the NETPBM output as a base64
    string.  If *imageName* is greyscale, then the NETPBM output will be 1
    8-bit component per pixel (PGMRAW), otherwise it will contain 3 8-bit
    components per pixel (PPMRAW).  

    The following switches are supported:

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels.  *ColorSpec* is a color
      specification. The default background color is "white".

    **-data** *varName*
      Specifies the name of TCL variable to be set with the binary PBM
      data. *VarName* is the name of a global TCL variable.  It will
      contain a byte array object.

    **-file** *fileName*
      Write the PBM output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. If *numPicture* is a negative, all pictures will be
      exported.  The default is 0, which is the first picture.

**pdf**
~~~~~~~

The *pdf* module reads and writes Adobe's Portable Document format (PDF)
data.  The PDF format supports 24-bit pixels with an alpha channel.  The
package can be manually loaded as follows.

    **package require blt_picture_pdf**

By default this package is automatically loaded when you use the *pdf*
format in the **import** or **export** operations.

*imageName* **import pdf** ?\ *switches* ... ?
    Imports PDF data into *imageName*.  This command requires that the
    **ghostscript** interpreter **gs** be in your PATH.  Either the
    **-file** or **-data** switch (described below) is required. The
    following import switches are supported:

    **-data** *string*
     Reads the PDF information from *string*.

    **-dpi** *number*
     Specifies the dots per index (DPI) when converting the PDF input.
     The default is "100".

    **-file** *fileName*
     Reads the PDF file from *fileName*.

    **-nocrop** 
     Indicates to not crop the image at the BoundingBox.  The can
     add a border around the image.  The default is to crop the data.

    **-papersize** *string*
     Specifies the paper size. *String* is . The default is "letter".

*imageName* **export pdf** ?\ *switches* ... ?
    Exports *imageName* into PDF data.  If no **-file** or **-data** switch
    is provided, this command returns the binary PDF output as a string.  
    If *imageName* contains non-opaque pixels, *imageName* will be blended
    in with the background color specified by the **-background** switch
    or the PDF output will contain a SoftMask depending on the **-alpha**
    switch.

    The following switches are supported:

    **-alpha**
      Indicates to create PDF data with an SoftMask for the 8-bit alpha
      channel.  This option affects only non-opaque pixels in *imageName*.
      By default non-opaque pixels are blended with a background color (see
      the **-background** option).

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels. *ColorSpec* is a color specification. The
      default background color is "white".

    **-comments** *string*
      Specifies comments to be included in the PDF data. *String* is a TCL
      list of key value pairs.

    **-data** *varName*
      Specifies the name of TCL variable to be set with the PDF
      data. *VarName* is the name of a global TCL variable.  

    **-file** *fileName*
      Writes the PDF output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

**png**
~~~~~~~

The *png* module reads and writes Portable Network Graphics (PNG) data.
The package can be manually loaded as follows.

    **package require blt_picture_png**

By default this package is automatically loaded when you use the *png*
format in the **import** or **export** operations.

*imageName* **import png** ?\ *switches* ... ?
    Imports PNG data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required.  The following import switches are
    supported:

    **-data** *string*
     Read the PNG information from *string*.

    **-file** *fileName*
     Read the PNG file from *fileName*.

*imageName* **export png** ?\ *switches* ... ?
    Exports *imageName* into PNG data.  If no **-file** or **-data** switch
    is provided, this command returns the PNG output as a base64 string.  If
    *imageName* is greyscale, then the PNG output will be 1 8-bit component
    per pixel, otherwise it will contain 3 8-bit components per pixel.  If
    any pixel in *imageName* is not opaque, then an extra alpha component is
    output.

    The following switches are supported:

    **-comments** *list*
      Specifies comments to be included in the PNG data. *List* is a TCL list
      of key value pairs.

    **-data** *varName*
      Specifies the name of TCL variable to be set with the binary PNG
      data. *VarName* is the name of a global TCL variable.  It will
      contain a byte array object.

    **-file** *fileName*
      Write the PNG output to the file *fileName*.

**ps**
~~~~~~

The *ps* module reads and writes Adobe's PostScript format (PS) data.
The PS format supports 24-bit pixels.  The package can be manually loaded
as follows.

    **package require blt_picture_ps**

By default this package is automatically loaded when you use the *ps*
format in the **import** or **export** operations.

*imageName* **import ps** ?\ *switches* ... ?
    Imports PS data into *imageName*. This command requires that the
    **ghostscript** interpreter **gs** be in your PATH.  Either the
    **-file** or **-data** switch (described below) is required. The
    following import switches are supported:

    **-data** *string*
     Reads the PS information from *string*.

    **-dpi** *number*
     Specifies the dots per index (DPI) when converting the PS input.
     The default is "100".

    **-file** *fileName*
     Reads the PS file from *fileName*.

    **-nocrop** 
     Indicates to not crop the image at the BoundingBox.  The can
     add a border around the image.  The default is to crop the data.

    **-papersize** *string*
     Specifies the paper size. *String* is . The default is "letter".

*imageName* **export ps** ?\ *switches* ... ?
    Exports *imageName* into PS data.  If no **-file** or **-data** switch
    is provided, this command returns the PS output as a string.  If
    *imageName* contains non-opaque pixels, *imageName* will be blended in
    with the background color specified by the **-background** switch.  The
    following switches are supported.

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels. *ColorSpec* is a color specification. The
      default background color is "white".

    **-center** 
      Indicates to center the image on the page.

    **-comments** *string*
      Specifies comments to be included in the PS data. 

    **-data** *varName*
      Specifies the name of TCL variable to be set with the PS
      data. *VarName* is the name of a global TCL variable.  

    **-file** *fileName*
      Writes the PS output to the file *fileName*.

    **-greyscale** 
      Indicates to convert the image to greyscale before exporting to PS.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

    **-landscape**
      Indicates to rotate the image 90 degrees. The the x-coordinates of
      the image run along the long dimension of the page.

    **-level** *pslevel*
      Specifies the PostScript level.

    **-maxpect** 
      Indicates to scale the image so that it fills the PostScript page.
      The aspect ratio of the picture is still retained.  

    **-padx** *numPica*
      Specifies the horizontal padding for the left and right page borders.
      The borders are exterior to the image.  *NumPixels* can be a list of
      one or two screen distances.  If *numPica* has two elements, the left
      border is padded by the first distance and the right border by the
      second.  If *numPica* has just one distance, both the left and right
      borders are padded evenly. The default is "1i".

    **-pady** *numPica*
      Specifies the vertical padding for the top and bottom page
      borders. The borders are exterior to the image.  *NumPica* can be a
      list of one or two page distances.  If *numPica* has two elements,
      the top border is padded by the first distance and the bottom border
      by the second.  If *numPica* has just one distance, both the top and
      bottom borders are padded evenly.  The default is "1i".

    **-paperheight** *numPica*
      Specifies the height of the PostScript page.  This can be used to
      select between different page sizes (letter, A4, etc).  The default
      height is "11.0i".

    **-paperwidth** *numPica*
      Specifies the width of the PostScript page.  This can be used to
      select between different page sizes (letter, A4, etc).  The default
      width is "8.5i".

**tga**
~~~~~~~

The *tga* module reads and writes Truevision Graphics Adapter (TGA) aka
TARGA data.  The TGA format supports 8, 15, 16, 24, and 32 bit pixels.
The 32-bit format supports 8-bit RGB components with an 8-bit alpha
channel.  The package can be manually loaded as follows.

    **package require blt_picture_tga**

By default this package is automatically loaded when you use the *tga*
format in the **import** or **export** operations.

*imageName* **import tga** ?\ *switches* ... ?
    Imports TGA data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required.  The following import switches
    are supported:

    **-data** *string*
     Read the TGA information from *string*.

    **-file** *fileName*
     Read the TGA file from *fileName*.

    **-info** *varName*
     Specifies the name of TCL variable *varName* that will be set with a
     list of metadata from the TGA data examined.  *VarName* is the name of
     a global TCL variable.  The list will contain key/value pairs.
     
*imageName* **export tga** ?\ *switches* ... ?
    Exports *imageName* into TGA data.  If no **-file** or **-data** switch
    is provided, this command returns the TGA output as a base64 string.  If
    *imageName* is greyscale, then the TGA output will be 1 8-bit component
    per pixel, otherwise it will contain 3 8-bit components per pixel.  If
    any pixel in *imageName* is not opaque, then an extra alpha component is
    output.

    The following switches are supported:

    **-alpha**
      Indicates to create TGA data with an 8-bit alpha channel.  This
      option affects only non-opaque pixels in *imageName*.  By default
      non-opaque pixels are blended with a background color (see the
      **-background** option).

    **-author** *string*
      Specifies a string for the author's name to included in the TGA data. 
      *String* may contain no more than 40 characters.
      
    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels and the **-alpha** switch is not set.
      *ColorSpec* is a color specification. The default background color
      is "white".

    **-comments** *string*
      Specifies comments to be included in the TGA data. *String* may
      contain up to 4 lines (separated by newlines) with each line no more
      than of 80 characters.

    **-data** *varName*
      Specifies the name of TCL variable to be set with the binary TGA
      data. *VarName* is the name of a global TCL variable.  It will
      contain a byte array object.

    **-file** *fileName*
      Write the TGA output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

    **-job** *string*
      Specifies a job name (image name) to be included an ID for the TGA
      data. *String* is may be a maximum of 40 characters.

    **-rle** 
      Indicates to compress the image data using run-length encoding.

    **-software** *string*
      Specifies an application name that created the image data to be
      included the software name for the TGA data. *String* is may contain
      no more than 40 characters.

**tif**
~~~~~~~

The *tif* module reads and writes Tagged Image File Format (TIFF) data.
The TIFF format supports 8, 15, 16, 24, and 32 bit pixels.  The 32-bit
format supports 8-bit RGB components with an 8-bit alpha channel.  The
package can be manually loaded as follows.

    **package require blt_picture_tif**

  By default this package is automatically loaded when you use the *tif*
  format in the **import** or **export** operations.

*imageName* **import tif** ?\ *switches* ... ?
    Imports TIFF data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required. The following import switches are
    supported:

    **-data** *string*
     Reads the TIFF information from *string*.

    **-file** *fileName*
     Reads the TIFF file from *fileName*.

*imageName* **export tif** ?\ *switches* ... ?
    Exports *imageName* into TIFF data.  If no **-file** or **-data** switch
    is provided, this command returns the TIFF output as a base64 string.  If
    *imageName* is greyscale, then the TIFF output will be 1 8-bit component
    per pixel, otherwise it will contain 3 8-bit components per pixel.  If
    any pixel in *imageName* is not opaque, then an extra alpha component is
    output.

    The following switches are supported:

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels and the **-alpha** switch is not set.
      *ColorSpec* is a color specification. The default is "white".

    **-compress** *compressType*

      Specifies the type of compress to perform on the image
      data. *CompressType* can be one of the following.

      **lzw**
        Lempel-Ziv & Welch

      **ojpeg**
        6.0 JPEG
  
      **peg**
        JPEG DCT compression.

      **next**
        NeXT 2-bit RLE.

      **packbits**
        Macintosh RLE.

      **thunderscan**
        ThunderScan RLE

      **pixarfilm**
        Pixar companded 10bit LZW

      **pixarlog**
        Pixar companded 11bit ZIP

      **deflate**
        Deflate compression.

      **adobe_deflate**
        Adobe's deflate.

      **dcs**
        Kodak DCS encoding.

      **sgilog**
        SGI Log Luminance RLE.

      **sgilog24**
        SGI Log 24-bit packed

    **-data** *varName*
      Specifies the name of TCL variable to be set with the binary TIFF
      data. *VarName* is the name of a global TCL variable.  It will
      contain a byte array object.

    **-file** *fileName*
      Writes the TIFF output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.


**xbm**
~~~~~~~

The *xbm* module reads and writes X Bitmap format (XBM) data.  The XBM
format supports 1-bit pixels.  The values of the pixels are either 0
or 1. The package can be manually loaded as follows.

    **package require blt_picture_xbm**

By default this package is automatically loaded when you use the *xbm*
format in the **import** or **export** operations.

*imageName* **import xbm** ?\ *switches* ... ?
    Imports XBM data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required. The following import switches are
    supported:

    **-background** *colorSpec*
      Specifies the color of the background.  These are 0 pixels in the
      bitmap. The default is "black".

    **-data** *string*
     Reads the XBM information from *string*.

    **-file** *fileName*
     Reads the XBM file from *fileName*.

    **-foreground** *colorSpec*
      Specifies the color of the foreground.  These are 1 pixels in the
      bitmap. The default is "white".

    **-maskdata** *string*
     Reads the XBM information from *string* representing the bitmap mask.

    **-maskfile** *fileName*
     Reads the XBM file from *fileName* representing the bitmap mask.

*imageName* **export xbm** ?\ *switches* ... ?
    Exports *imageName* into XBM data.  If no **-file** or **-data** switch
    is provided, this command returns the XBM output as a string.  If
    *imageName* contains more than 2 colors, it will be dithered to 2 colors.

    The following switches are supported:

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels. *ColorSpec* is a color specification. The
      default is "white".

    **-data** *varName*
      Specifies the name of TCL variable to be set with the XBM
      data. *VarName* is the name of a global TCL variable.  

    **-file** *fileName*
      Writes the XBM output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

**xpm**
~~~~~~~

The *xpm* module reads and writes X Pixmap format (XPM) data.  The XPM
format supports 8-bit pixels.  The package can be manually loaded as
follows.

    **package require blt_picture_xpm**

By default this package is automatically loaded when you use the *xpm*
format in the **import** or **export** operations.

*imageName* **import xpm** ?\ *switches* ... ?
    Imports XPM data into *imageName*.  Either the **-file** or **-data**
    switch (described below) is required.  The following import switches are
    supported:

    **-data** *string*
     Reads the XPM information from *string*.

    **-file** *fileName*
     Reads the XPM file from *fileName*.

*imageName* **export xpm** ?\ *switches* ... ?
    Exports *imageName* into XPM data.  If no **-file** or **-data** switch
    is provided, this command returns the XPM output as a string.  If
    *imageName* contains more than 2 colors, it will be dithered to 2 colors.

    The following switches are supported:

    **-background** *colorSpec*
      Specifies the color of the background.  This is used if *imageName*
      contains non-opaque pixels. *ColorSpec* is a color specification. The
      default is "white".

    **-data** *varName*
      Specifies the name of TCL variable to be set with the XPM
      data. *VarName* is the name of a global TCL variable.  

    **-file** *fileName*
      Writes the XPM output to the file *fileName*.

    **-index** *numPicture*
      Specifies the picture in the list of pictures of *imageName* to be
      exported. *NumPicture* is a non-negative number.  The default is 0,
      which is the first picture.

    **-noquantize** 
      Indicates to not reduce the number of colors in *imageName* before
      outputing the XPM data.  The default is to reduce the number of
      colors by quantizing *imageName*.

KEYWORDS
--------

picture, image

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
