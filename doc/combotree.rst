===============
blt::combotree
===============

----------------------------------------
Create and manipulate combotree widgets.
----------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::combotree** *pathName* ?\ *option value* ... ?

DESCRIPTION
-----------

The **blt::combotree** command creates and manages *combotree* widgets.  A
*combotree* widget contains a tree view and optional embedded Tk scrollbar
widgets (vertical and/or horizontal).  The scrollbars are automatically
exposed or hidden as necessary when the *combotree* widget is resized.
Whenever the *combotree* window is smaller horizontally and/or vertically
than the actual menu, the appropiate scrollbar is exposed.

When mapped, the *combotree* displays a tree of items.  There exist several
different types of items, each with different actions and properties.
Items of different types may be combined in a single menu.  

SYNTAX
------

The **blt::combotree** command creates a new window using the *pathName*
argument and makes it into a *combotree* widget.  The command has the
following form.

  **blt::combotree** *pathName* ?\ *option value* ... ?

Additional options may be specified on the command line or in the option
database to configure aspects of the combotree such as its background color
or scrollbars. The **blt::combotree** command returns its *pathName*
argument.  There must not already exist a window named *pathName*, but
*pathName*'s parent must exist.

The normal size of the *combotree* is the computed size to display all menu
items. You can specify the maximum and minimum width and height of the
*combotree* window using the **-width** and **-height** widget options.
Scrollbars are specified for *combotree* by the **-xscrollbar** and
**-yscrollbar** widget options.  The scrollbars must be children of the
*pathName*.  The scrollbars may be specified before they exist.  If the
specified width or height is less than the computed size of the combotree,
the appropiate scrollbar is automatically displayed.

TREE ENTRIES
------------

