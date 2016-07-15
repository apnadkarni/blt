=============
blt::checksum
=============

----------------------------------
Compute checksum for given input.
----------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::checksum crc32** ?\ *switches* ... ?

**blt::checksum md5** ?\ *switches* ... ?

DESCRIPTION
-----------

This **blt::checksum** command lets you compute MD5 or CRC-32 checksums
given either a string or file.

SYNTAX
------

**blt::checksum crc32** ?\ *switches* ... ?
  Computes the cyclic redundancy check (CRC) as a 32-bit interger.
  The 8 character hexidecimal number representing this checksum is returned.
  *Switches* control the given input.  Either the **-file** or
  **-data** switch must be provided, but not both. The following switches are
  available.

  **-data**  *string*
    Computes the CRC-32 hash from *string*.

  **-file**  *fileName*
    Computes the CRC-32 hash from *fileName*. If *fileName* starts with an
    '@' character, then what follows is the name of a TCL channel, instead
    of a file name (such as "@stdin").

**blt::checksum md5** ?\ *switches* ... ?
  Computes the MD5 hash. A 32 character hexidecimal string is returned
  representing the checksum.  *Switches* control how the data is parsed.
  Either the **-file** or **-data** switch must be given, but not both. The
  following switches are available.

  **-data** *string*
    Computes the md5 hash from *string*.

  **-file**  *fileName*
    Computes the md5 hash from the contents of *fileName*.  If *fileName*
    starts with an '@' character, then what follows is the name of a TCL
    channel, instead of a file name (such as "@stdin"). 

EXAMPLE
-------

You supply data to checksum to the **blt::checksum** command with either
the **-data** or **-file** switches.  One of these switches must be used.

  ::

     package require BLT

     set data {
       a,b,c,d,e,f
       1,2,3,4,5
       A,B,C,D
       1,2,3
     }
     # Compute the crc-32 checksum.
     blt::checksum crc32 -data $data 

     # Compute the md5 checksum.
     blt::checksum md5 -data $data 

KEYWORDS
--------

crc32, md5

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

The crc32 implementation is covered by the following copyright.

  Copyright (C) 1986 Gary S. Brown.  You may use this program, or
  code or tables extracted from it, as desired without restriction.

The md5 hash implementation is covered by the following copyright.

  Copyright (C) 1999, 2000, 2002 Aladdin Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from
  the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

  L. Peter Deutsch
  ghost@aladdin.com


