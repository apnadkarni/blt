=========
blt::sftp
=========

-----------------------------------
Transfer files to/from SFTP server.
-----------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::sftp create** ?\ *sftpName*\ ?  ?\ *switches* ... ? 

**blt::sftp destroy** *sftpName*...

**blt::sftp names**  ?\ *pattern*\ ?

DESCRIPTION
-----------

The **blt::sftp** command creates sftp objects.  A sftp object lets you
connect to a SFTP server to transfer files, get a directory listing etc.
Most operations that you can perform with a sftp client program can be done
programmatically with a sftp object.

The  **blt::sftp** command requires the **blt_sftp** package.

 ::
    
    package require BLT
    package require blt_sftp


SYNTAX
------

**blt::sftp create** ?\ *sftpName*\ ? ?\ *switches* ... ?  
  Creates a new sftp object.  The name of the new sftp object is returned.
  If no *sftpName* argument is present, then the name of the sftp is
  automatically generated in the form "sftp0", "sftp1", etc.  If the
  substring "#auto" is found in *sftpName*, it is automatically substituted
  by a generated name.  For example, the name ".foo.#auto.bar" will be
  translated to ".foo.sftp0.bar".

  A new TCL command by the same name as the sftp object is also created.
  Another TCL command or sftp object can not already exist as *sftpName*.
  If the TCL command is deleted, the sftp will also be freed.  The new sftp
  will contain just the root node.  Sftp objects are by default, created in
  the current namespace, not the global namespace, unless *sftpName*
  contains a namespace qualifier, such as "fred::mySftp".

  The following switches are available.

  **-user** *string*  
    Specifies the username of the remote SFTP account.

  **-host** *string* 
    Specifies the hostname of the remote SFTP server.

  **-password** *string* 
    Specifies the password of the user account on the remote SFTP server.

  **-prompt** *cmdString* 
    Specifies a TCL script to be called whenever the user name or password of
    the user account on the remote SFTP is required. The script should return a
    list in the form "*username password*".

  **-publickey** *fileName* 
    Specifies the location of the public key file.  The default location
    is "$HOME/.ssh/id_rsa.pub".

  **-timeout** *seconds* 
    Specifies the idle timeout for the SFTP connection.  When there is no
    **sftp** operation performed after the specified number of seconds, the
    connection is automatically dropped. The sftp object will automatically
    reconnect (if needed) on the next operation.  If *timeout* is zero, then
    no timeout will occur.  The default is "0".

**blt::sftp destroy** ?\ *sftpName* ... ?
  Disconnects and destroys one of more *sftp* objects.  The TCL command
  associated with *sftpName* is also deleted.

**blt::sftp names** ?\ *pattern* ... ?
  Returns the names of all sftp objects.  If a *pattern* argument is given,
  then the any sftp objects whose name matches *pattern* will be listed.
  *Pattern* is a glob-style pattern to match.  Matching is done in a
  fashion similar to that TCL **glob** command.

REMOTE PATHS
------------

Paths on the remote SFTP server may be relative or absolute. Initially the
current working directory is the home directory of the user account on the
SFTP server.

SFTP OPERATIONS
---------------

Once you create a *sftp* object, you can use its TCL command to perform
operations associated with the connection.  The general form is

  *sftpName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of the
command.  The operations available for *sftp* objects are listed below.

*sftpName* **atime** *path* ?\ *time*\ ?
  Sets or gets the access time of the remote file or directory specified by
  *path*.  If a no *time* argument is given, then this command returns the
  access time of *path*.  Otherwise, *time* is a decimal string
  representing in seconds the time at which *path* was last accessed. The
  time is measured in the standard POSIX fashion as seconds from a fixed
  starting time (often January 1, 1970).

*sftpName* **auth**
  Returns the type of authentication used to connect to the remote SFTP
  server.  The possible types are "password" or "publickey".

*sftpName* **chdir** ?\ *path*\ ?
  Changes the current working directory on the remote SFTP server to
  *path*.  If no *path* argument is given, then the user's home directory
  is assumed.

*sftpName* **chgrp** *path* ?\ *gid*\ ? ?\ *-recurse*\ ?
  Sets or gets the group of the file or directory described by *path*
  on the remote SFTP server.  If *gid* is provided, then the group of
  *path* is changed.  *Gid* is the numeric id of the remote group.  If the
  *-recurse* flag is set and *path* is a directory, then the group of the
  subdirectories and files underneath *path* are also changed.

