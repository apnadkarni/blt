#!../src/bltwish

package require BLT

set shadow 8
set xoff 5
set yoff 5
set width 250
set height 250
set r 200
set cx [expr $width / 2]
set cy [expr $height / 2]
set x [expr $cx + $xoff]
set y [expr $cy + $yoff]

set l1 [image create picture -width $width -height $height]
set l2 [image create picture -width $width -height $height]
set bg [image create picture \
	    -file images/blt98.gif \
	    -width 200 \
	    -height 200 \
	    -maxpect yes]
#$bg and 0x00FFFFFF
$l1 blank 0x00000000
set w [image width $bg]
set h [image height $bg]
$l1 draw rectangle 8 8 $w $h -color 0xFFFFFFFF -linewidth 0  
$l1 blur $l1 8
$l2 select $l1 0x01000000 0xFFFFFFFF
$l1 and 0xFF000000 -mask $l2
$l1 or $bg -mask $l2
#$l1 sub 0x0F000000 -mask $l2
set bg [image create picture -width $width -height $height]
set brush [blt::paintbrush create gradient \
		-high yellow  \
		-low black  \
		-jitter 2 \
		-scale log]
$bg draw rectangle 0 0 $width $height -color $brush
$bg blend $bg $l1 

blt::tk::label .l0 -image $l1
blt::tk::label .l1 -image $l2
blt::tk::label .l3 -image $bg

puts stderr [$l1 info]
puts stderr [$l2 info]
puts stderr [$bg info]
blt::table . \
    1,1 .l0  \
    1,2 .l1 \
    1,3 .l3 
