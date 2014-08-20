# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# tableview.tcl
#
#   Event bindings for the BLT tableview widget.
#
#	Copyright 2012 George A Howlett.
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the
#   "Software"), to deal in the Software without restriction, including
#   without limitation the rights to use, copy, modify, merge, publish,
#   distribute, sublicense, and/or sell copies of the Software, and to
#   permit persons to whom the Software is furnished to do so, subject to
#   the following conditions:
#
#   The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
#   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
#   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
#   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

namespace eval blt {
    namespace eval TableView {
	variable _private
	array set _private {
	    bindtags ""
	    afterId -1
	    scroll	0
	    column	""
	    row     ""
	    space   off
	    x	0
	    y	0
	    activeSelection 0
	    posting none
	    icon blt::TableView::filter
	}
    }
}

# Sorting direction icons
# Down arrow
image create picture blt::TableView::sortdown -data {
    AAEBAAAjACAAAAAACQANAAgIAAA/QQAAyOUAAAAAAABAhAAAgDIAAAAOAAAAAwAA
    zVwAALzpAACo4AAAzf8AAAAnAAAAHAAAnDYAAIiaAAAABgAAzZkAAAARAAB4EQAA
    AAIAAIKhAAC22gAAzJoAAAABAACvYAAAxcAAAMzXAACnSwAAxj8AAAA3AAAADwAA
    jcEAAL6aAAAABAAAFWICAgICExcCAgICAgIPAAsTAgICAhMYCAMMAgICAg0BCgki
    BQICEhkKCgofHQYCHBYaChUgBBMCAgIWChQRAgICAgIWChQRAgICAgIWChQRAgIC
    AgIWChQRAgICAgIWCg4eAgICAgIHEBshAgICAgICAgICAgLvAQAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAABQASANwHAAA1ADoAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAATAQAAAAAAAFRSVUVWSVNJT04tWEZJTEUuAA==
}
# Up arrow 
image create picture blt::TableView::sortup -data {
    AAEBAAAnACAAAAAACQANAAgIewAAOgAAAABdAACaOwAASgAAAA4AAAADhQAA4YkA
    AJp7AADZTAAAOQAAABFXAAClgwAAQAAAAA00AABsfQAAwAAAAAKKAAD2cAAAS4MA
    AKFXAACkAAAABVkAAKFiAACRiwAA/wAAAAxrAADUAAAAATUAAB2ZAAAKAAAAD4oA
    APUAAAAhjAAAegAAAASLAADWjAAAH2cAAGGMAABHAQEBIh4NEAEBAQEBEgIODQEB
    AQEBBxgWCgEBAQEBBxgWCgEBAQEBBxgWCgEBAQEBBxgWCgEBAQUEExgLIBkbAQAX
    CBgaFAMiASQjGBgYDxwBAQEmHxgGCRsBAQEBIRElIgEBAQEBHQwVAQEBAQEBAQEB
    AQEB7wEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUAEgDcBwAANQA6AAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAIwEAAAAAAABUUlVFVklTSU9OLVhGSUxFLgA=
}

# Filter 
image create picture blt::TableView::filter -data {
    AAEBAABbACAAAAAAEAAQAAgIao1I+Iq6YP8AAAAASI4KRY+wcf9mnTX/yeC2/7nW
    n/9roDz/xd2w/1Z2OP9XjyX/TJENQ3SZUf9ulUz/P1sk+UqQDEOkyoP/krRy/2Kd
    Lv+VvnL/a5BM/5i9d/+GuFr/caFH/0KHB0m82KP/e7FM/2qMS/9AcxP/iqpq/73Y
    pv+qzYv/k79s/zpVIfmEt1f/kbVv/22VR/ez05j/mbWA/6PJgf+v0JL/eJ1T/1GM
    Hv9DeRP/RV8r/4m6Xv9Yii3/dJBa/6HIgP+FsV//tNOZ/1BtNP+KtmP/PncM/5PA
    bP9bgTj/SY8LRHiaV/hynkz/cZpN/02TDkN9o1n/YYc+92iNQ/eZtIH/cI5U/z5Y
    J/94pVD/Q3QY/5S6cv+82KT/RIcHSEViKfmOrW//dptR/0aAE/98s03/iKlo+Ft/
    Ofeu0JD/p8yI/3CmQf91nU/3fK9P/32zTv+Rs3H/cZhK90pmL/9egj34OmcT/wIC
    AgICAgICAgICAgICAgICAgICAgIiIhkCAgICAgICAgICAgICQ0IPSAICAgICAgIC
    AgICAi1GOEkCAgICAgICAgICAgJYFghYAgICAgICAgICAgICNBYINAICAgICAgIC
    AgICAwoWCAoDAgICAgICAgICOU9BUCM+WTkCAgICAgICED8nGjcbEw0AEAICAgIC
    DEAEHxEBUgs2HDoMAgICPSVWUQcxNRgvRVowTj0CAlcSKEcJJig3F1VNTSRXAgJL
    KC4zICFUBStMLB0VSwICKigpBgdRFDJEOzwOFSoCAlMeSkpKSkpKSkpKSh5TAgIC
    AgICAgICAgICAgICAgLvAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABgAY
    ANwHDAAdAAwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAB+AgAAAAAAAFRSVUVWSVNJT04t
    WEZJTEUuAA==
}

