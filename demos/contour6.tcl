package require BLT
blt::contour .g

set x [blt::vector create]
set y [blt::vector create]
$x seq 0 1 100
$y seq 0 1 100
set x2 [blt::vector create]
$x2 expr $x*$x
set x21 [blt::vector create]
$x21 expr {1.0 - $x2}
set x2y [blt::vector create]
$x2y expr {$y - $x2}

set z {}
foreach  i [$x2y values] {
    foreach  j [$x21 values] {
	lappend z [expr ($j * $j) + 100.0 * ($i * $i) + 1.0]
    }
}
set zv [blt::vector create]
$zv set $z
puts stderr "minz=[$zv min] maxz=[$zv max]"

set mesh [blt::mesh create regular -y {1 100 10} -x {1 100 10}]
.g colormap create greyscale -palette greyscale
.g element create banana -values $z -mesh $mesh -colormap greyscale
.g axis configure z -logscale yes
.g element isoline steps banana 10 

proc Fix { what } {
    global show

    set bool $show($what)
    puts stderr ".g element configure banana -display$what $bool"
    .g element configure banana -display$what $bool
}
proc UpdateAxis {} {
     global useLogScale 
    .g axis configure z -logscale $useLogScale
}
proc UpdateColors {} {
     global usePaletteColors 
     if { $usePaletteColors } {
        .g element configure banana -color palette -fill palette
    } else {
        .g element configure banana -color black -fill red
    }
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
blt::tk::label .label -text ""
blt::tk::checkbutton .palette -text "Use palette colors" \
    -variable usePaletteColors -command "UpdateColors"
blt::tk::checkbutton .logscale -text "Log scale" \
    -variable useLogScale -command "UpdateAxis"
blt::table . \
    0,0 .label -fill x \
    1,0 .g -fill both -rowspan 9 \
    1,1 .hull -anchor w \
    2,1 .colormap -anchor w \
    3,1 .isolines -anchor w \
    4,1 .edges -anchor w \
    5,1 .symbols -anchor w \
    6,1 .values -anchor w \
    7,1 .palette -anchor w \
    8,1 .logscale -anchor w 
blt::table configure . r* c1 -resize none
blt::table configure . r9 -resize both
foreach key [array names show] {
    set show($key) [.g element cget banana -display$key]
}

Blt_ZoomStack .g

