# -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- 
#
# bltComboTree.tcl
#
# Bindings for the BLT combotree widget
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

# FIXME: merge fixes from combomenu.tcl

namespace eval blt {
    namespace eval ComboTree {
	array set _private {
	    afterId -1
	    scroll 0
	    column ""
	    space   off
	    x 0
	    y 0
	}
    }
}

image create picture ::blt::ComboTree::openIcon -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}
image create picture ::blt::ComboTree::closeIcon -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM1WBrM+rAEMigJ8c3Kb3OSII6kGABhp1JnaK1VGwjwKwtvHqNzzd263M3H4n2OH1QBwGw6
    nQkAOw==
}


# ----------------------------------------------------------------------
#
# Initialize --
#
#	Invoked by internally by Combotree_Init routine.  Initializes
#	the default bindings for the combotree widget entries.  These
#	are local to the widget, so they can't be set through the
#	widget's class bind tags.
#
# ----------------------------------------------------------------------
proc blt::ComboTree::Initialize { w } {
    #
    # Active entry bindings
    #
    $w bind Entry <Enter> { 
	%W activate current
	%W entry highlight current 
    }
    $w bind Entry <Leave> { 
	%W activate none
	%W entry highlight "" 
    }

    #
    # Button bindings
    #
    $w button bind all <ButtonRelease-1> {
	set index [%W nearest %x %y blt::ComboTree::_private(who)]
	if { [%W index current] == $index && 
	     $blt::ComboTree::_private(who) == "button" } {
	    %W see -anchor nw current
	    %W toggle current
	}
    }
    $w button bind all <Enter> {
	%W button highlight current
    }
    $w button bind all <B1-Enter> {
	%W button highlight current
    }
    $w button bind all <Leave> {
	%W button highlight ""
    }

    #
    # ButtonPress-1
    #
    #	Performs the following operations:
    #
    #	1. Clears the previous selection.
    #	2. Selects the current entry.
    #	3. Sets the focus to this entry.
    #	4. Scrolls the entry into view.
    #	5. Sets the selection anchor to this entry, just in case
    #	   this is "multiple" mode.
    #
    
    #
    # ButtonRelease-1
    #
    #	For "multiple" mode only.  
    #
    $w bind Entry <ButtonRelease-3> { 
	%W invoke active
	%W unpost
	after cancel $blt::ComboTree::_private(afterId)
	set blt::ComboTree::_private(afterId) -1
	set blt::ComboTree::_private(scroll) 0
    }
    $w bind Entry <ButtonRelease-1> { 
	%W invoke active
	%W unpost
	after cancel $blt::ComboTree::_private(afterId)
	set blt::ComboTree::_private(afterId) -1
	set blt::ComboTree::_private(scroll) 0
    }

    $w bind Entry <Double-ButtonPress-1> {
	%W see -anchor nw active
	%W toggle active
    }

    #
    # Shift-ButtonPress-1
    #
    #	For "multiple" mode only.
    #

    $w bind Entry <Shift-ButtonPress-1> { 
	if { [%W cget -selectmode] == "multiple" && [%W selection present] } {
	    if { [%W index anchor] == "" } {
		%W selection anchor current
	    }
	    set index [%W index anchor]
	    %W selection clearall
	    %W selection set $index current
	} else {
	    blt::ComboTree::SetSelectionAnchor %W current
	}
    }
    $w bind Entry <Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Shift-B1-Motion> { 
	# do nothing
    }
    $w bind Entry <Shift-ButtonRelease-1> { 
	after cancel $blt::ComboTree::_private(afterId)
	set blt::ComboTree::_private(afterId) -1
	set blt::ComboTree::_private(scroll) 0
    }

    #
    # Control-ButtonPress-1
    #
    #	For "multiple" mode only.  
    #
    $w bind Entry <Control-ButtonPress-1> { 
	if { [%W cget -selectmode] == "multiple" } {
	    set index [%W index current]
	    %W selection toggle $index
	    %W selection anchor $index
	} else {
	    blt::ComboTree::SetSelectionAnchor %W current
	}
    }
    $w bind Entry <Control-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Control-B1-Motion> { 
	# do nothing
    }
    $w bind Entry <Control-ButtonRelease-1> { 
	after cancel $blt::ComboTree::_private(afterId)
	set blt::ComboTree::_private(afterId) -1
	set blt::ComboTree::_private(scroll) 0
    }

    $w bind Entry <Control-Shift-ButtonPress-1> { 
	if { [%W cget -selectmode] == "multiple" && [%W selection present] } {
	    if { [%W index anchor] == "" } {
		%W selection anchor current
	    }
	    if { [%W selection includes anchor] } {
		%W selection set anchor current
	    } else {
		%W selection clear anchor current
		%W selection set current
	    }
	} else {
	    blt::ComboTree::SetSelectionAnchor %W current
	}
    }
    $w bind Entry <Control-Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Control-Shift-B1-Motion> { 
	# do nothing
    }

    $w bind Entry <Shift-ButtonPress-3> { 
	blt::ComboTree::EditColumn %W %X %Y
    }
}

