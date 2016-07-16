============
blt::htext
============

---------------------------------------
Create and manipulate hypertext widgets
---------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::htext** *pathName* ?\ *option* *value* ... ?

DESCRIPTION
-----------

The **blt::htext** command creates a new window (given by the 
*pathName* argument) and makes it into a *htext* widget.
Additional options, described above, may be specified on the command line
or in the option database to configure aspects of the widget such as its 
color and font.  At the time this command is invoked, there must not 
exist a window named *pathName*, but *pathName*'s parent must exist.
The **blt::htext** command returns its *pathName*.   

The *htext* widget is hybrid of a non-editable text widget and a geometry
manager (e.g. the packer).  It displays text (optionally read from file) in
a window.  Text can be scrolled either horizontally or vertically using the
*htext* window as a viewport.  In addition, TCL commands can be embedded
into the text which are evaluated as the text is parsed.  Text between
special double characters (percent signs "%%") is immediately passed to the
TCL interpreter for evaluation.

Furthermore, any widget or widget hierarchy can be packed in-line and made
to appear on the current line of the text.  Widgets are packed using the
**append** operation.  All widgets must be children of the *htext* window
and must already exist before packing.  Once a widget has been packed it
cannot be moved to a different position within the text.  Widgets can be
resized but they will remain at the same position within the text.

Before a file or text string is parsed by the *htext* widget, all the
widget's current children are destroyed.  You can reload files or text
without worrying about unmapping or destroying each child window
beforehand.

Setting the either the **-filename** or **-text** configuration option 
will adjust the value of the other.  If both options are set, the file 
takes precedence.  When a new file is read using the **-filename** option,
the value of the **-text** option is reset to the empty string.  Likewise, 
when the **-text** option is set, the string representing the 
**-filename** option is cleared.

FILE FORMAT
-----------

The format of *htext* text file is an ASCII text.  Text enclosed by special
double characters (by default, percent signs "%%") is interpreted and
executed as TCL commands.  The special character may be specified by the
**-specialchar** option.  In the following example of a *htext* file, a
button widget is appended to the text between the words "a" and "which".
The *pathName* of the *htext* widget is ".ht".

 ::

    This will be displayed as normal text. 
    But this will become a %% 
      button .ht.button -text "button" -fg red
      .ht append .ht.button 
    %% which can invoke a TCL command.

INDICES
-------

Some of the widget operations (**selection**, **gotoline**, **search**,
etc.) take one or more indices as arguments.  An index is a string used to
indicate a particular place within the text, such as the first and last
characters in a range to be selected.

An index must have one of the following forms:

*line*\ **.**\ *char*
  Indicates *char*'th character on line *line*.
  Both lines and characters are number from 0, so "0.0" is the
  first beginning of the text.  *Char* may be undesignated.  In
  this case a character position of 0 is assumed.

**@**\ *x*\ **,**\ *y*
  Indicates the character that covers the pixel whose x and y coordinates
  within the text's window are *x* and *y*.

**end**
  Indicates the end of the text.

**anchor**
  Indicates the anchor point for the selection, which is set with the
  **selection** operation.

**sel.first**
  Indicates the first character in the selection.  It is an error to
  use this form if the selection isn't in the entry window.

**sel.last**
  Indicates the character just after the last one in the selection.
  It is an error to use this form if the selection isn't in the
  entry window.

VARIABLES
---------

The following global TCL variables are maintained when an 
**htext** file is parsed.  

**htext(widget)**
  is the pathname of the *htext* widget.

**htext(file)**
  is the name of the file the *htext* widget is currently parsing.  
  It is the empty string when the **-text** option is used.

**htext(line)**
  is the current line number in the text.  

This information might be used to construct hyper links between different
files and/or lines.

SYNTAX
------

The **blt::htext** command creates a new TCL command whose
name is *pathName*.  This command may be used to invoke various
operations on the widget.  It has the following general form:

  *pathName* *oper* ?\ *args* ... ?

*Oper* and *args* determine the exact behavior of the command.

OPERATIONS
----------

The following operations are available for *htext* widgets:

