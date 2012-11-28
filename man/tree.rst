===============
tree
===============

-------------------------------------------------
Create and manage tree data objects.
-------------------------------------------------

:Author: gahowlett@gmail.com
:Date:   2012-11-28
:Copyright: George A. Howlett.
    See the file "license.terms" for information on usage and redistribution
    of this file, and for a DISCLAIMER OF ALL WARRANTIES.
:Version: BLT 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

.. TODO: authors and author with name <email>

SYNOPSIS
========

**blt::tree create** ?\ *treeName*\ ?

**blt::tree destroy** *treeName*...

**blt::tree names** ?\ *pattern*\ ?

DESCRIPTION
===========

The **tree** command creates tree data objects.  A *tree object*
is general ordered tree of nodes.  Each node has both a label and a
key-value list of data.  Data can be heterogeneous, since nodes do not
have to contain the same data keys.  It is associated with a Tcl
command that you can use to access and modify the its structure and
data. Tree objects can also be managed via a C API.

SYNTAX
======

**blt::tree create** ?\ *treeName*\ ?  

  Creates a new tree object.  The name of the new tree is returned.  If no
  *treeName* argument is present, then the name of the tree is automatically
  generated in the form "``tree0``", "``tree1``", etc.  If the substring
  "``#auto``" is found in *treeName*, it is automatically substituted by a
  generated name.  For example, the name ``.foo.#auto.bar`` will be translated
  to ``.foo.tree0.bar``.

  A new Tcl command (by the same name as the tree) is also created.  Another
  Tcl command or tree object can not already exist as *treeName*.  If the Tcl
  command is deleted, the tree will also be freed.  The new tree will contain
  just the root node.  Trees are by default, created in the current namespace,
  not the global namespace, unless *treeName* contains a namespace qualifier,
  such as "``fred::myTree``".

**blt::tree destroy** *treeName*...

  Releases one of more trees.  The Tcl command associated with *treeName* is
  also removed.  Trees are reference counted.  The internal tree data object
  isn't destroyed until no one else is using the tree.

**blt::tree names** ?\ *pattern*\ ?

  Returns the names of all tree objects.  if a *pattern* argument
  is given, then the only those trees whose name matches pattern will
  be listed.

NODE IDS AND TAGS
=================

Nodes in a tree object may be referred in either of two ways: by id or by
tag.  Each node has a unique serial number or id that is assigned to that
node when it's created. The id of an node never changes and id numbers
are not re-used.

A node may also have any number of tags associated with it.  A tag is
just a string of characters, and it may take any form except that of
an integer.  For example, "``x123``" is valid, but "``123``"
isn't.  The same tag may be associated with many different nodes.
This is commonly done to group nodes in various interesting ways.

There are two built-in tags: The tag ``all`` is implicitly
associated with every node in the tree.  It may be used to invoke
operations on all the nodes in the tree.  The tag ``root`` is
managed automatically by the tree object. It applies to the node
currently set as root.

When specifying nodes in tree object commands, if the specifier is an
integer then it is assumed to refer to the single node with that id.
If the specifier is not an integer, then it is assumed to refer to all
of the nodes in the tree that have a tag matching the specifier.  The
symbol *node* is used below to indicate that an argument specifies
either an id that selects a single node or a tag that selects zero or
more nodes.  Many tree commands only operate on a single node at a
time; if *node* is specified in a way that names multiple items, then
an error "refers to more than one node" is generated.

NODE MODIFIERS
==============

You can also specify node in relation to another node by appending one
or more modifiers to the node id or tag.  A modifier refers to a node
in relation to the specified node.  For example, 
"``root->firstchild``"
selects the first subtree of the root node.

The following modifiers are available:

  firstchild  \
     Selects the first child of the node.  
  lastchild  \
    Selects the last child of the node.  
  next  \
    Selects the next node in preorder to the node.  
  nextsibling  \
    Selects the next sibling of the node.  
  parent  \
    Selects the parent of the node.  
  previous \
    Selects the previous node in preorder to the node.  
  prevsibling  \
    Selects the previous sibling of the node.  
  "label"  \
   Selects the node whose label is *label*.  Enclosing *label* in 
   quotes indicates to always search for a node by its label (for example, 
   even if the node is labeled "parent").

It's an error the node can't be found.  For example,
**lastchild** and **firstchild** will generate errors if the node
has no children.  The exception to this is the **index** operation.
You can use **index** to test if a modifier is valid.

