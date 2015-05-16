
===============
blt::tabset
===============

-------------------------------------
Create and manipulate tabset widgets.
-------------------------------------

:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

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

  **index**

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

  **label**

    The label of the tab.  Each tab a label.  Labels should not be numbers
    (to distinguish them from indices). Tab labels are distinct.  There can
    be duplicate tab labels.

  **tag**

    A tag is a string associated with a tab.  They are a useful for
    referring to groups of tabs. Tabs can have any number of tags
    associated with them.  There is one built-in tag is "all"".  Every tab
    has the tag "all".  Tags may be empty (associated with no tabs).  A tag
    may refer to multiple tabs.


TABSET OPERATIONS
-----------------

All **blt::tabset** operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName* *operation* ?\ *arg* *arg*\ ... ?

*PathName* is the name of the widget. *Operation* and the *arg*\ s
determine the exact behavior of the command.  The following operations are
available for *tabset* widgets:

*pathName* **activate** *tab* 

  Specifies to draw *tab* with its active colors (see the
  **-activebackground** and **-activeforeground** options) . *Tab* is
  an index, label, or tag but may not refer to more than one tab.  Only one
  tab may be active at a time.  If *tab* is "", then no tab will be be
  active.

*pathName* **bind** *tagName* ?*sequence*? ?*command*? 

  Associates *command* with *tagName* such that whenever the event sequence
  given by *sequence* occurs for a tab with this tag, *command* will be
  invoked.  The syntax is similar to the **bind** command except that it
  operates on tabs, rather than widgets. See the **bind** manual entry for
  complete details on *sequence* and the substitutions performed on
  *command*.
  
  If all arguments are specified then a new binding is created, replacing
  any existing binding for the same *sequence* and *tagName*.  If the first
  character of *command* is "+" then *command* augments an existing binding
  rather than replacing it.  If no *command* argument is provided then the
  command currently associated with *tagName* and *sequence* (it's an error
  occurs if there's no such binding) is returned.  If both *command* and
  *sequence* are missing then a list of all the event sequences for which
  bindings have been defined for *tagName*.

*pathName* **cget** *option*

  Returns the current value of the widget configuration option given by
  *option*.  *Option* may have any of the values accepted by the
  **configure** operation described below.

*pathName* **configure** ?*option*? ?*value option value ...*?

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
  is "Tabset".  The resource name is the name of the widget.

  ::

     option add *Tabset.Foreground white
     option add *Tabset.Background blue

  *Option* and *value* are described below.

  **-activebackground** *colorName*

    Sets the default active background color for tabs.  A tab is active
    when the mouse is positioned over it or set by the **activate**
    operation.  Individual tabs may override this option by setting the
    tab's **-activebackground** option.

  **-activeforeground** *colorName*

    Sets the default active foreground color for tabs.  A tab is active
    when the mouse is positioned over it or set by the **activate**
    operation.  Individual tabs may override this option by setting the
    tab's **-activeforeground** option.

  **-background** *colorName*

    Sets the default background color of folders.  Individual tabs can
    override this with their own **-background** option.

  **-borderwidth** *numPixels*

    Sets the width of the 3\-D border around tabs and folders. The
    **-relief** option determines how the border is to be drawn.  The
    default is "1".

  **-relief** *relief*

    Specifies the 3-D effect for both tabs and folders.  *Relief* specifies
    how the tabs should appear relative to background of the widget; for
    example, "raised" means the tab should appear to protrude.  The default
    is "raised".

  **-troughbackground** *colorName*

    Sets the background color of the trough under the tabs.  

  **-outerborderwidth** *numPixels*

    Sets the width of the 3\-D border around the outside edge of the
    widget.  The **-relief** option determines how the border is to be
    drawn.  The default is "0".

  **-outerpad** *numPixels*

    Sets the amount of padding between the highlight ring on the outer edge
    of the tabset and the folder.  The default is "0".

  **-outerrelief** *relief*

    Specifies the 3-D effect for the tabset widget.  *Relief* specifies how
    the tabset should appear relative to widget that it is packed into; for
    example, "raised" means the tabset should appear to protrude.  The
    default is "sunken".

  **-cursor** *cursor*

    Specifies the widget's cursor.  The default cursor is "".

  **-dashes** *dashList*

    Sets the dash style of the focus outline.  When a tab has the widget's
    focus, it is drawn with a dashed outline around its label.  *DashList*
    is a list of up to 11 numbers that alternately represent the lengths of
    the dashes and gaps on the cross hair lines.  Each number must be
    between 1 and 255.  If *dashList* is "", the outline will be a solid
    line.  The default value is "5 2".

  **-font** *fontName*

    Sets the default font for the text in tab labels.  Individual tabs may
    override this by setting the tab's **-font** option.  The default value
    is "Arial 9".

  **-foreground** *color* 

    Sets the default color of tab labels.  Individual tabs may override
    this option by setting the tab's **-foreground** option.  The default
    value is "black".

  **-gap** *numPixels*

    Sets the gap (in pixels) between tabs.  The default value is "2".

  **-height** *numPixels*

    Specifies the requested height of widget.  If *numPixels* is 0, then the
    height of the widget will be calculated based on the size the tabs and
    their pages.  The default is "0".

  **-highlightbackground**  *colorName*
    Sets the color to display in the traversal highlight region when the
    tabset does not have the input focus.

  **-highlightcolor** *color*

    Sets the color to use for the traversal highlight rectangle that is
    drawn around the widget when it has the input focus.  The default is
    "black".

  **-highlightthickness** *numPixels*

   Sets the width of the highlight rectangle to draw around the outside of
   the widget when it has the input focus. *NumPixels* is a non-negative
   value and may have any of the forms acceptable to **Tk_GetPixels**.  If
   the value is zero, no focus highlight is drawn around the widget.  The
   default is "2".

  **-pageheight** *numPixels*

    Sets the requested height of the page.  The page is the area under the
    tab used to display the page contents.  If *numPixels* is "0", the
    maximum height of all embedded tab windows is used.  The default is
    "0".

  **-pagewidth** *numPixels*

    Sets the requested width of the page.  The page is the area under the
    tab used to display the page contents.  If *numPixels* is "0", the
    maximum width of all embedded tab windows is used.  The default is "0".

  **-perforationcommand** *string*

    Specifies a TCL script to be invoked to tear off the current page in
    the tabset. This command is typically invoked when left mouse button is
    released over the tab perforation.  The default action is to tear-off
    the page and place it into a new toplevel window.

  **-rotate** *angle*

    Specifies the degrees to rotate text in tab labels.  *Angle* is a real
    value representing the number of degrees to rotate the text labels.
    The default is "0.0" degrees.

  **-tabwidth** *width*

    Indicates the width of each tab.  *Width* can be one of the
    following:

      variable
        The width of the tab is determined by its text and image.

      same
        The width of every tab is the maximum size.

      pixels
        The width of the tab is set to \fIpixels\R. 
        *Pixels* is a positive screen distance.

    The default is "same".

  **-scrollcommand** *string*

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

  **-selectbackground** *colorName*

    Sets the color to use when displaying background of the selected
    tab. Individual tabs can override this option by setting the tab's
    **-selectbackground** option.

  **-selectcommand** *string*

    Specifies a default TCL script to be associated with tabs.  This
    command is typically invoked when left mouse button is released over
    the tab.  Individual tabs may override this with the tab's **-command**
    option. The default value is "".

  **-selectforeground** *colorName*

    Sets the default color of the selected tab's text label.  Individual
    tabs can override this option by setting the tab's
    **-selectforeground** option. The default value is "black".

  **-selectpad** *numPixels*

    Specifies extra padding to be displayed around the selected tab.  The
    default value is "3".

  **-side** side

    Specifies the side of the widget to place tabs.  *Side* can be any of
    the following values.

      **top**
        Tabs are drawn along the top.
      **left**
        Tabs are drawn along the left side.
      **right**
        Tabs are drawn along the right side.
      **both**
        Tabs are drawn along the bottom side.

   The default value is "top".

**-slant** *tabSide*

    Specifies if the tabs should be slanted 45 degrees on the left and/or
    right sides. *TabSide* can be any of the following values.

      **none**
        Tabs are drawn as a rectangle.  
      **left**
        The left side of the tab is slanted.  
      **right**
        The right side of the tab is slanted.  
      **both**
        Boths sides of the tab are slanted.

    The default is "none".

  **-takefocus** *focus* 

    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *focus* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts decide
    whether to focus on the window.  The default is "1".

  **-tearoff** *boolean*

  **-textside** *side*

    If both images and text are specified for a tab, this option determines
    on which side of the tab the text is to be displayed. The valid sides
    are "left", "right", "top", and "bottom".  The default value is "left".

  **-tiers** *numTiers*

    Specifies the maximum number of tiers to use to display the tabs.  The
    default value is "1".  

  **-width** *numPixels*

    Specifies the requested width of the widget.  *NumPixels* is a
    non-negative value and may have any of the forms accept able to
    Tk_GetPixels.  If *numPixels* is "0", then the width of the widget will
    be calculated based on the size the tabs and their pages.  The default
    is "0".

*pathName* **delete** ?\ *tab* ... ?

  Deletes one or more tabs from the tabset.  *Tab* may be an index,
  tag, name, or label and may refer to multiple tabs.

*pathName* **focus** *tab*

  Specifies *tab* to get the widget's focus.  The tab is displayed with
  a dashed line around its label. *Tab* may be an index, tag, name, or
  label but may not reference more than one tab.

*pathName* **get** *tab*

  Returns the label of the *tab*.  The value of *index* may be in any
  form described in the section `INDICES`_.

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
  tab. If no *tab* argument is given, then a name is generated in the
  form "tabN".  Returns the name of the new tab.

*pathName* **invoke** *tab*

  Selects *tab*, displaying its folder in the tabset.  In addtion the TCL
  command associated with the tab (see the tabset's **-selectcommand**
  option or the tab's **-command** option) is invoked, if there is one.
  *Tab* may be an index, tag, or label but may not refer to more than one
  tab.  This command is ignored if the tab's state (see the **-state**
  option) is "disabled".

*pathName* **move** *tab* *how* *destTab*

  Moves the *tab* to a new position in the tabset. *How* is either
  "before" or "after". It indicates whether the *tab* is moved
  before or after *destTab*.

*pathName* **nearest** *x* *y*

  Returns the name of the tab nearest to given X-Y screen coordinate.

*pathName* **perforation highlight** *tab* *boolean*

*pathName* **perforation invoke** *tab*

  Invokes the command specified for perforations (see the
  **-perforationcommand** widget option). Typically this command places the
  page into a top level widget. The name of the toplevel is in the form
  "*pathName*-*tab*".  This command is ignored if the tab's state (see the
  **-state** option) is disabled.

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


