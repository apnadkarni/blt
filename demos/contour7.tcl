
package require BLT

set bg [blt::background create linear  \
	    -jitter 3 \
	    -from w -to e \
	    -colorscale log \
	    -lowcolor "grey99"\
	    -highcolor "grey85"\
	    -repeat reversing \
	    -relativeto .controls]
blt::tk::frame .controls -bg $bg -borderwidth 4

option add *BltTkCheckbutton*background $bg
option add *BltTkLabel*background $bg

option add *HighlightThickness 0
set palette amwg_blueyellowred.rgb
set x [blt::vector create]
set y [blt::vector create]
$x linspace -2 2 50
$y linspace -2 2.3 50
set x2 [blt::vector create]
$x2 expr { $x* $x }
set y2 [blt::vector create]
$y2 expr { $y * $y}

set z {}
foreach  i [$y2 values] {
    foreach  j [$x2 values] k [$x values] {
	set value [expr ($k * exp(-($i + $j)) + 0.001 * rand()*0.0) * 1000]
	lappend z $value
    }
}


set mesh [blt::mesh create regular -y {0 100 50} -x {0 100 50}]

blt::contour .g -highlightthickness 0 -bg white
.g element create myContour -values $z -mesh $mesh 
.g isoline steps 10 -element myContour
.g legend configure -hide yes 
.g axis configure x -tickdirection in  -scale linear
.g axis configure y -tickdirection in  -scale linear
.g axis configure z \
    -palette $palette \
    -colorbarthickness 20 \
    -tickdirection in \
    -scale linear \
    -margin right 

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
    .g axis configure z -palette $palette
}

proc FixSymbols {} {
    global palette
    .g pen configure activeContour configure z -palette $palette
}

proc LogScale {} {
    global logScale
    if { $logScale } {
	.g axis configure z -scale "log"
    } else {
	.g axis configure z -scale "linear"
    }
}

proc Decreasing {} {
    global decreasing
    .g axis configure z -decreasing $decreasing
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

blt::tk::checkbutton .controls.boundary -text "Boundary" -variable show(boundary) \
    -command "Fix boundary"
blt::tk::checkbutton .controls.wireframe -text "Wireframe" -variable show(wireframe) \
    -command "Fix wireframe"
blt::tk::checkbutton .controls.colormap -text "Colormap"  \
    -variable show(colormap) -command "Fix colormap"
blt::tk::checkbutton .controls.isolines -text "Isolines" \
    -variable show(isolines) -command "Fix isolines"
blt::tk::checkbutton .controls.values -text "Values" \
    -variable show(values) -command "Fix values"
blt::tk::checkbutton .controls.symbols -text "Symbols" \
    -variable show(symbols) -command "Fix symbols"
blt::tk::checkbutton .controls.interp -text "Use palette colors" \
    -variable usePaletteColors -command "UpdateColors"
blt::tk::checkbutton .controls.logscale -text "Log scale" \
    -variable logScale -command "LogScale"
blt::tk::checkbutton .controls.decreasing -text "Decreasing" \
    -variable decreasing -command "Decreasing"
blt::combobutton .controls.palettes \
    -textvariable palette \
    -relief sunken \
    -background white \
    -arrowon yes \
    -menu .controls.palettes.menu 
blt::tk::label .controls.palettesl -text "Palettes" 
blt::combomenu .controls.palettes.menu \
    -background white \
    -textvariable palette \
    -height 200 \
    -yscrollbar .controls.palettes.menu.ybar \
    -xscrollbar .controls.palettes.menu.xbar

blt::tk::scrollbar .controls.palettes.menu.xbar 
blt::tk::scrollbar .controls.palettes.menu.ybar

foreach pal [blt::palette names] {
    set pal [string trim $pal ::]
    lappend palettes $pal
}

.controls.palettes.menu listadd [lsort -dictionary $palettes] -command FixPalette

blt::table .controls \
    0,0 .controls.boundary -anchor w -cspan 2 \
    1,0 .controls.colormap -anchor w -cspan 2\
    2,0 .controls.isolines -anchor w -cspan 2 \
    3,0 .controls.wireframe -anchor w -cspan 2 \
    4,0 .controls.symbols -anchor w -cspan 2 \
    5,0 .controls.values -anchor w -cspan 2 \
    6,0 .controls.interp -anchor w -cspan 2 \
    7,0 .controls.logscale -anchor w -cspan 2 \
    8,0 .controls.decreasing -anchor w -cspan 2 \
    9,0 .controls.palettesl -anchor w  \
    9,1 .controls.palettes -fill x

blt::table configure .controls r* c1 -resize none
blt::table configure .controls r10 -resize both

blt::table . \
    0,0 .g -fill both \
    0,1 .controls -fill both 


foreach key [array names show] {
    set show($key) [.g element cget myContour -show$key]
}

Blt_ZoomStack .g

.g isoline bind all <Enter> {
    %W isoline deactivate all
    %W isoline activate current
}

.g isoline bind all <Leave> {
    %W isoline deactivate all
}

