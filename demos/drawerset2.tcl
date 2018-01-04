
package require BLT

blt::drawerset .ds \
    -animate yes \
    -handlethickness 3 \
    -autoraise 1 \
    -window .ds.g \
    -height 500 -width 600

set first 0
set second 0
blt::graph .ds.g -bg \#CCCCFF -height 800
blt::barchart .ds.left -bg \#FFCCCC -height 300 -width 300
blt::barchart .ds.top  -bg green2 -height 300 -width 300
blt::barchart .ds.bot1 -bg \#CCFFCC -height 300 -width 300
blt::barchart .ds.bot2 -bg yellow -height 300 -width 300
.ds add left \
    -side left \
    -window .ds.left \
    -resize no \
    -fill none \
    -anchor nw \
    -handlecolor pink3  \
    -variable first
.ds add top \
    -side top \
    -window .ds.top \
    -fill y \
    -handlecolor green3  \
    -anchor w \
    -variable second
.ds add bot1 \
    -side bottom \
    -window .ds.bot1 \
    -fill y \
    -handlecolor \#AAFFAA  \
    -anchor w \
    -variable third
.ds add bot2 \
    -side bottom \
    -window .ds.bot2 \
    -fill y \
    -handlecolor yellow2  \
    -anchor c \
    -variable fourth

blt::tk::pushbutton .first -text "L" -overrelief raised  \
    -variable first
blt::tk::pushbutton .second -text "T" -overrelief raised  \
    -variable second 
blt::tk::pushbutton .third -text "B" -overrelief raised  \
    -variable third
blt::tk::pushbutton .fourth -text "B" -overrelief raised  \
    -variable fourth

blt::table . \
    0,0 .ds -fill both -rspan 6 \
    1,1 .first \
    2,1 .second \
    3,1 .third  \
    4,1 .fourth

blt::table configure . r0 -height 0.251i
blt::table configure . c* r* -resize none
blt::table configure . c0 r5 -resize both
