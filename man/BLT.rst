
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

BLT is a library of extensions to the Tcl and Tk libraries.  It adds new
commands and variables to the application's interpreter.

TCL COMMANDS
------------

The following TCL commands are added to the interpreter from the BLT
library:

**blt::bgexec** 
  Works like TCL's **exec** command, running a pipeline of commands and
  allowing your TCL program to collect the results. The difference is that
  **blt::bgexec** executes in the background and doesn't block Tk events.
  This means you can execute a long-running command and Tk widgets will
  behave normally.  When the program finishes, its output and the exit
  status are written to TCL variables.  This makes it easy to monitor and
  save the output of a command.

**blt::datatable** 
  Creates datatable data objects.  A *datatable* object is table of cells.
  Row and columns both have indices and labels that address the cells.
  Cells may contain data or may be empty.

**blt::debug** 
  A simple TCL command tracing facility useful for debugging TCL code.
  Displays each TCL command before and after substitution along its level
  in the interpreter on standard error.

**blt::mesh** 
  The **blt::mesh** creates a 2-D mesh that can be used with the **-mesh**
  options of the BLT widgets.  Meshes can be shared between widgets.

**blt::sftp** 
  Creates sftp objects.  A sftp object lets you connect to a SFTP server to
  transfer files, get a directory listing etc.  Most operations that you
  can perform with a sftp client program can be done programmatically with
  a sftp object.

**blt::spline**
  Computes a natural (cubic) or quadratic spline fitting a set of data
  points (x and y vectors) and produces a vector of the interpolated images
  (y-coordinates) at a given set of absciccas (x-coordinates).

**blt::timestamp** 
  The **blt::timestamp** command is for converting date/time timestamps
  to/from double precision numbers.  It handles timestamps with fractional
  seconds, IS08601 time formats (separated by the letter 'T'), and work
  week dates.  This command is used internally by the **blt::datatable**
  data object to parse timestamps where the format of the timestamp isn't
  known.

**blt::tree** 
  The **blt::tree** command creates tree data objects.  A *tree object* is
  general ordered tree of nodes.  Each node has both a label and a
  key-value list of data.  Data can be heterogeneous, since nodes do not
  have to contain the same data fields.  It is associated with a TCL
  command that you can use to access and modify the its structure and
  data. Tree objects can also be managed via a C API.

**blt::utils::compare** 
  These utility commands are used by the **treeview** widget to filter
  rows.  The **number** operations handle cases where the floating point
  numbers are almost equal. The **string** operations provide switches to
  compactly handle different cases.

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

**blt::arcball** 
  The **blt::arcball** is a tool to rotate objects with the mouse
  naturally.  It provides an intuitive user interface for complex 3D object
  rotations via a simple, virtual sphere; the screen space analogy to the
  "arcball" input device.  The basic principle is to create a sphere around
  the object, and allow the user to click a point on the sphere and drag
  that point to a different location on the screen, having the object
  rotate to follow it.

**blt::background** 
  The **blt::background** creates gradient, tiled, or textured backgrounds
  that can be used with the **-background** options of the BLT widgets.

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
  The **blt::combobutton** command creates and manages *combobutton*
  widgets.  A *combobutton* widget displays button, that when pressed,
  posts a **blt::combomenu** widget.

**blt::comboeditor** 
  The **blt::comboeditor** command creates and manages *comboeditor*
  widgets.  A *comboeditor* is a popup text editor for quick edits of text.
  It was designed for use with bigger widgets like the **blt::treeview**
  and **blt::tableview** widgets. If contains an edit-able text area and
  optional scrollbars (vertical and/or horizontal).  The scrollbars are
  automatically exposed or hidden as necessary when the *comboeditor*
  widget is resized.  Whenever the *comboeditor* window is smaller
  horizontally and/or vertically than the actual text area, the appropiate
  scrollbar is exposed.

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

**blt::listview** 

**blt::paintbrush** 

**blt::palette** 

**blt::scrollset** 

**blt::paneset** 

**blt::stripchart** 

**blt::table** 
  A table geometry manager for Tk.  You specify window placements as table
  row and column positions. Windows can also span multiple rows or columns.
  It also has many options for setting and/or bounding window sizes.
  The manual is here_.

..  _here: file:://table.html

**blt::tableview** 

**blt::tabset** 
  The **blt::tabset** widget displays a series of tabbed folders where only
  one folder at a time is displayed. A folder can contain a Tk widget that
  is displayed when the folder is displayed.

**blt::tk:pushbutton** 

**blt::tk:button** 

**blt::tk:checkbutton** 

**blt::tk:radiobutton** 

**blt::tk:frame** 

**blt::tk:scrollbar** 

**blt::tk:toplevel** 

**blt::treeview** 
  The **blt::treeview** widget displays hierarchical data as a tree.  Data
  is represented as nodes in a general-ordered tree.  Each node can have
  sub-nodes and these nodes can in turn can have their own children.  The
  tree and it data is displayed as a table: each row of the table
  represents a node in the tree.  The tree (hierarchical view) is displayed
  in its own column.  Extra columns may be display data fields on either
  side.

**blt::winop** 
  The **blt::winop** command performs various operations on windows (Tk or
  foreign) using low-level windowing system function calls to work around
  window manager pecularities.  You can query the pointer, raise and lower
  windows, get the window hierarchy, etc.

IMAGE TYPES
-----------

**picture** 
  The **picture** is an image type for Tk. It is for full color images
  (32-bit pixels) with or without transparency.  Each color component in a
  picture is eight bits and there is an 8-bit alpha channel.  Image data
  for a picture image can be obtained from a file or a string, or it can be
  supplied from C code through a procedural interface.  Many image formats
  are supported (JPEG, GIF, TGA, BMP, TIFF, ICO, PDF, PS, etc.) as well as
  a number of operations that can be performed on the image such as
  resizing (through resampling).

CANVAS ITEMS
------------

**eps**
  The **eps** canvas item lets you place encapulated PostScript (EPS) on a
  canvas, controlling its size and placement.  The EPS item is displayed
  either as a solid rectangle or a preview image.  The preview image is
  designated in one of two ways: 1) the EPS file contains an ASCII
  hexidecimal preview, or 2) a Tk photo or BLT picture image.  When the
  canvas generates PostScript output, the EPS will be inserted with the
  proper translation and scaling to match that of the EPS item. So can use
  the canvas widget as a page layout tool.

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
