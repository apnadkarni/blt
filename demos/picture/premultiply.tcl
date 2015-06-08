#!../src/bltwish

package require BLT

set i1 [image create picture -file images/spinner03.png]
set w [image width $i1]
set h [image height $i1]
set i2 [image create picture -width $w -height $h]
$i2 blank white
$i2 composite $i2 $i1
label .l1 -image $i2
pack .l1
