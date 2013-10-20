package require BLT
set data {}
set numPoints 1000
set numIsolines 10
set xv [blt::vector create -length $numPoints]
set yv [blt::vector create -length $numPoints]
set zv [blt::vector create -length $numPoints]
$xv expr {random($xv)*10.0}
$yv expr {random($yv)*10.0}
$zv expr {abs(sin($xv*$yv/70.0))}

blt::contour .g
set mesh [blt::mesh create cloud -x $xv -y $yv]
.g colormap create rainbow -palette rainbow -min 0.0 -max 1.0 -axis z
.g element create sine -values $zv -mesh $mesh -colormap rainbow
.g element isoline steps sine $numIsolines

proc UpdateColors {} {
    global usePaletteColors
    if { $usePaletteColors } {
	.g element configure sine -color palette -fill palette
    } else {
        .g element configure sine -color black -fill red
    }
}

proc Fix { what } {
    global show

    set bool $show($what)
    if { [scan $what "isoline%d" number] == 1 } {
	.g element isoline configure sine $what -show $bool
	return
    }
    if { 0 && $what == "colormap" } {
	if { $bool } {
	    set color palette
	} else {
	    set color blue
	}
	.g element configure sine -color $color
    }
    .g element configure sine -display$what $bool
}

array set show {
    hull 0
    values 0
    symbols 0
    isolines 0
    colormap 0
    symbols 0
    edges 0
}
set usePaletteColors 0

blt::tk::checkbutton .hull -text "Boundary" -variable show(hull) \
    -command "Fix hull"
blt::tk::checkbutton .edges -text "Edges" -variable show(edges) \
    -command "Fix edges"
blt::tk::checkbutton .colormap -text "Colormap"  \
    -variable show(colormap) -command "Fix colormap"
blt::tk::checkbutton .isolines -text "Isolines" \
    -variable show(isolines) -command "Fix isolines"
blt::tk::checkbutton .values -text "Values" \
    -variable show(values) -command "Fix values"
blt::tk::checkbutton .symbols -text "Symbols" \
    -variable show(symbols) -command "Fix symbols"
blt::tk::checkbutton .interp -text "Use palette colors" \
    -variable usePaletteColors -command "UpdateColors"
blt::table . \
    0,0 .g -fill both -rowspan 15 \
    0,1 .hull -anchor w \
    1,1 .colormap -anchor w \
    2,1 .isolines -anchor w \
    3,1 .edges -anchor w \
    4,1 .symbols -anchor w \
    5,1 .values -anchor w \
    6,1 .interp -anchor w 
foreach key [array names show] {
    set show($key) [.g element cget sine -display$key]
}

Blt_ZoomStack .g

set count 7
foreach name [lsort -dictionary [.g element isoline names sine]] {
    blt::tk::checkbutton .$name -text "$name"  \
	-variable show($name) -command "Fix $name"
    .g element isoline configure sine $name -show yes
    set show($name) [.g element isoline cget sine $name -show]
    blt::table . \
	$count,1 .$name -anchor w 
    incr count
}
.g legend configure -anchor s
blt::table configure . r* c1 -resize none
blt::table configure . r$count -resize both
blt::table . \
    0,0 .g -fill both -rowspan [expr $count+1]
