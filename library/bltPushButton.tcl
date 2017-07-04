# -*- mode: tcl; tcl-indent-level: 4; indent-tabs-mode: nil -*- 
#
# bltPushButton.tcl --
#
# This file defines the default bindings for Tk label, button,
# checkbutton, and radiobutton widgets and provides procedures
# that help in implementing those bindings.
#
# Copyright (c) 1992-1994 The Regents of the University of California.
# Copyright (c) 1994-1996 Sun Microsystems, Inc.
# Copyright (c) 2002 ActiveState Corporation.
#
#   See the file "license.terms" for information on usage and redistribution
#   of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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

namespace eval blt {
    namespace eval Button {
	array set _private {
	    afterId -1
	    repeated  ""
	    window ""
	    trace		0
	    buttonWindow ""
	}
	proc trace { mesg } {
	    variable _private
	    if { $_private(trace) } {
		puts stderr $mesg
	    }
	}
    }
}

#-------------------------------------------------------------------------
# The code below creates the default class bindings for buttons.
#-------------------------------------------------------------------------

switch -glob -- [tk windowingsystem] {
    "classic" - "aqua" {
	source [file join $blt_library bltMacButton.tcl]
    }
    "x11" {
	source [file join $blt_library bltUnixButton.tcl]
    }
    "win*" {
	source [file join $blt_library bltWinButton.tcl]
    }
}

##################
# Shared routines
##################

# Invoke --
#
#	The procedure below is called when a button is invoked through
#	the keyboard.  It simulates a press of the button via the mouse.
#
#	Arguments:
#	w -		The name of the widget.
#
proc ::blt::Button::Invoke w {
    if {[$w cget -state] != "disabled"} {
	set oldRelief [$w cget -relief]
	set oldState [$w cget -state]
	$w configure -state active -relief sunken
	update idletasks
	after 100
	$w configure -state $oldState -relief $oldRelief
	uplevel #0 [list $w invoke]
    }
}

# AutoInvoke --
#
#	Invoke an auto-repeating button, and set it up to continue to repeat.
#
# Arguments:
#	w	button to invoke.
#
# Results:
#	None.
#
# Side effects:
#	May create an after event to call ::blt::Button::AutoInvoke.
#
proc ::blt::Button::AutoInvoke {w} {
    variable _private
    after cancel $_private(afterId)
    set delay [$w cget -repeatinterval]
    if {$_private(window) eq $w} {
	incr _private(repeated)
	uplevel #0 [list $w invoke]
    }
    if {$delay > 0} {
	set _private(afterId) [after $delay [list blt::Button::AutoInvoke $w]]
    }
}

# CheckRadioInvoke --
#
#	The procedure below is invoked when the mouse button is pressed in
#	a checkbutton or radiobutton widget, or when the widget is invoked
#	through the keyboard.  It invokes the widget if it isn't disabled.
#
#	Arguments:
#	w -	The name of the widget.
#	cmd -	The subcommand to invoke (one of invoke, select, or deselect).
#
proc ::blt::Button::CheckRadioInvoke {w {cmd invoke}} {
    if {[$w cget -state] != "disabled"} {
	uplevel #0 [list $w $cmd]
    }
}
bind BltTkButton <space> {
    blt::Button::Invoke %W
}
bind BltTkCheckbutton <space> {
    blt::Button::CheckRadioInvoke %W
}
bind BltTkRadiobutton <space> {
    blt::Button::CheckRadioInvoke %W
}
bind BltTkPushbutton <space> {
    blt::Button::CheckRadioInvoke %W
}
bind BltTkButton <FocusIn> {
    #empty
}
bind BltTkButton <Enter> {
    blt::Button::Enter %W
}
bind BltTkButton <Leave> {
    blt::Button::Leave %W
}
bind BltTkButton <1> {
    blt::Button::Down %W
}
bind BltTkButton <ButtonRelease-1> {
    blt::Button::Up %W
}
bind BltTkCheckbutton <FocusIn> {
    #empty
}
bind BltTkCheckbutton <Leave> {
    blt::Button::Leave %W
}
bind BltTkRadiobutton <FocusIn> {
    #empty
}
bind BltTkRadiobutton <Leave> {
    blt::Button::Leave %W
}
bind BltTkPushbutton <FocusIn> {
    #empty
}
bind BltTkPushbutton <Leave> {
    blt::Button::Leave %W
}

