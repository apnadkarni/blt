
# ======================================================================
#
# listview.tcl
#
# ----------------------------------------------------------------------
# Bindings for the BLT listview widget
# ----------------------------------------------------------------------
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies and that
# both that the copyright notice and warranty disclaimer appear in
# supporting documentation, and that the names of Lucent Technologies
# any of their entities not be used in advertising or publicity
# pertaining to distribution of the software without specific, written
# prior permission.
#
# Lucent Technologies disclaims all warranties with regard to this
# software, including all implied warranties of merchantability and
# fitness.  In no event shall Lucent be liable for any special, indirect
# or consequential damages or any damages whatsoever resulting from loss
# of use, data or profits, whether in an action of contract, negligence
# or other tortuous action, arising out of or in connection with the use
# or performance of this software.
#
# ======================================================================

namespace eval blt {
    namespace eval ListView {
	variable _private
	array set _private {
	    afterId	-1
	    column	""
	    scroll	0
	    space	off
	    x		0
	    y		0
	}
    }
}

image create picture ::blt::ListView::openIcon -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}
image create picture ::blt::ListView::closeIcon -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM1WBrM+rAEMigJ8c3Kb3OSII6kGABhp1JnaK1VGwjwKwtvHqNzzd263M3H4n2OH1QBwGw6
    nQkAOw==
}

if { $tcl_platform(platform) == "windows" } {
    if { $tk_version >= 8.3 } {
	set cursor "@[file join $blt_library treeview.cur]"
    } else {
	set cursor "size_we"
    }
    option add *BltListView.ResizeCursor [list $cursor]
} else {
    option add *BltListView.ResizeCursor \
	"@$blt_library/treeview.xbm $blt_library/treeview_m.xbm black white"
}

bind BltListView <Motion> {
    %W activate @%x,%y 
}

#
# ButtonPress-1
#
#	Performs the following operations:
#
#	1. Clears the previous selection.
#	2. Selects the current item.
#	3. Sets the focus to this item.
#	4. Scrolls the item into view.
#	5. Sets the selection anchor to this item, just in case
#	   this is "multiple" mode.
#

bind BltListView <ButtonPress-1> { 	
    blt::ListView::SetSelectionAnchor %W @%x,%y
    set blt::ListView::_private(scroll) 1
}

bind BltListView <Double-ButtonPress-1> {
    if { [%W index @%x,@%y] >= 0 } {
	blt::ListView::EditItem %W %X %Y
    }
}

#
# B1-Motion
#
#	For "multiple" mode only.  Saves the current location of the
#	pointer for auto-scrolling.  Resets the selection mark.  
#
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

#
# ButtonRelease-1
#
#	For "multiple" mode only.  
#
bind BltListView <ButtonRelease-1> { 
    if { [%W cget -selectmode] == "multiple" } {
	%W selection anchor @%x,%y
    }
    after cancel $blt::ListView::_private(afterId)
    set blt::ListView::_private(afterId) -1
    set blt::ListView::_private(scroll) 0
}

#
# Shift-ButtonPress-1
#
#	For "multiple" mode only.
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

bind BltListView <Shift-Double-ButtonPress-1> {
    # do nothing
}
bind BltListView <Shift-B1-Motion> { 
    # do nothing
}

bind BltListView <Shift-ButtonRelease-1> { 
    after cancel $blt::ListView::_private(afterId)
    set blt::ListView::_private(afterId) -1
    set blt::ListView::_private(scroll) 0
}

#
# Control-ButtonPress-1
#
#	For "multiple" mode only.  
#
bind BltListView <Control-ButtonPress-1> { 
    if { [%W cget -selectmode] == "multiple" } {
	%W selection toggle @%x,%y
	%W selection anchor @%x,%y
    } else {
	blt::ListView::SetSelectionAnchor %W @%x,%y
    }
}
bind BltListView <Control-Double-ButtonPress-1> {
    # do nothing
}
bind BltListView <Control-B1-Motion> { 
    # do nothing
}
bind BltListView <Control-ButtonRelease-1> { 
    after cancel $blt::ListView::_private(afterId)
    set blt::ListView::_private(afterId) -1
    set blt::ListView::_private(scroll) 0
}

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
bind BltListView <Control-Shift-Double-ButtonPress-1> {
    # do nothing
}
bind BltListView <Control-Shift-B1-Motion> { 
    # do nothing
}
bind BltListView <ButtonRelease-3> { 
    blt::ListView::EditItem %W %X %Y
}

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

