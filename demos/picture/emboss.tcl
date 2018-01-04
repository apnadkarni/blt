
package require BLT

set src [image create picture -height 30 -width 100]
$src blank 0x00FFFFFF
$src draw text "Emboss" 50 15 \
    -anchor c \
    -font "Times 14 bold" 
set dst [image create picture]
$dst emboss $src 45 55

label .l1 -image $src
label .l2 -image $dst 
blt::table . \
    0,0 .l1 \
    0,1 .l2 



