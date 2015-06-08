#!../src/bltwish

package require BLT

set file images/qv100.t.gif
set src [image create picture -file $file]  
option add *BltTkLabel.background white
. configure -bg white
set count 0
foreach r { -45 0 45 90 135 180 225 270 315 360 -315 } {
    set dest [image create picture -rotate $r -image $src]
    blt::tk::label .footer${count} -text "$r \u00B0"
    blt::tk::label .l${count} -image $dest
    blt::table . \
	0,${count} .l${count} \
	1,${count} .footer${count}
    incr count
}


