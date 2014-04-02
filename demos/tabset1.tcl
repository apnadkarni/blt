#!../src/bltwish

package require BLT
source scripts/demo.tcl
option add *Tabset.Tab.padY 0
#option add *Tabset.Tab.background green

image create picture bgTile -file ./images/chalk.gif
image create picture label1 -file ./images/mini-book1.gif
image create picture label2 -file ./images/mini-book2.gif
image create picture label3 -width 10 -height 10
label3 blank white

blt::tabset .t \
    -outerrelief raised \
    -tabwidth same \
    -outerborderwidth 0 \
    -highlightthickness 0 \
    -scrollcommand { .s set } \
    -clearbutton yes \
    -width 3i

.t add First \
    -image label1 \
    -anchor center \
    -selectbackground darkolivegreen2  


foreach page { Again Next another test of a widget } {
    .t add $page \
	-anchor center \
	-selectbackground darkolivegreen2 \
	-image label3
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

