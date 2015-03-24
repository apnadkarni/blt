===============
blt::datatable
===============

-------------------------------------------------
Create and manage datatable objects.
-------------------------------------------------

:Author: gahowlett@gmail.com
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
        Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

.. TODO: authors and author with name <email>

SYNOPSIS
--------

**blt::datatable create** ?\ *tableName*\ ?

**blt::datatable destroy** *tableName*...

**blt::datatable exists** *tableName*

**blt::datatable load** *format* *libPath*

**blt::datatable names** ?\ *pattern* ...\ ?

DESCRIPTION
===========

The **datatable** command creates datatable data objects.  A *datatable*
object is table of cells.  Row and columns both have indices and labels
that address the row or column.  Cells may contain data or may be empty.

SYNTAX
======

**blt::datatable create** ?\ *tableName*\ ?  

  Creates a new *datatable* object. The name of the new datatable is
  returned.  If no *tableName* argument is present, then the name of the
  datatable is automatically generated in the form "datatable0",
  "datatable1", etc.  If the substring "#auto" is found in *tableName*,
  it is automatically substituted by a generated name.  For example, the
  name ".foo.#auto.bar" will be translated to ".foo.datatable0.bar".
  Datatables are by default created in the current namespace, not the
  global namespace, unless *tableName* contains a namespace qualifier, such
  as "fred::myDataTable".
  
  A new TCL command (by the same name as the datatable) is created that you
  can use to access and manage the datatable object.  Another TCL command
  or datatable object can not already exist as *tableName*.  The new
  datatable will be empty with no rows or columns.  If the TCL command
  *tableName* is deleted, the datatable will also be released.

**blt::datatable destroy** *tableName*...

  Releases one of more datatables.  The TCL command associated with
  *tableName* is also removed.  Datatables are reference counted.  The
  internal datatable data object isn't destroyed until no one else is using
  it.

**blt::datatable exists** *tableName*

  Indicates if a datatable named *tableName* already exists.  Returns "1"
  is the datatable exists, "0" otherwise.

**blt::datatable load** *format* *libPath*

  Dynamically loads the named datatable module.  This is used internally
  to load datatable modules for importing and exporting data.

**blt::datatable names** ?\ *pattern* ...\ ?

  Returns the names of all datatable objects.  If one or more *pattern*
  arguments is provided, then the any datatables whose name matches *pattern*
  will be listed.

REFERENCING ROWS AND COLUMNS
============================

Row and columns in a *datatable* object may be referred by either their
index, label, or tag.

 *index*
   The number of the row or column.  Row and column indices start from 0.
   The index of a row or column may change as rows or columns are added,
   deleted, moved, or sorted.

 *label*
   The label of the row or column.  Each row and column has a label.
   Labels should not be numbers (to distinguish them from indices). Row and
   column labels are distinct.  There can be duplicate row or column
   labels.

 @*tag*
   A tag is a string associated with a row or column.  They are a useful of
   grouping rows and columns. Columns and rows can have any number of tags
   associated with them.  To use a tag you prefix it with a '@'
   character. This distinguishes tags from labels.  There are two built-in
   tags: "all" and "end".  Every row and column have the tag "all".  The
   last row and column in the table have the tag "end".  Row and column
   tags are distinct. Tags may be empty (associated with no rows or
   columns).
     
If the row or column specifier is an integer then it is assumed to refer to
a row or column index.  But you can also distinguish between indices,
labels and tables by prefixing them with "index:", "label:", or "tag:".

You can refer to multiple rows or columns in a tag (such as 'all') or
label.  Some datatable commands only operate on a single row or column at a
time; if *tag* or *label* refer to multiple rows or columns, then an error
is generated.

.. _datatable_operations:

DATATABLE OPERATIONS
====================

Once you create a datatable object, you can use its TCL command 
to query or modify it.  The general form of the command is

  *tableName* *operation* ?\ *arg* *arg* ...\ ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for datatables are listed below.

*tableName* **add** *srcTable* ?\ *switches* ...\ ?

  Adds the rows from *srcTable* to the bottom of *tableName*. Columns are
  matched by their labels. New columns are automatically created. For
  example if *tableName* doesn't have a column labeled "foo", one will
  be created.  The column tags are also copied. *Switches* can be any of
  the following:

  **-columns** *columnList*
    Specifies the subset of columns from *srcTable* to add.  By default
    all columns are added.

  **-notags** 
    Don't copy column tags. 

  **-row** *rowList*
    Specifies the subset of rows from *srcTable* to add.  By default
    all rows are added.
    
*tableName* **append** *row* *column* *value* ?\ *value* ...\ ?

  Appends one or more values to the current value at *row*, *column* in
  *tableName*.  This is normally used for "string" type cells, but can be
  used for other types as well.  Both *row* and *column* may be a label,
  index, or tag, and may represent more than one row or column.

*tableName* **attach** *newTable*

  Attaches to an existing datatable object *newTable*.  The underlying
  table (row, columns, cells) are shared with *tableName*.  Tags, traces,
  and watches are not shared. The current table associated with *tableName*
  is discarded.  It will be destroyed is no one else is using it.  The
  current set of tags, notifier events, and traces in *tableName* are
  reset.