option add *BltTableView.IncreasingIcon blt::TableView::sortup
option add *BltTableView.DecreasingIcon blt::TableView::sortdown
#option add *BltTableView.Column.FilterIcon blt::TableView::filter
option add *BltTableView.ColumnCommand blt::TableView::SortColumn

if { $tcl_platform(platform) == "windows" } {
    if { $tk_version >= 8.3 } {
	set cursor "@[file join $blt_library tableview.cur]"
    } else {
	set cursor "size_we"
    }
    option add *BltTableView.ResizeCursor [list $cursor] widgetDefault

} else {
    option add *BltTableView.ResizeCursor \
	[list @$blt_library/tableview.xbm \
	     $blt_library/tableview_m.xbm \
	     black white] 
}

bind BltTableView <KeyPress-Up> {
    %W focus up
    %W see focus
}
bind BltTableView <KeyPress-Down> {
    %W focus down
    %W see focus
}
bind BltTableView <KeyPress-Left> {
    %W focus left
    %W see focus
}
bind BltTableView <KeyPress-Right> {
    %W focus right
    %W see focus
}

bind BltTableView <KeyPress-space> {
    if { [%W cget -selectmode] == "single" } {
	if { [%W selection includes focus] } {
	    %W selection clearall
	} else {
	    %W selection clearall
	    %W selection set focus
	}
    } else {
	%W selection toggle focus
    }
    set blt::TableView::_private(space) on
}

bind BltTableView <KeyRelease-space> { 
    set blt::TableView::_private(space) off
}

bind BltTableView <KeyPress-Return> {
    blt::TableView::MoveFocus %W focus
    set blt::TableView::_private(space) on
}

bind BltTableView <KeyRelease-Return> { 
    set blt::TableView::_private(space) off
}

bind BltTableView <Enter> {
    focus %W
}

# 
# ButtonPress assignments
#
#	B1-Enter	start auto-scrolling
#	B1-Leave	stop auto-scrolling
#	ButtonPress-2	start scan
#	B2-Motion	adjust scan
#	ButtonRelease-2 stop scan
#

bind BltTableView <B1-Enter> {
    after cancel $blt::TableView::_private(afterId)
    set blt::TableView::_private(afterId) -1
}
bind BltTableView <B1-Leave> {
    if { $blt::TableView::_private(scroll) } {
	blt::TableView::AutoScroll %W 
    }
}
bind BltTableView <ButtonPress-2> {
    set blt::TableView::_private(cursor) [%W cget -cursor]
    %W configure -cursor hand1
    %W scan mark %x %y
}
bind BltTableView <B2-Motion> {
    %W scan dragto %x %y
}
bind BltTableView <ButtonRelease-2> {
    %W configure -cursor $blt::TableView::_private(cursor)
}
if {[string equal "x11" [tk windowingsystem]]} {
    bind BltTableView <4> {
	%W yview scroll -5 units
    }
    bind BltTableView <5> {
	%W yview scroll 5 units
    }
} else {
    bind BltTableView <MouseWheel> {
	%W yview scroll [expr {- (%D / 120) * 4}] units
    }
}