.. _`TREE OPERATIONS`:

TREE OPERATIONS
===============

Once you create a tree object, you can use its Tcl command 
to query or modify it.  The
general form is

*treeName* *operation* ?\ *arg*\ ?...

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for trees are listed below.

*treeName* **ancestor** *node1* *node2*

  Returns the mutual ancestor of the two nodes *node1* and *node2*.  The
  ancestor can be one of the two nodes.  For example, if *node1* and *node2*
  are the same nodes, their ancestor is *node1*.

*treeName* **append** *node* *key* ?\ *string*...\ ?

  Appends one or more strings to the data field *key* in the node *node*.  If
  no value exists at that location, it is given a value equal to the
  concatenation of all the string arguments.  The result of this command is
  the new value stored at *key*.  This command provides an efficient way to
  build up long string values incrementally.

*treeName* **apply** *node* ?\ *switches*\ ?

  Runs commands for all nodes matching the criteria given by *switches* for
  the subtree designated by *node*.  By default all nodes match, but you can
  set switches to narrow the match.  This operation differs from **find** in
  two ways: 1) Tcl commands can be invoked both pre- and post-traversal of a
  node and 2) the tree is always traversed in depth first order.

  The **-exact**, **-glob**, and **-regexp** switches indicate both what kind
  of pattern matching to perform and the pattern.  By default each pattern
  will be compared with the node label.  You can set more than one of these
  switches.  If any of the patterns match (logical or), the node matches.  If
  the **-key** switch is used, it designates the data field to be matched.

  The valid switches are listed below:

  **-depth** *number*

    Descend at most *number* (a non-negative integer) levels If *number* is
    ``1`` this means only apply the tests to the children of *node*.

  **-exact** *string*

    Matches each node using *string*.  The node must match *string* exactly.

  **-glob** *string*

    Test each node to *string* using global pattern matching.  Matching is
    done in a fashion similar to that used by the C-shell.

  **-invert**

    Select non-matching nodes.  Any node that *doesn't* match the given
    criteria will be selected.

  **-key** *key*

    If pattern matching is selected (using the **-exact**, **-glob**, or
    **-regexp** switches), compare the values of the data field keyed by *key*
    instead of the node's label.  If no pattern matching switches are set,
    then any node with this data key will match.

  **-leafonly**

    Only test nodes with no children.

  **-nocase**

    Ignore case when matching patterns.

  **-path**

    Use the node's full path when comparing nodes.  The node's full path is a
    list of labels, starting from the root of each ancestor and the node
    itself.

  **-precommand** *command*

    Invoke *command* for each matching node.  Before *command* is invoked, the
    id of the node is appended.  You can control processing by the return
    value of *command*.  If *command* generates an error, processing stops and
    the **find** operation returns an error.  But if *command* returns
    **break**, then processing stops, no error is generated.  If *command*
    returns **continue**, then processing stops on that subtree and continues
    on the next.

  **-postcommand** *command*

    Invoke *command* for each matching node.  Before *command* is invoked, the
    id of the node is appended.  You can control processing by the return
    value of *command*.  If *command* generates an error, processing stops and
    the **find** operation returns an error.  But if *command* returns
    **break**, then processing stops, no error is generated.  If *command*
    returns **continue**, then processing stops on that subtree and continues
    on the next.

  **-regexp** *string*

    Test each node using *string* as a regular expression pattern.

  **-tag** *string*

    Only test nodes that have the tag *string*.

*treeName* **attach** *treeObject* ?\ *switches*\ ?

  Attaches to an existing tree object *treeObject*.  The current tree
  associated with *treeName* is discarded.  In addition, the current set of
  tags, notifier events, and traces are removed. The valid *switches* are
  listed below:

  **-newtags** 

    By default, the tree will share the tags of the attached tree. If this
    flag is present, the tree will start with an empty tag table.

*treeName* **children** *node*

  Returns a list of children for *node*.  If *node* is a leaf,
  then an empty string is returned.

