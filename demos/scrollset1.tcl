
package require BLT
source scripts/demo.tcl

blt::scrollset .ss \
    -xscrollbar .ss.xsbar \
    -yscrollbar .ss.ysbar \
    -window .ss.g 

blt::tk::scrollbar .ss.ysbar -orient vertical 
blt::tk::scrollbar .ss.xsbar -orient horizontal 
blt::graph .ss.g 

pack .ss -fill both -expand yes
