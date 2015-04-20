# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# macButton.tcl
#
# Bindings for the BLT button widget for MacIntosh platforms.
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


####################
# Mac implementation
####################

bind BltTkRadiobutton <Enter> {
    blt::Button::Enter %W
}
bind BltTkRadiobutton <1> {
    blt::Button::Down %W
}
bind BltTkRadiobutton <ButtonRelease-1> {
    blt::Button::Up %W
}
bind BltTkCheckbutton <Enter> {
    blt::Button::Enter %W
}
bind BltTkCheckbutton <1> {
    blt::Button::Down %W
}
bind BltTkCheckbutton <ButtonRelease-1> {
    blt::Button::Up %W
}
bind BltTkPushbutton <Enter> {
    blt::Button::Enter %W
}
bind BltTkPushbutton <1> {
    blt::Button::Down %W
}
bind BltTkPushbutton <ButtonRelease-1> {
    blt::Button::Up %W
}

#
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
    if {[$w cget -state] != "disabled"} {

	# If there's an -overrelief value, set the relief to that.

	if {$_private(buttonWindow) eq $w} {
	    $w configure -state active
	} elseif {[set over [$w cget -overrelief]] ne ""} {
	    set _private($w,relief)  [$w cget -relief]
	    set _private($w,prelief) $over
	    $w configure -relief $over
	}
    }
    set _private(window) $w
}

# Leave --
#
# The procedure below is invoked when the mouse pointer leaves a
# button widget.  It changes the state of the button back to
# inactive.  If we're leaving the button window with a mouse button
# pressed (_private(buttonWindow) == $w), restore the relief of the
# button too.
#
# Arguments:
# w -		The name of the widget.

proc ::blt::Button::Leave w {
    variable _private
    if {$w eq $_private(buttonWindow)} {
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

    if {[$w cget -state] != "disabled"} {
	set _private(buttonWindow) $w
	$w configure -state active

	# If this button has a repeatdelay set up, get it going with an after
	after cancel $_private(afterId)
	set _private(repeated) 0
	if { ![catch {$w cget -repeatdelay} delay] } {
	    if {$delay > 0} {
		set _private(afterId) [after $delay [list blt::Button::AutoInvoke $w]]
	    }
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
    if {$_private(buttonWindow) eq $w} {
	set _private(buttonWindow) ""
	$w configure -state normal

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

}
