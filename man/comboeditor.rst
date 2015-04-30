================
blt::comboeditor
================

------------------------------------------
Create and manipulate comboeditor widgets.
------------------------------------------

:Author: George A Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::comboeditor** *pathName* ?\ *option value* ... ?

DESCRIPTION
-----------

The **blt::comboeditor** command creates and manages *comboeditor* widgets.
The *comboeditor* widget contains an edit-able text area and optional
scrollbars (vertical and/or horizontal).  The scrollbars are automatically
exposed or hidden as necessary when the *comboeditor* widget is resized.
Whenever the *comboeditor* window is smaller horizontally and/or vertically
than the actual menu, the appropiate scrollbar is exposed.

A *comboeditor* is a widget that lets you temporarily edit text.  It is
built to use with other widgets to edit fields, names etc.

SYNTAX
------

The **blt::comboeditor** command creates a new window using the *pathName*
argument and makes it into a *comboeditor* widget.  The command has the
following form.

  **blt::comboeditor** *pathName* ?\ *option value* ... ?

Additional options may be specified on the command line or in the option
database to configure aspects of the comboeditor such as its background color
or scrollbars. The **blt::comboeditor** command returns its *pathName*
argument.  There must not already exist a window named *pathName*, but
*pathName*'s parent must exist.

The normal size of the *comboeditor* is the computed size to display all menu
items. You can specify the maximum and minimum width and height of the
*comboeditor* window using the **-width** and **-height** widget options.
Scrollbars are specified for *comboeditor* by the **-xscrollbar** and
**-yscrollbar** widget options.  The scrollbars must be children of the
*pathName*.  The scrollbars may be specified before they exist.  If the
specified width or height is less than the computed size of the comboeditor,
the appropiate scrollbar is automatically displayed.

CHARACTER INDICES
-----------------

Character positions in the text can be defined by using one of more
indices. An index can be in one of the following forms.

  *indexNumber*
    The index of the character.  Character indices start at 0.
    
  **anchor**
    The character index of the anchor of the selection.

  **down**
    The character index one line down from the insertion cursor.  . 

  **end**
    The index after the the last character.

  **next**
    The next character in the text from the insertion cursor.

  **line.end**
    The end of the current line from the insertion cursor.  

  **line.start**
    The start of the current line from the insertion cursor.  

  **previous**
    The previous character in the text from the insertion cursor.  

  **sel.first**
    The index of the first character in the selection.  If there is
    no selection it is "-1".

  **sel.last**
    The index of the last character in the selection.  If there is
    no selection it is "-1".

  **space.end**
    The end of the current line from the insertion cursor.  

  **space.start**
    The start of the current line from the insertion cursor.  

  **up**
    The character index up one line from the insertion cursor.  

  **word.end**
    The end of the current line from the insertion cursor.  

  **word.start**
    The start of the current line from the insertion cursor.  

  **@**\ *x*\ ,\ *y*
    The character that is location at the specified by *x* and *y*
    (in screen coordinates).  If no character at at that point, then the
    index is "-1".

OPERATIONS
----------

