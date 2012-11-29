namespace eval blt {
    namespace eval ComboMenu {
	variable _private
	array set _private {
	    afterId         -
	    posted          ""
	    trace           0
	    cascades       ""
	    ignoreRelease  0
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

# Must set focus when mouse enters a menu, in order to allow
# mixed-mode processing using both the mouse and the keyboard.
# Don't set the focus if the event comes from a grab release,
# though:  such an event can happen after as part of unposting
# a cascaded chain of menus, after the focus has already been
# restored to wherever it was before menu selection started.

bind BltComboMenu <Enter> { 
    blt::ComboMenu::trace "blt::ComboMenu %# <Enter> %W"
    focus %W
}

bind BltComboMenu <Leave> { 
    blt::ComboMenu::trace "blt::ComboMenu %# %W <Leave> %s"
    if { %s == 0 } {
	#%W activate none 
    }
}

bind BltComboMenu <Motion> { 
    #blt::ComboMenu::trace "blt::ComboMenu Motion %\# %X,%Y"
    blt::ComboMenu::MotionEvent %X %Y
}

bind BltComboMenu <ButtonPress> { 
    blt::ComboMenu::trace "blt::ComboMenu %# <ButtonPress-1>  %W"
    blt::ComboMenu::ButtonPressEvent %W %X %Y
}

bind BltComboMenu <ButtonRelease> { 
    blt::ComboMenu::trace "blt::ComboMenu %# ButtonRelease-1 %W %X,%Y"
    blt::ComboMenu::ButtonReleaseEvent %W %X %Y
}

bind BltComboMenu <B1-Motion> { 
    blt::ComboMenu::MotionEvent %X %Y
}


bind BltComboMenu <B1-Enter> {
    after cancel $blt::ComboMenu::_private(afterId)
    set blt::ComboMenu::_private(afterId) -1
}

bind BltComboMenu <B1-Leave> {
    blt::ComboMenu::trace "ComboMenu B1-Leave"
    blt::ComboMenu::AutoScroll %W %x %y
}

bind BltComboMenu <Unmap> {
    after cancel $blt::ComboMenu::_private(afterId)
    set blt::ComboMenu::_private(afterId) -1
}


if 0 {
bind BltComboMenu <ButtonPress-2> { 
    blt::ComboMenu::trace "blt::ComboMenu %# ButtonPress-2 %W"
    %W configure -cursor diamond_cross
    update
    %W scan mark %x %y
}

bind BltComboMenu <B2-Motion> { 
    %W scan dragto %x %y
}

bind BltComboMenu <ButtonRelease-2> { 
    blt::ComboMenu::trace "blt::ComboMenu %W ButtonRelease-2"
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
#	If the menu item selected is a cascade menu, then post the cascade.
#	Otherwise tell the combobutton or comboentry that we've selected 
#	something by simulating a button release.  This will unpost all the
#	posted menus. Set the root coordinates of the event to be offscreen 
#	so that we don't inadvertantly lie over the arrow of the button.
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
    if { $y >= [winfo height $w] } {
	set i [$w next view.bottom]
    } elseif { $y < 0 } {
	set i [$w previous view.top]
    }
    if { $i > 0 } {
	trace $i
	$w activate $i
	$w see $i
    }
    set cmd [list blt::ComboMenu::AutoScroll $w $x $y]
    set _private(afterId) [after 50 $cmd]
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
    blt::ComboMenu::trace "blt::ComboMenu::popup $menu $x $y"
    if { [blt::grab top] == $menu } {
	blt::ComboMenu::trace "blt::ComboMenu::popup unposting $menu" 
	$menu unpost
    } else {
	blt::ComboMenu::trace "blt::ComboMenu::popup posting $menu"
	$menu post popup $x $y 
	if { [winfo viewable $menu] } {
	    trace "popup: setting global grab on $menu"
	    blt::grab push $menu -global
	    focus $menu
	}
    }
}

proc ::blt::ComboMenu::ButtonPressEvent { menu x y } {
    variable _private

    # Handle top most menu first.
    set item [$menu index @$x,$y]
    if { $item != -1 } {
	return;				# Found it.
    }
    # Now examine each of the cascaded menus.
    foreach m $_private(cascades) {
	set item [$m index @$x,$y]
	if { $item != -1 } {
	    return;			# Found it.
	}
    }
    # The button press event did not occur inside of any menu.
    $menu unpost 
    set _private(cascades) ""
    event generate $menu <<MenuSelect>>
    blt::grab pop $menu
}

proc ::blt::ComboMenu::ButtonReleaseEvent { menu x y } {
    variable _private
					
    # If the mouse hasn't moved, then ignore the button release event.
    set bool $_private(ignoreRelease)
    set _private(ignoreRelease) 0
    if { $bool } {
	return
    }
    # Handle top most menu first.
    set item [$menu index @$x,$y]
    if { $item != -1 } {
	if { [$menu type $item] == "cascade" } {
	    set cascade [$menu item cget $item -menu]
	    if { $cascade != "" } {
		$menu postcascade $item
		set _private(cascades) $cascade
	    }
	    return
	} 
	$menu unpost
	set _private(cascades) ""
	# Pop the grab before invoking the menu item command.
	blt::grab pop $menu
	event generate $menu <<MenuSelect>>
	$menu invoke $item
	return
    }
    # Now examine each of the cascaded menus.
    set stack {}
    foreach m $_private(cascades) {
	set item [$m index @$x,$y]
	lappend stack $m
	if { $item == -1 } {
	    continue
	}
	if { [$m type $item] == "cascade" } {
	    set cascade [$m item cget $item -menu]
	    if { $cascade != "" } {
		lappend stack $cascade
	    }
	    set _private(cascades) $stack
	    $m activate $item
	    return
	} 
	$m unpost
	set _private(cascades) ""
	# Pop the grab before invoking the menu item command.
	event generate $menu <<MenuSelect>>
	blt::grab pop $menu
	$m invoke $item
	return
    }
}

proc ::blt::ComboMenu::MotionEvent { x y } {
    variable _private

    # If there's any mouse motion, handle the button release event.
    set _private(ignoreRelease) 0

    # Handle the topmost menu first.
    set menu [blt::grab top]
    if { $menu == "" } {
	error "grab must be set on menu"
    }
    set item [$menu index @$x,$y]
    if { $item != -1 } {
	if { [$menu type $item] == "cascade" } {
	    set cascade [$menu item cget $item -menu]
	    if { $cascade != "" } {
		$menu activate $item
		$menu postcascade $item
		set _private(cascades) $cascade
		focus $cascade
	    }
	    return
	} 
	$menu activate $item
	$menu postcascade none
	set _private(cascades) ""
	return
    }

    # Now examine each of the cascade menus. 
    set stack {}
    foreach m $_private(cascades) {
	set item [$m index @$x,$y]
	lappend stack $m
	if { $item == -1 } {
	    continue
	}
	if { [$m type $item] == "cascade" } {
	    set cascade [$m item cget $item -menu]
	    if { $cascade != "" } {
		$m activate $item
		$m postcascade $item
		lappend stack $cascade
		focus $cascade
	    }
	    set _private(cascades) $stack
	    return
	} 
	$m postcascade none
	$m activate $item
	set _private(cascades) $stack
	return
    }

    # Handle activation of other combobuttons. 
    set w [winfo containing -display $menu $x $y]
    set parent [winfo parent $menu]
    if { $w != "" && $w != $parent && 
	 [winfo class $parent] == "BltComboButton" &&
	 [winfo class $w] == "BltComboButton" } {
	# Release the current combobutton, including removing the grab.
	set _private(cascades) ""
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
	return;				# We're already on the topmost menu. 
    }
    set stack {}
    set last $top
    foreach m $_private(cascades) {
	if { $m == $menu } {
	    break
	}
	set last $m
	lappend stack $m
    }
    set _private(cascades) $stack
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
	    lappend _private(cascades) $cascade 
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
	    lappend _private(cascades) $cascade 
	}
	return
    } 
    set menu [blt::grab top]
    if { $menu == "" } {
	error "grab must be set on menu"
    }
    $m unpost
    set _private(cascades) ""
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
    event generate $m <<MenuSelect>>
    blt::grab pop $m
}
