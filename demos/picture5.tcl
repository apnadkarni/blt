package require BLT
set img [image create picture -file images/blt98.gif]
label .l -image $img
pack .l
$img export pdf -file blt98_test.pdf
