
===========
blt::tabset
===========

-------------------------------------
Create and manipulate tabset widgets.
-------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::tabset** *pathName* ?\ *value* *options* ... ?

DESCRIPTION
-----------

The **blt::tabset** widget displays a series of tabbed folders where only one
folder at a time is displayed. A folder can contain a Tk widget that is
displayed when the folder is displayed.

There's no limit to the number of folders.  Tabs can be tiered (more than
one row).  If there are more tabs than can be displayed, tabs can also be
scrolled.  Any folder can also be torn off, when the contents (the Tk
widget contained by it) is temporarily moved into another toplevel widget.
A tabset may used as just a set of tabs, without a displaying any pages.
You can bind events to individual tabs, so it's easy to add features like
"tooltips".

SYNTAX
------

The **blt::tabset** command creates a new window using the *pathName*
argument and makes it into a tabset widget.

  **blt::tabset** *pathName* ?\ *option* value*\ ... ?

Additional options may be specified on the command line or in the option
database to configure aspects of the tabset such as its colors, font, text,
and relief.  The **blt::tabset** command returns its *pathName* argument.  At
the time this command is invoked, there must not exist a window named
*pathName*, but *pathName*'s parent must exist.

When first created the new tabset widget contains no tabs.  Tabs are added
using the **insert** operation described below.  The size of the tabset
window is determined the number of tiers of tabs requested and the sizes of
the Tk widgets embedded inside each folder.  The widest embedded widget
determines the width of all folders. The tallest determines the height.  If
no folders contain an embedded widget, the size is detemined solely by the
size of the tabs.  You can override either dimension with the tabset's
**-width** and **-height** options.

Tabs may be scrolled using the **-scrollcommand** option.  They also
support scanning (see the **scan** operation).  Tabs also may be arranged
along any side of the tabset window using the **-side** option.

REFERENCING TABS
----------------

An individual tabs/folders in the tabset may be described by its index, 
name, tag or text label.  

*index*
  An index is the order of the tab in the tabset.  Indices start from zero.
  In addition to numeric indices, there are additional special indices.
  They are described below:

  *number* 
    The index of the tab.  Indices start from 0.  Tab indices may
    change as tabs are added or removed.

  **@**\ *x*\ ,\ *y*
    Tab that covers the point in the tabset window
    specified by *x* and *y* (in screen coordinates).  If no
    tab covers that point, then the index is ignored.

  **selected** 
    The currently selected tab.  The **selected** index is 
    typically changed by either clicking on the tab with the left mouse 
    button or using the widget's **invoke** operation.

  **active** 
    The tab where the mouse pointer is currently located.  The label is
    drawn using its active colors (see the **-activebackground** and
    **-activeforeground** options).  The **active** index is typically
    changed by moving the mouse pointer over a tab or using the widget's
    **activate** operation. There can be only one active tab at a time.  If
    there is no tab located under the mouse pointer, the index is ignored.

  **focus** 
    Tab that currently has the widget's focus.  This tab is displayed with a
    dashed line around its label.  You can change this using the **focus**
    operation. If no tab has focus, then the index is ignored.

  **down** 
    Tab immediately below the tab that currently has focus,
    if there is one. If there is no tab below, the current 
    tab is returned.

  **left**
    Tab immediately to the left the tab that currently has focus, if there
    is one.  If there is no tab to the left, the current tab is returned.

  **right** 
    Tab immediately to the right the tab that currently has focus, if there
    is one. If there is no tab to the right, the current tab is returned.

  **up** 
    Tab immediately above, if there is one, to the tab that currently has
    focus. If there is no tab above, the current tab is returned.

  **end**
    Last tab in the tabset.  If there are no tabs in the tabset then the
    index is ignored.

  Some indices may not always be available.  For example, if the mouse is not
  over any tab, "active" does not have an index.  For most tabset operations
  this is harmless and ignored.

*label*
  The label of the tab.  Each tab a label.  Labels should not be numbers
  (to distinguish them from indices). Tab labels are distinct.  There can
  be duplicate tab labels.

*tag*
  A tag is a string associated with a tab.  They are a useful for referring
  to groups of tabs. Tabs can have any number of tags associated with them.
  There is one built-in tag is "all"".  Every tab has the tag "all".  Tags
  may be empty (associated with no tabs).  A tag may refer to multiple
  tabs.


STYLES
------

Tabs can be displayed with different styles.  A *style* is collection of
tab attributes that affect how the tab is displayed.  There is a built-in
style call "default" that tabs by default use.  When you set some widget
configuration options (such as **-background** and **-foreground**), you
are changing the "default" style.  This is the same are globally changing
the style for all tabs, since they by default use that style.

You can create new styles with the **style create** operation.  You can
specify the style for individual tabs using the tab **-style** option.
More than one tab can use the same style.

OPERATIONS
----------

All **blt::tabset** operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName* *operation* ?\ *arg* *arg*\ ... ?

*PathName* is the name of the widget. *Operation* and the *arg*\ s
determine the exact behavior of the command.  The following operations are
available for *tabset* widgets:

*pathName* **activate** *tabName* 
  Specifies to draw *tabName* with its active colors (see the
  **-activebackground** and **-activeforeground** options). *TabName* is
  an index, label, or tag but may not refer to more than one tab.  Only one
  tab may be active at a time.  If *tabName* is "", then no tab will be be
  active.

*pathName* **add** ?\ *nameString*\ ?  ?\ *value option ...*\ ?
  Adds a new tab to the tabset.  The tab is automatically placed after any
  existing tabs. If a *nameString* argument is present, this is the name of
  the tab. It is used to uniquely identify the tab, regardless of its
  position in the tabset. By default tabs are labeled "tabN" where N is a
  number.  If *nameString* is "+", this is a special case where a "+" tab
  is created that automatically creates new tabs.
  
  If one or more *option-value* pairs are specified, they modify the given
  tab option(s) to have the given value(s). *Option* and *value* are
  described in the **configure** operation below.

