
package require BLT
source scripts/demo.tcl

blt::drawerset .ds \
    -handlethickness 3 \
    -animate yes \
    -width 800 \
    -height 700 \
    -window .ds.g

blt::graph .ds.g -bg \#CCCCFF
blt::barchart .ds.top -bg \#FFCCCC -height 1600 -width 300
blt::barchart .ds.left -bg \#CCFFCC -height 300  -width 300
blt::barchart .ds.right -bg \#FFFFCC -height 300  -width 300
blt::barchart .ds.bot -bg \#CCFFFF -height 300  -width 300
.ds add top -window .ds.top \
    -side top \
    -variable top \
    -handlecolor \#FFCCCC \
    -showhandle no \
    -fill none
.ds add left \
    -window .ds.left \
    -side left \
    -variable left \
    -handlecolor \#CCFFCC \
    -showhandle yes \
    -fill x
.ds add right \
    -window .ds.right \
    -side right \
    -variable right \
    -handlecolor \#FFFFCC \
    -fill none
.ds add bottom \
    -window .ds.bot \
    -side bottom \
    -variable bottom \
    -handlecolor \#CCFFFF \
    -showhandle yes \
    -fill y

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
after 2000 {
    .ds raise top
}
focus .ds
