===============
blt::datatable
===============

------------------------------------
Create and manage datatable objects.
------------------------------------

:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::datatable create** ?\ *tableName*\ ?

**blt::datatable destroy** ?\ *tableName* ... ?

**blt::datatable exists** *tableName*

**blt::datatable load** *format* *libPath*

**blt::datatable names** ?\ *pattern* ... ?

DESCRIPTION
-----------

The **datatable** command creates datatable data objects.  A *datatable*
object is table of cells.  Row and columns both have indices and labels
that address the cells.  Cells may contain data or may be empty.

SYNTAX
------

**blt::datatable create** ?\ *tableName*\ ?  
  Creates a new *datatable* object.  If no *tableName* argument is present,
  then the name of the datatable is automatically generated in the form
  "datatable0", "datatable1", etc.  If the substring "#auto" is found in
  *tableName*, it is automatically substituted by a generated name.  For
  example, the name ".foo.#auto.bar" will be translated to
  ".foo.datatable0.bar".  Datatables are by default created in the current
  namespace, not the global namespace, unless *tableName* contains a
  namespace qualifier, such as "fred::myDataTable". The name of the new
  *datatable* is returned.
  
  A new TCL command *tableName* is created that you can use to access and
  manage the *datatable* object.  Another TCL command or *datatable* object can
  not already exist as *tableName*.  The new *datatable* will be empty with
  no rows or columns.  If the TCL command *tableName* is deleted, the
  *datatable* will also be released.

**blt::datatable destroy** ?\ *tableName* ... ?
  Releases one of more datatables.  The TCL command associated with
  *tableName* is also removed.  Datatables are reference counted.  The
  internal datatable data object isn't destroyed until no one else is using
  it.  See the **attach** operation for the details on sharing datatables.

**blt::datatable exists** *tableName*
  Indicates if a datatable named *tableName* already exists.  Returns "1"
  is the datatable exists, "0" otherwise.

**blt::datatable load** *format* *libPath*
  Dynamically loads the named datatable module.  This is used internally
  to load datatable modules for importing and exporting data.

**blt::datatable names** ?\ *pattern* ... ?
  Returns the names of all datatable objects.  If one or more *pattern*
  arguments are provided, then the names of any datatable matching 
  *pattern* will be returned. *Pattern* is a glob-style pattern. 

REFERENCING ROWS AND COLUMNS
----------------------------

Row and columns in a *datatable* object may be referred to by either their
index, label, or tag.

 **index**
   The number of the row or column.  Row and column indices start from 0.
   The index of a row or column may change as rows or columns are added,
   deleted, moved, or sorted.

 **label**
   The label of the row or column.  Each row and column has a label.
   Labels should not be numbers (to distinguish them from indices). Row and
   column labels are distinct.  There can be duplicate row or column
   labels.

 @\ **tag**
   A tag is a string associated with a row or column.  They are a useful for
   referring to groups of rows or columns. Columns and rows can have any
   number of tags associated with them.  To use a tag you prefix it with a
   '@' character. This distinguishes tags from labels.  There are two
   built-in tags: "all" and "end".  Every row and column have the tag
   "all".  The last row and column in the table have the tag "end".  Row
   and column tags are distinct. Tags may be empty (associated with no rows
   or columns).  A tag may refer to multiple rows or columns.
     
If the row or column specifier is an integer then it is assumed to be a row
or column index.  Likewise, if the specifier starts with an "@", it assumed
to be a tag.  You can also distinguish indices, labels and tables by
prefixing them with "index:", "label:", or "tag:".

You can refer to multiple rows or columns in a tag (such as 'all') or
label.  Some datatable commands only operate on a single row or column at a
time; if *tag* or *label* refer to multiple rows or columns, then an error
is generated.

DATATABLE OPERATIONS
--------------------

Once you create a datatable object, you can use its TCL command 
to query or modify it.  The general form of the command is
 
  *tableName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for *datatables* are listed below.

*tableName* **add** *srcTable* ?\ *switches* ... ?
  Adds the rows from *srcTable* to the bottom of *tableName*. *SrcTable* is
  another datatable. Source and destination columns are matched by their
  labels. New columns are automatically created in *tableName* is they
  don't already exist. Column tags are also copied. *Switches* can be any
  of the following:

  **-columns** *columnList*
    Specifies the subset of columns from *srcTable* to add.  *ColumnList*
    is a list of column specifiers. Each specifier may be a column label,
    index, or tag and may refer to multiple columns (example: "all"). By
    default all columns are added.

  **-notags** 
    Don't copy column tags. 

  **-row** *rowList*
    Specifies the subset of rows from *srcTable* to add.  *RowList* is a
    list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are added.
    
*tableName* **append** *row* *column* ?\ *value* ... ?
  Appends one or more values to the current value at *row*, *column* in
  *tableName*.  This is normally used for "string" type cells, but can be
  used for other types as well.  Both *row* and *column* may be a label,
  index, or tag, and may represent more than one row or column.

*tableName* **attach** *anotherTable*
  Attaches to an existing datatable object *anotherTable*.  The underlying
  data (row, columns, cells) of *anotherTable* is shared with *tableName*.
  Tags, traces, and watches are not shared. The current data associated
  with *tableName* is discarded.  It will be destroyed is no one else is
  using it.  The current set of tags, watches, and traces in *tableName*
  are discarded.

*tableName* **column copy** *srcColumn* *destColumn* ?\ *switches* ... ?
  Copies the values and tags from *srcColumn* into *destColumn*.
  *SrcColumn* and *destColumn* may be a column label, index, or tag, but
  may not represent more than one column.  If a column *destColumn* doesn't
  already exist in *tableName*, it is created.  *Switches* can be any of
  the following:

  **-append** 
    Append the values of *srcColumn* to *destColumn*.  By default the
    *destColumn* is overwritten by *srcColumn* (the values in *srcColumn*
    are first removed).

  **-new** 
    Always create a new column *destColumn* even if one already exists in
    *tableName*. The new column may have a duplicate label.

  **-notags** 
    Don't copy column tags. 

  **-table** *srcTable*
    Copy the column *srcColumn* from the datatable *srcTable*.  By default
    to *tableName* is also the source table.

*tableName* **column create** ?\ *switches* ... ?
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
    Specifies the label for the new column.

  **-tags** *tagList*
    Specifies the tags for the new column.

  **-type** *columnType*
    Specifies the type of column. *ColumnType* may be "string", "double",
    "integer", "boolean", "time", or , "blob". See the **column type**
    operation for a description of the different types.