*pathName* **bbox** *tabName*
  Returns the bounding box of *tabName*.  *TabName* may be an index, tag,
  name, or label but may not refer to more than one tab.  This command
  returns a list of 4 numbers that represent the coordinates of the
  upper-left and lower-right corners of the bounding box of the tab (but
  not its folder) in root coordinates.

*pathName* **bind** *tagName* *type* ?\ *sequence*\ ? ?\ *cmdString*\ ?
  Associates *cmdString* with *tagName* and *type* such that whenever the event
  sequence given by *sequence* occurs for a tab with this tag, *cmdString*
  will be invoked.  The syntax is similar to the **bind** command except
  that it operates on tabs, rather than widgets. See the **bind** manual
  entry for complete details on *sequence* and the substitutions performed
  on *cmdString*.

  *Type* is the area of the tab. It can be one of the following.

  **perforation**
    Matches the area under the perforation.
  **xbutton**
    Matches the area under the X button.
  **label**
    Matches all other areas of the tab label.
  
  If all arguments are specified then a new binding is created, replacing
  any existing binding for the same *sequence* and *tagName*.  If the first
  character of *cmdString* is "+" then *cmdString* augments an existing
  binding rather than replacing it.  If no *cmdString* argument is provided
  then the command currently associated with *tagName* and *sequence* (it's
  an error occurs if there's no such binding) is returned.  If both
  *cmdString* and *sequence* are missing then a list of all the event
  sequences for which bindings have been defined for *tagName*.

*pathName* **cget** *option*
  Returns the current value of the widget configuration option given by
  *option*.  *Option* may have any of the values accepted by the
  **configure** operation described below.


*pathName* **configure** ?\ *option*\ ? ?\ *value option value ...*\ ?
  Query or modify the configuration options of the widget.  If no *option*
  is specified, returns a list describing all the available options for
  *pathName* (see **Tk_ConfigureInfo** for information on the format of
  this list).  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more \fIoption\-value\fR pairs are
  specified, then the command modifies the given widget option(s) to have
  the given value(s); in this case the command returns an empty string.

  Widget configuration options may be set either by the **configure** 
  operation or the Tk **option** command.  The resource class
  is "BltTabset".  The resource name is the name of the widget.

  ::

     option add *BltTabset.Foreground white
     option add *BltTabset.Background blue

  *Option* and *value* are described below.

  **-activebackground** *bgName*
    Sets the active background color for the "default" style.  A tab is
    active when the mouse is positioned over it or set by the **activate**
    operation.  *BgName** may be a color name or the name of a background
    object created by the **blt::background** command.  The default is
    "skyblue4".
    
  **-activeforeground** *colorName*
    Sets the active foreground color for tabs for the "default" style.  A
    tab is active when the mouse is positioned over it or set by the
    **activate** operation.  The default is "white".

  **-activeperforationbackground** *bgName*
    Sets the active background color for perforations for the "default"
    style.  A perforation is active when the mouse is positioned over the
    perforation on the selected tab or by setting the **perforation
    activate** operation.  *BgName** may be a color name or the name of a
    background object created by the **blt::background** command.  The
    default is "grey90".
    
  **-activeperforationforeground** *colorName*
    Sets the active foreground color for perforations for the "default"
    style.  A perforation is active when the mouse is positioned over the
    perforation on the selected tab or by setting the **perforation
    activate** operation.  The default is "grey50".

  **-activeperforationrelief** *reliefName*
    Specifies the 3-D effect for a perforation when it is active.
    *ReliefName* specifies how the perforation should appear relative to
    background of the *tabset* widget. Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means the perforation should appear to protrude.  The
    default is "flat".

  **-background** *bgName*
    Sets the background color of the tab and folder for the "default"
    style.  *BgName** may be a color name or the name of a background
    object created by the **blt::background** command.  The default is
    "grey85".
    
  **-borderwidth** *numPixels*
    Sets the width of the 3\-D border around tabs and folders. The
    **-relief** option determines how the border is to be drawn.  The
    default is "1".

  **-cursor** *cursorName* 
    Specifies the cursor to be used for the widget. *CursorName* may have
    any of the forms acceptable to **Tk_GetCursor**.  If *cursorName* is "",
    this indicates that the widget should defer to its parent for cursor
    specification.  The default is "".

  **-font** *fontName*
    Sets the font for the text in tab labels for the "default" style. The
    default is "Arial 9".

  **-foreground** *colorName* 
    Specifies the color of the text in the tabs for the "default" style.
    The default is "black".

  **-gap** *numPixels*
    Sets the gap between tabs. The default is "1".

  **-height** *numPixels*
    Specifies the requested height of widget.  If *numPixels* is 0, then the
    height of the widget will be calculated based on the size the tabs and
    their pages.  The default is "0".

  **-highlightbackground** *BgName*
    Specifies the color of the traversal highlight region when *pathName*
    does not have the input focus.  *BgName* may be a color name or the
    name of a background object created by the **blt::background** command.
    The default is "grey85".

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

  **-justify**  *justifyName*
    Specifies how the tab's text should be justified.  *JustifyName* must
    be "left", "right", or "center".  The default is "center".

  **-outerborderwidth** *numPixels*
    Sets the width of the outer 3\-D border around tabset window. The
    **-outerrelief** option determines how the border is to be drawn.  The
    default is "1".

  **-outerpad** *numPixels*
    Sets the extra padding between the outer border and the folders.
    The default is "0".

  **-outerrelief** *reliefName*
    Specifies the 3-D effect for *pathName*. The relief is for the outer
    border of the widget.  *ReliefName* specifies how the window should
    appear relative its parent window.  Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means *pathName* should appear to protrude.  The
    default is "flat".

  **-pageheight** *numPixels*
    Sets the requested height of the page.  The page is the area under the
    tab used to display the page contents.  If *numPixels* is "0", the
    maximum height of all embedded tab windows is used.  The default is
    "0".

  **-pagewidth** *numPixels*
    Sets the requested width of the page.  The page is the area under the
    tab used to display the page contents.  If *numPixels* is "0", the
    maximum width of all embedded tab windows is used.  The default is "0".

  **-perforationbackground** *bgName*
    Sets the background color for perforations for the "default"
    style. *BgName** may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "grey85".
    
  **-perforationborderwidth** *numPixels*
    Sets the width of the 3\-D border around perforations. The
    **-perforationrelief** and **-activeperforationrelief** options
    determines how the border is to be drawn.  The default is "1".

  **-perforationcommand** *cmdString*
    Specifies a TCL script to be invoked to tear off the current page.
    This command is typically invoked when left mouse button is released
    over the tab perforation.  The standard action is to tear-off the page
    and place it into a new toplevel window.  This option can be overridden
    by the tab's **-perforationcommand** option.

  **-perforationforeground** *colorName*
    Sets the color for dotted line for perforations for the "default"
    style.  The default is "grey64".

  **-perforationcommand** *cmdString*
    Specifies a TCL script to be invoked to tear off the current page in
    the tabset. This command is typically invoked when left mouse button is
    released over the tab perforation.  The standard action is to tear-off
    the page and place it into a new toplevel window.
    Individual tabs may override this option
    by setting the tab's **-perforationcommand** option.

  **-perforationrelief** *reliefName*
    Specifies the 3-D effect for a perforation.
    *ReliefName* specifies how the perforation should appear relative to
    background of the tab. Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means the perforation should appear to protrude.  The
    default is "flat".

  **-relief** *reliefName*
    Specifies the 3-D effect for both tabs and folders.  *ReliefName*
    specifies how the tabs and folders should appear relative to background
    of the *pathName*. Acceptable values are **raised**, **sunken**,
    **flat**, **ridge**, **solid**, and **groove**. For example, "raised"
    means the tab should appear to protrude.  The default is "raised".

  **-rotate** *numDegrees*
    Specifies how to rotate the tab. *NumDegrees* must be one of the following.
    
    **auto** 
      The tab angle is determined solely from the side that the tabs are
      placed. See the **-side** option.  
    **0** 
      The tab will not be rotated, regardless of the side it is placed.
    **90** 
      The tab is rotated 90 degrees, regardless of the side it is placed.
    **180** 
      The tab is rotated 180 degrees, regardless of the side it is placed.
    **270** 
      The tab is rotated 180 degrees, regardless of the side it is placed.

    The default is "auto".

  **-scrollcommand** *cmdPrefix*
    Specifies the prefix for a command for communicating with scrollbars.
    Whenever the view in the widget's window changes, the widget will
    generate a TCL command by concatenating the scroll command and two
    numbers.  If this option is not specified, then no command will be
    executed.

  **-scrollincrement** *numPixels*
    Sets the smallest number of pixels to scroll the tabs.  If *numPixels*
    is greater than 0, this sets the units for scrolling (e.g., when you
    the change the view by clicking on the left and right arrows of a
    scrollbar).

  **-scrolltabs** *boolName*
    Indicates if *pathName* should scroll tabs if the tabs do not fit
    in the current window size.  The default is "0".

  **-selectbackground** *bgName*
    Sets the color to display for background of the selected tab and folder
    for the "default" style. *BgName** may be a color name or the name 
    of a background object created by the **blt::background** command.  
    The default is "grey85".
    
  **-selectcommand** *cmdString*
    Specifies a TCL script to be associated with tabs.  This command is
    typically invoked when left mouse button is released over the tab.
    Individual tabs may override this with the tab's **-selectcommand**
    option. The default is "".

  **-selectforeground** *colorName*
    Sets the color of the selected tab's text label in the "default" style.  
    The default is "black".

  **-selectpadx** *numPixels*
    Specifies extra padding to be displayed on the left and right side of
    the selected tab (no rotation).  The default is "4".

  **-selectpady** *numPixels*
    Specifies extra padding to be displayed above and below the selected
    tab (no rotation).  The default is "2".

  **-showtabs** *how*
    Specifies how to display tabs.  *How* can be one of the following:

    **always**
      Always display tabs and the folder outline of the selected folder. 

    **never**
      Do not display tabs or the folder outline. Only the embedded window
      for the selected tab will be displayed.  You can use the **select**
      operation to change the selected tab.

    **multiple**
      Display tabs when there is more than one tab. 

    The default is "always".

  **-side** *sideName*
    Specifies the side of the widget to place tabs.  *SideName* can be any
    of the following values.

    **top**
      Tabs are drawn along the top from left to right.  Tabs are not
      rotated.
    **left**
      Tabs are drawn along the left side from top to bottom. Tabs are
      rotated 90 degrees.
    **right**
      Tabs are drawn along the right side from top to bottom. Tabs are
      rotated 270 degrees.
    **both**
      Tabs are drawn along the bottom side from left to right. Tabs are not
      rotated.

    The default is "top".

  **-slant** *slantName*
    Specifies if the tabs should be slanted 45 degrees on the left and/or
    right sides. *SlantName* can be any of the following values.

    **none**
      Tabs are drawn as a rectangle.  
    **left**
      The left side of the tab is slanted.  
    **right**
      The right side of the tab is slanted.  
    **both**
      Boths sides of the tab are slanted.

    The default is "none".

  **-stipple** *bitmapName*
    Specifies a stipple pattern to use for the background of the folder
    when the window is torn off for the "default" style.  *BitmapName* 
    specifies a bitmap to use as the stipple pattern. The default is "BLT".

  **-tabwidth** *widthName*
    Indicates the width of each tab.  *WidthName* can be one of the
    following:

    **variable**
      The width of the tab is determined by its text and image.

    **same**
      The width of every tab is the maximum size.

    *numPixels*
      The width of the tab is set to *numPixels*. *NumPixels* is a positive
      screen distance.

    The default is "same".

  **-takefocus** *focus* 
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *focus* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts decide
    whether to focus on the window.  The default is "1".

  **-tearoff** *boolean*
    Indicates if tabs can be torn off (redrawing the tab in a new toplevel
    window) by clicking on the perforation on the lower portion of the tab.
    The tab's **-tearoff** option must also be set to true.  The default
    is "0".

  **-tiers** *numTiers*
    Specifies the maximum number of tiers to use to display the tabs.  The
    default is "1".  

  **-troughcolor** *bgName*
    Sets the background color of the trough surrounding the folder.  
    *BgName** may be a color name or the name of a background
    object created by the **blt::background** command. 
    The default is "grey60".

  **-width** *numPixels*
    Specifies the requested width of the widget.  *NumPixels* is a
    non-negative value and may have any of the forms accept able to
    Tk_GetPixels.  If *numPixels* is "0", then the width of the widget will
    be calculated based on the size the tabs and their pages.  The default
    is "0".

  **-xbutton** *how*
    Specifies if and when the X button is displayed for the tab. *How*
    must be one of the following.

    **always**
      Display the X button for every tab.

    **selected**
      Display the X button only on the selected tab.

    **unselected**
      Display the X button on unselected tabs.  Do not display it on
      the selected tab.

    **never**
      Do not display X buttons on tabs.

    The default is "never".

  **-xbuttoncommand** *cmdString*
    Specifies a TCL script to be invoked when the mouse button is released
    over the X button on the tab. By default, this command deletes current
    tab. 

*pathName* **deactivate** 
  Deactivates all tabs in the tabset.  

*pathName* **delete** ?\ *tabName* ... ?
  Deletes one or more tabs from the tabset.  *TabName* may be an index,
  tag, name, or label and may refer to multiple tabs.

*pathName* **dockall** 
  Docks all tabs that have been torn off. The forces the tabs back into the
  tabset widget.

*pathName* **exists** *tabName* 
  Indicates if the name tab exists in *pathName*.  Returns "1" if the
  tab is found and "0" otherwise.

*pathName* **focus** *tabName*
  Specifies *tabName* to get the widget's focus.  The tab is displayed with
  a dashed line around its label. *TabName* may be an index, tag, name, or
  label but may not reference more than one tab.

*pathName* **highlight** *tabName*
  Same as the **activate** operation.

*pathName* **identify** *tabName* *x* *y*
  Identifies the part of *tabName* under the given screen coordinates.
  Returns "icon", "text", "xbutton", or "". *TabName* may be an index, tag,
  name, or label but may not reference more than one tab.

*pathName* **index** ?\ *flag* ? *string* 
  Returns the node id of the tab specified by *string*.  If *flag* is
  **-name**, then *string* is the name of a tab.  If *flag* is **-index**,
  *string* is an index such as "active" or "focus".  If *flag* isn't
  specified, it defaults to **-index**.

*pathName* **insert** *position* ?\ *tabName* ? ?\ *option* *value* ... ?
  Inserts a new tab into *pathName*.  The new tab is inserted before the
  tab given by *position*.  *Position* is either a number, indicating where
  in the list the new tab should be added, or **end**, indicating that the
  new tab is to be added the end of the list.  *TabName* is the name of the
  tab. If no *tabName* argument is given, then a name is generated in the
  form "tabN".  Returns the name of the new tab.

*pathName* **invoke** *tabName*
  Selects *tabName*, displaying its folder in the tabset.  In addtion the
  TCL command associated with the tab (see the **-selectcommand** option)
  is invoked.  *TabName* may be an index, tag, or label but may not refer
  to more than one tab.  This command is ignored if the tab's state is
  "disabled" (see the **-state** option).

*pathName* **move** *tabName* *how* *destTabName*
  Moves the *tabName* to a new position in the tabset. *How* is either
  "before" or "after". It indicates whether the *tabName* is moved
  before or after *destTabName*.

*pathName* **nameof** *tabName*
  Returns the label of the *tabName*.  The value of *index* may be in any
  form described in the section `REFERENCING TABS`_.

*pathName* **names** ?\ *pattern* ... ?
  Returns the names of the tabs in *pathName*.  If one or more
  *pattern* arguments are provided, then the name of any tab matching
  *pattern* will be returned. *Pattern* is a **glob**\-style pattern.
  Matching is done in a fashion similar to that used by the TCL **glob**
  command.

*pathName* **nearest** *x* *y*
  Returns the index of the tab nearest to given X-Y screen coordinate.

*pathName* **perforation activate** *tabName* *boolean*
  Indicates if the perforation should be drawn as active using the
  **-activeperforationbackground**, **-activeperforationforeground**,
  **-activeperforationrelief** options.
  *TabName* may be an index, tag, or label but may not refer to more than
  one tab.

*pathName* **perforation invoke** *tabName*
  Invokes the command specified for perforations (see the
  **-perforationcommand** widget option).  *TabName* may be an index, tag,
  or label but may not refer to more than one tab.  Typically this command
  places the page into a top level widget. The name of the toplevel is in
  the form "*pathName*-*tabName*".  This command is ignored if the tab's
  state (see the **-state** option) is disabled.

*pathName* **scan mark** *x y*
  Records *x* and *y* and the current view in the tabset window; used with
  later **scan dragto** commands.  Typically this command is associated
  with a mouse button press in the widget.  It returns an empty string.

*pathName* **scan dragto** *x y*.
  This command computes the difference between its *x* and *y* arguments
  and the *x* and *y* arguments to the last **scan mark** command for the
  widget.  It then adjusts the view by 10 times the difference in
  coordinates.  This command is typically associated with mouse motion
  events in the widget, to produce the effect of dragging the list at high
  speed through the window.  The return value is an empty string.

*pathName* **see** *tabName* 
  Scrolls the tabset so that *tabName* is visible in the widget's window.

*pathName* **select** *tabName* 
  Selects *tabName* in the widget. The tab is drawn in its selected colors,
  its folder is raised and displayed, and its TCL command is invoked (see
  the **-selectcommand** option).  *TabName* may be a name, index, or tag,
  but may not represent more than one tab.
  
*pathName* **size**
  Returns the number of tabs in the tabset.

*pathName* **style cget** *styleName* *option*
  Returns the current value of the style configuration option given by
  *option* for *styleName*.  *StyleName* is the name of a style created by
  the **style create** operaton.  *Option* may be any option described
  below for the **style configure** operation.
   
*pathName* **style configure** *styleName* ?\ *option* *value* ... ?
  Queries or modifies the configuration options for the style *styleName*.
  *StyleName* is the name of a style created by the **style create**
  operaton.  If no *option* argument is specified, this command returns a
  list describing all the available options for *pathName* (see
  **Tk_ConfigureInfo** for information on the format of this list).  If
  *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sublist of the value returned if no *option* is specified).
  If one or more *option-value* pairs are specified, then this command
  modifies the given style option(s) to have the given value(s); in this
  case the command returns an empty string.  *Option* and *value* are
  described below.

  **-activebackground** *bgName*
    Sets the active background color.  A tab is active when the mouse is
    positioned over it or set by the **activate** operation.  *BgName** may
    be a color name or the name of a background object created by the
    **blt::background** command.  The default is "skyblue4".
    
  **-activeforeground** *colorName*
    Sets the active foreground color for tabs.  A tab is active when the
    mouse is positioned over it or set by the **activate** operation.  The
    default is "white".

  **-activeperforationbackground** *bgName*
    Sets the active background color for perforations.  A perforation is
    active when the mouse is positioned over the perforation on the
    selected tab or by setting the **perforation activate** operation.
    *BgName** may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "grey90".
    
  **-activeperforationforeground** *colorName*
    Sets the active foreground color for perforations.  A perforation is
    active when the mouse is positioned over the perforation on the
    selected tab or by setting the **perforation activate** operation.  The
    default is "grey50".

  **-background** *bgName*
    Sets the background color of the tab and folder.  *BgName** may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "grey85".
    
  **-font** *fontName*
    Sets the font for the text in tab labels. The default is "Arial 9".

  **-foreground** *colorName* 
    Specifies the color of the text in the tabs.  The default is "black".

  **-perforationbackground** *bgName*
    Sets the background color for perforations. *BgName** may be a color
    name or the name of a background object created by the
    **blt::background** command.  The default is "grey85".
    
  **-perforationforeground** *colorName*
    Sets the color for dotted line for perforations.  The default is
    "grey64".

  **-selectbackground** *bgName*
    Sets the color to display for background of the selected tab and
    folder.  *BgName** may be a color name or the name of a background
    object created by the **blt::background** command.  The default is
    "grey85".
    
  **-selectforeground** *colorName*
    Sets the color of the selected tab's text label.  The default is
    "black".

  **-stipple** *bitmapName*
    Specifies a stipple pattern to use for the background of the folder
    when the window is torn off.  *BitmapName* specifies a bitmap to use as
    the stipple pattern. The default is "BLT".

