===============
blt::datatable
===============

------------------------------------
Create and manage datatable objects.
------------------------------------

.. include:: man.rst
.. include:: toc.rst

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
  *pattern* will be returned. *Pattern* is a **glob**\-style pattern. 
  Matching is done in a fashion similar to that used by the TCL **glob**
  command.

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
time; if a tag or label refer to multiple rows or columns, then an error
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
    
*tableName* **append** *rowName* *columnName* ?\ *value* ... ?
  Appends one or more values to the current value at *rowName*,
  *columnName* in *tableName*.  This is normally used for "string" type
  cells, but can be used for other types as well.  Both *rowName* and
  *columnName* may be a label, index, or tag, and may represent more than
  one row or column.

*tableName* **attach** *anotherTable*
  Attaches to an existing datatable object *anotherTable*.  The underlying
  data (row, columns, cells) of *anotherTable* is shared with *tableName*.
  Tags, traces, and watches are not shared. The current data associated
  with *tableName* is discarded.  It will be destroyed is no one else is
  using it.  The current set of tags, watches, and traces in *tableName*
  are discarded.

*tableName* **column copy** *destColumn* *srcColumn* ?\ *switches* ... ?
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
    *tableName* is the source table.

*tableName* **column create** ?\ *switches* ... ?
  Creates a new column in *tableName*. The cells of the new column
  is initially empty. The index of the new column is returned.
  *Switches* can be any of the following:  

  **-after** *columnName*
    The position of the new column will be after *columnName*. *ColumnName*
    may be a label, index, or tag, but may not represent more than one
    column.

  **-before** *columnName*
    The position of the new column will be before
    *columnName*. *ColumnName* may be a label, index, or tag, but may not
    represent more than one column.

  **-label** *label*
    Specifies the label for the new column.

  **-tags** *tagList*
    Specifies the tags for the new column.

  **-type** *columnType*
    Specifies the type of column. *ColumnType* may be "string", "double",
    "integer", "boolean", "time", or , "blob". See the **column type**
    operation for a description of the different types.

*tableName* **column delete** ?\ *columnName* ... ?
  Deletes one or more columns from *tableName*. *ColumnName* may be a
  column label, index, or tag and may refer to multiple columns (example:
  "all").

*tableName* **column duplicate** ?\ *columnName* ... ?
  Creates duplicate columns for each *columnName* given.  The column label
  is duplicated and column tags are copied. *ColumnName* may be a column
  label, index, or tag and may refer to multiple columns (example: "all").
  
*tableName* **column empty** *columnName*
  Returns the row indices of the empty cells in *columnName*.  *ColumnName*
  may be a label, index, or tag, but may not represent more than one
  column.

*tableName* **column exists** *columnName*
  Indicates if *columnName* exists in *tableName*. *ColumnName* may be a
  label, index, or tag, but may not represent more than one column.
  Returns "1" if the column exists, "0" otherwise.

*tableName* **column extend** *numColumns* ?\ *switches* ... ?
  Extends *tableName* by one of more columns.  *NumColumns* indicates how
  many new columns to add. *Switches* can be any of the following:

  **-labels** *list*
    Specifies the labels for the new columns.  

*tableName* **column get** ?\ *-labels*\ ? *columnName* ?\ *rowName* ... ?
  Returns the values from the specified column.  *ColumnName* may be a
  label, index, or tag, but may not represent more than one column.  By
  default all the values of *columnName* are returned, but if one or more
  *rowName* arguments are specified, then only the values for specified
  rows are retrieved.  *RowName* may be a row label, index, or tag and may
  not represent more than one row.

  This command returns pairs of values and row indices of the selected
  cells. If the *-labels* flag is present, the row label is returned
  instead of the index.

*tableName* **column index** *columnName* 
  Returns the index of the specified column.  *ColumnName* may be a
  label, index, or tag, but may not represent more than one column.
  
*tableName* **column indices** ?\ *switches* ... ? ?\ *pattern* ... ?
  Returns the indices of the column whose labels match any *pattern*.
  *Pattern* is a **glob**\-style pattern to match.  Matching is done in a
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
    
*tableName* **column label** *columnName* ?\ *label*\ ?  ?\ *columnName* *label* ... ?
  Gets or sets the label of the specified column(s).  *ColumnName* may be a
  label, index, or tag, but may not represent more than one column.  If
  *columnName* is the only argument, then the column label is returned.  If
  one or more *columnName* and *label* pairs are specified, this command
  sets the labels of the specified columns.
  
