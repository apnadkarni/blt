# -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- 
#
# bltComboEditor.tcl
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
    namespace eval ComboEditor {
	variable _private
	array set _private {
	    activeItem		{}
	    afterId		-1
	    b1			""
	    lastFocus		{}
	    mouseMoved		0
	    postingButton	{}
	    trace		0
	    lastX		-1
	    x			-1
	    y			-1
	}
	proc trace { mesg } {
	    variable _private
	    if { $_private(trace) } {
		puts stderr $mesg
	    }
	}
    }
}

bind BltComboEditor <Enter> {
    puts stderr "window=%W"
    # Do nothing
}

bind BltComboEditor <Leave> {
    #%W button activate off
}

# Standard Motif bindings:

bind BltComboEditor <Motion> {
    if { [%W identify %x %y] == "button" } {
	%W button activate
    }
}

bind BltComboEditor <ButtonPress-1> {
    focus %W
    set blt::ComboEditor::_private(x) %x
    set blt::ComboEditor::_private(y) %y
    %W icursor @%x,%y
    %W selection clear
    %W selection from insert
}

bind BltTextEntry <ButtonRelease-1> {
    blt::ComboEditor::trace "ComboEditor %W at %X,%Y <ButtonRelease-1>"
    after cancel $blt::ComboEditor::_private(afterId)
    if { [%W identify -root %X %Y]  == "button" } {
	blt::ComboEditor::trace "button invoke"
	%W button invoke
    }
}

bind BltComboEditor <B1-Motion> {
    if { $blt::ComboEditor::_private(b1) != "button" } {
	if { abs($blt::ComboEditor::_private(x) - %x) > 3 ||
	     abs($blt::ComboEditor::_private(y) - %y) > 3 } {
	    %W selection to @%x,%y
	    set blt::ComboEditor::_private(x) %x
	    set blt::ComboEditor::_private(y) %y
	}
    }
}

bind BltComboEditor <B1-Enter> {
    after cancel $blt::ComboEditor::_private(afterId)
    set blt::ComboEditor::_private(afterId) -1
}

bind BltComboEditor <B1-Leave> {
    blt::ComboEditor::trace "ComboEditor B1-Leave"
    if { $blt::ComboEditor::_private(b1) == "text" } {
	set blt::ComboEditor::_private(lastX) %x
	blt::ComboEditor::AutoScan %W
    }
}

bind BltComboEditor <Double-1> {
    blt::ComboEditor::trace "Double-1"
    if { [%W identify %x %y] == "button" } {
	::blt::ComboEditor::HandleButtonPress %W %x %y
    } else {
	%W icursor @%x,%y
	%W selection range word.start word.end
	%W icursor sel.last
    }
}

bind BltComboEditor <Triple-1> {
    blt::ComboEditor::trace "Triple-1"
    if { [%W identify %x %y] != "button" } {
	%W icursor @%x,%y
	%W selection range line.start line.end
	%W icursor sel.last
    }
}

bind BltComboEditor <Shift-1> {
    %W selection adjust @%x,%y
}

bind BltComboEditor <ButtonPress-2> {
    %W scan mark %x
}

bind BltComboEditor <ButtonRelease-2> {
    if { abs([%W scan mark] - %x) <= 3 } {
	catch { 
	    %W insert insert [selection get]
	    %W see insert
	}
    }
}

bind BltComboEditor <B2-Motion> {
    %W scan dragto %x
}

bind BltComboEditor <Control-1> {
    %W icursor @%x,%y
}


# Go to beginning of line (Ctrl+A).
bind BltComboEditor <Control-a> {
    %W icursor line.start
    %W see insert
}

# Back one character (Ctrl+B).
bind BltComboEditor <Control-KeyPress-b> {
    %W icursor previous
    %W see insert
}

# Forward one character (Ctrl+F).
bind BltComboEditor <Control-f> {
    %W icursor next
    %W see insert
}

# Paste
bind BltComboEditor <Control-v> {
    %W insert insert [selection get]
}


# Copy selection to clipboard (Ctrl+C).
bind BltComboEditor <Control-c> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
    }
}

# Delete next character (Ctrl+D).
bind BltComboEditor <Control-d> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	%W delete insert next
    }
}

# Goto to end of line (Ctrl+E)
bind BltComboEditor <Control-e> {
    %W icursor line.end
    %W see insert
}

# Delete last character (Ctrl+H).
bind BltComboEditor <Control-h> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	%W delete previous insert 
	%W see insert
    }
}

# Kill to end of line (Ctrl+K).
bind BltComboEditor <Control-KeyPress-k> {
    %W delete insert line.end
    %W see insert
}

# Down one line (Ctrl+N).  
bind BltComboEditor <Control-KeyPress-n> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor down
    %W see insert
}

# Up one line (Ctrl+P).
bind BltComboEditor <Control-KeyPress-p> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor up
    %W see insert
}

