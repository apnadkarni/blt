
===============
blt::palette
===============

---------------------
Create color palettes
---------------------

:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
========

**blt::palette create**  ?\ *paletteName*\ ? ?\ *option* *value* ... ?

**blt::palette delete**   ?\ *paletteName* ... ?

**blt::palette draw** *paletteName* *imageName*

**blt::palette exists** *paletteName* 

**blt::palette interpolate** *paletteName* *value*

**blt::palette names** ?\ *pattern* ... ?

DESCRIPTION
===========

The **blt::palette** creates a color palette that can be used with the
**-palette** options of the BLT widgets.  A palette object contains a color
(RGB) mapping table and optionally opacity mapping table that translate a
data value into a color and opacity.

INTRODUCTION
============

The **blt::palette** command lets you define a color palette object that
you can use with different widgets.  The palette object translates a
numeric value to a color.  For example, the **blt::contour** widget uses
color palettes to determine the color to draw based on a numeric field
value for each location in the plot.

The palette object contains a color look-up table and optionally an opacity
look-up table.  Given some data value, the look-up tables convert the value
into a color and opacity (alpha value).  Combined together, they represent
the color associated with data value.  But rather than specifying a color
for every possible data value, you can specify a range and a starting and
ending color for that range.  The color for a specific value will be
interpolated linearly from the value.

Look-up tables contain one or more entries in the form

   *firstValue* *lastValue* *color1* *color2* 

where *firstValue* is the starting value, *lastValue* is the ending value.
*Color1* represents the color at *firstValue*.  *Color2* represents the
color at *lastValue*.  If a data value is within the range represented by
*firstValue* and *lastValue*, its color it interpolated from *color1* and
*color2*.  If a data value isn't within the range of any table entries
a default color (usually transparent) is returned.  For opacity look-up
tables, color is just the opacity (alpha value).
   
LOOK-UP TABLE ENTRIES
=====================

There are several ways to define look-up table entries.  It depends on the
type of spacing designated (see the **-colorspacing** and
**-opacityspacing** options).

**regular**
  The look-up table is generated from a list of two or more colors.  The
  colors are regularly spaced at values from 0 to 1. The number of
  entries is determined is number of colors minus 1. For example if the
  list contains 5 colors

  ::

    red orange yellow green indigo

  there would be four table entries.

  ::

    0.0  0.25 red orange
    0.25 0.5  orange yellow
    0.5  0.75 yellow green
    0.75 1.0  green indigo

  The first and last values for each entry are computed by evenly
  spacing the colors between 0 and 1.  Data values are always
  relative (normalized to be between 0 and 1).

**irregular**
  The palette is defined by two or more pairs of value and color.
  The number of entries is determined is number of pairs minus 1.
  For example if the list contains 5 pairs

  ::

    0.0 red 0.2 orange 0.4 yellow 0.5 green 1.0 indigo

  there would be four table entries.

  ::

    0.0  0.2 red orange
    0.2  0.4 orange yellow
    0.4  0.5 yellow green
    0.5  1.0 green indigo

  The first and last values for each entry are explicit from the 
  values in the list. Note that the values do not have to be evenly spaced.
  They values can be any scale.

**interval**
  The palette is defined by specifying individual entries: a starting
  and ending value and color.
  For example if the list was

  ::

    0.0 red 0.2 orange 0.2 orange 0.4 yellow 0.5 green 1.0 indigo


  there would be three table entries.  

  ::

    0.0  0.2 red orange
    0.2  0.4 orange yellow
    0.5  1.0 green indigo

  Note that entries to not need to cover the entire range of values.
  Here there is a hole between 0.4 and 0.5.  The default color is
  returned.

COLOR SPECIFICATIONS
====================

Colors for the look-up tables above can be specified any of the following
ways.

*colorName* 
  Each color is a single name such as "red" or "#FF0000".

*r* *g* *b* 
  Each color is a triplet of 3 numbers, representing the red, green,
  and blue components for the color.  The numbers are floating point
  numbers.

*h* *s* *v* 
  Each color is a triplet of 3 numbers, representing the hue, saturation,
  and value components for the color.  

OPERATIONS
==========

The following operations are available for the **blt::palette** command.

