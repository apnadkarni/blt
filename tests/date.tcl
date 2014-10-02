
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

