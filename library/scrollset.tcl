# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# scrollset.tcl
#
# Bindings for the BLT scrollset widget
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

namespace eval blt {
    namespace eval Scrollset {
	#empty
    }
}

proc blt::Scrollset::ConfigureScrollbars { scrollset } {
    set xscrollbar [$scrollset cget -xscrollbar]
    set yscrollbar [$scrollset cget -yscrollbar]
    set slave [$scrollset cget -window]
    if { $slave != "" } {
	set yscrollcmd [$scrollset cget -yscrollcommand]
	if { $yscrollcmd == "" } {
	    set yscrollcmd [list $slave yview]
	}
	if { [catch $yscrollcmd] == 0 } {
	    $slave configure -yscrollcommand [list $scrollset yset] 
	}
	set xscrollcmd [$scrollset cget -xscrollcommand]
	if { $xscrollcmd == "" } {
	    set xscrollcmd [list $slave xview]
	}
	if { [catch $xscrollcmd] == 0 } {
	    $slave configure -xscrollcommand [list $scrollset xset]
	}
    }
    if { $xscrollbar != "" } {
	$xscrollbar configure -command [list $scrollset xview] \
	    -orient horizontal -highlightthickness 0 
    }
    if { $yscrollbar != "" } {
	$yscrollbar configure -command [list $scrollset yview] \
	    -orient vertical  -highlightthickness 0 
    }
}

