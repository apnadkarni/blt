# -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- 
#
# bltComboEntry.tcl
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
    namespace eval ComboEntry {
	variable _private
	array set _private {
	    activeItem		""
	    afterId		-1
	    b1			""
	    lastFocus		""
	    lastX		-1
	    mouseMoved		0
	    postingButton	""
	    trace		0
	}
	proc trace { mesg } {
	    variable _private
	    if { $_private(trace) } {
		puts stderr $mesg
	    }
	}
    }
}

bind BltComboEntry <Enter> {
    # Do nothing
}

bind BltComboEntry <Leave> {
    %W activate off
}

# Standard Motif bindings:

bind BltComboEntry <Motion> {
    %W activate [%W identify %x %y] 
}

bind BltComboEntry <ButtonPress-1> {
    ::blt::ComboEntry::HandleButtonPress %W %x %y
}

bind BltComboEntry <ButtonRelease-1> {
    blt::ComboEntry::trace "ComboEntry %W at %X,%Y <ButtonRelease-1> state=[%W cget -state], grab=[blt::grab top]"
    after cancel $blt::ComboEntry::_private(afterId)
    blt::ComboEntry::trace "ComboEntry <ButtonRelease-1> identity=[%W identify %x %y]"
    switch -- [%W identify %x %y]  {
	"arrow" {
	    blt::ComboEntry::trace "invoke"
	    %W invoke
	}
	"button" {
	    blt::ComboEntry::trace "button invoke"
	    %W button invoke
	}
	default { 
	    blt::ComboEntry::trace "unpost"
	    blt::ComboEntry::UnpostMenu %W 
	}	
    }
}

bind BltComboEntry <B1-Motion> {
    if { $blt::ComboEntry::_private(b1) != "arrow" } {
	%W selection to [%W closest %x]
    }
}

bind BltComboEntry <B1-Enter> {
    after cancel $blt::ComboEntry::_private(afterId)
    set blt::ComboEntry::_private(afterId) -1
}

bind BltComboEntry <B1-Leave> {
    blt::ComboEntry::trace "ComboEntry B1-Leave"
    if { $blt::ComboEntry::_private(b1) == "text" } {
	set blt::ComboEntry::_private(lastX) %x
	blt::ComboEntry::AutoScan %W
    }
}

bind BltComboEntry <KeyPress-Down> {
    if { [%W cget -state] != "disabled" } {
	blt::grab push %W -global
	::blt::ComboEntry::PostMenu %W
    }
}

bind BltComboEntry <Double-1> {
    blt::ComboEntry::trace "Double-1"
    if { [%W identify %x %y] == "arrow" } {
	::blt::ComboEntry::HandleButtonPress %W %x %y
    } else {
	%W icursor [%W closest %x]
	%W selection range \
	    [string wordstart [%W get] [%W index previous]] \
	    [string wordend   [%W get] [%W index insert]]
	%W icursor sel.last
	%W see insert
    }
}

bind BltComboEntry <Triple-1> {
    blt::ComboEntry::trace "Triple-1"
    if { [%W identify %x %y] != "arrow" } {
	%W selection range 0 end
	%W icursor sel.last
    }
}

bind BltComboEntry <Shift-1> {
    %W selection adjust @%x
}

bind BltComboEntry <ButtonPress-2> {
    %W scan mark %x
}

bind BltComboEntry <ButtonRelease-2> {
    if { abs([%W scan mark] - %x) <= 3 } {
	catch { 
	    %W insert insert [selection get]
	    %W see insert
	}
    }
}

bind BltComboEntry <B2-Motion> {
    %W scan dragto %x
}

bind BltComboEntry <Control-1> {
    %W icursor @%x
}
bind BltComboEntry <KeyPress-Left> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor previous
    %W see insert
}

bind BltComboEntry <KeyPress-Right> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor next
    %W see insert
}

bind BltComboEntry <Shift-Left> {
    if {![%W selection present]} {
	%W selection range previous insert 
    } else {
	%W selection adjust previous
    }
    %W icursor previous
    %W see insert
}

