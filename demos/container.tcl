#!../src/bltwish

package require BLT
namespace import blt::*

#set cmd "xterm"
set cmd "xclock"
set cmd "xterm"
blt::container .c -width 800 -height 400 
eval bgexec myVar $cmd &
pack .c -fill both -expand yes
.c configure -relief raised -bd 2 
button .b -text "Quit" -command exit
pack .b
update
#.c configure -relief raised -bd 2 -name "Mozilla Firefox"
#.c configure -relief raised -bd 2 -command $cmd