*pathName* **see** *tab* 

  Scrolls the tabset so that the tab *tab* is visible in the widget's
  window.

*pathName* **size**

  Returns the number of tabs in the tabset.

*pathName* **tab cget** *tab* *option*

  Returns the current value of the configuration option given by *option*
  for tab *tab*.  *Option* may have any of the values accepted by the **tab
  configure** operation described below.

*pathName* **tab configure** *tab* ?\ *option* ? ?\ *value* *option* ...\ ?

  Query or modify the configuration options of one or more tabs.  More than
  one tab can be configured if *tab* refers to multiple tabs.  If no
  *option* is specified, this operation returns a list describing all the
  available options for *tab*.

  If *option* is specified, but not *value*, then a list describing the one
  named option is returned.  If one or more \fIoption\-value\fR pairs are
  specified, then each named tab (specified by *tab*) will have its
  configurations option(s) set the given value(s).  In this last case, the
  empty string is returned.  

  In addition to the **configure** operation, widget configuration
  options may also be set by the Tk **option** command.  The class
  resource name is "Tab".

    ::

       option add *Tabset.Tab.Foreground white
       option add *Tabset.name.Background blue

  *Option* and *value* are described below.

  **-activebackground** *colorName*

    Sets the active background color for *tab*.  A tab is active when the
    mouse is positioned over it or set by the **activate** operation.  This
    overrides the widget's **-activebackground** option.

  **-activeforeground** *colorName*

    Sets the default active foreground color *tab*.  A tab is active when
    the mouse is positioned over it or set by the **activate** operation.
    Individual tabs may override this option by setting the tab's
    **-activeforeground** option.

  **-anchor** *anchor* 

    Anchors the tab's embedded widget to a particular edge of the folder.
    This option has effect only if the space in the folder surrounding the
    embedded widget is larger than the widget itself. *Anchor* specifies
    how the widget will be positioned in the extra space.  For example, if
    *anchor* is "center" then the window is centered in the folder ; if
    *anchor* is "w" then the window will be aligned with the leftmost edge
    of the folder. The default value is "center".

  **-background** *color*

    Sets the background color for *tab*.  Setting this option overides the
    widget's **-tabbackground** option.

  **-bindtags** *tagList*

    Specifies the binding tags for this tab.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how
    commands for events in tabs are invoked.  Each tag in the list matching
    the event sequence will have its TCL command executed.  Implicitly the
    name of the tab is always the first tag in the list.  The default value
    is "all".

  **-command** *string*

    Specifies a TCL script to be associated with *tab*.  This
    command is typically invoked when left mouse button is released over
    the tab.  Setting this option overrides the widget's **-selectcommand**
    option.

  **-data** *string*

    Specifies a string to be associated with *tab*.  This value
    isn't used in the widget code.  It may be used in TCL bindings to
    associate extra data (other than the image or text) with the tab. The
    default value is "".

  **-fill** *fill*

    If the space in the folder surrounding the tab's embedded widget is
    larger than the widget, then *fill* indicates if the embedded widget
    should be stretched to occupy the extra space.  *Fill* is either
    "none", "x", "y", "both".  For example, if *fill* is "x", then the
    widget is stretched horizontally.  If *fill* is "y", the widget is
    stretched vertically.  The default is "none".

  **-font** *fontName* 

    Sets the font for the text in tab labels.  If *fontName* is not the
    empty string, this overrides the tabset's **-font** option.  The
    default value is "".

  **-foreground** *colorName* 

    Sets the color of the label for *nameOrIndex*.  If *colorName* is not
    the empty string, this overrides the widget's **-tabforeground**
    option.  The default value is "".

  **-image** *imageName*

    Specifies the image to be drawn in label for *tab*.  If
    *imageName* is "", no image will be drawn.  Both text and images may
    be displayed at the same time in tab labels.  The default value is
    "".

  **-ipadx** *pad*

    Sets the padding to the left and right of the label.  *Pad* can be a
    list of one or two screen distances.  If *pad* has two elements, the
    left side of the label is padded by the first distance and the right
    side by the second.  If *pad* has just one distance, both the left and
    right sides are padded evenly.  The default value is "0".

  **-ipady** *pad*

    Sets the padding to the top and bottom of the label.  *Pad* can be a
    list of one or two screen distances.  If *pad* has two elements, the
    top of the label is padded by the first distance and the bottom by the
    second.  If *pad* has just one distance, both the top and bottom sides
    are padded evenly.  The default value is "0".

  **-padx** *pad*

    Sets the padding around the left and right of the embedded widget, if
    one exists.  *Pad* can be a list of one or two screen distances.  If
    *pad* has two elements, the left side of the widget is padded by the
    first distance and the right side by the second.  If *pad* has just one
    distance, both the left and right sides are padded evenly.  The default
    value is "0".

  **-pady** *pad*

    Sets the padding around the top and bottom of the embedded widget, if
    one exists.  *Pad* can be a list of one or two screen distances.  If
    *pad* has two elements, the top of the widget is padded by the first
    distance and the bottom by the second.  If *pad* has just one distance,
    both the top and bottom sides are padded evenly.  The default value is
    "0".

  **-selectbackground** *color*

    Sets the color to use when displaying background of the selected
    tab. If *color* is not the empty string, this overrides the widget's
    **-selectbackground** option. The default value is "".

  **-shadow** *color*

    Sets the shadow color for the text in the tab's label. Drop shadows are
    useful when both the foreground and background of the tab have similar
    color intensities.  If *color* is the empty string, no shadow is drawn.
    The default value is "".

  **-state** *state*

    Sets the state of the tab. If *state* is "disable" the text of the tab
    is drawn as engraved and operations on the tab (such as **invoke** and
    **tab tearoff**) are ignored.  The default is "normal".

  **-stipple** *bitmapName*
  
    Specifies a stipple pattern to use for the background of the folder
    when the window is torn off.  *BitmapName* specifies a bitmap to use as
    the stipple pattern. The default is "BLT".

  **-text** *string*

    Specifies the text of the tab's label.  The exact way the text is drawn
    may be affected by other options such as **-state** or **-rotate**.

  **-window** *childName*

    Specifies the widget to be embedded into the tab.  *ChildName* is the
    pathname of a Tk widget and must be a child of the **blt::tabset**
    widget.  The tabset will "pack" and manage the size and placement of
    *childName*.  The default value is "".

  **-windowheight** *numPixels*

    Sets the requested height of the page.  The page is the area under the
    tab used to display the page contents.  If *numPixels* is "0", the
    maximum height of all embedded tab windows is used.  The default is
    "0".

  **-windowwidth** *numPixels*

    Sets the requested width of the page.  The page is the area under the
    tab used to display the page contents.  If *numPixels* is "0", the
    maximum width of all embedded tab windows is used.  The default is "0".