*tableName* **column delete** ?\ *column* ... ?
  Deletes one or more columns from *tableName*. *Column* may be a column
  label, index, or tag and may refer to multiple columns (example: "all").

*tableName* **column duplicate** ?\ *column* ... ?
  Creates duplicate columns for each *column* given.  The column label is
  duplicated and column tags are copied. *Column* may be a column label,
  index, or tag and may refer to multiple columns (example: "all").
  
*tableName* **column empty** *column*
  Returns the row indices of the empty cells in *column*.  *Column*
  may be a label, index, or tag, but may not represent more than one
  column.

*tableName* **column exists** *column*
  Indicates if *column* exists in *tableName*. *Column* may be a label,
  index, or tag, but may not represent more than one column.  Returns "1"
  if the column exists, "0" otherwise.

*tableName* **column extend** *numColumns* ?\ *switches* ... ?
  Extends *tableName* by one of more columns.  *NumColumns* indicates how
  many new columns to add. *Switches* can be any of the following:

  **-labels** *list*
    Specifies the labels for the new columns.  

*tableName* **column get** ?\ *-labels*\ ? *column* ?\ *row* ... ?
  Returns the values from the specified column.  *Column* may be a label,
  index, or tag, but may not represent more than one column.  By default
  all the values of *column* are returned, but if one or more *row*
  arguments are specified, then only the values for specified rows are
  retrieved.  *Row* may be a row label, index, or tag and may not represent
  more than one row.

  This command returns pairs of values and row indices of the selected
  cells. If the *-labels* flag is present, the row label is returned
  instead of the index.

*tableName* **column index** *column* 
  Returns the index of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column.
  
*tableName* **column indices** ?\ *switches* ... ? ?\ *pattern* ... ?
  Returns the indices of the column whose labels match any *pattern*.
  *Pattern* is a glob-style pattern to match.  Matching is done in a
  fashion similar to that TCL **glob** command.  *Switches* can be any of
  the following:

  **-duplicates** 
    Return only the indices of columns with duplicate labels.

*tableName* **column join** *srcTable* ?\ *switches* ... ?
  Copies the columns of *srcTable* into *tableName*. New columns are
  created for each column in *srcTable*. Duplicate column labels may be
  created. Column tags are also copied. *Switches* can be any of the
  following:

  **-columns** *columnList*
    Specifies the subset of columns from *srcTable* to add.  By default all
    columns are added.  *ColumnList* is a list of column specifiers. Each
    specifier may be a column label, index, or tag and may refer to
    multiple columns (example: "all").

  **-notags** 
    Don't copy column tags.
    
  **-row** *rowList*
    Specifies the subset of rows from *srcTable* to add.  *RowList* is a
    list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are added.
    
*tableName* **column label** *column* ?\ *label*\ ?  ?\ *column* *label* ... ?
  Gets or sets the label of the specified column(s).  *Column* may be a
  label, index, or tag, but may not represent more than one column.  If
  *column* is the only argument, then the column label is returned.  If one
  or more *column* and *label* pairs are specified, this command sets the
  labels of the specified columns.
  
*tableName* **column labels** *column* ?\ *labelList*\ ?
  Gets or sets the labels of the specified column.  *Column* may be a
  label, index, or tag, but may not represent more than one column. If a
  *labelList* argument is present, then the column labels are set from the
  list of labels.

*tableName* **column move** *srcColumn* *destColumn* ?\ *numColumns*\ ?
  Move *numColumns* columns in *tableName.  *SrcColumn* and *destColumn* may
  be a label, index, or tag, but may not represent more than one column.
  If a *numColumns* argument isn't specified then only 1 column is moved.
  Moves cannot overlap.
  
*tableName* **column names**  ?\ *pattern* ... ?
  Returns the labels of the columns in *tableName*.  If one of *pattern*
  arguments are present, then the label of any column matching one
  of the patterns is returned. *Pattern* is a glob-style pattern. 

*tableName* **column nonempty**  *column*
  Returns the row indices of the non-empty cells in the column.  *Column*
  may be a label, index, or tag, but may not represent more than one
  column.

*tableName* **column set**  *column* ?\ *row* *value* ... ? 
  Sets values for cells in the specified column. *Column* may be a label,
  index, or tag and may refer to multiple columns (example: "all").  If one
  or more *row* *value* pairs are given then the cell at *row*, *column* is
  set to *value*.  If either *row* or *column* does not exist, the row or
  column is automatically created. If the row or column is an index,
  *tableName* may automatically grow. If the column type is something other
  than *string*, *value* will be converted into the correct type.  If the
  conversion fails, an error will be returned.  See the **column type**
  operation for a description of the different types.

*tableName* **column tag add**  *tag* ?\ *column* ... ? 
  Adds the tag to *column*.  *Tag* is an arbitrary string but can't be one
  of the built-in tags ("all" or "end"). It is not an error if *column*
  already has the tag. If no *column* arguments are present, *tag* is added
  to *tableName* but refers to no columns.  This is useful for creating
  empty column tags.

*tableName* **column tag delete**  *column* ?\ *tag* ... ? 
  Removes one or more tags from *column*.  *Tag* is an arbitrary string but
  can't be one of the built-in tags ("all" or "end"). The built-in tags
  "all" and "end" can't be deleted.

*tableName* **column tag exists**  *tag* ?\ *column* ... ? 
  Indicates if any column in *tableName* has the tag.  *Tag* is an
  arbitrary string.  Returns "1" if the tag exists, "0" otherwise.  By
  default all columns are searched. But if one or more *column* arguments
  are present, then if the tag is found in any *column*, "1" is
  returned. *Column* may be a label, index, or tag and may refer to
  multiple columns (example: "all").

*tableName* **column tag forget**  ?\ *tag* ... ? 
  Remove one or more tags from all the columns in *tableName*. *Tag* is an
  arbitrary string but can't be one of the built-in tags ("all" or "end").

*tableName* **column tag get** *column* ?\ *pattern* ... ? 
  Returns the tags for *column*. *Column* may be a label, index, or tag,
  but may not represent more than one column. By default all tags for
  *column* are returned.  But if one or more *pattern* arguments are
  present, then any tag that matching one of the patterns will be returned.
  *Pattern* is a glob-style pattern.

*tableName* **column tag indices** ?\ *tag* ... ? 
  Returns the indices of columns that have one or more *tag*. *Tag* is an
  arbitrary string.