*treeName* **copy** *parent* ?\ *tree*\ ? *node* ?\ *switches*...\ ?

  Copies *node* into *parent*. Both nodes *node* and *parent* must already
  exist. The id of the new node is returned. You can also copy nodes from
  another tree.  If a *tree* argument is present, it indicates the name of the
  source tree.  The valid *switches* are listed below:

  **-label** *string*

    Label *destNode* as *string*.  By default, *destNode* has
    the same label as *srcNode*.

  **-overwrite**

    Overwrite nodes that already exist.  Normally nodes are always created,
    even if there already exists a node by the same name.  This switch
    indicates to add or overwrite the node's data fields.

  **-recurse**

    Recursively copy all the subtrees of *srcNode* as well.  In this case,
    *srcNode* can't be an ancestor of *destNode* as it would result in a
    cyclic copy.

  **-tags**

    Copy tag inforation.  Normally the following node is copied: its label and
    data fields.  This indicates to copy tags as well.

*treeName* **degree** *node* 

  Returns the number of children of *node*.

*treeName* **delete** *node*...

  Recursively deletes one or more nodes from the tree.  The node and all its
  descendants are removed.  The one exception is the root node.  In this case,
  only its descendants are removed.  The root node will remain.  Any tags or
  traces on the nodes are released.

*treeName* **depth** *node* 

  Returns the depth of the node.  The depth is the number of steps from the
  node to the root of the tree.  The depth of the root node is ``0``.

*treeName* **dir** *node* *path* ?\ *switches*...\ ?

  Loads the directory entry *path* into the tree at
  node *node*. The following switches are available:

  **-fields** *list* 

  **-readable**

    Only load files and directories that are readable by the user.

  **-readonly**

    Only load files and directories that are readable by the user.

  **-writable**

  **-executable**

  **-directory**

    Only load directories.

  **-link**

    Only load links.

  **-pattern** *pattern*

    Only load files and directories that match *pattern*.  The default
    pattern is "``*``".

  **-recurse** 

    If *path* is a directory, recusively load files and subdirectories
    into the tree.  New tree nodes are created for each file and subdirectory.

*treeName* **dump** *node* ?\ *switches*...\ ?

  Returns a list of the paths and respective data for *node* and its
  descendants.  The subtree designated by *node* is traversed returning the
  following information for each node: 1) the node's path relative to *node*,
  2) a sublist key value pairs representing the node's data fields, and 3) a
  sublist of tags.  This list returned can be used later to copy or restore
  the tree with the **restore** operation.  The following switches are 
  available:

  **-file** *fileName*

    Write the dump information to the file *fileName*.

  **-data** *varName*

    Saves the dump information in the TCL variable *varName*.

*treeName* **exists** *node* ?\ *key*\ ?

  Indicates if *node* exists in the tree.  If a *key* argument is present then
  the command also indicates if the named data field exists.

*treeName* **export** 

  Returns a list of all the formats with registered data handlers.

*treeName* **export** *format* ?\ *switches*\ ?

  Exports the tree contents into *format*. *Format* is the format of
  the exported data.  See TREE FORMATS for what file formats
  are available.

*treeName* **find** *node* ?\ *switches*\ ? 

  Finds for all nodes matching the criteria given by *switches* for the
  subtree designated by *node*.  A list of the selected nodes is returned.  By
  default all nodes match, but you can set switches to narrow the match.

  The **-exact**, **-glob**, and **-regexp** switches indicate both what kind
  of pattern matching to perform and the pattern.  By default each pattern
  will be compared with the node label.  You can set more than one of these
  switches.  If any of the patterns match (logical or), the node matches.  If
  the **-key** switch is used, it designates the data field to be matched.

  The order in which the nodes are traversed is controlled by the **-order**
  switch.  The possible orderings are **preorder**, **postorder**,
  **inorder**, and **breadthfirst**.  The default is **postorder**.

  The valid switches are listed below:

  **-addtag** *string* 

    Add the tag *string* to each selected node.  

  **-count** *number*
 
    Stop processing after *number* (a positive integer) matches. 

  **-depth** *number*

    Descend at most *number* (a non-negative integer) levels
    If *number* is ``1`` this means only apply the tests
    to the children of *node*.

  **-exact** *string*
 
    Matches each node using *string*.  The node must match *string*
    exactly.

  **-excludes** *nodeList*

   Excludes any node in the list *nodeList* from the search.  
   The subnodes of an excluded node are still examined.

  **-exec** *command*

    Invoke *command* for each matching node.  Before *command* is invoked, the
    id of the node is appended.  You can control processing by the return
    value of *command*.  If *command* generates an error, processing stops and
    the **find** operation returns an error.  But if *command* returns
    **break**, then processing stops, no error is generated.  If *command*
    returns **continue**, then processing stops on that subtree and continues
    on the next.

  **-glob** *string*

    Test each node to *string* using global pattern matching.  Matching is
    done in a fashion similar to that used by the C-shell.

  **-invert**

   Select non-matching nodes.  Any node that *doesn't* match the given
   criteria will be selected.

  **-key** *key*

    Compare the values of the data field keyed by *key* instead of the node's
    label. If no pattern is given (**-exact**, **-glob**, or **-regexp**
    switches), then any node with this data key will match.

  **-leafonly**
  
    Only test nodes with no children.

  **-nocase**
  
    Ignore case when matching patterns.

  **-order** *string* 

    Traverse the tree and process nodes according to *string*. *String* can be
    one of the following:

    breadthfirst
      Process the node and the subtrees at each sucessive level. Each node on a
      level is processed before going to the next level.
    inorder
      Recursively process the nodes of the first subtree, the node itself,
      and any the remaining subtrees.
    postorder
      Recursively process all subtrees before the node.
    preorder
      Recursively process the node first, then any subtrees.

  **-path**

    Use the node's full path when comparing nodes.

  **-regexp** *string*

    Test each node using *string* as a regular expression pattern.

  **-tag** *string*

    Only test nodes that have the tag *string*.

