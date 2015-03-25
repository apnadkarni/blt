===============
blt::sftp
===============

-------------------------------------------------
Transfer files to/from SFTP server.
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

**blt::sftp create** ?\ *sftpName*\ ?  ?\ *switches*\...? 

**blt::sftp destroy** *sftpName*...

**blt::sftp names**  ?\ *pattern*\ ?

DESCRIPTION
===========

The **sftp** command creates sftp objects.  A sftp object
lets you connect to a SFTP server to transfer files, get
a directory listing etc.  Most operations that you can perform with a
sftp client program, you can do programmatically with a sftp object.

SYNTAX
======

**blt::sftp create** ?\ *sftpName*\ ? ?\ *switches*...\ ?  

  Creates a new sftp object.  The name of the new sftp object is returned.  If
  no *sftpName* argument is present, then the name of the sftp is
  automatically generated in the form "sftp0", "sftp1", etc.  If the
  substring "#auto" is found in *sftpName*, it is automatically
  substituted by a generated name.  For example, the name ".foo.#auto.bar"
  will be translated to ".foo.sftp0.bar".

  A new TCL command by the same name as the sftp object is also created.
  Another TCL command or sftp object can not already exist as *sftpName*.  If
  the TCL command is deleted, the sftp will also be freed.  The new sftp will
  contain just the root node.  Sftp objects are by default, created in the
  current namespace, not the global namespace, unless *sftpName* contains a
  namespace qualifier, such as "fred::mySftp".

  The following switches are available.

  **-user** *string*  

    Specifies the username of the remote SFTP account.

  **-host** *string* 

    Specifies the hostname of the remote SFTP server.

  **-password** *string* 

    Specifies the password of the user account on the remote SFTP server.

  **-prompt** *command* 

    Specifies a TCL script to be called whenever the user name or password of
    the user account on the remote SFTP is required. The script should return a
    list in the form "*username password*".

  **-publickey** *file* 

    Specifies the location of the public key file.  The default location
    is "$HOME/.ssh/id_rsa.pub".

  **-timeout** *seconds* 

    Specifies the idle timeout for the SFTP connection.  When there is no
    **sftp** operation performed after the specfied number of seconds, the
    connection is automatically dropped. The sftp object will automatically
    reconnect (if needed) on the next operation.  If *timeout* is zero, then
    no timeout will occur.  The default is "0".

**blt::sftp destroy** *sftpName*...

  Disconnects and destroys one of more sftp objects.  The TCL command
  associated with *sftpName* is also removed.

**blt::sftp names** ?\ *pattern*\ ?

  Returns the names of all sftp objects.  if a *pattern* argument
  is given, then the only those sftp objects whose name matches pattern will
  be listed.

PATHS
=====

Paths on the remote SFTP server may be relative or absolute. Initially the
current working directory is the home directory of the user account on the
SFTP server.

SFTP OPERATIONS
===============

Once you create a sftp object, you can use its TCL command 
to perform operations associated with the connection.  The general form is

*sftpName* *operation* ?\ *arg*\ ?...

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for sftps are listed below.

*sftpName* **atime** *path* ?\ *time*\ ?

  Changes or queriers the access time of the remote file or directory
  specified by *path*.  If a no *time* argument is provided the access time of
  *path* is returned.  *Time* is a decimal string giving the time at which
  *path* was last accessed. The time is measured in the standard POSIX fashion
  as seconds from a fixed starting time (often January 1, 1970).

*sftpName* **auth**

 Returns the type of authentication used to connect to the remote SFTP server.
 The possible types are "password" or "publickey".

*sftpName* **chdir** ?\ *path*\ ?

  Change to current working directory on the remote SFTP server to *path*.  If
  no *path* argument is provided the user's home directory is assumed.

*sftpName* **chgrp** *path* ?\ *gid*\ ? ?\ *-recurse*\ ?

  Queries or changes the group of the file of directory described by *path* on
  the remote SFTP server.  If *gid* is provided, then the group of *path* is
  changed.  *Gid* is the numeric id of the remote group.  If the *-recurse*
  flag is set and *path* is a directory, then the group of the subdirectories
  and files underneath *path* are also changed.

*sftpName* **chmod** *path* ?\ *mode*\ ? ?\ *-recurse*\ ?

  Queries or changes the permissions of the file of directory described by
  *path* on the remote SFTP server.  If *mode* is provided, then the mode of
  *path* is changed.  *Mode* can be in various forms similar to the Unix chmod
  command. If the *-recurse* flag is set and *path* is a directory, then the
  mode of the subdirectories and files underneath *path* are also changed.

*sftpName* **delete** *path* ?\ *switches*...\ ?

  Deletes the file or directory described by *path* on the remote SFTP server.
  The valid switches are listed below\:

  **-force**  
    Forces the deletion of directories that are not empty.