All *comboeditor* operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *comboeditor* widgets:

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
  "BltComboEditor".  The resource name is the name of the widget::

    option add *BltComboeditor.anchor n
    option add *BltComboeditor.Anchor e

  The following widget options are available\:

  **-activeforeground** *colorName* 
    Specifies the color of the label when the menu item is active.  The
    default is "white".

  **-background** *background* 
    Specifies the background of the menu items.  *Background* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of the editor.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the editor.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-command** *cmdPrefix* 
    Specifies a TCL command to be invoked when a widget is invoked:
    either by clicking on the menu item or using the **invoke** operation.
    If *cmdPrefix* is "", then no command is invoked. The default is "".

  **-cursor** *cursorName* 
    Specifies the cursor to be used for the widget. *CursorName* may have
    any of the forms acceptable to **Tk_GetCursor**.  If *cursorName* is "",
    this indicates that the widget should defer to its parent for cursor
    specification.  The default is "".

  **-exportselection** *boolean* 
    Indicates if the selections are to be exported and copied to the clipboard.
    The default is "0".

  **-font** *colorName* 
    Specifies the font of labels in menu items.  The default is "{Sans
    Serif} 11".

  **-foreground** *colorName* 
    Specifies the color of labels in menu items.  The default is "black".

  **-height** *numPixels* 
    Specifies the height in the *comboeditor*.  *NumPixels* can be single
    value or a list.  If *numPixels* is a single value it is a non-negative
    value indicating the height the editor. The value may have any of the
    forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is a 2 element list, then this sets the minimum and maximum
    limits for the height of the menu. The menu will be at least the
    minimum height and less than or equal to the maximum. If *numPixels* is
    a 3 element list, then this specifies minimum, maximum, and nominal
    height or the menu.  The nominal size overrides the calculated height
    of the menu.  If *numPixels* is "", then the height of the menu is
    calculated based on all the menu items.  The default is "".

  **-insertbackground** *colorName* 
    Specifies the color of the insertion cursor.  The default is "black".

  **-insertofftime** *milliseconds* 
    Specifies the color of the insertion cursor.  The default is "black".

  **-insertontime** *milliseconds* 
    Specifies the color of the insertion cursor.  The default is "black".
    
  **-justify** *how* 
    Specifies the color of the insertion cursor.  The default is "black".
    
  **-postcommand** *string* 
    Specifies a TCL command to invoked when the editor is posted.  The
    command will be invoked before the editor is displayed onscreen.  If
    *string* is "", no command is invoked.  The default is "".

  **-readonly** *boolean* 
    Indicates to display but not allow editing of the text.
    The default is "0".

  **-relief** *relief* 
    Specifies the 3-D effect for the menu.  *Relief* indicates how the
    menu should appear relative to the root window; for example, "raised"
    means the menu should appear to protrude.  The default is "raised".

  **-restrictwidth** *option* 
    Specifies how the menu width should be restricted according to the
    parent widget that posted it. *Option* can be one of the following
    "none".

    **max**
      The menu width will be the maximum of the calculated menu width and
      the parent widget width.

    **min**
      The menu width will be the minimum of the calculated menu width and
      the parent widget width.

    **both**
      The menu width will the same as the parent widget width.

    **none**
      Don't restrict the menu width. This is the default.
       
  **-selectbackground** *colorName* 
    Specifies the color of the insertion cursor.  The default is "skyblue4".

  **-selectborderwidth** *numPixels* 
    Specifies the color of the insertion cursor.  The default is "0".
    
  **-selectforeground** *colorName* 
    Specifies the color of selected text.  The default is "white".

  **-selectrelief** *relief* 
    Specifies the relief of the rectangle surrounding selected text.  The
    default is "flat".

  **-show** *boolean* 
    Indicates to display text as circles instead of the text itself.
    The default is "0".

  **-text** *string* 
    Specifies to text to edit. The default is "".

  **-textbackground** *colorName* 
    Specifies the background color of the text area.  The default is
    "white".

  **-textforeground** *colorName* 
    Specifies the background color of the text area.  The default is
    "black".

  **-textwidth** *numCharacters* 
    Specifies the preferred width of widget in terms of characters.
    The default is "0".

  **-unpostcommand** *string*
    Specifies the TCL command to be invoked when the menu is unposted.  If
    *string* is "", no command is invoked. The default is "".

  **-width** *numPixels*
   Specifies the width in the *comboeditor*.  *NumPixels* can be single
   value or a list.  If *numPixels* is a single value it is a non-negative
   value indicating the width the menu. The value may have any of the
   forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
   *numPixels* is a 2 element list, then this sets the minimum and maximum
   limits for the width of the menu. The menu will be at least the minimum
   width and less than or equal to the maximum. If *numPixels* is a 3
   element list, then this specifies minimum, maximum, and nominal width
   or the menu.  The nominal size overrides the calculated width of the
   menu.  If *numPixels* is "", then the width of the menu is calculated
   based on the widths of all the menu items.  The default is "".

  **-xscrollbar** *widget*
    Specifies the name of a scrollbar widget to use as the horizontal
    scrollbar for this menu.  The scrollbar widget must be a child of the
    comboeditor and doesn't have to exist yet.  At an idle point later, the
    comboeditor will attach the scrollbar to widget, effectively packing the
    scrollbar into the menu.

  **-xscrollcommand** *string*
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window
    changes, the widget will generate a Tcl command by concatenating the
    scroll command and two numbers. If this option is not specified, then
    no command will be executed.  The widget's initialization script
    will automatically set this for you.

  **-xscrollincrement** *numPixels*
    Sets the horizontal scrolling unit. This is the distance the menu is
    scrolled horizontally by one unit. *NumPixels* is a non-negative value
    indicating the width of the 3-D border drawn around the menu. The
    value may have any of the forms accept able to **Tk_GetPixels**.  The
    default is "20".

  **-yscrollbar** *widget*
    Specifies the name of a scrollbar widget to use as the vertical
    scrollbar for this menu.  The scrollbar widget must be a child of the
    comboeditor and doesn't have to exist yet.  At an idle point later, the
    comboeditor will attach the scrollbar to widget, effectively packing the
    scrollbar into the menu.

  **-yscrollcommand** *string*
    Specifies the prefix for a command used to communicate with vertical
    scrollbars.  Whenever the vertical view in the widget's window
    changes, the widget will generate a Tcl command by concatenating the
    scroll command and two numbers.  If this option is not specified, then
    no command will be executed.  The widget's initialization script
    will automatically set this for you.

  **-yscrollincrement** *numPixels*
    Sets the vertical scrolling unit.  This is the distance the menu is
    scrolled vertically by one unit. *NumPixels* is a non-negative value
    indicating the width of the 3-D border drawn around the menu. The
    value may have any of the forms accept able to **Tk_GetPixels**.  The
    default is "20".

