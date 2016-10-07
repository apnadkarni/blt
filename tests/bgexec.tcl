package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test bgexec.1 {bgexec (no args) } {
    list [catch {blt::bgexec} msg] $msg
} {1 {wrong # args: should be "blt::bgexec varName ?options? command ?arg...?"}}

test bgexec.2 {bgexec var (no args) } {
    list [catch {blt::bgexec myVar} msg] $msg
} {1 {wrong # args: should be "blt::bgexec varName ?options? command ?arg...?"}}

test bgexec.3 {bgexec var tclsh files/null.tcl} {
    list [catch {blt::bgexec myVar tclsh files/null.tcl} msg] $msg
} {0 {}}

test bgexec.4 {bgexec var -help } {
    list [catch {blt::bgexec myVar -help} msg] $msg
} {1 {The following switches are available:
   -decodeerror encodingName
   -decodeoutput encodingName
   -detach bool
   -echo echoName
   -environ list
   -errorvariable varName
   -ignoreexitcode bool
   -keepnewline bool
   -killsignal signalName
   -lasterrorvariable varName
   -lastoutputvariable varName
   -linebuffered bool
   -onerror cmdString
   -onoutput cmdString
   -outputvariable varName
   -poll milliseconds
   -session 
   -tty 
   -updatevariable varName}}

test bgexec.5 {bgexec myVar -badSwitch } {
    list [catch {blt::bgexec myMyVar -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
The following switches are available:
   -decodeerror encodingName
   -decodeoutput encodingName
   -detach bool
   -echo echoName
   -environ list
   -errorvariable varName
   -ignoreexitcode bool
   -keepnewline bool
   -killsignal signalName
   -lasterrorvariable varName
   -lastoutputvariable varName
   -linebuffered bool
   -onerror cmdString
   -onoutput cmdString
   -outputvariable varName
   -poll milliseconds
   -session 
   -tty 
   -updatevariable varName}}

test bgexec.6 {bgexec myVar -decodeerror (no arg) } {
    list [catch {blt::bgexec myVar -decodeerror} msg] $msg
} {1 {value for "-decodeerror" missing}}

test bgexec.7 {bgexec myVar -decodeoutput (no arg) } {
    list [catch {blt::bgexec myVar -decodeoutput} msg] $msg
} {1 {value for "-decodeoutput" missing}}

test bgexec.8 {bgexec myVar -detach (no arg) } {
    list [catch {blt::bgexec myVar -detach} msg] $msg
} {1 {value for "-detach" missing}}

test bgexec.9 {bgexec myVar -echo (no arg) } {
    list [catch {blt::bgexec myVar -echo} msg] $msg
} {1 {value for "-echo" missing}}

test bgexec.10 {bgexec myVar -environ (no arg) } {
    list [catch {blt::bgexec myVar -environ} msg] $msg
} {1 {value for "-environ" missing}}

test bgexec.11 {bgexec myVar -errorvariable (no arg) } {
    list [catch {blt::bgexec myVar -errorvariable} msg] $msg
} {1 {value for "-errorvariable" missing}}

test bgexec.12 {bgexec myVar -ignoreexitcode (no arg) } {
    list [catch {blt::bgexec myVar -ignoreexitcode} msg] $msg
} {1 {value for "-ignoreexitcode" missing}}

test bgexec.13 {bgexec myVar -keepnewline (no arg) } {
    list [catch {blt::bgexec myVar -keepnewline} msg] $msg
} {1 {value for "-keepnewline" missing}}

test bgexec.14 {bgexec myVar -killsignal (no arg) } {
    list [catch {blt::bgexec myVar -killsignal} msg] $msg
} {1 {value for "-killsignal" missing}}

test bgexec.15 {bgexec myVar -lasterrorvariable (no arg) } {
    list [catch {blt::bgexec myVar -lasterrorvariable} msg] $msg
} {1 {value for "-lasterrorvariable" missing}}

test bgexec.16 {bgexec myVar -lastoutputvariable (no arg) } {
    list [catch {blt::bgexec myVar -lastoutputvariable} msg] $msg
} {1 {value for "-lastoutputvariable" missing}}

test bgexec.17 {bgexec myVar -linebuffered (no arg) } {
    list [catch {blt::bgexec myVar -linebuffered} msg] $msg
} {1 {value for "-linebuffered" missing}}

test bgexec.18 {bgexec myVar -onerror (no arg) } {
    list [catch {blt::bgexec myVar -onerror} msg] $msg
} {1 {value for "-onerror" missing}}

test bgexec.19 {bgexec myVar -onoutput (no arg) } {
    list [catch {blt::bgexec myVar -onoutput} msg] $msg
} {1 {value for "-onoutput" missing}}

test bgexec.20 {bgexec myVar -outputvariable (no arg) } {
    list [catch {blt::bgexec myVar -outputvariable} msg] $msg
} {1 {value for "-outputvariable" missing}}

test bgexec.21 {bgexec myVar -poll (no arg) } {
    list [catch {blt::bgexec myVar -poll} msg] $msg
} {1 {value for "-poll" missing}}

test bgexec.22 {bgexec myVar -updatevariable (no arg) } {
    list [catch {blt::bgexec myVar -updatevariable} msg] $msg
} {1 {value for "-updatevariable" missing}}

test bgexec.23 {bgexec myVar -decodeerror (bad arg) } {
    list [catch {blt::bgexec myVar -decodeerror badArg} msg] $msg
} {1 {unknown encoding "badArg"}}

test bgexec.24 {bgexec myVar -decodeoutput (bad arg) } {
    list [catch {blt::bgexec myVar -decodeoutput badArg} msg] $msg
} {1 {unknown encoding "badArg"}}

test bgexec.25 {bgexec myVar -detach (bad arg) } {
    list [catch {blt::bgexec myVar -detach badArg} msg] $msg
} {1 {expected boolean value but got "badArg"}}

test bgexec.26 {bgexec myVar -echo (bad arg) } {
    list [catch {blt::bgexec myVar -echo badArg} msg] $msg
} {1 {unknown echo value "badArg": should be error, output, both, or none.}}

test bgexec.27 {bgexec myVar -ignoreexitcode (bad arg) } {
    list [catch {blt::bgexec myVar -ignoreexitcode badArg} msg] $msg
} {1 {expected boolean value but got "badArg"}}

test bgexec.28 {bgexec myVar -keepnewline (bad arg) } {
    list [catch {blt::bgexec myVar -keepnewline badArg} msg] $msg
} {1 {expected boolean value but got "badArg"}}

test bgexec.29 {bgexec myVar -linebuffered (bad arg) } {
    list [catch {blt::bgexec myVar -linebuffered badArg} msg] $msg
} {1 {expected boolean value but got "badArg"}}

test bgexec.30 {bgexec myVar -poll (bad arg) } {
    list [catch {blt::bgexec myVar -poll badArg} msg] $msg
} {1 {expected integer but got "badArg"}}


test bgexec.31 {bgexec myVar badCmd } {
    list [catch {blt::bgexec myVar badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}

test bgexec.32 {bgexec myVar badCmd } {
    list [catch {blt::bgexec myVar badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}


test bgexec.33 {bgexec myVar redirect input } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.34 {bgexec myVar redirect input } {
    list [catch {blt::bgexec myVar tclsh files/cat.tcl << "test me"} msg] $msg
} {0 {test me}}

test bgexec.35 {bgexec myVar redirect input } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar tclsh files/cat.tcl <@ $file]
	close $file
	set out
    } msg] $msg
} {0 {exit 0}}


test bgexec.36 {bgexec myVar bad redirect syntax } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar tclsh files/cat.tcl @< $file]
	close $file
	set out
    } msg] $msg
} {1 {child process exited abnormally}}


test bgexec.37 {bgexec myVar -keepnewline } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline yes  \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.38 {bgexec myVar -keepnewline } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline yes \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0\n}}

