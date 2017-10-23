
package require BLT

namespace eval autofilter {
    variable _current ""
    variable _state
    variable _colors
    variable _table
    variable _families
    variable _tableData
    variable _chkImage
    variable _selected
    variable _isSelected
    variable _symToName
    variable _private
    array set _private {
	icon                blt::TableView::filter
    }
}

proc autofilter::MakeCheckImage { w h } {
    variable _chkImage

    set cw [expr $w - 4]
    set ch [expr $h - 4]
    set x 2
    set y 2
    lappend points \
	$x [expr $y + (0.4 * $ch)] \
	$x [expr $y + (0.6 * $ch)] \
	[expr $x + (0.4 * $cw)] [expr $y + $ch] \
	[expr $x + $cw] [expr $y + (0.2 * $ch)] \
	[expr $x + $cw] $y \
	[expr $x + (0.4 * $cw)] [expr $y + (0.7 * $ch)] \
	$x [expr $y + (0.4 * $ch)] 
    set img [image create picture -width $w -height $h]
    $img blank 0x0
    $img draw polygon -coords $points \
	-antialiased yes -color red2 \
	-shadow {-offset 2 -width 2 -color 0x5F000000}
    set _chkImage $img
}

proc autofilter::NewAutoFilter { w } {
    variable _table
    variable _state
    variable _families 
    variable _isSelected
    variable _symToName
    variable _private

    frame $w -bg white
    blt::comboview $w.comboview \
	-background white
    set view $w.comboview 
    $view configure -font "Arial 9"
    if { ![$view style exists mystyle] } {
        $view style create mystyle -font "Arial 9 italic"
    }
    $view delete all
    $view add -text "All" \
        -command [list blt::TableView::AllFilter $w] \
        -style mystyle \
        -icon $_private(icon) 
    $view add -text "Empty" \
        -command [list blt::TableView::EmptyFilter $w] \
        -style mystyle \
        -icon $_private(icon)
    $view add -text "Nonempty" \
        -command [list blt::TableView::NonemptyFilter $w] \
        -style mystyle \
        -icon $_private(icon)
    $view add -type cascade -text "Top 10" \
        -view $top10 \
        -style mystyle \
        -icon $_private(icon)
    $view add -type cascade -text "Custom" \
        -menu $search \
        -style mystyle \
        -icon $_private(icon)
	
    foreach name [array names _table] {
	array set elemInfo $_table($name)
	set _symToName($elemInfo(symbol)) $elemInfo(name)
	set _state($name) "normal"
    }
    canvas $w.table \
	-highlightthickness 0

    MakeCheckImage 11 11

    blt::scrollset $w.selections \
	-yscrollbar $w.selections.ys \
	-window $w.selections.view 
    blt::tk::scrollbar $w.selections.ys
    blt::comboview $w.selections.view \
	-checkbuttonoutlinecolor grey80  \
	-height 1.5i -borderwidth 0 -font "Arial 9"

    $w.selections.view add -type checkbutton -text (all) \
	-command [list periodicTable::Select $w all]  \
	-variable ::periodicTable::_isSelected(all)

    foreach family [lsort -dictionary [array names _families]] {
	regsub -all -- {-} $family { } label
	set label [string totitle $label]
	$w.selections.view add -type checkbutton -text $label \
	    -command [list periodicTable::Select $w $family] \
	    -variable ::periodicTable::_isSelected($family)
    }
    $w.selections.view add -type separator
    foreach name [lsort -dictionary [array names _table]] {
        array set elem $_table($name)
	set label [string totitle $elem(name)]
	$w.selections.view add -type checkbutton -text $label \
	    -command [list periodicTable::Select $w $name] \
	    -variable ::periodicTable::_isSelected($name)
    }
    blt::tk::button $w.cancel -text "Cancel" -font "Arial 9" \
	-command [list ::periodicTable::Cancel $w] \
	-relief flat -padx 1 -pady 1 -highlightthickness 0
    blt::tk::button $w.ok -text "OK" -font "Arial 9" \
	-command [list ::periodicTable::Ok $w] \
	-relief flat -padx 1 -pady 1 -highlightthickness 0
    blt::table $w \
	0,0 $w.table -fill both -cspan 3 \
	1,0 $w.selections -fill x -cspan 3 \
	2,1 $w.cancel -padx 10 -pady 5 -width 0.8i \
	2,2 $w.ok -padx 10 -pady 5 -width 0.8i
    
    RedrawTable $w
}
    
