package require BLT
source ./data/dentalscan.tcl 

blt::vector create dentalscan
dentalscan set $data

set mesh [blt::mesh create regular regular -x "0 512 512" -y "0 512 512"]

blt::contour .g -highlightthickness 0

set palette spectral.rgb
.g element create myContour -values dentalscan -mesh $mesh 
.g element isoline steps myContour 6
.g legend configure -hide yes
.g axis configure z -palette $palette -margin left -colorbarthickness 20 
proc UpdateColors {} {
     global usePaletteColors
     if { $usePaletteColors } {
        .g element configure myContour -color palette -fill palette
    } else {
        .g element configure myContour -color black -fill red
    }
}
proc FixPalette {} {
    global usePalette
    .g axis configure z -palette $usePalette
    .g2 axis configure x -palette $usePalette
}

proc Fix { what } {
    global show
    set bool $show($what)
    .g element configure myContour -show$what $bool
}

array set show {
    boundary 0
    values 0
    symbols 0
    isolines 0
    colormap 0
    symbols 0
    wireframe 0
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
    -textvariable usePalette  \
    -height 200 \
    -yscrollbar .palettes.menu.ybar \
    -xscrollbar .palettes.menu.xbar

blt::tk::scrollbar .palettes.menu.xbar 
blt::tk::scrollbar .palettes.menu.ybar

foreach pal [blt::palette names] {
    set pal [string trim $pal ::]
    lappend palettes $pal
}
.palettes.menu listadd [lsort -dictionary $palettes] -command FixPalette
set usePalette $palette

blt::table . \
    1,0 .g -fill both -rowspan 10 \
    1,1 .boundary -anchor w \
    2,1 .colormap -anchor w \
    3,1 .isolines -anchor w \
    4,1 .wireframe -anchor w  \
    5,1 .symbols -anchor w \
    6,1 .values -anchor w  \
    7,1 .interp -anchor w \
    8,1 .palettesl -anchor w  \
    9,1 .palettes -fill x 

foreach key [array names show] {
    set show($key) [.g element cget myContour -show$key]
}

Blt_ZoomStack .g

set numBins 256
set min [dentalscan min]
set max [dentalscan max]
set freq [blt::vector create]
# Get a histogram of the dental scan values
$freq frequency dentalscan $numBins
set w [expr ($max - $min) / double($numBins)]
# Compute the location for the bins within the range of values
set x [blt::vector create]
$x linspace [expr $min + ($w * 0.5)] [expr $max - ($w - 0.5)] $numBins

blt::barchart .g2 \
    -barwidth $w  -height 1i -highlightthickness 0
.g2 axis configure x -stepsize 0  -palette $palette
.g2 axis configure y -logscale yes -grid no -subdivisions 0
.g2 element create hist -x $x -y $freq -relief flat -colormap x \
    -outline ""
.g2 legend configure -hide yes
Blt_ZoomStack .g2

blt::table . \
    11,0 .g2 -fill both 

blt::table configure . r* c* -resize none
blt::table configure . c0 r10 r11 -resize both
