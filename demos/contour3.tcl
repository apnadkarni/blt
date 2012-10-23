package require BLT
blt::contour .g

set table [blt::datatable create]
$table import csv -file nplsms.csv
set labels [$table row values 0]
$table column labels $labels
$table row delete 0
$table column type all double
set allmin [$table min 1]
set allmax [$table max 1]
set mesh [blt::mesh create regular -y {1 37 37} -x {1 68 68}]
for { set col 2 } { $col < [$table numcolumns] } { incr col } {
    set min [$table min $col]
    set max [$table max $col]
    if { $min < $allmin } {
	set allmin $min
    }
    if { $max > $allmax } {
	set allmax $max
    }
}
.g colormap create nanohub -palette nanohub -min $allmin -max $allmax
.g element create sine \
	-values [list $table 1] \
	-mesh $mesh \
	-colormap nanohub 

set tmp [blt::vector create \#auto]
$tmp set [$table min]
set min [$tmp min]
$tmp set [$table max]
set max [$tmp max]
blt::vector destroy $tmp
.g element isoline steps sine 10 -min $min -max $max 
.g axis configure z -min $min -max $max

proc Fix { what } {
    global show

    set bool $show($what)
    puts stderr ".g element configure sine -display$what $bool"
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
blt::table . \
    0,0 .label -fill x \
    1,0 .g -fill both -rowspan 7 \
    1,1 .hull -anchor w \
    2,1 .colormap -anchor w \
    3,1 .isolines -anchor w \
    4,1 .edges -anchor w \
    5,1 .symbols -anchor w \
    6,1 .values -anchor w 
blt::table configure . r* c1 -resize none
blt::table configure . r7 -resize both
foreach key [array names show] {
    set show($key) [.g element cget sine -display$key]
}

Blt_ZoomStack .g

proc NextPlot { column } {
    global table 
    .g element configure sine -values [list $table $column]
    .label configure -text "[$table column label $column]"
    incr column
    if { $column == [$table numcolumns] } {
	set column 0
    }
    after 100 NextPlot $column
}
    
NextPlot 0

