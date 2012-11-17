
#
# tabset.tcl
#
# ----------------------------------------------------------------------
# Bindings for the BLT tabset widget
# ----------------------------------------------------------------------
#   AUTHOR:  George Howlett
#            Bell Labs Innovations for Lucent Technologies
#            gah@bell-labs.com
#            http://www.tcltk.com/blt
# ----------------------------------------------------------------------
# Copyright (c) 1998  Lucent Technologies, Inc.
# ======================================================================
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

#
# Indicates whether to activate (highlight) tabs when the mouse passes
# over them.  This is turned off during scan operations.
#

namespace eval blt {
    namespace eval Tabset {
	variable _private
	array set _private {
	    activate yes
	}
    }
}

# ----------------------------------------------------------------------
# 
# ButtonPress assignments
#
#   <ButtonPress-2>	Starts scan mechanism (pushes the tabs)
#   <B2-Motion>		Adjust scan
#   <ButtonRelease-2>	Stops scan
#   <ButtonPress-3>	Starts scan mechanism (pushes the tabs)
#   <B3-Motion>		Adjust scan
#   <ButtonRelease-3>	Stops scan
#
# ----------------------------------------------------------------------
bind BltTabset <B2-Motion> {
    %W scan dragto %x %y
}

bind BltTabset <ButtonPress-2> {
    set blt::Tabset::_private(cursor) [%W cget -cursor]
    set blt::Tabset::_private(activate) no
    %W configure -cursor hand1
    %W scan mark %x %y
}

bind BltTabset <ButtonRelease-2> {
    %W configure -cursor $::blt::Tabset::_private(cursor)
    set blt::Tabset::_private(activate) yes
    %W activate @%x,%y
}

bind BltTabset <B3-Motion> {
    %W scan dragto %x %y
}

bind BltTabset <ButtonPress-3> {
    set blt::Tabset::_private(cursor) [%W cget -cursor]
    set blt::Tabset::_private(activate) no
    %W configure -cursor hand1
    %W scan mark %x %y
}

bind BltTabset <ButtonRelease-3> {
    %W configure -cursor $blt::Tabset::_private(cursor)
    set blt::Tabset::_private(activate) yes
    %W activate @%x,%y
}

# ----------------------------------------------------------------------
# 
# KeyPress assignments
#
#   <KeyPress-Up>	Moves focus to the tab immediately above the 
#			current.
#   <KeyPress-Down>	Moves focus to the tab immediately below the 
#			current.
#   <KeyPress-Left>	Moves focus to the tab immediately left of the 
#			currently focused tab.
#   <KeyPress-Right>	Moves focus to the tab immediately right of the 
#			currently focused tab.
#   <KeyPress-space>	Invokes the commands associated with the current
#			tab.
#   <KeyPress-Return>	Same as above.
#   <KeyPress>		Go to next tab starting with the ASCII character.
#   <KeyPress-End>	Moves focus to the last tab.
#   <KeyPress-Home>	Moves focus to the last tab.
#
# ----------------------------------------------------------------------

bind BltTabset <KeyPress-Up> { 
    blt::Tabset::MoveFocus %W "up" 
}
bind BltTabset <KeyPress-Down> { 
    blt::Tabset::MoveFocus %W "down" 
}
bind BltTabset <KeyPress-Right> { 
    blt::Tabset::MoveFocus %W "right" 
}
bind BltTabset <KeyPress-Left> { 
    blt::Tabset::MoveFocus %W "left" 
}
bind BltTabset <KeyPress-Home> { 
    blt::Tabset::MoveFocus %W "first" 
}
bind BltTabset <KeyPress-End> { 
    blt::Tabset::MoveFocus %W "last" 
}
bind BltTabset <KeyPress-space> { 
    blt::Tabset::Select %W "focus" 
}

bind BltTabset <KeyPress-Return> { 
    %W invoke focus 
}

bind BltTabset <KeyPress> {
    if { [string match {[A-Za-z0-9]*} "%A"] } {
	blt::Tabset::FindMatch %W %A
    }
}

# ----------------------------------------------------------------------
#
# FindMatch --
#
#	Find the first tab (from the tab that currently has focus) 
#	starting with the same first letter as the tab.  It searches
#	in order of the tab positions and wraps around. If no tab
#	matches, it stops back at the current tab.
#
# Arguments:	
#	widget		Tabset widget.
#	key		ASCII character of key pressed
#
# ----------------------------------------------------------------------
proc blt::Tabset::FindMatch { w key } {
    set key [string tolower $key]
    set itab [$w index focus]
    set numTabs [$w size]
    for { set i 0 } { $i < $numTabs } { incr i } {
	if { [incr itab] >= $numTabs } {
	    set itab 0
	}
	set label [string tolower [$w tab cget $itab -text]]
	if { [string index $label 0] == $key } {
	    break
	}
    }
    $w focus $itab
    $w see focus
}

# ----------------------------------------------------------------------
#
# Select --
#
#	Invokes the command for the tab.  If the widget associated tab 
#	is currently torn off, the tearoff is raised.
#
# Arguments:	
#	widget		Tabset widget.
#	x y		Unused.
#
# ----------------------------------------------------------------------
proc blt::Tabset::Select { w tab } {
    set index [$w index $tab]
    if { $index != "" } {
	$w focus $index
	set tearoff [$w tearoff $index]
	if { ($tearoff != "") && ($tearoff != "$w") } {
	    raise [winfo toplevel $tearoff]
	}
	$w invoke $index
    }
}

