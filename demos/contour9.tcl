
package require BLT
blt::contour .g

set pi2 [expr 3.14159265358979323846 * 2]

set x [blt::vector create \#auto]
set y [blt::vector create \#auto]
$x seq -2 2 100
$y seq -2 3 100
set x2 [blt::vector create \#auto]
$x2 expr $x*$x
set y2 [blt::vector create \#auto]
$y2 expr $y*$y
set e 2.7182818284590452354
set z {}
foreach  i [$y2 values] {
    foreach  j [$x2 values] k [$x values] {
	lappend z [expr $k * pow($e, -$j - $i)]
    }
}
	
puts stderr [blt::palette names]
#.g axis configure z -logscale yes

set mesh [blt::mesh create irregular -y $x -x $y]
.g colormap create cmap -palette blue 
.g element create banana -values $z -mesh $mesh -colormap cmap
.g element isoline steps banana 10 
proc UpdateColors {} {
     global usePaletteColors
     if { $usePaletteColors } {
        .g element configure banana -color palette -fill palette
    } else {
        .g element configure banana -color black -fill red
    }
}
proc FixPalette {} {
    global usePalette
    .g element configure cmap -palette $usePalette
}

proc Fix { what } {
    global show

    set bool $show($what)
    puts stderr ".g element configure banana -display$what $bool"
    .g element configure banana -display$what $bool
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
    -textvariable usePalette \
    -relief sunken \
    -background white \
    -arrowon yes \
    -menu .palettes.menu 
blt::tk::label .palettesl -text "Palettes" 
blt::combomenu .palettes.menu \
    -background white \
    -textvariable usePalette 

.palettes.menu listadd [blt::palette names] -command FixPalette
set usePalette "blue"

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
    set show($key) [.g element cget banana -display$key]
}

Blt_ZoomStack .g
