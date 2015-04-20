#
# dnd.tcl
#
# Bindings for the BLT drag&drop command
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

if { $tcl_version >= 8.0 } {
    set cmd blt::dnd
} else {
    set cmd dnd
}
for { set i 1 } { $i <= 5 } { incr i } {
    bind BltDndButton$i <ButtonPress-$i>  [list $cmd select %W %X %Y %t]
    bind BltDndButton$i <B$i-Motion>	  [list $cmd drag %W %X %Y]
    bind BltDndButton$i <ButtonRelease-$i> [list $cmd drop %W %X %Y]
}

# ----------------------------------------------------------------------
#
# DndInit --
#
#	Invoked from C whenever a new drag&drop source is created.
#	Sets up the default bindings for the drag&drop source.
#
#	<ButtonPress-?>	 Starts the drag operation.
#	<B?-Motion>	 Updates the drag.
#	<ButtonRelease-?> Drop the data on the target.
#
# Arguments:	
#	widget		source widget
#	button		Mouse button used to activate drag.
#	cmd		"dragdrop" or "blt::dragdrop"
#
# ----------------------------------------------------------------------

proc blt::DndInit { widget button } {
    set tagList {}
    if { $button > 0 } {
	lappend tagList BltDndButton$button
    }
    foreach tag [bindtags $widget] {
	if { ![string match BltDndButton* $tag] } {
	    lappend tagList $tag
	}
    }
    bindtags $widget $tagList
}

proc blt::DndStdDrop { widget args } {
    array set info $args
    set fmt [lindex $info(formats) 0]
    dnd pull $widget $fmt 
    return 0
}

proc blt::PrintInfo { array } {
    upvar $array state

    parray state
    if { $info(state) & 0x01 } {
	puts "Shift-Drop"
    }
    if { $info(state) & 0x02 } {
	puts "CapsLock-Drop"
    }
    if { $info(state) & 0x04 } {
	puts "Control-Drop"
    }
    if { $info(state) & 0x08 } {
	puts "Alt-Drop"
    }
    if { $info(state) & 0x10 } {
	puts "NumLock-Drop"
    }
}
