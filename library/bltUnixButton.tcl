# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# bltUnixButton.tcl
#
# Bindings for the BLT button widget for Unix platforms.
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

#####################
# Unix implementation
#####################

bind BltTkCheckbutton <Return> {
    if {!$tk_strictMotif} {
	blt::Button::CheckRadioInvoke %W
    }
}

bind BltTkRadiobutton <Return> {
    if {!$tk_strictMotif} {
	blt::Button::CheckRadioInvoke %W
    }
}

bind BltTkPushbutton <Return> {
    if {!$tk_strictMotif} {
	blt::Button::CheckRadioInvoke %W
    }
}

bind BltTkCheckbutton <1> {
    blt::Button::CheckRadioInvoke %W
}

bind BltTkRadiobutton <1> {
    blt::Button::CheckRadioInvoke %W
}

bind BltTkPushbutton <1> {
    blt::Button::CheckRadioInvoke %W
}

bind BltTkCheckbutton <Enter> {
    blt::Button::Enter %W
}

bind BltTkRadiobutton <Enter> {
    blt::Button::Enter %W
}

bind BltTkPushbutton <Enter> {
    blt::Button::Enter %W
}

# Enter --
#
# The procedure below is invoked when the mouse pointer enters a
# button widget.  It records the button we're in and changes the
# state of the button to active unless the button is disabled.
#
# Arguments:
# w -		The name of the widget.

proc ::blt::Button::Enter {w} {
    variable _private
    if { [$w cget -state] != "disabled" } {
	# On unix the state is active just with mouse-over
	$w configure -state active

	# If the mouse button is down, set the relief to sunken on entry.
	# Overwise, if there's an -overrelief value, set the relief to that.

	set _private($w,relief) [$w cget -relief]
	if {$_private(buttonWindow) eq $w} {
	    $w configure -relief sunken
	    set _private($w,prelief) sunken
	} elseif {[set over [$w cget -overrelief]] ne ""} {
	    $w configure -relief $over
	    set _private($w,prelief) $over
	}
    }
    set _private(window) $w
}

# Leave --
#
# The procedure below is invoked when the mouse pointer leaves a
# button widget.  It changes the state of the button back to inactive.
# Restore any modified relief too.
#
# Arguments:
# w -		The name of the widget.

proc ::blt::Button::Leave w {
    variable _private
    if {[$w cget -state] != "disabled"} {
	$w configure -state normal
    }

    # Restore the original button relief if it was changed by Tk.
    # That is signaled by the existence of _private($w,prelief).

    if {[info exists _private($w,relief)]} {
	if {[info exists _private($w,prelief)] && \
		$_private($w,prelief) eq [$w cget -relief]} {
	    $w configure -relief $_private($w,relief)
	}
	unset -nocomplain _private($w,relief) _private($w,prelief)
    }

    set _private(window) ""
}

# Down --
#
# The procedure below is invoked when the mouse button is pressed in
# a button widget.  It records the fact that the mouse is in the button,
# saves the button's relief so it can be restored later, and changes
# the relief to sunken.
#
# Arguments:
# w -		The name of the widget.

proc ::blt::Button::Down w {
    variable _private

    # Only save the button's relief if it does not yet exist.  If there
    # is an overrelief setting, Priv($w,relief) will already have been set,
    # and the current value of the -relief option will be incorrect.

    if {![info exists _private($w,relief)]} {
	set _private($w,relief) [$w cget -relief]
    }

    if {[$w cget -state] != "disabled"} {
	set _private(buttonWindow) $w
	$w configure -relief sunken
	set _private($w,prelief) sunken

	# If this button has a repeatdelay set up, get it going with an after
	after cancel $_private(afterId)
	set delay [$w cget -repeatdelay]
	set _private(repeated) 0
	if {$delay > 0} {
	    set cmd [list blt::Button::AutoInvoke $w]
	    set _private(afterId) [after $delay $cmd]
	}
    }
}

# Up --
#
# The procedure below is invoked when the mouse button is released
# in a button widget.  It restores the button's relief and invokes
# the command as long as the mouse hasn't left the button.
#
# Arguments:
# w -		The name of the widget.

proc ::blt::Button::Up w {
    variable _private
    if {$w eq $_private(buttonWindow)} {
	set _private(buttonWindow) ""

	# Restore the button's relief if it was cached.

	if {[info exists _private($w,relief)]} {
	    if {[info exists _private($w,prelief)] && \
		    $_private($w,prelief) eq [$w cget -relief]} {
		$w configure -relief $_private($w,relief)
	    }
	    unset -nocomplain _private($w,relief) _private($w,prelief)
	}

	# Clean up the after event from the auto-repeater
	after cancel $_private(afterId)

	if {$_private(window) eq $w && [$w cget -state] != "disabled"} {
	    # Only invoke the command if it wasn't already invoked by the
	    # auto-repeater functionality
	    if { $_private(repeated) == 0 } {
		uplevel #0 [list $w invoke]
	    }
	}
    }
}