A menu is a widget that displays a collection of item arranged in one or
more columns.  There exist several different types of items (specified by
the item's **-type** option), each with different properties.  Items of
different types may be combined in a single menu.  Menu items are not
distinct widgets; the entire *combotree* is one widget.

A menu item can be one of the following types: 

**button**
  The *button* is the most common type of menu item that behaves much
  like a Tk **button** widget.  When it isinvoked, a TCL command is
  executed.  The TCL command is specified with the **-command** option.

**cascade**
  A *cascade* item has an associated *combotree* (specified by the item's
  **-menu** option) that is posted to the right of the *cascade* item
  when the item is active.  The **postcascade** operation also posts the
  associated *combotree*. *Cascade* items allow the construction of
  cascading menus.  The associated menu must be a child of the
  *combotree* containing the *cascade* item.

**checkbutton**
  A *checkbutton* menu item behaves much like a Tk **checkbutton** widget.
  When it is invoked it toggles back and forth between the selected and
  deselected states.  When the item is selected, a particular value is
  stored in a particular global variable (as determined by the
  **-onvalue** and **-variable** options for the item); when the item is
  deselected another value (determined by the **-offvalue** option) is
  stored in the global variable.  An indicator box is displayed to the
  left of the label.  When the item is selected, a check is displayed in
  the indicator. If a **-command** option is specified, its value is
  evaluated as a TCL command each time the item is invoked; this happens
  after toggling the item's selected state.

**radiobutton**
  A *radiobutton* menu item behaves much like a Tk **radiobutton** widget.
  *Radiobutton* items are organized in groups of which only one item may be
  selected at a time.  Whenever a particular item becomes selected it
  stores a particular value into a particular global variable (as
  determined by the **-value** and **-variable** options for the item).
  This action causes any previously selected item in the same group to
  deselect itself.  Once an item has become selected, any change to the
  item's associated variable will cause the item to deselect itself.
  Grouping of *radiobutton* items is determined by their associated
  variables: if two items have the same associated variable then they are
  in the same group.  An indicator circle is displayed to the left of the
  label.  If the item is selected then the indicator's center is filled
  with a solid color.  If a **-command** option is specified then its value
  is evaluated as a TCL command when the item is invoked; this happens
  after selecting the item.

**separator**
  A *separator* item displays a horizontal dividing line.  A *separator*
  can't be activated or invoked, and it has no behavior other than its
  display appearance.

Menu items are displayed with up to four separate fields.

*label*
  The main field is a label in the form of a text string, or an image,
  controlled by the **-text** and **-image** options for the item.

*icon*
  If the **-icon** option is specified, then a image is displayed to the
  left of the label.

*accelerator*
  If the **-accelerator** option is specified for an item then a second
  textual field is displayed to the right of the label.  The accelerator
  typically describes a keystroke sequence that may be typed in the
  application to cause the same result as invoking the menu entry.

*indicator*
  The indicator is present only for *checkbutton*, *radiobutton*, and
  *cascade* entries.  For *checkbutton* and *radiobutton* items it
  indicates whether the item is selected or not, and is displayed to the
  left of the entry's string.  For *cascade* items it indicates that
  clicking on item will post yet another menu and is displayed to the right
  of the accelerator.

REFERENCING ENTRIES
-------------------

Entries may be referred to by either their node index, label, or tag.

*index*
  The number of the menu item.  Indices start from 0.  The index of an
  item as other items are added, deleted, moved, or sorted. There are
  also the following special non-numeric indices.

  **active**
    The item that that is currently active.  Typically this is the
    one that the pointer is over.

  **end**
    The last item in the menu. 
    
  **first**
    The first item in the menu. Disable and hidden items are ignored.

  **last**
    The last item in the menu. Disable and hidden items are ignored.

  **next**
    The next item from the currently active item.
    
  **previous**
    The previous item from the currently active item.

  **selected**
    The last selected menu item.   
    
  **view.bottom**
    The last visible item in the menu.  This changes are the the
    menu is scrolled.
    
  **view.top**
    The first visible item in the menu.  This changes are the the
    menu is scrolled.

*label*
  The label of the item (specified by the **-text** menu item option).
  Labels should not be numbers (to distinguish them from indices) or tags.

*tag*
  A tag is a string associated with an item.  They are a useful for
  referring to groups of items. Items can have any number of tags
  associated with them (specified by the **-tags** menu item option).  A
  tag may refer to multiple items.  There are two built-in tags: "all" and
  "end".  Every item has the tag "all".  The last item in the menu will
  have the tag "end".
     
If an item is specified by an integer it is assumed to be an index.  If it
is specified by a string, it is first tested if it's a valid label and then
a tag.  This means that you shouldn't have tags and labels that are the
same.  They will always be interpreted as labels.  Unlike labels, tags
aren't seen by the user, so you can do whatever you want to make them
unique (example: "mytag::fred").

If an item's **-command** option is specified, a TCL command will be invoke
whenever the item is selected (typically by clicking on the item).
  
OPERATIONS
----------

All *combotree* operations are invoked by specifying the widget's
pathname, the operation, and any arguments that pertain to that
operation.  The general form is:

  *pathName operation* ?\ *arg arg ...*\ ?

*Operation* and the *arg*\ s determine the exact behavior of the
command.  The following operations are available for *combotree* widgets:

*pathName* **activate** *entryName* 
  Redisplays *entryName* using its active colors and relief.  This
  typically is used by widget bindings to highlight tree entries when the
  pointer is moved over items in the menu. Any previously active entry is
  deactivated.  *EntryName* may be a label, index, or tag, but may not
  represent more than one entry.

*pathName* **bbox** *entryName* 
  Returns of list of four numbers describing the bounding box of *entryName*.
  The numbers represent the x and y root coordinates of two opposite
  corners of the box.  *EntryName* may be a label, index, or tag, but may
  not represent more than one entry.

*pathName* **button activate** *entryName*
  Designates the button associated with *entryName* as active.  When an entry's
  button is active it is drawn using its active colors (see the button's
  **-activeforground** and **-activebackground** options).  Note that there
  can be only one active entry at a time.  *EntryName* is a node id, label or a
  tag but may not reference multiple entries.  For example the special id
  **active** indicates the currently active entry.
  
*pathName* **button bind** *tagName* ?\ *sequence*\ ? ?\ *cmdString*\ ?
  Associates *cmdString* with *tagName* such that whenever the event
  sequence given by *sequence* occurs for an button of a node entry with
  this tag, *cmdString* will be invoked.  The syntax is similar to the
  **bind** command except that it operates on **treeview** buttons, rather
  than widgets. See the **bind** manual entry for complete details on
  *sequence* and the substitutions performed on *cmdString* before invoking
  it.

  If all arguments are specified then a new binding is created, replacing
  any existing binding for the same *sequence* and *tagName*.  If the first
  character of *cmdString* is "+" then *cmdString* augments an existing binding
  rather than replacing it.  If no *cmdString* argument is provided then the
  command currently associated with *tagName* and *sequence* (it's an error
  occurs if there's no such binding) is returned.  If both *cmdString* and
  *sequence* are missing then a list of all the event sequences for which
  bindings have been defined for *tagName*.

*pathName* **button cget** *option*
  Returns the current value of the button configuration option given by
  *option*.  *Option* may have any of the values accepted by the **button
  configure** operation described below.

*pathName* **button configure** ?\ *option*\ ? ?\ *value*\ ? ?\ *option* *value* ... ?
  Query or modify the button configuration options.  If no *option*
  is specified, returns a list describing all of the available options for
  the button (see **Tk_ConfigureInfo** for information on the format of
  this list).  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*-*value* pairs are
  specified, then the command modifies the given button option(s) to have
  the given value(s); in this case the command returns an empty string.
  *Option* and *value* are described below.

  **-activebackground** *colorName*
    Sets the background color of an active button.  A button is made active
    when the mouse passes over it or by the **button activate** operation.

  **-activeforeground** *colorName*
    Sets the foreground color of an active button.  A button is made active
    when the mouse passes over it or by the **button activate** operation.

  **-background** *colorName*
    Sets the background of the button.  The default is "white".

  **-borderwidth** *numPixels*
    Sets the width of the 3-D border around the button.  The **-relief**
    option determines if a border is to be drawn.  The default is "1".

  **-closerelief** *reliefName*
    Specifies the 3-D effect for the button when closed.  *ReliefName*
    indicates how the button should appear relative to the
    widget. Acceptable values are **raised**, **sunken**, **flat**,
    **ridge**, **solid**, and **groove**. For example, "raised" means the
    button should appear to protrude.  The default is "solid".

  **-foreground** *colorName* 
    Sets the foreground color of buttons.  The default is "black".

  **-images** *imageList*
    Specifies images to be displayed for the button.  *ImageList* is a list of
    two Tk images: the first image is displayed when the button is open,
    the second when it is closed.  If the *imageList* is the empty string,
    then a plus/minus gadget is drawn.  The default is "".

  **-openrelief** *reliefName*
    Specifies the 3-D effect of the button when open.  *ReliefName*
    indicates how the button should appear relative to the
    widget. Acceptable values are **raised**, **sunken**, **flat**,
    **ridge**, **solid**, and **groove**. For example, "raised" means the
    button should appear to protrude.  The default is "flat".

  **-size** *numPixels*
    Sets the requested size of the button.  The default is "0".

*pathName* **cget** *option*  
  Returns the current value of the widget configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described in the **configure**
  operation below.

*pathName* **close** ?\ **-recurse**\ ? *entryName* ... ?
  Closes the entry specified by *entryName*.  In addition, if a TCL script
  was specified by the **-closecommand** option, it is invoked.
  *EntryName* may be a label, index, or tag, and may refer to more than one
  entry (such as "all"). If the **-recurse** flag is present, each child
  node is recursively closed.  If *entryName* is already closed, the TCL
  command is not invoked.

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
  **BltCombotree**.  The resource name is the name of the widget::

    option add *BltCombotree.anchor n
    option add *BltCombotree.Anchor e

  The following widget options are available\:

  **-activebackground** *colorName*
    Sets the background color for active entries.  A node is active when
    the mouse passes over its entry or using the **activate** operation.

  **-activeforeground** *colorName*
    Sets the foreground color of the active node.  A node is active when
    the mouse passes over its entry or using the **activate** operation.

  **-alternatebackground** *bgName*
    Sets the background color for alternate even entries.
    If *bgName* is "", then the same color (see **-background**) is used
    for all entries. The default is "".

  **-background** *colorName*
    Sets the background color of the widget.  The default is "white".

  **-borderwidth** *numPixels*
    Sets the width of the 3-D border around the outside edge of the widget.
    The **-relief** option determines if the border is to be drawn.  The
    default is "2".

  **-button** *buttonMode*
    Specifies whether to display a button for entries.  *ButtonMode* is
    either a boolean (in the form accepted by **Tcl_GetBoolean**) or
    **auto**.  If *buttonMode* is "auto", buttons are displayed only when
    the node has children.  The default is "auto"
    
  **-closecommand** *string*
    Specifies a TCL script to be invoked when a node is closed.  You can
    overrider this for individual entries using the entry's
    **-closecommand** option. The default is "".  Percent substitutions are
    performed on *string* before it is executed.  The following
    substitutions are valid:

    **%W**
      The pathname of the widget.

    **%p**
      The name of the node.

    **%P**
      The full pathname of the node.

    **%#**
      The id of the node.

    **%%**
      Translates to a single percent.

  **-cursor** *cursor*
    Specifies the widget's cursor.  The default cursor is "".

  **-dashes** *dashList*
    Sets the dash style of the horizontal and vertical lines drawn
    connecting entries. *DashList* is the length in numPixels of the dashes and
    gaps in the line. If *dashList* is "0", solid lines will be drawn. The
    default is "1" (dotted).

  **-flat** *boolean*
    Indicates whether to display the tree as a flattened list.  If
    *boolean* is true, then the hierarchy will be a list of full paths for
    the nodes.  This option also has affect on sorting.  See the **sort**
    operation** section for more information.  The default is "no".

  **-font** *fontName* 
    Specifies the font for entry labels.  You can override this for
    individual entries with the entry's **-font** configuration option.
    The default is "{Sans Serif} 9"

  **-foreground** *colorName* 
    Sets the text color of entry labels.  You can override this for
    individual entries with the entry's **-foreground** configuration
    option.  The default is "black".

  **-height** *numPixels*
    Specifies the requested height of widget.  The default is "400".

  **-hideleaves** *boolean*
    If *boolean* is true, it indicates that not to display entries for the
    nodes without children. The default is "0".

  **-hideroot** *boolean*
    If *boolean* is true, it indicates that no entry for the root node
    should be displayed.  The default is "0".

  **-icons** *imageList*
    Specifies images for the entry's icon.  *ImageList* is a list of two Tk
    images: the first image is displayed when the node is open, the second
    when it is closed.

  **-linecolor** *colorName*
    Sets the color of the connecting lines drawn between entries.  The
    default is "black".

  **-linespacing** *numPixels*
    Sets the number of pixels spacing between entries.  The default is "0".

  **-linewidth** *numPixels*
    Set the width of the lines drawn connecting entries.  If *numPixels* is
    "0", no vertical or horizontal lines are drawn.  The default is "1".

  **-newtags** *boolean* 
    If *boolean* is true, when sharing a tree object (see the **-tree**
    option), don't share its tags too.  The default is "0".

  **-opencommand** *string*
    Specifies a TCL script to be invoked when a node is open.  You can
    override this for individual entries with the entry's **-opencommand**
    configuration option.  The default is "".  Percent substitutions are
    performed on *string* before it is executed.  The following
    substitutions are valid:

    **%W**
      The pathname of the widget.

    **%p**
      The name of the node.

    **%P**
      The full pathname of the node.

    **%#**
      The id of the node.

    **%%**
      Translates to a single percent.

  **-relief** *reliefName*
    Specifies the 3-D effect for the widget.  *ReliefName* specifies how
    the *treeview* widget should appear relative to widget it is packed
    into. Acceptable values are **raised**, **sunken**, **flat**,
    **ridge**, **solid**, and **groove**. For example, "raised" means the
    *treeview* widget should appear to protrude.  The default is "sunken".

  **-separator** *string*
    Specifies the character sequence to use when spliting the path
    components.  The separator may be several characters wide (such as
    "::") Consecutive separators in a pathname are treated as one.  If
    *string* is the empty string, the pathnames are TCL lists.  Each
    element is a path component.  The default is "".

  **-takefocus** *focus* 
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *focus* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts make the
    decision whether to focus on the window.  The default is "1".

  **-textvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    label of the selected item.  If *varName* is "", no variable is
    used. The default is "".

  **-tree** *treeName* 
     FIXME
     
  **-width** *numPixels*
    Sets the requested width of the widget.  If *numPixels* is 0, then the
    with is computed from the contents of the *treeview* widget.  The
    default is "200".

  **-xscrollbar** *scrollbarWidget*
    Specifies the name of a scrollbar widget to use as the horizontal
    scrollbar for this menu.  The scrollbar widget must be a child of the
    combotree and doesn't have to exist yet.  At an idle point later, the
    combotree will attach the scrollbar to widget, effectively packing the
    scrollbar into the menu.

  **-xscrollcommand** *cmdPrefix*
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window
    changes, the widget will generate a TCL command by concatenating the
    scroll command and two numbers.  If this option is not specified, then
    no command will be executed.

  **-xscrollincrement** *numPixels*
    Sets the horizontal scrolling distance. The default is 20 pixels.

  **-yscrollbar** *scrollbarWidget*
    Specifies the name of a scrollbar widget to use as the vertical
    scrollbar for this menu.  The scrollbar widget must be a child of the
    combotree and doesn't have to exist yet.  At an idle point later, the
    combotree will attach the scrollbar to widget, effectively packing the
    scrollbar into the menu.

  **-yscrollcommand** *cmdPrefix*
    Specifies the prefix for a command used to communicate with vertical
    scrollbars.  Whenever the vertical view in the widget's window changes,
    the widget will generate a TCL command by concatenating the scroll
    command and two numbers.  If this option is not specified, then no
    command will be executed.

  **-yscrollincrement** *numPixels*
    Sets the vertical scrolling distance. The default is 20 pixels.

  ####-------------------------
  
  **-acceleratorfont** *fontName* 
    Specifies the font for the accelerator.  The default is "{Sans Serif}
    9".

  **-acceleratorforeground** *colorName* 
    Specifies the color of the accelerator.  The default is "black".

  **-activeacceleratorforeground** *colorName* 
    Specifies the active color of the accelerator.  The default is "white".

  **-activeforeground** *colorName* 
    Specifies the color of the label when the menu item is active.  The
    default is "white".

  **-activerelief** *reliefName* 
    Specifies the relief of active menu items.  This determines the 3-D
    effect for the menu item.  *ReliefName* indicates how the item should
    appear relative to the menu window. Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means the item should appear to protrude.  The
    default is "flat".
    
  **-background** *bgName* 
    Specifies the background of the menu items.  *BgName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of the menu.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the menu.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-checkbuttoncolor** *colorName*
    Specifies the color of the check for checkbutton items.  The default is
    "red3".

  **-checkbuttonfillcolor** *colorName*
    Specifies the fill color of the box for checkbutton items. If
    *colorName* is "", then the box color is the background color of
    the menu item.  The default is "".

  **-checkbuttonoutlinecolor** *colorName*
    Specifies the outline color of the box for checkbutton items.  If
    *colorName* is "", then the no outline is drawn. The default is "".

  **-checkbuttonsize** *numPixels*
    Specifies the size of the box of for checkbutton items.  *NumPixels* is
    a non-negative value indicating the width and height of the check
    box. The value may have any of the forms accept able to Tk_GetPixels.
    The default is "12".

  **-command** *string* 
    Specifies a TCL command to be invoked when a menu item is selected:
    either by clicking on the menu item or using the **select** operation.
    If *string* is "", then no command is invoked. The default is "".

  **-cursor** *cursorName* 
    Specifies the cursor to be used for the widget. *CursorName* may have
    any of the forms acceptable to **Tk_GetCursor**.  If *cursorName* is "",
    this indicates that the widget should defer to its parent for cursor
    specification.  The default is "".

  **-disabledacceleratorforeground** *colorName* 
    Specifies the color of the accelerator of menu items that are
    disabled. The default is "grey90".

  **-disabledbackground** *colorName* 
    Specifies the background of menu items that are disabled.  *ColorName*
    may be a color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-disabledforeground** *colorName* 
    Specifies the color of the label for menu items that are disabled.  The
    default is "grey70".

  **-font** *colorName* 
    Specifies the font of labels in menu items.  The default is "{Sans
    Serif} 11".

  **-foreground** *colorName* 
    Specifies the color of labels in menu items.  The default is "black".

  **-height** *numPixels* 
    Specifies the height in the *combotree*.  *NumPixels* can be single
    value or a list.  If *numPixels* is a single value it is a non-negative
    value indicating the height the menu. The value may have any of the
    forms accept able to **Tk_GetPixels**, such as "200" or "2.4i".  If
    *numPixels* is a 2 element list, then this sets the minimum and maximum
    limits for the height of the menu. The menu will be at least the
    minimum height and less than or equal to the maximum. If *numPixels* is
    a 3 element list, then this specifies minimum, maximum, and nominal
    height or the menu.  The nominal size overrides the calculated height
    of the menu.  If *numPixels* is "", then the height of the menu is
    calculated based on all the menu items.  The default is "".

  **-iconvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    name of the image representing the icon of the selected menu item.  If
    *varName* is "", no variable is used. The default is "".

  **-itemborderwidth** *numPixels* 
    Specifies the borderwidth of menu items in the menu.  *NumPixels* is a
    non-negative value indicating the width of the 3-D border drawn around
    the item. The value may have any of the forms acceptable to
    **Tk_GetPixels**.  The default is "0". 

  **-postcommand** *string* 
    Specifies a TCL command to invoked when the menu is posted.  The
    command will be invoked before the menu is displayed onscreen.  For
    example, this may be used to disable menu items that may not be valid
    when the menu is posted. If *string* is "", no command is invoked.  The
    default is "".

  **-radiobuttoncolor** *colorName*
    Specifies the color of the indicator circle for radiobutton items. The
    indicator circle is displayed when the radiobutton item is
    selected. The default is "red3".

  **-radiobuttonfillcolor** *colorName*
    Specifies the fill color of the circle for radiobutton items.  The
    default is "white".

  **-radiobuttonsize** *numPixels*
    Specifies the size of the circle for radiobutton items.  *NumPixels* is
    a non-negative value indicating the width and height of the radiobutton
    circle. The value may have any of the forms acceptable to
    *Tk_GetPixels*, such as "1.2i".  The default is "12".

  **-relief** *reliefName* 
    Specifies the 3-D effect for the menu.  *ReliefName* indicates how the
    menu should appear relative to the root window. Acceptable values are
    **raised**, **sunken**, **flat**, **ridge**, **solid**, and
    **groove**. For example, "raised" means the menu should appear to
    protrude.  The default is "raised".

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
       
  **-takefocus** *boolean*
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *boolean* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts make the
    decision whether to focus on the window.  The default is "".

  **-textvariable** *varName* 
    Specifies the name of a global TCL variable that will be set to the
    label of the selected item.  If *varName* is "", no variable is
    used. The default is "".

  **-unpostcommand** *string*
    Specifies the TCL command to be invoked when the menu is unposted.  If
    *string* is "", no command is invoked. The default is "".

  **-width** *numPixels*
   Specifies the width in the *combotree*.  *NumPixels* can be single
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
    combotree and doesn't have to exist yet.  At an idle point later, the
    combotree will attach the scrollbar to widget, effectively packing the
    scrollbar into the menu.

  **-xscrollcommand** *string*
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window
    changes, the widget will generate a TCL command by concatenating the
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
    combotree and doesn't have to exist yet.  At an idle point later, the
    combotree will attach the scrollbar to widget, effectively packing the
    scrollbar into the menu.

  **-yscrollcommand** *string*
    Specifies the prefix for a command used to communicate with vertical
    scrollbars.  Whenever the vertical view in the widget's window
    changes, the widget will generate a TCL command by concatenating the
    scroll command and two numbers.  If this option is not specified, then
    no command will be executed.  The widget's initialization script
    will automatically set this for you.

  **-yscrollincrement** *numPixels*
    Sets the vertical scrolling unit.  This is the distance the menu is
    scrolled vertically by one unit. *NumPixels* is a non-negative value
    indicating the width of the 3-D border drawn around the menu. The
    value may have any of the forms accept able to **Tk_GetPixels**.  The
    default is "20".

*pathName* **deactivate** 
  Redisplays all entries using their normal colors.  This typically is
  used by widget bindings to un-highlight entries as the pointer is
  moved over the tree. 

*pathName* **entry activate** *entryName*
  Sets the active entry to *entryName*.  When an entry is active it is
  drawn using its active icon (see the **-activeicon** option).  Note that
  there can be only one active node at a time.  The special id of the
  currently active node is **active**.

*pathName* **entry cget** *option*
  Returns the current value of the configuration option given by *option*.
  *Option* may have any of the values accepted by the **configure**
  operation described below.

*pathName* **entry configure** *entryName* ?\ *option*\ ? ?\ *value*\? ?\ *option* *value* ... ?
  Query or modify the configuration options of the *treeview* entry.  If no
  *option* is specified, returns a list describing all of the available
  options for *entryName* (see **Tk_ConfigureInfo** for information on the
  format of this list).  If *option* is specified with no *value*, then the
  command returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified). In this case, *entryName* may be a label, index,
  or tag, but may not represent more than one entry.  

  If one or more *option*-*value* pairs are specified, then the command
  modifies the given entry option(s) to have the given value(s); in this
  case the command returns an empty string.  In this case, *entryName* may
  refer to multiple entries (such as "all").  *Option* and *value* are
  described below:

  **-bindtags** *tagList*
    Specifies the binding tags for *entryName*.  *TagList* is a list of
    binding tag names.  The tags and their order will determine how events
    are handled for entries.  Each tag in the list matching the current
    event sequence will have its TCL command executed.  The default value
    is "all".

  **-button** *how*
    Indicates whether a button should be displayed on the left side of the
    node entry.  *How* can be "yes", "no", or "auto".  If "auto", then a
    button is automatically displayed if the node has children.  This is
    the default.

  **-closecommand** *cmdString*
    Specifies a TCL script to be invoked when the node is closed.  This
    overrides the global **-closecommand** option for this entry.  The
    default is "".  Percent substitutions are performed on *cmdString*
    before it is executed.  The following substitutions are valid:

    **%W**
      The pathname of the widget.

    **%p**
      The name of the node.

    **%P**
      The full pathname of the node.

    **%#**
      The id of the node.

    **%%**
      Translates to a single percent.

  **-label** *labelString*
    Sets the text for the entry's label.  If not set, this defaults to the
    name of the node. The default is "".

  **-opencommand** *cmdString*
    Specifies a TCL script to be invoked when the entry is opened.  This
    overrides the widget's **-opencommand** option for this node.  The
    default is "".  Percent substitutions are performed on *cmdString*
    before it is executed.  The following substitutions are valid:

    **%W**
      The pathname of the widget.

    **%p**
      The name of the node.

    **%P**
      The full pathname of the node.

    **%#**
      The id of the node.

    **%%**
      Translates to a single percent.

  **-style** *styleName*
     FIXME
     
