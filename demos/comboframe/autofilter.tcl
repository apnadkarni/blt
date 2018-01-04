
package require BLT

if { [blt::winop xdpi] > 150 } {
  image create picture autofilter::filter \
      -file $blt_library/icons/32x32/filter.png
} else {
  image create picture autofilter::filter \
      -file $blt_library/icons/16x16/filter.png
}

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
	icon                autofilter::filter
	textvariable ""
	iconvariable ""
	column		2
	table		""
	tableview	""
	showfreq	0
	selectall	0
    }
}

proc ::autofilter::BuildNumberSearchFilterMenu { w menu } {
    variable _private

    blt::combomenu $menu \
        -textvariable autofilter::_private(textvariable) \
        -iconvariable autofilter::_private(iconvariable) \
        -command [list autofilter::UpdateFilter $w]

    # FIXME: May be a comboframe someday.
    if { ![$menu style exists mystyle] } {
        $menu style create mystyle -font "Arial 9 italic"
    }
    $menu add -text "Equals..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::EqualsNumberSearch $w] 
    $menu add -text "Not equals..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::NotEqualsNumberSearch $w] 
    $menu add -type separator
    $menu add -text "Greater than..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::GreaterThanNumberSearch $w] 
    $menu add -text "Greater than or equal to..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::GreaterThanOrEqualToNumberSearch $w] 
    $menu add -text "Less than..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::LessThanNumberSearch $w] 
    $menu add -text "Less than or equal to..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::LessThanOrEqualToNumberSearch $w] 
    $menu add -type separator
    $menu add -text "Between..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::BetweenNumberSearch $w] 
}

proc ::autofilter::BuildTextSearchFilterMenu { w menu } {
    variable _private

    blt::combomenu $menu \
        -textvariable autofilter::_private(textvariable) \
        -iconvariable autofilter::_private(iconvariable) \
        -command [list autofilter::UpdateFilter $w]

    # FIXME: May be a comboframe someday.
    if { ![$menu style exists mystyle] } {
        $menu style create mystyle -font "Arial 9 italic"
    }
    $menu add -text "Equals..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::EqualsTextSearch $w] 
    $menu add -text "Does not equal..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::NotEqualsTextSearch $w] 
    $menu add -type separator
    $menu add -text "Begins with..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::BeginsWithTextSearch $w] 
    $menu add -text "Ends with..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::EndsWithTextSearch $w] 
    $menu add -type separator
    $menu add -text "Contains..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::ContainsTextSearch $w] 
    $menu add -text "Does not contain..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::NotContainsTextSearch $w] 
    $menu add -type separator
    $menu add -text "Between..." \
        -icon $_private(icon) \
        -style mystyle \
        -command [list autofilter::BetweenTextSearch $w] 
}