*tableName* **column labels** *columnName* ?\ *labelList*\ ?
  Gets or sets the labels of the specified column.  *ColumnName* may be a
  label, index, or tag, but may not represent more than one column. If a
  *labelList* argument is present, then the column labels are set from the
  list of labels.

*tableName* **column move** *destColumn* *firstColumn* *lastColumn* ?\ *switches* ... ?
  Move one or more columns in *tableName.  *DestColumn*, *firstColumn*, and
  *lastColumn* are columns in *tableName*.  Each may be a label, index, or
  tag, but may not represent more than one column.  *FirstColumn and
  *lastColumn* designate a range of columns to move.  To move one column,
  specify *firstColumn* and *lastColumn* as the same column.  *DestColumn*
  is the destination location where to move the columns.  It can not be in
  the designated range of columns.  The following *switches* are available.

  **-after** 
    Specifies that the destination location is the column after *destColumn*.
    The default is to position the columns before *destColumn*.  This allows
    you to move columns to the end.

*tableName* **column names**  ?\ *pattern* ... ?
  Returns the labels of the columns in *tableName*.  If one of *pattern*
  arguments are present, then the label of any column matching one
  of the patterns is returned. *Pattern* is a **glob**\-style pattern. 
  Matching is done in a fashion similar to that used by the TCL **glob**
  command.

*tableName* **column nonempty**  *columnName*
  Returns the row indices of the non-empty cells in the column.
  *ColumnName* may be a label, index, or tag, but may not represent more
  than one column.

*tableName* **column set**  *columnName* ?\ *rowName* *value* ... ? 
  Sets values for cells in the specified column. *ColumnName* may be a
  label, index, or tag and may refer to multiple columns (example: "all").
  If one or more *rowName* *value* pairs are given then the cell at
  *rowName*, *columnName* is set to *value*.  If either *rowName* or
  *columnName* does not exist, the row or column is automatically
  created. If the row or column is an index, *tableName* may automatically
  grow. If the column type is something other than **string**, *value* will
  be converted into the correct type.  If the conversion fails, an error
  will be returned.  See the **column type** operation for a description of
  the different types.

*tableName* **column tag add**  *tagName* ?\ *columnName* ... ? 
  Adds the tag to *columnName*.  *TagName* is an arbitrary string but can't
  be one of the built-in tags ("all" or "end"). It is not an error if
  *columnName* already has the tag. If no *columnName* arguments are
  present, *tagName* is added to *tableName* but refers to no columns.
  This is useful for creating empty column tags.

*tableName* **column tag delete**  *columnName* ?\ *tagName* ... ? 
  Removes one or more tags from *columnName*.  *TagName* is an arbitrary
  string but can't be one of the built-in tags ("all" or "end"). The
  built-in tags "all" and "end" can't be deleted.

*tableName* **column tag exists**  *tagName* ?\ *columnName* ... ? 
  Indicates if any column in *tableName* has the tag.  *TagName* is an
  arbitrary string.  Returns "1" if the tag exists, "0" otherwise.  By
  default all columns are searched. But if one or more *columnName*
  arguments are present, then if the tag is found in any *columnName*, "1"
  is returned. *ColumnName* may be a label, index, or tag and may refer to
  multiple columns (example: "all").

*tableName* **column tag forget**  ?\ *tagName* ... ? 
  Remove one or more tags from all the columns in *tableName*. *TagName* is
  a column tag, but can't be one of the built-in tags ("all" or "end").

*tableName* **column tag get** *columnName* ?\ *pattern* ... ? 
  Returns the tags for *columnName*. *ColumnName* may be a label, index, or
  tag, but may not represent more than one column. By default all tags for
  *columnName* are returned.  But if one or more *pattern* arguments are
  present, then any tag that matching one of the patterns will be returned.
  *Pattern* is a **glob**\-style pattern.  Matching is done in a fashion
  similar to that used by the TCL **glob** command.

*tableName* **column tag indices** ?\ *tagName* ... ? 
  Returns the indices of columns that have one or more tags. *TagName* is
  an is a tag string.

*tableName* **column tag labels** ?\ *tagName* ... ? 
  Returns the column labels that have one or more tags. *TagName* is a 
  tag string.

*tableName* **column tag names** ?\ *pattern* ... ? 
  Returns the column tags of the table. By default all column tags are
  returned. But if one or more *pattern* arguments are present, then any
  tag matching one of the patterns will be returned. *Pattern* is a
  **glob**\-style pattern.  Matching is done in a fashion similar to that
  used by the TCL **glob** command.

