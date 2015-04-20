# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# bltTreeView.tcl
#
# Bindings for the BLT treeview widget.
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

namespace eval ::blt::TreeView {
    variable _private
    array set _private {
	afterId -1
	scroll	0
	column	""
	space   off
	x	0
	y	0
    }
}

image create picture ::blt::TreeView::closeIcon -data {
    AAEBAABZACAAAAAAFgAWAAgIAAAAAACPrf8Aj5j/AOn+/wDC//8AJy3kADpE+gAo
    LOMA5///AN7//wAFCLEANkT6AMD8/wDj//8AAAAKAJCt/wDa//8AkJj/ACgt5AAS
    FN4AHynzAAAAEQAAAFYAyOz/AJWt/wDO+P8AAAAGAISP/wAAAF0AN0T6ANb//wA7
    QfgAJC3kAAAAUgDa+f8AAABvAMz3/wAAAGQAka3/AOv+/wDD9/8AKDb6ACUx7wA8
    RPoAKS3kAAAAWQDS//8Awfj/AMj3/wA6Q/oAm6//AML+/wCNmP8AJS3kADhE+gAY
    H9sAzff/AM7//wBha/8Akq3/AJKY/wDz//8APUT6AAAADAB7mP8AKi3kANf7/wAa
    I+4ANET6AJet/wDK//8ANEL6AC1B+gDv//8Ajpj/ACYt5AA5RPoABwm5AAAAfADO
    9/8Ak63/AMH9/wA+RPoAKy3kAOv//wB9i/8AAABQACIt5ADK9/8AAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAjFCkpKSkpKSkpKSkp
    KSkpKSkpQxwAKjMEBAQEBAQEBAQEBAQEBAQEBAw3AERRUVFRUVFRUVFRUVFRUVFR
    UVFRVwALRkZGRkZGRkZGRkZGRkZGRkZGRiAAHTk5OTk5OTk5OTk5OTk5OTk5OTk1
    ADYuLi4uLi4uLi4uLi4uLi4uLi4uNQBMHh4eHh4eHh4eHh4eHh4eHh4eHksABhAQ
    EBAQEBAQEBAQEBAQEBAQEBAFACsJCQkJCQkJCQkJCQkJCQkJCQkJEgA+DQ0NDQ0N
    DQ0NDQ0NDQ0NDQ0NDRIAUggICAgICAgICAgICAgICAgICAgsAFJUVFRUVFRUVFRU
    VFRUVFRUVFRUQQBIKDBYJE9POCQZJ0lJSUlJSUlJSVMARy8vLy8vLy8vLyI9PT09
    PT09PT0HADFCQkJCQkJCQkIXMkUYUDsmDwFACgAfAwMDAwMDAwMDOk4lJSUlJSUl
    LQ4ATVU0ETw8EQJKGxMaAAAAAAAAAAAAAD9WFhYWFhYWFiEVAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA7wEAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUAHgDbBwsACgAvAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAWgMAAAAAAABUUlVFVklTSU9OLVhGSUxFLgA=
}

