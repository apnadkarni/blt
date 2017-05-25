
===============
label
===============

-------------------------------------------------
Label canvas item that can be rotated and scaled.
-------------------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

*canvasName* **create label** *x* *y* ?\ *option* *value* ... ?

DESCRIPTION
-----------

A **label** canvas displays a string of characters on the screen.  The text
must be in a single font, but it can occupy multiple lines on the screen
(if it contains newlines). The label may also optionally display a
rectangular filled background or outline. The item can be rotated and the
font is scaled as the item is scaled.  You can use **scale** canvas option
ans the label text will scale.  

SYNTAX
------

*canvasName* **create label** *x* *y* ?\ *option* *value* ... ?

  The **label** item creates a new canvas item. *CanvasName* is the name of a
  **canvas** widget.  You must supply the X-Y coordinate of the new label
  item.  This coordinate and the **-anchor** option (see below) control
  the location of item.

  Additional options may be specified on the command line to configure
  aspects of the label item such as its color and font.  The
  following *option* and *value* pairs are valid.

  **-activebackground** *colorName*
     See **-activefill** option.

  **-activebg** *colorName*
     See **-activeoutline** option.

  **-activedashes** *numPixels*
     Specifies the interval of on/off dashes of the outline around the
     label when the item's state is active (see the **-state** option). If
     *numPixels* is zero, then the outline is a solid line.  The default is
     0.

  **-activedashoffset** *numPixels*
     Specifies the offset of dash pattern used to draw the outline when the
     item's state is active (see the **-state** option). This can be used
     to create a marching ants effect by periodically increasing the
     offset.  The default is 0.

  **-activefg** *colorName*
    See **-activeoutline** option.

  **-activefill** *colorName*
    Specifies the background color of the label rectangle when the item is
    active (see the **-state** option).  If *colorSpec* is "", then no
    background is drawn.  *ColorSpec* may also be a valid BLT *paintbrush*
    name.

  **-activeforeground** *colorName*
    See **-activeoutline** option.

  **-activelinewidth** *numPixels*
    Specifies the width of the rectangular outline.  If *numPixels* is 0,
    no outline is drawn.  The default is 0.

  **-activeoutline** *colorName*
    Specifies the color of the label's rectangular outline and text when
    the item's state is active (see the **-state** option).  The default is
    "black".

  **-anchor** *anchorName*
    Specifies how to position the label item relative to the X-Y coordinate
    specified. The default is "center".

  **-background** *colorName*
    Same as **-fill** option.

  **-bg** *colorName*
    Same as **-fill** option.

  **-dashes** *numPixels*
    Specifies the interval of on/off dashes of the outline around the label
    when the item's state is normal (see the **-state** option). If
    *numPixels* is zero, then the outline is a solid line.  The default is
    0.

  **-dashoffset** *numPixels*
     Specifies the offset of dash pattern used to draw the outline when the
     item's state is normal (see the **-state** option).  The default is 0.

  **-disabledbackground** *colorName*
     See **-disablefill** option.

  **-disabledbg** *colorName*
     See **-disabledfill** option.

  **-disableddashes** *numPixels*
     Specifies the interval of on/off dashes of the outline around the
     label when the item's state is disabled (see the **-state** option). If
     *numPixels* is zero, then the outline is a solid line.  The default is
     0.

  **-disableddashoffset** *numPixels*
     Specifies the offset of dash pattern used to draw the outline when the
     item's state is disabled (see the **-state** option). The default is 0.

  **-disabledfg** *colorName*
    See **-disabledoutline** option.

  **-disabledfill** *colorName*
    Specifies the background color of the label rectangle when the item is
    disabled.  If *colorSpec* is "", then no background is drawn.
    *ColorSpec* may also be a valid BLT *paintbrush* name. The default is
    "".

  **-disabledforeground** *colorName*
    See **-disabledoutline** option.

  **-disabledlinewidth** *numPixels*
    Specifies the width of the rectangular outline drawn when the item's
    state is disabled.  If *numPixels* is 0, no outline is drawn.  The
    default is 0.

  **-disabledoutline** *colorName*
    Specifies the color of the label's rectangular outline and text when
    the item is disabled (see the **-state** option).  The default is
    "grey90".

  **-fg** *colorName*
     Same as the **-outline** option.

  **-fill** *colorSpec*
    Specifies the background color of the label rectangle.  *ColorSpec* may
    be a color name or the name of a paintbrush object created by the
    **blt::paintbrush** command. If *colorSpec* is "", then no background
    is drawn.  The default is "".

  **-font** *fontName* 
     Specifies the base font of the label. The default is "{Sans Serif} 10".

  **-foreground** *colorName*
     Same as the **-outline** option.

  **-height** *numPixels*
     Specifies the height of the label item.  If *numPixels* is "0", then the
     height is determined from font size and text.  The default is "0".

  **-linewidth** *numPixels*
    Specifies the width of the rectangular outline drawn when the item's
    state is normal.  If *numPixels* is 0, no outline is drawn.  The
    default is 0.

  **-maxfontsize** *numPoints*
     Not implemented.

  **-minfontsize** *numPoints*
     Not implemented.

  **-outline** *colorName*
    Specifies the color of the label's rectangular outline and text when
    the item is normal (see the **-state** option).  The default is
    "black".

  **-padx** *numPixels*
    Sets how much padding to add to the left and right of the label.
    *NumPixels* can be a list of one or two numbers.  If *numPixels* has
    two elements, the left side of the label is padded by the first value
    and the right side by the second value.  If *numPixels* has just one
    value, both the left and right sides are padded evenly by the value.
    The default is "0".

  **-pady** *numPixels*
    Sets how much padding to add above and below the label.  *NumPixels*
    can be a list of one or two numbers.  If *numPixels* has two elements,
    the top side of the label is padded by the first value and the bottom
    side by the second value.  If *numPixels* has just one value, both the
    top and bottom sides are padded evenly by the value.  The default is
    "0".

  **-rotate** *numDegrees*
    Specifies the rotation of the label.  The default is "0".

  **-state** *stateName*
    Specifies one of three states for the item: 

    **active**
      Active state means that the item should appear to active.  In this
      state the item is displayed according to the **-activefill**,
      **-activedashes**, **-activedashoffset**, **-activelinewidth**, and
      **-activeoutline** options.

    **normal**
      In the normal state he item is displayed using **-fill**,
      **-dashes**, **-dashoffset**, **-linewidth**, and **-outline**
      options.

    **disabled**
      Active state means that the item should appear to insensitive.  In
      this state the item is displayed according to the **-disabledfill**,
      **-disableddashes**, **-disableddashoffset**, **-disabledlinewidth**,
      and **-disabledoutline** options.

    The default is "normal".

  **-tags** *tagList*
    Specifies a set of tags to apply to the item.  *TagList*  consists
    of  a list of tag names, which replace any existing tags for the
    item.  *TagList* may be an empty list.  The default is "".

  **-text** *string*
    Indicates whether to display the image preview (if one exists), or a
    simple rectangle.  The default is "yes".

  **-textanchor** *anchorName*
    Specifies a bitmap to used to stipple the rectangle representing the
    LABEL item.  The default is "".

  **-width** *numPixels*
    Specifies the height of the label item.  If *numPixels* is "0", then
    the width is determined from font size and text.  If the text is does
    not fit the width, it will be clipped according to the **-textanchor**
    option. The default is "0".

  **-xscale** *numPixels*
    Used to query the current X scale of the item.  Setting *numPixels* 
    has no effect.

  **-yscale** *numPixels*
    Used to query the current Y scale of the item.  Setting *numPixels* 
    has no effect.

NOTES
-----

The label item does not perform anisomorphic scaling correctly as this
would mean distorting the base font.  It will scale the rectangle
background correctly, but the text will be scale to the maximum of the X
and Y scales.

EXAMPLE
-------

Let's say you have for PostScript files of four graphs which you want to
tile two-by-two on a single page.  Maybe you'd like to annotate the graphs
by putting a caption at the bottom of each graph.

Normally, you would have to resort to an external tool or write your own
PostScript program.  The **label** canvas item lets you do this through Tk's
canvas widget.  An **label** item displays an image (or rectangle)
representing the encapsulated PostScript file.  It also scales and
translates the LABEL file when the canvas is printed.

SEE ALSO
--------

canvas

KEYWORDS
--------

label, canvas

COPYRIGHT
---------

2017 George A. Howlett. All rights reserved.

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
