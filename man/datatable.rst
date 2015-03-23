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

**blt::datatable names** ?\ *pattern*...\ ?

DESCRIPTION
===========

The **datatable** command creates datatable data objects.  A *datatable*
object is table of cells.  Row and columns both have labels that address
the row or column.  Cells contain data.  nodes do not have to contain the
same data keys.  It is associated with a TCL command that you can use to
access and modify the its structure and data. 

SYNTAX
======

**blt::datatable create** ?\ *tableName*\ ?  

  Creates a new datatable object. The name of the new datatable is
  returned.  If no *tableName* argument is present, then the name of the
  datatble is automatically generated in the form "`datatable0`",
  "`datatable1`", etc.  If the substring "`#auto`" is found in *tableName*,
  it is automatically substituted by a generated name.  For example, the
  name `.foo.#auto.bar` will be translated to `.foo.datatable0.bar`.

  A new TCL command (by the same name as the datatable) is also created.
  Another TCL command or datatble object can not already exist as
  *tableName*.  If the TCL command is deleted, the datatable will also be
  freed.  The new datatable will be empty, no rows or columns.  Datatables
  are by default, created in the current namespace, not the global
  namespace, unless *tableName* contains a namespace qualifier, such as
  "`fred::myDataTable`".

**blt::datatable destroy** *tableName*...

  Releases one of more datatables.  The TCL command associated with
  *tableName* is also removed.  Datatables are reference counted.
  The internal datatable data object isn't destroyed until no one else
  is using it.

**blt::datatable exists** *tableName*

  Indicates if the named table already exists.  Returns `1` is
  the table exists, `0` otherwise.

**blt::datatable load** *format* *libPath*

  Dynamically loads the named datatable module.  This is used internally
  to load datatable modules.

**blt::datatable names** ?\ *pattern*...\ ?

  Returns the names of all datatable objects.  if a *pattern* argument
  is given, then the only those datatables whose name matches pattern will
  be listed.

REFERENCING ROWS AND COLUMNS
============================

Row and columns in a *datatable* object may be referred by either 
their index, label, or tag.

 *index*
   The number of the row or column.  Row and column indices start from 0.
   The index of a row or column may change as rows or columns are add,
   deleted, moved, or sorted.

 *label*
   The label of the row or column.  Each row and column has a label.
   Labels can not be numbers (to distinguish them from indices). There may
   be duplicate row or column labels.

 *tag*
   A tag is a string associated with a row or column.  They are a useful of
   grouping rows and columns.  Columns and rows can have any number of tags
   associated with them.  There are two built-in tags: "all" and "end".
   Every row and column have the tag "all".  The last row and column in the
   table have the tag "end".  Row and column tags are distinct.  Tags may
   be empty (associated with no rows or columns).
     
You can also distinguish between indices, labels and tables by prefixing
them with "index:", "label:", or "tag:".  

When specifying row or columns in datatable object commands, if the
specifier is an integer then it is assumed to be a row or column index.
Some datatable commands only operate on a single row or column at a
time; if *tag* or *label* specified multiple rows or columns, then
an error is generated.

.. _`DATATABLE OPERATIONS`:

DATATABLE OPERATIONS
====================

Once you create a datatable object, you can use its TCL command 
to query or modify it.  The general form is

  *tableName* *operation* ?\ *arg*\ ?...

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for datatables are listed below.

*tableName* **add** *srcTable* ?\ *switches*...\ ?

  Adds the rows from *srcTable* to the bottom of *tableName*. Columns are
  matched by their labels. New columns are automatically created. For
  example, if *tableName* doesn't have a column labeled "`foo`", one will
  be created.  The column tags are also copied. ?Switches? can be any of
  the following:

  **-columns** *columnList*
    Specifies the subset of columns from *srcTable* to add.  By default
    all columns are added.

  **-row** *rowList*
    Specifies the subset of rows from *srcTable* to add.  By default
    all rows are added.
    
  **-notags** 
    Don't copy column tags. 

