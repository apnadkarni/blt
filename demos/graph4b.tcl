#!../src/bltwish

package require BLT

set tcl_precision 15 

set graph .graph
option add *default			normal

option add *BltHtext.font		"Times 18 bold"
option add *Text.font			"Times 18 bold"
option add *header.font			"Times 12"
option add *footer.font			"Times 18"
option add *BltGraph.relief		raised
option add *BltGraph.height		5i
option add *BltGraph.plotBackground	black
option add *BltGraph.width		7i
option add *BltGraph.halo		0
option add *BltGraph.title		"s27.out"

option add *Axis.titleColor		red2
option add *x.title			"Time"
option add *y.title			"Signals"

option add *Crosshairs.Color		white

option add *activeLine.Fill		navyblue
option add *activeLine.LineWidth	2
option add *Element.ScaleSymbols	yes
option add *Element.Smooth		natural

option add *Symbol			square
option add *Element.LineWidth		1
option add *Pen.LineWidth		1
option add *Pixels			1

option add *Grid.color			grey50
option add *Grid.dashes			"2 4"
option add *Grid.hide			no

option add *Legend.ActiveRelief		sunken
option add *Legend.Position		right
option add *Legend.Relief		flat
option add *Legend.font			"Helvetica 6"
option add *Legend.Pad			0
option add *Legend.hide			no

option add *LineMarker.Dashes		5
option add *LineMarker.Foreground	white
option add *zoomOutline.outline		yellow

option add *TextMarker.Background	{}
option add *TextMarker.Foreground	white

set table [blt::datatable create]
$table restore -file graph4a.tab

set attributes {
    V1	v1	red	red  
    V2  v2	green	red  
    V3	v3	blue	red  
    V4  v4	yellow  red  
    V5  v5	magenta red  
    V6  v6	cyan	red  
    V7	v7	white	red  
    V8  v8	red	green  
    V9  v9	green	green  
    V10 v10	blue	green  
    V11 v11	yellow	green  
    V12 v12	magenta	green  
    V13	v13	cyan	green  
    V14	v14	red	red  
    V15 v15	green	red  
    V16	v16	blue	red  
    V17 v17	yellow  red  
    V18 v18	magenta red  
    V19 v19	cyan	red  
    V20	v20	white	red  
    V21 v21	red	green  
    V22 v22	green	green  
    V23 v23	blue	green  
    V24 v24	yellow	green  
    V25 v25	magenta	green  
    V26	v26	cyan	green  
    V27	v27	red	red  
    V28 v28	green	red  
    V29	v29	blue	red  
    V30 v30	yellow  red  
    V31 v31	magenta red  
    V32 v32	cyan	red  
    V33	v33	white	red  
    V34 v34	red	green  
    V35 v35	green	green  
    V36 v36	blue	green  
    V37 v37	yellow	green  
    V38 v38	magenta	green  
    V39	v39	cyan	green  
}

text .header -wrap word -width 0 -height 6

set text {
To zoom in on a region of the graph, simply click once on the left 
mouse button to pick one corner of the area to be zoomed.  Move the 
mouse to the other corner and click again. 
} 

regsub -all "\n" $text "" text
.header insert end "$text\n"
.header insert end { You can click on the }
set im [image create picture -file ./images/qv100.t.gif]
button .header.snap -image $im -command { MakeSnapshot }
.header window create end -window .header.snap
.header insert end { button to see a picture image snapshot.}
.header configure -state disabled
blt::graph $graph 

blt::htext .footer -text {Hit the %%
    set im [image create picture -file ./images/stopsign.gif]
    button $htext(widget).quit -image $im -command { exit }
    $htext(widget) append $htext(widget).quit 
%% button when you've seen enough. %%
    label $htext(widget).logo -bitmap BLT
    $htext(widget) append $htext(widget).logo 
%%}

foreach {label yData outline color} $attributes {
    set xx [list $table "x"]
    set yy [list $table $yData]
    .graph element create $label -x $xx -y $yy -outline $outline -color $color
}

set unique 0

proc Sharpen { photo } {
    set kernel { -1 -1 -1 -1  16 -1 -1 -1 -1 } 
    #set kernel { 0 -1 0 -1  4.9 -1 0 -1 0 }
    blt::winop convolve $photo $photo $kernel
}

proc MakeSnapshot {} {
    update idletasks
    global unique
    set top ".snapshot[incr unique]"
    set im1 [image create picture]
    .graph snap $im1
    set width 210
    set height 150
    set thumb1 [image create picture -width $width -height $height -gamma 2.2]
    $thumb1 resize $im1 -filter sinc 
    image delete $im1

    set thumb2 [image create picture -window .graph -width $width \
	-height $height -filter sinc -gamma 2.2 -aspect yes]

    toplevel $top
    wm title $top "Snapshot \#$unique of \"[.graph cget -title]\""
    label $top.l1 -image $thumb1 
    label $top.l2 -image $thumb2 

    button $top.but -text "Dismiss" -command "DestroySnapshot $top"
    blt::table $top \
	0,0 $top.l1 \
	0,1 $top.l2 \
        1,0 $top.but -pady 4 
    focus $top.but
}

proc DestroySnapshot { win } {
    set im [$win.l1 cget -image]
    $im export jpg -file test.jpg
    image delete $im
    destroy $win
}

blt::table . \
    .header 0,0 -fill x \
    .graph 1,0  -fill both \
    .footer 2,0 -fill x

blt::table configure . r0 r2 -resize none

Blt_ZoomStack $graph
Blt_Crosshairs $graph
#Blt_ActiveLegend $graph
Blt_ClosestPoint $graph
Blt_PrintKey $graph

$graph element bind all <Enter> {
    %W legend activate [%W element get current]
}

$graph element bind all <Leave> {
    %W legend deactivate [%W element get current]
}

if 0 {
$table column extend "x"
$table import vector "x" 1
$table column type "x" double
set col 1
foreach vector [lsort -dictionary [blt::vector names ::v*]] {
    set name [string trim $vector ::]
    $table column extend $name
    $table column type $name double
    incr col
    $table import vector $vector $col
}

$table dump -file graph4.tab
}

blt::LegendSelections $graph
focus $graph
toplevel .top
update
$graph legend configure \
    -exportselection yes \
    -selectbackground lightblue4 \
    -selectforeground white \
    -position .top.legend
pack .top.legend -fill both -expand yes
#-nofocusselectbackground grey90
#    -nofocusselectforeground white 

proc Reset {} {
    global graph
    $graph play configure -first 0 -last 0
}

proc Next { step } {
    global graph table
    set max [$table row length]
    set last [$graph play cget -last]
    incr last $step
    $graph play configure -last $last
}

proc Prev { step } {
    global graph table
    set max [$table row length]
    set last [$graph play cget -last]
    incr last $step
    $graph play configure -last $last
}

update

proc Play {} {
    global table
    for { set i 0 } { $i < [$table row length] } { incr i 1 } {
	Next 1
	update
	after 50
    }
    for { } { $i > 0 } { incr i -1 } {
	Prev -1
	update
	after 50
    }
}
Reset
after 2000 Play