#
# Initialize --
#
#   Invoked when a new TableView widget is created.  Initializes the
#   default bindings for the tableview widget cells, rows, and columns.
#   These bindings can't be set through the widget's class bind tags.  The
#   are specific to the widget instance and need to be set when a tableview
#   widget is created.
#
proc blt::TableView::Initialize { w } {
    variable _private


    # B1-Motion
    #
    #	For "multiple" mode only.  Saves the current location of the
    #	pointer for auto-scrolling.  Resets the selection mark.  
    #
    # ButtonRelease-1

    # Shift-ButtonPress-1
    #
    #	For "multiple" mode only.
    #
    $w bind all <Shift-ButtonPress-1> { 
	if { [%W cget -selectmode] == "multiple" } {
	    if { [%W row index anchor] == -1 } {
		%W selection anchor current
	    }
	    %W selection clearall
	    %W selection set current
	} else {
	    blt::TableView::SetSelectionAnchor %W current
	}
    }
    $w bind all <Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind all <Shift-B1-Motion> { 
	# do nothing
    }
    $w bind all <Shift-ButtonRelease-1> { 
	after cancel $blt::TableView::_private(afterId)
	set blt::TableView::_private(afterId) -1
	set blt::TableView::_private(scroll) 0
    }

    #
    # Control-ButtonPress-1
    #
    #	For "multiple" mode only.  
    #
    $w bind all <Control-ButtonPress-1> { 
	switch -- [%W cget -selectmode] {
	    "multiple" {
		%W selection toggle current
		%W selection anchor current
	    } 
	    "single" {
		blt::TableView::SetSelectionAnchor %W current
	    }
	}
    }
    $w bind all <Control-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind all <Control-B1-Motion> { 
	# do nothing
    }
    $w bind all <Control-ButtonRelease-1> { 
	after cancel $blt::TableView::_private(afterId)
	set blt::TableView::_private(afterId) -1
	set blt::TableView::_private(scroll) 0
    }

    $w bind all <Control-Shift-ButtonPress-1> { 
	switch [%W cget -selectmode] {
	    "multiple" {
		if { [%W selection present] } {
		    if { [%W index anchor] == "" } {
			%W selection anchor current
		    }
		    if { [%W selection includes anchor] } {
			%W selection set anchor current
		    } else {
			%W selection clear anchor current
			%W selection set current
		    }
		}
	    }
	    "single" {
		blt::TableView::SetSelectionAnchor %W current
	    }
	}
    }
    $w bind all <Control-Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind all <Control-Shift-B1-Motion> { 
	# do nothing
    }
    # Column title 
    $w column bind all <Enter> {
	%W column activate current
    }
    $w column bind all <Leave> {
	%W column deactivate
    }
    $w column bind all <ButtonPress-1> {
	set blt::TableView::_private(column) [%W column index current]
	%W column configure $blt::TableView::_private(column) \
	    -activetitlerelief sunken
    }
    $w column bind all <ButtonRelease-1> {
	%W column invoke current
	%W column configure $blt::TableView::_private(column) \
	    -activetitlerelief raised
    }
    # Row title
    $w row bind all <Enter> {
	%W row activate current
    }
    $w row bind all <Leave> {
	%W row deactivate
    }
    $w row bind all <ButtonPress-1> {
	set blt::TableView::_private(row) [%W row index current]
	%W row configure $blt::TableView::_private(row) \
	    -activetitlerelief sunken
    }
    $w row bind all <ButtonRelease-1> {
	%W row invoke current
	%W row configure $blt::TableView::_private(row) \
	    -activetitlerelief raised
    }
    # Column filter 
    $w column bind ColumnFilter <Enter> {
	%W filter activate current
    }
    $w column bind ColumnFilter <Leave> {
	%W filter deactivate
    }
    $w column bind ColumnFilter <ButtonPress-1> { 
	set blt::TableView::_private(column) [%W column index current]
	blt::TableView::PostFilter %W current
    }
    $w column bind ColumnFilter <B1-Motion> { 
	break
    }
    # We only get <ButtonRelease> events that are generated by the combomenu
    # because of the grab on the combomenu window.  The combomenu will get all
    # normal <ButtonRelease> events.
    #
    # If the pointer is inside of the active cell, this is the click-click
    # method of selecting a menu item.  So wait for the next ButtonRelease
    # event.
    #
    # Otherwise unpost the menu.  The user clicked either on the menu
    # (selected an item) or outside the menu (canceling the operation).
    $w column bind ColumnFilter <ButtonRelease-1> { 
	#empty
    }


    # Column resize 
    $w column bind Resize <Enter> {
	%W column activate current
	%W column resize activate current
    }
    $w column bind Resize <Leave> {
	%W column deactivate
	%W column resize deactivate 
    }
    $w column bind Resize <ButtonPress-1> {
	%W column resize anchor %x
    }
    $w column bind Resize <B1-Motion> {
	%W column resize mark %x
	%W column configure current -width [%W column resize current]
	%W column resize anchor %x
    }
    $w column bind Resize <ButtonRelease-1> {
	%W column configure current -width [%W column resize current]
    }
    # Row resize 
    $w row bind Resize <Enter> {
	%W row activate current
	%W row resize activate current
    }
    $w row bind Resize <Leave> {
	%W row deactivate
	%W row resize deactivate 
    }
    $w row bind Resize <ButtonPress-1> {
	%W row resize anchor %y
    }
    $w row bind Resize <B1-Motion> {
	%W row resize mark %y
	%W row configure active -height [%W row resize current]
	%W row resize anchor %y
    }
    $w row bind Resize <ButtonRelease-1> {
	%W row configure active -height [%W row resize current]
    }
    # TextBoxStyle
    $w bind TextBoxStyle <Enter> { 
	%W activate current 
    }
    $w bind TextBoxStyle <Leave> { 
	%W deactivate 
    }
    $w bind TextBoxStyle <ButtonPress-1> { 	
	blt::TableView::SetSelectionAnchor %W current
    }
    $w bind TextBoxStyle <B1-Motion> { 
	set blt::TableView::_private(x) %x
	set blt::TableView::_private(y) %y
	set cell [%W index @%x,%y]
	set blt::TableView::_private(scroll) 1
	if { $cell != "" } {
	    if { $blt::TableView::_private(activeSelection) } {
		%W selection mark $cell
	    } else {
		blt::TableView::SetSelectionAnchor %W $cell
	    }
	}
    }
    $w bind TextBoxStyle <ButtonRelease-1> { 
	after cancel $blt::TableView::_private(afterId)
	set blt::TableView::_private(afterId) -1
	set blt::TableView::_private(scroll) 0
	if { $blt::TableView::_private(activeSelection) } {
	    %W selection mark @%x,%y
	} else {
	    %W invoke active
	}
    }
    $w bind TextBoxStyle <ButtonPress-3> { 
 	if { [%W writable current] } {
	    blt::TableView::PostEditor %W current
 	    break
 	}
    }
    $w bind TextBoxStyle <ButtonRelease-3> { 
 	if { [%W writable @%x,%y] } {
	    if { ![%W inside active %X %Y] } {
		blt::TableView::UnpostEditor %W active
	    }	
	    break
	}
    }
    # CheckBoxStyle
    $w bind CheckBoxStyle <Enter> { 
	%W activate current 
    }
    $w bind CheckBoxStyle <Leave> { 
	%W deactivate 
    }
    $w bind CheckBoxStyle <ButtonPress-1> { 
	if { [%W writable active] } {
	    blt::TableView::ToggleValue %W active
	} else {
	    blt::TableView::SetSelectionAnchor %W current
	}
    }
    $w bind CheckBoxStyle <B1-Motion> { 
	break
    }
    $w bind CheckBoxStyle <ButtonRelease-1> { 
	%W invoke current
    }
    # ComboBoxStyle
    $w bind ComboBoxStyle <Enter> { 
	if { [%W style cget [%W cell style current] -state] != "posted" } {
	    %W cell activate current 
	}
    }
    $w bind ComboBoxStyle <Leave> { 
	if { [%W style cget [%W cell style current] -state] != "posted" } {
	    %W cell deactivate 
	}
    }
    $w bind ComboBoxStyle <ButtonPress-1> { 
	set blt::TableView::_private(activeSelection) 0
	if { [%W cell identify current %X %Y] == "button" } {
	    blt::TableView::PostComboBoxMenu %W current
	} else {
	    blt::TableView::SetSelectionAnchor %W current
	}
    }
    $w bind ComboBoxStyle <B1-Motion> { 
	if { [%W style cget [%W cell style current] -state] != "posted" } {
	    set blt::TableView::_private(x) %x
	    set blt::TableView::_private(y) %y
	    set cell [%W index @%x,%y]
	    set blt::TableView::_private(scroll) 1
	    if { $cell != "" } {
		if { $blt::TableView::_private(activeSelection) } {
		    %W selection mark $cell
		} else {
		    blt::TableView::SetSelectionAnchor %W $cell
		}
	    }
	}
	break
    }
    # We only get <ButtonRelease> events that are generated by the combomenu
    # because of the grab on the combomenu window.  The combomenu will get all
    # normal <ButtonRelease> events.
    #
    # If the pointer is inside of the active cell, this is the click-click
    # method of selecting a menu item.  So wait for the next ButtonRelease
    # event.
    #
    # Otherwise unpost the menu.  The user clicked either on the menu
    # (selected an item) or outside the menu (canceling the operation).
    $w bind ComboBoxStyle <ButtonRelease-1> { 
	after cancel $blt::TableView::_private(afterId)
	set blt::TableView::_private(afterId) -1
	set blt::TableView::_private(scroll) 0
	if { $blt::TableView::_private(activeSelection) } {
	    %W selection mark @%x,%y
	}
    }
    # ImageBoxStyle
    $w bind ImageBoxStyle <Enter> { 
	%W cell activate current 
    }
    $w bind ImageBoxStyle <Leave> { 
	%W cell deactivate 
    }
    $w bind ImageBoxStyle <ButtonPress-1> { 	
	blt::TableView::SetSelectionAnchor %W current
    }
    $w bind ImageBoxStyle <B1-Motion> { 
	set blt::TableView::_private(x) %x
	set blt::TableView::_private(y) %y
	set cell [%W index @%x,%y]
	set blt::TableView::_private(scroll) 1
	if { $cell != "" } {
	    if { $blt::TableView::_private(activeSelection) } {
		%W selection mark $cell
	    } else {
		blt::TableView::SetSelectionAnchor %W $cell
	    }
	}
    }
    $w bind ImageBoxStyle <ButtonRelease-1> { 
	after cancel $blt::TableView::_private(afterId)
	set blt::TableView::_private(afterId) -1
	set blt::TableView::_private(scroll) 0
	if { $blt::TableView::_private(activeSelection) } {
	    %W selection mark @%x,%y
	} else {
	    %W invoke active
	}
    }
}

