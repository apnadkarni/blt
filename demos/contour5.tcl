package require BLT

set palette "spectral.rgb"
set mesh [blt::mesh create regular -x "0 100 2" -y "0 100 2"]
set values [blt::vector create]
$values set { 1 1 10 1 }

blt::contour .g -highlightthickness 0

.g element create myContour -mesh $mesh -values $values
.g isoline step 20 -element myContour
.g element configure myContour \
    -fill palette \
    -outline black 
.g legend configure -hide yes
.g axis configure z \
    -palette $palette \
    -margin right \
    -colorbarthickness 20 \
    -tickdirection in 
foreach key { boundary values symbols isolines colormap symbols wireframe } {
    set show($key) [.g element cget myContour -show$key]
}

proc Fix { what } {
    global show 
    set bool $show($what)
    .g element configure myContour -show$what $bool
}

blt::tk::checkbutton .boundary -text "Boundary" -variable show(boundary) \
    -command "Fix boundary"
blt::tk::checkbutton .wireframe -text "Wireframe" -variable show(wireframe) \
    -command "Fix wireframe"
blt::tk::checkbutton .colormap -text "Colormap"  \
    -variable show(colormap) -command "Fix colormap"
blt::tk::checkbutton .isolines -text "Isolines" \
    -variable show(isolines) -command "Fix isolines"
blt::tk::checkbutton .values -text "Values" \
    -variable show(values) -command "Fix values"
blt::tk::checkbutton .symbols -text "Symbols" \
    -variable show(symbols) -command "Fix symbols"

blt::table . \
    0,0 .g -fill both -rowspan 7 \
    0,1 .boundary -anchor w \
    1,1 .colormap -anchor w \
    2,1 .isolines -anchor w \
    3,1 .wireframe -anchor w \
    4,1 .symbols -anchor w \
    5,1 .values -anchor w 
blt::table configure . r* c1 -resize none
blt::table configure . r6 -resize both

#Blt_ZoomStack .g

proc FindIsoline { g x y } {
    set results [$g isoline nearest $x $y -halo 1i]
    set markerName "myMarker"
    $g marker delete active
    $g isoline deactivate all
    if { $results == "" } {
	return
    }
    array set info $results
    # --------------------------------------------------------------
    # info(name)		- element Id
    # info(index)		- index of closest point
    # info(x) find(y)		- coordinates of closest point
    #				  or closest point on line segment.
    # info(dist)		- distance from sample coordinate
    # --------------------------------------------------------------
    set mesg [format "%s: #%d value=%g\n x=%g y=%g" \
		  $info(name) $info(index) $info(value) $info(x) $info(y)]
    set coords [$g invtransform $x $y]
    set nx [lindex $coords 0]
    set ny [lindex $coords 1]
    set id [$g marker create text \
	-coords [list $nx $ny] \
	-text $mesg \
	-font "Arial 6" \
	-anchor center \
	-justify left \
	-yoffset 0 -bg {}]
    $g marker tag add "active" $id

    $g isoline activate $info(name)
    set id [$g marker create line -coords "$nx $ny $info(x) $info(y)"]
    $g marker tag add "active" $id 
}

.g isoline bind all <Enter> {
    %W isoline deactivate all
    %W isoline activate current
}

.g isoline bind all <Leave> {
    %W isoline deactivate all
}