*tableName* **column copy** *srcColumn* *destColumn* ?\ *switches* ...\ ?

  Copies the column *srcColumn* into *destColumn*.  If a column
  *destColumn* doesn't already exist in *tableName*, one is created.
  *SrcColumn* and *destColumn* may be a label, index, or tag, but may not
  represent more than one column.  *Switches* can be any of the following:

  **-append** 
    Append the values of *srcColumn* to *destColumn*.  By default the
    *destColumn* is overwritten by *srcColumn* (the values in *srcColumn*
    are first removed).

  **-new** 
    Always create a new column *destColumn* even if one already exists in
    *tableName*. The new column will have a duplicate column label.

  **-notags** 
    Don't copy column tags. 

  **-table** *srcTable*
    Copy the column *srcColumn* from the datatable *srcTable*.  By default
    to *tableName* is also the source table.

*tableName* **column create** ?\ *switches* ...\ ?

  Creates a new column in *tableName*. The cells of the new column
  is initially empty. The index of the new column is returned.
  *Switches* can be any of the following:  

  **-after** *column*
    The position of the new column will be after *column*. *Column* may
    be a label, index, or tag, but may not represent more than one
    column.

  **-before** *column*
    The position of the new column will be before *column*. *Column* may
    be a label, index, or tag, but may not represent more than one
    column.

  **-label** *label*
    Specifies the column label for the new column.

  **-tags** *tagList*
    Specifies the tags to add to the column.

  **-type** *columnType*
    Specifies the type of column. The type may be "string", "double",
    "integer", "boolean", "time", or , "blob".

*tableName* **column delete** ?\ *column* ...\?

  Deletes columns from the table. *Column* may be a column label, index,
  or tag and may refer to multiple columns (example: "all").  

*tableName* **column duplicate** *column*...

  Creates duplicate columns for each *column* given.  The column label is
  duplicated.  The column tags are copied. *Column* may be a column label,
  index, or tag and may refer to multiple columns (example: "all").
  
*tableName* **column empty** *column*

  Returns the indices of the empty rows in *column*.  *Column*
  may be a label, index, or tag, but may not represent more than one
  column.

*tableName* **column exists** *column*

  Indicates if a column labeled *column* in the table. Returns "1" if
  the column exists, "0" otherwise.

*tableName* **column extend** *numColumns* ?\ *switches* ...\ ?

  Extends the table by one of more columns.  If *numColumns* is not present
  then new 1 column is added.  *Switches* can be any of the following:

  **-labels** *list*
    Specifies the column labels for the new columns.

*tableName* **column get** ?\ *-labels*\ ? *column* ?\ *row* ...\ ?

  Retrieves the values from the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  Normally all the values of *column* are retrieved. If one or more
  *row* arguments are specified, then only the rows specified are
  retrieved.  *Row* may be a row label, index, or tag.

  Returns the pairs of values and indices of the selected rows. If the
  *-labels* flag is present, the row label is returned instead of the
  index.

*tableName* **column index** *column* 

  Returns the index of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  
*tableName* **column indices** ?\ *switches* ...\ ? ?\ *pattern* ...\ ?

  Returns the indices of the column whose labels match any *pattern*. 
  *Switches* can be any of the following:

  **-duplicates** 
    Return only the indices of the duplicate columns.

*tableName* **column join** *srcTable* ?\ *switches* ...\ ?

  FIXME:
  Joins the columns of *srcTable* with *tableName*.
  The column tags are also copied. *Switches* can be any of
  the following:

  **-columns** *columnList*
    Specifies the subset of columns from *srcTable* to add.  By default
    all columns are added.

  **-notags** 
    Don't copy column tags.
    
  **-row** *rowList*
    Specifies the subset of rows from *srcTable* to add.  By default
    all rows are added.
    
*tableName* **column label** *column* ?\ *label*?  ?\ *column* *label* ...?

  Gets or sets the labels of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  If *column* is the only argument, then the label of the column
  is returned.  If *column* and *label* pairs are specified, then
  set the labels of the specified columns are set.  
  
*tableName* **column labels** *column* ?\ *labelList*?

  Gets or sets the all labels of the specified column.  If *labelList* is
  present, then column labels are set from the list of column labels.  

*tableName* **column move** *srcColumn* *destColumn* ?\ *numColumns*\ ?

  Move one or move columns in the table.  *SrcColumn* and *destColumn* may
  be a label, index, or tag, but may not represent more than one column.
  By default only 1 column is moved, but if *numColumns* is present then
  the more columns may be specified.  Moves cannot overlap.
  
*tableName* **column names**  ?\ *pattern* ...\ ?

  Returns the labels of the columns in the table.  If one of *pattern*
  arguments are present, then any of the column labels matching one
  of the patterns is returned.

*tableName* **column nonempty**  *column*

  Returns the indices of the non-empty rows in the column.  *Column* may be
  a label, index, or tag, but may not represent more than one column.

*tableName* **column set**  *column* ?\ *row* *value*\...? 

  Sets values for rows in the specified column. *Column* may be a label,
  index, or tag and may refer to multiple columns (example: "all").  If one
  or more *row* *value* pairs are found *value* is set at *row*, *column*
  in the table.  If either *row* or *column* does not exist, the row or
  column is automatically created. If the row or column is an index, the
  table may be grown. *Value* is the value to be set.  If the type of
  *column* is not *string*, *value* is converted into the correct type.  If
  the conversion fails, an error will be returned.

*tableName* **column tag add**  *tag* ?\ *column* *column* ...\ ? 

  Adds the *tag* to *column*.  If no *column* arguments are present, *tag*
  is added to the column tags managed by *tableName*.  This is use for
  creating empty column tags (tags that refer to no columns).

*tableName* **column tag delete**  *tag* ?\ *column* *column* ...\ ? 
  
  Removes the *tag* from *column*.  The built-in tags "all" and "end" can't
  be deleted and are ignored.

