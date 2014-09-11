
package require BLT

proc FormatDate { g value } {
    puts stderr value=$value
    return [clock format [expr $value]]
# -format "%b %Y" ]
}
    

set t [blt::datatable create]

for { set i 0 } { $i < 12 } { incr i } {
    set month [expr $i + 1]
    set d1 [blt::date scan "1990/$month/1"]
    set d2 [clock scan "$month/01/1990" -timezone :UTC]
    puts "d1=$d1 d2=$d2"
    $t set $i "date" $d1
    $t set $i "value" [expr sin($i/2.0)]
    puts stderr [blt::date format [expr $d1]]
}

blt::graph .g 

.g element create test \
    -x [list $t "date"] \
    -y [list $t "value"] \
    -smooth quadratic

.g axis configure x -title "Date" -timescale yes  -command FormatDate \
    -rotate 90 -loose yes
.g axis configure y -title "Value" 

blt::table . \
    0,0 .g -fill both

$t export csv -file dates.csv
Blt_ClosestPoint .g
Blt_ZoomStack .g
