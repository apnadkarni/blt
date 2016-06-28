
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

blt::scrollset .ss \
    -xviewcommand { .ss.fs view } \
    -xscrollbar .ss.xs \
    -yscrollbar .ss.xs \
    -window .ss.fs 

blt::filmstrip .ss.fs  -height 400 \
    -scrolldelay 10 -scrollincrement 30 -animate yes \
    -gripthickness 0 \
    -orient vertical

blt::tk::scrollbar .ss.xs 
blt::tk::scrollbar .ss.ys 

for { set i 0 } { $i < 32 } { incr i } {
    set color [lindex $autocolors $i]
    set g .ss.fs.g$i
    blt::graph $g -bg $color -height 300 -width 500
    set frame [.ss.fs add -window $g -fill y -showgrip yes]
    bind $g <ButtonPress-1>  [list Move %W $frame]
    bind $g <ButtonPress-2>  [list Move %W 0]
    bind $g <ButtonPress-3>  [list Move %W end]
}

blt::table . \
    0,0 .ss -fill both 
focus .ss.fs


