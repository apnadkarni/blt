package require BLT
set file images/blt98.gif
#set file images/spinner08.png
set img [image create picture -file $file -rotate 90]
$img greyscale $img
label .l -image $img -bg white 
pack .l
$img export pdf -file test.pdf 
#-comments "Author {This Program}"
