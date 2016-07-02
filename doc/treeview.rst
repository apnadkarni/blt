
=============
blt::treeview
=============

----------------------------------------
Create and manipulate treeview widgets.
----------------------------------------

:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::treeview** *pathName* ?\ *option* *value* ... ?

DESCRIPTION
-----------

The *treeview* widget displays a tree of data.  It replaces both the
**hiertable** and **hierbox** widgets.  The **treeview** is 100% syntax
compatible with the **hiertable** widget.  The **hiertable** command is
retained for sake of script-level compatibility.  This widget obsoletes the
**hierbox** widget.  It does everything the old **hierbox** widget did, but
also provides data sharing (via *tree data objects*) and the ability to
tag nodes.

INTRODUCTION
------------

The *treeview* widget displays hierarchical data as a tree.  Data is
represented as nodes in a general-ordered tree.  Each node can have
sub-nodes and these nodes can in turn can have their own children.  The
tree and it data is displayed as a table: each row of the table represents
a node in the tree.  The tree (hierarchical view) is displayed in its own
column.  Extra columns may be display data fields on either side.

Each entry in the tree column has a label and icon.  If a node has
children, its entry is includes with a small button to the left of the
label.  Clicking the mouse over this button opens or closes the node.  When
a node is *open*, its children are exposed.  When it is *closed*, the
children and their descedants are hidden.  The button is normally a "+" or
"-" symbol (ala Windows Explorer), but can be replaced with a pair of Tk
images (open and closed images).

Other columns (displaying data fields in the nodes) run vertically on
either side.  For each node, cells in the columns will display the data
fields.  There are several styles of cells (the default is a text box):

 **textbox**
   Display a text string (may be multi-lined separated by newlines)
   representing the data field.
   
 **checkbox**
   Displays a checkbox and optionally a text string representing the
   data field.  Click on the checkbox can toggle the data field between
   one of two values.
   
 **radiobutton**
   Displays a circular radiobutton and optionally a text string
   representing the data field.  Clicking on the radiobutton can toggle the
   data field between one of two values.

 **combobox**
   Displays a text string and button that will display a **blt::combomenu**
   when clicked.

 **imagebox**
   Displays an image.
   
The data fields at each node can be displayed in tabular columns.  By
default, data fields are text box column You can control the color, font,
etc. of each entry.  Any entry label or data field can be edited in-place.

TREE DATA OBJECT
----------------

The tree is not stored inside the widget but in a tree data object (see the
**blt::tree** command for a further explanation).  Tree data objects can be
shared among different clients, such as a *treeview* widget or the
**blt::tree** command.  You can walk the tree and manage its data with the
**blt::tree** command tree, while displaying it with the *treeview* widget.
Whenever the tree is updated, the *treeview* widget is automatically
redrawn.

By default, the *treeview* widget creates its own tree object.  The tree
initially contains just a root node.  But you can also display trees
created by the **blt::tree** command using the **-tree** configuration
option.  *treeview* widgets can share the same tree object, possibly
displaying different views of the same data.

A tree object has both a TCL and C API.  You can insert or delete nodes
using *treeview* widget or **tree** command operations, but also from C
code.  For example, you can load the tree from your C code while still
managing and displaying the tree from TCL. The widget is automatically
notified whenever the tree is modified via C or TCL.

SYNTAX
------

  **blt::treeview** *pathName* ?\ *option* *value* ... ?

The **blt::treeview** command creates a new window *pathName* and makes it
into a *treeview* widget.  At the time this command is invoked, there must
not exist a window named *pathName*, but *pathName*'s parent must exist.
Additional options may be specified on the command line or in the option
database to configure aspects of the widget such as its colors and font.
See the widget's **configure** operation below for the exact details about
what *option* and *value* pairs are valid.

If successful, **treeview** returns the path name of the widget.  It also
creates a new TCL command by the same name.  You can use this command to
invoke various operations that query or modify the widget.  The general
form is:

  *pathName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available are described in the
`TREEVIEW OPERATIONS`_ section.

REFERENCING NODES
-----------------

Tree nodes can be refererenced in the same ways the **blt::tree** command:
by id or by tag (see the **blt::tree** manual for details). In addition the
*treeview* widget has special identifiers specific to the widget.

  **active**
    This is the node where the mouse pointer is currently located.  When a
    node is active, it is drawn using its active icon (see the
    **-activeicon** option).  The **active** id is changes when
    you move the mouse pointer over another node or by using the **entry
    activate** operation. Note that there can be only one active node at a
    time.

  **anchor**
    This is the node representing the fixed end of the current selection.  
    The anchor is set by the **selection anchor** operation.  In the is
    no selection, this returns an error.

  **endview**
    This is the last node in the treeview.  If may be different depending
    id **--flatview** is set.

  **current**
    The is the node where the mouse pointer is currently located.  Unlike

  **active**, this id changes while the selection is dragged.  It is used
    to determine the current node during button drags.

  **view.down**
    The next open node from the current focus. The **down** of the last
    open node is the same.

  **end**
    The last open node (in depth-first order) on the tree.  

  **focus**
    The node that currently has focus.  When a node has focus,
    it receives key events.  To indicate focus, the node
    is drawn with a dotted line around its label.  You can change the 
    focus using the **focus** operation.

  **mark**
    The node representing the non-fixed end of the current selection.  
    The mark is set by the **selection mark** operation.

  **view.bottom**
    Last node that's current visible in the widget.

  **view.first**
    This is the first node in the treeview.  If may be different depending
    id **--flatview** is set.

  **view.last**
    This is the first node in the treeview.  If may be different depending
    id **--flatview** is set.

  **view.next**
    The next open node from the current focus.  But unlike **down**,
    when the focus is on last open node, **next** wraps around to the 
    root node.

  **view.prev**
    The last open node from the current focus. But unlike **up**,
    when the focus is at root, **last** wraps around to the last
    open node in the tree.

  **view.top**
    First node that's current visible in the widget.

  **view.up**
    The last open node (in depth-first order) from the current focus. The
    **up** of the root node (i.e. the root has focus) is also the root.

  **@**\ *x*\ **,**\ *y*
    Indicates the node that covers the point in the treeview window
    specified by *x* and *y* (in pixel coordinates).  If no
    part of the entryd covers that point, then the closest node to that
    point is used.

REFERENCING COLUMNS
-------------------

  **active**
    This is the node where the mouse pointer is currently located.  When a
    node is active, it is drawn using its active icon (see the
    **-activeicon** option).  The **active** id is changes when
    you move the mouse pointer over another node or by using the **entry
    activate** operation. Note that there can be only one active node at a
    time.

  **current**
    The is the node where the mouse pointer is currently located.  Unlike

  **treeView**
    The is the node where the mouse pointer is currently located.  Unlike

  *index*
    The index of the column.  Columns are number from zero. Add and deleting
    columns may change the indices of other columns.

  *name*
     The name of the column.
     
DATA FIELDS
-----------

A node in the tree can have *data fields*.  A data field is a
name-value pair, used to represent arbitrary data in the node.  Nodes
can contain different fields (they aren't required to contain the same
fields).  You can optionally display these fields in the
*treeview* widget in columns running on either side of the
displayed tree.  A node's value for the field is drawn in the column
along side its node in the hierarchy.  Any node that doesn't have a
specific field is left blank.  Columns can be interactively resized,
hidden, or, moved.

ENTRY BINDINGS
--------------

You can bind TCL commands to be invoked when events occur on nodes
(much like Tk canvas items).  You can bind a node using its id or
its *bindtags*.  Bindtags are simply names that associate a
binding with one or more nodes.  There is a built-in tag "all"
that all node entries automatically have.

TREEVIEW OPERATIONS
-------------------

The **treeview** operations are the invoked by specifying
the widget's pathname, the operation, and any arguments that pertain 
to that operation.  The general form is:

  *pathName* *operation* ?\ *arg* *arg* ...  ?

*Operation* and the *arg*s determine the exact behavior of the
command.  The following operation are available for *treeview* widgets:

*pathName* **bbox** ?**-screen**? *tagOrId...*
  Returns a list of 4 numbers, representing a bounding box of around the
  specified entries. The entries is given by one or more *tagOrId*
  arguments.  If the **-screen** flag is given, then the x-y coordinates of
  the bounding box are returned as screen coordinates, not virtual
  coordinates. Virtual coordinates start from "0" from the root node.  The
  returned list contains the following values.

  *x* 
     X-coordinate of the upper-left corner of the bounding box.

  *y*
     Y-coordinate of the upper-left corner of the bounding box.

  *width*
     Width of the bounding box.

  *height*
     Height of the bounding box.

