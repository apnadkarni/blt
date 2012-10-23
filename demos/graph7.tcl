#!../src/bltwish


set blt_library ../library
package require BLT
set blt_library ../library
set auto_path [linsert $auto_path 0 ../library]
source scripts/demo.tcl

option add *TileOffset			0
option add *HighlightThickness		0
option add *Element.ScaleSymbols	no
option add *Element.Smooth		linear
option add *activeLine.Color		yellow4
option add *activeLine.Fill		yellow
option add *activeLine.LineWidth	0
option add *Element.Pixels		3
option add *BltGraph.halo			7i

set visual [winfo screenvisual .] 
if { $visual != "staticgray" } {
    option add *print.background yellow
    option add *quit.background red
}

proc FormatLabel { w value } {
    return $value
}

set graph .graph

set s1 [image create picture -width 25 -height 25]
$s1 blank 0x00000000
$s1 draw circle 12 12 5 -shadow 0 -linewidth 1 \
	-fill 0x90FF0000 -antialias yes 

set length 2500000
blt::graph $graph -title "Scatter Plot\n$length points"  -font Arial \
    -plotborderwidth 1 -plotrelief solid  -plotpadx 0 -plotpady 0
$graph xaxis configure \
    -loose no \
    -title "X Axis Label" 
$graph yaxis configure \
    -title "Y Axis Label" 
$graph y2axis configure \
    -title "Y2 Axis Label"  
$graph legend configure \
    -activerelief sunken \
    -background ""

$graph element create line3 -symbol circle -color green4 -fill green2 \
    -linewidth 0 -outlinewidth 1 -pixels 4
blt::table . .graph 0,0  -fill both
update

blt::vector x($length) y($length)
x expr random(x)
y expr random(y)
x sort y
$graph element configure line3 -x x -y y

wm min . 0 0

Blt_ZoomStack $graph
Blt_Crosshairs $graph
Blt_ActiveLegend $graph
Blt_ClosestPoint $graph

blt::busy hold $graph
update
blt::busy release $graph