*pathName* **delete** *firstIndex* ?\ *lastIndex*\ ?
  Deletes one or more characters. *FirstIndex* describes index of the first
  character to be deleted.  If a *lastIndex* argument is present then they
  describe a range of characters to be deleted.

*pathName* **get** ?\ *firstIndex* *lastIndex*\ ?
  Returns the text from the widget.  If *firstIndex* and *lastIndex*
  arguments are present, they describe the region of characters to be
  returned.

*pathName* **icursor** *charIndex* 
  Specifies the location of the insertion cursor.
  *CharIndex* is the index of character before which the insertion cursor
  will be placed.

*pathName* **index** *charIndex* 
  Returns the index of *charIndex*. *CharIndex* may be a label, index, or
  tag, but may not represent more than one menu item.  If *charIndex* does
  represent a valid character index, "-1" is returned.
  
*pathName* **insert after** *item* ?\ *option *value* ... ? 
  Creates a new menu item and inserts it after *item*.  Normally menu items
  are appended to the end of the menu, but this command allows you to
  specify its location. Note that this may change the indices of previously
  created menu items. *Item* may be a label, index, or tag, but may not
  represent more than one menu item. If one or more *option-value* pairs
  are specified, they modifies the given menu item option(s) to have the
  given value(s).  *Option* and *value* are described in the **item
  configure** operation.
  
*pathName* **insert** *charIndex* *string*
  Inserts the string in to the text at *charIndex*.
  
*pathName* **invoke** 
  Invokes a TCL command specified by *widget*'s **-command** option. 
  
*pathName* **post** ?\ *switches* ... ? 
  Arranges for the *pathName* to be displayed on the screen. The position
  of *pathName* depends upon *switches*.

  The position of the *comboeditor* may be adjusted to guarantee that the
  entire widget is visible on the screen.  This command normally returns an
  empty string.  If the **-postcommand** option has been specified, then
  its value is executed as a Tcl script before posting the menu and the
  result of that script is returned as the result of the post widget
  command.  If an error returns while executing the command, then the error
  is returned without posting the menu.

  *Switches* can be one of the following:

  **-align** *how*
    Aligns the menu horizontally to its parent according to *how*.  *How*
    can be "left", "center", or "right".

  **-box** *coordList*
    Specifies the region of the parent window that represent the button.
    Normally comboeditors are aligned to the parent window.  This allows you
    to align the menu a specific screen region.  *CoordList* is a list of
    two x,y coordinates pairs representing the two corners of the box.

  **-cascade** *coordList*
    Specifies how to position the menu.  This option is for
    *cascade* menus. *CoordList* is a list of x and y coordinates
    representing the position of the cascade menu.

  **-popup** *coordList*
    Specifies how to position the menu.  This option is for
    *popup* menus. *CoordList* is a list of x and y coordinates
    representing the position of the popup menu.

  **-window** *window*
    Specifies the name of window to align the menu to.  Normally *comboeditor*s
    are aligned to its parent window.  *Window* is the name of another
    widget.

