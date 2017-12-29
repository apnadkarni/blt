# -*- mode: tcl; tcl-indent-level: 4; indent-tabs-mode: nil -*- 
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
	    afterId	-1
	    column	""
	    lastButton	-1
	    lastEntry	-1
	    scroll	0
	    space	off
	    trace	0
	    x		0
	    y		0
	    popOnRelease    0
	}
	proc trace { mesg } {
	    variable _private 
	    if { $_private(trace) } {
		puts stderr $mesg
	    }
	}
    }
}

if { [blt::winop xdpi] > 150 } {
    image create picture ::blt::ComboTree::closeIcon \
        -file $blt_library/icons/32x32/folder.png 
    image create picture ::blt::ComboTree::openIcon \
        -file $blt_library/icons/32x32/folder-open.png
} else {
    image create picture ::blt::ComboTree::closeIcon \
        -file $blt_library/icons/16x16/folder.png 
    image create picture ::blt::ComboTree::openIcon \
        -file $blt_library/icons/16x16/folder-open.png
}    

# Left
#   Close the current node.
bind BltComboTree <KeyPress-Left> {
    %W close active
}

# Right (arrow key)
#   Open the current node.
bind BltComboTree <KeyPress-Right> {
    %W open active
    %W see active
}

# Up (arrow key)
#   Move up to the previous entry. 
bind BltComboTree <KeyPress-Up> {
    %W activate up
    %W see active
}

# Down (arrow key)
#   Move down to the next entry.
bind BltComboTree <KeyPress-Down> {
    %W activate down
    %W see active
}

# Home
#   Move the first entry.  Ignores nodes whose ancestors are closed.
bind BltComboTree <KeyPress-Home> {
    %W open top
    %W see active -anchor w
}

# End 
#   Move the last entry. Ignores nodes whose ancestors are closed.
bind BltComboTree <KeyPress-End> {
    blt::ComboTree::MoveFocus %W bottom
}

# PgUp 
#   Move the first entry in the view.
bind BltComboTree <KeyPress-Prior> {
    %W activate top
    %W see active
}

# PgDn
#   Move the last entry in the view.
bind BltComboTree <KeyPress-Next> {
    %W activate bottom
    %W see active
}

# Shift+Up (arrow key)
#   Move the previous sibling node.
bind BltComboTree <Shift-KeyPress-Up> {
    %W activate prevsibling
    %W see active
}

# Shift+Down (arrow key)
#   Move the next sibling node.
bind BltComboTree <Shift-KeyPress-Down> {
    %W activate nextsibling
    %W see active
}

# Space (arrow key)
#   Invoke 
bind BltComboTree <KeyPress-space> {
    %W invoke active
}

# Return
#   Invoke 
bind BltComboTree <KeyPress-Return> {
    %W invoke active
}

# F1
#   Open all entries.
bind BltComboTree <KeyPress-F1> {
    %W open -recurse root
}

# F2
#   Close all entries.
bind BltComboTree <KeyPress-F2> {
    eval %W close -r [%W entry children root] 
}

