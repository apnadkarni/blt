package require BLT

set palette "spectral"
set mesh [blt::mesh create regular -x "0 100 2" -y "0 100 2"]
set values [blt::vector create]
$values set { 1 1 10 1 }

blt::contour .g -highlightthickness 0

.g colormap create myColormap -palette $palette
.g element create myContour -mesh $mesh -values $values
.g element isoline step myContour 20
.g element configure myContour \
    -fill palette \
    -outline black \
    -colormap myColormap
.g legend configure -hide yes

foreach key { hull values symbols isolines colormap symbols edges } {
    set show($key) [.g element cget myContour -display$key]
}

proc Fix { what } {
    global show 
    set bool $show($what)
    .g element configure myContour -display$what $bool
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

blt::table . \
    0,0 .g -fill both -rowspan 7 \
    0,1 .hull -anchor w \
    1,1 .colormap -anchor w \
    2,1 .isolines -anchor w \
    3,1 .edges -anchor w \
    4,1 .symbols -anchor w \
    5,1 .values -anchor w 
blt::table configure . r* c1 -resize none
blt::table configure . r6 -resize both

Blt_ZoomStack .g