*tableName* **column tag exists**  *tag* ?\ *column* ...\ ? 

  Indicates if any column in *tableName* has *tag*.  Returns "1" if the tag
  exists, "0" otherwise.  By default all columns are searched. But if one
  or more *column* arguments are present, then if the tag is found in any
  *column*, "1" is returned. *Column* may be a label, index, or tag and may
  refer to multiple columns (example: "all").

*tableName* **column tag forget**  ?\ *tag* ...\ ? 

  Remove one or more tags from all the columns in *tableName*.

*tableName* **column tag get** *column* ?\ *pattern* ...\ ? 

  Returns the tags for *column*. *Column* may be a label, index, or tag,
  but may not represent more than one column. By default all tags for
  *column* are returned.  But if one or more *pattern* arguments are
  present, then any tag that matching one of the patterns will be returned.

*tableName* **column tag indices** ?\ *tag* ...\ ? 

  Returns the column indices that have one or more *tag*.

*tableName* **column tag labels** ?\ *tag* ...\ ? 

  Returns the column labels that have one or more *tag*.

*tableName* **column tag names** ?\ *pattern* ...\ ? 

  Returns the column tags of the table. By default all column tags are
  returned. But if one or more *pattern* arguments are present, then any
  tag that matching one of the patterns will be returned.

*tableName* **column tag range** *first* *last* ?\ *tag* ...\ ? 

  Adds one or more tags the columns in the range given.  *First* and *last*
  may be a label, index, or tag, but may not represent more than one
  column.

*tableName* **column tag set** *column* ?\ *tag* ...\?

  Adds one or more tags to *column*. *Column* may be a column label, index, or
  tag and may refer to multiple columns (example: "all").

*tableName* **column tag unset** *column* \ *tag*...

  Remove one or more tags from *column*. *Column* may be a column label,
  index, or tag and may refer to multiple columns (example: "all").

*tableName* **column type**  *column* ?\ *type*? ?\ *column* *type* ...\ ?

  Gets or sets the type of values for the specified column.  *Column* may
  be a label, index, or tag, but may not represent more than one column.
  If only one *column* argument is present, the current type of the
  column is returned.  If one or more *column* and *type* pairs are
  specified, then this sets the type of the column. *Type* can any of
  the following:

  *string*
    Values in the column are strings.  

  *double*
    Values in the column are double precision numbers. Each value
    in the column is converted to double precision number.  

  *integer*
    Values in the column are integers.  Each value in the column
    is converted to an integer.

  *boolean*
    Values in the column are booleans.  Each value in the column
    is converted to an boolean.

  *time*
    Values in the column are timestamps.  Each value in the column
    is converted to an double representation of the time.

  *blob*
    Values in the column are blobs. 

*tableName* **column unset**  *column* ?\ *row* ... \?

  Unsets the values of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  One or more *row* and *value* pairs may be specified.  
  *Row* may be a row label, index, or tag.  It specifies the row
  whose value is to be unset.  

*tableName* **column values**  *column* ?\ *valueList*?

  Gets or sets the values of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  If *valueList* is present, then the values of the table are
  set from the elements of the list.  If there are more values in the
  list than rows in the table, the table is extended.  If there
  are less, the remaining rows remain the same.

*tableName* **copy** *srcTable* 

  Makes a copy of *srcTable in *tableName*.  All previous rows,
  column, cells, and tags in *tableName* are first removed.

*tableName* **dir** *path* ?\ *switches* ...\ ?

  Fills the table with the directory listing specified by *path*. If
  *path* is a directory, then its entries are added to the table.
  *Switches* can be any of the following:

  **-directory** 
    Add directory entries to the table.

  **-executable** 
    Add executable file and directory entries to the table.

  **-file** 
    Add file entries to the table.

  **-hidden** 
    Add hidden file and directory entries to the table.  

  **-link** 
    Add link entries to the table.

  **-pattern** *pattern*
    Only add entries matching *pattern* to the table.

  **-readable** 
    Add readable file and directory entries to the table.

  **-readonly** 
    Add read-only (not writable) file and directory entries to the table.

  **-writable** 
    Add writable file and directory entries to the table.

  The new columns are the following:
   
  *name*
    The name of the directory entry.

  *type*
    The type of entry.  *Type* may be "file", "directory",
    "characterSpecial", "blockSpecial", "fifo", or "link".

  *size*
    The number of bytes for the entry.

  *uid*
    The number representing the user ID or the entry,

  *gid*
    The number representing the group ID of the entry,

  *atime*
    The number representing the last access time of the entry,

  *mtime*
    The number representing the last modification time of the entry,

  *ctime*
    The number representing the last change time of the entry,

  *mode*
    The number representing the mode (permissions) of the entry,

*tableName* **dump** ?\ *switches* ...\ ?

  Converts the table contents into a string representation.  This includes
  the row/column labels and tags and cell values. *Switches* can be any of
  the following:

  **-column** *columnList*
    Specifies the subset of columns from *tablename* to dump.  By default
    all columns are dumped.
    
  **-file** *fileName*
    Write the contents to the file *fileName*.

  **-rows** *rowList*
    Specifies the subset of rows from *tableName* to dump.  By default
    all rows are dumped.

*tableName* **duplicate** ?\ *table*\ ?

*tableName* **emptyvalue** ?\ *newValue*\ ?

*tableName* **exists** *row* *column*

  Indicates if a value exists at *row*, *column* in *tableName*.  
  *Row* and *column* may be a label, index, or tag, but may not represent
  more than one row or column. If the cell is empty, then "0" is returned.
  If either *row* or *column* do not exist, "0" is returned.  Otherwise,
  "1" is returned.