*tableName* **column tag labels** ?\ *tag* ... ? 
  Returns the column labels that have one or more *tag*. *Tag* is an
  arbitrary string.

*tableName* **column tag names** ?\ *pattern* ... ? 
  Returns the column tags of the table. By default all column tags are
  returned. But if one or more *pattern* arguments are present, then any
  tag matching one of the patterns will be returned. *Pattern* is a
  glob-style pattern.

*tableName* **column tag range** *first* *last* ?\ *tag* ... ? 
  Adds one or more tags the columns in the range given.  *First* and *last*
  may be a column label, index, or tag, but may not represent more than one
  column. *Tag* is an arbitrary string but can't be one of the built-in
  tags ("all" or "end").

*tableName* **column tag set** *column* ?\ *tag* ... ?
  Adds one or more tags to *column*. *Column* may be a column label, index,
  or tag and may refer to multiple columns (example: "all"). *Tag* is an
  arbitrary string but can't be one of the built-in tags ("all" or "end").

*tableName* **column tag unset** *column*  ?\ *tag* ... ?
  Remove one or more tags from *column*. *Column* may be a column label,
  index, or tag and may refer to multiple columns (example: "all").  *Tag*
  is an arbitrary string but can't be one of the built-in tags ("all" or
  "end").

*tableName* **column type**  *column* ?\ *type*\ ? ?\ *column* *type* ... ?
  Gets or sets the type of values for the specified column.  *Column* may
  be a label, index, or tag, but may not represent more than one column.
  If only one *column* argument is present, the current type of *column*
  is returned.  If one or more *column* and *type* pairs are specified,
  then this command sets the column type. *Type* can any of the following:

  *string*
    Values in the column are strings.  

  *double*
    Values in the column are double precision numbers. Each value
    in the column is converted to double precision number.  

  *integer*
    Values in the column are integers.  Each value in the column
    is converted to an integer.

  *boolean*
    Values in the column are booleans.  Acceptable boolean values are
    "0", "false", "no", "off", "1", "true", "yes", or "on". The values
    is converted to 0 (false) or 1 (true).

  *time*
    Values in the column are timestamps.  The timestamps can be in any
    form accepts by the **blt::date** command.  Each value in the column
    is converted to a double precision number representing the time.

  *blob*
    Values in the column are blobs. 

*tableName* **column unset**  *column* ?\ *row* ... ?
  Unsets the cell values of *column*.  *Column* may be a label, index, or
  tag, but may not represent more than one column.  Bu default all cells in
  *column* are unset, but one or more *row* and *value* pairs are
  specified, only those cells at *row*, *column* are unset.  *Row* may be a
  row label, index, or tag and may refer to multiple rows (example: "all").

*tableName* **column values**  *column* ?\ *valueList*\ ?
  Gets or sets the cell values of *column*.  *Column* may be a label,
  index, or tag, but may not represent more than one column.  If no
  *valueList* argument is present, this command returns the values of all
  the cells in *column*.  Otherwise this command sets the cell values of
  *column* from the elements of the list *valueList*.  If there are more
  values in *valueList* than rows in the table, the table is extended.  If
  there are less, the remaining cells remain the same.

*tableName* **copy** *srcTable* 
  Makes a copy of *srcTable in *tableName*.  *SrcTable* is the another
  datatable.  Any datatable data in *tableName* (rows, column, cells, and
  tags) are first removed.

*tableName* **dir** *path* ?\ *switches* ... ?
  Fills *tableName* with the directory listing specified by *path*. If
  *path* is a directory, then its entries are added to the table.
  *Switches* can be any of the following:

  **-fields** *list* 
    Specifies the fields to be collected and written into the table.
    *List* is a TCL list of field names.  Any of the field names
    below may be used.

    **size**
      Collects a decimal string giving the size of file name in bytes.
    **mode**
      Collects a decimal string giving the mode of the file or directory.
      The 12 bits corresponding to the mask 07777 are the file mode bits
      and the least significant 9 bits (0777) are the file permission bits.
    **type**
      Collects the type of the file or directory. The type of file name
      will be one of "file", "directory", "characterSpecial",
      "blockSpecial", "fifo", "link", or "socket".
    **uid**
      Collects the user ID of the owner of the file or directory.
    **gid**
      Collects the group ID of the owner of the file or directory.
    **atime**
      Collects a decimal string giving the time at which file name was
      last accessed.   
    **ctime**
      Collects a decimal string giving the time at which the status of file
      name was changed. Status may be changed by writing or by setting
      inode information (i.e., owner, group, link count, mode, etc.).
    **mtime**
      Collects a decimal string giving the time at which file name was
      last modified.   
    **permissions**
      Collects a decimal string giving the time at which file name was
      last modified.   
    **all**
      Collect the all the above fields.

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
    Only add entries matching *pattern* to the table. *Pattern* is a
    glob-style pattern. 

  **-readable** 
    Add readable file and directory entries to the table.

  **-readonly** 
    Add read-only (not writable) file and directory entries to the table.

  **-writable** 
    Add writable file and directory entries to the table.

*tableName* **dump** ?\ *switches* ... ?
  Converts the contents of *tableName* into a string representation.  This
  includes the row/column labels and tags and cell values. By default all
  rows and columns are dumped. This command returns the string
  representation unless the **-file** switch is set.  *Switches* can be any
  of the following:

  **-column** *columnList*
    Specifies a list of columns from *tableName* to dump. *ColumnList* is a
    list of column specifiers. Each specifier may be a column label, index,
    or tag and may refer to multiple columns (example: "all").
    
  **-file** *fileName*
    Write the contents to the file *fileName*.

  **-rows** *rowList*
    Specifies a list of rows from *tableName* to dump. *RowList* is a
    list of row specifiers. Each specifier may be a row label, index,
    or tag and may refer to multiple columns (example: "all").

*tableName* **duplicate** ?\ *newName*\ ?
  Creates a new datatable that is a duplicate of *tableName*.  If no
  *newName* argument is given the new datatable name is generated.
  Otherwise it will be named *newName*. No TCL command or datatable
  *newName* can already exist.

*tableName* **emptyvalue** ?\ *newValue*\ ?
  Sets or gets the string representing empty cells in the table.  If no
  *newValue* argument is given, this command returns the empty value
  string.  Otherwise this command sets the empty value string to
  *newValue*.  This is string is used in the **column get**, **column
  values**, **get** **sort**, **row get**, and **row values** operations.
  The default is "".