*pathName* **entry ishidden** *entryName*
  Returns 1 if the node is currently hidden and 0 otherwise.  A node is
  also hidden if any of its ancestor nodes are closed or hidden.
  *EntryName* may be a node id, label, or tag, but may not represent more
  than one entry.

*pathName* **entry isopen** *entryName*
  Returns 1 if the node is currently open and 0 otherwise.
  *EntryName* may be a node id, label, or tag, but may not represent more
  than one entry.

*pathName* **exists** *entryName*
  Returns the *entryName* exists in the tree.  *EntryName* may be a node
  id, label, or tag, but may not represent more than one entry. Returns "1"
  is the entry exists, "0" otherwise.
  
*pathName* **get** *string* ?\ *switches* ... ?
  Searches for the next menu item that matches *string*.  Returns the
  index of the matching item or "-1" if no match is found.  *Switches* can
  be one of the following:

    

*pathName* **hide** *item* ?\ *switches*\ ? ?\ *entry*\ ... ?

  **-exact**
  **-glob**
  **-regexp**
  **-nonmatching**
  **-name** *string*
  **-full** *string*
  **-data** *string*
  **--**
  The **-exact**, **-glob**, and **-regexp** switches indicate both what
  kind of pattern matching to perform and the pattern.  By default each
  pattern will be compared with the node label.  You can set more than one
  of these switches.  If any of the patterns match (logical or), the node
  matches.  If the **-key** switch is used, it designates the data field to
  be matched.  *Switches* may be any of the following.

  **-exact** *string*
    Matches each entry with the label *string*.  

  **-glob** *pattern*
    Test each entry label against *pattern* using global pattern matching.
    Matching is done in a fashion similar to that used by the **glob**
    command.

  **-invert**
    Select non-matching nodes.  Any node that *doesn't* match the given
    criteria will be selected.

  **-name** *tagName*
    Only test nodes that have the tag *tagName*.

  **-regexp** *string*
    Test each entry label *string* as a regular expression pattern.

  **-tag** *tagName*
    Only test nodes that have the tag *tagName*.

