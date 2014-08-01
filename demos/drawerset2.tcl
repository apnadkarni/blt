
package require BLT

blt::drawerset .ds \
    -animate yes \
    -handlethickness 3 \
    -autoraise 1 \
    -window .ds.g
set first 0
set second 0
blt::graph .ds.g -bg \#CCCCFF -height 800
blt::barchart .ds.b1 -bg \#FFCCCC -height 300 -width 300
blt::barchart .ds.b2 -bg \#CCFFCC -height 300 -width 300
blt::barchart .ds.b3 -bg yellow -height 300 -width 300
.ds add \
    -side left \
    -window .ds.b1 \
    -resize none \
    -fill none \
    -anchor nw \
    -variable first
.ds add \
    -side bottom \
    -window .ds.b2 \
    -fill y \
    -anchor w \
    -variable second
.ds add \
    -side bottom \
    -window .ds.b3 \
    -fill y \
    -anchor c \
    -variable third

checkbutton .first -text "1" -overrelief raised  \
    -variable first -indicatoron no
checkbutton .second -text "2" -overrelief raised  \
    -variable second -indicatoron no
checkbutton .third -text "2" -overrelief raised  \
    -variable third -indicatoron no

blt::table . \
    0,0 .ds -fill both -rspan 4 \
    0,1 .first \
    1,1 .second \
    2,1 .third \

blt::table configure . r* -resize none
blt::table configure . r3 -resize both

