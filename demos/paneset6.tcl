
package require BLT

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

proc Move { w pane } {
    .ps see $pane
}

blt::filmstrip .ps  -bg grey -width 600 -scrollcommand { .s set } \
    -gripthickness 3 -grippad 1 -gripborderwidth 2 \
    -activegriprelief raised -griprelief flat 

for { set i 0 } { $i < 32 } { incr i } {
    set color [lindex $autocolors $i]
    blt::graph .ps.g$i -bg $color -width 500
    set pane [.ps add -window .ps.g$i -fill both -showgrip yes]
    bind .ps.g$i <ButtonPress-1>  [list Move %W $pane]
    bind .ps.g$i <ButtonPress-2>  [list Move %W pane0]
    bind .ps.g$i <ButtonPress-3>  [list Move %W pane35]
}
blt::tk::scrollbar .s -command { .ps view } -orient horizontal

set pressed 0
bind BltPaneset <Enter> { %W activate } 
bind BltPaneset <Leave> { 
    if { !$pressed } { %W deactivate } 
}
bind BltPaneset <KeyPress-Left> { %W move -10 0 }
bind BltPaneset <KeyPress-Right> { %W move 10  0 }
bind BltPaneset <KeyPress-Up> { %W move 0 -10 }
bind BltPaneset <KeyPress-Down> { %W move 0 10 }
bind BltPaneset <Shift-KeyPress-Left> { %W move -100 0 }
bind BltPaneset <Shift-KeyPress-Right> { %W move 100  0 }
bind BltPaneset <Shift-KeyPress-Up> { %W move 0 -100 }
bind BltPaneset <Shift-KeyPress-Down> { %W move 0 100 }
bind BltPaneset <ButtonPress-1> { 
    set pressed 1
    %W anchor %X %Y 
    focus %W
}
bind BltPaneset <B1-Motion> { %W mark %X %Y }
bind BltPaneset <ButtonRelease-1> { 
    set pressed 0
    %W set %X %Y 
}

blt::table . \
    0,0 .ps -fill both \
    1,0 .s -fill x 

blt::table configure . r1 -resize none

focus .ps
after 5000 {
    #.ps pane configure 1 -size { 0 10000 10 }
    focus .
    #.ps see pane2
    #.ps size pane2 1i
}

