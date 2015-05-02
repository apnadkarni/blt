==========
blt::table
==========

----------------------------
Arranges widgets in a table
----------------------------

:Author: George A Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::table** *tableName* ?\ *pathName* *index* *option* *value* ... ?

**blt::table arrange** *tableName*

**blt::table cget** *tableName* ?\ *item*\ ? *option*

**blt::table column** *tableName* *args* ...

**blt::table configure** *tableName* ?\ *item* ... ?\ *option* *value* ... ?

**blt::table containers** ?\ *switches* ... ? 

**blt::table forget**  ?\ *pathName* ... ?

**blt::table info** *tableName* *pathName*

**blt::table locate** *tableName* *x* *y*

**blt::table names** ?\ *switches* ... ? 

**blt::table row** *tableName* *args* ...

**blt::table save** *tableName* 

**blt::table search** *tableName* ?\ *switches* ... ?

DESCRIPTION
-----------

The **blt::table** command arranges widgets in a table.  The alignment of
widgets is determined by their row and column positions and the number of
rows or columns that they span.

INTRODUCTION
------------

Probably the most painstaking aspect of building a graphical application is
getting the placement and size of the widgets just right.  It usually takes
many iterations to align widgets and adjust their spacing.  That's because
managing the geometry of widgets is simply not a packing problem, but also
graphical design problem.  Attributes such as alignment, symmetry, and
balance are more important than minimizing the amount of space used for
packing.

The *table* geometry manager arranges widgets in a table.  It's easy to
align widgets (horizontally and vertically) or to create empty space to
balance the arrangement of the widgets.  Widgets (called *slaves* in the Tk
parlance) are arranged inside a containing widget (called the *master*).
Widgets are positioned at row,column locations and may span any number of
rows or columns.  More than one widget can occupy a single location.

The placement of widget windows determines both the size and arrangement of
the table.  The table queries the requested size of each widget.  The
*requested size* of a widget is the natural size of the widget (before
the widget is shrunk or expanded).  The height of each row and the width of
each column is the largest widget spanning that row or column.  The size of
the table is in turn the sum of the row and column sizes.  This is the
table's *normal size*.

The total number of rows and columns in a table is determined from the
indices specified.  The table grows dynamically as windows are added
at larger indices.

SPECIFYING SIZES
----------------

Sometimes it's more useful to limit resizes to an acceptable range, than to
fix the size to a particular value or disallow resizing altogether.
Similar to the way the **wm** command lets you specify a **minsize** and
**maxsize** for a toplevel window, you can bound the sizes the container, a
widget, row, or column may take.  The **-width**, **-height**,
**-reqwidth**, and **-reqheight** options, take a list of one, two, or
three values.  We can take a previous example and instead preventing
resizing, bound the size of the scrollbars between two values.

 ::

    blt::table . \
	.title   0,0 -cspan 3 \
	.canvas  1,1 -fill both \
	.vscroll 1,2 -fill y \
	.hscroll 2,1 -fill x

    # Bound the scrollbars between 1/8 and 1/2 inch
    blt::table configure . c2 -width { 0.125 0.5 }
    blt::table configure . r2 -height { 0.125 0.5 }
    blt::table configure . vscroll .hscroll -fill both

The scrollbars will get no smaller than 1/8 of an inch, or bigger than 1/2
inch.  The initial size will be their requested size, so long as it is
within the specified bounds.

How the elements of the list are interpreted is dependent upon the number
of elements in the list.

**""**
  Empty list. No bounds are set. The default sizing is performed.
 
*number*
  Fixes the size to *number*.  *Number* may have any of the forms accept
  able to **Tk_GetPixels**, such as "1.2i".  The window or partition does
  not grow or shrink.
 
**"**\ *min* *max*\ **"**
  Sets up minimum and maximum limits for the size of the window or
  partition.  *Min* and *max* may have any of the forms accept able to
  **Tk_GetPixels**, such as "1.2i".  The window or partition can be reduced
  less than *min*, nor can it be stretched beyond *max*.
 
**"**\ *min* *max* *nom*\ **"**
  Specifies minimum and maximum size limits, but also specifies a nominal
  size *nom*.  *Min*, *max*, and *nom* may have any of the forms accept
  able to **Tk_GetPixels**, such as "1.2i".  This overrides the calculated
  size of the window or partition.

OPERATIONS
----------

The following operations are available for the *table* geometry manager.
*TableName* is the name of the container window.