**blt::palette create** ?\ *paletteName*\ ? ?\ *option* *value* ... ?
  Creates a *palette* object. If no *paletteName* argument is present, then
  the name of the palette is automatically generated in the form
  "palette0", "palette1", etc. Another palette object can not already exist
  as *paletteName*.  This command returns the name of palette object.

  *Option* and *value* determine the palette data an can be any of the
  following.   One of the **-colordata** or **-colorfile** options is
  required.

  **-cdata** *dataString*
    Same as **-colordata** option.

  **-cfile** *fileName*
    Same as **-colorfile** option.

  **-colordata** *dataString*
    Specifies the data for the color portion of the palette. The exact
    format of *dataString* is determined by the **-colorformat** option.
    
  **-colorfile** *fileName*
    Specifies the file to read data for the color portion of the palette.
    The exact format of *dataString* is determined by the **-colorformat**
    option.  The data isn't read and loaded until the palette is used.
    
  **-colorformat** *formatType*
    Determines how colors are specified in the **-colordata** or
    **-colorfile** options.  *FormatType* can be one of the following.

    **name**
       Colors is specified by a single color name ("red") or hex number.
       ("#FF0000").

    **rgb**
       Each color is a triplet of 3 numbers, representing the red, green,
       and blue (RGB) values.  

    **hsv**
       Each color is a triplet of 3 numbers, representing the hue,
       saturation, and value (HSV) values. 
    
    The default is "rgb".
    
  **-colorspacing** *spacingType*
    Specifies the spacing colors.  *SpacingType* can be in any of the
    "regular", "irregular", or "interval". See `LOOK-UP TABLE ENTRIES`_ for
    details.  The default is "regular".

  **-fade** *percent*
    Specifies an overall transparency to be applied to the computed
    color.  If *percent* is "0", no fading is done.  The default is "0".

  **-odata** *dataString*
    Same as **-opacitydata** option.

  **-ofile** *fileName*
    Same as **-opacityfile** option.

  **-opacitydata** *dataString*
    Specifies the a list of numbers defining the opacities in the palette.
    The format of *dataString* is determined by the **-opacityspacing**
    option.  

  **-opacityfile** *fileName*
    Specifies the a file containing list of numbers defining the opacities
    in the palette.  The format of *dataString* is determined by the
    **-opacityspacing** option.

  **-opacityspacing** *spacingType*
    Specifies the spacing of opacities.  *SpacingType* can be in any of the
    "regular", "irregular", or "interval". See `LOOK-UP TABLE ENTRIES`_ for
    details.  The default is "regular".

**blt::palette delete** ?\ *paletteName* ... ?
  Releases resources allocated by one or more palettes.  Palettes are
  reference counted so that the internal palette structures are not
  actually deleted until no one is using the palette any
  more. *PaletteName* must be the name of a palette returned by the
  **create** operation, otherwise an error is reported.

**blt::palette draw** *paletteName* *imageName*
  Draws a color bar representing the palette into *imageName*.  *ImageName*
  is a BLT picture image.  The orientation of the bar is determined by
  whether the width or height or the image is greatest.

**blt::palette exists** *paletteName* 
  Returns "1" if a palette *paletteName* exists, and "0" otherwise.

**blt::palette interpolate** *paletteName* *value*
  Returns the interpolated color for the given value. *Value* is a floating
  point number.  The color is returned as a list of 4 decimal numbers 0 to
  255 representing the alpha, red, green, and blue channels of the color.
  If the value isn't in range of any palette entry, the "" is returned.
  This is useful for debugging palettes.

**blt::palette names** ?\ *pattern* ... ?
  Returns the names of all the palettes currently created.  If one or more
  *pattern* arguments are provided, then the name of any palette matching
  *pattern* will be returned. *Pattern* is a **glob**\ -style pattern.

EXAMPLE
=======

Create a *palette* object with the **blt::palette** command.

 ::

    package require BLT

    # Create a new regular palette.
    blt::palette create regular myPalette \
        -x { 0 10 10 } \
	-y { 0 10 10 } 
        

Now we can create widgets that use the palette.

 ::

    blt::contour .graph
    .graph element create elem1 -palette myPalette

To remove the palette, use the **delete** operation.

 ::

    blt::palette delete myPalette
     
Please note the following:

1. The palettes created by the **blt::palette** command are only recognized by
   BLT widgets.

2. Palette-es are reference counted.  If you delete a palette, its resources
   are not freed until there is no widget is using it.
   
KEYWORDS
========

palette


COPYRIGHT
=========

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