# Transpose current and last characters (Ctrl+T).
bind BltComboEditor <Control-t> {
    set index [%W index insert]
    if { $index != 0 && $index != [%W index end] } {
	set a [string index [%W get] [%W index previous]]
	set b [string index [%W get] [%W index insert]]
	%W delete previous next
	%W insert insert "$b$a"
    }
}

# Cut text and copy to clipboard (Ctrl+X).
bind BltComboEditor <Control-x> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
	%W delete sel.first sel.last
    }
}

# Redo last edit (Ctrl+Y).
bind BltComboEditor <Control-y> {
    %W redo
    %W see insert
}

# Undo last edit (Ctrl+Z).
bind BltComboEditor <Control-z> {
    %W undo
    %W see insert
}

bind BltComboEditor <KeyPress-Return> {
    %W insert insert "\n"
}

bind BltComboEditor <Escape> {
    %W unpost
}
bind BltComboEditor <KP_Enter> {
    %W insert insert "\n"
}

bind BltComboEditor <BackSpace> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete previous insert 
	%W see insert
    }
}

bind BltComboEditor <Delete> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete insert next
    }
}

bind BltComboEditor <KeyPress-Down> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor down
    %W see insert
}

bind BltComboEditor <KeyPress-Up> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor up
    %W see insert
}

bind BltComboEditor <KeyPress-Left> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor previous
    %W see insert
}

bind BltComboEditor <KeyPress-Right> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor next
    %W see insert
}

bind BltComboEditor <Home> {
    %W icursor 0
    %W see insert
}

bind BltComboEditor <End> {
    %W icursor end
    %W see insert
}


bind BltComboEditor <Shift-Left> {
    if {![%W selection present]} {
	%W selection range previous insert 
    } else {
	%W selection adjust previous
    }
    %W icursor previous
    %W see insert
}

bind BltComboEditor <Shift-Right> {
    if {![%W selection present]} {
	%W selection range insert next
    } else {
	%W selection adjust next
    }
    %W icursor next
    %W see insert
}

bind BltComboEditor <Shift-Control-Left> {
    set previous [string wordstart [%W get] [%W index previous]]
    if {![%W selection present]} {
	%W selection range $previous insert 
    } else {
	%W selection adjust $previous
    }
    %W icursor $previous
    %W see insert
}

bind BltComboEditor <Shift-Control-Right> {
    set next [string wordend [%W get] [%W index insert]]
    if {![%W selection present]} {
	%W selection range insert $next
    } else {
	%W selection adjust $next
    }
    %W icursor $next
    %W see insert
}

bind BltComboEditor <Shift-Home> {
    if {![%W selection present]} {
	%W selection range 0 insert
    } else {
	%W selection adjust 0
    }
    %W icursor 0
    %W see insert
}


bind BltComboEditor <Shift-End> {
    if {![%W selection present]} {
	%W selection range insert end
    } else {
	%W selection adjust end
    }
    %W icursor end
    %W see insert
}


bind BltComboEditor <Control-space> {
    %W selection from insert
}

bind BltComboEditor <Select> {
    %W selection from insert
}

bind BltComboEditor <Control-Shift-space> {
    %W selection adjust insert
}

bind BltComboEditor <Shift-Select> {
    %W selection adjust insert
}

bind BltComboEditor <Control-slash> {
    %W selection range 0 end
}

bind BltComboEditor <Control-backslash> {
    %W selection clear
}

bind BltComboEditor <<Cut>> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
	%W delete sel.first sel.last
    }
}

bind BltComboEditor <<Copy>> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [selection get]
    }
}

bind BltComboEditor <<Paste>> {
    %W insert insert [selection get]
    %W see insert
}

bind BltComboEditor <<Clear>> {
    %W delete sel.first sel.last
}


bind Entry <<PasteSelection>> {
    if { $tk_strictMotif || 
	 ![info exists blt::ComboEditor::_private(mouseMoved)] || 
	 !$blt::ComboEditor::_private(mouseMoved)} {
	tk::EntryPaste %W %x
    }
}


