
package require BLT

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

#set icon [image create picture -data $imgData]
set icon [image create picture -file images/mini-book1.gif]
#set icon [image create picture -file images/blt98.gif]
#set image [image create picture -file ~/images.jpeg]
set activebg [blt::background create gradient -high  grey70 -low grey85 \
	-jitter 10 -scale log -relativeto self]
set bg [blt::background create gradient -high  grey80 -low grey95 \
	-jitter 10 -scale log -relativeto self]

set image ""
option add *ComboEntry.takeFocus 1

proc Doit {} {
    puts stderr grab=[grab current]
    puts stderr "-command invoked"
}

blt::comboentry .e \
    -textvariable t \
    -font { arial 10 } \
    -image $image \
    -iconvariable icon \
    -edit yes \
    -textwidth 6 \
    -menu .e.m \
    -exportselection yes \
    -xscrollcommand { .s set }  \
    -command "Doit" \
    -closebutton yes \
    -closecommand { .e delete 0 end } 

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

blt::table . \
    0,0 .e -fill both -cspan 2 -padx 2 -pady 2 

blt::table configure . c1 -resize shrink
blt::table configure . r0 -resize shrink
blt::table configure . c0 -pad { 2 0 }
blt::table configure . c1 -pad { 0 2 }

blt::combomenu .e.m  -relief sunken -bg white -textvariable t
.e.m add -text "one" -accelerator "^A" 
.e.m add -text "two" -accelerator "^B" 
.e.m add -text "three" -accelerator "^C"
.e.m add -text "four" -accelerator "^D" 
.e.m add -type cascade -text "cascade" -accelerator "^E" -menu .e.m.m

blt::combomenu .e.m.m -relief sunken -bg white -textvariable t
.e.m.m add -text "five" -accelerator "^A"
.e.m.m add -text "six" -accelerator "^B" 
.e.m.m add -text "seven" -accelerator "^C" 
.e.m.m add -text "eight" -accelerator "^D" 
.e.m.m add -text "nine" -accelerator "^D" -command "set t {really really really long entry}"
.e.m.m add -type cascade -text "cascade" -accelerator "^E" 


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

