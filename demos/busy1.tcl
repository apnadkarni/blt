#!../src/bltwish

package require BLT
source scripts/demo.tcl

set normalBg [blt::bgpattern create texture -high grey95 -low grey90]
set activeBg [blt::bgpattern create texture -high red1 -low red2]

#
# General widget class resource attributes
#
option add *Button.padX 	10
option add *Button.padY 	2
option add *Scale.relief 	sunken
#option add *Scale.orient	horizontal
option add *Entry.relief 	sunken
option add *borderWidth 	1

set visual [winfo screenvisual .] 
if { $visual == "staticgray"  || $visual == "grayscale" } {
    set activeBg black
    set normalBg white
    set bitmapFg black
    set bitmapBg white
    option add *f1.background 		white
} else {
#    set activeBg red
    set bitmapFg blue
    set bitmapBg green
    option add *Button.background       khaki2
    option add *Button.activeBackground khaki1
    option add *Frame.background        khaki2

    option add *releaseButton.background 		limegreen
    option add *releaseButton.activeBackground 	springgreen
    option add *releaseButton.foreground 		black

    option add *holdButton.background 		red
    option add *holdButton.activeBackground	pink
    option add *holdButton.foreground 		black
    option add *f1.background 		springgreen
}

#
# Instance specific widget options
#
option add *f1.relief 		sunken
option add *f1.background 	$normalBg
option add *testButton.text 	"Test"
option add *quitButton.text 	"Quit"
option add *newButton.text 	"New\nButton"
option add *holdButton.text 	"Hold"
option add *releaseButton.text 	"Release"
option add *buttonLabel.text	"Buttons"
option add *entryLabel.text	"Entries"
option add *scaleLabel.text	"Scales"
option add *textLabel.text	"Text"

bind keepRaised <Visibility> { raise %W } 

proc KeepRaised { w } {
    bindtags $w keepRaised
}

#
# This never gets used; it's reset by the Animate proc. It's 
# here to just demonstrate how to set busy window options via
# the host window path name
#
#option add *f1.busyCursor 	bogosity 

#
# Counter for new buttons created by the "New button" button
#
set numWin 0

#
# Create two frames. The top frame will be the host window for the
# busy window.  It'll contain widgets to test the effectiveness of
# the busy window.  The bottom frame will contain buttons to 
# control the testing.
#

blt::tk::frame .f1 -bg $normalBg
blt::tk::frame .f2 -bg $normalBg

#
# Create some widgets to test the busy window and its cursor
#
label .buttonLabel
    blt::tk::button .testButton -command { 
    puts stdout "Not busy." 
}
blt::tk::button .quitButton -command { exit }
entry .entry 
scale .scale
text .text -width 20 -height 4

#
# The following buttons sit in the lower frame to control the demo
#
blt::tk::button .newButton -command {
    global numWin
    incr numWin
    set name button#${numWin}
    blt::tk::button .f1.$name -text "$name" \
	-command [list .f1 configure -bg blue]
    blt::table .f1 \
	.f1.$name $numWin+3,0 -padx 10 -pady 10
}

blt::tk::button .holdButton -command {
    if { [blt::busy isbusy .f1] == "" } {
        global activeBg
	.f1 configure -bg $activeBg
    }
    blt::busy .f1 
    focus -force . 
}

blt::tk::button .releaseButton -command {
    if { [blt::busy isbusy .f1] == ".f1" } {
        blt::busy release .f1
    }
    global normalBg
    .f1 configure -bg $normalBg
}

#
# Notice that the widgets packed in .f1 and .f2 are not their children
#
blt::table .f1 \
    0,0		.testButton \
    1,0		.scale		-fill y \
    0,1		.entry		-fill x \
    1,1		.text		-fill both \
    2,0		.quitButton	-cspan 2

blt::table .f2 \
    0,0		.holdButton \
    0,1		.releaseButton  \
    0,2		.newButton

blt::table configure .f1 \
    .testButton .scale .entry .quitButton -padx 10 -pady 10
blt::table configure .f2 \
    .newButton .holdButton .releaseButton -padx 10 -pady 4 -reqwidth 1.i

blt::table configure .f1 r0 r2 -resize none
blt::table configure .f2 r* -resize none

#
# Finally, realize and map the top level window
#
blt::table . \
    0,0		.f1		-fill both \
    1,0		.f2		-fill both

blt::table configure . r1 -resize none

blt::table configure .f1 c1 -weight 2.0

# Initialize a list of bitmap file names which make up the animated 
# fish cursor. The bitmap mask files have a "m" appended to them.

set bitmapList { 
    left left1 mid right1 right 
}

#
# Simple cursor animation routine: Uses the "after" command to 
# circulate through a list of cursors every 0.075 seconds. The
# first pass through the cursor list may appear sluggish because 
# the bitmaps have to be read from the disk.  Tk's cursor cache
# takes care of it afterwards.
#
proc StartAnimation { widget count } {
    global bitmapList
    set prefix bitmaps/fish/[lindex $bitmapList $count]
    set cursor [list @${prefix}.xbm ${prefix}m.xbm blue green ]
    blt::busy configure $widget -cursor $cursor

    incr count
    set limit [llength $bitmapList]
    if { $count >= $limit } {
	set count 0
    }
    global afterId
    set afterId($widget) [after 125 StartAnimation $widget $count]
}

proc StopAnimation { widget } {    
    global afterId
    after cancel $afterId($widget)
}

proc TranslateBusy { window } {
    set widget [string trimright $window "_Busy"]
    if { $widget != "." } {
        set widget [string trimright $widget "."]
    }
    return $widget
}

if { [info exists tcl_platform] && $tcl_platform(platform) == "unix" } {
    bind BltBusy <Map> { 
	StartAnimation [TranslateBusy %W] 0
    }
    bind BltBusy <Unmap> { 
	StopAnimation  [TranslateBusy %W] 
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

bind .f1 <Enter> { puts stderr "Entering %W" }
bind .f1 <Leave> { puts stderr "Leaving %W" }
bind .f1 <B1-Leave> { puts stderr "B1 Leaving %W" }
bind .f1 <B1-Enter> { puts stderr "B1 Entering %W" }

bind .f1 <Motion> { puts stderr "Motion %W" }

.testButton configure -font "{San Serif} 6"
puts stderr [.testButton configure]
