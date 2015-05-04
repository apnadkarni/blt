============
blt::arcball
============

---------------------------------
Create and manage arcball objects
---------------------------------

:Author: George A Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::arcball create** ?\ *arcballName*\ ?

**blt::arcball destroy** ?\ *arcballName* ... ?

**blt::arcball names** ?\ *pattern*\ ?

DESCRIPTION
-----------

The **blt::arcball** command creates an arcball object.  A *arcball* object
represents the rotation of a 3-D object. Mouse input is used to compute a
rotation axis and an angular velocity. This idea is based on the ArcBall
Rotation Control technique presented in the book Graphics Gems IV by Ken
Shoemake.

  Ken Shoemake, Arcball Rotation Control, p. 175-192, code: p. 178-191
  Graphics Gems IV editor: Paul S. Heckbert, Academic Press, 1994.

 
INTRODUCTION
------------

The arcball is a tool to rotate objects with the mouse naturally.  It
provides an intuitive user interface for complex 3D object rotations via a
simple, virtual sphere – the screen space analogy to the "arcball" input
device.  The basic principle is to create a sphere around the object, and
allow the user to click a point on the sphere and drag that point to a
different location on the screen, having the object rotate to follow it.

A *arcball object* uses 4 numbers for an unit quaternion to represent the
rotation of an object.  Compared to Euler angles they are simpler to
compose and avoid the problem of gimbal lock. Compared to rotation matrices
they are more numerically stable and may be more efficient.

SYNTAX
------

**blt::arcball create** ?\ *arcballName*\ ?  ?\ *option value* ... ?
  Creates a new arcball object.  The name of the new arcball object is returned.
  If no *arcballName* argument is present, then the name of the arcball is
  automatically generated in the form "arcball0", "arcball", etc.  If the
  substring "#auto" is found in *arcballName*, it is automatically substituted
  by a generated name.  For example, the name ".foo.#auto.bar" will be
  translated to ".foo.arcball0.bar".

  A new TCL command (by the same name as the arcball) is created.  Another
  TCL command or arcball object can not already exist as *arcballName*.  If the
  TCL command is deleted, the arcball will also be freed.  Note that
  arcballs are by default, created in the current namespace, not the global
  namespace, unless *arcballName* contains a namespace qualifier, such as
  "fred::myArcball".

  If one or more *option-value* pairs are specified, they modifies the given
  arcball option(s) to have the given value(s). *Option* and *value* are
  described in the **configure** operation below.

**blt::arcball destroy** ?\ *arcballName* ... ?
  Frees one of more arcballs.  The TCL command associated with *arcballName*
  is also removed.  

**blt::arcball names** ?\ *pattern*\ ?
  Returns the names of all arcball objects.  if a *pattern* argument is
  given, then the only those arcballs whose name matches pattern will be
  listed.

ARCBALL OPERATIONS
------------------

After you create an *arcball* object, you can use its TCL command to query or
modify it.  The general form is

  *arcballName* *operation* ?\ *arg*\ ?...

Both *operation* and its arguments determine the exact behavior of the
command.  The operations available for arcballs are listed below.

*arcballName* **cget** *option*  
  Returns the current value of the arcball configuration option given by
  *option*. *Option* may have any of the values accepted by the
  **configure** operation. They are described below.

*arcballName* **configure** ?\ *option*\ ? ?\ *value*? ?\ *option value ...*\ ?
  Queries or modifies the configuration options of the arcball.  If no
  *option* is specified, this command returns a list describing all the
  available options for *arcballName* (see **Tk_ConfigureInfo** for
  information on the format of this list).  If *option* is specified with
  no *value*, then a list describing the one named option (this list will
  be identical to the corresponding sublist of the value returned if no
  *option* is specified) is returned.  If one or more *option-value* pairs
  are specified, then this command modifies the given widget option(s) to
  have the given value(s); in this case the command returns an empty
  string.  *Option* and *value* are described below.

  **-height** *numPixels* 
    Specifies the height of the viewing area.  *NumPixels* may have any of
    the forms acceptable to **Tk_GetPixels**.  The default is "100".

  **-width** *numPixels* 
    Specifies the width of the viewing area.  *NumPixels* may have any of
    the forms acceptable to **Tk_GetPixels**.  The default is "100".

*arcballName* **euler** ?\ *eulerAngles*\ ?
  Gets or sets the quaternion in terms of euler angles. *EulerAngles* is a
  list of 3 numbers representing the x, y, and z rotation in degrees.

  In the conversion from euler angles to the quaternion you should note
  that the two following assumptions:

  1. The order in which the angles are applied is z, y, x.
  2. The z-axis points up.

*arcballName* **matrix** ?\ *rotationMatrix*\ ?
  Gets or sets the quaternion in terms of a rotation matrix.
  *RotationMatrix* is a list of 9 numbers, representing the 3-by-3 rotation
  matrix.  The matrix is ordered row-major.

*arcballName* **quaternion** ?\ *quaternion*\ ?
  Gets or sets the quaternion associated with the *arcball*. *Quaternion*
  is a list of 4 numbers representing the quaternion of the *arcball*.

*arcballName* **reset** 
  Resets the quaternion to identity.

*arcballName* **resize** *width* *height*
  Sets new dimensions for the *arcball*.  These dimensions represent
  the bounds of the *arcball*.  Both *width* and *height* may have
  any of the forms acceptable to **Tk_GetPixels**, such as "1.2i".

*arcballName* **rotate** *x1* *y1* *x2* *y2*
  Rotates the *arcball* given the screen coordinates. *X1* and *y1* are the
  starting x-y coordinates of the rotation.  *X2* and *y2* are the ending
  coordinates.  The rotated quaternion is returned as a list of 4 numbers.

EXAMPLE
-------

You create an arcball with the **blt::arcball** command.  There are
optionally arguments to set the width and height of the viewing area.
You can also resize the area with the **resize** operation.

 ::

   set arcball [blt::arcball create -width 100 -height 100]

This creates a new TCL command that we save in the variable "arcball".
You can use this command to set the arcball's quaternion.  You can set
the quaternion with the **euler**, **matrix**, or **quaternion** operations.

 ::

   set qw 0.853553
   set qx -0.353553
   set qy 0.353553
   set qz 0.146447
   set q [list $qw $qx $qy $qz]
   $arcball quaternion $q


If the viewing area size changes you have to update the *arcball* object
using the **configure** or **resize** operations.

 ::

   bind .widget <Configure> {
       $arcball configure -width [winfo width %W] -height [winfo height %W]
   }


The **rotate** operation computes the new quaternion based on the motion
of the mouse pointer.  

 ::

   bind .widget <ButtonPress-1> {
      set click(x) %x
      set click(y) %y
   }
   bind .widget <B1-Motion> {
      set q [$arcball rotate %x %y $click(x) $click(y)]
      set click(x) %x
      set click(y) %y
      .widget rotate $q 
   }

In this case, we're assuming that our widget understands how to rotate
based on a unit quaternion.  Alternatively, we could get the euler angles
or the rotation matrix.

Finally, we remove the arcball.  This will also remove the associated TCL
command.

::
    
   blt::arcball destroy $arcball

KEYWORDS
--------

arcball

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
