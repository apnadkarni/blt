
package require BLT

proc FormatDate { g value } {
    return [clock format [expr $value] -timezone :UTC -format "%b %Y" ]
}
    

set t [blt::datatable create]

set year 1969
set month 1
for { set i 0 } { $i < 20 } { incr i } {
    incr month
    if { $month > 12 } {
	set month 1
	incr year
    }
    set d1 [blt::date scan "$year/$month/1"]
    set d2 [clock scan "$month/01/$year" -timezone :UTC]
    if { $d1 != $d2 } {
	error "date scan and clock scan don't agree"
    }
    $t set $i "date" $d1
    $t set $i "value" [expr sin($i/2.0)]
    puts stderr [blt::date format [expr $d1]]
}

blt::graph .g 

.g element create test \
    -x [list $t "date"] \
    -y [list $t "value"] \
    -smooth quadratic \
    -symbol splus

.g axis configure x -title "Date" -timescale yes  -command FormatDate \
    -rotate 90 -loose no
.g axis configure y -title "Value" 

blt::table . \
    0,0 .g -fill both

$t export csv -file dates.csv
Blt_ClosestPoint .g
Blt_ZoomStack .g
Blt_Crosshairs .g
