================
blt::comboentry
================

------------------------------------------
Create and manipulate comboentry widgets
------------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::comboentry** *pathName* ?\ *option value* ... ?

DESCRIPTION
-----------

The **blt::comboentry** command creates and manages *comboentry* widgets.
A *comboentry* widget displays button, that when pressed, posts a
**blt::combomenu** widget.

The *comboentry* widget can be used as a normal entry widget. It
has undo and redo capabilities, a configurable X button, and a
icon.

SYNTAX
------

The **blt::comboentry** command creates a new window using the *pathName*
argument and makes it into a *comboentry* widget.  The command has the
following form.

  **blt::comboentry** *pathName* ?\ *option value* ... ?

Additional options may be specified on the command line or in the option
database to configure aspects of the *comboentry* such as its background
color or scrollbars. The **blt::comboentry** command returns its *pathName*
argument.  There must not already exist a window named *pathName*, but
*pathName*'s parent must exist.

The normal size of the *comboentry* is the computed size to display the
text. You can specify the width and height of the *comboentry* window using
the **-width** and **-height** widget options.

ELEMENTS
--------

The *comboentry* widget is composed of the followung elements.

**text**
  This is the region in the widget where the text is displayed.
**arrow**
  The arrow button is displayed on the far right side of the widget. It is
  used to post the associated menu.
**button**
  The X button is displayed on the right, before the arrow.  It is
  typically used to clear the text.

The **identify** operation is used to determine the element the mouse is
currently over.

CHARACTER INDICES
-----------------

Character positions in the text can be designated in the following forms.

*number*
  The index of the character.  The index of the first character is 0.

**anchor**
  The index of the character anchoring of the selection.

**end**
  The index of the next character after the last character in the text.

**next**
  The index of the next character in the text from the insertion cursor.

**previous**
  The index of the previous character in the text from the insertion cursor.  

**sel.first**
  The index of the first character in the selection.  If there are no
  selected characters, the index is "-1".

**sel.last**
  The index of the last character in the selection.  If there are no
  selected characters, the index is "-1".

**@**\ *x*\
  The index of the character that is located at the given x-coordinate.
  *X* is relative to the *comboentry* window.  If no character is at that
  point, then the index is "-1".

OPERATIONS
----------

All *comboentry* operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *comboentry* widgets:

*pathName* **activate** *itemName*
  Redisplays *item* using its active colors and relief.  This typically is
  used by widget bindings to highlight buttons when the pointer is moved
  over items in the menu. Any previously active item is deactivated.
  *ItemName* may be "button" or "arrow".

*pathName* **button activate** 
  Redisplays the button using its active colors and relief.  This typically
  is used by widget bindings to highlight the button when the pointer is
  moved over it.

*pathName* **button cget**  *option*
  Returns the current value of the configuration option given by *option*.
  *Option* may have any of the values accepted by the **configure**
  operation described below.

*pathName* **button configure**  ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Query or modify the configuration options of the button element.  If no
  *option* is specified, returns a list describing all of the available
  options for the button.  If *option* is specified with no *value*, then the
  command returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*\ -\ *value* pairs are
  specified, then the command modifies the given button option(s) to have
  the given value(s); in this case the command returns an empty string.
  *Option* and *value* are described below.

  **-activebackground** *bgName* 
    Specifies the background color of the button when it is active.
    *BgName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "red".

  **-activeforeground** *colorName* 
    Specifies the foreground color of the button when it is active.  The
    default is "white".

  **-activerelief** *reliefName* 
    Specifies the relief of the button when it is active.  This determines
    the 3-D effect for the arrow.  *ReliefName* indicates how the button
    should appear relative to the *comboentry* window. Acceptable values are
    **raised**, **sunken**, **flat**, **ridge**, **solid**, and
    **groove**. For example, "raised" means the button should appear to
    protrude.  The default is "raised".

  **-background** *bgName* 
    Specifies the background color of the button.  *BgName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "lightblue".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of the button.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the button.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-command** *cmdString* 
    Specifies a TCL command to be executed when a button is invoked:
    either by clicking on the menu item or using the **invoke** operation.
    If *cmdString* is "", then no command is invoked. The default is "".

  **-foreground** *colorName* 
    Specifies the foreground color of the button.  The default is
    "lightblue2".

  **-relief** *reliefName* 
    Specifies the 3-D effect for the button.  *ReliefName* indicates how
    the button should appear relative to the *comboentry* window.
    Acceptable values are **raised**, **sunken**, **flat**, **ridge**,
    **solid**, and **groove**. For example, "raised" means the button
    should appear to protrude.  The default is "raised".

