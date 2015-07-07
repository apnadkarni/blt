
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

proc Move { w pane } {
    .ss.fs see $pane
}

blt::scrollset .ss \
    -xviewcommand { .ss.fs view } \
    -xscrollbar .ss.xs \
    -window .ss.fs 

blt::filmstrip .ss.fs  -width 600 \
    -scrolldelay 10 -scrollincrement 30 -animate yes \
    -scrollcommand { .ss xset }

blt::tk::scrollbar .ss.xs -orient horizontal -command { .ss xview }

for { set i 0 } { $i < 32 } { incr i } {
    set color [lindex $autocolors $i]
    set g .ss.fs.g$i
    blt::graph $g -bg $color -width 500
    set pane [.ss.fs add -window $g -fill x -showgrip yes]
    bind $g <ButtonPress-1>  [list Move %W $pane]
    bind $g <ButtonPress-2>  [list Move %W 0]
    bind $g <ButtonPress-3>  [list Move %W end]
}

blt::table . \
    0,0 .ss -fill both 
focus .ss.fs