proc autofilter::GetColumnFilterRows { tableview column } {
    variable _private 
    set list {}
    set table [$tableview cget -table]
    for { set c 0 } { $c < [$table numcolumns] } { incr c } {
        set expr [$tableview column cget $c -filterdata]
        if { $c == $column } {
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

proc autofilter::ApplyFilters { tableview } {
    set table [$tableview cget -table]
    set rows [GetColumnFilterRows $tableview -1]
    if { $rows == "@all" } {
        eval $w row expose @all
    } else {
        eval $w row hide @all
        eval $w row expose $rows
    }
}

proc autofilter::EqualsNumberSearch { w } {
    variable _private

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::comboentry $f.entry \
        -hidearrow yes \
        -xbutton yes
    blt::tk::label $f.label \
        -text "Search for values that equal:" 
    blt::tk::label $f.hint \
        -text "(one or more values separated by spaces)" \
        -font "Arial 9 italic"
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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

    set list [split $list]
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

proc autofilter::NotEqualsNumberSearch { w } {
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
        -text "(one or more values separated by spaces)" \
        -font "Arial 9 italic"
    blt::tk::button $f.ok \
        -text "Apply" -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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

    set list [split $list]
    foreach value $list {
        if { ![string is double -strict $value] } {
            set result 0
            break
        }
    }
    if { $result && [llength $list] > 0 } {
        set col $_private(column)X
        set index [$w column index $col]
        set list [list $list]
        set expr "(!\[info exists ${index}\]) ||
            (!\[blt::utils::number inlist \$${index} $list])"
        #puts stderr expr=$expr
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

proc autofilter::GreaterThanNumberSearch { w } {
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
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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

proc autofilter::GreaterThanOrEqualToNumberSearch { w } {
    variable _private

    set top [CreateSearchDialog $w]
    set bg [$top cget -bg]
    set f $top.frame

    blt::comboentry $f.entry \
        -hidearrow yes 
    blt::tk::label $f.label \
        -text "Search for values greater than or equal to:" 
    blt::tk::button $f.ok \
        -text "Apply" -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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

proc autofilter::LessThanNumberSearch { w } {
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
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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

proc autofilter::LessThanOrEqualToNumberSearch { w } {
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
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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

proc autofilter::BetweenNumberSearch { w } {
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
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
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

proc autofilter::EqualsTextSearch { w } {
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
        -text "(one or more values separated by spaces)" \
        -font "Arial 9 italic" 
    blt::tk::checkbutton $f.ignore \
        -text "Ignore case" \
        -variable autofilter::_private(ignoreCase) 
    blt::tk::checkbutton $f.trim \
        -text "Trim whitespace" \
        -variable autofilter::_private(trim) 
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set autofilter::_private(search) 1 } 
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 } 
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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
#        set list [split $list]
#        set list [list $list]
        set expr "\[info exists ${index}\] &&
            (\[blt::utils::string inlist \$${index} $list $flags])"
        #puts stderr expr=$expr
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

proc autofilter::NotEqualsTextSearch { w } {
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
        -text "(one or more values separated by spaces)" \
        -font "Arial 9 italic"
    blt::tk::checkbutton $f.ignore \
        -text "Ignore case" \
        -variable autofilter::_private(ignoreCase)
    blt::tk::checkbutton $f.trim \
        -text "Trim whitespace" \
        -variable autofilter::_private(trim)
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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
#        set list [split $list]
#        set list [list $list]
        set expr "(!\[info exists ${index}\]) ||
            (!\[blt::utils::string inlist \$${index} $list $flags])"
        #puts stderr expr=$expr
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

proc autofilter::BeginsWithTextSearch { w } {
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
        -variable autofilter::_private(ignoreCase)
    blt::tk::checkbutton $f.trim \
        -text "Trim whitespace" \
        -variable autofilter::_private(trim)
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
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

    set flags {}
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
            (\[blt::utils::string begins \$${index} $value $flags])"
        #puts stderr expr=$expr
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

proc autofilter::EndsWithTextSearch { w } {
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
        -variable autofilter::_private(ignoreCase)
    blt::tk::checkbutton $f.trim \
        -text "Trim whitespace" \
        -variable autofilter::_private(trim)
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.ignore -anchor w \
        2,1 $f.trim -anchor w \
        3,0 $f.cancel -width 1i \
        3,1 $f.ok -width 1i
    blt::table configure $f c0 c1 -width 1.4i
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
        #puts stderr expr=$expr
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

proc autofilter::ContainsTextSearch { w } {
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
        -variable autofilter::_private(ignoreCase)
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
    }
    blt::table $f \
        0,0 $f.label -cspan 2 -anchor w -pady 4 \
        1,0 $f.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $f.ignore -cspan 2 -anchor w \
        3,0 $f.cancel -width 1i \
        3,1 $f.ok -width 1i
    blt::table configure $f c0 c1 -width 1.4i
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
        #puts stderr expr=$expr
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

proc autofilter::NotContainsTextSearch { w } {
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
        -variable autofilter::_private(ignoreCase)
    blt::tk::button $top.frame.ok \
        -text "Apply" \
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $top.frame.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    bind $f.entry <KeyPress-Return> {
        set autofilter::_private(search) 1
    }
    blt::table $top.frame \
        0,0 $top.frame.label -cspan 2 -anchor w -pady 4 \
        1,0 $top.frame.entry -cspan 2 -fill x -padx 0.1i \
        2,0 $top.frame.ignore -cspan 2 -anchor w \
        3,0 $top.frame.cancel -width 1i \
        3,1 $top.frame.ok -width 1i
    blt::table configure $top.frame c0 c1 -width 1.4i
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
        #puts stderr expr=$expr
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

proc autofilter::BetweenTextSearch { w } {
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
        -variable autofilter::_private(ignoreCase) 
    blt::tk::button $f.ok \
        -text "Apply" \
        -command { set autofilter::_private(search) 1 }
    blt::tk::button $f.cancel \
        -text "Cancel" \
        -command { set autofilter::_private(search) 0 }
    blt::table $f \
        0,0 $f.label -cspan 3 -anchor w -pady 4 \
        1,0 $f.first_l -anchor e \
        1,1 $f.first -fill x -cspan 2 -padx 4 \
        2,0 $f.last_l -anchor e \
        2,1 $f.last -fill x -cspan 2 -padx 4 \
        3,1 $f.ignore -cspan 2 -anchor w \
        4,1 $f.cancel -width 1i \
        4,2 $f.ok -width 1i   
    blt::table configure $f c1 c2 -width 1.4i
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
        #puts stderr expr=$expr
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

proc autofilter::CreateSearchDialog { w } {
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
    wm protocol $top WM_DELETE { set autofilter::_private(search) 0 }       

    set img autofilter::xbutton
    blt::tk::button $top.button -image $img -padx 0 -pady 0 \
        -relief flat -bg _srchBg -overrelief flat -highlightthickness 0 \
        -command { set autofilter::_private(search) 0 }
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

proc autofilter::ApplyFilters { tableview } {
    set table [$tableview cget -table]
    set rows [GetColumnFilterRows $tableview -1]
    if { $rows == "@all" } {
        eval $tableview row expose @all
    } else {
        eval $tableview row hide @all
        eval $tableview row expose $rows
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

    $_private(tableview) column configure $_private(column) -filterdata ""
    ApplyFilters $_private(tableview)
    $w configure -text "All" -icon $_private(icon)
    Ok $w
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

    set index [$_private(tableview) column index $_private(column)]
    set expr " (!\[info exists ${index}\]) "
    $_private(tableview) column configure $_private(column) -filterdata $expr
    ApplyFilters $_private(tableview)
    $w configure -text "Empty" -icon $_private(icon)
    Ok $w
}

proc autofilter::NonemptyFilter { w } {
    variable _private

    set index [$_private(tableview) column index $_private(column)]
    set expr " (\[info exists ${index}\]) "
    $_private(tableview) column configure $_private(column) -filterdata $expr
    $w configure -text "Nonempty" -icon $_private(icon)
    ApplyFilters $_private(tableview)
    Ok $w
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

proc autofilter::AutoFilter { w tableview } {
    variable _private

    blt::comboframe $w \
	-restrictwidth min \
	-window $w.frame \
	-highlightthickness 0 \
	-bd 1 -relief solid	-fill both \
	-iconvariable myIcon \
	-textvariable myText

    set table [$tableview cget -table]
    set _private(table) $table
    set _private(tableview) $tableview
    set f $w.frame
    blt::tk::frame $w.frame -bd 0 

    blt::comboview $f.comboview \
	-background white -bd 0 \
        -iconvariable $_private(iconvariable)  \
        -textvariable $_private(textvariable)  

    set view $f.comboview 
    $view configure -font "Arial 9"
    if { ![$view style exists mystyle] } {
        $view style create mystyle -font "Arial 9 italic"
    }
    $view delete all
    set top10 $view.top10
    set search $view.search
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
        -menu $top10 \
        -style mystyle \
        -icon $_private(icon)
    $view add -type cascade -text "Custom" \
        -menu $search \
        -style mystyle \
        -icon $_private(icon)
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
    if { [winfo exists $search] } {
        destroy $search
    }
    switch [$table column type $_private(column)] {
        "int" - "long" - "double" {
            BuildNumberSearchFilterMenu $w $search
        }
        default {
            BuildTextSearchFilterMenu $w $search
        }
    }

    set table [blt::datatable create]
    $table column create -label "Select"  
    $table column create -label "Value"  
    $table column create -label "Frequency" 
    set values [$_private(table) sort -frequencylist freq \
		    -column $_private(column) -values -unique] 
    $table column type "Value" [$_private(table) column type $_private(column)]
    $table column type "Frequency" integer
    $table column values "Value" $values
    $table column values "Frequency" $freq
    blt::scrollset $f.ss \
	-window $f.ss.tableview \
	-yscrollbar $f.ss.ys \
	-xscrollbar $f.ss.xs
    blt::tableview $f.ss.tableview \
	-table $table \
	-bd 0 \
	-width 2i \
	-height 1i \
	-highlightthickness 0 \
	-columntitleborderwidth 0 \
	-columntitlebackground grey97 \

    blt::tk::scrollbar $f.ss.xs 
    blt::tk::scrollbar $f.ss.ys
    $f.ss.tableview style configure default -font "Arial 9" -altbg white
    $f.ss.tableview style create checkbox checkbox -showvalue 0 \
	-fillcolor "white"  -altbg white
    $f.ss.tableview configure \
	-columntitlefont "Arial 8"
    $f.ss.tableview column configure "Select" \
	-title " " -style checkbox -weight 0.0
    $f.ss.tableview column configure "Value" \
	-title [$_private(table) column label $_private(column)]
    $f.ss.tableview column configure "Frequency" -hide yes

    blt::tk::checkbutton $f.all \
	-text "Select All" -font "Arial 8" -relief flat \
	-variable autofilter::_private(selectall) \
	-command [list autofilter::SelectAll $w]
    blt::tk::checkbutton $f.freq \
	-text "Show Frequency" -font "Arial 8" -relief flat \
	-variable autofilter::_private(showfreq) \
	-command [list autofilter::ShowFrequency $w]
    blt::tk::button $f.cancel -text "Cancel" \
	-command [list autofilter::Cancel $w] \
	-font "Arial 10" 
    blt::tk::button $f.done -text "Done" \
	-command [list autofilter::Ok $w] \
	-font "Arial 10" 

    blt::table $f \
	0,0 $f.comboview -fill both -cspan 2 -pady {0 2} \
	1,0 $f.ss -fill both -cspan 2 \
	2,0 $f.all -cspan 2 -anchor w \
	3,0 $f.freq -cspan 2 -anchor w \
	4,0 $f.cancel -width 0.8i -pady 2 \
	4,1 $f.done -width 0.8i -pady 2 
    #update idletasks
    #update 
}
    
proc autofilter::ShowFrequency { w } {
    variable _private
    $w.frame.ss.tableview column configure Frequency -show $_private(showfreq)
}

proc autofilter::SelectAll { w } {
    variable _private

    set table [$w.frame.ss.tableview cget -table]
    $table set @all Select $_private(selectall)
}

proc autofilter::Cancel { menu } {
    $menu unpost 0
}

proc autofilter::Ok { menu } {
    $menu unpost 1
}

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set icon2 [image create picture -file images/blt98.gif]
set icon [image create picture -data $imgData]
set bg white

option add *ComboEntry.takeFocus 1

if { [file exists ../library] } {
    set blt_library ../library
}

    set myIcon ""
blt::comboentry .e \
    -textvariable myText \
    -iconvariable myIcon \
    -arrowrelief flat \
    -textwidth 0 \
    -menu .e.autofilter \
    -exportselection yes \
    -xbutton yes 

#    -bg $bg 


set table [blt::datatable create]
$table restore -file ./data/graph4a.tab
$table sort -frequencyarray freqArr -column 2
blt::scrollset .ss \
    -window .ss.tableview \
    -yscrollbar .ss.ys \
    -xscrollbar .ss.xs
blt::tableview .ss.tableview \
    -table $table \
    -bd 0 
blt::tk::scrollbar .ss.xs 
blt::tk::scrollbar .ss.ys

parray freqArr
autofilter::AutoFilter .e.autofilter .ss.tableview

blt::table . \
    0,0 .e -anchor e -width 2i  \
    1,0 .ss -fill both 


bind .e.autofilter <Leave> {
    if { ![string match ".e.autofilter.frame.comboview.*" [winfo containing %X %Y]] } {
	.e.autofilter.frame.comboview deactivate
    }
}

