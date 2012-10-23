package require BLT
source scripts/demo.tcl

blt::paneset .ps  -bg grey -width 800 \
    -sashthickness 3 -background red \
    -sashborderwidth 1 -sashrelief sunken \
    -sashpad 1 

blt::graph .ps.g -bg \#CCCCFF ;#-width 300
blt::barchart .ps.b -bg \#FFCCCC ;# -width 300
blt::barchart .ps.b2 -bg \#CCFFCC ;#-width 300

.ps add -window .ps.g -fill both
.ps add -window .ps.b -fill both 
.ps add -window .ps.b2 -fill both

focus .ps

blt::table . \
    0,0 .ps -fill both

blt::table configure . r1 -resize none
