===================
blt::utils::csv
===================

------------------------------------------------
Utility command to parse comma separated files.
------------------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::csv guess** ?\ *switches* ... ?

**blt::csv parse** ?\ *switches* ... ?

DESCRIPTION
-----------

This **blt::csv** command provide a utility for parsing comma separated
value (CSV) files.  This is like the CSV reading in the **blt::datatable**
expect that it outputs a list representing the data.

SYNTAX
------

**blt::csv guess** ?\ *switches* ... ?
  Examines the given CSV data and determines the separator character by
  looking at the first 20 lines of data at counting the most common
  separator character.  The guessed separator character is returned.
  *Switches* control how the data is parsed.  Either the **-file** or
  **-data** switch must be given, but not both. The following switches are
  available.

  **-comment** *commChar*
    Specifies a comment character.  Any line in the CSV data that starts
    with *commChar* is treated as a comment and ignored.  By default
    the comment character is "", indicating no comments.

  **-data**  *csvString*
    Read the CSV data from *csvString*.

  **-encoding**  *encodingName*
    Specifies the encoding of the CSV data.  

  **-file**  *fileName*
    Read the CSV input data from *fileName*.  If *fileName* starts with an
    '@' character, then what follows is the name of a TCL channel, instead
    of a file name.

  **-maxrows** *numRows*
    Specifies the number of rows to examine.  The default is 20.
    Comment lines and blank lines are ignored.

  **-possibleseparators**  *sepString*
    Specifies a string of chararacters to test as possible separators for
    the data. It *sepString* is "", then default set of characters is used.
    The default is ",\t|;".

  **-quote**  *quoteChar*
    Specifies the quote character.  This is by default the double quote (")
    character.

**blt::csv parse** ?\ *switches* ... ?
  Parses the given CSV data and returns a TCL list the data. Each element
  of the returned list represents a row. In turn, the list element for the
  row is itself, containing all the values for that particular row.
  *Switches* control how the data is parsed.  Either the **-file** or
  **-data** switch must be given, but not both. The following switches are
  available.

  **-comment** *commChar*
    Specifies a comment character.  Any line in the CSV data that starts
    with *commChar* is treated as a comment and ignored.  By default
    the comment character is "", indicating no comments.

  **-data**  *csvString*
    Read the CSV input data from *csvString*.

  **-emptyvalue**  *string*
    Specifies a string value to use for cells when empty fields
    are found in the CSV data.

  **-encoding**  *encodingName*
    Specifies the encoding of the CSV input data.  

  **-file**  *fileName*
    Read the CSV input data from *fileName*.  If *fileName* starts with an
    '@' character, then what follows is the name of a TCL channel, instead
    of a file name.
    
  **-maxrows** *numRows*
    Specifies the number of rows to examine.  The default is 20.
    Comment lines and blank lines are ignored.

  **-possibleseparators**  *sepString*
    Specifies a string of chararacters to test as possible separators for
    the data. It *sepString* is "", then default set of characters is used.
    The default is ",\t|;".

  **-quote**  *quoteChar*
    Specifies the quote character.  This is by default the double quote (")
    character.

  **-separator**  *sepChar*
    Specifies the separator character.  By default this is the comma (,)
    character. If *sepChar* is "", then the separator is determined by
    looking at the first 20 rows for the most frequent separator. The
    default is ",".



EXAMPLE
-------

You supply the CSV formatted data **blt::csv** command with either the
**-data** or **-file** switches.  One of these switches must be used.

  ::

     package require BLT

     set csvdata {
       a,b,c,d,e,f
       1,2,3,4,5
       A,B,C,D
       1,2,3
     }
     # Read CSV data from variable csvdata.
     blt::csv parse -data $csvdata 

The **guess** operation can be used to probe the data for the most
common separator character.

  ::

     set sep [blt::csv guess -data $csvdata -possiblesepartors "|\t"]


KEYWORDS
--------

csv

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