*tableName* **exists** *row* *column*
  Indicates if a cell value exists at *row*, *column* in *tableName*.
  *Row* and *column* may be a label, index, or tag, but may not represent
  more than one row or column. If the cell is empty, then "0" is returned.
  If either *row* or *column* do not exist, "0" is returned.  Otherwise,
  "1" is returned.

*tableName* **export** *dataFormat* ?\ *switches* ... ?
  Exports *tableName* into another format. *DataFormat* is one of the
  different formats are described in the section `DATATABLE FORMATS`_
  below. *Switches* are specific to *dataFormat*.

*tableName* **find** *rowExpr* ?\ *switches* ... ?
  Finds the rows that satisfy *rowExpr*.  *RowExpr* is a TCL expression.
  The expression is evaluated for each row in the table.  The cell values
  in the row can be read via special column variables. Column variable
  names are either the column index or label.  They return the values in
  the cell for that row and column.  Note that if a cell is empty it won't
  have a variable associated with it.  You can use the **-emptyvalue**
  switch to return a known value for empty cells, or you can test for empty
  cells by using the **info exists** TCL command in the expression.

  **-addtag**  *tagName*
    Add *tagName* to each returned row.

  **-emptyvalue**  *string*
    Return *string* for empty cells when evaluating column variables.

  **-invert**  
    Returns rows that where *rowExpr* is false.

  **-maxrows**  *numRows*
    Stop when *numRows* rows have been found.

  **-rows** *rowList*
    Consider only the rows in *rowList*.  *RowList* is a list of
    of row labels, indices, or tags that may refer to multiple rows.

*tableName* **get** *row* *column* ?\ *defValue*\ ?
  Returns the cell value at *row*, *column* in *tableName*.  
  *Row* and *column* may be a label, index, or tag, but may not represent
  more than one row or column. If the cell is empty, then the empty value
  string is returned. By default it's an error if either *row* or *column*
  do not exist but the *defValue* argument lets you return a known value
  instead of generating an error. *DefValue* can be any string.
  
*tableName* **import** *dataFormat* ?\ *switches* ... ?
  Import data into *tableName* from another format. *DataFormat* is one of
  the different formats are described in the section `DATATABLE FORMATS`_
  below. *Switches* are specific to *dataFormat*.

*tableName* **keys** *column* ?\ *column* ... ?

  Generates an internal lookup table from the columns given.  This is
  especially useful when a combination of column values uniquely represent
  rows of the table. *Column* may be a label, index, or tag, but may not
  represent more than one row or column.
  
*tableName* **lappend** *row* *column* ?\ *value* ... ?
  Appends one or more values to the current value at *row*, *column* in
  *tableName*.  Each new value is appended as a list element. Both *row*
  and *column* may be a label, index, or tag, and may represent more than
  one row or column. This command is for "string" cells only.

*tableName* **limits** ?\ *column*\ ?
  Returns the minimum and maximum cell values in *tableName*.  If *column*
  is present, the minimum and maximum cell values in *column* are returned.
  *Column* may be a column label, index, or tag, but may not represent more
  than one column.

*tableName* **lookup** ?\ *value* ... ?
  Searches for the row matching the values keys given.  *Value* is a value
  from the columns specified by the **keys** operation.  The order and number
  of the values must be the same as the columns that were specified in the
  **keys** operation.  If a matching combination is found, the index of the
  row is returned, otherwise "-1".

*tableName* **maximum** ?\ *column*\ ?
  Returns the maximum cell value in the table.  If a *column* argument is
  present, the maximum cell value in *column* is returned.  *Column* may be
  a column label, index, or tag, but may not represent more than one
  column.

*tableName* **minimum** ?\ *column*\ ?
  Returns the minimum cell value in the table.  If a *column* argument is
  present, the maximum cell value in *column* is returned.  *Column* may be
  a column label, index, or tag, but may not represent more than one
  column.

*tableName* **numcolumns** ?\ *numColumns*\ ?
  Sets or gets the number of column in *tableName*.  If no *numColumns*
  argument is present, this command returns the current number of columns
  in *tableName*. Otherwise this command resizes *tableName* to
  *numColumns* number of columns.

*tableName* **numrows** ?\ *numRows*\ ?
  Sets or gets the number of column in *tableName*.  If no *numRows*
  argument is present, this command returns the current number of rows in
  *tableName*. Otherwise this command resizes *tableName* to *numRows*
  number of rows.

*tableName* **restore** ?\ *switches* ... ?
  Restores *tableName* from a previously dumped state (see the **dump**
  operation).  *Switches* can be any of the following:
  
  **-data**  *string*
    Reads the dump information from *string*.

  **-file**  *fileName*
    Reads the dump information from *fileName*.

  **-notags**  
    Ignore row and columns tags found in the dump information.

  **-overwrite**  
    Overwrite any rows or columns.

*tableName* **row copy** *srcRow* *destRow* ?\ *switches* ... ?
  Copies the values and tags from *srcRow* into *destRow*.  *SrcRow* and
  *destRow* may be a row label, index, or tag, but may not represent more
  than one row.  If a row *destRow* doesn't already exist in *tableName*,
  it is created.  *Switches* can be any of the following:

  **-append** 
    Append the values of *srcRow* to *destRow*.  By default the *destRow*
    is overwritten by *srcRow* (the values in *srcRow* are first removed).

  **-new** 
    Always create a new row *destRow* even if one already exists in
    *tableName*. The new row may have a duplicate label.

  **-notags** 
    Don't copy row tags. 

  **-table** *srcTable*
    Copy the row *srcRow* from the datatable *srcTable*.  By default
    to *tableName* is also the source table.

*tableName* **row create** ?\ *switches* ... ?
  Creates a new row in *tableName*. The cells of the new row
  is initially empty. The index of the new row is returned.
  *Switches* can be any of the following:  

  **-after** *row*
    The position of the new row will be after *row*. *Row* may
    be a label, index, or tag, but may not represent more than one
    row.

  **-before** *row*
    The position of the new row will be before *row*. *Row* may
    be a label, index, or tag, but may not represent more than one
    row.

  **-label** *label*
    Specifies the label for the new row.

  **-tags** *tagList*
    Specifies the tags for the new row.

*tableName* **row delete** ?\ *row* ... ?
  Deletes one or more rows from *tableName*. *Row* may be a row
  label, index, or tag and may refer to multiple rows (example: "all").

