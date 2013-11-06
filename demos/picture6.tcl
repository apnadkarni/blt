package require BLT

set img [image create picture -width 200 -height 200]
$img blank grey
$img draw circle  100 100 15 -color 0xA0000000
$img blur $img 1
$img draw circle 99 99 15 -color white
$img draw circle 99 99 15 -color 0xA0000000
$img draw circle 99 99 15 -color white -linewidth 3.0
$img draw text 2 99 99 -anchor c -color white -font "Arial 12 bold"
label .l -image $img -bg white
pack .l -expand yes -fill both