*pathName* **style create** *styleName* ?\ *option* *value* ... ?
  Creates a new style named *styleName*.  By default all tabs use the
  same set of global style (called "default").  The style specifies the
  tab's appearance: color, font, etc.  Styles contain sets of configuration
  options that you can apply to tabs (using the its **-style**
  option) to change their appearance. More than one tab can use the same
  style. *StyleName* can not already exist.  If one or more
  *option*-*value* pairs are specified, they specify options valid for the
  **style configure** operation.  The name of the style is returned.
   
*pathName* **style delete** ? *styleName* ... ?
  Deletes one or more styles.  *StyleName* is the name of a style created
  by the **style create** operaton.  Styles are reference counted.  The
  resources used by *styleName* are not freed until no item is using it.
   
*pathName* **style exists** *styleName*
  Indicates if the style *styleName* exists in the widget. Returns "1" if
  it exists, "0" otherwise.
   
*pathName* **style names** ?\ *pattern* ... ?
  Returns the names of all the styles in the widget.  If one or more
  *pattern* arguments are provided, then the names of any style matching
  *pattern* will be returned. *Pattern* is a **glob**\-style pattern.
  Matching is done in a fashion similar to that used by the TCL **glob**
  command.

*pathName* **tab cget** *tabName* *option*
  Returns the current value of the configuration option given by *option*
  for *tabName*.  *Option* may have any of the values accepted by the **tab
  configure** operation described below.

