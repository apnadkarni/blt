
package require BLT

set img [image create picture]
label .orig -image [image create picture -file images/blt98.gif]

label .copy -image $img
blt::table . \
    0,0 .orig -fill both \
    0,1 .copy -fill both
update idletasks
update
after 2000 {
$img snap .orig -from {0 0 600 600} 
}