#
# PostComboBoxMenu --
#
#   Posts the combo menu at the location of the cell requesting it.  The
#   menu is selected to the current cell value and we bind to the menu's
#   <<MenuSelect>> event to know if a menu item was selected.
#
#   The most important part is that we set a grab on the menu.  This will
#   force <ButtonRelease> events to be interpreted by the combo menu
#   instead of the tableview widget.
#
proc blt::TableView::PostComboBoxMenu { w cell } {
    variable _private

    set style [$w cell style $cell]
    set menu  [$w style cget $style -menu]
    if { $menu == "" } {
	puts stderr "no menu specified"
	return;				# No menu specified.
    }
    # Get the current value of the cell and select the corresponding menu
    # item.
    set table [$w cget -table]
    foreach { row col } [$w index $cell] break
    set value [$table get $row $col ""]
    set item [$menu index -value $value]
    if { $item >= 0 } {
	$menu select $item
    }
    $w cell configure $cell -state "posted"
    # Watch for <<MenuSelect>> events on the menu.  Set the cell value to
    # the selected value when we get one.
    set _private(posting) [$w index $cell]
    bind $menu <<MenuSelect>> \
	[list blt::TableView::ImportFromComboBoxMenu $w $_private(posting) $menu]

    # Post the combo menu at the bottom of the cell.
    foreach { x1 y1 x2 y2 } [$w bbox $cell] break
    incr x1 [winfo rootx $w]
    incr y1 [winfo rooty $w]
    incr x2 [winfo rootx $w]
    incr y2 [winfo rooty $w]
    $menu post right $x2 $y2 $x1 $y1
    blt::grab push $menu 
    bind $menu <Unmap> [list blt::TableView::UnpostComboBoxMenu $w]
}

#
# ImportFromMenu --
#
#   This is called whenever a menu item is selected (via the <<MenuSelect>>
#   event generated by the combomenu).  Gets the currently selected value
#   from the combo menu and sets the corresponding table cell to it.
#
proc blt::TableView::ImportFromComboBoxMenu { w cell menu } {
    set value [$menu value active]
    set table [$w cget -table]
    if { $table != "" } {
	foreach { row col } [$w index $cell] break
	$table set $row $col $value
    }
    # Execute the callback associated with the style
    $w cell invoke $cell
}