*tableName* **append** *row* *column* *value* ?\ *value*...\ ?

  Appends one or more values to the current value at *row*, *column* in
  *tableName*.  This is normally used for `string` type cells, but can be
  used for other types as well.  Both *row* and *column* may be a label,
  index, or tag, and may represent more than one row or column.

*tableName* **attach** *newTable*

  Attaches to an existing datatable object *newTable*.  The underlying
  table (row, columns, cells) are shared with *tableName*.  Tags, traces,
  and watches are not shared. The current table associated with *tableName*
  is discarded.  It will be destroyed is no one else is using it.  The
  current set of tags, notifier events, and traces in *tableName* are
  reset.

*tableName* **column copy** *srcColumn* *destColumn* ?\ *switches*...\ ?

  Copies the column *srcColumn* into *destColumn*.  If a column
  *destColumn* doesn't already exist in *tableName*, one is created.
  *SrcColumn* and *destColumn* may be a label, index, or tag, but may not
  represent more than one column.  ?Switches? can be any of the following:

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

*tableName* **column create** ?\ *switches*...\ ?

  Creates a new column in *tableName*. The cells of the new column
  is initially empty. The index of the new column is returned.
  ?Switches? can be any of the following:  

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
    Specifies the type of column. The type may be `string`, `double`,
    `integer`, `boolean`, `time`, or , `blob`.

*tableName* **column delete** *column*...

  Deletes columns from the table. *Column* may be a column label, index,
  or tag and may refer to multiple columns (example: `all`).  

*tableName* **column duplicate** *column*...

  Creates duplicate columns for each *column* given.  The column label is
  duplicated.  The column tags are copied. *Column* may be a column label,
  index, or tag and may refer to multiple columns (example: `all`).
  
*tableName* **column empty** *column*

  Returns a list of the indices of the empty rows in *column*.  *Column*
  may be a label, index, or tag, but may not represent more than one
  column.

*tableName* **column exists** *column*

  Indicates if a column labeled *column* in the table. Returns `1` if
  the column exists, `0` otherwise.

*tableName* **column extend** *numColumns* ?\ *switches*...\ ?

  Extends the table by one of more columns.  If *numColumns* is not present
  then new 1 column is added.  ?Switches? can be any of the following:

  **-labels** *list*
    Specifies the column labels for the new columns.

*tableName* **column get** ?\ *-labels*\ ? *column* ?\ *row*...\ ?

  Retrieves the values from the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  Normally all the values of *column* are retrieved. If one or more
  *row* arguments are specified, then only the rows specified are
  retrievd.  *Row* may be a row label, index, or tag.

  Returns a list containing pairs of values and indices of the selected
  rows. If the *-labels* flag is present, the row label is returned instead
  of the index.

*tableName* **column index** *column* 

  Returns the index of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  
*tableName* **column indices** ?\ *switches*...\ ? ?\ *pattern*...\ ?

  Returns the indices of the column whose labels match any *pattern*. 
  ?Switches? can be any of the following:

  **-duplicates** 
    Return only the indices of the duplicate columns.

*tableName* **column join** *srcTable* ?\ *switches*...\ ?

  FIXME:
  Joins the columns of *srcTable* with *tableName*.
  The column tags are also copied. ?Switches? can be any of
  the following:

  **-columns** *columnList*
    Specifies the subset of columns from *srcTable* to add.  By default
    all columns are added.

  **-row** *rowList*
    Specifies the subset of rows from *srcTable* to add.  By default
    all rows are added.
    
  **-notags** 
    Don't copy column tags.
    
*tableName* **column label** *column* ?\ *label*\ *column*\ *label*\ ?...

  Gets or sets the labels of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  If *column* is the only argument, then the label of the column
  is returned.  If *column* and *label* pairs are specified, then
  set the labels of the specificed columns are set.  
  
*tableName* **column labels** *column* ?\ *labelList*\ ?

  Gets or sets the all labels of the specified column.  If *labelList* is
  present, then column labels are set from the list of column labels.  

*tableName* **column move** *src* *dest* ?\ *numColumns*\ ?

  Move one or move columns in the table.  *Src* and *dest* may be a
  label, index, or tag, but may not represent more than one column.
  By default only 1 column is moved, but if *numColumns* is present then
  the more columns may be specified.  Moves cannot overlap.  
  
