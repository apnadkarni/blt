
package require BLT

blt::mesh create regular "exgrid-0-0.4" \
    -x "1.2145224 3.4495224 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.05-0.4" \
    -x "1.2142221 3.4492221 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.1-0.4" \
    -x "1.2141615 3.4491615 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.15-0.4" \
    -x "1.2141758 3.4491758 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.2-0.4" \
    -x "1.2141744 3.4491744 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.25-0.4" \
    -x "1.2141709 3.4491709 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.3-0.4" \
    -x "1.2141671 3.4491671 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.35-0.4" \
    -x "1.2141646 3.4491646 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.4-0.4" \
    -x "1.2141631 3.4491631 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.45-0.4" \
    -x "1.2141516 3.4541516 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.5-0.4" \
    -x "1.2141562 3.4491562 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.55-0.4" \
    -x "1.2141699 3.4491699 208" -y "0 34.209 64"
blt::mesh create regular "exgrid-0.6-0.4" \
    -x "1.2142394 3.4492394 208" -y "0 34.209 64"

array set plots {
    "Density of states at Vg=0 and Vd=0.4"    exgrid-0-0.4
    "Density of states at Vg=0.05 and Vd=0.4" exgrid-0.05-0.4
    "Density of states at Vg=0.1 and Vd=0.4"  exgrid-0.1-0.4
    "Density of states at Vg=0.15 and Vd=0.4" exgrid-0.15-0.4
    "Density of states at Vg=0.2 and Vd=0.4"  exgrid-0.2-0.4
    "Density of states at Vg=0.25 and Vd=0.4" exgrid-0.25-0.4
    "Density of states at Vg=0.3 and Vd=0.4"  exgrid-0.3-0.4
    "Density of states at Vg=0.35 and Vd=0.4" exgrid-0.35-0.4
    "Density of states at Vg=0.4 and Vd=0.4"  exgrid-0.4-0.4
    "Density of states at Vg=0.45 and Vd=0.4" exgrid-0.45-0.4
    "Density of states at Vg=0.5 and Vd=0.4"  exgrid-0.5-0.4
    "Density of states at Vg=0.55 and Vd=0.4" exgrid-0.55-0.4
    "Density of states at Vg=0.6 and Vd=0.4"  exgrid-0.6-0.4
}

set table [blt::datatable create]
$table import csv -file omw.csv
set labels [$table row values 0]
$table column labels $labels
$table row delete 0
$table column type all double
set count 0

blt::contour .g
.g colormap create greyscale -palette greyscale
.g element create test -color blue -colormap greyscale
.g element isoline steps test 10 
proc UpdateColors {} {
     global usePaletteColors 
     if { $usePaletteColors } {
        .g element configure test -color palette -fill palette
    } else {
        .g element configure test -color black -fill red
    }
}

proc UpdateAxis {} {
     global useLogScale 
    .g axis configure z -logscale $useLogScale
}

proc Fix { what } {
    global show elem

    set bool $show($what)
    puts stderr ".g element configure banana -display$what $bool"
    .g element configure test -display$what $bool
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
    set show($key) [.g element cget test -display$key]
}

proc NextPlot { column } {
    global table plots
    set label [$table column label $column]
    .g element configure test -values [list $table $column] \
	-mesh $plots($label)
    .label configure -text "$label"
    incr column
    if { $column == [$table numcolumns] } {
	set column 0
    }
    after 100 NextPlot $column
}
    
NextPlot 0

Blt_ZoomStack .g