*tableName* **export** *format* ?\ *switches* ...\ ?

  Exports the table values into another format. *format* is one 
  of the different formats_ are described in a section below. *Switches*
  are specific to *format*.  

*tableName* **find** *expression* ?\ *switches* ...\ ?

  Finds the rows that satisfy *expression*.  *Expression* is a TCL
  expression.  The expression is evaluated for each row in the table.  The
  column values can be read via special variables. Column variable names
  are either the column index or label.  They return the values in the row
  for that column.  Note that if a cell is empty it doesn't have a variable
  associated with it.  You can use **-emptyvalue** to return a known value
  for empty cells, or you can test for empty cells by the "info exists"
  command. 

  **-addtag**  *tagName*
    Add *tagName* to each returned row.

  **-emptyvalue**  *string*
    Return *string* for empty cells when evaluating column variables.

  **-invert**  
    Returns rows that where *expression* is false.

  **-maxrows**  *numRows*
    Stop when *number* rows have been found.

  **-rows** *rowList*
    Consider only the rows in *rowList*.  *RowList* is a list of
    of row labels, indices, or tags that may refer to multiple rows.

*tableName* **get** *row* *column* ?\ *defValue*\ ?

  Returns the value at *row*, *column* in *tableName*.  
  *Row* and *column* may be a label, index, or tag, but may not represent
  more than one row or column. If the cell is empty, then the empty value
  string is returned. By default it is an error if either *row* or *column*
  do not exist.  The *defValue* argument lets you return a known value
  instead of generating an error. *DefValue* can be any string.
  
*tableName* **import** *format* ?\ *switches* ...\ ?

  Imports the table values from another format. *format* is one 
  of the different formats_ are described in a section below. *Switches*
  are specific to *format*.  
  
*tableName* **keys** *column* ?\ *column* ...\ ?

  Generates an internal lookup table from the columns given.  This is
  especially useful when a combination of column values uniquely represent
  rows of the table. *Column* may be a label, index, or tag, but may not
  represent more than one row or column.
  
*tableName* **lappend** *row* *column* ?\ *value* ...\ ?

  Appends one or more values to the current value at *row*, *column* in
  *tableName*.  Both *row* and *column* may be a label, index, or tag, and
  may represent more than one row or column. This is for "string" cells
  only.  Each new value is appended as a list element.

*tableName* **limits** ?\ *column*\ ?

  Returns the minimum and maximum values in *tableName*.  If *column* is
  present, the minimum and maximum values in *column* are returned.

*tableName* **lookup** ?\ *value* ...\ ?

  Searches for the row matching the values keys given.  *Value* is a value
  from the columns specified by the **keys** operation.  The order and number
  of the values must be the same as the columns that were specified in the
  **keys** operation.  If a matching combination is found, the index of the
  row is returned, otherwise "-1".

*tableName* **maximum** ?\ *column*\ ?

  Returns the maximum value in the table.  If *column* is present, 
  the maximum value in *column* is returned.

*tableName* **minimum** ?\ *column*\ ?

  Returns the minimum value in the table.  If *column* is present, 
  the maximum value in *column* is returned.

*tableName* **numcolumns** ?\ *numColumns*?

  Sets or gets the number of column in *tableName*.  If *numRows* is
  present, the table is resized to the specified number of columns.

*tableName* **numrows** ?\ *numRows*\ ?

  Sets or gets the number of rows in *tableName*.  If *numRows* is
  present, the table is resized to the specified number of rows.

*tableName* **restore** ?\ *switches* ...\ ?

  Restores the table from a previously dumped state (see the **dump**
  operation).  *Switches* can be any of the following:
  
  **-data**  *string*
    Reads the dump information from *string*.

  **-file**  *fileName*
    Reads the dump information from *fileName*.

  **-notags**  
    Ignore row and columns tags found in the dump information.

  **-overwrite**  
    Overwrite any rows or columns.

*tableName* **row copy** *srcRow* *destRow* ?\ *switches* ...\ ?

  Copies the row *srcRow* into *destRow*.  If a row *destRow* doesn't
  already exist in *tableName*, one is created.  *SrcRow* and *destRow* may
  be a label, index, or tag, but may not represent more than one row.
  *Switches* can be any of the following:

  **-append** 
    Append the values of *srcRow* to *destRow*.  By default the
    *destRow* is overwritten by *srcRow* (the values in *srcRow* are
    first removed).

  **-new** 
    Always create a new row *destRow* even if one already exists in
    *tableName*. The new row will have a duplicate row label.

  **-notags** 
    Don't copy row tags. 

  **-table** *srcTable*
    Copy the row *srcRow* from the datatable *srcTable*.  By default
    *tableName* is the source table.

*tableName* **row create** ?\ *switches* ...\ ?

  Creates a new row in *tableName*. The cells of the new row is initially
  empty. The index of the new row is returned.  *Switches* can be any of
  the following:

  **-after** *row*
    The position of the new row will be after *row*. *Row* may
    be a label, index, or tag, but may not represent more than one
    row.

  **-before** *row*
    The position of the new row will be before *row*. *Row* may
    be a label, index, or tag, but may not represent more than one
    row.

  **-label** *label*
    Specifies the row label for the new row.

  **-tags** *tagList*
    Specifies the tags to add to the row.

*tableName* **row delete** *row*...

  Deletes rows from the table. *Row* may be a row label, index,
  or tag and may refer to multiple rows (example: "all").  

*tableName* **row duplicate** *row*...

  Creates duplicate rows for each *row* given.  The row label is
  duplicated.  The row tags are copied. *Row* may be a row label,
  index, or tag and may refer to multiple rows (example: "all").
  
