
package require BLT

blt::paneset .ps -width 800 -mode spreadsheet 

option add *Divisions 4
blt::graph .ps.g -height 300 
blt::barchart .ps.b -height 300
blt::barchart .ps.b2 -height 300 

.ps add -window .ps.g -fill both 
.ps add -window .ps.b -fill both 
.ps add -window .ps.b2 -fill both 

focus .ps

blt::table . \
    0,0 .ps -fill both 

blt::table configure . r1 -resize none
