#!../src/bltwish

set count 0
package require BLT

set spinner [image create picture]
set files [glob -nocomplain ./images/spinner*.png]
set i 0
$spinner sequence length [llength $files] 
foreach file [lsort -dictionary $files] {
    set img [image create picture -file $file]
    $spinner sequence put $i $img
    image delete $img 
    incr i
}

proc FormatType { w id value } {
    if { $value != "directory" } {
       $w entry configure $id -button no
    }
    return $value
}


proc FormatSize { id value } {
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

proc FormatDate { id value } {
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

proc FormatPerms { id mode } {
    global modes
    set mode [format %o [expr $mode & 07777]]
    set owner $modes([string index $mode 0])
    set group $modes([string index $mode 1])
    set world $modes([string index $mode 2])
    return "${owner}${group}${world}"
}

proc Find { tree parent dir } {
    $tree dir $parent $dir 
    foreach node [$tree find $parent -key type -exact "directory"] {
	.ss.t entry configure $node -button yes 
    }
}

set top [file normalize ".."]
set trim "$top"

set tree [blt::tree create]    

proc OpenNode  { tree node parent top } {
    if { [file type $top/$parent] == "directory" } {
	global spinner 
	blt::busy hold .ss.t -opacity 50 
	update
	Find $tree $node $top/$parent
	update
	blt::busy release .ss.t
    }
}

proc CloseNode  { tree node } {
    eval $tree delete [$tree children $node]
}

blt::scrollset .ss \
    -window .ss.t \
    -xscrollbar .ss.xs \
    -yscrollbar .ss.ys 
blt::tk::scrollbar .ss.xs
blt::tk::scrollbar .ss.ys

blt::treeview .ss.t \
    -width 0 \
    -height 5i \
    -selectmode multiple \
    -separator / \
    -tree $tree \
    -opencommand [list OpenNode $tree %\# %P $top]  \
    -closecommand [list CloseNode $tree %\#]

blt::table . \
    0,0 .ss  -fill both 

set img [image create picture -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM1WBrM+rAEMigJ8c3Kb3OSII6kGABhp1JnaK1VGwjwKwtvHqNzzd263M3H4n2OH1QBwGw6
    nQkAOw==
}]
	 
.ss.t style textbox mode -font "{courier new} 9"
.ss.t column configure treeView -text "name"  -sorttype dictionary
.ss.t column insert end mtime -formatcommand FormatDate -justify right \
    -sorttype integer
.ss.t column insert end perms -formatcommand FormatPerms -justify right \
    -style mode -sorttype integer
.ss.t column insert end type -formatcommand [list FormatType .ss.t] \
    -justify center -sorttype ascii
.ss.t column insert end size -formatcommand FormatSize -justify right \
    -sorttype integer
focus .ss.t

puts "$count entries"

.ss.t column bind all <ButtonRelease-3> {
    %W configure -flat yes
}

.ss.t style checkbox check \
    -onvalue 30 -offvalue "50" \
    -showvalue yes 

.ss.t style combobox combo -icon $img

#.ss.t column configure owner -style combo 
#.ss.t column configure group -style check
if 0 {
.ss.t sort configure -column type 
}
#.ss.t sort auto yes 

.ss.t entry configure 0 -font "Arial -12 italic"