*tableName* **row empty** *row*

  Returns the indices of the empty columns in *row*.  *Row* may be a label,
  index, or tag, but may not represent more than one row.

*tableName* **row exists** *row*

  Indicates if a row labeled *row* in the table. Returns "1" if
  the row exists, "0" otherwise.

*tableName* **row extend** *numRows* ?\ *switches* ...\ ?

  Extends the table by one of more rows.  If *numRows* is not present
  then new 1 row is added.  *Switches* can be any of the following:

  **-labels** *list*
    Specifies the row labels for the new rows.

*tableName* **row get** ?\ *-labels*\ ? *row* ?\ *column* *column* ...\ ?

  Retrieves the values from the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  Normally all the values of *row* are retrieved. If one or more
  *column* arguments are specified, then only the columns specified are
  retrievd.  *Column* may be a column label, index, or tag.

  Returns pairs of values and indices of the selected columns. If the
  *-labels* flag is present, the column label is returned instead of the
  index.

*tableName* **row index** *row* 

  Returns the index of the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  
*tableName* **row indices** ?\ *switches* ...\ ? ?\ *pattern* ...\ ?

  Returns the indices of the rows whose labels match any *pattern*. 
  *Switches* can be any of the following:

  **-duplicates** 
    Return only the indices of the duplicate row labels.

*tableName* **row join** *srcTable* ?\ *switches* ...\ ?

  FIXME:
  Joins the rows of *srcTable* with *tableName*.
  The row tags are also copied. *Switches* can be any of
  the following:

  **-column** *columnList*
    Specifies the subset of columns from *srcTable* to add.  By default
    all columns are added.
    
  **-notags** 
    Don't copy row tags.
    
  **-rows** *rowList*
    Specifies the subset of rows from *srcTable* to add.  By default
    all rows are added.

*tableName* **row label** *row* ?\ *label*? ?\ *row* *label* ...\ ?

  Gets or sets the labels of the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  If *row* is the only argument, then the label of the row
  is returned.  If *row* and *label* pairs are specified, then
  set the labels of the specificed rows are set.  
  
*tableName* **row labels** *row* ?\ *labelList*?

  Gets or sets the all labels of the specified row.  If *labelList* is
  present, then row labels are set from the list of row labels.  

*tableName* **row move** *src* *dest* ?\ *numRows*\ ?

  Move one or move rows in the table.  *Src* and *dest* may be a
  label, index, or tag, but may not represent more than one row.
  By default only 1 row is moved, but if *numRows* is present then
  the more rows may be specified.  Moves cannot overlap.  
  
*tableName* **row names**  ?\ *pattern* ...\ ?

  Returns the labels of the rows in the table.  If one of *pattern*
  arguments are present, then any of the row labels matching one
  of the patterns is returned.

*tableName* **row nonempty**  *row*

  Returns the indices of the non-empty columns in the row.  *Row* may be
  a label, index, or tag, but may not represent more than one row.

*tableName* **row set**  *row* ?\ *column*\ *value* ...\ ? 

  Sets values for columns in the specified row. *Row* may be a label,
  index, or tag and may refer to multiple rows (example: "all").  If one
  or more *column* *value* pairs are found *value* is set at *row*, *column*
  in the table.  If either *row* or *column* does not exist, the row or
  column is automatically created. If the row or column is an index, the
  table may be grown. *Value* is the value to be set.  If the type of
  *column* is not *string*, *value* is converted into the correct type.  If
  the conversion fails, an error will be returned.

*tableName* **row tag add**  *tag* ?\ *row* ...\ ? 

  Adds the *tag* to *row*.  If no *row* arguments are present, *tag*
  is added to the row tags managed by *tableName*.  This is use for
  creating empty row tags (tags that refer to no rows).

*tableName* **row tag delete**  *tag* ?\ *row* ...\ ? 
  
  Removes the *tag* from *row*.  The built-in tags "all" and "end" can't
  be deleted and are ignored.

*tableName* **row tag exists**  *tag* ?\ *row* ...\ ? 

  Indicates if any row in *tableName* has *tag*.  Returns "1" if the tag
  exists, "0" otherwise.  By default all rows are searched. But if one
  or more *row* arguments are present, then if the tag is found in any
  *row*, "1" is returned. *Row* may be a label, index, or tag and may
  refer to multiple rows (example: "all").

*tableName* **row tag forget**  ?\ *tag* ...\ ? 

  Remove one or more tags from all the rows in *tableName*.

*tableName* **row tag get** *row* ?\ *pattern* ...\ ? 

  Returns the tags for *row*.  *Row* may be a label, index, or tag, but may
  not represent more than one row. By default all tags for *row* are
  returned.  But if one or more *pattern* arguments are present, then any
  tag that matching one of the patterns will be returned.

*tableName* **row tag indices** ?\ *tag* ...\ ? 

  Returns the row indices that have one or more *tag*.

*tableName* **row tag labels** ?\ *tag*...\ ? 

  Returns the row labels that have one or more *tag*.

*tableName* **row tag names** ?\ *pattern* ...\ ? 

  Returns the row tags of the table. By default all row tags are
  returned. But if one or more *pattern* arguments are present, then any
  tag that matching one of the patterns will be returned.

*tableName* **row tag range** *first* *last* ?\ *tag* ...\ ? 

  Adds one or more tags the rows in the range given.  *First* and *last*
  may be a label, index, or tag, but may not represent more than one
  row.

