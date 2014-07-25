
package require BLT
source scripts/demo.tcl

blt::paneset .ps  -bg grey -width 900 

blt::graph    .ps.g  -bg \#CCCCFF -width 600 -height 800
blt::barchart .ps.b1 -bg \#FFCCCC -width 700 -height 800
blt::barchart .ps.b2 -bg \#CCFFCC -width 500 -height 800
blt::tk::scrollbar .s -command { .ps view } -orient horizontal

.ps add -window .ps.g -fill both 
.ps add -window .ps.b1 ;#-fill both 
.ps add -window .ps.b2 -fill both 

blt::table . \
    0,0 .ps -fill both \
    1,0 .s -fill x 

blt::table configure . r1 -resize none

focus .ps
bind .ps.g  <ButtonPress-1>  ".ps see pane0"
bind .ps.b1 <ButtonPress-1>  ".ps see pane1"
bind .ps.b2 <ButtonPress-1>  ".ps see pane2"