test bgexec.39 {bgexec myVar stderr collect w/ -errorvariable } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -errorvariable myErr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.40 {bgexec myVar stderr collect w/ -errorvariable -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -errorvariable myErr -keepnewline yes tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.41 {bgexec myVar stdout collect w/ -outputvariable } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -outputvariable myOut tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}


test bgexec.42 {bgexec myVar stdout collect /w -updatevariable } {
    list [catch {
	set myOut {}
	trace variable myUpdateVar w CollectStdout
	proc CollectStdout { name1 name2 how } {
	    global myOut myUpdateVar
	    append myOut $myUpdateVar
	    set myUpdateVar {}
	}
	blt::bgexec myVar -updatevariable myUpdateVar tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.43 {bgexec myVar stdout collect } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}

test bgexec.44 {bgexec pipeline } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.45 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.46 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.47 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.48 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.49 {bgexec pipeline } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl | tclsh files/cat.tcl | tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.50 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode yes tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.51 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode yes tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.52 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode no tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.53 {bgexec myVar -environ } {
    list [catch {
	blt::bgexec myVar -environ "MYVAR myValue" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.54 {bgexec myVar -environ } {
    list [catch {
	blt::bgexec myVar -environ "" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.55 {bgexec myVar -environ } {
    list [catch {
	blt::bgexec myVar -environ "PATH myPath" \
	    tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.56 {bgexec myVar -echo error } {
    list [catch {
	blt::bgexec myVar -echo error tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.57 {bgexec myVar -echo output } {
    list [catch {
	blt::bgexec myVar -echo output tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.58 {bgexec myVar -echo both } {
    list [catch {
	blt::bgexec myVar -echo both tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.59 {bgexec myVar -echo none } {
    list [catch {
	blt::bgexec myVar -echo none tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.60 {bgexec myVar stderr collect w/ -onerror } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -onerror CollectStderr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.61 {bgexec myVar stdout collect w/ -onoutput } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -onoutput CollectStdout tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.62 {bgexec myVar stderr collect w/ -onerror } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar \
	    -onerror CollectStderr \
	    -keepnewline yes \
	    tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.63 {bgexec myVar stdout collect w/ -onoutput } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar\
	    -keepnewline yes \
	    -onoutput CollectStdout \
	    tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout\n}}

test bgexec.64 {bgexec kill with myVar } {
    list [catch {
	set myVar -1
	set myOut {}
	blt::bgexec ::myVar -outputvariable myOut tclsh files/sleep.tcl 4000  &
	after 100
	set myVar die
	set myOut
    } msg] $msg
} {0 {}}

# Session tests
test bgexec.65 {bgexec myVar -session badCmd } {
    list [catch {blt::bgexec myVar -session badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}

test bgexec.66 {bgexec myVar -session badCmd } {
    list [catch {blt::bgexec myVar -session badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}

test bgexec.67 {bgexec myVar -session redirect input } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0}}


test bgexec.68 {bgexec myVar -session redirect input } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.69 {bgexec myVar -session redirect input } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar -session tclsh files/cat.tcl <@ $file]
	close $file
	set out
    } msg] $msg
} {0 {exit 0}}

test bgexec.70 {bgexec myVar -session bad redirect syntax } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar -session tclsh files/cat.tcl @< $file]
	close $file
	set out
    } msg] $msg
} {1 {child process exited abnormally}}

test bgexec.71 {bgexec myVar -session -keepnewline } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline 1 \
	    -session \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.72 {bgexec myVar -session stdout collect w/ -outputvariable -keepnewline} {
    list [catch {
	set myOut {}
	blt::bgexec myVar \
	    -session \
	    -outputvariable myOut \
	    -keepnewline yes \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.73 {bgexec myVar -session redirect input } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.74 {bgexec myVar -session redirect input } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.75 {bgexec myVar -session redirect input } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar -session tclsh files/cat.tcl <@ $file]
	close $file
	set out
    } msg] $msg
} {0 {exit 0}}


test bgexec.76 {bgexec myVar -session bad redirect syntax } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar -session tclsh files/cat.tcl @< $file]
	close $file
	set out
    } msg] $msg
} {1 {child process exited abnormally}}


test bgexec.77 {bgexec myVar -session -keepnewline } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline yes \
	    -session \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.78 {bgexec myVar -session -keepnewline } {
    list [catch {
	blt::bgexec myVar \
	    -session \
	    -keepnewline yes \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.79 {bgexec myVar -session collect w/ -errorvariable } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -session -errorvariable myErr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.80 {bgexec myVar -session collect w/ -errorvariable -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar \
	    -session \
	    -errorvariable myErr \
	    -keepnewline 1 \
	    tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr
}}


test bgexec.81 {bgexec myVar -session collect w/ -outputvariable } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -session -outputvariable myOut tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}


test bgexec.82 {bgexec myVar -session stdout collect /w -updatevariable } {
    list [catch {
	set myOut {}
	trace variable myUpdateVar w CollectStdout
	proc CollectStdout { name1 name2 how } {
	    global myOut myUpdateVar
	    append myOut $myUpdateVar
	    set myUpdateVar {}
	}
	blt::bgexec myVar -session -updatevariable myUpdateVar \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.83 {bgexec myVar -session stdout collect } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.84 {bgexec pipeline } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.85 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.86 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.87 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.88 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.89 {bgexec pipeline } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/stdout.tcl | tclsh files/cat.tcl | tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.90 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.91 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.92 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode no \
	    tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.93 {bgexec myVar -session -environ } {
    list [catch {
	blt::bgexec myVar -session -environ "MYVAR myValue" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.94 {bgexec myVar -session -environ } {
    list [catch {
	blt::bgexec myVar -session -environ "" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.95 {bgexec myVar -session -environ } {
    list [catch {
	blt::bgexec myVar -session -environ "PATH myPath" \
	    tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.96 {bgexec myVar -session -echo error } {
    list [catch {
	blt::bgexec myVar -session -echo error tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.97 {bgexec myVar -session -echo output } {
    list [catch {
	blt::bgexec myVar -session -echo output tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.98 {bgexec myVar -session -echo both } {
    list [catch {
	blt::bgexec myVar -session -echo both tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.99 {bgexec myVar -session -echo none } {
    list [catch {
	blt::bgexec myVar -session -echo none tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.100 {bgexec myVar -session stderr collect w/ -onerror } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -session -onerror CollectStderr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.101 {bgexec myVar -session stdout collect w/ -onoutput } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -session -onoutput CollectStdout \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.102 {bgexec myVar -session stderr collect w/ -onerror } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -session -onerror CollectStderr -keepnewline yes \
	    tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.103 {bgexec myVar -session stdout collect w/ -onoutput } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -session -keepnewline yes -onoutput CollectStdout \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.104 {bgexec myVar -session stdout collect w/ -outputvariable -keepnewline} {
    list [catch {
	set myOut {}
	blt::bgexec myVar -outputvariable myOut -keepnewline yes -session \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.105 {bgexec myVar -session stdout collect w/ -outputvariable -keepnewline} {
    list [catch {
	set myOut {}
	blt::bgexec myVar -outputvariable myOut -keepnewline yes -session \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.106 {bgexec -session kill with myVar } {
    list [catch {
	set myVar -1
	set myOut {}
	blt::bgexec myVar -session -outputvariable myOut \
	    tclsh files/sleep.tcl 10000 &
	after 100
	set myVar die  
	set myOut
    } msg] $msg
} {0 {}}

test bgexec.107 {bgexec -session kill grandchildren with myVar } {
    list [catch {
	set myVar {}
	set myOut {}
	blt::bgexec ::myVar -session -outputvariable myOut \
	    /bin/sh files/children.sh &
	after 100
	set myVar die
	set myOut
    } msg] $msg
} {0 {}}

test bgexec.108 {bgexec myVar -tty } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}

test bgexec.109 {bgexec myVar -tty both } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl 2>@1
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.110 {bgexec myVar -session both } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl 2>@1
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.111 {bgexec myVar -tty both } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr \
	    tclsh files/both.tcl
    } msg] [list $msg $myErr]
} {0 {{This is stdout} {This is stderr}}}

test bgexec.112 {bgexec myVar -tty stdout } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr \
	    tclsh files/stdout.tcl
    } msg] [list $msg $myErr]
} {0 {{This is stdout} {}}}

test bgexec.113 {bgexec myVar -tty stderr } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr \
	    tclsh files/stderr.tcl
    } msg] [list $msg $myErr]
} {0 {{} {This is stderr}}}

test bgexec.114 {bgexec myVar | } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.115 {bgexec myVar | stderr } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {}}

test bgexec.116 {bgexec myVar |& both } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl |& tclsh files/cat.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.117 {bgexec myVar |& stderr } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stderr}}

test bgexec.118 {bgexec myVar |& stdout } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.119 {bgexec myVar |& stdout } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.120 {bgexec input redirect from file } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.121 {bgexec input redirect badFile } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < badFile
    } msg] $msg
} {1 {can't read file "badFile": no such file or directory}}

test bgexec.122 {bgexec multiple redirects } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < files/null.tcl < files/null.tcl
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.123 {bgexec input redirect missing file } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl <
    } msg] $msg
} {1 {can't specify "<" as last word in command}}

test bgexec.124 {bgexec input redirect missing literal } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl <<
    } msg] $msg
} {1 {can't specify "<<" as last word in command}}

test bgexec.125 {bgexec input redirect missing channel } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl <@
    } msg] $msg
} {1 {can't specify "<@" as last word in command}}

test bgexec.126 {bgexec multiple input redirects } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < files/null.tcl << "hi"
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.127 {bgexec multiple input redirects } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl << "hi" << "there"
    } msg] $msg
} {1 {ambigious input redirect.}}

set f [open files/null.tcl "r"]
test bgexec.128 {bgexec multiple input redirects } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl << "hi" <@ $f
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.129 {bgexec input redirect /w channel } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl <@ $f
    } msg] $msg
} {0 {exit 0}}

test bgexec.130 {bgexec input redirect /w channel } {
    list [catch {
	seek $f 0
	blt::bgexec myVar tclsh files/cat.tcl <@ $f | tclsh files/cat.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.131 {bgexec input redirect no command } {
    list [catch {
	blt::bgexec myVar < files/null.tcl
    } msg] $msg
} {1 {missing command for "<"}}

test bgexec.132 {bgexec input redirect no command } {
    list [catch {
	blt::bgexec myVar << "hi"
    } msg] $msg
} {1 {missing command for "<<"}}

test bgexec.133 {bgexec input redirect no command } {
    list [catch {
	blt::bgexec myVar <@ $f
    } msg] $msg
} {1 {missing command for "<@"}}

test bgexec.134 {bgexec input pipe no command } {
    list [catch {
	blt::bgexec myVar |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.135 {bgexec input pipe no command } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.136 {bgexec input pipe no command } {
    list [catch {
	blt::bgexec myVar | tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.134 {bgexec input pipe no command } {
    list [catch {
	blt::bgexec myVar |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.135 {bgexec input pipe no command } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.136 {bgexec input pipe no command } {
    list [catch {
	blt::bgexec myVar |& tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

exit 0