#
# UnpostComboBoxMenu --
#
#   Unposts the combobox menu.  Note that the current value set in the cell
#   style is not propagated to the table here.  This is done via a
#   <<MenuSelect>> event.  We don't know if we're unposting the menu
#   because a menu item was selected or if the user clicked outside of the
#   menu to cancel the operation.
#
proc ::blt::TableView::UnpostComboBoxMenu { w } {
    variable _private

    # Restore focus right away (otherwise X will take focus away when the
    # menu is unmapped and under some window managers (e.g. olvwm) we'll
    # lose the focus completely).
    catch { focus $_private(focus) }
    set _private(focus) ""
    set cell $_private(posting)
    set _private(posting) none
    # This causes the cell in the table to be set to the current
    # value in the combo style.
    set style [$w cell style $cell]
    set menu  [$w style cget $style -menu]
    if { [info exists _private(cursor)] } {
	$w style configure $style -cursor $_private(cursor)
    }
    $w cell configure $cell -state normal
    if { $menu != "" } {
	# Release grab, if any, and restore the previous grab, if there was
	# one.
	$menu unpost
	#blt::grab pop $menu
    }
}

#
# PostEditor --
#
#   Posts the editor at the location of the cell requesting it.  The editor
#   is initialized to the current cell value and we bind to the editor's
#   <<Value>> event to know if the text was edited.
#
#   The most important part is that we set a grab on the editor.  This will
#   force <ButtonRelease> events to be interpreted by the editor instead of
#   the tableview widget.
#
proc blt::TableView::PostEditor { w cell } {
    set style [$w cell style $cell]
    set editor [$w style cget $style -editor]
    if { $editor == "" } {
	return;				# No editor specified.
    }
    # Get the current value of the cell and copy it to the corresponding
    # editor.
    set table [$w cget -table]
    set value [$table get $row $col ""]
    $editor configure -icon [$w stype cget -icon $style]
    $editor delete 0 end
    $editor insert 0 $value

    # Watch for <<MenuSelect>> events on the menu.  Set the cell value to
    # the selected value when we get one.
    bind $editor <<Value>> \
	[list blt::TableView::ImportFromEditor $w $cell $editor]

    # Post the combo menu at the bottom of the cell.
    foreach { x1 y1 x2 y2 } [$w bbox $cell] break
    incr x2 [winfo rootx $w]
    incr y2 [winfo rooty $w]
    $editor post $x2 $y2 right
    update
    blt::grab push $editor -global
    bind $editor <Unmap> [list blt::TableView::UnpostEditor $w]
}

#
# ImportFromEditor --
#
#   This is called whenever a editor text changes (via the <<Value>> event
#   generated by the editor).  Gets the currently selected value from the
#   editor and sets the corresponding table cell to it.
#
proc blt::TableView::ImportFromEditor { w cell editor } {
    set value [$editor get 0 end]
    set table [$w cget -table]
    if { $table != "" } {
	foreach { row col } [$w index $cell] break
	$table set $row $col $value
    }
    # Execute the callback associated with the cell
    $w cell invoke $cell
}

#
# UnpostEditor --
#
#   Unposts the editor.  Note that the current value set in the cell style
#   is not propagated to the table here.  This is done via a <<Value>>
#   event.  We don't know if we're unposting the editor because the text
#   was changed or if the user clicked outside of the editor to cancel the
#   operation.
#
proc ::blt::TableView::UnpostEditor { w cell } {
    variable _private

    if { [$w type $cell] != "textbox" } {
	return;				# Not a combobox style cell
    }

    # Restore focus right away (otherwise X will take focus away when the
    # editor is unmapped and under some window managers (e.g. olvwm) we'll
    # lose the focus completely).
    catch { focus $_private(focus) }
    set _private(focus) ""

    # This causes the cell in the table to be set to the current
    # value in the text style.
    set style  [$w cell style $cell]
    set editor [$w style cget $style -editor]
    $w cell configure $cell -state normal
    set _private(posting) none
    if { [info exists _private(cursor)] } {
	$w style configure $style -cursor $_private(cursor)
    }
    if { $editor != "" } {
	# Release grab, if any, and restore the previous grab, if there was
	# one.
	set grab [blt::grab current]
	if { $grab != "" } {
	    blt::grab pop $grab
	}
    }
}

#
# ToggleValue --
#
#   Toggles the value at the location of the cell requesting it.  This is
#   called only for checkbox style cells. The value is pulled from the
#   table and compared against the style's on value.  If its the "on"
#   value, set the cell value in the table to its "off" value.
#
proc blt::TableView::ToggleValue { w cell } {
    set style [$w cell style $cell]
    set off   [$w style cget $style -offvalue]
    set on    [$w style cget $style -onvalue]

    # Get the current value of the cell and select the corresponding menu
    # item.
    set table [$w cget -table]
    foreach { row col } [$w index $cell] break
    set value [$table get $row $col ""]
    if { [string compare $value $on] == 0 } {
	set value $off
    } else {
	set value $on
    }
    set table [$w cget -table]
    if { $table != "" } {
	foreach { row col } [$w index $cell] break
	$table set $row $col $value
    }
    # Execute the callback associated with the cell
    $w cell invoke $cell
}

