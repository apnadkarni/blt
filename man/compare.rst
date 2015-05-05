===================
blt::utils::compare
===================

------------------------------------------------
Utility commands to compare strings and numbers.
------------------------------------------------

:Author: George A. Howlett <gahowlett@gmail.com>
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::utils::number** *oper* ?\ *arg* ... ?

**blt::utils::string** *oper* ?\ *arg* ... ?

DESCRIPTION
-----------

These utility commands are used by the **treeview** widget to filter rows.
The **number** operations handle cases where the floating point numbers are
almost equal. The **string** operations provide switches to compactly
handle different cases.

SYNTAX
------

**blt::utils::number** *operation*  ?\ *arg* ... ?
  Compares floating point numbers. Both *operation* and its arguments
  determine the exact behavior of the command.  The operations available
  for datatables are listed below in `NUMBER COMPARISONS`_.

**blt::utils::string** *operation*  ?\ *arg* ... ?
  Compares strings. Both *operation* and its arguments
  determine the exact behavior of the command.  The operations available
  for datatables are listed below in `STRING COMPARISONS`_.


NUMBER COMPARISONS
------------------

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
------------------

**blt::utils::string begins** *string* *pattern* ?\ *switches* ... ?
  Indicates if *string* begins with the string *pattern*.  *String* and
  *pattern* are ordinary TCL strings.  If *string* is begins with *pattern*
  "1" is returned, otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when determining if *string* starts with
    *pattern*.

  **-trim** 
    Trims leading whitespace from *string* before determining if *string*
    starts with *pattern*.

**blt::utils::string between** *string* *first* *last** ?\ *switches* ... ?
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

**blt::utils::string contains** *string* *pattern* ?\ *switches* ... ?
  Indicates if *string* is contains *pattern*.  *String* and *pattern* are
  ordinary TCL strings.  If *string* is contains *pattern* "1" is returned,
  otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when determining if *string* starts with
    *pattern*.

  **-trim** 
    Trims leading whitespace for *string* before determining if *string*
    starts with *pattern*.

**blt::utils::string ends** *string* *pattern* ?\ *switches* ... ?
  Indicates if *string* ends with the string *pattern*.  *String* and
  *pattern* are ordinary TCL strings.  If *string* is ends with *pattern*
  "1" is returned, otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when determining if *string* ends with
    *pattern*.

  **-trim** 
    Trims trailing whitespace from *string* before determining if *string*
    ends with *pattern*.

**blt::utils::string equals** *string1* *string2* ?\ *switches* ... ?
  Indicates if *string1* equals *string2*.  *String1* and *string2* are
  ordinary TCL strings.  If *string1* is equals *string2* "1" is returned,
  otherwise "0". *Switches* can be any of the following.

  **-nocase** 
    Specifies to ignore case when determining if *string* equals 
    *string2*.

  **-trim** 
    Trims leading and trailing whitespace from *string* before determining
    if *string1* equals *string2*.

**blt::utils::string inlist** *string* *strList* ?\ *switches* ... ?
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
-------

KEYWORDS
--------

datatable, tableview

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
