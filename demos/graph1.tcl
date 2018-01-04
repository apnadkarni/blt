#!../src/bltwish

if { [info exists env(BLT_LIBRARY)] } {
   lappend auto_path $env(BLT_LIBRARY)
}
package require BLT

option add *BltComboButton.relief flat
option add *BltComboButton.foreground grey33
option add *BltComboButton.arrowOn true

set normalBg [blt::background create linear \
		  -highcolor grey80 -lowcolor grey98 -jitter 3]

option add *BltComboButton.background $normalBg
option add *BltComboButton.font "Arial 10 italic"
option add *BltTkFrame.background $normalBg
option add *BltTkLabel.background $normalBg
option add *BltHtext.background $normalBg
option add *BltTabset.selectBackground $normalBg
set xbg [blt::background create linear -relativeto self\
		  -highcolor grey80 -lowcolor grey98 -jitter 3]
option add *BltComboMenu.background $xbg

blt::tk::frame .f 

set g .f.g
blt::graph $g \
    -bg white \
    -borderwidth 1 \
    -height 600 \
    -plotborderwidth 1 \
    -plotrelief solid \
    -plotpadx 0 \
    -plotpady 0 \
    -relief raised \
    -title "A Simple X-Y Graph" \
    -width 700 

$g legend configure \
    -activeborderwidth 1  \
    -activerelief sunken \
    -anchor ne \
    -background "" \
    -borderwidth 0 \
    -padx 10 \
    -pady 10 \
    -position plotarea \
    -relief flat 

$g axis configure x \
    -activeforeground red3 \
    -tickdirection in \
    -scale linear \
    -scrollcommand { .f.xbar set }  \
    -title "X" 

$g axis configure y \
    -activeforeground red3 \
    -tickdirection in \
    -rotate 0 \
    -scale linear \
    -scrollcommand { .f.ybar set } \
    -scrollmax 1000 \
    -scrollmin -100  \
    -title "Y" 

$g axis configure y2 \
    -tickdirection in \
    -hide no \
    -rotate 0 \
    -scrollmax 1.0 \
    -scrollmin 0.0 \
    -title "Y2"

$g axis configure x2 \
    -tickdirection in \
    -hide no \
    -rotate 0 \
    -scrollmax 1.0 \
    -scrollmin 0.0 \
    -title "X2"

blt::htext .f.header \
    -text \
    {This is a graph widget. It plots 2D data with assorted
line attributes and symbols. Press the %%
    blt::tk::button $htext(widget).print -text print -command {
        puts stderr [time {
	    blt::busy hold .
	    update
	    $g postscript output demo1.eps  -width 5i -height 5i
	    update
	    blt::busy release .
	    update
        }]
    } 
    $htext(widget) append $htext(widget).print
%% button
to create a PostScript file "xy.ps".}


set X { 
    2.00000e-01 4.00000e-01 6.00000e-01 8.00000e-01 1.00000e+00 
    1.20000e+00 1.40000e+00 1.60000e+00 1.80000e+00 2.00000e+00 
    2.20000e+00 2.40000e+00 2.60000e+00 2.80000e+00 3.00000e+00 
    3.20000e+00 3.40000e+00 3.60000e+00 3.80000e+00 4.00000e+00 
    4.20000e+00 4.40000e+00 4.60000e+00 4.80000e+00 5.00000e+00 
} 

set Y1 { 
    4.07008e+01 7.95658e+01 1.16585e+02 1.51750e+02 1.85051e+02 
    2.16479e+02 2.46024e+02 2.73676e+02 2.99427e+02 3.23267e+02 
    3.45187e+02 3.65177e+02 3.83228e+02 3.99331e+02 4.13476e+02 
    4.25655e+02 4.35856e+02 4.44073e+02 4.50294e+02 4.54512e+02 
    4.56716e+02 4.57596e+02 4.58448e+02 4.59299e+02 4.60151e+02 
}

set Y2 { 
    5.14471e-00 2.09373e+01 2.84608e+01 3.40080e+01 3.75691e+01
    3.91345e+01 3.92706e+01 3.93474e+01 3.94242e+01 3.95010e+01 
    3.95778e+01 3.96545e+01 3.97313e+01 3.98081e+01 3.98849e+01 
    3.99617e+01 4.00384e+01 4.01152e+01 4.01920e+01 4.02688e+01 
    4.03455e+01 4.04223e+01 4.04990e+01 4.05758e+01 4.06526e+01 
}

set Y3 { 
    2.61825e+01 5.04696e+01 7.28517e+01 9.33192e+01 1.11863e+02 
    1.28473e+02 1.43140e+02 1.55854e+02 1.66606e+02 1.75386e+02 
    1.82185e+02 1.86994e+02 1.89802e+02 1.90683e+02 1.91047e+02 
    1.91411e+02 1.91775e+02 1.92139e+02 1.92503e+02 1.92867e+02 
    1.93231e+02 1.93595e+02 1.93958e+02 1.94322e+02 1.94686e+02 
}