bind BltComboEntry <Shift-Right> {
    if {![%W selection present]} {
	%W selection range insert next
    } else {
	%W selection adjust next
    }
    %W icursor next
    %W see insert
}

bind BltComboEntry <Shift-Control-Left> {
    set previous [string wordstart [%W get] [%W index previous]]
    if {![%W selection present]} {
	%W selection range $previous insert 
    } else {
	%W selection adjust $previous
    }
    %W icursor $previous
    %W see insert
}

bind BltComboEntry <Shift-Control-Right> {
    set next [string wordend [%W get] [%W index insert]]
    if {![%W selection present]} {
	%W selection range insert $next
    } else {
	%W selection adjust $next
    }
    %W icursor $next
    %W see insert
}

bind BltComboEntry <Home> {
    %W icursor 0
    %W see insert
}

bind BltComboEntry <Shift-Home> {
    if {![%W selection present]} {
	%W selection range 0 insert
    } else {
	%W selection adjust 0
    }
    %W icursor 0
    %W see insert
}

bind BltComboEntry <End> {
    %W icursor end
    %W see insert
}

bind BltComboEntry <Shift-End> {
    if {![%W selection present]} {
	%W selection range insert end
    } else {
	%W selection adjust end
    }
    %W icursor end
    %W see insert
}

bind BltComboEntry <Delete> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete insert next
    }
}

bind BltComboEntry <BackSpace> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete previous insert 
	%W see insert
    }
}

bind BltComboEntry <Control-space> {
    %W selection from insert
}

bind BltComboEntry <Select> {
    %W selection from insert
}

bind BltComboEntry <Control-Shift-space> {
    %W selection adjust insert
}

bind BltComboEntry <Shift-Select> {
    %W selection adjust insert
}

bind BltComboEntry <Control-slash> {
    %W selection range 0 end
}

bind BltComboEntry <Control-backslash> {
    %W selection clear
}

bind BltComboEntry <Control-z> {
    %W undo
    %W see insert
}

bind BltComboEntry <Control-Z> {
    %W redo
    %W see insert
}

bind BltComboEntry <Control-y> {
    %W redo
    %W see insert
}

bind BltComboEntry <<Cut>> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
	%W delete sel.first sel.last
    }
}

bind BltComboEntry <<Copy>> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
    }
}

bind BltComboEntry <<Paste>> {
    %W insert insert [selection get]
    %W see insert
}

bind BltComboEntry <<Clear>> {
    %W delete sel.first sel.last
}


bind Entry <<PasteSelection>> {
    if { $tk_strictMotif || 
	 ![info exists blt::ComboEntry::_private(mouseMoved)] || 
	 !$blt::ComboEntry::_private(mouseMoved)} {
	tk::EntryPaste %W %x
    }
}

# Paste
bind BltComboEntry <Control-v> {
    %W insert insert [selection get]
}

# Cut
bind BltComboEntry <Control-x> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
	%W delete sel.first sel.last
    }
}
# Copy
bind BltComboEntry <Control-c> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
    }
}

bind BltComboEntry <Return> {
    %W invoke 
}

bind BltComboEntry <KeyPress> {
    if { [string compare %A {}] == 0 } {
	continue
    }
    if { [%W selection present] } {
	%W delete sel.first sel.last
    }
    %W insert insert %A
    %W see insert
}

# Additional emacs-like bindings:

bind BltComboEntry <Control-a> {
    %W icursor 0
    %W see insert
}

bind BltComboEntry <Control-b> {
    %W icursor previous 
    %W see insert
}

bind BltComboEntry <Control-d> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	%W delete insert next
    }
}

bind BltComboEntry <Control-e> {
    %W icursor end
    %W see insert
}

bind BltComboEntry <Control-f> {
    %W icursor next
    %W see insert
}

bind BltComboEntry <Control-h> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	%W delete previous insert 
	%W see insert
    }
}

bind BltComboEntry <Control-k> {
    %W delete insert end
}