*pathName* **button deactivate** 
  Redisplays the button using its normal colors and relief.  

*pathName* **button invoke** 
  Invokes a TCL command specified by *widget*'s **-xbuttoncommand** or
  button's **-command** option.  This is typically called from the widget's
  bindings when the user clicks on the button.

*pathName* **cget** *option*  
  Returns the current value of the widget configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **closest** *x*  
  Returns the index of the character closest to the given screen
  coordinate.  *X* is screen coordinates relative to the *comboentry*
  
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
  **BltComboentry**.  The resource name is the name of the widget::

    option add *BltComboentry.anchor activebackground white
    option add *BltComboentry.Anchor ActiveForeground black

  The following widget options are available\:

  **-activearrowrelief** *reliefName* 
    Specifies the 3-D effect for the arrow.  *ReliefName* indicates how the
    arrow should appear relative to the widget window. Acceptable values
    are **raised**, **sunken**, **flat**, **ridge**, **solid**, and
    **groove**. For example, "raised" means the arrow should appear to
    protrude.  The default is "raised".

  **-activebackground** *bgName* 
    Specifies the background color of *pathName* when it is active.
    *BgName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "white".

  **-activeforeground** *colorName* 
    Specifies the color of the text when *pathName* is active.  The
    default is "black".

  **-arrowborderwidth** *numPixels* 
    Specifies the borderwidth of the arrow.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the menu.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-arrowpad** *numPixels*
    Specifies the amount of padding for the left and right sides of the
    arrow button.  *NumPixels* is a non-negative value and may have any of
    the forms acceptable to **Tk_GetPixels**.  The default is "0".
    
  **-arrowrelief** *reliefName* 
    Specifies the relief of the arrow.  This determines the 3-D effect for
    the arrow.  *ReliefName* indicates how the arrow should appear relative
    to the *comboentry* window.  Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means the arrow should appear to protrude.  The
    default is "raised".
    
  **-arrowwidth** *numPixels* 
    Specifies the width of the arrow.  *NumPixels* is a non-negative value
    and may have any of the forms acceptable to **Tk_GetPixels**.
    If *numPixels* is "0", the width of the arrow is determined from the
    size of the font (see the **-font** option) The default is "0".

  **-background** *bgName** 
    Specifies the background color of *pathName*.  *BgName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of *pathName*.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the
    *comboentry* window.  *NumPixels* may have any of the forms acceptable
    to **Tk_GetPixels**.  The default is "1".

  **-command** *cmdString* 
    Specifies a TCL command to be executed when the widget's arrow is
    clicking on. If *cmdString* is "", then no command is invoked. The
    default is "".

  **-cursor** *cursorName* 
    Specifies the cursor to be used for the *comboentry*
    widget. *CursorName* may have any of the forms acceptable to
    **Tk_GetCursor**.  If *cursorName* is "", this indicates that the
    widget should defer to its parent for cursor specification.  The
    default is "".

  **-disabledbackground** *bgName* 
    Specifies the background of *pathName* when it is disabled.  *BgName*
    may be a color name or the name of a background object created by the
    **blt::background** command.  The default is "grey90".

  **-disabledforeground** *colorName* 
    Specifies the color of the text when *pathName* is disabled.  The
    default is "grey70".

  **-editable** *boolean* 
    Indicates whether the text can be edited.  The default is "1".
    
  **-exportselection** *boolean* 
    Indicates whether selections are to be exported and copied to the
    clipboard.  The default is "0".

  **-font** *colorName* 
    Specifies the font of the text.  The default is "{Sans Serif} 10".

  **-foreground** *colorName* 
    Specifies the color of the text.  The default is "black".

  **-height** *numPixels* 
    Specifies the height of the widget.  *NumPixels* is a non-negative
    value indicating the height of *pathName* in pixels.  It may have any of
    the forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is 0, then the height is computed from the size of the
    text. The default is "0".

  **-hidearrow** *boolean* 
    Indicates whether to display the arrow that is used to post to the
    menu.  The default is "0".
    
  **-highlightbackground** *bgName*
    Specifies the color of the traversal highlight region when the
    *comboentry* does not have the input focus.  *BgName* may be a color
    name or the name of a background object created by the
    **blt::background** command.  If *bgName* is "", the widget's
    background color (see the **-background** option) is used.  The default
    is "".

  **-highlightcolor** *bgName*
    Specifies the color of the traversal highlight region when *pathName*
    has input focus.  *BgName* may be a color name or the name of a
    background object created by the **blt::background** command. The
    default is "black".

  **-highlightthickness** *numPixels*
    Specifies a non-negative value for the width of the highlight rectangle
    to drawn around the outside of the widget.  *NumPixels* may have any of
    the forms acceptable to **Tk_GetPixels**.  If *numPixels* is "0", no
    focus highlight is drawn around the widget.  The default is "2".

  **-icon** *imageName* 
    Specifies an image to be displayed as an icon to the left of the text.
    *ImageName* is the name of an Tk photo or BLT picture. If *imageName*
    is "", no icon is displayed. The default is "".

  **-iconvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    name of the image representing the icon.  If *varName* is set to
    another image name, the icon in *pathName* will change accordingly.  If
    *varName* is "", no variable is used. The default is "".

  **-iconwidth** *numPixels* 
    Specifies the width of the icon.  *NumPixels* may have any of the forms
    acceptable to **Tk_GetPixels**.  If *numPixels* is "0", the width of
    the icon is set from the width of the image.  The default is "0".
    
  **-image** *imageName* 
    Specifies an image to be displayed instead of the text for the *combentry*.
    *ImageName* is the name of an Tk photo or BLT picture.
    If *imageName* is "", the text is displayed. The default is "".

  **-insertcolor** *colorName* 
    Specifies the color of the insertion cursor.  The default is "black".
    
  **-insertofftime** *milliseconds* 
    Specifies the number of milliseconds the insertion cursor should remain
    "off" in each blink cycle.  If this *milliseconds* is zero then the
    cursor will not blink: it is on all the time. The default is "300".

  **-insertontime** *milliseconds* 
    Specifies the number of milliseconds the insertion cursor should remain
    "on" in each blink cycle.  If *milliseconds* is "0", no insertion cursor
    will be displayed.  The default is "600".

  **-menu** *menuName* 
    Specifies the path name of the menu associated with this *comboentry*.
    *MenuName* must be a **blt::combomenu** widget and a child of
    *pathName*.  If *menuName* is "", no menu will be displayed. The
    default is "".

  **-postcommand** *cmdString* 
    Specifies a TCL command to invoked when the menu is posted.  The
    command will be invoked before the menu is displayed onscreen.
    If *cmdString* is "", no command is invoked. The default is "".

  **-relief** *reliefName* 
    Specifies the 3-D effect for *pathName*.  *ReliefName* indicates how
    the *comboentry* should appear relative to the window it's packed into.
    Acceptable values are **raised**, **sunken**, **flat**, **ridge**,
    **solid**, and **groove**. For example, "raised" means *pathName*
    should appear to protrude.  The default is "raised".

  **-selectbackground** *bgName* 
    Specifies the color of the rectangle surrounding selected text.
    *BgName* may be a color name or the name of a background object created
    by the **blt::background** command.  The default is "skyblue4".

  **-selectborderwidth** *numPixels* 
    Specifies the borderwidth of the selected rectangle.  *NumPixels* is a
    non-negative value indicating the width of the 3-D border drawn around
    the selected text.  *NumPixels* may have any of the forms acceptable to
    **Tk_GetPixels**.  If *numPixels* is "0", no 3-D relief is drawn.
    The default is "0".

  **-selectcommand** *cmdString*
    Specifies a TCL command to be invoked the selection changes.
    If *cmdString* is "", then no command is invoked. The default is "".

  **-selectforeground** *colorName* 
    Specifies the color of selected text.  The default is "white".

  **-selectrelief** *reliefName*
    Specifies the 3-D effect for the rectangle surrounding the selected
    text.  *ReliefName* indicates how the rectangle should appear relative
    to the normal text. Acceptable values are **raised**, **sunken**,
    **flat**, **ridge**, **solid**, and **groove**. For example, "raised"
    means the rectangle should appear to protrude.  The default is "flat".

  **-show** *cipherChar*
    Specifies a character to displayed instead of the actual characters in
    the text.  This may be used for displaying passwords. If *cipherChar*
    is "", then the normal chararcters are displayed .  The default is "".

  **-state** *state*
    Specifies one of three states for *pathName*: 

    **normal**
      Display the widget in its normal colors and relief (see the
      **-foreground**, **-background**, and **-relief** options).

    **disabled**
      Display the widget in its disabled colors (see the
      **-displayforeground**, **-disabledbackground** options).  Disabled
      state means that *pathName* should be insensitive: the default
      bindings will not activate or invoke the item.

    **posted**
      The menu associated with *pathName* is posted.
      The widget* is displayed according to the **-postedforeground**,
      the **-postedbackground**, and **-postrelief**  options.

    The default is "normal".

  **-takefocus** *focusBool*
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *focusBool* is
    "0", this means that this window should be skipped entirely during
    keyboard traversal.  "1" means that the this window should always
    receive the input focus.  An empty value means that the traversal
    scripts make the decision whether to focus on the window.  The default
    is "1".

  **-text** *textString* 
    Specifies the text to be display in *pathName*. If *textString* is not
    "", this option overrides the **-textvariable** option.  The default is
    "".

  **-textbackground** *bgName* 
    Specifies the background color of the text area.  *BgName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-textfocusbackground** *bgName* 
    Specifies the background color of the text area when the widget has
    focus.  *BgName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "white".

  **-textfocusforeground** *colorName* 
    Specifies the color of the text when the widget has focus.  The default
    is "black".
    
  **-textforeground** *colorName* 
    Specifies the color of the text.  The default is "black".

  **-textvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    text of *pathName*.  If the value of *varName* is changed, the text in
    *pathName* will change accordingly. If *varName* is "", no variable is
    used. The default is "".

  **-textwidth** *numChars* 
    Specifies the preferred width of widget in terms of characters.
    If *numChars* is "0", then the **-width** option is used to determine
    the width of the widget. The default is "0".

  **-width** *numPixels*
    Specifies the width of the widget*.  *NumPixels* is a non-negative
    value indicating the width of *pathName*. The value may have any of the
    forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is "0", the width of *pathName* is computed from its text.
    The default is "".

  **-xbutton** *boolean* 
    Indicates whether to display the X button. The default is "0".
    
  **-xbuttoncommand** *cmdString* 
    Specifies a TCL command to be executed when the X button is
    invoked: either by clicking on the button or using the button's
    **invoke** operation.  If *cmdString* is "", then no command is
    invoked. The default is "".

  **-xscrollcommand** *cmdPrefix*
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window
    changes, the widget will generate a TCL command by concatenating the
    scroll command and two numbers. If this option is not specified, then
    no command will be executed.
    
  **-xscrollincrement** *numPixels*
    Sets the horizontal scrolling unit. This is the distance the editor is
    scrolled horizontally by one unit. *NumPixels* is a non-negative value
    indicating the width of the 3-D border drawn around the editor. The
    value may have any of the forms accept able to **Tk_GetPixels**.  The
    default is "2".
  