*pathName* **bind** *tagName* ?\ *sequence*\? ?\ *command*\ ?
  Associates *command* with *tagName* such that whenever the event sequence
  given by *sequence* occurs for a node with this tag, *command* will be
  invoked.  The syntax is similar to the **bind** command except that it
  operates on **treeview** entries, rather than widgets. See the **bind**
  manual entry for complete details on *sequence* and the substitutions
  performed on *command* before invoking it.

  If all arguments are specified then a new binding is created, replacing
  any existing binding for the same *sequence* and *tagName*.  If the first
  character of *command* is "+" then *command* augments an existing binding
  rather than replacing it.  If no *command* argument is provided then the
  command currently associated with *tagName* and *sequence* (it's an error
  occurs if there's no such binding) is returned.  If both *command* and
  *sequence* are missing then a list of all the event sequences for which
  bindings have been defined for *tagName*.

*pathName* **button activate** *tagOrId*
  Designates the node given by *tagOrId* as active.  
  When a node is active it's entry is drawn using its active icon 
  (see the **-activeicon** option). 
  Note that there can be only one active entry at a time.
  The special id **active** indicates the currently active node.

*pathName* **button bind** *tagName* ?\ *sequence*\ ? ?\ *command*\ ?
  Associates *command* with *tagName* such that whenever the event sequence
  given by *sequence* occurs for an button of a node entry with this tag,
  *command* will be invoked.  The syntax is similar to the **bind** command
  except that it operates on **treeview** buttons, rather than widgets. See
  the **bind** manual entry for complete details on *sequence* and the
  substitutions performed on *command* before invoking it.

  If all arguments are specified then a new binding is created, replacing
  any existing binding for the same *sequence* and *tagName*.  If the first
  character of *command* is "+" then *command* augments an existing binding
  rather than replacing it.  If no *command* argument is provided then the
  command currently associated with *tagName* and *sequence* (it's an error
  occurs if there's no such binding) is returned.  If both *command* and
  *sequence* are missing then a list of all the event sequences for which
  bindings have been defined for *tagName*.

*pathName* **button cget** *option*
  Returns the current value of the configuration option given by *option*.
  *Option* may have any of the values accepted by the **configure**
  operation described below.

*pathName* **button configure** ?*option*? ?\ *value*\ ? ?\ *option* *value* ... ?
  Query or modify the configuration options of the widget.  If no *option*
  is specified, returns a list describing all of the available options for
  *pathName* (see **Tk_ConfigureInfo** for information on the format of
  this list).  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*-*value* pairs are
  specified, then the command modifies the given widget option(s) to have
  the given value(s); in this case the command returns an empty string.
  *Option* and *value* are described in the section `BUTTON OPTIONS`_
  below.

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

  **-closerelief** *relief*
    Specifies the 3-D effect for the closed button.  *Relief* indicates how
    the button should appear relative to the widget; for example, "raised"
    means the button should appear to protrude.  The default is "solid".

  **-foreground** *colorName* 
    Sets the foreground color of buttons.  The default is "black".

  **-images** *imageList*
    Specifies images to be displayed for the button.  *ImageList* is a list of
    two Tk images: the first image is displayed when the button is open,
    the second when it is closed.  If the *imageList* is the empty string,
    then a plus/minus gadget is drawn.  The default is "".

  **-openrelief** *relief*
    Specifies the 3-D effect of the open button.  *Relief* indicates how
    the button should appear relative to the widget; for example, "raised"
    means the button should appear to protrude.  The default is "flat".

  **-size** *numPixels*
    Sets the requested size of the button.  The default is "0".

*pathName* **cget** *option*
  Returns the current value of a widget configuration option.  *Option* may
  have any of the values accepted by the **configure** operation described
  below.

*pathName* **close** ?\ **-recurse**\ ? *entryName* ... ?
  Closes the entry specified by *entryName*.  In addition, if a TCL
  script was specified by the **-closecommand** option, it is
  invoked.  If the entry is already closed, this command has no effect.
  If the **-recurse** flag is present, each child node is
  recursively closed.

*pathName* **column activate** ?\ *columnName*\ ?
  Sets or gets the active column.  If no *columnName* argument is given,
  this command returns the name of the currently active column.  Otherwise
  *columnName* is the name of a column in the *treeview* widget to be made
  active. When a column is active, it's drawn using its
  **-activetitlebackground** and **-activetitleforeground** colors. If
  *columnName* is the "", then no column will be active.

*pathName* **column cget** *columnName* *option*
  Returns the current value of a column configuration option for
  *columnName*.  *ColumnName* is the name of column in the widget that
  corresponds to a data field in the tree.  *Option* may have any of the
  values accepted by the **column configure** operation described below.

*pathName* **column configure** *columnName* ?\ *option*\ ? ?\ *value*\ ? ?\ *option* *value* ... ?
  Query or modify the configuration options of the column designated by
  *columnName*. *ColumnName* is the name of the column in the widget that
  corresponds to a data field in the tree.  If no *option* is specified,
  returns a list describing all of the available options for *pathName*
  (see **Tk_ConfigureInfo** for information on the format of this list).
  If *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sublist of the value returned if no *option* is specified).
  If one or more *option*-*value* pairs are specified, then the command
  modifies the given widget option(s) to have the given value(s); in this
  case the command returns an empty string.  *Option* and *value* are
  described below.

  **-activetitlebackground** *colorName*

  **-activetitleforeground** *colorName*

  **-bindtags** *tagList*
    Specifies the binding tags *columnName*.  *TagList* is a list of binding
    tag names.  The tags and their order will determine how events are
    handled for columns.  Each tag in the list matching the current event
    sequence will have its TCL command executed.  The default value is
    "all".

  **-borderwidth** *numPixels*
    Sets the width of the 3-D border of the column.  The column's
    **-relief** option (see below) determines if a border is to be drawn.
    The default is "0".

  **-command** *cmdPrefix*
    Specifies a TCL procedure to be called when column's **invoke**
    operation is executed.  This typically happens when the column title is
    clicked.  *CmdPrefix* is called with 2 extra arguments (the pathname of
    the widget and the name of the column) that are appended to the end.
    
  **-decreasingicon** *imageName*
    Specifies an image to displayed when the column is sorted in decreasing
    order. *ImageName* is the name of Tk image.  The default is image
    is a red arrow.

  **-formatcommand** *cmdPrefix*
    Specifies a TCL procedure to be called to format the contents of cells
    in *columnName*. This lets you display the data field values in a
    readable form while retaining their original format.  *CmdPrefix* is
    called with 2 extra arguments (the node id of the entry and the cell's
    value) that are appended to the end.

  **-hide** *boolean*
    If *boolean* is true, the column is not displayed.  The default is
    "yes".

  **-icon** *imageName*
    Specifies an image to displayed to the left of the column title.
    *ImageName* is the name of Tk image.  If *imageName* is "", then
    no icon is display. The default is "".

  **-increasingicon** *imageName*
    Specifies an image to displayed when the column is sorted in increasing
    order. *ImageName* is the name of Tk image.  The default is image
    is a blue arrow.

  **-justify** *justify*
    Specifies how the column data fields title should be justified within
    the column.  This matters only when the column is wider than the data
    field to be display.  *Justify* must be "left", "right", or "center".
    The default is "left".

  **-max** *relief*

  **-min** *relief*

  **-pad** *numPixels*
    Specifies how much padding for the left and right sides of the column.
    *NumPixels* is a list of one or two screen distances.  If *numPixels*
    has two elements, the left side of the column is padded by the first
    distance and the right side by the second.  If *numPixels* has just one
    distance, both the left and right sides are padded evenly.  The default
    is "2".

  **-relief** *relief*
    Specifies the 3-D effect of the column.  *Relief* specifies how the
    column should appear relative to the widget; for example, "raised"
    means the column should appear to protrude.  The default is "flat".

  **-rulecolor** *colorName*

  **-ruledashes** *dashlist*

  **-rulewidth** *numPixels*

  **-show** *boolean*

  **-sortcommand** *cmdPrefix*

  **-sorttype** *sortType*

  **-state** *state*
    Sets the state of *columnName*. If *state* is "disable" then the column
    title can not be activated nor invoked.  The default is "normal".

  **-title** *string*
    Sets the title for *columnName*.  The default is "".

  **-titlebackground** *colorName* 
    Sets the background color of the column title.  The default is "black".

  **-titleborderwidth** *numPixels*
    Sets the width of the 3-D border around the column title.  The
    **-titlerelief** option determines if a border is to be drawn.  The
    default is "0".

  **-titlefont** *fontName* 
    Sets the font for a column's title. The default is "{Sans Serif} 9".

  **-titleforeground** *colorName* 
    Sets the foreground color of the column title.  The default is "black".

  **-titlejustify** *justify*
    Specifies how the column title should be justified within the column.
    This matters only when the column is wider than the title.  *Justify*
    must be "left", "right", or "center".  The default is "left".

  **-titlerelief** *relief*
    Specifies the 3-D effect of the column title.  *Relief* specifies how the
    title should appear relative to the widget; for example, "raised"
    means the title should appear to protrude.  The default is "flat".

  **-weight** *number*
    Sets the requested width of the column.  This overrides the computed
    with of the column.  If *numPixels* is 0, the width is computed as from
    the contents of the column. The default is "0".

  **-width** *numPixels*
    Sets the requested width of the column.  This overrides the computed
    with of the column.  If *numPixels* is 0, the width is computed as from
    the contents of the column. The default is "0".

*pathName* **column delete** ?\ *columnName* ... ?
  Deletes one of more columns designated by *columnName*.  Note that you
  can't delete the "treeView" column and that deleting a column does not
  delete the corresponding data field in the tree. *ColumnName* is the
  name of a column returned by the **column create** operation.

*pathName* **column insert** *insertPos* *fieldName* ?\ *option* *value* ... ?
  Creates a new column named *fieldName*.  A column displays data fields
  with the same name.  *FieldName* is the name of the new column and a data
  field.  The data field doesn't have to exist (all the cells will be
  empty).  But a column named *fieldName* must not already exist in the
  widget.  *InsertPos* specifies where to position the column in the list of
  columns. *InsertPos* can be an index or "end". For example, if *insertPos*
  is "0", the new column will be the left most column.

