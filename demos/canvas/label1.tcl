
package require BLT

canvas .c -bg white
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
puts stderr "MarchingAnts $dashOffset"
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
}
proc Deactivate { canvas id } {
  global afterId
  $canvas itemconfigure $id -state normal -bg [NextColor]
  after cancel $afterId
}
.c create rectangle 100 100 300 200 -fill lightblue3

set id [.c create label 100 100 \
	    -text "Hello, World" \
	    -bg lightgreen \
	    -activebg red3 -linewidth 2 \
	    -anchor nw \
	    -textanchor c \
	    -padx .2i \
	    -activedashes 4 \
	    -rotate 45 \
	    -width 0 \
	    -height 75]

blt::table . \
    0,0 .c -fill both

.c bind $id <Enter> [list Activate .c $id]
.c bind $id <Leave> [list Deactivate .c $id]

after 2000 {
  set id [.c find overlapping 90 90 99 99]
  puts id=$id
}