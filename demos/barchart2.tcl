#!../src/bltwish

package require BLT
source scripts/demo.tcl

proc FormatXTicks { w value } {

    # Determine the element name from the value

    set index [expr round($value)]
    if { $index != $value } {
	return $value 
    }
    incr index -1

    set name [lindex { A1 B1 A2 B2 C1 D1 C2 A3 E1 } $index]
    return $name
}

source scripts/stipples.tcl

#image create picture bgTexture -file ./images/chalk.gif

option add *Button.padX			5

option add *Radiobutton.relief		flat
option add *Radiobutton.borderWidth     2
option add *Radiobutton.highlightThickness 0
option add *Radiobutton.font		"Arial 10 bold"
option add *Htext.tileOffset		no

option add *Barchart.title		"Comparison of Simulators"

option add *x.Command			FormatXTicks
option add *x.Title			"Simulator"
option add *y.Title			"Time (hrs)"
option add *Axis.tickInterior		yes
option add *plotPadX			0
option add *plotPadY			0

option add *activeBar.Foreground	pink
option add *activeBar.stipple		dot3
option add *Element.Background		red
option add *Element.Relief		solid

option add *Grid.dashes			{ 2 4 }
option add *Grid.hide			no
option add *Grid.mapX			""

option add *Legend.activeBorderWidth	2 
option add *Legend.activeRelief		raised 
option add *Legend.anchor		ne 
option add *Legend.borderWidth		0 
option add *Legend.position		right

set visual [winfo screenvisual .] 
if { $visual != "staticgray" && $visual != "grayscale" } {
    option add *print.background	yellow
    option add *quit.background		red
    option add *quit.activeBackground	red2
}

blt::htext .title -text {\
 Data points with like x-coordinates, can have their bar segments displayed
 in one of the following modes (using the -barmode option):
}
blt::htext .header -text {
    %%
    blt::tk::radiobutton .header.stacked -text stacked -variable barMode \
	-anchor w -value "stacked" -command {
            .graph configure -barmode $barMode
        } 
    .header append .header.stacked -width 1.0i -anchor w
    %%      Bars are stacked on top of each other. The overall height is the     
                                                   sum of the y-coordinates. 
    %% 
    blt::tk::radiobutton .header.aligned -text aligned -variable barMode \
	-anchor w -value "aligned" -command {
            .graph configure -barmode $barMode
        }
    .header append .header.aligned -width 1.0i -fill x
    %%      Bars are drawn side-by-side at a fraction of their normal width. 
    %%
    blt::tk::radiobutton .header.overlap -text "overlap" -variable barMode \
	-anchor w -value "overlap" -command {
            .graph configure -barmode $barMode
        } 
    .header append .header.overlap -width 1.0i -fill x
    %%      Bars overlap slightly. 
    %%
    blt::tk::radiobutton .header.normal -text "normal" -variable barMode \
	-anchor w -value "normal" -command {
            .graph configure -barmode $barMode
        } 
    .header append .header.normal -width 1.0i -fill x
    %%      Bars are overlayed one on top of the next. 
}

blt::htext .footer -text { Hit the %%
    set im [image create picture -file ./images/stopsign.gif]
    button $htext(widget).quit -image $im -command { exit }
    $htext(widget) append $htext(widget).quit -pady 2
%% button when you've seen enough. %%
    label $htext(widget).logo -bitmap BLT
    $htext(widget) append $htext(widget).logo 
%%}

if 0 {
set file ./images/jan25_palm3x_L.jpg
set file ./images/blt98.gif
set file ./images/folder.gif
image create picture bgTexture -file $file 
    set style [blt::background create tile \
		   -image bgTexture \
		   -relativeto self]
} else {
    set style [blt::background create linear \
		   -highcolor grey60 -lowcolor grey85 \
		   -relativeto self \
		   -colorscale log]
}

blt::barchart .graph -bg $style  -barwidth 0.9

blt::vector X Y0 Y1 Y2 Y3 Y4

X set { 1 2 3 4 5 6 7 8 9 }
Y0 set { 
    0.729111111  0.002250000  0.09108333  0.006416667  0.026509167 
    0.007027778  0.1628611    0.06405278  0.08786667  
}
Y1 set {
    0.003120278	 0.004638889  0.01113889  0.048888889  0.001814722
    0.291388889  0.0503500    0.13876389  0.04513333 
}
Y2 set {
    11.534444444 3.879722222  4.54444444  4.460277778  2.334055556 
    1.262194444  1.8009444    4.12194444  3.24527778  
}
Y3 set {
    1.015750000  0.462888889  0.49394444  0.429166667  1.053694444
    0.466111111  1.4152500    2.17538889  2.55294444 
}
Y4 set {
    0.022018611  0.516333333  0.54772222  0.177638889  0.021703889 
    0.134305556  0.5189278    0.07957222  0.41155556  
}


#
# Element attributes:  
#
#    Label     yData	Foreground	Background	Stipple	    Borderwidth
set attributes { 
    "Setup"	Y1	lightyellow3	lightyellow1	fdiagonal1	1
    "Read In"	Y0	lightgoldenrod3	lightgoldenrod1	bdiagonal1	1
    "Other"	Y4	lightpink3	lightpink1	fdiagonal1	1
    "Solve"	Y3	cyan3		cyan1		bdiagonal1	1
    "Load"	Y2	lightblue3	lightblue1	fdiagonal1	1
}


foreach {label yData fg bg stipple bd} $attributes {
#     set img [image create picture -width 5 -height 5]
#     foreach {r g b} [winfo rgb . $bg] break
#     set r [expr int($r / 257.0)]
#     set g [expr int($g / 257.0)]
#     set b [expr int($b / 257.0)]
#     set color [format "0xA0%0.2x%0.2x%0.2x" $r $g $b]
#     puts stderr color=$color
#     $img blank $color
    .graph element create -label $label -bd $bd -relief raised \
	-y $yData -x X -outline $fg -fill $bg  -opacity 50.0
}
.header.stacked invoke

blt::table . \
    0,0 .title -fill x \
    1,0 .header -fill x  \
    2,0 .graph -fill both \
    3,0 .footer -fill x

blt::table configure . r0 r1 r3 -resize none

Blt_ZoomStack .graph
Blt_Crosshairs .graph
Blt_ActiveLegend .graph
Blt_ClosestPoint .graph

.graph marker bind all <B2-Motion> {
    set coords [%W invtransform %x %y]
    catch { %W marker configure [%W marker get current] -coords $coords }
}

.graph marker bind all <Enter> {
    set marker [%W marker get current]
    catch { %W marker configure $marker -bg green}
}

.graph marker bind all <Leave> {
    set marker [%W marker get current]
    catch { %W marker configure $marker -bg ""}
}

.graph element bind all <Enter> {
    set data [.graph element nearest %x %y]
    if { $data != "" } {
	array set info $data
	puts stderr "$info(x) $info(y)"
    }
}
