package require BLT

set img [image create picture -file images/blt98.gif]
set brush [blt::paintbrush create tile -image $img -opacity 50]

set s1 [image create picture -width 25 -height 25]
$s1 blank 0x0
$s1 draw circle 12 12 5 -shadow 0 -linewidth 1 \
	-color $brush -antialias yes 

set bg [image create picture -width 600 -height 600]
$bg blank white


set n 500
puts stderr [time {
    blt::vector create points -size $n
    points random
    points expr { round(points * 512) + 30 }
}]

puts stderr [time {
    foreach {x y} [points values] {
	#puts stderr "x=$x y=$y"
	#$bg copy $s1 -to "$x $y" -blend yes
	$bg draw circle [expr int($x)] [expr int($y)] 20 \
	    -shadow 0 -linewidth 0 \
	    -color $brush -antialias no
    }
}]
label .l -image $bg
pack .l