*tableName* **row tag set** *row* \ *tag*\... 

  Adds one or more tags to *row*. *Row* may be a row label, index, or
  tag and may refer to multiple rows (example: "all").

*tableName* **row tag unset** *row* \ *tag*\...

  Remove one or more tags from *row*. *Row* may be a row label,
  index, or tag and may refer to multiple rows (example: "all").

*tableName* **row unset**  *row* ?\ *column* ...\?

  Unsets the values of the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  One or more *column* and *value* pairs may be specified.  
  *Column* may be a column label, index, or tag.  It specifies the column
  whose value is to be unset.  

*tableName* **row values**  *row* ?\ *valueList*?

  Gets or sets the values of the specified row.  *Row* may be a label,
  index, or tag, but may not represent more than one row.  If *valueList*
  is present, then the values of the table are set from the elements of the
  list.  If there are more values in the list than columns in the table,
  the table is extended.  If there are less, the remaining columns remain
  the same.

*tableName* **set** *row* *column* *value* 

  Sets the value at *row*, *column* in *tableName*.  *Row* and *column* may
  be a label, index, or tag and may refer to multiple rows (example:
  "all"). If either *row* or *column* does not exist, the row or column is
  automatically created.  If the row or column is an index, the table may
  be grown. *Value* is the value to be set.  If the type of *column* is not
  *string*, *value* is converted into the correct type.  If the conversion
  fails, an error will be returned.

*tableName* **sort** ?\ *switches* ...\ ?

  Sorts the table based on the columns specified.  The type comparison is
  determined from the column type.  But you can use **-ascii** or
  **-dictionary** switch to sort the rows.  If the **-list**,
  **-nonempty**, **-unique**, or **-values** switches are present, a list
  of the sort rows is returned instead of rearranging the rows in the
  table. *Switches* can be one of the following:

  **-ascii**
    Use string comparison with Unicode code-point collation order (the name
    is for backward-compatibility reasons.)  The string representation of
    the values are compared.   

  **-columns** *columnList*
    Compare values in the columns in *columnList*.  This defines
    the comparison order.

  **-decreasing** 
    Sort the rows highest to lowest. By default the rows are sorted
    lowest to highest.

  **-dictionary** 
    Use dictionary-style comparison. This is the same as -ascii except (a)
    case is ignored except as a tie-breaker and (b) if two strings contain
    embedded numbers, the numbers compare as integers, not characters.  For
    example, in -dictionary mode, bigBoy sorts between bigbang and bigboy,
    and x10y sorts between x9y and x11y.

  **-frequency** 
    Sort the rows according to frequency of the values.

  **-list** 
    Return a list of the sorted rows instead of rearranging the rows
    in the table.

  **-nonempty** 
    Return only non-empty values.  This only has affect when the
    **-values** switch is set.

  **-rows** *rowList*
    Consider only the rows in *rowList*.  *RowList* is a list of
    of row labels, indices, or tags that may refer to multiple rows.
    The list of rows will be returned.

  **-unique** 
    Return a list of unique values.  

  **-values** 
    Return the row values.  By default the row indices are returned.

*tableName* **trace cell** *row* *column* *ops* *command*

  Registers a callback to *command* when the cell (designated by *row* and
  *column*) value is read, written, or unset. *Row* and *column* may be a
  label, index, or tag and may refer to multiple rows (example: "all").
  *Ops* indicates which operations are of interest, and consists of one or
  more of the following letters:

  **r**
    Invoke *command* whenever the cell value is read. 
  **w**
    Invoke *command* whenever the cell value is written.  
  **c**
    Invoke *command* whenever the cell value is created.  This happens
    when the cell was previously empty.
  **u** 
    Invoke *command* whenever the cell value is unset.  

*tableName* **trace column** *column* *ops* *command*

  Registers a callback to *command* when any cell in the *column* is read,
  written, or unset. *Column* may be a label, index, or tag and may refer
  to multiple columns (example: "all").  *Ops* indicates which operations
  are of interest, and consists of one or more of the following letters:

  **r**
    Invoke *command* whenever the cell value is read. 
  **w**
    Invoke *command* whenever the cell value is written.  
  **c**
    Invoke *command* whenever the cell value is created.  This happens
    when the cell was previously empty.
  **u** 
    Invoke *command* whenever the cell value is unset.  

*tableName* **trace delete** *traceName*...

  Deletes the trace associated with *traceName*.

*tableName* **trace info** *traceName*

  Describes *traceName*.  A list of name value pairs is returned.
  The *name*, *row*, *column*, *flags*, and *command* are returned.
  
*tableName* **trace names** ?\ *pattern* ...\ ?

  Returns the names of the traces currently registered. This includes cell,
  row, and column traces.  If one of *pattern* arguments are present, then
  any of the trace name matching one of the patterns is returned.
   
*tableName* **trace row** *row* *how* *command*

  Registers a callback to *command* when any cell in the *row* is read,
  written, or unset. *Row* may be a label, index, or tag and
  may refer to multiple rows (example: "all").  *Ops* indicates which
  operations are of interest, and consists of one or more of the following
  letters:

  **r**
    Invoke *command* whenever the cell value is read. 
  **w**
    Invoke *command* whenever the cell value is written.  
  **c**
    Invoke *command* whenever the cell value is created.  This happens
    when the cell was previously empty.
  **u** 
    Invoke *command* whenever the cell value is unset.  


*tableName* **unset** *row* *column* ?\ *row*\ *column* ...\ ?

  Unsets the values located at one or more *row*, *column* locations.
  *Row* and *column* may be a label, index, or tag.  Both may represent
  more than mulitple rows or columns (example "all").  When a value
  if unset, the cell is empty.
  
