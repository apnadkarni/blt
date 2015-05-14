#!../src/bltwish

package require BLT

# --------------------------------------------------------------------------
# Starting with Tcl 8.x, the BLT commands are stored in their own 
# namespace called "blt".  The idea is to prevent name clashes with
# Tcl commands and variables from other packages, such as a "table"
# command in two different packages.  
#
# You can access the BLT commands in a couple of ways.  You can prefix
# all the BLT commands with the namespace qualifier "blt::"
#  
#    blt::graph .g
#    blt::table . .g -resize both
# 
# or you can import all the command into the global namespace.
#
#    namespace import blt::*
#    graph .g
#    table . .g -resize both
#
# --------------------------------------------------------------------------

#source scripts/demo.tcl

set file ../demos/images/chalk.gif
set active ../demos/images/rain.gif


image create photo calendar.texture.1 -file $file
image create photo calendar.texture.2 -file $active

set bg [blt::background create tile -image calendar.texture.1]
set bg2 [blt::background create tile -image calendar.texture.2]

option add *HighlightThickness		0
option add *BltTkButton.foreground	navyblue
option add *BltTkLabel.ipadX			200
option add *Calendar.BltTkLabel.borderWidth	0
option add *Calendar.BltTkFrame.borderWidth	2
option add *Calendar.BltTkLabel.relief	sunken
option add *Calendar.BltTkFrame.relief	raised
option add *Calendar.BltTkLabel.font		{ {San Serif} 11 }
option add *Calendar.BltTkLabel.foreground	navyblue
option add *Calendar*background $bg
option add *Calendar.weekframe.Relief sunken

array set monthInfo {
    Jan { January 31 }
    Feb { February 28 } 
    Mar { March 31 } 
    Apr { April 30 } 
    May { May 31 } 
    Jun { June 30 } 
    Jul { July 31 }
    Aug { August 31 }
    Sep { September 30 }
    Oct { October 31 }
    Nov { November 30 }
    Dec { December 31 }
}

set abbrDays { Sun Mon Tue Wed Thu Fri Sat }

proc Calendar { weekday day month year } {
    global monthInfo abbrDays 
    
    set wkdayOffset [lsearch $abbrDays $weekday]
    if { $wkdayOffset < 0 } {
	error "Invalid week day \"$weekday\""
    }
    set dayOffset [expr ($day-1)%7]
    if { $wkdayOffset < $dayOffset } {
	set wkdayOffset [expr $wkdayOffset+7]
    }
    set wkday [expr $wkdayOffset-$dayOffset-1]
    if { [info commands .calendar] == ".calendar" } {
	destroy .calendar 
    }
    blt::tk::frame .calendar -class Calendar -width 3i -height 3i
    
    if ![info exists monthInfo($month)] {
	error "Invalid month \"$month\""
    }

    set info $monthInfo($month)
    blt::tk::label .calendar.month \
	-text "[lindex $info 0] $year"  \
	-font { {Sans Serif} 14 }
    blt::table .calendar .calendar.month 1,0 -cspan 7  -pady 10
    
    set cnt 0
    blt::tk::frame .calendar.weekframe -bd 1 
    blt::table .calendar .calendar.weekframe 2,0 -columnspan 7 -fill both  
    foreach dayName $abbrDays {
	set name [string tolower $dayName]
	blt::tk::label .calendar.$name \
	    -text $dayName \
	    -font { {Sans Serif} 9 }
	blt::table .calendar .calendar.$name 2,$cnt -pady 2 -padx 2
	incr cnt
    }
    blt::table configure .calendar c* r2 -pad 4 
    set week 0
    set numDays [lindex $info 1]
    for { set cnt 1 } { $cnt <= $numDays } { incr cnt } {
	blt::tk::label .calendar.day${cnt} -text $cnt 
	if { $cnt == $day } {
	    .calendar.day${cnt} configure -relief sunken -bd 1
	}
	incr wkday
	if { $wkday == 7 } {
	    incr week
	    set wkday 0
	}
	blt::table .calendar .calendar.day${cnt} $week+3,$wkday \
	    -fill both -ipadx 10 -ipady 4 
    }
    blt::tk::frame .calendar.quit -bd 1 -relief sunken
    blt::tk::button .calendar.quit.button -command { exit } -text {Quit} -bd 2 
    blt::table .calendar.quit \
	.calendar.quit.button -padx 4 -pady 4
    blt::table .calendar \
	.calendar.quit $week+4,5 -cspan 2 -pady 4 
    blt::table . \
	.calendar -fill both
    blt::table configure .calendar r0 -resize none
    blt::table configure .calendar c0 c6
}

set date [clock format [clock seconds] -format {%a %b %d %Y}]
scan $date { %s %s %d %d } weekday month day year

Calendar $weekday $day $month $year

puts stderr [.calendar.weekframe configure]