*pathName* **redo** 
  
*pathName* **scan dragto** *x* *y*
  This command computes the difference between *x* and *y* and the
  coordinates to the last **scan mark** command for the widget.  It then
  adjusts the view by 10 times the difference in coordinates.  This command
  is typically associated with mouse motion events in the widget, to
  produce the effect of dragging the item list at high speed through the
  window.  The return value is an empty string.
   
*pathName* **scan mark** *x* *y*
  Records *x* and *y* and the current view in the menu window; to be used
  with later **scan dragto** commands. *X* and *y* are window coordinates
  (i.e. relative to menu window).  Typically this command is associated
  with a mouse button press in the widget.  It returns an empty string.

*pathName* **see** *charIndex* 
  Scrolls the menu so that *item* is visible in the widget's window.
  *Item* may be a label, index, or tag, but may not represent more than one
  menu item.
  
*pathName* **selection adjust** *charIndex*
  Sets the end of the selection nearest to the character given by
  *charIndex*, and adjust that end of the selection to be at *charIndex*
  (i.e. including but not going beyond *charIndex*).  The other end of the
  selection is made the anchor point for future select to commands.  If no
  characters are currently selected, then a new selection is created to
  include the characters between *charIndex* and the most recent selection
  anchor point, inclusive.

*pathName* **selection clear**
  Clears the selection.  If no characters are selected, then the command
  has no effect.

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

*pathName* **size**
  Returns the number of characters in the text.  
   
*pathName* **undo**

*pathName* **unpost**
  Unposts the *comboeditor* window so it is no longer displayed onscreen.  If
  one or more lower level cascaded menus are posted, they are unposted too.

*pathName* **withdraw** 
  Returns the value associated with *item*.  The value is specified by the
  menu item's **-value** option.  *Item* may be a label, index, or tag,
  but may not represent more than one menu item.
   
*pathName* **xview moveto** fraction
  Adjusts the horizontal view in the *comboeditor* window so the portion of
  the menu starting from *fraction* is displayed.  *Fraction* is a number
  between 0.0 and 1.0 representing the position horizontally where to
  start displaying the menu.
   
*pathName* **xview scroll** *number* *what*
  Adjusts the view in the window horizontally according to *number* and
  *what*.  *Number* must be an integer.  *What* must be either "units" or
  "pages".  If *what* is "units", the view adjusts left or right by
  *number* units.  The number of pixel in a unit is specified by the
  **-xscrollincrement** option.  If *what* is "pages" then the view
  adjusts by *number* screenfuls.  If *number* is negative then the view
  if scrolled left; if it is positive then it is scrolled right.

*pathName* **yview moveto** fraction
  Adjusts the vertical view in the *comboeditor* window so the portion of
  the menu starting from *fraction* is displayed.  *Fraction* is a number
  between 0.0 and 1.0 representing the position vertically where to start
  displaying the menu.
   
*pathName* **yview scroll** *number* *what*
  Adjusts the view in the window vertically according to *number* and
  *what*.  *Number* must be an integer.  *What* must be either "units" or
  "pages".  If *what* is "units", the view adjusts up or down by *number*
  units.  The number of pixels in a unit is specified by the
  **-yscrollincrement** option.  If *what* is "pages" then the view
  adjusts by *number* screenfuls.  If *number* is negative then earlier
  items become visible; if it is positive then later item becomes visible.
   
DEFAULT BINDINGS
----------------