*pathName* **deactivate** 
  Redisplays *pathName* using its normal colors.  This typically is used by
  widget bindings to un-highlight *pathName* as the pointer is 
  moved away from the button.

*pathName* **delete** *firstIndex* ?\ *lastIndex*\ ?
  Deletes one or more characters. *FirstIndex* describes index of the first
  character to be deleted.  If a *lastIndex* argument is present then
  the characters from *firstIndex* to just before *lastIndex* are deleted.
  For example, if *firstIndex* is "0" and *lastIndex* is "2", the first
  two characters are deleted.
  
*pathName* **get** ?\ *firstIndex* *lastIndex*\ ?
  Returns the text from the widget.  But default, the entire text is
  returned. But if *firstIndex* and *lastIndex* arguments are present, they
  indicate the range of characters to be returned.

*pathName* **icursor** *charIndex*
  Specifies the location of the insertion cursor.  *CharIndex* is the index
  of character before which the insertion cursor will be
  placed. *CharIndex* may be in any of the forms described in `CHARACTER
  INDICES`_.

*pathName* **identify** *x* *y*
  Returns the name of the element under the point given by *x* and *y*
  (such as **button** or **arrow**), or an empty string if the point does
  not lie in any element of the *comboentry* widget.  *X* and *y* must be
  screen coordinates relative to the *comboentry* widget.

