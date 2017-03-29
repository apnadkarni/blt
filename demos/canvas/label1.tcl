
package require BLT

canvas .c -bg white

.c create rectangle 100 100 300 200 -fill lightblue3

set id [.c create label 100 100 -text "Hello, World" -bg lightgreen \
	    -activebg red3 -activefg white -linewidth 2 \
	    -anchor nw \
	    -textanchor w \
	   -width 50]

blt::table . \
    0,0 .c -fill both

.c bind $id <Enter> [list .c itemconfigure $id -state active]
.c bind $id <Leave> [list .c itemconfigure $id -state normal]
