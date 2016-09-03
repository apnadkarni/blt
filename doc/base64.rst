===========================
blt::encode, blt::decode
===========================

------------------------------------------------
Command to encode/decode formatted data.
------------------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::decode ascii85** *encodedString* ?\ *switches*\ ...?

**blt::decode base64** *encodedString* ?\ *switches*\ ...?

**blt::decode hexadecimal** *encodedString* ?\ *switches*\ ...?

**blt::encode ascii85** *byteString* ?\ *switches*\ ...?

**blt::encode base64** *byteString* ?\ *switches*\ ...?

**blt::encode hexadecimal** *byteString* ?\ *switches*\ ...?

DESCRIPTION
-----------

This **blt::base64** command provides a utility for encode and decoding
base64 strings.  

SYNTAX
------

**blt::decode ascii85** *encodedString*  ?\ *switches*\ ...?
  Decodes *encodedString* returning the decoded string.  *EncodeString* is
  an ascii85 encoded string.  It may contain whitespace.  Invalid
  characters normally return an error unless the **-ignorebadchars** switch
  is present.  The special 'z' and 'y' encode characters are expanded
  automatically.  The following switches are available:

  **-data** *varName*
    Specifies a TCL variable to contain the decoded string.
    If *varName* is "", then no variable is written.

  **-file** *fileName*
    Specifies the name of a file or channel to contain the decoded string.
    If *fileName* is "", then no file is written. The default is "".
  
  **-ignorebadchars** 
    Indicates to ignore invalid characters in *encodedString*.
    The default is to return an error when an invalid character is found.

**blt::decode base64** *encodedString*  ?\ *switches*\ ...?
  Decodes *encodedString* returning the decoded string. *EncodeString* is a
  base64 encoded string. It may contain whitespace, but the number of
  encoded characters must be a mulitple of 4.  Invalid characters normally
  return an error unless the **-ignorebadchars** switch is present. The
  following switches are available:

  **-data** *varName*
    Specifies a TCL variable to contain the decoded string.
    If *varName* is "", then no variable is written.  The default is "".

  **-file** *fileName*
    Specifies the name of a file or channel to contain the decoded string.
    If *fileName* is "", then no file is written. The default is "".
  
  **-ignorebadchars** 
    Indicates to ignore invalid characters in *encodedString*.
    The default is to return an error when an invalid character is found.

**blt::decode hexadecimal** *encodedString*  ?\ *switches*\ ...?
  Decodes *encodedString* returning the decoded string. *EncodeString* is a
  hexadecimal encoded string.  It may contain whitespace, but the number of
  encoded characters must be a mulitple of 2.  Invalid characters normally
  return an error unless the **-ignorebadchars** switch is present.  The
  following switches are available:

  **-data** *varName*
    Specifies a TCL variable to contain the decoded string.
    If *varName* is "", then no variable is written. The default is "".

  **-file** *fileName*
    Specifies the name of a file or channel to contain the decoded string.
    If *fileName* is "", then no file is written. The default is "".
  
  **-ignorebadchars** 
    Indicates to ignore invalid characters in *encodedString*.
    The default is to return an error when an invalid character is found.
    
**blt::encode ascii85** *byteString*  ?\ *switches*\ ...?
  Encodes *byteString* with base64 encoding.  The encoded string is
  returned. The following switches are available:

  **-brackets**
     Indicates to add beginning and ending brackets "<-" and "->"  to
     the resulting encoded string
     
  **-data** *varName*
    Specifies a TCL variable to contain the encoded string.
    If *varName* is "", then no variable is written. The default is "".

  **-file** *fileName*
    Specifies the name of a file or channel to contain the encoded string.
    If *fileName* is "", then no file is written. The default is "".

  **-foldzeros**
    Indicates to encode a 4-byte block of zeros as a single 'z' character.
    By default, no compression is performed.

  **-foldspaces**
    Indicates to encode a 4-byte block of spaces as a single 'y' character.
    By default, no compression is performed.

  **-pad** *padString*
    Specifies a string to prefix to each line of encoded characters.
    The default is "".
    
  **-wrapchars** *wrapString*
    Specifies a string to append to each line of encoded characters.
    The default is "\n".
  
  **-wraplength** *numChars*
    Specifies the number of encoded characters on each line.  This does not
    include the padding or wrap chararacters (see the **-pad** and
    **-wrapchars** switches). If *numChars* is 0, then no line wrapping
    is performed. The default is 60.

**blt::encode base64** *byteString*  ?\ *switches*\ ...?
  Encodes *byteString* with base64 encoding.  The encoded string is
  returned. The following switches are available:

  **-data** *varName*
    Specifies a TCL variable to contain the encoded string.
    If *varName* is "", then no variable is written. The default is "".

  **-file** *fileName*
    Specifies the name of a file or channel to contain the encoded string.
    If *fileName* is "", then no file is written. The default is "".
  
  **-pad** *padString*
    Specifies a string to prefix to each line of encoded characters.
    The default is "".
    
  **-wrapchars** *wrapString*
    Specifies a string to append to each line of encoded characters.
    The default is "\n".
  
  **-wraplength** *numChars*
    Specifies the number of encoded characters on each line.  This does not
    include the padding or wrap chararacters (see the **-pad** and
    **-wrapchars** switches). If *numChars* is 0, then no line wrapping
    is performed. The default is 76.

**blt::encode hexadecimal** *byteString*  ?\ *switches*\ ...?
  Encodes *byteString* with base64 encoding.  The encoded string is
  returned. The following switches are available:

  **-data** *varName*
    Specifies a TCL variable to contain the encoded string.
    If *varName* is "", then no variable is written. The default is "".

  **-file** *fileName*
    Specifies the name of a file or channel to contain the encoded string.
    If *fileName* is "", then no file is written. The default is "".
  
  **-lowercase**
    Indicates to encode characters A through F as a through f respectively.
    The default is use upper case characters.
    
  **-pad** *padString*
    Specifies a string to prefix to each line of encoded characters.
    The default is "".
    
  **-wrapchars** *wrapString*
    Specifies a string to append to each line of encoded characters.
    The default is "\n".
  
  **-wraplength** *numChars*
    Specifies the number of encoded characters on each line.  This does not
    include the padding or wrap chararacters (see the **-pad** and
    **-wrapchars** switches). If *numChars* is 0, then no line wrapping
    is performed. The default is 60.


EXAMPLE
-------

You supply the BASE64 formatted data **blt::base64** command with either the
**-data** or **-file** switches.  One of these switches must be used.

  ::

     package require BLT

     set base64data {
       a,b,c,d,e,f
       1,2,3,4,5
       A,B,C,D
       1,2,3
     }
     # Read BASE64 data from variable base64data.
     blt::encode parse -data $base64data 

The **guess** operation can be used to probe the data for the most
common separator character.

  ::

     set sep [blt::base64 guess -data $base64data -possiblesepartors "|\t"]


KEYWORDS
--------

base64, ascii85, hexadecimal

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