set configOptions {
    Element.Pixels		6
    Element.Smooth		none
    activeLine.Color		yellow4
    activeLine.Fill		yellow
    background			khaki3
    line1.Color			red4
    line1.Fill			red1
    line1.Symbol		splus
    line2.Color			purple4
    line2.Fill			purple1
    line2.Symbol		arrow
    line3.Color			green4
    line3.Fill			green1
    line3.Symbol		triangle
    x.Descending		no
    x.Loose			no
    x.Title			"X Axis Label"
    y.Rotate			0
    y.Title			"Y Axis Label" 
}

set resource [string trimleft $g .]
foreach { option value } $configOptions {
    option add *$resource.$option $value
}
$g element create -x $X -y $Y2 
$g element create -x $X -y $Y3 
$g element create -x $X -y $Y1 

Blt_ZoomStack $g
Blt_Crosshairs $g
#Blt_ActiveLegend $g
Blt_ClosestPoint $g

blt::htext .f.footer \
    -text {Hit the %%
blt::tk::button $htext(widget).quit -text quit -command { exit } 
$htext(widget) append $htext(widget).quit 
%% button when you've seen enough.%%
blt::tk::label $htext(widget).logo -bitmap BLT
$htext(widget) append $htext(widget).logo 
%%}

proc MultiplexView { args } { 
    global g
    eval $g axis view y $args
    eval $g axis view y2 $args
}

blt::tk::scrollbar .f.xbar \
    -command { $g axis view x } \
    -orient horizontal \
    -highlightthickness 0

blt::tk::scrollbar .f.ybar \
    -command MultiplexView \
    -orient vertical -highlightthickness 0

blt::table .f \
    0,0 .f.header -cspan 3 -fill x \
    1,0 $g -fill both \
    1,1 .f.ybar -fill y  -padx 0 -pady 0 \
    2,0 .f.xbar -fill x \
    3,0 .f.footer -cspan 3 -fill x

blt::table configure .f c1 -resize none
blt::table configure .f c2 -resize both
#blt::table configure .f c3 r0 r4 r5 -resize none

blt::table . \
    0,0 .f -fill both

$g pen configure "activeLine" \
    -showvalues y
$g element bind all <Enter> {
    %W legend activate current
}
$g element bind all <Leave> {
    %W legend deactivate
}
$g axis bind all <Enter> {
    set axis [%W axis get current]
    %W axis activate $axis
    %W axis focus $axis
}
$g axis bind all <Leave> {
    set axis [%W axis get current]
    %W axis deactivate $axis
    %W axis focus ""
}
$g configure -leftvariable left 
#trace variable left w [list UpdateTable $g]
proc UpdateTable { g p1 p2 how } {
    blt::table configure .f c0 -width  [$g extents leftmargin]
    blt::table configure .f c2 -width  [$g extents rightmargin]
    blt::table configure .f r1 -height [$g extents topmargin]
    blt::table configure .f r3 -height [$g extents bottommargin]
}

set image1 [image create picture -file bitmaps/sharky.xbm]
set image2 [image create picture -file images/buckskin.gif]

set bg1 [blt::paintbrush create color -color blue -opacity 20]
set bg2 [blt::paintbrush create color -color green -opacity 20]
set bg3 [blt::paintbrush create color -color red -opacity 20]

#$g element configure line3 -areabackground $bg3 -areaforeground blue 
$g element configure line2 -areabackground $bg2 -areaforeground blue 
$g element configure line1 -areabackground $bg1 -areaforeground blue 

$g marker create line -name "y100" -coords { -Inf 100 Inf 100 } -dashes 1 \
	-outline green3 -linewidth 1

if { $tcl_platform(platform) == "windows" } {
    if 0 {
        set name [lindex [blt::printer names] 0]
        set printer {Lexmark Optra E310}
	blt::printer open $printer
	blt::printer getattrs $printer attrs
	puts $attrs(Orientation)
	set attrs(Orientation) Landscape
	set attrs(DocumentName) "This is my print job"
	blt::printer setattrs $printer attrs
	blt::printer getattrs $printer attrs
	puts $attrs(Orientation)
	after 5000 {
	    $g print2 $printer
	    blt::printer close $printer
	}
    } else {
	after 5000 {
	    $g print2 
	}
    }	
    if 1 {
	after 2000 {
		
	    $g snap CLIPBOARD -format emf
	}
    }
}

focus $g
$g xaxis bind <Left>  { 
    $g xaxis view scroll -1 units 
} 

$g xaxis bind <Right> { 
    $g xaxis view scroll 1 units 
}

$g yaxis bind <Up>  { 
    $g yaxis view scroll -1 units 
} 

$g yaxis bind <Down> { 
    $g yaxis view scroll 1 units 
}

$g y2axis bind <Up>  { 
    $g y2axis view scroll -1 units 
} 

$g y2axis bind <Down> { 
    $g y2axis view scroll 1 units 
}

