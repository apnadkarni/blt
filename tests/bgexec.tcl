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
   -echo bool
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
   -pty 
   -session 
   -updatevariable varName}}

test bgexec.5 {bgexec myVar -badSwitch } {
    list [catch {blt::bgexec myMyVar -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
The following switches are available:
   -decodeerror encodingName
   -decodeoutput encodingName
   -detach bool
   -echo bool
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
   -pty 
   -session 
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

test bgexec.11 {bgexec myVar -error (no arg) } {
    list [catch {blt::bgexec myVar -error} msg] $msg
} {1 {value for "-error" missing}}

test bgexec.12 {bgexec myVar -ignoreexitcode (no arg) } {
    list [catch {blt::bgexec myVar -ignoreexitcode} msg] $msg
} {1 {value for "-ignoreexitcode" missing}}

test bgexec.13 {bgexec myVar -keepnewline (no arg) } {
    list [catch {blt::bgexec myVar -keepnewline} msg] $msg
} {1 {value for "-keepnewline" missing}}

test bgexec.14 {bgexec myVar -killsignal (no arg) } {
    list [catch {blt::bgexec myVar -killsignal} msg] $msg
} {1 {value for "-killsignal" missing}}

test bgexec.15 {bgexec myVar -lasterror (no arg) } {
    list [catch {blt::bgexec myVar -lasterror} msg] $msg
} {1 {value for "-lasterror" missing}}

test bgexec.16 {bgexec myVar -lastoutput (no arg) } {
    list [catch {blt::bgexec myVar -lastoutput} msg] $msg
} {1 {value for "-lastoutput" missing}}

test bgexec.17 {bgexec myVar -linebuffered (no arg) } {
    list [catch {blt::bgexec myVar -linebuffered} msg] $msg
} {1 {value for "-linebuffered" missing}}

test bgexec.18 {bgexec myVar -onerror (no arg) } {
    list [catch {blt::bgexec myVar -onerror} msg] $msg
} {1 {value for "-onerror" missing}}

test bgexec.19 {bgexec myVar -onoutput (no arg) } {
    list [catch {blt::bgexec myVar -onoutput} msg] $msg
} {1 {value for "-onoutput" missing}}

test bgexec.20 {bgexec myVar -output (no arg) } {
    list [catch {blt::bgexec myVar -output} msg] $msg
} {1 {value for "-output" missing}}

test bgexec.21 {bgexec myVar -poll (no arg) } {
    list [catch {blt::bgexec myVar -poll} msg] $msg
} {1 {value for "-poll" missing}}

test bgexec.22 {bgexec myVar -update (no arg) } {
    list [catch {blt::bgexec myVar -update} msg] $msg
} {1 {value for "-update" missing}}

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
} {1 {expected boolean value but got "badArg"}}

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

test bgexec.39 {bgexec myVar stderr collect w/ -error } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -error myErr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.40 {bgexec myVar stderr collect w/ -error -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -error myErr -keepnewline yes tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.41 {bgexec myVar stdout collect w/ -output } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -output myOut tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}


test bgexec.42 {bgexec myVar stdout collect } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}

test bgexec.43 {bgexec pipeline } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.44 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.45 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.46 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.47 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.48 {bgexec pipeline } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl | tclsh files/cat.tcl | tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.49 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode yes tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.50 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode yes tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.51 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode no tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.52 {bgexec myVar -environ } {
    list [catch {
	blt::bgexec myVar -environ "MYVAR myValue" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.53 {bgexec myVar -environ } {
    list [catch {
	blt::bgexec myVar -environ "" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.54 {bgexec myVar -environ } {
    list [catch {
	blt::bgexec myVar -environ "PATH myPath" \
	    tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.55 {bgexec myVar -echo } {
    list [catch {
	blt::bgexec myVar -echo yes tclsh files/both.tcl 2> /dev/null
    } msg] $msg
} {0 {This is stdout}}

test bgexec.56 {bgexec myVar -echo } {
    list [catch {
	blt::bgexec myVar -echo no tclsh files/both.tcl 2> /dev/null
    } msg] $msg
} {0 {This is stdout}}

test bgexec.57 {bgexec myVar stderr collect w/ -onerror } {
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

test bgexec.58 {bgexec myVar stdout collect w/ -onoutput } {
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

test bgexec.59 {bgexec myVar stderr collect w/ -onerror } {
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

test bgexec.60 {bgexec myVar stdout collect w/ -onoutput } {
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

test bgexec.60 {bgexec kill with statVar } {
    list [catch {
	set myVar {}
	set myOut {}
	blt::bgexec ::myVar -output myOut tclsh files/sleep.tcl 2000  &
	after 100 { set ::myVar {} } 
	set myOut
    } msg] $msg
} {0 {}}

# Session tests
test bgexec.61 {bgexec myVar -session badCmd } {
    list [catch {blt::bgexec myVar -session badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}

test bgexec.62 {bgexec myVar -session badCmd } {
    list [catch {blt::bgexec myVar -session badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}


test bgexec.63 {bgexec myVar -session redirect input } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0}}


test bgexec.64 {bgexec myVar -session redirect input } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.65 {bgexec myVar -session redirect input } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar -session tclsh files/cat.tcl <@ $file]
	close $file
	set out
    } msg] $msg
} {0 {exit 0}}


test bgexec.66 {bgexec myVar -session bad redirect syntax } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar -session tclsh files/cat.tcl @< $file]
	close $file
	set out
    } msg] $msg
} {1 {child process exited abnormally}}


test bgexec.67 {bgexec myVar -session -keepnewline } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline 1 \
	    -session \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.68 {bgexec myVar -session stdout collect w/ -output -keepnewline} {
    list [catch {
	set myOut {}
	blt::bgexec myVar \
	    -session \
	    -output myOut \
	    -keepnewline yes \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.69 {bgexec myVar -session redirect input } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.70 {bgexec myVar -session redirect input } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.71 {bgexec myVar -session redirect input } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar -session tclsh files/cat.tcl <@ $file]
	close $file
	set out
    } msg] $msg
} {0 {exit 0}}


test bgexec.72 {bgexec myVar -session bad redirect syntax } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar -session tclsh files/cat.tcl @< $file]
	close $file
	set out
    } msg] $msg
} {1 {child process exited abnormally}}


test bgexec.73 {bgexec myVar -session -keepnewline } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline yes \
	    -session \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.74 {bgexec myVar -session -keepnewline } {
    list [catch {
	blt::bgexec myVar \
	    -session \
	    -keepnewline yes \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.75 {bgexec myVar -session stderr collect w/ -error } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -session -error myErr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.76 {bgexec myVar -session stderr collect w/ -error -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar \
	    -session \
	    -error myErr \
	    -keepnewline 1 \
	    tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.77 {bgexec myVar -session stdout collect w/ -output } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -session -output myOut tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}


test bgexec.78 {bgexec myVar -session stdout collect } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.79 {bgexec pipeline } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.80 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.81 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.82 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.83 {bgexec exitcode } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.84 {bgexec pipeline } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/stdout.tcl | tclsh files/cat.tcl | tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.85 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.86 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.87 {bgexec -ignoreexitcode } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode no \
	    tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.88 {bgexec myVar -session -environ } {
    list [catch {
	blt::bgexec myVar -session -environ "MYVAR myValue" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.89 {bgexec myVar -session -environ } {
    list [catch {
	blt::bgexec myVar -session -environ "" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.90 {bgexec myVar -session -environ } {
    list [catch {
	blt::bgexec myVar -session -environ "PATH myPath" \
	    tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.91 {bgexec myVar -session -echo } {
    list [catch {
	blt::bgexec myVar -session -echo yes tclsh files/both.tcl 2> /dev/null
    } msg] $msg
} {0 {This is stdout}}

test bgexec.92 {bgexec myVar -session -echo } {
    list [catch {
	blt::bgexec myVar -session -echo no tclsh files/both.tcl 2> /dev/null
    } msg] $msg
} {0 {This is stdout}}

test bgexec.93 {bgexec myVar -session stderr collect w/ -onerror } {
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

test bgexec.94 {bgexec myVar -session stdout collect w/ -onoutput } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -session \
	    -onoutput CollectStdout \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.95 {bgexec myVar -session stderr collect w/ -onerror } {
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

test bgexec.96 {bgexec myVar -session stdout collect w/ -onoutput } {
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

test bgexec.97 {bgexec myVar -session stdout collect w/ -output -keepnewline} {
    list [catch {
	set myOut {}
	blt::bgexec myVar \
	    -output myOut \
	    -keepnewline yes \
	    -session \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.97 {bgexec myVar -session stdout collect w/ -output -keepnewline} {
    list [catch {
	set myOut {}
	blt::bgexec myVar \
	    -output myOut \
	    -keepnewline yes \
	    -session \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.60 {bgexec -session kill with statVar } {
    list [catch {
	set myVar {}
	set myOut {}
	blt::bgexec ::myVar \
	    -session \
	    -output myOut \
	    tclsh files/sleep.tcl 2000 &
	after 100 { set ::myVar {} } 
	set myOut
    } msg] $msg
} {0 {}}

test bgexec.60 {bgexec -session kill grandchildren with statVar } {
    list [catch {
	set myVar {}
	set myOut {}
	blt::bgexec ::myVar \
	    -session \
	    -output myOut \
	    /bin/sh files/children.sh &
	after 100 { set ::myVar {} } 
	set myOut
    } msg] $msg
} {0 {}}

exit 0

exit 0


