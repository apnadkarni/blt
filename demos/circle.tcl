package require BLT

set img [image create picture -width 100 -height 100]
$img blank white

$img draw circle 50 50 30 -color cyan2

blt::tk::label .l -image $img
blt::table . \
    0,0 .l -fill both


