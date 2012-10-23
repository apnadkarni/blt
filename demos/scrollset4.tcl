#!../src/bltwish

package require BLT

image create picture label1 -file ./images/mini-book1.gif
image create picture label2 -file ./images/mini-book2.gif
image create picture testImage -file ./images/txtrflag.gif

blt::scrollset .ss \
    -xviewcommand { .ss.t view } \
    -xscrollbar .ss.xsbar \
    -yscrollbar .ss.ysbar \
    -window .ss.t 

blt::tk::scrollbar .ss.ysbar -orient vertical 
blt::tk::scrollbar .ss.xsbar -orient horizontal 

blt::tabset .ss.t \
    -font { Arial 8 } \
    -side right \
    -tabwidth same \
    -scrollcommand { .ss set x } \
    -scrollincrement 1 

blt::table . \
    .ss 0,0 -fill both 

focus .ss.t

set attributes {
    graph1 "Graph \#1" pink	
    graph2 "Graph \#2" lightblue	
    graph3 "Graph \#3" orange
    graph5 "Graph \#5" yellow	
    barchart2 "Barchart \#2" green
}

foreach { name label color } $attributes {
    .ss.t insert end $name -text $label 
}

blt::tk::label .ss.t.l -image testImage
.ss.t insert end Image -window .ss.t.l

.ss.t focus 0

foreach file { graph1 graph2 graph3 graph5 barchart2 } {
    namespace eval $file {
	if { [string match graph* $file] } {
	    set graph [blt::graph .ss.t.$file]
	} else {
	    set graph [blt::barchart .ss.t.$file]
	}
	source scripts/$file.tcl
	.ss.t tab configure $file -window $graph -fill both 
    }
}