*pathName* **tab names** ?\ *pattern*\ ... ?

  Returns the names of all the tabs matching the given pattern. If no
  *pattern* argument is provided, then all tab names are returned.

*pathName* **tab tearoff** *tab* ?\ *window*\ ... ?

  Moves the widget embedded the folder *tab* (see the **-window** option),
  placing it inside of *window*.  *Window* is either the name of an new
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


DEFAULT BINDINGS
----------------

BLT automatically generates class bindings that supply tabsets their
default behaviors. The following event sequences are set by default 
for tabsets (via the class bind tag "Tabset"):

**<ButtonPress-2>**

**<B2-Motion>**

**<ButtonRelease-2>**

  Mouse button 2 may be used for scanning.
  If it is pressed and dragged over the tabset, the contents of
  the tabset drag at high speed in the direction the mouse moves.

**<KeyPress-Up>**

**<KeyPress-Down>**

  The up and down arrow keys move the focus to the tab immediately above
  or below the current focus tab.  The tab with focus is drawn
  with the a dashed outline around the tab label.

**<KeyPress-Left>**

**<KeyPress-Right>**

   The left and right arrow keys move the focus to the tab immediately to
   the left or right of the current focus tab.  The tab with focus is drawn
   with the a dashed outline around the tab label.

**<KeyPress-space>**

