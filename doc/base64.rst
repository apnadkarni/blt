===================
blt::utils::base64
===================

------------------------------------------------
Utility command to encode/decode base64 data.
------------------------------------------------

:Author: George A. Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::base64 decode** *base64String*

**blt::base64 encode** *dataString*

DESCRIPTION
-----------

This **blt::base64** command provides a utility for encode and decoding
base64 strings.  

SYNTAX
------

**blt::base64 decode** *base64string*
  Decodes the give base64 formatted string returning the decoded string.

**blt::base64 encode** *dataString*
  Encodes *dataString* into base64 encoding.  The encoded string is
  returned.

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
     blt::base64 parse -data $base64data 

The **guess** operation can be used to probe the data for the most
common separator character.

  ::

     set sep [blt::base64 guess -data $base64data -possiblesepartors "|\t"]


KEYWORDS
--------

base64

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
