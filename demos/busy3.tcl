#!../src/bltwish

package require BLT

#
# Script to test the "busy" command.
# 
proc MakeBusy {} {
    global spinner
    blt::busy .f1 -opacity 15 -color grey10 -image $spinner -delay 50
    blt::busy .#menu
}
	       
proc MakeUnbusy {} {
    if { [blt::busy isbusy .f1] } {
	blt::busy forget .f1
	blt::busy release .#menu
    }
}

set bg [blt::background create stripe -oncolor grey95 -offcolor grey90]

set spinner [image create picture]
set files [glob -nocomplain ./images/spinner*.png]
foreach file [lsort -dictionary $files] {
    set img [image create picture -file $file]
    $spinner list append $img
    image delete $img 
}
$spinner list delete 0 1

menu .menu 
.menu add command -label "First"
.menu add command -label "Second"
.menu add command -label "Third"
.menu add command -label "Fourth"
. configure -menu .menu

# Create two frames. The top frame will be the host window for the busy
# window.  It'll contain widgets to test the effectiveness of the busy
# window.  The bottom frame will contain buttons to control the testing.

blt::tk::frame .f1 -background $bg -borderwidth 2 -relief sunken
blt::tk::frame .f2 -background $bg -borderwidth 2 

#
# Create some widgets to test the busy window and its cursor
#
blt::tk::button .f1.test \
    -activebackground #FFFED1 \
    -background #FFFFEB \
    -command { puts stdout "Not busy." } \
    -highlightbackground $bg \
    -text "Test" 

blt::tk::button .f1.quit \
    -activebackground #FFFED1 \
    -background #FFFFEB \
    -command { exit } \
    -highlightbackground $bg \
    -text "Quit" 

entry .f1.entry -relief sunken
scale .f1.scale -relief sunken
text .f1.text   -relief sunken -width 20 -height 4 

#
# The following buttons sit in the lower frame to control the demo
#
blt::tk::button .f2.hold \
    -activebackground pink \
    -background red \
    -command MakeBusy \
    -foreground black \
    -text "Hold" 

blt::tk::button .f2.release \
    -activebackground springgreen \
    -background limegreen \
    -command MakeUnbusy \
    -foreground black \
    -text "Release" 

blt::table .f1 \
    0,0 .f1.test \
    1,0 .f1.scale            -padx 10 \
    0,1 .f1.entry -fill x    -padx 10 \
    1,1 .f1.text  -fill both -padx 10 \
    2,0 .f1.quit 

blt::table .f2 \
    0,0 .f2.hold    -width .9i -padx 10 -pady 10 \
    0,1 .f2.release -width .9i -padx 10 -pady 10 

blt::table . \
    .f1 0,0  -fill both \
    .f2 1,0  -fill x 

blt::table configure . r1 -resize none

wm withdraw . 
#
# For testing, allow the top level window to be resized 
#
wm min . 0 0

#
# Force the demo to stay raised
#
raise .
update
wm deiconify . 