*pathName* **tab configure** *tabName* ?\ *option* ? ?\ *value* *option* ...\ ?
  Query or modify the configuration options of one or more tabs.  More than
  one tab can be configured if *tabName* refers to multiple tabs.  If no
  *option* is specified, this operation returns a list describing all the
  available options for *tabName*.

  If *option* is specified, but not *value*, then a list describing the one
  named option is returned.  If one or more \fIoption\-value\fR pairs are
  specified, then each named tab (specified by *tabName*) will have its
  configurations option(s) set the given value(s).  In this last case, the
  empty string is returned.

  In addition to the **configure** operation, widget configuration
  options may also be set by the Tk **option** command.  The class
  resource name is "Tab".

    ::

       option add *BltTabset.Tab.Anchor ne
       option add *BltTabset.name.Fill both

  *Option* and *value* are described below.

  **-anchor** *anchorName* 
    Anchors the tab's embedded widget to a particular position in the
    folder.  This option has effect only if the space in the folder
    surrounding the embedded widget is larger than the widget
    itself. *AnchorName* specifies how the widget will be positioned in the
    extra space.  For example, if *anchorName* is "center" then the window
    is centered in the folder ; if *anchorName* is "w" then the window will
    be aligned with the leftmost edge of the folder. The default is
    "center".

  **-bindtags** *tagList*
    Specifies the binding tags for *tabName*.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    are handled for the tab.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  The default 
    is "all".

  **-data** *dataString*
    Specifies a string to be associated with *tabName*.  This value
    may be used in TCL scripts to associate extra data (other than the
    icon or text) with the tab. The default is "".

  **-deletecommand** *cmdString*
    Specifies a TCL command to invoked when the tab is deleted (via the
    *tabset*\ 's **delete** operation, or destroying *pathName*).  The
    command will be invoked before the tab is actually deleted.  If
    *cmdString* is "", no command is invoked.  The default is "".

  **-fill** *fillName*
    If the space in the folder surrounding the tab's embedded widget is
    larger than the widget, then *fillName* indicates if the embedded
    widget should be stretched to occupy the extra space. *FillName* is one
    of the following.
  
    **x**
      The embedded widget can grow horizontally.  

    **y**
      The embedded widget can grow vertically.  

    **both**
      The embedded widget can grow both vertically and horizontally.  

    **none**
      The embedded widget does not grow along with the span.  

    The default is "both".

  **-icon** *imageName*
    Specifies the icon to be drawn in label for *tabName*.  If
    *imageName* is "", no icon will be drawn.  Both text and icons may
    be displayed at the same time in tab labels.  The default is
    "".

  **-ipadx** *padName*
    Sets the padding to the left and right of the tab's text.  *PadName*
    can be a list of one or two screen distances.  If *padName* has two
    elements, the left side of the text is padded by the first distance
    and the right side by the second.  If *padName* has just one distance,
    both the left and right sides are padded evenly.  The default is
    "0".

  **-ipady** *padName*
    Sets the padding to the top and bottom of the tab's text.  *PadName*
    can be a list of one or two screen distances.  If *padName* has two
    elements, the top of the text is padded by the first distance and the
    bottom by the second.  If *padName* has just one distance, both the top
    and bottom sides are padded evenly.  The default is "0".

  **-padx** *padName*
    Sets the padding around the left and right of the embedded widget.
    *PadName* can be a list of one or two screen distances.  If *padName*
    has two elements, the left side of the widget is padded by the first
    distance and the right side by the second.  If *padName* has just one
    distance, both the left and right sides are padded evenly.  The default
    is "0".

  **-pady** *padName*
    Sets the padding around the top and bottom of the embedded widget.
    *PadName* can be a list of one or two screen distances.  If *padName*
    has two elements, the top of the widget is padded by the first distance
    and the bottom by the second.  If *padName* has just one distance, both
    the top and bottom sides are padded evenly.  The default is "0".

  **-perforationcommand** *cmdString*
    Specifies a TCL script to be invoked to tear off the current page in
    the tabset. This command is typically invoked when left mouse button is
    released over the tab perforation.  The standard action is to tear-off
    the page and place it into a new toplevel window.
    This overrides the widgets **-perforationcommand** option.

  **-selectcommand** *cmdString*
    Specifies a TCL script to be associated with *tabName*.  This command
    is typically invoked when left mouse button is released over the tab.
    Setting this option overrides the widget's **-selectcommand** option.
    If *cmdString* is "" then the widget's **-selectcommand** option value
    is used.  The default is "".

  **-state** *stateName*
    Sets the state of the tab. *StateName* must be one of the following.

    **active**
      The tab and its folder are drawn in its active colors. See the 
      **-activebackground** and **-activeforeground** options.

    **disabled** 
      The text of *tabName* is drawn as engraved and operations on the tab 
      (such as **invoke** and **tab tearoff**) are ignored.  

    **hidden**
      The tab and its folder are not displayed.

    **normal**
      The tab and its folder are drawn normally. See the **-background**
      and **-foreground** options.

    The default is "normal".

  **-style** *styleName* Specifies the style to use to draw the tab and
    folder. *StyleName* is the name of a style created by the **style
    create** operation. The default built-in style is called "default".
    The default is "default".

  **-tearoff** *boolean*
    Indicates if the tab can be torn off (redrawing the tab in a new
    toplevel window) by clicking on the perforation.  The widget option
    **-tearoff** must also be set to true.  The default is "1".

  **-text** *textString*
    Specifies the text of the tab's label.  *TextString* can be an 
    arbitrary string.

  **-window** *childName*
    Specifies the widget to be embedded into the tab.  *ChildName* is the
    pathname of a Tk widget and must be a child of the **blt::tabset**
    widget.  The tabset will "pack" and manage the size and placement of
    *childName*.  The default is "".

  **-windowheight** *numPixels*
    Sets the requested height of the page.  The page is the area under the
    tab used to display the page contents.  If *numPixels* is "0", the
    maximum height of all embedded tab windows is used.  The default is
    "0".

  **-windowwidth** *numPixels*
    Sets the requested width of the page.  The page is the area under the
    tab used to display the page contents.  If *numPixels* is "0", the
    maximum width of all embedded tab windows is used.  The default is "0".

  **-xbutton** *bool*
    Forces the X button to be displayed for *tabName*, regardless if the
    tab is selected or not.  This overrides the widget's **-xbutton**
    option for this particular tab.

  **-xbuttoncommand** *cmdString*
    Specifies a TCL script to be invoked when the mouse button is released
    over the X button on *tabName*. By default, this command deletes current
    tab.  This option overrides the widget **-xbuttoncommand** option.

*pathName* **tearoff** *tabName* ?\ *windowName*\ ... ?
  Moves the widget embedded the folder *tabName* (see the **-window** option),
  placing it inside of *windowName*.  *WindowName* is either the name of an new
  widget that will contain the embedded widget or the name of the
  **blt::tabset** widget.  It the last case, the embedded widget is put
  back into the folder.

  If no *window* argument is provided, then the name of the current parent
  of the embedded widget is returned.

*pathName* **view** 
  Returns a list of two numbers between 0.0 and 1.0 that describe the
  amount and position of the tabset that is visible in the window.  For
  example, if *view* is "0.2 0.6", 20% of the tabset's text is off-screen
  to the left, 40% is visible in the window, and 40% of the tabset is
  off-screen to the right.  These are the same values passed to scrollbars
  via the **-scrollcommand** option.

*pathName* **view moveto** *fraction*
  Adjusts the view in the window so that *fraction* of the
  total width of the tabset text is off-screen to the left.
  *fraction* must be a number between 0.0 and 1.0.

*pathName* **view scroll** *number* *what* 
  This command shifts the view in the window (left/top or right/bottom)
  according to *number* and *what*.  *Number* must be an integer. *What*
  must be either **units** or **pages** or an abbreviation of these.  If
  *what* is **units**, the view adjusts left or right by *number* scroll
  units (see the **-scrollincrement** option).  ; if it is **pages** then
  the view adjusts by *number* widget windows.  If *number* is negative
  then tabs farther to the left become visible; if it is positive then tabs
  farther to the right become visible.

*pathName* **xbutton activate** *tabName*
  Activated the tab's X button. The button is drawn in its active colors.
  See the button's **-activebackground** and **-activeforeground** options.
  *TabName* is an index, label, or tag but may not refer to more than one
  tab.  Only one tab button may be active at a time.

*pathName* **xbutton cget** *option*
  Returns the current value of the button configuration option given by
  *option*.  *Option* may have any of the values accepted by the
  **xbutton configure** operation described below.

*pathName* **xbutton configure** ?\ *option*\ ? ?\ *value option value ...*\ ?
  Query or modify the configuration options of the button.  If no *option*
  is specified, returns a list describing all the available options for
  *pathName* (see **Tk_ConfigureInfo** for information on the format of
  this list).  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more \fIoption\-value\fR pairs are
  specified, then the command modifies the given button option(s) to have
  the given value(s); in this case the command returns an empty string.

  Button configuration options may be set either by the **xbutton configure** 
  operation or the Tk **option** command.  The resource class
  is "XButton".  The resource name is "xbutton".

  ::

     option add *BltTabset.XButton.Foreground white
     option add *BltTabset.XButton.Background blue

  *Option* and *value* are described below.

  **-activebackground** *colorName*
    Sets the active background color for buttons.  An X button is
    active when the mouse is positioned over it or set by the **button
    activate** operation. The default is "skyblue4".
    
  **-activeforeground** *colorName*
    Sets the active foreground color for buttons.  An X button is
    active when the mouse is positioned over it or set by the **button
    activate** operation. The default is "white".

  **-background** *colorName*
    Sets the background color of X buttons.  This is the color of the
    background circle. The default is "grey82".

  **-foreground** *colorName* 
    Sets the foreground color of X buttons.  This is the color of the X.
    The default color is "grey50".

*pathName* **xbutton invoke** *tabName*
  Invokes a command associated with the X button for *tabName*. See the
  tab or widget **-xbuttoncommand** option.
  *TabName* is an index, label, or tag but may not refer to more than one
  tab.  Only one tab button may be invoked at a time.


DEFAULT BINDINGS
----------------

BLT automatically generates class bindings that supply tabsets their
default behaviors. The following event sequences are set by default 
for tabsets (via the class bind tag "BltTabset"):

**<ButtonPress-2>**

**<B2-Motion>**

**<ButtonRelease-2>**

  Mouse button 2 may be used for scanning.
  If it is pressed and dragged over the tabset, the contents of
  the tabset drag at high speed in the direction the mouse moves.

**<KeyPress-Up>**
  The up arrow key moves the focus to the tab immediately above
  the current focus tab.  The tab with focus is drawn
  with the a dashed outline around the tab label.

**<KeyPress-Down>**
  The down arrow key moves the focus to the tab immediately below
  the current focus tab.  The tab with focus is drawn
  with the a dashed outline around the tab label.

**<KeyPress-Left>**
   The left arrow key move the focus to the tab immediately to
   the left of the current focus tab.  The tab with focus is drawn
   with the a dashed outline around the tab label.

**<KeyPress-Right>**
   The right arrow key move the focus to the tab immediately to
   the right of the current focus tab.  The tab with focus is drawn
   with the a dashed outline around the tab label.

**<KeyPress-space>**
  The space key selects the current tab given focus.  When a
  folder is selected, its command is invoked and the embedded widget is
  mapped.

**<KeyPress-Return>**
  The return key selects the current tab given focus.  When a folder is
  selected, its command is invoked and the embedded widget is
  mapped.

**<Enter>**
  When the mouse pointer enters a tab, it is activated (i.e. drawn in
  its active colors).

**<Leave>**
  When the mouse pointer leaves a tab, it is redrawn in
  its normal colors.

**<ButtonRelease-1>**
  Clicking with the left mouse button on a tab causes the tab to be
  selected and its TCL script (see the **-selectcommand**
  options) to be invoked.  The folder and any embedded widget (if one is
  specified) is automatically mapped.

**<ButtonRelease-3>**
  Clicking on the right mouse button (or the left mouse button with the
  Control key held down) tears off the current page into its own toplevel
  widget. The embedded widget is re-packed into a new toplevel and an
  outline of the widget is drawn in the folder.  Clicking again (toggling)
  will reverse this operation and replace the page back in the folder.

**<Control-ButtonRelease-1>**
  Clicking on the right mouse button (or the left mouse button with the
  Control key held down) tears off the current page into its own toplevel
  widget. The embedded widget is re-packed into a new toplevel and an
  outline of the widget is drawn in the folder.  Clicking again (toggling)
  will reverse this operation and replace the page back in the folder.

BIND TAGS
---------

You can bind commands to tabs that are triggered when a particular
event sequence occurs in them, much like canvas items in Tk's 
canvas widget.  Not all event sequences are valid.  The only binding 
events that may be specified are those related to the mouse and 
keyboard (such as **Enter**, **Leave**, **ButtonPress**, 
**Motion**, and **KeyPress**).

It is possible for multiple bindings to match a particular event.
This could occur, for example, if one binding is associated with the
tab name and another is associated with the tab's tags
(see the **-bindtags** option).  When this occurs, all the 
matching bindings are invoked.  A binding associated with the tab
name is invoked first, followed by one binding for each of the tab's 
bindtags.  If there are multiple matching bindings for a single tag, 
then only the most specific binding is invoked.  A continue command 
in a binding script terminates that script, and a break command 
terminates that script and skips any remaining scripts for the event, 
just as for the bind command.

The **-bindtags** option for tabs controls addition tag names that
can be matched.  Implicitly the first tag for each tab is its name.
Setting the value of the **-bindtags** option doesn't change this.

EXAMPLE
-------

You create a tabset widget with the **blt::tabset** command.

  ::
   
    package require BLT
     
    # Create a new tabset
    blt::tabset .ts -relief sunken -borderwidth 2 

A new TCL command ".ts" is also created.  This command can be
used to query and modify the tabset.  For example, to change the
font used by all the tab labels, you use the new command and
the tabset's **configure** operation.

  ::

    # Change the font.
    .ts configure \-font "fixed"

You can then add folders using the **insert** operation.

  ::

    # Create a new folder "f1"
    .ts insert 0 "f1"

This inserts the new tab named "f1" into the tabset.  The index
"0" indicates location to insert the new tab.  You can also use
the index "end" to append a tab to the end of the tabset.  By
default, the text of the tab is the name of the tab.  You can change
this by configuring the **-text** option.

  ::

    # Change the label of "f1"
    .ts tab configure "f1" -text "Tab #1" 

The **insert** operation lets you add one or more folders at a time.

  ::

    .ts insert end "f2" -text "Tab #2" "f3" "f4" 

The tab on each folder contains a label.  A label may display both
an image and a text string.  You can reconfigure the tab's attributes
(foreground/background colors, font, rotation, etc) using the **tab
configure** operation.

  ::

    # Add an image to the label of "f1"
    set image [image create photo -file stopsign.gif]
    .ts tab configure "f1" -image $image
    .ts tab configure "f2" -rotate 90

Each folder may contain an embedded widget to represent its contents.
The widget to be embedded must be a child of the tabset widget.  Using
the **-window** option, you specify the name of widget to be
embedded.  But don't pack the widget, the tabset takes care of placing
and arranging the widget for you.

  ::

    blt::graph .ts.graph
    .ts tab configure "f1" -window ".ts.graph" \\
       -fill both -padx 0.25i -pady 0.25i

The size of the folder is determined the sizes of the Tk widgets
embedded inside each folder.  The folder will be as wide as the widest
widget in any folder. The tallest determines the height.  You can use
the tab's **-pagewidth** and **-pageheight** options override this.

Other options control how the widget appears in the folder.  The
**-fill** option says that you wish to have the widget stretch to
fill the available space in the folder.

  ::

    .ts tab configure "f1" -fill both -padx 0.25i -pady 0.25i


Now when you click the left mouse button on "f1", the
graph will be displayed in the folder.  It will be automatically
hidden when another folder is selected.  If you click on the right
mouse button, the embedded widget will be moved into a toplevel widget 
of its own.  Clicking again on the right mouse button puts it back into 
the folder.

If you want to share a page between two different folders, the
**-selectcommand** option lets you specify a TCL command to be invoked
whenever the folder is selected.  You can reset the **-window**
option for the tab whenever it's clicked.

  ::

    .ts tab configure "f2" -selectcommand { 
         .ts tab configure "f2" -window ".ts.graph"
     }
     .ts tab configure "f1" -selectcommand { 
         .ts tab configure "f1" -window ".ts.graph"
     }

If you have many folders, you may wish to stack tabs in multiple
tiers.  The tabset's **-tiers** option requests a maximum
number of tiers.   The default is one tier.  

  ::

    .ts configure -tiers 2

If the tabs can fit in less tiers, the widget will use that many.  
Whenever there are more tabs than can be displayed in the maximum number
of tiers, the tabset will automatically let you scroll the tabs.  You
can even attach a scrollbar to the tabset.

  ::

    .ts configure -scrollcommand { .sbar set }  -scrollincrement 20
    .sbar configure -orient horizontal -command { .ts view }

By default tabs are along the top of the tabset from left to right.  
But tabs can be placed on any side of the tabset using the **-side**
option.

  ::

    # Arrange tabs along the right side of the tabset. 
    .ts configure -side right -rotate 270


KEYWORDS
--------

tabset, widget

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
