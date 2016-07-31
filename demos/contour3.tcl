package require BLT
blt::contour .g

set palette spectral.rgb

# All the data fits the same grid: a uniform rectangular grid 68x37
set xGridSize 68
set yGridSize 37

# Create a table and load the data file (csv) into it.
set table [blt::datatable create]
$table import csv -file data/nplsms.csv

# The first row of the csv contains the column labels.
set labels [$table row values 0]
$table column labels $labels
$table row delete 0
# The rest of the table contains all numbers.
$table column type @all double

# Get the max and max of the 1nd column
set allmin [$table min 1]
set allmax [$table max 1]

set mesh [blt::mesh create regular \
	-x [list 1 $xGridSize $xGridSize] \
	-y [list 1 $yGridSize $yGridSize]] 

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

.g element create sine \
	-values [list $table 1] \
	-mesh $mesh 

set tmp [blt::vector create \#auto]
$tmp set [$table min]
set min [$tmp min]
$tmp set [$table max]
set max [$tmp max]
blt::vector destroy $tmp
.g element isoline steps sine 10 -min $min -max $max 
.g legend configure -hide yes
.g axis configure z \
    -min $min -max $max \
    -palette $palette \
    -margin right \
    -colorbarthickness 20 \
    -tickdirection in 

proc Fix { what } {
    global show

    set bool $show($what)
    puts stderr ".g element configure sine -show$what $bool"
    .g element configure sine -show$what $bool
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
blt::tk::label .label -text ""
blt::table . \
    0,0 .label -fill x \
    1,0 .g -fill both -rowspan 7 \
    1,1 .boundary -anchor w \
    2,1 .colormap -anchor w \
    3,1 .isolines -anchor w \
    4,1 .wireframe -anchor w \
    5,1 .symbols -anchor w \
    6,1 .values -anchor w 
blt::table configure . r* c1 -resize none
blt::table configure . r7 -resize both
foreach key [array names show] {
    set show($key) [.g element cget sine -show$key]
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

