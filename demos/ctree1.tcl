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

blt::comboentry .e \
    -font { arial 9 } \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -textwidth 20 \
    -menu .e.m \
    -menuanchor se \
    -exportselection yes \
    -command {puts "button pressed: [blt::grab current]"}

blt::combotree .e.m \
    -tree $tree \
    -borderwidth 1 \
    -font { arial 10 } \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -separator / \
    -height -200 \
    -linecolor grey50 \
    -yscrollbar .e.m.ybar \
    -xscrollbar .e.m.xbar

blt::tk::scrollbar .e.m.xbar
blt::tk::scrollbar .e.m.ybar

focus .e.m

blt::table . \
    .e -fill x 