# ----------------------------------------------------------------------
#
# MoveFocus --
#
#	Invokes the command for the tab.  If the widget associated tab 
#	is currently torn off, the tearoff is raised.
#
# Arguments:	
#	widget		Tabset widget.
#	x y		Unused.
#
# ----------------------------------------------------------------------
proc blt::Tabset::MoveFocus { w tab } {
    set index [$w index $tab]
    if { $index != "" } {
	$w focus $index
	$w select $index
    }
}

# ----------------------------------------------------------------------
#
# DestroyTearoff --
#
#	Destroys the toplevel window and the container tearoff 
#	window holding the embedded widget.  The widget is placed
#	back inside the tab.
#
# Arguments:	
#	widget		Tabset widget.
#	tab		Tab selected.
#
# ----------------------------------------------------------------------
proc blt::Tabset::DestroyTearoff { w tab } {
    set id [$w id $tab]
    set top "$w.toplevel-$id"
    if { [winfo exists $top] } {
	wm withdraw $top
	update
	$w tearoff $tab $w
	destroy $top
    }
}

# ----------------------------------------------------------------------
#
# CreateTearoff --
#
#	Creates a new toplevel window and moves the embedded widget
#	into it.  The toplevel is placed just below the tab.  The
#	DELETE WINDOW property is set so that if the toplevel window 
#	is requested to be deleted by the window manager, the embedded
#	widget is placed back inside of the tab.  Note also that 
#	if the tabset container is ever destroyed, the toplevel is
#	also destroyed.  
#
# Arguments:	
#	widget		Tabset widget.
#	tab		Tab selected.
#	x y		The coordinates of the mouse pointer.
#
# ----------------------------------------------------------------------

proc blt::Tabset::CreateTearoff { w tab rootX rootY } {

    # ------------------------------------------------------------------
    # When reparenting the window contained in the tab, check if the
    # window or any window in its hierarchy currently has focus.
    # Since we're reparenting windows behind its back, Tk can
    # mistakenly activate the keyboard focus when the mouse enters the
    # old toplevel.  The simplest way to deal with this problem is to
    # take the focus off the window and set it to the tabset widget
    # itself.
    # ------------------------------------------------------------------

    set focus [focus]
    set win [$w tab cget $tab -window]
    set index [$w index $tab]
    if { ($focus == $w) || ([string match  ${win}.* $focus]) } {
	focus -force $w
    }
    set id [$w id $index]
    set top "$w.toplevel-$id"
    toplevel $top
    $w tearoff $tab $top.container
    blt::table $top 0,0 $top.container -fill both

    incr rootX 10 ; incr rootY 10
    wm geometry $top +$rootX+$rootY

    set parent [winfo toplevel $w]
    wm title $top "[wm title $parent]: [$w tab cget $index -text]"
    #wm transient $top $parent

    # If the user tries to delete the toplevel, put the window back
    # into the tab folder.  

    wm protocol $top WM_DELETE_WINDOW \
	[list blt::Tabset::DestroyTearoff $w $tab]

    # If the container is ever destroyed, automatically destroy the
    # toplevel too.  

    bind $top.container <Destroy> [list destroy $top]
}

# ----------------------------------------------------------------------
#
# ToggleTearoff --
#
#	Toggles the tab tearoff.  If the tab contains a embedded widget, 
#	it is placed inside of a toplevel window.  If the widget has 
#	already been torn off, the widget is replaced back in the tab.
#
# Arguments:	
#	widget		tabset widget.
#	x y		The coordinates of the mouse pointer.
#
# ----------------------------------------------------------------------

proc blt::Tabset::ToggleTearoff { w index } {
    set tab [$w index $index]
    if { $tab == "" } {
	return
    }
    $w invoke $tab
    set win [$w tearoff $index]
    if { $win == "$w" } {
	foreach { x1 y1 x2 y2 } [$w extents $tab] break
	CreateTearoff $w $tab $x1 $y1
    } elseif { $win != "" } {
	DestroyTearoff $w $tab
    }
}

# ----------------------------------------------------------------------
#
# Init
#
#	Invoked from C whenever a new tabset widget is created.
#	Sets up the default bindings for the all tab entries.  
#	These bindings are local to the widget, so they can't be 
#	set through the usual widget class bind tags mechanism.
#
#	<Enter>		Activates the tab.
#	<Leave>		Deactivates all tabs.
#	<ButtonPress-1>	Selects the tab and invokes its command.
#	<Control-ButtonPress-1>	
#			Toggles the tab tearoff.  If the tab contains
#			a embedded widget, it is placed inside of a
#			toplevel window.  If the widget has already
#			been torn off, the widget is replaced back
#			in the tab.
#
# Arguments:	
#	widget		tabset widget
#
# ----------------------------------------------------------------------

proc blt::Tabset::Init { w } {
    $w bind all <Enter> { 
	if { $::blt::Tabset::_private(activate) } {
	    %W activate current
        }
    }
    $w bind all <Leave> { 
        %W activate "" 
    }
    $w bind all <ButtonPress-1> { 
	blt::Tabset::Select %W "current"
    }
    $w bind all <Control-ButtonPress-1> { 
	blt::Tabset::ToggleTearoff %W active
    }
    $w configure -perforationcommand [list blt::Tabset::ToggleTearoff $w]
    $w bind Perforation <Enter> { 
	%W perforation activate on
    }
    $w bind Perforation <Leave> { 
	%W perforation activate off
    }
    $w bind Perforation <ButtonRelease-1> { 
	%W perforation invoke 
    }
    $w bind Button <Enter> { 
	%W button activate current 
    }
    $w bind Button <Leave> { 
	%W button activate ""
    }
    $w bind Button <ButtonRelease-1> { 
	if { [catch {%W close current}] == 0 } {
	    %W delete current
	}
    }
    set _private(cursor) [$w cget -cursor]
}