*pathName* **column invoke** *columnName*
  Invokes the TCL command associated with *columnName*, if there is one
  (see the column's **-command** option).  This command is ignored if the
  column's **-state** option set to "disabled".

*pathName* **column move** *srcName* *destName* 
  Moves the column *srcName* to the destination position.  *SrcName* is the
  name of a column.  *DestName* can be either the name of another column or
  a screen position in the form **@**\ *x*\ **,**\ *y*.

*pathName* **column names** ? *pattern* ... ?
  Returns the names of all the columns in the widget. If one or more
  *pattern* arguments are provided, then the name of any column matching
  *pattern* will be returned. *Pattern* is a glob-style pattern.

*pathName* **column nearest**  *x* *y* ?\ *switches* ... ?
  Returns the name of the column closest to the given screen
  coordinate.  *x* and *y* are screen coordinates relative to the
  treeview window unless the **-root** switch is given.
  *Switches* can be any of the following.

  **-root** 
    Indicates that *x* and *y* are root coordinates (they 
    are relative to the root window).  By default the coordinates
    are relative to the treeview window.

  **-title**
    Return the name of the column only if the pointer is over the column's
    title.

*pathName* **configure** ?\ *option*\ ? ?\ *value*\ ? ? *option value* ... ?
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

  **-activebackground** *colorName*
    Sets the background color for active entries.  A node is active when
    the mouse passes over it's entry or using the **activate** operation.

  **-activeforeground** *colorName*
    Sets the foreground color of the active node.  A node is active when
    the mouse passes over it's entry or using the **activate** operation.

  **-activeicons** *images*
    Specifies images to be displayed for an entry's icon when it is
    active. *Images* is a list of two Tk images: the first image is
    displayed when the node is open, the second when it is closed.

  **-autocreate** *boolean*
    If *boolean* is true, automatically create missing ancestor nodes when
    inserting new nodes. Otherwise flag an error.  The default is "no".

  **-allowduplicates** *boolean*
    If *boolean* is true, allow nodes with duplicate pathnames when
    inserting new nodes.  Otherwise flag an error.  The default is "no".

  **-background** *colorName*
    Sets the background color of the widget.  The default is "white".

  **-borderwidth** *numPixels*
    Sets the width of the 3-D border around the outside edge of the widget.
    The **-relief** option determines if the border is to be drawn.  The
    default is "2".

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

  **-dashes** *number*
    Sets the dash style of the horizontal and vertical lines drawn
    connecting entries. *Number* is the length in numPixels of the dashes and
    gaps in the line. If *number* is "0", solid lines will be drawn. The
    default is "1" (dotted).

  **-exportselection** *boolean* 
    Indicates if the selection is exported.  If the widget is exporting its
    selection then it will observe the standard X11 protocols for handling
    the selection.  Selections are available as type **STRING**; the value
    of the selection will be the label of the selected nodes, separated by
    newlines.  The default is "no".

  **-flat** *boolean*
    Indicates whether to display the tree as a flattened list.  If
    *boolean* is true, then the hierarchy will be a list of full paths for
    the nodes.  This option also has affect on sorting.  See the **sort**
    operation** section for more information.  The default is "no".

  **-focusdashes** *dashList* 
    Sets the dash style of the outline rectangle drawn around the entry
    label of the node that current has focus. *Number* is the length in
    numPixels of the dashes and gaps in the line.  If *number* is "0", a solid
    line will be drawn. The default is "1".

  **-focusforeground** *colorName* 
    Sets the color of the focus rectangle.  The default is "black".

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

  **-hideroot** *boolean*
    If *boolean* is true, it indicates that no entry for the root node
    should be displayed.  The default is "no".

  **-highlightbackground**  *colorName*
    Specifies the normal color of the traversal highlight region when the
    widget does not have the input focus.

  **-highlightcolor** *colorName*
    Specifies the color of the traversal highlight rectangle when the
    widget has the input focus.  The default is "black".

  **-highlightthickness** *numPixels*
    Specifies the width of the highlight rectangle indicating when the
    widget has input focus. The value may have any of the forms acceptable
    to **Tk_GetPixels**.  If the value is zero, no focus highlight will be
    displayed.  The default is "2".

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

  **-relief** *relief*
    Specifies the 3-D effect for the widget.  *Relief* specifies how the
    *treeview* widget should appear relative to widget it is packed into;
    for example, "raised" means the *treeview* widget should appear to
    protrude.  The default is "sunken".

  **-scrollmode** *mode* 
    Specifies the style of scrolling to be used.  The following styles are
    valid.  This is the default is "hierbox".

    **listbox**
      Like the **listbox** widget, the last entry can always be scrolled to
      the top of the widget window.  This allows the scrollbar thumb to
      shrink as the last entry is scrolled upward.

    **hierbox**
      The last entry can only be viewed at the bottom of the widget window.
      The scrollbar stays a constant size.

    **canvas**
      Like the **canvas** widget, the entries are bound within the
      scrolling area.

  **-selectbackground** *colorName*
    Sets the background color selected node entries.  The default is
    "#ffffea".

  **-selectborderwidth** *numPixels*
    Sets the width of the raised 3-D border drawn around the labels of
    selected entries. The default is "0".

  **-selectcommand** *string*
    Specifies a TCL script to invoked when the set of selected nodes
    changes.  The default is "".

  **-selectforeground** *colorName*
    Sets the color of the labels of selected node entries.  The default is
    "black".

  **-selectmode** *mode*
    Specifies the selection mode. If *mode* is "single", only one node can
    be selected at a time.  If "multiple" more than one node can be
    selected.  The default is "single".

  **-separator** *string*
    Specifies the character sequence to use when spliting the path
    components.  The separator may be several characters wide (such as
    "::") Consecutive separators in a pathname are treated as one.  If
    *string* is the empty string, the pathnames are TCL lists.  Each
    element is a path component.  The default is "".

  **-showtitles** *boolean*
    If *boolean* is false, column titles are not be displayed.  The default
    is "yes".

  **-sortselection** *boolean*
    If *boolean* is true, nodes in the selection are ordered as they are
    currently displayed (depth-first or sorted), not in the order they were
    selected. The default is "no".

  **-takefocus** *focus* 
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *focus* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts make the
    decision whether to focus on the window.  The default is "1".

  **-trim** *string*
    Specifies a string leading characters to trim from entry pathnames
    before parsing.  This only makes sense if the **-separator** is also
    set.  The default is "".

  **-width** *numPixels*
    Sets the requested width of the widget.  If *numPixels* is 0, then the
    with is computed from the contents of the *treeview* widget.  The
    default is "200".

  **-xscrollcommand** *string*
    Specifies the prefix for a command used to communicate with horizontal
    scrollbars.  Whenever the horizontal view in the widget's window
    changes, the widget will generate a TCL command by concatenating the
    scroll command and two numbers.  If this option is not specified, then
    no command will be executed.

  **-xscrollincrement** *numPixels*
    Sets the horizontal scrolling distance. The default is 20 pixels.

  **-yscrollcommand** *string*
    Specifies the prefix for a command used to communicate with vertical
    scrollbars.  Whenever the vertical view in the widget's window changes,
    the widget will generate a TCL command by concatenating the scroll
    command and two numbers.  If this option is not specified, then no
    command will be executed.

  **-yscrollincrement** *numPixels*
    Sets the vertical scrolling distance. The default is 20 pixels.

*pathName* **curselection**
  Returns a list containing the ids of all of the entries that are
  currently selected.  If there are no entries are selected, then the empty
  string is returned.

*pathName* **delete** ?\ *entryName* ... ?
  Deletes one or more entries given by *entryName* and its children.

*pathName* **entry activate** *entryName*
  Sets the active entry to *entryName*.  When an entry is active it is
  drawn using its active icon (see the **-activeicon** option).  Note that
  there can be only one active node at a time.  The special id of the
  currently active node is **active**.

*pathName* **entry cget** *option*
  Returns the current value of the configuration option given by *option*.
  *Option* may have any of the values accepted by the **configure**
  operation described below.

*pathName* **entry children** *entryName*  ?\ *firstPos*\ ? ?\ *lastPos*\ ?
  Returns a list of ids for the given range of children of *entryName*.
  *EntryName* is the id or tag of the node to be examined.  If only a
  *firstPos* argument is present, then the id of the that child at that
  numeric position is returned.  If both *firstPos* and *lastPos* arguments
  are given, then the ids of all the children in that range are returned.
  Otherwise the ids of all children are returned.

*pathName* **entry configure** ?\ *option*\ ? ?\ *value*\? ?\ *option* *value* ... ?
  Query or modify the configuration options of the widget.  If no *option*
  is specified, returns a list describing all of the available options for
  *pathName* (see **Tk_ConfigureInfo** for information on the format of
  this list).  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*-*value* pairs are
  specified, then the command modifies the given widget option(s) to have
  the given value(s); in this case the command returns an empty string.
  *Option* and *value* are described below:

  **-bindtags** *tagList*
    Specifies the binding tags for entries.  *TagList* is a list of binding
    tag names.  The tags and their order will determine how events are
    handled for entries.  Each tag in the list matching the current event
    sequence will have its TCL command executed.  The default value is
    "all".

  **-button** *how*
    Indicates whether a button should be displayed on the left side of the
    node entry.  *How* can be "yes", "no", or "auto".  If "auto", then a
    button is automatically displayed if the node has children.  This is
    the default.

  **-closecommand** *commandString*
    Specifies a TCL script to be invoked when the node is closed.  This
    overrides the global **-closecommand** option for this entry.  The
    default is "".  Percent substitutions are performed on *commandString*
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

  **-command** *commandString*

  **-data** *string*
    Sets data fields for the node.  *String* is a list of name-value pairs
    to be set. The default is "".

  **-font** *fontName* 
    Sets the font for entry labels.  This overrides the widget's **-font**
    option for this node.  The default is "{Sans Serif} 9".

  **-foreground** *colorName* 
    Sets the text color of the entry label.  This overrides the widget's
    **-foreground** configuration option.  The default is "".

  **-icons** *imageList*
    Specifies images to be displayed for the entry's icon.  This overrides
    the global **-icons** configuration option.  *ImageList* is a list of
    two Tk images: the first image is displayed when the node is open, the
    second when it is closed.

  **-label** *string*
    Sets the text for the entry's label.  If not set, this defaults to the
    name of the node. The default is "".

  **-opencommand** *commandString*
    Specifies a TCL script to be invoked when the entry is opened.  This
    overrides the widget's **-opencommand** option for this node.  The
    default is "".  Percent substitutions are performed on *commandString*
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

  **-rulecolor** *colorName*

  **-ruleheight** *numPixels*

  **-styles** *styleList*

*pathName* **entry delete** *entryName* ?\ *firstPos* *lastPos*\ ?
  Deletes the one or more children nodes of the parent *tagOrId*.  If
  *firstPos* and *lastPos* arguments are present, they are positions
  designating a range of children nodes to be deleted.

*pathName* **entry isbefore** *entryName1* *entryName2*
  Returns 1 if *entryName1* is before *entryName2* and 0 otherwise.

*pathName* **entry ishidden** *entryName*
  Returns 1 if the node is currently hidden and 0 otherwise.  A node is
  also hidden if any of its ancestor nodes are closed or hidden.

*pathName* **entry isopen** *entryName*
  Returns 1 if the node is currently open and 0 otherwise.

*pathName* **entry size** **-recurse** *entryName*
  Returns the number of children for parent node *entryName*.  If the
  **-recurse** flag is set, the number of all its descendants is returned.
  The node itself is not counted.

*pathName* **find** ?\ *switches* ... ? *first* *last*
  Finds for all entries matching the criteria given by *flags*.  A list of
  ids for all matching nodes is returned. *First* and *last* are ids
  designating the range of the search in depth-first order. If *last* is
  before *first*, then nodes are searched in reverse order.  *Switches*
  can be any of the following.

  **-addtag** *tag*
    Add the tag *tag* to all the selected nodes.

  **-count** *numMatches*
    Stop after selecting *numMatches* nodes.

  **-exact**
    Patterns must match exactly.  The is the default.

  **-exec** *string*
    Specifies a TCL script to be invoked for each selected node.
    Percent substitutions are performed on *string* before 
    it is executed.  The following substitutions are valid:

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

    **-count** *number*
     Stop searching after *number* matches.

    **--**
     Indicates the end of flags.

  **-full** *pattern*
    Match *pattern* against the full node pathnames.

  **-glob**
    Use global pattern matching.  Matching is done in a fashion similar to
    that used by the C-shell.  For the two strings to match, their contents
    must be identical except that the following special sequences may
    appear in pattern:

  **-name** *pattern*
    Match pattern node names.

  **-nonmatching**
    Select entries that don't match.  

  **-regexp**
    Use regular expression pattern matching (i.e. the same as implemented
    by the **regexp** command).  

  **-tag** *tag*
    Match nodes with the tag *tag*.

  **-**\ *option* *pattern*
    Specifies pattern to match against the node entry's configuration option.

    ** * **
      Matches  any  sequence  of  characters in
      string, including a null string.

    **?**
      Matches any single character in string.

    **[**\ *chars*\ **]**
      Matches any character in the set given by *chars*. If a sequence of
      the form *x*-*y* appears in *chars*, then any character between *x*
      and *y*, inclusive, will match.

    *x*
      Matches the single character *x*.  This provides a way of avoiding
      the special interpretation of the characters "\*?[]\\" the pattern.

*pathName* **focus** *tagOrId*
  Sets the focus to the node given by *tagOrId*.  When a node has focus, it
  can receive keyboard events.  The special id **focus** designates the
  node that currently has focus.

*pathName* **get** ?\ *switches* ... *tagOrId* ... 
  Translates one or more ids to their node entry names.  It returns a list of 
  names for all the ids specified. *Switches* can be any of the following.

  **-full**
     Full pathnames are returned..  

  Note: If the widget's **-separator** option is the empty string (the
  default), the result is always a list of lists, even if there is only one
  node specified.
 
