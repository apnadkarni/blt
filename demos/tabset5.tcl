
package require BLT

blt::tabset .ts \
    -notabs 1
    
blt::graph .ts.g1 -bg blue
blt::graph .ts.g2 -bg green
blt::barchart .ts.g3 -bg red

.ts add a -window .ts.g1 -fill both
.ts add b -window .ts.g2 -fill both
.ts add c -window .ts.g3 -fill both

blt::tk::radiobutton .ra -text "A" -value a -variable pane \
    -command ".ts select a"
blt::tk::radiobutton .rb -text "B" -value b -variable pane \
    -command ".ts select b"
blt::tk::radiobutton .rc -text "C" -value c -variable pane \
    -command ".ts select c"

blt::table . \
    0,0 .ts  -fill both -rspan 4 \
    0,1 .ra \
    1,1 .rb \
    2,1 .rc
.ra select
blt::table configure . r* -resize none
blt::table configure . r3 -resize both
