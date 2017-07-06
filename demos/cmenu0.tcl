
package require BLT

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set src [image create picture -file images/blt98.gif]
set bg white

if { [file exists ../library] } {
    set blt_library ../library
}

set myIcon ""
blt::comboentry .e \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -arrowrelief flat \
    -textwidth 0 \
    -menu .e.m \
    -exportselection yes \
    -clearbutton yes 
	
blt::combomenu .e.m  \
    -restrictwidth min \
    -height 200 \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -yscrollbar .e.m.ybar \
    -xscrollbar .e.m.xbar
blt::tk::scrollbar .e.m.xbar 
blt::tk::scrollbar .e.m.ybar

.e.m add -text "Image Fail" -type command -image $src
label .l -image $src
blt::table . \
    0,0 .e -fill x -anchor n  \
    1,0 .l -fill both