*pathName* **hide** ?\ *switches* ... ? *tagOrId* ...
  Hides all nodes matching the criteria given by *flags*.  The
  search is performed recursively for each node given by *tagOrId*.
  *Switches* can be any of the following.

  **-name** *pattern*
    Specifies pattern to match against node names.

  **-full** *pattern*
    Specifies pattern to match against node pathnames.

  **-**\ *option* *pattern*
    Specifies pattern to match against the node entry's configuration option.

  **-exact**
   Match patterns exactly.  The is the default.

  **-glob**
    Use global pattern matching.  Matching is done in a fashion
    similar to that used by the C-shell.  For  the  two
    strings  to match, their contents must be identical
    except that the  following  special  sequences  may
    appear in pattern:

    ** * **
      Matches  any  sequence  of  characters in
      string, including a null string.

    **?**
      Matches any single character in string.

    **[**\ *chars*\ **]**
      Matches any character in the set given by *chars*. If a sequence of
      the form *x*-*y* appears in *chars*, then any character between *x*
      and *y*, inclusive, will match.

    *x*
      Matches the single character *x*.  This provides a way of avoiding
      the special interpretation of the characters "\*?[]\\" the pattern.

  **-regexp**
    Use regular expression pattern matching (i.e. the same as implemented
    by the **regexp** command).  

  **-nonmatching**
    Hide nodes that don't match.  

  **--**
    Indicates the end of flags.

