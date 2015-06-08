package require BLT

set i1 [image create picture -file ../images/base.png]
set w [image width  $i1]
set h [image height $i1]
set i2 [image create picture -file ../images/blend.png]
set i3 [image create picture -width $w -height $h]

proc fade { value } {
    global i3 i1 i2 transition
    $i3 $transition $i1 $i2 -steps 100 -goto $value -scale linear
}

set m .b.menu
blt::combobutton .b -arrowon no -textvariable transition \
    -menu $m
blt::combomenu .b.menu -textvariable transition 
$m add -text "crossfade"
$m add -text "dissolve"
$m add -text "wipe"
$m select "crossfade"

blt::tk::label .l1 -image $i1
blt::tk::label .l2 -image $i2
blt::tk::label .l3 -image $i3
scale .s -from 0 -to 100  -orient horizontal -command fade

blt::table . \
    0,0 .b \
    1,0 .l1 \
    1,1 .l2 \
    1,2 .l3 \
    2,0 .s -cspan 3 -fill x 
.s set 50

