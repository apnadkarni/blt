# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# bltListView.tcl
#
# Bindings for the BLT listview widget.
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
    namespace eval ListView {
	variable _private
	array set _private {
	    afterId	-1
	    scroll	0
	    space	off
	    x		0
	    y		0
	}
    }
}

# Motion
bind BltListView <Motion> {
    %W activate @%x,%y 
}

# ButtonPress-1
bind BltListView <ButtonPress-1> { 	
    blt::ListView::SetSelectionAnchor %W @%x,%y
    set blt::ListView::_private(scroll) 1
}

# B1-Motion
bind BltListView <B1-Motion> { 
    set blt::ListView::_private(x) %x
    set blt::ListView::_private(y) %y
    if { [%W cget -selectmode] == "multiple" } {
	%W selection mark @%x,%y
	%W activate @%x,%y
    } else {
	blt::ListView::SetSelectionAnchor %W @%x,%y
    }
}
# ButtonRelease-1
bind BltListView <ButtonRelease-1> { 
    if { [%W cget -selectmode] == "multiple" } {
	%W selection anchor @%x,%y
    }
    after cancel $blt::ListView::_private(afterId)
    set blt::ListView::_private(afterId) -1
    set blt::ListView::_private(scroll) 0
}
# B1-Leave
bind BltListView <B1-Leave> {
    if { $blt::ListView::_private(scroll) } {
	blt::ListView::AutoScroll %W 
    }
}
# B1-Enter
bind BltListView <B1-Enter> {
    after cancel $blt::ListView::_private(afterId)
    set blt::ListView::_private(afterId) -1
}


# ButtonPress-2
bind BltListView <ButtonPress-2> {
    set blt::ListView::_private(cursor) [%W cget -cursor]
    %W configure -cursor hand1
    %W scan mark %x %y
}
# B2-Motion
bind BltListView <B2-Motion> {
    %W scan dragto %x %y
}
# ButtonRelease-2
bind BltListView <ButtonRelease-2> {
    %W configure -cursor $blt::ListView::_private(cursor)
}


# ButtonRelease-3
bind BltListView <ButtonRelease-3> { 
    blt::ListView::EditItem %W %X %Y
}


# Double-ButtonPress-1
bind BltListView <Double-ButtonPress-1> {
    if { [%W index @%x,@%y] >= 0 } {
	blt::ListView::EditItem %W %X %Y
    }
}

# Shift-ButtonPress-1
#
bind BltListView <Shift-ButtonPress-1> { 
    if { [%W cget -selectmode] == "multiple" && [%W selection present] } {
	if { [%W selection anchor] == -1 } {
	    %W selection anchor @%x,%y
	}
	set index [%W selection anchor]
	if { $index >= 0 } {
	    %W selection clearall
	    %W selection set $index @%x,%y
	}
    } else {
	blt::ListView::SetSelectionAnchor %W @%x,%y
    }
}

bind BltListView <Shift-B1-Motion> { 
    # do nothing
}

bind BltListView <Shift-ButtonRelease-1> { 
    after cancel $blt::ListView::_private(afterId)
    set blt::ListView::_private(afterId) -1
    set blt::ListView::_private(scroll) 0
}

bind BltListView <Shift-Double-ButtonPress-1> {
    # do nothing
}

# Control-ButtonPress-1
bind BltListView <Control-ButtonPress-1> { 
    if { [%W cget -selectmode] == "multiple" } {
	%W selection toggle @%x,%y
	%W selection anchor @%x,%y
    } else {
	blt::ListView::SetSelectionAnchor %W @%x,%y
    }
}
# Control-B1-Motion
bind BltListView <Control-B1-Motion> { 
    # do nothing
}
# Control-ButtonRelease-1
bind BltListView <Control-ButtonRelease-1> { 
    after cancel $blt::ListView::_private(afterId)
    set blt::ListView::_private(afterId) -1
    set blt::ListView::_private(scroll) 0
}
bind BltListView <Control-Double-ButtonPress-1> {
    # do nothing
}

