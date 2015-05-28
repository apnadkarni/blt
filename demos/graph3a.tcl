
package require BLT

set table [blt::datatable create]
$table import csv -file data/rwn.csv

set labels [$table row values 0]
$table column labels $labels
$table row delete 0

$table column type time time

set graph [blt::graph .g -title "Rectangular Wall Test" \
	       -stackaxes yes -background white -plotborderwidth 0]
set dir "out"
$graph legend configure -hide yes
$graph axis configure x -title "# samples" -loose yes -tickdirection $dir
$graph axis configure y -hide yes
set time [lindex $labels 0]

foreach label "displacement force strain" color "red green3 blue" units "in kip \u00b5" {
    set axis $label
    $graph axis create $label -title "$label ($units)" \
	-titlefont "{Sans Serif} 8.5" \
	-margin left \
	-loose yes \
	-tickdirection $dir \
	-tickdefault 2 \
	-grid yes \
	-tickfont "{Sans Serif} 8"

    $graph marker create rectangle \
	-coords "-Inf -Inf Inf Inf" \
	-mapy $axis \
	-under yes \
	-fill "grey99"

    $graph element create $label \
	-x [list $table sample#]  \
	-y [list $table $label] \
	-symbol none \
	-color $color \
	-mapy $axis
}
blt::table . \
    0,0 $graph -fill both

Blt_Crosshairs $graph
Blt_ZoomStack $graph