*tableName* **column tag range** *firstColumn* *lastColumn* ?\ *tagName* ... ? 
  Adds one or more tags the columns in the range given.  *FirstColumn* and
  *lastColumn* may be a column label, index, or tag, but may not represent
  more than one column. *TagName* is an arbitrary string but can't be one
  of the built-in tags ("all" or "end").

*tableName* **column tag set** *columnName* ?\ *tagName* ... ?
  Adds one or more tags to *columnName*. *ColumnName* may be a column
  label, index, or tag and may refer to multiple columns (example:
  "all"). *TagName* is an arbitrary string but can't be one of the built-in
  tags ("all" or "end").

*tableName* **column tag unset** *columnName*  ?\ *tagName* ... ?
  Remove one or more tags from *columnName*. *ColumnName* may be a column
  label, index, or tag and may refer to multiple columns (example: "all").
  *TagName* is an arbitrary string but can't be one of the built-in tags
  ("all" or "end").

*tableName* **column type**  *columnName* ?\ *typeName*\ ? ?\ *columnName* *typeName* ... ?
  Gets or sets the type of values for the specified column.  *ColumnName*
  may be a label, index, or tag, but may not represent more than one
  column.  If only one *columnName* argument is present, the current type
  of *columnName* is returned.  If one or more *columnName* and *type*
  pairs are specified, then this command sets the column type. *Type* can
  any of the following:

  **string**
    Values in the column are strings.  

  **double**
    Values in the column are double precision numbers. Each value
    in the column is converted to double precision number.  

  **integer**
    Values in the column are integers.  Each value in the column
    is converted to an integer.

  **boolean**
    Values in the column are booleans.  Acceptable boolean values are
    "0", "false", "no", "off", "1", "true", "yes", or "on". The values
    is converted to 0 (false) or 1 (true).

  **time**
    Values in the column are timestamps.  The timestamps can be in any
    form accepts by the **blt::date** command.  Each value in the column
    is converted to a double precision number representing the time.

  **blob**
    Values in the column are blobs. 

*tableName* **column unset**  *columnName* ?\ *rowName* ... ?
  Unsets the cell values of *columnName*.  *ColumnName* may be a label,
  index, or tag, but may not represent more than one column.  Bu default
  all cells in *columnName* are unset, but one or more *rowName* and
  *value* pairs are specified, only those cells at *rowName*, *columnName*
  are unset.  *RowName* may be a row label, index, or tag and may refer to
  multiple rows (example: "all").

*tableName* **column values**  *columnName* ?\ *valueList*\ ?
  Gets or sets the cell values of *columnName*.  *ColumnName* may be a
  label, index, or tag, but may not represent more than one column.  If no
  *valueList* argument is present, this command returns the values of all
  the cells in *columnName*.  Otherwise this command sets the cell values
  of *columnName* from the elements of the list *valueList*.  If there are
  more values in *valueList* than rows in the table, the table is extended.
  If there are less, the remaining cells remain the same.

*tableName* **copy** *srcTable* 
  Makes a copy of *srcTable in *tableName*.  *SrcTable* is the another
  datatable.  Any datatable data in *tableName* (rows, column, cells, and
  tags) are first removed.

*tableName* **dir** *pathName* ?\ *switches* ... ?
  Fills *tableName* with the directory listing specified by *pathName*. If
  *pathName* is a directory, then its entries are added to the table.
  *Switches* can be any of the following:

  The following switches are available:

  **-fields** *fieldsList* 
    Specifies what directory information is to be added as columns in the
    table.  *FieldsList* is a TCL list of field names.  The valid field
    names is listed below.

    **size**
      Adds a decimal string giving the size of file name in bytes.
    **mode**
      Adds a decimal string giving the mode of the file or directory.
      The 12 bits corresponding to the mask 07777 are the file mode bits
      and the least significant 9 bits (0777) are the file permission bits.
    **type**
      Adds the type of the file or directory. The type of file name
      will be one of "file", "directory", "characterSpecial",
      "blockSpecial", "fifo", "link", or "socket".
    **uid**
      Adds the user ID of the owner of the file or directory.
    **gid**
      Adds the group ID of the owner of the file or directory.
    **atime**
      Adds a decimal string giving the time at which file name was
      last accessed.   
    **ctime**
      Adds a decimal string giving the time at which the status of file
      name was changed. Status may be changed by writing or by setting
      inode information (i.e., owner, group, link count, mode, etc.).
    **mtime**
      Adds a decimal string giving the time at which file name was
      last modified.   
    **all**
      Adds all the above fields.

  **-hidden**   
    Include hidden files (files that start with a ".")

  **-nocase**   
    Ignore case when matching patterns using the **-patterns** switch.
    
  **-patterns** *patternList*
    Only include files and directories that match one or more patterns.
    *PatternList* is a TCL list of **glob**\-style patterns.  (such as
    "*.jpg").  Matching is done in a fashion similar to that used by the
    TCL **glob** command.

  **-permissions** *permString*
    Only include files or directories that have the one or more of the
    permssions in *permString*.  *PermString* is a string that may contain
    any of the following letters.

    **r**
      Entry is readable by the user.
    **w**
      Entry is writable by the user.
    **x**
      Entry is executable by the user.

  **-readonly**
    Only include files and directories that are readable by the user.

  **-type** *typeList*
    Only include files or directories that have the match types in
    *typeList*.  *TypeList* may contain any of the following.

    **block**
      Type of entry is block (buffered) special.
    **character**
      Type of entry is character (unbuffered) special.
    **directory**
      Type of entry is a directory.
    **file**
      Type of entry is a regular file.
    **link**
      Type of entry is a link.  
    **pipe**
      Type of entry is a pipe (fifo).  
    **socket**
      Type of entry is a socket.

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