*pathName* **index** ?\ **-at**\ ?**-path**? *tagOrId*? *string* 
  Returns the id of the node specified by *string*.  *String* may be a tag
  or node id.  Some special ids are normally relative to the node that has
  focus.  The **-at** flag lets you select another node.

*pathName* **insert** ?\ **-at** *tagOrId*\ ? *position* *path* ?\ *option* value* ...? ?\ *path*\ ? ?\ *options *value* ... ? 
  Inserts one or more nodes at *position*.  *Position* is the location
  (number or "end") where the new nodes are added to the parent node.
  *Path* is the pathname of the new node.  Pathnames can be formated either
  as a TCL list (each element is a path component) or as a string separated
  by a special character sequence (using the **-separator** option).
  Pathnames are normally absolute, but the **-at** switch lets you select a
  relative starting point.  Its value is the id of the starting node.

  All ancestors of the new node must already exist, unless the
  **-autocreate** option is set.  It is also an error if a node already
  exists, unless the **-allowduplicates** option is set.

  *Option* and *value* may have any of the values accepted by the **entry
  *configure** operation.  This command returns a list of the ids of the
  new entries.

*pathName* **move** *tagOrId* *how* *destId*
  Moves the node given by *tagOrId* to the destination node.  The
  node can not be an ancestor of the destination.  *DestId* is
  the id of the destination node and can not be the root of the
  tree.  In conjunction with *how*, it describes how the move is
  performed.

  **before**
    Moves the node before the destination node.

  **after**
    Moves the node after the destination node.

  **into**
    Moves the node to the end of the destination's list of children.

*pathName* **nearest** *x* *y* ?\ *varName*\ ?
  Returns the id of the node entry closest to the given X-Y screen
  coordinate.  If the coordinate is not directly over any node, then the
  empty string is returned.  If the argument *varName* is present, this is
  a TCL variable that is set to either "button", "label", "label", or ""
  depending what part of the entry the coordinate lies.

*pathName* **open** ?\ **-recurse**\ ? *tagOrId* ...
  Opens the one or more nodes specified by *tagOrId*.  If a node is not
  already open, the TCL script specified by the **-opencommand** option is
  invoked. If the **-recurse** flag is present, then each descendant is
  recursively opened.

*pathName* **range** ?\ **-open**\ ? *first* *last*
  Returns the ids in depth-first order of the nodes between the *first* and
  *last* ids.  If the **-open** flag is present, it indicates to consider
  only open nodes.  If *last* is before *first*, then the ids are returned
  in reverse order.

*pathName* **scan mark** *x* *y*
  Records *x* and *y* and the current view in the treeview window; used in
  conjunction with later **scan dragto** commands.  Typically this command
  is associated with a mouse button press in the widget.  It returns an
  empty string.

*pathName* **scan dragto** *x* *y*.
  Computes the difference between its *x* and *y* arguments and the *x* and
  *y* arguments to the last **scan mark** command for the widget.  It then
  adjusts the view by 10 times the difference in coordinates.  This command
  is typically associated with mouse motion events in the widget, to
  produce the effect of dragging the list at high speed through the window.
  The return value is an empty string.

*pathName* **see** ?**-anchor** *anchor*? *tagOrId*
  Adjusts the view of entries so that the node given by *tagOrId* is
  visible in the widget window.  It is an error if **tagOrId** is a
  tag that refers to more than one node.  By default the node's entry
  is displayed in the middle of the window.  This can changed using the
  **-anchor** flag.  Its value is a Tk anchor position.

*pathName* **selection anchor** *tagOrId*
  Sets the selection anchor to the node given by *tagOrId*.  If *tagOrId*
  refers to a non-existent node, then the closest node is used.  The
  selection anchor is the end of the selection that is fixed while dragging
  out a selection with the mouse.  The special id **anchor** may be used to
  refer to the anchor node.

*pathName* **selection cancel**
  Clears the temporary selection of entries back to the current anchor.
  Temporary selections are created by the **selection mark** operation.

*pathName* **selection clear** *first* ?\ *last*\ ?
  Removes the entries between *first* and *last* (inclusive) from the
  selection.  Both *first* and *last* are ids representing a range of
  entries.  If *last* isn't given, then only *first* is deselected.
  Entries outside the selection are not affected.

*pathName* **selection clearall**
  Clears the entire selection.  

*pathName* **selection mark** *tagOrId*
  Sets the selection mark to the node given by *tagOrId*.  This causes the
  range of entries between the anchor and the mark to be temporarily added
  to the selection.  The selection mark is the end of the selection that is
  fixed while dragging out a selection with the mouse.  The special id
  **mark** may be used to refer to the current mark node.  If *tagOrId*
  refers to a non-existent node, then the mark is ignored.  Resetting the
  mark will unselect the previous range.  Setting the anchor finalizes the
  range.

*pathName* **selection includes** *tagOrId*
  Returns 1 if the node given by *tagOrId* is currently
  selected, 0 if it isn't.

*pathName* **selection present**
  Returns 1 if any nodes are currently selected and 0 otherwise.

*pathName* **selection set** *first* ?\ *last*\ ?
  Selects all of the nodes in the range between *first* and *last*,
  inclusive, without affecting the selection state of nodes outside that
  range.

*pathName* **selection toggle** *first* ?\ *last*\ ?
  Selects/deselects nodes in the range between *first* and *last*,
  inclusive, from the selection.  If a node is currently selected, it
  becomes deselected, and visa versa.

 
*pathName* **show** ?\ *switches* ... ? *tagOrId* ...
  Exposes all nodes matching the criteria given by *flags*.  This
  is the inverse of the **hide** operation.  The search is performed
  recursively for each node given by *tagOrId*.  The valid flags are
  described below:

  **-name** *pattern**
   Specifies pattern to match against node names.

  **-full** *pattern**
   Specifies pattern to match against node pathnames.

  **-**\ *option* *pattern*
   Specifies pattern to match against the entry's configuration option.

  **-exact**
   Match patterns exactly.  The is the default.

  **-glob**
    Use global pattern matching.  Matching is done in a fashion similar to
    that used by the C-shell.  For the two strings to match, their contents
    must be identical except that the following special sequences may appear
    in pattern:

    ** * **
      Matches any sequence of characters in string, including a null string.

    **?**
      Matches any single character in string.

    **[**\ *chars*\ **]**
      Matches any character in the set given by *chars*. If a sequence of the
      form *x*-*y* appears in *chars*, then any character between 
      *x* and *y*, inclusive, will match.

    **\\**\ *x*
      Matches the single character *x*.  This provides a way of avoiding the
      special interpretation of the characters "\*?[]\\" in the pattern.

  **-regexp**
    Use regular expression pattern matching (i.e. the same as implemented
    by the **regexp** command).  

  **-nonmatching**
    Expose nodes that don't match.  

  **--**
    Indicates the end of flags.

*pathName* **sort auto** ?*boolean*
  Turns on/off automatic sorting of node entries.  If *boolean* is
  true, entries will be automatically sorted as they are opened,
  closed, inserted, or deleted.  If no *boolean* argument is
  provided, the current state is returned.

*pathName* **sort cget** *option*
  Returns the current value of the configuration option given by *option*.
  *Option* may have any of the values accepted by the **sort configure**
  operation described below.

*pathName* **sort configure** ?*option*? ?\ *value*\ ? ?\ *option* *value* ... ?
  Query or modify the sorting configuration options of the widget.  If no
  *option* is specified, returns a list describing all of the available
  options for *pathName* (see **Tk_ConfigureInfo** for information on the
  format of this list).  If *option* is specified with no *value*, then the
  command returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*-*value* pairs are
  specified, then the command modifies the given sorting option(s) to have
  the given value(s); in this case the command returns an empty string.
  *Option* and *value* are described below:

  **-column** *string*
  Specifies the column to sort. Entries in the widget are rearranged
  according to this column.  If *column* is "" then no sort is performed.

  **-command** *string*
  Specifies a TCL procedure to be called when sorting nodes.  The procedure
  is called with three arguments: the pathname of the widget and the fields
  of two entries.  The procedure returns 1 if the first node is greater
  than the second, -1 is the second is greater, and 0 if equal.

  **-decreasing** *boolean*
  Indicates to sort in ascending/descending order.  If *boolean* 
  is true, then the entries as in descending order. The default is 
  "no".

  **-mode** *string*
  Specifies how to compare entries when sorting. *String*
  may be one of the following:

  **ascii**
    Use string comparison based upon the ASCII collation order.
  **dictionary**
    Use dictionary-style comparison.  This is the same as "ascii"
    except (a) case is ignored except as a tie-breaker and (b) if two
    strings contain embedded numbers, the numbers compare as integers, not
    characters.  For example, "bigBoy" sorts between "bigbang" and
    "bigboy", and "x10y" sorts between "x9y" and "x11y".

  **integer**
    Compares fields as integers.
  **real**
    Compares fields as floating point numbers.
  *command*
    Use the TCL proc specified by the **-command** option to compare
    entries when sorting.  If no command is specified, the sort reverts to
    "ascii" sorting.