#
# AutoScroll --
#
#   Invoked when the user is selecting elements in a tableview widget and
#   drags the mouse pointer outside of the widget.  Scrolls the view in the
#   direction of the pointer.
#
proc blt::TableView::AutoScroll { w } {
    variable _private

    if { ![winfo exists $w] } {
	return
    }
    if { !$_private(scroll) } {
	return 
    }
    set x $_private(x)
    set y $_private(y)
    set row [$w row nearest $y]
    set col [$w column nearest $x]
    set cell [list $row $col]
    if {$y >= [winfo height $w]} {
	$w yview scroll 1 units
	set neighbor down
    } elseif {$y < 0} {
	$w yview scroll -1 units
	set neighbor up
    } else {
	set neighbor $cell
    }
    if {$x >= [winfo width $w]} {
	$w xview scroll 1 units
	set neighbor left
    } elseif {$x < 0} {
	$w xview scroll -1 units
	set neighbor right
    } else {
	set neighbor $cell
    }
    if { [$w cget -selectmode] == "single" } {
	SetSelectionAnchor $w $neighbor
    } else {
	if { $cell != "" && [$w selection present] } {
	    $w selection mark $cell
	}
    }
    set _private(afterId) [after 50 blt::TableView::AutoScroll $w]
}

#
# SetSelectionAnchor --
#
#   Sets the selection anchor.  Depending upon the mode this could select a
#   row, multiple rows, or one or more cells.
#
proc blt::TableView::SetSelectionAnchor { w cell } {
    variable _private

    set index [$w index $cell]
    if { $index == "" } {
	return
    }
    foreach { row col } $index break
    set _private(activeSelection) 0
    switch -- [$w cget -selectmode] {
	"cells" {
	    $w selection clearall
	    $w selection anchor $cell
	} "single" {
	    $w see $cell
	    $w focus $cell
	    $w selection clearall
	    $w selection set $cell 
	} "multiple" {
	    $w see $cell
	    $w focus $cell
	    $w selection clearall
	    $w selection set $cell 
	    set _private(activeSelection) 1
	}
    }
}

#
# MoveFocus --
#
#    Invoked by KeyPress bindings.  Moves the active selection to the cell
#    $cel, which is an index such as "up", "down", "previous", "next", etc.
#
proc blt::TableView::MoveFocus { w cell } {
    catch {$w focus $cell}
    if { [$w cget -selectmode] == "single" } {
        $w selection clearall
        $w selection set focus
	$w selection anchor focus
    }
    $w see focus
}

#
# MovePage --
#
#    Invoked by KeyPress bindings.  Pages the current view up or down.  The
#    <where> argument should be either "top" or "bottom".
#
proc blt::TableView::MovePage { w where } {

    # If the focus is already at the top/bottom of the window, we want
    # to scroll a page. It's really one page minus an entry because we
    # want to see the last entry on the next/last page.
    if { [$w index focus] == [$w index view.$where] } {
        if {$where == "top"} {
	    $w yview scroll -1 pages
	    $w yview scroll 1 units
        } else {
	    $w yview scroll 1 pages
	    $w yview scroll -1 units
        }
    }
    update

    # Adjust the entry focus and the view.  Also activate the entry.
    # just in case the mouse point is not in the widget.
    $w entry highlight view.$where
    $w focus view.$where
    $w row see view.$where
    if { [$w cget -selectmode] == "single" } {
        $w selection clearall
        $w selection set focus
    }
}

# 
# KeyPress assignments
#
#	Up			
#	Down
#	Shift-Up
#	Shift-Down
#	Prior (PageUp)
#	Next  (PageDn)
#	Left
#	Right
#	space		Start selection toggle of entry currently with focus.
#	Return		Start selection toggle of entry currently with focus.
#	Home
#	End
#	F1
#	F2
#	ASCII char	Go to next open entry starting with character.
#
# KeyRelease
#
#	space		Stop selection toggle of entry currently with focus.
#	Return		Stop selection toggle of entry currently with focus.


if 0 {
bind BltTableView <KeyPress-Prior> {
    blt::TableView::MovePage %W top
}

bind BltTableView <KeyPress-Next> {
    blt::TableView::MovePage %W bottom
}
}
bind BltTableView <KeyPress-Home> {
    blt::TableView::MoveFocus %W top
}

bind BltTableView <KeyPress-End> {
    blt::TableView::MoveFocus %W bottom
}

bind BltTableView <Control-KeyPress-c> { 
    exit 0 
}

bind BltTableView <Control-KeyPress-d> { 
    blt::grab pop %W
}

#
# SetGrab --
#
#   Simulates a grab for a cell.  All component events are redirected to
#   the cell.  The widget Key and Button events are shunted by a bindtag
#   that simply ignores and short circuits event handlers.
#
proc blt::TableView::SetGrab { w cell } {
    variable _private

    set _private(bindtags) [bindtags $w]

    bind grabcell <ButtonPress> break
    bind grabcell <ButtonRelease> {
	if { [%W inside $blt::TableView::_private(posting) %X %Y] } {
	    %W invoke active
	} else { 
	    blt::TableView::UnpostComboBoxMenu %W
	}
	break
    }
    bind grabcell <KeyPress> break
    bind grabcell <Motion> break
    bind grabcell <KeyRelease> break
    bindtags $w [concat grabcell $_private(bindtags)]
    # Redirect all events back to the cell.
    $w grab $cell
}

#
# SortColumn --
#
#   This is called when the column title button is pressed to sort the
#   table according to this column.  Clicking again will change the order
#   of the sort (increasing or decreasing).
#
proc blt::TableView::SortColumn { w col } {
    set old [$w sort cget -column]
    set decreasing 0
    if { $old == $col } {
	set decreasing [$w sort cget -decreasing]
	set decreasing [expr !$decreasing]
    }
    $w sort configure \
	-decreasing $decreasing \
	-columns $col \
	-mark $col
    $w sort once
    $w see [list view.top $col]
    blt::busy hold $w
    update
    blt::busy release $w
}

