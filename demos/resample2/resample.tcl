
package require Tk
package require BLT

set file ../images/blt98.gif
set orig [image create picture -file $file]
set sw [image width $orig]
set sh [image height $orig]
set dw [expr $sw * 1]
set dh [expr $sh * 1]

set dest1 [image create picture -width $dw -height $dh]
set dest2 [image create picture -width $dw -height $dh]
set diff [image create picture -width $dw -height $dh]

blt::tk::label .orig -image $orig
blt::tk::label .old -image $dest1
blt::tk::label .new -image $dest2
blt::tk::label .diff -image $diff

$dest1 resample $orig -filter box
$dest2 zresample $orig -filter box
$diff copy $dest2
$diff subtract $dest1
$diff or 0xFF000000
#$diff multiply 100

blt::table . \
    0,0 .orig \
    0,1 .old \
    0,2 .new \
    0,3 .diff

$dest1 export pbm -file resample1.pbm -plain
$dest2 export pbm -file resample2.pbm -plain
$diff  export pbm -file diff.pbm -plain
$diff  export png -file diff.png 
set out [list [catch {
    exec cmp resample1.pbm resample2.pbm
} msg] $msg]
puts stderr compare=$out

image create picture -file diff.pbm