There are many default class bindings for *comboeditor* widgets.

 1. Clicking mouse button 1 positions the insertion cursor just before the
    character underneath the mouse cursor and clears any selection in the
    widget.  Dragging with mouse button 1 strokes out a selection between
    the insertion cursor and the character under the mouse.

 2. Double-clicking with mouse button 1 selects the word or whitespace
    under the pointer and positions the insertion cursor at the end of the
    word or whitespace.  Dragging after a double click will stroke out a
    selection consisting of whole words.

 3. Triple-clicking with mouse button 1 selects line of text under the
    the pointer and positions the insertion cursor at the end of the line.

 4. The ends of the selection can be adjusted by dragging with mouse button
    1 while the Shift key is down; this will adjust the end of the
    selection that was nearest to the mouse cursor when button 1 was
    pressed.  If the button is double-clicked before dragging then the
    selection will be adjusted in units of whole words.

 5. Clicking mouse button 1 with the Control key down will position the
    insertion cursor in the entry without affecting the selection.

 6. If any normal printing characters are typed in an *comboeditor*, they are
    inserted at the point of the insertion cursor.

 7. The view in the editor can be adjusted by dragging with mouse button 2.
    If mouse button 2 is clicked without moving the mouse, the selection is
    copied into the entry at the position of the mouse cursor.

 8. If the mouse is dragged out of the entry on the left or right sides
    while button 1 is pressed, the entry will automatically scroll to make
    more text visible (if there is more text off- screen on the side where
    the mouse left the window).

 9. The Left and Right keys move the insertion cursor one character to the
    left or right; they also clear any selection in the entry and set the
    selection anchor.  If Left or Right is typed with the Shift key down,
    then the insertion cursor moves and the selection is extended to
    include the new character.  Control- Left and Control-Right move the
    insertion cursor by words, and Control-Shift-Left and
    Control-Shift-Right move the insertion cursor by words and also extend
    the selection.  Control-b and Control-f behave the same as Left and
    Right, respectively.  Meta-b and Meta-f behave the same as Control-Left
    and Control- Right, respectively.

 10. The Home key, or Control-a, will move the insertion cursor to the
     beginning of the entry and clear any selection in the entry.
     Shift-Home moves the insertion cursor to the beginning of the entry
     and also extends the selection to that point.

 11. The End key, or Control-e, will move the insertion cursor to the end
     of the entry and clear any selection in the entry.  Shift- End moves
     the cursor to the end and extends the selection to that point.

 12. The Select key and Control-Space set the selection anchor to the
     position of the insertion cursor.  They do not affect the cur- rent
     selection.  Shift-Select and Control-Shift-Space adjust the selection
     to the current position of the insertion cursor, selecting from the
     anchor to the insertion cursor if there was not any selection
     previously.
 
 13. Control-/ selects all the text in the entry.

 14.  Control-\ clears any selection in the entry.

 15. The F16 key (labelled Copy on many Sun workstations) or Meta-w copies
     the selection in the widget to the clipboard, if there is a selection.

 16. The F20 key (labelled Cut on many Sun workstations) or Control-w
     copies the selection in the widget to the clipboard and deletes the
     selection.  If there is no selection in the widget then these keys
     have no effect.

 17. The F18 key (labelled Paste on many Sun workstations) or Control-y
     inserts the contents of the clipboard at the position of the insertion
     cursor.

 18. The Delete key deletes the selection, if there is one in the entry.  If
     there is no selection, it deletes the character to the right of the
     insertion cursor.

 19. The BackSpace key and Control-h delete the selection, if there is one
     in the entry.  If there is no selection, it deletes the character to
     the left of the insertion cursor.

 **Control** +  **a**
   Selects all characters. Positions the insertion cursor at the end.

 **Control** +  **b**
   Positions the insertion cursor before the previous character.

 **Control** +  **c**
   Copies the selected characters to clipboard.  This happens automatically
   is the **-exportselection** option is true.

 **Control** +  **d**
   Deletes the character to the right of the insertion cursor.

 **Control** +  **e**
   Positions the insertion cursor at the end containing the insertion
   cursor.

 **Control** +  **f**
   Positions the insertion cursor before the next character.

 **Control** +  **h**
   Deletes the character previous to the insertion cursor.  

 **Control** +  **k**
   Deletes all the characters from the insertion cursor to end of the line.
   If there are no characters before the end of the line,
   the newline is deleted.

 **Control** +  **n**
   Positions the insertion cursor on the next line down.  If the
   cursor already on the last line, nothing happens.  The cursor will be
   the same number of characters over in the next line, unless the
   line does not have that many characters.  Then the cursor will
   be at the end of the next line.

 **Control** +  **p**
   Positions the insertion cursor on the previous line up.  If the cursor
   is already the first line, nothing happens.  The cursor will be the same
   number of characters over in the previous line, unless the line does not
   have that many characters.  Then the cursor will be at the end of the
   previous line.

 **Control** +  **t**
   Reverses the order of the two characters to the right of the
   insertion cursor.

 **Control** +  **v**
   Inserts text from the clipboard at the current position.

 **Control** +  **x**
   Copies the selected characters to the clipboard and then deletes them
   from the text.

 **Control** +  **y**
   Redo last edit.

 **Control** +  **z**
   Undo last edit.

 **Alt** +  **b**
   Positions the insertion cursor before the last word.

 **Alt** +  **f**
   Positions the insertion cursor after the next word.

 **BackSpace** 
   Same as  **Control** +  **h**.

 **Delete** 
   Same as  **Control** +  **d**.

 **Down** (down arrow)
   Same as  **Control** +  **n**.

 **End** 
   Moves the insertion cursor after the last character.

 **Escape** 
   Cancels the session by unposting the editor.

 **Home** 
   Moves the insertion cursor before the first character.

 **Left** (left arrow)
   Same as  **Control** +  **b**.

 **Right** (right arrow)
   Same as  **Control** +  **f**.

 **Up** (up arrow)
   Same as  **Control** +  **p**.

 **Control** + **Left** 
   Same as  **Alt** +  **b**.

 **Control** + **Right** 
   Same as  **Alt** +  **f**.

 **Shift** + **End** 
   Moves the insertion cursor after the last character and extends the
   selection.

 **Shift** +  **Home** 
   Moves the insertion cursor before the first character and 
   extends the selection.

 **Shift** +  **Left** 
   Positions the insertion cursor before the previous character and
   extends the selection.

 **Shift** +  **Right** 
   Positions the insertion cursor before the next character and
   extends the selection.