image create picture ::blt::TreeView::openIcon -data {
    AAEJAAB/ACAAAAAAFgAWAAgIAAAAAADL//8AWJz/ANn//wDR9v8Al/P/AAAAEgAA
    AFcAAAB0AHSJ/wDP9/8AAACRAAAABwCGmP8A5///AKr1/wDJ/v8Az/X/AFhb/wCi
    rf8ATVX+AAAAUwDK3P8AWXb/AAAAKwDs//8AXmX/AKj0/wDH/f8AzP//AK32/wCE
    lf8AVFv/AAAACgAAACcAAABEAHWJ/wAAABEAjOz/AAAAVgDK/v8An8b/AGp1/wDO
    9v8ApPL/AK74/wB4iv8ABAWqAGmT/wBVW/8AWHX/AOv+/wAiK+YA6f//AIjq/wDS
    //8AvPT/AJLw/wAAAAkAu///ANH1/wAAAKUAf63/AAAAQwDg//8AyvT/AML+/wAA
    AC0AISvsAAAAcgDu//8AlMH/AK/4/wByif8Am63/ACk28wDF7v8AV2X/AJXx/wDl
    //8AJy/eAFZb/wDz//8AAAAMAM31/wCzxv8A6v//AAAAAQCJrf8AGR7WAAAAnQB/
    kP8AcIb/AAAAEwAAAE0Ayv//AKv2/wBSW/8Aep//AND2/wCm8v8Am6X/AO///wAA
    ABoAc4n/ANj//wCAlv8Aw+f/AAAA3gAYHMoAd5b/AJOt/wDI/v8AQEj0AAAAjgCX
    p/8AV1v/AJmk/wCF6P8ABgemAAAA5QCP7v8Arvf/AOv//wCZ9P8AAAAWAMX1/5UA
    gQCAJZBDgFOAAIAAgCKAS48XgDKARIBngACARYBHkEKAYoBegACAL4B+kB2AKYAI
    gACAUJE3gEyAWoAYgE2SA4BtgHKADZJAgHGAeIBVkg6AKoBsgFuAa4BBgFSAEYA8
    gASAY4ArgAqAM4hGgHWAIYALgAKAdoA2gCaAeYA5gE6ABYB8gDiBGYB7glaCNYBz
    gACAJ4A+hzuASIAsgGSAG4APgGCAHoB6gC2AMIA9gACAJ4BYhwGAKIAcgHCCEIAo
    gV+AboAjgACAJ4BvkGmAaoAjgACAJ4BKkE+AH4A/gACAJ4ATh2aAFoAugCSACYFo
    gUmAXIA0gF2AAIAVgGWHUoAagAeGI4B9gACAAIAGgFmAYYAxgHSAEoB0gFGAIIAU
    gHeAV4gAggCAOoUhgAyKAJUAlQCVAO8BAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAFAB4A2wcLAAoALwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEYDAAAAAAAAVFJV
    RVZJU0lPTi1YRklMRS4A
}