*tableName* **exists** *rowName* *columnName*
  Indicates if a cell value exists at *rowName*, *columnName* in
  *tableName*.  *RowName* and *columnName* may be a label, index, or tag,
  but may not represent more than one row or column. If the cell is empty,
  then "0" is returned.  If either *rowName* or *columnName* do not exist,
  "0" is returned.  Otherwise, "1" is returned.

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

  **-emptyvalue**  *emptyString*
    Return *emptyString* for empty cells when evaluating column variables.

  **-invert**  
    Returns rows that where *rowExpr* is false.

  **-maxrows**  *numRows*
    Stop when *numRows* rows have been found.

  **-rows** *rowList*
    Consider only the rows in *rowList*.  *RowList* is a list of
    of row labels, indices, or tags that may refer to multiple rows.

*tableName* **get** *rowName* *columnName* ?\ *defValue*\ ?
  Returns the cell value at *rowName*, *columnName* in *tableName*.
  *RowName* and *columnName* may be a label, index, or tag, but may not
  represent more than one row or column. If the cell is empty, then the
  empty value string is returned. By default it's an error if either
  *rowName* or *columnName* do not exist but the *defValue* argument lets
  you return a known value instead of generating an error. *DefValue* can
  be any string.
  
*tableName* **import** *dataFormat* ?\ *switches* ... ?
  Import data into *tableName* from another format. *DataFormat* is one of
  the different formats are described in the section `DATATABLE FORMATS`_
  below. *Switches* are specific to *dataFormat*.

*tableName* **keys** *columnName* ?\ *columnName* ... ?
  Generates an internal lookup table from the columns given.  This is
  especially useful when a combination of column values uniquely represent
  rows of the table. *ColumnName* may be a label, index, or tag, but may
  not represent more than one row or column.
  
*tableName* **lappend** *rowName* *columnName* ?\ *value* ... ?
  Appends one or more values to the current value at *rowName*,
  *columnName* in *tableName*.  Each new value is appended as a list
  element. Both *rowName* and *columnName* may be a label, index, or tag,
  and may represent more than one row or column. This command is for
  "string" cells only.

*tableName* **limits** ?\ *columnName*\ ?
  Returns the minimum and maximum cell values in *tableName*.  If
  *columnName* is present, the minimum and maximum cell values in
  *columnName* are returned.  *ColumnName* may be a column label, index, or
  tag, but may not represent more than one column.

*tableName* **lookup** ?\ *value* ... ?
  Searches for the row matching the values keys given.  *Value* is a value
  from the columns specified by the **keys** operation.  The order and number
  of the values must be the same as the columns that were specified in the
  **keys** operation.  If a matching combination is found, the index of the
  row is returned, otherwise "-1".

*tableName* **maximum** ?\ *columnName*\ ?
  Returns the maximum cell value in the table.  If a *columnName* argument
  is present, the maximum cell value in *columnName* is returned.
  *ColumnName* may be a column label, index, or tag, but may not represent
  more than one column.

*tableName* **minimum** ?\ *columnName*\ ?
  Returns the minimum cell value in the table.  If a *columnName* argument
  is present, the maximum cell value in *columnName* is returned.
  *ColumnName* may be a column label, index, or tag, but may not represent
  more than one column.

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
  
  **-data**  *dataString*
    Reads the dump information from *dataString*.

  **-file**  *fileName*
    Reads the dump information from *fileName*.

  **-notags**  
    Ignore row and columns tags found in the dump information.

  **-overwrite**  
    Overwrite any rows or columns.

