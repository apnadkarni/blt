#!../src/bltwish

package require BLT

option add *Axis.loose			true
option add *x.title			X
option add *y.title			Y
option add *Axis*titleFont		{ Arial 9 } widgetDefault
option add *Element.pixels		6
option add *Element.dashes		1
option add *Legend.padY			0
option add *Button*Font			{ Arial 10 } widgetDefault
option add *Legend*Font			{ Arial 10 } widgetDefault
option add *Legend*Relief		flat
option add *BltGraph.Font		{ Arial 12 } widgetDefault
option add *BltGraph.title		"Symbol Types"
option add *BltGraph.width		4i
option add *BltGraph.height		3i
#option add *BltGraph.plotPadY		.25i
#option add *BltGraph.plotPadX		.25i

set graph .graph

blt::graph $graph -bg white -bd 0 

set img [image create picture hobbes]
$img import xbm  -file bitmaps/hobbes.xbm -maskfile bitmaps/hobbes_mask.xbm 


set attributes {
    "arrow"	arrow		brown	brown4		
    "circle"	circle		yellow	yellow4		
    "cross"	cross		cyan	cyan4		
    "diamond"	diamond		green	green4		
    "none"	none		red	red4		
    "plus"	plus		magenta	magenta4	
    "scross"	scross		red	red4		
    "splus"	splus		Purple	purple4		
    "square"	square		orange	orange4		
    "triangle"	triangle	blue	blue4		
    "@image"	"@hobbes"	white	black
}

set x [blt::vector create]
$x seq 0 1 20
set count 0
foreach {label symbol fill color} $attributes {
    set y [blt::vector create]
    $y expr sin($x*($count))
    $graph element create line${count} \
	-label $label -symbol $symbol -color $color -fill $fill -x $x -y $y
    $graph element lower line${count}
    incr count
}
$graph pen configure activeLine -symbol none -linewidth 2
button .quit -text Quit -command exit
blt::table . \
  $graph 0,0 -fill both \
  .quit  1,0 
blt::table configure . r1 -resize none -pady 4

Blt_ZoomStack $graph
Blt_Crosshairs $graph
Blt_ActiveLegend $graph
Blt_ClosestPoint $graph
Blt_PrintKey $graph
