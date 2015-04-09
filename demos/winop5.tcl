#!../src/bltwish

package require BLT

set filter sinc
set shadow 8

set xoff 5
set yoff 5
set width 500 
set height 500

set bg  [image create picture -width $width -height $height]
set bg2 [image create picture -width $width -height $height]
set r 200
set cx [expr $width / 2]
set cy [expr $height / 2]
set x  [expr $cx + $xoff]
set y  [expr $cy + $yoff]
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
set brush1 [blt::paintbrush create linear \
		-from n -to s \
		-highcolor green4  \
		-lowcolor green2  \
		-jitter 10 \
		-colorscale log]
$bg2 draw rectangle 0 0 $width $height -color $brush1 
$layer1 and $bg2 
set brush2 [blt::paintbrush create linear \
		-from n -to s \
		-highcolor blue4  \
		-lowcolor blue1  \
		-jitter 10 \
		-colorscale log]
$bg draw rectangle 0 0 $width $height -color $brush2
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
