
package require BLT

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set icon2 [image create picture -file images/blt98.gif]
set icon [image create picture -data $imgData]
set bg white

set image ""
option add *ComboEntry.takeFocus 1

if { [file exists ../library] } {
    set blt_library ../library
}

#    -postcommand {.e.m configure -width [winfo width .e] ; update} \
set myIcon ""
blt::comboentry .e \
    -image $image \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -arrowrelief flat \
    -textwidth 0 \
    -menu .e.m \
    -exportselection yes \
    -clearbutton yes 
	
#    -bg $bg 

blt::comboframe .e.m  \
    -restrictwidth min \
    -height 200 \
    -window .e.m.g

blt::graph .e.m.g

blt::table . \
    0,0 .e -fill x -anchor n 

