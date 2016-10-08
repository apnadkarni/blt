package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test bgexec.1 {set up file channel } {
    list [catch {
	set file [open files/null.tcl "r"]
    } msg] $msg
} {0 file3}

test bgexec.2 {bgexec (no args) } {
    list [catch {blt::bgexec} msg] $msg
} {1 {wrong # args: should be "blt::bgexec varName ?options? command ?arg...?"}}

test bgexec.3 {bgexec var (no args) } {
    list [catch {blt::bgexec myVar} msg] $msg
} {1 {wrong # args: should be "blt::bgexec varName ?options? command ?arg...?"}}

test bgexec.4 {bgexec var tclsh files/null.tcl} {
    list [catch {blt::bgexec myVar tclsh files/null.tcl} msg] $msg
} {0 {}}

test bgexec.5 {bgexec var -help } {
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

test bgexec.6 {bgexec myVar -badSwitch } {
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

test bgexec.7 { -decodeerror (no arg) } {
    list [catch {blt::bgexec myVar -decodeerror} msg] $msg
} {1 {value for "-decodeerror" missing}}

test bgexec.8 { -decodeoutput (no arg) } {
    list [catch {blt::bgexec myVar -decodeoutput} msg] $msg
} {1 {value for "-decodeoutput" missing}}

test bgexec.9 { -detach (no arg) } {
    list [catch {blt::bgexec myVar -detach} msg] $msg
} {1 {value for "-detach" missing}}

test bgexec.10 { -echo (no arg) } {
    list [catch {blt::bgexec myVar -echo} msg] $msg
} {1 {value for "-echo" missing}}

test bgexec.11 { -environ (no arg) } {
    list [catch {blt::bgexec myVar -environ} msg] $msg
} {1 {value for "-environ" missing}}

test bgexec.12 { -errorvariable (no arg) } {
    list [catch {blt::bgexec myVar -errorvariable} msg] $msg
} {1 {value for "-errorvariable" missing}}

test bgexec.13 { -ignoreexitcode (no arg) } {
    list [catch {blt::bgexec myVar -ignoreexitcode} msg] $msg
} {1 {value for "-ignoreexitcode" missing}}

test bgexec.14 { -keepnewline (no arg) } {
    list [catch {blt::bgexec myVar -keepnewline} msg] $msg
} {1 {value for "-keepnewline" missing}}

test bgexec.15 { -killsignal (no arg) } {
    list [catch {blt::bgexec myVar -killsignal} msg] $msg
} {1 {value for "-killsignal" missing}}

test bgexec.16 { -lasterrorvariable (no arg) } {
    list [catch {blt::bgexec myVar -lasterrorvariable} msg] $msg
} {1 {value for "-lasterrorvariable" missing}}

test bgexec.17 { -lastoutputvariable (no arg) } {
    list [catch {blt::bgexec myVar -lastoutputvariable} msg] $msg
} {1 {value for "-lastoutputvariable" missing}}

test bgexec.18 { -linebuffered (no arg) } {
    list [catch {blt::bgexec myVar -linebuffered} msg] $msg
} {1 {value for "-linebuffered" missing}}

test bgexec.19 { -onerror (no arg) } {
    list [catch {blt::bgexec myVar -onerror} msg] $msg
} {1 {value for "-onerror" missing}}

test bgexec.20 { -onoutput (no arg) } {
    list [catch {blt::bgexec myVar -onoutput} msg] $msg
} {1 {value for "-onoutput" missing}}

test bgexec.21 { -outputvariable (no arg) } {
    list [catch {blt::bgexec myVar -outputvariable} msg] $msg
} {1 {value for "-outputvariable" missing}}

test bgexec.22 { -poll (no arg) } {
    list [catch {blt::bgexec myVar -poll} msg] $msg
} {1 {value for "-poll" missing}}

test bgexec.23 { -updatevariable (no arg) } {
    list [catch {blt::bgexec myVar -updatevariable} msg] $msg
} {1 {value for "-updatevariable" missing}}

test bgexec.24 { -decodeerror (bad arg) } {
    list [catch {blt::bgexec myVar -decodeerror badArg} msg] $msg
} {1 {unknown encoding "badArg"}}

test bgexec.25 { -decodeoutput (bad arg) } {
    list [catch {blt::bgexec myVar -decodeoutput badArg} msg] $msg
} {1 {unknown encoding "badArg"}}

test bgexec.26 { -detach (bad arg) } {
    list [catch {blt::bgexec myVar -detach badArg} msg] $msg
} {1 {expected boolean value but got "badArg"}}

test bgexec.27 { -echo (bad arg) } {
    list [catch {blt::bgexec myVar -echo badArg} msg] $msg
} {1 {unknown echo value "badArg": should be error, output, both, or none.}}

test bgexec.28 { -ignoreexitcode (bad arg) } {
    list [catch {blt::bgexec myVar -ignoreexitcode badArg} msg] $msg
} {1 {expected boolean value but got "badArg"}}

test bgexec.29 { -keepnewline (bad arg) } {
    list [catch {blt::bgexec myVar -keepnewline badArg} msg] $msg
} {1 {expected boolean value but got "badArg"}}

test bgexec.30 { -linebuffered (bad arg) } {
    list [catch {blt::bgexec myVar -linebuffered badArg} msg] $msg
} {1 {expected boolean value but got "badArg"}}

test bgexec.31 { -poll (bad arg) } {
    list [catch {blt::bgexec myVar -poll badArg} msg] $msg
} {1 {expected integer but got "badArg"}}

test bgexec.32 { badCmd } {
    list [catch {blt::bgexec myVar badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}

test bgexec.33 { badCmd } {
    list [catch {blt::bgexec myVar badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}

########################################################################
#	Standard tests for all platforms
########################################################################

test bgexec.34 { redirect input from file } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.35 { redirect input from literal } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.36 { redirect input from channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar tclsh files/cat.tcl <@ $file
    } msg] $msg
} {0 {exit 0}}


test bgexec.37 { bad redirect syntax is considered arg } {
    list [catch {
	seek $file 0
	blt::bgexec myVar tclsh files/cat.tcl @< $file
    } msg] $msg
} {1 {child process exited abnormally}}

test bgexec.38 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline yes  \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.39 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline yes \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0\n}}

test bgexec.40 { collect stderr w/ -errorvariable } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -errorvariable myErr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.41 { collect stderr w/ -errorvariable -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -errorvariable myErr -keepnewline yes \
	    tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.42 { collect stdout w/ -outputvariable } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -outputvariable myOut tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}


test bgexec.43 { collect stdout /w -updatevariable } {
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

test bgexec.44 { collect stdout (command result) } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.45 { pipe output } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.46 { command returns exit code 0 } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.47 { command returns exit code 1 } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.48 { command returns exit code 100 } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.49 { command returns exit code -1 } {
    set code [catch {
	blt::bgexec myVar tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.50 { multiple pipes } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl | tclsh files/cat.tcl | tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.51 { -ignoreexitcode yes (exit code 1) } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.52 { -ignoreexitcode yes (exit code 0) } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.53 { -ignoreexitcode no (exit code 100) } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode no \
	    tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.54 { -environ "MYVAR myValue" } {
    list [catch {
	blt::bgexec myVar -environ "MYVAR myValue" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.55 { -environ "" } {
    list [catch {
	blt::bgexec myVar -environ "" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.56 { -environ "PATH myPath" } {
    list [catch {
	blt::bgexec myVar -environ "PATH myPath" \
	    tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.57 { -echo error (see on screen) } {
    list [catch {
	blt::bgexec myVar -echo error tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.58 { -echo output (see on screen) } {
    list [catch {
	blt::bgexec myVar -echo output tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.59 { -echo both (see on screen) } {
    list [catch {
	blt::bgexec myVar -echo both tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.60 { -echo none } {
    list [catch {
	blt::bgexec myVar -echo none tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.61 { collect stdout w/ -onoutput } {
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

test bgexec.62 { collect stdout w/ -onoutput -keepnewline } {
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

test bgexec.63 { collect stderr w/ -onerror } {
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

test bgexec.64 { collect stderr w/ -onerror -keepnewline } {
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

test bgexec.65 { pipe both stderr and stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar \
	    tclsh files/both.tcl |& tclsh files/cat.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.66 { pipe just stderr w/ |&  } {
    list [catch {
	blt::bgexec myVar \
	    tclsh files/stderr.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stderr}}

test bgexec.67 { pipe just stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar \
	    tclsh files/stdout.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.68 { redirect input from file } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.69 { redirect input badFile } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < badFile
    } msg] $msg
} {1 {can't read file "badFile": no such file or directory}}

test bgexec.70 { multiple input redirections w/ two files } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < files/null.tcl < files/null.tcl
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.71 { redirect input w/ missing file } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl <
    } msg] $msg
} {1 {can't specify "<" as last word in command}}

test bgexec.72 { redirect input w/ missing literal } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl <<
    } msg] $msg
} {1 {can't specify "<<" as last word in command}}

test bgexec.73 { redirect input w/ missing channel } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl <@
    } msg] $msg
} {1 {can't specify "<@" as last word in command}}

test bgexec.74 { multiple input redirections /w literal and file } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl < files/null.tcl << "hi"
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.75 { multiple input redirections w/ two literals. } {
    list [catch {
	blt::bgexec myVar tclsh files/cat.tcl << "hi" << "there"
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.76 { multiple input redirections w/ literal and channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar tclsh files/cat.tcl << "hi" <@ $file
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.77 { redirect input /w channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar tclsh files/cat.tcl <@ $file
    } msg] $msg
} {0 {exit 0}}

test bgexec.78 { redirect input /w channel and pipe } {
    list [catch {
	seek $file 0
	blt::bgexec myVar tclsh files/cat.tcl <@ $file | tclsh files/cat.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.79 {  redirect input from file w/ no command } {
    list [catch {
	blt::bgexec myVar < files/null.tcl
    } msg] $msg
} {1 {missing command for "<"}}

test bgexec.80 { redirect input from literal w/ no command } {
    list [catch {
	blt::bgexec myVar << "hi"
    } msg] $msg
} {1 {missing command for "<<"}}

test bgexec.81 { redirect input from channel w/ no command } {
    list [catch {
	seek $file 0
	blt::bgexec myVar <@ $file
    } msg] $msg
} {1 {missing command for "<@"}}

test bgexec.82 { pipe w/ no commands } {
    list [catch {
	blt::bgexec myVar |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.83 { pipe w/ no output command } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.84 {  pipe w/ no input command } {
    list [catch {
	blt::bgexec myVar | tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.85 { pipe both |& w/ no commands } {
    list [catch {
	blt::bgexec myVar |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.86 { pipe both |& w/ no output command } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.87 { pipe both |& w/ no input command } {
    list [catch {
	blt::bgexec myVar |& tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.88 { echo of switches -- } {
    list [catch {
	blt::bgexec myVar -- tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.89 { redirect output to no file } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl > 
    } msg] $msg
} {1 {can't specify ">" as last word in command}}

test bgexec.90 { redirect output to bad file } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl > badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.91 { redirect output to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stdout.tcl > testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.92 { redirect output and append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stdout.tcl > testfile
	blt::bgexec myVar tclsh files/stdout.tcl >> testfile
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.93 { redirect output append to no file } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl >> 
    } msg] $msg
} {1 {can't specify ">>" as last word in command}}

test bgexec.94 { redirect output append to bad file } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl >> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.95 { redirect output append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stdout.tcl >> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.96 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stdout.tcl > testfile > testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.97 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stdout.tcl > testfile >> testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.98 { redirect stdout to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar tclsh files/stdout.tcl >@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.99 { redirect stdout to no channel } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl >@ 
    } msg] $msg
} {1 {can't specify ">@" as last word in command}}

test bgexec.100 { redirect stdout to bad channel } {
    list [catch {
	blt::bgexec myVar tclsh files/stdout.tcl >@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.101 { redirect stdout to file (verify empty string is result) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stdout.tcl > testfile
    } msg] $msg
} {0 {}}

test bgexec.102 { multiple output redirections /w two channels } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar tclsh files/stdout.tcl >@ $f >@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.103 { redirect stderr to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stderr.tcl 2> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.104 { redirect input (literal) and output (file) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/cat.tcl << "testme" > testfile
	file size testfile
    } msg] $msg
} {0 6}

test bgexec.105 { redirect stderr to no file } {
    list [catch {
	blt::bgexec myVar tclsh files/stderr.tcl 2> 
    } msg] $msg
} {1 {can't specify "2>" as last word in command}}

test bgexec.106 { redirect stderr to bad file } {
    list [catch {
	blt::bgexec myVar tclsh files/stderr.tcl 2> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.107 { redirect stderr append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stderr.tcl 2> testfile
	blt::bgexec myVar tclsh files/stderr.tcl 2>> testfile
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.108 { redirect stderr append to no file } {
    list [catch {
	blt::bgexec myVar tclsh files/stderr.tcl 2>>
    } msg] $msg
} {1 {can't specify "2>>" as last word in command}}

test bgexec.109 { redirect stderr append to bad file } {
    list [catch {
	blt::bgexec myVar tclsh files/stderr.tcl 2>> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.110 { redirect stderr append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stderr.tcl 2>> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.111 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stderr.tcl 2> testfile 2> testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.112 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/stderr.tcl 2> testfile 2>> testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.113 { redirect both stderr and stdout to files } {
    list [catch {
	file delete -force testfile1 testfile2
	blt::bgexec myVar tclsh files/both.tcl 2> testfile1 > testfile2
	expr [file size testfile1] + [file size testfile2]
    } msg] $msg
} {0 30}

test bgexec.114 { redirect both stderr and stdout append to files } {
    list [catch {
	file delete -force testfile1 testfile2
	blt::bgexec myVar tclsh files/both.tcl 2>> testfile1 >> testfile2
	expr [file size testfile1] + [file size testfile2]
    } msg] $msg
} {0 30}

test bgexec.115 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/both.tcl 2> testfile > testfile
	expr [file size testfile]
    } msg] $msg
} {0 15}

test bgexec.116 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/both.tcl 2>> testfile >> testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.117 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/both.tcl >& testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.118 { redirect both stderr and stdout to no file } {
    list [catch {
	blt::bgexec myVar tclsh files/both.tcl >& 
    } msg] $msg
} {1 {can't specify ">&" as last word in command}}

test bgexec.119 { redirect both stderr and stdout to bad file } {
    list [catch {
	blt::bgexec myVar tclsh files/both.tcl >& badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.120 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar tclsh files/both.tcl >>& testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.121 { redirect stderr to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar tclsh files/stderr.tcl 2>@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.122 { redirect stderr to no channel } {
    list [catch {
	blt::bgexec myVar tclsh files/stderr.tcl 2>@ 
    } msg] $msg
} {1 {can't specify "2>@" as last word in command}}

test bgexec.123 { redirect stderr to bad channel } {
    list [catch {
	blt::bgexec myVar tclsh files/stderr.tcl 2>@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.124 { redirect stderr to file (verify empty string is result) } {
    list [catch {
	file delete -force testfile
	set myErr {}
	blt::bgexec myVar -errorvariable myErr \
	    tclsh files/stderr.tcl 2> testfile 
	set myErr
    } msg] $msg
} {0 {}}

test bgexec.125 { multiple stderr redirections /w two channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar tclsh files/stdout.tcl 2>@ $f 2>@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.126 { redirect both stderr and stdout to one channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar tclsh files/both.tcl >&@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.127 { redirect both stderr and stdout to no channel } {
    list [catch {
	blt::bgexec myVar tclsh files/both.tcl >&@ 
    } msg] $msg
} {1 {can't specify ">&@" as last word in command}}

test bgexec.128 { multiple stderr/stdout redirections /w channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar tclsh files/both.tcl >&@ $f >&@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.129 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar tclsh files/both.tcl >&@ $f > testfile
	close $f
	file size tetfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.130 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar tclsh files/both.tcl >&@ $f 2> testfile
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.131 { redirect both stderr and stdout  } {
    list [catch {
	blt::bgexec myVar tclsh files/both.tcl 2>@1
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.132 { redirect both stderr and stdout /w extraArg } {
    list [catch {
	blt::bgexec myVar tclsh files/both.tcl 2>@1 extraArg
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {1 {must specify "2>@1" as last word in command}}

test bgexec.133 { kill by setting status variable } {
    list [catch {
	set myVar -1
	set myOut {}
	blt::bgexec ::myVar -outputvariable myOut \
	    tclsh files/sleep.tcl 4000 &
	after 100
	set myVar die
	set myOut
    } msg] $msg
} {0 {}}

########################################################################
#	Session tests
########################################################################


test bgexec.134 { redirect input from file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.135 { redirect input from literal } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.136 { redirect input from channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -session tclsh files/cat.tcl <@ $file
    } msg] $msg
} {0 {exit 0}}


test bgexec.137 { bad redirect syntax is considered arg } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -session tclsh files/cat.tcl @< $file
    } msg] $msg
} {1 {child process exited abnormally}}

test bgexec.138 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar -session \
	    -keepnewline yes  \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.139 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar -session \
	    -keepnewline yes \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0\n}}

test bgexec.140 { collect stderr w/ -errorvariable } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -session -errorvariable myErr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.141 { collect stderr w/ -errorvariable -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -session -errorvariable myErr -keepnewline yes \
	    tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.142 { collect stdout w/ -outputvariable } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -session -outputvariable myOut \
	    tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}


test bgexec.143 { collect stdout /w -updatevariable } {
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

test bgexec.144 { collect stdout (command result) } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.145 { pipe output } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.146 { command returns exit code 0 } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.147 { command returns exit code 1 } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.148 { command returns exit code 100 } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.149 { command returns exit code -1 } {
    set code [catch {
	blt::bgexec myVar -session tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.150 { multiple pipes } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/stdout.tcl | tclsh files/cat.tcl | tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.151 { -ignoreexitcode yes (exit code 1) } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.152 { -ignoreexitcode yes (exit code 0) } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.153 { -ignoreexitcode no (exit code 100) } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode no \
	    tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.154 { -environ "MYVAR myValue" } {
    list [catch {
	blt::bgexec myVar -session -environ "MYVAR myValue" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.155 { -environ "" } {
    list [catch {
	blt::bgexec myVar -session -environ "" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.156 { -environ "PATH myPath" } {
    list [catch {
	blt::bgexec myVar -session -environ "PATH myPath" \
	    tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.157 { -echo error (see on screen) } {
    list [catch {
	blt::bgexec myVar -session -echo error tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.158 { -echo output (see on screen) } {
    list [catch {
	blt::bgexec myVar -session -echo output tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.159 { -echo both (see on screen) } {
    list [catch {
	blt::bgexec myVar -session -echo both tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.160 { -echo none } {
    list [catch {
	blt::bgexec myVar -session -echo none tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.161 { collect stdout w/ -onoutput } {
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

test bgexec.162 { collect stdout w/ -onoutput -keepnewline } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -session\
	    -keepnewline yes \
	    -onoutput CollectStdout \
	    tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout\n}}

test bgexec.163 { collect stderr w/ -onerror } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -session -onerror CollectStderr \
	    tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.164 { collect stderr w/ -onerror -keepnewline } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -session -onerror CollectStderr -keepnewline yes \
	    tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.165 { pipe both stderr and stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/both.tcl |& tclsh files/cat.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.166 { pipe just stderr w/ |&  } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/stderr.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stderr}}

test bgexec.167 { pipe just stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/stdout.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.168 { redirect input from file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.169 { redirect input badFile } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl < badFile
    } msg] $msg
} {1 {can't read file "badFile": no such file or directory}}

test bgexec.170 { multiple input redirections w/ two files } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/cat.tcl < files/null.tcl < files/null.tcl
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.171 { redirect input w/ missing file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl <
    } msg] $msg
} {1 {can't specify "<" as last word in command}}

test bgexec.172 { redirect input w/ missing literal } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl <<
    } msg] $msg
} {1 {can't specify "<<" as last word in command}}

test bgexec.173 { redirect input w/ missing channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl <@
    } msg] $msg
} {1 {can't specify "<@" as last word in command}}

test bgexec.174 { multiple input redirections /w literal and file } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/cat.tcl < files/null.tcl << "hi"
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.175 { multiple input redirections w/ two literals. } {
    list [catch {
	blt::bgexec myVar -session tclsh files/cat.tcl << "hi" << "there"
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.176 { multiple input redirections w/ literal and channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -session tclsh files/cat.tcl << "hi" <@ $file
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.177 { redirect input /w channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -session tclsh files/cat.tcl <@ $file
    } msg] $msg
} {0 {exit 0}}

test bgexec.178 { redirect input /w channel and pipe } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -session \
	    tclsh files/cat.tcl <@ $file | tclsh files/cat.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.179 {  redirect input from file w/ no command } {
    list [catch {
	blt::bgexec myVar -session < files/null.tcl
    } msg] $msg
} {1 {missing command for "<"}}

test bgexec.180 { redirect input from literal w/ no command } {
    list [catch {
	blt::bgexec myVar -session << "hi"
    } msg] $msg
} {1 {missing command for "<<"}}

test bgexec.181 { redirect input from channel w/ no command } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -session <@ $file
    } msg] $msg
} {1 {missing command for "<@"}}

test bgexec.182 { pipe w/ no commands } {
    list [catch {
	blt::bgexec myVar -session |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.183 { pipe w/ no output command } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.184 {  pipe w/ no input command } {
    list [catch {
	blt::bgexec myVar -session | tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.185 { pipe both |& w/ no commands } {
    list [catch {
	blt::bgexec myVar -session |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.186 { pipe both |& w/ no output command } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.187 { pipe both |& w/ no input command } {
    list [catch {
	blt::bgexec myVar -session |& tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.188 { echo of switches -- } {
    list [catch {
	blt::bgexec myVar -session -- tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.189 { redirect output to no file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl > 
    } msg] $msg
} {1 {can't specify ">" as last word in command}}

test bgexec.190 { redirect output to bad file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl > badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.191 { redirect output to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stdout.tcl > testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.192 { redirect output and append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stdout.tcl > testfile
	blt::bgexec myVar -session tclsh files/stdout.tcl >> testfile
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.193 { redirect output append to no file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl >> 
    } msg] $msg
} {1 {can't specify ">>" as last word in command}}

test bgexec.194 { redirect output append to bad file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl >> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.195 { redirect output append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stdout.tcl >> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.196 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/stdout.tcl > testfile > testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.197 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/stdout.tcl > testfile >> testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.198 { redirect stdout to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/stdout.tcl >@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.199 { redirect stdout to no channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl >@ 
    } msg] $msg
} {1 {can't specify ">@" as last word in command}}

test bgexec.200 { redirect stdout to bad channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl >@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.201 { redirect stdout to file (verify empty string is result) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stdout.tcl > testfile
    } msg] $msg
} {0 {}}

test bgexec.202 { multiple output redirections /w two channels } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/stdout.tcl >@ $f >@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.203 { redirect stderr to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stderr.tcl 2> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.204 { redirect input (literal) and output (file) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/cat.tcl << "testme" > testfile
	file size testfile
    } msg] $msg
} {0 6}

test bgexec.205 { redirect stderr to no file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2> 
    } msg] $msg
} {1 {can't specify "2>" as last word in command}}

test bgexec.206 { redirect stderr to bad file } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/stderr.tcl 2> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.207 { redirect stderr append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stderr.tcl 2> testfile
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>> testfile
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.208 { redirect stderr append to no file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>>
    } msg] $msg
} {1 {can't specify "2>>" as last word in command}}

test bgexec.209 { redirect stderr append to bad file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.210 { redirect stderr append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.211 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/stderr.tcl 2> testfile 2> testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.212 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/stderr.tcl 2> testfile 2>> testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.213 { redirect both stderr and stdout to files } {
    list [catch {
	file delete -force testfile1 testfile2
	blt::bgexec myVar -session \
	    tclsh files/both.tcl 2> testfile1 > testfile2
	expr [file size testfile1] + [file size testfile2]
    } msg] $msg
} {0 30}

test bgexec.214 { redirect both stderr and stdout append to files } {
    list [catch {
	file delete -force testfile1 testfile2
	blt::bgexec myVar -session \
	    tclsh files/both.tcl 2>> testfile1 >> testfile2
	expr [file size testfile1] + [file size testfile2]
    } msg] $msg
} {0 30}

test bgexec.215 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/both.tcl 2> testfile > testfile
	expr [file size testfile]
    } msg] $msg
} {0 15}

test bgexec.216 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/both.tcl 2>> testfile >> testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.217 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/both.tcl >& testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.218 { redirect both stderr and stdout to no file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl >& 
    } msg] $msg
} {1 {can't specify ">&" as last word in command}}

test bgexec.219 { redirect both stderr and stdout to bad file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl >& badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.220 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/both.tcl >>& testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.221 { redirect stderr to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.222 { redirect stderr to no channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>@ 
    } msg] $msg
} {1 {can't specify "2>@" as last word in command}}

test bgexec.223 { redirect stderr to bad channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.224 { redirect stderr to file (verify empty string is result) } {
    list [catch {
	file delete -force testfile
	set myErr {}
	blt::bgexec myVar -session -errorvariable myErr \
	    tclsh files/stderr.tcl 2> testfile 
	set myErr
    } msg] $msg
} {0 {}}

test bgexec.225 { multiple stderr redirections /w two channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/stdout.tcl 2>@ $f 2>@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.226 { redirect both stderr and stdout to one channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/both.tcl >&@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.227 { redirect both stderr and stdout to no channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl >&@ 
    } msg] $msg
} {1 {can't specify ">&@" as last word in command}}

test bgexec.228 { multiple stderr/stdout redirections /w channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/both.tcl >&@ $f >&@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.229 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/both.tcl >&@ $f > testfile
	close $f
	file size tetfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.230 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/both.tcl >&@ $f 2> testfile
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.231 { redirect both stderr and stdout  } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl 2>@1
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.232 { redirect both stderr and stdout /w extraArg } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl 2>@1 extraArg
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {1 {must specify "2>@1" as last word in command}}

test bgexec.233 { kill by setting status variable } {
    list [catch {
	set myVar -1
	set myOut {}
	blt::bgexec myVar -session -outputvariable myOut \
	    tclsh files/sleep.tcl 4000 &
	after 100
	set myVar die
	set myOut
    } msg] $msg
} {0 {}}


test bgexec.234 {bgexec -tty kill with myVar } {
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

test bgexec.235 {bgexec -tty kill grandchildren with myVar } {
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

########################################################################
#	tty tests
########################################################################

test bgexec.236 { redirect input from file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.237 { redirect input from literal } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.238 { redirect input from channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -tty tclsh files/cat.tcl <@ $file
    } msg] $msg
} {0 {exit 0}}


test bgexec.239 { bad redirect syntax is considered arg } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -tty tclsh files/cat.tcl @< $file
    } msg] $msg
} {1 {child process exited abnormally}}

test bgexec.240 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar -tty \
	    -keepnewline yes  \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.241 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar -tty \
	    -keepnewline yes \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0\n}}

test bgexec.242 { collect stderr w/ -errorvariable } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.243 { collect stderr w/ -errorvariable -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr -keepnewline yes \
	    tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.244 { collect stdout w/ -outputvariable } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -tty -outputvariable myOut \
	    tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}


test bgexec.245 { collect stdout /w -updatevariable } {
    list [catch {
	set myOut {}
	trace variable myUpdateVar w CollectStdout
	proc CollectStdout { name1 name2 how } {
	    global myOut myUpdateVar
	    append myOut $myUpdateVar
	    set myUpdateVar {}
	}
	blt::bgexec myVar -tty -updatevariable myUpdateVar \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.246 { collect stdout (command result) } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.247 { pipe output } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.248 { command returns exit code 0 } {
    set code [catch {
	blt::bgexec myVar -tty tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.249 { command returns exit code 1 } {
    set code [catch {
	blt::bgexec myVar -tty tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.250 { command returns exit code 100 } {
    set code [catch {
	blt::bgexec myVar -tty tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.251 { command returns exit code -1 } {
    set code [catch {
	blt::bgexec myVar -tty tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.252 { multiple pipes } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl | tclsh files/cat.tcl | tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.253 { -ignoreexitcode yes (exit code 1) } {
    set code [catch {
	blt::bgexec myVar -tty -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.254 { -ignoreexitcode yes (exit code 0) } {
    set code [catch {
	blt::bgexec myVar -tty -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.255 { -ignoreexitcode no (exit code 100) } {
    set code [catch {
	blt::bgexec myVar -tty -ignoreexitcode no \
	    tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.256 { -environ "MYVAR myValue" } {
    list [catch {
	blt::bgexec myVar -tty -environ "MYVAR myValue" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.257 { -environ "" } {
    list [catch {
	blt::bgexec myVar -tty -environ "" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.258 { -environ "PATH myPath" } {
    list [catch {
	blt::bgexec myVar -tty -environ "PATH myPath" \
	    tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.259 { -echo error (see on screen) } {
    list [catch {
	blt::bgexec myVar -tty -echo error tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.260 { -echo output (see on screen) } {
    list [catch {
	blt::bgexec myVar -tty -echo output tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.261 { -echo both (see on screen) } {
    list [catch {
	blt::bgexec myVar -tty -echo both tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.262 { -echo none } {
    list [catch {
	blt::bgexec myVar -tty -echo none tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.263 { collect stdout w/ -onoutput } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -tty -onoutput CollectStdout \
	    tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.264 { collect stdout w/ -onoutput -keepnewline } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -tty\
	    -keepnewline yes \
	    -onoutput CollectStdout \
	    tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout\n}}

test bgexec.265 { collect stderr w/ -onerror } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -tty -onerror CollectStderr \
	    tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.266 { collect stderr w/ -onerror -keepnewline } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -tty -onerror CollectStderr -keepnewline yes \
	    tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.267 { pipe both stderr and stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl |& tclsh files/cat.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.268 { pipe just stderr w/ |&  } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stderr}}

test bgexec.269 { pipe just stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.270 { redirect input from file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.271 { redirect input badFile } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl < badFile
    } msg] $msg
} {1 {can't read file "badFile": no such file or directory}}

test bgexec.272 { multiple input redirections w/ two files } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/cat.tcl < files/null.tcl < files/null.tcl
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.273 { redirect input w/ missing file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl <
    } msg] $msg
} {1 {can't specify "<" as last word in command}}

test bgexec.274 { redirect input w/ missing literal } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl <<
    } msg] $msg
} {1 {can't specify "<<" as last word in command}}

test bgexec.275 { redirect input w/ missing channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl <@
    } msg] $msg
} {1 {can't specify "<@" as last word in command}}

test bgexec.276 { multiple input redirections /w literal and file } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/cat.tcl < files/null.tcl << "hi"
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.277 { multiple input redirections w/ two literals. } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl << "hi" << "there"
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.278 { multiple input redirections w/ literal and channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -tty tclsh files/cat.tcl << "hi" <@ $file
    } msg] $msg
} {1 {ambigious input redirect.}}

test bgexec.279 { redirect input /w channel } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -tty tclsh files/cat.tcl <@ $file
    } msg] $msg
} {0 {exit 0}}

test bgexec.280 { redirect input /w channel and pipe } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -tty \
	    tclsh files/cat.tcl <@ $file | tclsh files/cat.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.281 {  redirect input from file w/ no command } {
    list [catch {
	blt::bgexec myVar -tty < files/null.tcl
    } msg] $msg
} {1 {missing command for "<"}}

test bgexec.282 { redirect input from literal w/ no command } {
    list [catch {
	blt::bgexec myVar -tty << "hi"
    } msg] $msg
} {1 {missing command for "<<"}}

test bgexec.283 { redirect input from channel w/ no command } {
    list [catch {
	seek $file 0
	blt::bgexec myVar -tty <@ $file
    } msg] $msg
} {1 {missing command for "<@"}}

test bgexec.284 { pipe w/ no commands } {
    list [catch {
	blt::bgexec myVar -tty |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.285 { pipe w/ no output command } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.286 {  pipe w/ no input command } {
    list [catch {
	blt::bgexec myVar -tty | tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.287 { pipe both |& w/ no commands } {
    list [catch {
	blt::bgexec myVar -tty |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.288 { pipe both |& w/ no output command } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.289 { pipe both |& w/ no input command } {
    list [catch {
	blt::bgexec myVar -tty |& tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.290 { echo of switches -- } {
    list [catch {
	blt::bgexec myVar -tty -- tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.291 { redirect output to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl > 
    } msg] $msg
} {1 {can't specify ">" as last word in command}}

test bgexec.292 { redirect output to bad file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl > badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.293 { redirect output to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl > testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.294 { redirect output and append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl > testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl >> testfile
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.295 { redirect output append to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl >> 
    } msg] $msg
} {1 {can't specify ">>" as last word in command}}

test bgexec.296 { redirect output append to bad file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl >> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.297 { redirect output append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl >> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.298 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl > testfile > testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.299 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl > testfile >> testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.300 { redirect stdout to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/stdout.tcl >@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.301 { redirect stdout to no channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl >@ 
    } msg] $msg
} {1 {can't specify ">@" as last word in command}}

test bgexec.302 { redirect stdout to bad channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl >@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.303 { redirect stdout to file (verify empty string is result) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl > testfile
    } msg] $msg
} {0 {}}

test bgexec.304 { multiple output redirections /w two channels } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/stdout.tcl >@ $f >@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.305 { redirect stderr to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.306 { redirect input (literal) and output (file) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/cat.tcl << "testme" > testfile
	file size testfile
    } msg] $msg
} {0 6}

test bgexec.307 { redirect stderr to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2> 
    } msg] $msg
} {1 {can't specify "2>" as last word in command}}

test bgexec.308 { redirect stderr to bad file } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl 2> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.309 { redirect stderr append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2> testfile
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>> testfile
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.310 { redirect stderr append to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>>
    } msg] $msg
} {1 {can't specify "2>>" as last word in command}}

test bgexec.311 { redirect stderr append to bad file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.312 { redirect stderr append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>> testfile
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.313 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl 2> testfile 2> testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.314 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl 2> testfile 2>> testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.315 { redirect both stderr and stdout to files } {
    list [catch {
	file delete -force testfile1 testfile2
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl 2> testfile1 > testfile2
	expr [file size testfile1] + [file size testfile2]
    } msg] $msg
} {0 30}

test bgexec.316 { redirect both stderr and stdout append to files } {
    list [catch {
	file delete -force testfile1 testfile2
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl 2>> testfile1 >> testfile2
	expr [file size testfile1] + [file size testfile2]
    } msg] $msg
} {0 30}

test bgexec.317 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl 2> testfile > testfile
	expr [file size testfile]
    } msg] $msg
} {0 15}

test bgexec.318 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl 2>> testfile >> testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.319 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/both.tcl >& testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.320 { redirect both stderr and stdout to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl >& 
    } msg] $msg
} {1 {can't specify ">&" as last word in command}}

test bgexec.321 { redirect both stderr and stdout to bad file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl >& badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.322 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/both.tcl >>& testfile
	expr [file size testfile]
    } msg] $msg
} {0 30}

test bgexec.323 { redirect stderr to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 15}

test bgexec.324 { redirect stderr to no channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>@ 
    } msg] $msg
} {1 {can't specify "2>@" as last word in command}}

test bgexec.325 { redirect stderr to bad channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.326 { redirect stderr to file (verify empty string is result) } {
    list [catch {
	file delete -force testfile
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr \
	    tclsh files/stderr.tcl 2> testfile 
	set myErr
    } msg] $msg
} {0 {}}

test bgexec.327 { multiple stderr redirections /w two channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/stdout.tcl 2>@ $f 2>@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.328 { redirect both stderr and stdout to one channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ $f
	close $f
	file size testfile
    } msg] $msg
} {0 30}

test bgexec.329 { redirect both stderr and stdout to no channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ 
    } msg] $msg
} {1 {can't specify ">&@" as last word in command}}

test bgexec.330 { multiple stderr/stdout redirections /w channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ $f >&@ $f
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.331 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ $f > testfile
	close $f
	file size tetfile
    } msg] $msg
} {1 {ambigious output redirect.}}

test bgexec.332 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ $f 2> testfile
	close $f
	file size testfile
    } msg] $msg
} {1 {ambigious error redirect.}}

test bgexec.333 { redirect both stderr and stdout  } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl 2>@1
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.334 { redirect both stderr and stdout /w extraArg } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl 2>@1 extraArg
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {1 {must specify "2>@1" as last word in command}}

test bgexec.335 { kill by setting status variable } {
    list [catch {
	set myVar -1
	set myOut {}
	blt::bgexec myVar -tty -outputvariable myOut \
	    tclsh files/sleep.tcl 4000 &
	after 100
	set myVar die
	set myOut
    } msg] $msg
} {0 {}}


test bgexec.336 {bgexec -tty kill with myVar } {
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

test bgexec.337 {bgexec -tty kill grandchildren with myVar } {
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

test bgexec.338 { kill with myVar } {
    list [catch {
	set myVar -1
	set myOut {}
	blt::bgexec myVar -tty -outputvariable myOut \
	    tclsh files/sleep.tcl 10000 &
	after 100
	set myVar die  
	set myOut
    } msg] $msg
} {0 {}}

test bgexec.339 { kill grandchildren with myVar } {
    list [catch {
	set myVar {}
	set myOut {}
	blt::bgexec ::myVar -tty -outputvariable myOut \
	    /bin/sh files/children.sh &
	after 100
	set myVar die
	set myOut
    } msg] $msg
} {0 {}}

##################

test bgexec.340 { cleanup } {
    list [catch {
	file delete -force testfile testfile1 testfile2
	close $file
    } msg] $msg
} {0 {}}


exit 0