proc periodicTable::RedrawTable { w } {
    variable _table
    variable _colors
    variable _state
    variable _chkImage

    set sqwidth [winfo pixels . 0.22i]
    set sqheight [winfo pixels . 0.22i]
    set xoffset 4
    set yoffset 4
    set last ""
    set c $w.table
    $c delete all
    foreach name [array names _table] {
        array set elem $_table($name)
        set x1 [expr ($elem(column)-1)*$sqwidth+$xoffset]
        set y1 [expr ($elem(row)-1)*$sqheight+$yoffset]
        set x2 [expr ($elem(column)*$sqwidth)-2+$xoffset]
        set y2 [expr ($elem(row)*$sqheight)-2+$yoffset]
        set family $elem(family)
        if { $_state($name) == "selected" } {
            set fg $_colors($family-foreground)
            set bg $_colors($family-background)
        } else { 
            set fg $_colors($family-foreground)
            set fg white
            set bg $_colors($family-disabledforeground)
        }
        $c create rectangle $x1 $y1 $x2 $y2 -outline $fg -fill $bg \
            -tags $elem(name)-rect
        if { $elem(symbol) != "*" } {
            $c create text [expr ($x2+$x1)/2+1] [expr ($y2+$y1)/2+4] \
                -anchor c -fill $fg \
                -text [string range $elem(symbol) 0 4] \
                -font "Arial 6" -tags $elem(name)-symbol
            $c create text [expr $x2-2] [expr $y1+2] -anchor ne \
                -text $elem(number) -fill $fg \
                -font "math1 4" -tags $elem(name)-number
        }
	if { $_state($elem(name)) == "selected" } {
	    $c create image $x1 $y1 -image $_chkImage -tags $elem(name) \
		-anchor nw
	}
        $c create rectangle $x1 $y1 $x2 $y2 -outline "" -fill "" \
            -tags $elem(name) 
        if { $_state($elem(name)) != "disabled" } {
	    $c bind $elem(name) <Enter> \
		[list periodicTable::ActivateElement %W $elem(name) %X %Y]
	    $c bind $elem(name) <Leave> \
		[list periodicTable::DeactivateElement %W $elem(name)]
	    $c bind $elem(name) <ButtonRelease-1> \
		[list periodicTable::ToggleSelection $w $elem(name)]
        }
    }

    set x [expr 2*$sqwidth+$xoffset+5]
    set y [expr $yoffset+3]
    
    set x [expr 3*$sqwidth+$xoffset+5]
    $c create text [expr $x - 5] $y -text "?" -tag "symbolName elemInfo" \
	-font "Arial 8 bold" -anchor ne
    set x [expr 3*$sqwidth+$xoffset+5]
    $c create text [expr $x + 5] $y -text "?" -tag "elementName elemInfo" \
	-font "Arial 8 italic" -anchor nw
    set x [expr 11*$sqwidth+$xoffset]
    $c create text [expr $x - 5] $y -text "?" \
	-tag "atomicNumber elemInfo" \
	-font "Arial 8" -anchor ne
    $c create text [expr $x + 5] $y -text "?" \
	-tag "elementFamily elemInfo" \
	-font "Arial 8 italic" -anchor nw
    $c itemconfigure elemInfo -state hidden
    update
    foreach { x1 y1 x2 y2 } [$c bbox all] break
    set width [expr $x2-$x1+$xoffset*2]
    set height [expr $y2-$y1+$yoffset*2]
    $c configure -height $height -width $width -background white
}


proc periodicTable::Cancel { w } {
    set menu [winfo parent $w]
    set mb [winfo parent $menu]
    $menu unpost
}

proc periodicTable::Ok { w } {
    variable _state
    variable _table
    
    set selected {}
    foreach name [array names _state] {
	if { $_state($name) == "selected" } {
	    array set elem $_table($name)
	    lappend selected $elem(symbol)
	}
    }
    set text [lsort -dictionary $selected]
    set menu [winfo parent $w]
    set mb [winfo parent $menu]
    $mb configure -text [join $text {, }]
    $menu unpost
}