*pathName* **sort once** ?\ *flags*\ ? *tagOrId* ...
  Sorts the children for each entries specified by *tagOrId*.  By default,
  entries are sorted by name, but you can specify a TCL proc to do your own
  comparisons.

  **-recurse**
    Recursively sort the entire branch, not just the children.

*pathName* **tag add** *string* *id*...
  Adds the tag *string* to one of more entries.

*pathName* **tag delete** *string* *id*...
  Deletes the tag *string* from one or more entries.  

*pathName* **tag forget** *string*
  Removes the tag *string* from all entries.  It's not an error if no
  entries are tagged as *string*.

*pathName* **tag names** ?*id*?
  Returns a list of tags used.  If an *id* argument
  is present, only those tags used by the node designated by *id* 
  are returned.

*pathName* **tag nodes** *string*
  Returns a list of ids that have the tag *string*.  If no node
  is tagged as *string*, then an empty string is returned.

*pathName* **text** *operation* ?*args*?
  This operation is used to provide text editing for cells (data fields in
  a column) or entry labels.  It has several forms, depending on
  *operation*:

*pathName* **text apply**
  Applies the edited buffer, replacing the entry label or data field. The
  edit window is hidden.

*pathName* **text cancel**
 Cancels the editing operation, reverting the entry label or data value
 back to the previous value. The edit window is hidden.

*pathName* **text cget** *value*
  Returns the current value of the configuration option given by *option*.
  *Option* may have any of the values accepted by the **configure**
  operation described below.

*pathName* **text configure** ?\ *option* *value* ... ?
  Query or modify the configuration options of the edit window.  If no
  *option* is specified, returns a list describing all of the available
  options (see **Tk_ConfigureInfo** for information on the format of this
  list).  If *option* is specified with no *value*, then the command
  returns a list describing the one named option (this list will be
  identical to the corresponding sublist of the value returned if no
  *option* is specified).  If one or more *option*-*value* pairs are
  specified, then the command modifies the given widget option(s) to have
  the given value(s); in this case the command returns an empty string.
  *Option* and *value* are described in the section `TEXT EDITING OPTIONS`_
  below.

*pathName* **text delete** *first* last*
  Deletes the characters in the edit buffer between the two given
  character positions.  

*pathName* **text get** ? **-root** ? *x* *y*

*pathName* **text icursor** *index*

*pathName* **text index** *index*
  Returns the text index of given *index*.

*pathName* **text insert** *index* *string*
  Insert the text string *string* into the edit buffer at the index 
  *index*.  For example, the index 0 will prepend the buffer.

*pathName* **text selection adjust** *index*
  Adjusts either the first or last index of the selection.

*pathName* **text selection clear**
  Clears the selection.

*pathName* **text selection from** *index*
  Sets the anchor of the selection.

*pathName* **text selection present**
  Indicates if a selection is present.

*pathName* **text selection range** *start* *end*
Sets both the anchor and mark of the selection.

*pathName* **text selection to** *index*
Sets the unanchored end (mark) of the selection.

*pathName* **toggle** *tagOrId*
  Opens or closes the node given by *tagOrId*.  If the corresponding 
  **-opencommand** or **-closecommand** option is set, then that
  command is also invoked. 

*pathName* **xview**
  Returns a list containing two elements.  Each element is a real fraction
  between 0 and 1; together they describe the horizontal span that is
  visible in the window.  For example, if the first element is .2 and the
  second element is .6, 20% of the *treeview* widget's text is off-screen
  to the left, the middle 40% is visible in the window, and 40% of the text
  is off-screen to the right.  These are the same values passed to
  scrollbars via the **-xscrollcommand** option.

*pathName* **xview** *tagOrId*
  Adjusts the view in the window so that the character position given by
  *tagOrId* is displayed at the left edge of the window.
  Character positions are defined by the width of the character **0**.

*pathName* **xview moveto** *fraction**
  Adjusts the view in the window so that *fraction* of the
  total width of the *treeview* widget's text is off-screen to the left.
  *fraction* must be a fraction between 0 and 1.

*pathName* **xview scroll** *number* *what*
  This command shifts the view in the window left or right according to
  *number* and *what*.  *Number* must be an integer.  *What* must be either
  **units** or **pages** or an abbreviation of one of these.  If *what* is
  **units**, the view adjusts left or right by *number* character units
  (the width of the **0** character) on the display; if it is **pages**
  then the view adjusts by *number* screenfuls.  If *number* is negative
  then characters farther to the left become visible; if it is positive
  then characters farther to the right become visible.

*pathName* **yview**
  Returns a list containing two elements, both of which are real fractions
  between 0 and 1.  The first element gives the position of the node at the
  top of the window, relative to the widget as a whole (0.5 means it is
  halfway through the treeview window, for example).  The second element
  gives the position of the node just after the last one in the window,
  relative to the widget as a whole.  These are the same values passed to
  scrollbars via the **-yscrollcommand** option.

*pathName* **yview** *tagOrId*
  Adjusts the view in the window so that the node given by *tagOrId* is
  displayed at the top of the window.

*pathName* **yview moveto** *fraction*
  Adjusts the view in the window so that the node given by *fraction*
  appears at the top of the window.  *Fraction* is a fraction between 0 and
  1; 0 indicates the first node, 0.33 indicates the node one-third the way
  through the *treeview* widget, and so on.

*pathName* **yview scroll** *number* what*
  This command adjusts the view in the window up or down according to
  *number* and *what*.  *Number* must be an integer.  *What* must be either
  **units** or **pages**.  If *what* is **units**, the view adjusts up or
  down by *number* lines; if it is **pages** then the view adjusts by
  *number* screenfuls.  If *number* is negative then earlier nodes become
  visible; if it is positive then later nodes become visible.


TREEVIEW OPTIONS
----------------

In addition to the **configure** operation, widget configuration
options may also be set by the Tk **option** command.  The class
resource name is "TreeView".

  ::

    option add *TreeView.Foreground white
    option add *TreeView.Background blue

The following widget options are available:

ENTRY OPTIONS
-------------

Many widget configuration options have counterparts in entries.  For
example, there is a **-closecommand** configuration option for both
widget itself and for individual entries.  Options set at the widget
level are global for all entries.  If the entry configuration option
is set, then it overrides the widget option.  This is done to avoid
wasting memory by replicated options.  Most entries will have
redundant options.

There is no resource class or name for entries.


BUTTON OPTIONS
--------------

Button configuration options may also be set by the **option** command.
The resource subclass is "Button".  The resource name is always "button".

  ::

    option add *TreeView.Button.Foreground white
    option add *TreeView.button.Background blue

The following are the configuration options available for buttons.


COLUMN OPTIONS
--------------

Column configuration options may also be set by the **option** command.
The resource subclass is "Column".   The resource name is the 
name of the column.

  ::

    option add *TreeView.Column.Foreground white
    option add *TreeView.treeView.Background blue

The following configuration options are available for columns.

BINDINGS
--------

Tk automatically creates class bindings for treeviews that give them
Motif-like behavior.  Much of the behavior of a *treeview* widget is
determined by its **-selectmode** option, which selects one of two ways of
dealing with the selection.

If the selection mode is **single**, only one node can be selected at a
time.  Clicking button 1 on an node selects it and deselects any other
selected item.