*pathName* **append** *childName* ?\ *option* *value* ... ?
  Embeds the widget *childName* into *pathName*.  *ChildName* is
  the pathname of the widget to be embedded which must be a child of
  *pathName*.  *ChildName* will be positioned in the *htext* widget
  at the current location of the text. If *option* and *value*
  pairs are present, they configure various aspects how *childName*
  appears in *pathName*.  The following options are available.

  **-anchor** *anchorPos*
    Specifies how *childName* will be arranged if there is any extra space in
    the cavity surrounding the window.  For example, if *anchorPos* is
    **center** then the window is centered in the cavity; if *anchorPos* is
    **w** then the window will be drawn such it touches the leftmost edge
    of the cavity. The default is "center".

  **-fill** *fillName*
    Specifies how the *childName* should be stretched to occupy the extra
    space in the cavity surrounding it (if any exists).  *FillName* can
    be one of the following:

    **x**
      The width of *childName* is expanded to fill the cavity.
    **y**
      The height is expanded to fill the cavity.
    **both**
      Both the width and height are expanded.
    **none**
      *ChildName* is not resized.

    The default is "none".

  **-height** *numPixels*
    Sets the height of the cavity surrounding *childName*.  If *numPixels* is
    zero, the height of the cavity will be the same as the requested height
    of *childName*.  If *numPixels* is less than the requested height of
    *childName*, *childName* will be reduced to fit the cavity.  The default is
    "0".

  **-ipadx** *pad*
    Sets the amount of internal padding to be added to the width *childName*.
    *Pad* can be a list of one or two numbers.  If *pad* has two elements,
    the left side of *childName* is extended by the first value and the right
    side by the second value.  If *pad* is just one value, both the left
    and right sides are padded by evenly by the value.  The default is "0".

  **-ipady** *pad*
    Sets an amount of internal padding to be added to the height of
    *childName*.  *Pad* can be a list of one or two numbers.  If *pad* has two
    elements, the top of *childName* is padded by the first value and the
    bottom by the second value.  If *pad* is just one number, both the top
    and bottom are padded evenly by the value.  The default is "0".

  **-justify** *justifyName*
    Justifies *childName* vertically within the cavity containing it in
    relation to the line of text. *JustifyName* is **top**, **bottom**, or
    **center**.  If *justify* is "center" the widget is centered along the
    baseline of the line of text.  The default is "center".

  **-padx** *pad*
    Sets the padding on the left and right sides of *childName*.  *Pad* can be
    a list of one or two numbers.  If *pad* has two elements, the left side
    of *childName* is padded by the first value and the right side by the
    second value.  If *pad* has just one value, both the left and right
    sides are padded evenly by the value.  The default is "0".

  **-pady** *pad*
    Sets the padding above and below *childName*.  *Pad* can be a list of one
    or two numbers.  If *pad* has two elements, the area above *childName* is
    padded by the first value and the area below by the second value.  If
    *pad* is just one number, both the top and bottom are padded by the
    value.  The default is "0".

  **-relheight** *value*
    Specifies the height of the cavity containing *childName* relative to the
    height of *pathName*.  *Value* is real number indicating the ratio of
    the height of the cavity to the height of *pathName*.  As the height of
    *pathName* changes, so will the height of *childName*.  If *value* is 0.0
    or less, the height of the cavity is the requested height *childName*.
    The default is "0.0".

  **-relwidth** *value*
    Specifies the width of the cavity containing *childName* relative to the
    width of *pathName*.  *Value* is real number indicating the ratio of
    the width of the cavity to the width of *pathName*.  As the height of
    *pathName* changes, so will the height of *childName*.  If *value* is 0.0
    or less, the width of the cavity is the requested width of *childName*.
    The default is "0.0".

  **-width** *value*
    Species the width of the cavity containing the child window.  *Value*
    must be in a form accepted by **Tk_GetPixels**.  If *value* is greater
    than zero, the cavity is resized to that width.  If the requested window
    width is greater than the cavity's width, the window will be reduced to
    fit the cavity.  By default, the cavity is requested width of the child
    window.