*pathName* **index** *charIndex*
  Returns the index of *charIndex*. *CharIndex* may be in any of the forms
  described in `CHARACTER INDICES`_. If *charIndex* does represent a valid
  character index, "-1" is returned.
  
*pathName* **insert** *charIndex* *string*
  Inserts the characters from string into the text at *charIndex*. If
  *charIndex* is "end", the characters are appended.
  
*pathName* **invoke** 
  Invokes the TCL command specified by the **-command** option.  
  
*pathName* **post** 
  Posts the menu associated with the widget (see the **-menu** option)
  to be displayed on the screen. The menu is displayed directly below
  the *comboentry* window.

*pathName* **redo** 
  Re-applies the last reverted change.  This command only has effect if the
  last command was a **undo** operation. The text and insertion cursor are
  possibly changed.
  
*pathName* **scan dragto** *x* *y*
  This command computes the difference between *x* and *y* and the
  coordinates to the last **scan mark** command for the widget.  It then
  adjusts the view by 10 times the difference in coordinates.  *X* and *y*
  are screen coordinates relative to *comboentry* window.  This command is
  typically associated with mouse motion events in the widget, to produce
  the effect of dragging the item list at high speed through the window.
   
*pathName* **scan mark** *x* *y*
  Records *x* and *y* and the current view in the *comboentry* window; to
  be used with later **scan dragto** commands. *X* and *y* are screen
  coordinates relative to editor window.  Typically this command is
  associated with a mouse button press in the widget.