# Sorting direction icons
# Down arrow
image create picture blt::TreeView::sortdown -data {
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
image create picture blt::TreeView::sortup -data {
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

option add *BltTreeView*Column.increasingIcon blt::TreeView::sortup
option add *BltTreeView*Column.decreasingIcon blt::TreeView::sortdown
option add *BltTreeView.ColumnCommand blt::TreeView::SortColumn

if { $tcl_platform(platform) == "windows" } {
    if { $tk_version >= 8.3 } {
	set cursor "@[file join $blt_library treeview.cur]"
    } else {
	set cursor "size_we"
    }
    option add *BltTreeView.ResizeCursor [list $cursor]
} else {
    option add *BltTreeView.ResizeCursor \
	[list @$blt_library/treeview.xbm $blt_library/treeview_m.xbm black white]
}

# ----------------------------------------------------------------------------
#
# Initialize --
#
#	Invoked by internally by Treeview_Init routine.  Initializes the
#	default bindings for the treeview widget entries.  These are local
#	to the widget, so they can't be set through the widget's class bind
#	tags.
#
# ----------------------------------------------------------------------------
proc blt::TreeView::Initialize { w } {
    variable _private
    #
    # Active entry bindings
    #
    $w bind Entry <Enter> { 
	%W entry highlight current 
    }
    $w bind Entry <Leave> { 
	%W entry highlight "" 
    }

    #
    # Button bindings
    #
    $w button bind all <ButtonRelease-1> {
	set index [%W nearest %x %y blt::TreeView::_private(who)]
	if { [%W index current] == $index && 
	     $blt::TreeView::_private(who) == "button" } {
	    %W see -anchor nw current
	    %W toggle current
	}
    }
    $w button bind all <Enter> {
	%W button highlight current
    }
    $w button bind all <Leave> {
	%W button highlight ""
    }

    #
    # ButtonPress-1
    #
    #	Performs the following operations:
    #
    #	1. Clears the previous selection.
    #	2. Selects the current entry.
    #	3. Sets the focus to this entry.
    #	4. Scrolls the entry into view.
    #	5. Sets the selection anchor to this entry, just in case
    #	   this is "multiple" mode.
    #
    
    $w bind Entry <ButtonPress-1> { 	
	blt::TreeView::SetSelectionAnchor %W current
    }

    $w bind Entry <Double-ButtonPress-1> {
	if { ![%W cget -flat] } {
	    %W toggle current
	}
    }

    #
    # B1-Motion
    #
    #	For "multiple" mode only.  Saves the current location of the
    #	pointer for auto-scrolling.  Resets the selection mark.
    #
    $w bind Entry <B1-Motion> { 
	set blt::TreeView::_private(x) %x
	set blt::TreeView::_private(y) %y
	set index [%W nearest %x %y]
	set blt::TreeView::_private(scroll) 1
	switch -- [%W cget -selectmode] {
	    "multiple" {
		%W selection mark $index
	    }
	    "single" {
		blt::TreeView::SetSelectionAnchor %W $index
	    }
	}
    }

    #
    # ButtonRelease-1
    #
    $w bind Entry <ButtonRelease-1> { 
	after cancel $blt::TreeView::_private(afterId)
	set blt::TreeView::_private(afterId) -1
	set blt::TreeView::_private(scroll) 0
	switch -- [%W cget -selectmode] {
	    "multiple" {
		%W selection anchor current
	    } 
	    "single" {
		%W entry invoke current
	    }
	}
    }

    #
    # Shift-ButtonPress-1
    #
    #	For "multiple" mode only.
    #

    $w bind Entry <Shift-ButtonPress-1> { 
	if { [%W cget -selectmode] == "multiple" && [%W selection present] } {
	    if { [%W index anchor] == -1 } {
		%W selection anchor current
	    }
	    set index [%W index anchor]
	    %W selection clearall
	    %W selection set $index current
	} elseif { [%W cget -selectmode] == "single" } {
	    blt::TreeView::SetSelectionAnchor %W current
	}
    }
    $w bind Entry <Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Shift-B1-Motion> { 
	# do nothing
    }
    $w bind Entry <Shift-ButtonRelease-1> { 
	after cancel $blt::TreeView::_private(afterId)
	set blt::TreeView::_private(afterId) -1
	set blt::TreeView::_private(scroll) 0
    }

    #
    # Control-ButtonPress-1
    #
    #	For "multiple" mode only.  
    #
    $w bind Entry <Control-ButtonPress-1> { 
	switch -- [%W cget -selectmode] {
	    "multiple" {
		set index [%W index current]
		%W selection toggle $index
		%W selection anchor $index
	    }
	    "single" {
		blt::TreeView::SetSelectionAnchor %W current
	    }
	}
    }
    $w bind Entry <Control-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Control-B1-Motion> { 
	# do nothing
    }
    $w bind Entry <Control-ButtonRelease-1> { 
	after cancel $blt::TreeView::_private(afterId)
	set blt::TreeView::_private(afterId) -1
	set blt::TreeView::_private(scroll) 0
    }

    $w bind Entry <Control-Shift-ButtonPress-1> { 
	if { [%W cget -selectmode] == "multiple" && [%W selection present] } {
	    if { [%W index anchor] == -1 } {
		%W selection anchor current
	    }
	    if { [%W selection includes anchor] } {
		%W selection set anchor current
	    } else {
		%W selection clear anchor current
		%W selection set current
	    }
	} elseif { [%W cget -selectmode] == "single" } {
	    blt::TreeView::SetSelectionAnchor %W current
	}
    }
    $w bind Entry <Control-Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Control-Shift-B1-Motion> { 
	# do nothing
    }

    $w column bind all <Enter> {
	%W column activate current
    }
    $w column bind all <Leave> {
	%W column deactivate
    }
    $w column bind Rule <Enter> {
	%W column activate current
	%W column resize activate current
    }
    $w column bind Rule <Leave> {
	%W column deactivate
	%W column resize deactivate 
    }
    $w column bind Rule <ButtonPress-1> {
	%W column resize anchor %x
    }
    $w column bind Rule <B1-Motion> {
	%W column resize mark %x
    }
    $w column bind Rule <ButtonRelease-1> {
	%W column configure active -width [%W column resize set]
    }
    $w column bind all <ButtonPress-1> {
	set blt::TreeView::_private(column) active
	%W column configure $blt::TreeView::_private(column) \
	    -titlerelief sunken
    }
    $w column bind all <ButtonRelease-1> {
	%W column invoke active
	%W column configure active -titlerelief raised
    }

    # TextBoxStyle
    $w bind TextBoxStyle <Enter> { 
	%W cell activate current 
    }
    $w bind TextBoxStyle <Leave> { 
	%W cell deactivate 
    }
    $w bind TextBoxStyle <ButtonPress-1> { 
	if { ![%W cell writable active] } {
	    blt::TreeView::SetSelectionAnchorFromCell %W active
	}
    }
    $w bind TextBoxStyle <B1-Motion> { 
	set blt::TreeView::_private(x) %x
	set blt::TreeView::_private(y) %y
	set index [%W nearest %x %y]
	set blt::TreeView::_private(scroll) 1
	switch -- [%W cget -selectmode] {
	    "multiple" {
		%W selection mark $index
	    } 
	    "single" {
		blt::TreeView::SetSelectionAnchor %W $index
	    }
	}
    }

    # CheckBoxStyle
    $w bind CheckBoxStyle <Enter> { 
	%W cell activate current 
    }
    $w bind CheckBoxStyle <Leave> { 
	%W cell deactivate 
    }
    $w bind CheckBoxStyle <ButtonPress-1> { 
	if { [%W cell writable active]  && 
	     [%W cell identify active %X %Y] == "button" } {
	    blt::TreeView::ToggleValue %W active
	} else {
	    blt::TreeView::SetSelectionAnchorFromCell %W active
	}
    }
    $w bind CheckBoxStyle <B1-Motion> { 
	break
    }
    $w bind CheckBoxStyle <B1-Motion> { 
	set blt::TreeView::_private(x) %x
	set blt::TreeView::_private(y) %y
	set index [%W nearest %x %y]
	set blt::TreeView::_private(scroll) 1
	switch -- [%W cget -selectmode] {
	    "multiple" {
		%W selection mark $index
	    } 
	    "single" {
		blt::TreeView::SetSelectionAnchor %W $index
	    }
	}
    }

    # ComboBoxStyle
    $w bind ComboBoxStyle <Enter> { 
	set style [%W cell style current]
	if { [%W cell cget current -state] != "posted" } {
	    %W cell activate current 
	}
    }
    $w bind ComboBoxStyle <Leave> { 
	set style [%W cell style current]
	if { [%W cell cget current -state] != "posted" } {
	    %W cell deactivate 
	}
    }
    $w bind ComboBoxStyle <ButtonPress-1> { 
	set blt::TreeView::_private(activeSelection) 0
	if { [%W cell identify current %X %Y] == "button" } {
	    blt::TreeView::PostComboBoxMenu %W current
	} else {
	    blt::TreeView::SetSelectionAnchorFromCell %W active
	}
    }
    $w bind CheckBoxStyle <Motion> { 
	if { [%W cell identify current %X %Y] == "button" } {
	    #%W cell activate current
	} else {
	    #%W cell deactivate 
	}
    }
    $w bind ComboBoxStyle <B1-Motion> { 
	set style [%W cell style current]
	if { [%W cell cget current -state] != "posted" } {
	    set blt::TreeView::_private(x) %x
	    set blt::TreeView::_private(y) %y
	    set cell [%W index @%x,%y]
	    set blt::TreeView::_private(scroll) 1
	    if { $cell != "" } {
		if { $blt::TreeView::_private(activeSelection) } {
		    %W selection mark $cell
		} else {
		    blt::TreeView::SetSelectionAnchorFromCell %W active
		}
	    }
	}
	break
    }
    # We only get <ButtonRelease> events that are generated by the
    # combomenu because of the grab on the combomenu window.  The combomenu
    # will get all normal <ButtonRelease> events.
    #
    # If the pointer is inside of the active cell, this is the click-click
    # method of selecting a menu item.  So wait for the next ButtonRelease
    # event.
    #
    # Otherwise unpost the menu.  The user clicked either on the menu
    # (selected an item) or outside the menu (canceling the operation).
    $w bind ComboBoxStyle <ButtonRelease-1> { 
	after cancel $blt::TreeView::_private(afterId)
	set blt::TreeView::_private(afterId) -1
	set blt::TreeView::_private(scroll) 0
	if { $blt::TreeView::_private(activeSelection) } {
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

    # RadioButtonStyle
    $w bind RadioButtonStyle <Enter> { 
	%W cell activate current 
    }
    $w bind RadioButtonStyle <Leave> { 
	%W cell deactivate 
    }
    $w bind RadioButtonStyle <ButtonPress-1> { 
	if { [%W cell writable active] } {
	    blt::TreeView::SetRadioValue %W active
	}
    }
    $w bind RadioButtonStyle <B1-Motion> { 
	break
    }
}

#
# PostComboBoxMenu --
#
#	Posts the combo menu at the location of the cell requesting it.
#	The menu is selected to the current cell value and we bind to the
#	menu's <<MenuSelect>> event to know if a menu item was selected.
#
#	The most important part is that we set a grab on the menu.  This
#	will force <ButtonRelease> events to be interpreted by the combo
#	menu instead of the tableview widget.
#
proc blt::TreeView::PostComboBoxMenu { w cell } {
    variable _private

    set style [$w cell style $cell]
    set menu [$w style cget $style -menu]
    if { $menu == "" } {
	puts stderr "no menu specified"
	return;				# No menu specified.
    }
    # Get the current value of the cell and select the corresponding menu
    # item.
    set tree [$w cget -tree]
    foreach { row col } [$w cell index $cell] break
    set value [$tree get $row $col ""]
    set item [$menu index -value $value]
    if { $item >= 0 } {
	$menu select $item
    }
    $w cell configure $cell -state posted
    # Watch for <<MenuSelect>> events on the menu.  Set the cell value to
    # the selected value when we get one.
    set _private(posting) [$w cell index $cell]
    bind $menu <<MenuSelect>> \
	[list blt::TreeView::ImportFromComboMenu $w $_private(posting) $menu]

    # Post the combo menu at the bottom of the cell.
    foreach { x1 y1 x2 y2 } [$w cell bbox $cell] break
    incr x1 [winfo rootx $w]
    incr y1 [winfo rooty $w]
    incr x2 [winfo rootx $w]
    incr y2 [winfo rooty $w]
    $menu post right $x2 $y2 $x1 $y1
    blt::grab push $menu 
    bind $menu <Unmap> [list blt::TreeView::UnpostComboBoxMenu $w]
}

#
# ImportFromComboMenu --
#
#	This is called whenever a menu item is selected (via the
#	<<MenuSelect>> event generated by the combomenu).  Gets the
#	currently selected value from the combo menu and sets the
#	corresponding table cell to it.
#
proc blt::TreeView::ImportFromComboMenu { w cell menu } {
    set value [$menu value active]
    set tree [$w cget -tree]
    if { $tree != "" } {
	foreach { row col } [$w cell index $cell] break
	$tree set $row $col $value
    }
    # Execute the callback associated with the style
    $w cell invoke $cell
}

#
# UnpostComboBoxMenu --
#
#	Unposts the combobox menu.  Note that the current value set in
#	the cell style is not propagated to the table here.  This is done
#	via a <<MenuSelect>> event.  We don't know if we're unposting
#	the menu because a menu item was selected or if the user clicked
#	outside of the menu to cancel the operation.
#
proc ::blt::TreeView::UnpostComboBoxMenu { w } {
    variable _private

    # Restore focus right away (otherwise X will take focus away when the
    # menu is unmapped and under some window managers (e.g. olvwm) we'll
    # lose the focus completely).
    catch { focus $_private(focus) }
    set _private(focus) ""
    set cell $_private(posting)
    set _private(posting) none
    # This causes the cell in the table to be set to the current value in
    # the combo style.
    set style [$w cell style $cell]
    set menu [$w style cget $style -menu]
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

# ----------------------------------------------------------------------
#
# AutoScroll --
#
#	Invoked when the user is selecting elements in a treeview widget
#	and drags the mouse pointer outside of the widget.  Scrolls the
#	view in the direction of the pointer.
#
# ----------------------------------------------------------------------
proc blt::TreeView::AutoScroll { w } {
    variable _private

    if { ![winfo exists $w] } {
	return
    }
    if { !$_private(scroll) } {
	return 
    }
    set x $_private(x)
    set y $_private(y)

    set index [$w nearest $x $y]
    if {$y >= [winfo height $w]} {
	$w yview scroll 1 units
	set neighbor down
    } elseif {$y < 0} {
	$w yview scroll -1 units
	set neighbor up
    } else {
	set neighbor $index
    }
    switch -- [$w cget -selectmode] {
	"single" {
	    SetSelectionAnchor $w $neighbor
	}
	"multiple" {
	    $w selection mark $index
	}
    }
    set _private(afterId) [after 50 blt::TreeView::AutoScroll $w]
}

#
# ToggleValue --
#
#	Toggles the value at the location of the cell requesting it.  This
#	is called only for checkbox style cells. The value is pulled from
#	the tree and compared against the style's on value.  If its the
#	"on" value, set the cell value in the table to its "off" value.
#
proc blt::TreeView::ToggleValue { w cell } {
    set style [$w cell style $cell]
    set off [$w style cget $style -offvalue]
    set on  [$w style cget $style -onvalue]
    # Get the cell's current value and set the tree node field to that.
    set tree [$w cget -tree]
    foreach { node key } [$w cell index $cell] break
    set value [$tree get $node $key ""]
    if { [string compare $value $on] == 0 } {
	set value $off
    } else {
	set value $on
    }
    if { $tree != "" } {
	$tree set $node $key $value
    }
    # Execute the callback associated with the style
    $w cell invoke $cell
}

#
# SetRadioValue --
#
#	Toggles the value at the location of the cell requesting it.  This
#	is called only for checkbox style cells. The value is pulled from
#	the tree and compared against the style's on value.  If its the
#	"on" value, set the cell value in the table to its "off" value.
#
proc blt::TreeView::SetRadioValue { w cell } {
    set style [$w cell style $cell]
    set on [$w style cget $style -onvalue]
    set off [$w style cget $style -offvalue]

    # Get the cell's current value and set the tree node field to that.
    set tree [$w cget -tree]
    foreach { node key } [$w cell index $cell] break
    foreach c [$w style cells $style] {
	foreach { n k } $c break
	$tree set $n $k $off
    }
    $tree set $node $key $on
    # Execute the callback associated with the style
    $w cell invoke $cell
}

proc blt::TreeView::SetSelectionAnchorFromCell { w cell } {
    foreach { node col } [$w cell index $cell] break
    SetSelectionAnchor $w $node
}

proc blt::TreeView::SetSelectionAnchor { w tagOrId } {
    set index [$w index $tagOrId]
    $w selection clearall
    $w see $index
    $w focus $index
    if { [$w cget -selectmode] != "none" } {
	$w selection set $index
	$w selection anchor $index
    }
}

# ----------------------------------------------------------------------
#
# MoveFocus --
#
#	Invoked by KeyPress bindings.  Moves the active selection to the
#	entry <where>, which is an index such as "up", "down",
#	"prevsibling", "nextsibling", etc.
#
# ----------------------------------------------------------------------
proc blt::TreeView::MoveFocus { w tagOrId } {
    catch {$w focus $tagOrId}
    if { [$w cget -selectmode] == "single" } {
        $w selection clearall
        $w selection set focus
	$w selection anchor focus
    }
    $w see focus
}

# ----------------------------------------------------------------------
#
# MovePage --
#
#	Invoked by KeyPress bindings.  Pages the current view up or down.
#	The <where> argument should be either "top" or "bottom".
#
# ----------------------------------------------------------------------
proc blt::TreeView::MovePage { w where } {

    # If the focus is already at the top/bottom of the window, we want to
    # scroll a page. It's really one page minus an entry because we want to
    # see the last entry on the next/last page.
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

    # Adjust the entry focus and the view.  Also activate the entry.  just
    # in case the mouse point is not in the widget.
    $w entry highlight view.$where
    $w focus view.$where
    $w see view.$where
    if { [$w cget -selectmode] == "single" } {
        $w selection clearall
        $w selection set focus
    }
}

# ----------------------------------------------------------------------
#
# NextMatch --
#
#	Invoked by KeyPress bindings.  Searches for an entry that starts
#	with the letter <char> and makes that entry active.
#
# ----------------------------------------------------------------------
proc blt::TreeView::NextMatch { w key } {
    if {[string match {[ -~]} $key]} {
	set last [$w index focus]
	if { $last == -1 } {
	    return;			# No focus. 
	}
	set next [$w index next]
	if { $next == -1 } {
	    set next $last
	}
	while { $next != $last } {
	    set label [$w entry cget $next -label]
	    set label [string index $label 0]
	    if { [string tolower $label] == [string tolower $key] } {
		break
	    }
	    set next [$w index -at $next next]
	}
	$w focus $next
	if {[$w cget -selectmode] == "single"} {
	    $w selection clearall
	    $w selection set focus
	}
	$w see focus
    }
}

#------------------------------------------------------------------------
#
# InsertText --
#
#	Inserts a text string into an entry at the insertion cursor.  If
#	there is a selection in the entry, and it covers the point of the
#	insertion cursor, then delete the selection before inserting.
#
# Arguments:
#	w 	Widget where to insert the text.
#	text	Text string to insert (usually just a single character)
#
#------------------------------------------------------------------------
proc blt::TreeView::InsertText { w text } {
    if { [string length $text] > 0 } {
	set index [$w index insert]
	if { ($index >= [$w index sel.first]) && 
	     ($index <= [$w index sel.last]) } {
	    $w delete sel.first sel.last
	}
	$w insert $index $text
    }
}

#------------------------------------------------------------------------
#
# Transpose -
#
#	This procedure implements the "transpose" function for entry
#	widgets.  It tranposes the characters on either side of the
#	insertion cursor, unless the cursor is at the end of the line.  In
#	this case it transposes the two characters to the left of the
#	cursor.  In either case, the cursor ends up to the right of the
#	transposed characters.
#
# Arguments:
#	w 	The entry window.
#
#------------------------------------------------------------------------
proc blt::TreeView::Transpose { w } {
    set i [$w index insert]
    if {$i < [$w index end]} {
	incr i
    }
    set first [expr {$i-2}]
    if {$first < 0} {
	return
    }
    set new [string index [$w get] [expr {$i-1}]][string index [$w get] $first]
    $w delete $first $i
    $w insert insert $new
}

#------------------------------------------------------------------------
#
# GetSelection --
#
#	Returns the selected text of the entry with respect to the -show
#	option.
#
# Arguments:
#	w          Entry window from which the text to get
#
#------------------------------------------------------------------------

proc blt::TreeView::GetSelection { w } {
    set text [string range [$w get] [$w index sel.first] \
                       [expr [$w index sel.last] - 1]]
    if {[$w cget -show] != ""} {
	regsub -all . $text [string index [$w cget -show] 0] text
    }
    return $text
}

#
# SortColumn --
#
#	This is called when the column title button is pressed to sort the
#	table according to this column.  Clicking again will change the
#	order of the sort (increasing or decreasing).
#
proc blt::TreeView::SortColumn { w col } {
    set old [$w sort cget -mark]
    set decreasing 0
    if { $old == $col } {
	set decreasing [$w sort cget -decreasing]
	set decreasing [expr !$decreasing]
    }
    $w sort configure -decreasing $decreasing \
	-columns [list $col treeView] \
	-mark $col
    blt::busy hold $w
    $w sort once
    update
    blt::busy release $w
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

bind BltTreeView <ButtonPress-2> {
    set blt::TreeView::_private(cursor) [%W cget -cursor]
    %W configure -cursor hand1
    %W scan mark %x %y
}

bind BltTreeView <B2-Motion> {
    %W scan dragto %x %y
}

bind BltTreeView <ButtonRelease-2> {
    %W configure -cursor $blt::TreeView::_private(cursor)
}

bind BltTreeView <B1-Leave> {
    if { $blt::TreeView::_private(scroll) } {
	blt::TreeView::AutoScroll %W 
    }
}

bind BltTreeView <B1-Enter> {
    after cancel $blt::TreeView::_private(afterId)
    set blt::TreeView::_private(afterId) -1
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


bind BltTreeView <KeyPress-Up> {
    blt::TreeView::MoveFocus %W up
    if { $blt::TreeView::_private(space) } {
	%W selection toggle focus
    }
}

bind BltTreeView <KeyPress-Down> {
    blt::TreeView::MoveFocus %W down
    if { $blt::TreeView::_private(space) } {
	%W selection toggle focus
    }
}

bind BltTreeView <Shift-KeyPress-Up> {
    blt::TreeView::MoveFocus %W prevsibling
}

bind BltTreeView <Shift-KeyPress-Down> {
    blt::TreeView::MoveFocus %W nextsibling
}

bind BltTreeView <KeyPress-Prior> {
    blt::TreeView::MovePage %W top
}

bind BltTreeView <KeyPress-Next> {
    blt::TreeView::MovePage %W bottom
}

bind BltTreeView <KeyPress-Left> {
    %W close focus
}
bind BltTreeView <KeyPress-Right> {
    %W open focus
    %W see focus -anchor w
}

bind BltTreeView <KeyPress-space> {
    switch -- [%W cget -selectmode] {
	"single" {
	    if { [%W selection includes focus] } {
		%W selection clearall
	    } else {
		%W selection clearall
		%W selection set focus
	    }
	}
	"multiple" {
	    %W selection toggle focus
	}
    }
    set blt::TreeView::_private(space) on
}

bind BltTreeView <KeyRelease-space> { 
    set blt::TreeView::_private(space) off
}

bind BltTreeView <KeyPress-Return> {
    blt::TreeView::MoveFocus %W focus
    set blt::TreeView::_private(space) on
}

bind BltTreeView <KeyRelease-Return> { 
    set blt::TreeView::_private(space) off
}

bind BltTreeView <KeyPress> {
    blt::TreeView::NextMatch %W %A
}

bind BltTreeView <KeyPress-Home> {
    blt::TreeView::MoveFocus %W top
}

bind BltTreeView <KeyPress-End> {
    blt::TreeView::MoveFocus %W bottom
}

bind BltTreeView <KeyPress-F1> {
    %W open -r root
}

bind BltTreeView <KeyPress-F2> {
    eval %W close -r [%W entry children root] 
}

if {[string equal "x11" [tk windowingsystem]]} {
    bind BltTreeView <4> {
	%W yview scroll -5 units
    }
    bind BltTreeView <5> {
	%W yview scroll 5 units
    }
} else {
    bind BltTreeView <MouseWheel> {
	%W yview scroll [expr {- (%D / 120) * 4}] units
    }
}