*sftpName* **dirlist** *path* ?\ *switches*\ ?

  Lists the contents of the directory described by *path* on the remote SFTP
  server.  The files and directories of *path* are returned as a list.  The
  valid switches are listed below\:

  **-fields** *list*  

    Specifies the field to reported.  *List* is a TCL list that may contain
    one of more of the following field names\: "atime", "gid", "mode",
    "mtime", "name", "size", "type", "uid", "longentry",
    "default", and "all".

  **-listing** *boolean*  

    If true, returns the text listing.  This is similar to the output of the
    "ls" command in a sftp client.

  **-long** *boolean*  

    If true, the attributes of the file and directories are returned
    in addition to their names.

  **-table** *tableName*  

    Specifies a **datatable** object to be loaded with the directory entries.

  **-timeout** *seconds*  

    Discontinue retrieving the directory listing after the specified number of 
    seconds.

*sftpName* **dirtree** *path* *tree* ?\ *switches*\ ?

  Loads the contents of the directory described by *path* on the remote SFTP
  server into *tree*. *Tree* is the name of a tree object. The following
  switches are available\:

  **-fields** *list*  

    Specifies the field to reported.  *List* is a TCL list that can 
    contain one or more of the following field names\:
    "atime", "gid", "mode", "mtime", "name", 
    "size", "type", "uid", "longentry", "default", and
    "all".

  **-cancel** *varName*  

    Specifies the name of a TCL variable that when set will terminate 
    the operation.

  **-depth** *number*  

    Descend at most *number* levels of directories.  
    If *number* is "0", then only *path* itself is loaded.
    If *number* is "-1", then there is now limit. The default
    is "-1".

  **-overwrite** *boolean*  

    If true, overwrite any entries that already exist in the tree.  By default,
    duplicate entires are added.

  **-root** *node*  

    Specifies at what node of *tree* to load the directory entries from the 
    remote server.  The default is the root of the tree.

  **-timeout** *seconds* 

    Discontinue retrieving the directory listing after the specified number of 
    seconds.

*sftpName* **exec** *command* 

  Executes a Unix shell command on the remote system.  The output of 
  *command* will be the returned.

*sftpName* **exists** *path* 

  Return "1" is the file or directory *path* exists on the 
  remote SFTP server and "0" otherwise.

*sftpName* **get** *path*  ?\ *file*\ ? ?\ *switches*\ ?

  Transfers *path* from the remote SFTP server to the local system.
  If the *file* argument is present, this is the name to create on 
  the local system, otherwise the remote name is used.  The following
  switches are available.

  **-cancel** *varName* 

    Specifies the name of a TCL variable that when set will terminate the
    operation.

  **-maxsize** *number*  

    Specifies the maximum number of bytes to transfer regardless of the size
    of the file.  If the size of *path* is greater then *number*, then
    the file is truncated.

  **-progress** *command*  

    Specifies a TCL command to be invoked periodically as *path* is 
    being transfered.  Two arguments are appended\: the number of bytes 
    read and the size of the remote file.

  **-resume**   

    Specifies that if the local file exists and is smaller than the remote
    file, the local file is presumed to be a partially transferred copy of
    the remote file and the transfer is continued from the apparent point of
    failure.  This command is useful when transferring very large files over
    networks that are prone to dropping connections.

  **-timeout** *seconds* 

    Discontinue retrieving the directory listing after the specified number of 
    seconds.

*sftpName* **groups** ?\ *gid*\ ?

  Returns a list of the groups that the remote user is a member.  The groups
  are returned as list of group id and group name pairs. If a *gid* argument
  is present, the only the group name associated with that group id is
  returned.

*sftpName* **isdirectory** *path* 

  Return "1" if *path* is a directory on the remote server and "0"
  otherwise.

*sftpName* **isfile** *path* 

  Return "1" if *path* is a file on the remote server and "0" otherwise.

*sftpName* **lstat** *path* *varName*

  Similar to the **stat** operation (see below) except that if *path* refers
  to a symbolic link the information returned is for the link rather than the
  file it refers to. *VarName* is name of a TCL variable, treated as an array
  variable. The following elements of that variable are set\: "atime",
  "gid", "mode", "mtime", "size", "type", and "uid".  Returns an
  empty string.

*sftpName* **mkdir** *path* ?\ *switches*\ ?

  Creates each a directory specified by *path*.  Directories for *path* as
  well as all non-existing parent directories will be created. It is not an
  error is the directory *path* already exists.  Trying to overwrite an
  existing file with a directory will result in an error.  The following
  switches are available.

  **-mode** *mode*  

    Specifies the permissions for the newly created directory.

