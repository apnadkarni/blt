================
blt::combobutton
================

------------------------------------------
Create and manipulate combobutton widgets
------------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::combobutton** *pathName* ?\ *option value* ... ?

DESCRIPTION
-----------

The **blt::combobutton** command creates and manages *combobutton* widgets.
A *combobutton* widget displays button, that when pressed, posts a
**blt::combomenu** widget.

SYNTAX
------

The **blt::combobutton** command creates a new window using the *pathName*
argument and makes it into a *combobutton* widget.  The command has the
following form.

  **blt::combobutton** *pathName* ?\ *option value* ... ?

Additional options may be specified on the command line or in the option
database to configure aspects of the combobutton such as its background color
or scrollbars. The **blt::combobutton** command returns its *pathName*
argument.  There must not already exist a window named *pathName*, but
*pathName*'s parent must exist.

The normal size of the *combobutton* is the computed size to display the
text. You can specify the width and height of the *combobutton* window
using the **-width** and **-height** widget options.

OPERATIONS
----------

All *combobutton* operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *combobutton* widgets:

*pathName* **activate** 
  Redisplays *item* using its active colors and relief.  This typically is
  used by widget bindings to highlight buttons when the pointer is moved
  over items in the menu. Any previously active item is deactivated.
  *Item* may be a label, index, or tag, but may not represent more than one
  menu item.

