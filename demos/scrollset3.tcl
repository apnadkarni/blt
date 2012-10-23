#!../src/bltwish

package require BLT
source scripts/demo.tcl

proc FormatSize { size } {
   set string ""
   while { $size > 0 } {
       set rem [expr $size % 1000]
       set size [expr $size / 1000]
       if { $size > 0 } {
           set rem [format "%03d" $rem]
       } 
       if { $string != "" } {
           set string "$rem,$string"
       } else {
           set string "$rem"
       }
   } 
   return $string
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

proc FormatMode { mode } {
   global modes

   set mode [format %o [expr $mode & 07777]]
   set owner $modes([string index $mode 0])
   set group $modes([string index $mode 1])
   set world $modes([string index $mode 2])

   return "${owner}${group}${world}"
}

proc Find { tree parent dir } {
    global count 
    set saved [pwd]

    cd $dir
    foreach f [glob -nocomplain *] {
	set name [file tail $f]
	if { [catch { file stat $f info }] != 0 } {
	    set node [$tree insert $parent -label $name]
	} else {
	    if 0 {
	    if { $info(type) == "file" } {
		set info(type) [list @::blt::TreeView::openIcon $name]
	    } else {
		set info(type) "@::blt::TreeView::openIcon "
	    }
	    }
	    set info(mtime) [clock format $info(mtime) -format "%b %d, %Y"]
	    set info(atime) [clock format $info(atime) -format "%b %d, %Y"]
	    set info(ctime) [clock format $info(ctime) -format "%b %d, %Y"]
            set info(size)  [FormatSize $info(size)]
	    set info(mode)  [FormatMode $info(mode)]
	    set node [$tree insert $parent -label $name -data [array get info]]
	}
	incr count
	if { [file type $f] == "directory" } {
	    Find $tree $node $f
	}
    }
    cd $saved
}


proc GetAbsolutePath { dir } {
    set saved [pwd]
    cd $dir
    set path [pwd] 
    cd $saved
    return $path
}

option add *TreeView.focusOutSelectForeground white
option add *TreeView.focusOutSelectBackground grey80
#option add *TreeView.Button.activeBackground pink
option add *TreeView.Button.activeBackground grey90
#option add *TreeView.Button.background grey95
option add *TreeView.Column.background grey90
option add *TreeView.CheckBoxStyle.activeBackground white
if 0 {
if 1 {
    option add *TreeView.Column.titleFont { Arial 10 }
    option add *TreeView.text.font { Monotype.com 10 Bold } 
    option add *TreeView.CheckBoxStyle.font Courier-10
    option add *TreeView.ComboBoxStyle.font Helvetica-10
    option add *TreeView.TextBoxStyle.font {Arial 10 bold }
} else {
    option add *TreeView.Column.titleFont { Arial 14 }
}
}
button .b -font { Helvetica 11 bold }
set top [GetAbsolutePath ..]
#set top [GetAbsolutePath /home/gah]
set trim "$top"

set tree [blt::tree create]    

blt::scrollset .ss \
    -xscrollbar .ss.hs \
    -yscrollbar .ss.vs \
    -window .ss.t 

blt::tk::scrollbar .ss.vs -orient vertical 
blt::tk::scrollbar .ss.hs -orient horizontal

blt::treeview .ss.t \
    -width 0 \
    -height 0 \
    -highlightthickness 0 \
    -borderwidth 0 \
    -selectmode multiple \
    -separator / \
    -tree $tree 

.ss.t column configure treeView -text ""  -edit yes
#file
foreach col { mtime atime gid } {
    .ss.t column insert 0 $col
}
foreach col { nlink mode type ctime uid ino size dev } {
    .ss.t column insert end  $col
}
.ss.t column configure uid -relief raised .ss.t column configure mtime -hide no -relief raised
foreach col { size gid nlink uid ino dev type } {
    .ss.t column configure $col -justify left -edit yes
}
.ss.t column configure treeView -hide no -edit yes \
	-icon ::blt::TreeView::openIcon
focus .ss.t


blt::table . \
    0,0 .ss  -fill both 

set count 0
Find $tree root $top
puts "$count entries"

$tree find root -glob *.c -addtag "c_files"
$tree find root -glob *.h -addtag "header_files"
$tree find root -glob *.tcl -addtag "tcl_files"

.ss.t entry configure "c_files" -foreground green4
.ss.t entry configure "header_files" -foreground cyan4
.ss.t entry configure "tcl_files" -foreground red4 

.ss.t column bind all <ButtonRelease-3> {
    %W configure -flat no
}

#.ss.t style configure text -background #F8fbF8 -selectbackground #D8fbD8 

.ss.t style checkbox check \
    -onvalue 100 -offvalue "50" \
    -showvalue yes 

.ss.t style combobox combo -icon ::blt::TreeView::openIcon

.ss.t column configure uid -style combo 
.ss.t column configure gid -style check


