
package require BLT

proc FormatDate { g value } {
    return [clock format [expr $value] -timezone :UTC]
# -format "%b %Y" ]
}
    
proc SetAxisLoose {} {
    global loose
    .g axis configure x -loose $loose
}

proc SetAxisGrid {} {
    global grid
    .g axis configure x -grid $grid
}

set loose 0

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
    -rotate 90 -loose $loose
.g axis configure y -title "Value" 
.g legend configure -hide yes
    
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
	$t set $i "value" [expr sin($i/12.0)]
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
	set d1 [blt::date scan $year]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "$year: date scan and clock scan don't agree"
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
	    error "$year: date scan ($d1) and clock scan ($d2) don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/6.0)]
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
    set month 12
    set day 1
    for { set i 0 } { $i < 3 } { incr i } {
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
	incr month
    }
}

proc months2 { t } {
    $t row delete all
    set year 1999
    set month 0
    set day 1
    for { set i 3 } { $i < 11 } { incr i } {
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

proc months3 { t } {
    $t row delete all
    set year 1999
    set month 0
    set day 1
    for { set i 100 } { $i < 220 } { incr i } {
	set date [format "%4d-%03d" $year $i]
	set d1 [blt::date scan $date]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/12.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}


proc days1 { t } {
    $t row delete all
    set year 1999
    set month 0
    set day 350
    for { set i 0 } { $i < 5 } { incr i } {
	incr day
	if { $day > 365 } {
	    incr year
	    set day 1
	}
	set d1 [blt::date scan [format "%4d-%03d" $year $day]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc days2 { t } {
    $t row delete all
    set year 1999
    set month 0
    set day 350
    for { set i 0 } { $i < 12 } { incr i } {
	incr day
	if { $day > 365 } {
	    incr year
	    set day 1
	}
	set d1 [blt::date scan [format "%4d-%03d" $year $day]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}


proc days3 { t } {
    $t row delete all
    set year 1999
    set month 0
    set day 348
    for { set i 0 } { $i < 32 } { incr i } {
	incr day
	if { $day > 365 } {
	    incr year
	    set day 1
	}
	set d1 [blt::date scan [format "%4d-%03d" $year $day]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc days4 { t } {
    $t row delete all
    set year 1980
    set month 0
    set day 350
    for { set i 0 } { $i < 45 } { incr i } {
	incr day
	if { $day > 365 } {
	    incr year
	    set day 1
	}
	set d1 [blt::date scan [format "%4d-%03d" $year $day]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc days5 { t } {
    $t row delete all
    set year 1980
    set month 1
    set day 16
    for { set i 0 } { $i < 10 } { incr i } {
	incr day
	set date [format "%4d/%02d/%02d" $year $month $day]
	set d1 [blt::date scan $date]
	puts stderr $date=[blt::date format $d1]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc hours1 { t } {
    $t row delete all
    set year 1980
    set day 350
    set hour 0
    for { set i 0 } { $i < 126 } { incr i } {
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set date [format "%4d-%03d %02d:00" $year $day $hour]
	set d1 [blt::date scan $date]
	puts stderr \n$date=[blt::date format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/12.0)]
	incr hour
    }
}

proc hours1.1 { t } {
    $t row delete all
    set year 1980
    set day 350
    set hour 0
    for { set i 0 } { $i < 34 } { incr i } {
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set date [format "%4d-%03d %02d:00" $year $day $hour]
	set d1 [blt::date scan $date]
	puts stderr \n$date=[blt::date format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/12.0)]
	incr hour
    }
}

proc hours2 { t } {
    $t row delete all
    set year 1980
    set month 0
    set day 350
    for { set i 0 } { $i < 34 } { incr i } {
	incr hour
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set d1 [blt::date scan [format "%4d-%03d %02d:00" $year $day $hour]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc hours3 { t } {
    $t row delete all
    set year 1980
    set month 0
    set day 350
    for { set i 0 } { $i < 14 } { incr i } {
	incr hour
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set d1 [blt::date scan [format "%4d-%03d %02d:00" $year $day $hour]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc hours4 { t } {
    $t row delete all
    set year 1980
    set hour 20
    set day 350
    for { set i 0 } { $i < 8 } { incr i } {
	incr hour
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set d1 [blt::date scan [format "%4d-%03d %02d:00" $year $day $hour]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::date format [expr $d1]]
    }
}

proc hours5 { t } {
    $t row delete all
    set year 1980
    set hour 1
    set min 0
    set day 350
    for { set i 0 } { $i <= 12 } { incr i } {
	if { $min >= 60 } {
	    incr hour
	    set min 0
	}
	set date [format "%4d-%03d %02d:%02d" $year $day $hour $min]
	set d1 [blt::date scan $date]
	puts stderr \n$date=[blt::date format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	incr min 10
    }
}

proc hours6 { t } {
    $t row delete all
    set year 1980
    set hour 0
    set min 22
    set day 1
    for { set i 0 } { $i <= 12 } { incr i } {
	if { $min >= 60 } {
	    incr hour
	    set min 0
	}
	set date [format "%4d-%03d %02d:%02d" $year $day $hour $min]
	set d1 [blt::date scan $date]
	puts stderr \n$date=[blt::date format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	incr min 10
    }
}

years1 $t

$t export csv -file dates.csv
Blt_ClosestPoint .g
Blt_ZoomStack .g
Blt_Crosshairs .g

set m .b.menu 

blt::comboentry .b \
    -width 3.5i \
    -textvariable test \
    -edit no \
    -menu $m

blt::combomenu $m \
    -text "Test" \
    -textvariable test
foreach {proc desc} { 
    years1 "30 months, Major: every 1 year, Minor: 12 months"
    years2 "130 months, Major: every 2 years, Minor: every 1 year"
    years3 "25 years, Major: every 5 years, Minor: every 2 years"
    years4 "114 years, Major: every 20 years: Minor: every 10 years"
    years5 "24 months, Major: every 1 year, Minor: every 1 month"
    months1 "2 months, Major: every 1 month: Minor: every 1 day"
    months2 "7 months, Major: every 1 months: Minor: 1/2 month"
    months3 "120 days, Major: every 1 month: Minor: 1/2 month"
    days1 "5 days, Major: every 1 day: Minor: every 6 hours"
    days2 "12 days, Major: every 1 day: Minor: every 6 hours"
    days3 "32 days, Major: every 1 day: Minor: every 6 hours"
    days4 "45 days, Major: every 1 month: Minor: every 1 day"
    days5 "10 days, Major: every 1 day: Minor: every 6 hours"
    hours1 "126 hours, Major: every 1 day: Minor: every 6 hours"
    hours1.1 "72 hours, Major: every 1 day: Minor: every 6 hours"
    hours2 "34 hours, Major: every 4 hours: Minor: every 1 hour"
    hours3 "14 hours, Major: every 4 hours: Minor: every 1 hour"
    hours4 "8 hours, Major: every 1 hour: Minor: every 15 minutes"
    hours5 "2 hours, Major: every 1 hour: Minor: every 15 minutes"
    hours6 "2 hours, Major: every 1 hour: Minor: every 15 minutes"
} {
    $m add \
	-text $desc \
	-command [list $proc $t] \
	-variable test
}

$m select "30 months, Major: every 1 year, Minor: 12 months"

blt::tk::checkbutton .loose \
    -text "loose" \
    -variable loose \
    -command [list SetAxisLoose]

blt::tk::checkbutton .grid \
    -text "grid" \
    -variable grid \
    -command [list SetAxisGrid]


blt::table . \
    0,0 .b -anchor w \
    0,1 .loose -anchor w \
    0,2 .grid -anchor w \
    1,0 .g -fill both -cspan 3

blt::table configure . r0 c1 c2 -resize none
