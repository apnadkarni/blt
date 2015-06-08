package require BLT

set img [image create picture -file ./images/blt98.gif]
set brush [blt::paintbrush create tile -image $img -opacity 70]

if 0 {
set s1 [image create picture -width 25 -height 25]
$s1 blank 0x0
$s1 draw circle 12 12 5 -shadow 0 -linewidth 1 \
	-color $brush -antialias yes 
}

set w [image width $img]
set h [image height $img]
set bg [image create picture -width $w -height $h]
$bg blank white

set n 50
puts stderr [time {
    blt::vector create points -length $n
    points random
    points expr { round(points * $h) }
}]

puts stderr [time {
    foreach {x y} [points values] {
	$bg draw circle [expr int($x)] [expr int($y)] 20 \
	    -shadow 0 -linewidth 0 \
	    -color $brush -antialias no
    }
}]
label .l -image $bg
pack .l

