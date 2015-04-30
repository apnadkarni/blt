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
    # Do nothing
}

bind BltComboEditor <Leave> {
    #%W button activate off
}

# Standard Motif bindings:

# If we're over the editor, clear the selection and set the anchor of the
# new one.
bind BltComboEditor <ButtonPress-1> {
    blt::ComboEditor::trace "ButtonPress-1"
    if { [%W index @%x,%y]  != -1 } {
	focus %W
	set blt::ComboEditor::_private(x) %x
	set blt::ComboEditor::_private(y) %y
	%W icursor @%x,%y
	%W selection clear
	%W selection from insert
    }
}

# Turn off the auto-scan.  If we're not over the editor, cancel the session.
bind BltComboEditor <ButtonRelease> {
    blt::ComboEditor::trace "ButtonRelease-1"
    after cancel $blt::ComboEditor::_private(afterId)
    if { [%W index @%x,%y]  == -1 } {
	%W unpost
    } 
}

# If the pointer moved, update the selection.
bind BltComboEditor <B1-Motion> {
    if { abs($blt::ComboEditor::_private(x) - %x) > 3 ||
	 abs($blt::ComboEditor::_private(y) - %y) > 3 } {
	%W selection to @%x,%y
	%W icursor sel.last
	set blt::ComboEditor::_private(x) %x
	set blt::ComboEditor::_private(y) %y
    }
}

# Turn off the auto-scan.
bind BltComboEditor <B1-Enter> {
    after cancel $blt::ComboEditor::_private(afterId)
    set blt::ComboEditor::_private(afterId) -1
}

# Turn on the auto-scan.
bind BltComboEditor <B1-Leave> {
    blt::ComboEditor::trace "ComboEditor B1-Leave"
    set blt::ComboEditor::_private(y) %x
    set blt::ComboEditor::_private(x) %y
    blt::ComboEditor::AutoScan %W
}

# Select the whitespace or word at the x-y coordinate position.
bind BltComboEditor <Double-ButtonPress-1> {
    blt::ComboEditor::trace "Double-1"
    %W icursor @%x,%y
    if { [%W get insert next] == " " } {
	%W selection range space.start space.end
    } else {
	%W selection range word.start word.end
    }	    
    %W icursor sel.last
}

# Select all the text on the  line including the insertion cursor.
bind BltComboEditor <Triple-ButtonPress-1> {
    blt::ComboEditor::trace "Triple-1"
    %W icursor @%x,%y
    %W selection range line.start line.end
    %W icursor sel.last
}

# Adjust the selection but leave the insertion cursor alone.
bind BltComboEditor <Shift-ButtonPress-1> {
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

# Move the insertion cursor, but leave the selection alone.
bind BltComboEditor <Control-ButtonPress-1> {
    %W icursor @%x,%y
}

# Ctrl+A
#   Position insertion cursor at beginning of current line (the line
#   where the insertion cursor current resides).  
bind BltComboEditor <Control-a> {
    if {[%W selection present]} {
	%W selection clear
    }
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
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor line.end
    %W see insert
}

# Ctrl+F
#   Position the insertion cursor before the next character.
bind BltComboEditor <Control-f> {
    if {[%W selection present]} {
	%W selection clear
    }
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

#   Position the insertion cursor on the next line down.  If we're already
#   on the last line, nothing happens.  The cursor will be the same number
#   of characters from the start of the next line, unless the line does not
#   have that many characters.  Then the cursor will be at the end of the
#   next line.
bind BltComboEditor <Control-KeyPress-n> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor down
    %W see insert
}

# Ctrl+P
#   Position the insertion cursor on the previous line up.  If we're
#   already on the first line, nothing happens.  The cursor will be the
#   same number of characters from the start in the previous line, unless
#   the line does not have that many characters.  Then the cursor will be
#   at the end of the previous line.
bind BltComboEditor <Control-KeyPress-p> {
    if {[%W selection present]} {
	%W selection clear
    }
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
    if {[%W selection present]} {
	%W selection clear
    }
    %W redo
    %W see insert
}
# Ctrl+Z
#   Undo the last edit.
bind BltComboEditor <Control-z> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W undo
    %W see insert
}

