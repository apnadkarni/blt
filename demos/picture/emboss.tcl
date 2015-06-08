
package require BLT

set src [image create picture -height 30 -width 100]
$src draw text "Emboss" 50 15 \
    -anchor c \
    -font "Serif 14 bold" \
    -color white
set dst [image create picture]
$dst emboss $src 45 55

label .l1 -image $src
label .l2 -image $dst 
blt::table . \
    0,0 .l1 \
    0,1 .l2 