*pathName* **cget** *option*  
  Returns the current value of the widget configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **configure** ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of the widget.  If no
  *option* is specified, this command returns a list describing all the
  available options for *pathName* (see **Tk_ConfigureInfo** for
  information on the format of this list).  If *option* is specified with
  no *value*, then a list describing the one named option (this list will
  be identical to the corresponding sublist of the value returned if no
  *option* is specified) is returned.  If one or more *option-value* pairs
  are specified, then this command modifies the given widget option(s) to
  have the given value(s); in this case the command returns an empty
  string.  *Option* and *value* are described below.

  Widget configuration options may be set either by the **configure**
  operation or the Tk **option** command.  The resource class is
  **BltComboButton**.  The resource name is the name of the widget::

    option add *BltComboButton.anchor n
    option add *BltComboButton.Anchor e

  The following widget options are available\:

  **-activebackground** *background* 
    Specifies the background color of *pathName* when it is active.
    *Background* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "white".

  **-activeforeground** *colorName* 
    Specifies the color of the text when *pathName* is active.  The
    default is "black".

  **-arrowon** *boolean* 
    Indicates if an arrow is to be displayed on the right side of the text.
    The default is "0".

  **-arrowborderwidth** *numPixels* 
    Specifies the borderwidth of the arrow.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the menu.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-arrowrelief** *reliefName* 
    Specifies the relief of the arrow.  This determines the 3-D effect for
    the arrow.  *ReliefName* indicates how the arrow should appear relative
    to the combobutton window. Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means the arrow should appear to protrude.  The
    default is "raised".
    
  **-arrowwidth** *numPixels* 
    Specifies the width of the arrow.  *NumPixels* is a non-negative value
    and may have any of the forms acceptable to **Tk_GetPixels**.
    If *numPixels* is "0", the width of the arrow is determined from the
    size of the font (see the **-font** option) The default is "0".

  **-background** *background* 
    Specifies the background color of *pathName*.  *Background* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of *pathName*.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the button.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-command** *string* 
    Specifies a TCL command to be executed when a button is invoked:
    either by clicking on the menu item or using the **invoke** operation.
    If *string* is "", then no command is invoked. The default is "".

  **-cursor** *cursorName* 
    Specifies the cursor to be used for the widget. *CursorName* may have
    any of the forms acceptable to **Tk_GetCursor**.  If *cursorName* is "",
    this indicates that the widget should defer to its parent for cursor
    specification.  The default is "".

  **-disabledbackground** *colorName* 
    Specifies the background of *pathName* when it is disabled.  *ColorName*
    may be a color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-disabledforeground** *colorName* 
    Specifies the color of the text when *pathName* is disabled.  The
    default is "grey70".

  **-font** *colorName* 
    Specifies the font of the text.  The default is "{Sans Serif} 11".

  **-foreground** *colorName* 
    Specifies the color of the text.  The default is "black".

  **-height** *numPixels* 
    Specifies the height of the widget.  *NumPixels* is a non-negative
    value indicating the height of *pathName* in pixels.  It may have any of
    the forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is 0, then the height is computed from the size of the
    text. The default is "0".

  **-highlightbackground** *colorName*
    Specifies the color of the traversal highlight region when the
    graph does not have the input focus.  *ColorName* may be a color name
    or the name of a background object created by the **blt::background**
    command.  The default is "grey85".

  **-highlightcolor** *colorName*
    Specifies the color of the traversal highlight region when *pathName*
    has input focus.  *ColorName* may be a color name or the name of a
    background object created by the **blt::background** command. The
    default is "black".

  **-highlightthickness** *numPixels*
    Specifies a non-negative value for the width of the highlight rectangle
    to drawn around the outside of the widget.  *NumPixels* may have any of
    the forms acceptable to **Tk_GetPixels**.  If *numPixels* is "0", no
    focus highlight is drawn around the widget.  The default is "2".

  **-icon** *imageName* 
    Specifies an image to be displayed as an icon on the left of the text
    in *pathName*.  *ImageName* is the name of an Tk photo or BLT picture.
    If *imageName* is "", no icon is displayed. The default is "".

  **-iconvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    name of the image representing the icon.  If *varName* is set to
    another image name, the icon in *pathName* will change accordingly.  If
    *varName* is "", no variable is used. The default is "".

  **-image** *imageName* 
    Specifies the name of a global TCL variable that will be set to the
    name of the image representing the icon of *pathName*.  If *varName* is
    "", no variable is used. The default is "".

  **-justify**  *justifyName*
    Specifies how the text should be justified.  This matters only when the
    button contains more than one line of text. *JustifyName* must be
    "left", "right", or "center".  The default is "center".

  **-menu** *menuName* 
    Specifies the path name of the menu associated with this combobutton.
    *MenuName* must be a **blt::combomenu** widget and a child of
    *pathName*.  If *menuName* is "", no menu will be displayed. The
    default is "".

  **-postcommand** *cmdString* 
    Specifies a TCL command to invoked when the menu is posted.  The
    command will be invoked before the menu is displayed onscreen.  For
    example, this may be used to disable buttons that may not be valid
    when the menu is posted. If *cmdString* is "", no command is invoked.
    The default is "".

  **-postedbackground** *colorName*
    Specifies the background color of *pathName* when its menu is posted.
    *ColorName* may be a color name or the name of a background object
    created by the **blt::background** command. The default is "skyblue4".

  **-postedforeground** *colorName*
    Specifies the text color when *pathName* when its menu is posted.  The
    default is "white".

  **-postedrelief**  *reliefName*
    Specifies the 3-D effect for *pathName* when its menu is posted.
    *ReliefName* indicates how *pathName* should appear relative to the
    window it's packed into. Acceptable values are **raised**, **sunken**,
    **flat**, **ridge**, **solid**, and **groove**. For example, "raised"
    means the button should appear to protrude.  The default is "sunken".

  **-relief** *reliefName* 
    Specifies the 3-D effect for *pathName*.  *ReliefName* indicates how
    the button should appear relative to the window it's packed
    into. Acceptable values are **raised**, **sunken**, **flat**,
    **ridge**, **solid**, and **groove**. For example, "raised" means
    *pathName* should appear to protrude.  The default is "raised".

  **-state** *stateName*
    Specifies one of three states for *pathName*: 

    **normal**
      In normal state *pathName* is displayed using the **-foreground**
      **-background**, and **-relief**  options.

    **disabled**
      Disabled state means that *pathName* should be insensitive: the default
      bindings will not activate or invoke the item.  In this state
      *pathName* is displayed according to the **-disabledforeground** and
      the **-disabledbackground** options.

    **posted**
      The menu associated with *pathName* is posted.
      *pathName* is displayed according to the **-postedforeground**,
      the **-postedbackground**, and **-postrelief**  options.

    The default is "normal".

  **-takefocus** *boolean*
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *boolean* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts make the
    decision whether to focus on the window.  The default is "".

  **-text** *textString* 
    Specifies the text to be display in *pathName*. If *textString* is not
    "", this option overrides the **-textvariable** option.  The default is
    "".

  **-textvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    text of *pathName*.  If *varName* is set to another value, the text in
    *pathName* will change accordingly. If *varName* is "", no variable is
    used. The default is "".

  **-underline** *charIndex*
    Specifies the index of the character to be underlined when displaying
    *pathName*.  In addition the underlined character is used in the
    *combobutton* widget's bindings.  When the key associated with the
    underlined character is pressed, the button is invoked.  *CharIndex* is
    the index of the character in the text, starting from zero.  If
    *charIndex* is not a valid index, no character is underlined. The
    default is -1.

  **-width** *numPixels*
   Specifies the width of the widget*.  *NumPixels* is a non-negative value
   indicating the width of *pathName*. The value may have any of the forms
   accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
   *numPixels* is "0", the width of *pathName* is computed from its text.
   The default is "".

