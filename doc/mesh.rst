
===============
blt::mesh
===============

---------------------------
Create and manage 2D meshes
---------------------------

:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
========

**blt::mesh cget** *meshName* ?\ *option*\ ?

**blt::mesh configure** *meshName* ?\ *option* *value* ... ?

**blt::mesh create** *type* ?\ *meshName*\ ? ?\ *option* *value* ... ?

**blt::mesh delete**  ?\ *meshName* ... ?

**blt::mesh hide** *meshName* ?\ *index* ... ?

**blt::mesh hull** *meshName* ?\ *-vertices*\ ?

**blt::mesh names** ?\ *pattern* ... ?

**blt::mesh triangles** *meshName* 

**blt::mesh type** *meshName* 

**blt::mesh vertices** *meshName* 

DESCRIPTION
===========

The **blt::mesh** creates a 2-D mesh that can be used with the **-mesh**
options of the BLT widgets.  Meshes can be shared between widgets.

INTRODUCTION
============

Many types of data define a topology (a mesh) for one or more data
measurements.  For example, a contour plot uses 2-D mesh and interpolates
the data measurements (field values) based on that mesh.  It is not unusual
to have several different measurements at the same points of the mesh.
The **blt::mesh** command lets you define a mesh that you can also share
between different widgets.

A mesh can have one of the following input types: 

**cloud**
  The mesh is defined by a point cloud. A point cloud is unordered set of
  points.  A triangular mesh is computed using Delaunay triangulation
  from the points. A network of triangles is built over the existing
  vertices of the point cloud.  Field values associated with a *cloud*
  mesh should have the same ordering as the cloud points.

  Reference: Steve J. Fortune (1987) A Sweepline Algorithm for Voronoi
  Diagrams, Algorithmica 2, 153-174.

**irregular**
  The mesh is defined by a non-uniform rectilinear grid.  The coordinates
  of the grid lines the run along the X and Y axes are exactly
  specified. The coordinates do not have to be uniformly spaced.  Field
  values associated with an *irregular* mesh should follow the convention
  of x-coordinates changing fastest (row-major). For example in a 20x10
  non-uniform grid the order of the field values would be:

     ::

       (0,0), (0,1), (0,2) ... (0,19)
       (1,0), (1,1), (1,2) ... (1,19)
       ...
       (9,0), (9,1), (9,2) ... (9,19)


**regular**
  The mesh is defined by a uniform rectangular grid where the grid lines
  run along the X and Y axes. Each axis has 3 numbers that specify the
  minimum and maximum values, and the number of grid lines.  Field values
  associated with a *regular* mesh should follow the convention of
  x-coordinates changing fastest (row-major). For example in a 20x10 grid
  the order of the field values would be:

   ::

       (0,0), (0,1), (0,2) ... (0,19)
       (1,0), (1,1), (1,2) ... (1,19)
       ...
       (9,0), (9,1), (9,2) ... (9,19)


**triangle**
  The mesh is defined by a set of triangles that are connected by their
  common edges or corners.  The vertices of the triangles and the
  individual triangles (by the indices of the vertices) must be
  specified.  Field values associated with a *triangle* mesh should have
  the same ordering as the vertices.

DATA SOURCES
============

Data can be supplied in a variety of ways: a list of numbers,
a BLT *vector*, or a column in a BLT *datatable*.

*list*
  The coordinates are in a list of floating point numbers.

*vector*
  The coordinates are in a BLT vector.  *Vector* is the name of a vector
  returned by the **blt::vector** command.

*dataTable*  *column* 
   The coordinates are in a column in BLT datatable.  *DataTable* is the
   name of a datatable returned by the **blt::datatable**
   command. *Column* is the label, index, or tag representing the column,
   but may not refer to more than one column.
     
The order of the coordinates specified for the mesh determines the order of
the field values.

OPERATIONS
==========

The following operations are available for the **blt::mesh** command.

