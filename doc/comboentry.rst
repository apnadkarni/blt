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

SYNTAX
------

The **blt::comboentry** command creates a new window using the *pathName*
argument and makes it into a *comboentry* widget.  The command has the
following form.

  **blt::comboentry** *pathName* ?\ *option value* ... ?

Additional options may be specified on the command line or in the option
database to configure aspects of the comboentry such as its background
color or scrollbars. The **blt::comboentry** command returns its *pathName*
argument.  There must not already exist a window named *pathName*, but
*pathName*'s parent must exist.

The normal size of the *comboentry* is the computed size to display the
text. You can specify the width and height of the *comboentry* window using
the **-width** and **-height** widget options.

CHARACTER INDICES
-----------------

Character positions in the text can be defined by using one of more
indices. An index can be in one of the following forms.

*number*
  The index of the character.  Character indices start at 0.

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
  The index of the character that is located at the *x* screen coordinates.
  If no character is at that point, then the index is "-1".

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

*pathName* **button cget**  *option*
  Returns the current value of the configuration option given by *option*.
  *Option* may have any of the values accepted by the **configure**
  operation described below.

*pathName* **button configure**  ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Query or modify the configuration options of the widget.  If no *option*
  is specified, returns a list describing all of the available options for
  *pathName* (see **Tk_ConfigureInfo** for information on the format of
  this list).  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*-*value* pairs are
  specified, then the command modifies the given widget option(s) to have
  the given value(s); in this case the command returns an empty string.
  *Option* and *value* are described below.

  **-activebackground** *bgName* 
    Specifies the background color of the button when it is active.
    *BgName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "white".

  **-activeforeground** *colorName* 
    Specifies the color of the button it is active.  The
    default is "black".

  **-activerelief** *reliefName* 
    Specifies the relief of the button when it is active.  This determines
    the 3-D effect for the arrow.  *ReliefName* indicates how the button
    should appear relative to the comboentry window; for example, "raised"
    means the arrow should appear to protrude.  The default is "raised".

  **-background** *bgName* 
    Specifies the background color of *pathName*.  *BgName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of *pathName*.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the button.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-command** *cmdString* 
    Specifies a TCL command to be executed when a button is invoked:
    either by clicking on the menu item or using the **invoke** operation.
    If *cmdString* is "", then no command is invoked. The default is "".

  **-relief** *reliefName* 
    Specifies the 3-D effect for the button.  *ReliefName* indicates how
    the button should appear relative to the *comboentry* window; for
    example, "raised" means the button should appear to protrude.  The
    default is "raised".

*pathName* **button invoke** 
  FIXME
  Selects *cellName* and invokes the TCL command associated the button
  (see the **-command** button or **-clearcommand** widget option). 

