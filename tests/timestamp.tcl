
package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

set VERBOSE 0

test timestamp.1 {timestamp no args} {
    list [catch {blt::timestamp} msg] $msg
} {1 {wrong # args: should be one of...
  blt::timestamp format seconds ?switches?
  blt::timestamp parse timeStamp
  blt::timestamp scan timeStamp}}

test timestamp.2 {timestamp bad arg} {
    list [catch {blt::timestamp badArg} msg] $msg
} {1 {bad operation "badArg": should be one of...
  blt::timestamp format seconds ?switches?
  blt::timestamp parse timeStamp
  blt::timestamp scan timeStamp}}

test timestamp.3 {timestamp scan no arg} {
    list [catch {blt::timestamp scan} msg] $msg
} {1 {wrong # args: should be "blt::timestamp scan timeStamp"}}

test timestamp.4 {timestamp format no arg} {
    list [catch {blt::timestamp format} msg] $msg
} {1 {wrong # args: should be "blt::timestamp format seconds ?switches?"}}

test timestamp.5 {timestamp scan 0} {
    list [catch {blt::timestamp scan 0} msg] $msg
} {0 -62167219200.0}

# One difference between "clock scan" and "blt::timestamp scan" is that the
# default timezone is always GMT, not the _locale timezone.
set timestamp "Jun 21, 1968"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.6 {timestamp scan "Jun 21, 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "Jun 21, 1968"]
	expr { $d1 - double($d2) }
    } msg] $msg
} {0 0.0}

test timestamp.7 {timestamp scan "June 21, 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "June 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.8 {timestamp scan "June 21 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "June 21 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.9 {timestamp scan "21 June 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "21 June 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.10 {timestamp scan "6 21 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "6 21 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.11 {timestamp scan "6-21-1968"} {
    list [catch {
	set d1 [blt::timestamp scan "6-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.12 {timestamp scan "06-21-1968"} {
    list [catch {
	set d1 [blt::timestamp scan "06-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.13 {timestamp scan Jun-21-1968} {
    list [catch {
	set d1 [blt::timestamp scan "Jun-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.14 {timestamp scan 06/21/1968} {
    list [catch {
	set d1 [blt::timestamp scan "06/21/1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.15 {timestamp scan "6/21/1968"} {
    list [catch {
	set d1 [blt::timestamp scan "6/21/1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.16 {timestamp scan "1968-June-21"} {
    list [catch {
	set d1 [blt::timestamp scan "1968-June-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.17 {timestamp scan "Tuesday Jun 21, 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "Tuesday Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.18 {timestamp scan "Wednesday Jun 21, 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "Wednesday Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.19 {timestamp scan "Wed Jun 21, 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "Wed Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.20 {timestamp scan "Wed. Jun. 21, 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "Wed. Jun. 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.21 {timestamp scan "Wed, 21 Jun 1968"} {
    list [catch {
	set d1 [blt::timestamp scan "Wed, 21 Jun 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.22 {timestamp scan "Thu, 21 Jun 68 00:00:00 GMT"} {
    list [catch {
	set d1 [blt::timestamp scan "Thu, 21 Jun 68 00:00:00 GMT"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.23 {timestamp scan "19680621"} {
    list [catch {
	set d1 [blt::timestamp scan "19680621"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.24 {timestamp scan "1968-Jun-21"} {
    list [catch {
	set d1 [blt::timestamp scan "1968-Jun-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.25 {timestamp scan "1968-June-21"} {
    list [catch {
	set d1 [blt::timestamp scan "1968-June-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.26 {timestamp scan "1968-06-21"} {
    list [catch {
	set d1 [blt::timestamp scan "1968-06-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.27 {timestamp scan "1968-6-21"} {
    list [catch {
	set d1 [blt::timestamp scan "1968-6-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 1, 1970"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.28 {timestamp scan "1970"} {
    list [catch {
	set d1 [blt::timestamp scan "1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.29 {timestamp scan "January 1970"} {
    list [catch {
	set d1 [blt::timestamp scan "January 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.30 {timestamp scan "Jan 1970"} {
    list [catch {
	set d1 [blt::timestamp scan "Jan 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.31 {timestamp scan "1 Jan 1970"} {
    list [catch {
	set d1 [blt::timestamp scan "1 Jan 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.32 {timestamp scan "1/1"} {
    list [catch {
 	set d1 [blt::timestamp scan "1/1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.33 {timestamp scan "1970 Jan"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1970 Jan"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.34 {timestamp scan "1970 Jan 1"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1970 Jan 1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.35 {timestamp scan "00:00"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.36 {timestamp scan "00:00:00"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# hh:mm:ss.fff
test timestamp.37 {timestamp scan "00:00:00.000"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# A 6-digit number is interpretered as hhmmss.
test timestamp.38 {timestamp scan "000000"} {
    list [catch {
	set d1 [blt::timestamp scan "000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# hh.mm.ss
test timestamp.39 {timestamp scan "00.00.00"} {
    list [catch {
	set d1 [blt::timestamp scan "00.00.00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.40 {timestamp scan "00.00.00.000"} {
    list [catch {
	set d1 [blt::timestamp scan "00.00.00.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.41 {timestamp scan "00.00.00.00000000"} {
    list [catch {
	set d1 [blt::timestamp scan "00.00.00.00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.42 {timestamp scan "00:00:00,00000000"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00,00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.43 {timestamp scan "T00:00:00,00000000"} {
    list [catch {
	set d1 [blt::timestamp scan "T00:00:00,00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.44 {timestamp scan "00:00:00+0000"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00+0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.45 {timestamp scan "00:00:00-0000"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.46 {timestamp scan "00:00:00 GMT"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00 GMT"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.47 {timestamp scan "00:00:00 UTC"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.48 {timestamp scan "00:00:00Z"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00Z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.49 {timestamp scan "00:00:00 Z"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00 Z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.50 {timestamp scan "00:00:00 GMT+0000"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00 GMT+0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.51 {timestamp scan "00:00:00 gmt-0000"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00 gmt-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.52 {timestamp scan "00:00:00 GMT-0000"} {
    list [catch {
	set d1 [blt::timestamp scan "00:00:00 GMT-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# This is pretty useless.  It's the epoch with the timezone GMT.
test timestamp.53 {timestamp scan "GMT"} {
    list [catch {
	set d1 [blt::timestamp scan "GMT"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Allow empty strings to successfully pass (returns 0).
test timestamp.54 {timestamp scan ""} {
    list [catch {
 	set d1 [blt::timestamp scan ""]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Word-ie days like 23rd can be unambiguously parsed as the day of the month.
test timestamp.55 {timestamp scan "Jan 1st"} {
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1st"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year (1-366) plus time.  The default year is the epoch.
test timestamp.56 {timestamp scan "+001:00:00:01"} {
    list [catch {
 	set d1 [blt::timestamp scan "+001:00:00:01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

# "Doctor it hurts when I use 2 digit years." This the 1st day of January,
# not January of year 1.  1-2 digit years and day of the month are
# ambiguous, so we picked the day of year.  I assume that "Feb 20" is more
# likely to mean February 20th instead of Febrary 1920.
test timestamp.57 {timestamp scan "Jan 1"} {
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Jan 1 1970 is a Thurday. Note that ISO weekdays are Monday 1 - Sunday 7.  
test timestamp.58 {timestamp scan "1970-W01-4"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1970-W01-4"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year.
test timestamp.59 {timestamp scan "1970-001"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1970-001"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year (1-366).  The default year is the epoch.
test timestamp.60 {timestamp scan "001"} {
    list [catch {
 	set d1 [blt::timestamp scan "001"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 2, 2006 15:04:05 MST"
set d2 [clock scan $timestamp -gmt yes]

# This timestamp was found in Go.  There's no +- on the timezone offset.
# Without the colon, the offset is confused with 4 digit years.
test timestamp.61 {timestamp scan "2006-01-02T15:04:05Z07:00"} {
    list [catch {
 	set d1 [blt::timestamp scan "2006-01-02T15:04:05Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.62 {timestamp scan "Mon Jan 2 15:04:05 MST 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon Jan 2 15:04:05 MST 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.63 {timestamp scan "Mon. Jan 2 15:04:05 MST 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon. Jan. 2 15:04:05 MST 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.64 {timestamp scan "Mon., Jan. 2 15:04:05 MST 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon., Jan 2 15:04:05 MST 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.65 {timestamp scan "Mon Jan 02 15:04:05 -0700 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon Jan 02 15:04:05 -0700 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.66 {timestamp scan "Mon Jan 02 15:04:05 -07 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon Jan 02 15:04:05 -07 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.67 {timestamp scan "Mon Jan 02 15:04:05 -7 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon Jan 02 15:04:05 -7 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Dec 29, 1969"
set d2 [clock scan $timestamp -gmt yes]

# Jan 1 1970 is a Thurday.  So the first week starts on Monday of the
# previous year.
test timestamp.68 {timestamp scan "W01 1970"} { 
    list [catch {
 	set d1 [blt::timestamp scan "W01 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 2, 2006 15:04:05"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.69 {timestamp scan "Mon Jan 2 15:04:05 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon Jan 2 15:04:05 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 2, 1906 15:04 MST"
set d2 [clock scan $timestamp -gmt yes]

# "Doctor it hurts when I use 2 digit years." The year "06" is 1906, not
# 2006.  The base of the century of a 2 digit year is always assumed to be
# 1900, not 2000.  This because it's more likely that files generated in
# the 20th century will contain 2 digit years, than ones in 21st.
test timestamp.70 {timestamp scan "02 Jan 06 15:04 MST"} {
    list [catch {
 	set d1 [blt::timestamp scan "02 Jan 06 15:04 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


test timestamp.71 {timestamp scan "02 Jan 06 15:04 -0700"} {
    list [catch {
 	set d1 [blt::timestamp scan "02 Jan 06 15:04 -0700"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.72 {timestamp scan "Monday, 02-Jan-06 15:04:05"} {
    list [catch {
 	set d1 [blt::timestamp scan "Monday, 02-Jan-06 15:04:05 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

set timestamp "Jan 2, 2006 15:04 MST"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.73 {timestamp scan "Mon, 02 Jan 2006 15:04:05 MST"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon, 02 Jan 2006 15:04:05 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test timestamp.74 {timestamp scan "Mon, 02 Jan 2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon, 02 Jan 2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test timestamp.75 {timestamp scan "01/02/2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::timestamp scan "01/02/2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test timestamp.76 {timestamp scan "01-02-2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::timestamp scan "01-02-2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test timestamp.77 {timestamp scan "Mon, 02 Jan 2006 15:04:05 -0700"} {
    list [catch {
 	set d1 [blt::timestamp scan "Mon, 02 Jan 2006 15:04:05 -0700"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test timestamp.78 {timestamp scan "2006-01-02T15:04:05Z07:00"} {
    list [catch {
 	set d1 [blt::timestamp scan "2006-01-02T15:04:05Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test timestamp.79 {timestamp scan "2006-01-02T15:04:05.999999999Z07:00"} {
    list [catch {
 	set d1 [blt::timestamp scan "2006-01-02T15:04:05.999999999Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 6.0}

set timestamp "Jan 1, 1970 03:04pm"
set d2 [clock scan $timestamp -gmt yes]

# The default year/month/day is Jan 1st 1970 (the start of the epoch).
# This lets you use times without timestamps that act like a duration
# (probably without the PM).
test timestamp.80 {timestamp scan "3:04PM"} {
    list [catch {
 	set d1 [blt::timestamp scan "3:04PM"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 2, 2006 15:04"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.81 {timestamp scan "Jan 2 15:04:05 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "Jan 2 15:04:05 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

# Don't test with non-zero fractions yet.  
test timestamp.82 {timestamp scan "Jan 2 2006 15:04:05.000"} {
    list [catch {
 	set d1 [blt::timestamp scan "Jan 2 2006 15:04:05.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test timestamp.83 {timestamp scan "2006 Jan 2 15:04:05.000000"} {
    list [catch {
 	set d1 [blt::timestamp scan "2006 Jan 2 15:04:05.000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test timestamp.84 {timestamp scan "Jan 2 2006 15:04:05.000000000"} {
    list [catch {
 	set d1 [blt::timestamp scan "Jan 2 2006 15:04:05.000000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}


set timestamp "Jan 31, 2012"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.85 {timestamp scan "31 Jan 2012"} {
    list [catch {
 	set d1 [blt::timestamp scan "31 Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.86 {timestamp scan "31-Jan-2012"} {
    list [catch {
 	set d1 [blt::timestamp scan "31-Jan-2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.87 {timestamp scan "31. Jan 2012"} {
    list [catch {
 	set d1 [blt::timestamp scan "31. Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.88 {timestamp scan "31/Jan/2012"} {
    list [catch {
 	set d1 [blt::timestamp scan "31/Jan/2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.89 {timestamp scan "31 Jan 2012"} {
    list [catch {
 	set d1 [blt::timestamp scan "31 Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


test timestamp.90 {timestamp scan "31/Jan/2012"} {
    list [catch {
 	set d1 [blt::timestamp scan "31/Jan/2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# A 3 digit number is interpretered as the day of the year. 
# +ddd:hh:mm:ss YYYY
test timestamp.91 {timestamp scan "+031:00:00:00 2012"} {
    list [catch {
 	set d1 [blt::timestamp scan "+031:00:00:00 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 1, 2005 1:29PM"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.92 {timestamp scan "Jan 1 2005 1:29PM"} {
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1 2005 1:29PM"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Nov 23, 1998"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.93 {timestamp scan "11/23/98"} {
    list [catch {
 	set d1 [blt::timestamp scan "11/23/98"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 1, 1972"
set d2 [clock scan $timestamp -gmt yes]

# "Doctor it hurts when I use 2 digit years." Normally NN.NN.NN would be
# interpreted as a time.  This works only because the 72 isn't a valid hour
# specification.
test timestamp.94 {timestamp scan "72.01.01"} {
    list [catch {
 	set d1 [blt::timestamp scan "72.01.01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# It's always year month day.
test timestamp.95 {timestamp scan "1972.01.01"} {
    list [catch {
 	set d1 [blt::timestamp scan "1972.01.01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Feb 19, 1972"
set d2 [clock scan $timestamp -gmt yes]

# Normally this is interpreted as month/day/year.  This works because 19
# isn't a valid month specification.  Use month names or abbreviations.
test timestamp.96 {timestamp scan "19/02/72"} {
    list [catch {
 	set d1 [blt::timestamp scan "19/02/72"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# This is parsed as day/month/year. 
test timestamp.97 {timestamp scan "19/02/1972"} {
    list [catch {
 	set d1 [blt::timestamp scan "19/02/1972"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Dec 25, 1905"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.98 {timestamp scan "25/12/05"} {
    list [catch {
 	set d1 [blt::timestamp scan "25/12/05"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Dec 05, 1925"
set d2 [clock scan $timestamp -gmt yes]

# This is an example of an accepted, but ambiguous format. NN.NN.NN is
# normally interpreted as a time.  But since 25 isn't a valid hour
# specification, it's interpreted as a timestamp.  With dots as separators
# we assume that this is dd.mm.yy".  The year 25 is 1925 not 2025.
test timestamp.99 {timestamp scan "25.12.05"} {
    list [catch {
 	set d1 [blt::timestamp scan "25.12.05"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 24, 1998"
set d2 [clock scan $timestamp -gmt yes]

# Another accepted, but ambiguous format.  24 isn't a valid month
# specification.
test timestamp.100 {timestamp scan "24-01-98"} {
    list [catch {
 	set d1 [blt::timestamp scan "24-01-98"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "April 1, 1902"
set d2 [clock scan $timestamp -gmt yes]

# Is this "day-month-year" or "year-month-day" or "month-day-year"?  It's
# parsed as "dd-month-day-year". And "02" is 1902 not 2002.
test timestamp.101 {timestamp scan "04-01-02"} {
    list [catch {
 	set d1 [blt::timestamp scan "04-01-02"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 24, 1998"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.102 {timestamp scan "24-01-1998"} {
    list [catch {
 	set d1 [blt::timestamp scan "24-01-1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 04, 1906"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.103 {timestamp scan "04 Jul 06"} {
    list [catch {
 	set d1 [blt::timestamp scan "04 Jul 06"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 04, 2006"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.104 {timestamp scan "04 Jul 2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "04 Jul 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 24, 1998"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.105 {timestamp scan "Jan 24, 98"} {
    list [catch {
 	set d1 [blt::timestamp scan "Jan 24, 1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test timestamp.106 {timestamp scan "Jan 24, 1998"} {
    list [catch {
 	set d1 [blt::timestamp scan "Jan 24, 1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 1, 1970 03:24:53"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.107 {timestamp scan "03:24:53"} {
    list [catch {
 	set d1 [blt::timestamp scan "03:24:53"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Apr 28, 2006 12:32:29PM"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.108 {timestamp scan "Apr 28 2006 12:32:29:253PM"} {
    list [catch {
 	set d1 [blt::timestamp scan "Apr 28 2006 12:32:29:253PM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.253}

set timestamp "Jan 1, 1906"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.109 {timestamp scan "01-01-06"} {
    list [catch {
 	set d1 [blt::timestamp scan "01-01-06"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 1, 2006"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.110 {timestamp scan "01-01-2006"} {
    list [catch {
 	set d1 [blt::timestamp scan "01-01-2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Nov 23, 1998"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.111 {timestamp scan "98/11/23"} {
    list [catch {
 	set d1 [blt::timestamp scan "1998/11/23"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# NNNNNN is always a time pattern.  Better to fail on 98 as an hour than to
# parse as a timestamp and silently switch back to time in 2 years
# "001123".  If you really want a number as the timestamp, then use
# "19981123".
test timestamp.112 {timestamp scan "981123"} {
    list [catch {
 	set d1 [blt::timestamp scan "981123"]
	expr { $d1 - $d2 }
    } msg] $msg
} {1 {hour "98" is out of range.}}

test timestamp.113 {timestamp scan "19981123"} {
    list [catch {
 	set d1 [blt::timestamp scan "19981123"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Apr 28, 2006 00:34:55"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.114 {timestamp scan "28 Apr 2006 00:34:55:190"} {
    list [catch {
 	set d1 [blt::timestamp scan "28 Apr 2006 00:34:55:190"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.19}

set timestamp "Jan 1, 1970 11:34:23"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.115 {timestamp scan "11:34:23:013"} {
    list [catch {
 	set d1 [blt::timestamp scan "11:34:23:013"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.013}

set timestamp "Jan 1, 1972 13:42:24"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.116 {timestamp scan "1972-01-01 13:42:24"} {
    list [catch {
 	set d1 [blt::timestamp scan "1972-01-01 13:42:24"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Feb 19, 1972 06:35:24"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.117 {timestamp scan "1972-02-19 06:35:24.489"} {
    list [catch {
 	set d1 [blt::timestamp scan "1972-02-19 06:35:24.489"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.489}

set timestamp "Nov 23, 1998 11:25:43"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.118 {timestamp scan "1998-11-23T11:25:43:250"} {
    list [catch {
 	set d1 [blt::timestamp scan "1998-11-23T11:25:43:250"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.25}

set timestamp "Apr 28, 2006 12:39:32AM"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.119 {timestamp scan "28 Apr 2006 12:39:32:429AM"} {
    list [catch {
 	set d1 [blt::timestamp scan "28 Apr 2006 12:39:32:429AM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.429}

# This is parsed dd/mm/yyyy because 28 is not a valid month.
# NN/NN/NNNN is normally parsed as mm/dd/yyyy.  
# FIXME: probably should fail on an invalid month.
test timestamp.120 {timestamp scan "28/04/2006 12:39:32:429AM"} {
    list [catch {
 	set d1 [blt::timestamp scan "28/04/2006 12:39:32:429AM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.429}

set timestamp "Jan 2, 2003"
set d2 [clock scan $timestamp -gmt yes]

# Slashes assume is format mm/dd/yyyy.
test timestamp.121 {timestamp scan "01/02/2003"} {
    list [catch {
 	set d1 [blt::timestamp scan "01/02/2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Same with dashes mm-dd-yyyy.
test timestamp.122 {timestamp scan "01-02-2003"} {
    list [catch {
 	set d1 [blt::timestamp scan "01-02-2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# But periods are dd.mm.yyyy.
test timestamp.123 {timestamp scan "02.01.2003"} {
    list [catch {
 	set d1 [blt::timestamp scan "02.01.2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 2, 1903"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.124 {timestamp scan "01-02-03"} {
    list [catch {
 	set d1 [blt::timestamp scan "01-02-03"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


set timestamp "Oct 30, 2005 10:45"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.125 {timestamp scan "2005-10-30 T 10:45 UTC"} { 
    list [catch {
 	set d1 [blt::timestamp scan "2005-10-30 T 10:45 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Nov 09, 2007 11:20"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.126 {timestamp scan "2007-11-09 T 11:20 UTC"} { 
    list [catch {
 	set d1 [blt::timestamp scan "2007-11-09 T 11:20 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 23, 2005 02:16:57"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.127 {timestamp scan "Sat Jul 23 02:16:57 2005"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Sat Jul 23 02:16:57 2005"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 21, 1969 02:56"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.128 {timestamp scan "1969-07-21 T 02:56 UTC"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1969-07-21 T 02:56 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Dec 11, 2012 07:38"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.129 {timestamp scan "07:38, 11 December 2012 (UTC)"} { 
    list [catch {
 	set d1 [blt::timestamp scan "07:38, 11 December 2012 (UTC)"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 1, 1997"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.130 {timestamp scan "1997"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1997"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 1, 1997"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.131 {timestamp scan "1997-07"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1997-07"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 16, 1997"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.132 {timestamp scan "1997-07-16"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1997-07-16"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# 5 digit year
test timestamp.133 {timestamp scan "01997-07-16"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1997-07-16"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 16, 1997 19:20a"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.134 {timestamp scan "1997-07-16T19:20+01:00"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1997-07-16T19:20+01:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 16, 1997 19:20:30a"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.135 {timestamp scan "1997-07-16T19:20:30+01:00"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1997-07-16T19:20:30+01:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jul 16, 1997 19:20:30a"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.136 {timestamp scan "1997-07-16T19:20:30.45+01:00"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1997-07-16T19:20:30.45+01:00"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.45}

set timestamp "Apr 12, 1985 23:20:50"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.137 {timestamp scan "1985-04-12T23:20:50.52Z"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1985-04-12T23:20:50.52Z"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.52}
      
set timestamp "Dec 19, 1996 16:39:57u"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.138 {timestamp scan "1996-12-19T16:39:57-08:00"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1996-12-19T16:39:57-08:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Dec 31, 1990 23:59:59"
set d2 [clock scan $timestamp -gmt yes]

# Handle leap second.
test timestamp.139 {timestamp scan "1990-12-31T23:59:60Z"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1990-12-31T23:59:60Z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

set timestamp "Dec 31, 1990 15:59:59u"
set d2 [clock scan $timestamp -gmt yes]

# Handle leap second.
test timestamp.140 {timestamp scan "1990-12-31T15:59:60-08:00"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1990-12-31T15:59:60-08:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

set timestamp "Jan 1, 1937 12:00:27"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.141 {timestamp scan "1937-01-01T12:00:27.87+00:20"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1937-01-01T12:00:27.87+00:20"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 -1199.13};				# 20 minutes from GMT + .87 seconds
      
set timestamp "Dec 17, 1997 07:37:16 PST"
set d2 [clock scan $timestamp -gmt yes]

# ISO 	ISO 8601/SQL standard 	
test timestamp.142 {timestamp scan "1997-12-17 07:37:16-08"} { 
    list [catch {
 	set d1 [blt::timestamp scan "1997-12-17 07:37:16-08"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# SQL 	traditional style 	
test timestamp.143 {timestamp scan "12/17/1997 07:37:16.00 PST"} { 
    list [catch {
 	set d1 [blt::timestamp scan "12/17/1997 07:37:16.00 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# POSTGRES 	original style
test timestamp.144 {timestamp scan "Wed Dec 17 07:37:16 1997 PST"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Wed Dec 17 07:37:16 1997 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# German 	regional style
test timestamp.145 {timestamp scan "17.12.1997 07:37:16.00 PST"} { 
    list [catch {
 	set d1 [blt::timestamp scan "17.12.1997 07:37:16.00 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 1, 2005"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.146 {timestamp scan "2004-W53-6"} { 
    list [catch {
 	set d1 [blt::timestamp scan "2004-W53-6"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 2, 2005"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.147 {timestamp scan "2004-W53-7"} { 
    list [catch {
 	set d1 [blt::timestamp scan "2004-W53-7"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


set timestamp "Dec 31, 2005"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.148 {timestamp scan "2005-W52-6"} { 
    list [catch {
 	set d1 [blt::timestamp scan "2005-W52-6"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Jan 1, 2007"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.149 {timestamp scan "2007-W01-1"} { 
    list [catch {
 	set d1 [blt::timestamp scan "2007-W01-1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set timestamp "Dec 25, 2016"
set d2 [clock scan $timestamp -gmt yes]

test timestamp.150 {timestamp scan "Dec 25, 2016"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Dec 25, 2016"]
	blt::timestamp format $d1 -format "%V %U"
    } msg] $msg
} {0 {51 53}}

test timestamp.151 {timestamp scan "Jan 1, 2014"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 2014"]
	blt::timestamp format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 01}}

test timestamp.152 {timestamp scan "Jan 4, 2014"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 4, 2014"]
	blt::timestamp format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 01}}

test timestamp.153 {timestamp scan "Jan 5, 2014"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 02}}

test timestamp.154 {timestamp scan "Jan 5, 1969"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 02}}

# Default format
test timestamp.155 {timestamp format} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1
    } msg] $msg
} {0 {Sun Jan 05 00:00:00 +0000 1969}}

# Percent
test timestamp.156 {timestamp format %} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%%"
    } msg] $msg
} {0 %}

# Abbreviated weekday
test timestamp.157 {timestamp format %a} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%a"
    } msg] $msg
} {0 Sun}

# Weekday
test timestamp.158 {timestamp format %A} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%A"
    } msg] $msg
} {0 Sunday}

# Abbreviated month.
test timestamp.159 {timestamp format %b} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%b"
    } msg] $msg
} {0 Jan}

# Month
test timestamp.160 {timestamp format %B} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%B"
    } msg] $msg
} {0 January}

# Abbreviated month.
test timestamp.161 {timestamp format %h} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%h"
    } msg] $msg
} {0 Jan}

# Timestamp and time.
test timestamp.162 {timestamp format %c} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%c"
    } msg] $msg
} {0 {Sun Jan 5 00:00:00 1969}}

# Century without last two digits.
test timestamp.163 {timestamp format %C} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%C"
    } msg] $msg
} {0 19}

# Day of month.
test timestamp.164 {timestamp format %d} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%d"
    } msg] $msg
} {0 05}

# Timestamp mm/dd/yy
test timestamp.165 {timestamp format %D} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%D"
    } msg] $msg
} {0 01/05/69}

# Day of month space padded
test timestamp.166 {timestamp format %e} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%e"
    } msg] $msg
} {0 { 5}}

# Full timestamp yyyy-mm-dd
test timestamp.167 {timestamp format %F} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%F"
    } msg] $msg
} {0 1969-01-05}

# Last 2 digits of ISO year
test timestamp.168 {timestamp format %g} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%g"
    } msg] $msg
} {0 69}

# ISO year
test timestamp.169 {timestamp format %G} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%G"
    } msg] $msg
} {0 1969}

# ISO year
test timestamp.170 {timestamp format %G} { 
    list [catch {
 	set d1 [blt::timestamp scan "W01 1970"]
	blt::timestamp format $d1 -format "%G"
    } msg] $msg
} {0 1969}

# Hour
test timestamp.171 {timestamp format %H} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969"]
	blt::timestamp format $d1 -format "%H"
    } msg] $msg
} {0 00}

# Hour
test timestamp.172 {timestamp format %H} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 11:00"]
	blt::timestamp format $d1 -format "%H"
    } msg] $msg
} {0 11}

# Hour
test timestamp.173 {timestamp format %H} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 11:00pm"]
	blt::timestamp format $d1 -format "%H"
    } msg] $msg
} {0 23}

# Hour (01-12)
test timestamp.174 {timestamp format %I} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 00:00"]
	blt::timestamp format $d1 -format "%I"
    } msg] $msg
} {0 12}

# Day of year (1-366)
test timestamp.175 {timestamp format %j} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 00:00"]
	blt::timestamp format $d1 -format "%j"
    } msg] $msg
} {0 005}


# Hour space padded
test timestamp.176 {timestamp format %k} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 00:00"]
	blt::timestamp format $d1 -format "%k"
    } msg] $msg
} {0 { 0}}

# Hour space padded
test timestamp.177 {timestamp format %k} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:00"]
	blt::timestamp format $d1 -format "%k"
    } msg] $msg
} {0 13}

# Hour space padded
test timestamp.178 {timestamp format %l} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:00"]
	blt::timestamp format $d1 -format "%l"
    } msg] $msg
} {0 { 1}}

# Month 01-12
test timestamp.179 {timestamp format %m} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:00"]
	blt::timestamp format $d1 -format "%m"
    } msg] $msg
} {0 01}

# Minute
test timestamp.180 {timestamp format %M} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:00"]
	blt::timestamp format $d1 -format "%M"
    } msg] $msg
} {0 00}

# Minute
test timestamp.181 {timestamp format %M} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:59"]
	blt::timestamp format $d1 -format "%M"
    } msg] $msg
} {0 59}

# Nanoseconds
test timestamp.182 {timestamp format %N} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1970"]
	blt::timestamp format $d1 -format "%N"
    } msg] $msg
} {0 0}

# Nanoseconds in an hour
test timestamp.183 {timestamp format %N} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1970 01:00"]
	blt::timestamp format $d1 -format "%N"
    } msg] $msg
} {0 3600000000000}

# Nanoseconds in a day.
test timestamp.184 {timestamp format %N} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 2, 1970"]
	blt::timestamp format $d1 -format "%N"
    } msg] $msg
} {0 86400000000000}

# Nanoseconds in a year.
test timestamp.185 {timestamp format %N} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1971"]
	blt::timestamp format $d1 -format "%N"
    } msg] $msg
} {0 31536000000000000}

# AM/PM
test timestamp.186 {timestamp format %P} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:59"]
	blt::timestamp format $d1 -format "%P"
    } msg] $msg
} {0 pm}

# AM/PM
test timestamp.187 {timestamp format %p} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:59"]
	blt::timestamp format $d1 -format "%p"
    } msg] $msg
} {0 PM}

# 12 hour clock
test timestamp.188 {timestamp format %r} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:59"]
	blt::timestamp format $d1 -format "%r"
    } msg] $msg
} {0 {01:59:00 PM}}

# 24 hour clock
test timestamp.189 {timestamp format %R} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 1969 13:59"]
	blt::timestamp format $d1 -format "%R"
    } msg] $msg
} {0 13:59}

# Seconds since epoch
test timestamp.190 {timestamp format %s} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1970"]
	blt::timestamp format $d1 -format "%s"
    } msg] $msg
} {0 0}

# Seconds since epoch
test timestamp.191 {timestamp format %s} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 2, 1970"]
	blt::timestamp format $d1 -format "%s"
    } msg] $msg
} {0 86400}

# Second (00-59)
test timestamp.192 {timestamp format %S} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1970"]
	blt::timestamp format $d1 -format "%S"
    } msg] $msg
} {0 00}

# Second (00-59)
test timestamp.193 {timestamp format %S} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1970 00:00:59"]
	blt::timestamp format $d1 -format "%S"
    } msg] $msg
} {0 59}

# Second (00-59) does not contain fraction
test timestamp.194 {timestamp format %S} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1970 00:00:59.9999123"]
	blt::timestamp format $d1 -format "%S"
    } msg] $msg
} {0 59}

# Time hh:mm:ss
test timestamp.195 {timestamp format %T} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1970 01:02:03"]
	blt::timestamp format $d1 -format "%T"
    } msg] $msg
} {0 01:02:03}

# Day of week 1-7 Sunday = 1
test timestamp.196 {timestamp format %u} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 1, 1970"]
	blt::timestamp format $d1 -format "%u"
    } msg] $msg
} {0 5}

# Day of week 1-7 Sunday = 1
test timestamp.197 {timestamp format %u} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%u"
    } msg] $msg
} {0 1}

# Week number (01-53).  Sunday is first day of week.
test timestamp.198 {timestamp format %U} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%U"
    } msg] $msg
} {0 02}

# ISO week number (01-53).  Monday is first day of week.
test timestamp.199 {timestamp format %V} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%V"
    } msg] $msg
} {0 01}

# Week day (0-6). Sunday is 0.
test timestamp.200 {timestamp format %w} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%w"
    } msg] $msg
} {0 0}

# Week (00-53). Monday is the first day of week. (I don't know what this
# is.)
test timestamp.201 {timestamp format %W} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%W"
    } msg] $msg
} {0 01}

# Timestamp representation mm/dd/yy
test timestamp.202 {timestamp format %x} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%x"
    } msg] $msg
} {0 01/05/14}

# Year last 2 digits (yy)
test timestamp.203 {timestamp format %y} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%y"
    } msg] $msg
} {0 14}

# Year 4 digits (yyyy)
test timestamp.204 {timestamp format %Y} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%Y"
    } msg] $msg
} {0 2014}

# Year 5 digits (yyyyy)
test timestamp.205 {timestamp format %Y} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 12014"]
	blt::timestamp format $d1 -format "%Y"
    } msg] $msg
} {0 12014}

# Timezone
test timestamp.206 {timestamp format %z} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014"]
	blt::timestamp format $d1 -format "%z"
    } msg] $msg
} {0 +0000}

# Timezone (always +0000).
test timestamp.207 {timestamp format %z} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014 PST"]
	blt::timestamp format $d1 -format "%z"
    } msg] $msg
} {0 +0000}

# Mixed.
test timestamp.208 {timestamp format "abcd%aefghi"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014 PST"]
	blt::timestamp format $d1 -format "abcd%aefghi"
    } msg] $msg
} {0 abcdSunefghi}

# Non-substitutions.
test timestamp.209 {timestamp format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014 PST"]
	blt::timestamp format $d1 -format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z"
    } msg] $msg
} {0 %E%f%i%J%K%L%n%q%Q%t%v%X%Z}

# Non-substitutions.
test timestamp.210 {timestamp format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z%%s"} { 
    list [catch {
 	set d1 [blt::timestamp scan "Jan 5, 2014 PST"]
	blt::timestamp format $d1 -format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z%%s"
    } msg] $msg
} {0 %E%f%i%J%K%L%n%q%Q%t%v%X%Z%s}


test timestamp.211 {timestamp scan "dd month yyyy"} { 
    list [catch { blt::timestamp scan "01 January 1970" } msg] $msg
} {0 0.0}

test timestamp.212 {timestamp scan "dd mon yyyy"} { 
    list [catch { blt::timestamp scan "01 Jan 1970" } msg] $msg
} {0 0.0}

test timestamp.213 {timestamp scan "dd mm yyyy"} { 
    list [catch { blt::timestamp scan "01 01 1970" } msg] $msg
} {0 0.0}

test timestamp.214 {timestamp scan "dd month"} { 
    list [catch { blt::timestamp scan "01 January" } msg] $msg
} {0 0.0}

test timestamp.215 {timestamp scan "dd mon"} { 
    list [catch { blt::timestamp scan "01 Jan" } msg] $msg
} {0 0.0}

test timestamp.216 {timestamp scan "dd mm"} { 
    list [catch { blt::timestamp scan "01 01" } msg] $msg
} {0 0.0}

test timestamp.217 {timestamp scan "dd-month-yyyy"} { 
    list [catch { blt::timestamp scan "01-January-1970" } msg] $msg
} {0 0.0}

test timestamp.218 {timestamp scan "dd-mon-yyyy"} { 
    list [catch { blt::timestamp scan "01-Jan-1970" } msg] $msg
} {0 0.0}

test timestamp.219 {timestamp scan "dd-mm-yyyy"} { 
    list [catch { blt::timestamp scan "01-01-1970" } msg] $msg
} {0 0.0}

test timestamp.220 {timestamp scan "dd/month/yyyy"} { 
    list [catch { blt::timestamp scan "01/January/1970" } msg] $msg
} {0 0.0}

test timestamp.221 {timestamp scan "dd/mon/yyyy"} { 
    list [catch { blt::timestamp scan "01/Jan/1970" } msg] $msg
} {0 0.0}

test timestamp.222 {timestamp scan "dd/mm/yyyy"} { 
    list [catch { blt::timestamp scan "01/01/1970" } msg] $msg
} {0 0.0}

test timestamp.223 {timestamp scan "month dd"} { 
    list [catch { blt::timestamp scan "January 01" } msg] $msg
} {0 0.0}

test timestamp.224 {timestamp scan "mon dd"} { 
    list [catch { blt::timestamp scan "Jan 01" } msg] $msg
} {0 0.0}

test timestamp.225 {timestamp scan "mm dd"} { 
    list [catch { blt::timestamp scan "01 01" } msg] $msg
} {0 0.0}


test timestamp.226 {timestamp scan "month dd yyyy"} { 
    list [catch { blt::timestamp scan "January 01 1970" } msg] $msg
} {0 0.0}

test timestamp.227 {timestamp scan "mon dd yyyy"} { 
    list [catch { blt::timestamp scan "Jan 01 1970" } msg] $msg
} {0 0.0}

test timestamp.228 {timestamp scan "mm dd yyyy"} { 
    list [catch { blt::timestamp scan "01 01 1970" } msg] $msg
} {0 0.0}

test timestamp.229 {timestamp scan "dd month yyyy"} { 
    list [catch { blt::timestamp scan "02 January 1970" } msg] $msg
} {0 86400.0}

test timestamp.230 {timestamp scan "dd mon yyyy"} { 
    list [catch { blt::timestamp scan "02 Jan 1970" } msg] $msg
} {0 86400.0}

# Can't tell mm from dd. This if February 1st not January 2nd.
test timestamp.231 {timestamp scan "dd mm yyyy"} { 
    list [catch { blt::timestamp scan "02 01 1970" } msg] $msg
} {0 2678400.0}

test timestamp.232 {timestamp scan "dd month"} { 
    list [catch { blt::timestamp scan "02 January" } msg] $msg
} {0 86400.0}

test timestamp.233 {timestamp scan "dd mon"} { 
    list [catch { blt::timestamp scan "02 Jan" } msg] $msg
} {0 86400.0}

# Can't tell mm from dd. This if February 1st not January 2nd.
test timestamp.234 {timestamp scan "dd mm"} { 
    list [catch { blt::timestamp scan "02 01" } msg] $msg
} {0 2678400.0}

test timestamp.235 {timestamp scan "dd-month-yyyy"} { 
    list [catch { blt::timestamp scan "02-January-1970" } msg] $msg
} {0 86400.0}

test timestamp.236 {timestamp scan "dd-mon-yyyy"} { 
    list [catch { blt::timestamp scan "02-Jan-1970" } msg] $msg
} {0 86400.0}

# Can't tell mm from dd. This if February 1st not January 2nd.
test timestamp.237 {timestamp scan "dd-mm-yyyy"} { 
    list [catch { blt::timestamp scan "02-01-1970" } msg] $msg
} {0 2678400.0}

test timestamp.238 {timestamp scan "dd/month/yyyy"} { 
    list [catch { blt::timestamp scan "02/January/1970" } msg] $msg
} {0 86400.0}

test timestamp.239 {timestamp scan "dd/mon/yyyy"} { 
    list [catch { blt::timestamp scan "02/Jan/1970" } msg] $msg
} {0 86400.0}

# Can't tell mm from dd. This if February 1st not January 2nd.
test timestamp.240 {timestamp scan "dd/mm/yyyy"} { 
    list [catch { blt::timestamp scan "02/01/1970" } msg] $msg
} {0 2678400.0}

test timestamp.241 {timestamp scan "month dd"} { 
    list [catch { blt::timestamp scan "January 02" } msg] $msg
} {0 86400.0}

test timestamp.242 {timestamp scan "mon dd"} { 
    list [catch { blt::timestamp scan "Jan 02" } msg] $msg
} {0 86400.0}

test timestamp.243 {timestamp scan "mm dd"} { 
    list [catch { blt::timestamp scan "01 02" } msg] $msg
} {0 86400.0}


test timestamp.244 {timestamp scan "month dd yyyy"} { 
    list [catch { blt::timestamp scan "January 02 1970" } msg] $msg
} {0 86400.0}

test timestamp.245 {timestamp scan "mon dd yyyy"} { 
    list [catch { blt::timestamp scan "Jan 02 1970" } msg] $msg
} {0 86400.0}

test timestamp.246 {timestamp scan "mm dd yyyy"} { 
    list [catch { blt::timestamp scan "01 02 1970" } msg] $msg
} {0 86400.0}

test timestamp.247 {timestamp scan "dd month yyyy"} { 
    list [catch { blt::timestamp scan "31 January 1970" } msg] $msg
} {0 2592000.0}

test timestamp.248 {timestamp scan "dd mon yyyy"} { 
    list [catch { blt::timestamp scan "31 Jan 1970" } msg] $msg
} {0 2592000.0}

test timestamp.249 {timestamp scan "dd mm yyyy"} { 
    list [catch { blt::timestamp scan "31 01 1970" } msg] $msg
} {0 2592000.0}

test timestamp.250 {timestamp scan "dd month"} { 
    list [catch { blt::timestamp scan "31 January" } msg] $msg
} {0 2592000.0}

test timestamp.251 {timestamp scan "dd mon"} { 
    list [catch { blt::timestamp scan "31 Jan" } msg] $msg
} {0 2592000.0}

test timestamp.252 {timestamp scan "dd mm"} { 
    list [catch { blt::timestamp scan "31 01" } msg] $msg
} {0 2592000.0}

test timestamp.253 {timestamp scan "dd-month-yyyy"} { 
    list [catch { blt::timestamp scan "31-January-1970" } msg] $msg
} {0 2592000.0}

test timestamp.254 {timestamp scan "dd-mon-yyyy"} { 
    list [catch { blt::timestamp scan "31-Jan-1970" } msg] $msg
} {0 2592000.0}

test timestamp.255 {timestamp scan "dd-mm-yyyy"} { 
    list [catch { blt::timestamp scan "31-01-1970" } msg] $msg
} {0 2592000.0}

test timestamp.256 {timestamp scan "dd/month/yyyy"} { 
    list [catch { blt::timestamp scan "31/January/1970" } msg] $msg
} {0 2592000.0}

test timestamp.257 {timestamp scan "dd/mon/yyyy"} { 
    list [catch { blt::timestamp scan "31/Jan/1970" } msg] $msg
} {0 2592000.0}

test timestamp.258 {timestamp scan "dd/mm/yyyy"} { 
    list [catch { blt::timestamp scan "31/01/1970" } msg] $msg
} {0 2592000.0}

test timestamp.259 {timestamp scan "month dd"} { 
    list [catch { blt::timestamp scan "January 31" } msg] $msg
} {0 2592000.0}

test timestamp.260 {timestamp scan "mon dd"} { 
    list [catch { blt::timestamp scan "Jan 31" } msg] $msg
} {0 2592000.0}

test timestamp.261 {timestamp scan "mm dd"} { 
    list [catch { blt::timestamp scan "01 31" } msg] $msg
} {0 2592000.0}

test timestamp.262 {timestamp scan "month dd yyyy"} { 
    list [catch { blt::timestamp scan "January 31 1970" } msg] $msg
} {0 2592000.0}

test timestamp.263 {timestamp scan "mon dd yyyy"} { 
    list [catch { blt::timestamp scan "Jan 31 1970" } msg] $msg
} {0 2592000.0}

test timestamp.264 {timestamp scan "mm dd yyyy"} { 
    list [catch { blt::timestamp scan "01 31 1970" } msg] $msg
} {0 2592000.0}

test timestamp.265 {timestamp scan "month dd, yyyy"} { 
    list [catch { blt::timestamp scan "January 31, 1970" } msg] $msg
} {0 2592000.0}

test timestamp.266 {timestamp scan "mon dd, yyyy"} { 
    list [catch { blt::timestamp scan "Jan 31, 1970" } msg] $msg
} {0 2592000.0}

test timestamp.267 {timestamp scan "mm dd yyyy"} { 
    list [catch { blt::timestamp scan "01 31, 1970" } msg] $msg
} {0 2592000.0}

test timestamp.268 {timestamp scan "month yyyy"} { 
    list [catch { blt::timestamp scan "January 1970" } msg] $msg
} {0 0.0}

test timestamp.269 {timestamp scan "mon yyyy"} { 
    list [catch { blt::timestamp scan "Jan 1970" } msg] $msg
} {0 0.0}

test timestamp.270 {timestamp scan "mm yyyy"} { 
    list [catch { blt::timestamp scan "01 1970" } msg] $msg
} {0 0.0}

test timestamp.271 {timestamp scan "month, yyyy"} { 
    list [catch { blt::timestamp scan "January, 1970" } msg] $msg
} {0 0.0}

test timestamp.272 {timestamp scan "mon, yyyy"} { 
    list [catch { blt::timestamp scan "Jan, 1970" } msg] $msg
} {0 0.0}

test timestamp.273 {timestamp scan "mm, yyyy"} { 
    list [catch { blt::timestamp scan "01, 1970" } msg] $msg
} {0 0.0}

test timestamp.274 {timestamp scan "yyyy"} { 
    list [catch { blt::timestamp scan "1970" } msg] $msg
} {0 0.0}

# FIXME: error "week "-1" is out of range."
test timestamp.275 {timestamp scan "yyyy-Www"} { 
    list [catch { blt::timestamp scan "1970-W00" } msg] $msg
} {1 {week "0" is out of range.}}

test timestamp.276 {timestamp scan "yyyy-Www-4"} { 
    list [catch { blt::timestamp scan "1970W014" } msg] $msg
} {0 0.0}

test timestamp.277 {timestamp scan "yyyy-month"} { 
    list [catch { blt::timestamp scan "1970-January" } msg] $msg
} {0 0.0}

test timestamp.278 {timestamp scan "yyyy-mon"} { 
    list [catch { blt::timestamp scan "1970-Jan" } msg] $msg
} {0 0.0}

test timestamp.279 {timestamp scan "yyyy-mm"} { 
    list [catch { blt::timestamp scan "1970-01" } msg] $msg
} {0 0.0}

test timestamp.280 {timestamp scan "yyyy-month-dd"} { 
    list [catch { blt::timestamp scan "1970-January-01" } msg] $msg
} {0 0.0}

test timestamp.281 {timestamp scan "yyyy-mon-dd"} { 
    list [catch { blt::timestamp scan "1970-Jan-01" } msg] $msg
} {0 0.0}

test timestamp.282 {timestamp scan "yyyy-mm-01"} { 
    list [catch { blt::timestamp scan "1970-01-01" } msg] $msg
} {0 0.0}

test timestamp.283 {timestamp scan "yyyy/month/dd"} { 
    list [catch { blt::timestamp scan "1970/January/01" } msg] $msg
} {0 0.0}

test timestamp.284 {timestamp scan "yyyy/mon/dd"} { 
    list [catch { blt::timestamp scan "1970/Jan/01" } msg] $msg
} {0 0.0}

test timestamp.285 {timestamp scan "yyyy/mm/01"} { 
    list [catch { blt::timestamp scan "1970/01/01" } msg] $msg
} {0 0.0}

test timestamp.286 {timestamp scan "yyyyWww"} { 
    list [catch { blt::timestamp scan "1970W01" } msg] $msg
} {0 -259200.0}

test timestamp.287 {timestamp scan "yyyyWwwd"} { 
    list [catch { blt::timestamp scan "1970W014" } msg] $msg
} {0 0.0}

test timestamp.288 {timestamp scan "yyyymmdd"} { 
    list [catch { blt::timestamp scan "19700101" } msg] $msg
} {0 0.0}

test timestamp.289 {timestamp scan "yyyymmdd"} { 
    list [catch { blt::timestamp scan "19700102" } msg] $msg
} {0 86400.0}


# This is yyyyddd  
test timestamp.290 {timestamp scan "yyyyddd"} { 
    list [catch { blt::timestamp scan "1970011" } msg] $msg
} {0 864000.0}


test timestamp.291 {timestamp scan "January 1st, 1970 America/New_York"} { 
    list [catch { blt::timestamp scan "January 1st, 1970 America/New_York" } msg] $msg
} {0 18000.0}

test timestamp.292 {timestamp scan "January 1st, 1970 EST"} { 
    list [catch { blt::timestamp scan "January 1st, 1970 EST" } msg] $msg
} {0 18000.0}

test timestamp.293 {timestamp scan "January 1st, 1970 EDT"} { 
    list [catch { blt::timestamp scan "January 1st, 1970 EDT" } msg] $msg
} {0 14400.0}

# This makes no sense but...
test timestamp.294 {timestamp scan "January 1st, 1970 EST DST"} { 
    list [catch { blt::timestamp scan "January 1st, 1970 EST DST" } msg] $msg
} {0 14400.0}

set d2 [clock scan "November 12, 2007 12:00:00 PST" -gmt yes]
test timestamp.295 {timestamp scan "November 12, 2007 12:00:00 PST"} { 
    list [catch {
	set d1 [blt::timestamp scan "November 12, 2007 12:00:00 PST"]
	expr { $d1 - double($d2) }
    } msg] $msg
} {0 0.0}

set d2 [clock scan "January 01, 2015 EDT" -gmt yes]
test timestamp.296 {timestamp scan "Jan 1st 2015 EDT"} { 
    list [catch {
	set d1 [blt::timestamp scan "Jan 1st 2015 EDT"]
	expr { $d1 - double($d2) }
    } msg] $msg
} {0 0.0}

test timestamp.297 {timestamp scan "Jan 1st 2015 America/New_York"} { 
    list [catch {
	set d1 [blt::timestamp scan "Jan 1st 2015 America/New_York"]
	expr { $d1 - double($d2) }
    } ] $msg
} {0 0.0}

test timestamp.298 {timestamp scan "Jan 1st 2015 America/New_York DST"} { 
    list [catch {
	set d1 [blt::timestamp scan "Jan 1st 2015 America/New_York DST"]
	expr { $d1 - double($d2) }
    } msg] $msg
} {0 0.0}


test timestamp.299 {timestamp scan "Jan 1st 2015 EST5EDT"} { 
    list [catch {
	set d1 [blt::timestamp scan "Jan 1st 2015 EST5EDT"]
	expr { $d1 - double($d2) }
    } msg] $msg
} {0 0.0}

# We don't check the validity of timezones (yet). Presumably we can
# use the tz_database America/Los_Angeles rules to test PDT.
set d2 [clock scan "November 02, 2007 12:00 PDT" -gmt yes]
test timestamp.300 {timestamp scan "2007-11-02 12:00 America/Los_Angeles PDT"} {
    list [catch {
	set d1 [blt::timestamp scan "2007-11-02 12:00 America/Los_Angeles PDT"]
	expr { $d1 - double($d2) }
    } msg] $msg
} {0 0.0}


set count 301
foreach name [array names blt::timezones] {
    set timestamp "2007-11-02 12:00 $name"
    test timestamp.$count "timestamp scan $timestamp" {
	list [catch {
	    blt::timestamp scan $timestamp
	    set msg 0.0
	} msg] $msg
    } {0 0.0}
    incr count
}
    