**blt::mesh cget** *meshName* *option*
  Returns the current value of the *mesh* configuration option given
  by *option*. *MeshName* is the name of *mesh* object returned by the
  **create** operation. *Option* and may have any of the values accepted by
  the **configure** operation. They are specific to the type of mesh
  for *meshName*. They are described in the **create** operations below.

**blt::mesh configure** *meshName* ?\ *option* *value* ... ?
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

**blt::mesh create cloud** ?\ *meshName*\ ? ?\ *option* *value* ... ?
  Creates a cloud *mesh* object. A cloud mesh isn't really a mesh but a
  random set of points.  The numbers represents points in the cloud.
  A triangular mesh is computed using Delaunay triangulation from the
  points. A network of triangles is built over the existing vertices
  of the point cloud.  Field values associated with a *cloud* mesh should
  have the same ordering as the cloud points.

  If no *meshName* argument is present, then the name of the mesh is
  automatically generated in the form "mesh0", "mesh1", etc. Another mesh
  object can not already exist as *meshName*.  This command returns the
  name of mesh object.  The name of the mesh is returned. *Option* and
  *value* are specific to **cloud** meshes and are listed below.

  **-x** *dataSource*
    Specifies the x-coordinates of the points in the cloud.  *DataSource*
    can be in any form described in the section `DATA SOURCES`_ above.
    
  **-y** *dataSource*
    Specifies the y-coordinates of the points in the cloud.  *DataSource*
    can be in any form described in the section `DATA SOURCES`_ above.

**blt::mesh create irregular** ?\ *meshName*\ ? ?\ *option* *value* ... ?
  Creates an irregular *mesh* object. An irregular mesh is a a non-uniform
  rectilinear grid.  The coordinates of the grid lines of the X and Y axes
  are exactly specified. The coordinates do not have to be uniformly
  spaced.  Field values associated with an *irregular* mesh should follow
  the convention of x-coordinates changing fastest. 
  
  If no *meshName* argument is present, then the name of the mesh is
  automatically generated in the form "mesh0", "mesh1", etc. Another mesh
  object can not already exist as *meshName*.  This command returns the
  name of mesh object.  The name of the mesh is returned. *Option* and
  *value* are specific to **irregular** meshes and are listed below.

  **-x** *dataSource*
    Specifies the coordinates of the grid lines the X-axis.  The
    coordinates do not have to be uniformly spaced and can be in any order.
    *DataSource* can be in any form described in the section `DATA
    SOURCES`_ above.

  **-y** *dataSource*
    Specifies the coordinates of the grid lines on the Y-axis. The
    coordinates do not have to be uniformly spaced and can be in any order.
    *DataSource* can be in any form described in the section `DATA
    SOURCES`_ above.

**blt::mesh create regular** ?\ *meshName*\ ? ?\ *option* *value* ... ?
  Creates a regular *mesh* object.  A regular mesh is a uniform rectangular
  grid where the grid lines run along the X and Y axes. You specify the
  minimum and maximum values, and the number of grid lines for each axis.
  Field values associated with a *regular* mesh should follow the
  convention of x-coordinates changing fastest. 

  If no *meshName* argument is present, then the name of the mesh is
  automatically generated in the form "mesh0", "mesh1", etc. Another mesh
  object can not already exist as *meshName*.  This command returns the
  name of mesh object.  The name of the mesh is returned. *Option* and
  *value* are specific to **regular** meshes and are listed below.

  **-x** *dataSource*
    Specifies 3 numbers: the minimum value for the X-axis, the maximum
    value for the X-axis, and the number points on the X-axis, including
    the minimum and maximum values. *DataSource* can be in any form
    described in the section `DATA SOURCES`_ above.

  **-y** *dataSource*
    Specifies 3 numbers: the minimum value for the Y-axis, the maximum
    value for the Y-axis, and the number points on the Y-axis, including
    the minimum and maximum values. *DataSource* can be in any form
    described in the section `DATA SOURCES`_ above.
    