*pathName* **identify** *entryName* ?\ **-root**\ ? *x* *y*
  Returns the name of the portion of the entry that the pointer is over.
  *X* and *y* are the window coordinates of the pointer.  If the **-root**
  switch is present, then *x* and *y* represent root coordinates. This
  command returns a string described below.

  **button**
    The mouse pointer is over the entry's button.
  **icon**
    The mouse pointer is over the entry's icon.
  **label**
    The mouse pointer is over the entry's label.
  ""
    The mouse pointer is not over the entry.

*pathName* **index** *entryName* 
  Returns the index of *entryName*. *Item* may be a label, index, or tag, but
  may not represent more than one menu item.  If the entry does not
  exist, "-1" is returned.
  
*pathName* **nearest**  ?\ **-root**\ ? *x* *y*
  Returns the index of the entry closest to the coordinates specified.  *X*
  and *y* are the window coordinates of the pointer.  If the **-root**
  switch is present, then *x* and *y* represent root coordinates.  
  
*pathName* **open** ?\ **-recurse**\ ? *entryName* 

*pathName* **overbutton** *x* *y* 
  for this menu.  *X* and *y* are root coordinates.  This command uses the
  information set by the **post** operation to determine where the button
  region is.  Returns "1" if the coordinate is in the button region, "0"
  otherwise.

