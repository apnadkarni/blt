
package require BLT

blt::paneset .vps -orient vertical 
blt::paneset .vps.hps  

option add *Divisions 4
blt::graph .vps.g -height 200 -width 250
blt::barchart .vps.hps.b  -height 200 -width 250 
blt::barchart .vps.hps.b2 -height 200 -width 250 

.vps add -window .vps.g -fill both  
.vps add -window .vps.hps -fill both 
.vps.hps add -window .vps.hps.b -fill both 
.vps.hps add -window .vps.hps.b2 -fill both

focus .vps
pack .vps -fill both -expand yes