# ----------------------------------------------------------------------
#
# AutoScroll --
#
#	Invoked when the user is selecting elements in a treeview
#	widget and drags the mouse pointer outside of the widget.
#	Scrolls the view in the direction of the pointer.
#
# ----------------------------------------------------------------------
proc blt::ListView::AutoScroll { w } {
    variable _private

    if { ![winfo exists $w] } {
	return
    }
    set x $_private(x)
    set y $_private(y)

    if { [$w cget -layoutmode] == "column" } {
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

# ----------------------------------------------------------------------
#
# MoveFocus --
#
#	Invoked by KeyPress bindings.  Moves the active selection to
#	the item <where>, which is an index such as "up", "down",
#	"prevsibling", "nextsibling", etc.
#
# ----------------------------------------------------------------------
proc blt::ListView::MoveFocus { w item } {
    catch {$w focus $item} 
    if { [$w cget -selectmode] == "single" } {
        $w selection clearall
        $w selection set focus
	$w selection anchor focus
    }
    $w see focus
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
proc blt::ListView::MovePage { w where } {

    # If the focus is already at the top/bottom of the window, we want
    # to scroll a page. It's really one page minus an item because we
    # want to see the last item on the next/last page.
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

    # Adjust the item focus and the view.  Also activate the item.
    # just in case the mouse point is not in the widget.
    $w activate view.$where
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
#	Invoked by KeyPress bindings.  Searches for an item that
#	starts with the letter <char> and makes that item active.
#
# ----------------------------------------------------------------------
proc blt::ListView::NextMatch { w key } {
    if {[string match {[ -~]} $key]} {
	set last [$w index focus]
	if { $last == -1 } {
	    return;			# No focus. 
	}
	set next [$w index next]
	if { $next == -1 } {
	    set next $last
	}
	while { $next != $last } {
	    set label [$w item cget $next -label]
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

#------------------------------------------------------------------------
#
# InsertText --
#
#	Inserts a text string into an item at the insertion cursor.  
#	If there is a selection in the item, and it covers the point 
#	of the insertion cursor, then delete the selection before 
#	inserting.
#
# Arguments:
#	w 	Widget where to insert the text.
#	text	Text string to insert (usually just a single character)
#
#------------------------------------------------------------------------
proc blt::ListView::InsertText { w text } {
    if { [string length $text] > 0 } {
	set index [$w index insert]
	if { ($index >= [$w index sel.first]) && 
	     ($index <= [$w index sel.last]) } {
	    $w delete sel.first sel.last
	}
	$w insert $index $text
    }
}

#------------------------------------------------------------------------
#
# Transpose -
#
#	This procedure implements the "transpose" function for item
#	widgets.  It tranposes the characters on either side of the
#	insertion cursor, unless the cursor is at the end of the line.
#	In this case it transposes the two characters to the left of
#	the cursor.  In either case, the cursor ends up to the right
#	of the transposed characters.
#
# Arguments:
#	w 	The item window.
#
#------------------------------------------------------------------------
proc blt::ListView::Transpose { w } {
    set i [$w index insert]
    if {$i < [$w index end]} {
	incr i
    }
    set first [expr {$i-2}]
    if {$first < 0} {
	return
    }
    set new [string index [$w get] [expr {$i-1}]][string index [$w get] $first]
    $w delete $first $i
    $w insert insert $new
}

#------------------------------------------------------------------------
#
# GetSelection --
#
#	Returns the selected text of the item with respect to the
#	-show option.
#
# Arguments:
#	w          Item window from which the text to get
#
#------------------------------------------------------------------------

proc blt::ListView::GetSelection { w } {
    set text [string range [$w get] [$w index sel.first] \
                       [expr [$w index sel.last] - 1]]
    if {[$w cget -show] != ""} {
	regsub -all . $text [string index [$w cget -show] 0] text
    }
    return $text
}

proc blt::ListView::EditItem { w x y } {
    $w see active
    if { [winfo exists $w.edit] } {
	destroy $w.edit
	return
    }
    if { ![$w edit -root -test $x $y] } {
	return
    }
    $w edit -root $x $y
    update
    if { [winfo exists $w.edit] } {
	focus $w.edit
	$w.edit selection range 0 end
	blt::grab push $w.edit
	tkwait window $w.edit
	blt::grab pop $w.edit
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

bind BltListView <ButtonPress-2> {
    set blt::ListView::_private(cursor) [%W cget -cursor]
    %W configure -cursor hand1
    %W scan mark %x %y
}

bind BltListView <B2-Motion> {
    %W scan dragto %x %y
}

bind BltListView <ButtonRelease-2> {
    %W configure -cursor $blt::ListView::_private(cursor)
}

bind BltListView <B1-Leave> {
    if { $blt::ListView::_private(scroll) } {
	blt::ListView::AutoScroll %W 
    }
}

bind BltListView <B1-Enter> {
    after cancel $blt::ListView::_private(afterId)
    set blt::ListView::_private(afterId) -1
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
#	space		Start selection toggle of item currently with focus.
#	Return		Start selection toggle of item currently with focus.
#	Home
#	End
#	ASCII char	Go to next open item starting with character.
#
# KeyRelease
#
#	space		Stop selection toggle of item currently with focus.
#	Return		Stop selection toggle of item currently with focus.


bind BltListView <KeyPress-Up> {
    blt::ListView::MoveFocus %W previous
    if { $blt::ListView::_private(space) } {
	%W selection toggle focus
    }
}

bind BltListView <KeyPress-Down> {
    blt::ListView::MoveFocus %W next
    if { $blt::ListView::_private(space) } {
	%W selection toggle focus
    }
}

bind BltListView <KeyPress-Home> {
    blt::ListView::MoveFocus %W 0
}

bind BltListView <KeyPress-End> {
    blt::ListView::MoveFocus %W end
}

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

bind BltListView <KeyRelease-space> { 
    set blt::ListView::_private(space) off
}

bind BltListView <KeyPress-Return> {
    blt::ListView::MoveFocus %W focus
    set blt::ListView::_private(space) on
}

bind BltListView <KeyRelease-Return> { 
    set blt::ListView::_private(space) off
}

if 0 {
bind BltListView <KeyPress> {
    blt::ListView::NextMatch %W %A
}
}

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
# Differences between id "current" and operation nearest.
#
#	set index [$w index current]
#	set index [$w nearest $x $y]
#
#	o Nearest gives you the closest item.
#	o current is "" if
#	   1) the pointer isn't over an item.
#	   2) the pointer is over a open/close button.
#	   3) 
#

#
#  Edit mode assignments
#
#	ButtonPress-3   Enables/disables edit mode on item.  Sets focus to 
#			item.
#
#  KeyPress
#
#	Left		Move insertion position to previous.
#	Right		Move insertion position to next.
#	Up		Move insertion position up one line.
#	Down		Move insertion position down one line.
#	Prior		Move insertion position up one line.
#	Next		Move insertion position down one line.
#	Return		End edit mode.
#	Shift-Return	Line feed.
#	Home		Move to first position.
#	End		Move to last position.
#	ASCII char	Insert character left of insertion point.
#	Del		Delete character right of insertion point.
#	Delete		Delete character left of insertion point.
#	Ctrl-X		Cut
#	Ctrl-V		Copy
#	Ctrl-P		Paste
#	
#  KeyRelease
#
#	ButtonPress-1	Start selection if in item, otherwise clear selection.
#	B1-Motion	Extend/reduce selection.
#	ButtonRelease-1 End selection if in item, otherwise use last
#			selection.
#	B1-Enter	Disabled.
#	B1-Leave	Disabled.
#	ButtonPress-2	Same as above.
#	B2-Motion	Same as above.
#	ButtonRelease-2	Same as above.
#	
#


# Standard Motif bindings:

bind BltListViewEditor <ButtonPress-1> {
    %W icursor @%x,%y
    %W selection clear
}

bind BltListViewEditor <Left> {
    %W icursor last
    %W selection clear
}

bind BltListViewEditor <Right> {
    %W icursor next
    %W selection clear
}

bind BltListViewEditor <Shift-Left> {
    set new [expr {[%W index insert] - 1}]
    if {![%W selection present]} {
	%W selection from insert
	%W selection to $new
    } else {
	%W selection adjust $new
    }
    %W icursor $new
}

bind BltListViewEditor <Shift-Right> {
    set new [expr {[%W index insert] + 1}]
    if {![%W selection present]} {
	%W selection from insert
	%W selection to $new
    } else {
	%W selection adjust $new
    }
    %W icursor $new
}

bind BltListViewEditor <Home> {
    %W icursor 0
    %W selection clear
}
bind BltListViewEditor <Shift-Home> {
    set new 0
    if {![%W selection present]} {
	%W selection from insert
	%W selection to $new
    } else {
	%W selection adjust $new
    }
    %W icursor $new
}
bind BltListViewEditor <End> {
    %W icursor end
    %W selection clear
}
bind BltListViewEditor <Shift-End> {
    set new end
    if {![%W selection present]} {
	%W selection from insert
	%W selection to $new
    } else {
	%W selection adjust $new
    }
    %W icursor $new
}

bind BltListViewEditor <Delete> {
    if { [%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete insert
    }
}

bind BltListViewEditor <BackSpace> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	set index [expr [%W index insert] - 1]
	if { $index >= 0 } {
	    %W delete $index $index
	}
    }
}

bind BltListViewEditor <Control-space> {
    %W selection from insert
}

bind BltListViewEditor <Select> {
    %W selection from insert
}

bind BltListViewEditor <Control-Shift-space> {
    %W selection adjust insert
}

bind BltListViewEditor <Shift-Select> {
    %W selection adjust insert
}

bind BltListViewEditor <Control-slash> {
    %W selection range 0 end
}

bind BltListViewEditor <Control-backslash> {
    %W selection clear
}

bind BltListViewEditor <KeyPress> {
    blt::ListView::InsertText %W %A
}

# Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for Escape, Return, and Tab.

bind BltListViewEditor <Alt-KeyPress> {
    # nothing
}

bind BltListViewEditor <Meta-KeyPress> {
    # nothing
}

bind BltListViewEditor <Control-KeyPress> {
    # nothing
}

bind BltListViewEditor <Escape> { 
    %W cancel 
}

bind BltListViewEditor <Return> { 
    %W apply 
}

bind BltListViewEditor <Shift-Return> {
    blt::ListView::InsertText %W "\n"
}

bind BltListViewEditor <KP_Enter> {
    # nothing
}

bind BltListViewEditor <Tab> {
    # nothing
}

if {![string compare $tcl_platform(platform) "macintosh"]} {
    bind BltListViewEditor <Command-KeyPress> {
	# nothing
    }
}

# On Windows, paste is done using Shift-Insert.  Shift-Insert already
# generates the <<Paste>> event, so we don't need to do anything here.
if { [string compare $tcl_platform(platform) "windows"] != 0 } {
    bind BltListViewEditor <Insert> {
	catch {blt::ListView::InsertText %W [selection get -displayof %W]}
    }
}

# Additional emacs-like bindings:
bind BltListViewEditor <ButtonRelease-3> {
    set parent [winfo parent %W]
    %W cancel
}

bind BltListViewEditor <Control-a> {
    %W icursor 0
    %W selection clear
}

bind BltListViewEditor <Control-b> {
    %W icursor [expr {[%W index insert] - 1}]
    %W selection clear
}

bind BltListViewEditor <Control-d> {
    %W delete insert
}

bind BltListViewEditor <Control-e> {
    %W icursor end
    %W selection clear
}

bind BltListViewEditor <Control-f> {
    %W icursor [expr {[%W index insert] + 1}]
    %W selection clear
}

bind BltListViewEditor <Control-h> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	set index [expr [%W index insert] - 1]
	if { $index >= 0 } {
	    %W delete $index $index
	}
    }
}

bind BltListViewEditor <Control-k> {
    %W delete insert end
}

if 0 {
    bind BltListViewEditor <Control-t> {
	blt::ListView::Transpose %W
    }
    bind BltListViewEditor <Meta-b> {
	%W icursor [blt::ListView::PreviousWord %W insert]
	%W selection clear
    }
    bind BltListViewEditor <Meta-d> {
	%W delete insert [blt::ListView::NextWord %W insert]
    }
    bind BltListViewEditor <Meta-f> {
	%W icursor [blt::ListView::NextWord %W insert]
	%W selection clear
    }
    bind BltListViewEditor <Meta-BackSpace> {
	%W delete [blt::ListView::PreviousWord %W insert] insert
    }
    bind BltListViewEditor <Meta-Delete> {
	%W delete [blt::ListView::PreviousWord %W insert] insert
    }
    # tkEntryNextWord -- Returns the index of the next word position
    # after a given position in the entry.  The next word is platform
    # dependent and may be either the next end-of-word position or the
    # next start-of-word position after the next end-of-word position.
    #
    # Arguments:
    # w -		The item window in which the cursor is to move.
    # start -	Position at which to start search.
    
    if {![string compare $tcl_platform(platform) "windows"]}  {
	proc blt::ListView::NextWord {w start} {
	    set pos [tcl_endOfWord [$w get] [$w index $start]]
	    if {$pos >= 0} {
		set pos [tcl_startOfNextWord [$w get] $pos]
	    }
	    if {$pos < 0} {
		return end
	    }
	    return $pos
	}
    } else {
	proc blt::ListView::NextWord {w start} {
	    set pos [tcl_endOfWord [$w get] [$w index $start]]
	    if {$pos < 0} {
		return end
	    }
	    return $pos
	}
    }
    
    # PreviousWord --
    #
    # Returns the index of the previous word position before a given
    # position in the item.
    #
    # Arguments:
    # w -		The item window in which the cursor is to move.
    # start -	Position at which to start search.
    
    proc blt::ListView::PreviousWord {w start} {
	set pos [tcl_startOfPreviousWord [$w get] [$w index $start]]
	if {$pos < 0} {
	    return 0
	}
	return $pos
    }
}

