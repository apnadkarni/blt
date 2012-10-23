#!../src/bltwish

package require BLT
source scripts/demo.tcl

set filter sinc
set shadow 8
#set imgfile ./images/sample.gif
set imgfile ./images/blt98.gif
#set imgfile ~/.icons/cd-player.xpm
if { [llength $argv] > 0 } {
    set imgfile [lindex $argv 0]
}
if { [ file exists $imgfile] } {
    set src [image create picture -file $imgfile]  
} else {
    puts stderr "no image file"
    exit 0
}
set xoff 5
set yoff 5
set name [file root [file tail $imgfile]]
set width 500 
set height 500
set bg [image create picture -width $width -height $height]
set bg2 [image create picture -width $width -height $height]
set r 200
set cx [expr $width / 2]
set cy [expr $height / 2]
set x [expr $cx + $xoff]
set y [expr $cy + $yoff]
#$bg draw circle $x $y $r -color blue -thickness 0  
set layer1 [image create picture -width $width -height $height]
$layer1 blank 0x00000000
$layer1 draw circle $x $y $r -color 0xFFFFFFFF -linewidth 0  
$layer1 blur $layer1 10
set layer2 [image create picture -width $width -height $height]
set layer3 [image create picture -width $width -height $height]
$layer2 select $layer1 0x01000000 0xFFFFFFFF
$layer2 and 0xFF000000
$layer2 and $layer1 
$layer3 copy $layer1
$layer3 select $layer1 0x01000000 0xFFFFFFFF
#$layer3 and 0x00FFFF00
$bg2 gradient -low green2 -high green4 -jitter 10 -scale log
puts bg2=[$bg2 get 200 200]
#$bg2 and 0x00FFFFFF
puts bg2=[$bg2 get 200 200]
puts layer1=[$layer1 get 200 200]
$layer1 and $bg2 
puts and=[$layer1 get 200 200]
$bg gradient -low blue1 -high blue4 -jitter 10 -scale log 
#$layer1 or 0xFF000000
#$layer1 blend $bg $layer1 
#-matte $layer3

#$layer1 min $bg
$bg blend $bg $layer2  
#-matte $layer1

set src $layer1
set src $bg
set dest $layer1

option add *Label.font *helvetica*10*
option add *Label.background white

label .l0 -image $src
label .header0 -text "$width x $height"
label .footer0 -text "100%"
. configure -bg white
set iw $width
set ih $height
label .header -text "$iw x $ih"
label .footer -text "$filter"
label .l1 -image $dest

blt::table . \
   0,1 .header \
   1,1 .l0  1,2 .l1 \
   2,1 .footer
