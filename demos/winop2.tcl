#!../src/bltwish

package require BLT
source scripts/demo.tcl

set file images/qv100.t.gif

if { [file exists $file] } {
    set photo [image create picture -file $file]  
} else {
    puts stderr "no image file"
    exit 0
}

option add *Label.font *helvetica*10*
option add *Label.background lightsteelblue

set i 0
foreach r { -45 0 45 90 135 180 225 270 315 360 -315 } {
    set dest [image create picture -rotate $r -image $photo]
    label .footer$i -text "$r degrees"
    label .l$i -image $dest
    blt::table . \
	0,$i .l$i \
	1,$i .footer$i
    update
    incr i
}


