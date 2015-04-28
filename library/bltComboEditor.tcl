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
	    trace		0
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
    if { [%W identify %x %y] != "button" } {
	set blt::ComboEditor::_private(y) %x
	set blt::ComboEditor::_private(x) %y
	blt::ComboEditor::AutoScan %W
    }
}

bind BltComboEditor <Double-1> {
    blt::ComboEditor::trace "Double-1"
    if { [%W identify %x %y] == "button" } {
	::blt::ComboEditor::HandleButtonPress %W %x %y
    } else {
	%W icursor @%x,%y
	if { [%W get insert next] == " " } {
	    %W selection range space.start space.end
	} else {
	    %W selection range word.start word.end
	}	    
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

# Ctrl+A
#   Position insertion cursor at beginning of current line (the line
#   where the insertion cursor current resides).  
bind BltComboEditor <Control-a> {
    %W icursor line.start
    %W see insert
}

# Ctrl+A
#   Select all characters. Position insertion cursor at the end.
bind BltComboEditor <Control-a> {
    %W selection range 0 end
    %W icursor end
}

# Ctrl+B
#   Position the insertion cursor before the previous character.
bind BltComboEditor <Control-KeyPress-b> {
    %W icursor previous
    %W see insert
}

# Ctrl+C
#   Copy the selected characters to clipboard.
bind BltComboEditor <Control-c> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [$W selection get]
    }
}

# Ctrl+D
#   Deletes the next character (from the insertion cursor).
bind BltComboEditor <Control-d> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	%W delete insert next
    }
}

# Ctrl+E
#   Position the insertion cursor at the end of the current line (the line
#   where the insertion cursor current resides). 
bind BltComboEditor <Control-e> {
    %W icursor line.end
    %W see insert
}

# Ctrl+F
#   Position the insertion cursor before the next character.
bind BltComboEditor <Control-f> {
    %W icursor next
    %W see insert
}

# Ctrl+H
#   Delete the character previous to the insertion cursor.  
bind BltComboEditor <Control-h> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	%W delete previous insert 
	%W see insert
    }
}

# Ctrl+K
#   Deletes all characters to newline.  If there are no characters
#   before the end of the line, the newline is deleted.
bind BltComboEditor <Control-KeyPress-k> {
    if { [%W get insert line.end] == "" } {
        %W delete insert next
    } else {
	%W delete insert line.end
    }
    %W see insert
}

# Ctrl+N
#   Position the insertion cursor on the next line down.  If we're
#   already on the last line, nothing happens.  The cursor will be
#   the same number of characters over in the next line, unless the
#   line does not have that many characters.  Then the cursor will
#   be at the end of the next line.
bind BltComboEditor <Control-KeyPress-n> {
    %W icursor down
    %W see insert
}

# Ctrl+P
#   Position the insertion cursor on the previous line up.  If we're
#   already on the first line, nothing happens.  The cursor will be
#   the same number of characters over in the previous line, unless the
#   line does not have that many characters.  Then the cursor will
#   be at the end of the previous line.
bind BltComboEditor <Control-KeyPress-p> {
    %W icursor up
    %W see insert
}

# Ctrl+T
#   Transpose current and last characters.
bind BltComboEditor <Control-t> {
    set index [%W index insert]
    if { $index != 0 && $index != [%W index end] } {
	set a [%W get previous insert]
	set b [%W get insert next]
	%W delete previous next
	%W insert insert "$b$a"
    }
}

# Ctrl+V
#   Insert text from the clipboard at the current position.
bind BltComboEditor <Control-v> {
    %W insert insert [selection get]
}

# Ctrl+X
#   Copies the selected characters to the clipboard and then deletes them
#   from the text.
bind BltComboEditor <Control-x> {
    if { [%W selection present] } {
	clipboard clear -displayof %W
	clipboard append -displayof %W [%W selection get]
	%W delete sel.first sel.last
    }
}
# Ctrl+Y
#   Redo the last edit.
bind BltComboEditor <Control-y> {
    %W redo
    %W see insert
}
# Ctrl+Z
#   Undo the last edit.
bind BltComboEditor <Control-z> {
    %W undo
    %W see insert
}

# Return
#   Insert a newline into the text at the current location.
bind BltComboEditor <KeyPress-Return> {
    %W insert insert "\n"
}

# Escape
#   Cancel the editor. Return break.
bind BltComboEditor <Escape> {
    %W unpost
}

# Keypad Return
#   Insert a newline into the text at the current location.
bind BltComboEditor <KP_Enter> {
    %W insert insert "\n"
}

# BackSpace
#   Same at Ctrl+H
bind BltComboEditor <BackSpace> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete previous insert 
	%W see insert
    }
}

# Delete
#   Same as Ctrl+D. 
bind BltComboEditor <Delete> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete insert next
    }
}

# Down Arrow
#   Same as Ctrl+N. 
bind BltComboEditor <KeyPress-Down> {
    %W icursor down
    %W see insert
}

# Up Arrow
#   Same as Ctrl+P. 
bind BltComboEditor <KeyPress-Up> {
    %W icursor up
    %W see insert
}

# Left Arrow
#   Same as Ctrl+B. 
bind BltComboEditor <KeyPress-Left> {
    %W icursor previous
    %W see insert
}

# Right Arrow
#   Same as Ctrl+F.
bind BltComboEditor <KeyPress-Right> {
    %W icursor next
    %W see insert
}

# Home
#   Position the insertion cursor before the first character.
bind BltComboEditor <Home> {
    %W icursor 0
    %W see insert
}

# End
#   Position the insertion cursor after the last character.
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

# Alt+B
#   Position the insertion cursor before the current word.
bind BltComboEditor <Alt-b> {
    %W icursor space.start
    %W icursor word.start
    %W see insert
}

# Alt+D
#   Deletes character from the insertion cursor to the end of the current
#   word.
bind BltComboEditor <Alt-d> {
    %W delete insert word.end
    %W see insert
}

# Alt+F
#   Position the insertion cursor before the next word.
bind BltComboEditor <Alt-f> {
    %W icursor space.end
    %W icursor word.end
    %W see insert
}

# Alt+Backspace
#   Deletes the characters from the insertion cursor to the beginning
#   of the current word.
bind BltComboEditor <Alt-BackSpace> {
    %W delete word.start insert
    %W see insert
}

# Alt-Backspace
#   Deletes the characters from the insertion cursor to the end
#   of the current word.
bind BltComboEditor <Alt-Delete> {
    %W delete insert word.end
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

    set x $_private(x)
    set y $_private(y)
    if { ![winfo exists $w] } {
	return
    }
    if { $x >= [winfo width $w] } {
	$w xview scroll 2 units
    } elseif { $x < 0 } {
	$w xview scroll -2 units
    }
    if { $y >= [winfo height $w] } {
	$w yview scroll 2 units
    } elseif { $y < 0 } {
	$w yview scroll -2 units
    }
    set _private(afterId) [after 50 [list blt::ComboEditor::AutoScan $w]]
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

catch { 
    itk::usual ComboEditor {
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
