
namespace eval blt {
    namespace eval ComboButton {
	variable _private
	array set _private {
	    posted      {}
	    cursor      {}
	    focus       {}
	    trace	0
	}
	proc trace { mesg } {
	    variable _private 
	    if { $_private(trace) } {
		puts stderr $mesg
	    }
	}
    }
}

#
# When a menu is posted the pointer grab is transferred to the menu from
# the combobutton (the implicit ButtonPress grab).  This allows us to 
# implement  both click-drag-release and click-click menu selection methods.
#
# click-drag-release:
# The user clicks, drags the mouse over a menu item, and then releases 
# the button. The <Motion> and <ButtonRelease> events will be intepreted 
# by the menu.  The <ButtonRelease> event is propagated back to the 
# combobutton. It's the responsibility of the combobutton to unpost the 
# menu, release the grab, and reset the focus.
#
# click-click:
# The user clicks and releases the the mouse button over the combo button,
# This <ButtonPress> event posts the menu.  The menu gets the <ButtonRelease>
# event and propagates it back to the combobutton. We test if the 
# <ButtonRelease> occurred over the combobutton.  This means to leave both
# the grab and posted menu intact.
#

bind BltComboButton <Enter> {
    %W activate yes
}

bind BltComboButton <Leave> {
    %W activate no
}

# Standard Motif bindings:

bind BltComboButton <ButtonPress-1> {
    blt::ComboButton::trace "ComboButton <ButtonPress-1> %# sendevent=%E state=[%W cget -state]"
    if { [%W cget -state] == "posted" } {
	blt::ComboButton::Unpost %W
    } else {
	blt::ComboButton::Post %W
    }
}

bind BltComboButton <ButtonRelease-1> {
    blt::ComboButton::trace \
	"ComboButton <ButtonRelease-1> %# sendevent=%E state=[%W cget -state]"
    if { [winfo containing -display  %W %X %Y] == "%W" } {
	blt::ComboButton::trace "invoke"
	%W invoke
    } else { 
	blt::ComboButton::Unpost %W
    }	
}

bind BltComboButton <Unmap> {
	blt::ComboButton::Unpost %W
}

bind BltComboButton <KeyPress-Down> {
    blt::ComboButton::Post %W
}

# Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for Escape, Return, and Tab.

bind BltComboButton <Alt-KeyPress> {
    # nothing
}
bind BltComboButton <Meta-KeyPress> { 
    puts %K 
}
bind BltComboButton <Control-KeyPress> {
    # nothing
}
bind BltComboButton <Escape> {
    # nothing
}

bind BltComboButton <Tab> {
    # nothing
}
if {[string equal [tk windowingsystem] "classic"] || 
    [string equal [tk windowingsystem] "aqua"]} {
    bind BltComboButton <Command-KeyPress> {
	# nothing
    }
}

#
# Post --
#
#	Posts the menu associated with a given menubutton. This performs
#	various housekeeping tasks such as changing the grab and focus,
#	and unposting any other menu that is currently posted.
#
proc ::blt::ComboButton::Post { w } {
    variable _private

    trace "proc ComboButton::Post $w, state=[$w cget -state]"
    if { [$w cget -state] == "disabled" } {
	error "menu button $w is disabled"
    }
    set menu [$w cget -menu]
    if { $menu == "" } {
	return
    }
    set last $_private(posted)
    if { $last != "" } {
	Unpost $last
    }
    # Change the cursor, saving the old cursor
    set _private(cursor) [$w cget -cursor]
    $w configure -cursor arrow
    set _private(posted) $w
    # Remember the current focus setting so that we can restore it later.
    set _private(focus) [focus]
    $menu activate none
    update idletasks
    if { [catch { $w post } msg] != 0 } {
	# There was an error posting the menu (for example the -postcommand
	# failed). Unpost the menu and propagate the error.
	global errorInfo
	set savedInfo $errorInfo
	Unpost $w
	error $msg $savedInfo
    }
    # Set the focus and grab to the newly posted menu.
    focus $menu
    if { [winfo viewable $menu] } {
	trace "setting global grab on $menu"
	bind $menu <Unmap> [list blt::ComboButton::Unpost $w]
	if { [blt::grab top] == $w } {
	    blt::grab set -global $menu
	} else {
	    blt::grab push $menu -global
	}
    }
}

#
# Unpost --
#
#	Performs housekeeping tasks to unpost the menu (and all is descendants).
#	It releases the grab on the button, resets the focus, and reconfigures
#	the combobutton state back to normal.
#
proc ::blt::ComboButton::Unpost { w } {
    trace "proc ComboButton::Unpost $w"
    variable _private
    catch { focus $_private(focus) }
    set _private(focus) ""
    $w unpost
    set _private(posted) {}
    if { [info exists _private(cursor)] } {
	$w configure -cursor $_private(cursor)
    }
    if { [$w cget -state] != "disabled" } {
	$w configure -state normal
    }
    set menu [$w cget -menu]
    blt::grab pop ; #$menu
    bind $menu <Unmap> {}
}