*treeName* **findchild** *node* *label*

  Searches for a child node with the label *label* in the parent *node*.  
  The id of the child node is returned if found.  Otherwise ``-1`` is returned.

*treeName* **firstchild** *node* 

  Returns the id of the first child in the *node*'s list of subtrees.  If
  *node* is a leaf (has no children), then ``-1`` is returned.

*treeName* **get** *node* ?\ *key*\ ? ?\ *defaultValue*\ ?

  Returns a list of key-value pairs of data for the node.  If *key* is
  present, then onlyx the value for that particular data field is returned.
  It's normally an error if *node* does not contain the data field *key*.  But
  if you provide a *defaultValue* argument, this value is returned instead
  (*node* will still not contain *key*).  This feature can be used to access a
  data field of *node* without first testing if it exists.  This operation may
  trigger **read** data traces.

*treeName* **import** 

  Returns a list of all the formats with registered data handlers.

*treeName* **import** *format* ?\ *switches*\ ?

  Imports the tree contents into *format*. *Format* is the format of
  the exported data.  See TREE FORMATS for what file formats
  are available.

*treeName* **index** *node*

  Returns the id of *node*.  If *node* is a tag, it can only specify one node.
  If *node* does not represent a valid node id or tag, or has modifiers that
  are invalid, then ``-1`` is returned.

*treeName* **insert** *parent* ?\ *switches*\ ? 

  Inserts a new node into parent node *parent*.  The id of the new node is
  returned. The following switches are available:

  **-after** *child* 

    Position *node* after *child*.  The node *child* must be a 
    child of *parent*.

  **-at** *number* 

    Inserts the node into *parent*'s list of children at 
    position *number*.  The default is to append *node*.

  **-before** *child* 

    Position *node* before *child*.  The node *child* must be a 
    child of *parent*.

  **-data** *dataList*

    Sets the value for each data field in *dataList* for the 
    new node. *DataList* is a list of key-value pairs.

  **-label** *string* 

    Designates the labels of the node as *string*.  By default, nodes
    are labeled as ``node0``, ``node1``, etc.

  **-node** *id* 

    Designates the id for the node.  Normally new ids are automatically
    generated.  This allows you to create a node with a specific id.
    It is an error if the id is already used by another node in the tree.

  **-tags** *tagList*

    Adds each tag in *tagList* to the new node. *TagList* is a list
    of tags, so be careful if a tag has embedded spaces.

*treeName* **isancestor** *node1* *node2*

  Indicates if *node1* is an ancestor of *node2*. 
  Returns ``1`` if true and ``0`` otherwise.  

*treeName* **isbefore**  *node1* *node2*

  Indicates if *node1* is before *node2* in depth first traversal. 
  Returns ``1`` if true and ``0`` otherwise.  

*treeName* **isleaf** *node*

  Indicates if *node* is a leaf (it has no subtrees).
  Returns ``1`` if true and ``0`` otherwise.  

*treeName* **isroot** *node*

  Indicates if *node* is the designated root.  This can be changed
  by the **chroot** operation.
  Returns ``1`` if true and ``0`` otherwise.  

