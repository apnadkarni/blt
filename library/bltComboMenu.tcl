# -*- mode: tcl; tcl-indent-level: 4; indent-tabs-mode: nil -*- 
#
# bltComboMenu.tcl
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
    namespace eval ComboMenu {
	variable _private
	array set _private {
	    afterId         -1
	    popOnRelease    0
	    trace           0
	    ignoreRelease   0
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

bind BltComboMenu <Enter> { 
    blt::ComboMenu::trace "blt::ComboMenu %# <Enter> %W"
    after cancel $blt::ComboMenu::_private(afterId)
    focus %W
}

bind BltComboMenu <Leave> { 
    blt::ComboMenu::trace "blt::ComboMenu %# %W <Leave> %s"
    if { %s == 0 } {
	#%W deactivate
    }
}

bind BltComboMenu <Motion> { 
    #blt::ComboMenu::trace "blt::ComboMenu Motion %\# %X,%Y"
    blt::ComboMenu::MotionEvent %W %X %Y
}

bind BltComboMenu <ButtonPress-1> { 
    blt::ComboMenu::trace "blt::ComboMenu %# <ButtonPress-1>  %W"
    blt::ComboMenu::ButtonPressEvent %W %X %Y
}

bind BltComboMenu <ButtonRelease-1> { 
    blt::ComboMenu::trace "blt::ComboMenu %# <ButtonRelease-1> %W %X,%Y"
    blt::ComboMenu::ButtonReleaseEvent %W %X %Y
}

bind BltComboMenu <B1-Motion> { 
    blt::ComboMenu::MotionEvent %W %X %Y
}

bind BltComboMenu <B1-Enter> {
    after cancel $blt::ComboMenu::_private(afterId)
    set blt::ComboMenu::_private(afterId) -1
}

bind BltComboMenu <B1-Leave> {
    blt::ComboMenu::trace "ComboMenu <B1-Leave>"
    blt::ComboMenu::AutoScroll %W %x %y
}

bind BltComboMenu <Unmap> {
    after cancel $blt::ComboMenu::_private(afterId)
    set blt::ComboMenu::_private(afterId) -1
}


if 0 {
bind BltComboMenu <ButtonPress-2> { 
    blt::ComboMenu::trace "blt::ComboMenu %# <ButtonPress-2> %W"
    %W configure -cursor diamond_cross
    update
    %W scan mark %x %y
}

bind BltComboMenu <B2-Motion> { 
    %W scan dragto %x %y
}

bind BltComboMenu <ButtonRelease-2> { 
    blt::ComboMenu::trace "blt::ComboMenu %W <ButtonRelease-2>"
    %W configure -cursor arrow
}
}

bind BltComboMenu <KeyPress-space> {
    blt::ComboMenu::trace "blt::ComboMenu Keypress-space %W"
    blt::ComboMenu::SelectItem
}

bind BltComboMenu <KeyRelease> {
    if { [string compare %A {}] == 0 } {
	continue
    }
    set index [%W find "%A" -underline]
    if { $index >= 0 } {
	%W activate $index
	%W see $index
    }
}

# KeyPress-Return -- 
#
#   If the menu item selected is a cascade menu, then post the cascade.
#   Otherwise tell the combobutton or comboentry that we've selected
#   something by simulating a button release.  This will unpost all the
#   posted menus. Set the root coordinates of the event to be offscreen so
#   that we don't inadvertantly lie over the arrow of the button.
#
bind BltComboMenu <KeyPress-Return> {
    blt::ComboMenu::SelectItem
}

bind BltComboMenu <Escape> {
    blt::ComboMenu::trace "blt::ComboMenu Keypress-escape %W"
    blt::ComboMenu::Cancel
}

bind BltComboMenu <Left> {
    blt::ComboMenu::LastMenu
}

bind BltComboMenu <Right> {
    blt::ComboMenu::NextMenu
}

bind BltComboMenu <KeyPress-Up> {
    blt::ComboMenu::LastItem
}

bind BltComboMenu <KeyPress-Down> {
    blt::ComboMenu::NextItem
}

bind BltComboMenu <KeyPress-Home> {
    %W activate first
    %W see active
}

bind BltComboMenu <KeyPress-End> {
    %W activate end
    %W see active
}

bind BltComboMenu <KeyPress-Prior> {
    %W yview scroll -1 page
    %W activate view.top
    %W see active
}

bind BltComboMenu <KeyPress-Next> {
    %W yview scroll 1 page
    %W activate view.bottom
    %W see active
}

if { [tk windowingsystem] == "x11" } {
    bind BltComboMenu <4> {
	%W yview scroll -5 units
    }
    bind BltComboMenu <5> {
	%W yview scroll 5 units
    }
} else {
    bind BltComboMenu <MouseWheel> {
	%W yview scroll [expr {- (%D / 120) * 4}] units
    }
}

proc ::blt::ComboMenu::AutoScroll {w x y} {
    variable _private

    trace "AutoScroll $w $y"
    if { ![winfo ismapped $w] } {
	set _private(afterId) -1
	return
    }
    set i -1
    set scroll 0
    if { ($x < 0) || ($x >= [winfo width $w]) } {
	return;				# Not within width of menu
    }
    if { ($y >= 0) && ($y < [winfo height $w]) } {
	return;				# Within height of menu
    }
    if { $y >= [winfo height $w] } {
	set i [$w next view.bottom]
	if { ($i+1) < [$w size] } {
	    set scroll 1;		# There's more to scroll
	}
    } elseif { $y < 0 } {
	set i [$w previous view.top]
	if { $i > 0 } {
	    set scroll 1;		# There's more to scroll 
	}
    } else {
	return
    }
    if { $i > 0 } {
	trace $i
	$w activate $i
	$w see $i
    }
    if { $scroll } {
	set cmd [list blt::ComboMenu::AutoScroll $w $x $y]
	set _private(afterId) [after 100 $cmd]
    }
}

proc blt::ComboMenu::ConfigureScrollbars { menu } {
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

proc ::blt::popup { menu x y } {
    blt::ComboMenu::Popup $menu $x $y
}

#
# Popup --
#
#	This procedure pops up a menu and sets things up for traversing
#	the menu and its submenus.
#
#
#	Arguments:
#	menu		Name of the menu to be displayed.
#	x, y		Root window coordinates at which to display the menu.
#
proc ::blt::ComboMenu::Popup { menu x y } {
    variable _private

    # Unless there's mouse motion, ignore the button release event.
    set _private(ignoreRelease) 1
    blt::ComboMenu::trace "blt::ComboMenu::Popup $menu $x $y"
    if { [blt::grab top] == $menu } {
	blt::ComboMenu::trace "blt::ComboMenu::Popup unposting $menu" 
	$menu unpost
	set parent [winfo parent $menu]
	event generate $parent <ButtonPress-1>
    } else {
	blt::ComboMenu::trace "blt::ComboMenu::Popup posting $menu"
	$menu post -popup [list $x $y]
	if { [winfo viewable $menu] } {
	    trace "popup: setting global grab on $menu"
	    blt::grab push $menu -global
	    focus $menu
	}
    }
}

proc ::blt::ComboMenu::FindCascades { menu } {
    # Follow the cascade menus to the bottom
    set m $menu
    set list {}
    while {1} {
	set m [$m postcascade]
	if { $m == "" } {
	    break
	}
	set list [linsert $list 0 $m]
    }
    return $list
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
proc ::blt::ComboMenu::ButtonPressEvent { menu x y } {
    variable _private

    # Examine each of the cascaded menus.
    foreach m [FindCascades $menu] {
	set item [$m index @$x,$y]
	if { $item != -1 } {
	    return;			# Found it.
	}
    }
    # Next handle top most menu.
    set item [$menu index @$x,$y]
    if { $item != -1 } {
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
proc ::blt::ComboMenu::ButtonReleaseEvent { menu x y } {
    variable _private
					
    # If the mouse hasn't moved, then ignore the button release event.
    set bool $_private(ignoreRelease)
    set _private(ignoreRelease) 0
    if { $bool } {
	return
    }
    # Examine each of the cascaded menus first.
    foreach m [FindCascades $menu] {
	set item [$m index @$x,$y]
	if { $item == -1 } {
	    continue
	}
	if { [$m type $item] == "cascade" } {
	    $m activate $item
	    return
	} 
	$m unpost
	# Pop the grab before invoking the menu item command.
	blt::grab pop $menu
	event generate $menu <<MenuSelect>>
	$m invoke $item
    }

    # Next handle the first menu.
    set item [$menu index @$x,$y]
    if { $item != -1 } {
	if { [$menu type $item] == "cascade" } {
	    set cascade [$menu item cget $item -menu]
	    if { $cascade != "" } {
		$menu postcascade $item
		focus $cascade
	    }
	    return
	} 
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
proc ::blt::ComboMenu::MotionEvent { menu x y } {
    variable _private

    # If there's any mouse motion, handle the button release event.
    set _private(ignoreRelease) 0

    # First try to find the item in any the cascades.
    foreach m [FindCascades $menu] {
	set item [$m index @$x,$y]
	if { $item == -1 } {
	    continue
	}
	if { [$m type $item] == "cascade" } {
	    set cascade [$m item cget $item -menu]
	    if { $cascade != "" } {
		$m activate $item
		$m postcascade $item
		focus $cascade
	    }
	} else {
	    $m postcascade none
	    $m activate $item
	}
	return
    }

    # Next handle the first menu.
    set item [$menu index @$x,$y]
    if { $item != -1 } {
	if { [$menu type $item] == "cascade" } {
	    set cascade [$menu item cget $item -menu]
	    if { $cascade != "" } {
		$menu activate $item
		$menu postcascade $item
		focus $cascade
	    }
	    return
	} 
	$menu activate $item
	$menu postcascade none
	return
    }

    # Handle activation of other combobuttons. 
    set w [winfo containing -display $menu $x $y]
    set parent [winfo parent $menu]
    if { $w != "" && $w != $parent && 
	 [winfo class $parent] == "BltComboButton" &&
	 [winfo class $w] == "BltComboButton" } {
	# Release the current combobutton, including removing the grab.
	$menu unpost
	# Reset the grab to the new button.
	blt::grab set -global $w 
	# Simulate pressing the new combobutton widget.
	event generate $w <ButtonPress-1>
    }
}


proc ::blt::ComboMenu::LastMenu {} {
    variable _private

    set menu [focus]
    if { $menu == "" || [winfo class $menu] != "BltComboMenu" } {
	return
    }
    set top [blt::grab top]
    if { $menu == $top } {
	return;				# We're already on the first menu. 
    }
    set last [winfo parent $menu] 
    if { $last == "" } {
	# No more cascades; the last menu is the first menu.
	return
    }
    if { [winfo class $last] != "BltComboMenu" } {
	return
    }
    $last postcascade none
    focus $last;			# Put focus on the last menu
}

proc ::blt::ComboMenu::NextMenu {} {
    variable _private

    set m [focus]
    if { [winfo class $m] != "BltComboMenu" } {
	return
    }
    set item [$m index active]
    if { [$m type $item] == "cascade" } {
	set cascade [$m  item cget $item -menu]
	if { $cascade != "" } {
	    $m postcascade $item
	    focus $cascade
	}
    }
}

proc ::blt::ComboMenu::LastItem {} {
    variable _private 

    set m [focus]
    if { $m == "" || [winfo class $m] != "BltComboMenu" } {
	return
    }
    $m activate previous
    $m see active
}

proc ::blt::ComboMenu::NextItem {} {
    variable _private 

    set m [focus]
    if { $m == "" || [winfo class $m] != "BltComboMenu" } {
	return
    }
    $m activate next
    $m see active
}

proc ::blt::ComboMenu::SelectItem {} {
    variable _private 

    set m [focus]
    if { $m == "" || [winfo class $m] != "BltComboMenu" } {
	return
    }
    set item [$m index active]
    if { $item == -1 } {
	return
    }
    if { [$m type $item] == "cascade" } {
	set cascade [$m item cget $item -menu]
	if { $cascade != "" } {
	    $m postcascade $item
	    focus $cascade
	}
	return
    } 
    set menu [blt::grab top]
    if { $menu == "" } {
	error "grab must be set on menu"
    }
    $m unpost
    # Pop the grab before invoking the menu item command.
    blt::grab pop $m
    event generate $m <<MenuSelect>>
    $m invoke $item
}

proc ::blt::ComboMenu::Cancel {} {
    variable _private 

    set m [blt::grab top]
    if { $m == "" || [winfo class $m] != "BltComboMenu" } {
	return
    }
    $m unpost 
    blt::grab pop $m
}
