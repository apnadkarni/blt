
===============
blt::mesh
===============

---------------------------
Create and manage 2D meshes
---------------------------

:Author: George A Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::mesh cget** *meshName* ?\ *option*\ ?

**blt::mesh configure** *meshName* ?\ *option* *value* ...\ ?

**blt::mesh create** *type* ?\ *meshName*\ ? ?\ *option* *value* ...\ ?

**blt::mesh delete**  ?\ *meshName* ...\ ?

**blt::mesh hide** *meshName* ?\ *index ... ?

**blt::mesh hull** *meshName* ?\ *-vertices*\ ?

**blt::mesh names** ?\ *pattern* ...\ ?

**blt::mesh triangles** *meshName* 

**blt::mesh type** *meshName* 

**blt::mesh vertices** *meshName* 

DESCRIPTION
-----------

The **blt::mesh** creates a 2D mesh that can be used
with the **-mesh** options of the BLT widgets.  

INTRODUCTION
------------

Normally the mesh of a Tk widget is specified by color name
that specifies a solid color for the mesh.  The **blt::mesh**
command lets you defined different types of mesh (for example a
gradient), that you can use with the BLT widgets.  

A mesh can have one of the following types: 

  **cloud**

    A cloud *mesh* is point cloud.  There is not inferred topology.
    The mesh is generated using a 2D voronoi.

  **irregular**

    A color *mesh* object draws a single color.
    
  **regular**

    A regular *mesh* has uniform steps for both x and y axes.
    
  **triangle**

    A triangle *mesh* specifies the vertices of the triangle and the
    indices that form each triangle of the mesh.

OPERATIONS
----------

The following operations are available for the **blt::mesh** command:

**blt::mesh cget** *meshName* *option*

  Returns the current value of the *mesh* configuration option given
  by *option*. *MeshName* is the name of *mesh* object returned by the
  **create** operation. *Option* and may have any of the values accepted by
  the **configure** operation. They are specific to the type of mesh
  for *meshName*. They are described in the **create** operations below.

**blt::mesh configure** *meshName* ?\ *option* *value* ...\ ?

  Queries or modifies the *mesh* configuration options for
  *meshName*. *MeshName* is the name of *mesh* object returned by the
  **create** operation.  *Option* and *value* are specific to the type
  of *meshName*.  If no options are specified, a list describing all of the
  available options for *meshName* (see **Tk_ConfigureInfo** for information
  on the format of this list) is returned.  If *option* is specified with
  no *value*, then this command returns a list describing the one named
  option (this list will be identical to the corresponding sublist of the
  value returned if no *option* is specified).  If one or more *option*\
  -*value* pairs are specified, then this command modifies the given widget
  option(s) to have the given value(s); in this case the command returns
  the empty string.  *Option* and *value* can any of the values accepted by
  the **create** operation.

**blt::mesh create regular** ?\ *option* *value* ...\ ?

  Creates a checker *mesh* object. Radial gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *mesh* object.  The name of the
  *mesh* is automatically generated in the form "mesh0",
  "mesh1", etc.  The name of the new *mesh* is
  returned. *Option* and *value* are specific to "linear" meshs and
  are listed below.

  **-x** *dataSource*

  **-y** *dataSource*

    Specifies the border color of the mesh object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
**blt::mesh create irregular** ?\ *option* *value* ...\ ?

  Creates a new conical gradient *mesh* object. Conical gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *mesh* object.  The name of the
  *mesh* is automatically generated in the form "mesh0",
  "mesh1", etc.  The name of the new *mesh* is
  returned. *Option* and *value* are specific to "linear" meshs and
  are listed below.

  **-x** *dataSource*

  **-y** *dataSource*

    Specifies the border color of the mesh object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
**blt::mesh create cloud** ?\ *option* *value* ...\ ?

  Creates a new linear gradient *mesh* object. Linear gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *mesh* object.  The name of the
  *mesh* is automatically generated in the form "mesh0",
  "mesh1", etc.  The name of the new *mesh* is
  returned. *Option* and *value* are specific to "linear" meshs and
  are listed below.

  **-x** *dataSource*

  **-y** *dataSource*

    Specifies the border color of the mesh object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
**blt::mesh create triangle** ?\ *option* *value* ...\ ?

  Creates a new radial gradient *mesh* object. Radial gradients are
  defined by an axis (the gradient line segment) with each point on it
  interpolated to a specific color. The lines perpendicular to the gradient
  line have the same color as the point is crosses the gradient line.
  
  This command returns the name of *mesh* object.  The name of the
  *mesh* is automatically generated in the form "mesh0",
  "mesh1", etc.  The name of the new *mesh* is
  returned. *Option* and *value* are specific to "linear" meshs and
  are listed below.

  **-x** *dataSource*

  **-y** *dataSource*

    Specifies the border color of the mesh object.  If a widget
    has a 3D relief, this specifies the colors of the bevels. 
    
  **-triangle** *indices*

**blt::mesh delete** ?\ *meshName* ...\ ?

  Releases resources allocated by the mesh command for *window*, including
  the mesh window.  User events will again be received again by *window*.
  Resources are also released when *window* is destroyed. *Window* must be
  the name of a widget specified in the **create** operation, otherwise an
  error is reported.

**blt::mesh hide** *meshName* ?\ *index* ... ?

  Hides one or more triangles designated by *index*.  *MeshName* is the
  name of a mesh created by the **create** operation.

**blt::mesh hull** *meshName* ?\ *-vertices*\ ?

  Returns the indices of the vertices of the convex hull forming the
  boundary *meshName*. *MeshName* is the name of a mesh created by the
  **create** operation.

**blt::mesh names** ?\ *pattern* ...\ ?

  Returns the names of all the meshs currently created.  If one or
  more *pattern* arguments are provided, then the name of any mesh
  matching *pattern* will be returned. *Pattern* is a glob-style pattern.

**blt::mesh triangles** *meshName*

  Returns the indices of the triangles of the mesh for *meshName*.
  *MeshName* is the name of a mesh created by the **create** operation.

**blt::mesh type** *meshName*

  Returns the type of the mesh for *meshName*.  *MeshName* is the
  name of a mesh created by the **create** operation.

**blt::mesh vertices** *meshName*

  Returns the vertices of *meshName*.  *MeshName* is the name of a mesh
  created by the **create** operation.

EXAMPLE
-------

Create a *mesh* object with the **blt::mesh** command.

 ::
    package require BLT

    # Create a new regular mesh.
    blt::mesh create regular myMesh \
        -x { 0 10 10 } \
	-y { 0 10 10 } 
        
Now we can create widgets that use the mesh.

 ::

    blt::contour .graph
    .graph element create elem1 -mesh myMesh

To remove the mesh, use the **delete** operation.

 ::

    blt::mesh delete myMesh
     
Please note the following:

1. The meshes created by the **blt::mesh** command are only recognized by
   BLT widgets.

2. If you change a mesh option (such as **-x**) the widgets using the mesh
   object may automatically be notified.

3. Meshes are reference counted.  If you delete a mesh, its resources are
   not freed until no widget is using it.
   
KEYWORDS
--------

mesh


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
