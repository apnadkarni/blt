package require BLT

blt::tk::button .quit   -command exit -text "Quit"
blt::tk::button .cancel -command exit -text "Cancel"
blt::tk::button .cont   -command exit -text "Continue"

blt::combomenu .m  \
    -textvariable myText1  \
    -iconvariable myIcon1 \
    -relief raised 

set image1 [image create picture -file ~/hubs/neeshub/indeed/src/icons/sensor/hidden/moment.png]
set image2 [image create picture -file ~/hubs/neeshub/indeed/src/icons/sensor/moment.png]
set image3 [image create picture -width 20 -height 20]
$image3 copy $image1 -from "0 0 11 20"
$image3 copy $image2 -from "10 0 20 20" -to "10 0"
.m add -type checkbutton -text "Hide" -accel "Ctrl-H" \
    -command "puts hide" -underline 0 -icon $image1 -variable hide
.m add -type checkbutton -text "Show" -accel "Ctrl-S" \
    -command "puts show" -underline 0 -icon $image2 -variable hide
.m add -text "Toggle" -accel "Ctrl-T" \
    -command "puts toggle" -underline 0

proc Post { x y } {
    blt::popup .m $x $y
}

bind all <ButtonPress-3> { Post %X %Y }

blt::table . \
    0,0 .cont -pady 10 \
    0,1 .quit -pady 10 \
    0,2 .cancel -pady 10 


