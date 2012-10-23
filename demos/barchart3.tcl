#!../src/bltwish

package require BLT

source scripts/demo.tcl
source scripts/stipples.tcl
source scripts/patterns.tcl

option add *graph.xTitle "X Axis Label"
option add *graph.yTitle "Y Axis Label"
option add *graph.title "A Simple Barchart"
option add *graph.xFont *Times-Medium-R*12*
option add *graph.elemBackground white
option add *graph.elemRelief raised

set visual [winfo screenvisual .] 
if { $visual != "staticgray" && $visual != "grayscale" } {
    option add *print.background yellow
    option add *quit.background red
}

blt::htext .header -text {
This is an example of the barchart widget.  To create a postscript 
file "bar.ps", press the %% 
button $htext(widget).print -text {Print} -command {
  $graph postscript output bar.ps  -maxpect 1
} 
$htext(widget) append $htext(widget).print
%% button.}

set graph [blt::barchart .b]
$graph configure \
    -invert false \
    -baseline 1.2
$graph xaxis configure \
    -command FormatLabel \
    -descending no
$graph legend configure \
    -hide yes

blt::htext .footer -text {Hit the %%
    button $htext(widget).quit -text quit -command exit
    $htext(widget) append $htext(widget).quit 
%% button when you've seen enough.%%
    label $htext(widget).logo -bitmap BLT
    $htext(widget) append $htext(widget).logo -padx 20
%%}

set data { 
    One     1  1  red	   green     bdiagonal1	raised
    Two     2  2  green	   blue      bdiagonal2	raised
    Three   3  3  blue	   purple    checker2	raised
    Four    4  4  purple   orange    checker3	raised
    Five    5  5  orange   brown     cross1	raised
    Six     6  6  brown	   cyan      cross2	raised
    Seven   7  7  cyan	   navy      cross3	raised
    Eight   8  8  navy	   red       crossdiag	raised
    Nine    9 -1  pink     black      ""		solid
    Ten	   10 -2  seagreen palegreen hobbes	raised
    Eleven 11 -3  blue     blue4     ""		raised
}

foreach { name x y fg bg bitmap relief }  $data {
    $graph element create $name \
	-data { $x $y } -fg $fg -bg $bg -stipple $bitmap  \
	-relief $relief -bd 2 
    set labels($x) $name
}

$graph marker create bitmap \
    -coords { 4 0.3 } -anchor center \
    -bitmap @bitmaps/sharky.xbm \
    -name bitmap -fill ""

$graph marker create text \
    -coords { 10 5.3 } -anchor center \
    -text "Hi there" \
    -name text  -rotate 45 -font "Arial 14"  -fg blue

$graph marker create polygon \
    -coords { 5 0 7 2  10 10  10 2 } \
    -name poly -linewidth 1 -fill "" -outline red4 -under yes

blt::table . \
    .header 0,0 -padx .25i \
    $graph 1,0 -fill both \
    .footer 2,0 -padx .25i  

blt::table configure . r0 r2 -resize none

wm min . 0 0

proc FormatLabel { w value } {
    global labels

    set value [expr int(round($value))]
    # Determine the element name from the value
    if { [info exists labels($value)] } {
	return "$labels($value)"
    } 
    return "$value"
}

Blt_ZoomStack $graph
Blt_Crosshairs $graph
Blt_ActiveLegend $graph
Blt_ClosestPoint $graph

$graph marker bind all <B3-Motion> {
    set coords [%W invtransform %x %y]
    catch { %W marker configure [%W marker get current] -coords $coords }
}

$graph marker bind all <Enter> {
    set marker [%W marker get current]
    catch { %W marker configure $marker -fill green3 }
}

$graph marker bind all <Leave> {
    set marker [%W marker get current]
    catch { %W marker configure $marker -fill "" }
}

