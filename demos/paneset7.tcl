
package require BLT
source scripts/demo.tcl

blt::paneset .ps  -bg grey -width 900 

blt::graph .ps.g -bg \#CCCCFF -width 600 -height 800
blt::barchart .ps.b -bg \#FFCCCC -width 700 -height 800
blt::barchart .ps.b2 -bg \#CCFFCC -width 500 -height 800
blt::tk::scrollbar .s -command { .ps view } -orient horizontal

.ps add -window .ps.g -fill both 
.ps add -window .ps.b ;#-fill both 
.ps add -window .ps.b2 -fill both 

if 0 {
.ps bind Sash <Enter> { .ps sash activate current } 
.ps bind Sash <Leave> { .ps sash activate none } 

.ps bind Sash <ButtonPress-1> {
    %W sash anchor %x %y
}
.ps bind Sash <B1-Motion> {
    %W sash mark %x %y
}
.ps bind Sash <ButtonRelease-1> {
    %W sash set %x %y
}
}

blt::table . \
    0,0 .ps -fill both \
    1,0 .s -fill x 

blt::table configure . r1 -resize none

focus .ps
after 5000 {
    #.ps pane configure 1 -size { 0 10000 10 }
    focus .
    #.ps see pane2
    #.ps size pane2 1i
}

bind .ps.g <ButtonPress-1>  ".ps see pane0"
bind .ps.b <ButtonPress-1>  ".ps see pane1"
bind .ps.b2 <ButtonPress-1>  ".ps see pane2"
