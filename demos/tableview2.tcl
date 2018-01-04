#!../src/bltwish

set count 0
package require BLT

set spinner [image create picture]
set files [glob -nocomplain ./images/spinner*.png]
set numFiles [llength $files]
set img [image create picture]
puts $numFiles
$spinner sequence length $numFiles
for { set i 0 } { $i < $numFiles } { incr i } {
    $img configure -file [lindex $files $i]
    $spinner sequence put $i $img
}
image delete $img 

lappend iconpath ~/hubs/neeshub/indeed/src/icons/filetype/22x22

proc GetFileStyle { w file type } {
    set name [file extension $file]
    set name [string range $name 1 end]
    if { $name == "" || $type == "directory" } {
	set name "folder"
    } 
    if { [string match ~* $name] } {
	set name unknown
    }
    global styles
    if {[info exists styles($name)]} {
	return $styles($name)
    }
    global iconpath
    foreach dir $iconpath {
	set path [file join $dir ${name}.*]
	set files [glob -nocomplain $path]
	set file [lindex $files 0]
	if {"" != $file} {
	    break
	}
    }
    if { $file == "" } {
	set file ~/hubs/neeshub/indeed/src/icons/filetype/22x22/unknown.png
    }
    if { ![file exists $file] } { 
	return ""
    }
    if { [file isdirectory $file] } { 
	return ""
    }
    set img [image create picture -file $file]
    $w style create textbox $name -icon $img -side left
    set styles($name) $name
    return $name
}

proc FixExtIcons { w } {
    set table [$w cget -table]
    foreach {row value} [$table column get name] {
	set type [$table get $row "type"]
	set style [GetFileStyle $w $value $type]
	if { $style != "" } {
	    $w style apply $style [list $row name]
	}
    }
}

proc FormatType { w row col } {
    set table [$w cget -table]
    set value [$table get $row $col]
    if { $value != "directory" } {
    }
    return $value
}


proc FormatSize { w row col } {
    set table [$w cget -table]
    set value [$table get $row $col]
    if { $value < 1000 } {
	return $value
    } elseif { $value < 1e6 } {
	return [format "%.3g kB" [expr $value / 1.0e3]]
    } elseif { $value < 1e9 } {
	return [format "%.3g MB" [expr $value / 1.0e6]]
    } else {
	return [format "%.3g GB" [expr $value / 1.0e9]]
    }
}

proc FormatDate { w row col } {
    set table [$w cget -table]
    set value [$table get $row $col]
    set format "year %Y month %b wknum %U day %j min %M"
    set time [clock seconds]
    array set now [clock format $time -format $format]
    array set then [clock format $value -format $format]
    set now(day) [string trimleft $now(day) 0]
    set then(day) [string trimleft $then(day) 0]
    if { $now(year) != $then(year) } {
	return [clock format $value -format "%h %e, %Y"]
    } elseif { $now(day) >= ($then(day) + 7) } {
	return [clock format $value -format "%h %e %l:%M %p"]
    } elseif { $now(day) != $then(day) } {
	return [clock format $value -format "%a %h %d %l:%M %p"]
    } elseif { $now(day) == ($then(day) + 1) } {
	return [clock format $value -format "Yesterday %l:%M %p"]
    } elseif { $now(min) != $then(min) } {
	return [clock format $value -format "Today %l:%M %p"]
    } else {
	return [clock format $value -format "Today %l:%M:%S %p"]
    }
}

array set modes {
   0	---
   1    --x
   2    -w-
   3    -wx
   4    r-- 
   5    r-x
   6    rw-
   7    rwx
}

proc FormatMode { w row col } {
    set table [$w cget -table]
    set value [$table get $row $col]
    global modes
    set mode [format %o [expr $value & 07777]]
    set owner $modes([string index $mode 0])
    set group $modes([string index $mode 1])
    set world $modes([string index $mode 2])
    return "${owner}${group}${world}"
}

set view .ss.view

set top [file normalize $env(HOME)/src/blt]
set trim "$top"

set table [blt::datatable create]    
puts stderr [time { $table dir $top -pattern "*" -fields all }]
#puts stderr [time { $table dir $top -pattern ".*" -fields all}]

blt::scrollset .ss \
    -window $view \
    -xscrollbar .ss.xs \
    -yscrollbar .ss.ys 
blt::tk::scrollbar .ss.xs
blt::tk::scrollbar .ss.ys

blt::tableview $view \
    -width 6i \
    -height 4i \
    -selectmode multiple \
    -table $table \
    -columnfilters yes

blt::table . \
    0,0 .ss  -fill both 

set img [image create picture -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM1WBrM+rAEMigJ8c3Kb3OSII6kGABhp1JnaK1VGwjwKwtvHqNzzd263M3H4n2OH1QBwGw6
    nQkAOw==
}]
	 
$view style create textbox date \
    -justify right 
$view column configure name \
    -width 2i \
    -rulewidth 1 \
    -title "Name"
$view column configure mtime \
    -style date \
    -title "Modification Time" \
    -formatcommand [list FormatDate $view]
$view column configure atime \
    -style date \
    -title "Access Time" \
    -formatcommand [list FormatDate $view]
$view column configure ctime \
    -style date \
    -title "Inode Change Time" \
    -formatcommand [list FormatDate $view]
$view style create textbox mode \
    -font "{courier new} 9" \
    -justify right 
$view column configure mode \
    -style mode \
    -title "Mode" \
    -formatcommand [list FormatMode $view]
$view style create textbox type \
    -justify right
$view column configure type \
    -style type \
    -formatcommand [list FormatType $view]
$view style create textbox size \
    -justify right
$view column configure size \
    -style size \
    -formatcommand [list FormatSize $view]
$view column configure dev \
    -style date 

FixExtIcons $view

focus $view


puts "$count entries"

$view style create checkbox check \
    -onvalue "directory" -offvalue "file" \
    -showvalue yes
$view column configure type -style check

$view style create combobox combo \
    -menu $view.menu \
    -textvariable textVar \
    -iconvariable iconVar 
blt::combomenu $view.menu  \
    -restrictwidth min \
    -textvariable textVar \
    -iconvariable iconVar \
    -yscrollbar $view.menu.ybar \
    -xscrollbar $view.menu.xbar
blt::tk::scrollbar $view.menu.xbar 
blt::tk::scrollbar $view.menu.ybar

$view.menu add -text directory -value directory
$view.menu add -text file -value file

#$view column configure type -style combo 
