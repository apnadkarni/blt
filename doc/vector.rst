
===========
blt::vector
===========

------------------
Vector data object
------------------

:Author: George A. Howlett
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT built-in commands

.. contents:: Table of Contents


SYNOPSIS
========

**blt::vector create** *vecName* ?\ *vecName* ... ? ?\ *switches* ... ? 

**blt::vector destroy** *vecName* ?\ *vecName* ... ?

**blt::vector expr** *expression*

**blt::vector names** ?\ *pattern* ... ?

DESCRIPTION
===========

The **blt::vector** command creates an array of floating point numbers
representing 1-D points.  The vector's points can be manipulated in three
ways: through a TCL array variable, a TCL command, or the C API.

INTRODUCTION
============

A *vector* is an ordered set of real numbers representing points of the
vector.  The points are indexed by integers.

Vectors are common data structures for many applications.  For example, a
**blt::graph** can use two vectors to represent the X-Y coordinates of the
data points plotted.  The graph will automatically be redrawn when the
vectors are updated or changed. By using **blt::vector**\ s, you can
separate data analysis from the graph widget.  This makes it easier, for
example, to add data transformations, such as splines.  It's possible to
plot the same data to multiple graphs, where each graph presents a
different view or scale of the data.

VECTOR INDICES
==============

Individual points in the *vector* can be accessed via the *vector*'s array
variable or TCL command.  You can reference points in the vector either by
its integer, expression, a range, or a special keyword.

 *number*
  The index is a integer that must lie within the current range of the
  vector, otherwise an error message is returned.  Normally the indices
  of a vector start from 0, but you can use the **offset** operation to
  change a *vector*\ 's indices.

 *expr*
  You can also use numeric expressions with indices (such
  as "$i+3").  The result of the expression must be an integer value.

 *keyword*
  The following special non-numeric indices are available.

  **min**
     is the index of the point that has the minimum value.
  **max**
     is the index of the point that has the maximum value.
  **end**
     is the index of the last point in the vector.
  **++end**
     Adds a new point to the vector.  It is the index of the new
     last point in the vector. 

 *first*:\ *last*
  A range of indices can be indicated by a colon (:).  *First* and *last*
  are integer indices.  *Last* must be greater than or equal to *first*. 
  If *first* isn't supplied the first point is assumed. If *last* isn't
  supplied the last point is assumed.

VECTOR COMMAND OPERATIONS
=========================

**blt::vector create** *vecName*\ ... ?\ *switches* ... ? 
  Creates a new vector *vecName*.  The **create** operation can be invoked
  in one of three forms:

   **blt::vector create** ?\ *vecName*\ ?
     This creates a new vector *vecName* which initially has no points.
     If no *vecName* argument is given, then a name is generated in the
     form "vector0", "vector1", etc.

   **blt::vector create** *vecName*\ (*size*)
     Creates a new vector *vecName* and sets the number of points.  The
     points will be indexed starting from zero. The default value for the
     points is "0.0".

   **blt::vector create** *vecName*\ (*first*:*last*)
     Creates a new vector *vecName* and sets the number of points.  The
     points will be indexed *first* through *last*.  *First* and *last*
     can be any integer value so long as *first* is less than *last*. The
     default value for the points is "0.0".

  *VecName* must start with a letter and consist of letters, digits, or
  underscores.  You can automatically generate *vector* names using the
  "#auto" *vector* name.

  Both a TCL command and an array variable *vecName* are also created.  The
  name *vecName* must be unique, so another TCL command or array variable
  can not already exist.  You can access the points of the *vector* using
  its variable.  If you change the value of an element in the array or
  unset an array element, *vecName* is updated to reflect the changes.
  When the variable *vecName* is unset, the vector and its TCL command are
  also destroyed.

  Switches can be any of the following:

  **-command** *cmdName*
     Maps a TCL command to *vecName*. The vector can be accessed using
     *cmdName* and one of the *vector* instance operations.  A TCL command by
     that name cannot already exist.  If *cmdName* is the empty string, no
     command mapping will be made.

  **-variable** *varName*
     Specifies the name of a TCL variable to be mapped to *vecName*. If
     the variable already exists, it is first deleted, then recreated. 
     If *varName* is the empty string, then no variable will be mapped.
     You can also map a variable to the vector using the vector's 
     **variable** operation.

  **-watchunset** *boolean*
    Indicates if *vecName* should automatically be destroyed if the
    TCL variable associated with the vector is unset.  If *boolean* is true,
    the vector will be destroyed. The default is 0.

**blt::vector destroy** ?\ *vecName* ... ?
  Deletes one or more vectors.  Both the TCL command and array variable
  are removed.