*pathName* **post** ?\ *switches* ... ? 
  Arranges for the *pathName* to be displayed on the screen. The position
  of *pathName* depends upon *switches*.

  The position of the *combotree* may be adjusted to guarantee that the
  entire widget is visible on the screen.  This command normally returns an
  empty string.  If the **-postcommand** option has been specified, then
  its value is executed as a TCL script before posting the menu and the
  result of that script is returned as the result of the post widget
  command.  If an error returns while executing the command, then the error
  is returned without posting the menu.

  *Switches* can be one of the following:

  **-align** *how*
    Aligns the menu horizontally to its parent according to *how*.  *How*
    can be "left", "center", or "right".

  **-box** *coordList*
    Specifies the region of the parent window that represent the button.
    Normally combotrees are aligned to the parent window.  This allows you
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
    Specifies the name of window to align the menu to.  Normally *combotree*s
    are aligned to its parent window.  *Window* is the name of another
    widget.

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

*pathName* **see** *item* 
  Scrolls the menu so that *item* is visible in the widget's window.
  *Item* may be a label, index, or tag, but may not represent more than one
  menu item.
  
*pathName* **show**

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
  modifies the given widget option(s) to have the given value(s); in this
  case the command returns an empty string.  *Option* and *value* are
  described below.

  **-acceleratorfont** *fontName* 
    Specifies the font for the accelerator.  The default is "{Sans Serif}
    9".

  **-acceleratorforeground** *colorName* 
    Specifies the color of the accelerator.  The default is "black".

  **-activeacceleratorforeground** *colorName* 
    Specifies the color of the label when the menu item is active.  The
    default is "white".

  **-activeforeground** *colorName* 
    Specifies the active color of the label.  The default is
    "black".

  **-activerelief** *reliefName* 
    Specifies the relief of active menu items.  This determines the 3-D
    effect for the menu item.  *ReliefName* indicates how the item should
    appear relative to the menu window.  Acceptable values are **raised**,
    **sunken**, **flat**, **ridge**, **solid**, and **groove**. For
    example, "raised" means the item should appear to protrude.  The
    default is "flat".
    
  **-background** *bgName* 
    Specifies the background of the menu item.  *BgName* may be a color
    name or the name of a background object created by the
    **blt::background** command.  The default is "white".
    
  **-borderwidth** *numPixels* 
    Specifies the borderwidth of the menu item.  *NumPixels* is a
    non-negative value indicating the width of the 3-D border drawn around
    the menu item. The value may have any of the forms accept able to
    **Tk_GetPixels**.  The default is "1".

  **-checkbuttoncolor** *colorName*
    Specifies the color of the check for *checkbutton* items.  The default is
    "red3".

  **-checkbuttonfillcolor** *colorName*
    Specifies the fill color of the box for *checkbutton* items. If
    *colorName* is "", then the box color is the background color of
    the menu item.  The default is "".

  **-checkbuttonoutlinecolor** *colorName*
    Specifies the outline color of the box for *checkbutton* items.  If
    *colorName* is "", then the no outline is drawn. The default is "".

  **-checkbuttonsize** *numPixels*
    Specifies the size of the box of for *checkbutton* items.  *NumPixels* is
    a non-negative value indicating the width and height of the check
    box. The value may have any of the forms accept able to Tk_GetPixels.
    The default is "12".

  **-disabledacceleratorforeground** *colorName* 
    Specifies the color of the accelerator of menu items that are
    disabled. The default is "grey90".

  **-disabledbackground** *bgName* 
    Specifies the background of menu items that are disabled. *BgName*
    may be a color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-disabledforeground** *colorName* 
    Specifies the color of the label for menu items that are disabled.  The
    default is "grey70".

  **-font** *colorName* 
    Specifies the font of labels in menu items.  The default is "{Sans
    Serif} 11".

  **-foreground** *colorName* 
    Specifies the color of labels in menu items.  The default is "black".

  **-radiobuttoncolor** *colorName*
    Specifies the color of the indicator circle for radiobutton items. The
    indicator circle is displayed when the radiobutton item is
    selected. The default is "red3".

  **-radiobuttonfillcolor** *colorName*
    Specifies the fill color of the circle for radiobutton items.  The
    default is "white".

  **-radiobuttonsize** *numPixels*
    Specifies the size of the circle for radiobutton items.  *NumPixels* is
    a non-negative value indicating the width and height of the radiobutton
    circle. The value may have any of the forms acceptable to
    *Tk_GetPixels*, such as "1.2i".  The default is "12".

  **-relief** *reliefName* 
    Specifies the 3-D effect for the border around the menu item.
    *ReliefName* specifies how the interior of the legend should appear
    relative to the menu.  Acceptable values are **raised**, **sunken**,
    **flat**, **ridge**, **solid**, and **groove**. For example, "raised"
    means the item should appear to protrude from the menu.  The default is
    "flat".