*pathName* **cget** *option*  
  Returns the current value of the widget configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **closest** *x*  
  Returns the index of the character closest to the given screen
  coordinate.  *X* is screen coordinates relative to the
  *comboentry* window unless the **-root** switch is given.
  *Switches* can be any of the following.

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
  "BltComboentry".  The resource name is the name of the widget::

    option add *BltComboentry.anchor n
    option add *BltComboentry.Anchor e

  The following widget options are available\:

  **-activearrowrelief** *colorName* 
    Specifies the color of the text when *pathName* is active.  The
    default is "black".

  **-activebackground** *background* 
    Specifies the background color of *pathName* when it is active.
    *Background* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "white".

  **-activeforeground** *colorName* 
    Specifies the color of the text when *pathName* is active.  The
    default is "black".

  **-arrowborderwidth** *numPixels* 
    Specifies the borderwidth of the arrow.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the menu.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-arrowpad** *padName*
  
  **-arrowrelief** *relief* 
    Specifies the relief of the arrow.  This determines the 3-D effect for
    the arrow.  *Relief* indicates how the arrow should appear relative to
    the comboentry window; for example, "raised" means the arrow should
    appear to protrude.  The default is "raised".
    
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

  **-clearbutton** *boolean* 

  **-clearcommand** *cmdString* 

  **-command** *cmdString* 
    Specifies a TCL command to be executed when a button is invoked:
    either by clicking on the menu item or using the **invoke** operation.
    If *cmdString* is "", then no command is invoked. The default is "".

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

  **-editable** *boolean* 

  **-exportselection** *boolean* 
    Indicates if the selections are to be exported and copied to the
    clipboard.  The default is "0".

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

  **-hidearrow** *boolean* 

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

  **-iconwidth** *numPixels* 

  **-image** *imageName* 
    Specifies the name of a global TCL variable that will be set to the
    name of the image representing the icon of *pathName*.  If *varName* is
    "", no variable is used. The default is "".

  **-insertbackground** *colorName* 
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
    Specifies the path name of the menu associated with this comboentry.
    *MenuName* must be a **blt::combomenu** widget and a child of
    *pathName*.  If *menuName* is "", no menu will be displayed. The
    default is "".

  **-menuanchor** *anchorName* 

  **-postcommand** *cmdString* 
    Specifies a TCL command to invoked when the menu is posted.  The
    command will be invoked before the menu is displayed onscreen.  For
    example, this may be used to disable buttons that may not be valid
    when the menu is posted. If *cmdString* is "", no command is invoked.
    The default is "".

  **-relief** *relief* 
    Specifies the 3-D effect for *pathName*.  *Relief* indicates how the
    menu should appear relative to the root window; for example, "raised"
    means *pathName* should appear to protrude.  The default is "raised".

  **-selectbackground** *colorName* 
    Specifies the color of the rectangle surrounding selected text.
    The default is "skyblue4".

  **-selectborderwidth** *numPixels* 
    Specifies the borderwidth of the selected rectangle.  *NumPixels* is a
    non-negative value indicating the width of the 3-D border drawn around
    the selected text.  *NumPixels* may have any of the forms acceptable to
    **Tk_GetPixels**.  If *numPixels* is "0", no 3-D relief is drawn.
    The default is "0".

  **-selectcommand** *cmdString*
    Specifies a TCL command to be invoked when an item is selected: either
    by clicking on the item or using the **select** operation.  If
    *cmdString* is "", then no command is invoked. The default is "".

  **-selectforeground** *colorName* 
    Specifies the color of selected text.  The default is "white".

  **-selectrelief** *reliefName*
    Specifies the 3-D effect for the rectangle surrounding the selected
    text.  *Relief* indicates how the rectangle should appear relative to the
    normal text; for example, "raised" means the rectangle should appear to
    protrude.  The default is "flat".  

  **-show** *boolean*
    Indicates to display text as circles instead of the text itself.
    The default is "0".

  **-state** *state*
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

  **-textbackground** *colorName* 
    Specifies the background color of the text area.  *ColorName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-textfocusbackground** *bgName* 
    Specifies the background color of the text area.  *ColorName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-textfocusforeground** *colorName* 

  **-textforeground** *colorName* 
    Specifies the color of the text.  The default is "black".

  **-textvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    text of *pathName*.  If *varName* is set to another value, the text in
    *pathName* will change accordingly. If *varName* is "", no variable is
    used. The default is "".

  **-textwidth** *numCharacters* 
    Specifies the preferred width of widget in terms of characters.
    If *numCharacters* is "0", then the **-width** option is used to determine
    the width of the widget. The default is "0".

  **-width** *numPixels*
    Specifies the width of the widget*.  *NumPixels* is a non-negative value
    indicating the width of *pathName*. The value may have any of the forms
    accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is "0", the width of *pathName* is computed from its text.
    The default is "".

  **-xscrollcommand** *cmdPrefix*
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window
    changes, the widget will generate a TCL command by concatenating the
    scroll command and two numbers. If this option is not specified, then
    no command will be executed.  The widget's initialization script
    will automatically set this for you.

  **-xscrollincrement** *numPixels*
    Sets the horizontal scrolling unit. This is the distance the editor is
    scrolled horizontally by one unit. *NumPixels* is a non-negative value
    indicating the width of the 3-D border drawn around the editor. The
    value may have any of the forms accept able to **Tk_GetPixels**.  The
    default is "20".
  
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
  Returns the text from the widget.  If *firstIndex* and *lastIndex*
  arguments are present, they describe the region of characters to be
  returned.

