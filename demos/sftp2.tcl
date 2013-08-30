package require BLT
package require blt_sftp

set status ""

proc Cancel {} {
    global credentials 
    set credentials ""
}
	     
proc CollectCredentials {} {
    global credentials 
    set user [.login.user get]
    set pass [.login.pass get]
    set credentials [list $user $pass]
}

proc SetChars {} {
    set varName [.login.hidechars cget -variable]
    global $varName
    set bool [set $varName]
    if { $bool } {
	.login.pass configure -show ""
    } else {
	.login.pass configure 	-show \u25CF 
    }
}

proc GetPassword { user } {
    global status credentials 
    set w [blt::tk::toplevel .login]
    blt::tk::label $w.user_l -text "User:" 
    blt::comboentry $w.user \
	-textwidth 20 \
	-hidearrow yes
    $w.user insert 0 $user
    blt::tk::label $w.pass_l -text "Password:" 
    blt::comboentry $w.pass \
	-textwidth 20 \
	-textvariable ::password \
	-show \u25CF \
	-hidearrow yes
    blt::tk::checkbutton $w.hidechars -text "Show Characters" \
	-variable hidechars -command SetChars
    blt::tk::button $w.cancel -text "Cancel" -command Cancel
    blt::tk::button $w.ok -text "Ok" -command CollectCredentials
    blt::tk::label $w.status -text $status

    set credentials ""
    blt::table $w \
	0,0 $w.user_l -anchor e  \
	0,1 $w.user -fill x  \
	1,0 $w.pass_l -anchor e  \
	1,1 $w.pass -fill x  \
	2,1 $w.hidechars -anchor w \
	3,0 $w.status -fill x -cspan 2 \
	4,0 $w.cancel \
	4,1 $w.ok
    blt::table configure $w c0 -resize none
    blt::table configure $w c1 -resize both
    update
    tkwait variable credentials
    destroy $w
    return $credentials
}

set sftp [blt::sftp create -host nees.org -prompt GetPassword -numtries 10]


set tree [blt::tree create]
puts stderr "before tree"
puts stderr time=[time {$sftp dirtree ~/indeed $tree }]
puts stderr "after tree"

puts stderr numentries=[$tree size 0]
$tree sort 0 -recurse -reorder
set view .ss.view

blt::scrollset .ss \
    -window $view \
    -xscrollbar .ss.x \
    -yscrollbar .ss.y 
blt::tk::scrollbar .ss.x
blt::tk::scrollbar .ss.y

blt::treeview $view -tree $tree \
	-height 3i

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