bind BltComboEditor <KeyPress> {
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


bind BltComboEditor <Alt-b> {
    %W icursor [string wordstart [%W get] [%W index previous]]
    %W see insert
}

bind BltComboEditor <Alt-d> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltComboEditor <Alt-f> {
    %W icursor [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltComboEditor <Alt-BackSpace> {
    %W delete [string wordstart [%W get] [%W index previous]] insert
    %W see insert
}

bind BltComboEditor <Alt-Delete> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

####
bind BltComboEditor <Meta-b> {
    %W icursor [string wordstart [%W get] [%W index previous]]
    %W see insert
}

bind BltComboEditor <Meta-d> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltComboEditor <Meta-f> {
    %W icursor [string wordend [%W get] [%W index insert]]
    %W see insert
}

bind BltComboEditor <Meta-BackSpace> {
    %W delete [string wordstart [%W get] [%W index previous]] insert
    %W see insert
}

bind BltComboEditor <Meta-Delete> {
    %W delete insert [string wordend [%W get] [%W index insert]]
    %W see insert
}


# Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for Escape, Return, and Tab.

bind BltComboEditor <Alt-KeyPress> {
    # Do nothing.
}
bind BltComboEditor <Meta-KeyPress> { 
    blt::ComboEditor::trace %K 
}
bind BltComboEditor <Control-KeyPress> {
    # Do nothing.
}
bind BltComboEditor <Tab> {
    # Do nothing.
}
switch -- [tk windowingsystem] {
    "classic" - "aqua"  {
	bind BltComboEditor <Command-KeyPress> {
	    # Do nothing.
	}
    }
}

proc ::blt::ComboEditor::AutoScan {w} {
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
    set _private(afterId) [after 50 [list blt::ComboEditor::AutoScan $w]]
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

proc ::blt::ComboEditor::PostMenu { w } {
    variable _private

    trace "proc PostMenu $w, state=[$w cget -state]"
    if { [$w cget -state] == "disabled" } {
	return
    }
    if { [$w cget -state] == "posted" } {
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
	UnpostMenu $cur
    }
    set _private(cursor) [$w cget -cursor]
    $w configure -cursor arrow
    
    set _private(postingButton) $w
    set _private(lastFocus) [focus]
    $menu activate none
    #blt::ComboEditor::GenerateMenuSelect $menu


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
	trace "setting global grab on $menu"
	bind $menu <Unmap> [list blt::ComboEditor::UnpostMenu $w]
	blt::grab push $menu -global 
    }
}

# ::blt::ComboEditor::UnpostMenu --
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

proc ::blt::ComboEditor::UnpostMenu { w } {
    variable _private

    trace "proc UnpostMenu $w"
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
    trace MENU=$menu
    # Release grab, if any, and restore the previous grab, if there
    # was one.
    blt::grab pop $menu
}

proc ::blt::ComboEditor::GenerateMenuSelect {menu} {
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

proc ::blt::ComboEditor::HandleButtonPress { w x y } {
    variable _private

    trace "blt::ComboEditor::HandleButtonPress $w state=[$w cget -state]"
    set _private(b1) [$w identify $x $y]
    if { [$w cget -state] == "posted" } {
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
    focus %W
    puts stderr "focus=[focus]"
    grab -local %W
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
    blt::ComboEditor::trace "ComboEditor %W at %X,%Y <ButtonRelease-1> state=[%W cget -state]"
    after cancel $blt::ComboEditor::_private(afterId)
    if { [%W identify -root %X %Y]  == "button" } {
	blt::ComboEditor::trace "button invoke"
	%W button invoke
    }
}

bind BltTextEntry <B1-Motion> {
    %W selection to [%W closest %x]
}

bind BltTextEntry <B1-Enter> {
    after cancel $blt::ComboEditor::_private(afterId)
    set blt::ComboEditor::_private(afterId) -1
}

bind BltTextEntry <B1-Leave> {
    blt::ComboEditor::trace "ComboEditor B1-Leave"
    if { $blt::ComboEditor::_private(b1) == "text" } {
	set blt::ComboEditor::_private(lastX) %x
	blt::ComboEditor::AutoScan %W
    }
}

bind BltTextEntry <Double-1> {
    blt::ComboEditor::trace "Double-1"
    %W icursor [%W closest %x]
    %W selection range \
	[string wordstart [%W get] [%W index previous]] \
	[string wordend   [%W get] [%W index insert]]
    %W icursor sel.last
    %W see insert
}

bind BltTextEntry <Triple-1> {
    blt::ComboEditor::trace "Triple-1"
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
    %W icursor left
    %W see insert
}

bind BltTextEntry <KeyPress-Right> {
    if { [%W selection present] } {
	%W icursor sel.last
	%W selection clear
    } 
    %W icursor right
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
	 ![info exists blt::ComboEditor::_private(mouseMoved)] || 
	 !$blt::ComboEditor::_private(mouseMoved)} {
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
    blt::ComboEditor::trace %K 
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
    itk::usual ComboEditor {
	keep -background -cursor 
    }
    itk::usual TextEntry {
	keep -background -cursor 
    }
}

proc blt::ComboEditor::ConfigureScrollbars { w } {
    set ys [$w cget -yscrollbar]
    if { $ys != "" } {
	if { [$w cget -yscrollcommand] == "" } {
	    $w configure -yscrollcommand [list $ys set]
	}
	if { [$ys cget -command] == "" } {
	    $ys configure -command [list $w yview] -orient vertical \
		-highlightthickness 0
	}
    }
    set xs [$w cget -xscrollbar]
    if { $xs != "" } {
	if { [$w cget -xscrollcommand] == "" } {
	    $w configure -xscrollcommand [list $xs set]
	}
	if { [$xs cget -command] == "" } {
	    $xs configure -command [list $w xview] -orient horizontal \
		-highlightthickness 0
	}
    }
}
