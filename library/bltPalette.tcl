# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# bltPalette.tcl
#
# Pre-defined color palettes for the BLT
#
# Copyright 2015 George A. Howlett. All rights reserved.  
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#   1) Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#   2) Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the
#      distribution.
#   3) Neither the name of the authors nor the names of its contributors
#      may be used to endorse or promote products derived from this
#      software without specific prior written permission.
#   4) Products derived from this software may not be called "BLT" nor may
#      "BLT" appear in their names without specific prior written
#      permission from the author.
#
#   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
#   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
#   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

blt::palette create rainbow -colorformat name -cdata {
    "red" "#FFA500" "yellow" "#008000" "blue" "#4B0082" "#EE82EE"
}
blt::palette create greyscale -colorformat name -cdata {
    "black" "white"
}
blt::palette create nanohub  -colorformat name -cdata {
    white yellow green cyan blue magenta
}

blt::palette create "BGYOR" -colorformat name -cdata {
    "blue" "#008000" "yellow" "#FFA500" "red" 
}

blt::palette create "ROYGB" -colorformat name -cdata {
    "red" "#FFA500" "yellow" "#008000" "blue" 
}

blt::palette create "RYGCB" -colorformat name -cdata {
    "red" "yellow" "green" "cyan" "blue"
}

blt::palette create "BCGYR" -colorformat name -cdata {
    "blue" "cyan" "green" "yellow" "red" 
}

if { [info exists blt_library] } {
    foreach file [glob -nocomplain $blt_library/palettes/*.ncmap \
		  $blt_library/palettes/*.rgb] {
	set tail [file tail $file]
	if { [catch {
	    blt::palette create $tail -colorfile $file
	} errs] != 0 } {
	    puts stderr "file=$file errs=$errs"
	}
    }
}

