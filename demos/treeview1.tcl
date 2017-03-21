#!../src/bltwish

package require BLT

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

proc FormatMode { id mode } {
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
   set mode [format %o [expr $mode & 07777]]
   set owner $modes([string index $mode 0])
   set group $modes([string index $mode 1])
   set world $modes([string index $mode 2])

   return "${owner}${group}${world}"
}

proc Find { tree parent dir } {
    global count 

    $tree dir $parent $dir -fields "atime ctime gid uid type size mtime mode"
    incr count [$tree degree $parent]
    foreach node [$tree children $parent] {
	set name [$tree label $node]
	if { [$tree get $node "type"] == "directory" } {
	    Find $tree $node [file join $dir $name]
	}
    }
}

proc GetAbsolutePath { dir } {
    set saved [pwd]
    cd $dir
    set path [pwd] 
    cd $saved
    return $path
}

button .b -font { Helvetica 11 bold }
#set top [GetAbsolutePath ..]
set top [GetAbsolutePath "$env(HOME)"]
set top [GetAbsolutePath ".."]
set trim "$top"

set tree [blt::tree create]    

set view .ss.t 

blt::scrollset .ss \
    -window $view \
    -xscrollbar .ss.x \
    -yscrollbar .ss.y 
blt::tk::scrollbar .ss.x
blt::tk::scrollbar .ss.y

option add *BltTreeView.Entry.RuleHeight 1
option add *BltTreeView.Column.ruleWidth 1

blt::treeview $view \
    -width 0 \
    -height 4i \
    -selectmode multiple \
    -separator / \
    -tree $tree  -font "Arial 10"

$view column configure treeView -text "name" -edit yes \
    -sorttype dictionary 
$view column insert 0 mtime -sorttype integer -formatcommand FormatDate 
$view column insert 0 atime -sorttype integer -formatcommand FormatDate
$view column insert 0 gid -sorttype integer
$view column insert end mode -sorttype integer -formatcommand FormatMode 
$view column insert end type -sorttype dictionary
$view column insert end ctime -sorttype integer -formatcommand FormatDate
$view column insert end uid -sorttype integer 
$view column insert end size -sorttype integer -formatcommand FormatSize \
    -justify right

$view sort configure -columns treeView 
focus $view

foreach c [$view column names] {
    $view column configure $c \
	-titleborderwidth 1 -borderwidth 1 -relief sunken -titlefont "Arial 10"
}
blt::table . \
    0,0 .ss  -fill both 

set count 0
Find $tree root $top
puts "$count entries"

$view style checkbox check \
    -onvalue "file" -offvalue "directory" \
    -showvalue yes

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

$view column configure uid -style combo 
$view column configure type -style check

$view style create textbox textbox \
    -editor $view.editor -edit yes -fg red
blt::comboeditor $view.editor  \
    -height { 0 2i }  \
    -yscrollbar $view.editor.ybar \
    -xscrollbar $view.editor.xbar
blt::tk::scrollbar $view.editor.xbar 
blt::tk::scrollbar $view.editor.ybar
$view column configure ctime -style textbox 
$view column configure treeView -style textbox 
#$view configure -icons ""

wm protocol . WM_DELETE_WINDOW { destroy . }

bind $view <Enter> { focus %W }