# ----------------------------------------------------------------------
#
# AutoScroll --
#
#	Invoked when the user is selecting elements in a combotree
#	widget and drags the mouse pointer outside of the widget.
#	Scrolls the view in the direction of the pointer.
#
# ----------------------------------------------------------------------
proc blt::ComboTree::AutoScroll { w } {
    variable _private

    if { ![winfo exists $w] } {
	return
    }
    set x $_private(x)
    set y $_private(y)

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
    SetSelectionAnchor $w $neighbor
    set _private(afterId) [after 50 blt::ComboTree::AutoScroll $w]
}

proc blt::ComboTree::SetSelectionAnchor { w tagOrId } {
    set index [$w index $tagOrId]
    # If the anchor hasn't changed, don't do anything
    if { $index != [$w index anchor] } {
	#$w selection clearall
	$w see $index
	#$w focus $index
	#$w selection set $index
	#$w selection anchor $index
    }
}

# ----------------------------------------------------------------------
#
# MoveFocus --
#
#	Invoked by KeyPress bindings.  Moves the active selection to
#	the entry <where>, which is an index such as "up", "down",
#	"prevsibling", "nextsibling", etc.
#
# ----------------------------------------------------------------------
proc blt::ComboTree::MoveFocus { w index } {
    $w activate $index
    $w see active
}

# ----------------------------------------------------------------------
#
# MovePage --
#
#	Invoked by KeyPress bindings.  Pages the current view up or
#	down.  The <where> argument should be either "top" or
#	"bottom".
#
# ----------------------------------------------------------------------
proc blt::ComboTree::MovePage { w where } {

    # If the focus is already at the top/bottom of the window, we want
    # to scroll a page. It's really one page minus an entry because we
    # want to see the last entry on the next/last page.
    if { [$w index focus] == [$w index view.$where] } {
        if {$where == "top"} {
	    $w yview scroll -1 pages
	    $w yview scroll 1 units
        } else {
	    $w yview scroll 1 pages
	    $w yview scroll -1 units
        }
    }
    update

    # Adjust the entry focus and the view.  Also activate the entry.
    # just in case the mouse point is not in the widget.
    $w entry highlight view.$where
    $w focus view.$where
    $w see view.$where
    if { [$w cget -selectmode] == "single" } {
        $w selection clearall
        $w selection set focus
    }
}

# ----------------------------------------------------------------------
#
# NextMatch --
#
#	Invoked by KeyPress bindings.  Searches for an entry that
#	starts with the letter <char> and makes that entry active.
#
# ----------------------------------------------------------------------
proc blt::ComboTree::NextMatch { w key } {
    if {[string match {[ -~]} $key]} {
	set last [$w index focus]
	set next [$w index next]
	while { $next != $last } {
	    set label [$w entry cget $next -label]
	    set label [string index $label 0]
	    if { [string tolower $label] == [string tolower $key] } {
		break
	    }
	    set next [$w index -at $next next]
	}
	$w focus $next
	if {[$w cget -selectmode] == "single"} {
	    $w selection clearall
	    $w selection set focus
	}
	$w see focus
    }
}