If the selection mode is **multiple**, any number of entries may be
selected at once, including discontiguous ranges.  Clicking
Control-Button-1 on a node entry toggles its selection state without
affecting any other entries.  Pressing Shift-Button-1 on a node entry
selects it, extends the selection.

 1. In **extended** mode, the selected range can be adjusted by pressing
    button 1 with the Shift key down: this modifies the selection to
    consist of the entries between the anchor and the entry under the
    mouse, inclusive.  The un-anchored end of this new selection can also
    be dragged with the button down.

 2. In **extended** mode, pressing button 1 with the Control key down
    starts a toggle operation: the anchor is set to the entry under the
    mouse, and its selection state is reversed.  The selection state of
    other entries isn't changed.  If the mouse is dragged with button 1
    down, then the selection state of all entries between the anchor and
    the entry under the mouse is set to match that of the anchor entry; the
    selection state of all other entries remains what it was before the
    toggle operation began.

 3. If the mouse leaves the treeview window with button 1 down, the window
    scrolls away from the mouse, making information visible that used to
    be off-screen on the side of the mouse.  The scrolling continues until
    the mouse re-enters the window, the button is released, or the end of
    the hierarchy is reached.

 4. Mouse button 2 may be used for scanning.  If it is pressed and dragged
    over the *treeview* widget, the contents of the hierarchy drag at high
    speed in the direction the mouse moves.

 5. If the Up or Down key is pressed, the location cursor (active entry)
    moves up or down one entry.  If the selection mode is **browse** or
    **extended** then the new active entry is also selected and all other
    entries are deselected.  In **extended** mode the new active entry
    becomes the selection anchor.

 6. In **extended** mode, Shift-Up and Shift-Down move the location
    cursor (active entry) up or down one entry and also extend
    the selection to that entry in a fashion similar to dragging
    with mouse button 1.

 7. The Left and Right keys scroll the *treeview* widget view left and
    right by the width of the character **0**.  Control-Left and
    Control-Right scroll the *treeview* widget view left and right by the
    width of the window.  Control-Prior and Control-Next also scroll left
    and right by the width of the window.

 8. The Prior and Next keys scroll the *treeview* widget view up and down
    by one page (the height of the window).

 9. The Home and End keys scroll the *treeview* widget horizontally to
    the left and right edges, respectively.

 10. Control-Home sets the location cursor to the the first entry, selects
     that entry, and deselects everything else in the widget.

 11. Control-End sets the location cursor to the the last entry, selects
     that entry, and deselects everything else in the widget.

 12. In **extended** mode, Control-Shift-Home extends the selection to the
     first entry and Control-Shift-End extends the selection to the last
     entry.

 13. In **multiple** mode, Control-Shift-Home moves the location cursor to
     the first entry and Control-Shift-End moves the location cursor to
     the last entry.

 14. The space and Select keys make a selection at the location cursor
     (active entry) just as if mouse button 1 had been pressed over this
     entry.

 15. In **extended** mode, Control-Shift-space and Shift-Select extend the
     selection to the active entry just as if button 1 had been pressed
     with the Shift key down.

 16. In **extended** mode, the Escape key cancels the most recent
     selection and restores all the entries in the selected range to their
     previous selection state.

 17. Control-slash selects everything in the widget, except in **single**
     and **browse** modes, in which case it selects the active entry and
     deselects everything else.

 18. Control-backslash deselects everything in the widget, except in
     **browse** mode where it has no effect.

 19. The F16 key (labelled Copy on many Sun workstations) or Meta-w copies
     the selection in the widget to the clipboard, if there is a
     selection.

The behavior of *treeview* widgets can be changed by defining new bindings 
for individual widgets or by redefining the class bindings.

WIDGET BINDINGS
~~~~~~~~~~~~~~~

In addition to the above behavior, the following additional behavior
is defined by the default widget class (BltTreeView) bindings.

  **<ButtonPress-2>** 
    Starts scanning. 
  **<B2-Motion>** 
    Adjusts the scan.
  **<ButtonRelease-2>**
    Stops scanning.
  **<B1-Leave>** 
    Starts auto-scrolling.
  **<B1-Enter>**
    Starts auto-scrolling 
  **<KeyPress-Up>** 
    Moves the focus to the previous entry.
  **<KeyPress-Down>** 
    Moves the focus to the next entry.
  **<Shift-KeyPress-Up>**
    Moves the focus to the previous sibling.
  **<Shift-KeyPress-Down>**
    Moves the focus to the next sibling.
  **<KeyPress-Prior>** 
    Moves the focus to first entry.  Closed or hidden entries are ignored.
  **<KeyPress-Next>** 
    Move the focus to the last entry. Closed or hidden entries are ignored.
  **<KeyPress-Left>** 
    Closes the entry.  It is not an error if the entry has no children.
  **<KeyPress-Right>** 
    Opens the entry, displaying its children.  It is not an error if the
    entry has no children.
  **<KeyPress-space>** 
    In "single" select mode this selects the entry.  In "multiple" mode,
    it toggles the entry (if it was previous selected, it is not
    deselected).
  **<KeyRelease-space>** 
    Turns off select mode.
  **<KeyPress-Return>** 
    Sets the focus to the current entry.
  **<KeyRelease-Return>** 
    Turns off select mode.
  **<KeyPress>** 
    Moves to the next entry whose label starts with the letter typed.
  **<KeyPress-Home>** 
    Moves the focus to first entry.  Closed or hidden entries
    are ignored.
  **<KeyPress-End>** 
    Move the focus to the last entry. Closed or hidden entries
    are ignored.
  **<KeyPress-F1>** 
    Opens all entries.
  **<KeyPress-F2>**
    Closes all entries (except root).


BUTTON BINDINGS
~~~~~~~~~~~~~~~

