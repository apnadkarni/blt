package require BLT

blt::paneset .ps -height 500 \
    -orient vertical

option add *Divisions 4
blt::graph .ps.g 
blt::barchart .ps.b 
blt::barchart .ps.b2 

.ps add -window .ps.g -fill both 
.ps add -window .ps.b -fill both 
.ps add -window .ps.b2 -fill both 

focus .ps

blt::table . \
    0,0 .ps -fill both 

blt::table configure . r1 -resize none