*tableName* **column names**  ?\ *pattern*...\ ?

  Returns the labels of the columns in the table.  If one of *pattern*
  arguments are present, then any of the column labels matching one
  of the patterns is returned.

*tableName* **column nonempty**  *column*

  Returns the indices of the non-empty rows in the column.  *Column* may be
  a label, index, or tag, but may not represent more than one column.

*tableName* **column set**  *column* ?\ *row*\ *value*\...? 

  Sets the values of the specified column.  *Column* may be a label, index,
  or tag, but may not represent more than one column.  One or more *row*
  and *value* pairs may be specified.  *Row* may be a row label, index, or
  tag.  It specifies the row whose value is to be set.  *Value* is the new
  value.

*tableName* **column tag add**  *tag* ?\ *column*\...? 

  Adds the *tag* to *column*.  If no *column* arguments are present, *tag*
  is added to the column tags managed by *tableName*.  This is use for
  creating empty column tags (tags that refer to no columns).

*tableName* **column tag delete**  *tag* ?\ *column*\...? 
  
  Removes the *tag* from *column*.  The built-in tags `all` and `end` can't
  be deleted and are ignored.

*tableName* **column tag exists**  *tag* ?\ *column*\ ? 

  Indicates if any column in *tableName* has *tag*.  If a *column* argument
  is given, then if only *column* is tested for the tag.  If Returns `1` if
  the tag exists, `0` otherwise.

*tableName* **column tag forget**  ?\ *tag*\...? 

  Remove one or more tags from all the columns in *tableName*.

*tableName* **column tag get** *column* ?\ *pattern*\...? 

  Returns the tags for *column*.  By default all tags for *column* are
  returned.  But if one or more *pattern* arguments are present, then any
  tag that matching one of the patterns will be returned.

*tableName* **column tag indices** ?\ *tag*\...? 

  Returns the column indices that have one or more *tag*.

*tableName* **column tag labels** ?\ *tag*\...? 

  Returns the column labels that have one or more *tag*.

*tableName* **column tag names** ?\ *pattern*\...? 

  Returns the column tags of the table. By default all column tags are
  returned. But if one or more *pattern* arguments are present, then any
  tag that matching one of the patterns will be returned.

*tableName* **column tag range** *first* *last* ?\ *tag*\...? 

  Adds one or more tags the columns in the range given.  *First* and *last*
  may be a label, index, or tag, but may not represent more than one
  column.

*tableName* **column tag set** *column* \ *tag*\... 

  Adds one or more tags to *column*. *Column* may be a column label, index, or
  tag and may refer to multiple columns (example: `all`).

*tableName* **column tag unset** *column* \ *tag*\...

  Remove one or more tags from *column*. *Column* may be a column label,
  index, or tag and may refer to multiple columns (example: `all`).

*tableName* **column type**  *column* ?\ *type* *column* *type*...?

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

*tableName* **column unset**  *column* ?\* row*...\?

  Unsets the values of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  One or more *row* and *value* pairs may be specified.  
  *Row* may be a row label, index, or tag.  It specifies the row
  whose value is to be unset.  

*tableName* **column values**  *column* ?\ *valueList* \?

  Gets or sets the values of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  If *valueList* is present, then the values of the table are
  set from the elements of the list.  If there are more values in the
  list than rows in the table, the table is extended.  If there
  are less, the remaining rows remain the same.

*tableName* **copy** *srcTable* 

  Makes a copy of *srcTable in *tableName*.  All previous rows,
  column, cells, and tags in *tableName* are first removed.

