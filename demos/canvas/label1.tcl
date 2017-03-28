
package require BLT

canvas .c -bg white

.c create label 200 100 -text "Hello, World" -bg blue 

blt::table . \
    0,0 .c -fill both