proc periodicTable::ResetMenu { w string } {
    variable _symToName
    variable _state
    regsub -all {,} $string { } string
    foreach name [array names _state] {
	set _state($name) "normal"
    }
    foreach sym $string {
	set name $_symToName($sym)
	set _state($name) selected
    }
    after idle [list periodicTable::RedrawTable $w]
}

# ----------------------------------------------------------------------
# USAGE: select <name> 
#
# Used to manipulate the selection in the table.
#
# ----------------------------------------------------------------------
proc periodicTable::Select { w family } {
    variable _families
    variable _state
    variable _isSelected
    variable _table

    if { $family == "all" } {
	if { $_isSelected($family) } {
	    foreach name [array names _table] {
		set _state($name) "selected"
	    }
	} else {
	    foreach name [array names _table] {
		set _state($name) "normal"
	    }
	    foreach family [array names _isSelected] {
		set _isSelected($family) 0
	    }
	}
    } elseif { [info exists _families($family)] } {
	if { $_isSelected($family) } {
	    foreach name $_families($family) {
		set _state($name) "selected"
	    }
	} else {
	    foreach name $_families($family) {
		set _state($name) "normal"
	    }
	}
    } elseif { [info exists _state($family)] } {
	if { $_isSelected($family) } {
	    set _state($family) "selected"
	} else {
	    set _state($family) "normal"
	}
    }	
    after idle [list periodicTable::RedrawTable $w]
}

# ----------------------------------------------------------------------
# USAGE: select <name> 
#
# Used to manipulate the selection in the table.
#
# ----------------------------------------------------------------------
proc periodicTable::ToggleSelection { w name } {
    variable _state

    if { ![info exists _state($name)] } {
	set _state($name) "selected"
    } else {
	set state $_state($name)
	if { $state == "selected" } {
	    set _state($name) "normal"
	} else {
	    set _state($name) "selected"
	}
    }
    after idle [list periodicTable::RedrawTable $w]
}

proc periodicTable::ActivateElement { c id x y } {
    variable _colors
    variable _table
    
    array set elem $_table($id)
    set family $elem(family)
    set fg $_colors($family-activeforeground)
    set bg $_colors($family-activebackground)
    $c itemconfigure $id-rect -outline black -width 1 -fill $bg
    $c itemconfigure $id-number -fill white
    $c itemconfigure $id-symbol -fill white

    $c itemconfigure elementName -text $elem(name)
    $c itemconfigure symbolName -text $elem(symbol)
    $c itemconfigure atomicNumber -text "#$elem(number)"
    $c itemconfigure atomicWeight -text $elem(weight)
    regsub -all -- {-} $elem(family) { } family
    $c itemconfigure elementFamily -text [string totitle $family]
    $c itemconfigure elemInfo -state normal
}

proc periodicTable::DeactivateElement { c id } {
    variable _table
    variable _colors
    variable _state
    
    array set elem $_table($id)
    set family $elem(family)
    set fg $_colors($family-foreground)
    set bg $_colors($family-background)
    if { $_state($id) == "selected" } {
	set fg $_colors($family-foreground)
	set bg $_colors($family-background)
    } else { 
	set fg $_colors($family-foreground)
	set fg white
	set bg $_colors($family-disabledforeground)
    }
    $c itemconfigure $id-rect -outline $fg -width 1 -fill $bg
    $c itemconfigure $id-number -fill $fg
    $c itemconfigure $id-symbol -fill $fg
    $c itemconfigure elemInfo -state hidden
}

proc periodicTable::value {{value "" }} {
    variable _current
    
    if { $value != "" } {
        set _current $value
    }
}


set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set icon2 [image create picture -file images/blt98.gif]
set icon [image create picture -data $imgData]
set bg white

set image ""
option add *ComboEntry.takeFocus 1

if { [file exists ../library] } {
    set blt_library ../library
}

#    -postcommand {.e.m configure -width [winfo width .e] ; update} \
    set myIcon ""
blt::comboentry .e \
    -image $image \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -arrowrelief flat \
    -textwidth 0 \
    -menu .e.m \
    -exportselection yes \
    -clearbutton yes 

#    -bg $bg 

blt::comboframe .e.m  \
    -restrictwidth min \
    -window .e.m.ptable \
    -highlightthickness 0 \
    -resetcommand [list periodicTable::ResetMenu .e.m.ptable]

autofilter::NewAutoFilter .e.m.autofilter

blt::table . \
    0,0 .e -fill x -anchor n 

