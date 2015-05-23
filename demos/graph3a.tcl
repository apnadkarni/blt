
package require BLT

set table [blt::datatable create]
$table import csv -file data/rwn.csv

set labels [$table row values 0]
$table column labels $labels
$table row delete 0

$table column type time time

set graph [blt::graph .g -stackaxes yes]
$graph legend configure -hide yes
$graph axis configure x -title "# samples"
$graph axis configure y -hide yes
set time [lindex $labels 0]
foreach label "displacement force strain time" {
    set axis [$graph axis create $label -title $label \
		  -margin left -loose yes -rotate 0 \
		  -tickdefault 2 -grid yes]
    $graph element create $label \
	-x [list $table sample#]  \
	-y [list $table $label] \
	-symbol none \
	-mapy $axis
    puts stderr label=$label
}
$graph axis configure time -scale time

blt::table . \
    0,0 $graph -fill both

Blt_Crosshairs $graph
Blt_ZoomStack $graph