*tableName* **watch column**  *column* ?\ *flags* ...\ ? *command*

  **-allevents** 
    Notify when columns are created, deleted, moved, or relabeled.

  **-create** 
    Notify when columns are created.

  **-delete** 
    Notify when columns are deleted.

  **-move** 
    Notify when columns are moved.  This included when the table is sorted.

  **-relabel** 
    Notify when columns are relabeled.

  **-whenidle** 
    Don't trigger the callback immediately.  Wait until the next idle time.

*tableName* **watch delete** *watchName*...

*tableName* **watch info** ?\ *watchName*\ ?

*tableName* **watch names** ?\ *pattern* ...\ ?

  Returns the names of the watches registered in the table.  This includes
  both row and column watches.  If one of *pattern* arguments are present,
  then any of the watch names matching one of the patterns is returned.

*tableName* **watch row**  *row* ?\ *flags*\ ? *command*

  **-allevents** 
    Notify when rows are created, deleted, moved, or relabeled.

  **-create** 
    Notify when rows are created.

  **-delete** 
    Notify when rows are deleted.

  **-move** 
    Notify when rows are moved.  This included when the table is sorted.

  **-relabel** 
    Notify when rows are relabeled.

  **-whenidle** 
    Don't trigger the callback immediately.  Wait until the next idle time.


.. _formats:

DATATABLE FORMATS
=================

Handlers for various datatable formats can be loaded using the TCL
**package** mechanism.  The formats supported are "csv", "xml", "sqlite",
"mysql", "psql", "vector", and "tree".

