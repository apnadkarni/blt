package require BLT

set file ./images/blt98.gif
set img [image create picture -file $file]
$img greyscale $img
label .l -image $img -bg white 
pack .l