*pathName* **deactivate** 
  Redisplays *pathName* using its normal colors.  This typically is used by
  widget bindings to un-highlight *pathName* as the pointer is 
  moved away from the button.

*pathName* **invoke** 
  Invokes the TCL command specified by the **-command** option. 
  
*pathName* **post** 
  Posts the menu associated with the widget (see the **-menu** option)
  to be displayed on the screen. The menu is displayed underneath
  the combobutton window.

*pathName* **unpost**
  Unposts the *combobutton* window so it is no longer displayed onscreen.  

DEFAULT BINDINGS
----------------

There are several default class bindings for *combobutton* widgets.

**Enter** 
  The button activates whenever the pointer passes over the button window.
**Leave**
  The button deactivates whenever the pointer leaves the button window.
**ButtonPress-1**
  Pressing button 1 over the combobutton posts its associated combomenu
  if one is specified. The relief  of the button  changes to raised and
  its associated menu is posted under the combobutton.

**B1-Motion**
  If the mouse is dragged down into the menu with the button still down,
  and if the mouse button is then released over an entry in the menu, the
  combobutton is unposted and the menu item is invoked.

  If button 1 is pressed over a combobutton and then dragged over some
  other combobutton, the original combobutton unposts itself and the new
  combobutton posts.

**ButtonRelease-1**
  If button 1 is pressed over a combobutton and then released over that
  combobutton, the combobutton stays posted: you can still move the mouse
  over the menu and click button 1 on an entry to invoke it.  Once a menu
  item has been invoked, the combobutton unposts itself.

  If button 1 is pressed over a combobutton and released outside any
  combobutton or menu, the combobutton unposts without invoking any menu
  item.

**Alt-KeyPress-**\ *key*
  If the **-underline** option has been specified then keyboard traversal
  may be used to post the combobutton's menu: Alt+\ *key*, where *key* is the
  underlined character (or its lower-case or upper-case equivalent), may be
  typed in any window under the combobutton's toplevel to post the
  combobutton's menu.

**KeyPress-Space** or  **KeyPress-Return**
   If a combobutton has the input focus, the space and  return  keys
   post the combobutton's menu.

The behavior of combobuttons can be changed by defining new bindings for
individual widgets or by redefining the class bindings.

EXAMPLE
-------

You create a *combobutton* widget with the **blt::combobutton** command.

 ::

    package require BLT

    # Create a new combobutton and combomenu.

    blt::combobutton .file \
	-textvariable "myTextVar" \
	-iconvariable "myIconVar" \
	-menu .file.m 

    blt::combomenu .file.m  \
	-textvariable "myTextVar" \
	-iconvariable "myIconVar"
    .file.m add -text "New" -accelerator "Ctrl+N" -underline 0 \
        -icon $image(new_window)

Please note the following:

1. The  **blt::combomenu** widget doesn't have to already exist when you
   specify the **-menu** option.

2. Both the **blt::combomenu** and **blt::combobutton** widgets have
   **-textvariable** and **-iconvariable** options that let them change
   the text and icon through TCL variables.

3. You can't use a Tk **menu** with *combobutton*\ s.  The menu must
   be a **blt::combomenu** widget.

DIFFERENCES WITH TK MENUS
-------------------------

The **blt::combobutton** widget has the following differences with the Tk
**menubutton** widget.

1. *Combobuttons* can not post by a Tk **menu**.

   
KEYWORDS
--------

combobutton, widget

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