**blt::vector expr** *exprString*
  All binary operators take vectors as operands (numbers are
  treated as one-point vectors).  The exact action of binary operators
  depends upon the length of the second operand.  If the second operand has
  only one point, then each element of the first *vector* operand is
  computed by that value.  For example, the expression "x * 2" multiples
  all elements of the *vector* x by 2.  If the second operand has more than
  one point, both operands must be the same length.  Each pair of
  corresponding elements are computed.  So "x + y" adds the the first
  points of x and y together, the second, and so on.

  The valid operators are listed below, grouped in decreasing order
  of precedence:

    **-**  **!**
      Unary minus and logical NOT.  The unary minus flips the sign of each
      point in the *vector*.  The logical not operator returns a *vector*
      whose values are 0.0 or 1.0.  For each "true" value the point 1.0 is
      returned, 0.0 otherwise.

    **^**
      Exponentiation.  

    **/**  **%**
      Multiply, divide, remainder.  

    **+**  **-**
      Add and subtract.  

    **<<**  **>>**
      Left and right shift.  Circularly shifts the values of the vector 
      (not implemented yet).

    **>**  **<**  **<=**  **>=**
     Boolean less, greater, less than or equal, and greater than or equal.
     Each operator returns a vector of ones and zeros.  If the condition is
     true, 1.0 is the point value, 0.0 otherwise.

    **==**  **!=**
     Boolean equal and not equal.  Each operator returns a vector of ones
     and zeros.  If the condition is true, 1.0 is the point value, 0.0
     otherwise.

    **|**
      Bit-wise OR.  (Not implemented).

    **&&**
      Logical AND.  Produces a 1 result if both operands are non-zero, 0
      otherwise.

    **||**
      Logical OR.  Produces a 0 result if both operands are zero, 1 otherwise.

    *x* **?** *y* **:** *z*
      If-then-else, as in C.  (Not implemented yet).

  See the C manual for more details on the results produced by each
  operator.  All of the binary operators group left-to-right within the
  same precedence level.

  Several mathematical functions are supported for *vector*\ s.  Each of the
  following functions invokes the math library function of the same name;
  see the manual entries for the library functions for details on what they
  do.  The operation is applied to all the points of the *vector*.

    **abs**\ (*vecName*)
      Returns the absolute value of each floating-point number in *vecName*.

    **acos**\ (*vecName*)
      Returns the arc cosine function of each number in *vecName*.

    **asin**\ (*vecName*)
      Returns the arc sine function of each number in *vecName*.

    **asinh**\ (*vecName*)
      Returns the hyperbolic arc sine function of each number in *vecName*.

    **atan**\ (*vecName*)
      Returns the arc tangent function of each number in *vecName*.

    **ceil**\ (*vecName*)
      Returns the smallest integral value not less than the floating-point
      number for each number in *vecName*.

    **cos**\ (*vecName*)
      Returns the cosine function of each number in *vecName*.

    **cosh**\ (*vecName*)
      Returns the hyperbolic cosine function of each number in *vecName*.

    **exp**\ (*vecName*)
      Returns the value of e (the base of natural logarithms) raised to the
      point of the floating point number for each number in *vecName*.

    **floor**\ (*vecName*)
      Returns the largest integral value not greater than the floating-point
      number for each number in *vecName*.

    **log**\ (*vecName*)
      Returns the natural logarithm of each floating-point number of
      *vecName*. If the number is a NaN, a NaN is returned. 

    **log10**\ (*vecName*)
      Returns the base 10 logarithm of each floating-point number of
      *vecName*. If the number is a NaN, a NaN is returned. 

    **random**\ (*vecName*)
      Returns a vector of non-negative values uniformly distributed between
      [0.0, 1.0) using **drand48**.  The seed comes from the internal clock of
      the machine or may be set manually with the **random** operation.  The
      length of the returned vector is the same as the length of *vecName*.

    **round**\ (*vecName*)
      Returns the rounded number for each point of *vecName*.
      The numbers are rounded to the nearest integer, but rounds halfway
      cases away from zero. For example, rounding of 0.5 is 1.0, and
      rounding of -0.5 is -1.0.

    **sin**\ (*vecName*)
      Returns the sine function of each number of *vecName*.

    **sinh**\ (*vecName*)
      Returns the hyperbolic sine function of each number in *vecName*.

    **sqrt**\ (*vecName*)
      Returns the square root of each floating-point number in *vecName*. If
      the number is a NaN, a NaN is returned.

    **tan**\ (*vecName*)
      Returns the tangent function of each number in *vecName*.

    **tanh**\ (*vecName*)
      Returns the hyperbolic tangent function of each number in *vecName*.

  The following functions return a single value.

    **adev**\ (*vecName*)
      Returns the average deviation (defined as the sum of the absolute values 
      of the differences between point and the mean, divided by the length
      of *vecName*).

    **kurtosis**\ (*vecName*)
     Returns the degree of peakedness (fourth moment) of *vecName*.

    **length**\ (*vecName*)
     Returns the number of points in *vecName*.

    **max**\ (*vecName*)
      Returns *vecName*\ 's maximum value.

    **mean**\ (*vecName*)
      Returns the mean value of *vecName*.

    **median**\ (*vecName*)
      Returns the median of *vecName*.

    **min**\ (*vecName*)
      Returns *vecName*\ 's minimum value.

    **nonempty**\ (*vecName*)
      Returns the number of non-empty points in *vecName*.  

    **nonzero**\ (*vecName*)
      Returns the number of non-zero points in *vecName*.  This does not
      include empty values.

    **q1**\ (*vecName*)
      Returns the first quartile of *vecName*.

    **q3**\ (*vecName*)
      Returns the third quartile of *vecName*.

    **prod**\ (*vecName*)
      Returns the product of the points.

    **sdev**\ (*vecName*) 
      Returns the standard deviation (defined as the square root of the variance)
      of *vecName*.

    **skew**\ (*vecName*) 
      Returns the skewness (or third moment) of *vecName*.  This characterizes
      the degree of asymmetry of the vector about the mean.

    **sum**\ (*vecName*) 
      Returns the sum of the points.

    **var**\ (*vecName*)
      Returns the variance of *vecName*. The sum of the squared differences 
      between each point and the mean is computed.  The variance is 
      the sum divided by the length of *vecName* minus 1.

  The last set returns a vector of the same length as the argument.

    **norm**\ (*vecName*) 
     Scales the normalized values of *vecName* (values lie in the range
     [0.0..1.0]).

    **sort**\ (*vecName*)
      Returns *vecName*\ 's points sorted in ascending order.


**blt::vector names** ?\ *pattern* ... ?
  Returns the names of all the BLT vectors.  If one or more *pattern*
  arguments are provided, then the name of any vector matching
  *pattern* will be returned. *Pattern* is a glob-style pattern.

VECTOR INSTANCE OPERATIONS
==========================

After you create a vector using the **create** operation, you can use the
vector's new TCL command to query or modify the vector instance.  The
general form is

  *vecName* *operation* ?\ *arg* ... ?

Both *operation* and its arguments determine the exact behavior of
the command.  The operations available for vectors are listed below.

*vecName* **append** ?\ *item* ... ?
  Appends one or more lists or vectors to *vecName*.  *Item* can be either
  the name of a vector or a list of numbers.

*vecName* **binread** *channelName* ?\ *length*\ ? ?\ *switches* ... ? 
  Reads binary values from a TCL channel. Values are either appended
  to the end of the vector or placed at a given index (using the
  **-at** option), overwriting existing values.  Data is read until EOF
  is found on the channel or a specified number of values *length* 
  are read (note that this is not necessarily the same as the number of 
  bytes). The following switches are supported:

  **-swap**
   Swap bytes and words.  The default endian is the host machine.

  **-at** *index*
   New values will start at vector index *index*.  This will
   overwrite any current values.

  **-format** *format*
   Specifies the format of the data.  *Format* can be one of the following:
   "i1", "i2", "i4", "i8", "u1, "u2", "u4", "u8", "r4", "r8", or "r16".
   The number indicates the number of bytes required for each value.  The
   letter indicates the type: "i" for signed, "u" for unsigned, "r" or
   real.  The default format is "r16".

  Reference: The binary reader was contributed by Harold Kirsch.

*vecName* **clear** 
  Clears the element indices from the array variable associated with
  *vecName*.  This doesn't affect the points of *vecName*.  By
  default, the number of entries in the TCL array doesn't match the number
  of points in *vecName*.  This is because its too expensive to
  maintain decimal strings for both the index and value for each point.
  Instead, the index and value are saved only when you read or write an
  element with a new index.  This command removes the index and value
  strings from the array.  This is useful when the vector is large.

*vecName* **count** *valueType*
  Returns the number of points in *vecName*. *ValueType* specifies the
  the type of points to count. *ValueType* is one of the following:

  **empty**
     Counts the number of empty points (i.e. where the value is NaN).
     
  **nonempty**
     Counts the number of non-empty point values.

  **nonzero**
     Counts the number of non-zero point values.
  
  **zero**
     Counts the number of zero point values.

*vecName* **delete** ?\ *index* ... ?
  Deletes points from *vecName*.  *Index* is
  the index of the element to be deleted.  This is the same as unsetting
  the array variable element *index*.  The vector is compacted after all
  the indices have been deleted.

*vecName* **duplicate** ?\ *destName*\ ?
  Creates a duplicate of *vecName*.  If a *destName* argument exists, it is
  the name of the new vector, otherwise a name is generated in the form
  "vector0", "vector1", etc.  A vector *destName* can not already exist.

*vecName* **export** *format* ?\ *switches* ... ?
  Exports *vecName* as a binary string. *Format* is either "double" or
  "float".  If neither a **-data** or **-file** switch is given, then 
  this command returns the binary string.
  
  **-data** *varName*
   Specifies a TCL variable *varName* to write the binary output. 

  **-empty** *value*
   Specifies the a value for empty points.  By default, a NaN is
   written for each empty point.  *Value* is a real number.

  **-file** *path*
   Specifies a file *path* to write the binary output.

  **-from** *index*
   Specifies the starting index of values to export.  *Index* is vector
   index. The default is to export values from 0.

  **-to** *index*
   Specifies the ending index of values to export.  *Index* is vector
   index. The default is to export values to the end of *vecName*.

*vecName* **expr** *exprString*
  Computes the expression and resets the values of *vecName* accordingly.
  The is similar to the **blt::vector expr** operation. The difference is
  that *vecName* is reset with the new values.  The format of *exprString*
  is described above for the **blt::vector expr** operation.

*vecName* **fft** *destName* ?\ *switches* ... ?
  Returns the discrete Fourier transform (DFT) of *vecName*, computed with
  a fast Fourier transform (FFT) algorithm. The vector *destName* will hold
  the real-valued results.
  
  **-imagpart** *vecName*
   Specifies *vecName* to store the imaginary part transform.

  **-noconstant**

  **-spectrum** 
    Computes the modulus of the transforms, scaled by 1/N^2 
    or 1/(N * Wss) for windowed data.

  **-bartlett** 
   Specifies the use a Bartlett Window.

  **-delta** *number*
   Specifies the ending index of values to export.  *Index* is vector
   index. The default is to export values to the end of *vecName*.

  **-frequencies** *vecName*
   Specifies *vecName* to store the frequencies of the transform.

  Reference: This was contributed by Andrea Spinelli (spinellia@acm.org).
  
*vecName* **frequency** *destName* *numBins*
  Fills *destName* with the frequency of values found in *vecName*.
  *DestName* is the name a vector created by the **create** operation.
  *NumBins* is an non-zero integer specifying the number of bins to use
  when computing the frequency.  Bins represent regular intervals of
  values from the minimum to the maximum vector value.

*vecName* **indices** *valueType*
  Returns the indices of points in *vecName*. *ValueType* specifies the
  type of points to consider. *ValueType* is one of the following:

  **empty**
     Returns the indices of the empty points (i.e. where the value
     is NaN).
     
  **nonempty**
     Returns the indices of non-empty point values.

  **nonzero**
     Returns the indices non-zero point values.
  
  **zero**
     Returns the indices of non-empty point values.

*vecName* **inversefft** *vecName* *vecName*
  Returns the discrete Fourier transform (DFT) of *vecName*, computed with
  a fast Fourier transform (FFT) algorithm. The vector *destName* will hold
  the real-valued results.
  
  **-imagpart** *vecName*
   Specifies *vecName* to store the imaginary part transform.

  **-noconstant**

  **-spectrum** 
    Computes the modulus of the transforms, scaled by 1/N^2 
    or 1/(N * Wss) for windowed data.

  **-bartlett** 
   Specifies the use a Bartlett Window.

  **-delta** *number*
   Specifies the ending index of values to export.  *Index* is vector
   index. The default is to export values to the end of *vecName*.

  **-frequencies** *vecName*
   Specifies *vecName* to store the frequencies of the transform.

  Reference: This was contributed by Andrea Spinelli (spinellia@acm.org).

*vecName* **length** ?\ *newSize*\ ?
  Queries or resets the number of points in *vecName*.  *NewSize* is a
  number specifying the new size of *vecName*.  If *newSize* is smaller
  than the current size of *vecName*, *vecName* is truncated.  If *newSize*
  is greater, *vecName* is extended and the new points are initialized
  to "0.0".  If no *newSize* argument is present, the current length
  of *vecName* is returned.

*vecName* **linspace** *firstValue* *lastValue* ?\ *numSteps*\ ?
  Generates linearly spaced vector values. *FirstValue* and *lastValue* are
  numbers representing the minimum and maximum values.  If *firstValue* is
  greater than *lastValue* the values will be decreasing.  *NumSteps* is
  the number of points to generate.  *VecName* will be resized to
  *numSteps* points. If no *numSteps* argument is given, then the current
  length of *vecName* is used as the number of points.
  
*vecName* **maximum**
  Returns the maximum value in *vecName*.

*vecName* **merge** ?\ *srcName* ...?
  Merges one or more vectors into *vecName*.  *SrcName* is the name a
  vector created by the **create** operation.  All *srcName* vectors must
  be the same length.  The length of *vecName* will be grown to hold all
  the points from each *srcName* vector.  The points are merged one at a
  time for each index, by adding the points for each vector *srcName*,

*vecName* **minimum**
  Returns the maximum value in *vecName*.

*vecName* **normalize** ?\ *destName*\ ?
  Normalizes *vecName* to have values between 0 and 1.  If a *destName*
  exists, it is the name a vector created by the **create** operation.
  *DestName* will be resized if necessary to hold the normalized values.
  If no *destName* argument is present, then this command will return the
  normalized values.

*vecName* **notify** *keyword*
  Controls how vector clients are notified of changes to *vecName*.  
  The exact behavior is determined by *keyword*.

  **always**
    Indicates that clients are to be notified immediately whenever the
    vector is updated.

  **never**
    Indicates that no clients are to be notified.

  **whenidle**
    Indicates that clients are to be notified at the next idle point
    whenever *vecName* is updated.

  **now**
   If any client notifications are currently pending, they are notified
   immediately.

  **cancel**
   Cancels pending notifications of clients using *vecName*.

  **pending**
   Returns "1" if a client notification is pending, and "0" otherwise.

*vecName* **offset** ?\ *count*\ ?
  Offsets the indices of *vecName* by the amount specified by *count*.
  *Count* is an integer number.  For example if *count* is "-5", the index
  of the first point in *vecName* is "-5".  If no *count* argument is
  given, the current offset is returned.

*vecName* **populate** *destName* ?\ *density*\ ?
  Creates a vector *destName* which is a superset of *vecName*.  *DestName*
  in the name of an output vector that will include all the points of
  *vecName*, in addition the interval between each of the original points
  will contain a *density* number of new points, whose values are evenly
  distributed between the original points values.  This is useful for
  generating abscissas to be interpolated along a spline.

*vecName* **print** *fmtString* ?\ *switches* ... ?
  Returns a string of representing the values of *vecName*. *FmtString* is a
  **printf**\ -like format string. The number of specifiers in *fmtString*
  determines how many points are used for each successive print.
  *Switches* may be any of the following:

  **-from** *index*
   Specifies the starting index of values to print.  *Index* is vector
   index. The default is to print values from 0.

  **-to** *index*
   Specifies the ending index of values to print.  *Index* is vector
   index. The default is to print values to the end of *vecName*.

*vecName* **random** ?\ *seed*\ ?
  Generates a random value for each point in *vecName*.  *Seed* is a
  integer value that specifies the seed of the random number generator.

*vecName* **range** ?\ *firstIndex* *lastIndex* \?
  Returns a list of numeric values representing the vector points
  between two indices. Both *firstIndex* and *lastIndex* are indices
  representing the range of points to be returned. If *lastIndex* is
  less than *firstIndex*, the points are listed in reverse order.
  If the *firstIndex* and *lastIndex* arguments are omitted, then
  the entire vector is returned.
  
*vecName* **search** *value* ?\ *value*\ ?  
  Searches for a value or range of values among the points of *vecName*.
  If one *value* argument is given, a list of indices of the points which
  equal *value* is returned.  If a second *value* is also provided, then
  the indices of all points which lie within the range of the two values
  are returned.  If no points are found, then "" is returned.

*vecName* **sequence** *start* ?\ *stop*\ ? ?\ *step*\ ?
  Generates a sequence of values starting with the number *start*.  *Stop*
  indicates the terminating number of the sequence.  *VecName* is
  automatically resized to contain just the sequence.  If three arguments
  are present, *step* designates the interval.

  With only two arguments (no *stop* argument), the sequence will
  continue until *vecName* is filled.  With one argument, the interval
  defaults to 1.0.

*vecName* **set** *item*
  Sets the points of *vecName* to *item*. *Item* can be either a list of
  numbers or a vector name.

*vecName* **simplify** *x* *y* ?\ *tolerance*\ ?
  Reduces the number of points in *vecName* using the Douglas-Peucker line
  simplification algorithm, first selecting a single line from start to end
  and then finding the largest deviation from this straight line, and if it
  is greater than *tolerance*, the point is added, splitting the original
  line into two new line segments. This repeats recursively for each new
  line segment created.  The indices of the reduced set of points is
  returned.

  *X* and *y* are the names input vectors representing the curve to be
  simplified.  The lengths of both vectors must be the same.  *Tolerance*
  is a real number representing the tolerance. The default is "1.0".

  Reference: David Douglas and Thomas Peucker, "Algorithms for the
  reduction of the number of points required to represent a
  digitized line or its caricature", The Canadian Cartographer
  10(2), 112–122, 1973.
   
*vecName* **sort** ?\ *switches* ... ? ?\ *destName* ... ?
  Sorts the points of *vecName*. If one of more *destName* arguments are
  given, they are parallel vectors that will also be considered when
  sorting.  Each *destName* vector must be the same length as *vecName*.
  Normally this command rearranges the points of each vector. But if the
  **-indices** or **-values** switches are given, then vectors will not be
  rearranged, and this command returns the values or indices.  *Switches*
  can be any of the following:
  
  **-decreasing**
   Sort the points from highest to lowest.  By default points are
   sorted lowest to highest.

  **-indices** 
   Returns the indices of the sorted points instead of their values.
   Returns a list of the indices from the sorted points.  The points of
   *vecName* and *destName* are not rearranged.

  **-reverse** *
   Same as the **-decreasing** switch above.

  **-unique** 
   Returns the unique values.  

  **-values** 
   Returns a list of the values from the sorted points.  For each point
   there will be as many values as vectors. The points of *vecName* and
   *destName* are not rearranged.

*vecName* **value get** *index* 
  Returns the value at the point in *vecName* indexed by *index*. *Index*
  is a vector index. 

*vecName* **value set** *index* *value*
  Sets the value at the point in *vecName* indexed by *index*. *Index*
  is a vector index. *Value* is a real number.

*vecName* **value unset** ?\ *index* ... ?
  Unsets the value at the point in *vecName* indexed by *index*. *Index*
  is a *vector* index. The value of the point becomes NaN.

*vecName* **values** ?\ *switches* ... \?
  Returns a list of the values in *vecName*.  *Switches* can be any
  of the following:
  
  **-empty** *value*
   Specifies the a value for empty points.  By default, a NaN is
   written for each empty point.  *Value* is a real number.

  **-format** *fmtString*
   Specifies how to format each value in *vecName*.  *FmtString* is a
   **printf**\ -like format string. There can be only one specifier in
   *fmtString*.

  **-from** *index*
   Specifies the starting index of values to print.  *Index* is vector
   index. The default is 0.

  **-to** *index*
   Specifies the ending index of values to print.  *Index* is vector
   index. The default is to print values to the end of *vecName*.

*vecName* **variable** *varName*
  Maps a TCL variable to *vecName*, creating another means for accessing
  *vecName*.  The variable *varName* can't already exist. This overrides
  any current variable mapping *vecName* may have. 

C LANGUAGE API
==============

You can create, modify, and destroy vectors from C code, using library
routines.  You need to include the header file "blt.h". It contains
the definition of the structure **Blt_Vector**, which represents the
vector.  It appears below.

  ::

    typedef struct {
        double *valueArr; 
        int numValues;    
        int arraySize;    
        double min, max;  
    } Blt_Vector;

The field *valueArr* points to memory holding the vector points.  The
points are stored in a double precision array, whose size size is
represented by *arraySize*.  *NumValues* is the length of vector.  The size
of the array is always equal to or larger than the length of the vector.
*Min* and *max* are minimum and maximum point values.

The following routines are available from C to manage vectors.  Vectors are
identified by the vector name.

**Blt_CreateVector**\ (Tcl_Interp *\ *interp*, char *\ *vecName*, int *length*, Blt_Vector \*\*\ *vecPtrPtr*)
  Creates a new vector *vecName* with a length of *length*.
  **Blt_CreateVector** creates both a new TCL command and array variable
  *vecName*.  Neither a command nor variable named *vecName* can already
  exist.  A pointer to the vector is placed into *vecPtrPtr*.

  Returns TCL_OK if the vector is successfully created.  If
  *length* is negative, a TCL variable or command *vecName* already
  exists, or memory cannot be allocated for the vector, then
  TCL_ERROR is returned and *interp->result* will contain an
  error message.

**Blt_DeleteVectorByName**\ (Tcl_Interp *\ *interp*, char *\ *vecName*)
  Removes the vector *vecName*.  *VecName* is the name of a vector
  which must already exist.  Both the TCL command and array variable
  *vecName* are destroyed.  All clients of *vecName* will be notified
  immediately that the vector has been destroyed.

  Returns TCL_OK if the vector is successfully deleted.  If
  *vecName* is not the name a vector, then TCL_ERROR is returned
  and *interp->result* will contain an error message.

**Blt_DeleteVector**\ (Blt_Vector *\ *vecPtr*) 
  Removes the vector pointed to by *vecPtr*.  *VecPtr* is a pointer to a
  vector, typically set by **Blt_GetVector** or **Blt_CreateVector**.  Both
  the TCL command and array variable of the vector are destroyed.  All
  clients of the vector will be notified immediately that the vector has
  been destroyed.


  Returns TCL_OK if the vector is successfully deleted.  If
  *vecName* is not the name a vector, then TCL_ERROR is returned
  and *interp->result* will contain an error message.

**Blt_GetVector**\ (Tcl_Interp *\ *interp*, char *\ *vecName*, Blt_Vector \*\*\ *vecPtrPtr*)
  Retrieves the vector *vecName*.  *VecName* is the name of a vector which
  must already exist.  *VecPtrPtr* will point be set to the address of the
  vector.

  Returns TCL_OK if the vector is successfully retrieved.  If
  *vecName* is not the name of a vector, then TCL_ERROR is returned
  and *interp->result* will contain an error message.  

**Blt_ResetVector**\ (Blt_Vector *\ *vecPtr*, double *\ *dataArr*, int *numValues*, int *arraySize, Tcl_FreeProc *\ *freeProc*) 
  Resets the points of the vector pointed to by *vecPtr*.  Calling
  **Blt_ResetVector** will trigger the vector to dispatch notifications to
  its clients. *DataArr* is the array of doubles which represents the
  vector data. *NumValues* is the number of elements in the
  array. *ArraySize* is the actual size of the array (the array may be
  bigger than the number of values stored in it). *FreeProc* indicates how
  the storage for the vector point array (*dataArr*) was allocated.  It is
  used to determine how to reallocate memory when the vector is resized or
  destroyed.  It must be TCL_DYNAMIC, TCL_STATIC, TCL_VOLATILE, or a
  pointer to a function to free the memory allocated for the vector
  array. If *freeProc* is TCL_VOLATILE, it indicates that *dataArr* must be
  copied and saved.  If *freeProc* is TCL_DYNAMIC, it indicates that
  *dataArr* was dynamically allocated and that TCL should free *dataArr* if
  necessary.  "Static" indicates that nothing should be done to release
  storage for *dataArr*.

  Returns TCL_OK if the vector is successfully resized.  If *newSize* is
  negative, a vector *vecName* does not exist, or memory cannot be
  allocated for the vector, then TCL_ERROR is returned and *interp->result*
  will contain an error message.

**Blt_ResizeVector**\ (Blt_Vector *\ *vecPtr*, int *newSize*)
  Resets the length of the vector pointed to by *vecPtr* to *newSize*.  If
  *newSize* is smaller than the current size of the vector, it is
  truncated.  If *newSize* is greater, the vector is extended and the new
  points are initialized to "0.0".  Calling **Blt_ResetVector**
  will trigger the vector to dispatch notifications.

  Returns TCL_OK if the vector is successfully resized.  If *newSize* is
  negative or memory can not be allocated for the vector, then TCL_ERROR
  is returned and *interp->result* will contain an error message.


**Blt_VectorExists**\ (Tcl_Interp *\ *interp*, char *\ *vecName*) 
  Indicates if a vector named *vecName* exists in *interp*.
  Returns "1" if a vector *vecName* exists and "0" otherwise.

**Blt_AllocVectorId**\ (Tcl_Interp *\ *interp*, char *\ *vecName*) 
  Allocates an client identifier for with the vector *vecName*.  This
  identifier can be used to specify a call-back which is triggered when the
  vector is updated or destroyed.

  Returns a client identifier if successful.  If *vecName* is not the name
  of a vector, then "NULL" is returned and *interp->result* will
  contain an error message.

**Blt_GetVectorById**\ (Tcl_Interp *\ *interp*, Blt_VectorId *clientId*, Blt_Vector \*\*\ *vecPtrPtr*) 
  Retrieves the vector used by *clientId*.  *ClientId* is a valid vector
  client identifier allocated by **Blt_AllocVectorId**.  *VecPtrPtr* will
  point be set to the address of the vector.

  Returns TCL_OK if the vector is successfully retrieved.  


**Blt_SetVectorChangedProc**\ (Blt_VectorId *clientId*, Blt_VectorChangedProc \*\ *proc*, ClientData *clientData*)
  Specifies a call-back routine to be called whenever the vector associated
  with *clientId* is updated or deleted.  *Proc* is a pointer to call-back
  routine and must be of the type **Blt_VectorChangedProc**.  *ClientData*
  is a one-word value to be passed to the routine when it is invoked. If
  *proc* is "NULL", then the client is not notified.

  The designated call-back procedure will be invoked when the vector is 
  updated or destroyed.

  If your application needs to be notified when a vector changes, it can
  allocate a unique client identifier for itself.  Using this
  identifier, you can then register a call-back to be made whenever the
  vector is updated or destroyed.  By default, the call-backs are made at
  the next idle point.  This can be changed to occur at the time the vector
  is modified.  An application can allocate more than one identifier for
  any vector.  When the client application is done with the vector, it
  should free the identifier.

  The callback routine must of the following type.

  ::

    typedef void (**Blt_VectorChangedProc**) (TCL_Interp **interp*, 
         ClientData *clientData*, Blt_VectorNotify *notify*);


  *ClientData* is passed to this routine whenever it is called.  You can
  use this to pass information to the call-back.  The *notify* argument
  indicates whether the vector has been updated of destroyed. It is an
  enumerated type.

  ::

    typedef enum {
        BLT_VECTOR_NOTIFY_UPDATE=1,
        BLT_VECTOR_NOTIFY_DESTROY=2
    } Blt_VectorNotify;


**Blt_FreeVectorId**\ (Blt_VectorId *clientId*)
  Frees the client identifier.  Memory allocated for the identifier is
  released.  The client will no longer be notified when the vector is
  modified.

  The designated call-back procedure will be no longer be invoked when the
  vector is updated or destroyed.  

**Blt_NameOfVectorId**\ (Blt_VectorId *clientId*)
  Retrieves the name of the vector associated with the client identifier
  *clientId*.

  Returns the name of the vector associated with *clientId*.  If *clientId*
  is not an identifier or the vector has been destroyed, "NULL" is
  returned.  


**Blt_InstallIndexProc**\ (char \*\ *indexName*, Blt_VectorIndexProc \*\ *proc*)
  Registers a function to be called to retrieved the index *indexName*
  from the vector's array variable.  

  ::

    typedef double Blt_VectorIndexProc(Vector *vecPtr);

  The function will be passed a pointer to the vector.  The function must
  return a double representing the value at the index.

  The new index is installed into the vector.

EXAMPLE
=======

You create vectors using the **blt::vector** command and its **create**
operation.

  ::

    package require BLT

    # Create a new vector. 
    blt::vector create y(50)

This creates a new vector named "y".  It has fifty points, by default,
initialized to "0.0".  In addition, both a TCL command and array variable,
both named "y", are created.  You can use either the command or variable to
query or modify points of the vector.

  ::

    # Set the first value. 
    set y(0) 9.25
    puts "y has [y length] points"

The array "y" can be used to read or set individual points of the vector.
Vector points are indexed from zero.  The array index must be a number less
than the number of points.  For example, it's an error if you try to set
the 51st element of "y".

  ::

    # This is an error. The vector only has 50 points.
    set y(50) 0.02

You can also specify a range of indices using a colon (:) to separate the
first and last indices of the range.

  ::

    # Set the first six points of y 
    set y(0:5) 25.2

If you don't include an index, then it will default to the first and/or
last point of the vector.

  ::

    # Print out all the points of y 
    puts "y = $y(:)"

There are special non-numeric indices.  The index "end", specifies the last
point of the vector.  It's an error to use this index if the vector is
empty (length is zero).  The index "++end" can be used to extend the vector
by one point and initialize it to a specific value.  You can't read from
the array using this index, though.

  ::

    # Extend the vector by one point.
    set y(++end) 0.02

The other special indices are "min" and "max".  They return the current
smallest and largest points of the vector.

  ::

    # Print the bounds of the vector
    puts "min=$y(min) max=$y(max)"

To delete points from a vector, simply unset the corresponding array
element. In the following example, the first point of "y" is deleted.  All
the remaining points of "y" will be moved down by one index as the length
of the vector is reduced by one.

  ::

    # Delete the first point
    unset y(0)
    puts "new first element is $y(0)"

The vector's TCL command can also be used to query or set the vector.

  ::

    # Create and set the points of a new vector
    blt::vector create x
    x set { 0.02 0.04 0.06 0.08 0.10 0.12 0.14 0.16 0.18 0.20 }

Here we've created a vector "x" without a initial length specification.  In
this case, the length is zero.  The **set** operation resets the vector,
extending it and setting values for each new point.

There are several operations for vectors.  The **range** operation lists
the points of a vector between two indices.

  ::

    # List the points 
    puts "x = [x range 0 end]"

You can search for a particular value using the **search** operation.  It
returns a list of indices of the points with the same value.  If no point
has the same value, it returns "".

  ::

    # Find the index of the biggest point
    set indices [x search $x(max)]

Other operations copy, append, or sort vectors.  You can append vectors or
new values onto an existing vector with the **append** operation.

  ::

    # Append assorted vectors and values to x
    x append x2 x3 { 2.3 4.5 } x4

The **sort** operation sorts the vector.  If any additional vectors are
specified, they are rearranged in the same order as the vector.  For
example, you could use it to sort data points represented by x and y
vectors.

  ::

    # Sort the data points
    x sort y

The vector "x" is sorted while the points of "y" are rearranged so that the
original x,y coordinate pairs are retained.

The **expr** operation lets you perform arithmetic on vectors.  The result
is stored in the vector.

  ::

    # Add the two vectors and a scalar
    x expr { x + y }
    x expr { x * 2 }

When a vector is modified, resized, or deleted, it may trigger
call-backs to notify the clients of the vector.  For example, when a
vector used in the **blt::graph** widget is updated, the vector
automatically notifies the widget that it has changed.  The graph can
then redrawn itself at the next idle point.  By default, the
notification occurs when Tk is next idle.  This way you can modify the
vector many times without incurring the penalty of the graph redrawing
itself for each change.  You can change this behavior using the
**notify** operation.

  ::

    # Make vector x notify after every change
    x notify always
            ...
    # Never notify
    x notify never
            ...
    # Force notification now
    x notify now

To delete a vector, use the **delete** operation.  Both the vector and its
corresponding TCL command are destroyed.

  ::

    # Remove vector x
    blt::vector destroy x

C API EXAMPLE
=============

The following example opens a file of binary data and stores it in an array
of doubles. The array size is computed from the size of the file. If the
vector "data" exists, calling **Blt_VectorExists**, **Blt_GetVector** is
called to get the pointer to the vector.  Otherwise the routine
**Blt_CreateVector** is called to create a new vector and returns a pointer
to it. Just like the TCL interface, both a new TCL command and array
variable are created when a new vector is created. It doesn't make any
difference what the initial size of the vector is since it will be reset
shortly. The vector is updated when **Blt_ResetVector** is called.
**Blt_ResetVector** makes the changes visible to the TCL interface and other
vector clients (such as a graph widget).

  ::

     #include <tcl.h>
     #include <blt.h>                           
     ...
     Blt_Vector *vecPtr;
     double *newArr;
     FILE *f;
     struct stat statBuf;
     int numBytes, numValues;

     f = fopen("binary.dat", "r");
     fstat(fileno(f), &statBuf);
     numBytes = (int)statBuf.st_size;

     /* Allocate an array big enough to hold all the data */
     newArr = (double *)malloc(numBytes);
     numValues = numBytes / sizeof(double);
     fread((void *)newArr, numValues, sizeof(double), f);
     fclose(f);

     if (Blt_VectorExists(interp, "data"))  {
         if (Blt_GetVector(interp, "data", &vecPtr) != TCL_OK) {
             return TCL_ERROR;
         }
     } else {
        if (Blt_CreateVector(interp, "data", 0, &vecPtr) != TCL_OK) {
             return TCL_ERROR;
        }
     }
     /* 
      * Reset the vector. Clients will be notified when Tk is idle. 
      * TCL_DYNAMIC tells the vector to free the memory allocated 
      * if it needs to reallocate or destroy the vector.
      */
     if (Blt_ResetVector(vecPtr, newArr, numValues, numValues, 
             TCL_DYNAMIC) != TCL_OK) {
         return TCL_ERROR;
     }