# Any other key press.
#   Goto the next matching entry.
bind BltComboTree <KeyPress> {
    blt::ComboTree::NextMatch %W %A
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

bind BltComboTree <Motion> {
    blt::ComboTree::HighlightActiveEntry %W %x %y
}

# ButtonPress-1
bind BltComboTree <ButtonPress-1> {
    blt::ComboTree::trace "blt::ComboTree ButtonPress-1 on widget"
    blt::ComboTree::HandleButtonPress %W %X %Y
}
# B1-Motion
bind BltComboTree <B1-Motion> {
    blt::ComboTree::trace "blt::ComboTree B1-Motion on widget"
    blt::ComboTree::HighlightActiveEntry %W %x %y
    #blt::ComboTree::ButtonMotionEvent %W %X %Y
    set blt::ComboTree::_private(scroll) 1
}

# ButtonRelease-1
bind BltComboTree <ButtonRelease-1> {
    blt::ComboTree::trace "blt::ComboTree ButtonRelease-1 on widget"
    blt::ComboTree::ButtonReleaseEvent %W %X %Y
}

# B1 Enter
#   Stop auto-scrolling
bind BltComboTree <B1-Enter> {
    after cancel $blt::ComboTree::_private(afterId)
    set blt::ComboTree::_private(afterId) -1
}

# B1 Leave
#   Start auto-scrolling
bind BltComboTree <B1-Leave> {
    if { $blt::ComboTree::_private(scroll) } {
	blt::ComboTree::AutoScroll %W 
    }
}

# ButtonPress 2
#  Start scanning
bind BltComboTree <ButtonPress-2> {
    set blt::ComboTree::_private(cursor) [%W cget -cursor]
    %W configure -cursor hand1
    %W scan mark %x %y
}

# B2 Motion
#  Continue scanning
bind BltComboTree <B2-Motion> {
    %W scan dragto %x %y
}

# ButtonRelease 2
#  Stop scanning
bind BltComboTree <ButtonRelease-2> {
    %W configure -cursor $blt::ComboTree::_private(cursor)
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
    # Button bindings
    #

    # ButtonPress-1
    #   Save the index of the current entry (whose button was pressed).
    $w button bind all <ButtonPress-1> {
        set blt::ComboTree::_private(lastButton) [%W index current]
    }

    # ButtonRelease-1
    #   If over the same button where the button was pressed, open or close
    #   the entry.
    $w button bind all <ButtonRelease-1> {
        blt::ComboTree::trace "blt::ComboTree ButtonRelease-1 for Button"
	set index [%W nearest %x %y blt::ComboTree::_private(who)]
	if { [%W index current] == $index && 
	     $blt::ComboTree::_private(who) == "button" } {
	    %W see -anchor nw current
	    %W toggle current
	}
    }
    if 0 {
	$w button bind all <Enter> {
	    blt::ComboTree::trace "blt::ComboTree Enter Button"
	    %W button highlight current
	    set blt::ComboTree::_private(lastButton) [%W index current]
	}
	$w button bind all <Leave> {
	    blt::ComboTree::trace "blt::ComboTree Leave Button"
	    %W button highlight ""
	    set blt::ComboTree::_private(lastButton) -1
	}
	if 0 {
	    $w button bind all <B1-Enter> {
		%W button highlight current
	    }
	}
	
	
	#
	# ButtonRelease-1
	#
	#	For "multiple" mode only.  
	#
	$w bind Entry <ButtonRelease-3> { 
	    %W invoke active
	    #%W unpost
	    after cancel $blt::ComboTree::_private(afterId)
	    set blt::ComboTree::_private(afterId) -1
	    set blt::ComboTree::_private(scroll) 0
	}
	# ButtonPress-1
	#   Sets the entry, set focus, clear previous selection.
	$w bind Entry <ButtonPress-1> { 	
	    blt::ComboTree::trace "blt::ComboTree ButtonPress-1 for Entry"
	    blt::ComboTree::HandleButtonPress %W %x %y
	    blt::ComboTree::SetEntry %W current
	}
	
	# B1-Motion
	#	Saves the current location of the pointer for auto-scrolling.
	$w bind Entry <B1-Motion> { 
	    set blt::ComboTree::_private(x) %x
	    set blt::ComboTree::_private(y) %y
	    set index [%W nearest %x %y]
	    set blt::ComboTree::_private(scroll) 1
	    blt::ComboTree::SetEntry %W $index
	}
	
	# ButtonRelease-1
	#  Sets the select anchor and in invokes the entry's -command option.
	$w bind Entry <ButtonRelease-1> { 
	    blt::ComboTree::trace "blt::ComboTree ButtonRelease-1 for Entry: [%W index active]"
	    blt::ComboTree::ButtonReleaseEvent %W %X %Y
	}
	
	# Double-ButtonPress-1
	#   Open or close the entry.
	$w bind Entry <Double-ButtonPress-1> {
	    %W see -anchor nw active
	    %W toggle active
	}
	
	# Shift-ButtonPress-1
	#
	$w bind Entry <Shift-ButtonPress-1> { 
	    blt::ComboTree::SetEntry %W current
	}
	# Shift-Double-ButtonPress-1
	#   Prevent less specific bindings for triggering.
	$w bind Entry <Shift-Double-ButtonPress-1> {
	    # Do nothing.
	}
	# Shift-B1-Motion
	#   Prevent less specific bindings for triggering.
	$w bind Entry <Shift-B1-Motion> { 
	    # Do nothing.
	}
	# Shift-ButtonRelease-1
	#   Turn off autoscrolling.
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
	    blt::ComboTree::SetEntry %W current
	}
	# Control-Double-ButtonPress-1
	#   Prevent less specific bindings for triggering.
	$w bind Entry <Control-Double-ButtonPress-1> {
	    # Do nothing.
	}
	# Control-B1-Motion
	#   Prevent less specific bindings for triggering.
	$w bind Entry <Control-B1-Motion> { 
	    # Do nothing.
	}
	$w bind Entry <Control-ButtonRelease-1> { 
	    after cancel $blt::ComboTree::_private(afterId)
	    set blt::ComboTree::_private(afterId) -1
	    set blt::ComboTree::_private(scroll) 0
	}
	# Control-Shift-ButtonRelease-1
	$w bind Entry <Control-Shift-ButtonPress-1> { 
	    blt::ComboTree::SetEntry %W current
	}
	# Control-Shift-Double-ButtonPress-1
	#   Prevent less specific bindings for triggering.
	$w bind Entry <Control-Shift-Double-ButtonPress-1> {
	    # Do nothing.
	}
    }
    # Control-Shift-B1-Motion
    #   Prevent less specific bindings for triggering.
    $w bind Entry <Control-Shift-B1-Motion> { 
	# Do nothing.
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
    SetEntry $w $neighbor
    set _private(afterId) [after 50 blt::ComboTree::AutoScroll $w]
}

proc blt::ComboTree::SetEntry { w tagOrId } {
    variable _private

    blt::ComboTree::trace "blt::ComboTree::SetEntry"
    set index [$w index $tagOrId]
    # If the anchor hasn't changed, don't do anything
    if { $index == [$w index anchor] } {
	return
    }
    $w see $index
    $w activate $index
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
    if { [$w index active] == [$w index view.$where] } {
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
    $w activate view.$where
    $w see view.$where
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
	set last [$w index active]
	set next [$w index next]
	while { $next != $last } {
	    set label [$w entry cget $next -label]
	    set label [string index $label 0]
	    if { [string tolower $label] == [string tolower $key] } {
		break
	    }
	    set next [$w index -at $next next]
	}
	$w active $next
	$w see active
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

proc ::blt::ComboTree::ButtonReleaseEvent { menu x y } {
    variable _private
					
    blt::ComboTree::trace "blt::ComboTree::ButtonReleaseEvent menu=$menu x=$x y=$y popOnRelease=$_private(popOnRelease)"

    after cancel $_private(afterId)
    set _private(afterId) -1
    set _private(scroll) 0
    set entry [$menu index active]
    if { $entry != -1  && [$menu identify $entry -root $x $y] == "button" } {
	puts stderr "returning from button release"
	return
    } else {
	puts stderr "entry=$entry x=$x y=$y"
	if { $entry != -1 } {
	    puts stderr "???identify=[$menu identify $entry -root $x $y]"
	}
    }
    set popOnRelease 1
    if { !$_private(popOnRelease) } {
	set popOnRelease 0
    }
    set parent [winfo parent $menu]
    set w [winfo containing -display $menu $x $y]
    if { $w != $menu && $w != $parent } {
	set popOnRelease 1
    }
    blt::ComboTree::trace "blt::ComboTree::ButtonReleaseEvent entry is $entry"
    if { $entry != -1 || $popOnRelease } {
	# This isn't the first time the menu was posted.  That happens when
	# the menubutton is pressed, the menu is posted, and the grab is
	# set on the menu.  This routine gets called on the button release.
	# Any further button releases should unpost the menu.  Just not on
	# the first release.
	$menu unpost 
	blt::grab pop $menu
    }
    set _private(popOnRelease) 0
    if { $entry != -1 } {
	$menu invoke $entry
    }
}


proc ::blt::ComboTree::HandleButtonPress { menu x y } {
    variable _private

    blt::ComboTree::trace "blt::ComboTree::HandleButtonPress menu=$menu x=$x y=$y"
    # This is called only when the grab is on.  This means that menu will
    # already be posted at this point.  On release, unpost the menu.
    set _private(popOnRelease) 1
}

#
# MotionEvent --
#
#	Process a motion event on the given menu.  Check if the the button
#	release occurred over a cascade or the first menu.  Cascade menus
#	are stacked in reverse order of their posting.  This is so that if
#	menus overlap (a cascade menu is on top of a previous menu) we will
#	find the topmost cascade.
#
#	Once we find a menu, trim the cascade stack removing cascade menus
#	that are no longer available.
#
proc ::blt::ComboTree::ButtonMotionEvent { menu x y } {
    variable _private

    blt::ComboTree::trace "blt::ComboTree::ButtonMotionEvent menu=$menu x=$x y=$y"
    # Handle activation of other combobuttons. 
    set w [winfo containing -display $menu $x $y]
    set parent [winfo parent $menu]
    if { $w != "" && $w != $parent && 
	 [winfo class $parent] == "BltComboButton" &&
	 [winfo class $w] == "BltComboButton" } {
	# Release the current combobutton, including removing the grab.
	#$menu unpost
	# Reset the grab to the new button.
	blt::grab set -global $w 
	# Simulate pressing the new combobutton widget.
	event generate $w <ButtonPress-1>
    }
}

proc ::blt::ComboTree::HighlightActiveEntry { menu x y } {
    variable _private

    set entry [$menu index @$x,$y]
    if { $entry != -1  } {
	$menu activate $entry
    } else {
	$menu deactivate
    }
}
