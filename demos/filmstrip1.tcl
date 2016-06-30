
package require BLT
source scripts/demo.tcl

set pictures [glob -nocomplain "*.jpg"]
set autocolors {
#0000cd
#cd0000
#00cd00
#3a5fcd
#cdcd00
#cd1076
#009acd
#00c5cd
#a2b5cd
#7ac5cd
#66cdaa
#a2cd5a
#cd9b9b
#cdba96
#cd3333
#cd6600
#cd8c95
#cd00cd
#9a32cd
#6ca6cd
#9ac0cd
#9bcd9b
#00cd66
#cdc673
#cdad00
#cd5555
#cd853f
#cd7054
#cd5b45
#cd6889
#cd69c9
#551a8b
}

#blt::debug 100

proc Move { w frame } {
    .ss.fs see $frame
}

proc MoveTo { w x y } {

    set index [$w index @$x,$y]
    if { $index == -1 } {
	return
    }
    foreach { x1 y1 x2 y2 } [$w bbox $index] break
    if { (double($x - $x1) / ($x2 - $x1)) > 0.5 } {
	$w see next
    } else {
	$w see prev
    }
}

blt::scrollset .ss \
    -xviewcommand { .ss.fs view } \
    -xscrollbar .ss.xs \
    -window .ss.fs 

if 0 {
    blt::filmstrip .ss.fs -width 560 \
    -scrolldelay 40 -scrollincrement 30 -animate yes \
    -scrollcommand { .ss xset } -bg grey85 \
    -relwidth 1.0 \

    }

blt::filmstrip .ss.fs -width 560 -animate yes \
    -scrollcommand { .ss xset } \

blt::tk::scrollbar .ss.xs -orient horizontal -command { .ss xview }

bind .ss.fs <ButtonPress-1>  [list MoveTo %W %x %y]

for { set i 0 } { $i < 32 } { incr i } {
    set color [lindex $autocolors $i]
    set g .ss.fs.g$i
    blt::graph $g -bg $color -width 300
    .ss.fs add \
	-window $g \
	-showgrip yes \
	-borderwidth 4 \
	-padx 30 \
	-pady 10 \
	-relief sunken
    bind $g <ButtonPress-2>  [list Move %W 0]
    bind $g <ButtonPress-3>  [list Move %W end]
}

blt::table . \
    0,0 .ss -fill both 
focus .ss.fs

