
package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

set VERBOSE 0

test utils.1 {string no arg} {
    list [catch {blt::utils::string} msg] $msg
} {1 {wrong # args: should be one of...
  blt::utils::string begins str pattern ?switches?
  blt::utils::string between str1 first last ?switches?
  blt::utils::string contains str pattern ?switches?
  blt::utils::string ends str1 str2 ?switches?
  blt::utils::string equals str1 str2 ?switches?
  blt::utils::string inlist str1 list ?switches?}}

test utils.2 {number no arg} {
    list [catch {blt::utils::number} msg] $msg
} {1 {wrong # args: should be one of...
  blt::utils::number between value first last ?switches?
  blt::utils::number eq value1 value2 ?switches?
  blt::utils::number ge value1 value2 ?switches?
  blt::utils::number gt value1 value2 ?switches?
  blt::utils::number le value1 value2 ?switches?
  blt::utils::number lt value1 value2 ?switches?
  blt::utils::number inlist value list ?switches?}}

test utils.3 {number badArg} {
    list [catch {blt::utils::number badArg} msg] $msg
} {1 {bad operation "badArg": should be one of...
  blt::utils::number between value first last ?switches?
  blt::utils::number eq value1 value2 ?switches?
  blt::utils::number ge value1 value2 ?switches?
  blt::utils::number gt value1 value2 ?switches?
  blt::utils::number le value1 value2 ?switches?
  blt::utils::number lt value1 value2 ?switches?
  blt::utils::number inlist value list ?switches?}}