*pathName* **icursor** *charIndex*
  Specifies the location of the insertion cursor.  *CharIndex* is the index
  of character before which the insertion cursor will be placed. *CharIndex*
  may be in any of the forms described in `CHARACTER INDICES`_.

*pathName* **identify** *x* *y*
  Returns the name of the element under the point given by *x* and *y*
  (such as **button** or **arrow**), or an empty string if the point does
  not lie in any element of the *comboentry* widget.  *X* and *y* must be
  pixel coordinates relative to the widget.

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
  to be displayed on the screen. The menu is displayed underneath
  the comboentry window.

*pathName* **redo** 
  Re-applies the last reverted change.  This command only has effect if the
  last command was a **undo** operation. The text and insertion cursor are
  possibly changed.
  
*pathName* **scan dragto** *x* *y*
  This command computes the difference between *x* and *y* and the
  coordinates to the last **scan mark** command for the widget.  It then
  adjusts the view by 10 times the difference in coordinates.  *X* and *y*
  are screen coordinates relative to editor window.  This command is
  typically associated with mouse motion events in the widget, to produce
  the effect of dragging the item list at high speed through the window.
   
*pathName* **scan mark** *x* *y*
  Records *x* and *y* and the current view in the editor window; to be used
  with later **scan dragto** commands. *X* and *y* are screen coordinates
  relative to editor window.  Typically this command is associated
  with a mouse button press in the widget.  

*pathName* **see** *charIndex*
  Scrolls the editor so that character at *charIndex* is visible in the
  widget's window. *CharIndex* may be in any of the forms described in
  `CHARACTER INDICES`_.

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
  option) so it is no longer displayed onscreen.  

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

**<Enter>** 
  The button activates whenever the pointer passes over the button window.
**<Leave>**
  The button deactivates whenever the pointer leaves the button window.
**<ButtonPress-1>**
  Pressing button 1 over the comboentry posts its associated combomenu
  if one is specified. The relief  of the button  changes to raised and
  its associated menu is posted under the comboentry.

**<B1-Motion>**
  If the mouse is dragged down into the menu with the button still down,
  and if the mouse button is then released over an entry in the menu, the
  comboentry is unposted and the menu item is invoked.

  If button 1 is pressed over a comboentry and then dragged over some
  other comboentry, the original comboentry unposts itself and the new
  comboentry posts.

**<ButtonRelease-1>**
  If button 1 is pressed over a comboentry and then released over that
  comboentry, the comboentry stays posted: you can still move the mouse
  over the menu and click button 1 on an entry to invoke it.  Once a menu
  item has been invoked, the comboentry unposts itself.

  If button 1 is pressed over a comboentry and released outside any
  comboentry or menu, the comboentry unposts without invoking any menu
  item.

**<Alt-KeyPress-**\ *key *\ **>**
  If the **-underline** option has been specified then keyboard traversal
  may be used to post the comboentry's menu: Alt+\ *key*, where *key* is the
  underlined character (or its lower-case or upper-case equivalent), may be
  typed in any window under the comboentry's toplevel to post the
  comboentry's menu.

**<KeyPress-Space>** or  **<KeyPress-Return>**
   If a comboentry has the input focus, the space and  return  keys
   post the comboentry's menu.

The behavior of comboentrys can be changed by defining new bindings for
individual widgets or by redefining the class bindings.

EXAMPLE
-------

You create a *comboentry* widget with the **blt::comboentry** command.

 ::

    package require BLT

    # Create a new comboentry and combomenu.

    blt::comboentry .file \
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

2. Both the **blt::combomenu** and **blt::comboentry** widgets have
   **-textvariable** and **-iconvariable** options that let them change
   the text and icon through TCL variables.

3. You can't use a Tk **menu** with *comboentry*\ s.  The menu must
   be a **blt::combomenu** widget.

DIFFERENCES WITH TK MENUS
-------------------------

The **blt::comboentry** widget has the following differences with the Tk
**menubutton** widget.

1. *Comboentrys* can not post by a Tk **menu**.

   
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