*tableName* **row duplicate** ?\ *row* ... ?
  Creates duplicate rows for each *row* given.  The row label is
  duplicated and row tags are copied. *Row* may be a row label,
  index, or tag and may refer to multiple rows (example: "all").
  
*tableName* **row empty** *row*
  Returns the column indices of the empty cells in *row*.  *Row*
  may be a label, index, or tag, but may not represent more than one
  row.

*tableName* **row exists** *row*
  Indicates if *row* exists in *tableName*. *Row* may be a label,
  index, or tag, but may not represent more than one row.  Returns "1"
  if the row exists, "0" otherwise.

*tableName* **row extend** *numRows* ?\ *switches* ... ?
  Extends *tableName* by one of more rows.  *NumRows* indicates how
  many new rows to add. *Switches* can be any of the following:

  **-labels** *list*
    Specifies the labels for the new rows.  

*tableName* **row get** ?\ *-labels*\ ? *row* ?\ *column* ... ?
  Returns the values from the specified row.  *Row* may be a label, index,
  or tag, but may not represent more than one row.  By default all the
  values of *row* are returned, but if one or more *column* arguments are
  specified, then only the values for specified columns are retrieved.
  *Column* may be a column label, index, or tag and may not represent more
  than one column.

  This command returns pairs of values and column indices of the selected
  cells. If the *-labels* flag is present, the column label is returned
  instead of the index.

*tableName* **row index** *row* 
  Returns the index of the specified row.  *Row* may be a
  label, index, or tag, but may not represent more than one row.
  
*tableName* **row indices** ?\ *switches* ... ? ?\ *pattern* ... ?
  Returns the indices of the row whose labels match any *pattern*.
  *Pattern* is a glob-style pattern to match.  Matching is done in a
  fashion similar to that TCL **glob** command.  *Switches* can be any of
  the following:

  **-duplicates** 
    Return only the indices of rows with duplicate labels.

*tableName* **row join** *srcTable* ?\ *switches* ... ?
  Copies the rows of *srcTable* into *tableName*. New rows are
  created for each row in *srcTable*. Duplicate row labels may be
  created. Row tags are also copied. *Switches* can be any of the
  following:

  **-rows** *rowList*
    Specifies the subset of rows from *srcTable* to add.  *RowList* is a
    list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are added.

  **-notags** 
    Don't copy row tags.
    
  **-column** *columnList*
    Specifies the subset of columns from *srcTable* to add.  *ColumnList*
    is a list of column specifiers. Each specifier may be a column label,
    index, or tag and may refer to multiple columns (example: "all"). By
    default all columns are added.
    
*tableName* **row label** *row* ?\ *label*\ ?  ?\ *row* *label* ... ?
  Gets or sets the label of the specified row(s).  *Row* may be a label,
  index, or tag, but may not represent more than one row.  If *row* is the
  only argument, then the row label is returned.  If one or more *row* and
  *label* pairs are specified, this command sets the labels of the
  specified rows.
  
*tableName* **row labels** *row* ?\ *labelList*\ ?
  Gets or sets the labels of the specified row.  *Row* may be a label,
  index, or tag, but may not represent more than one row. If a *labelList*
  argument is present, then the row labels are set from the list of labels.

*tableName* **row move** *srcRow* *destRow* ?\ *numRows*\ ?
  Move *numRows* rows in *tableName.  *SrcRow* and *destRow* may be a
  label, index, or tag, but may not represent more than one row.  If a
  *numRows* argument isn't specified then only 1 row is moved.  Moves
  cannot overlap.
  
*tableName* **row names**  ?\ *pattern* ... ?
  Returns the labels of the rows in *tableName*.  If one of *pattern*
  arguments are present, then the label of any row matching one of the
  patterns is returned. *Pattern* is a glob-style pattern.

*tableName* **row nonempty**  *row*
  Returns the column indices of the non-empty cells in the row.  *Row* may
  be a label, index, or tag, but may not represent more than one row.

*tableName* **row set**  *row* ?\ *column* *value* ... ? 
  Sets values for cells in the specified row. *Row* may be a label, index,
  or tag and may refer to multiple rows (example: "all").  If one or more
  *column* *value* pairs are given then the cell at *column*, *row* is set
  to *value*.  If either *column* or *row* does not exist, the column or
  row is automatically created. If the column or row is an index,
  *tableName* may automatically grow. If the column type is something other
  than *string*, *value* will be converted into the correct type.  If the
  conversion fails, an error will be returned.  See the **column type**
  operation for a description of the different types.

*tableName* **row tag add**  *tag* ?\ *row* ... ? 
  Adds the tag to *row*.  *Tag* is an arbitrary string but can't be one of
  the built-in tags ("all" or "end"). It is not an error if *row* already
  has the tag. If no *row* arguments are present, *tag* is added to
  *tableName* but refers to no rows.  This is useful for creating empty row
  tags.

*tableName* **row tag delete**  *row* ?\ *tag* ... ? 
  Removes one or more tags from *row*.  *Tag* is an arbitrary string but
  can't be one of the built-in tags ("all" or "end"). The built-in tags
  "all" and "end" can't be deleted.

*tableName* **row tag exists**  *tag* ?\ *row* ... ? 
  Indicates if any row in *tableName* has the tag.  *Tag* is an arbitrary
  string.  Returns "1" if the tag exists, "0" otherwise.  By default all
  rows are searched. But if one or more *row* arguments are present, then
  if the tag is found in any *row*, "1" is returned. *Row* may be a label,
  index, or tag and may refer to multiple rows (example: "all").

*tableName* **row tag forget**  ?\ *tag* ... ? 
  Remove one or more tags from all the rows in *tableName*. *Tag* is an
  arbitrary string but can't be one of the built-in tags ("all" or "end").

*tableName* **row tag get** *row* ?\ *pattern* ... ? 
  Returns the tags for *row*. *Row* may be a label, index, or tag, but may
  not represent more than one row. By default all tags for *row* are
  returned.  But if one or more *pattern* arguments are present, then any
  tag that matching one of the patterns will be returned.  *Pattern* is a
  glob-style pattern.

*tableName* **row tag indices** ?\ *tag* ... ? 
  Returns the indices of rows that have one or more *tag*. *Tag* is an
  arbitrary string.

*tableName* **row tag labels** ?\ *tag* ... ? 
  Returns the row labels that have one or more *tag*. *Tag* is an arbitrary
  string.