**blt::mesh create triangle** ?\ *meshName*\ ? ?\ *option* *value* ... ?
  Creates a triangle *mesh* object. A triangle mesh comprises a set of
  triangles that are connected by their common edges or corners.  Triangles
  are defined by their vertices.  Field values associated with a *triangle*
  mesh should have the same ordering as the vertices.
  
  If no *meshName* argument is present, then the name of the mesh is
  automatically generated in the form "mesh0", "mesh1", etc. Another mesh
  object can not already exist as *meshName*.  This command returns the
  name of mesh object.  The name of the mesh is returned. *Option* and
  *value* are specific to **triangle** meshes and are listed below.

  **-x** *dataSource*
    Specifies the x-coordinates of the vertices.  *DataSource*
    can be in any form described in the section `DATA SOURCES`_ above.

  **-y** *dataSource*
    Specifies the y-coordinates of the vertices.  *DataSource*
    can be in any form described in the section `DATA SOURCES`_ above.
    
  **-triangles** *indices*
    Specifies the triangles formed by the vertices defined by the **-x**
    and **-y** options.  *Indices* is a list of non-negative integers.
    Each index refers to the x and y coordinates of the vertex at that
    index.  Indices start from 0.  Every 3 indices represent the vertices
    of a triangle.

**blt::mesh delete** ?\ *meshName* ... ?
  Releases resources allocated by one or more meshes.  Meshes are reference
  counted so that the internal mesh structures are not actually deleted
  until no one is using the mesh any more. *MeshName* must be the name of a
  mesh returned by the **create** operation, otherwise an error is
  reported.

**blt::mesh hide** *meshName* ?\ *index* ... ?
  Specifies triangles to be hidden. Each triangle specified by the index
  of the triangle will be excluded from the output of the mesh. If no
  indices are specified, all triangles are included in the mesh output.

**blt::mesh hull** *meshName* ?\ **-vertices**\ ?
  Returns the indices of the vertices of the convex hull. The convex hull
  forms the boundary for *meshName*. *MeshName* is the name of a mesh
  returned by the **create** operation.  If a **-vertices** argument is
  present, the vertices (x and y coordinates) of the hull will be returned
  instead of their indices.

**blt::mesh names** ?\ *pattern* ... ?
  Returns the names of all the meshes currently created.  If one or
  more *pattern* arguments are provided, then the name of any mesh
  matching *pattern* will be returned. *Pattern* is a **glob**-style pattern.

**blt::mesh triangles** *meshName*
  Returns the indices of the triangles of the mesh for *meshName*.  
  *MeshName* is the name of a mesh returned by the **create** operation.

**blt::mesh type** *meshName*
  Returns the type of the mesh for *meshName*.  *MeshName* is the
  name of a mesh returned by the **create** operation.

**blt::mesh vertices** *meshName*
  Returns the vertices of *meshName*.  *MeshName* is the name of a mesh
  returned by the **create** operation.  The x and y coordinates representing
  each vertex is returned.

EXAMPLE
=======

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
   object may automatically be updated.

3. Meshes are reference counted.  If you delete a mesh, its resources are
   not freed until no widget is using it.
   
KEYWORDS
========

mesh


COPYRIGHT
=========

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

The author of the sweep-line triangulator is Steven Fortune.  

  Copyright (c) 1994 by AT&T Bell Laboratories.

  Permission to use, copy, modify, and distribute this software for any
  purpose without fee is hereby granted, provided that this entire notice is
  included in all copies of any software which is or includes a copy or
  modification of this software and in all copies of the supporting
  documentation for such software.  THIS SOFTWARE IS BEING PROVIDED "AS IS",
  WITHOUT ANY EXPRESS OR IMPLIED WARRANTY.  IN PARTICULAR, NEITHER THE
  AUTHORS NOR AT&T MAKE ANY REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING
  THE MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
  PURPOSE.