*sftpName* **chmod** *path* ?\ *mode*\ ? ?\ *-recurse*\ ?
  Sets or gets the permissions of the file or directory described by *path*
  on the remote SFTP server.  If *mode* is provided, then the mode of
  *path* is changed.  *Mode* can be in various forms similar to the Unix
  **chmod** command. If the **-recurse** flag is set and *path* is a
  directory, then the mode of the subdirectories and files underneath
  *path* are also changed.

*sftpName* **delete** *path* ?\ *switches* ... ?
  Deletes the file or directory described by *path* on the remote SFTP server.
  *Switches* can be any of the following.

  **-force**  
    Forces the deletion of directories that are not empty.

*sftpName* **dirlist** *path* ?\ *switches* ... ?
  Lists the contents of the directory described by *path* on the remote SFTP
  server.  The files and subdirectories of *path* are returned as a list.
  *Switches* can be any of the following.

  **-fields** *fieldList*
    Specifies the fields to reported.  *FieldList* is a TCL list that may
    contain one of more of the following field names.

    **all**
      Specifies all fields.

    **atime**
      Reports the time in seconds of the last time entry was accessed.
      
    **default**
      Specifies the default fields: **size**, **type**, **mtime**, **mode**,
      and **name**. This is the default set of fields reported if no
      **-fields** switch is specified.

    **gid**
      Reports the numeric group id of the entry.
      
    **mode**
      Reports the mode and permissions of the entry.

    **mtime**
      Reports the time in seconds of the last time the entry was modified.

    **name**
      Reports the name of the entry.
      
    **size**
      Reports the size in bytes of the entry.
      
    **type**
      Reports the type of the entry. This may be either "file", "directory",
      "characterSpecial", "blockSpecial", "fifo", "link", or "socket".

    **uid**
      Reports the numeric user id of the entry.
      
    **longentry**
      Reports a string resembling the long style output of **ls -l**.
      
  **-listing** 
    Indicates to return the text listing.  This is similar to the output of
    the **ls** command in a sftp client.

  **-long** 
    Indicates that the attributes of the file and directories are returned
    in addition to their names.

  **-table** *tableName*  
    Specifies a **blt::datatable** object to be loaded with the directory
    entries.
    
  **-timeout** *seconds*  
    Discontinue retrieving the directory listing after the specified number of 
    seconds.

*sftpName* **dirtree** *path* *treeName* ?\ *switches* ... ?
  Loads the contents of the directory described by *path* on the remote
  SFTP server into *treeName*. *TreeName* is the name of a *tree object*
  (see the **blt::tree** manual entry).  Switches can be any of the
  following.

  **-cancel** *varName*  
    Specifies the name of a TCL variable to terminate the operation.
    If *varName* is set, the **dirtree** operation is discontinued.

  **-depth** *numLevels*  
    Descend at most *numLevels* levels of subdirectories.  If *numLevels*
    is "0", then only *path* itself is loaded.  If *numLevels* is "-1",
    there is no limit. The default is "-1".

  **-fields** *fieldList*  
    Specifies the fields to reported.  *FieldList* is a TCL list that may
    contain one of more of the following field names.

    **all**
      Specifies all fields.

    **atime**
      Reports the time in seconds of the last time entry was accessed.
      
    **default**
      Specifies the default fields: **size**, **type**, **mtime**, **mode**,
      and **name**. This is the default set of fields reported if no
      **-fields** switch is specified.

    **gid**
      Reports the numeric group id of the entry.
      
    **mode**
      Reports the mode and permissions of the entry.

    **mtime**
      Reports the time in seconds of the last time the entry was modified.

    **name**
      Reports the name of the entry.
      
    **size**
      Reports the size in bytes of the entry.
      
    **type**
      Reports the type of the entry. This may be either "file", "directory",
      "characterSpecial", "blockSpecial", "fifo", "link", or "socket".

    **uid**
      Reports the numeric user id of the entry.
      
    **longentry**
      Reports a string resembling the long style output of **ls -l**.

  **-overwrite** 
    If true, overwrite any entries that already exist in the tree.  By default,
    duplicate entries are added.

  **-root** *rootNode*  
    Specifies the node of *tree* to load the directory entries from the 
    remote server.  The default is the root of the tree.

  **-timeout** *seconds* 
    Discontinue retrieving the directory listing after the specified number
    of seconds.

*sftpName* **exec** *cmdString* 
  Executes a Unix shell command on the remote system.  The output of
  *cmdString* will be the returned.

