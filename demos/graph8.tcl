
package require BLT

proc FormatDate { g value } {
    return [clock format [expr int($value)] -timezone :UTC]
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

.g axis configure x \
    -title "Date" \
    -timescale yes  \
    -rotate 0 \
    -loose $loose \

#    -command FormatDate 
.g axis configure y -title "Value" 
.g legend configure -hide yes
    
proc test1 { t } {
    $t row delete @all
    set year 1969
    set month 0
    set day 1
    for { set i 0 } { $i < 30 } { incr i } {
	incr month
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::timestamp scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test2 { t } {
    $t row delete @all
    set year 1969
    set month 0
    set day 1
    for { set i 0 } { $i < 130 } { incr i } {
	incr month
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::timestamp scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/12.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test3 { t } {
    $t row delete @all
    set year 1968
    set month 1
    set day 1
    for { set i 0 } { $i < 25 } { incr i } {
	incr year
	set d1 [blt::timestamp scan $year]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "$year: date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test4 { t } {
    $t row delete @all
    set year 1899
    set month 1
    set day 1
    for { set i 0 } { $i < 114 } { incr i } {
	incr year
	set d1 [blt::timestamp scan "$year"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "$year: date scan ($d1) and clock scan ($d2) don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/6.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test5 { t } {
    $t row delete @all
    set year 1969
    set month 0
    set day 1
    for { set i 0 } { $i < 25 } { incr i } {
	incr month
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::timestamp scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test6 { t } {
    $t row delete @all
    set year 1969
    set month 12
    set day 1
    for { set i 0 } { $i < 3 } { incr i } {
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::timestamp scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
	incr month
    }
}

proc test7 { t } {
    $t row delete @all
    set year 1999
    set month 0
    set day 1
    for { set i 3 } { $i < 11 } { incr i } {
	incr month
	if { $month > 12 } {
	    set month 1
	    incr year
	}
	set d1 [blt::timestamp scan "$year/$month/$day"]
	set d2 [clock scan "$month/$day/$year" -timezone :UTC]
	if { $d1 != $d2 } {
	    error "date scan and clock scan don't agree"
	}
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test8 { t } {
    $t row delete @all
    set year 1999
    set month 0
    set day 1
    for { set i 100 } { $i < 220 } { incr i } {
	set date [format "%4d-%03d" $year $i]
	set d1 [blt::timestamp scan $date]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/12.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}


proc test9 { t } {
    $t row delete @all
    set year 1999
    set month 0
    set day 350
    for { set i 0 } { $i < 5 } { incr i } {
	incr day
	if { $day > 365 } {
	    incr year
	    set day 1
	}
	set d1 [blt::timestamp scan [format "%4d-%03d" $year $day]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test10 { t } {
    $t row delete @all
    set year 1999
    set month 0
    set day 350
    for { set i 0 } { $i < 12 } { incr i } {
	incr day
	if { $day > 365 } {
	    incr year
	    set day 1
	}
	set d1 [blt::timestamp scan [format "%4d-%03d" $year $day]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}


proc test11 { t } {
    $t row delete @all
    set year 1999
    set month 0
    set day 348
    for { set i 0 } { $i < 32 } { incr i } {
	incr day
	if { $day > 365 } {
	    incr year
	    set day 1
	}
	set d1 [blt::timestamp scan [format "%4d-%03d" $year $day]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test12 { t } {
    $t row delete @all
    set year 1980
    set month 0
    set day 350
    for { set i 0 } { $i < 45 } { incr i } {
	incr day
	if { $day > 365 } {
	    incr year
	    set day 1
	}
	set d1 [blt::timestamp scan [format "%4d-%03d" $year $day]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test13 { t } {
    $t row delete @all
    set year 1980
    set month 1
    set day 16
    for { set i 0 } { $i < 10 } { incr i } {
	incr day
	set date [format "%4d/%02d/%02d" $year $month $day]
	set d1 [blt::timestamp scan $date]
	puts stderr $date=[blt::timestamp format $d1]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test14 { t } {
    $t row delete @all
    set year 1980
    set day 350
    set hour 0
    for { set i 0 } { $i < 126 } { incr i } {
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set date [format "%4d-%03d %02d:00" $year $day $hour]
	set d1 [blt::timestamp scan $date]
	puts stderr \n$date=[blt::timestamp format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/12.0)]
	incr hour
    }
}

proc test15 { t } {
    $t row delete @all
    set year 1980
    set day 350
    set hour 0
    for { set i 0 } { $i < 34 } { incr i } {
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set date [format "%4d-%03d %02d:00" $year $day $hour]
	set d1 [blt::timestamp scan $date]
	puts stderr \n$date=[blt::timestamp format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/12.0)]
	incr hour
    }
}

proc test16 { t } {
    $t row delete @all
    set year 1980
    set month 0
    set day 350
    for { set i 0 } { $i < 34 } { incr i } {
	incr hour
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set d1 [blt::timestamp scan [format "%4d-%03d %02d:00" $year $day $hour]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test17 { t } {
    $t row delete @all
    set year 1980
    set month 0
    set day 350
    for { set i 0 } { $i < 14 } { incr i } {
	incr hour
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set d1 [blt::timestamp scan [format "%4d-%03d %02d:00" $year $day $hour]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test18 { t } {
    $t row delete @all
    set year 1980
    set hour 20
    set day 350
    for { set i 0 } { $i < 8 } { incr i } {
	incr hour
	if { $hour > 23 } {
	    incr day
	    set hour 0
	}
	set d1 [blt::timestamp scan [format "%4d-%03d %02d:00" $year $day $hour]]
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	puts stderr [blt::timestamp format [expr $d1]]
    }
}

proc test19 { t } {
    $t row delete @all
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
	set d1 [blt::timestamp scan $date]
	puts stderr \n$date=[blt::timestamp format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	incr min 10
    }
}

proc test20 { t } {
    $t row delete @all
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
	set d1 [blt::timestamp scan $date]
	puts stderr \n$date=[blt::timestamp format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	incr min 10
    }
}

proc test21 { t } {
    $t row delete @all
    set year 1980
    set hour 0
    set min 22
    set day 1
    for { set i 0 } { $i <= 30 } { incr i } {
	if { $min >= 60 } {
	    incr hour
	    set min 0
	}
	set date [format "%4d-%03d %02d:%02d" $year $day $hour $min]
	set d1 [blt::timestamp scan $date]
	puts stderr \n$date=[blt::timestamp format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	incr min 
    }
}

proc test22 { t } {
    $t row delete @all
    set year 1980
    set hour 0
    set min 22
    set day 1
    set sec 0
    for { set i 0 } { $i <= 30 } { incr i } {
	if { $sec >= 60 } {
	    incr min
	    set sec 0
	}
	set date [format "%4d-%03d %02d:%02d:%02d" $year $day $hour $min $sec]
	set d1 [blt::timestamp scan $date]
	puts stderr \n$date=[blt::timestamp format $d1]\n
	$t set $i "date" $d1
	$t set $i "value" [expr sin($i/2.0)]
	incr sec 
    }
}

test1 $t

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
    test1 "30 months, Major: every 1 year, Minor: 12 months"
    test2 "130 months, Major: every 2 years, Minor: every 1 year"
    test3 "25 years, Major: every 5 years, Minor: every 1 year"
    test4 "114 years, Major: every 20 years: Minor: every 10 years"
    test5 "24 months, Major: every 1 year, Minor: every 1 month"
    test6 "2 months, Major: every 1 week: Minor: every 1 day"
    test7 "7 months, Major: every 1 months: Minor: every 1 week"
    test8 "120 days, Major: every 1 month: Minor: every 1 week"
    test9 "5 days, Major: every 1 day: Minor: every 12 hours"
    test10 "12 days, Major: every 1 day: Minor: every 12 hours"
    test11 "32 days, Major: every 1 week: Minor: every 1 day"
    test12 "45 days, Major: every 1 week: Minor: every 1 day"
    test13 "10 days, Major: every 1 day: Minor: every 12 hours"
    test14 "126 hours, Major: every 1 day: Minor: every 12 hours"
    test15 "72 hours, Major: every 6 hours: Minor: every 4/6 hours"
    test16 "34 hours, Major: every 6 hours: Minor: every 4/6 hours"
    test17 "14 hours, Major: every 4 hours: Minor: every 1 hour"
    test18 "8 hours, Major: every 2 hours: Minor: every 30 minutes"
    test19 "2 hours, Major: every 20 minutes: Minor: every 10 minutes"
    test20 "2 hours, Major: every 20 minutes: Minor: every 10 minutes"
    test21 "32 minutes, Major: every 5 minutes: Minor: every 5/2 minutes"
    test22 "30 seconds, Major: every 5 seconds: Minor: every 5/2 minutes"
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

.g element bind all <Enter> {
    puts stderr "entered element [.g element get current]"
}
.g axis bind all <Enter> {
    puts stderr "entered axis [.g axis get current]"
}

.g axis configure all -activeforeground red

.g axis bind all <Enter> {
    set axis [%W axis get current]
    puts stderr ENTER=$axis
    %W axis activate $axis
    %W axis focus $axis
}

.g axis bind all <Leave> {
    set axis [%W axis get current]
    puts stderr LEAVE=$axis
    %W axis deactivate $axis
    %W axis focus ""
}