bind BltComboEntry <Control-t> {
    set index [%W index insert]
    if { $index != 0 && $index != [%W index end] } {
	set a [string index [%W get] [%W index previous]]
	set b [string index [%W get] [%W index insert]]
	%W delete previous next
	%W insert insert "$b$a"
    }
}

bind BltComboEntry <Alt-b> {
    %W icursor [string wordstart [%W get] [%W index previous]]
    %W see insert
}

bind BltComboEntry <Alt-d> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltComboEntry <Alt-f> {
    %W icursor [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltComboEntry <Alt-BackSpace> {
    %W delete [string wordstart [%W get] [%W index previous]] insert
    %W see insert
}

bind BltComboEntry <Alt-Delete> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

####
bind BltComboEntry <Meta-b> {
    %W icursor [string wordstart [%W get] [%W index previous]]
    %W see insert
}

bind BltComboEntry <Meta-d> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltComboEntry <Meta-f> {
    %W icursor [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltComboEntry <Meta-BackSpace> {
    %W delete [string wordstart [%W get] [%W index previous]] insert
    %W see insert
}

bind BltComboEntry <Meta-Delete> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}


# Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for Escape, Return, and Tab.

bind BltComboEntry <Alt-KeyPress> {
    # Do nothing.
}
bind BltComboEntry <Meta-KeyPress> { 
    blt::ComboEntry::trace %K 
}
bind BltComboEntry <Control-KeyPress> {
    # Do nothing.
}
bind BltComboEntry <Escape> {
    # Do nothing.
}
bind BltComboEntry <KP_Enter> {
    # Do nothing.
}
bind BltComboEntry <Tab> {
    # Do nothing.
}
switch -- [tk windowingsystem] {
    "classic" - "aqua"  {
	bind BltComboEntry <Command-KeyPress> {
	    # Do nothing.
	}
    }
}

proc ::blt::ComboEntry::AutoScan {w} {
    variable _private

    set x $_private(lastX)
    if { ![winfo exists $w] } {
	return
    }
    if { $x >= [winfo width $w] } {
	$w xview scroll 2 units
    } elseif { $x < 0 } {
	$w xview scroll -2 units
    }
    set _private(afterId) [after 50 [list blt::ComboEntry::AutoScan $w]]
}

# PostMenu --
#
#	Given a menubutton, this procedure does all the work of posting
#	its associated menu and unposting any other menu that is currently
#	posted.
#
# Arguments:
# w -			The name of the menubutton widget whose menu
#			is to be posted.
# x, y -		Root coordinates of cursor, used for positioning
#			option menus.  If not specified, then the center
#			of the menubutton is used for an option menu.

proc ::blt::ComboEntry::PostMenu { w } {
    variable _private

    trace "proc PostMenu $w, state=[$w cget -state]"
    if { [$w cget -state] == "disabled" } {
	return
    }
    if { [$w cget -state] == "posted" } {
        blt::ComboEntry::trace "from PostMenu"
	UnpostMenu $w
	return
    } 
    set menu [$w cget -menu]
    if { $menu == "" } {
	return
    }
    set cur $_private(postingButton)
    if { $cur != "" } {
	#
        blt::ComboEntry::trace "2 from PostMenu"
	UnpostMenu $cur
    }
    set _private(cursor) [$w cget -cursor]
    $w configure -cursor arrow
    
    set _private(postingButton) $w
    set _private(lastFocus) [focus]
    $menu activate none
    #blt::ComboEntry::GenerateMenuSelect $menu


    # If this looks like an option menubutton then post the menu so
    # that the current entry is on top of the mouse.  Otherwise post
    # the menu just below the menubutton, as for a pull-down.

    update idletasks
    if { [catch { $w post } msg] != 0 } {
	# Error posting menu (e.g. bogus -postcommand). Unpost it and
	# reflect the error.
	global errorInfo
	set savedInfo $errorInfo
	#
        blt::ComboEntry::trace "3 from PostMenu"
	UnpostMenu $w 
	error $msg $savedInfo
    }

    focus $menu
    set value [$w get]
    set index [$menu index $value]
    if { $index != -1 } {
	$menu see $index
	$menu activate $index
    }
    if { [winfo viewable $menu] } {
        # Automatically turn off grab on unposted menu
        bind $menu <Unmap> [list blt::ComboEntry::HandleUnmap %W $menu $w]
	trace "setting global grab on $menu"
	blt::grab push $menu -global 
    }
}

proc ::blt::ComboEntry::HandleUnmap { unmapped menu w } {
    puts stderr "menu=$menu unmapped=$unmapped"
    if { $menu != $unmapped } {
	return
    }
    UnpostMenu $w
}

# ::blt::ComboEntry::UnpostMenu --
#
# This procedure unposts a given menu, plus all of its ancestors up
# to (and including) a menubutton, if any.  It also restores various
# values to what they were before the menu was posted, and releases
# a grab if there's a menubutton involved.  Special notes:
# 1. It's important to unpost all menus before releasing the grab, so
#    that any Enter-Leave events (e.g. from menu back to main
#    application) have mode NotifyGrab.
# 2. Be sure to enclose various groups of commands in "catch" so that
#    the procedure will complete even if the menubutton or the menu
#    or the grab window has been deleted.
#
# Arguments:
# menu -		Name of a menu to unpost.  Ignored if there
#			is a posted menubutton.

proc ::blt::ComboEntry::UnpostMenu { w } {
    variable _private

    set menu [$w cget -menu]
    trace "proc UnpostMenu $w level=[info level] mapped=[winfo ismapped $menu]"
    catch { 
	# Restore focus right away (otherwise X will take focus away when the
	# menu is unmapped and under some window managers (e.g. olvwm) we'll
	# lose the focus completely).
	focus $_private(lastFocus) 
    }
    set _private(lastFocus) ""

    # Unpost menu(s) and restore some stuff that's dependent on what was
    # posted.

    $w unpost
    set _private(postingButton) {}
    if { [info exists _private(cursor)] } {
	$w configure -cursor $_private(cursor)
    }
    if { [$w cget -state] != "disabled" } {
	#$w configure -state normal
    }
    set menu [$w cget -menu]
    if { $menu == "" } {
	return
    }
    trace "MENU=$menu grab=[blt::grab current]"
    # Release grab, if any, and restore the previous grab, if there
    # was one.
    blt::grab pop $menu
}

proc ::blt::ComboEntry::GenerateMenuSelect {menu} {
    if 0 {
    variable _private
    if { $_private(activeComboMenu) != $menu ||
	 $_private(activeItem) != [$menu index active] } {
	set _private(activeComboMenu) $menu
	set _private(activeItem) [$menu index active]
	event generate $menu <<MenuSelect>>
    }
    }
}

proc ::blt::ComboEntry::HandleButtonPress { w x y } {
    variable _private

    trace "blt::ComboEntry::HandleButtonPress $w state=[$w cget -state]"
    set _private(b1) [$w identify $x $y]
    if { [$w cget -state] == "posted" } {
        blt::ComboEntry::trace "from HandleButtonPress"
	UnpostMenu $w
    } elseif { $_private(b1) == "arrow" } {
	PostMenu $w
    } else {
	trace "else: priv(v1)=$_private(b1) state=[$w cget -state]"
	focus $w
	$w icursor [$w closest $x]
	$w selection clear
	$w selection from insert
    }
}

########################################################################
bind BltTextEntry <Enter> {
    # Do nothing
}

bind BltTextEntry <Leave> {
    %W activate off
}

# Standard Motif bindings:

bind BltTextEntry <Motion> {
    %W activate [%W identify %x %y] 
}

bind BltTextEntry <ButtonPress-1> {
    focus %W
    %W icursor [%W closest %x]
    %W selection clear
    %W selection from insert
}

bind BltTextEntry <ButtonRelease-1> {
    blt::ComboEntry::trace "ComboEntry %W at %X,%Y <ButtonRelease-1> state=[%W cget -state]"
    after cancel $blt::ComboEntry::_private(afterId)
    if { [%W identify -root %X %Y]  == "button" } {
	blt::ComboEntry::trace "button invoke"
	%W button invoke
    }
}

bind BltTextEntry <B1-Motion> {
    %W selection to [%W closest %x]
}

bind BltTextEntry <B1-Enter> {
    after cancel $blt::ComboEntry::_private(afterId)
    set blt::ComboEntry::_private(afterId) -1
}

bind BltTextEntry <B1-Leave> {
    blt::ComboEntry::trace "ComboEntry B1-Leave"
    if { $blt::ComboEntry::_private(b1) == "text" } {
	set blt::ComboEntry::_private(lastX) %x
	blt::ComboEntry::AutoScan %W
    }
}

bind BltTextEntry <Double-1> {
    blt::ComboEntry::trace "Double-1"
    %W icursor [%W closest %x]
    %W selection range \
	[string wordstart [%W get] [%W index previous]] \
	[string wordend   [%W get] [%W index insert]]
    %W icursor sel.last
    %W see insert
}

bind BltTextEntry <Triple-1> {
    blt::ComboEntry::trace "Triple-1"
    %W selection range 0 end
    %W icursor sel.last
}

bind BltTextEntry <Shift-1> {
    %W selection adjust @%x
}

bind BltTextEntry <ButtonPress-2> {
    %W scan mark %x
}

bind BltTextEntry <ButtonRelease-2> {
    if { abs([%W scan mark] - %x) <= 3 } {
	catch { 
	    %W insert insert [selection get]
	    %W see insert
	}
    }
}

bind BltTextEntry <B2-Motion> {
    %W scan dragto %x
}

bind BltTextEntry <Control-1> {
    %W icursor @%x
}
bind BltTextEntry <KeyPress-Left> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor previous
    %W see insert
}

bind BltTextEntry <KeyPress-Right> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor next
    %W see insert
}

bind BltTextEntry <Shift-Left> {
    if {![%W selection present]} {
	%W selection range previous insert 
    } else {
	%W selection adjust previous
    }
    %W icursor previous
    %W see insert
}

bind BltTextEntry <Shift-Right> {
    if {![%W selection present]} {
	%W selection range insert next
    } else {
	%W selection adjust next
    }
    %W icursor next
    %W see insert
}

bind BltTextEntry <Shift-Control-Left> {
    set previous [string wordstart [%W get] [%W index previous]]
    if {![%W selection present]} {
	%W selection range $previous insert 
    } else {
	%W selection adjust $previous
    }
    %W icursor $previous
    %W see insert
}

bind BltTextEntry <Shift-Control-Right> {
    set next [string wordend [%W get] [%W index insert]]
    if {![%W selection present]} {
	%W selection range insert $next
    } else {
	%W selection adjust $next
    }
    %W icursor $next
    %W see insert
}

bind BltTextEntry <Home> {
    %W icursor 0
    %W see insert
}

bind BltTextEntry <Shift-Home> {
    if {![%W selection present]} {
	%W selection range 0 insert
    } else {
	%W selection adjust 0
    }
    %W icursor 0
    %W see insert
}

bind BltTextEntry <End> {
    %W icursor end
    %W see insert
}

bind BltTextEntry <Shift-End> {
    if {![%W selection present]} {
	%W selection range insert end
    } else {
	%W selection adjust end
    }
    %W icursor end
    %W see insert
}

bind BltTextEntry <Delete> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete insert next
    }
}