*tableName* **row copy** *destRow* *srcRow* ?\ *switches* ... ?
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
    *tableName* is the source table.

*tableName* **row create** ?\ *switches* ... ?
  Creates a new row in *tableName*. The cells of the new row is initially
  empty. The index of the new row is returned.  *Switches* can be any of
  the following:

  **-after** *rowName*
    The position of the new row will be after *rowName*. *RowName* may be a
    label, index, or tag, but may not represent more than one row.

  **-before** *rowName*
    The position of the new row will be before *rowName*. *RowName* may be
    a label, index, or tag, but may not represent more than one row.

  **-label** *label*
    Specifies the label for the new row.

  **-tags** *tagList*
    Specifies the tags for the new row.

*tableName* **row delete** ?\ *rowName* ... ?
  Deletes one or more rows from *tableName*. *RowName* may be a row label,
  index, or tag and may refer to multiple rows (example: "all").

*tableName* **row duplicate** ?\ *rowName* ... ?
  Creates duplicate rows for each *rowName* given.  The row label is
  duplicated and row tags are copied. *RowName* may be a row label, index,
  or tag and may refer to multiple rows (example: "all").
  
*tableName* **row empty** *rowName*
  Returns the column indices of the empty cells in *rowName*.  *RowName*
  may be a label, index, or tag, but may not represent more than one row.

*tableName* **row exists** *rowName*
  Indicates if *rowName* exists in *tableName*. *RowName* may be a label,
  index, or tag, but may not represent more than one row.  Returns "1" if
  the row exists, "0" otherwise.

*tableName* **row extend** *numRows* ?\ *switches* ... ?
  Extends *tableName* by one of more rows.  *NumRows* indicates how many
  new rows to add. *Switches* can be any of the following:

  **-labels** *list*
    Specifies the labels for the new rows.  

*tableName* **row get** ?\ *-labels*\ ? *rowName* ?\ *columnName* ... ?
  Returns the values from the specified row.  *RowName* may be a label,
  index, or tag, but may not represent more than one row.  By default all
  the values of *rowName* are returned, but if one or more *columnName*
  arguments are specified, then only the values for specified columns are
  retrieved.  *ColumnName* may be a column label, index, or tag and may not
  represent more than one column.

  This command returns pairs of values and column indices of the selected
  cells. If the *-labels* flag is present, the column label is returned
  instead of the index.

*tableName* **row index** *rowName* 
  Returns the index of the specified row.  *RowName* may be a label, index,
  or tag, but may not represent more than one row.
  
*tableName* **row indices** ?\ *switches* ... ? ?\ *pattern* ... ?
  Returns the indices of the row whose labels match any *pattern*.
  *Pattern* is a **glob**\-style pattern to match.  Matching is done in a
  fashion similar to that TCL **glob** command.  *Switches* can be any of
  the following:

  **-duplicates** 
    Return only the indices of rows with duplicate labels.

*tableName* **row join** *srcTable* ?\ *switches* ... ?
  Copies the rows of *srcTable* into *tableName*. New rows are created for
  each row in *srcTable*. Duplicate row labels may be created. Row tags are
  also copied. *Switches* can be any of the following:

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
    
*tableName* **row label** *rowName* ?\ *label*\ ?  ?\ *rowName* *label* ... ?
  Gets or sets the label of the specified row(s).  *RowName* may be a
  label, index, or tag, but may not represent more than one row.  If
  *rowName* is the only argument, then the row label is returned.  If one
  or more *rowName* and *label* pairs are specified, this command sets the
  labels of the specified rows.
  
*tableName* **row labels** *rowName* ?\ *labelList*\ ?
  Gets or sets the labels of the specified row.  *RowName* may be a label,
  index, or tag, but may not represent more than one row. If a *labelList*
  argument is present, then the row labels are set from the list of labels.

*tableName* **row move** *destRow* *firstRow* *lastRow* ?\ *switches* ... ?
  Move one or more rows in *tableName.  *DestRow*, *firstRow*, and
  *lastRow* are rows in *tableName*.  Each may be a label, index, or
  tag, but may not represent more than one row.  *FirstRow and
  *lastRow* designate a range of rows to move.  To move one row,
  specify *firstRow* and *lastRow* as the same row.  *DestRow*
  is the destination location where to move the rows.  It can not be in
  the designated range of rows.  The following *switches* are available.

  **-after** 
    Specifies that the destination location is the row after *destRow*.
    The default is to position the rows before *destRow*.  This allows
    you to move rows to the end.

