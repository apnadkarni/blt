package require BLT
set file images/blt98.gif
set img [image create picture -file $file]
label .l -image $img
pack .l
$img export pdf -file test.pdf 
#-comments "Author {This Program}"
