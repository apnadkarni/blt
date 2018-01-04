
package require BLT


set bg [image create picture -file ./images/blend.png]
set fg [image create picture -file ./images/base.png]

set modes {
    colorburn
    colordodge
    darken
    difference
    divide
    exclusion
    hardlight
    hardmix
    lighten
    linearburn
    lineardodge
    linearlight
    multiply
    normal
    overlay
    pinlight
    screen
    softlight
    softlight2
    subtract
    vividlight
}
. configure -bg white
set xbg [blt::background create linear \
	     -highcolor grey97 -lowcolor grey87 \
	    -from n -to s -relativeto self]
set ybg [blt::background create linear \
	     -highcolor grey97 -lowcolor grey87 \
	    -from n -to s -relativeto self -decreasing yes]

set img [image create picture]
blt::tk::frame .top -bg $xbg
blt::tk::frame .middle -bg $ybg
blt::tk::label .top.label -text "Color Blending Mode" -bg $xbg
blt::tk::label .middle.result_l -text Result -bg $ybg
blt::tk::label .middle.base_l -text Background  -bg $ybg
blt::tk::label .middle.blend_l -text Foreground -bg $ybg
blt::tk::label .result -image $img
blt::tk::label .base -image $bg
blt::tk::label .blend -image $fg
set m .top.entry.menu
blt::comboentry .top.entry \
    -menu .top.entry.menu \
    -textvariable textVar \
    -width 200 \

blt::combomenu .top.entry.menu \
    -textvariable textVar \
    -xscrollbar .top.entry.menu.xs \
    -yscrollbar .top.entry.menu.ys \
    -restrictwidth min \
    -height 1i \
    -command {
	$img colorblend $bg $fg -mode $blend
    }
blt::tk::scrollbar .top.entry.menu.xs
blt::tk::scrollbar .top.entry.menu.ys

foreach mode $modes {
    $m add -text $mode -variable blend -value $mode
}
$m invoke "normal"
blt::table . \
    0,0 .top -fill x -cspan 3 \
    1,0 .base \
    1,1 .blend \
    1,2 .result \
    2,0 .middle -fill x -cspan 3 

blt::table .top \
    0,1 .top.label -anchor e \
    0,2 .top.entry -anchor w 
blt::table configure .top c* -resize none   
blt::table configure .top c3 -resize both   
blt::table configure .top c0 -width 0.1i

blt::table .middle \
    0,0 .middle.base_l -width 1i\
    0,1 .middle.blend_l -width 1i \
    0,2 .middle.result_l -width 1i 