*tableName* **row names**  ?\ *pattern* ... ?
  Returns the labels of the rows in *tableName*.  If one of *pattern*
  arguments are present, then the label of any row matching one of the
  patterns is returned. *Pattern* is a **glob**\-style pattern.  Matching
  is done in a fashion similar to that used by the TCL **glob** command.

*tableName* **row nonempty**  *rowName*
  Returns the column indices of the non-empty cells in the row.  *RowName* may
  be a label, index, or tag, but may not represent more than one row.

*tableName* **row set**  *rowName* ?\ *columnName* *value* ... ? 
  Sets values for cells in the specified row. *RowName* may be a label,
  index, or tag and may refer to multiple rows (example: "all").  If one or
  more *columnName* *value* pairs are given then the cell at *columnName*,
  *rowName* is set to *value*.  If either *columnName* or *rowName* does
  not exist, the column or row is automatically created. If the column or
  row is an index, *tableName* may automatically grow. If the column type
  is something other than **string**, *value* will be converted into the
  correct type.  If the conversion fails, an error will be returned.  See
  the **column type** operation for a description of the different types.

*tableName* **row tag add**  *tagName* ?\ *rowName* ... ? 
  Adds the tag to *rowName*.  *TagName* is an arbitrary string but can't be
  one of the built-in tags ("all" or "end"). It is not an error if
  *rowName* already has the tag. If no *rowName* arguments are present,
  *tagName* is added to *tableName* but refers to no rows.  This is useful
  for creating empty row tags.

*tableName* **row tag delete**  *rowName* ?\ *tagName* ... ? 
  Removes one or more tags from *rowName*.  *TagName* is an arbitrary
  string but can't be one of the built-in tags ("all" or "end"). The
  built-in tags "all" and "end" can't be deleted.

*tableName* **row tag exists**  *tagName* ?\ *rowName* ... ? 
  Indicates if any row in *tableName* has the tag.  *TagName* is an
  arbitrary string.  Returns "1" if the tag exists, "0" otherwise.  By
  default all rows are searched. But if one or more *rowName* arguments are
  present, then if the tag is found in any *rowName*, "1" is
  returned. *RowName* may be a label, index, or tag and may refer to
  multiple rows (example: "all").

*tableName* **row tag forget**  ?\ *tagName* ... ? 
  Remove one or more tags from all the rows in *tableName*. *TagName* is an
  arbitrary string but can't be one of the built-in tags ("all" or "end").

*tableName* **row tag get** *rowName* ?\ *pattern* ... ? 
  Returns the tags for *rowName*. *RowName* may be a label, index, or tag,
  but may not represent more than one row. By default all tags for
  *rowName* are returned.  But if one or more *pattern* arguments are
  present, then any tag that matching one of the patterns will be returned.
  *Pattern* is a **glob**\-style pattern.  Matching is done in a fashion
  similar to that used by the TCL **glob** command.

*tableName* **row tag indices** ?\ *tagName* ... ? 
  Returns the indices of rows that have one or more *tagName*. *TagName* is
  an arbitrary string.

*tableName* **row tag labels** ?\ *tagName* ... ? 
  Returns the row labels that have one or more *tagName*. *TagName* is an
  arbitrary string.

*tableName* **row tag names** ?\ *pattern* ... ? 
  Returns the row tags of the table. By default all row tags are
  returned. But if one or more *pattern* arguments are present, then any
  tag matching one of the patterns will be returned. *Pattern* is a
  **glob**\-style pattern.  Matching is done in a fashion similar to that
  used by the TCL **glob** command.

*tableName* **row tag range** *first* *last* ?\ *tagName* ... ? 
  Adds one or more tags the rows in the range given.  *First* and *last*
  may be a row label, index, or tag, but may not represent more than one
  row. *TagName* is an arbitrary string but can't be one of the built-in
  tags ("all" or "end").

*tableName* **row tag set** *rowName* ?\ *tagName* ... ?
  Adds one or more tags to *rowName*. *RowName* may be a row label, index,
  or tag and may refer to multiple rows (example: "all"). *TagName* is an
  arbitrary string but can't be one of the built-in tags ("all" or "end").

*tableName* **row tag unset** *rowName* \? *tagName* ... ?
  Remove one or more tags from *rowName*. *RowName* may be a row label,
  index, or tag and may refer to multiple rows (example: "all").  *TagName*
  is an arbitrary string but can't be one of the built-in tags ("all" or
  "end").