*tableName* **row tag names** ?\ *pattern* ... ? 
  Returns the row tags of the table. By default all row tags are
  returned. But if one or more *pattern* arguments are present, then any
  tag matching one of the patterns will be returned. *Pattern* is a
  glob-style pattern.

*tableName* **row tag range** *first* *last* ?\ *tag* ... ? 
  Adds one or more tags the rows in the range given.  *First* and *last*
  may be a row label, index, or tag, but may not represent more than one
  row. *Tag* is an arbitrary string but can't be one of the built-in tags
  ("all" or "end").

*tableName* **row tag set** *row* ?\ *tag* ... ?
  Adds one or more tags to *row*. *Row* may be a row label, index, or tag
  and may refer to multiple rows (example: "all"). *Tag* is an arbitrary
  string but can't be one of the built-in tags ("all" or "end").

*tableName* **row tag unset** *row* \? *tag* ... ?
  Remove one or more tags from *row*. *Row* may be a row label, index, or
  tag and may refer to multiple rows (example: "all").  *Tag* is an
  arbitrary string but can't be one of the built-in tags ("all" or "end").

*tableName* **row unset**  *row* ?\ *column* ... ?
  Unsets the cell values of *row*.  *Row* may be a label, index, or tag,
  but may not represent more than one row.  Bu default all cells in *row*
  are unset, but one or more *column* and *value* pairs are specified, only
  those cells at *column*, *row* are unset.  *Column* may be a column
  label, index, or tag and may refer to multiple columns (example: "all").

*tableName* **row values**  *row* ?\ *valueList*\ ?
  Gets or sets the cell values of *row*.  *Row* may be a label, index, or
  tag, but may not represent more than one row.  If no *valueList* argument
  is present, this command returns the values of all the cells in *row*.
  Otherwise this command sets the cell values of *row* from the elements of
  the list *valueList*.  If there are more values in *valueList* than
  columns in the table, the table is extended.  If there are less, the
  remaining cells remain the same.

*tableName* **set** *row* *column* *value* 
  Sets the value at *row*, *column* in *tableName*.  *Row* and *column* may
  be a label, index, or tag and may refer to multiple rows (example:
  "all"). If either *row* or *column* is and index or label and does not
  already exist, the row or column is automatically created.  If the row or
  column is an index, *tableName* may automatically grow. *Value* is the
  value to be set.  If the column type is not *string*, *value* is
  converted into the correct type.  If the conversion fails, an error will
  be returned.

*tableName* **sort** ?\ *switches* ... ?
  Sorts the table.  Column are compared in order. The type comparison is
  determined from the column type.  But you can use **-ascii** or
  **-dictionary** switch to sort the rows.  If the **-list**,
  **-nonempty**, **-unique**, or **-values** switches are present, a list
  of the sorted rows is returned instead of rearranging the rows in the
  table. *Switches* can be one of the following:

  **-ascii**
    Uses string comparison with Unicode code-point collation order (the name
    is for backward-compatibility reasons.)  The string representation of
    the values are compared.   

  **-columns** *columnList*
    Compares the cells in order of the columns in *columnList*.
    *ColumnList* is a list of column specifiers. Each specifier may be a
    column label, index, or tag and may refer to multiple columns (example:
    "all"). By default all columns are compared in their order in the
    datatable.

  **-decreasing** 
    Sorts rows highest to lowest. By default rows are sorted lowest to
    highest.

  **-dictionary** 
    Uses dictionary-style comparison. This is the same as **-ascii**
    except (a) case is ignored except as a tie-breaker and (b) if two
    strings contain embedded numbers, the numbers compare as integers, not
    characters.  For example, in **-dictionary** mode, "bigBoy" sorts
    between "bigbang" and "bigboy", and "x10y" sorts between "x9y" and
    "x11y".

  **-frequency** 
    Sorts rows according to the frequency of their values.  The rows
    of *tableName* will not be rearranged.  A list of the row
    indices will be returned instead.

  **-list** 
    Returns a list of the sorted rows instead of rearranging the rows
    in the table.  The rows of *tableName* will not be
    rearranged.  This switch is implied when the **-frequency**,
    **-nonempty**, **-unique**, or **-values** switches are used.

  **-nocase** 
    Ignores case when comparing values.  This only has affect when the
    **-ascii** switch is present.

  **-nonempty** 
    Sorts only non-empty cells. The rows of *tableName* will not be
    rearranged.  A list of the row indices will be returned instead.

  **-rows** *rowList*
    Consider only the rows in *rowList*.  *RowList* is a list of
    of row labels, indices, or tags that may refer to multiple rows.
    The list of rows will be returned.

  **-unique** 
    Returns a list of the unique rows.  The rows of *tableName* will not be
    rearranged.  A list of the row indices will be returned instead.

  **-values** 
    Returns the row values instead of their indices.  The rows of
    *tableName* will not be rearranged.  A list of the row values
    will be returned instead.

*tableName* **trace cell** *row* *column* *ops* *cmdPrefix*
  Registers a command to be invoked when the cell (designated by *row* and
  *column*) value is read, written, or unset. *Row* and *column* may be a
  label, index, or tag and may refer to multiple rows (example: "all").
  *Ops* indicates which operations are of interest, and consists of one or
  more of the following letters:

  **r**
    Invoke *cmdPrefix* whenever the cell value is read. 
  **w**
    Invoke *cmdPrefix* whenever the cell value is written.  
  **c**
    Invoke *cmdPrefix* whenever the cell value is created.  This happens
    when the cell was previously empty.
  **u** 
    Invoke *cmdPrefix* whenever the cell value is unset.  

  *CmdPrefix* is a TCL command prefix.  The traced row index, column index
  and the operation letter are appended to the command before it is
  invoked.

*tableName* **trace column** *column* *ops* *cmdPrefix*
  Registers a command to be invoked any cell in *column* is read,
  written, or unset. *Column* may be a label, index, or tag and may refer
  to multiple columns (example: "all").  *Ops* indicates which operations
  are of interest, and consists of one or more of the following letters:

  **r**
    Invoke *cmdPrefix* whenever the cell value is read. 
  **w**
    Invoke *cmdPrefix* whenever the cell value is written.  
  **c**
    Invoke *cmdPrefix* whenever the cell value is created.  This happens
    when the cell was previously empty.
  **u** 
    Invoke *cmdPrefix* whenever the cell value is unset.  

  *CmdPrefix* is a TCL command prefix.  The traced row index, column index
  and the operation letter are appended to the command before it is
  invoked.

