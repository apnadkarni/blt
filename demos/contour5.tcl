package require BLT
blt::mesh create regular test -x "1 10000 2" -y "1 10000 2"
puts stderr triangles=[blt::mesh triangles test]
blt::vector create test
test set {
    1 1 10 1
}
blt::contour .g

.g colormap create nanohub -palette nanohub
.g element create test -mesh test -values test 
.g element isoline step test 20
.g element configure test -fill palette -outline black -colormap nanohub
foreach key { hull values symbols isolines colormap symbols edges } {
    set show($key) [.g element cget test -display$key]
}

proc Fix { what } {
    global show 
    set bool $show($what)
    puts stderr ".g element configure test -display$what $bool"
    .g element configure test -display$what $bool
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
blt::table configure . r* -resize none
blt::table configure . r6 -resize both

Blt_ZoomStack .g


