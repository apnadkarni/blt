package require BLT
package require blt_sftp

set sftp [blt::sftp create -host nees.org]

set tree [blt::tree create]
puts stderr "before tree"
puts stderr time=[time {$sftp dirtree ~/indeed $tree}]
puts stderr "after tree"

set view .ss.view

blt::scrollset .ss \
    -window $view \
    -xscrollbar .ss.x \
    -yscrollbar .ss.y 
blt::tk::scrollbar .ss.x
blt::tk::scrollbar .ss.y

blt::treeview $view -tree $tree
blt::table . \
    0,0 .ss -fill both

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
    set tree [$w cget -tree]
    foreach {row value} [$table column get name] {
	set type [$table get $row "type"]
	set style [GetFileStyle $w $value $type]
	if { $style != "" } {
	    $w style apply $style [list $row name]
	}
    }
}

proc FormatSize { node value } {
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

proc FormatDate { node  value } {
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

proc FormatMode { node mode } {
    global modes
    set mode [format %o [expr $mode & 07777]]
    set owner $modes([string index $mode 0])
    set group $modes([string index $mode 1])
    set world $modes([string index $mode 2])
    return "${owner}${group}${world}"
}

foreach name { type mtime mode size } {
    $view column insert end $name
}

$view style create textbox date \
    -justify right 
$view column configure treeView \
    -title "Name" 
$view column configure mtime \
    -style date \
    -title "Modification Time" \
    -formatcommand FormatDate 
$view style create textbox mode \
    -justify right 
$view column configure mode \
    -style mode \
    -title "Mode" \
    -formatcommand FormatMode 
$view style create textbox size \
    -justify right
$view column configure size \
    -style size \
    -formatcommand FormatSize 

focus $view
