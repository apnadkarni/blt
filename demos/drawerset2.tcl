
package require BLT
source scripts/demo.tcl

blt::graph .g -bg \#CCCCFF -height 800
set w [blt::drawerset .g -side top -sashthickness 20]
blt::barchart .g.b -bg \#FFCCCC -height 300
blt::barchart .g.b2 -bg \#CCFFCC -height 300
$w add -window .g.b -fill x -resize both
$w add -window .g.b2 -fill y
$w open pane0
set pressed 0
bind BltPaneset <Enter> { if { !$pressed } { %W activate } } 
bind BltPaneset <Leave> { if { !$pressed } { %W deactivate } } 
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
}
bind BltPaneset <B1-Motion> { %W mark %X %Y }
bind BltPaneset <ButtonRelease-1> { 
    set pressed 0
    %W set %X %Y 
}

blt::table . \
    0,0 .g -fill both 
