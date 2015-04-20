# -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- 
#
# bltScrollbar.tcl --
#
# This file defines the default bindings for Tk scrollbar widgets.
# It also provides procedures that help in implementing the bindings.
#
# Copyright (c) 1994 The Regents of the University of California.
# Copyright (c) 1994-1996 Sun Microsystems, Inc.
#
#   See the file "license.terms" for information on usage and
#   redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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

#-------------------------------------------------------------------------
# The code below creates the default class bindings for scrollbars.
#-------------------------------------------------------------------------

# Standard Motif bindings:

namespace eval ::blt::TkScrollbar {
    variable _private

    array set _private {
	activeBg	""
	afterId		-1
	border		1
	initPos		0
	initValues	""
	x		0
	y		0
	relief		flat
    }
}

bind BltTkScrollbar <Enter> {
    if {$tk_strictMotif} {
	set blt::TkScrollbar::_private(activeBg) [%W cget -activebackground]
	%W configure -activebackground [%W cget -background]
    }
    %W activate [%W identify %x %y]
}
bind BltTkScrollbar <Motion> {
    %W activate [%W identify %x %y]
}

# The "info exists" command in the following binding handles the
# situation where a Leave event occurs for a scrollbar without the Enter
# event.  This seems to happen on some systems (such as Solaris 2.4) for
# unknown reasons.

bind BltTkScrollbar <Leave> {
    if {$tk_strictMotif && [info exists blt::TkScrollbar::_private(activeBg)]} {
	%W configure -activebackground $blt::TkScrollbar::_private(activeBg)
    }
    %W activate {}
}
bind BltTkScrollbar <1> {
    blt::TkScrollbar::ScrollButtonDown %W %x %y
}
bind BltTkScrollbar <B1-Motion> {
    blt::TkScrollbar::ScrollDrag %W %x %y
}
bind BltTkScrollbar <B1-B2-Motion> {
    blt::TkScrollbar::ScrollDrag %W %x %y
}
bind BltTkScrollbar <ButtonRelease-1> {
    blt::TkScrollbar::ScrollButtonUp %W %x %y
}
bind BltTkScrollbar <B1-Leave> {
    # Prevents <Leave> binding from being invoked.
}
bind BltTkScrollbar <B1-Enter> {
    # Prevents <Enter> binding from being invoked.
}
bind BltTkScrollbar <2> {
    blt::TkScrollbar::ScrollButton2Down %W %x %y
}
bind BltTkScrollbar <B1-2> {
    # Do nothing, since button 1 is already down.
}
bind BltTkScrollbar <B2-1> {
    # Do nothing, since button 2 is already down.
}
bind BltTkScrollbar <B2-Motion> {
    blt::TkScrollbar::ScrollDrag %W %x %y
}
bind BltTkScrollbar <ButtonRelease-2> {
    blt::TkScrollbar::ScrollButtonUp %W %x %y
}
bind BltTkScrollbar <B1-ButtonRelease-2> {
    # Do nothing:  B1 release will handle it.
}
bind BltTkScrollbar <B2-ButtonRelease-1> {
    # Do nothing:  B2 release will handle it.
}
bind BltTkScrollbar <B2-Leave> {
    # Prevents <Leave> binding from being invoked.
}
bind BltTkScrollbar <B2-Enter> {
    # Prevents <Enter> binding from being invoked.
}
bind BltTkScrollbar <Control-1> {
    blt::TkScrollbar::ScrollTopBottom %W %x %y
}
bind BltTkScrollbar <Control-2> {
    blt::TkScrollbar::ScrollTopBottom %W %x %y
}