$g axis bind all <ButtonPress-1> { 
    set b1(x) %x
    set b1(y) %y
    set axis [%W axis get current]
    %W axis activate $axis
}
$g axis bind all <ButtonRelease-1> { 
    set b1(x) %x
    set b1(y) %y
    set axis [%W axis get current]
    %W axis deactivate $axis
#    %W axis focus ""
}

$g xaxis bind <B1-Motion> { 
    set dist [expr %x - $b1(x)]
    $g xaxis view scroll $dist pixels
    set b1(x) %x
}

$g yaxis bind <B1-Motion> { 
    set dist [expr %y - $b1(y)]
    $g yaxis view scroll $dist pixels
    set b1(y) %y
}

blt::Graph::InitLegend $g
$g legend configure -selectmode multiple


proc FixAxes { g option value } {
    global axisd
    parray axisd
    foreach a [$g axis names $axisd(axis)] {
	$g axis configure $a $option $value
    }
}

proc AxisOptions { w } {
    global axisd g
    $w insert end "Axis" 
    set t [blt::tk::frame $w.axis]
    $w tab configure "Axis" -window $w.axis

    blt::tk::label $t.axis_l -text  "Select Axis:" 
    blt::combobutton $t.axis -textvariable axisd(-axis) \
	-menu $t.axis.m -command "puts hi" 
    set m [blt::combomenu $t.axis.m -textvariable axisd(-axis)]
    foreach axis [$g axis names] {
	$m add -type radiobutton -text $axis -value $axis 
    }
    $m add -type radiobutton -text "all" -value "*" 
    $m item configure all -variable axisd(axis)

    blt::tk::label $t.tickdirection_l -text  "-tickdirection" 
    blt::combobutton $t.tickdirection -textvariable axisd(-tickdirection) \
	-menu $t.tickdirection.m
    set m [blt::combomenu $t.tickdirection.m]
    $m add -type radiobutton -text "in" 
    $m add -type radiobutton -text "out"
    $m item configure all -variable axisd(-tickdirection) \
	-command { FixAxes $g -tickdirection $axisd(-tickdirection) }

    blt::tk::label $t.color_l -text  "-color" 
    blt::combobutton $t.color -textvariable axisd(-color) \
	-menu $t.color.m
    set m [blt::combomenu $t.color.m]
    $m add -type radiobutton -text "black" 
    $m add -type radiobutton -text "blue" 
    $m add -type radiobutton -text "green"
    $m add -type radiobutton -text "red" 
    $m add -type radiobutton -text "white" 
    $m add -type radiobutton -text "yellow" 
    $m item configure all -variable axisd(-color) \
	-command { FixAxes $g -color $axisd(-color) }

    blt::tk::label $t.linewidth_l -text  "-linewidth" 
    blt::combobutton $t.linewidth -textvariable axisd(-linewidth) \
	-menu $t.linewidth.m
    set m [blt::combomenu $t.linewidth.m]
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10" 
    $m item configure all -variable axisd(-linewidth) \
	-command { FixAxes $g -linewidth $axisd(-linewidth) }

    blt::tk::label $t.showticks_l -text  "-showticks" 
    blt::combobutton $t.showticks -textvariable axisd(-showticks) \
	-menu $t.showticks.m
    set m [blt::combomenu $t.showticks.m]
    $m add -type radiobutton -text "yes" 
    $m add -type radiobutton -text "no" 
    $m item configure all -variable axisd(-showticks) \
	-command { FixAxes $g -showticks $axisd(-showticks) }

    blt::tk::label $t.hide_l -text  "-hide" 
    blt::combobutton $t.hide -textvariable axisd(-hide) \
	-menu $t.hide.m
    set m [blt::combomenu $t.hide.m]
    $m add -type radiobutton -text "yes" 
    $m add -type radiobutton -text "no" 
    $m item configure all -variable axisd(-hide) \
	-command { FixAxes $g -hide $axisd(-hide) }


    blt::tk::label $t.loose_l -text  "-loose" 
    blt::combobutton $t.loose -textvariable axisd(-loose) \
	-menu $t.loose.m
    set m [blt::combomenu $t.loose.m]
    $m add -type radiobutton -text "yes" 
    $m add -type radiobutton -text "no" 
    $m add -type radiobutton -text "always" 
    $m item configure all -variable axisd(-loose) \
	-command { FixAxes $g -loose $axisd(-loose) }
    
    blt::tk::label $t.title_l -text  "-title" 
    blt::combobutton $t.title -textvariable axisd(-title) \
	-menu $t.title.m
    set m [blt::combomenu $t.title.m]
    $m add -type radiobutton -text "title1" 
    $m add -type radiobutton -text "Title2" 
    $m add -type radiobutton -text "none" -value ""
    $m item configure all -variable axisd(-title) \
	-command { FixAxes $g -title $axisd(-title) }
    $m select 0
    $t.axis.m select 0
    foreach option { color tickdirection showticks linewidth loose title hide } {
	set value [$g axis cget $axisd(axis) -$option]
	set axisd(-$option) $value
    }
    blt::table $t \
	0,0 $t.axis_l -anchor e \
	0,1 $t.axis -fill x \
	2,0 $t.color_l -anchor e \
	2,1 $t.color -fill x \
	3,0 $t.tickdirection_l -anchor e \
	3,1 $t.tickdirection -fill x \
	4,0 $t.hide_l -anchor e \
	4,1 $t.hide -fill x \
	5,0 $t.linewidth_l -anchor e \
	5,1 $t.linewidth -fill x \
	6,0 $t.loose_l -anchor e \
	6,1 $t.loose -fill x \
	7,0 $t.showticks_l -anchor e \
	7,1 $t.showticks -fill x  \
	8,0 $t.title_l -anchor e \
	8,1 $t.title -fill x 
    blt::table configure $t c0 -padx { 0 1i }
    blt::table configure $t r0 -pady 8
    blt::table configure $t r* -resize none
    blt::table configure $t r9 -resize both
}