# Return
#   Insert a newline into the text at the current location.
bind BltComboEditor <KeyPress-Return> {
    %W unpost
    %W invoke
}

# Escape
#   Cancel the editor. Return break.
bind BltComboEditor <Escape> {
    %W unpost
}

# Ctrl+Return
#   Insert a newline into the text at the current location.
bind BltComboEditor <Control-KeyPress-Return> {
    %W insert insert "\n"
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
    }
    %W see insert
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
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor down
    %W see insert
}

# Up Arrow
#   Same as Ctrl+P. 
bind BltComboEditor <KeyPress-Up> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor up
    %W see insert
}

# Left Arrow
#   Same as Ctrl+B. 
bind BltComboEditor <KeyPress-Left> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor previous
    %W see insert
}

# Right Arrow
#   Same as Ctrl+F.
bind BltComboEditor <KeyPress-Right> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor next
    %W see insert
}

# Home
#   Position the insertion cursor before the first character.
bind BltComboEditor <Home> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor 0
    %W see insert
}

# End
#   Position the insertion cursor after the last character.
bind BltComboEditor <End> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor end
    %W see insert
}

# Shift + Left 
#   Adjust the selection by one character (previous).
bind BltComboEditor <Shift-Left> {
    if {[%W selection present]} {
	%W selection adjust previous
    } else {
	%W selection range previous insert 
    }
    %W icursor previous
    %W see insert
}

# Shift + Right 
#   Adjust the selection by one character (next).
bind BltComboEditor <Shift-Right> {
    if {[%W selection present]} {
	%W selection adjust next
    } else {
	%W selection range insert next
    }
    %W icursor next
    %W see insert
}

# Shift + Control + Left 
#   Adjust the selection by one word (last word).
bind BltComboEditor <Shift-Control-Left> {
    %W icursor space.start
    if {[%W selection present]} {
	%W selection adjust word.start
    } else {
	%W selection range word.start insert 
    }
    %W icursor word.start
    %W see insert
}

# Shift + Control + Right 
#   Adjust the selection by one word (next word).
bind BltComboEditor <Shift-Control-Right> {
    %W icursor space.end
    if {[%W selection present]} {
	%W selection adjust word.end
    } else {
	%W selection range insert word.end
    }
    %W icursor word.end
    %W see insert
}

# Shift + Home 
#   Adjust the selection to the beginning of the text.
bind BltComboEditor <Shift-Home> {
    if {[%W selection present]} {
	%W selection adjust 0
    } else {
	%W selection range 0 insert
    }
    %W icursor 0
    %W see insert
}

# Shift + End 
#   Adjust the selection to the end of the text.
bind BltComboEditor <Shift-End> {
    if {[%W selection present]} {
	%W selection adjust end
    } else {
	%W selection range insert end
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
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor space.start
    %W icursor word.start
    %W see insert
}

# Alt+D
#   Deletes character from the insertion cursor to the end of the current
#   word.
bind BltComboEditor <Alt-d> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W delete insert word.end
    %W see insert
}

# Alt+F
#   Position the insertion cursor before the next word.
bind BltComboEditor <Alt-f> {
    if {[%W selection present]} {
	%W selection clear
    }
    %W icursor space.end
    %W icursor word.end
    %W see insert
}

# Alt+Backspace
#   Deletes the characters from the insertion cursor to the beginning
#   of the current word.
bind BltComboEditor <Alt-BackSpace> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete word.start insert
    }
    %W see insert
}

# Alt-Delete
#   Deletes the characters from the insertion cursor to the end
#   of the current word.
bind BltComboEditor <Alt-Delete> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete insert word.end
    }
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
# which is wrong.  Ditto for Tab.

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
