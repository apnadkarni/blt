
package require BLT

namespace eval autofilter {
    variable _current ""
    variable _state
    variable _colors
    variable _table
    variable _tableData
    variable _chkImage
    variable _selected
    variable _isSelected
    variable _private
    array set _private {
	icon                blt::TableView::filter
	textvariable ""
	iconvariable ""
    }
}

proc autofilter::ApplyFilters { w } {
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w -1]
    if { $rows == "@all" } {
        eval $w row expose @all
    } else {
        eval $w row hide @all
        eval $w row expose $rows
    }
}

proc autofilter::UpdateFilter { w } {
    variable _private

    set col $_private(column)
    set menu $w._filter
    if { ![winfo exists $menu] } {
        return
    }
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

proc autofilter::AllFilter { w } {
    variable _private

    set col $_private(column)
    $w column configure $col -filterdata ""
    ApplyFilters $w
}


proc autofilter::Top10 { w } {
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

proc autofilter::Top100 { w } {
    variable _private

    set col $_private(column)
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w $col]
    set rows [$table sort -columns $col -row $rows -nonempty]
    set numRows [llength $rows]
    if { $numRows > 100 } {
        set rows [lrange $rows [expr $numRows - 100] end]
    }
    set expr "(\[lsearch {$rows} \${#}\] >= 0)"
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc autofilter::Bottom10 { w } {
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

proc autofilter::Bottom100 { w } {
    variable _private

    set col $_private(column)
    set table [$w cget -table]
    set rows [GetColumnFilterRows $w $col]
    set rows [$table sort -columns $col -row $rows -decreasing]
    set numRows [llength $rows]
    if { $numRows > 100 } {
        set rows [lrange $rows [expr $numRows - 100] end]
    }
    set expr "(\[lsearch {$rows} \${#}\] >= 0)"
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc autofilter::EmptyFilter { w } {
    variable _private

    set col $_private(column)
    set index [$w column index $col]
    set expr " (!\[info exists ${index}\]) "
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc autofilter::NonEmptyFilter { w } {
    variable _private

    set col $_private(column)
    set index [$w column index $col]
    set expr " (\[info exists ${index}\]) "
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc autofilter::SingleValueFilter { w } {
    variable _private

    set col $_private(column)
    set index [$w column index $col]
    set menu $w._filter
    if { ![winfo exists $menu] } {
        return
    }
    set item [$menu index selected]
    set value [$menu item cget $item -value]
    if { $value == "" } {
        set value [$menu item cget $item -text]
    }
    set expr "\[info exists ${index}\] && (\$${index} == \"${value}\")"
    $w column configure $col -filterdata $expr
    ApplyFilters $w
}

proc autofilter::NewAutoFilter { w } {
    variable _private

    blt::tk::frame $w -bg white

    blt::comboview $w.comboview \
	-background white
    set view $w.comboview 
    $view configure -font "Arial 9"
    if { ![$view style exists mystyle] } {
        $view style create mystyle -font "Arial 9 italic"
    }
    $view delete all
    $view add -text "All" \
        -command [list autofilter::AllFilter $w] \
        -style mystyle \
        -icon $_private(icon) 
    $view add -text "Empty" \
        -command [list autofilter::EmptyFilter $w] \
        -style mystyle \
        -icon $_private(icon)
    $view add -text "Nonempty" \
        -command [list autofilter::NonemptyFilter $w] \
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
    set top10 $view.top10
    if { ![winfo exists $top10] } {
        blt::combomenu $top10 \
            -textvariable _private(textvariable) \
            -iconvariable _private(iconvariable) \
            -command [list autofilter::UpdateFilter $w]
        if { ![$top10 style exists mystyle] } {
            $top10 style create mystyle -font "Arial 9 italic"
        }
        $top10 add -text "Top 10" \
            -icon $_private(icon) \
            -style mystyle \
            -command [list autofilter::Top10 $w] 
        $top10 add -text "Top 100" \
            -icon $_private(icon) \
            -style mystyle \
            -command [list autofilter::Top100 $w] 
        $top10 add -text "Bottom 10" \
            -icon $_private(icon) \
            -style mystyle \
            -command [list autofilter::Bottom10 $w] 
        $top10 add -text "Bottom 100" \
            -icon $_private(icon) \
            -style mystyle \
            -command [list autofilter::Bottom100 $w] 
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

    blt::scrollset $w.ss \
	-window $w.ss.tableview \
	-yscrollbar $w.ss.ys \
	-xscrollbar $w.ss.xs
    blt::tableview $w.tableview \
	-table $_private(table) \
	-columntitles 
    set table [blt::datatable create]
    $_private(table) column create "Value" 
    $_private(table) column create "Frequency" 
    blt::tk::button $w.all -text "All"
    blt::tk::button $w.none -text "None"
    blt::tk::button $w.cancel -text "Cancel" \
	-command [list periodTable::Cancel $w]
    blt::tk::button $w.done -text "Done" \
	-command [list periodTable::Ok $w]

    blt::table $w \
	0,0 $w.comboview -fill both \
	1,0 $w.ss -fill both \
	2,0 $w.all \
	2,1 $w.none \
	3,0 $w.cancel \
	3,1 $w.done
}
    
proc autofilter::Cancel { w } {
    set menu [winfo parent $w]
    $menu unpost
}

proc autofilter::Ok { w } {
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

proc autofilter::ResetMenu { w string } {
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
    after idle [list autofilter::RedrawTable $w]
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
    -window .e.m.autofilter \
    -highlightthickness 0 \
    -resetcommand [list autofilter::ResetMenu .e.m.autofilter]

autofilter::NewAutoFilter .e.m.autofilter

blt::table . \
    0,0 .e -fill x -anchor n 