bind BltTkScrollbar <Up> {
    blt::TkScrollbar::ScrollByUnits %W v -1
}
bind BltTkScrollbar <Down> {
    blt::TkScrollbar::ScrollByUnits %W v 1
}
bind BltTkScrollbar <Control-Up> {
    blt::TkScrollbar::ScrollByPages %W v -1
}
bind BltTkScrollbar <Control-Down> {
    blt::TkScrollbar::ScrollByPages %W v 1
}
bind BltTkScrollbar <Left> {
    blt::TkScrollbar::ScrollByUnits %W h -1
}
bind BltTkScrollbar <Right> {
    blt::TkScrollbar::ScrollByUnits %W h 1
}
bind BltTkScrollbar <Control-Left> {
    blt::TkScrollbar::ScrollByPages %W h -1
}
bind BltTkScrollbar <Control-Right> {
    blt::TkScrollbar::ScrollByPages %W h 1
}
bind BltTkScrollbar <Prior> {
    blt::TkScrollbar::ScrollByPages %W hv -1
}
bind BltTkScrollbar <Next> {
    blt::TkScrollbar::ScrollByPages %W hv 1
}
bind BltTkScrollbar <Home> {
    blt::TkScrollbar::ScrollToPos %W 0
}
bind BltTkScrollbar <End> {
    blt::TkScrollbar::ScrollToPos %W 1
}


if {[tk windowingsystem] eq "classic" || [tk windowingsystem] eq "aqua"} {
    bind BltTkScrollbar <MouseWheel> {
        blt::TkScrollbar::ScrollByUnits %W v [expr {- (%D)}]
    }
    bind BltTkScrollbar <Option-MouseWheel> {
        blt::TkScrollbar::ScrollByUnits %W v [expr {-10 * (%D)}]
    }
    bind BltTkScrollbar <Shift-MouseWheel> {
        blt::TkScrollbar::ScrollByUnits %W h [expr {- (%D)}]
    }
    bind BltTkScrollbar <Shift-Option-MouseWheel> {
        blt::TkScrollbar::ScrollByUnits %W h [expr {-10 * (%D)}]
    }
}
# blt::TkScrollbar::ScrollButtonDown --
# This procedure is invoked when a button is pressed in a scrollbar.
# It changes the way the scrollbar is displayed and takes actions
# depending on where the mouse is.
#
# Arguments:
# w -		The scrollbar widget.
# x, y -	Mouse coordinates.

proc blt::TkScrollbar::ScrollButtonDown {w x y} {
    variable _private

    set _private(relief) [$w cget -activerelief]
    set _private(border) [$w cget -activebackground]
    $w configure -activerelief sunken
    set element [$w identify $x $y]
    $w select $element
    if {$element eq "slider"} {
	ScrollStartDrag $w $x $y
    } else {
	ScrollSelect $w $element initial
    }
}

# ScrollButtonUp --
# This procedure is invoked when a button is released in a scrollbar.
# It cancels scans and auto-repeats that were in progress, and restores
# the way the active element is displayed.
#
# Arguments:
# w -		The scrollbar widget.
# x, y -	Mouse coordinates.

proc ::blt::TkScrollbar::ScrollButtonUp {w x y} {
    variable _private

    CancelRepeat
    if {[info exists _private(relief)]} {
	# Avoid error due to spurious release events
	$w configure -activerelief $_private(relief)
	ScrollEndDrag $w $x $y
	set element [$w identify $x $y]
	$w activate [$w identify $x $y]
	$w select {}
    }
}

# ScrollSelect --
# This procedure is invoked when a button is pressed over the scrollbar.
# It invokes one of several scrolling actions depending on where in
# the scrollbar the button was pressed.
#
# Arguments:
# w -		The scrollbar widget.
# element -	The element of the scrollbar that was selected, such
#		as "arrow1" or "trough2".  Shouldn't be "slider".
# repeat -	Whether and how to auto-repeat the action:  "noRepeat"
#		means don't auto-repeat, "initial" means this is the
#		first action in an auto-repeat sequence, and "again"
#		means this is the second repetition or later.