*pathName* **see** *charIndex*
  Scrolls the editor so that character at *charIndex* is visible in the
  *comboentry* widget's window. *CharIndex* may be in any of the forms
  described in `CHARACTER INDICES`_.

*pathName* **selection adjust** *charIndex*
  Sets the end of the selection nearest to the character given by
  *charIndex*, and adjust that end of the selection to be at *charIndex*
  (i.e. including but not going beyond *charIndex*).  The other end of the
  selection is made the anchor point for future select to commands.  If no
  characters are currently selected, then a new selection is created to
  include the characters between *charIndex* and the most recent selection
  anchor point, inclusive.

*pathName* **selection clear**
  Clears the selection.  No characters are selected.

*pathName* **selection from** *charIndex*
  Sets the selection anchor point to just before the character given by
  *charIndex*.  

*pathName* **selection present**
  Indicates if any characters are currently selected.  Returns "1" if
  there is are characters selected and "0" if nothing is selected.

*pathName* **selection range** *firstIndex* *lastIndex*
  Sets the selection to include the characters starting with *firstIndex*
  and ending just before *lastIndex* .  If *lastIndex* is less than of
  equal to *firstIndex*, then the selection is cleared.

*pathName* **selection to** *charIndex*
  If *charIndex* is before the anchor point, sets the selection to the
  characters from *charIndex* up to but not including the anchor point.  If
  *charIndex* is the same as the anchor point, do nothing.  If *charIndex*
  is after the anchor point, set the selection to the characters from the
  anchor point up to but not including *charIndex*.  The anchor point is
  determined by the most recent select from or select adjust command in
  this widget.  If the selection is not in this widget then a new selection
  is created using the most recent anchor point specified for the widget.

