package require BLT

proc find { tree parent dir } {
    global count 
    set saved [pwd]

    cd $dir
    foreach f [glob -nocomplain *] {
	set name [file tail $f]
	if { [file type $f] == "directory" } {
	    set node [$tree insert $parent -label $name]
	    find $tree $node $f
	}
    }
    cd $saved
}

set tree [blt::tree create]
set path ../..
find $tree root $path
$tree label root [file normalize $path]
if { [file exists ../library] } {
    set blt_library ../library
}
puts [$tree label 0]
#    -postcommand {.e.m configure -width [winfo width .e] ; update} \
set myIcon ""
blt::combobutton .b \
    -text "Select Directory" \
    -command "puts {button pressed}" \
    -font { arial 10 bold } \
    -menu .b.m 

blt::combotree .b.m \
    -tree $tree \
    -borderwidth 1 \
    -font { arial 10 } \
    -separator / \
    -yscrollbar .b.m.ybar \
    -xscrollbar .b.m.xbar

blt::tk::scrollbar .b.m.xbar
blt::tk::scrollbar .b.m.ybar

focus .b

blt::table . \
    .b -fill x 