proc ::blt::TkScrollbar::ScrollSelect {w element repeat} {
    variable _private

    if {![winfo exists $w]} {
	return
    }
    switch -- $element {
	"arrow1"	{ScrollByUnits $w hv -1}
	"trough1"	{ScrollByPages $w hv -1}
	"trough2"	{ScrollByPages $w hv 1}
	"arrow2"	{ScrollByUnits $w hv 1}
	default		{return}
    }
    set cmd [list blt::TkScrollbar::ScrollSelect $w $element again]
    if {$repeat eq "again"} {
	set _private(afterId) [after [$w cget -repeatinterval] $cmd]
    } elseif {$repeat eq "initial"} {
	set delay [$w cget -repeatdelay]
	if {$delay > 0} {
	    set _private(afterId) [after $delay $cmd]
	}
    }
}

# ScrollStartDrag --
# This procedure is called to initiate a drag of the slider.  It just
# remembers the starting position of the mouse and slider.
#
# Arguments:
# w -		The scrollbar widget.
# x, y -	The mouse position at the start of the drag operation.

proc ::blt::TkScrollbar::ScrollStartDrag {w x y} {
    variable _private

    if {[$w cget -command] eq ""} {
	return
    }
    set _private(x) $x
    set _private(y) $y
    set _private(initValues) [$w get]
    set iv0 [lindex $_private(initValues) 0]
    if {[llength $_private(initValues)] == 2} {
	set _private(initPos) $iv0
    } elseif {$iv0 == 0} {
	set _private(initPos) 0.0
    } else {
	set _private(initPos) [expr {(double([lindex $_private(initValues) 2])) \
		/ [lindex $_private(initValues) 0]}]
    }
}

# ScrollDrag --
# This procedure is called for each mouse motion even when the slider
# is being dragged.  It notifies the associated widget if we're not
# jump scrolling, and it just updates the scrollbar if we are jump
# scrolling.
#
# Arguments:
# w -		The scrollbar widget.
# x, y -	The current mouse position.

proc ::blt::TkScrollbar::ScrollDrag {w x y} {
    variable _private

    if {$_private(initPos) eq ""} {
	return
    }
    set delta \
	[$w delta [expr {$x - $_private(x)}] [expr {$y - $_private(y)}]]
    if {[$w cget -jump]} {
	if {[llength $_private(initValues)] == 2} {
	    $w set [expr {[lindex $_private(initValues) 0] + $delta}] \
		    [expr {[lindex $_private(initValues) 1] + $delta}]
	} else {
	    set delta [expr {round($delta * [lindex $_private(initValues) 0])}]
	    eval [list $w] set [lreplace $_private(initValues) 2 3 \
		    [expr {[lindex $_private(initValues) 2] + $delta}] \
		    [expr {[lindex $_private(initValues) 3] + $delta}]]
	}
    } else {
	ScrollToPos $w [expr {$_private(initPos) + $delta}]
    }
}

# ScrollEndDrag --
# This procedure is called to end an interactive drag of the slider.
# It scrolls the window if we're in jump mode, otherwise it does nothing.
#
# Arguments:
# w -		The scrollbar widget.
# x, y -	The mouse position at the end of the drag operation.

proc ::blt::TkScrollbar::ScrollEndDrag {w x y} {
    variable _private

    if {$_private(initPos) eq ""} {
	return
    }
    if {[$w cget -jump]} {
	set delta [$w delta [expr {$x-$_private(x)}] [expr {$y-$_private(y)}]]
	ScrollToPos $w [expr {$_private(initPos) + $delta}]
    }
    set _private(initPos) ""
}

# ScrollByUnits --
# This procedure tells the scrollbar's associated widget to scroll up
# or down by a given number of units.  It notifies the associated widget
# in different ways for old and new command syntaxes.
#
# Arguments:
# w -		The scrollbar widget.
# orient -	Which kinds of scrollbars this applies to:  "h" for
#		horizontal, "v" for vertical, "hv" for both.
# amount -	How many units to scroll:  typically 1 or -1.

