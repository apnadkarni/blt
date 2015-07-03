===============
blt::cutbuffer
===============

------------------------------------
Manipulate X11 cut buffer properties
------------------------------------

:Author: George A Howlett
:Date:   2012-11-28
:Copyright: 2015 George A. Howlett.
:Version: 4.0
:Manual section: n
:Manual group: BLT Built-In Commands

SYNOPSIS
--------

**blt::cutbuffer get** ?\ *number*\ ?

**blt::cutbuffer rotate** ?\ *count*\ ?

**blt::cutbuffer set** *value* ?\ *number*\ ?

DESCRIPTION
-----------

The **blt::cutbuffer** command allows you to read or modify the eight X cut
buffer properties. You can also rotate the buffers properties.

OPERATIONS
----------

The following operations are available for the **blt::cutbuffer** command:

**cutbuffer get** ?\ *bufferNumber*\ ?
  Returns the value of a cutbuffer *bufferNumber*.  *BufferNumber* must be
  an integer between 0 and 7.  The default is 0.  The cutbuffer is returned
  exactly, except that NUL bytes are converted to '@' characters.  If a cut
  buffer for *bufferNumber* does not exist, then "" is returned.

**cutbuffer rotate** ?\ *count*\ ?
  Rotates the cut buffers by *count*. *Count* must be an integer between -7
  and 7. The default is 1.

**cutbuffer set** *value* ?\ *bufferNumber*\ ?  
  Sets the cutbuffer *bufferNumber* to *value*.  *BufferNumber* must be an
  integer between 0 and 7.  The default is 0.

KEYWORDS
--------

cut buffer, property

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