*pathName* **undo**
  Undoes the last change.  The text and insertion cursor are reverted
  to what there were before the last edit.

*pathName* **unpost**
  Unposts the menu associated with *comboentry* widget (see the **-menu**
  option) so it is no longer displayed on-screen.

*pathName* **xview moveto** fraction
  Adjusts the horizontal view in the *combontry* window so the portion of
  the editor starting from *fraction* is displayed.  *Fraction* is a number
  between 0.0 and 1.0 representing the position horizontally where to
  start displaying the editor.
   
*pathName* **xview scroll** *number* *what*
  Adjusts the view in the window horizontally according to *number* and
  *what*.  *Number* must be an integer.  *What* must be either "units" or
  "pages".  If *what* is "units", the view adjusts left or right by
  *number* units.  The number of pixel in a unit is specified by the
  **-xscrollincrement** option.  If *what* is "pages" then the view
  adjusts by *number* screenfuls.  If *number* is negative then the view
  if scrolled left; if it is positive then it is scrolled right.

DEFAULT BINDINGS
----------------

There are several default class bindings for *comboentry* widgets.

**<Motion>** 
  Activates/deactivates the *comboentry* elements as the mouse pointer
  is passed over them.

**<ButtonPress-1>**
  Pressing button 1 over the *comboentry* arrow posts its associated
  combomenu if one is specified. The relief of the arrow changes to raised
  and its associated menu is posted under the *comboentry*.

  Pressing button 1 over the *comboentry* X button clears the text
  in the widget.
  
  Clicking mouse button 1 positions the insertion cursor just before the
  character underneath the mouse cursor, sets the input focus to this
  widget, and clears any selection in the widget.  Dragging with mouse
  button 1 strokes out a selection between the insertion cursor and the
  character under the mouse.

**<Triple-ButtonPress-1>**
  Triple-clicking with mouse button 1 selects all of the text in the entry
  and positions the insertion cursor at the end of the line.

**<Shift-ButtonPress-1>**
  The ends of the selection can be adjusted by dragging with mouse button
  1 while the Shift key is down; this will adjust the end of the selection
  that was nearest to the mouse cursor when button 1 was pressed.

**<Control-ButtonPress-1>**
  Clicking mouse button 1 with the Control key down will position the
  insertion cursor in the entry without affecting the selection.

**<B1-Motion>**
  If the mouse is dragged down into the menu with the button still down,
  and if the mouse button is then released over an entry in the menu, the
  *comboentry* is unposted and the menu item is invoked.

  If a selection was started, it is continued, adding characters that
  the mouse passes over.
  
**<ButtonRelease-1>**
  If button 1 is pressed over a *comboentry* and then released over that
  *comboentry*, the *comboentry* stays posted: you can still move the mouse
  over the menu and click button 1 on an entry to invoke it.  Once a menu
  item has been invoked, the *comboentry* unposts itself.

  If button 1 is pressed over a *comboentry* and released outside any
  *comboentry* or menu, the *comboentry* unposts without invoking any menu
  item.

**<KeyPress>**
  If any normal printing characters are typed in an entry, they are
  inserted at the point of the insertion cursor.

**<KeyPress-Return>**
  If a *comboentry* has the input focus, the return  key posts
  menu associated with the *comboentry*.

**<ButtonPress-2>**
  The view in the entry can be adjusted by dragging with mouse button 2.
  If mouse button 2 is clicked without moving the mouse, the selection is
  copied into the entry at the position of the mouse cursor.

**<KeyPress-Left>** and **<KeyPress-Right>**
  The Left and Right keys move the insertion cursor one character to the
  left or right; they also clear any selection in the entry and set the
  selection anchor.

