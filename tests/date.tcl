
package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

set VERBOSE 0

test date.1 {date no args} {
    list [catch {blt::date} msg] $msg
} {1 {wrong # args: should be one of...
  blt::date debug date
  blt::date format seconds ?switches?
  blt::date scan date}}

test date.2 {date bad arg} {
    list [catch {blt::date badArg} msg] $msg
} {1 {bad operation "badArg": should be one of...
  blt::date debug date
  blt::date format seconds ?switches?
  blt::date scan date}}

test date.3 {date scan no arg} {
    list [catch {blt::date scan} msg] $msg
} {1 {wrong # args: should be "blt::date scan date"}}

test date.4 {date format no arg} {
    list [catch {blt::date format} msg] $msg
} {1 {wrong # args: should be "blt::date format seconds ?switches?"}}

test date.5 {date scan 0} {
    list [catch {blt::date scan 0} msg] $msg
} {0 -62167219200.0}

# One difference between "clock scan" and "blt::date scan" is that
# default timezone is always GMT, not the localtime zone.  
set date "Jun 21, 1968"
set d2 [clock scan $date -gmt yes]

test date.6 {date scan "Jun 21, 1968"} {
    list [catch {
	set d1 [blt::date scan "Jun 21, 1968"]
	expr { $d1 - double($d2) }
    } msg] $msg
} {0 0.0}

test date.7 {date scan "June 21, 1968"} {
    list [catch {
	set d1 [blt::date scan "June 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.8 {date scan "June 21 1968"} {
    list [catch {
	set d1 [blt::date scan "June 21 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.9 {date scan "21 June 1968"} {
    list [catch {
	set d1 [blt::date scan "21 June 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.10 {date scan "6 21 1968"} {
    list [catch {
	set d1 [blt::date scan "6 21 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.11 {date scan "6-21-1968"} {
    list [catch {
	set d1 [blt::date scan "6-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.12 {date scan "06-21-1968"} {
    list [catch {
	set d1 [blt::date scan "06-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.13 {date scan Jun-21-1968} {
    list [catch {
	set d1 [blt::date scan "Jun-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.14 {date scan 06/21/1968} {
    list [catch {
	set d1 [blt::date scan "06/21/1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.15 {date scan "6/21/1968"} {
    list [catch {
	set d1 [blt::date scan "6/21/1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.16 {date scan "1968-June-21"} {
    list [catch {
	set d1 [blt::date scan "1968-June-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.17 {date scan "Tuesday Jun 21, 1968"} {
    list [catch {
	set d1 [blt::date scan "Tuesday Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.18 {date scan "Wednesday Jun 21, 1968"} {
    list [catch {
	set d1 [blt::date scan "Wednesday Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.19 {date scan "Wed Jun 21, 1968"} {
    list [catch {
	set d1 [blt::date scan "Wed Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.20 {date scan "Wed. Jun. 21, 1968"} {
    list [catch {
	set d1 [blt::date scan "Wed. Jun. 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.21 {date scan "Wed, 21 Jun 1968"} {
    list [catch {
	set d1 [blt::date scan "Wed, 21 Jun 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.22 {date scan "Thu, 21 Jun 68 00:00:00 GMT"} {
    list [catch {
	set d1 [blt::date scan "Thu, 21 Jun 68 00:00:00 GMT"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.23 {date scan "19680621"} {
    list [catch {
	set d1 [blt::date scan "19680621"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.24 {date scan "1968-Jun-21"} {
    list [catch {
	set d1 [blt::date scan "1968-Jun-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.25 {date scan "1968-June-21"} {
    list [catch {
	set d1 [blt::date scan "1968-June-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.26 {date scan "1968-06-21"} {
    list [catch {
	set d1 [blt::date scan "1968-06-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.27 {date scan "1968-6-21"} {
    list [catch {
	set d1 [blt::date scan "1968-6-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 1, 1970"
set d2 [clock scan $date -gmt yes]

test date.28 {date scan "1970"} {
    list [catch {
	set d1 [blt::date scan "1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.29 {date scan "January 1970"} {
    list [catch {
	set d1 [blt::date scan "January 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.30 {date scan "Jan 1970"} {
    list [catch {
	set d1 [blt::date scan "Jan 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.31 {date scan "1 Jan 1970"} {
    list [catch {
	set d1 [blt::date scan "1 Jan 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.32 {date scan "1/1"} {
    list [catch {
 	set d1 [blt::date scan "1/1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.33 {date scan "1970 Jan"} { 
    list [catch {
 	set d1 [blt::date scan "1970 Jan"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.34 {date scan "1970 Jan 1"} { 
    list [catch {
 	set d1 [blt::date scan "1970 Jan 1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.35 {date scan "00:00"} {
    list [catch {
	set d1 [blt::date scan "00:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.36 {date scan "00:00:00"} {
    list [catch {
	set d1 [blt::date scan "00:00:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# hh:mm:ss.fff
test date.37 {date scan "00:00:00.000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# A 6-digit number is interpretered as hhmmss.
test date.38 {date scan "000000"} {
    list [catch {
	set d1 [blt::date scan "000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# hh.mm.ss
test date.39 {date scan "00.00.00"} {
    list [catch {
	set d1 [blt::date scan "00.00.00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.40 {date scan "00.00.00.000"} {
    list [catch {
	set d1 [blt::date scan "00.00.00.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.41 {date scan "00.00.00.00000000"} {
    list [catch {
	set d1 [blt::date scan "00.00.00.00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.42 {date scan "00:00:00,00000000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00,00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.43 {date scan "t00:00:00,00000000"} {
    list [catch {
	set d1 [blt::date scan "t00:00:00,00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.44 {date scan "00:00:00+0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00+0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.45 {date scan "00:00:00-0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.46 {date scan "00:00:00 gmt"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 gmt"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.47 {date scan "00:00:00 utc"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 utc"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.48 {date scan "00:00:00z"} {
    list [catch {
	set d1 [blt::date scan "00:00:00z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.49 {date scan "00:00:00 z"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.50 {date scan "00:00:00 gmt+0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 gmt+0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.51 {date scan "00:00:00 gmt-0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 gmt-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.52 {date scan "00:00:00 GMT-0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 GMT-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# This is pretty useless.  It's the epoch with the timezone GMT.
test date.53 {date scan "GMT"} {
    list [catch {
	set d1 [blt::date scan "GMT"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Allow empty strings to successfully pass (returns 0).
test date.54 {date scan ""} {
    list [catch {
 	set d1 [blt::date scan ""]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Word-ie days like 23rd can be unambiguously parsed as the day of the month.
test date.55 {date scan "Jan 1st"} {
    list [catch {
 	set d1 [blt::date scan "Jan 1st"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year (1-366) plus time.  The default year is the epoch.
test date.56 {date scan "+001:00:00:01"} {
    list [catch {
 	set d1 [blt::date scan "+001:00:00:01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

# "Doctor it hurts when I use 2 digit years." This the 1st day of January,
# not January of year 1.  1-2 digit years and day of the month are
# ambiguous, so we picked the day of year.  I assume that "Feb 20" is more
# likely to mean February 20th instead of Febrary 1920.
test date.57 {date scan "Jan 1"} {
    list [catch {
 	set d1 [blt::date scan "Jan 1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Jan 1 1970 is a Thurday. Note that ISO weekdays are Monday 1 - Sunday 7.  
test date.58 {date scan "1970-W01-4"} { 
    list [catch {
 	set d1 [blt::date scan "1970-W01-4"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year.
test date.59 {date scan "1970-001"} { 
    list [catch {
 	set d1 [blt::date scan "1970-001"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year (1-366).  The default year is the epoch.
test date.60 {date scan "001"} {
    list [catch {
 	set d1 [blt::date scan "001"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 2, 2006 15:04:05 MST"
set d2 [clock scan $date -gmt yes]

# This date was found in Go.  There's no +- on the timezone offset.
# Without the colon, the offset is confused with 4 digit years.
test date.61 {date scan "2006-01-02T15:04:05Z07:00"} {
    list [catch {
 	set d1 [blt::date scan "2006-01-02T15:04:05Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.62 {date scan "Mon Jan 2 15:04:05 MST 2006"} {
    list [catch {
 	set d1 [blt::date scan "Mon Jan 2 15:04:05 MST 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.63 {date scan "Mon Jan 02 15:04:05 -0700 2006"} {
    list [catch {
 	set d1 [blt::date scan "Mon Jan 02 15:04:05 -0700 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.64 {date scan "Mon Jan 02 15:04:05 -07 2006"} {
    list [catch {
 	set d1 [blt::date scan "Mon Jan 02 15:04:05 -07 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.65 {date scan "Mon Jan 02 15:04:05 -7 2006"} {
    list [catch {
 	set d1 [blt::date scan "Mon Jan 02 15:04:05 -7 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Dec 29, 1969"
set d2 [clock scan $date -gmt yes]

# Jan 1 1970 is a Thurday.  So the first week starts on Monday of the
# previous year.
test date.66 {date scan "W01 1970"} { 
    list [catch {
 	set d1 [blt::date scan "W01 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 2, 2006 15:04:05"
set d2 [clock scan $date -gmt yes]

test date.67 {date scan "Mon Jan 2 15:04:05 2006"} {
    list [catch {
 	set d1 [blt::date scan "Mon Jan 2 15:04:05 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 2, 1906 15:04 MST"
set d2 [clock scan $date -gmt yes]

# "Doctor it hurts when I use 2 digit years." The year "06" is 1906, not
# 2006.  The base of the century of a 2 digit year is always assumed to be
# 1900, not 2000.  This because it's more likely that files generated in
# the 20th century will contain 2 digit years, than ones in 21st.
test date.68 {date scan "02 Jan 06 15:04 MST"} {
    list [catch {
 	set d1 [blt::date scan "02 Jan 06 15:04 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


test date.69 {date scan "02 Jan 06 15:04 -0700"} {
    list [catch {
 	set d1 [blt::date scan "02 Jan 06 15:04 -0700"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.70 {date scan "Monday, 02-Jan-06 15:04:05"} {
    list [catch {
 	set d1 [blt::date scan "Monday, 02-Jan-06 15:04:05 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

set date "Jan 2, 2006 15:04 MST"
set d2 [clock scan $date -gmt yes]

test date.71 {date scan "Mon, 02 Jan 2006 15:04:05 MST"} {
    list [catch {
 	set d1 [blt::date scan "Mon, 02 Jan 2006 15:04:05 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test date.72 {date scan "Mon, 02 Jan 2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::date scan "Mon, 02 Jan 2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test date.73 {date scan "01/02/2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::date scan "01/02/2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test date.74 {date scan "01-02-2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::date scan "01-02-2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test date.75 {date scan "Mon, 02 Jan 2006 15:04:05 -0700"} {
    list [catch {
 	set d1 [blt::date scan "Mon, 02 Jan 2006 15:04:05 -0700"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test date.76 {date scan "2006-01-02T15:04:05Z07:00"} {
    list [catch {
 	set d1 [blt::date scan "2006-01-02T15:04:05Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test date.77 {date scan "2006-01-02T15:04:05.999999999Z07:00"} {
    list [catch {
 	set d1 [blt::date scan "2006-01-02T15:04:05.999999999Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 6.0}

set date "Jan 1, 1970 03:04pm"
set d2 [clock scan $date -gmt yes]

# The default year/month/day is Jan 1st 1970 (the start of the epoch).
# This lets you use times without dates that act like a duration (probably
# without the PM).
test date.78 {date scan "3:04PM"} {
    list [catch {
 	set d1 [blt::date scan "3:04PM"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 2, 2006 15:04"
set d2 [clock scan $date -gmt yes]

test date.79 {date scan "Jan 2 15:04:05 2006"} {
    list [catch {
 	set d1 [blt::date scan "Jan 2 15:04:05 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

# Don't test with non-zero fractions yet.  
test date.80 {date scan "Jan 2 2006 15:04:05.000"} {
    list [catch {
 	set d1 [blt::date scan "Jan 2 2006 15:04:05.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test date.81 {date scan "2006 Jan 2 15:04:05.000000"} {
    list [catch {
 	set d1 [blt::date scan "2006 Jan 2 15:04:05.000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test date.82 {date scan "Jan 2 2006 15:04:05.000000000"} {
    list [catch {
 	set d1 [blt::date scan "Jan 2 2006 15:04:05.000000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}


set date "Jan 31, 2012"
set d2 [clock scan $date -gmt yes]

test date.83 {date scan "31 Jan 2012"} {
    list [catch {
 	set d1 [blt::date scan "31 Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.84 {date scan "31-Jan-2012"} {
    list [catch {
 	set d1 [blt::date scan "31-Jan-2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.85 {date scan "31. Jan 2012"} {
    list [catch {
 	set d1 [blt::date scan "31. Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.86 {date scan "31/Jan/2012"} {
    list [catch {
 	set d1 [blt::date scan "31/Jan/2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.87 {date scan "31 Jan 2012"} {
    list [catch {
 	set d1 [blt::date scan "31 Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


test date.88 {date scan "31/Jan/2012"} {
    list [catch {
 	set d1 [blt::date scan "31/Jan/2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# A 3 digit number is interpretered as the day of the year. 
# +ddd:hh:mm:ss YYYY
test date.89 {date scan "+031:00:00:00 2012"} {
    list [catch {
 	set d1 [blt::date scan "+031:00:00:00 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 1, 2005 1:29PM"
set d2 [clock scan $date -gmt yes]

test date.90 {date scan "Jan 1 2005 1:29PM"} {
    list [catch {
 	set d1 [blt::date scan "Jan 1 2005 1:29PM"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Nov 23, 1998"
set d2 [clock scan $date -gmt yes]

test date.91 {date scan "11/23/98"} {
    list [catch {
 	set d1 [blt::date scan "11/23/98"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 1, 1972"
set d2 [clock scan $date -gmt yes]

# "Doctor it hurts when I use 2 digit years." Normally NN.NN.NN would
# be interpreted as a time.  This works only because the 72 isn't a valid
# hour specification.   
test date.92 {date scan "72.01.01"} {
    list [catch {
 	set d1 [blt::date scan "72.01.01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# It's always year month day.
test date.93 {date scan "1972.01.01"} {
    list [catch {
 	set d1 [blt::date scan "1972.01.01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Feb 19, 1972"
set d2 [clock scan $date -gmt yes]

# Normally this is interpreted as month/day/year.  This works because 19
# isn't a valid month specification.  Use month names or abbreviations.
test date.94 {date scan "19/02/72"} {
    list [catch {
 	set d1 [blt::date scan "19/02/72"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# This is parsed as day/month/year. 
test date.95 {date scan "19/02/1972"} {
    list [catch {
 	set d1 [blt::date scan "19/02/1972"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Dec 25, 1905"
set d2 [clock scan $date -gmt yes]

test date.96 {date scan "25/12/05"} {
    list [catch {
 	set d1 [blt::date scan "25/12/05"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Dec 05, 1925"
set d2 [clock scan $date -gmt yes]

# This is an example of an accepted, but ambiguous format. NN.NN.NN is 
# normally interpreted as a time.  But since 25 isn't a valid hour 
# specification, it's interpreted as a date.  With dots as separators
# we assume that this is dd.mm.yy".  The year 25 is 1925 not 2025.
test date.97 {date scan "25.12.05"} {
    list [catch {
 	set d1 [blt::date scan "25.12.05"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 24, 1998"
set d2 [clock scan $date -gmt yes]

# Another accepted, but ambiguous format.  24 isn't a valid month
# specification.
test date.98 {date scan "24-01-98"} {
    list [catch {
 	set d1 [blt::date scan "24-01-98"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "April 1, 1902"
set d2 [clock scan $date -gmt yes]

# Is this "day-month-year" or "year-month-day" or "month-day-year"?  It's
# parsed as "dd-month-day-year". And "02" is 1902 not 2002.
test date.99 {date scan "04-01-02"} {
    list [catch {
 	set d1 [blt::date scan "04-01-02"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 24, 1998"
set d2 [clock scan $date -gmt yes]

test date.100 {date scan "24-01-1998"} {
    list [catch {
 	set d1 [blt::date scan "24-01-1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 04, 1906"
set d2 [clock scan $date -gmt yes]

test date.101 {date scan "04 Jul 06"} {
    list [catch {
 	set d1 [blt::date scan "04 Jul 06"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 04, 2006"
set d2 [clock scan $date -gmt yes]

test date.102 {date scan "04 Jul 2006"} {
    list [catch {
 	set d1 [blt::date scan "04 Jul 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 24, 1998"
set d2 [clock scan $date -gmt yes]

test date.103 {date scan "Jan 24, 98"} {
    list [catch {
 	set d1 [blt::date scan "Jan 24, 1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.104 {date scan "Jan 24, 1998"} {
    list [catch {
 	set d1 [blt::date scan "Jan 24, 1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 1, 1970 03:24:53"
set d2 [clock scan $date -gmt yes]

test date.105 {date scan "03:24:53"} {
    list [catch {
 	set d1 [blt::date scan "03:24:53"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Apr 28, 2006 12:32:29PM"
set d2 [clock scan $date -gmt yes]

test date.106 {date scan "Apr 28 2006 12:32:29:253PM"} {
    list [catch {
 	set d1 [blt::date scan "Apr 28 2006 12:32:29:253PM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.253}

set date "Jan 1, 1906"
set d2 [clock scan $date -gmt yes]

test date.107 {date scan "01-01-06"} {
    list [catch {
 	set d1 [blt::date scan "01-01-06"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 1, 2006"
set d2 [clock scan $date -gmt yes]

test date.108 {date scan "01-01-2006"} {
    list [catch {
 	set d1 [blt::date scan "01-01-2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Nov 23, 1998"
set d2 [clock scan $date -gmt yes]

test date.109 {date scan "98/11/23"} {
    list [catch {
 	set d1 [blt::date scan "1998/11/23"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# NNNNNN is always a time pattern.  Better to fail on 98 as an hour than to
# parse as a date and silently switch back to time in 2 years "001123".  If
# you really want a number as the date, then use "19981123".
test date.110 {date scan "981123"} {
    list [catch {
 	set d1 [blt::date scan "981123"]
	expr { $d1 - $d2 }
    } msg] $msg
} {1 {hour "98" is out of range.}}

test date.111 {date scan "19981123"} {
    list [catch {
 	set d1 [blt::date scan "19981123"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Apr 28, 2006 00:34:55"
set d2 [clock scan $date -gmt yes]

test date.112 {date scan "28 Apr 2006 00:34:55:190"} {
    list [catch {
 	set d1 [blt::date scan "28 Apr 2006 00:34:55:190"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.19}

set date "Jan 1, 1970 11:34:23"
set d2 [clock scan $date -gmt yes]

test date.113 {date scan "11:34:23:013"} {
    list [catch {
 	set d1 [blt::date scan "11:34:23:013"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.013}

set date "Jan 1, 1972 13:42:24"
set d2 [clock scan $date -gmt yes]

test date.114 {date scan "1972-01-01 13:42:24"} {
    list [catch {
 	set d1 [blt::date scan "1972-01-01 13:42:24"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Feb 19, 1972 06:35:24"
set d2 [clock scan $date -gmt yes]

test date.115 {date scan "1972-02-19 06:35:24.489"} {
    list [catch {
 	set d1 [blt::date scan "1972-02-19 06:35:24.489"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.489}

set date "Nov 23, 1998 11:25:43"
set d2 [clock scan $date -gmt yes]

test date.116 {date scan "1998-11-23T11:25:43:250"} {
    list [catch {
 	set d1 [blt::date scan "1998-11-23T11:25:43:250"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.25}

set date "Apr 28, 2006 12:39:32AM"
set d2 [clock scan $date -gmt yes]

test date.117 {date scan "28 Apr 2006 12:39:32:429AM"} {
    list [catch {
 	set d1 [blt::date scan "28 Apr 2006 12:39:32:429AM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.429}

# This is parsed dd/mm/yyyy because 28 is not a valid month.
# NN/NN/NNNN is normally parsed as mm/dd/yyyy.  
# FIXME: probably should fail on an invalid month.
test date.118 {date scan "28/04/2006 12:39:32:429AM"} {
    list [catch {
 	set d1 [blt::date scan "28/04/2006 12:39:32:429AM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.429}

set date "Jan 2, 2003"
set d2 [clock scan $date -gmt yes]

# Slashes assume is format mm/dd/yyyy.
test date.119 {date scan "01/02/2003"} {
    list [catch {
 	set d1 [blt::date scan "01/02/2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Same with dashes mm-dd-yyyy.
test date.120 {date scan "01-02-2003"} {
    list [catch {
 	set d1 [blt::date scan "01-02-2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# But periods are dd.mm.yyyy.
test date.121 {date scan "02.01.2003"} {
    list [catch {
 	set d1 [blt::date scan "02.01.2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 2, 1903"
set d2 [clock scan $date -gmt yes]

test date.122 {date scan "01-02-03"} {
    list [catch {
 	set d1 [blt::date scan "01-02-03"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


set date "Oct 30, 2005 10:45"
set d2 [clock scan $date -gmt yes]

test date.123 {date scan "2005-10-30 T 10:45 UTC"} { 
    list [catch {
 	set d1 [blt::date scan "2005-10-30 T 10:45 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Nov 09, 2007 11:20"
set d2 [clock scan $date -gmt yes]

test date.124 {date scan "2007-11-09 T 11:20 UTC"} { 
    list [catch {
 	set d1 [blt::date scan "2007-11-09 T 11:20 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 23, 2005 02:16:57"
set d2 [clock scan $date -gmt yes]

test date.125 {date scan "Sat Jul 23 02:16:57 2005"} { 
    list [catch {
 	set d1 [blt::date scan "Sat Jul 23 02:16:57 2005"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 21, 1969 02:56"
set d2 [clock scan $date -gmt yes]

test date.126 {date scan "1969-07-21 T 02:56 UTC"} { 
    list [catch {
 	set d1 [blt::date scan "1969-07-21 T 02:56 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Dec 11, 2012 07:38"
set d2 [clock scan $date -gmt yes]

test date.127 {date scan "07:38, 11 December 2012 (UTC)"} { 
    list [catch {
 	set d1 [blt::date scan "07:38, 11 December 2012 (UTC)"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 1, 1997"
set d2 [clock scan $date -gmt yes]

test date.128 {date scan "1997"} { 
    list [catch {
 	set d1 [blt::date scan "1997"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 1, 1997"
set d2 [clock scan $date -gmt yes]

test date.129 {date scan "1997-07"} { 
    list [catch {
 	set d1 [blt::date scan "1997-07"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 16, 1997"
set d2 [clock scan $date -gmt yes]

test date.130 {date scan "1997-07-16"} { 
    list [catch {
 	set d1 [blt::date scan "1997-07-16"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# 5 digit year
test date.131 {date scan "01997-07-16"} { 
    list [catch {
 	set d1 [blt::date scan "1997-07-16"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 16, 1997 19:20a"
set d2 [clock scan $date -gmt yes]

test date.132 {date scan "1997-07-16T19:20+01:00"} { 
    list [catch {
 	set d1 [blt::date scan "1997-07-16T19:20+01:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 16, 1997 19:20:30a"
set d2 [clock scan $date -gmt yes]

test date.133 {date scan "1997-07-16T19:20:30+01:00"} { 
    list [catch {
 	set d1 [blt::date scan "1997-07-16T19:20:30+01:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jul 16, 1997 19:20:30a"
set d2 [clock scan $date -gmt yes]

test date.134 {date scan "1997-07-16T19:20:30.45+01:00"} { 
    list [catch {
 	set d1 [blt::date scan "1997-07-16T19:20:30.45+01:00"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.45}

set date "Apr 12, 1985 23:20:50"
set d2 [clock scan $date -gmt yes]

test date.135 {date scan "1985-04-12T23:20:50.52Z"} { 
    list [catch {
 	set d1 [blt::date scan "1985-04-12T23:20:50.52Z"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.52}
      
set date "Dec 19, 1996 16:39:57u"
set d2 [clock scan $date -gmt yes]

test date.136 {date scan "1996-12-19T16:39:57-08:00"} { 
    list [catch {
 	set d1 [blt::date scan "1996-12-19T16:39:57-08:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Dec 31, 1990 23:59:59"
set d2 [clock scan $date -gmt yes]

# Handle leap second.
test date.137 {date scan "1990-12-31T23:59:60Z"} { 
    list [catch {
 	set d1 [blt::date scan "1990-12-31T23:59:60Z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

set date "Dec 31, 1990 15:59:59u"
set d2 [clock scan $date -gmt yes]

# Handle leap second.
test date.138 {date scan "1990-12-31T15:59:60-08:00"} { 
    list [catch {
 	set d1 [blt::date scan "1990-12-31T15:59:60-08:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

set date "Jan 1, 1937 12:00:27"
set d2 [clock scan $date -gmt yes]

test date.139 {date scan "1937-01-01T12:00:27.87+00:20"} { 
    list [catch {
 	set d1 [blt::date scan "1937-01-01T12:00:27.87+00:20"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 -1199.13};				# 20 minutes from GMT + .87 seconds
      
set date "Dec 17, 1997 07:37:16 PST"
set d2 [clock scan $date -gmt yes]

# ISO 	ISO 8601/SQL standard 	
test date.140 {date scan "1997-12-17 07:37:16-08"} { 
    list [catch {
 	set d1 [blt::date scan "1997-12-17 07:37:16-08"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# SQL 	traditional style 	
test date.141 {date scan "12/17/1997 07:37:16.00 PST"} { 
    list [catch {
 	set d1 [blt::date scan "12/17/1997 07:37:16.00 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# POSTGRES 	original style
test date.142 {date scan "Wed Dec 17 07:37:16 1997 PST"} { 
    list [catch {
 	set d1 [blt::date scan "Wed Dec 17 07:37:16 1997 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# German 	regional style
test date.143 {date scan "17.12.1997 07:37:16.00 PST"} { 
    list [catch {
 	set d1 [blt::date scan "17.12.1997 07:37:16.00 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 1, 2005"
set d2 [clock scan $date -gmt yes]

test date.144 {date scan "2004-W53-6"} { 
    list [catch {
 	set d1 [blt::date scan "2004-W53-6"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 2, 2005"
set d2 [clock scan $date -gmt yes]

test date.145 {date scan "2004-W53-7"} { 
    list [catch {
 	set d1 [blt::date scan "2004-W53-7"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


set date "Dec 31, 2005"
set d2 [clock scan $date -gmt yes]

test date.146 {date scan "2005-W52-6"} { 
    list [catch {
 	set d1 [blt::date scan "2005-W52-6"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 1, 2007"
set d2 [clock scan $date -gmt yes]

test date.147 {date scan "2007-W01-1"} { 
    list [catch {
 	set d1 [blt::date scan "2007-W01-1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Dec 25, 2016"
set d2 [clock scan $date -gmt yes]

test date.148 {date scan "Dec 25, 2016"} { 
    list [catch {
 	set d1 [blt::date scan "Dec 25, 2016"]
	blt::date format $d1 -format "%V %U"
    } msg] $msg
} {0 {51 53}}

test date.149 {date scan "Jan 1, 2014"} { 
    list [catch {
 	set d1 [blt::date scan "Jan 1, 2014"]
	blt::date format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 01}}

test date.150 {date scan "Jan 4, 2014"} { 
    list [catch {
 	set d1 [blt::date scan "Jan 4, 2014"]
	blt::date format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 01}}

test date.151 {date scan "Jan 5, 2014"} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 02}}

test date.152 {date scan "Jan 5, 1969"} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 02}}

# Default format
test date.153 {date format} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1
    } msg] $msg
} {0 {Sun Jan 05 00:00:00 +0000 1969}}

# Percent
test date.154 {date format %} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%%"
    } msg] $msg
} {0 %}

# Abbreviated weekday
test date.155 {date format %a} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%a"
    } msg] $msg
} {0 Sun}

# Weekday
test date.156 {date format %A} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%A"
    } msg] $msg
} {0 Sunday}

# Abbreviated month.
test date.157 {date format %b} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%b"
    } msg] $msg
} {0 Jan}

# Month
test date.158 {date format %B} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%B"
    } msg] $msg
} {0 January}

# Abbreviated month.
test date.159 {date format %h} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%h"
    } msg] $msg
} {0 Jan}

# Date and time.
test date.160 {date format %c} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%c"
    } msg] $msg
} {0 {Sun Jan 5 00:00:00 1969}}

# Century without last two digits.
test date.161 {date format %C} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%C"
    } msg] $msg
} {0 19}

# Day of month.
test date.162 {date format %d} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%d"
    } msg] $msg
} {0 05}

# Date mm/dd/yy
test date.163 {date format %D} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%D"
    } msg] $msg
} {0 01/05/69}

# Day of month space padded
test date.164 {date format %e} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%e"
    } msg] $msg
} {0 { 5}}

# Full date yyyy-mm-dd
test date.165 {date format %F} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%F"
    } msg] $msg
} {0 1969-01-05}

# Last 2 digits of ISO year
test date.166 {date format %g} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%g"
    } msg] $msg
} {0 69}

# ISO year
test date.167 {date format %G} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%G"
    } msg] $msg
} {0 1969}

# ISO year
test date.168 {date format %G} { 
    list [catch {
 	set d1 [blt::date scan "W01 1970"]
	blt::date format $d1 -format "%G"
    } msg] $msg
} {0 1969}

# Hour
test date.169 {date format %H} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969"]
	blt::date format $d1 -format "%H"
    } msg] $msg
} {0 00}

# Hour
test date.170 {date format %H} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 11:00"]
	blt::date format $d1 -format "%H"
    } msg] $msg
} {0 11}

# Hour
test date.171 {date format %H} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 11:00pm"]
	blt::date format $d1 -format "%H"
    } msg] $msg
} {0 23}

# Hour (01-12)
test date.172 {date format %I} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 00:00"]
	blt::date format $d1 -format "%I"
    } msg] $msg
} {0 12}

# Day of year (1-366)
test date.173 {date format %j} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 00:00"]
	blt::date format $d1 -format "%j"
    } msg] $msg
} {0 005}


# Hour space padded
test date.174 {date format %k} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 00:00"]
	blt::date format $d1 -format "%k"
    } msg] $msg
} {0 { 0}}

# Hour space padded
test date.175 {date format %k} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:00"]
	blt::date format $d1 -format "%k"
    } msg] $msg
} {0 13}

# Hour space padded
test date.176 {date format %l} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:00"]
	blt::date format $d1 -format "%l"
    } msg] $msg
} {0 { 1}}

# Month 01-12
test date.177 {date format %m} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:00"]
	blt::date format $d1 -format "%m"
    } msg] $msg
} {0 01}

# Minute
test date.178 {date format %M} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:00"]
	blt::date format $d1 -format "%M"
    } msg] $msg
} {0 00}

# Minute
test date.179 {date format %M} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:59"]
	blt::date format $d1 -format "%M"
    } msg] $msg
} {0 59}

# Nanoseconds
test date.180 {date format %N} { 
    list [catch {
 	set d1 [blt::date scan "Jan 1, 1970"]
	blt::date format $d1 -format "%N"
    } msg] $msg
} {0 0.0}

# AM/PM
test date.181 {date format %P} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:59"]
	blt::date format $d1 -format "%P"
    } msg] $msg
} {0 pm}

# AM/PM
test date.182 {date format %p} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:59"]
	blt::date format $d1 -format "%p"
    } msg] $msg
} {0 PM}

# 12 hour clock
test date.183 {date format %r} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:59"]
	blt::date format $d1 -format "%r"
    } msg] $msg
} {0 {01:59:00 PM}}

# 24 hour clock
test date.184 {date format %R} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 1969 13:59"]
	blt::date format $d1 -format "%R"
    } msg] $msg
} {0 13:59}

# Seconds since epoch
test date.185 {date format %s} { 
    list [catch {
 	set d1 [blt::date scan "Jan 1, 1970"]
	blt::date format $d1 -format "%s"
    } msg] $msg
} {0 0}

# Seconds since epoch
test date.186 {date format %s} { 
    list [catch {
 	set d1 [blt::date scan "Jan 2, 1970"]
	blt::date format $d1 -format "%s"
    } msg] $msg
} {0 86400}

# Second (00-59)
test date.187 {date format %S} { 
    list [catch {
 	set d1 [blt::date scan "Jan 1, 1970"]
	blt::date format $d1 -format "%S"
    } msg] $msg
} {0 00}

# Second (00-59)
test date.188 {date format %S} { 
    list [catch {
 	set d1 [blt::date scan "Jan 1, 1970 00:00:59"]
	blt::date format $d1 -format "%S"
    } msg] $msg
} {0 59}

# Second (00-59) does not contain fraction
test date.189 {date format %S} { 
    list [catch {
 	set d1 [blt::date scan "Jan 1, 1970 00:00:59.9999123"]
	blt::date format $d1 -format "%S"
    } msg] $msg
} {0 59}

# Time hh:mm:ss
test date.190 {date format %T} { 
    list [catch {
 	set d1 [blt::date scan "Jan 1, 1970 01:02:03"]
	blt::date format $d1 -format "%T"
    } msg] $msg
} {0 01:02:03}

# Day of week 1-7 Sunday = 1
test date.191 {date format %u} { 
    list [catch {
 	set d1 [blt::date scan "Jan 1, 1970"]
	blt::date format $d1 -format "%u"
    } msg] $msg
} {0 5}

# Day of week 1-7 Sunday = 1
test date.192 {date format %u} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%u"
    } msg] $msg
} {0 1}

# Week number (01-53).  Sunday is first day of week.
test date.193 {date format %U} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%U"
    } msg] $msg
} {0 02}

# ISO week number (01-53).  Monday is first day of week.
test date.194 {date format %V} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%V"
    } msg] $msg
} {0 01}

# Week day (0-6). Sunday is 0.
test date.195 {date format %w} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%w"
    } msg] $msg
} {0 0}

# Week (00-53). Monday is the first day of week. (I don't know what this
# is.)
test date.196 {date format %W} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%W"
    } msg] $msg
} {0 01}

# Date representation mm/dd/yy
test date.197 {date format %x} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%x"
    } msg] $msg
} {0 01/05/14}

# Year last 2 digits (yy)
test date.198 {date format %y} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%y"
    } msg] $msg
} {0 14}

# Year 4 digits (yyyy)
test date.199 {date format %Y} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%Y"
    } msg] $msg
} {0 2014}

# Year 5 digits (yyyyy)
test date.200 {date format %Y} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 12014"]
	blt::date format $d1 -format "%Y"
    } msg] $msg
} {0 12014}

# Timezone
test date.201 {date format %z} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014"]
	blt::date format $d1 -format "%z"
    } msg] $msg
} {0 +0000}

# Timezone (always +0000).
test date.202 {date format %z} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014 PST"]
	blt::date format $d1 -format "%z"
    } msg] $msg
} {0 +0000}

# Mixed.
test date.203 {date format "abcd%aefghi"} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014 PST"]
	blt::date format $d1 -format "abcd%aefghi"
    } msg] $msg
} {0 abcdSunefghi}

# Non-substitutions.
test date.204 {date format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z"} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014 PST"]
	blt::date format $d1 -format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z"
    } msg] $msg
} {0 %E%f%i%J%K%L%n%q%Q%t%v%X%Z}

# Non-substitutions.
test date.204 {date format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z%%s"} { 
    list [catch {
 	set d1 [blt::date scan "Jan 5, 2014 PST"]
	blt::date format $d1 -format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z%%s"
    } msg] $msg
} {0 %E%f%i%J%K%L%n%q%Q%t%v%X%Z%s}