proc ::blt::TkScrollbar::ScrollByUnits {w orient amount} {
    set cmd [$w cget -command]
    if {$cmd eq "" || ([string first [string index [$w cget -orient] 0] $orient] < 0)} {
	return
    }
    set info [$w get]
    if {[llength $info] == 2} {
	uplevel #0 $cmd scroll $amount units
    } else {
	uplevel #0 $cmd [expr {[lindex $info 2] + $amount}]
    }
}

# ScrollByPages --
# This procedure tells the scrollbar's associated widget to scroll up
# or down by a given number of screenfuls.  It notifies the associated
# widget in different ways for old and new command syntaxes.
#
# Arguments:
# w -		The scrollbar widget.
# orient -	Which kinds of scrollbars this applies to:  "h" for
#		horizontal, "v" for vertical, "hv" for both.
# amount -	How many screens to scroll:  typically 1 or -1.

proc ::blt::TkScrollbar::ScrollByPages {w orient amount} {
    set cmd [$w cget -command]
    if {$cmd eq "" || ([string first [string index [$w cget -orient] 0] $orient] < 0)} {
	return
    }
    set info [$w get]
    if {[llength $info] == 2} {
	uplevel #0 $cmd scroll $amount pages
    } else {
	uplevel #0 $cmd [expr {[lindex $info 2] + $amount*([lindex $info 1] - 1)}]
    }
}

# ScrollToPos --
# This procedure tells the scrollbar's associated widget to scroll to
# a particular location, given by a fraction between 0 and 1.  It notifies
# the associated widget in different ways for old and new command syntaxes.
#
# Arguments:
# w -		The scrollbar widget.
# pos -		A fraction between 0 and 1 indicating a desired position
#		in the document.

proc ::blt::TkScrollbar::ScrollToPos {w pos} {
    set cmd [$w cget -command]
    if {$cmd eq ""} {
	return
    }
    set info [$w get]
    if {[llength $info] == 2} {
	uplevel #0 $cmd moveto $pos
    } else {
	uplevel #0 $cmd [expr {round([lindex $info 0]*$pos)}]
    }
}

#
# ScrollTopBottom --
#
#	Scroll to the top or bottom of the document, depending on the mouse
#	position.
#
# Arguments:
#	w 	The scrollbar widget.
#	x, y 	Mouse coordinates within the widget.
#
proc ::blt::TkScrollbar::ScrollTopBottom {w x y} {
    variable _private

    set element [$w identify $x $y]
    if {[string match *1 $element]} {
	ScrollToPos $w 0
    } elseif {[string match *2 $element]} {
	ScrollToPos $w 1
    }
    # Set _private(relief), since it's needed by ScrollButtonUp.
    set _private(relief) [$w cget -activerelief]
}

#
# ScrollButton2Down --
#
#	This procedure is invoked when button 2 is pressed over a scrollbar.
#	If the button is over the trough or slider, it sets the scrollbar to
#	the mouse position and starts a slider drag.  Otherwise it just
#	behaves the same as button 1.
#
# Arguments:
#	w 	The scrollbar widget.
#	x, y 	Mouse coordinates within the widget.

proc ::blt::TkScrollbar::ScrollButton2Down {w x y} {
    variable _private

    set element [$w identify $x $y]
    if {[string match {arrow[12]} $element]} {
	ScrollButtonDown $w $x $y
	return
    }
    ScrollToPos $w [$w fraction $x $y]
    set _private(relief) [$w cget -activerelief]

    # Need the "update idletasks" below so that the widget calls us
    # back to reset the actual scrollbar position before we start the
    # slider drag.

    update idletasks
    $w configure -activerelief sunken 
    $w activate slider
    ScrollStartDrag $w $x $y
}

proc ::blt::TkScrollbar::CancelRepeat {} {
    variable _private
    after cancel $_private(afterId)
    set _private(afterId) -1
}