test utils.4 {number between } {
    list [catch {blt::utils::number between} msg] $msg
} {1 {wrong # args: should be "blt::utils::number between value first last ?switches?"}}

test utils.5 {number between 1} {
    list [catch {blt::utils::number between 1} msg] $msg
} {1 {wrong # args: should be "blt::utils::number between value first last ?switches?"}}

test utils.6 {number between 1 2} {
    list [catch {blt::utils::number between 1 2} msg] $msg
} {1 {wrong # args: should be "blt::utils::number between value first last ?switches?"}}

test utils.7 {number between 1 2 3} {
    list [catch {blt::utils::number between 1 2 3} msg] $msg
} {0 0}

test utils.8 {number between 1 0 3} {
    list [catch {blt::utils::number between 1 0 3} msg] $msg
} {0 1}

test utils.9 {number between badArg 0 3} {
    list [catch {blt::utils::number between badArg 0 3} msg] $msg
} {1 {expected floating-point number but got "badArg"}}

test utils.10 {number between 1 badArg 3} {
    list [catch {blt::utils::number between 1 badArg 3} msg] $msg
} {1 {expected floating-point number but got "badArg"}}

test utils.11 {number between 1 0 badArg} {
    list [catch {blt::utils::number between 1 0 badArg} msg] $msg
} {1 {expected floating-point number but got "badArg"}}

test utils.12 {number between 1 0 3 -badSwitch} {
    list [catch {blt::utils::number between 1 0 3 -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -exact }}

test utils.13 {number between 1 0 3 -exact -badSwitch} {
    list [catch {blt::utils::number between 1 0 3 -exact -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -exact }}

test utils.14 {number between 1 3 0} {
    list [catch {blt::utils::number between 1 3 0} msg] $msg
} {0 1}

test utils.15 {number between 1.00000001 1 2} {
    list [catch {blt::utils::number between 1.000000001 1 2} msg] $msg
} {0 1}

test utils.16 {number between 1.00000001 1 0} {
    list [catch {blt::utils::number between 1.000000001 1 0} msg] $msg
} {0 0}

test utils.17 {number between 1.000000000001 1 2 -exact} {
    list [catch {blt::utils::number between 1.000000000001 1 2 -exact} msg] $msg
} {0 1}

test utils.18 {number eq 1.000000000000000001 -exact} {
    list [catch {blt::utils::number eq 1.00000000000000001 1 -exact} msg] $msg
} {0 1}

test utils.19 {number eq 1.00000000000000000001 1 } {
    list [catch {blt::utils::number eq 1.00000000000000000001 1} msg] $msg
} {0 1}

test utils.20 {number eq} {
    list [catch {blt::utils::number eq} msg] $msg
} {1 {wrong # args: should be "blt::utils::number eq value1 value2 ?switches?"}}

test utils.21 {number eq badArg} {
    list [catch {blt::utils::number eq badArg} msg] $msg
} {1 {wrong # args: should be "blt::utils::number eq value1 value2 ?switches?"}}

test utils.22 {number eq badArg badArg} {
    list [catch {blt::utils::number eq badArg badArg} msg] $msg
} {1 {expected floating-point number but got "badArg"}}

test utils.23 {number eq 1 1 -badSwitch} {
    list [catch {blt::utils::number eq 1 1 -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -exact }}

test utils.30 {number inlist} {
    list [catch {blt::utils::number inlist} msg] $msg
} {1 {wrong # args: should be "blt::utils::number inlist value list ?switches?"}}

test utils.31 {number inlist arg} {
    list [catch {blt::utils::number inlist arg } msg] $msg
} {1 {wrong # args: should be "blt::utils::number inlist value list ?switches?"}}

test utils.32 {number inlist 1.0 ""} {
    list [catch {blt::utils::number inlist 1.0 "" } msg] $msg
} {0 0}

test utils.33 {number inlist 1.0 "1 2 3"} {
    list [catch {blt::utils::number inlist 1.0 "1 2 3" } msg] $msg
} {0 1}

test utils.34 {number inlist 1.0 "1e0 2 3"} {
    list [catch {blt::utils::number inlist 1.0 "1e0 2 3" } msg] $msg
} {0 1}

test utils.35 {number inlist 1.0 "3 2 1"} {
    list [catch {blt::utils::number inlist 1.0 "3 2 1" } msg] $msg
} {0 1}

test utils.36 {number inlist 1.0 "3 1 2 1"} {
    list [catch {blt::utils::number inlist 1.0 "3 1 2 1" } msg] $msg
} {0 1}

test utils.37 {number inlist 100 "3 1 2 1e2"} {
    list [catch {blt::utils::number inlist 100 "3 2 1e2" } msg] $msg
} {0 1}


test utils.38 {scan "June 21, 1968"} {
    list [catch {
	set d1 [blt::utils::scan "June 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.39 {scan "June 21 1968"} {
    list [catch {
	set d1 [blt::utils::scan "June 21 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.40 {scan "21 June 1968"} {
    list [catch {
	set d1 [blt::utils::scan "21 June 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.41 {scan "6 21 1968"} {
    list [catch {
	set d1 [blt::utils::scan "6 21 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.42 {scan "6-21-1968"} {
    list [catch {
	set d1 [blt::utils::scan "6-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.43 {scan "06-21-1968"} {
    list [catch {
	set d1 [blt::utils::scan "06-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.44 {scan Jun-21-1968} {
    list [catch {
	set d1 [blt::utils::scan "Jun-21-1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.45 {scan 06/21/1968} {
    list [catch {
	set d1 [blt::utils::scan "06/21/1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.46 {scan "6/21/1968"} {
    list [catch {
	set d1 [blt::utils::scan "6/21/1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.47 {scan "1968-June-21"} {
    list [catch {
	set d1 [blt::utils::scan "1968-June-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.48 {scan "Tuesday Jun 21, 1968"} {
    list [catch {
	set d1 [blt::utils::scan "Tuesday Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.49 {scan "Wednesday Jun 21, 1968"} {
    list [catch {
	set d1 [blt::utils::scan "Wednesday Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.50 {scan "Wed Jun 21, 1968"} {
    list [catch {
	set d1 [blt::utils::scan "Wed Jun 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.51 {scan "Wed. Jun. 21, 1968"} {
    list [catch {
	set d1 [blt::utils::scan "Wed. Jun. 21, 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.52 {scan "Wed, 21 Jun 1968"} {
    list [catch {
	set d1 [blt::utils::scan "Wed, 21 Jun 1968"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.53 {scan "Thu, 21 Jun 68 00:00:00 GMT"} {
    list [catch {
	set d1 [blt::utils::scan "Thu, 21 Jun 68 00:00:00 GMT"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.54 {scan "19680621"} {
    list [catch {
	set d1 [blt::utils::scan "19680621"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.55 {scan "1968-Jun-21"} {
    list [catch {
	set d1 [blt::utils::scan "1968-Jun-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.56 {scan "1968-June-21"} {
    list [catch {
	set d1 [blt::utils::scan "1968-June-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.57 {scan "1968-06-21"} {
    list [catch {
	set d1 [blt::utils::scan "1968-06-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.58 {scan "1968-6-21"} {
    list [catch {
	set d1 [blt::utils::scan "1968-6-21"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 1, 1970"
set d2 [clock scan $-gmt yes]

test utils.59 {scan "1970"} {
    list [catch {
	set d1 [blt::utils::scan "1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.60 {scan "January 1970"} {
    list [catch {
	set d1 [blt::utils::scan "January 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.61 {scan "Jan 1970"} {
    list [catch {
	set d1 [blt::utils::scan "Jan 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.62 {scan "1 Jan 1970"} {
    list [catch {
	set d1 [blt::utils::scan "1 Jan 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.63 {scan "1/1"} {
    list [catch {
 	set d1 [blt::utils::scan "1/1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.64 {scan "1970 Jan"} { 
    list [catch {
 	set d1 [blt::utils::scan "1970 Jan"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.65 {scan "1970 Jan 1"} { 
    list [catch {
 	set d1 [blt::utils::scan "1970 Jan 1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.66 {scan "00:00"} {
    list [catch {
	set d1 [blt::utils::scan "00:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.67 {scan "00:00:00"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# hh:mm:ss.fff
test utils.68 {scan "00:00:00.000"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# A 6-digit number is interpretered as hhmmss.
test utils.69 {scan "000000"} {
    list [catch {
	set d1 [blt::utils::scan "000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# hh.mm.ss
test utils.70 {scan "00.00.00"} {
    list [catch {
	set d1 [blt::utils::scan "00.00.00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.71 {scan "00.00.00.000"} {
    list [catch {
	set d1 [blt::utils::scan "00.00.00.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.72 {scan "00.00.00.00000000"} {
    list [catch {
	set d1 [blt::utils::scan "00.00.00.00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.73 {scan "00:00:00,00000000"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00,00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.74 {scan "t00:00:00,00000000"} {
    list [catch {
	set d1 [blt::utils::scan "t00:00:00,00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.75 {scan "00:00:00+0000"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00+0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.76 {scan "00:00:00-0000"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.77 {scan "00:00:00 gmt"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00 gmt"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.78 {scan "00:00:00 utc"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00 utc"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.79 {scan "00:00:00z"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.80 {scan "00:00:00 z"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00 z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.81 {scan "00:00:00 gmt+0000"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00 gmt+0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.82 {scan "00:00:00 gmt-0000"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00 gmt-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.83 {scan "00:00:00 GMT-0000"} {
    list [catch {
	set d1 [blt::utils::scan "00:00:00 GMT-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# This is pretty useless.  It's the epoch with the timezone GMT.
test utils.84 {scan "GMT"} {
    list [catch {
	set d1 [blt::utils::scan "GMT"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Allow empty strings to successfully pass (returns 0).
test utils.85 {scan ""} {
    list [catch {
 	set d1 [blt::utils::scan ""]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Word-ie days like 23rd can be unambiguously parsed as the day of the month.
test utils.86 {scan "Jan 1st"} {
    list [catch {
 	set d1 [blt::utils::scan "Jan 1st"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year (1-366) plus time.  The default year is the epoch.
test utils.87 {scan "+001:00:00:01"} {
    list [catch {
 	set d1 [blt::utils::scan "+001:00:00:01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

# "Doctor it hurts when I use 2 digit years." This the 1st day of January,
# not January of year 1.  1-2 digit years and day of the month are
# ambiguous, so we picked the day of year.  I assume that "Feb 20" is more
# likely to mean February 20th instead of Febrary 1920.
test utils.88 {scan "Jan 1"} {
    list [catch {
 	set d1 [blt::utils::scan "Jan 1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Jan 1 1970 is a Thurday. Note that ISO weekdays are Monday 1 - Sunday 7.  
test utils.89 {scan "1970-W01-4"} { 
    list [catch {
 	set d1 [blt::utils::scan "1970-W01-4"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year.
test utils.90 {scan "1970-001"} { 
    list [catch {
 	set d1 [blt::utils::scan "1970-001"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Day of year (1-366).  The default year is the epoch.
test utils.91 {scan "001"} {
    list [catch {
 	set d1 [blt::utils::scan "001"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 2, 2006 15:04:05 MST"
set d2 [clock scan $-gmt yes]

# This was found in Go.  There's no +- on the timezone offset.
# Without the colon, the offset is confused with 4 digit years.
test utils.92 {scan "2006-01-02T15:04:05Z07:00"} {
    list [catch {
 	set d1 [blt::utils::scan "2006-01-02T15:04:05Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.93 {scan "Mon Jan 2 15:04:05 MST 2006"} {
    list [catch {
 	set d1 [blt::utils::scan "Mon Jan 2 15:04:05 MST 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.94 {scan "Mon Jan 02 15:04:05 -0700 2006"} {
    list [catch {
 	set d1 [blt::utils::scan "Mon Jan 02 15:04:05 -0700 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.95 {scan "Mon Jan 02 15:04:05 -07 2006"} {
    list [catch {
 	set d1 [blt::utils::scan "Mon Jan 02 15:04:05 -07 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.96 {scan "Mon Jan 02 15:04:05 -7 2006"} {
    list [catch {
 	set d1 [blt::utils::scan "Mon Jan 02 15:04:05 -7 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Dec 29, 1969"
set d2 [clock scan $-gmt yes]

# Jan 1 1970 is a Thurday.  So the first week starts on Monday of the
# previous year.
test utils.97 {scan "W01 1970"} { 
    list [catch {
 	set d1 [blt::utils::scan "W01 1970"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 2, 2006 15:04:05"
set d2 [clock scan $-gmt yes]

test utils.98 {scan "Mon Jan 2 15:04:05 2006"} {
    list [catch {
 	set d1 [blt::utils::scan "Mon Jan 2 15:04:05 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 2, 1906 15:04 MST"
set d2 [clock scan $-gmt yes]

# "Doctor it hurts when I use 2 digit years." The year "06" is 1906, not
# 2006.  The base of the century of a 2 digit year is always assumed to be
# 1900, not 2000.  This because it's more likely that files generated in
# the 20th century will contain 2 digit years, than ones in 21st.
test utils.99 {scan "02 Jan 06 15:04 MST"} {
    list [catch {
 	set d1 [blt::utils::scan "02 Jan 06 15:04 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


test utils.100 {scan "02 Jan 06 15:04 -0700"} {
    list [catch {
 	set d1 [blt::utils::scan "02 Jan 06 15:04 -0700"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.101 {scan "Monday, 02-Jan-06 15:04:05"} {
    list [catch {
 	set d1 [blt::utils::scan "Monday, 02-Jan-06 15:04:05 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

set "Jan 2, 2006 15:04 MST"
set d2 [clock scan $-gmt yes]

test utils.102 {scan "Mon, 02 Jan 2006 15:04:05 MST"} {
    list [catch {
 	set d1 [blt::utils::scan "Mon, 02 Jan 2006 15:04:05 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test utils.103 {scan "Mon, 02 Jan 2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::utils::scan "Mon, 02 Jan 2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test utils.104 {scan "01/02/2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::utils::scan "01/02/2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test utils.105 {scan "01-02-2006 3:04:05pm MST"} {
    list [catch {
 	set d1 [blt::utils::scan "01-02-2006 3:04:05pm MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test utils.106 {scan "Mon, 02 Jan 2006 15:04:05 -0700"} {
    list [catch {
 	set d1 [blt::utils::scan "Mon, 02 Jan 2006 15:04:05 -0700"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test utils.107 {scan "2006-01-02T15:04:05Z07:00"} {
    list [catch {
 	set d1 [blt::utils::scan "2006-01-02T15:04:05Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test utils.108 {scan "2006-01-02T15:04:05.999999999Z07:00"} {
    list [catch {
 	set d1 [blt::utils::scan "2006-01-02T15:04:05.999999999Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 6.0}

set "Jan 1, 1970 03:04pm"
set d2 [clock scan $-gmt yes]

# The default year/month/day is Jan 1st 1970 (the start of the epoch).
# This lets you use times without compares that act like a duration (probably
# without the PM).
test utils.109 {scan "3:04PM"} {
    list [catch {
 	set d1 [blt::utils::scan "3:04PM"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 2, 2006 15:04"
set d2 [clock scan $-gmt yes]

test utils.110 {scan "Jan 2 15:04:05 2006"} {
    list [catch {
 	set d1 [blt::utils::scan "Jan 2 15:04:05 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

# Don't test with non-zero fractions yet.  
test utils.111 {scan "Jan 2 2006 15:04:05.000"} {
    list [catch {
 	set d1 [blt::utils::scan "Jan 2 2006 15:04:05.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test utils.112 {scan "2006 Jan 2 15:04:05.000000"} {
    list [catch {
 	set d1 [blt::utils::scan "2006 Jan 2 15:04:05.000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}

test utils.113 {scan "Jan 2 2006 15:04:05.000000000"} {
    list [catch {
 	set d1 [blt::utils::scan "Jan 2 2006 15:04:05.000000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 5.0}


set "Jan 31, 2012"
set d2 [clock scan $-gmt yes]

test utils.114 {scan "31 Jan 2012"} {
    list [catch {
 	set d1 [blt::utils::scan "31 Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.115 {scan "31-Jan-2012"} {
    list [catch {
 	set d1 [blt::utils::scan "31-Jan-2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.116 {scan "31. Jan 2012"} {
    list [catch {
 	set d1 [blt::utils::scan "31. Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.117 {scan "31/Jan/2012"} {
    list [catch {
 	set d1 [blt::utils::scan "31/Jan/2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.118 {scan "31 Jan 2012"} {
    list [catch {
 	set d1 [blt::utils::scan "31 Jan 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


test utils.119 {scan "31/Jan/2012"} {
    list [catch {
 	set d1 [blt::utils::scan "31/Jan/2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# A 3 digit number is interpretered as the day of the year. 
# +ddd:hh:mm:ss YYYY
test utils.120 {scan "+031:00:00:00 2012"} {
    list [catch {
 	set d1 [blt::utils::scan "+031:00:00:00 2012"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 1, 2005 1:29PM"
set d2 [clock scan $-gmt yes]

test utils.121 {scan "Jan 1 2005 1:29PM"} {
    list [catch {
 	set d1 [blt::utils::scan "Jan 1 2005 1:29PM"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Nov 23, 1998"
set d2 [clock scan $-gmt yes]

test utils.122 {scan "11/23/98"} {
    list [catch {
 	set d1 [blt::utils::scan "11/23/98"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 1, 1972"
set d2 [clock scan $-gmt yes]

# "Doctor it hurts when I use 2 digit years." Normally NN.NN.NN would
# be interpreted as a time.  This works only because the 72 isn't a valid
# hour specification.   
test utils.123 {scan "72.01.01"} {
    list [catch {
 	set d1 [blt::utils::scan "72.01.01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# It's always year month day.
test utils.124 {scan "1972.01.01"} {
    list [catch {
 	set d1 [blt::utils::scan "1972.01.01"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Feb 19, 1972"
set d2 [clock scan $-gmt yes]

# Normally this is interpreted as month/day/year.  This works because 19
# isn't a valid month specification.  Use month names or abbreviations.
test utils.125 {scan "19/02/72"} {
    list [catch {
 	set d1 [blt::utils::scan "19/02/72"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# This is parsed as day/month/year. 
test utils.126 {scan "19/02/1972"} {
    list [catch {
 	set d1 [blt::utils::scan "19/02/1972"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Dec 25, 1905"
set d2 [clock scan $-gmt yes]

test utils.127 {scan "25/12/05"} {
    list [catch {
 	set d1 [blt::utils::scan "25/12/05"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Dec 05, 1925"
set d2 [clock scan $-gmt yes]

# This is an example of an accepted, but ambiguous format. NN.NN.NN is 
# normally interpreted as a time.  But since 25 isn't a valid hour 
# specification, it's interpreted as a utils.  With dots as separators
# we assume that this is dd.mm.yy".  The year 25 is 1925 not 2025.
test utils.128 {scan "25.12.05"} {
    list [catch {
 	set d1 [blt::utils::scan "25.12.05"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 24, 1998"
set d2 [clock scan $-gmt yes]

# Another accepted, but ambiguous format.  24 isn't a valid month
# specification.
test utils.129 {scan "24-01-98"} {
    list [catch {
 	set d1 [blt::utils::scan "24-01-98"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "April 1, 1902"
set d2 [clock scan $-gmt yes]

# Is this "day-month-year" or "year-month-day" or "month-day-year"?  It's
# parsed as "dd-month-day-year". And "02" is 1902 not 2002.
test utils.130 {scan "04-01-02"} {
    list [catch {
 	set d1 [blt::utils::scan "04-01-02"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 24, 1998"
set d2 [clock scan $-gmt yes]

test utils.131 {scan "24-01-1998"} {
    list [catch {
 	set d1 [blt::utils::scan "24-01-1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 04, 1906"
set d2 [clock scan $-gmt yes]

test utils.132 {scan "04 Jul 06"} {
    list [catch {
 	set d1 [blt::utils::scan "04 Jul 06"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 04, 2006"
set d2 [clock scan $-gmt yes]

test utils.133 {scan "04 Jul 2006"} {
    list [catch {
 	set d1 [blt::utils::scan "04 Jul 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 24, 1998"
set d2 [clock scan $-gmt yes]

test utils.134 {scan "Jan 24, 98"} {
    list [catch {
 	set d1 [blt::utils::scan "Jan 24, 1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test utils.135 {scan "Jan 24, 1998"} {
    list [catch {
 	set d1 [blt::utils::scan "Jan 24, 1998"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 1, 1970 03:24:53"
set d2 [clock scan $-gmt yes]

test utils.136 {scan "03:24:53"} {
    list [catch {
 	set d1 [blt::utils::scan "03:24:53"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Apr 28, 2006 12:32:29PM"
set d2 [clock scan $-gmt yes]

test utils.137 {scan "Apr 28 2006 12:32:29:253PM"} {
    list [catch {
 	set d1 [blt::utils::scan "Apr 28 2006 12:32:29:253PM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.253}

set "Jan 1, 1906"
set d2 [clock scan $-gmt yes]

test utils.138 {scan "01-01-06"} {
    list [catch {
 	set d1 [blt::utils::scan "01-01-06"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 1, 2006"
set d2 [clock scan $-gmt yes]

test utils.139 {scan "01-01-2006"} {
    list [catch {
 	set d1 [blt::utils::scan "01-01-2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Nov 23, 1998"
set d2 [clock scan $-gmt yes]

test utils.140 {scan "98/11/23"} {
    list [catch {
 	set d1 [blt::utils::scan "1998/11/23"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# NNNNNN is always a time pattern.  Better to fail on 98 as an hour than to
# parse as a and silently switch back to time in 2 years "001123".  If
# you really want a number as the compare, then use "19981123".
test utils.141 {scan "981123"} {
    list [catch {
 	set d1 [blt::utils::scan "981123"]
	expr { $d1 - $d2 }
    } msg] $msg
} {1 {hour "98" is out of range.}}

test utils.142 {scan "19981123"} {
    list [catch {
 	set d1 [blt::utils::scan "19981123"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Apr 28, 2006 00:34:55"
set d2 [clock scan $-gmt yes]

test utils.143 {scan "28 Apr 2006 00:34:55:190"} {
    list [catch {
 	set d1 [blt::utils::scan "28 Apr 2006 00:34:55:190"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.19}

set "Jan 1, 1970 11:34:23"
set d2 [clock scan $-gmt yes]

test utils.144 {scan "11:34:23:013"} {
    list [catch {
 	set d1 [blt::utils::scan "11:34:23:013"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.013}

set "Jan 1, 1972 13:42:24"
set d2 [clock scan $-gmt yes]

test utils.145 {scan "1972-01-01 13:42:24"} {
    list [catch {
 	set d1 [blt::utils::scan "1972-01-01 13:42:24"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Feb 19, 1972 06:35:24"
set d2 [clock scan $-gmt yes]

test utils.146 {scan "1972-02-19 06:35:24.489"} {
    list [catch {
 	set d1 [blt::utils::scan "1972-02-19 06:35:24.489"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.489}

set "Nov 23, 1998 11:25:43"
set d2 [clock scan $-gmt yes]

test utils.147 {scan "1998-11-23T11:25:43:250"} {
    list [catch {
 	set d1 [blt::utils::scan "1998-11-23T11:25:43:250"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.25}

set "Apr 28, 2006 12:39:32AM"
set d2 [clock scan $-gmt yes]

test utils.148 {scan "28 Apr 2006 12:39:32:429AM"} {
    list [catch {
 	set d1 [blt::utils::scan "28 Apr 2006 12:39:32:429AM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.429}

# This is parsed dd/mm/yyyy because 28 is not a valid month.
# NN/NN/NNNN is normally parsed as mm/dd/yyyy.  
# FIXME: probably should fail on an invalid month.
test utils.149 {scan "28/04/2006 12:39:32:429AM"} {
    list [catch {
 	set d1 [blt::utils::scan "28/04/2006 12:39:32:429AM"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.429}

set "Jan 2, 2003"
set d2 [clock scan $-gmt yes]

# Slashes assume is format mm/dd/yyyy.
test utils.150 {scan "01/02/2003"} {
    list [catch {
 	set d1 [blt::utils::scan "01/02/2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# Same with dashes mm-dd-yyyy.
test utils.151 {scan "01-02-2003"} {
    list [catch {
 	set d1 [blt::utils::scan "01-02-2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# But periods are dd.mm.yyyy.
test utils.152 {scan "02.01.2003"} {
    list [catch {
 	set d1 [blt::utils::scan "02.01.2003"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 2, 1903"
set d2 [clock scan $-gmt yes]

test utils.153 {scan "01-02-03"} {
    list [catch {
 	set d1 [blt::utils::scan "01-02-03"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


set "Oct 30, 2005 10:45"
set d2 [clock scan $-gmt yes]

test utils.154 {scan "2005-10-30 T 10:45 UTC"} { 
    list [catch {
 	set d1 [blt::utils::scan "2005-10-30 T 10:45 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Nov 09, 2007 11:20"
set d2 [clock scan $-gmt yes]

test utils.155 {scan "2007-11-09 T 11:20 UTC"} { 
    list [catch {
 	set d1 [blt::utils::scan "2007-11-09 T 11:20 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 23, 2005 02:16:57"
set d2 [clock scan $-gmt yes]

test utils.156 {scan "Sat Jul 23 02:16:57 2005"} { 
    list [catch {
 	set d1 [blt::utils::scan "Sat Jul 23 02:16:57 2005"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 21, 1969 02:56"
set d2 [clock scan $-gmt yes]

test utils.157 {scan "1969-07-21 T 02:56 UTC"} { 
    list [catch {
 	set d1 [blt::utils::scan "1969-07-21 T 02:56 UTC"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Dec 11, 2012 07:38"
set d2 [clock scan $-gmt yes]

test utils.158 {scan "07:38, 11 December 2012 (UTC)"} { 
    list [catch {
 	set d1 [blt::utils::scan "07:38, 11 December 2012 (UTC)"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 1, 1997"
set d2 [clock scan $-gmt yes]

test utils.159 {scan "1997"} { 
    list [catch {
 	set d1 [blt::utils::scan "1997"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 1, 1997"
set d2 [clock scan $-gmt yes]

test utils.160 {scan "1997-07"} { 
    list [catch {
 	set d1 [blt::utils::scan "1997-07"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 16, 1997"
set d2 [clock scan $-gmt yes]

test utils.161 {scan "1997-07-16"} { 
    list [catch {
 	set d1 [blt::utils::scan "1997-07-16"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# 5 digit year
test utils.162 {scan "01997-07-16"} { 
    list [catch {
 	set d1 [blt::utils::scan "1997-07-16"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 16, 1997 19:20a"
set d2 [clock scan $-gmt yes]

test utils.163 {scan "1997-07-16T19:20+01:00"} { 
    list [catch {
 	set d1 [blt::utils::scan "1997-07-16T19:20+01:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 16, 1997 19:20:30a"
set d2 [clock scan $-gmt yes]

test utils.164 {scan "1997-07-16T19:20:30+01:00"} { 
    list [catch {
 	set d1 [blt::utils::scan "1997-07-16T19:20:30+01:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jul 16, 1997 19:20:30a"
set d2 [clock scan $-gmt yes]

test utils.165 {scan "1997-07-16T19:20:30.45+01:00"} { 
    list [catch {
 	set d1 [blt::utils::scan "1997-07-16T19:20:30.45+01:00"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.45}

set "Apr 12, 1985 23:20:50"
set d2 [clock scan $-gmt yes]

test utils.166 {scan "1985-04-12T23:20:50.52Z"} { 
    list [catch {
 	set d1 [blt::utils::scan "1985-04-12T23:20:50.52Z"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 0.52}
      
set "Dec 19, 1996 16:39:57u"
set d2 [clock scan $-gmt yes]

test utils.167 {scan "1996-12-19T16:39:57-08:00"} { 
    list [catch {
 	set d1 [blt::utils::scan "1996-12-19T16:39:57-08:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Dec 31, 1990 23:59:59"
set d2 [clock scan $-gmt yes]

# Handle leap second.
test utils.168 {scan "1990-12-31T23:59:60Z"} { 
    list [catch {
 	set d1 [blt::utils::scan "1990-12-31T23:59:60Z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

set "Dec 31, 1990 15:59:59u"
set d2 [clock scan $-gmt yes]

# Handle leap second.
test utils.169 {scan "1990-12-31T15:59:60-08:00"} { 
    list [catch {
 	set d1 [blt::utils::scan "1990-12-31T15:59:60-08:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 1.0}

set "Jan 1, 1937 12:00:27"
set d2 [clock scan $-gmt yes]

test utils.170 {scan "1937-01-01T12:00:27.87+00:20"} { 
    list [catch {
 	set d1 [blt::utils::scan "1937-01-01T12:00:27.87+00:20"]
	format %g [expr { $d1 - $d2 }]
    } msg] $msg
} {0 -1199.13};				# 20 minutes from GMT + .87 seconds
      
set "Dec 17, 1997 07:37:16 PST"
set d2 [clock scan $-gmt yes]

# ISO 	ISO 8601/SQL standard 	
test utils.171 {scan "1997-12-17 07:37:16-08"} { 
    list [catch {
 	set d1 [blt::utils::scan "1997-12-17 07:37:16-08"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# SQL 	traditional style 	
test utils.172 {scan "12/17/1997 07:37:16.00 PST"} { 
    list [catch {
 	set d1 [blt::utils::scan "12/17/1997 07:37:16.00 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# POSTGRES 	original style
test utils.173 {scan "Wed Dec 17 07:37:16 1997 PST"} { 
    list [catch {
 	set d1 [blt::utils::scan "Wed Dec 17 07:37:16 1997 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

# German 	regional style
test utils.174 {scan "17.12.1997 07:37:16.00 PST"} { 
    list [catch {
 	set d1 [blt::utils::scan "17.12.1997 07:37:16.00 PST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 1, 2005"
set d2 [clock scan $-gmt yes]

test utils.175 {scan "2004-W53-6"} { 
    list [catch {
 	set d1 [blt::utils::scan "2004-W53-6"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 2, 2005"
set d2 [clock scan $-gmt yes]

test utils.176 {scan "2004-W53-7"} { 
    list [catch {
 	set d1 [blt::utils::scan "2004-W53-7"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


set "Dec 31, 2005"
set d2 [clock scan $-gmt yes]

test utils.177 {scan "2005-W52-6"} { 
    list [catch {
 	set d1 [blt::utils::scan "2005-W52-6"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Jan 1, 2007"
set d2 [clock scan $-gmt yes]

test utils.178 {scan "2007-W01-1"} { 
    list [catch {
 	set d1 [blt::utils::scan "2007-W01-1"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set "Dec 25, 2016"
set d2 [clock scan $-gmt yes]

test utils.179 {scan "Dec 25, 2016"} { 
    list [catch {
 	set d1 [blt::utils::scan "Dec 25, 2016"]
	blt::utils::format $d1 -format "%V %U"
    } msg] $msg
} {0 {51 53}}

test utils.180 {scan "Jan 1, 2014"} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 2014"]
	blt::utils::format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 01}}

test utils.181 {scan "Jan 4, 2014"} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 4, 2014"]
	blt::utils::format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 01}}

test utils.182 {scan "Jan 5, 2014"} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 02}}

test utils.183 {scan "Jan 5, 1969"} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%V %U"
    } msg] $msg
} {0 {01 02}}

# Default format
test utils.184 {format} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1
    } msg] $msg
} {0 {Sun Jan 05 00:00:00 +0000 1969}}

# Percent
test utils.185 {format %} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%%"
    } msg] $msg
} {0 %}

# Abbreviated weekday
test utils.186 {format %a} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%a"
    } msg] $msg
} {0 Sun}

# Weekday
test utils.187 {format %A} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%A"
    } msg] $msg
} {0 Sunday}

# Abbreviated month.
test utils.188 {format %b} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%b"
    } msg] $msg
} {0 Jan}

# Month
test utils.189 {format %B} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%B"
    } msg] $msg
} {0 January}

# Abbreviated month.
test utils.190 {format %h} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%h"
    } msg] $msg
} {0 Jan}

# and time.
test utils.191 {format %c} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%c"
    } msg] $msg
} {0 {Sun Jan 5 00:00:00 1969}}

# Century without last two digits.
test utils.192 {format %C} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%C"
    } msg] $msg
} {0 19}

# Day of month.
test utils.193 {format %d} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%d"
    } msg] $msg
} {0 05}

# mm/dd/yy
test utils.194 {format %D} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%D"
    } msg] $msg
} {0 01/05/69}

# Day of month space padded
test utils.195 {format %e} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%e"
    } msg] $msg
} {0 { 5}}

# Full yyyy-mm-dd
test utils.196 {format %F} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%F"
    } msg] $msg
} {0 1969-01-05}

# Last 2 digits of ISO year
test utils.197 {format %g} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%g"
    } msg] $msg
} {0 69}

# ISO year
test utils.198 {format %G} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%G"
    } msg] $msg
} {0 1969}

# ISO year
test utils.199 {format %G} { 
    list [catch {
 	set d1 [blt::utils::scan "W01 1970"]
	blt::utils::format $d1 -format "%G"
    } msg] $msg
} {0 1969}

# Hour
test utils.200 {format %H} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969"]
	blt::utils::format $d1 -format "%H"
    } msg] $msg
} {0 00}

# Hour
test utils.201 {format %H} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 11:00"]
	blt::utils::format $d1 -format "%H"
    } msg] $msg
} {0 11}

# Hour
test utils.202 {format %H} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 11:00pm"]
	blt::utils::format $d1 -format "%H"
    } msg] $msg
} {0 23}

# Hour (01-12)
test utils.203 {format %I} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 00:00"]
	blt::utils::format $d1 -format "%I"
    } msg] $msg
} {0 12}

# Day of year (1-366)
test utils.204 {format %j} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 00:00"]
	blt::utils::format $d1 -format "%j"
    } msg] $msg
} {0 005}


# Hour space padded
test utils.205 {format %k} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 00:00"]
	blt::utils::format $d1 -format "%k"
    } msg] $msg
} {0 { 0}}

# Hour space padded
test utils.206 {format %k} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:00"]
	blt::utils::format $d1 -format "%k"
    } msg] $msg
} {0 13}

# Hour space padded
test utils.207 {format %l} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:00"]
	blt::utils::format $d1 -format "%l"
    } msg] $msg
} {0 { 1}}

# Month 01-12
test utils.208 {format %m} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:00"]
	blt::utils::format $d1 -format "%m"
    } msg] $msg
} {0 01}

# Minute
test utils.209 {format %M} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:00"]
	blt::utils::format $d1 -format "%M"
    } msg] $msg
} {0 00}

# Minute
test utils.210 {format %M} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:59"]
	blt::utils::format $d1 -format "%M"
    } msg] $msg
} {0 59}

# Nanoseconds
test utils.211 {format %N} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1970"]
	blt::utils::format $d1 -format "%N"
    } msg] $msg
} {0 0}

# Nanoseconds in an hour
test utils.212 {format %N} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1970 01:00"]
	blt::utils::format $d1 -format "%N"
    } msg] $msg
} {0 3600000000000}

# Nanoseconds in a day.
test utils.213 {format %N} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 2, 1970"]
	blt::utils::format $d1 -format "%N"
    } msg] $msg
} {0 86400000000000}

# Nanoseconds in a year.
test utils.214 {format %N} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1971"]
	blt::utils::format $d1 -format "%N"
    } msg] $msg
} {0 31536000000000000}

# AM/PM
test utils.215 {format %P} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:59"]
	blt::utils::format $d1 -format "%P"
    } msg] $msg
} {0 pm}

# AM/PM
test utils.216 {format %p} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:59"]
	blt::utils::format $d1 -format "%p"
    } msg] $msg
} {0 PM}

# 12 hour clock
test utils.217 {format %r} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:59"]
	blt::utils::format $d1 -format "%r"
    } msg] $msg
} {0 {01:59:00 PM}}

# 24 hour clock
test utils.218 {format %R} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 1969 13:59"]
	blt::utils::format $d1 -format "%R"
    } msg] $msg
} {0 13:59}

# Seconds since epoch
test utils.219 {format %s} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1970"]
	blt::utils::format $d1 -format "%s"
    } msg] $msg
} {0 0}

# Seconds since epoch
test utils.220 {format %s} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 2, 1970"]
	blt::utils::format $d1 -format "%s"
    } msg] $msg
} {0 86400}

# Second (00-59)
test utils.221 {format %S} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1970"]
	blt::utils::format $d1 -format "%S"
    } msg] $msg
} {0 00}

# Second (00-59)
test utils.222 {format %S} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1970 00:00:59"]
	blt::utils::format $d1 -format "%S"
    } msg] $msg
} {0 59}

# Second (00-59) does not contain fraction
test utils.223 {format %S} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1970 00:00:59.9999123"]
	blt::utils::format $d1 -format "%S"
    } msg] $msg
} {0 59}

# Time hh:mm:ss
test utils.224 {format %T} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1970 01:02:03"]
	blt::utils::format $d1 -format "%T"
    } msg] $msg
} {0 01:02:03}

# Day of week 1-7 Sunday = 1
test utils.225 {format %u} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 1, 1970"]
	blt::utils::format $d1 -format "%u"
    } msg] $msg
} {0 5}

# Day of week 1-7 Sunday = 1
test utils.226 {format %u} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%u"
    } msg] $msg
} {0 1}

# Week number (01-53).  Sunday is first day of week.
test utils.227 {format %U} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%U"
    } msg] $msg
} {0 02}

# ISO week number (01-53).  Monday is first day of week.
test utils.228 {format %V} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%V"
    } msg] $msg
} {0 01}

# Week day (0-6). Sunday is 0.
test utils.229 {format %w} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%w"
    } msg] $msg
} {0 0}

# Week (00-53). Monday is the first day of week. (I don't know what this
# is.)
test utils.230 {format %W} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%W"
    } msg] $msg
} {0 01}

# representation mm/dd/yy
test utils.231 {format %x} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%x"
    } msg] $msg
} {0 01/05/14}

# Year last 2 digits (yy)
test utils.232 {format %y} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%y"
    } msg] $msg
} {0 14}

# Year 4 digits (yyyy)
test utils.233 {format %Y} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%Y"
    } msg] $msg
} {0 2014}

# Year 5 digits (yyyyy)
test utils.234 {format %Y} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 12014"]
	blt::utils::format $d1 -format "%Y"
    } msg] $msg
} {0 12014}

# Timezone
test utils.235 {format %z} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014"]
	blt::utils::format $d1 -format "%z"
    } msg] $msg
} {0 +0000}

# Timezone (always +0000).
test utils.236 {format %z} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014 PST"]
	blt::utils::format $d1 -format "%z"
    } msg] $msg
} {0 +0000}

# Mixed.
test utils.237 {format "abcd%aefghi"} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014 PST"]
	blt::utils::format $d1 -format "abcd%aefghi"
    } msg] $msg
} {0 abcdSunefghi}

# Non-substitutions.
test utils.238 {format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z"} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014 PST"]
	blt::utils::format $d1 -format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z"
    } msg] $msg
} {0 %E%f%i%J%K%L%n%q%Q%t%v%X%Z}

# Non-substitutions.
test utils.239 {format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z%%s"} { 
    list [catch {
 	set d1 [blt::utils::scan "Jan 5, 2014 PST"]
	blt::utils::format $d1 -format "%E%f%i%J%K%L%n%q%Q%t%v%X%Z%%s"
    } msg] $msg
} {0 %E%f%i%J%K%L%n%q%Q%t%v%X%Z%s}