**csv**
 The *csv* module reads and writes comma separated values (CSV) data.
 The package can be manually loaded as follows.

   **package require blt_datatable_csv**

 By default this package is automatically loaded when you use the *csv*
 format in the **import** or **export** operations.

 *tableName* **import csv** ?\ *switches..*\ ?

  Imports the CSV data into the datatable. The following import switches
  are supported.  One of the **-file** or **-data** switches must be
  specified, but not both.

  **-autoheaders** 
   Set the column labels from the first row of the CSV data.  

  **-columnlabels** *labelList*
   Set the column labels from the list of labels in *labelList*.

  **-comment** *char*
   Specifies a comment character.  Any line in the CSV file starting
   with this character is treated as a comment and ignored.  By default
   the comment character is "", indicating no comments.

  **-data** *string*
   Read the CSV information from *string*.

  **-emptyvalue** *string*
   Specifies a string value to use for cells when empty fields
   are found in the CSV data.

  **-headers** *labelList*
   Specifies the column labels from the list of labels in *labelList*.

  **-file** *fileName*
   Read the CSV file from *fileName*.

  **-maxrows** *numRows*
   Specifies the maximum number of rows to load into the table. 

  **-quote** *char*
   Specifies the quote character.  This is by default the double quote (")
   character.

  **-separator** *char*
   Specifies the separator character.  By default this is the comma (,)
   character. If *char* is "auto", then the separator is automatically
   determined.

 *tableName* **export csv** ?\ *switches..*\ ?

  Exports the datatable into CSV data.  If no **-file** switch is provided,
  the CSV output is returned as the result of the command.  The following
  import switches are supported:

   **-columnlabels** 
    Indicates to create an extra row in the CSV containing the
    column labels.

   **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.  By default
    all columns are exported.

   **-file** *fileName*
    Write the CSV output to the file *fileName*.

   **-quote** *char*
     Specifies the quote character.  This is by default the double quote (")
     character.

   **-rowlabels** 
    Indicates to create an extra column in the CSV containing the
    row labels.

   **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  By default
    all rows are exported.

   **-separator** *char*
    Specifies the separator character.  This is by default the comma (,)
    character.

**mysql**
 The *mysql* module reads and writes tables a Mysql database.
 The package can be manually loaded as follows.

   **package require blt_datatable_mysql**

 By default this package is automatically loaded when you use the *mysql*
 format in the **import** or **export** operations.

 *tableName* **import mysql** ?\ *switches..*\ ?

   Imports a table from a *Mysql* database.  The following switches
   are supported:

   **-db** *dbName*
    Specifies the name of the database.  

   **-host** *hostName*
    Specifies the name or address of the *Mysql* server host.  

   **-user** *userName*
    Specifies the name of the *Mysql* user.  By default, the USER
    environment variable is used.

   **-password** *password*
    Specifies the password of the *Mysql* user. 

   **-port** *portNumber*
    Specifies the port number of the *Mysql* server.

   **-query** *string*
    Specifies the SQL query to make to the *Mysql* database.

**psql**

 The *psql* module reads and writes tables from a *Postgresql* database.
 The package can be manually loaded as follows.

   **package require blt_datatable_psql**

 By default this package is automatically loaded when you use the *psql*
 format in the **import** or **export** operations.

 *tableName* **import psql** ?\ *switches..*\ ?

  Imports a table from a *Postgresql* database.  The **-db**, **-host**,
  **-password**, **-port** and **-query** switches are required. The
  following switches are supported:

  **-db** *dbName*
   Specifies the name of the database.  

  **-host** *hostName*
   Specifies the name or address of the *Postgresql* server host.  

  **-user** *userName*
   Specifies the name of the *Postgresql* user.  By default, the "USER"
   environment variable is used.

  **-password** *password*
   Specifies the password of the *Postgresql* user. 

  **-port** *portNumber*
   Specifies the port number of the *Postgresql* server.

  **-query** *string*
   Specifies the SQL query to make to the *Postgresql* database.

  **-table** *tableName*
   Specifies the name of the *Postgresql* table being queried.

 *tableName* **export psql** ?\ *switches..*\ ?

  Exports *tableName* to a *Postgresql* database.  The **-db**, **-host**,
  **-password**, **-port** and **-table** switches are required. The
  following switches are supported:

  **-columns** *columnList*
   Specifies the subset of columns from *tableName* to export.  By default
   all columns are exported.

  **-db** *dbName*
   Specifies the name of the database.  

  **-host** *hostName*
   Specifies the name or address of the *Postgresql* server host.  

  **-user** *userName*
   Specifies the name of the *Postgresql* user.  By default, the "USER"
   environment variable is used.

  **-password** *password*
   Specifies the password of the *Postgresql* user. 

  **-port** *portNumber*
   Specifies the port number of the *Postgresql* server.

  **-rows** *rowList*
   Specifies the subset of rows from *tableName* to export.  By default
   all rows are exported.

  **-table** *tableName*
   Specifies the name of the *Postgresql* table being written.

**sqlite**

 The *sqlite* module reads and writes tables a *Sqlite3* database.
 The package can be manually loaded as follows.

   **package require blt_datatable_sqlite**

 By default this package is automatically loaded when you use the *sqlite*
 format in the **import** or **export** operations.

 *tableName* **import sqlite** ?\ *switches* ...\ ?

  Imports a table from a *Sqlite* database.  The following export switches
  are supported:

   **-file** *fileName*
     Read from the *Sqlite* file *fileName*.

   **-query** *string*
     Specifies the SQL query to make to the *Sqlite* database.

 *tableName* **export sqlist** ?\ *switches..*\ ?

  Exports the datatable into *Sqlite* data.  The **-file** switch is
  required. The following import switches are supported:

  **-columns** *columnList*
   Specifies the subset of columns from *tableName* to export.  By default
   all columns are exported.

  **-file** *fileName*
   Write the *Sqlite* output to the file *fileName*.

  **-rowlabels** 
   Export the row labels from *tableName* as an extra column "_rowId" in
   the *Sqlite* table.

  **-rows** *rowList*
   Specifies the subset of rows from *tableName* to export.  By default
   all rows are exported.

  **-table** *tableName*
   Name of the *Sqlite* table to write to.  If a *tableName* already
   exists, it is overwritten.

**tree**

 The *tree* module reads from and writes to BLT trees.
 The package can be manually loaded as follows.

   **package require blt_datatable_tree**

  By default this package is automatically loaded when you use the *tree*
  format in the **import** or **export** operations.

  *tableName* **import tree** *treeName* ?\ *switches..*\ ?

   Imports a BLT tree into the datatable.  *TreeName* is the name of the
   BLT tree.

   **-depth** *maxDepth*
     Traverse *treeName* a maximum of *maxDepth* levels starting
     from *node*.

   **-inodes** 
     Store the indices of the tree nodes in a column called "inode".

   **-root** *node*
     Specifies the root node of the branch to be imported. By default,
     the root of the tree is the root node.

 *tableName* **export tree** *treeName* ?\ *switches..*\ ?

   Exports the datatable into a BLT tree.  *TreeName* is the name of the
   BLT tree.

   **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.  By default
    all columns are exported.

   **-root** *node*
    Specifies the root node of the branch where the datatable is to be
    exported. By default the root of the tree is the root node.

   **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  By default
    all rows are exported.

**vector**

 The *vector* module reads and writes data from a BLT vector.
 The package can be manually loaded as follows.

   **package require blt_datatable_vector**

 By default this package is automatically loaded when you use the *vector*
 format in the **import** or **export** operations.

 *tableName* **import vector** ?\ *destColumn* *vecName* ...\ ?

   Imports a columns from one of more BLT vectors.  *VecName* is the name of
   a BLT vector.  *DestColumn* may be a label, index, or tag, but may not
   represent more than one column.  If *destColumn* does not exist, it is
   automatically created.  All the values previously in *destColumn* are
   deleted.  Rows may added to the datatable to store the vector values.

 *tableName* **export vector** ?\ *srcColumn* *vecName* ...\ ?

   Exports the values from one more columns to BLT vectors.  *VecName* is
   the name of a BLT vector.  *SrcColumn* may be a label, index, or tag,
   but may not represent more than one column.  All the values previously
   in *vecName* are deleted.

**xml**

 The *xml* module reads and writes XML data.  The package can be manually
 loaded as follows.

   **package require blt_datatable_xml**

 By default this package is automatically loaded when you use the *xml*
 format in the **import** or **export** operations.

 *tableName* **import xml** ?\ *switches..*\ ?

   Imports XML data into the datatable.  The following export switches are
   supported:

   **-data** *string*
     Read XML from the data *string*.

   **-file** *fileName*
     Read XML from the file *fileName*.

   **-noattrs** 
     Don't import XML attributes into the datatable.

   **-noelems** 
     Don't import XML elements into the datatable.

   **-nocdata** 
     Don't import XML character data (CDATA) into the datatable.

 *tableName* **export xml** ?\ *switches..*\ ?

  Exports the datatable into XML data.  If no **-file** switch is provided,
  the XML output is returned as the result of the command.  The following
  import switches are supported:

  **-columns** *columnList*
   Specifies the subset of columns from *tableName* to export.  By default
   all columns are exported.

  **-file** *fileName*
   Write the XML output to the file *fileName*.

  **-rows** *rowList*
   Specifies the subset of rows from *tableName* to export.  By default
   all rows are exported.

EXAMPLE
=======

KEYWORDS
========

datatable, tableview