*tableName* **trace delete** *traceName*...
  Removes the one of more traces from *tableName*. *TraceName* is
  the name of a trace created by the **trace cell**, **trace column**,
  or **trace row** operations.

*tableName* **trace info** *traceName*
  Describes *traceName*.  A list of the trace's *name*, *row*, *column*,
  *flags*, and *command* and their values is returned.
  
*tableName* **trace names** ?\ *pattern* ... ?
  Returns the names of the traces currently registered. This includes cell,
  row, and column traces.  If one or more of *pattern* arguments are present
  then any trace name matching one of the patterns is returned. *Pattern*
  is a glob-style pattern.
   
*tableName* **trace row** *row* *how* *cmdPrefix*
  Registers a command when any cell in *row* is read, written, or
  unset. *Row* may be a label, index, or tag and may refer to multiple rows
  (example: "all").  *Ops* indicates which operations are of interest, and
  consists of one or more of the following letters:

  **r**
    Invoke *cmdPrefix* whenever the cell value is read. 
  **w**
    Invoke *cmdPrefix* whenever the cell value is written.  
  **c**
    Invoke *cmdPrefix* whenever the cell value is created.  This happens
    when the cell was previously empty.
  **u** 
    Invoke *cmdPrefix* whenever the cell value is unset.  

  *CmdPrefix* is a TCL command prefix.  The traced row index, column index
  and the operation letter are appended to the command before it is
  invoked.

*tableName* **unset** *row* *column* ?\ *row* *column* ... ?
  Unsets the values located at one or more *row*, *column* locations.
  *Row* and *column* may be a label, index, or tag and may refer
  to multiple rows or columns (example "all").  When a value
  is unset the cell becomes empty.
  
*tableName* **watch column**  *column* ?\ *flags* ... ? *cmdPrefix*
  Registers a command to be invoked when an event occurs on a column of
  *tableName*. The events include when columns are added, deleted, moved or
  relabeled.  *Column* may be a label, index, or tag and may refer to
  multiple columns (example: "all").  *Flags* indicates which events are of
  interest. They are described below.

  **-allevents** 
    Watch when columns are created, deleted, moved, or relabeled.

  **-create** 
    Watch when columns are created.

  **-delete** 
    Watch when columns are deleted.

  **-move** 
    Watch when columns are moved.  This includes when the table is sorted.

  **-relabel** 
    Watch when columns are relabeled.

  **-whenidle** 
    Don't trigger the callback immediately.  Wait until the next idle time.

  *CmdPrefix* is a TCL command prefix.  The name of the event and column index
  are appended to the command before it is invoked.

*tableName* **watch delete** *watchName*...
  Removes the one of more watches from *tableName*. *WatchName* is
  the name of a trace created by the  **watch column** or **watch row**
  operations.

*tableName* **watch info** ?\ *watchName*\ ?
  Describes *watchName*.  A list of the watch's name, one or more event
  flags, and the row or column index is returned.

*tableName* **watch names** ?\ *pattern* ... ?
  Returns the names of the watches currently registered. This includes both
  row and column watches.  If one or more of *pattern* arguments are present
  then any watch name matching one of the patterns is returned. *Pattern*
  is a glob-style pattern.
   
*tableName* **watch row**  *row* ?\ *flags*\ ? *cmdPrefix*
  Registers a command to be invoked when an event occurs on a row of
  *tableName*. The events include when rows are added, deleted, moved or
  relabeled.  *Row* may be a label, index, or tag and may refer to
  multiple rows (example: "all").  *Flags* indicates which events are of
  interest. They are described below.

  **-allevents** 
    Watch when rows are created, deleted, moved, or relabeled.

  **-create** 
    Watch when rows are created.

  **-delete** 
    Watch when rows are deleted.

  **-move** 
    Watch when rows are moved.  

  **-relabel** 
    Watch when rows are relabeled.

  **-whenidle** 
    Don't trigger the callback immediately.  Wait until the next idle time.

  *CmdPrefix* is a TCL command prefix.  The name of the event and column index
  are appended to the command before it is invoked.

DATATABLE FORMATS
-----------------

Datatables can import and export their data into various formats.  They are
loaded using the TCL **package** mechanism. Normally this is done
automatically for you when you invoke an **import** or **export** operation
on a datatable.

The available formats are "csv", "xml", "sqlite", "mysql", "psql",
"vector", and "tree" and are described below.

**csv**
~~~~~~~

The *csv* module reads and writes comma separated values (CSV) data.  The
package can be manually loaded as follows.

  **package require blt_datatable_csv**

By default this package is automatically loaded when you use the *csv*
format in the **import** or **export** operations.

