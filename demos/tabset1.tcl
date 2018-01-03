#!../src/bltwish
package require BLT
if 0 {
option add *Tabset*Tab.PadY 0
option add *Tabset*Tab.PadX 0
option add *Tabset.Tab.iPadY 0
option add *Tabset.Tab.iPadX 0
#option add *Tabset.Tab.background green
}
if { [blt::winop xdpi] > 150 } {
  image create picture img1 -file $blt_library/icons/32x32/filter.png
} else {
  image create picture img1 -file $blt_library/icons/16x16/filter.png
}
image create picture img2 -file ./images/mini-book2.gif
image create picture img3 -width 10 -height 30
img3 blank white

set side bottom
blt::tabset .t \
    -outerrelief raised \
    -side $side \
    -tearoff yes \
    -slide yes \
    -slant none \
    -scrolltabs yes \
    -justify center \
    -tabwidth same \
    -outerborderwidth 0 \
    -highlightthickness 0 \
    -scrollcommand { .s set } \
    -rotate auto \
    -xbutton selected 
.t add First \
    -icon img1 \
    -anchor center \
   
if 1 {
.t style create green \
	-selectbackground darkolivegreen2 \
	-perforationbackground darkolivegreen2  
.t style create blue \
	-selectbackground lightblue \
	-perforationbackground lightblue 

foreach page { Second Third Fourth Fifth } {
    .t add $page \
	-anchor center \
	-style green
    #	-icon img3
}

.t add -text Sixth \
    -style blue
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

.t style create plus \
    -selectbackground yellow 
.t add "+" \
    -anchor center \
    -style plus \
    -selectcommand NewTab


if { $side == "bottom" || $side == "top" } {
  set orient "horizontal"
  set loc 1,0
  set fill x
} else {
  set orient "vertical"
  set loc 0,1
  set fill y

}

blt::tk::scrollbar .s -command { .t view } -orient $orient
blt::table . \
    .t 0,0 -fill both \
    .s $loc -fill $fill

blt::table configure . r1 -resize none
focus .t

