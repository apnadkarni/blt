package require BLT

set img [image create picture -width 200 -height 200]
$img add layer 
$img add layer $pict
$img blank white
$img draw circle  100 100 15 -color 0xA0000000
$img blur $img 1
$img draw circle 98 98 15 -color white
$img draw circle 98 98 15 -color 0xA0000000
$img draw circle 98 98 15 -color white -linewidth 2.0
$img draw text 2 98 98 -anchor c -color white
label .l -image $img -bg white
pack .l -expand yes -fill both