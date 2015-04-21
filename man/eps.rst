
===============
eps
===============

------------------------------------
Encapsulated PostScript canvas item.
------------------------------------

:Author: George A Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

.. TODO: authors and author with name <email>

SYNOPSIS
--------

*canvasName* **create eps** *x* *y* ?\ *option* *value* ... ?

DESCRIPTION
-----------

The **eps** canvas item lets you place encapulated PostScript (EPS) on a
canvas, controlling its size and placement.  The EPS item is displayed
either as a solid rectangle or a preview image.  The preview image is
designated in one of two ways: 1) the EPS file contains an ASCII
hexidecimal preview, or 2) a Tk photo or BLT picture image.  When the
canvas generates PostScript output, the EPS will be inserted with the
proper translation and scaling to match that of the EPS item. So can use
the canvas widget as a page layout tool.

SYNTAX
------

*canvasName* **create eps** *x* *y* ?\ *option* *value* ... ?

  The **eps** item creates a new canvas item. *CanvasName* is the name of a
  **canvas** widget.  You must supply the X-Y coordinate of the new eps
  item.  How the coordinate is exactly interpretered is controlled by the
  **-anchor** option (see below).

  Additional options may be specified on the command line to configure
  aspects of the eps item such as its color, stipple, and font.  The
  following *option* and *value* pairs are valid.

  **-anchor** *anchor*
     Specifies how to position the EPS item relative to its X-Y coordinate.
     The default is "center".

  **-background** *colorName*
     Sets the background color of the EPS rectangle.

  **-borderwidth** *numPixels*
     Sets the width of the 3-D border around the outside edge of the item.
     The **-relief** option determines if the border is to be drawn.
     The default is "0".

  **-file** *fileName*
     Specifies the name of the EPS file.  The first line of an EPS file
     must start with "%!PS" and contain a "EPS" version specification.  The
     other requirement is that there be a "%%BoundingBox:" entry which
     contains four integers representing the lower-left and upper-right
     coordinates of the area bounding the EPS.  The default is "".

  **-font** *fontName* 
     Specifies the font of the title. The default is "{Sans Serif} 9".

  **-foreground** *colorName*
     Specifies the foreground color of the EPS rectangle.  The option
     matters only when the **-stipple** option is set.  The default is
     "white".

  **-height** *numPixels*
     Specifies the height EPS item.  If *numPixels* is "0", then the
     height is determined from the PostScript "BoundingBox:" entry in the
     EPS file.  The default is "0".

  **-image** *imageName*
     Specifies the name of a Tk photo or BLT picture image to be displayed
     as in the item as a preview image.  This option overrides any preview
     specification found in the EPS file.  The default is "".

  **-justify** *justify*
     Specifies how the title should be justified.  This matters only when
     the title contains more than one line of text. *Justify* must be
     "left", "right", or "center".  The default is "center".

  **-relief** *relief*
     Specifies the 3-D effect for the EPS item.  *Relief* specifies how the
     item should appear relative to canvas; for example, "raised" means the
     item should appear to protrude.  The default is "flat".

  **-shadowcolor** *colorName*
     Specifies the color of the drop shadow used for the title.  The option
     with the **-shadowoffset** option control how the title's drop shadow
     appears.  The default is "grey".

  **-shadowoffset** *numPixels*
     Specifies the offset of the drop shadow from the title's text.  If
     *numPixels* is "0", no shadow will be seen.  The default is "0".

  **-showimage** *boolean*
     Indicates whether to display the image preview (if one exists), or a
     simple rectangle.  The default is "yes".

  **-stipple** *bitmap*
     Specifies a bitmap to used to stipple the rectangle representing the
     EPS item.  The default is "".

  **-title** *titleString*
     Sets the title of the EPS item.  If *titleString* is "", then the
     title specified by the PostScript "Title:" entry is used.  You can set
     the string a single space to display no title.  The default is "".

  **-titleanchor** *anchor*
     Tells how to position the title within EPS item.  The default is "n".

  **-titlecolor** *colorName*
     Specifies the color of the title.  The default is "white".

  **-titlerotate** *degrees*
     Sets the rotation of the title.  *Degrees* is a real number
     representing the angle of rotation.  The title is first rotated in
     space and then placed according to the **-titleanchor** position.
     The default rotation is "0.0".

  **-width** *numPixels*
     Specifies the width EPS item.  If *numPixels* is "0", then the width
     is determined from the PostScript "BoundingBox:" entry in the EPS
     file.  The default is "0". 

EXAMPLE
-------

Let's say you have for PostScript files of four graphs which you want to
tile two-by-two on a single page.  Maybe you'd like to annotate the graphs
by putting a caption at the bottom of each graph.

Normally, you would have to resort to an external tool or write your own
PostScript program.  The **eps** canvas item lets you do this through Tk's
canvas widget.  An **eps** item displays an image (or rectangle)
representing the encapsulated PostScript file.  It also scales and
translates the EPS file when the canvas is printed.

SEE ALSO
--------

canvas

KEYWORDS
--------

eps, canvas

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
