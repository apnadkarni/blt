#!../src/bltwish

package require BLT
#source scripts/demo.tcl

set spcount 0
#
# Script to test the "busy" command.
# 

proc GetBusyWindow { w } {
   set parent [winfo parent $w]
}

#
# General widget class resource attributes
#
option add *Button.padX 	10
option add *Button.padY 	2
option add *Scale.relief 	sunken
#option add *Scale.orient	horizontal
option add *Entry.relief 	sunken
option add *Frame.borderWidth 	2

set visual [winfo screenvisual .] 
if { $visual == "staticgray"  || $visual == "grayscale" } {
    set activeBg black
    set normalBg white
    set bitmapFg black
    set bitmapBg white
    option add *f1.background 		white
} else {
    set activeBg red
    set normalBg springgreen
    set bitmapFg blue
    set bitmapBg green
    option add *Button.background       khaki2
    option add *Button.activeBackground khaki1
    option add *Frame.background        khaki2
    option add *f2.tile		textureBg
#    option add *Button.tile		textureBg

    option add *f2.releaseButton.background 		limegreen
    option add *f2.releaseButton.activeBackground 	springgreen
    option add *f2.releaseButton.foreground 		black

    option add *f2.holdButton.background 		red
    option add *f2.holdButton.activeBackground	pink
    option add *f2.holdButton.foreground 		black
    option add *f1.background 		springgreen
}

#
# Instance specific widget options
#
option add *f1.relief 		sunken
option add *f1.background 	$normalBg
option add *f1.testButton.text 	"Test"
option add *f1.quitButton.text 	"Quit"
option add *f2.holdButton.text 	"Hold"
option add *f2.releaseButton.text 	"Release"
option add *buttonLabel.text	"Buttons"
option add *f1.entryLabel.text	"Entries"
option add *scaleLabel.text	"Scales"
option add *textLabel.text	"Text"


set spinner [image create picture]
set files [glob -nocomplain ./images/spinner*.png]
foreach file [lsort -dictionary $files] {
    set img [image create picture -file $file]
    $spinner list append $img
    image delete $img 
}
$spinner list delete 0 1

proc LoseFocus {} { 
    focus -force . 
}
proc KeepRaised { w } {
    bindtags $w keepRaised
}

bind keepRaised <Visibility> { raise %W } 

set file ./images/chalk.gif
image create picture textureBg -file $file

#
# This never gets used; it's reset by the Animate proc. It's 
# here to just demonstrate how to set busy window options via
# the host window path name
#
#option add *f1.busyCursor 	bogosity 


menu .menu 
.menu add command -label "First"
.menu add command -label "Second"
.menu add command -label "Third"
.menu add command -label "Fourth"
. configure -menu .menu

#
# Create two frames. The top frame will be the host window for the
# busy window.  It'll contain widgets to test the effectiveness of
# the busy window.  The bottom frame will contain buttons to 
# control the testing.
#
frame .f1
frame .f2

#
# Create some widgets to test the busy window and its cursor
#
label .buttonLabel
button .f1.testButton -command { 
    puts stdout "Not busy." 
}
button .f1.quitButton -command { exit }
entry .f1.entry 
scale .f1.scale
text .f1.text -width 20 -height 4

#
# The following buttons sit in the lower frame to control the demo
#
button .f2.holdButton -command {
    if { ![blt::busy isbusy .f1] } {
        global activeBg
	.f1 configure -bg $activeBg
    }
    blt::busy .f1 -opaque 1 -darken 40 -image $spinner -delay 50
    blt::busy .#menu
    if 0 {
	set w .f1_Busy
	GetBusyWindow .f1

	blt::tk::button $w.s -text "Testing" -command "puts stderr edit" \
	    -cursor left_ptr -width 20 -height 3
	blt::table $w \
	    0,0 $w.s -padx 0.24i -pady 0.24i
	LoseFocus
	#Shake 3000
    }
}
button .f2.releaseButton -command {
    if { [blt::busy isbusy .f1] } {
        blt::busy release .f1
        blt::busy release .#menu
    }
    global normalBg
    .f1 configure -bg $normalBg
    destroy .f1_Busy.s
}

proc Shake { count } {
    set w .f1_Busy.s
    if { ![winfo exists $w] } {
	return
    }
    incr count -3
    if { $count <= 0 } {
	return
    }
    set dx [expr int(sin($count * 0.1) * 10)]
    if { $dx < 0 } {
	set pad [list [expr abs($dx)] 0] 
    } else {
	set pad [list 0 $dx]
    }
    blt::table .f1_Busy 0,0 .f1_Busy.s -padx $pad 
    after 10 [list Shake $count]
}


#
# Notice that the widgets packed in .f1 and .f2 are their children
#
blt::table .f1 \
    .f1.testButton 0,0 \
    .f1.scale 1,0 \
    .f1.entry 0,1 \
    .f1.text 1,1 -fill both \
    .f1.quitButton 2,0 

blt::table .f2 \
    .f2.holdButton 1,0 \
    .f2.releaseButton 2,0  

blt::table configure .f1 .f1.testButton .f1.scale .f1.entry .f1.quitButton \
    -padx 10 -pady 10 -fill both
blt::table configure .f2 .f2.holdButton .f2.releaseButton \
    -padx 10 -pady 10 
blt::table configure .f2 c0 -resize none
#
# Finally, realize and map the top level window
#
blt::table . \
    .f1 0,0  \
    .f2 1,0 

blt::table configure . .f1 .f2 -fill both
# Initialize a list of bitmap file names which make up the animated 
# fish cursor. The bitmap mask files have a "m" appended to them.

blt::table configure . r1 -resize none

set bitmapList { left left1 mid right1 right }

#
# Simple cursor animation routine: Uses the "after" command to 
# circulate through a list of cursors every 0.075 seconds. The
# first pass through the cursor list may appear sluggish because 
# the bitmaps have to be read from the disk.  Tk's cursor cache
# takes care of it afterwards.
#
proc StartAnimation { widget count } {
    global bitmapList
    set prefix "bitmaps/fish/[lindex $bitmapList $count]"
    set cursor [list @${prefix}.xbm ${prefix}m.xbm black white ]
    blt::busy configure $widget -cursor $cursor

    incr count
    set limit [llength $bitmapList]
    if { $count >= $limit } {
	set count 0
    }
    global afterId
    set afterId($widget) [after 175 StartAnimation $widget $count]
}

proc StopAnimation { widget } {    
    global afterId
    after cancel $afterId($widget)
}

proc TranslateBusy { window } {
    #set widget [string trimright $window "_Busy"]
    set widget [string trimright $window "Busy"]
    set widget [string trimright $widget "_"]
    if { [winfo toplevel $widget] != $widget } {
        set widget [string trimright $widget "."]
    }
    return $widget
}

if 0 {
if { [info exists tcl_platform] && $tcl_platform(platform) == "unix" } {
    bind BltBusy <Map> { 
	StartAnimation [TranslateBusy %W] 0
    }
    bind BltBusy <Unmap> { 
	StopAnimation  [TranslateBusy %W] 
    }
}
}

#
# For testing, allow the top level window to be resized 
#
wm min . 0 0

#
# Force the demo to stay raised
#
raise .
KeepRaised .