**blt::table**  *tableName* ?\ *row*\ **,**\ *column* *pathName*  *option* *value* ... ?  
  Adds the widget *pathName* to the table at *row* and *column*.  *Row* and
  *column* are the respective row and column indices, starting from zero
  (0,0 is the upper leftmost position).  *Row* and *column* may also be
  numeric expressions that are recursively evaluated.  If a table geometry
  manager doesn't exist for *tableName*, one is created.  *PathName* is the
  path name of the window, that must already exist, to be arranged inside
  of *tableName*. *Option* and *value* are described below.

  **-anchor** *anchor* 
    Anchors *pathName* to a particular edge of the cell(s) it resides.
    This option has effect only if the space of the spans surrounding
    *pathName* is larger than *pathName*. *Anchor* specifies
    how *pathName* will be positioned in the space.  For example, if
    *anchor* is "center" then the window is centered in the rows
    and columns it spans; if *anchor* is "w" then the window will
    be aligned with the leftmost edge of the span. The default is
    "center".

  **-columnspan** *numColumns*
    Specifies the number of columns *pathName* will span.  The default is
    "1".

  **-columncontrol** *control*
    Specifies how the width of *pathName* should control the width of the
    columns it spans. *Control* must be one of the following.

    **none**
      The width of *pathName* is not considered.   

    **full**
      Only the width of *pathName* will be considered when computing the
      widths of the columns. 

    **normal**
      Indicates that the widest widget spanning the column will determine 
      the width of the span.

    The default is "normal".

  **-fill** *fillName*
    Specifies if *pathName* should be stretched to fill any free space
    in the span surrounding *pathName*. *FillName* is one of the following.
  
    **x**
      The widget can grow horizontally.  

    **y**
      The widget can grow vertically.  

    **both**
      The widget can grow both vertically and horizontally.  

    **none**
      The widget does not grow along with the span.  

    The default is "none".

  **-ipadx** *numPixels* 
    Sets how much horizontal padding to add internally on the left and
    right sides of *pathName*.  *NumPixels* must be a valid screen distance
    like "2" or "0.3i".  The default is "0".

  **-ipady** *numPixels*
    Sets how much vertical padding to add internally on the top and bottom
    of *pathName*.  *NumPixels* must be a valid screen distance
    like "2" or "0.3i".  The default is "0".

  **-padx** *pad*
    Sets how much padding to add to the left and right exteriors of
    *pathName*.  *Pad* can be a list of one or two numbers.  If *pad* has
    two elements, the left side of *pathName* is padded by the first value
    and the right side by the second value.  If *pad* has just one value,
    both the left and right sides are padded evenly by the value.  The
    default is "0".

  **-pady** *numPixels*
    Sets how much padding to add to the top and bottom exteriors of
    *pathName*.  *NumPixels* can be a list of one or two elements where
    each element is a valid screen distance like "2" or "0.3i".  If
    *numPixels* is two elements, the area above *pathName* is padded by the
    first distance and the area below by the second.  If *numPixels* is
    just one element, both the top and bottom areas are padded by the
    same distance.  The default is "0".

  **-reqheight** *height*
    Specifies the limits of the requested height for *pathName*.  *Height* is
    a list of bounding values.  See the `SPECIFYING SIZES`_ section for a
    description of this list.  By default, the height of *pathName* is its
    requested height with its internal padding (see the **-ipady** option).
    The bounds specified by *height* either override the height completely,
    or bound the height between two sizes.  The default is """".

  **-reqwidth** *width*
    Specifies the limits of the requested width for *pathName*.  *Width* is
    a list of bounding values.  See the `SPECIFYING SIZES`_ section for a
    description of this list.  By default, the width of *pathName* is its
    requested width with its internal padding (set the **-ipadx** option).
    The bounds specified by *width* either override the width completely,
    or bound the height between two sizes.  The default is "".

  **-rowspan** *numRows*
    Sets the number of rows *pathName* will span. The default is "1".

  **-rowcontrol** *control*
    Specifies how the height of *pathName* should control the height of the
    rows it spans. *Control* is one of the following.

    **none**
      The height of *pathName* is not considered.   

    **full**
      Only the height of *pathName* will be considered when computing the
      heights of the rows. 

    **normal**
      Indicates that the tallest widget spanning the row will determine 
      the height of the span.

    The default is "normal".

**blt::table arrange** *tableName*
  Computes the layout of the table.  Normally, the *table*
  geometry manager will wait until the next idle point, before calculating
  the size of its rows and columns.  This is useful for collecting the
  *normal* sizes of rows and columns, that are based upon the requested
  widget sizes.

**blt::table cget** *tableName* ?\ *item*\ ? *option*
  Returns the current value of the configuration option specific to
  *item* given by *option*.  *Item* can be in one of the following forms.

  *columnIndex*
    Specifies a column by its index.  The index is prefixed with the letter
    "c", such as "c0".  Column indices start from 0. *Option* is one of
    the column configuration options described in the **column configure**
    operation below.

  *rowIndex*
    Specifies a row by its index.  The index is prefixed with the letter
    "r", such as "r0".  Row indices start from 0. *Option* is one of
    the row configuration options described below in the **row configure**
    operation below.

  *pathName*
    Specifies a widget by its path name. *Option* is one of the widget
    options described above.

  No argument.
    Specifies the table itself. *Option* is one of the table configuration
    options described below.

**blt::table column cget** *tableName* *columnIndex* *option*
  Returns the current value of the column configuration option.
  *ColumnIndex* is the index of a column in the table. The column must
  already exist. Column indices start from 0. *Option* is one of the column
  configuration options described the **column configure** operation below.

**blt::table column configure** *tableName* *columnIndex* ?\ *option* *value* ... ?
  Queries or modifies the column configuration options for *tableName*.
  *ColumnIndex* in the index of the column to be queried or modified.
  Column indices start from 0. If *option* is specified with no *value*,
  then the command returns a list describing the one named option (this
  list will be identical to the corresponding sublist of the value returned
  if no *option* is specified).

  If one or more *option*-*value* pairs are specified, then the command
  modifies the given option(s) to have the given value(s); in this case the
  command returns the empty string.  If *columnIndex* is equal to or
  greater than the number of columns in the table, new columns are
  automatically created and configured. *Option* and *value* are described
  below.
  
  **-padx** *pad*
    Specifies the padding to the left and right of the column.  *Pad* can
    be a list of one or two numbers.  If *pad* has two elements, the left
    side of the column is padded by the first value and the right side by
    the second value.  If *pad* has just one value, both the left and right
    sides are padded evenly by the value.  The default is "0".

  **-resize** *resizeMode*
    Indicates that the column can expand or shrink from its requested width
    when *tableName* is resized.  *ResizeMode* must be one of the
    following.

    **none**
      The width of the column does not change as the container window
      is resized.
    **expand**
      The width of the column is expanded if there is extra space in
      the container window. 
    **shrink**
      The width of the column is reduced beyond its requested width if
      there is not enough space in the container.
    **both**
      The width of the column may grow or shrink depending on the size of
      the container.

    The default is "none".

  **-width** *width*
    Specifies the limits within that the width of the column may expand or
    shrink.  *Width* is a list of bounding values.  See the section
    `SPECIFYING SIZES`_ for a description of this list.  By default there are
    no constraints.
    
**blt::table column delete** *tableName* ? *firstIndex* ?\ *lastIndex*\ ?
  Deletes one or more columns from the table. *FirstIndex* and *lastIndex*
  are column indices. If no *lastIndex* argument is given, the this command
  deletes the column designated by *firstIndex*.  If *lastIndex* is
  present, then both indices designate a range of columns to be deleted.

**blt::table column extents** *tableName* *columnIndex*
  Returns the extents (x, y, width, and height) of the column.
  *ColumnIndex* specifies a column by its index.  The returned list will
  contain the x and y coordinates of the upper-left corner of the column
  and its width and height.

**blt::table column find** *tableName* *x*
  Finds the column containing the given x-coordinate.  *X* is screen
  coordinate relative to the container.  The index of the column is
  returned.  If no column is under *x* then "-1" is returned.

**blt::table column info** *tableName* *columnIndex* 
  Returns the current column configuration options for *columnIndex*.  The
  format of list is so that is may be directly used with the **blt::table**
  command.  It can be used to save and restore table column configurations.
  *ColumnIndex* specifies a column in the table.
  
**blt::table column insert** *tableName* ?\ *switches* ... ? 
  Inserts one or more columns into the table managing *tableName*.  By
  default the column is added to the end of the table.  *Switches* can be
  any of the following.
  
  **-after** *columnIndex*
    Specifies to add the new column(s) after *columnIndex*.  By default
    columns are appended. *ColumnIndex* is the index of the column in the
    table.

  **-before** *columnIndex*
    Specifies to add the new column(s) before *columnIndex*.  By default
    columns are appended. *ColumnIndex* is the index of the column in the
    table.

  **-numcolumns** *numColumns**
    Specifies the number of new columns to add.  The default is 1.

**blt::table column join** *tableName* *firstIndex* *lastIndex*
  Joins the specified range of columns into one column.  The widgets
  contained in the span range moved to *firstIndex* and the extra columns
  deleted.  *FirstIndex* and *lastIndex* are column indices.  *FirstIndex*
  must be less than *lastIndex*.  The column indices will change.
  
**blt::table column split** *tableName* *columnIndex* *numDivisions*
  Splits the column in the designated number of columns.  *ColumnIndex* is
  the index of column.  Column indices start from 0.  *NumDivisions* must
  be greater than 1 and is the number of new columns minus one.  New
  columns starting from *columnIndex* are created and the spans of widgets
  crossing *columnIndex* are adjusted.  The column indices will change.

**blt::table configure** *tableName* ?\ *option* *value* ... ?
  Queries or modifies table configuration options.  If no *option* is
  specified, this command returns a list describing all of the available
  table configuration options for *tableName*.  *Option* and *value* are
  described below.
 
  **-padx** *pad*
    Sets how much padding to add to the left and right exteriors of the
    table.  *Pad* can be a list of one or two numbers.  If *pad* has two
    elements, the left side of the table is padded by the first value and
    the right side by the second value.  If *pad* has just one value, both
    the left and right sides are padded evenly by the value.  The default
    is "0".

  **-pady** *pad*
    Sets how much padding to add to the top and bottom exteriors of the
    table.  *Pad* can be a list of one or two numbers.  If *pad* has two
    elements, the area above the table is padded by the first value and the
    area below by the second value.  If *pad* is just one number, both the
    top and bottom areas are padded by the value.  The default is "0".

  **-propagate** *boolean* 
    Indicates if the table should override the requested width and height
    of the *tableName* window.  If *boolean* is false, *tableName* will not
    be resized.  *TableName* will be its requested size.  The default is
    "1".

**blt::table configure** *tableName* *item* ?\ *option* *value* ... ?
  Queries or modifies the configuration options for *tableName*.
  *Item* is one of the following.

  *columnIndex*
    Specifies a column by its index.  The index is prefixed with the letter
    "c", such as "c0".  Column indices start from 0. *Option* is one of
    the column configuration options described in the **column configure**
    operation above.

  *rowIndex*
    Specifies a row by its index.  The index is prefixed with the letter
    "r", such as "r0".  Row indices start from 0. *Option* is one of
    the row configuration options described below in the **row configure**
    operation below.

  *pathName*
    Specifies a widget by its path name. *Option* is one of the widget
    options described above.

  If *option* is specified with no *value*, then the command returns a list
  describing the one named option (this list will be identical to the
  corresponding sublist of the value returned if no *option* is specified).
  If one or more *option*-*value* pairs are specified, then the command
  modifies the given option(s) to have the given value(s); in this case the
  command returns the empty string. *Option* and *value* are specific
  to *item*.

**blt::table containers** ?\ *switches* ... ?
  Returns the names of the container windows matching a given criteria.  If
  no *switches* arguments are given, the names of all container windows are
  returned.  The following are valid switches:

  **-pattern** *pattern*
    Returns a list of path names for all container windows matching
    *pattern*.  *Pattern* is glob-style pattern.  Matching is done in a
    fashion similar to that used by the TCL **glob** command.

  **-slave** *pathName*
    Returns the name of the container window of table managing *pathName*.
    *PathName* must be the path name of widget.  If *pathName* is not
    managed by any table, the empty string is returned.

**blt::table find** *tableName* *x* *y*
  Returns the row and column index of the cell containing the given screen
  coordinates.  The *x* and *y* are screen coordinates relative to to
  *tableName*.  If no cell is at the given point, then "" is returned.

**blt::table forget** ?\ *pathName* ... ?
  Requests that one or more widgets longer have their geometry managed by
  the table.  *PathName* is the pathname of the window currently managed by
  a **blt::table** geometry manager. The window will be unmapped so that it
  no longer appears on the screen.  If *pathName* is not currently managed
  by any table, an error message is returned, otherwise the empty string.

**blt::table info** *tableName* *pathName* 
  Returns a list of the current configuration options for *pathName*.  
  The list returned is exactly in the form that might be specified to the
  **blt::table** command.  It can be used to save and restore table 
  *PathName* is the name of the widget contained in the table.
  The list of widget configuration options and their respective values
  for *pathName* is returned.

**blt::table names** ?\ *switches* ... ?
  Returns the names of the container windows matching a given criteria.  If
  no *switches* arguments are given, the names of all container windows are
  returned.  The following are valid switches:

  **-pattern** *pattern*
    Returns a list of path names for all container windows matching
    *pattern*.  *Pattern* is glob-style pattern.  Matching is done in a
    fashion similar to that used by the TCL **glob** command.

  **-slave** *pathName*
    Returns the name of the container window of table managing *pathName*.
    *PathName* must be the path name of widget.  If *pathName* is not
    managed by any table, the empty string is returned.

**blt::table row cget** *tableName* *rowIndex* *option*
  Returns the current value of the row configuration option.  *RowIndex* is
  the index of a row in the table. The row must already exist. Row indices
  start from 0. *Option* is one of the row configuration options described
  the **row configure** operation below.

**blt::table row configure** *tableName* *rowIndex* ?\ *option* *value* ... ?
  Queries or modifies the row configuration options for *tableName*.
  *RowIndex* in the index of the row to be queried or modified.  Row
  indices start from 0. If *option* is specified with no *value*,
  then the command returns a list describing the one named
  option (this list will be identical to the corresponding sublist of the
  value returned if no *option* is specified).

  If one or more *option*-*value* pairs are specified, then the command
  modifies the given option(s) to have the given value(s); in this case the
  command returns the empty string.  If *rowIndex* is equal to or greater
  than the number of rows in the table, new rows are automatically created
  and configured. *Option* and *value* are described below.
  
  **-height** *height*
    Specifies the limits of the height that the row may expand or shrink
    to.  *Height* is a list of bounding values.  See the section `SPECIFYING
    SIZES`_ for a description of this list.  By default there are no
    constraints.

  **-pady** *pad*
    Sets the padding above and below the row.  *Pad* can be a list of one
    or two numbers.  If *pad* has two elements, the area above the row is
    padded by the first value and the area below by the second value.  If
    *pad* is just one number, both the top and bottom areas are padded by
    the value.  The default is "0".

  **-resize** *resizeMode*
    Specifies that the row can expand or shrink from its requested height
    when *tableName* is resized.  *ResizeMode* must be one of the following.

    **none**
      The height of the row does not change as the container window
      is resized.
    **expand**
      The height of the row is expanded if there is extra space in
      the container window. 
    **shrink**
      The height of the row is reduced beyond its requested width if
      there is not enough space in the container.
    **both**
      The height of the row may grow or shrink depending on the size of
      the container.

    The default is "none".
    
**blt::table row delete** *tableName* ? *firstIndex* ?\ *lastIndex*\ ?
  Deletes one or more rows from the table. *FirstIndex* and *lastIndex* are
  row indices. If no *lastIndex* argument is given,
  the this command deletes the row designated by *firstIndex*.  If *lastIndex*
  is present, then both indices designate a range of rows to be deleted. 

**blt::table row extents** *tableName* *rowIndex*
  Returns the extents (the location and dimensions) of the row.  *RowIndex*
  specifies a row by its index.  The returned list
  will contain the x and y coordinates of the upper-left corner of the row and
  its width and height.

**blt::table row find** *tableName* *y*
  Finds the row containing the given y-coordinate.  *Y* is screen coordinate
  relative to the container.  The index of the row is returned.  If no row
  is under *y* then "-1" is returned.

**blt::table row info** *tableName* *rowIndex* 
  Returns the current row configuration options for *rowIndex*.  
  The format of list is so that is may be directly used with the 
  **blt::table** command.  It can be used to save and restore table 
  row configurations.  *RowIndex* specifies a row in the table.
  
**blt::table row insert** *tableName* ?\ *switches* ... ? 
  Inserts one or more rows into the table managing *tableName*.
  By default the row is added to the end of the table.
  *Switches* can be any of the following.
  
  **-after** *rowIndex*
    Specifies to add the new row(s) after *rowIndex*.  By default rows
    are appended. *RowIndex* is the index of the row in the table.

  **-before** *rowIndex*
    Specifies to add the new row(s) before *rowIndex*.  By default rows
    are appended. *RowIndex* is the index of the row in the table.

  **-numrows** *numRows*
    Specifies the number of new rows to add.  The default is 1.

**blt::table row join** *tableName* *firstIndex* *lastIndex*
  Joins the specified range of rows into one row.  The widgets 
  contained in the span range moved to *firstIndex* and the extra rows
  deleted.  *FirstIndex* and *lastIndex* are row indices.  *FirstIndex*
  must be less than *lastIndex*.  The row indices will change.
  
**blt::table row save** *tableName*
  Returns a TCL script containing the commands to rebuild the table.
  This is useful is you are trying to save the state of a particular layout.

**blt::table row split** *tableName* *rowIndex* *numDivisions* Splits the
  row in the designated number of rows.  *RowIndex* is the index of row.
  Row indices start from 0.  *NumDivisions* must be greater than 1 and is
  the number of new rows minus one.  New rows starting from *rowIndex* are
  created and the spans of widgets crossing *rowIndex* are adjusted.  The
  row indices will change.

**blt::table search** *tableName* ?\ *switches* ... ?
  Returns the names of all the widgets in *tableName* matching the criteria
  given by *switches*.  *TableName* is name of the container window
  associated with the table to be searched.  The name of the widget is
  returned if any one criteria matches. If no *switches* arguments are
  given, the names of all widgets managed by *tableName* are returned.  The
  following are switches are available:

  **-pattern** *pattern*
    Returns the names of any names of the widgets matching *pattern*.

  **-span** *row*\ **,**\ *column*
    Returns the names of widgets that span *index*. A widget does not need
    to start at *index* to be included.  *Index* must be in the form
    *row*,*column*, where *row* and *column* are valid row and column
    numbers.

  **-start** *row*\ **,**\ *column*
    Returns the names of widgets that start at *index*.  *Index* must be in
    the form *row*,*column*, where *row* and *column* are valid row and
    column numbers.

EXAMPLE
-------

The table geometry manager is created by invoking the **blt::table** command.

 ::

    # Manage the toplevel "." with table geometry manager.
    blt::table .

The window "." is now the *container* of the table.  Widgets are packed
into the table and displayed within the confines of the container.

You add widgets to the table at row and column indices.  Row and column
indices start from zero.

 ::

    label .title -text "This is a title"

    # Add a label to the table
    blt::table . .title 0,0 

The label widget ".title" is added to the table.  We can add more widgets
in the same way.

 ::
    
    button .ok -text "Ok"
    button .cancel -text "Cancel"

    # Add two buttons
    blt::table . .ok 1,0
    blt::table . .cancel 1,1

Two buttons ".ok" and ".cancel" are now packed into the second row of the
table.  They each occupy one cell of the table.  By default, widgets span
only a single row and column.

The first column contains two widgets, ".title" and ".ok".  By default, the
widest of the two widgets will define the width of the column.  However, we
want ".title" to be centered horizontally along the top of the table.  We
can make ".title" span two columns using the **configure** operation.

  ::

     # Make the label span both columns
     blt::table configure . .title -cspan 2

The label ".title" will now be centered along the top row of the table.

In the above example, we've create and arranged the layout for the table
invoking the **blt::table** command several times.  Alternately, we could
have used a single **blt::table** command.

 ::

    label .title -text "This is a title"
    button .ok -text "Ok"
    button .cancel -text "Cancel"

    # Create and pack the table
    blt::table . \
	0,0  .title -cspan 2 \
	1,0  .ok \
	1,1 .cancel 

The table will override the requested width and height of the container
so that the window fits the table exactly.  This also means
that any change to the size of table will be propagated up through the
Tk window hierarchy.  This feature can be turned off using the
**configure** operation again.

 ::

    blt::table configure . -propagate no

You can also set the width of height of the table to a specific
value. This supersedes the calculated table size.

 ::

    # Make the container 4 inches wide, 3 inches high
    blt::table configure . -reqwidth 4i -reqheight 3i

If a widget is smaller than the cell(s) it occupies, the widget will
float within the extra space.  By default, the widget will be centered
within the space, but you can anchor the widget to any side of cell
using the **-anchor** configuration option.

 ::

    blt::table configure . .ok -anchor w

The **-fill** option expands the widget to fill the 
extra space either vertically or horizontally (or both).

  ::

    # Make the title label fill the entire top row
    blt::table configure . .title -cspan 2 -fill x 

    # Each button will be as height of the 2nd row.
    blt::table configure . .ok .cancel -fill y

The width of ".title" will be the combined widths of both columns.  Both
".ok" and ".cancel" will become as tall as the second row.

The **-padx** and **-pady** options control the amount of padding 
around the widget.  Both options take a list of one or two values.

  ::

    # Pad the title by two pixels above and below.
    blt::table configure . .title -pady 2

    # Pad each button 2 pixels on the left, and 4 on the right.
    blt::table configure . .ok .cancel -padx { 2 4 }

If the list has only one value, then both exterior sides (top and bottom
or left and right) of the widget are padded by that amount.  If the
list has two elements, the first specifies padding for the top or left
side and the second for the bottom or right side.

Like the container, you can also override the requested widths and
heights of widgets using the **-reqwidth** and
**-reqheight** options.  This is especially useful with
character-based widgets (such as buttons, labels, text, listbox, etc)
that let you specify their size only in units of characters and lines,
instead of pixels.

 ::
    
    # Make all buttons one inch wide
    blt::table configure . .ok .cancel -reqwidth 1i

Each row and column of the table can be configured, again using the
**configure** operation.  Rows are and columns are designated by **R**\ *i*
and **C**\ *i* respectively, where *i* is the index of the row or column.

For example, you can set the size of a row or column.

 ::

    # Make the 1st column 2 inches wide
    blt::table configure . c0 -width 2.0i

    # Make the 2nd row 1/2 inch high.
    blt::table configure . r1 -height 0.5i

The new size for the row or column overrides its calculated size.  If
no widgets span the row or column, its height or width is zero.
So you can use the **-width** and **-height** options to create
empty spaces in the table.

 ::

    # Create an empty row and column
    blt::table configure . r2 c2 -width 1i

The **-pady** option lets you add padding to the top and bottom
sides of rows.  The **-padx** option adds padding to the left and
right sides of columns.  Both options take a list of one or two
values.

 ::

    # Pad above the title by two pixels 
    blt::table configure . r0 -pady { 2 0 }

    # Pad each column 4 pixels on the left, and 2 on the right.
    blt::table configure . c* -padx { 2 4 }

Notice that you can configure all the rows and columns using either
"R*" or "C*".

When the container is resized, the rows and columns of the table are also
resized.  Only the rows or columns that contain widgets (a widget spans the
row or column) grow or shrink.  The **-resize** option indicates whether
the row or column can be shrunk or stretched.  If the value is "shrink",
the row or column can only be resized smaller.  If "expand", it can only be
resized larger.  If "none", the row or column is frozen at its requested
size.

 ::

    # Let the 1st column get smaller, but not bigger
    blt::table configure . c0 -resize shrink

    # Let the 2nd column get bigger, not smaller
    blt::table configure . c1 -resize expand

    # Don't resize the first row 
    blt::table configure . r0 -resize none

The following example packs a canvas, two scrollbars, and a title.
The rows and columns containing the scrollbars are frozen at their
requested size, so that even if the frame is resized, the scrollbars will
remain the same width.

 ::

    blt::table . \
	.title   0,0 -cspan 3 \
	.canvas  1,1 -fill both \
	.vscroll 1,2 -fill y \
	.hscroll 2,1 -fill x

    # Don't let the scrollbars resize
    blt::table configure . c2 r2 -resize none

    # Create an empty space to balance the scrollbar
    blt::table configure . c0 -width .vscroll

Note that the value of the **-width** option is the name of a widget
window.  This indicates that the width of the column should be the
same as the requested width of ".vscroll".

Finally, the **forget** operation removes widgets from the table.

 ::

    # Remove the windows from the table
    blt::table forget .quit .frame

It's not necessary to specify the container.  The **blt::table**
command determines the container from the widget name.

BUGS
----

A long standing bug in Tk (circa 1993), there is no way to detect if a
window is already a container of a different geometry manager. This is
usually done by accident, such as the following where all three widgets are
arranged in the same container ".", but using different geometry managers.

 ::

    blt::table .f1
	...
    pack .f2
	...
    grid .f3

This leads to bizarre window resizing, as each geometry manager applies its
own brand of layout policies.  When the container is a top level window
(such as "."), your window manager may become locked up as it responds to
the never-ending stream of resize requests.

KEYWORDS
--------

frame, geometry manager, location, table, size

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