Buttons have bindings.  There are associated with the "all" bindtag (see
the entry's -bindtag option).  You can use the **bind** operation to change
them.

  **<Enter>** 
    Highlights the button of the current entry.
  **<Leave>** 
    Returns the button back to its normal state.
  **<ButtonRelease-1>**
    Adjust the view so that the current entry is visible.


ENTRY BINDINGS
~~~~~~~~~~~~~~

Entries have default bindings.  There are associated with the "all" bindtag
(see the entry's -bindtag option).  You can use the **bind** operation to
modify them.

  **<Enter>** 
    Highlights the current entry.
  **<Leave>** 
    Returns the entry back to its normal state.
  **<ButtonPress-1>** 
    Sets the selection anchor the current entry.
  **<Double-ButtonPress-1>**
    Toggles the selection of the current entry.
  **<B1-Motion>**
    For "multiple" mode only.  Saves the current location of the
    pointer for auto-scrolling.  Resets the selection mark.  
  **<ButtonRelease-1>**
    For "multiple" mode only.  Sets the selection anchor to the 
    current entry.
  **<Shift-ButtonPress-1>**
    For "multiple" mode only. Extends the selection.
  **<Shift-Double-ButtonPress-1>** 
    Place holder. Does nothing.
  **<Shift-B1-Motion>** 
    Place holder. Does nothing.
  **<Shift-ButtonRelease-1>** 
    Stop auto-scrolling.
  **<Control-ButtonPress-1>** 
    For "multiple" mode only.  Toggles and extends the selection.
  **<Control-Double-ButtonPress-1>**
    Place holder. Does nothing.
  **<Control-B1-Motion>** 
    Place holder. Does nothing.
  **<Control-ButtonRelease-1>** 
    Stops auto-scrolling.
  **<Control-Shift-ButtonPress-1>** 
    ???
  **<Control-Shift-Double-ButtonPress-1>**
    Place holder. Does nothing.
  **<Control-Shift-B1-Motion>** 
    Place holder. Does nothing.

COLUMN BINDINGS
~~~~~~~~~~~~~~~

Columns have bindings too.  They are associated with the column's "all"
bindtag (see the column -bindtag option).  You can use the **column bind**
operation to change them.

  **<Enter>** 
    Highlights the current column title.
  **<Leave>** 
    Returns the column back to its normal state.
  **<ButtonRelease-1>**
    Invokes the command (see the column's -command option) if one
    if specified.  

COLUMN RULE BINDINGS
~~~~~~~~~~~~~~~~~~~~

  **<Enter>** 
    Highlights the current and activates the ruler.
  **<Leave>** 
    Returns the column back to its normal state. Deactivates the ruler.
  **<ButtonPress-1>** 
    Sets the resize anchor for the column.
  **<B1-Motion>** 
    Sets the resize mark for the column.
  **<ButtonRelease-1>** 
    Adjust the size of the column, based upon the resize anchor and mark
    positions.

EXAMPLE
-------

The **treeview** command creates a new widget.  

  ::

    package require BLT

    blt::treeview .tv -bg white

A new TCL command ".tv" is also created.  This command can be used to query
and modify the *treeview* widget.  For example, to change the background
color of the table to "green", you use the new command and the widget's
**configure** operation.

  ::

    # Change the background color.
    .tv configure -background "green"

By default, the *treeview* widget will automatically create a new tree
object to contain the data.  The name of the new tree is the pathname of
the widget.  Above, the new tree object name is ".tv".  But you can use the
**-tree** option to specify the name of another tree.

  ::

    # View the tree "myTree".
    .tv configure -tree "myTree"

When a new tree is created, it contains only a root node.  The node is
automatically opened.  The id of the root node is always "0" (you can use
also use the special id "root"). The **insert** operation lets you insert
one or more new entries into the tree.  The last argument is the node's
*pathname*.

  ::

    # Create a new entry named "myEntry"
    set id [.tv insert end "myEntry"]

This appends a new node named "myEntry".  It will positioned as the
last child of the root of the tree (using the position "end").  You
can supply another position to order the node within its siblings.

  ::

    # Prepend "fred".
    set id [.tv insert 0 "fred"]

Entry names do not need to be unique.  By default, the node's label is its
name.  To supply a different text label, add the **-label** option.

  ::

    # Create a new node named "fred"
    set id [.tv insert end "fred" -label "Fred Flintstone"]

The **insert** operation returns the id of the new node.  You can also use
the **index** operation to get this information.

  ::

    # Get the id of "fred"
    .tv index "fred"

To insert a node somewhere other than root, use the **-at** switch.  It
takes the id of the node where the new child will be added.

  ::

    # Create a new node "barney" in "fred".
    .tv insert -at $id end "barney" 

A pathname describes the path to an entry in the hierarchy.  It's a list of
entry names that compose the path in the tree.  Therefore, you can also add
"barney" to "fred" as follows.

  ::

    # Create a new sub-entry of "fred"
    .tv insert end "fred barney" 

Every name in the list is ancestor of the next.  All ancestors must already
exist.  That means that an entry "fred" is an ancestor of "barney" and must
already exist.  But you can use the **-autocreate** configuration option to
force the creation of ancestor nodes.

  ::

    # Force the creation of ancestors.
    .tv configure -autocreate yes 
    .tv insert end "fred barney wilma betty" 

Sometimes the pathname is already separated by a character sequence rather
than formed as a list.  A file name is a good example of this.  You can use
the **-separator** option to specify a separator string to split the path
into its components.  Each pathname inserted is automatically split using
the separator string as a separator.  Multiple separators are treated as
one.

  ::

    .tv configure -separator /
    .tv insert end "/usr/local/tcl/bin" 

If the path is prefixed by extraneous characters, you can automatically
trim it off using the **-trim** option.  It removed the string from the
path before it is parsed.

  ::

    .tv configure -trim C:/windows -separator /
    .tv insert end "C:/window/system" 

You can insert more than one entry at a time with the **insert** operation.
This can be much faster than looping over a list of names.

  ::

    # The slow way
    foreach f [glob $dir/*] {
        .tv insert end $f
    }
    # The fast way
    eval .tv insert end [glob $dir/*]

In this case, the **insert** operation will return a list of ids of the new
entries.

You can delete entries with the **delete** operation.  It takes one or more
tags of ids as its argument. It deletes the entry and all its children.

  ::

    .tv delete $id

Entries have several configuration options.  They control the appearance of
the entry's icon and label.  We have already seen the **-label** option
that sets the entry's text label.  The **entry configure** operation lets
you set or modify an entry's configuration options.

  ::

    .tv entry configure $id -color red -font fixed

You can hide an entry and its children using the **-hide** option.

  ::

    .tv entry configure $id -hide yes

More that one entry can be configured at once.  All entries specified
are configured with the same options.

  ::

    .tv entry configure $i1 $i2 $i3 $i4 -color brown 

An icon is displayed for each entry.  It's a Tk image drawn to the left of
the label.  You can set the icon with the entry's **-icons** option.  It
takes a list of two image names: one to represent the open entry, another
when it is closed.

  ::

    set im1 [image create photo -file openfolder.gif]
    set im2 [image create photo -file closefolder.gif]
    .tv entry configure $id -icons "$im1 $im2"

If **-icons** is set to the empty string, no icons are display.

If an entry has children, a button is displayed to the left of the
icon. Clicking the mouse on this button opens or closes the sub-hierarchy.
The button is normally a "+" or "-" symbol, but can be configured in a
variety of ways using the **button configure** operation.  For example, the
"+" and "-" symbols can be replaced with Tk images.

  ::

    set im1 [image create photo -file closefolder.gif]
    set im2 [image create photo -file downarrow.gif]
    .tv button configure $id -images "$im1 $im2" \\
        -openrelief raised -closerelief raised

Entries can contain an arbitrary number of *data fields*.  Data
fields are name-value pairs.  Both the value and name are strings.
The entry's **-data** option lets you set data fields.

  ::

    .tv entry configure $id -data {mode 0666 group users}

The **-data** takes a list of name-value pairs.  

You can display these data fields as *columns* in the *treeview* widget.
You can create and configure columns with the **column** operation.  For
example, to add a new column to the widget, use the **column insert**
operation.  The last argument is the name of the data field that you want
to display.

  ::

    .tv column insert end "mode"

The column title is displayed at the top of the column.  By default,
it's is the field name.  You can override this using the column's
**-title** option.

  ::

    .tv column insert end "mode" -title "File Permissions"

Columns have several configuration options.  The **column configure**
operation lets you query or modify column options.

  ::

    .tv column configure "mode" -justify left

The **-justify** option says how the data is justified within in the
column.  The **-hide** option indicates whether the column is displayed.

  ::

    .tv column configure "mode" -hide yes

Entries can be selected by clicking on the mouse.  Selected entries
are drawn using the colors specified by the **-selectforeground** 
and **-selectbackground** configuration options.
The selection itself is managed by the **selection** operation.

  ::

    # Clear all selections
    .tv selection clear 0 end
    # Select the root node
    .tv selection set 0 

The **curselection** operation returns a list of ids of all the selected
entries.

  ::

    set ids [.tv curselection]

You can use the **get** operation to convert the ids to their pathnames.

  ::

    set names [eval .tv get -full $ids]

If a treeview is exporting its selection (using the **-exportselection**
option), then it will observe the standard X11 protocols for handling the
selection.  Treeview selections are available as type **STRING**; the value
of the selection will be the pathnames of the selected entries, separated
by newlines.

The **treeview** supports two modes of selection: "single"
and "multiple".  In single select mode, only one entry can be
selected at a time, while multiple select mode allows several entries
to be selected.  The mode is set by the widget's **-selectmode**
option.

  ::

    .tv configure -selectmode "multiple"

You can be notified when the list of selected entries changes.  The
widget's **-selectcommand** specifies a TCL procedure that is called
whenever the selection changes.

  ::

    proc SelectNotify { widget } {
       set ids [\&$widget curselection]
    }
    .tv configure -selectcommand "SelectNotify .tv"

The widget supports the standard Tk scrolling and scanning operations.  The
**treeview** can be both horizontally and vertically. You can attach
scrollbars to the **treeview** the same way as the listbox or canvas
widgets.

  ::

    scrollbar .xbar -orient horizontal -command ".tv xview"
    scrollbar .ybar -orient vertical -command ".tv yview"
    .tv configure -xscrollcommand ".xbar set" \\
        -yscrollcommand ".ybar set"

There are three different modes of scrolling: "listbox", "canvas", and
"hierbox".  In "listbox" mode, the last entry can always be scrolled to the
top of the widget.  In "hierbox" mode, the last entry is always drawn at
the bottom of the widget.  The scroll mode is set by the widget's
**-selectmode** option.

  ::

    .tv configure -scrollmode "listbox"

Entries can be programmatically opened or closed using the **open**
and **close** operations respectively.  

  ::

    .tv open $id
    .tv close $id

When an entry is opened, a TCL procedure can be automatically invoked.
The **-opencommand** option specifies this procedure.  This
procedure can lazily insert entries as needed.

  ::

    proc AddEntries { dir } {
       eval .tv insert end [glob -nocomplain $dir/*] 
    }
    .tv configure -opencommand "AddEntries %P"

Now when an entry is opened, the procedure "AddEntries" is called and adds
children to the entry.  Before the command is invoked, special "%"
substitutions (like **bind**) are performed. Above, "%P" is translated to
the pathname of the entry.

The same feature exists when an entry is closed.  The **-closecommand**
option specifies the procedure.

  ::

    proc DeleteEntries { id } {
       .tv entry delete $id 0 end
    }
    .tv configure -closecommand "DeleteEntries %#"

When an entry is closed, the procedure "DeleteEntries" is called
and deletes the entry's children using the **entry delete** operation
("%#" is the id of entry).

KEYWORDS
--------

treeview, widget

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
