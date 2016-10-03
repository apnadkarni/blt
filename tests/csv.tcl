
package require BLT

# Test switches
#
# -before, -after, -node for "insert" operation
#

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1


test csv.1 {csv no args} {
    list [catch {blt::csv} msg] $msg
} {1 {wrong # args: should be one of...
  blt::csv guess ?switches?
  blt::csv parse ?switches?}}

test csv.2 {csv parse -file (noArg) } {
    list [catch {
	blt::csv parse -file
    } msg] $msg
} {1 {value for "-file" missing}}

test csv.3 {csv parse -file badFile } {
    list [catch {
	blt::csv parse -file badFile
    } msg] $msg
} {1 {couldn't open "badFile": no such file or directory}}

test csv.4 {csv parse -badSwitch badArg } {
    list [catch {
	blt::csv parse -badSwitch badArg
    } msg] $msg
} {1 {unknown switch "-badSwitch"
The following switches are available:
   -comment char
   -data string
   -emptyvalue string
   -encoding string
   -file fileName
   -maxrows numRows
   -possibleseparators string
   -quote char
   -separator char}}

set data {
    a,b,c,d,e,f
    1,2,3,4,5
    A,B,C,D
    1,2,3
}

test csv.5 {csv parse -data -maxrows } {
    list [catch {
	set out [blt::csv parse -data $data -maxrows 1]
	llength $out
    } msg] $msg
} {0 1}

test csv.6 {csv parse -data -maxrows } {
    list [catch {
	set out [blt::csv parse -data $data -maxrows 10]
	llength $out
    } msg] $msg
} {0 4}

test csv.7 {csv parse -data -maxrows -1 } {
    list [catch {
	set out [blt::csv parse -data $data -maxrows -1]
	llength $out
    } msg] $msg
} {1 {bad value "-1": can't be negative}}



test csv.8 {csv parse -emptyvalue XXX } {
    list [catch {
	set tmpdata {
	    a,b,XXX,d,XXX,f
	    XXX,XXX,3,4,5
	    A,B,C,XXX
	    1,XXX,3
	}
	blt::csv parse -data $tmpdata -emptyvalue XXX
    } msg] $msg
} {0 {{a b {} d {} f} {{} {} 3 4 5} {A B C {}} {1 {} 3}}}

test csv.9 {csv parse -quote ' } {
    list [catch {
	set tmpdata {
	    a,b,'hi there',d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	blt::csv parse -data $tmpdata -quote '
    } msg] $msg
} {0 {{a b {hi there} d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.10 {csv parse -quote \" } {
    list [catch {
	set tmpdata {
	    a,b,"hi there",d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	blt::csv parse -data $tmpdata -quote \"
    } msg] $msg
} {0 {{a b {hi there} d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.11 {csv parse -quote ' } {
    list [catch {
	set tmpdata {
	    a,b,"hi there",d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	blt::csv parse -data $tmpdata -quote '
    } msg] $msg
} {0 {{a b {"hi there"} d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}


test csv.12 {csv parse -separator | } {
    list [catch {
	set tmpdata {
	    a|b|c|d|e|f
	    1|2|3|4|5
	    A|B|C|D
	    1|2|3
	}
	blt::csv parse -data $tmpdata -separator |
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.13 {csv parse -separator ; } {
    list [catch {
	set tmpdata {
	    a;b;c;d;e;f
	    1;2;3;4;5
	    A;B;C;D
	    1;2;3
	}
	blt::csv parse -data $tmpdata -separator \;
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.14 {csv parse -separator ? } {
    list [catch {
	set tmpdata {
	    a?b?c?d?e?f
	    1?2?3?4?5
	    A?B?C?D
	    1?2?3
	}
	blt::csv parse -data $tmpdata -separator \?
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.15 {csv parse -separator \t } {
    list [catch {
	set tmpdata {
             a	b	c	d	e	f
             1	2	3	4	5
             A	B	C	D
             1	2	3 }
	blt::csv parse -data $tmpdata -separator \t
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.16 {csv parse -separator " " } {
    list [catch {
	set tmpdata {
a b c d e f
1 2 3 4 5
A B C D
1 2 3
}
	blt::csv parse -data $tmpdata -separator " "
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.17 {csv parse -possibleseparators ? } {
    list [catch {
	set tmpdata {
	    a?b?c?d?e?f
	    1?2?3?4?5
	    A?B?C?D
	    1?2?3
	}
	blt::csv parse -data $tmpdata -possibleseparators "?;\t,"
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.18 {csv parse trim whitespace } {
    list [catch {
	set tmpdata {
	    a, b, c, d, e,	f
	    1, 2, 3, 4,		5
	    A, B, C, D
	    1, 2, 3
	}
	blt::csv parse -data $tmpdata 
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.19 {csv parse quote whitespace } {
    list [catch {
	set tmpdata {
	    a, b, c, d, e,"	f"
	    1, 2, 3, 4,"	5"
	    A, B, C, D
	    1, 2, 3
	}
	blt::csv parse -data $tmpdata 
    } msg] $msg
} {0 {{a b c d e {	f}} {1 2 3 4 {	5}} {A B C D} {1 2 3}}}

test csv.20 {csv parse quote quote } {
    list [catch {
	set tmpdata {
	    a, b, c, d, e,""f""
	    1, 2, 3, 4,"5"""
	    A, B, C, D
	    1, 2, 3
	}
	blt::csv parse -data $tmpdata 
    } msg] $msg
} {0 {{a b c d e f\"\"} {1 2 3 4 5\"} {A B C D} {1 2 3}}}

test csv.21 {csv parse -comment } {
    list [catch {
	set tmpdata {
	    # This is a comment
	    a, b, c, d, e, f
	    1, 2, 3, 4, 5
	    # This is a comment
	    A, B, C, D
	    1, 2, 3
	    # This is a comment
	} 
	blt::csv parse -data $tmpdata -comment \#
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.22 {csv parse -comment } {
    list [catch {
	set tmpdata {
	    # This is a comment
	    a, b, c, d, e, #f
	    1#, 2, 3, 4, 5
	    # This is a comment
	    A, B, C, D
	    1, 2, 3
	    # This is a comment
	} 
	blt::csv parse -data $tmpdata -comment \#
    } msg] $msg
} {0 {{a b c d e #f} {1# 2 3 4 5} {A B C D} {1 2 3}}}


test csv.23 {csv parse -file} {
    list [catch {
	set tmpdata {
	    a,b,c,d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	set f [open "/tmp/tmpdata.csv" "w"]
	puts $f $tmpdata
	close $f
	set data [blt::csv parse -file /tmp/tmpdata.csv]
	file delete -force /tmp/tmpdata.csv
	set data
    } msg] $msg
} {0 {{a b c d e f} {1 2 3 4 5} {A B C D} {1 2 3}}}

test csv.24 {csv parse -file -data} {
    list [catch {
	set tmpdata {
	    a,b,c,d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	set f [open "/tmp/tmpdata.csv" "w"]
	puts $f $tmpdata
	close $f
	set data [blt::csv parse -file /tmp/tmpdata.csv -data $tmpdata]
	file delete -force /tmp/tmpdata.csv
	set data
    } msg] $msg
} {1 {can't set both -file and -data switches.}}

test csv.25 {csv guess } {
    list [catch {
	blt::csv guess
    } msg] $msg
} {0 {}}

test csv.26 {csv guess -badSwitch } {
    list [catch {
	blt::csv guess -badSwitch
    } msg] $msg
} {1 {unknown switch "-badSwitch"
The following switches are available:
   -comment char
   -data string
   -encoding string
   -file fileName
   -maxrows numRows
   -possibleseparators string}}

test csv.27 {csv guess -data } {
    list [catch {
	set tmpdata {
	    a,b,c,d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	blt::csv guess -data $tmpdata
    } msg] $msg
} {0 {, 14 {	} 5 | 0 {;} 0}}

test csv.28 {csv guess -data -maxrows} {
    list [catch {
	set tmpdata {
	    a,b,c,d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	blt::csv guess -data $tmpdata -maxrows 100
    } msg] $msg
} {0 {, 14 {	} 5 | 0 {;} 0}}

test csv.29 {csv guess -data -maxrows } {
    list [catch {
	set tmpdata {
	    a,b,c,d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	blt::csv guess -data $tmpdata -maxrows 1
    } msg] $msg
} {0 {, 5 {	} 1 | 0 {;} 0}}

test csv.30 {csv guess -data -possibleseparators } {
    list [catch {
	set tmpdata {
	    a,b,c,d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	blt::csv guess -data $tmpdata -possibleseparators ","
    } msg] $msg
} {0 {, 14}}

test csv.31 {csv guess -data -possibleseparators } {
    list [catch {
	set tmpdata {
	    a,b,c,d,e,f
	    1,2,3,4,5
	    A,B,C,D
	    1,2,3
	}
	blt::csv guess -data $tmpdata -possibleseparators ",|;"
    } msg] $msg
} {0 {, 14 | 0 {;} 0}}

test csv.32 {csv guess -data -possibleseparators } {
    list [catch {
	set tmpdata {
	    a,b,c,d,e,f
	    1|2|3|4|5
	    A,B,C,D
	    1|2|3
	}
	blt::csv guess -data $tmpdata -possibleseparators ",|;"
    } msg] $msg
} {0 {, 8 | 6 {;} 0}}

test csv.33 {csv guess -comment } {
    list [catch {
	set tmpdata {
	    # This is a comment
	    a, b, c, d, e, f
	    1, 2, 3, 4, 5
	    # This is a comment
	    A, B, C, D
	    1, 2, 3
	    # This is a comment
	} 
	blt::csv guess -data $tmpdata -comment \#
    } msg] $msg
} {0 {, 14 {	} 8 | 0 {;} 0}}

exit 0



