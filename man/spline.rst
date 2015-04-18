
===============
blt::spline
===============

-------------------------------------
Fit curves with spline interpolation.
-------------------------------------

:Author: George A. Howlett <gahowlett@gmail.com>
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

**blt::spline natural** *x* *y* *sx* *sy*

**blt::spline quadratic** *x* *y* *sx* *sy*

DESCRIPTION
-----------

The **spline** command computes a spline fitting a set of data points (x
and y vectors) and produces a vector of the interpolated images
(y-coordinates) at a given set of x-coordinates.  Both the **natural** and
**quadratic** operations produce splice which run through the origin knots
(points).

INTRODUCTION
------------

Curve fitting has many applications.  In graphs, curve fitting can be
useful for displaying curves which are aesthetically pleasing to the eye.
Another advantage is that you can quickly generate arbitrary points on the
curve from a small set of data points.

A spline is a device used in drafting to produce smoothed curves.  The
points of the curve, known as *knots*, are fixed and the *spline*,
typically a thin strip of wood or metal, is bent around the knots to create
the smoothed curve.  Spline interpolation is the mathematical equivalent.
The curves between adjacent knots are piecewise functions such that the
resulting spline runs exactly through all the knots.  The order and
coefficients of the polynominal determine the "looseness" or "tightness" of
the curve fit from the line segments formed by the knots.

The **spline** command performs spline interpolation using cubic
("natural") or quadratic polynomial functions.  It computes the spline
based upon the knots, which are given as x and y vectors.  The interpolated
new points are determined by another vector which represents the abscissas
(x-coordinates) or the new points.  The ordinates (y-coordinates) are
interpolated using the spline and written to another vector.

OPERATIONS
----------

**blt::spline natural** *x* *y* *sx* *sy*
  Computes a cubic spline from the data points represented by the vectors
  *x* and *y* and interpolates new points using vector *sx* as
  the x-coordinates.  The resulting y-coordinates are written to a new
  vector *sy*. The vectors *x* and *y* must be the same length
  and contain at least three components.  The order of the components of
  *x* must be monotonically increasing.  *Sx* is the vector
  containing the x-coordinates of the points to be interpolated.  No
  component of *sx* can be less than first component of *x* or
  greater than the last component.  The order of the components of *sx*
  must be monotonically increasing.  *Sy* is the name of the vector
  where the calculated y-coordinates will be stored.  If *sy* does not
  already exist, a new vector will be created.

**blt::spline quadratic** *x* *y* *sx* *sy*
  Computes a quadratic spline from the data points represented by the
  vectors *x* and *y* and interpolates new points using vector
  *sx* as the x-coordinates.  The resulting y-coordinates are written
  to a new vector *sy*.  The vectors *x* and *y* must be the
  same length and contain at least three components.  The order of the
  components of *x* must be monotonically increasing.  *Sx* is the
  vector containing the x-coordinates of the points to be interpolated. No
  component of *sx* can be less than first component of *x* or
  greater than the last component.  The order of the components of *sx*
  must be monotonically increasing.  *Sy* is the name of the vector
  where the calculated y-coordinates are stored.  If *sy* does not
  already exist, a new vector will be created.

EXAMPLE
-------

Before we can use the **spline** command, we need to create two BLT vectors
which will represent the knots (x and y coordinates) of the data that we're
going to fit.  Obviously, both vectors must be the same length.

  ::

    # Create sample data of ten points. 
    blt::vector x(10) y(10)

    for {set i 10} {$i > 0} {incr i -1} {
	set x($i-1) [expr $i*$i]
	set y($i-1) [expr sin($i*$i*$i)]
    }

We now have two vectors "x" and "y" representing the ten data
points we're trying to fit.  The order of the values of "x" must
be monotonically increasing.  We can use the vector's **sort** operation 
to sort the vectors.

 ::

    x sort y

The components of "x" are sorted in increasing order.  The components of
"y" are rearranged so that the original x,y coordinate pairings are
retained.

A third vector is needed to indicate the abscissas (x-coordinates) of the
new points to be interpolated by the spline.  Like the x vector, the vector
of abscissas must be monotonically increasing.  All the abscissas must lie
between the first and last knots (x vector) forming the spline.

How the abscissas are picked is arbitrary.  But if we are going to plot the
spline, we will want to include the knots too.  Since both the quadratic
and natural splines preserve the knots (an abscissa from the x vector will
always produce the corresponding ordinate from the y vector), we can simply
make the new vector a superset of "x".  It will contain the same
coordinates as "x", but also the abscissas of the new points we want
interpolated.  A simple way is to use the vector's **populate** operation.

 ::
    
    x populate sx 10

This creates a new vector "sx".  It contains the abscissas of "x", but in
addition "sx" will have ten evenly distributed values between each
abscissa.  You can interpolate any points you wish, simply by setting the
vector values.

Finally, we generate the ordinates (the images of the spline) using the
**spline** command.  The ordinates are stored in a fourth vector.

  ::
     
    blt::spline natural x y sx sy

This creates a new vector "sy".  It will have the same length as "sx".  The
vectors "sx" and "sy" represent the smoothed curve which we can now plot.

  ::
    blt::graph .graph
    .graph element create original -x x -y x -color blue
    .graph element create spline -x sx -y sy -color red
    blt::table . .graph

The **natural** operation employs a cubic interpolant when forming the
spline.  In terms of the draftmen's spline, a \fInatural spline\fR requires
the least amount of energy to bend the spline (strip of wood), while still
passing through each knot.  In mathematical terms, the second derivatives
of the first and last points are zero.

Alternatively, you can generate a spline using the **quadratic** operation.
Quadratic interpolation produces a spline which follows the line segments
of the data points much more closely.

  ::
    blt::spline quadratic x y sx sy 

REFERENCES
----------

Numerical Analysis
by R. Burden, J. Faires and A. Reynolds.	
Prindle, Weber & Schmidt, 1981, pp. 112

Shape Preserving Quadratic Splines 
by D.F.Mcallister & J.A.Roulier
Coded by S.L.Dodd & M.Roulier N.C.State University.

The original code for the quadratric spline can be found in TOMS #574.

KEYWORDS
--------

spline, vector, graph