# Control-Shift-ButtonPress-1
bind BltListView <Control-Shift-ButtonPress-1> { 
    if { [%W cget -selectmode] == "multiple" && [%W selection present] } {
	if { [%W index anchor] == -1 } {
	    %W selection anchor @%x,%y
	}
	if { [%W selection includes anchor] } {
	    %W selection set anchor @%x,%y
	} else {
	    %W selection clear anchor @%x,%y
	    %W selection set @%x,%y
	}
    } else {
	blt::ListView::SetSelectionAnchor %W @%x,%y
    }
}
# Control-Shift-B1-Motion
bind BltListView <Control-Shift-B1-Motion> { 
    # do nothing
}
# Control-Shift-Double-ButtonPress-1
bind BltListView <Control-Shift-Double-ButtonPress-1> {
    # do nothing
}
# KeyPress-Up
bind BltListView <KeyPress-Up> {
    blt::ListView::MoveFocus %W previous
    if { $blt::ListView::_private(space) } {
	%W selection toggle focus
    }
}
# KeyPress-Down
bind BltListView <KeyPress-Down> {
    blt::ListView::MoveFocus %W next
    if { $blt::ListView::_private(space) } {
	%W selection toggle focus
    }
}
# KeyPress-Home
bind BltListView <KeyPress-Home> {
    blt::ListView::MoveFocus %W 0
}
# KeyPress-End
bind BltListView <KeyPress-End> {
    blt::ListView::MoveFocus %W end
}
# KeyPress-space
bind BltListView <KeyPress-space> {
    if { [%W cget -selectmode] == "single" } {
	if { [%W selection includes focus] } {
	    %W selection clearall
	} else {
	    %W selection clearall
	    %W selection set focus
	}
    } else {
	%W selection toggle focus
    }
    set blt::ListView::_private(space) on
}
# KeyRelease-space
bind BltListView <KeyRelease-space> { 
    set blt::ListView::_private(space) off
}
# KeyPress-Return
bind BltListView <KeyPress-Return> {
    blt::ListView::MoveFocus %W focus
    set blt::ListView::_private(space) on
}
# KeyRelease-Return
bind BltListView <KeyRelease-Return> { 
    set blt::ListView::_private(space) off
}

# KeyPress-Any
bind BltListView <KeyPress> {
    if { [string compare %A {}] == 0 } {
	continue
    }
    set index [%W find "%A*" -from next -count 1 -wrap]
    if { $index >= 0 } {
	%W see $index
	%W focus $index
    }
}
# Mouse wheel
if {[string equal "x11" [tk windowingsystem]]} {
    bind BltListView <4> {
	%W yview scroll -5 units
    }
    bind BltListView <5> {
	%W yview scroll 5 units
    }
} else {
    bind BltListView <MouseWheel> {
	%W yview scroll [expr {- (%D / 120) * 4}] units
    }
}


#
# AutoScroll --
#
#	Invoked when the user is selecting elements in a listview widget
#	and drags the mouse pointer outside of the widget.  Scrolls the
#	view in the direction of the pointer.
#
proc blt::ListView::AutoScroll { w } {
    variable _private

    if { ![winfo exists $w] } {
	return
    }
    set x $_private(x)
    set y $_private(y)

    if { [$w cget -layoutmode] == "columns" } {
	set index [$w nearest $x $y]
	if { $x >= [winfo width $w] } {
	    $w xview scroll 1 units
	    set neighbor down
	} elseif { $x < 0 } {
	    $w xview scroll -1 units
	    set neighbor up
	} else {
	    set neighbor $index
	}
    } else {
	# This covers both row and icons layouts.
	set index [$w nearest $x $y]
	if {$y >= [winfo height $w]} {
	    $w yview scroll 1 units
	    set neighbor down
	} elseif {$y < 0} {
	    $w yview scroll -1 units
	    set neighbor up
	} else {
	    set neighbor $index
	}
    }
    if { [$w cget -selectmode] == "single" } {
	if { [$w exists $neighbor] } {
	    SetSelectionAnchor $w $neighbor
	}
    } else {
	catch { $w selection mark $index }
    }
    set _private(afterId) [after 50 blt::ListView::AutoScroll $w]
}

#
# SetSelectionAnchor --
#
proc blt::ListView::SetSelectionAnchor { w item } {
    $w selection clearall
    set index [$w index $item]
    if { $index >= 0 } {
	$w see $index
	$w focus $index
	$w selection set $index
	$w selection anchor $index
    }
}

#
# MoveFocus --
#
#	Invoked by KeyPress bindings.  Moves the active selection to the
#	item, which is an index such as "up", "down", etc.
#
proc blt::ListView::MoveFocus { w item } {
    catch {$w focus $item} 
    if { [$w cget -selectmode] == "single" } {
        $w selection clearall
        $w selection set focus
	$w selection anchor focus
    }
    $w see focus
}