bind BltTextEntry <BackSpace> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete previous insert 
	%W see insert
    }
}

bind BltTextEntry <Control-space> {
    %W selection from insert
}

bind BltTextEntry <Select> {
    %W selection from insert
}

bind BltTextEntry <Control-Shift-space> {
    %W selection adjust insert
}

bind BltTextEntry <Shift-Select> {
    %W selection adjust insert
}

bind BltTextEntry <Control-slash> {
    %W selection range 0 end
}

bind BltTextEntry <Control-backslash> {
    %W selection clear
}

bind BltTextEntry <Control-z> {
    %W undo
    %W see insert
}

bind BltTextEntry <Control-Z> {
    %W redo
    %W see insert
}

bind BltTextEntry <Control-y> {
    %W redo
    %W see insert
}

bind BltTextEntry <<Cut>> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
	%W delete sel.first sel.last
    }
}

bind BltTextEntry <<Copy>> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
    }
}

bind BltTextEntry <<Paste>> {
    %W insert insert [selection get]
    %W see insert
}

bind BltTextEntry <<Clear>> {
    %W delete sel.first sel.last
}

bind Entry <<PasteSelection>> {
    if { $tk_strictMotif || 
	 ![info exists blt::ComboEntry::_private(mouseMoved)] || 
	 !$blt::ComboEntry::_private(mouseMoved)} {
	tk::EntryPaste %W %x
    }
}

