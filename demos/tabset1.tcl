#!../src/bltwish

package require BLT
option add *Tabset*Tab.PadY 0
option add *Tabset*Tab.PadX 0
option add *Tabset.Tab.iPadY 0
option add *Tabset.Tab.iPadX 0
#option add *Tabset.Tab.background green

image create picture bgTile -file ./images/chalk.gif
image create picture img1 -file $blt_library/icons/32x32/filter.png
image create picture img2 -file ./images/mini-book2.gif
image create picture img3 -width 10 -height 30
img3 blank white

blt::tabset .t \
    -outerrelief raised \
    -side top \
    -tearoff yes \
    -slant none \
    -justify center \
    -tabwidth same \
    -outerborderwidth 0 \
    -highlightthickness 0 \
    -scrollcommand { .s set } \
    -xbutton selected \
    -rotate 90 \
    -height 3i -width 3i

.t add First \
    -icon img1 \
    -anchor center \
   
if 1 {
foreach page { Second Third Fourth Fifth } {
    .t add $page \
	-anchor center \
	-selectbackground darkolivegreen2 \
	-perforationbackground darkolivegreen2 \

    #	-icon img3
}

.t add -text Sixth \
    -selectbackground lightblue \
    -perforationbackground lightblue 
}
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
    -font "Arial 9" \
    -anchor center \
    -selectbackground yellow \
    -command NewTab


blt::tk::scrollbar .s -command { .t view } -orient horizontal
blt::table . \
    .t 0,0 -fill both \
    .s 1,0 -fill x 

blt::table configure . r1 -resize none
focus .t

#puts stderr [.t configure]
