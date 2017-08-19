
package require Tk
package require BLT

set file ../images/blt98.gif
set file test.jpg
set orig [image create picture -file $file]
#$orig crop 0 0 10000 1
set sw [image width $orig]
set sh [image height $orig]
set dw [expr $sw / 4]
set dh [expr $sh / 4]

set dest1 [image create picture -width $dw -height $dh]
set dest2 [image create picture -width $dw -height $dh]
set diff1 [image create picture -width $dw -height $dh]
set diff2 [image create picture -width $dw -height $dh]

blt::tk::label .orig -image $orig
blt::tk::label .old -image $dest1
blt::tk::label .new -image $dest2
blt::tk::label .diff1 -image $diff1
blt::tk::label .diff2 -image $diff2

puts stderr [time {$dest1 resample $orig -filter box}]
puts stderr [time {$dest2 zresample $orig -filter box}]
$diff1 copy $dest2
$diff1 subtract $dest1
$diff1 or 0xFF000000
$diff2 copy $dest1
$diff2 subtract $dest2
$diff2 or 0xFF000000
#$diff multiply 100

blt::table . \
    0,0 .orig \
    0,1 .old \
    0,2 .new \
    0,3 .diff1 \
    0,4 .diff2

$dest1 export pbm -file resample1.pbm -plain
$dest2 export pbm -file resample2.pbm -plain
$diff1  export pbm -file diff.pbm -plain
set out [list [catch {
    exec cmp resample1.pbm resample2.pbm
} msg] $msg]
puts stderr compare=$out

image create picture -file diff.pbm
