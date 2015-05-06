#!../src/bltwish

package require BLT

# Example of a pareto chart.
#
# The pareto chart mixes line and bar elements in the same graph.
# Each processing operating is represented by a bar element.  The
# total accumulated defects is displayed with a single line element.

blt::barchart .b \
    -title "Defects Found During Inspection" \
    -font {{Sans Serif} 12 Bold} \
    -width 6i \
    -height 5i \
    -bg white \
    -plotborderwidth 1 \
    -plotrelief solid \
    -invertxy yes

blt::table . .b -fill both

set data {
    "Spot Weld"		82	yellow
    "Lathe"		49	orange
    "Gear Cut"		38	green
    "Drill"		24	blue
    "Grind"		17	red
    "Lapping"		12	brown
    "Press"		8	purple
    "De-burr"		4	pink
    "Packaging"		3	cyan
    "Other"		12	magenta
}

# Create an X-Y graph line element to trace the accumulated defects.
.b line create accum -label "" -symbol none -color red

# Define a bitmap to be used to stipple the background of each bar.
blt::bitmap define pattern1 { {4 4} {01 02 04 08} }

# For each process, create a bar element to display the magnitude.
set count 0
set sum 0
set ydata 0
set xdata 0
set b [blt::paintbrush create color -color orange1 -opacity 70]
set areab [blt::paintbrush create color -color blue -opacity 20]
foreach { label value color } $data {
    incr count
    .b element create $label \
	-xdata $count \
	-ydata $value \
	-fg $color \
	-relief raised \
	-borderwidth 1 \
	-bg $b 

    set labels($count) $label
    # Get the total number of defects.
    set sum [expr $value + $sum]
    lappend ydata $sum
    lappend xdata $count
}

# Configure the coordinates of the accumulated defects, 
# now that we know what they are.
.b line configure accum -xdata $xdata -ydata $ydata \
	-areabackground $areab 
.b element lower accum
# Add text markers to label the percentage of total at each point.
foreach x $xdata y $ydata {
    set percent [expr ($y * 100.0) / $sum]
    if { $x == 0 } {
	set text "0%"
	set xoff  16
	set yoff 8
    } else {
	set text [format %.1f $percent] 
	set xoff -16
	set yoff 10
    }
    .b marker create text \
	-coords "$x $y" \
	-text $text \
	-font {Math 9} \
	-fg red4 \
	-anchor c \
	-xoffset $xoff -yoffset $yoff
}

# Display an auxillary y-axis for percentages.
.b axis configure y2 \
    -hide no \
    -min 0.0 \
    -max 100.0 \
    -exterior no \
    -title "Percentage" -grid no

# Title the y-axis
.b axis configure y -title "Defects" -grid no \
    -exterior no 

# Configure the x-axis to display the process names, instead of numbers.
.b axis configure x \
    -title "Process" \
    -command FormatLabels \
    -rotate 0 \
    -tickanchor nw \
    -tickfont {{Sans Serif} 9} \
    -ticklength 5 \
    -exterior no \
    -descending yes \
    -subdivisions 0

proc FormatLabels { widget value } {
    global labels
    set value [expr round($value)]
    if {[info exists labels($value)] } {
	return $labels($value)
    }
    return ""
}

# No legend needed.
.b legend configure -hide yes

# Configure the grid lines.
.b axis configure x -gridcolor lightblue -grid yes 

Blt_ZoomStack .b
