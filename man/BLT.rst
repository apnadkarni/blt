
===
BLT
===

--------------------------------
Introduction to the BLT library.
--------------------------------

:Author: George A. Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

DESCRIPTION
-----------

BLT is a library of extensions to the Tk library.  It adds new commands and
variables to the application's interpreter.

TCL COMMANDS
------------

The following commands are added to the interpreter from the BLT library:

**blt::bgexec** 
  Like TCL's **exec** command, **blt::bgexec** runs a pipeline of Unix
  commands in the background.  Unlike TCL **exec**, the output of the last
  process is collected and a global TCL variable is set upon its
  completion.  **blt::bgexec** can be used with **tkwait** to wait for Unix
  commands to finish while still handling expose events.  Intermediate
  output is also available while the pipeline is active.

**blt::datatable** 
  Creates datatable data objects.  A *datatable* object is table of cells.
  Row and columns both have indices and labels that address the cells.
  Cells may contain data or may be empty.

**blt::debug** 
  A simple TCL command tracing facility useful for debugging TCL code.
  Displays each TCL command before and after substitution along its level
  in the interpreter on standard error.

**blt::mesh** 

**blt::sftp** 
  Creates sftp objects.  A sftp object lets you connect to a SFTP server to
  transfer files, get a directory listing etc.  Most operations that you
  can perform with a sftp client program can be done programmatically with
  a sftp object.

**blt::spline**
  Computes a spline fitting a set of data points (x and y vectors) and
  produces a vector of the interpolated images (y-coordinates) at a given
  set of x-coordinates.

**blt::timestamp** 
  Converts date/time timestamps to/from double precision numbers.  It
  handles timestamps with fractional seconds, IS08601 time formats
  (separated by the letter 'T'), and work week dates.  

**blt::tree** 
  Creates tree data objects.  A *tree object* is general ordered tree of
  nodes.  Each node has both a label and a key-value list of data.  

**blt::utils::compare** 

**blt::utils::crc32** 

**blt::utils::csv** 

**blt::vector** 
  Creates a vector of floating point values.  The vector's components can
  be manipulated in three ways: through a TCL array variable, a TCL
  command, or the C API.

**blt::watch** 
  Arranges for TCL procedures to be called before and/or after the
  execution of every TCL command. This command may be used in the logging,
  profiling, or tracing of TCL code.

TK COMMANDS
------------

**blt::background** 
  Creates specialized backgrounds that can be used with the **-background**
  options of the BLT widgets.

**blt::barchart** 
  A barchart widget.  Plots two-variable data as rectangular bars in a
  window.  The x-coordinate values designate the position of the bar along
  the x-axis, while the y-coordinate values designate the magnitude.  The
  **blt::barchart** widget has of several components; coordinate axes,
  crosshairs, a legend, and a collection of elements and tags.

**blt::beep** 

**blt::bitmap** 
  Reads and writes bitmaps from TCL.  New X bitmaps can be defined on-the-fly
  from TCL, obviating the need to copy around bitmap files.  Other options
  query loaded X bitmap's dimensions and data.

**blt::busy** 
  Creates a "busy window" which prevents user-interaction when an application
  is busy.  The busy window also provides an easy way to have temporary busy
  cursors (such as a watch or hourglass).

**blt::combobutton** 

**blt::comboentry** 

**blt::combolist** 

**blt::combomenu** 

**blt::combotree** 

**blt::contour** 

**blt::cutbuffer** 
  Lets you to read or modify the eight X cut buffer properties. You can
  also rotate the buffers properties.

**blt::drag&drop**
  Provides a drag-and-drop facility for Tk.  Information (represented by a
  token window) can be dragged to and from any Tk window, including those of
  another Tk application.  **blt::drag&drop** acts as a coordinator,
  directing Tk **send** commands between (or within) TCL/Tk applications.

**blt::drawerset** 

**blt::filmstrip** 

**blt::graph** 
  A 2D plotting widget.  Plots two variable data in a window with an
  optional legend and annotations.  It has of several components;
  coordinate axes, crosshairs, a legend, and a collection of elements and
  tags.

**blt::htext** 
  A simple hypertext widget.  Combines text and Tk widgets into a single
  scroll-able window.  TCL commands can be embedded into text, which are
  invoked as the text is parsed.  In addition, Tk widgets can be
  appended to the window at the current point in the text.  **blt::htext**
  can be also used to create scrolled windows of Tk widgets.

**blt::paintbrush** 

**blt::palette** 

**blt::scrollset** 

**blt::paneset** 

**blt::stripchart** 

**blt::table** 
  A table geometry manager for Tk.  You specify window placements as table
  row,column positions and windows can also span multiple rows or columns.
  It also has many options for setting and/or bounding window sizes.
  The manual is here_.

..  _here: file:://table.html

**blt::tableview** 

**blt::tabset** 

**blt::tk:pushbutton** 

**blt::tk:button** 

**blt::tk:checkbutton** 

**blt::tk:radiobutton** 

**blt::tk:frame** 

**blt::tk:scrollbar** 

**blt::tk:toplevel** 

**blt::treeview** 

**blt::winop** 
  Raise, lower, map, or, unmap any window.  The raise and lower functions
  are useful for stacking windows above or below "busy windows".


**picture** 

**eps**

VARIABLES
---------

The following TCL variables are either set or used by BLT at various times
in its execution:

**blt_library**
  This variable contains the name of a directory containing a library of
  TCL scripts and other files related to BLT.  Currently, this directory
  contains the **blt::drag&drop** protocol scripts and the PostScript
  prolog used by **blt::graph** and **blt::barchart**.  The value of this
  variable is taken from the **BLT_LIBRARY** environment variable, if one
  exists, or else from a default value compiled into the BLT library.

**blt_versions** 
  This variable is set in the interpreter for each application. It is an
  array of the current version numbers for each of the BLT commands in the
  form *major*\ .*minor*\ .  *Major* and *minor* are integers.  The major
  version number increases in any command that includes changes that are
  not backward compatible (i.e. whenever existing applications and scripts
  may have to change to work with the new release).  The minor version
  number increases with each new release of a command, except that it
  resets to zero whenever the major version number changes.  The array is
  indexed by the individual command name.

ADDING BLT TO YOUR APPLICATIONS
-------------------------------

It's easy to add BLT to an existing Tk application.  BLT requires no
patches or edits to the TCL or Tk libraries.  To add BLT, simply add the
following code snippet to your application's tkAppInit.c file.

  ::

    if (Blt_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }

Recompile and link with the BLT library (libBLT.a) and that's it.

Alternately, you can dynamically load BLT, simply by invoking the
command

  ::

     package require BLT

from your TCL script.

KEYWORDS
--------

BLT

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
