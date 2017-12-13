package require BLT

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set icon [image create picture -data $imgData]
set activebg [blt::background create linear \
		  -highcolor  grey70 -lowcolor grey85 \
		  -jitter 10 -colorscale log]
set bg [blt::background create linear -highcolor  grey80 -lowcolor grey95 \
	    -jitter 10 -colorscale log -relativeto self]

set image ""
option add *ComboEntry.takeFocus 1

set hideChars 0

proc ToggleHideChars {} {
    set varName [.cb cget -variable]
    global $varName
    set bool [set $varName]
    if { $bool } {
	.e configure -show \u25CF 
    } else {
	.e configure -show ""
    }
}

blt::comboentry .e \
    -textvariable t \
    -font { arial 11 } \
    -image $image \
    -iconvariable icon \
    -edit yes \
    -menu .e.m \
    -exportselection yes \
    -xscrollcommand { .s set }  \
    -xbutton yes 


blt::combobutton .b \
    -textvariable t \
    -image $image \
    -iconvariable icon \
    -bg $bg \
    -font { Arial 12  } -justify left \
    -underline 19 \
    -arrowborderwidth 2 \
    -arrowrelief flat  

blt::tk::scrollbar .s -orient vertical -command { .e xview } 
blt::tk::checkbutton .cb -text "Hide Characters" -variable hideChars \
    -command ToggleHideChars

blt::table . \
    1,0 .e -fill both -cspan 2 -padx 2 -pady 2  \
    0,0 .cb 


blt::table configure . c1 -resize shrink
blt::table configure . r0 -resize shrink
blt::table configure . c0 -pad { 2 0 }
blt::table configure . c1 -pad { 0 2 }

blt::combomenu .e.m  -relief sunken -bg white -textvariable t
.e.m add -text "one" -accelerator "Ctrl+A" 
.e.m add -text "two" -accelerator "Ctrl+B" 
.e.m add -text "three" -accelerator "Ctrl+C"
.e.m add -text "four" -accelerator "Ctrl+D" 
.e.m add -type cascade -text "cascade" -accelerator "Ctrl+E" -menu .e.m.m

blt::combomenu .e.m.m -relief sunken -bg white -textvariable t
.e.m.m add -text "five" -accelerator "^A"
.e.m.m add -text "six" -accelerator "^B" 
.e.m.m add -text "seven" -accelerator "^C" 
.e.m.m add -text "eight" -accelerator "^D" 
.e.m.m add -text "nine" -accelerator "^E" -command "set t {really really really long entry}"
.e.m.m add -type cascade -text "cascade" -accelerator "^F" 


after idle { 
    set t "Hello, World" 
    .e insert 0 "Fred says: \n"
    puts "($t)"
    update
}

proc AddEntry { e m } {
    set s [$e get]
    puts stderr "current entry is $s"

    puts stderr "$m find $s -from 0 -type command => [$m find $s -from 0 -type command]"
    puts stderr "$m find * -from 0 -type separator -glob => [$m find * -from 0 -type separator -glob]"

    if { [$m find $s -from 0 -type command] < 0 } {
	set sep [$m index "mysep"]
	if { $sep < 0 } {
	    $m insert before 0 -type separator -text "mysep"
	}
	$m insert before 0 -text $s 
    } 
}

bind .e <Return> [list AddEntry .e .e.m]

