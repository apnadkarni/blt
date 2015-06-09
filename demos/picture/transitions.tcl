package require BLT

set i1 [image create picture -file ./images/base.png]
set w [image width  $i1]
set h [image height $i1]
set i2 [image create picture -file ./images/blend.png]
set i3 [image create picture -width $w -height $h]

proc Transition { value } {
    global i3 i1 i2 transition scale
    $i3 $transition $i1 $i2 -steps 100 -goto $value -scale $scale
    .s set $value
}
blt::tk::label .transition_l -text "Transition"
blt::tk::checkbutton .logscale \
    -text "Log scale" \
    -offvalue "linear" \
    -onvalue "log" \
    -variable scale 

scale .s -from 0 -to 100  -orient horizontal -command Transition
set m .b.menu
blt::comboentry .b -editable no -textvariable transition \
    -menu $m -width 150 
blt::combomenu .b.menu -textvariable transition \
    -command "Transition 0"
$m add -text "crossfade"
$m add -text "dissolve"
$m add -text "wipe"
$m select "crossfade"

blt::tk::label .l1 -image $i1
blt::tk::label .l2 -image $i2
blt::tk::label .l3 -image $i3

blt::table . \
    0,0 .transition_l	-anchor e \
    0,1 .b		-anchor w \
    0,2 .logscale	-anchor e \
    1,0 .l1 \
    1,1 .l2 \
    1,2 .l3 \
    2,0 .s -cspan 3 -fill x 

blt::table configure . r* c* -resize none

Transition 0

