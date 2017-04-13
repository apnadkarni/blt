
package require BLT

set afterId -1
set dashOffset 0
set nextColor -1
set autocolors {
#DCFFD8
#FFD0D0
#D4D5FF
#FFFFC9
#D6FBFF
#EDD3FF
#E7FFCF
#FFEAC3
#D9E5FF
#C4FFE0
#FFD5ED
#FFDBA8
#B5CFFF
#FF7684
#85FF9B
#CEFFF8
#FFDFD0
#D0FFCF
#AFFFFF
#FFFFC2
#FFE0F4
#D8E9FF
#ECD5FF
#D2FFCA
#FFF9C4
#EFFFD8
}

set bg1 grey90
set bg2 [blt::paintbrush create color -color blue -opacity 30]
set bg3 [blt::paintbrush create linear -jitter 3 \
	     -colorscale linear \
	     -from w \
	     -to e \
	     -high grey90 -low grey98 \
	     -repeat reversing]

proc NextColor {} {
  global nextColor autocolors
  incr nextColor
  if { $nextColor >= [llength $autocolors] } {
    set nextColor 0
  }
  return [lindex $autocolors $nextColor]
}

proc MarchingAnts { canvas id } {
  global afterId dashOffset
  incr dashOffset
  $canvas itemconfigure $id -activedashoffset $dashOffset
  set afterId [after 100 [list MarchingAnts $canvas $id]]
}

proc Activate { canvas id } {
  global afterId dashOffset
  $canvas itemconfigure $id -state active -activebg [NextColor]
  set dashOffset -1
  after cancel $afterId
  MarchingAnts $canvas $id




















    $canvas raise $id
}
proc Deactivate { canvas id } {
  global afterId bg1
  $canvas itemconfigure $id -state normal -bg $bg1
  after cancel $afterId
}

blt::scrollset .ss \
    -xscrollbar .ss.xs \
    -yscrollbar .ss.ys \
    -window .ss.c 
blt::tk::scrollbar .ss.ys
blt::tk::scrollbar .ss.xs
canvas .ss.c -bg white

blt::table . \
    0,0 .ss -fill both


set id [.ss.c create label 100 100 \
	    -text "Hello, World" \
	    -bg $bg1 \
	    -activebg red3 -activelinewidth 2 -activedashes 4 \
	    -linewidth 1 \
	    -anchor nw \
	    -textanchor c \
	    -padx 0 \
	    -font "Arial 13" \
	    -rotate 90 \
	    -width 150 \
	    -height 50]

.ss.c bind $id <Enter> [list Activate .ss.c $id]
.ss.c bind $id <Leave> [list Deactivate .ss.c $id]

set id [.ss.c create label 150 100 \
	    -text "Hello, World" \
	    -bg $bg1 \
	    -activebg red3 -activelinewidth 2 -activedashes 4 \
	    -linewidth 1 \
	    -anchor nw \
	    -textanchor c \
	    -padx 0 \
	    -font "Arial 13" \
	    -rotate 90 \
	    -width 150 \
	    -height 50]

.ss.c bind $id <Enter> [list Activate .ss.c $id]
.ss.c bind $id <Leave> [list Deactivate .ss.c $id]

set id [.ss.c create label 100 50 \
	    -text "Hello, World" \
	    -bg $bg2 \
	    -activebg red3 -activelinewidth 2 -activedashes 4 \
	    -linewidth 1 \
	    -anchor nw \
	    -textanchor c \
	    -padx 0 \
	    -font "Arial 13" \
	    -rotate 0 \
	    -width 150 \
	    -height 50]

blt::table . \
    0,0 .ss -fill both

.ss.c bind $id <Enter> [list Activate .ss.c $id]
.ss.c bind $id <Leave> [list Deactivate .ss.c $id]
set x2 [expr [winfo reqwidth .ss.c] - 10]
set y2 [expr [winfo reqheight .ss.c] - 10]
#.ss.c configure -scrollregion [list 0 0  $x2 $y2]

bind .ss.c  <4>  { 
    set cx [expr [winfo width .ss.c] / 2]
    set cy [expr [winfo height .ss.c] / 2]
    .ss.c scale all $cx $cy 1.1 1.1 
}
bind .ss.c  <5>  {
    set cx [expr [winfo width .ss.c] / 2]
    set cy [expr [winfo height .ss.c] / 2]
    .ss.c scale all $cx $cy 0.9 0.9
}