*tableName* **row unset**  *rowName* ?\ *columnName* ... ?
  Unsets the cell values of *rowName*.  *RowName* may be a label, index, or
  tag, but may not represent more than one row.  Bu default all cells in
  *rowName* are unset, but one or more *columnName* and *value* pairs are
  specified, only those cells at *columnName*, *rowName* are unset.
  *ColumnName* may be a column label, index, or tag and may refer to
  multiple columns (example: "all").

*tableName* **row values**  *rowName* ?\ *valueList*\ ?
  Gets or sets the cell values of *rowName*.  *RowName* may be a label,
  index, or tag, but may not represent more than one row.  If no
  *valueList* argument is present, this command returns the values of all
  the cells in *rowName*.  Otherwise this command sets the cell values of
  *rowName* from the elements of the list *valueList*.  If there are more
  values in *valueList* than columns in the table, the table is extended.
  If there are less, the remaining cells remain the same.

*tableName* **set** *rowName* *columnName* *value* 
  Sets the value at *rowName*, *columnName* in *tableName*.  *RowName* and
  *columnName* may be a label, index, or tag and may refer to multiple rows
  (example: "all"). If either *rowName* or *columnName* is and index or
  label and does not already exist, the row or column is automatically
  created.  If the row or column is an index, *tableName* may automatically
  grow. *Value* is the value to be set.  If the column type is not
  **string**, *value* is converted into the correct type.  If the conversion
  fails, an error will be returned.

*tableName* **sort** ?\ *switches* ... ?
  Sorts the table.  Column are compared in order. The type comparison is
  determined from the column type.  But you can use **-ascii** or
  **-dictionary** switch to sort the rows.  If the **-noreorder**,
  **-nonempty**, **-unique**, or **-rows** switches are present, the list of
  the sorted rows will be returned but the rows of the table will not be
  rearranged the table. *Switches* can be one of the following:

  **-ascii**
    Uses string comparison with Unicode code-point collation order (the name
    is for backward-compatibility reasons.)  The string representation of
    the values are compared.   

  **-byfrequency** 
    Sorts rows according to the frequency of their values.  The rows
    of *tableName* will not be rearranged.  A list of the row
    indices will be returned instead.

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

  **-nocase** 
    Ignores case when comparing values.  This only has affect when the
    **-ascii** switch is present.

  **-nonempty** 
    Sorts only non-empty cells. The rows of *tableName* will not be
    rearranged.  A list of the row indices will be returned instead.

  **-noreorder** 
    Indicates not to reorder the rows in the table.  This switch is implied
    when the **-nonempty**, **-unique**, or **-rows** switches are used.

  **-rows** *rowList*
    Consider only the rows in *rowList*.  *RowList* is a list of
    of row labels, indices, or tags that may refer to multiple rows.
    The table will not be reordered.

  **-unique** 
    Returns only the unique rows.  The rows of *tableName* will not be
    rearranged.  

  **-valuesvariable** *varName* 
    Returns the row values instead of their indices.  The rows of
    *tableName* will not be rearranged.  A list of the row values
    will be returned instead.

*tableName* **trace cell** *rowName* *columnName* *ops* *cmdPrefix*
  Registers a command to be invoked when the cell (designated by *rowName*
  and *columnName*) value is read, written, or unset. *RowName* and
  *columnName* may be a label, index, or tag and may refer to multiple rows
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

*tableName* **trace column** *columnName* *ops* *cmdPrefix*
  Registers a command to be invoked any cell in *columnName* is read,
  written, or unset. *ColumnName* may be a label, index, or tag and may
  refer to multiple columns (example: "all").  *Ops* indicates which
  operations are of interest, and consists of one or more of the following
  letters:

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
  Describes *traceName*.  A list of the trace's name, row, column, flags,
  and command and their values is returned.
  
*tableName* **trace names** ?\ *pattern* ... ?
  Returns the names of the traces currently registered. This includes cell,
  row, and column traces.  If one or more of *pattern* arguments are
  present then any trace name matching one of the patterns is
  returned. *Pattern* is a **glob**\-style pattern.  Matching is done in a
  fashion similar to that used by the TCL **glob** command.
   
*tableName* **trace row** *rowName* *how* *cmdPrefix*
  Registers a command when any cell in *rowName* is read, written, or
  unset. *RowName* may be a label, index, or tag and may refer to multiple
  rows (example: "all").  *Ops* indicates which operations are of interest,
  and consists of one or more of the following letters:

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

*tableName* **unset** *rowName* *columnName* ?\ *rowName* *columnName* ... ?
  Unsets the values located at one or more *rowName*, *columnName*
  locations.  *RowName* and *columnName* may be a label, index, or tag and
  may refer to multiple rows or columns (example "all").  When a value is
  unset the cell becomes empty.
  
