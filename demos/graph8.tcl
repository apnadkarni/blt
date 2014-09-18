
package require BLT

proc FormatDate { g value } {
    return [clock format [expr $value] -timezone :UTC]
# -format "%b %Y" ]
}
    

set t [blt::datatable create]
$t set 0 "date" 1
$t set 0 "value" 1
$t set 1 "date" 100
$t set 1 "value" 100

blt::graph .g 

.g element create test \
    -x [list $t "date"] \
    -y [list $t "value"] \
    -smooth quadratic \
    -symbol circle \
    -fill green2 \
    -pixels 5

.g axis configure x -title "Date" -timescale yes  -command FormatDate \
    -rotate 90 -loose yes
.g axis configure y -title "Value" 


proc years1 { t } {
    $t row delete all
    set year 1969
    set month 0
    set day 1
    for { set i 0 } { $i < 30 } { incr i } {
	incr month
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::date scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc years2 { t } {
    $t row delete all
    set year 1969
    set month 0
    set day 1
    for { set i 0 } { $i < 130 } { incr i } {
	incr month
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::date scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc years3 { t } {
    $t row delete all
    set year 1968
    set month 1
    set day 1
    for { set i 0 } { $i < 25 } { incr i } {
	incr year
	set d1 [blt::date scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc years4 { t } {
    $t row delete all
    set year 1899
    set month 1
    set day 1
    for { set i 0 } { $i < 114 } { incr i } {
	incr year
	set d1 [blt::date scan "$year"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc years5 { t } {
    $t row delete all
    set year 1969
    set month 0
    set day 1
    for { set i 0 } { $i < 25 } { incr i } {
	incr month
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::date scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc months1 { t } {
    $t row delete all
    set year 1969
    set month 0
    set day 1
    for { set i 0 } { $i < 3 } { incr i } {
	incr month
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::date scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

years1 $t

$t export csv -file dates.csv
Blt_ClosestPoint .g
Blt_ZoomStack .g
Blt_Crosshairs .g

set m .b.menu 

blt::comboentry .b \
    -width 3i \
    -textvariable test \
    -edit no \
    -menu $m

blt::combomenu $m \
    -text "Test" \
    -textvariable test
foreach {proc desc} { 
    years1 "30 months, Major: 3 years, Minor: 12 months"
    years2 "130 months, Major: 14 years, Minor: every 1 year"
    years3 "25 years, Major: 30 years, Minor: every 2 years"
    years4 "114 years, Major: 120 years: Minor: every 5 years"
    years5 "Major: years < 3: Minor: 12 months"
    months1 "Major years < 3: Minor: 12 months"
} {
    $m add \
	-text $desc \
	-command [list $proc $t] \
	-variable test
}


blt::table . \
    0,0 .b -anchor w \
    1,0 .g -fill both
