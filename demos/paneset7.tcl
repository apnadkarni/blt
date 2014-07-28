
package require BLT

blt::paneset .ps  -bg grey -width 900 

blt::graph    .ps.g  -bg \#CCCCFF -width 600 -height 500
blt::barchart .ps.b1 -bg \#FFCCCC -width 700 -height 500
blt::barchart .ps.b2 -bg \#CCFFCC -width 500 -height 500

.ps add -window .ps.g -fill both 
.ps add -window .ps.b1 ;#-fill both 
.ps add -window .ps.b2 -fill both 

blt::table . \
    0,0 .ps -fill both \

blt::table configure . r1 -resize none

focus .ps