*pathName* **configure** ?\ *childName*\ ? ?\ *option*\ ? ?\ *value* *option* *value* ... ?
  Queries or modifies the configuration options of the *htext* widget or
  one of its embedded widgets.  If no *childName* argument is present, the
  htext widget itself is configured.  Otherwise *childName* is the pathname
  of a widget already embedded into the htext widget.  Then this command
  configure the options for the embedded widget.

  If *option* isn't specified, a list describing all of the current options
  for *pathName* or *childName* is returned.  If *option* is specified, but
  not *value*, then a list describing the option *option* is returned.  If
  one or more *option* and *value* pairs are specified, then for each pair,
  the htext or embedded window option *option* is set to *value*.

  The following options are valid for the htext widget.

  **-background** *colorName*
    Sets the background to *colorName*.  This default is "white".

  **-cursor** *cursor*
    Specifies the cursor.  The default cursor is "pencil".

  **-filename** *fileName*
    Specifies a *htext* file to be displayed in the window.  If the value
    is the empty string, the **-text** option is used instead.  See the
    section `FILE FORMAT`_ for a description of the *htext* file format.

  **-font** *fontName* 
    Sets the font of the text in the htext widget to *fontName*. The
    default is "{Sans Serif} 9".

  **-foreground** *colorName*
    Sets the text color to *colorName*.  This default is "black".

  **-height** *numPixels*
    Specifies the height of the window. 

  **-linespacing** *numPixels*
    Specifies the spacing between each line of text.  The value must be in
    a form accepted by **Tk_GetPixels**. The default value is 1 pixel.

  **-specialchar** *number*
    Specifies the ASCII value of the special double character delimiters.
    In *htext* files, the text between these special characters is
    evaluated as a block of TCL commands. The default special character is
    the "0x25" (percent sign).

  **-text** *text*
    Specifies the text to be displayed in the htext widget.  *Text* can be
    any valid string of characters. See `FILE FORMAT`_ for a description.

  **-xscrollcommand** *cmdPrefix* 
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  When the view in the htext widget's window changes (or
    whenever anything else occurs that could change the display in a
    scrollbar, such as a change in the total size of the widget's
    contents), the widget invoke *cmdPrefix* concatenated by two numbers.
    Each of the numbers is a fraction between 0 and 1, which indicates a
    position in the document.  If this option is not specified, then no
    command will be executed.

  **-yscrollcommand** *cmdPrefix*
    Specifies the prefix for a command used to communicate with vertical
    scrollbars.  When the view in the htext widget's window changes (or
    whenever anything else occurs that could change the display in a
    scrollbar, such as a change in the total size of the widget's
    contents), the widget invoke *cmdPrefix* concatenated by two numbers.
    Each of the numbers is a fraction between 0 and 1, which indicates a
    position in the document.  If this option is not specified, then no
    command will be executed.

  **-width** *numPixels*
    Specifies the desired width of the viewport window.  If the *pixels* is
    less than one, the window will grow to accommodate the widest line of
    text.

  **-xscrollunits** *numPixels*
    Specifies the horizontal scrolling distance. The default is 10 pixels.

  **-yscrollunits** *numPixels*
    Specifies the vertical scrolling distance. The default is 10 pixels.


*pathName* **gotoline** ?\ *index*\ ?
  Sets or gets the top line of the text.  *Index* must be a valid text
  index (the character offset is ignored).  If an *index* isn't provided,
  the current line number is returned.

*pathName* **scan mark** *position*
  Records *position* and the current view in the text window; used in
  conjunction with the **scan dragto** operation.  *Position* must be in
  the form "**@**\ *x*\ ,\ *y*", where *x* and *y* are window coordinates.
  Typically this command is associated with a mouse button press in the
  widget.  It returns an empty string.

*pathName* **scan dragto** *position*
  Computes the difference between *position* and the position registered in
  the last **scan mark** command for the widget.  The view is then adjusted
  up or down by 10 times the difference in coordinates.  This command is
  can be associated with mouse motion events to produce the effect of
  dragging the text at high speed through the window.  *Position* must be
  in the form "**@**\ *x*\ ,\ *y*", where *x* and *y* are window
  coordinates. The command returns an empty string.

*pathName* **search** *pattern* ?\ *from*\ ? ?\ *to*\ ?
  Returns the number of the next line matching *pattern*.  *Pattern* is a
  string which obeys the matching rules of **Tcl_StringMatch**.  *From* and
  *to* are text line numbers (inclusive) which bound the search.  If no
  match for *pattern* can be found, "-1" is returned.

*pathName* **xview** ?\ *position*\ ?
  Moves the viewport horizontally to the new text x-coordinate position.
  *Position* is the offset from the left side of the text to the current
  position and must be in a form accepted by **Tk_GetPixels**. If
  *position* is not present, the current text position is returned.

*pathName* **yview** ?\ *position*\ ?
  Moves the viewport vertically to the new text y-coordinate position.
  *Position* is the offset from the top of the text to the current position
  and must be in a form accepted by **Tk_GetPixels**. If *position* is not
  present, the current text position is returned.


KEYWORDS
--------

hypertext, widget

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
