#!../src/bltwish

package require BLT
source scripts/demo.tcl

proc random {{max 1.0} {min 0.0}} {
    global randomSeed

    set randomSeed [expr (7141*$randomSeed+54773) % 259200]
    set num  [expr $randomSeed/259200.0*($max-$min)+$min]
    return $num
}
set randomSeed 14823


set graph .graph

source scripts/stipples.tcl
source scripts/patterns.tcl


option add *x.Title			"X Axis"
option add *y.Title			"Y Axis"
option add *LineMarker.Foreground	yellow

set visual [winfo screenvisual .] 
if { $visual != "staticgray" && $visual != "grayscale" } {
    option add *print.background yellow
    option add *quit.background red
    option add *graph.background palegreen
}

blt::htext .header -text \
{   This is an example of the barchart widget.  The barchart has 
    many components; x and y axis, legend, crosshairs, elements, etc.  
    To create a postscript file "bar.ps", press the %%
    set w $htext(widget)
    button $w.print -text {Print} -command {
	$graph postscript output bar.ps 
    } 
    $w append $w.print

%% button.  
}
blt::barchart $graph 
$graph xaxis configure -rotate 90 -stepsize 0

blt::htext .footer -text {    Hit the %%
    set im [image create picture -file ./images/stopsign.gif]
    button $htext(widget).quit -image $im -command { exit }
    $htext(widget) append $htext(widget).quit -pady 2
%% button when you've seen enough. %%
    label $htext(widget).logo -bitmap BLT
    $htext(widget) append $htext(widget).logo 
%%}

set attributes { 
    red		bdiagonal1
    orange	bdiagonal2
    yellow	fdiagonal1
    green	fdiagonal2
    blue	hline1 
    cyan	hline2
    magenta	vline1 
    violetred	vline2
    purple	crossdiag
    lightblue 	hobbes	
}

set count 0
foreach { color stipple } $attributes {
    $graph pen create pen$count \
	-fill ${color}1 -outline ${color}4 -relief solid
    lappend styles [list pen$count $count $count]
    incr count
}

blt::vector x y w

x seq 0 1000 400
y expr random(x)*90.0
w expr round(y/10.0)%$count
y expr y+10.0

$graph element create data -label {} \
    -x x -y y -weight w -styles $styles

blt::table . \
    0,0 .header -fill x  \
    1,0 .graph -fill both \
    2,0 .footer -fill x

blt::table configure . r0 r2 -resize none
	
wm min . 0 0

Blt_ZoomStack $graph
Blt_Crosshairs $graph
Blt_ActiveLegend $graph
Blt_ClosestPoint $graph