DIFFERENCES WITH TCL ARRAYS
===========================

You could try to use TCL's associative arrays as vectors.  TCL arrays are
easy to use.  You can access individual elements randomly by specifying the
index, or the set the entire array by providing a list of index and value
pairs for each element.  The disadvantages of associative arrays as vectors
lie in the fact they are implemented as hash tables.

 +
  There's no implied ordering to the associative arrays.  If you used
  vectors for plotting, you would want to insure the second point comes
  after the first, an so on.  This isn't possible since arrays are actually
  hash tables.  For example, you can't get a range of values between two
  indices.  Nor can you sort an array.

 +
  Arrays consume lots of memory when the number of elements becomes large
  (tens of thousands).  This is because each element's index and value are
  stored as strings in the hash table.

 +
  The C programming interface is unwieldy.  Normally with vectors, you
  would like to view the TCL array as you do a C array, as an array of
  floats or doubles.  But with hash tables, you must convert both the index
  and value to and from decimal strings, just to access an element in the
  array.  This makes it cumbersome to perform operations on the array as a
  whole.

The **blt::vector** command tries to overcome these disadvantages while
still retaining the ease of use of TCL arrays.  The **blt::vector** command
creates both a new TCL command and associate array which are linked to the
vector points.  You can randomly access vector points though the elements
of array.  Not have all indices are generated for the array, so printing
the array (using the TCL **parray** procedure) does not print out all the
point values.  You can use the **blt::vector** command to access the array
as a whole.  You can copy, append, or sort vector using its command.  If
you need greater performance, or customized behavior, you can write your
own C code to manage vectors.

KEYWORDS
========

vector, graph, widget

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

               