*treeName* **keys** *node* ?\ *node*...\ ?

  FIXME: Returns the label of the node designated by *node*.  If *newLabel*
  is present, the node is relabeled using it as the new label.

*treeName* **label** *node* ?\ *newLabel*\ ?

  Returns the label of the node designated by *node*.  If *newLabel*
  is present, the node is relabeled using it as the new label.

*treeName* **lastchild** *node*

  Returns the id of the last child in the *node*'s list
  of subtrees.  If *node* is a leaf (has no children), 
  then ``-1`` is returned.

*treeName* **move** *node* *newParent* ?\ *switches*\ ?

  Moves *node* into *newParent*. *Node* is appended to the
  list children of *newParent*.  *Node* can not be an ancestor
  of *newParent*.  The valid flags for *switches* are described below.

  **-after** *child* 

    Position *node* after *child*.  The node *child* must be a 
    child of *newParent*.

  **-at** *number* 

    Inserts *node* into *parent*'s list of children at 
    position *number*. The default is to append the node.

  **-before** *child* 

    Position *node* before *child*.  The node *child* must be a 
    child of *newParent*.

*treeName* **names** *node* ?\ *key*\ ?

  Returns the names of the data fields present for node *node*.  
  If *key* is given, then *key* is an array value and the names 
  of the array elements are returned.

*treeName* **next** *node*

  Returns the next node from *node* in a preorder traversal.
  If *node* is the last node in the tree, 
  then ``-1`` is returned.

*treeName* **nextsibling** *node*

  Returns the node representing the next subtree from *node*
  in its parent's list of children.  If *node* is the last child, 
  then ``-1`` is returned.

*treeName* **notify** *args* 

  Manages notification events that indicate that the tree structure has 
  been changed.
  See the `NOTIFY OPERATIONS`_ section below.

*treeName* **parent** *node*

  Returns the parent node of *node*.  If *node* is the root
  of the tree, 
  then ``-1`` is returned.

*treeName* **path** *node*

  Returns the full path (from root) of *node*.

*treeName* **position** *node*

  Returns the position of the node in its parent's list of children.
  Positions are numbered from 0.  The position of the root node is always 0.

*treeName* **previous** *node*

  Returns the previous node from *node* in a preorder traversal.
  If *node* is the root of the tree, 
  then ``-1`` is returned.

*treeName* **prevsibling** *node*

  Returns the node representing the previous subtree from *node*
  in its parent's list of children.  If *node* is the first child, 
  then ``-1`` is returned.

*treeName* **restore** *node* ?\ *switches*...\ ?

  Performs the inverse function of the **dump** operation, restoring nodes to
  the tree. The format of *dataString* is exactly what is returned by the
  **dump** operation.  It's a list containing information for each node to be
  restored.  The information consists of 1) the relative path of the node, 2)
  a sublist of key value pairs representing the node's data, and 3) a list of
  tags for the node.  Nodes are created starting from *node*. Nodes can be
  listed in any order.  If a node's path describes ancestor nodes that do not
  already exist, they are automatically created.  The valid *switches* are
  listed below:

  **-overwrite**

    Overwrite nodes that already exist.  Normally nodes are always created,
    even if there already exists a node by the same name.  This switch
    indicates to add or overwrite the node's data fields.

  **-file** *fileName*

    Read the dump information from the file *fileName*.

  **-data** *string*

    Reads the dump information from *string*.

*treeName* **root** ?\ *node*\ ?

  Returns the id of the root node.  Normally this is node ``0``.  If
  a *node* argument is provided, it will become the new root of the
  tree. This lets you temporarily work within a subset of the tree.
  Changing root affects operations such as **next**, **path**,
  **previous**, etc.

*treeName* **set** *node* *key value* ?\ *key value*...\ ?

  Sets one or more data fields in *node*. *Node* may be a tag that represents
  several nodes.  *Key* is the name of the data field to be set and *value* is
  its respective value.  This operation may trigger **write** and **create**
  data traces.

*treeName* **size** *node*

  Returns the number of nodes in the subtree. This includes the node
  and all its descendants.  The size of a leaf node is 1.

