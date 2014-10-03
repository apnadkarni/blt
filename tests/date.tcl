
package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test date.1 {date no args} {
    list [catch {blt::date} msg] $msg
} {1 {wrong # args: should be one of...
  blt::date format seconds ?switches?
  blt::date scan date}}

test date.2 {date bad arg} {
    list [catch {blt::date badArg} msg] $msg
} {1 {bad operation "badArg": should be one of...
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

test date.32 {date scan "00:00"} {
    list [catch {
	set d1 [blt::date scan "00:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.33 {date scan "00:00:00"} {
    list [catch {
	set d1 [blt::date scan "00:00:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.34 {date scan "00:00:00.000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.35 {date scan "000000"} {
    list [catch {
	set d1 [blt::date scan "000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.36 {date scan "00.00.00"} {
    list [catch {
	set d1 [blt::date scan "00.00.00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.37 {date scan "00.00.00.000"} {
    list [catch {
	set d1 [blt::date scan "00.00.00.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.38 {date scan "00.00.00.00000000"} {
    list [catch {
	set d1 [blt::date scan "00.00.00.00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.39 {date scan "00:00:00,00000000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00,00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.40 {date scan "t00:00:00,00000000"} {
    list [catch {
	set d1 [blt::date scan "t00:00:00,00000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.41 {date scan "00:00:00+0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00+0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.42 {date scan "00:00:00-0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.43 {date scan "00:00:00 gmt"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 gmt"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.44 {date scan "00:00:00 utc"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 utc"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.45 {date scan "00:00:00z"} {
    list [catch {
	set d1 [blt::date scan "00:00:00z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.46 {date scan "00:00:00 z"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 z"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.47 {date scan "00:00:00 gmt+0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 gmt+0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.48 {date scan "00:00:00 gmt-0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 gmt-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.49 {date scan "00:00:00 GMT-0000"} {
    list [catch {
	set d1 [blt::date scan "00:00:00 GMT-0000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.50 {date scan "GMT"} {
    list [catch {
	set d1 [blt::date scan "GMT"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.51 {date scan ""} {
    list [catch {
 	set d1 [blt::date scan ""]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.67 {date scan "Jan 1st"} {
    list [catch {
 	set d1 [blt::date scan "Jan 1st"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.67 {date scan "1-Jan"} {
    list [catch {
 	puts stderr [blt::date parse "1-Jan"]
 	set d1 [blt::date scan "1-Jan"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

exit
set date "Jan 2, 2006 15:04:05 MST"
set d2 [clock scan $date -gmt yes]

test date.52 {date scan "2006-01-02T15:04:05Z07:00"} {
    list [catch {
 	set d1 [blt::date scan "2006-01-02T15:04:05Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.53 {date scan "Mon Jan 2 15:04:05 MST 2006"} {
    list [catch {
 	set d1 [blt::date scan "Mon Jan 2 15:04:05 MST 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.54 {date scan "Mon Jan 02 15:04:05 -0700 2006"} {
    list [catch {
 	set d1 [blt::date scan "Mon Jan 02 15:04:05 -0700 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 2, 2006 15:04:05"
set d2 [clock scan $date -gmt yes]

test date.55 {date scan "Mon Jan 2 15:04:05 2006"} {
    list [catch {
 	set d1 [blt::date scan "Mon Jan 2 15:04:05 2006"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

set date "Jan 2, 2006 15:04 MST"
set d2 [clock scan $date -gmt yes]

test date.56 {date scan "02 Jan 06 15:04 MST"} {
    list [catch {
 	set d1 [blt::date scan "02 Jan 06 15:04 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}


test date.57 {date scan "02 Jan 06 15:04 -0700"} {
    list [catch {
 	set d1 [blt::date scan "02 Jan 06 15:04 -0700"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.58 {date scan "Monday, 02-Jan-06 15:04:05"} {
    list [catch {
 	set d1 [blt::date scan "Monday, 02-Jan-06 15:04:05 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.59 {date scan "Mon, 02 Jan 2006 15:04:05 MST"} {
    list [catch {
 	set d1 [blt::date scan "Mon, 02 Jan 2006 15:04:05 MST"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.60 {date scan "Mon, 02 Jan 2006 15:04:05 -0700"} {
    list [catch {
 	set d1 [blt::date scan "Mon, 02 Jan 2006 15:04:05 -0700"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.61 {date scan "2006-01-02T15:04:05Z07:00"} {
    list [catch {
 	set d1 [blt::date scan "2006-01-02T15:04:05Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.62 {date scan "2006-01-02T15:04:05.999999999Z07:00"} {
    list [catch {
 	set d1 [blt::date scan "2006-01-02T15:04:05.999999999Z07:00"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.63 {date scan "3:04PM"} {
    list [catch {
 	set d1 [blt::date scan "3:04PM"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.64 {date scan "Jan 2 15:04:05"} {
    list [catch {
 	set d1 [blt::date scan "Jan 2 15:04:05"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.65 {date scan "Jan 2 15:04:05.000"} {
    list [catch {
 	set d1 [blt::date scan "Jan 2 15:04:05.000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.66 {date scan "Jan 2 15:04:05.000000"} {
    list [catch {
 	set d1 [blt::date scan "Jan 2 15:04:05.000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}

test date.67 {date scan "Jan 2 15:04:05.000000000"} {
    list [catch {
 	set d1 [blt::date scan "Jan 2 15:04:05.000000000"]
	expr { $d1 - $d2 }
    } msg] $msg
} {0 0.0}



