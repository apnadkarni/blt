
package require BLT
source scripts/demo.tcl

blt::drawerset .ds \
    -handlethickness 3 \
    -animate yes \
    -window .ds.g

blt::graph .ds.g -bg \#CCCCFF -height 800 -width 801
blt::barchart .ds.b1 -bg \#FFCCCC -height 1600 -width 300
blt::barchart .ds.b2 -bg \#CCFFCC -height 300 -width 300
blt::barchart .ds.b3 -bg \#FFFFCC -height 300 -width 300
blt::barchart .ds.b4 -bg \#CCFFFF -height 300 -width 300
.ds add top -window .ds.b1 -fill no \
    -side top -variable top -handlecolor \#FFCCCC -showhandle no
.ds add left -window .ds.b2 -side left -variable left -handlecolor \#CCFFCC \
    -showhandle yes
.ds add right -window .ds.b3 -side right -variable right -handlecolor \#FFFFCC
.ds add bottom -window .ds.b4 -side bottom -variable bottom \
    -handlecolor \#CCFFFF  -showhandle yes
checkbutton .left -text "L" -overrelief raised  \
    -variable left -indicatoron no
checkbutton .right -text "R" -overrelief raised  \
    -variable right -indicatoron no
checkbutton .top -text "T" -overrelief raised  \
    -variable top -indicatoron no
checkbutton .bottom -text "B" -overrelief raised  \
    -variable bottom -indicatoron no

blt::table . \
    0,0 .ds -fill both -rspan 5 \
    0,1 .left \
    1,1 .right \
    2,1 .top \
    3,1 .bottom

blt::table configure . r* -resize none
blt::table configure . r4 -resize both

.ds open all
update
puts stderr "left is [.ds isopen left] left=$left"
after 2000 {
    .ds raise top
}
focus .ds