EXAMPLE
-------

Create a *comboeditor* widget with the **blt::comboeditor** command.

 ::

    package require BLT

    # Create a new comboeditor and add menu items to it.

    blt::combobutton .file -text "File" -menu .file.m \
      -xscrollbar .file.xs \
      -yscrollbar .file.ys 

    blt::comboeditor .file.m 
    .file.m add -text "New Window" -accelerator "Ctrl+N" -underline 0 \
	-icon $image(new_window)
    .file.m add -text "New Tab" -accelerator "Ctrl+T" -underline 4 \
        -icon $icon(new_tab)
    .file.m add -text "Open Location..." -accelerator "Ctrl+L" -underline 5
    .file.m add -text "Open File..." -accelerator "Ctrl+O" -underline 0 \
       -icon $icon(open_file)
    .file.m add -text "Close Window" -accelerator "Ctrl+Shift+W" -underline 9
    .file.m add -text "Close Tab" -accelerator "Ctrl+W" -underline 0
    blt::tk::scrollbar .file.ysbar 
    blt::tk::scrollbar .file.xsbar 

Please note the following:

1. You can't use a Tk **menubutton** with *comboeditor*\ s.  The menu is
   posted by either a **blt::combobutton** or **blt::comboentry**
   widget.

2. You specify scrollbar widgets with the **-xscrollbar** and
   **-yscrollbar** options.  The scrollbars do not already have to exist.

3. You create menu items with the **add** operation.  The type of item is
   specified by the **-type** option.  The default type is "button".

4. You don't pack the scrollbars.  This is done for you.

5. You don't have to specify the **-orient** or **-command** options to
   the scrollbars. This is done for you.

KEYWORDS
--------

comboeditor, widget

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