*treeName* **sort** *node* ?\ *switches*...\ ? 

  Sorts the subtree starting at *node*.  The following switches are
  available:

  **-ascii** 

    Compare strings using ASCII collation order.

  **-command** *string*

    Use command *string* as a comparison command.  To compare two elements,
    evaluate a Tcl script consisting of command with the two elements appended
    as additional arguments.  The script should return an integer less than,
    equal to, or greater than zero if the first element is to be considered
    less than, equal to, or greater than the second, respectively.

  **-decreasing**

    Sort in decreasing order (largest items come first).

  **-dictionary**

    Compare strings using a dictionary-style comparison.  This is the same as
    **-ascii** except (a) case is ignored except as a tie-breaker and (b) if
    two strings contain embedded numbers, the numbers compare as integers, not
    characters.  For example, in **-dictionary** mode, bigBoy sorts between
    bigbang and bigboy, and x10y sorts between x9y and x11y.

  **-integer**

    Compare the nodes as integers.  

  **-key** *string*

    Sort based upon the node's data field keyed by *string*. Normally
    nodes are sorted according to their label. 

  **-path**

    Compare the full path of each node.  The default is to compare only its
    label.

  **-real**

    Compare the nodes as real numbers.

  **-recurse**
 
    Recursively sort the entire subtree rooted at *node*.

  **-reorder** 

    Recursively sort subtrees for each node.  **Warning**.  Unlike the normal
    flat sort, where a list of nodes is returned, this will reorder the tree.

*treeName* **tag** *args*

  Manages tags for the tree object.  See the `TAG OPERATIONS`_ section below.

*treeName* **trace** *args*

  Manages traces for data fields in the tree object.  Traces cause Tcl
  commands to be executed whenever a data field of a node is created, read,
  written, or unset.  Traces can be set for a specific node or a tag,
  representing possibly many nodes.  See the `TRACE OPERATIONS`_ section
  below.

*treeName* **type** *node* *key*

  Returns the type of the data field *key* in the node *node*.

*treeName* **unset** *node* *key*...

  Removes one or more data fields from *node*. *Node* may be a tag that
  represents several nodes.  *Key* is the name of the data field to be
  removed.  It's not an error is *node* does not contain *key*.  This
  operation may trigger **unset** data traces.

.. _`TAG OPERATIONS`:

TAG OPERATIONS
==============

Tags are a general means of selecting and marking nodes in the tree.
A tag is just a string of characters, and it may take any form except
that of an integer.  The same tag may be associated with many
different nodes.  

There are two built-in tags: The tag **all** is implicitly
associated with every node in the tree.  It may be used to invoke
operations on all the nodes in the tree.  The tag **root** is
managed automatically by the tree object.  It specifies the node
that is currently set as the root of the tree.

Most tree operations use tags.  And several operations let you
operate on multiple nodes at once.  For example, you can use the
**set** operation with the tag **all** to set a data field in 
for all nodes in the tree.

Tags are invoked by the **tag** operation.  The
general form is

*treeName* **tag** *operation* ?\ *arg*\ ?...

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for tags are listed below.

*treeName* **tag add** *string* *node*...

  Adds the tag *string* to one of more nodes.

*treeName* **tag delete** *string* *node*...

  Deletes the tag *string* from one or more nodes.  

*treeName* **tag forget** *string*

  Removes the tag *string* from all nodes.  It's not an error if no
  nodes are tagged as *string*.

*treeName* **tag get** *node* *pattern*...

  Returns the tag names for a given node.  If one of more pattern
  arguments are provided, then only those matching tags are returned.

*treeName* **tag names** ?\ *node*\ ?

  Returns a list of tags used by the tree.  If a *node* argument
  is present, only those tags used by *node* are returned.

*treeName* **tag nodes** *string*

  Returns a list of nodes that have the tag *string*.  If no node
  is tagged as *string*, then an empty string is returned.

*treeName* **tag set** *node* *string*...

  Sets one or more tags for a given node.  Tag names can't start with a
  digit (to distinquish them from node ids) and can't be a reserved tag
  ("root" or "all").

*treeName* **tag unset** *node* *string*...  

  Removes one or more tags from a given node. Tag names that don't exist 
  or are reserved ("root" or "all") are silently ignored.

.. _`TRACE OPERATIONS`:

TRACE OPERATIONS
================

Data fields can be traced much in the same way that you can trace Tcl
variables.  Data traces cause Tcl commands to be executed whenever a
particular data field of a node is created, read, written, or unset.
A trace can apply to one or more nodes.  You can trace a specific node
by using its id, or a group of nodes by a their tag.

