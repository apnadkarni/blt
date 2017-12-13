#!../src/bltwish

package require BLT
option add *Tabset.Tab.padY 0
#option add *Tabset.Tab.background green

image create picture bgTile -file ./images/chalk.gif
image create picture img1 -file ./images/mini-book1.gif
image create picture img2 -file ./images/mini-book2.gif
image create picture img3 -width 10 -height 10
img3 blank purple

blt::tabset .t \
    -outerrelief raised \
    -tabwidth same \
    -outerborderwidth 0 \
    -highlightthickness 0 \
    -scrollcommand { .s set } \
    -xbutton yes \
    -width 7i
font create myFont -family Arial -size 9
puts stderr myFont=[font configure myFont]
puts stderr fm=[font metrics myFont]
puts stderr meas=[font measure myFont "Hello World"]
.t add First \
    -image img1 \
    -anchor center \
   


foreach page { Again Next another test of a widget } {
    .t add $page \
	-anchor center \
	-selectbackground darkolivegreen2 \
	-image img3
}

.t add -text Again -selectbackground lightblue 

set tabcount 0
proc NewTab { args } {
    global tabcount
    set i [.t insert end "New Tab $tabcount"]
    .t select $i
    update
    .t focus $i
    .t see last
    incr tabcount
}

.t add "+" \
    -font "Arial 10" \
    -anchor center \
    -selectbackground yellow \
    -command NewTab


blt::tk::scrollbar .s -command { .t view } -orient horizontal
blt::table . \
    .t 0,0 -fill both \
    .s 1,0 -fill x 

blt::table configure . r1 -resize none
focus .t

puts stderr [.t button configure]