# Paste
bind BltTextEntry <Control-v> {
    %W insert insert [selection get]
}

# Cut
bind BltTextEntry <Control-x> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
	%W delete sel.first sel.last
    }
}
# Copy
bind BltTextEntry <Control-c> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
    }
}

bind BltTextEntry <Return> {
    %W invoke 
}

bind BltTextEntry <KeyPress> {
    if { [string compare %A {}] == 0 } {
	continue
    }
    if { [%W selection present] } {
	%W delete sel.first sel.last
    }
    %W insert insert %A
    %W see insert
}

# Additional emacs-like bindings:

bind BltTextEntry <Control-a> {
    %W icursor 0
    %W see insert
}

bind BltTextEntry <Control-b> {
    %W icursor previous 
    %W see insert
}

bind BltTextEntry <Control-d> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	%W delete insert next
    }
}

bind BltTextEntry <Control-e> {
    %W icursor end
    %W see insert
}

bind BltTextEntry <Control-f> {
    %W icursor next
    %W see insert
}

bind BltTextEntry <Control-h> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	%W delete previous insert 
	%W see insert
    }
}

bind BltTextEntry <Control-k> {
    %W delete insert end
}

bind BltTextEntry <Control-t> {
    set index [%W index insert]
    if { $index != 0 && $index != [%W index end] } {
	set a [string index [%W get] [%W index previous]]
	set b [string index [%W get] [%W index insert]]
	%W delete previous next
	%W insert insert "$b$a"
    }
}