*sftpName* **mtime** *path* ?\ *time*\ ?

  Returns a decimal string giving the time at which file name was last
  modified. If *time* is specified, it is a modification time to set for the
  file. The time is measured in the standard POSIX fashion as seconds from a
  fixed starting time (often January 1, 1970).  If the file does not exist or
  its modified time cannot be queried or set then an error is generated.

*sftpName* **normalize** *path* 

  Returns a unique normalized path representation for *path*.

*sftpName* **owned** *path* 

  Returns "1" if *path* is owned by the current user, 0 otherwise.

*sftpName* **put** *file* ?\ *path*\ ? ?\ *switches*\ ? 

  Transfers *file* to the remote SFTP server.  *File* is a file on the local
  machine. If *path* is not specified, the remote file is given the same name
  it has on the local machine.  It is an error if the remote file already
  exists or is a directory.  The following switches are valid.

  **-cancel** *varName*  

    Specifies the name of a TCL variable that when set will terminate 
    the operation.

  **-force**   

    If the remote file already exists, it will be overwritten.  It is normally
    an error to overwrite a remote file.

  **-mode** *mode*  

    Specifies the permissions for the newly created file.

  **-progress** *command*  

    Specifies a TCL command to be invoked periodically as *path* is 
    being transfered.  Two arguments are appended\: the number of bytes 
    written and the size of the local file.

  **-resume**   

    Specifies that if the remote file exists and is smaller than the local
    file, the remote file is presumed to be a partially transferred copy of
    the local file and the transfer is continued from the apparent point of
    failure.  This command is useful when transferring very large files over
    networks that are prone to dropping connections.

  **-timeout** *seconds*  

    Discontinue retrieving the directory listing after the specified number of 
    seconds.

*sftpName* **pwd**

  Returns the current working directory on the remote server.

*sftpName* **read** *path* ?\ *switches*\ ? 

  Returns the contents of *path*. *Path* is a file on the remote SFTP server.
  It is an error if *path* does not exist.  The following switches are
  available.

  **-cancel** *varName*  

    Specifies the name of a TCL variable that when set will terminate the
    operation.

  **-maxsize** *number*   

    Specifies the maximum number of bytes to transfer regardless of the size
    of the file.  If the size of *path* is greater then *number*, then
    the read is truncated.

  **-progress** *command* 

    Specifies a TCL command to be invoked periodically as *path* is 
    being transfered.  Two arguments are appended\: the number of bytes 
    read and the size of the remote file.

  **-timeout** *seconds*   

    Discontinue retrieving the file after the specified number of 
    seconds.

*sftpName* **readable** *path*

  Returns "1" if *path* is readable by the current user, 0 otherwise.  It is
  an error is *path* does not exist.

*sftpName* **readlink** *path*

  Returns the value of the symbolic link given by *path* (i.e. the name of the
  file it points to).  If *path* is not a symbolic link or its value cannot be
  read, then an error is returned.

*sftpName* **rename** *old* *new* ?\ *-force*\ ?

  Renames or moves the file or directory *old* to *new*.  

*sftpName* **rmdir** *path* 

  Removes the directory specified by *path*. The directory
  must be empty.  

*sftpName* **size** *path* 

  Returns the size of in bytes of *path*. An error is generated
  is *path* does not exist.

*sftpName* **slink** *path* *link*

  Returns the size of in bytes of *path*. An error is generated
  is *path* does not exist.

*sftpName* **stat** *path* *varName*

  Fills *varName* with the attributes of *path\fR.  *VarName* is name of a
  TCL variable that is treated as an array variable. The following elements of
  that variable are set\: "atime", "gid", "mode", "mtime", "size",
  "type", and "uid".

*sftpName* **type** *path*

  Returns a string representing the type of *path\fR: \f(CWfile*,
  "directory", "characterSpecial", "blockSpecial", "fifo", "link",
  or "socket".  It is an error is *path* does not exist.

*sftpName* **writable** *path*

  Returns "1" if *path* is writable by the current user, 0 otherwise.  It is
  an error is *path* does not exist.

*sftpName* **write** *path* *string* ?\ *switches*\ ?

  Writes *string* to a file on the remote SFTP server.  *Path* is a file on
  the remote machine.  It is an error if the remote file is a directory.  The
  following switches are valid.

  **-append**   

    Append the data to the remote file instead of overwriting it.

  **-cancel** *varName*   

    Specifies the name of a TCL variable that when set will terminate 
    the operation.

  **-progress** *command*  

   Specifies a TCL command to be invoked periodically as *path* is 
   being transfered.  Two arguments are appended: the number of bytes 
   written and the size of the local file.

  **-timeout** *seconds*   

    Discontinue retrieving the directory listing after the specified number of 
    seconds.

EXAMPLE
=======


KEYWORDS
========

sftp, datatable, tree
