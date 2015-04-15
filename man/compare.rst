===================
blt::utils::compare
===================

------------------------------------------------
Utility commands to compare strings and numbers.
------------------------------------------------

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

**blt::utils::number** *oper* ?\ *arg* *arg* ...\ ?

**blt::utils::string** *oper* ?\ *arg* *arg* ...\ ?

DESCRIPTION
===========

These utility commands are used by the **treeview** widget to filter rows.
The **number** operations handle cases where the floating point numbers are
almost equal. The **string** operations provide switches to compactly
handle different cases.

SYNTAX
======

**blt::utils::number** *operation*  ?\ *arg* *arg* ...\ ?

  Compares floating point numbers. Both *operation* and its arguments
  determine the exact behavior of the command.  The operations available
  for datatables are listed below in **NUMBER COMPARISONS**.

**blt::utils::string** *operation*  ?\ *arg* *arg* ...\ ?

  Compares strings. Both *operation* and its arguments
  determine the exact behavior of the command.  The operations available
  for datatables are listed below in **STRING COMPARISONS**.


NUMBER COMPARISONS
==================

**blt::utils::number between** *number* *first* *last*

  Indicates if *number* is between *first* and *last*.  *Number*, *first*,
  and *last* are floating point numbers.  If *number* is greater than or
  equal to *first* and *number* is less than of equal to *last* "1" is
  returned.  Otherwise "0".

**blt::utils::number eq** *number1* *number2* 

  Indicates if *number1* is equal to *number2*.  *Number1*
  and *number2* are floating point numbers.  If *number1* is 
  equal to *number2* "1" is returned, otherwise "0".

**blt::utils::number ge** *number1* *number2* 

  Indicates if *number1* is equal to *number2*.  *Number1*
  and *number2* are floating point numbers.  If *number1* is 
  greater than or equal to *number2* "1" is returned, otherwise "0".

**blt::utils::number gt** *number1* *number2* 

  Indicates if *number1* is equal to *number2*.  *Number1*
  and *number2* are floating point numbers.  If *number1* is 
  greater than *number2* "1" is returned, otherwise "0".

**blt::utils::number le** *number1* *number2* 

  Indicates if *number1* is equal to *number2*.  *Number1*
  and *number2* are floating point numbers.  If *number1* is 
  less than or equal to *number2* "1" is returned, otherwise "0".

**blt::utils::number lt** *number1* *number2* 

  Indicates if *number1* is equal to *number2*.  *Number1*
  and *number2* are floating point numbers.  If *number1* is 
  less than *number2* "1" is returned, otherwise "0".

**blt::utils::number inlist** *number* *numList* ?\ *switches ...* ?

  Indicates if *number* is equal to one of the numbers in *numList*.
  *Number* is a floating point. *NumList* is a list or floating point
  numbers.  If *number* is in the list "1" is returned, otherwise "0".
  *Switches* are described below.

  **-sorted** *how*
    Specifies that the list is sorted. Searching long lists is sped
    up by sorting of *numList*.  If *how* is "increasing*, the
    list is sorted in increasing order (lowest to highest). If *how* is
    "descreasing" the list is sorted in decreasing order (highest to
    lowest).  

STRING COMPARISONS
==================

**blt::utils::string begins** *string* *pattern* ?\ *switches* ...\ ?

  Indicates if *string* begins with the string *pattern*.  *String* and
  *pattern* are ordinary TCL strings.  If *string* is begins with *pattern*
  "1" is returned, otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when determining if *string* starts with
    *pattern*.

  **-trim** 
   Trims leading whitespace from *string* before determining if *string*
   starts with *pattern*.

**blt::utils::string between** *string* *first* *last** ?\ *switches* ...\ ?

  Indicates if *string* is between *first* and *last*.  *String*, *first*
  and *last* are ordinary TCL strings.  If *string* is greater than or
  equal to *first* and *string* is less than or equal to *last* "1" is
  returned, otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when comparing strings.

  **-ascii**
    Use string comparison with Unicode code-point collation order (the name
    is for backward-compatibility reasons.)  

  **-dictionary** 
    Use dictionary-style comparison. This is the same as **-ascii**
    except (a) case is ignored except as a tie-breaker and (b) if two
    strings contain embedded numbers, the numbers compare as integers, not
    characters.  For example, in -dictionary mode, "bigBoy" sorts between
    "bigbang" and "bigboy", and "x10y" sorts between "x9y" and "x11y".

**blt::utils::string contains** *string* *pattern* ?\ *switches* ...\ ?

  Indicates if *string* is contains *pattern*.  *String* and *pattern* are
  ordinary TCL strings.  If *string* is contains *pattern* "1" is returned,
  otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when determining if *string* starts with
    *pattern*.

  **-trim** 
   Trims leading whitespace for *string* before determining if *string*
   starts with *pattern*.

**blt::utils::string ends** *string* *pattern* ?\ *switches* ...\ ?

  Indicates if *string* ends with the string *pattern*.  *String* and
  *pattern* are ordinary TCL strings.  If *string* is ends with *pattern*
  "1" is returned, otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when determining if *string* ends with
    *pattern*.

  **-trim** 
   Trims trailing whitespace from *string* before determining if *string*
   ends with *pattern*.

**blt::utils::string equals** *string1* *string2* ?\ *switches* ...\ ?

  Indicates if *string1* equals *string2*.  *String1* and *string2* are
  ordinary TCL strings.  If *string1* is equals *string2* "1" is returned,
  otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when determining if *string* equals 
    *string2*.

  **-trim** 
   Trims leading and trailing whitespace from *string* before determining
   if *string1* equals *string2*.

**blt::utils::string inlist** *string* *strList* ?\ *switches ...* ?

  Indicates if *string* is equal to one of the strings in *strList*.
  *String* is an ordinary TCL string. *StrList* is a list or TCL strings.
  If *string* is in the list "1" is returned, otherwise "0".
  *Switches* are described below.

  **-sorted** *how*
    Specifies that the list is sorted. Searching long lists is sped
    up by sorting *strList*.  If *how* is "increasing*, the
    list is sorted in increasing order (lowest to highest). If *how* is
    "descreasing" the list is sorted in decreasing order (highest to
    lowest).  

EXAMPLE
=======

KEYWORDS
========

datatable, tableview
