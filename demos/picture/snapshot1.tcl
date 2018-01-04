
package require BLT

set img [image create picture]

$img snap root -from {0 0 600 600} 

label .l -image $img
blt::table . \
    0,0 .l -fill both
