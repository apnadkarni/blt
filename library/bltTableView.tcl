# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# bltTableView.tcl
#
#   Event bindings for the BLT tableview widget.
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
    namespace eval TableView {
        variable _private
        array set _private {
            activeSelection     0
            lastFilter          0
            afterId             -1
            bindtags            ""
            column              ""
            icon                blt::TableView::filter
            iconvariable        ""
            posting             none
            row                 ""
            scroll              0
            space               off
            textvariable        ""
            x                   0

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

image create picture blt::TableView::xbutton2 -data {
    AAEBAAAHACAAAAAAEAAQAAgIAAAAywAAAAAAAAD9AAAAlwAAAP8AAAD7AAAAlQEB
    AQEBAQEBAQEBAQEBAQEBAQADAQEBAQEBAQEDAAEBAQACAgMBAQEBAQEDAgIAAQED
    AgQFBgEBAQEDAgQCAwEBAQYFBAUGAQEGBQQCAwEBAQEBBgUEBQYGBQQFAwEBAQEB
    AQEGBQQEBAQFBgEBAQEBAQEBAQYEBAQEBgEBAQEBAQEBAQEGBAQEBAYBAQEBAQEB
    AQEGBQQEBAQCAwEBAQEBAQEDBQQFBgMCBAIDAQEBAQEDAgQFBgEBAwIEAgMBAQED
    AgQFBgEBAQEDAgQCAwEBAAIFBgEBAQEBAQMCAgABAQEABgEBAQEBAQEBAwABAQEB
    AQEBAQEBAQEBAQEBAQHvAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAwAE
    AN8HFQAHACEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAuAQAAAAAAAFRSVUVWSVNJT04t
    WEZJTEUuAA==
}

image create picture blt::TableView::xbutton -data {
AAEBAAA2ACAAAAAACgAKAAgIAAAAAGNjYv+0tLP/hYWB/5eXlv/BwcD/t7ey/1pa
Wf8gIB//ampn/6mpp/9CQkL/LS0s/xUVFf8iIiL/jo6N/7u7uP9kZGH/GhoZ/x8f
H/9UVFH/HBwc/729u/+6urr/Xl5b/4iIhf8uLi3/bW1t/zMzM/8WFhb/WFhX/ysr
Kv+srKv/MDAw/xsbGv9VVVL/KCgn/8bGxf9/f3z/kZGR/4eHg/8AAAD/QkJB/7Gx
rf8qKir/FRUU/1xcWf9xcW//i4uL/1lZWP+jo57/i4uH/5WVlf9BQT//MhEuAAAA
AC4UMyYTEgkAAAMSLSMoJCkpBgYMKSIYACsaDTU1CBoZAAAAEB4pKSoQAAAAABYx
KSkLFgAAAAopKQcHHSkvAA8sKSkFBSEpFQE0HB8gAAAXHw4bJQQnAAAAACcwAu8B
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFAAcA3wcLACkAFgAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAE4BAAAAAAAAVFJVRVZJU0lPTi1YRklMRS4A
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
bind BltTableView <Control-KeyPress-a> {
    %W selection clearall
    %W selection anchor { first first }
    %W selection mark { last last }
}

# 
# ButtonPress assignments
#
#       B1-Enter        start auto-scrolling
#       B1-Leave        stop auto-scrolling
#       ButtonPress-2   start scan
#       B2-Motion       adjust scan
#       ButtonRelease-2 stop scan
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
    #   For "multiple" mode only.  Saves the current location of the
    #   pointer for auto-scrolling.  Resets the selection mark.  
    #
    # ButtonRelease-1

    # Shift-ButtonPress-1
    #
    #   For "multiple" mode only.
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
    #   For "multiple" mode only.  
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
        blt::TableView::PostFilterMenu %W current
    }
    $w column bind ColumnFilter <B1-Motion> { 
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
        %W column resize set 
    }
    $w column bind Resize <ButtonRelease-1> {
        %W column resize mark %x
        %W column resize set 
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

    # PushButtonStyle
    $w bind PushButtonStyle <Enter> { 
        %W cell activate current 
    }
    $w bind PushButtonStyle <Leave> { 
        %W cell deactivate 
    }
    $w bind PushButtonStyle <ButtonPress-1> {   
        #blt::TableView::SetSelectionAnchor %W current
    }
    $w bind PushButtonStyle <B1-Motion> { 
        set blt::TableView::_private(x) %x
        set blt::TableView::_private(y) %y
        set cell [%W index @%x,%y]
        set blt::TableView::_private(scroll) 1
        if { 0 && $cell != "" } {
            if { $blt::TableView::_private(activeSelection) } {
                %W selection mark $cell
            } else {
                blt::TableView::SetSelectionAnchor %W $cell
            }
        }
    }
    $w bind PushButtonStyle <ButtonRelease-1> { 
        after cancel $blt::TableView::_private(afterId)
        set blt::TableView::_private(afterId) -1
        set blt::TableView::_private(scroll) 0
        if { 0 && $blt::TableView::_private(activeSelection) } {
            %W selection mark @%x,%y
        } else {
            set style [%W style get active]
            set var [%W style cget $style -variable]
            set $var [%W index active]
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
        return;                         # No menu specified.
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
    $menu post -align right -box [list $x2 $y2 $x1 $y1]
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
        return;                         # No editor specified.
    }
    # Get the current value of the cell and copy it to the corresponding
    # editor.
    set table [$w cget -table]
    foreach { row col } [$w index $cell] break
    set value [$table get $row $col ""]

    # Post the combo menu at the bottom of the cell.
    foreach { x1 y1 x2 y2 } [$w bbox $cell] break
    incr x1 [winfo rootx $w] 
    incr y1 [winfo rooty $w]
    incr x2 [winfo rootx $w]
    incr y2 [winfo rooty $w]
    $editor post -align right -box [list $x1 $y1 $x2 $y2] \
        -command [list blt::TableView::ImportFromEditor $table $row $col] \
        -text $value
    blt::grab push $editor 
    focus -force $editor
    bind $editor <Unmap> [list blt::TableView::UnpostEditor $w $cell]
}

#
# ImportFromEditor --
#
#   This is called whenever a editor text changes (via the -command
#   callback from the invoke operation of the editor).  Gets the edited
#   text from the editor and sets the corresponding table cell to it.
#
proc blt::TableView::ImportFromEditor { table row col value } {
    $table set $row $col $value
}

#
# UnpostEditor --
#
#   Unposts the editor.  Note that the current value set in the cell style
#   is not propagated to the table here.  This is done via -command
#   callback.  We don't know if we're unposting the editor because the text
#   was changed or if the user clicked outside of the editor to cancel the
#   operation.
#
proc ::blt::TableView::UnpostEditor { w cell } {
    variable _private

    if { [$w type $cell] != "textbox" } {
        return;                         # Not a combobox style cell
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
#       Up                      
#       Down
#       Shift-Up
#       Shift-Down
#       Prior (PageUp)
#       Next  (PageDn)
#       Left
#       Right
#       space           Start selection toggle of entry currently with focus.
#       Return          Start selection toggle of entry currently with focus.
#       Home
#       End
#       F1
#       F2
#       ASCII char      Go to next open entry starting with character.
#
# KeyRelease
#
#       space           Stop selection toggle of entry currently with focus.
#       Return          Stop selection toggle of entry currently with focus.


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
    set _private(lastFilterText) [$w column cget $col -filtertext]
    set _private(lastFilterIcon) [$w column cget $col -filtericon]
    set _private(lastFilterHighlight) [$w column cget $col -filterhighlight]
    
    $menu configure -command [list blt::TableView::UpdateFilter $w]
    $menu configure -font "Arial 9"
    if { ![$menu style exists mystyle] } {
        $menu style create mystyle -font "Arial 9 italic"
    }
    $menu configure \
        -textvariable blt::TableView::_private(textvariable) \
        -iconvariable blt::TableView::_private(iconvariable) 
    set top10 $menu.top10
    if { ![winfo exists $top10] } {
        blt::combomenu $top10 \
            -textvariable blt::TableView::_private(textvariable) \
            -iconvariable blt::TableView::_private(iconvariable) \
            -command [list blt::TableView::UpdateFilter $w]
        if { ![$top10 style exists mystyle] } {
            $top10 style create mystyle -font "Arial 9 italic"
        }
        $top10 add -text "Top 10 by value" \
            -icon $_private(icon) \
            -style mystyle \
            -command [list blt::TableView::Top10ByValueFilter $w] 
        $top10 add -text "Top 10 by frequency" \
            -icon $_private(icon) \
            -style mystyle \
            -command [list blt::TableView::Top10ByFrequencyFilter $w] 
        $top10 add -text "Bottom 10 by value" \
            -icon $_private(icon) \
            -style mystyle \
            -command [list blt::TableView::Bottom10ByValueFilter $w] 
        $top10 add -text "Bottom 10 by frequency" \
            -icon $_private(icon) \
            -style mystyle \
            -command [list blt::TableView::Bottom10ByFrequencyFilter $w] 
    } 
    set search $menu.search
    if { [winfo exists $search] } {
        destroy $search
    }
    switch [$table column type $col] {
        "int" - "long" - "double" {
            BuildNumberSearchFilterMenu $w $search
        }
        default {
            BuildTextSearchFilterMenu $w $search
        }
    }
    $menu delete all
    $menu add -text "All" \
        -command [list blt::TableView::AllFilter $w] \
        -style mystyle \
        -icon $_private(icon) 
    $menu add -text "Empty" \
        -command [list blt::TableView::EmptyFilter $w] \
        -style mystyle \
        -icon $_private(icon)
    $menu add -text "Nonempty" \
        -command [list blt::TableView::NonemptyFilter $w] \
        -style mystyle \
        -icon $_private(icon)
    $menu add -type cascade -text "Top 10" \
        -menu $top10 \
        -style mystyle \
        -icon $_private(icon)
    $menu add -type cascade -text "Custom" \
        -menu $search \
        -style mystyle \
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
            -command  [list blt::TableView::SingleValueFilter $w]
    } else {
        set rows [$table sort -columns $col -unique -rows $rows]
        if { [llength $rows] > 0 } {
            $menu add -type separator
        }
        foreach row $rows {
            set fmtvalue [eval $fmtcmd $row $col]
            set value [$table get $row $col]
            $menu add \
                -text $fmtvalue \
                -value $value \
                -command  [list blt::TableView::SingleValueFilter $w]
        }
    }
    set text [$w column cget $col -filtertext]
    if { $text == "" } {
        $menu select first
    } else {
        set item [$menu index text:$text]
        if { $item != -1 } {
            $menu select $item
        }
    }
}

proc blt::TableView::UpdateFilter { w } {
    variable _private

    set col $_private(column)
    set menu [$w filter cget -menu]
    set item [$menu index selected]
    set text $_private(textvariable)
    set icon $_private(iconvariable)
    if { $text == "All" } {
        $w column configure $col \
            -filtertext "" -filtericon "" -filterhighlight 0
    } else {
        $w column configure $col \
            -filtertext $text -filtericon $icon -filterhighlight 1
    }
    if { $item >= 0 } {
        set style [$menu item cget $item -style]
        set font [$menu style cget $style -font]
        $w column configure $col -filterfont $font
    }
}

proc blt::TableView::AllFilter { w } {
    variable _private

    set col $_private(column)
    $w column configure $col -filterdata ""
    ApplyFilters $w
}

proc IsMember {  list row } {
    puts stderr "IsMember row=$row list=$list"
    set n [lsearch $list $row]
    if { $n >= 0 } {
        puts stderr "$row is true"
    } else {
        puts stderr "$row is false"
    }
    return $n
}

proc blt::TableView::Top10ByFrequencyFilter { w } {
    variable _private

    set col $_private(column)
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w $col]
    set rows [$table sort -frequency -columns $col -row $rows]
    set numRows [llength $rows]
    if { $numRows > 10 } {
        set rows [lrange $rows [expr $numRows - 10] end]
    }
    set expr "(\[lsearch {$rows} \${#}\] >= 0)"
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::Top10ByValueFilter { w } {
    variable _private

    set col $_private(column)
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w $col]
    set rows [$table sort -columns $col -row $rows -nonempty]
    set numRows [llength $rows]
    if { $numRows > 10 } {
        set rows [lrange $rows [expr $numRows - 10] end]
    }
    set expr "(\[lsearch {$rows} \${#}\] >= 0)"
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::Bottom10ByFrequencyFilter { w } {
    variable _private

    set col $_private(column)
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w $col]
    set rows [$table sort -frequency -columns $col -row $rows -decreasing]
    set numRows [llength $rows]
    if { $numRows > 10 } {
        set rows [lrange $rows [expr $numRows - 10] end]
    }
    set expr "(\[lsearch {$rows} \${#}\] >= 0)"
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::Bottom10ByValueFilter { w } {
    variable _private

    set col $_private(column)
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w $col]
    set rows [$table sort -columns $col -row $rows -decreasing]
    set numRows [llength $rows]
    if { $numRows > 10 } {
        set rows [lrange $rows [expr $numRows - 10] end]
    }
    set expr "(\[lsearch {$rows} \${#}\] >= 0)"
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::EmptyFilter { w } {
    variable _private

    set col $_private(column)
    set index [$w column index $col]
    set expr " (!\[info exists ${index}\]) "
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::NonemptyFilter { w } {
    variable _private

    set col $_private(column)
    set index [$w column index $col]
    set expr " (\[info exists ${index}\]) "
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc blt::TableView::SingleValueFilter { w } {
    variable _private

    set col $_private(column)
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
#       puts stderr "find \"$expr\" <= [$table find $expr]"
        return [$table find $expr]
    }
    return "@all"
}

proc blt::TableView::ApplyFilters { w } {
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w -1]
    if { $rows == "@all" } {
        eval $w row expose @all
    } else {
        eval $w row hide @all
        eval $w row expose $rows
    }
}

#
# PostFilterMenu --
#
#   Posts the filter combo menu at the location of the column requesting
#   it.  The menu is selected to the current cell value and we bind to the
#   menu's <<MenuSelect>> event to know if a menu item was selected.
#
#   The most important part is that we set a grab on the menu.  This will
#   force <ButtonRelease> events to be interpreted by the combo menu
#   instead of the tableview widget.
#
proc blt::TableView::PostFilterMenu { w col } {
    variable _private

    set menu [$w filter cget -menu]
    if { $menu == "" } {
        puts stderr "no menu specified"
        return;                         # No menu specified.
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
    bind $menu <Unmap> [list blt::TableView::UnpostFilterMenu $w]
    blt::grab push $menu -global
}

#
# UnpostFilterMenu --
#
#   Unposts the filter menu.  Note that the current value set in the cell
#   style is not propagated to the table here.  This is done via a
#   <<MenuSelect>> event.  We don't know if we're unposting the menu
#   because a menu item was selected or if the user clicked outside of the
#   menu to cancel the operation.
#
proc ::blt::TableView::UnpostFilterMenu { w } {
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


proc ::blt::TableView::BuildNumberSearchFilterMenu { w menu } {
    variable _private

    blt::combomenu $menu \
        -textvariable blt::TableView::_private(textvariable) \
        -iconvariable blt::TableView::_private(iconvariable) \
        -command [list blt::TableView::UpdateFilter $w]

    if { ![$menu style exists mystyle] } {
        $menu style create mystyle -font "Arial 9 italic"
    }
    $menu add -text "Equals..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::EqualsNumberSearch $w] 
    $menu add -text "Not equals..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::NotEqualsNumberSearch $w] 
    $menu add -type separator
    $menu add -text "Greater than..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::GreaterThanNumberSearch $w] 
    $menu add -text "Greater than or equal to..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::GreaterThanOrEqualToNumberSearch $w] 
    $menu add -text "Less than..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::LessThanNumberSearch $w] 
    $menu add -text "Less than or equal to..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::LessThanOrEqualToNumberSearch $w] 
    $menu add -type separator
    $menu add -text "Between..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::BetweenNumberSearch $w] 
}

proc ::blt::TableView::BuildTextSearchFilterMenu { w menu } {
    variable _private

    blt::combomenu $menu \
        -textvariable blt::TableView::_private(textvariable) \
        -iconvariable blt::TableView::_private(iconvariable) \
        -command [list blt::TableView::UpdateFilter $w]

    if { ![$menu style exists mystyle] } {
        $menu style create mystyle -font "Arial 9 italic"
    }
    $menu add -text "Equals..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::EqualsTextSearch $w] 
    $menu add -text "Does not equal..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::NotEqualsTextSearch $w] 
    $menu add -type separator
    $menu add -text "Begins with..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::BeginsWithTextSearch $w] 
    $menu add -text "Ends with..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::EndsWithTextSearch $w] 
    $menu add -type separator
    $menu add -text "Contains..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::ContainsTextSearch $w] 
    $menu add -text "Does not contain..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::NotContainsTextSearch $w] 
    $menu add -type separator
    $menu add -text "Between..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list blt::TableView::BetweenTextSearch $w] 
}

proc blt::TableView::EqualsNumberSearch { w } {
    variable _private

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::comboentry $f.entry \
        -hidearrow yes \
        -clearbutton yes
    blt::tk::label $f.label \
        -text "Search for values that equal:" 
    blt::tk::label $f.hint \
        -text "(one or more values separated by commas)" \
        -font "Arial 9 italic"
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.hint -cspan 2 \
        3,0 $f.cancel -width 1i \
        3,1 $f.ok -width 1i 
    blt::table configure $f r3 -pad 4
    
    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set list [$f.entry get]

    DestroySearchDialog $top

    regsub -all , $list " " list
    foreach value $list {
        if { ![string is double -strict $value] } {
            set result 0
            break
        }
    }
    if { $result && [llength $list] > 0 } {
        set col $_private(column)
        set index [$w column index $col]
        set list [list $list]
        set expr "\[info exists ${index}\] &&
            (\[blt::utils::number inlist \$${index} $list])"
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::NotEqualsNumberSearch { w } {
    variable _private

    set col $_private(column)

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values that do not equal:" 
    blt::tk::label $f.hint \
        -text "(one or more values separated by commas)" \
        -font "Arial 9 italic"
    blt::tk::button $f.ok \
        -text "Apply" -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.hint -cspan 2 \
        3,0 $f.cancel -width 1i \
        3,1 $f.ok -width 1i 
    blt::table configure $f r3 -pad 4

    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set list [$f.entry get]
    DestroySearchDialog $top

    regsub -all , $list " " list
    foreach value $list {
        if { ![string is double -strict $value] } {
            set result 0
            break
        }
    }
    if { $result && [llength $list] > 0 } {
        set col $_private(column)
        set index [$w column index $col]
        set list [list $list]
        set expr "(!\[info exists ${index}\]) ||
            (!\[blt::utils::number inlist \$${index} $list])"
        puts stderr expr=$expr
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::GreaterThanNumberSearch { w } {
    variable _private

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values greater than:" 
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.cancel -width 1i \
        2,1 $f.ok -width 1i 
    blt::table configure $f c0 c1 -width 1.4i
    blt::table configure $f r2 -pad 4

    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set value [$f.entry get]
    DestroySearchDialog $top

    if { $result && [string is double -strict $value] } {
        set col $_private(column)
        set index [$w column index $col]
        set expr "\[info exists ${index}\] &&
            (\[blt::utils::number gt \$${index} $value])"
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::GreaterThanOrEqualToNumberSearch { w } {
    variable _private

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values greater than or equal to:" 
    blt::tk::button $f.ok \
        -text "Apply" -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.cancel -width 1i \
        2,1 $f.ok -width 1i 
    blt::table configure $f c0 c1 -width 1.25i
    blt::table configure $f r2 -pad 4

    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set value [$f.entry get]
    DestroySearchDialog $top

    if { $result && [string is double -strict $value] } {
        set col $_private(column)
        set index [$w column index $col]
        set expr "\[info exists ${index}\] &&
            (\[blt::utils::number ge \$${index} $value])"
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::LessThanNumberSearch { w } {
    variable _private

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values less than:" 
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.cancel -width 1i \
        2,1 $f.ok -width 1i 
    blt::table configure $f c0 c1 -width 1.25i
    blt::table configure $f r2 -pad 4

    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set value [$f.entry get]
    DestroySearchDialog $top

    if { $result && [string is double -strict $value] } {
        set col $_private(column)
        set index [$w column index $col]
        set expr "\[info exists ${index}\] &&
            (\[blt::utils::number lt \$${index} $value])"
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::LessThanOrEqualToNumberSearch { w } {
    variable _private

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values less than or equal to:" 
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.cancel -width 1i \
        2,1 $f.ok -width 1i 
    blt::table configure $f c0 c1 -width 1.25i
    blt::table configure $f r2 -pad 4

    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set value [$f.entry get]
    DestroySearchDialog $top

    if { $result && [string is double -strict $value] } {
        set col $_private(column)
        set index [$w column index $col]
        set expr "\[info exists ${index}\] &&
            (\[blt::utils::number le \$${index} $value])"
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::BetweenNumberSearch { w } {
    variable _private

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::tk::label \
        $f.first_l \
        -text "First" 
    blt::comboentry $f.first \
        -hidearrow yes 
    blt::tk::label $f.last_l \
        -text "Last" 
    blt::comboentry $f.last \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values between first and last:" 
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    blt::table $f \
        0,0 $f.label -cspan 3 -anchor w -pady 4 \
        1,0 $f.first_l -anchor e \
        1,1 $f.first -fill x -cspan 2 -padx 4 \
        2,0 $f.last_l -anchor e \
        2,1 $f.last -fill x -cspan 2 -padx 4 \
        3,1 $f.cancel -width 1i \
        3,2 $f.ok -width 1i 
    blt::table configure $f c1 c2 -width 1.25i
    blt::table configure $f c0 -resize none -width 0.5i
    blt::table configure $f r3 -pad 4
    
    update
    focus $f.first
    set result [ActivateSearchDialog $w $top]
    set first [$f.first get]
    set last [$f.last get]
    DestroySearchDialog $top

    if { $result && [string is double -strict $first] &&
         [string is double -strict $last] } {
        set col $_private(column)
        set index [$w column index $col]
        set expr "\[info exists ${index}\] &&
            (\[blt::utils::number between \$${index} $first $last])"
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::EqualsTextSearch { w } {
    variable _private

    set col $_private(column)

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    set _private(ignoreCase) 0
    set _private(trim) 0
    blt::comboentry $f.entry \
        -hidearrow yes  
    blt::tk::label $f.label \
        -text "Search for values that equal:"  
    blt::tk::label $f.hint \
        -text "(one or more values separated by commas)" \
        -font "Arial 9 italic" 
    blt::tk::checkbutton $f.ignore \
        -text "Ignore case" \
        -variable blt::TableView::_private(ignoreCase) 
    blt::tk::checkbutton $f.trim \
        -text "Trim whitespace" \
        -variable blt::TableView::_private(trim) 
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 } 
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 } 
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.hint -cspan 2 \
        3,0 $f.ignore -anchor w \
        3,1 $f.trim -anchor w \
        4,0 $f.cancel -width 1i \
        4,1 $f.ok -width 1i
    blt::table configure $f c0 c1 -width 1.4i
    blt::table configure $f r4 -pad 4

    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set list [$f.entry get]
    DestroySearchDialog $top

    set flags ""
    if { $_private(trim) } {
        append flags " -trim both"
    }
    if { $_private(ignoreCase) } {
        append flags " -nocase"
    }
    if { $result && [llength $list] > 0 } {
        set col $_private(column)
        set index [$w column index $col]
        regsub -all , $list " " list
        set list [list $list]
        set expr "\[info exists ${index}\] &&
            (\[blt::utils::string inlist \$${index} $list $flags])"
        puts stderr expr=$expr
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::NotEqualsTextSearch { w } {
    variable _private

    set col $_private(column)
    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    set _private(ignoreCase) 0
    set _private(trim) 0
    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values that do not equal:" 
    blt::tk::label $f.hint \
        -text "(one or more values separated by commas)" \
        -font "Arial 9 italic"
    blt::tk::checkbutton $f.ignore \
        -text "Ignore case" \
        -variable blt::TableView::_private(ignoreCase)
    blt::tk::checkbutton $f.trim \
        -text "Trim whitespace" \
        -variable blt::TableView::_private(trim)
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.hint -cspan 2 \
        3,0 $f.ignore -anchor w \
        3,1 $f.trim -anchor w \
        4,0 $f.cancel -width 1i \
        4,1 $f.ok -width 1i
    blt::table configure $f c0 c1 -width 1.25i
    blt::table configure $f r4 -pad 4
    
    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set list [$f.entry get]
    DestroySearchDialog $top

    set flags ""
    if { $_private(trim) } {
        append flags " -trim both"
    }
    if { $_private(ignoreCase) } {
        append flags " -nocase"
    }
    if { $result && [llength $list] > 0 } {
        set col $_private(column)
        set index [$w column index $col]
        regsub -all , $list " " list
        set list [list $list]
        set expr "(!\[info exists ${index}\]) ||
            (!\[blt::utils::string inlist \$${index} $list $flags])"
        puts stderr expr=$expr
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::BeginsWithTextSearch { w } {
    variable _private

    set col $_private(column)
    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    set _private(ignoreCase) 0
    set _private(trim) 0
    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values that begin with:" 
    blt::tk::checkbutton $f.ignore \
        -text "Ignore case" \
        -variable blt::TableView::_private(ignoreCase)
    blt::tk::checkbutton $f.trim \
        -text "Trim whitespace" \
        -variable blt::TableView::_private(trim)
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.ignore -anchor w \
        2,1 $f.trim -anchor w \
        3,0 $f.cancel -width 1i \
        3,1 $f.ok -width 1i
    blt::table configure $f c0 c1 -width 1.25i
    blt::table configure $f r3 -pad 4
    
    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set value [$f.entry get]
    DestroySearchDialog $top

    if { $_private(trim) } {
        append flags " -trim both"
    }
    if { $_private(ignoreCase) } {
        append flags " -nocase"
    }
    if { $_private(search) && [string length $value] > 0 } {
        set col $_private(column)
        set index [$w column index $col]
        set value [list $value]
        set expr "(\[info exists ${index}\]) ||
            (\[blt::utils::string begins \$${index} $value $flags])"
        puts stderr expr=$expr
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::EndsWithTextSearch { w } {
    variable _private

    set col $_private(column)
    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    set _private(ignoreCase) 0
    set _private(trim) 0
    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values that end with:" 
    blt::tk::checkbutton $f.ignore \
        -text "Ignore case" \
        -variable blt::TableView::_private(ignoreCase)
    blt::tk::checkbutton $f.trim \
        -text "Trim whitespace" \
        -variable blt::TableView::_private(trim)
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.ignore -anchor w \
        2,1 $f.trim -anchor w \
        3,0 $f.cancel -width 1i \
        3,1 $f.ok -width 1i
    blt::table configure $f c0 c1 -width 1.25i
    blt::table configure $f r3 -pad 4

    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set value [$f.entry get]
    DestroySearchDialog $top

    set flags ""
    if { $_private(trim) } {
        append flags " -trim both"
    }
    if { $_private(ignoreCase) } {
        append flags " -nocase"
    }
    if { $_private(search) && [string length $value] > 0 } {
        set col $_private(column)
        set index [$w column index $col]
        set value [list $value]
        set expr "(\[info exists ${index}\]) &&
            (\[blt::utils::string ends \$${index} $value $flags])"
        puts stderr expr=$expr
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::ContainsTextSearch { w } {
    variable _private

    set col $_private(column)
    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    set _private(ignoreCase) 0
    set _private(trim) 0
    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values that contain:" 
    blt::tk::checkbutton $f.ignore \
        -text "Ignore case" \
        -variable blt::TableView::_private(ignoreCase)
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.ignore -cspan 2 -anchor w \
        3,0 $f.cancel -width 1i \
        3,1 $f.ok -width 1i
    blt::table configure $f c0 c1 -width 1.25i
    blt::table configure $f r3 -pad 4

    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set value [$f.entry get]
    DestroySearchDialog $top

    set flags ""
    if { $_private(ignoreCase) } {
        append flags " -nocase"
    }
    if { $result && [string length $value] > 0 } {
        set col $_private(column)
        set index [$w column index $col]
        set value [list $value]
        set expr "(\[info exists ${index}\]) &&
            (\[blt::utils::string contains \$${index} $value $flags])"
        puts stderr expr=$expr
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::NotContainsTextSearch { w } {
    variable _private

    set col $_private(column)
    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    set _private(ignoreCase) 0
    set _private(trim) 0
    blt::comboentry $top.frame.entry -hidearrow yes 
    blt::tk::label $top.frame.label \
        -text "Search for values that do not contain:" 
    blt::tk::checkbutton $top.frame.ignore \
        -text "Ignore case" \
        -variable blt::TableView::_private(ignoreCase)
    blt::tk::button $top.frame.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $top.frame.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set blt::TableView::_private(search) 1
    }
    blt::table $top.frame \
        0,0 $top.frame.label -cspan 2 -anchor w -pady 4 \
        1,0 $top.frame.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $top.frame.ignore -cspan 2 -anchor w \
        3,0 $top.frame.cancel -width 1i \
        3,1 $top.frame.ok -width 1i
    blt::table configure $top.frame c0 c1 -width 1.25i
    blt::table configure $top.frame r3 -pad 4
    
    update
    focus $f.entry
    set result [ActivateSearchDialog $w $top]
    set value [$f.entry get]
    DestroySearchDialog $top

    set flags ""
    if { $_private(ignoreCase) } {
        append flags " -nocase"
    }
    if { $result && [string length $value] > 0 } {
        set col $_private(column)
        set index [$w column index $col]
        set value [list $value]
        set expr "(!\[info exists ${index}\]) ||
            (!\[blt::utils::string contains \$${index} $value $flags])"
        puts stderr expr=$expr
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::BetweenTextSearch { w } {
    variable _private

    set col $_private(column)

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    set _private(ignoreCase) 0
    set _private(trim) 0
    blt::tk::label $f.first_l \
        -text "First" 
    blt::comboentry $f.first \
        -hidearrow yes 
    blt::tk::label $f.last_l \
        -text "Last" 
    blt::comboentry $f.last \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values between first and last:" 
    blt::tk::checkbutton $f.ignore \
        -text "Ignore case" \
        -variable blt::TableView::_private(ignoreCase) 
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set blt::TableView::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set blt::TableView::_private(search) 0 }
    blt::table $f \
        0,0 $f.label -cspan 3 -anchor w -pady 4 \
        1,0 $f.first_l -anchor e \
        1,1 $f.first -fill x -cspan 2 -padx 4 \
        2,0 $f.last_l -anchor e \
        2,1 $f.last -fill x -cspan 2 -padx 4 \
        3,1 $f.ignore -cspan 2 -anchor w \
        4,1 $f.cancel -width 1i \
        4,2 $f.ok -width 1i   
    blt::table configure $f c1 c2 -width 1.25i
    blt::table configure $f r4 -pad 4
    blt::table configure $f r3 -pad 2
    blt::table configure $f c0 -resize none -width 0.5i

    update
    focus $f.first
    set result [ActivateSearchDialog $w $top]
    set first [$f.first get]
    set last [$f.last get]
    DestroySearchDialog $top

    set flags ""
    if { $_private(ignoreCase) } {
        append flags " -nocase"
    }
    if { $result && [string length $first] > 0 && [string length $last] > 0} {
        set col $_private(column)
        set index [$w column index $col]
        set first [list $first]
        set last [list $last]
        set expr "(\[info exists ${index}\]) &&
            (\[blt::utils::string between \$${index} $first $last $flags])"
        puts stderr expr=$expr
        $w column configure $col -filterdata $expr
        ApplyFilters $w
    } else {
        set col $_private(column)
        $w column configure $col \
            -filtertext $_private(lastFilterText) \
            -filtericon $_private(lastFilterIcon) \
            -filterhighlight $_private(lastFilterHighlight)
    }
}

proc blt::TableView::CreateSearchDialog { w } {
    variable _private
    
    set top $w.dialog
    if { ![blt::background exists _srchBg] } {
        blt::background create linear _srchBg \
            -highcolor grey97 \
            -lowcolor grey85 \
            -jitter 10 \
            -colorscale log 
    }  
    blt::tk::toplevel $top \
        -borderwidth 2 \
        -relief raised \
        -class SearchDialog \
        -bg _srchBg

    blt::background configure _srchBg -relativeto $top

    wm overrideredirect $top true
    wm withdraw $top
    wm protocol $top WM_DELETE { set blt::TableView::_private(search) 0 }       

    set img blt::TableView::xbutton
    blt::tk::button $top.button -image $img -padx 0 -pady 0 \
        -relief flat -bg _srchBg -overrelief flat -highlightthickness 0 \
        -command { set blt::TableView::_private(search) 0 }
    blt::tk::frame $top.frame -bg _srchBg 
    option add *SearchDialog.frame.BltTkLabel.background _srchBg 
    option add *SearchDialog.frame.BltTkCheckbutton.background _srchBg 
    option add *SearchDialog.frame.BltTkCheckbutton.highlightBackground _srchBg 
    option add *SearchDialog.frame.BltTkButton.highlightBackground _srchBg 
    option add *SearchDialog.frame.BltTkButton.background _srchBg 
    blt::table $top \
        0,0 $top.button -anchor e -padx 2 \
        1,0 $top.frame -padx 4 -pady {0 4}
    return $top
}

proc blt::TableView::ActivateSearchDialog { w top } {
    variable _private
    
    set dw [winfo reqwidth $top]
    set dh [winfo reqheight $top]
    set vw [winfo width $w]
    set vh [winfo height $w]
    set rootx [winfo rootx $w]
    set rooty [winfo rooty $w]
    set x [expr $rootx + ($vw - $dw) / 2]
    set y [expr $rooty + ($vh - $dh) / 2]
    wm geometry $top +$x+$y
    wm deiconify $top
    
    set _private(search) 0
    blt::grab push $top
    tkwait variable blt::TableView::_private(search)
    blt::grab pop
    return $_private(search)
}

proc blt::TableView::DestroySearchDialog { top } {
    set bg [$top cget -background]
    destroy $top
#    blt::background delete $bg
}
