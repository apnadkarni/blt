
package require BLT

set palette blue
set x [blt::vector create]
set y [blt::vector create]
$x seq -2 2 100
$y seq -2 3 100
set x2 [blt::vector create]
$x2 expr { $x* $x }
set y2 [blt::vector create]
$y2 expr { $y * $y}

set z {}
foreach  i [$y2 values] {
    foreach  j [$x2 values] k [$x values] {
	lappend z [expr $k * exp(-($i + $j)) + 0.01 * rand()*0.4]
    }
}

#.g axis configure z -logscale yes

set mesh [blt::mesh create regular -y {0 100 100} -x {0 100 100}]

blt::contour .g -highlightthickness 0
.g colormap create myColormap -palette $palette 
.g element create myContour -values $z -mesh $mesh -colormap myColormap
.g element isoline steps myContour 10 
.g legend configure -hide yes

proc UpdateColors {} {
     global usePaletteColors
     if { $usePaletteColors } {
        .g element configure myContour -color palette -fill palette
    } else {
        .g element configure myContour -color black -fill red
    }
}
proc FixPalette {} {
    global palette
    .g colormap configure myColormap -palette $palette
}

proc Fix { what } {
    global show

    set bool $show($what)
    .g element configure myContour -display$what $bool
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
blt::tk::checkbutton .interp -text "Use palette colors" \
    -variable usePaletteColors -command "UpdateColors"
blt::combobutton .palettes \
    -textvariable palette \
    -relief sunken \
    -background white \
    -arrowon yes \
    -menu .palettes.menu 
blt::tk::label .palettesl -text "Palettes" 
blt::combomenu .palettes.menu \
    -background white \
    -textvariable palette 

foreach pal [blt::palette names] {
    set pal [string trim $pal ::]
    lappend palettes $pal
}

.palettes.menu listadd $palettes -command FixPalette

blt::table . \
    0,0 .label -fill x \
    1,0 .g -fill both -rowspan 9 \
    1,1 .hull -anchor w -cspan 2 \
    2,1 .colormap -anchor w -cspan 2\
    3,1 .isolines -anchor w -cspan 2 \
    4,1 .edges -anchor w -cspan 2 \
    5,1 .symbols -anchor w -cspan 2 \
    6,1 .values -anchor w -cspan 2 \
    7,1 .interp -anchor w -cspan 2 \
    8,1 .palettesl -anchor w  \
    8,2 .palettes -fill x

blt::table configure . r* c1 -resize none
blt::table configure . r9 -resize both

foreach key [array names show] {
    set show($key) [.g element cget myContour -display$key]
}

Blt_ZoomStack .g