*sftpName* **exists** *path* 
  Return "1" is the file or directory *path* exists on the 
  remote SFTP server and "0" otherwise.

*sftpName* **get** *path*  ?\ *file*\ ? ?\ *switches* ... ?
  Transfers *path* from the remote SFTP server to the local system.
  If the *file* argument is present, this will be the name of the file
  on the local system, otherwise the remote name is used.  *Switches*
  can be any of the following.

  **-cancel** *varName* 
    Specifies the name of a TCL variable to terminate the operation.
    If *varName* is set, the **get** operation is discontinued.

  **-maxsize** *numBytes*  
    Specifies the maximum number of bytes to transfer. * If the size of
    *path* is greater then *numBytes*, then the local file will
    be truncated.

  **-progress** *cmdPrefix*  
    Specifies a TCL command to be invoked periodically as data from
    *path* is transferred.  Two arguments are appended to *cmdPrefix*:
    the number of bytes read and the size of the remote file.

  **-resume**   
    Indicates that if the local file exists and is smaller than the remote
    file, the local file is presumed to be a partially transferred copy of
    the remote file and the transfer is continued from the apparent point of
    failure.  This command is useful when transferring very large files over
    networks that are prone to dropping connections.

  **-timeout** *seconds* 
    Discontinue transferring the file the specified number of seconds.

*sftpName* **groups** ?\ *gid*\ ?
  Returns a list of the groups of which the remote user is a member.  The
  list will contain pairs of the numeric group id and group name. If a
  *gid* argument is present, then only the group name associated with that
  group id is returned. *Gid* is a numeric group id.

*sftpName* **isdirectory** *path* 
  Return "1" if *path* is a directory on the remote server and "0"
  otherwise.

*sftpName* **isfile** *path* 
  Return "1" if *path* is a file on the remote server and "0" otherwise.

*sftpName* **lstat** *path* *varName*
  Similar to the **stat** operation (see below) except that if *path* refers
  to a symbolic link the information returned is for the link rather than the
  file it refers to. *VarName* is name of a TCL array variable.
  The array will contain the following keys and values.

  **atime**
    The time in seconds of the last time *path* was accessed.

  **gid**
    The numeric group id of *path*.

  **mode**
    The mode and permissions of *path*.

  **mtime**
    The time in seconds of the last time *path* was modified.

  **size**
    The size in bytes of *path*.

  **type**
    Reports the type of *path*. This may be either "file", "directory",
    "characterSpecial", "blockSpecial", "fifo", "link", or "socket".

  **uid**
    Reports the numeric user id of *path*.
      

*sftpName* **mkdir** *path* ?\ *switches* ... ?
  Creates each a directory specified by *path*.  Directories for *path* as
  well as all non-existing parent directories will be created. It is not an
  error if the directory *path* already exists, but trying to overwrite an
  existing file with a directory will result in an error.  *Switches*
  can be any of the following.

  **-mode** *mode*  
    Specifies the permissions for the newly created directory.

*sftpName* **mtime** *path* ?\ *time*\ ?
  Returns a decimal string giving the time at which file name was last
  modified. If *time* is specified, it is a modification time to set for the
  file. The time is measured in the standard POSIX fashion as seconds from a
  fixed starting time (often January 1, 1970).  It's an error if the
  file does not exist on the server or its modified time cannot be queried.

*sftpName* **normalize** *path* 
  Returns a unique normalized path representation for *path*.

*sftpName* **owned** *path* 
  Returns "1" if *path* is owned by the current user, 0 otherwise.

*sftpName* **put** *file* ?\ *path*\ ? ?\ *switches* ... ? 
  Transfers *file* to the remote SFTP server.  *File* is a file on the local
  machine. If *path* is not specified, the remote file will be create ing
  in the current working directory on the remote and have the same name
  as *file* on the local machine.  It is an error if the remote file already
  exists or is a directory.  *Switches* can be any of the following.

  **-cancel** *varName*  
    Specifies the name of a TCL variable to terminate the operation.
    If *varName* is set, the **put** operation is discontinued.

  **-force**   
    If the remote file already exists, it will be overwritten.  By default,
    it is an error to overwrite a remote file.

  **-mode** *mode*  
    Specifies the permissions for the newly created file.

  **-progress** *cmdPrefix*  
    Specifies a TCL command to be invoked periodically as *path* is 
    being transferred.  Two arguments are appended to *cmdPrefix*:
    the number of bytes written and the size of the local file.

  **-resume**   
    Indicates that if the remote file exists and is smaller than the local
    file, the remote file is presumed to be a partially transferred copy of
    the local file and the transfer is continued from the apparent point of
    failure.  This command is useful when transferring very large files over
    networks that are prone to dropping connections.

  **-timeout** *seconds*  
    Discontinue transferring the file after the specified number of
    seconds.

