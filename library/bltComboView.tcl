# -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- 
#
# bltComboView.tcl
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
    namespace eval ComboView {
	variable _private
	array set _private {
	    afterId         -
	    posted          ""
	    trace           0
	    cascades       ""
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

bind BltComboView <Enter> { 
    blt::ComboView::trace "blt::ComboView %# <Enter> %W"
    focus %W
}

bind BltComboView <Leave> { 
    blt::ComboView::trace "blt::ComboView %# %W <Leave> %s"
    if { %s == 0 } {
	#%W activate none 
    }
}

bind BltComboView <Motion> { 
    blt::ComboView::trace "blt::ComboView Motion %\# %X,%Y"
    blt::ComboView::MotionEvent %W %X %Y
}

bind BltComboView <ButtonPress> { 
    blt::ComboView::trace "blt::ComboView %# <ButtonPress-1>  %W"
    blt::ComboView::ButtonPressEvent %W %X %Y
}

bind BltComboView <ButtonRelease> { 
    blt::ComboView::trace "blt::ComboView %# ButtonRelease-1 %W %X,%Y"
    blt::ComboView::ButtonReleaseEvent %W %X %Y
}

bind BltComboView <B1-Motion> { 
    blt::ComboView::MotionEvent %W %X %Y
}


bind BltComboView <B1-Enter> {
    after cancel $blt::ComboView::_private(afterId)
    set blt::ComboView::_private(afterId) -1
}

bind BltComboView <B1-Leave> {
    blt::ComboView::trace "ComboView B1-Leave"
    blt::ComboView::AutoScroll %W %x %y
}

bind BltComboView <Unmap> {
    after cancel $blt::ComboView::_private(afterId)
    set blt::ComboView::_private(afterId) -1
}


if 0 {
bind BltComboView <ButtonPress-2> { 
    blt::ComboView::trace "blt::ComboView %# ButtonPress-2 %W"
    %W configure -cursor diamond_cross
    update
    %W scan mark %x %y
}

bind BltComboView <B2-Motion> { 
    %W scan dragto %x %y
}

bind BltComboView <ButtonRelease-2> { 
    blt::ComboView::trace "blt::ComboView %W ButtonRelease-2"
    %W configure -cursor arrow
}
}

bind BltComboView <KeyPress-space> {
    blt::ComboView::trace "blt::ComboView Keypress-space %W"
    blt::ComboView::SelectItem %W
}

bind BltComboView <KeyRelease> {
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
bind BltComboView <KeyPress-Return> {
    blt::ComboView::SelectItem %W
}

bind BltComboView <Escape> {
    blt::ComboView::trace "blt::ComboView Keypress-escape %W"
    blt::ComboView::Cancel
}

bind BltComboView <Left> {
    blt::ComboView::LastMenu
}

bind BltComboView <Right> {
    blt::ComboView::NextMenu
}

bind BltComboView <KeyPress-Up> {
    blt::ComboView::LastItem
}

bind BltComboView <KeyPress-Down> {
    blt::ComboView::NextItem
}

bind BltComboView <KeyPress-Home> {
    %W activate first
    %W see active
}

bind BltComboView <KeyPress-End> {
    %W activate end
    %W see active
}

bind BltComboView <KeyPress-Prior> {
    %W yview scroll -1 page
    %W activate view.top
    %W see active
}

bind BltComboView <KeyPress-Next> {
    %W yview scroll 1 page
    %W activate view.bottom
    %W see active
}

if { [tk windowingsystem] == "x11" } {
    bind BltComboView <4> {
	%W yview scroll -5 units
    }
    bind BltComboView <5> {
	%W yview scroll 5 units
    }
} else {
    bind BltComboView <MouseWheel> {
	%W yview scroll [expr {- (%D / 120) * 4}] units
    }
}

proc ::blt::ComboView::AutoScroll {w x y} {
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
    set cmd [list blt::ComboView::AutoScroll $w $x $y]
    set _private(afterId) [after 50 $cmd]
}

proc blt::ComboView::ConfigureScrollbars { view } {
    set ys [$view cget -yscrollbar]
    if { $ys != "" } {
	if { [$view cget -yscrollcommand] == "" } {
	    $view configure -yscrollcommand [list $ys set]
	}
	if { [$ys cget -command] == "" } {
	    $ys configure -command [list $view yview] -orient vertical \
		-highlightthickness 0
	}
    }
    set xs [$view cget -xscrollbar]
    if { $xs != "" } {
	if { [$view cget -xscrollcommand] == "" } {
	    $menu configure -xscrollcommand [list $xs set]
	}
	if { [$xs cget -command] == "" } {
	    $xs configure -command [list $view xview] -orient horizontal \
		-highlightthickness 0
	}
    }
}

proc ::blt::ComboView::ButtonPressEvent { view x y } {
    variable _private

    # Handle top most menu first.
    set item [$view index @$x,$y]
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
    $view unpost 
    set _private(cascades) ""
    event generate $view <<MenuSelect>>
    #blt::grab pop $view
}

proc ::blt::ComboView::ButtonReleaseEvent { view x y } {
    variable _private
					
    # Handle top most menu first.
    set item [$view index @$x,$y]
    if { $item != -1 } {
	if { [$view type $item] == "cascade" } {
	    set cascade [$view item cget $item -menu]
	    if { $cascade != "" } {
		blt::grab push $view -global
		$view postcascade $item
		set _private(cascades) $cascade
		update
		bind $cascade <Unmap> \
		    [list blt::ComboView::ReleaseGrab $view %W]
	    }
	    return
	} 
	$view unpost
	set _private(cascades) ""
	#blt::grab pop $view
	event generate $view <<MenuSelect>>
	$view invoke $item
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
	blt::grab pop $view
	event generate $view <<MenuSelect>>
	$m invoke $item
	return
    }
}

proc ::blt::ComboView::MotionEvent { altview x y } {
    variable _private

    set view [blt::grab current]
    if { $view == "" } {
	set view $altview
    } 
    if { ([winfo class $view] != "BltComboMenu" &&
	    [winfo class $view] != "BltComboView") } {
	return
    }
    # Handle the top most menu first.
    set item [$view index @$x,$y]
    if { $item != -1 } {
	if { [$view type $item] == "cascade" } {
	    set cascade [$view item cget $item -menu]
	    if { $cascade != "" } {
		blt::grab push $view -global
		focus $cascade
		bind $cascade <Unmap> \
		    [list blt::ComboView::ReleaseGrab $view %W]
		$view activate $item
		$view postcascade $item
		set _private(cascades) $cascade
	    }
	    return
	} 
	$view activate $item
	$view postcascade none
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
}


proc ::blt::ComboView::LastMenu {} {
    variable _private

    set menu [focus]
    if { $m == "" || ([winfo class $m] != "BltComboMenu" &&
		      [winfo class $m] != "BltComboView") } {
    }
    set top [blt::grab current]
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
    if {[winfo class $m] != "BltComboMenu" &&
	[winfo class $m] != "BltComboView" } {
	return
    }
    $last postcascade none
    focus $last;			# Put focus on the last menu
}

proc ::blt::ComboView::NextMenu {} {
    variable _private

    set m [focus]
    if { $m == "" || ([winfo class $m] != "BltComboMenu" &&
		      [winfo class $m] != "BltComboView") } {
	return
    }
    if { [$m type active] == "cascade" } {
	set cascade [$m item cget active -menu]
	if { $cascade != "" } {
	    $m postcascade active
	    focus $cascade
	    lappend _private(cascades) $cascade 
	}
    }
}

proc ::blt::ComboView::LastItem {} {
    variable _private 

    set m [focus]
    if { $m == "" || ([winfo class $m] != "BltComboMenu" &&
		      [winfo class $m] != "BltComboView") } {
	return
    }
    $m activate previous
    $m see active
}

proc ::blt::ComboView::NextItem {} {
    variable _private 

    set m [focus]
    if { $m == "" || ([winfo class $m] != "BltComboMenu" &&
		      [winfo class $m] != "BltComboView") } {
	return
    }
    $m activate next
    $m see active
}

proc ::blt::ComboView::SelectItem { view } {
    variable _private 

    set m [focus]
    if { $m == "" || ([winfo class $m] != "BltComboMenu" &&
		      [winfo class $m] != "BltComboView") } {
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
    $m unpost
    set _private(cascades) ""
    blt::grab pop $m
    event generate $m <<MenuSelect>>
    $m invoke $item
}

proc ::blt::ComboView::Cancel {} {
    variable _private 

    set m [blt::grab current]
    if { $m == "" || [winfo class $m] != "BltComboView" } {
	return
    }
    $m unpost 
    event generate $m <<MenuSelect>>
    blt::grab pop $m
}

proc ::blt::ComboView::ReleaseGrab { view menu } {
    variable _private 

    bind $menu <Unmap> {}
    blt::grab pop $view
}