**<KeyPress-Return>**

  The space and return keys select the current tab given focus.  When a
  folder is selected, it's command is invoked and the embedded widget is
  mapped.

  Each tab, by default, also has a set of bindings (via the tag "all").
  These bindings may be reset using the tabset's **bind** operation.

**<Enter>**

**<Leave>**

  When the mouse pointer enters a tab, it is activated (i.e. drawn in
  its active colors) and when the pointer leaves, it is redrawn in
  its normal colors.

**<ButtonRelease-1>**

  Clicking with the left mouse button on a tab causes the tab to be
  selected and its TCL script (see the **-command** or **-selectcommand**
  options) to be invoked.  The folder and any embedded widget (if one is
  specified) is automatically mapped.

**<ButtonRelease-3>**

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

     # Create a new tabset
     tabset .ts -relief sunken -borderwidth 2 

A new TCL command ".ts" is also created.  This command can be
used to query and modify the tabset.  For example, to change the
default font used by all the tab labels, you use the new command and
the tabset's **configure** operation.

  ::

     # Change the default font.
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

     graph .ts.graph
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
**-command** option lets you specify a TCL command to be invoked
whenever the folder is selected.  You can reset the **-window**
option for the tab whenever it's clicked.

  ::

     .ts tab configure "f2" -command { 
         .ts tab configure "f2" -window ".ts.graph"
     }
     .ts tab configure "f1" -command { 
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