*tableName* **import csv** ?\ *switches* ... ?
  Imports the CSV data into the datatable. The following import switches
  are supported.  One of the **-file** or **-data** switches must be
  specified, but not both.

  **-autoheaders** 
    Set the column labels from the first row of the CSV data.  

  **-columnlabels** *labelList*
    Set the column labels from the list of labels in *labelList*.

  **-comment** *commChar*
   Specifies a comment character.  Any line in the CSV file starting
   with this character is treated as a comment and ignored.  By default
   the comment character is "", indicating no comments.

  **-data** *string*
    Read the CSV data from *string*.

  **-emptyvalue** *string*
    Specifies a string value to use for cells when empty fields
    are found in the CSV data.

  **-encoding**  *encodingName*
    Specifies the encoding of the CSV data.  

  **-headers** *labelList*
    Specifies the column labels from the list of labels in *labelList*.

  **-file** *fileName*
    Read the CSV data from *fileName*. If *fileName* starts with an '@'
    character, then what follows is the name of a TCL channel,
    instead of a file name.

  **-maxrows** *numRows*
    Specifies the maximum number of rows to load into the table. 

  **-possibleseparators**  *string*
    Specifies a string of chararacters to test as possible separators for
    the data. It *string* is "", then default set of characters is used.
    The default is ",\t|;".

  **-quote** *quoteChar*
    Specifies the quote character.  This is by default the double quote (")
    character.

  **-separator** *sepChar*
    Specifies the separator character.  By default this is the comma (,)
    character. If *char* is "auto", then the separator is automatically
    determined.

*tableName* **export csv** ?\ *switches* ... ?
  Exports the datatable into CSV data.  If no **-file** switch is provided,
  the CSV output is returned as the result of the command.  The following
  import switches are supported:

  **-columnlabels** 
    Indicates to create an extra row in the CSV containing the
    column labels.

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.
    *ColumnList* is a list of column specifiers. Each specifier may be a
    column label, index, or tag and may refer to multiple columns (example:
    "all"). By default all columns are exported.

  **-file** *fileName*
    Write the CSV output to the file *fileName*.  If *fileName* starts with
    an '@' character, then what follows is the name of a TCL channel,
    instead of a file name.

  **-quote** *quoteChar*
    Specifies the quote character.  This is by default the double quote (")
    character.

  **-rowlabels** 
    Indicates to create an extra column in the CSV containing the
    row labels.

  **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  *RowList* is a
    list of row specifiers. Each specifier may be a row label, index, or tag
    and may refer to multiple row (example: "all").  By default all rows are
    exported.

  **-separator** *sepChar*
    Specifies the separator character.  This is by default the comma (,)
    character.

**mysql**
~~~~~~~~~

The *mysql* module reads and writes tables a MySql database.
The package can be manually loaded as follows.

  **package require blt_datatable_mysql**

By default this package is automatically loaded when you use the *mysql*
format in the **import** or **export** operations.

*tableName* **import mysql** ?\ *switches* ... ?
  Imports a table from a *Mysql* database.  The **-db**, **-host**,
  **-password**, **-port** and **-query** switches are required.
  The following switches
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

*tableName* **export mysql** ?\ *switches* ... ?
  Exports *tableName* to a *Mysql* database.  The **-db**, **-host**,
  **-password**, **-port** and **-query** switches are required.
  The following switches are supported:

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.  *ColumnList*
    is a list of column specifiers. Each specifier may be a column label,
    index, or tag and may refer to multiple columns (example: "all"). By
    default all columns are exported.

  **-db** *dbName*
    Specifies the name of the database.  

  **-host** *hostName*
    Specifies the name or address of the *Mysql* server host.  

  **-password** *password*
    Specifies the password of the *Mysql* user. 

  **-port** *portNumber*
    Specifies the port number of the *Mysql* server.

  **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  *RowList* is a
    list of row specifiers. Each specifier may be a row label, index, or tag
    and may refer to multiple row (example: "all").  By default all rows are
    exported.

  **-table** *tableName*
    Specifies the name of the table.

  **-user** *userName*
    Specifies the name of the *Mysql* user.  By default, the USER
    environment variable is used.

**psql**
~~~~~~~~

The *psql* module reads and writes tables from a *Postgresql* database.
The package can be manually loaded as follows.

   **package require blt_datatable_psql**

By default this package is automatically loaded when you use the *psql*
format in the **import** or **export** operations.

*tableName* **import psql** ?\ *switches* ... ?
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

*tableName* **export psql** ?\ *switches* ... ?
  Exports *tableName* to a *Postgresql* database.  The **-db**, **-host**,
  **-password**, **-port** and **-table** switches are required. The
  following switches are supported:

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.  *ColumnList*
    is a list of column specifiers. Each specifier may be a column label,
    index, or tag and may refer to multiple columns (example: "all"). By
    default all columns are exported.

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
    Specifies the subset of rows from *tableName* to export.  *RowList* is a
    list of row specifiers. Each specifier may be a row label, index, or tag
    and may refer to multiple row (example: "all").  By default all rows are
    exported.

  **-table** *tableName*
    Specifies the name of the *Postgresql* table being written.

**sqlite**
~~~~~~~~~~

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

*tableName* **export sqlist** ?\ *switches* ... ?
  Exports the datatable into *Sqlite* data.  The **-file** switch is
  required. The following import switches are supported:

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.  *ColumnList*
    is a list of column specifiers. Each specifier may be a column label,
    index, or tag and may refer to multiple columns (example: "all"). By
    default all columns are exported.

  **-file** *fileName*
    Write the *Sqlite* output to the file *fileName*.

  **-rowlabels** 
    Export the row labels from *tableName* as an extra column "_rowId" in
    the *Sqlite* table.

  **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  *RowList* is a
    list of row specifiers. Each specifier may be a row label, index, or tag
    and may refer to multiple row (example: "all").  By default all rows are
    exported.

  **-table** *tableName*
    Name of the *Sqlite* table to write to.  If a *tableName* already
    exists, it is overwritten.

**tree**
~~~~~~~~

The *tree* module reads and writes BLT trees.  The package can be manually
loaded as follows.

   **package require blt_datatable_tree**

By default this package is automatically loaded when you use the *tree*
format in the **import** or **export** operations.

*tableName* **import tree** *treeName* ?\ *switches* ... ?
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

*tableName* **export tree** *treeName* ?\ *switches* ... ?
  Exports the datatable into a BLT tree.  *TreeName* is the name of the
  BLT tree.

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.
    *ColumnList* is a list of column specifiers. Each specifier may be a
    column label, index, or tag and may refer to multiple columns (example:
    "all"). By default all columns are exported.

  **-root** *node*
    Specifies the root node of the branch where the datatable is to be
    exported. By default the root of the tree is the root node.

  **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  *RowList* is
    a list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are exported.

**vector**
~~~~~~~~~~

The *vector* module reads and writes BLT vectors.  The package
can be manually loaded as follows.

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
~~~~~~~

The *xml* module reads and writes XML data.  The package can be manually
loaded as follows.

   **package require blt_datatable_xml**

By default this package is automatically loaded when you use the *xml*
format in the **import** or **export** operations.

*tableName* **import xml** ?\ *switches* ... ?
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

*tableName* **export xml** ?\ *switches* ... ?
  Exports the datatable into XML data.  If no **-file** switch is provided,
  the XML output is returned as the result of the command.  The following
  import switches are supported:

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.
    *ColumnList* is a list of column specifiers. Each specifier may be a
    column label, index, or tag and may refer to multiple columns (example:
    "all"). By default all columns are exported.

  **-file** *fileName*
    Write the XML output to the file *fileName*.

  **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  *RowList* is a
    list of row specifiers. Each specifier may be a row label, index, or tag
    and may refer to multiple row (example: "all").  By default all rows are
    exported.

EXAMPLE
-------

  ::

     set t [blt::datatable create]
     $t import csv -file myData.csv
     set labels [$t get row 0]
     $t column labels $labels
     $t row delete 0
     $t column type all double

     $t set 0 0 1.2
   
KEYWORDS
--------

datatable, tableview

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
