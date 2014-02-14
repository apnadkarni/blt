
package require BLT

if 0 {
set src [image create picture -height 30 -width 100]
$src blank 0x0
$src draw text "Hi there" 50 15 -anchor c -font "Arial 12 bold" -color white
}
set src [image create picture -file ~/Dummy_user.png]
label .l1 -image $src
set dst [image create picture]
$dst emboss $src 45 55
label .l2 -image $dst -bg white
pack .l1  .l2