*tableName* **watch column**  *columnName* ?\ *flags* ... ? *cmdPrefix*
  Registers a command to be invoked when an event occurs on a column of
  *tableName*. The events include when columns are added, deleted, moved or
  relabeled.  *ColumnName* may be a label, index, or tag and may refer to
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
  row and column watches.  If one or more of *pattern* arguments are
  present then any watch name matching one of the patterns is
  returned. *Pattern* is a **glob**\-style pattern.  Matching is done in a
  fashion similar to that used by the TCL **glob** command.
   
*tableName* **watch row**  *rowName* ?\ *flags*\ ? *cmdPrefix*
  Registers a command to be invoked when an event occurs on a row of
  *tableName*. The events include when rows are added, deleted, moved or
  relabeled.  *RowName* may be a label, index, or tag and may refer to
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

  **-comment** *commentChar*
   Specifies a comment character.  Any line in the CSV file starting
   with this character is treated as a comment and ignored.  By default
   the comment character is "", indicating no comments.

  **-data** *dataString*
    Read the CSV data from *dataString*.

  **-emptyvalue** *emptyString*
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

  **-possibleseparators**  *separatorsString*
    Specifies a string of chararacters to test as possible separators for
    the data. It *separatorsString* is "", then default set of characters
    is used.  The default is ",\t|;".

  **-quote** *quoteChar*
    Specifies the quote character.  This is by default the double quote (")
    character.

  **-separator** *sepChar*
    Specifies the separator character.  By default this is the comma (,)
    character. If *sepChar* is "auto", then the separator is automatically
    determined.

*tableName* **export csv** ?\ *switches* ... ?
  Exports the datatable into CSV data.  If no **-file** switch is provided,
  the CSV output is returned as the result of the command.  The following
  import switches are supported:

  **-columnlabels** 
    Indicates to create an extra row in the CSV containing the column
    labels.

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
    Specifies the subset of rows from *tableName* to export.  *RowList* is
    a list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are exported.

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

  **-query** *queryString*
    Specifies the SQL query to make to the *Mysql* database.

*tableName* **export mysql** ?\ *switches* ... ?
  Exports *tableName* to a *Mysql* database.  The **-db**, **-host**,
  **-password**, **-port** and **-query** switches are required.  The
  following switches are supported:

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.
    *ColumnList* is a list of column specifiers. Each specifier may be a
    column label, index, or tag and may refer to multiple columns (example:
    "all"). By default all columns are exported.

  **-db** *dbName*
    Specifies the name of the database.  

  **-host** *hostName*
    Specifies the name or address of the *Mysql* server host.  

  **-password** *password*
    Specifies the password of the *Mysql* user. 

  **-port** *portNumber*
    Specifies the port number of the *Mysql* server.

  **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  *RowList* is
    a list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are exported.

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

  **-query** *queryString*
    Specifies the SQL query to make to the *Postgresql* database.

  **-table** *tableName*
    Specifies the name of the *Postgresql* table being queried.

*tableName* **export psql** ?\ *switches* ... ?
  Exports *tableName* to a *Postgresql* database.  The **-db**, **-host**,
  **-password**, **-port** and **-table** switches are required. The
  following switches are supported:

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.
    *ColumnList* is a list of column specifiers. Each specifier may be a
    column label, index, or tag and may refer to multiple columns (example:
    "all"). By default all columns are exported.

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
    Specifies the subset of rows from *tableName* to export.  *RowList* is
    a list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are exported.

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

  **-query** *queryString*
    Specifies the SQL query to make to the *Sqlite* database.

*tableName* **export sqlist** ?\ *switches* ... ?
  Exports the datatable into *Sqlite* data.  The **-file** switch is
  required. The following import switches are supported:

  **-columns** *columnList*
    Specifies the subset of columns from *tableName* to export.
    *ColumnList* is a list of column specifiers. Each specifier may be a
    column label, index, or tag and may refer to multiple columns (example:
    "all"). By default all columns are exported.

  **-file** *fileName*
    Write the *Sqlite* output to the file *fileName*.

  **-rowlabels** 
    Export the row labels from *tableName* as an extra column "_rowId" in
    the *Sqlite* table.

  **-rows** *rowList*
    Specifies the subset of rows from *tableName* to export.  *RowList* is
    a list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are exported.

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

  **-data** *dataString*
    Read XML from the data *dataString*.

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
    Specifies the subset of rows from *tableName* to export.  *RowList* is
    a list of row specifiers. Each specifier may be a row label, index, or
    tag and may refer to multiple row (example: "all").  By default all
    rows are exported.

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