#
# BuildFiltersMenu --
#
#   Builds a menu of filter options.  This is column-specific as the lower
#   portion of the menu is filled with the unique values of the column.
#
proc blt::TableView::BuildFiltersMenu { w col } {
    variable _private

    set menu [$w filter cget -menu]
    set table [$w cget -table]
    if { $menu == "" || $table == "" } {
	return
    }
    set col [$w column index $col]
    $menu configure -command [list blt::TableView::UpdateFilter $w $col]
    $menu configure -font "Arial 9"
    if { ![$menu style exists mystyle] } {
	$menu style create mystyle -font "Arial 9 italic"
    }
    $menu delete all
    $menu add -text "All" \
	-command [list blt::TableView::FilterAll $w $col] \
	-style mystyle \
	-icon $_private(icon) 
    $menu add -text "Top 10" \
	-command [list blt::TableView::FilterTop10 $w $col] \
	-style mystyle \
	-icon $_private(icon)
    $menu add -text "Bottom 10" \
	-command [list blt::TableView::FilterBottom10 $w $col] \
	-style mystyle \
	-icon $_private(icon)
    $menu add -text "Empty" \
	-command [list blt::TableView::FilterEmpty $w $col] \
	-style mystyle \
	-icon $_private(icon)
    $menu add -text "Nonempty" \
	-command [list blt::TableView::FilterNonempty $w $col] \
	-style mystyle \
	-icon $_private(icon)
    $menu add -text "Custom..." \
	-style mystyle \
	-command [list blt::TableView::CustomFilter $w $col] \
	-icon $_private(icon)
    if { [llength [$table column empty $col]] > 0 } {
	$menu item configure "Empty" -state normal
    } else {
	$menu item configure "Empty" -state disabled
    }
    set fmtcmd [$w column cget $col -formatcommand]
    set table [$w cget -table]
    set list {}
    set rows [GetColumnFilterRows $w $col]
    if { $fmtcmd == "" } {
	set values [$table sort -columns $col -values -unique -rows $rows]
	if { [llength $values] > 0 } {
	    $menu add -type separator
	}
	$menu listadd $values \
		-command  [list blt::TableView::SetFilter $w $col]
    } else {
	set rows [$table sort -columns $col -unique -rows $rows]
	if { [llength $rows] > 0 } {
	    $menu add -type separator
	}
	foreach row $rows {
	    set fmtvalue [eval $fmtcmd $row $col]
	    set value [$table get $row $col]
	    $menu add -text $fmtvalue -value $value \
		-command  [list blt::TableView::SetFilter $w $col]
	}
    }
    set text [$w column cget $col -filtertext]
    if { $text == "" } {
	$menu select 0
    } else {
	set item [$menu index text:$text]
	$menu select $item
    }
}

proc blt::TableView::UpdateFilter { w col } {
    set menu [$w filter cget -menu]
    set item [$menu index selected]
    set text [$menu item cget $item -text]
    set icon [$menu item cget $item -icon]
    if { $text == "All" } {
	$w column configure $col -filterhighlight 0
	$w column configure $col -filtertext "" -filtericon ""
    } else {
	$w column configure $col -filterhighlight 1
	$w column configure $col -filtertext $text -filtericon $icon 
    }
    set style [$menu item cget $item -style]
    set font [$menu style cget $style -font]
    $w column configure $col -filterfont $font
}

proc blt::TableView::FilterAll { w col } {
    $w column configure $col -filterdata ""
    ApplyFilters $w
}