*tableName* **dir** *path* ?\ *switches*...\ ?

  Fills the table with the directory listing specified by *path*. If
  *path* is a directory, then its entries are added to the table.
  ?Switches? can be any of the following:

  **-hidden** 
    Add hidden file and directory entries to the table.  

  **-readable** 
    Add readable file and directory entries to the table.

  **-writable** 
    Add writable file and directory entries to the table.

  **-readonly** 
    Add readonly (not writable) file and directory entries to the table.

  **-executable** 
    Add executable file and directory entries to the table.

  **-file** 
    Add file entries to the table.

  **-directory** 
    Add directory entries to the table.

  **-link** 
    Add link entries to the table.

  **-pattern** *pattern*
    Only add entries matching *pattern* to the table.

  The new columns are the following:
   
  *name*
    The name of the directory entry.

  *type*
    The type of entry.  *Type* may be `file`, `directory`,
    `characterSpecial`, `blockSpecial`, `fifo`, or `link`.

  *size*
    The number of bytes for the entry.

  *uid*
    The number representing the user ID or the entry,

  *gid*
    The number representing the group ID of the entry,

  *atime*
    The number representing the last access time of the entry,

  *mtime*
    The number representing the last modication time of the entry,

  *ctime*
    The number representing the last change time of the entry,

  *mode*
    The number representing the mode (permissions) of the entry,

*tableName* **dump** ?\ *switches*...\ ?

  **-column** *columnList*
    Specifies the subset of columns from *srcTable* to add.  By default
    all columns are added.
    
  **-file** *fileName*
    Don't copy row tags.

  **-rows** *rowList*
    Specifies the subset of rows from *srcTable* to add.  By default
    all rows are added.

*tableName* **duplicate** ?\ *table*\ ?

*tableName* **emptyvalue** ?\ *newValue*\ ?

*tableName* **exists** *row* *column*

  Indicates if a value exists at *row*, *column* in *tableName*.  
  *Row* and *column* may be a label, index, or tag, but may not represent
  more than one row or column. If the cell is empty, then `0` is returned.
  If either *row* or *column* do not exist, `0` is returned.  Otherwise,
  `1` is returned.

*tableName* **export** *format* ?\ *switches*...\ ?