proc GraphOptions { w } {
    global graphd g
    $w insert end "Graph" 
    set t [blt::tk::frame $w.graph]
    $w tab configure "Graph" -window $w.graph
    blt::tk::label $t.plotborderwidth_l -text  "-plotborderwidth" 
    blt::combobutton $t.plotborderwidth -textvariable graphd(-plotborderwidth) \
	-menu $t.plotborderwidth.m
    set m [blt::combomenu $t.plotborderwidth.m]
    $m add -type radiobutton -text [$g cget -plotborderwidth]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10" 
    $m item configure all -variable graphd(-plotborderwidth) \
	-command { $g configure -plotborderwidth $graphd(-plotborderwidth) }

    blt::tk::label $t.borderwidth_l -text  "-borderwidth" 
    blt::combobutton $t.borderwidth -textvariable graphd(-borderwidth) \
	-menu $t.borderwidth.m
    set m [blt::combomenu $t.borderwidth.m]
    $m add -type radiobutton -text [$g cget -borderwidth]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10" 
    $m item configure all -variable graphd(-borderwidth) \
	-command { $g configure -borderwidth $graphd(-borderwidth) }

    blt::tk::label $t.plotpady_l -text  "-plotpady" 
    blt::combobutton $t.plotpady -textvariable graphd(-plotpady) \
	-menu $t.plotpady.m
    set m [blt::combomenu $t.plotpady.m]
    $m add -type radiobutton -text [$g cget -plotpady]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10"
    $m item configure all -variable graphd(-plotpady) \
	-command { $g configure -plotpady $graphd(-plotpady) }

    blt::tk::label $t.plotpadx_l -text  "-plotpadx" 
    blt::combobutton $t.plotpadx -textvariable graphd(-plotpadx) \
	-menu $t.plotpadx.m
    set m [blt::combomenu $t.plotpadx.m]
    $m add -type radiobutton -text [$g cget -plotpadx]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10" 
    $m item configure all -variable graphd(-plotpadx) \
	-command { $g configure -plotpadx $graphd(-plotpadx) }

    blt::tk::label $t.plotrelief_l -text  "-plotrelief" 
    blt::combobutton $t.plotrelief -textvariable graphd(-plotrelief) \
	-menu $t.plotrelief.m
    set m [blt::combomenu $t.plotrelief.m]
    $m add -type radiobutton -text [$g cget -plotrelief]
    $m add -type separator
    $m add -type radiobutton -text "flat" 
    $m add -type radiobutton -text "groove" 
    $m add -type radiobutton -text "raised" 
    $m add -type radiobutton -text "ridge" 
    $m add -type radiobutton -text "solid" 
    $m add -type radiobutton -text "sunken"
    $m item configure all -variable graphd(-plotrelief) \
	-command { $g configure -plotrelief $graphd(-plotrelief) }

    blt::tk::label $t.relief_l -text  "-relief" 
    blt::combobutton $t.relief -textvariable graphd(-relief) \
	-menu $t.relief.m
    set m [blt::combomenu $t.relief.m]
    $m add -type radiobutton -text [$g cget -relief]
    $m add -type separator
    $m add -type radiobutton -text "flat" 
    $m add -type radiobutton -text "groove"
    $m add -type radiobutton -text "raised"
    $m add -type radiobutton -text "ridge"
    $m add -type radiobutton -text "solid" 
    $m add -type radiobutton -text "sunken"
    $m item configure all -variable graphd(-relief) \
	-command { $g configure -relief $graphd(-relief) }

    blt::tk::label $t.plotwidth_l -text  "-plotwidth" 
    blt::combobutton $t.plotwidth -textvariable graphd(-plotwidth) \
	-menu $t.plotwidth.m
    set m [blt::combomenu $t.plotwidth.m]
    $m add -type radiobutton -text [$g cget -plotwidth]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "100"
    $m add -type radiobutton -text "200"
    $m add -type radiobutton -text "2i" 
    $m add -type radiobutton -text "4i" 
    $m add -type radiobutton -text "8i" 
    $m item configure all -variable graphd(-plotwidth) \
	-command { $g configure -plotwidth $graphd(-plotwidth) }

    blt::tk::label $t.plotheight_l -text  "-plotheight" 
    blt::combobutton $t.plotheight -textvariable graphd(-plotheight) \
	-menu $t.plotheight.m
    set m [blt::combomenu $t.plotheight.m]
    $m add -type radiobutton -text [$g cget -plotheight]
    $m add -type separator
    $m add -type radiobutton -text "0"
    $m add -type radiobutton -text "100"
    $m add -type radiobutton -text "200"
    $m add -type radiobutton -text "2i" 
    $m add -type radiobutton -text "4i" 
    $m add -type radiobutton -text "8i" 
    $m item configure all -variable graphd(-plotheight) \
	-command { $g configure -plotheight $graphd(-plotheight) }


    blt::tk::label $t.width_l -text  "-width" 
    blt::combobutton $t.width -textvariable graphd(-width) \
	-menu $t.width.m
    set m [blt::combomenu $t.width.m]
    $m add -type radiobutton -text [$g cget -width]
    $m add -type separator
    $m add -type radiobutton -text "0"
    $m add -type radiobutton -text "100" 
    $m add -type radiobutton -text "2.5i"
    $m add -type radiobutton -text "4i"
    $m add -type radiobutton -text "6i"
    $m add -type radiobutton -text "8.5i" 
    $m item configure all -variable graphd(-width) \
	-command { $g configure -width $graphd(-width) }

    blt::tk::label $t.height_l -text  "-height" 
    blt::combobutton $t.height -textvariable graphd(-height) \
	-menu $t.height.m
    set m [blt::combomenu $t.height.m]
    $m add -type radiobutton -text [$g cget -height]
    $m add -type separator
    $m add -type radiobutton -text "0"
    $m add -type radiobutton -text "200" 
    $m add -type radiobutton -text "2.5i"
    $m add -type radiobutton -text "4i"
    $m add -type radiobutton -text "6i"
    $m add -type radiobutton -text "8.5i"
    $m item configure all -variable graphd(-height) \
	-command { $g configure -height $graphd(-height) }

    blt::tk::label $t.plotbackground_l -text  "-plotbackground" 
    blt::combobutton $t.plotbackground -textvariable graphd(-plotbackground) \
	-menu $t.plotbackground.m
    set m [blt::combomenu $t.plotbackground.m]
    $m add -type radiobutton -text [$g cget -plotbackground]
    $m add -type separator
    $m add -type radiobutton -text "black"
    $m add -type radiobutton -text "blue" 
    $m add -type radiobutton -text "green"
    $m add -type radiobutton -text "grey" 
    $m add -type radiobutton -text "red" 
    $m add -type radiobutton -text "white" 
    $m add -type radiobutton -text "yellow"
    $m item configure all -variable graphd(-plotbackground) \
	-command { $g configure -plotbackground $graphd(-plotbackground) }

    blt::tk::label $t.background_l -text  "-background" 
    blt::combobutton $t.background -textvariable graphd(-background) \
	-menu $t.background.m
    set m [blt::combomenu $t.background.m]
    $m add -type radiobutton -text [$g cget -background]
    $m add -type separator
    $m add -type radiobutton -text "black" 
    $m add -type radiobutton -text "blue" 
    $m add -type radiobutton -text "green" 
    $m add -type radiobutton -text "grey" 
    $m add -type radiobutton -text "red" 
    $m add -type radiobutton -text "white" 
    $m add -type radiobutton -text "yellow"
    $m item configure all -variable graphd(-background) \
	-command { $g configure -background $graphd(-background) }

    blt::tk::label $t.title_l -text  "-title" 
    blt::combobutton $t.title -textvariable graphd(-title) \
	-menu $t.title.m
    set m [blt::combomenu $t.title.m]
    $m add -type radiobutton -text [$g cget -title]
    $m add -type separator
    $m add -type radiobutton -text "title1" 
    $m add -type radiobutton -text "Title2" 
    $m add -type radiobutton -text "none" -value ""
    $m item configure all -variable graphd(-title) \
	-command { $g configure -title $graphd(-title) }

    blt::tk::label $t.leftmargin_l -text  "-leftmargin" 
    blt::combobutton $t.leftmargin -textvariable graphd(-leftmargin) \
	-menu $t.leftmargin.m
    set m [blt::combomenu $t.leftmargin.m]
    $m add -type radiobutton -text [$g cget -leftmargin]
    $m add -type separator
    $m add -type radiobutton -text "0"
    $m add -type radiobutton -text "10" 
    $m add -type radiobutton -text ".25i"
    $m add -type radiobutton -text ".5i"
    $m add -type radiobutton -text "1.0i"
    $m add -type radiobutton -text "2.0i" 
    $m item configure all -variable graphd(-leftmargin) \
	-command { $g configure -leftmargin $graphd(-leftmargin) }

    blt::tk::label $t.rightmargin_l -text  "-rightmargin" 
    blt::combobutton $t.rightmargin -textvariable graphd(-rightmargin) \
	-menu $t.rightmargin.m
    set m [blt::combomenu $t.rightmargin.m]
    $m add -type radiobutton -text [$g cget -rightmargin]
    $m add -type separator
    $m add -type radiobutton -text "0"
    $m add -type radiobutton -text "10" 
    $m add -type radiobutton -text ".25i"
    $m add -type radiobutton -text ".5i"
    $m add -type radiobutton -text "1.0i"
    $m add -type radiobutton -text "2.0i" 
    $m item configure all -variable graphd(-rightmargin) \
	-command { $g configure -rightmargin $graphd(-rightmargin) }

    blt::tk::label $t.topmargin_l -text  "-topmargin" 
    blt::combobutton $t.topmargin -textvariable graphd(-topmargin) \
	-menu $t.topmargin.m
    set m [blt::combomenu $t.topmargin.m]
    $m add -type radiobutton -text [$g cget -topmargin]
    $m add -type separator
    $m add -type radiobutton -text "0"
    $m add -type radiobutton -text "10" 
    $m add -type radiobutton -text ".25i"
    $m add -type radiobutton -text ".5i"
    $m add -type radiobutton -text "1.0i"
    $m add -type radiobutton -text "2.0i" 
    $m item configure all -variable graphd(-topmargin) \
	-command { $g configure -topmargin $graphd(-topmargin) }

    blt::tk::label $t.bottommargin_l -text  "-bottommargin" 
    blt::combobutton $t.bottommargin -textvariable graphd(-bottommargin) \
	-menu $t.bottommargin.m
    set m [blt::combomenu $t.bottommargin.m]
    $m add -type radiobutton -text [$g cget -bottommargin]
    $m add -type separator
    $m add -type radiobutton -text "0"
    $m add -type radiobutton -text "10" 
    $m add -type radiobutton -text ".25i"
    $m add -type radiobutton -text ".5i"
    $m add -type radiobutton -text "1.0i"
    $m add -type radiobutton -text "2.0i" 
    $m item configure all -variable graphd(-bottommargin) \
	-command { $g configure -bottommargin $graphd(-bottommargin) }

    foreach option { borderwidth plotrelief relief background
	plotbackground background plotborderwidth plotpadx plotpady 
	plotwidth plotheight width height title rightmargin leftmargin
	topmargin bottommargin } {
	$t.$option.m select 0
    }

    blt::table $t \
	1,0 $t.background_l -anchor e \
	1,1 $t.background -fill x  \
	2,0 $t.borderwidth_l -anchor e \
	2,1 $t.borderwidth -fill x \
	3,0 $t.bottommargin_l -anchor e \
	3,1 $t.bottommargin -fill x  \
	4,0 $t.height_l -anchor e \
	4,1 $t.height -fill x \
	5,0 $t.leftmargin_l -anchor e \
	5,1 $t.leftmargin -fill x  \
	6,0 $t.plotbackground_l -anchor e \
	6,1 $t.plotbackground -fill x  \
	7,0 $t.plotborderwidth_l -anchor e \
	7,1 $t.plotborderwidth -fill x  \
	8,0 $t.plotheight_l -anchor e \
	8,1 $t.plotheight -fill x \
	9,0 $t.plotpadx_l -anchor e \
	9,1 $t.plotpadx -fill x \
	10,0 $t.plotpady_l -anchor e \
	10,1 $t.plotpady -fill x \
	11,0 $t.plotrelief_l -anchor e \
	11,1 $t.plotrelief -fill x \
	12,0 $t.plotwidth_l -anchor e \
	12,1 $t.plotwidth -fill x \
	13,0 $t.relief_l -anchor e \
	13,1 $t.relief -fill x \
	14,0 $t.rightmargin_l -anchor e \
	14,1 $t.rightmargin -fill x  \
	15,0 $t.title_l -anchor e \
	15,1 $t.title -fill x \
	16,0 $t.topmargin_l -anchor e \
	16,1 $t.topmargin -fill x  \
	17,0 $t.width_l -anchor e \
	17,1 $t.width -fill x 
    blt::table configure $t c0 -padx { 0 1i }
    blt::table configure $t r* -resize none
    blt::table configure $t r18 -resize both
    puts stderr [blt::table save $t]
}