proc blt::TableView::FilterTop10 { w col } {
    set index [$w column index $col]
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w $col]
    set values [$table sort -columns $col -unique -values -row $rows \
		    -decreasing]
    set values [lrange $values 0 9]
    set list {}
    foreach value $values {
	lappend list "(\$${index} == \"$value\")"
    }
    set expr "\[info exists ${index}\]"
    if { [llength $list] > 0 } {
	set expr "$expr && ([join $list " || "]) "
    }
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::FilterBottom10 { w col } {
    set index [$w column index $col]
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w $col]
    set values [$table sort -columns $col -unique -values -row $rows]
    set values [lrange $values 0 9]
    set list {}
    foreach value $values {
	lappend list "(\$${index} == \"$value\")"
    }
    set expr "\[info exists ${index}\]"
    if { [llength $list] > 0 } {
	set expr "$expr && ([join $list " || "]) "
    }
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::FilterEmpty { w col } {
    set index [$w column index $col]
    set expr " (!\[info exists ${index}\]) "
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::FilterNonempty { w col } {
    set index [$w column index $col]
    set expr " (\[info exists ${index}\]) "
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::CustomFilter { w col } {
    $w row show all
}
    
proc blt::TableView::SetFilter { w col } {
    set index [$w column index $col]
    set menu [$w filter cget -menu]
    set item [$menu index selected]
    set value [$menu item cget $item -value]
    if { $value == "" } {
	set value [$menu item cget $item -text]
    }
    set expr "\[info exists ${index}\] && (\$${index} == \"${value}\")"
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::GetColumnFilterRows { w col } {
    set table [$w cget -table]
    set list {}
    for { set c 0 } { $c < [$table numcolumns] } { incr c } {
	set expr [$w column cget $c -filterdata]
	if { $c == $col } {
	    continue
	}
	if { $expr == "" } {
	    continue
	}
	lappend list $expr
    }
    set expr [join $list " && "]
    if { $expr != "" } {
#	puts stderr "find \"$expr\" <= [$table find $expr]"
	return [$table find $expr]
    }
    return "all"
}

proc blt::TableView::ApplyFilters { w } {
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w -1]
    if { $rows == "all" } {
	eval $w row expose all
    } else {
	eval $w row hide all
	eval $w row expose $rows
    }
}

#
# PostFilter --
#
#   Posts the filter combo menu at the location of the column requesting
#   it.  The menu is selected to the current cell value and we bind to the
#   menu's <<MenuSelect>> event to know if a menu item was selected.
#
#   The most important part is that we set a grab on the menu.  This will
#   force <ButtonRelease> events to be interpreted by the combo menu
#   instead of the tableview widget.
#
proc blt::TableView::PostFilter { w col } {
    variable _private

    set menu [$w filter cget -menu]
    if { $menu == "" } {
	puts stderr "no menu specified"
	return;				# No menu specified.
    }
    BuildFiltersMenu $w $col
    update idletasks
    update
    # Get the current value of the cell and select the corresponding menu
    # item.
    set table [$w cget -table]
    # Watch for <<MenuSelect>> events on the menu.  Set the cell value to the
    # selected value when we get one. 
    set _private(posting) [$w column index $col]
    # Post the combo menu at the bottom of the filter button.
    $w see [list view.top $col]
    $w filter post $col
    update
    bind $menu <Unmap> [list blt::TableView::UnpostFilter $w]
    blt::grab push $menu -global
}

#
# UnpostFilter --
#
#   Unposts the filter menu.  Note that the current value set in the cell
#   style is not propagated to the table here.  This is done via a
#   <<MenuSelect>> event.  We don't know if we're unposting the menu
#   because a menu item was selected or if the user clicked outside of the
#   menu to cancel the operation.
#
proc ::blt::TableView::UnpostFilter { w } {
    variable _private

    # Restore focus right away (otherwise X will take focus away when the
    # menu is unmapped and under some window managers (e.g. olvwm) we'll
    # lose the focus completely).
    catch { focus $_private(focus) }
    set _private(posting) none
    $w filter unpost
    set menu [$w filter cget -menu]
    bind $menu <Unmap> {}
    blt::grab pop $menu
}

#
# BuildFiltersMenu --
#
#   Builds a menu of filter options.  This is column-specific as the lower
#   portion of the menu is filled with the unique values of the column.
#
proc blt::TableView::BuildVersion2FiltersMenu { w col } {
    variable _private

    set menu [$w filter cget -menu]
    set table [$w cget -table]
    if { $menu == "" || $table == "" } {
	return
    }
    blt::tk::toplevel $w.filters
    blt::tk::button $w.filters.all \
	-text "All" \
	-image $_private(icon) \
	-command [list blt::TableView::FilterAll $w $col] \
	-compound left
    blt::tk::button $w.filters.topten \
	-text "Top 10" \
	-font "Arial 8 italic" \
	-image $_private(icon) \
	-command [list blt::TableView::FilterTop10 $w $col] \
	-compound left
    blt::tk::button $w.filters.bottomten \
	-text "Bottom 10" \
	-font "Arial 8 italic" \
	-image $_private(icon)  \
	-command [list blt::TableView::FilterBottom10 $w $col] \
	-compound left
    blt::tk::button $w.filters.empty \
	-text "Empty" -command 
	-font "Arial 8 italic" \
	-image $_private(icon) 
	-command [list blt::TableView::FilterEmpty $w $col] \
	-compound left
    blt::tk::button $w.filters.nonempty \
	-text "(non-empty)" \
	-image $_private(icon) \
	-command [list blt::TableView::FilterNonEmpty $w $col] \
	-compound left
    blt::comboview $w.filter.view 
    blt::tk::button $w.filters.apply \
	-text "Apply" \
	-command [list blt::TableView::FilterCancel $w $col] 
    set col [$w column index $col]
    $menu configure -command [list blt::TableView::UpdateFilter $w $col]
    $menu delete all
    $menu add -text "All" \
	-command [list blt::TableView::FilterAll $w $col] \
    $menu add -text "Top 10" \
	-command [list blt::TableView::FilterTop10 $w $col] \
	-icon $_private(icon)
    $menu add -text "Bottom 10" \
	-command [list blt::TableView::FilterBottom10 $w $col] \
	-icon $_private(icon)
    $menu add -text "Custom..." \
	-command [list blt::TableView::CustomFilter $w $col] \
	-icon $_private(icon)
    set fmtcmd [$w column cget $col -formatcommand]
    set rows [$w row expose]
    set values [$table sort -columns $col -unique -values -rows $rows]
    if { [llength $rows] > 0 } {
	$menu add -type separator
    }
    if { $fmtcmd == "" } {
	$menu listadd $values \
		-command  [list blt::TableView::SetFilter $w $col]
    } else {
	foreach value $values {
	    set fmt [eval $fmtcmd [list $value]]
	    $menu add -text $fmt -value $value \
		-command  [list blt::TableView::SetFilter $w $col]
	}
    }
    set text [$w column cget $col -filtertext]
    if { $text == "" } {
	$menu select 0
    } else {
	set item [$menu index text:$text]
	$menu select $item
    }
}