The tree's **get**, **set**, and **unset** operations can 
trigger various traces.  The **get** operation can cause 
a *read*  trace to fire.  The **set** operation causes a *write* 
trace to fire.  And if the data field is written for the first time, you
will also get a *create* trace.
The **unset** operation triggers *unset* traces.

Data traces are invoked by the **trace**
operation.  The general form is

*treeName* **trace** *operation* ?\ *arg*\ ?...

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for traces are listed below.

*treeName* **trace create** *node* *key* *ops* *command*

  Creates a trace for *node* on data field *key*.  *Node* can refer to more
  than one node (for example, the tag **all**). If *node* is a tag, any node
  with that tag can possibly trigger a trace, invoking *command*.  *Command*
  is command prefix, typically a procedure name.  Whenever a trace is
  triggered, four arguments are appended to *command* before it is invoked:
  *treeName*, id of the node, *key* and, *ops*.  Note that no nodes need have
  the field *key*.  A trace identifier in the form "``trace0``", "``trace1``",
  etc.  is returned.

  *Ops* indicates which operations are of interest, and consists of one or
  more of the following letters:

  **r**
    Invoke *command* whenever *key* is read. Both read and
    write traces are temporarily disabled when *command* is executed.
  **w**
    Invoke *command* whenever *key* is written.  Both read and
    write traces are temporarily disabled when *command* is executed.
  **c**
    Invoke *command* whenever *key* is created.
  **u** 
    Invoke *command* whenever *key* is unset.  Data fields are
    typically unset with the **unset** command.   Data fields are also 
    unset when the tree is released, but all traces are disabled prior
    to that.

*treeName* **trace delete** *traceId*...
  
  Deletes one of more traces.  *TraceId* is
  the trace identifier returned by the **trace create** operation.

*treeName* **trace info** *traceId* 

  Returns information about the trace *traceId*.  *TraceId* is a trace
  identifier previously returned by the **trace create** operation.  It's the
  same information specified for the **trace create** operation.  It consists
  of the node id or tag, data field key, a string of letters indicating the
  operations that are traced (it's in the same form as *ops*) and, the command
  prefix.

*treeName* **trace names**

  Returns a list of identifers for all the current traces.

.. _`NOTIFY OPERATIONS`:

NOTIFY OPERATIONS
=================

Tree objects can be shared among many clients, such as a
**hiertable** widget.  Any client can create or delete nodes,
sorting the tree, etc.  You can request to be notified whenever these
events occur.  Notify events cause Tcl commands to be executed
whenever the tree structure is changed.  

Notifications are handled by the **notify** operation.  The
general form is

*treeName* **notify** *operation* ?\ *arg*\ ?...

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for events are listed below.

*treeName* **notify create** ?\ *switches*...\ ? *command* ?\ *args*\ ?...  

  Creates a notifier for the tree.  A notify identifier in the form
  "``notify0``", "``notify1``", etc.  is returned.

  *Command* and *args* are saved and invoked whenever the tree
  structure is changed (according to *switches*). Two arguments are
  appended to *command* and *args* before it's invoked: the id
  of the node and a string representing the type of event that occured.
  One of more switches can be set to indicate the events that are of
  interest.  The valid switches are as follows:

  **-create** 

    Invoke *command* whenever a new node has been added.

  **-delete**

    Invoke *command* whenever a node has been deleted.

  **-move**

    Invoke *command* whenever a node has been moved.

  **-sort**

    Invoke *command* whenever the tree has been sorted and reordered.

  **-relabel**

    Invoke *command* whenever a node has been relabeled.

  **-allevents**

    Invoke *command* whenever any of the above events occur.

  **-whenidle**

    When an event occurs don't invoke *command* immediately, but queue it to
    be run the next time the event loop is entered and there are no events to
    process.  If subsequent events occur before the event loop is entered,
    *command* will still be invoked only once.

*treeName* **notify delete** *notifyId* 

  Deletes one or more notifiers from the tree.  *NotifyId* is the notifier
  identifier returned by the **notify create** operation.

*treeName* **notify info** *notifyId*

  Returns information about the notify event *notifyId*.  *NotifyId* is a
  notify identifier previously returned by the **notify create** operation.
  It's the same information specified for the **notify create** operation.  It
  consists of the notify id, a sublist of event flags (it's in the same form
  as *flags*) and, the command prefix.

*treeName* **notify names**

  Returns a list of identifers for all the current notifiers.

TREE FORMATS
============

EXAMPLE
=======

KEYWORDS
========

tree, hiertable, widget