*tableName* **find** *expression* ?\ *switches*...\ ?

  Finds the rows that satisfy *expression*.  *Expression* is a TCL
  expression.  The expression is evaluated for each row in the table.
  Variable names using column indices or labels (such as "${*index*}" and
  "${*label*}" may be used to refer the values in the row.  Note that
  if a cell is empty it won't have a variable associated with it.  You
  can test for this by "[info exists var]". 

  **-addtag**  *tagName*
    Notify when rows are created, deleted, moved, or relabeled.

  **-count**  *maxMatches*
    Notify when rows are created, deleted, moved, or relabeled.

  **-emptyvalue**  *string*
    Notify when rows are created, deleted, moved, or relabeled.

  **-invert**  
    Notify when rows are created, deleted, moved, or relabeled.

  **-rows** *rowList*
    Notify when rows are created, deleted, moved, or relabeled.

*tableName* **get** *row* *column* ?\ *defValue*\ ?

  Returns the value at *row*, *column* in *tableName*.  
  *Row* and *column* may be a label, index, or tag, but may not represent
  more than one row or column. If the cell is empty, then the empty value
  string is returned. By default it is an error if either *row* or *column*
  do not exist.  The *defValue* argument lets you return a known value
  instead of generating an error. *DefValue* can be any string.
  
*tableName* **import** *format* ?\ *switches*...\ ?

*tableName* **keys** *column*...

  Generates an internal lookup table from the columns given.  This is
  especially useful when a combination of column values uniquely represent
  rows of the table. *Column* may be a label, index, or tag, but may not
  represent more than one row or column.
  
*tableName* **lappend** *row* *column* value ?\ *value*...\ ?

  Appends one or more values to the current value at *row*, *column* in
  *tableName*.  Both *row* and *column* may be a label, index, or tag, and
  may represent more than one row or column. This is for `string` cells
  only.  Each new value is appended as a list element.

*tableName* **limits** ?\ *column*\ ?

  Returns a list of the minimum and maximum values in *tableName*.  If
  *column* is present, the minimum and maximum values in *column* are
  returned.

*tableName* **lookup** ?\ *value...*\ ?

  Searches for the row matching the values keys given.  *Value* is a value
  from the columns specified by the **keys** operation.  The order and number
  of the values must be the same as the columns that were specified in the
  **keys** operation.  If a matching combination is found, the index of the
  row is returned, otherwise `-1`.

*tableName* **maximum** ?\ *column*\ ?

  Returns the maximum value in the table.  If *column* is present, 
  the maximum value in *column* is returned.

*tableName* **minimum** ?\ *column*\ ?

  Returns the minimum value in the table.  If *column* is present, 
  the maximum value in *column* is returned.

*tableName* **numcolumns** ?\ *numColumns*\ ?

  Sets or gets the number of column in *tableName*.  If *numRows* is
  present, the table is grown or shrunk to accomodate the new size.

*tableName* **numrows** ?\ *numRows*\ ?

  Sets or gets the number of rows in *tableName*.  If *numRows* is
  present, the table is grown or shrunk to accomodate the new size.

*tableName* **restore** ?\ *switches*\ ?

  **-data**  *string*
    Notify when rows are created, deleted, moved, or relabeled.

  **-file**  *fileName*
    Notify when rows are created, deleted, moved, or relabeled.

  **-notags**  
    Notify when rows are created, deleted, moved, or relabeled.

  **-overwrite**  
    Notify when rows are created, deleted, moved, or relabeled.

*tableName* **row copy** *srcRow* *destRow* ?\ *switches*...\ ?

  Copies the row *srcRow* into *destRow*.  If a row *destRow* doesn't
  already exist in *tableName*, one is created.  *SrcRow* and *destRow* may
  be a label, index, or tag, but may not represent more than one row.
  ?Switches? can be any of the following:

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
    to *tableName* is also the source table.

*tableName* **row create** ?\ *switches*...\ ?

  Creates a new row in *tableName*. The cells of the new row
  is initially empty. The index of the new row is returned.
  ?Switches? can be any of the following:  

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

  **-type** *rowType*
    Specifies the type of row. The type may be `string`, `double`,
    `integer`, `boolean`, `time`, or , `blob`.

*tableName* **row delete** *row*...

  Deletes rows from the table. *Row* may be a row label, index,
  or tag and may refer to multiple rows (example: `all`).  

*tableName* **row duplicate** *row*...

  Creates duplicate rows for each *row* given.  The row label is
  duplicated.  The row tags are copied. *Row* may be a row label,
  index, or tag and may refer to multiple rows (example: `all`).
  
*tableName* **row empty** *row*

  Returns a list of the indices of the empty columns in *row*.  *Row*
  may be a label, index, or tag, but may not represent more than one
  row.

*tableName* **row exists** *row*

  Indicates if a row labeled *row* in the table. Returns `1` if
  the row exists, `0` otherwise.

*tableName* **row extend** *numRows* ?\ *switches*...\ ?

  Extends the table by one of more rows.  If *numRows* is not present
  then new 1 row is added.  ?Switches? can be any of the following:

  **-labels** *list*
    Specifies the row labels for the new rows.

*tableName* **row get** ?\ *-labels*\ ? *row* ?\ *column*...\ ?

  Retrieves the values from the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  Normally all the values of *row* are retrieved. If one or more
  *column* arguments are specified, then only the columns specified are
  retrievd.  *Column* may be a column label, index, or tag.

  Returns a list containing pairs of values and indices of the selected
  columns. If the *-labels* flag is present, the column label is returned
  instead of the index.

*tableName* **row index** *row* 

  Returns the index of the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  
*tableName* **row indices** ?\ *switches*...\ ? ?\ *pattern*...\ ?

  Returns the indices of the rows whose labels match any *pattern*. 
  ?Switches? can be any of the following:

  **-duplicates** 
    Return only the indices of the duplicate row labels.

*tableName* **row join** *srcTable* ?\ *switches*...\ ?

  FIXME:
  Joins the rows of *srcTable* with *tableName*.
  The row tags are also copied. ?Switches? can be any of
  the following:

  **-column** *columnList*
    Specifies the subset of columns from *srcTable* to add.  By default
    all columns are added.
    
  **-rows** *rowList*
    Specifies the subset of rows from *srcTable* to add.  By default
    all rows are added.

  **-notags** 
    Don't copy row tags.
    
*tableName* **row label** *row* ?\ *label*\ *row*\ *label*\ ?...

  Gets or sets the labels of the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  If *row* is the only argument, then the label of the row
  is returned.  If *row* and *label* pairs are specified, then
  set the labels of the specificed rows are set.  
  
*tableName* **row labels** *row* ?\ *labelList*\ ?

  Gets or sets the all labels of the specified row.  If *labelList* is
  present, then row labels are set from the list of row labels.  

*tableName* **row move** *src* *dest* ?\ *numRows*\ ?

  Move one or move rows in the table.  *Src* and *dest* may be a
  label, index, or tag, but may not represent more than one row.
  By default only 1 row is moved, but if *numRows* is present then
  the more rows may be specified.  Moves cannot overlap.  
  
*tableName* **row names**  ?\ *pattern*...\ ?

  Returns the labels of the rows in the table.  If one of *pattern*
  arguments are present, then any of the row labels matching one
  of the patterns is returned.

*tableName* **row nonempty**  *row*

  Returns the indices of the non-empty columns in the row.  *Row* may be
  a label, index, or tag, but may not represent more than one row.

*tableName* **row set**  *row* ?\ *column*\ *value*\...? 

  Sets the values of the specified row.  *Row* may be a label, index,
  or tag, but may not represent more than one row.  One or more *column*
  and *value* pairs may be specified.  *Column* may be a column label, index, or
  tag.  It specifies the column whose value is to be set.  *Value* is the new
  value.

*tableName* **row tag add**  *tag* ?\ *row*\...? 

  Adds the *tag* to *row*.  If no *row* arguments are present, *tag*
  is added to the row tags managed by *tableName*.  This is use for
  creating empty row tags (tags that refer to no rows).

*tableName* **row tag delete**  *tag* ?\ *row*\...? 
  
  Removes the *tag* from *row*.  The built-in tags `all` and `end` can't
  be deleted and are ignored.

*tableName* **row tag exists**  *tag* ?\ *row*\ ? 

  Indicates if any row in *tableName* has *tag*.  If a *row* argument
  is given, then if only *row* is tested for the tag.  If Returns `1` if
  the tag exists, `0` otherwise.

*tableName* **row tag forget**  ?\ *tag*\...? 

  Remove one or more tags from all the rows in *tableName*.

*tableName* **row tag get** *row* ?\ *pattern*\...? 

  Returns the tags for *row*.  By default all tags for *row* are
  returned.  But if one or more *pattern* arguments are present, then any
  tag that matching one of the patterns will be returned.

*tableName* **row tag indices** ?\ *tag*\...? 

  Returns the row indices that have one or more *tag*.

*tableName* **row tag labels** ?\ *tag*\...? 

  Returns the row labels that have one or more *tag*.

*tableName* **row tag names** ?\ *pattern*\...? 

  Returns the row tags of the table. By default all row tags are
  returned. But if one or more *pattern* arguments are present, then any
  tag that matching one of the patterns will be returned.

*tableName* **row tag range** *first* *last* ?\ *tag*\...? 

  Adds one or more tags the rows in the range given.  *First* and *last*
  may be a label, index, or tag, but may not represent more than one
  row.

*tableName* **row tag set** *row* \ *tag*\... 

  Adds one or more tags to *row*. *Row* may be a row label, index, or
  tag and may refer to multiple rows (example: `all`).

*tableName* **row tag unset** *row* \ *tag*\...

  Remove one or more tags from *row*. *Row* may be a row label,
  index, or tag and may refer to multiple rows (example: `all`).

*tableName* **row unset**  *row* ?\* column*...\?

  Unsets the values of the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  One or more *column* and *value* pairs may be specified.  
  *Column* may be a column label, index, or tag.  It specifies the column
  whose value is to be unset.  

*tableName* **row values**  *row* ?\ *valueList* \?

  Gets or sets the values of the specified row.  *Row* may be a label,
  index, or tag, but may not represent more than one row.  If *valueList*
  is present, then the values of the table are set from the elements of the
  list.  If there are more values in the list than columns in the table,
  the table is extended.  If there are less, the remaining columns remain
  the same.

*tableName* **set** *row* *column* *value* 

  Sets the value at *row*, *column* in *tableName*.  *Row* and *column* may
  be a label, index, or tag and may refer to multiple rows (example:
  `all`). If either *row* or *column* does not exist, the row or column is
  automatically created.  If the row or column is an index, the table may
  be grown. *Value* is the value to be set.  If the type of *column* is not
  *string*, *value* is converted into the correct type.  If the conversion
  fails, an error will be returned.

*tableName* **sort** ?\ *switches*...\ ?

  **-ascii** 
    Notify when rows are created, deleted, moved, or relabeled.

  **-columns** *columnList*
    Notify when rows are created, deleted, moved, or relabeled.

  **-decreasing** 
    Notify when rows are created, deleted, moved, or relabeled.

  **-dictionary** 
    Notify when rows are created, deleted, moved, or relabeled.

  **-list** 
    Notify when rows are created, deleted, moved, or relabeled.

  **-nonempty** 
    Notify when rows are created, deleted, moved, or relabeled.

  **-rows** *rowList*
    Notify when rows are created, deleted, moved, or relabeled.

  **-nonempty** 
    Notify when rows are created, deleted, moved, or relabeled.

  **-unique** 
    Notify when rows are created, deleted, moved, or relabeled.

  **-values** 
    Notify when rows are created, deleted, moved, or relabeled.

*tableName* **trace cell** *row* *column* *how* *command*

   Registers a callback to *command* when the cell is created, read, written,
   or unset. *How* describes what combinations of events.

*tableName* **trace column** *column* *how* *command*

*tableName* **trace delete** *traceName*...

*tableName* **trace info** *traceName*

  Describes *traceName*.
  
*tableName* **trace names** ?\ *pattern*...\ ?

  Returns a list of the traces currently registered. This includes cell,
  row, and column traces.  If one of *pattern* arguments are present, then
  any of the trace names matching one of the patterns is returned.
   
*tableName* **trace row** *row* *how* *command*

*tableName* **unset** *row* *column* ?\ *row*\ *column*\ ?...

  Unsets the values located at one or more *row*, *column* locations.
  *Row* and *column* may be a label, index, or tag.  Both may represent
  more than mulitple rows or columns (example `all`).  When a value
  if unset, the cell is empty.
  
*tableName* **watch column**  *column* ?\ *flags*\ ? *command*

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

*tableName* **watch names** ?\ *pattern*...\ ?

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


.. _`DATATABLE FORMATS`:

DATATABLE FORMATS
=================

Handlers for various datatable formats can be loaded using the TCL
**package** mechanism.  The formats supported are `csv`, `xml`, `sqlite`,
`mysql`, `psql`, `vector`, and `tree`.

**csv**
 To use the CSV handler you must first require the package.

   **package require blt_datatable_xml**

 Then the following **import** and **export** commands become available.

 *tableName* **import csv** ?\ *switches..*\ ?

  Imports the CSV data into the datatable. 
  The following import switches are supported:

  **-file** *fileName*
   Read the CSV file *fileName* to load the table.

  **-data** *string*
   Read the CSV information from *string*.

  **-separator** *char*
   Specifies the separator character.  This is by default the comma.
   If *char* is "auto", then the separator is automatically determined.

  **-escape** *char*
   Load the JSON information into the tree starting at *node*.  The
   default is the root node of the tree.

  **-quote** *char*
   Load the JSON information into the tree starting at *node*.  The
   default is the root node of the tree.

  **-quote** *char*
   Specifies the quote character.  This is by default the double quote.

  **-comment** *char*
   Specifies a comment character.  Any line in the CSV file starting
   with this character is treated as a comment and ignored.

  **-encoding** *string*
   Specifies the encoding to use when reading the CSV file.

  **-maxrows** *numRows*
   Specifies the maximum number of rows to load into the table. 

  **-empty** *string*
   Specifies a string value to use for cells when empty fields
   are found in the CSV data.

  **-headers** *labelList*
   Set the column labels from the list of labels in *labelList*.

  **-autoheaders** *boolean*
   Set the column labels from the first row of the CSV data.  

 *tableName* **export csv** ?\ *switches..*\ ?

  Exports the datatable into CSV data.  If no **-file** switch is provided,
  the CSV output is returned.  The following import switches are supported:

   **-columnlabels** 
    Indicates to create an extra row in the CSV containing the
    column labels.

   **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.  By default
    all columns are exported.

   **-file** *fileName*
    Write the CSV output to the file *fileName*.

   *-quote** *char*
     Specifies the quote character.  This is by default the double quote.

   **-rowlabels** 
    Indicates to create an extra column in the CSV containing the
    row labels.

   **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  By default
    all rows are exported.

   **-separator** *char*
    Specifies the separator character.  This is by default the comma.

**mysql**
 To use the CSV handler you must first require the package.

   **package require blt_datatable_xml**

 Then the following **import** and **export** commands become available.

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
 To use the CSV handler you must first require the package.

   **package require blt_datatable_xml**

 Then the following **import** and **export** commands become available.

 *tableName* **import psql** ?\ *switches..*\ ?

   Imports a table from a *Postgresql* database.  The following switches
   are supported:

   **-db** *dbName*
     Specifies the name of the database.  

   **-host** *hostName*
    Specifies the name or address of the *Postgresql* server host.  

   **-user** *userName*
     Specifies the name of the *Postgresql* user.  By default, the `USER`
     environment variable is used.

   **-password** *password*
     Specifies the password of the *Postgresql* user. 

   **-port** *portNumber*
     Specifies the port number of the *Postgresql* server.

   **-query** *string*
     Specifies the SQL query to make to the *Postgresql* database.

   **-table** *tableName*
     Specifies the name of the *Postgresql* table being queried.

**sqlite**
 To use the CSV handler you must first require the package.

   **package require blt_datatable_xml**

 Then the following **import** and **export** commands become available.

 *tableName* **import sqlite** ?\ *switches*\... ?

   Imports a table from an Sqlite database.  The following export switches are
   supported:

   **-file** *fileName*
     Read from the *Sqlite* file *fileName*.

   **-query** *string*
     Specifies the SQL query to make to the *Sqlite* database.

**tree**
 To use the CSV handler you must first require the package.

   **package require blt_datatable_xml**

 Then the following **import** and **export** commands become available.

 *tableName* **import tree** *treeName* ?\ *switches..*\ ?

   Imports a BLT tree into the table.  *TreeName* is the name of the BLT
   tree. 

   **-depth** *maxDepth*
     Traverse *treeName* a maximum of *maxDepth* levels starting
     from *node*.

   **-inodes** 
     Store the indices of the tree nodes in a column called "inode".

   **-root** *node*
     Specifies the root node of the branch to be imported. By default,
     the root of the tree is the root node.

**vector**
 To use the CSV handler you must first require the package.

   **package require blt_datatable_xml**

 Then the following **import** and **export** commands become available.

 *tableName* **import vector** ?\ *destColumn* *vecName*\ ?...

   Imports a columns from one of more BLT vectors.  *VecName* is the name of
   a BLT vector.  *DestColumn* may be a label, index, or tag, but may not
   represent more than one column.  If *destColumn* does not exist, it is
   automatically created.  All the values previously in *destColumn* are
   deleted.  Rows may added to the table to store the vector values.

**xml**
 To use the CSV handler you must first require the package.

   **package require blt_datatable_xml**

 Then the following **import** and **export** commands become available.

 *tableName* **import xml** ?\ *switches..*\ ?

   Imports XML into the table.  The following export switches are
   supported:

   **-data** *string*
     Read XML from the data *string*.

   **-file** *fileName*
     Read XML from the file *fileName*.

   **-noattrs** 
     Don't import XML attributes into the table.

   **-noelems** 
     Don't import XML elements into the table.

   **-nocdata** 
     Don't import XML character data (CDATA) into the table.

EXAMPLE
=======

KEYWORDS
========

datatable, tableview
