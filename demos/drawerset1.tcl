
package require BLT
source scripts/demo.tcl

blt::graph .g -bg \#CCCCFF -height 800 -width 801
set w [blt::drawerset .g -handlethickness 3 -animate yes]
blt::barchart .g.b -bg \#FFCCCC -height 1600 -width 300
blt::barchart .g.b2 -bg \#CCFFCC -height 300 -width 300
blt::barchart .g.b3 -bg \#FFFFCC -height 300 -width 300
blt::barchart .g.b4 -bg \#CCFFFF -height 300 -width 300
$w add top -window .g.b -fill no \
    -side top -variable top -handlecolor \#FFCCCC -showhandle no
$w add left -window .g.b2 -side left -variable left -handlecolor \#CCFFCC \
    -showhandle yes
$w add right -window .g.b3 -side right -variable right -handlecolor \#FFFFCC
$w add bottom -window .g.b4 -side bottom -variable bottom \
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
    0,0 .g -fill both -rspan 5 \
    0,1 .left \
    1,1 .right \
    2,1 .top \
    3,1 .bottom

blt::table configure . r* -resize none
blt::table configure . r4 -resize both

puts stderr [info commands .g.*]

$w open all
update
puts stderr drawer=$w
puts stderr "left is [$w isopen left] left=$left"
after 2000 {
    $w raise top
}