*pathName* **style create** *styleName* ?\ *option* *value* ... ?
  Creates a new style named *styleName*.  By default all menu items use the
  same set of global widget configuration options to specify the item's the
  color, font, borderwidth, etc.  Styles contain sets of configuration
  options that you can apply to a menu items (using the its **-style**
  option) to override their appearance. More than one item can use the same
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
  *pattern* will be returned. *Pattern* is a glob-style pattern.

*pathName* **toggle** *entryName*
  Returns the type of *item*.  The returned type is either "button",
  "cascade", "checkbutton", "radiobutton", or "separator".  *Item* may be a
  label, index, or tag, but may not represent more than one menu item.
   
*pathName* **unpost**
  Unposts the *combotree* window so it is no longer displayed onscreen.  If
  one or more lower level cascaded menus are posted, they are unposted too.

*pathName* **xview moveto** fraction
  Adjusts the horizontal view in the *combotree* window so the portion of
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
  Adjusts the vertical view in the *combotree* window so the portion of
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

There are many default class bindings for *combotree* widgets.

EXAMPLE
-------

Create a *combotree* widget with the **blt::combotree** command.

 ::

    package require BLT

    # Create a new combotree and add menu items to it.

    blt::combobutton .file -text "File" -menu .file.m \
      -xscrollbar .file.xs \
      -yscrollbar .file.ys 

    blt::combotree .file.m 
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

1. You can't use a Tk **menubutton** with *combotree*\ s.  The menu is
   posted by either a **blt::combobutton** or **blt::comboentry**
   widget.

2. You specify scrollbar widgets with the **-xscrollbar** and
   **-yscrollbar** options.  The scrollbars do not already have to exist.

3. You create menu items with the **add** operation.  The type of item is
   specified by the **-type** option.  The default type is "button".

4. You don't pack the scrollbars.  This is done for you.

5. You don't have to specify the **-orient** or **-command** options to
   the scrollbars. This is done for you.


DIFFERENCES WITH TK MENUS
-------------------------

The **blt::combotree** widget has several differences with the Tk **menu**
widget.

1. *Combotree* item types are specified by the **-type** option.

2. *Combotrees* can not be torn off.

3. *Combotrees* can not be invoked by a Tk **menubutton**.

4. *Combotrees* are a single column.
   
KEYWORDS
--------

combotree, widget

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