# 
# ButtonPress assignments
#
#	B1-Enter	start auto-scrolling
#	B1-Leave	stop auto-scrolling
#	ButtonPress-2	start scan
#	B2-Motion	adjust scan
#	ButtonRelease-2 stop scan
#

bind BltComboTree <ButtonPress-2> {
    set blt::ComboTree::_private(cursor) [%W cget -cursor]
    %W configure -cursor hand1
    %W scan mark %x %y
}

bind BltComboTree <B2-Motion> {
    %W scan dragto %x %y
}

bind BltComboTree <ButtonRelease-2> {
    %W configure -cursor $blt::ComboTree::_private(cursor)
}

bind BltComboTree <B1-Leave> {
    if { $blt::ComboTree::_private(scroll) } {
	blt::ComboTree::AutoScroll %W 
    }
}

bind BltComboTree <B1-Enter> {
    after cancel $blt::ComboTree::_private(afterId)
    set blt::ComboTree::_private(afterId) -1
}

# 
# KeyPress assignments
#
#	Up			
#	Down
#	Shift-Up
#	Shift-Down
#	Prior (PageUp)
#	Next  (PageDn)
#	Left
#	Right
#	space		Start selection toggle of entry currently with focus.
#	Return		Start selection toggle of entry currently with focus.
#	Home
#	End
#	F1
#	F2
#	ASCII char	Go to next open entry starting with character.
#
# KeyRelease
#
#	space		Stop selection toggle of entry currently with focus.
#	Return		Stop selection toggle of entry currently with focus.


bind BltComboTree <KeyPress-Up> {
    %W activate up
    %W see active
}

bind BltComboTree <KeyPress-Down> {
    %W activate down
    %W see active
}

bind BltComboTree <Shift-KeyPress-Up> {
    %W activate prevsibling
    %W see active
}

bind BltComboTree <Shift-KeyPress-Down> {
    %W activate nextsibling
    %W see active
}

bind BltComboTree <KeyPress-Prior> {
    %W activate top
    %W see active
}

bind BltComboTree <KeyPress-Next> {
    %W activate bottom
    %W see active
}

bind BltComboTree <KeyPress-Left> {
    %W close active
}

bind BltComboTree <KeyPress-Right> {
    %W open active
    %W see active
}

bind BltComboTree <KeyPress-space> {
    %W invoke active
}

bind BltComboTree <KeyPress-Return> {
    %W invoke active
}

bind BltComboTree <KeyPress> {
    blt::ComboTree::NextMatch %W %A
}

bind BltComboTree <KeyPress-Home> {
    %W open top
    %W see active -anchor w
}

bind BltComboTree <KeyPress-End> {
    blt::ComboTree::MoveFocus %W bottom
}

bind BltComboTree <KeyPress-F1> {
    %W open -recurse root
}

bind BltComboTree <KeyPress-F2> {
    eval %W close -r [%W entry children root] 
}

if {[string equal "x11" [tk windowingsystem]]} {
    bind BltComboTree <4> {
	%W yview scroll -5 units
    }
    bind BltComboTree <5> {
	%W yview scroll 5 units
    }
} else {
    bind BltComboTree <MouseWheel> {
	%W yview scroll [expr {- (%D / 120) * 4}] units
    }
}


proc blt::ComboTree::ConfigureScrollbars { menu } {
    set ys [$menu cget -yscrollbar]
    if { $ys != "" } {
	if { [$menu cget -yscrollcommand] == "" } {
	    $menu configure -yscrollcommand [list $ys set]
	}
	if { [$ys cget -command] == "" } {
	    $ys configure -command [list $menu yview] -orient vertical \
		-highlightthickness 0
	}
    }
    set xs [$menu cget -xscrollbar]
    if { $xs != "" } {
	if { [$menu cget -xscrollcommand] == "" } {
	    $menu configure -xscrollcommand [list $xs set]
	}
	if { [$xs cget -command] == "" } {
	    $xs configure -command [list $menu xview] -orient horizontal \
		-highlightthickness 0
	}
    }
}
