
package require BLT

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set src [image create picture -file images/blt98.gif]
set bg white
set dst [image create picture -width 60 -height 60]
$dst resample $src 
if { [file exists ../library] } {
    set blt_library ../library
}

set xbg [blt::background create linear -relativeto .e.m \
		  -highcolor grey0 -lowcolor grey98 -jitter 3]

option add *BltComboMenu.background $xbg

set myIcon ""
blt::comboentry .e \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -textwidth 0 \
    -width 500 \
    -menu .e.m \
    -exportselection yes \
    -xbutton yes 
	
blt::combomenu .e.m  \
    -restrictwidth min \
    -height 400 \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -checkbuttonoutlinecolor grey80 \
    -yscrollbar .e.m.ybar \
    -xscrollbar .e.m.xbar
 
blt::tk::scrollbar .e.m.xbar
blt::tk::scrollbar .e.m.ybar

.e.m add -text "Text" -type command
.e.m add -text "Image Fail" -type command -image $dst
.e.m add -text "CheckButton #1" -type checkbutton -variable "var1"
.e.m add -text "CheckButton #2" -type checkbutton -variable "var2"
label .l -image $src
blt::table . \
    0,0 .e -fill x -anchor n  \
    1,0 .l -fill both

after 10000 {
    set var1 1
}