*sftpName* **pwd**
  Returns the current working directory on the remote server.

*sftpName* **read** *path* ?\ *switches* ... ? 
  Returns the contents of *path*. *Path* is a file on the remote SFTP server.
  It is an error if *path* does not exist.  The following switches are
  available.

  **-cancel** *varName*  
    Specifies the name of a TCL variable to terminate the operation.
    If *varName* is set, the **read** operation is discontinued.

  **-maxsize** *numBytes*   
    Specifies the maximum number of bytes to transfer.  If the size of
    *path* is greater then *numBytes*, then the local copy will be
    truncated.

  **-progress** *cmdPrefix* 
    Specifies a TCL command to be invoked periodically as *path* is 
    being transferred.  Two arguments are appended to *cmdPrefix*:
    the number of bytes read and the size of the remote file.

  **-timeout** *seconds*   
    Discontinue retrieving the file after the specified number of 
    seconds.

*sftpName* **readable** *path*
  Returns "1" if *path* is readable by the current user, 0 otherwise.  It is
  an error if *path* does not exist on the server.

*sftpName* **readlink** *path*
  Returns the value of the symbolic link given by *path* (i.e. the name of the
  file it points to).  It's an error if *path* is not a symbolic link or
  its value cannot be read.

*sftpName* **rename** *old* *new* ?\ *-force*\ ?
  Renames or moves the file or directory *old* to *new*.  

*sftpName* **rmdir** *path* 
  Removes the directory specified by *path*. The directory must be empty.

*sftpName* **size** *path* 
  Returns the size of in bytes of *path*. It is an error if *path*
  does not exist on the server.

*sftpName* **slink** *path* *link*
  Creates a symbolic link on the remote *link* that links to *path*.
  It is an error if *path* does not exist on the server.

*sftpName* **stat** *path* *varName*
  Fills *varName* with the attributes of *path*.  *VarName* is name of a
  TCL array variable.  The array will contain the following keys and
  values.

  **atime**
    The time in seconds of the last time *path* was accessed.

  **gid**
    The numeric group id of *path*.

  **mode**
    The mode and permissions of *path*.

  **mtime**
    The time in seconds of the last time *path* was modified.

  **size**
    The size in bytes of *path*.

  **type**
    The type of *path*. This may be either "file", "directory",
    "characterSpecial", "blockSpecial", "fifo", "link", or "socket".

  **uid**
    The numeric user id of *path*.
      
*sftpName* **type** *path*
  Returns a string representing the type of *path*: "file", "directory",
  "characterSpecial", "blockSpecial", "fifo", "link", or "socket".  It is
  an error if *path* does not exist on the server.

*sftpName* **writable** *path*
  Returns "1" if *path* is writable by the current user, 0 otherwise.  It is
  an error if *path* does not exist on the server.

*sftpName* **write** *path* *string* ?\ *switches* ... ?
  Writes *string* to a file on the remote SFTP server.  *Path* is a file on
  the remote machine.  It is an error if the remote file is a directory.
  *Switches* may be any of the following.

  **-append**   
    Append the data to the remote file instead of overwriting it.

  **-cancel** *varName*   
    Specifies the name of a TCL variable to terminate the operation.
    If *varName* is set, the **write** operation is discontinued.

  **-progress** *cmdPrefix*  
    Specifies a TCL command to be invoked periodically as *path* is 
    being transferred.  Two arguments are appended to *cmdPrefix*:
    the number of bytes written and the size of the local file.

  **-timeout** *seconds*   
    Discontinue transferring the file after the specified number of
    seconds.

EXAMPLE
-------

The following example creates a BLT Datatable and loads the remote
directory listing into it.  This example assumes that we have a ssh public
key set up for myhost.org.

 ::
    
    package require BLT
    package require blt_sftp

    set table [blt::datatable create]
    set sftp [blt::sftp create -host myhost.org]
    $sftp dirlist ~ -table $table -fields all

    blt::sftp destroy $sftp

KEYWORDS
--------

sftp, datatable, tree

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