proc LegendOptions { w } {
    global g
    global legend
    $w insert end "Legend" 
    set t [blt::tk::frame $w.legend]
    $w tab configure "Legend" -window $w.legend
    blt::tk::label $t.selectborderwidth_l -text  "-selectborderwidth" 
    blt::combobutton $t.selectborderwidth \
	-textvariable legend(-selectborderwidth) \
	-menu $t.selectborderwidth.m
    set m [blt::combomenu $t.selectborderwidth.m]
    $m add -type radiobutton -text [$g legend cget -selectborderwidth]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10" 
    $m item configure all -variable legend(-selectborderwidth) \
	-command { $g legend configure -selectborderwidth $legend(-selectborderwidth) }

    blt::tk::label $t.borderwidth_l -text  "-borderwidth" 
    blt::combobutton $t.borderwidth -textvariable legend(-borderwidth) \
	-menu $t.borderwidth.m
    set m [blt::combomenu $t.borderwidth.m]
    $m add -type radiobutton -text [$g legend cget -borderwidth]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10" 
    $m item configure all -variable legend(-borderwidth) \
	-command { $g legend configure -borderwidth $legend(-borderwidth) }

    blt::tk::label $t.pady_l -text  "-pady" 
    blt::combobutton $t.pady -textvariable legend(-pady) \
	-menu $t.pady.m
    set m [blt::combomenu $t.pady.m]
    $m add -type radiobutton -text [$g legend cget -pady]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10"
    $m item configure all -variable legend(-pady) \
	-command { $g legend configure -pady $legend(-pady) }

    blt::tk::label $t.padx_l -text  "-padx" 
    blt::combobutton $t.padx -textvariable legend(-padx) \
	-menu $t.padx.m
    set m [blt::combomenu $t.padx.m]
    $m add -type radiobutton -text [$g legend cget -padx]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10" 
    $m item configure all -variable legend(-padx) \
	-command { $g legend configure -padx $legend(-padx) }

    blt::tk::label $t.selectrelief_l -text  "-selectrelief" 
    blt::combobutton $t.selectrelief -textvariable legend(-selectrelief) \
	-menu $t.selectrelief.m
    set m [blt::combomenu $t.selectrelief.m]
    $m add -type radiobutton -text [$g legend cget -selectrelief]
    $m add -type separator
    $m add -type radiobutton -text "flat" 
    $m add -type radiobutton -text "groove" 
    $m add -type radiobutton -text "raised" 
    $m add -type radiobutton -text "ridge" 
    $m add -type radiobutton -text "solid" 
    $m add -type radiobutton -text "sunken"
    $m item configure all -variable legend(-selectrelief) \
	-command { $g legend configure -selectrelief $legend(-selectrelief) }

    blt::tk::label $t.relief_l -text  "-relief" 
    blt::combobutton $t.relief -textvariable legend(-relief) \
	-menu $t.relief.m
    set m [blt::combomenu $t.relief.m]
    $m add -type radiobutton -text [$g legend cget -relief]
    $m add -type separator
    $m add -type radiobutton -text "flat" 
    $m add -type radiobutton -text "groove"
    $m add -type radiobutton -text "raised"
    $m add -type radiobutton -text "ridge"
    $m add -type radiobutton -text "solid" 
    $m add -type radiobutton -text "sunken"
    $m item configure all -variable legend(-relief) \
	-command { $g legend configure -relief $legend(-relief) }

    blt::tk::label $t.position_l -text  "-position" 
    blt::combobutton $t.position -textvariable legend(-position) \
	-menu $t.position.m
    set m [blt::combomenu $t.position.m]
    $m add -type radiobutton -text [$g legend cget -position]
    $m add -type separator
    $m add -type radiobutton -text "left" 
    $m add -type radiobutton -text "right"
    $m add -type radiobutton -text "top"
    $m add -type radiobutton -text "bottom" 
    $m add -type radiobutton -text "plotarea" 
    $m add -type radiobutton -text "@200,200" 
    $m item configure all -variable legend(-position) \
	-command { $g legend configure -position $legend(-position) }

    blt::tk::label $t.hide_l -text  "-hide" 
    blt::combobutton $t.hide -textvariable legend(-hide) \
	-menu $t.hide.m
    set m [blt::combomenu $t.hide.m]
    $m add -type radiobutton -text [$g legend cget -hide]
    $m add -type separator
    $m add -type radiobutton -text "yes" 
    $m add -type radiobutton -text "no" 
    $m item configure all -variable legend(-hide) \
	-command { $g legend configure -hide $legend(-hide) }

    blt::tk::label $t.activebackground_l -text  "-activebackground" 
    blt::combobutton $t.activebackground -textvariable legend(-activebackground) \
	-menu $t.activebackground.m
    set m [blt::combomenu $t.activebackground.m]
    $m add -type radiobutton -text [$g legend cget -activebackground]
    $m add -type separator
    $m add -type radiobutton -text "black"
    $m add -type radiobutton -text "blue" 
    $m add -type radiobutton -text "green"
    $m add -type radiobutton -text "grey" 
    $m add -type radiobutton -text "red" 
    $m add -type radiobutton -text "white" 
    $m add -type radiobutton -text "yellow"
    $m item configure all -variable legend(-activebackground) \
	-command { $g legend configure -activebackground $legend(-activebackground) }

    blt::tk::label $t.activeborderwidth_l -text  "-activeborderwidth" 
    blt::combobutton $t.activeborderwidth -textvariable legend(-activeborderwidth) \
	-menu $t.activeborderwidth.m
    set m [blt::combomenu $t.activeborderwidth.m]
    $m add -type radiobutton -text [$g legend cget -activeborderwidth]
    $m add -type separator
    $m add -type radiobutton -text "0" 
    $m add -type radiobutton -text "1" 
    $m add -type radiobutton -text "2" 
    $m add -type radiobutton -text "3" 
    $m add -type radiobutton -text "4" 
    $m add -type radiobutton -text "10" 
    $m item configure all -variable legend(-activeborderwidth) \
	-command { $g legend configure -activeborderwidth $legend(-activeborderwidth) }

    blt::tk::label $t.selectbackground_l -text  "-selectbackground" 
    blt::combobutton $t.selectbackground -textvariable legend(-selectbackground) \
	-menu $t.selectbackground.m
    set m [blt::combomenu $t.selectbackground.m]
    $m add -type radiobutton -text [$g legend cget -selectbackground]
    $m add -type separator
    $m add -type radiobutton -text "black"
    $m add -type radiobutton -text "blue" 
    $m add -type radiobutton -text "green"
    $m add -type radiobutton -text "grey" 
    $m add -type radiobutton -text "red" 
    $m add -type radiobutton -text "white" 
    $m add -type radiobutton -text "yellow"
    $m item configure all -variable legend(-selectbackground) \
	-command { $g legend configure -selectbackground $legend(-selectbackground) }

    blt::tk::label $t.selectforeground_l -text  "-selectforeground" 
    blt::combobutton $t.selectforeground -textvariable legend(-selectforeground) \
	-menu $t.selectforeground.m
    set m [blt::combomenu $t.selectforeground.m]
    $m add -type radiobutton -text [$g legend cget -selectforeground]
    $m add -type separator
    $m add -type radiobutton -text "black"
    $m add -type radiobutton -text "blue" 
    $m add -type radiobutton -text "green"
    $m add -type radiobutton -text "grey" 
    $m add -type radiobutton -text "red" 
    $m add -type radiobutton -text "white" 
    $m add -type radiobutton -text "yellow"
    $m item configure all -variable legend(-selectforeground) \
	-command { $g legend configure -selectforeground $legend(-selectforeground) }


    blt::tk::label $t.background_l -text  "-background" 
    blt::combobutton $t.background -textvariable legend(-background) \
	-menu $t.background.m
    set m [blt::combomenu $t.background.m]
    $m add -type radiobutton -text [$g legend cget -background]
    $m add -type separator
    $m add -type radiobutton -text "black" 
    $m add -type radiobutton -text "blue" 
    $m add -type radiobutton -text "green" 
    $m add -type radiobutton -text "grey" 
    $m add -type radiobutton -text "red" 
    $m add -type radiobutton -text "white" 
    $m add -type radiobutton -text "yellow"
    $m item configure all -variable legend(-background) \
	-command { $g legend configure -background $legend(-background) }
    foreach option { borderwidth selectrelief relief 
	selectbackground background selectborderwidth padx pady 
	selectforeground activebackground
	position hide activeborderwidth } {
	$t.$option.m select 0
    }

    blt::table $t \
	1,0 $t.activebackground_l -anchor e \
	1,1 $t.activebackground -fill x \
	2,0 $t.activeborderwidth_l -anchor e \
	2,1 $t.activeborderwidth -fill x \
	3,0 $t.background_l -anchor e \
	3,1 $t.background -fill x  \
	4,0 $t.borderwidth_l -anchor e \
	4,1 $t.borderwidth -fill x \
	5,0 $t.hide_l -anchor e \
	5,1 $t.hide -fill x \
	6,0 $t.padx_l -anchor e \
	6,1 $t.padx -fill x \
	7,0 $t.pady_l -anchor e \
	7,1 $t.pady -fill x \
	8,0 $t.position_l -anchor e \
	8,1 $t.position -fill x \
	9,0 $t.relief_l -anchor e \
	9,1 $t.relief -fill x \
	10,0 $t.selectbackground_l -anchor e \
	10,1 $t.selectbackground -fill x  \
	11,0 $t.selectborderwidth_l -anchor e \
	11,1 $t.selectborderwidth -fill x  \
	12,0 $t.selectforeground_l -anchor e \
	12,1 $t.selectforeground -fill x  \
	13,0 $t.selectrelief_l -anchor e \
	13,1 $t.selectrelief -fill x 
    blt::table configure $t c0 -padx { 0 1i }
    blt::table configure $t r* -resize none
    blt::table configure $t r14 -resize both
}

set t [toplevel .cntrl]
blt::tabset $t.tb
blt::table $t \
    0,0 $t.tb -fill both
GraphOptions $t.tb
AxisOptions $t.tb
LegendOptions $t.tb
Blt_ClosestPoint $g
