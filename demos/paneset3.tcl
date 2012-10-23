
package require BLT
source scripts/demo.tcl

blt::paneset .vps -orient vertical 
blt::paneset .vps.hps  

blt::graph .vps.g -height 400 -width 500
blt::barchart .vps.hps.b  -height 400 -width 400 
blt::barchart .vps.hps.b2 -height 400 -width 400 

.vps add -window .vps.g -fill both  
.vps add -window .vps.hps -fill both 
.vps.hps add -window .vps.hps.b -fill both 
.vps.hps add -window .vps.hps.b2 -fill both

focus .vps
pack .vps -fill both -expand yes
