# -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- 
#
# bltComboFrame.tcl
#
# Copyright 2017 George A. Howlett. All rights reserved.  
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
    namespace eval ComboFrame {
	variable _private
	array set _private {
	    popOnRelease    0
	    trace           0
	}
	proc trace { mesg } {
	    variable _private 
	    if { $_private(trace) } {
		puts stderr $mesg
	    }
	}
    }
}

# -----------------------------------------------------------------------------

# Must set focus when mouse enters a menu, in order to allow mixed-mode
# processing using both the mouse and the keyboard.  Don't set the focus if
# the event comes from a grab release, though: such an event can happen
# after as part of unposting a cascaded chain of menus, after the focus has
# already been restored to wherever it was before menu selection started.

bind BltComboFrame <Enter> { 
    blt::ComboFrame::trace "blt::ComboFrame %# <Enter> %W"
}

bind BltComboFrame <Leave> { 
    blt::ComboFrame::trace "blt::ComboFrame %# %W <Leave> %s"
    if { %s == 0 } {
	#%W deactivate
    }
}

bind BltComboFrame <ButtonPress-1> { 
    blt::ComboFrame::trace "blt::ComboFrame %# <ButtonPress-1>  %W"
    blt::ComboFrame::ButtonPressEvent %W %X %Y
}

bind BltComboFrame <ButtonRelease-1> { 
    blt::ComboFrame::trace "blt::ComboFrame %# <ButtonRelease-1> %W %X,%Y"
    blt::ComboFrame::ButtonReleaseEvent %W %X %Y
}

#
# ButtonPressEvent --
#
#    Process a button press event on the given menu.  If the the button
#    press did not occur over a menu (cascade or not), this means the user
#    is canceling the posting of all menus.  Otherwise ignore it (we select
#    menu items on button release events). The location is in root
#    coordinates.
#
proc ::blt::ComboFrame::ButtonPressEvent { menu x y } {
    variable _private

    # Next handle top most menu.
    set rootx [expr [winfo rootx $menu] + $x]
    set rooty [expr [winfo rooty $menu] + $y]

    if { [winfo containing $rootx $rooty] == $menu } {
	return;				# Found it.
    }
    # This is called only when the grab is on.  This means that menu will
    # already be posted at this point.  On release, unpost the menu.
    set _private(popOnRelease) 1
}

#
# ButtonReleaseEvent --
#
#	Process a button release event on the given menu.  Check if the the
#	button release occurred over a cascade or the first menu.  Cascade
#	menus are stacked in reverse order of their posting.  This is so
#	that if menus overlap (a cascade menu is on top of a previous menu)
#	we will find the topmost cascade.
#
#	Once we find a menu, trim the cascade stack removing cascade menus
#	that are no longer available.
#
proc ::blt::ComboFrame::ButtonReleaseEvent { menu x y } {
    variable _private
					
    # Next handle the first menu.
    set rootx [expr [winfo rootx $menu] + $x]
    set rooty [expr [winfo rooty $menu] + $y]
   if { [winfo containing $rootx $rooty] == $menu } {
       $menu unpost
       # Pop the grab before invoking the menu item command.
       blt::grab pop $menu
       event generate $menu <<MenuSelect>>
       $menu invoke $item
       return
   }
    set popOnRelease 1
    if { !$_private(popOnRelease) } {
	set popOnRelease 0
    }
    if { $popOnRelease } {
	# This isn't the first time the menu was posted.  That happens when
	# the menubutton is pressed, the menu is posted, and the grab is
	# set on the menu.  This routine gets called on the button release.
	# Any further button releases should unpost the menu.  Just not on
	# the first release.
	$menu unpost 
	blt::grab pop $menu
    }
    set _private(popOnRelease) 0
}

proc ::blt::ComboFrame::Cancel {} {
    variable _private 

    set m [blt::grab top]
    if { $m == "" || [winfo class $m] != "BltComboFrame" } {
	return
    }
    $m unpost 
    blt::grab pop $m
}