bind BltTextEntry <Alt-b> {
    %W icursor [string wordstart [%W get] [%W index previous]]
    %W see insert
}

bind BltTextEntry <Alt-d> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltTextEntry <Alt-f> {
    %W icursor [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltTextEntry <Alt-BackSpace> {
    %W delete [string wordstart [%W get] [%W index previous]] insert
    %W see insert
}

bind BltTextEntry <Alt-Delete> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

####
bind BltTextEntry <Meta-b> {
    %W icursor [string wordstart [%W get] [%W index previous]]
    %W see insert
}

bind BltTextEntry <Meta-d> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltTextEntry <Meta-f> {
    %W icursor [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltTextEntry <Meta-BackSpace> {
    %W delete [string wordstart [%W get] [%W index previous]] insert
    %W see insert
}

bind BltTextEntry <Meta-Delete> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}


# Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for Escape, Return, and Tab.

bind BltTextEntry <Alt-KeyPress> {
    # Do nothing.
}
bind BltTextEntry <Meta-KeyPress> { 
    blt::ComboEntry::trace %K 
}
bind BltTextEntry <Control-KeyPress> {
    # Do nothing.
}
bind BltTextEntry <Escape> {
    # Do nothing.
}
bind BltTextEntry <KP_Enter> {
    # Do nothing.
}
bind BltTextEntry <Tab> {
    # Do nothing.
}
switch -- [tk windowingsystem] {
    "classic" - "aqua"  {
	bind BltTextEntry <Command-KeyPress> {
	    # Do nothing.
	}
    }
}

catch { 
    itk::usual ComboEntry {
	keep -background -cursor 
    }
    itk::usual TextEntry {
	keep -background -cursor 
    }
}