**<Shift-KeyPress-Left>** and **<Shift-KeyPress-Right>**
  If Left or Right  is  typed with the Shift key down, then the
  insertion cursor moves and the selection is extended to include the
  new  character.

**<Control-KeyPress-Left>** and **<Control-KeyPress-Right>**
  Control-Left  and  Control-Right move the insertion cursor by words.

**<Control-Shift-KeyPress-Left>** and **<Control-Shift-KeyPress-Right>**
  Control-Shift-Left and Control-Shift-Right move the insertion cursor by
  words and also extend the selection.

**<Control-KeyPress-b>** and **<Control-KeyPress-f>**
  Control-b and Control-f behave the same as Left and Right, respectively.
   
**<Alt-KeyPress-b>** and **<Alt-KeyPress-f>**
  Meta-b and Meta-f behave the same as Control-Left and Control-Right,
  respectively.

**<KeyPress-Home>** and **<Control-KeyPress-a>**
  The Home key, or Control-a, will move the insertion cursor to the
  beginning of the entry and clear any selection in the entry.

**<Shift-KeyPress-Home>** 
  Shift-Home moves the insertion cursor to the  beginning  of  the
  entry and also extends the selection to that point.

**<KeyPress-End>** and **<Control-KeyPress-e>**
  The End key, or Control-e, will move the insertion cursor to the
  end of the entry and clear any selection in the  entry.

**<Shift-KeyPress-End>**
  Shift-End moves the cursor to the end and extends the selection to that
  point.

**<Control-KeyPress-space>**
  Control-Space sets the selection anchor to the position of the insertion
  cursor.  They do not affect the current selection.

**<Control-Shift-KeyPress-space>**
  Control-Shift-Space adjusts the selection to the current position of the
  insertion cursor, selecting from the anchor to the insertion cursor if
  there was not any selection previously.

**<Control-slash>**
   Control-/ selects all the text in the entry.

**<Control-backslash>**
  Control-\ clears any selection in the entry.

**<KeyPress-Delete>**
  The Delete key deletes the selection, if there is one in the entry.  If
  there is no selection, it deletes the character to the right of the
  insertion cursor.

**<KeyPress-Backspace>** and **<Control-KeyPress-h>**
  The BackSpace key and Control-h delete the selection, if there is one in
  the entry.  If there is no selection, it deletes the character to the
  left of the insertion cursor.

**<Control-KeyPress-d>**
  Control-d deletes the character to the right of the insertion cursor.

**<Alt-KeyPress-d>**
  Meta-d deletes the word to the right of the insertion cursor.

**<Control-KeyPress-k>**
  Control-k deletes all the characters to the right of the insertion cursor.

**<Control-KeyPress-t>**
  Control-t reverses the order of the two characters to the right of the
  insertion cursor.

The behavior of *comboentry* widgets can be changed by defining new
bindings for individual widgets or by redefining the class bindings.

EXAMPLE
-------

You create a *comboentry* widget with the **blt::comboentry** command.

 ::

    package require BLT

    # Create a new comboentry and combomenu.

    blt::comboentry .ce \
	-textvariable "myTextVar" \
	-iconvariable "myIconVar" \
	-menu .ce.m 

    blt::combomenu .ce.menu  \
	-textvariable "myTextVar" \
	-iconvariable "myIconVar"
    .ce.menu add -text "New" -accelerator "Ctrl+N" -underline 0 \
        -icon $image(new_window)

Please note the following:

1. The  **blt::combomenu** widget doesn't have to already exist when you
   specify the **-menu** option.

2. Both the **blt::combomenu** and **blt::comboentry** widgets have
   **-textvariable** and **-iconvariable** options that let them change
   the text and icon through TCL variables.

3. You can't use a Tk **menu** with *comboentry*\ s.  The menu must
   be a **blt::combomenu** widget.

DIFFERENCES WITH TK MENUS
-------------------------

The **blt::comboentry** widget has the following differences with the Tk
**menubutton** widget.

1. A *comboentry* can not post a Tk **menu**.

   
KEYWORDS
--------

comboentry, combomenu, widget

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
