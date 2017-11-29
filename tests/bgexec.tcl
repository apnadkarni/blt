package require BLT

if {[info procs test] != "test"} {
    source defs
}

set tclsh tclsh86

if [file exists ../library] {
    set blt_library ../library
}


proc ReadAndDeleteFile { fileName } {
    set f [open $fileName "r"]
    set contents [read $f]
    close $f
    file delete -force $fileName
    return $contents
}

#set VERBOSE 1

test bgexec.1 {set up file channel } {
    list [catch {
	set channel [open files/null.tcl "r"]
	string match "file*" $channel
    } msg] $msg
} {0 1}

test bgexec.2 {bgexec (no args) } {
    list [catch {blt::bgexec} msg] $msg
} {1 {wrong # args: should be "blt::bgexec varName ?options? command ?arg...?"}}

test bgexec.3 {bgexec var (no args) } {
    list [catch {blt::bgexec myVar} msg] $msg
} {1 {wrong # args: should be "blt::bgexec varName ?options? command ?arg...?"}}

test bgexec.4 {bgexec var tclsh files/null.tcl} {
    list [catch {blt::bgexec myVar $tclsh files/null.tcl} msg] $msg
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

test bgexec.34 { null } {
    list [catch {
	blt::bgexec myVar $tclsh files/null.tcl
    } msg] $msg
} {0 {}}

test bgexec.35 { stderr } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl
    } msg] $msg
} {0 {}}

test bgexec.36 { stderr with collection } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -errorvariable myErr $tclsh files/stderr.tcl
        set msg $myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.37 { stdout } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.38 { redirect input from file } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.39 { redirect input from literal } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.40 { redirect input from channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar $tclsh files/cat.tcl <@ $channel
    } msg] $msg
} {0 {exit 0}}


test bgexec.41 { bad redirect syntax is considered arg } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar $tclsh files/cat.tcl @< $channel
    } msg] $msg
} {1 {child process exited abnormally}}

test bgexec.42 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline yes  \
	    $tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.43 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar \
	    -keepnewline yes \
	    $tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0\n}}

test bgexec.44 { collect stderr w/ -errorvariable } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -errorvariable myErr $tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.45 { collect stderr w/ -errorvariable -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -errorvariable myErr -keepnewline yes \
	    $tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.46 { collect stdout w/ -outputvariable } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -outputvariable myOut $tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}


test bgexec.47 { collect stdout /w -updatevariable } {
    list [catch {
	set myOut {}
	trace variable myUpdateVar w CollectStdout
	proc CollectStdout { name1 name2 how } {
	    global myOut myUpdateVar
	    append myOut $myUpdateVar
	    set myUpdateVar {}
	}
	blt::bgexec myVar -updatevariable myUpdateVar $tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.48 { collect stdout (command result) } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.49 { pipe output } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl | $tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.50 { command returns exit code 0 } {
    set code [catch {
	blt::bgexec myVar $tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.51 { command returns exit code 1 } {
    set code [catch {
	blt::bgexec myVar $tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.52 { command returns exit code 100 } {
    set code [catch {
	blt::bgexec myVar $tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.53 { command returns exit code -1 } {
    set code [catch {
	blt::bgexec myVar $tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.54 { multiple pipes } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl | $tclsh files/cat.tcl | $tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.55 { -ignoreexitcode yes (exit code 1) } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode yes \
	    $tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.56 { -ignoreexitcode yes (exit code 0) } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode yes \
	    $tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.57 { -ignoreexitcode no (exit code 100) } {
    set code [catch {
	blt::bgexec myVar -ignoreexitcode no \
	    $tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.58 { -environ "MYVAR myValue" } {
    list [catch {
	blt::bgexec myVar -environ "MYVAR myValue" \
	    $tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.59 { -environ "" } {
    list [catch {
	blt::bgexec myVar -environ "" \
	    $tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.60 { -environ "PATH myPath" } {
    list [catch {
	blt::bgexec myVar -environ "PATH myPath" \
	    $tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.61 { -echo error (see on screen) } {
    list [catch {
	blt::bgexec myVar -echo error $tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.62 { -echo output (see on screen) } {
    list [catch {
	blt::bgexec myVar -echo output $tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.63 { -echo both (see on screen) } {
    list [catch {
	blt::bgexec myVar -echo both $tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.64 { -echo none } {
    list [catch {
	blt::bgexec myVar -echo none $tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.65 { collect stdout w/ -onoutput } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -onoutput CollectStdout $tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.66 { collect stdout w/ -onoutput -keepnewline } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar\
	    -keepnewline yes \
	    -onoutput CollectStdout \
	    $tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout\n}}

test bgexec.67 { collect stderr w/ -onerror } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -onerror CollectStderr $tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.68 { collect stderr w/ -onerror -keepnewline } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar \
	    -onerror CollectStderr \
	    -keepnewline yes \
	    $tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.69 { pipe both stderr and stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar \
	    $tclsh files/both.tcl |& $tclsh files/cat.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.70 { pipe just stderr w/ |&  } {
    list [catch {
	blt::bgexec myVar \
	    $tclsh files/stderr.tcl |& $tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stderr}}

test bgexec.71 { pipe just stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar \
	    $tclsh files/stdout.tcl |& $tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.72 { redirect input from file } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.73 { redirect input badFile } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl < badFile
    } msg] $msg
} {1 {can't read file "badFile": no such file or directory}}

test bgexec.74 { multiple input redirections w/ two files } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl < files/null.tcl < files/null.tcl
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.75 { redirect input w/ missing file } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl <
    } msg] $msg
} {1 {can't specify "<" as last word in command}}

test bgexec.76 { redirect input w/ missing literal } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl <<
    } msg] $msg
} {1 {can't specify "<<" as last word in command}}

test bgexec.77 { redirect input w/ missing channel } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl <@
    } msg] $msg
} {1 {can't specify "<@" as last word in command}}

test bgexec.78 { multiple input redirections /w literal and file } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl < files/null.tcl << "hi"
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.79 { multiple input redirections w/ two literals. } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl << "hi" << "there"
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.80 { multiple input redirections w/ literal and channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar $tclsh files/cat.tcl << "hi" <@ $channel
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.81 { redirect input /w channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar $tclsh files/cat.tcl <@ $channel
    } msg] $msg
} {0 {exit 0}}

test bgexec.82 { redirect input /w channel and pipe } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar $tclsh files/cat.tcl <@ $channel | $tclsh files/cat.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.83 {  redirect input from file w/ no command } {
    list [catch {
	blt::bgexec myVar < files/null.tcl
    } msg] $msg
} {1 {missing command for "<"}}

test bgexec.84 { redirect input from literal w/ no command } {
    list [catch {
	blt::bgexec myVar << "hi"
    } msg] $msg
} {1 {missing command for "<<"}}

test bgexec.85 { redirect input from channel w/ no command } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar <@ $channel
    } msg] $msg
} {1 {missing command for "<@"}}

test bgexec.86 { pipe w/ no commands } {
    list [catch {
	blt::bgexec myVar |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.87 { pipe w/ no output command } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.88 {  pipe w/ no input command } {
    list [catch {
	blt::bgexec myVar | $tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.89 { pipe both |& w/ no commands } {
    list [catch {
	blt::bgexec myVar |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.90 { pipe both |& w/ no output command } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.91 { pipe both |& w/ no input command } {
    list [catch {
	blt::bgexec myVar |& $tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.92 { echo of switches -- } {
    list [catch {
	blt::bgexec myVar -- $tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.93 { redirect output to no file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl > 
    } msg] $msg
} {1 {can't specify ">" as last word in command}}

test bgexec.94 { redirect output to bad file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl > badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.95 { redirect output to file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}


test bgexec.96 { redirect output and append to file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl > testfile
	blt::bgexec myVar $tclsh files/stdout.tcl >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
This is stdout
}}

test bgexec.97 { redirect output append to no file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl >> 
    } msg] $msg
} {1 {can't specify ">>" as last word in command}}

test bgexec.98 { redirect output append to bad file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl >> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.99 { redirect output append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar $tclsh files/stdout.tcl >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.100 { multiple output redirections w/ two files } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl > testfile > testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.101 { multiple output redirections w/ two files } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl > testfile >> testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.102 { redirect stdout to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar $tclsh files/stdout.tcl >@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.103 { redirect stdout to no channel } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl >@ 
    } msg] $msg
} {1 {can't specify ">@" as last word in command}}

test bgexec.104 { redirect stdout to bad channel } {
    list [catch {
	blt::bgexec myVar $tclsh files/stdout.tcl >@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.105 { redirect stdout to file (verify empty string is result) } {
    list [catch {
	set out1 [blt::bgexec myVar $tclsh files/stdout.tcl > testfile]
	set out2 [ReadAndDeleteFile testfile]
	list $out1 $out2
    } msg] $msg
} {0 {{} {This is stdout
}}}

test bgexec.106 { multiple output redirections /w two channels } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar $tclsh files/stdout.tcl >@ $f >@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}
close $f
test bgexec.107 { redirect stderr to file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}


test bgexec.108 { redirect input (literal) and output (file) } {
    list [catch {
	blt::bgexec myVar $tclsh files/cat.tcl << "testme" > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 testme}

test bgexec.109 { redirect stderr to no file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2> 
    } msg] $msg
} {1 {can't specify "2>" as last word in command}}

test bgexec.110 { redirect stderr to bad file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.111 { redirect stderr append to file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2> testfile
	blt::bgexec myVar $tclsh files/stderr.tcl 2>> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stderr
}}

test bgexec.112 { redirect stderr append to no file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2>>
    } msg] $msg
} {1 {can't specify "2>>" as last word in command}}

test bgexec.113 { redirect stderr append to bad file } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2>> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.114 { redirect stderr append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar $tclsh files/stderr.tcl 2>> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.115 { multiple stderr redirections w/ two files } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2> testfile 2> testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.116 { multiple stderr redirections w/ two files } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2> testfile 2>> testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.117 { redirect both stderr and stdout to files } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl 2> testfile1 > testfile2
	set first [ReadAndDeleteFile testfile1]
	set second [ReadAndDeleteFile testfile2]
	list $first $second
    } msg] $msg
} {0 {{This is stderr
} {This is stdout
}}}

test bgexec.118 { redirect both stderr and stdout append to files } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl 2>> testfile1 >> testfile2
	set first [ReadAndDeleteFile testfile1]
	set second [ReadAndDeleteFile testfile2]
	list $first $second
    } msg] $msg
} {0 {{This is stderr
} {This is stdout
}}}


test bgexec.119 { redirect both stderr and stdout to one file } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl 2> testfile > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}


test bgexec.120 { redirect both stderr and stdout appending to one file } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl 2>> testfile >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}


test bgexec.121 { redirect both stderr and stdout to one file } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl >& testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.122 { redirect both stderr and stdout to no file } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl >& 
    } msg] $msg
} {1 {can't specify ">&" as last word in command}}

test bgexec.123 { redirect both stderr and stdout to bad file } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl >& badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.124 { redirect both stderr and stdout appending to one file } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl >>& testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.125 { redirect stderr to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar $tclsh files/stderr.tcl 2>@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}


test bgexec.126 { redirect stderr to no channel } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2>@ 
    } msg] $msg
} {1 {can't specify "2>@" as last word in command}}

test bgexec.127 { redirect stderr to bad channel } {
    list [catch {
	blt::bgexec myVar $tclsh files/stderr.tcl 2>@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.128 { redirect stderr to file (verify empty string is result) } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -errorvariable myErr \
	    $tclsh files/stderr.tcl 2> testfile 
	set err [ReadAndDeleteFile testfile]
	list $myErr $err
    } msg] $msg
} {0 {{} {This is stderr
}}}

test bgexec.129 { multiple stderr redirections /w two channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar $tclsh files/stdout.tcl 2>@ $f 2>@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.130 { redirect both stderr and stdout to one channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar $tclsh files/both.tcl >&@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.131 { redirect both stderr and stdout to no channel } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl >&@ 
    } msg] $msg
} {1 {can't specify ">&@" as last word in command}}

test bgexec.132 { multiple stderr/stdout redirections /w channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar $tclsh files/both.tcl >&@ $f >&@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.133 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar $tclsh files/both.tcl >&@ $f > testfile
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.134 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar  $tclsh files/both.tcl >&@ $f 2> testfile
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.135 { redirect both stderr and stdout  } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl 2>@1
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.136 { redirect both stderr and stdout /w extraArg } {
    list [catch {
	blt::bgexec myVar $tclsh files/both.tcl 2>@1 extraArg
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {1 {must specify "2>@1" as last word in command}}

test bgexec.137 { kill by setting status variable } {
    list [catch {
	set myVar -1
	set myOut {}
	blt::bgexec ::myVar -outputvariable myOut \
	    $tclsh files/sleep.tcl 4000 &
	after 100
	set myVar die
	set myOut
    } msg] $msg
} {0 {}}


if { $tcl_platform(platform) == "windows" } {
    exit 0
}
########################################################################
#	Session tests
########################################################################


test bgexec.138 { redirect input from file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.139 { redirect input from literal } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.140 { redirect input from channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -session $tclsh files/cat.tcl <@ $channel
    } msg] $msg
} {0 {exit 0}}


test bgexec.141 { bad redirect syntax is considered arg } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -session $tclsh files/cat.tcl @< $channel
    } msg] $msg
} {1 {child process exited abnormally}}

test bgexec.142 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar -session \
	    -keepnewline yes  \
	    $tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.143 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar -session \
	    -keepnewline yes \
	    $tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0\n}}

test bgexec.144 { collect stderr w/ -errorvariable } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -session -errorvariable myErr $tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.145 { collect stderr w/ -errorvariable -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -session -errorvariable myErr -keepnewline yes \
	    $tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.146 { collect stdout w/ -outputvariable } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -session -outputvariable myOut \
	    $tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}


test bgexec.147 { collect stdout /w -updatevariable } {
    list [catch {
	set myOut {}
	trace variable myUpdateVar w CollectStdout
	proc CollectStdout { name1 name2 how } {
	    global myOut myUpdateVar
	    append myOut $myUpdateVar
	    set myUpdateVar {}
	}
	blt::bgexec myVar -session -updatevariable myUpdateVar \
	    $tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.148 { collect stdout (command result) } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.149 { pipe output } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl | $tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.150 { command returns exit code 0 } {
    set code [catch {
	blt::bgexec myVar -session $tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.151 { command returns exit code 1 } {
    set code [catch {
	blt::bgexec myVar -session $tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.152 { command returns exit code 100 } {
    set code [catch {
	blt::bgexec myVar -session $tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.153 { command returns exit code -1 } {
    set code [catch {
	blt::bgexec myVar -session $tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.154 { multiple pipes } {
    list [catch {
	blt::bgexec myVar -session \
	    $tclsh files/stdout.tcl | $tclsh files/cat.tcl | $tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.155 { -ignoreexitcode yes (exit code 1) } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode yes \
	    $tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.156 { -ignoreexitcode yes (exit code 0) } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode yes \
	    $tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.157 { -ignoreexitcode no (exit code 100) } {
    set code [catch {
	blt::bgexec myVar -session -ignoreexitcode no \
	    $tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.158 { -environ "MYVAR myValue" } {
    list [catch {
	blt::bgexec myVar -session -environ "MYVAR myValue" \
	    $tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.159 { -environ "" } {
    list [catch {
	blt::bgexec myVar -session -environ "" \
	    $tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.160 { -environ "PATH myPath" } {
    list [catch {
	blt::bgexec myVar -session -environ "PATH myPath" \
	    $tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.161 { -echo error (see on screen) } {
    list [catch {
	blt::bgexec myVar -session -echo error $tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.162 { -echo output (see on screen) } {
    list [catch {
	blt::bgexec myVar -session -echo output $tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.163 { -echo both (see on screen) } {
    list [catch {
	blt::bgexec myVar -session -echo both $tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.164 { -echo none } {
    list [catch {
	blt::bgexec myVar -session -echo none $tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.165 { collect stdout w/ -onoutput } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -session -onoutput CollectStdout \
	    $tclsh files/stdout.tcl
	set myOut
    } msg] $msg
} {0 {This is stdout}}

test bgexec.166 { collect stdout w/ -onoutput -keepnewline } {
    list [catch {
	set myOut {}
	proc CollectStdout { data } {
	    global myOut
	    append myOut $data
	}
	blt::bgexec myVar -session\
	    -keepnewline yes \
	    -onoutput CollectStdout \
	    $tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout\n}}

test bgexec.167 { collect stderr w/ -onerror } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -session -onerror CollectStderr \
	    $tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.168 { collect stderr w/ -onerror -keepnewline } {
    list [catch {
	set myErr {}
	proc CollectStderr { data } {
	    global myErr
	    append myErr $data
	}
	blt::bgexec myVar -session -onerror CollectStderr -keepnewline yes \
	    $tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.169 { pipe both stderr and stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar -session \
	    $tclsh files/both.tcl |& $tclsh files/cat.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.170 { pipe just stderr w/ |&  } {
    list [catch {
	blt::bgexec myVar -session \
	    $tclsh files/stderr.tcl |& $tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stderr}}

test bgexec.171 { pipe just stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar -session \
	    $tclsh files/stdout.tcl |& $tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.172 { redirect input from file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.173 { redirect input badFile } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/cat.tcl < badFile
    } msg] $msg
} {1 {can't read file "badFile": no such file or directory}}

test bgexec.174 { multiple input redirections w/ two files } {
    list [catch {
	blt::bgexec myVar -session \
	    $tclsh files/cat.tcl < files/null.tcl < files/null.tcl
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.175 { redirect input w/ missing file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/cat.tcl <
    } msg] $msg
} {1 {can't specify "<" as last word in command}}

test bgexec.176 { redirect input w/ missing literal } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/cat.tcl <<
    } msg] $msg
} {1 {can't specify "<<" as last word in command}}

test bgexec.177 { redirect input w/ missing channel } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/cat.tcl <@
    } msg] $msg
} {1 {can't specify "<@" as last word in command}}

test bgexec.178 { multiple input redirections /w literal and file } {
    list [catch {
	blt::bgexec myVar -session \
	    $tclsh files/cat.tcl < files/null.tcl << "hi"
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.179 { multiple input redirections w/ two literals. } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/cat.tcl << "hi" << "there"
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.180 { multiple input redirections w/ literal and channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -session $tclsh files/cat.tcl << "hi" <@ $channel
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.181 { redirect input /w channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -session $tclsh files/cat.tcl <@ $channel
    } msg] $msg
} {0 {exit 0}}

test bgexec.182 { redirect input /w channel and pipe } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -session \
	    $tclsh files/cat.tcl <@ $channel | $tclsh files/cat.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.183 {  redirect input from file w/ no command } {
    list [catch {
	blt::bgexec myVar -session < files/null.tcl
    } msg] $msg
} {1 {missing command for "<"}}

test bgexec.184 { redirect input from literal w/ no command } {
    list [catch {
	blt::bgexec myVar -session << "hi"
    } msg] $msg
} {1 {missing command for "<<"}}

test bgexec.185 { redirect input from channel w/ no command } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -session <@ $channel
    } msg] $msg
} {1 {missing command for "<@"}}

test bgexec.186 { pipe w/ no commands } {
    list [catch {
	blt::bgexec myVar -session |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.187 { pipe w/ no output command } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.188 {  pipe w/ no input command } {
    list [catch {
	blt::bgexec myVar -session | $tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.189 { pipe both |& w/ no commands } {
    list [catch {
	blt::bgexec myVar -session |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.190 { pipe both |& w/ no output command } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.191 { pipe both |& w/ no input command } {
    list [catch {
	blt::bgexec myVar -session |& $tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.192 { echo of switches -- } {
    list [catch {
	blt::bgexec myVar -session -- $tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.193 { redirect output to no file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl > 
    } msg] $msg
} {1 {can't specify ">" as last word in command}}

test bgexec.194 { redirect output to bad file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl > badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.195 { redirect output to file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.196 { redirect output and append to file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl > testfile
	blt::bgexec myVar -session $tclsh files/stdout.tcl >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
This is stdout
}}

test bgexec.197 { redirect output append to no file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl >> 
    } msg] $msg
} {1 {can't specify ">>" as last word in command}}

test bgexec.198 { redirect output append to bad file } {
    list [catch {
	blt::bgexec myVar -session $tclsh files/stdout.tcl >> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.199 { redirect output append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session $tclsh files/stdout.tcl >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.200 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/stdout.tcl > testfile > testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.201 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/stdout.tcl > testfile >> testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.202 { redirect stdout to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/stdout.tcl >@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.203 { redirect stdout to no channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl >@ 
    } msg] $msg
} {1 {can't specify ">@" as last word in command}}

test bgexec.204 { redirect stdout to bad channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stdout.tcl >@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.205 { redirect stdout to file (verify empty string is result) } {
    list [catch {
	set out1 [blt::bgexec myVar -session tclsh files/stdout.tcl > testfile]
	set out2 [ReadAndDeleteFile testfile]
	list $out1 $out2
    } msg] $msg
} {0 {{} {This is stdout
}}}

test bgexec.206 { multiple output redirections /w two channels } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/stdout.tcl >@ $f >@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.207 { redirect stderr to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stderr.tcl 2> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.208 { redirect input (literal) and output (file) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/cat.tcl << "testme" > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 testme}

test bgexec.209 { redirect stderr to no file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2> 
    } msg] $msg
} {1 {can't specify "2>" as last word in command}}

test bgexec.210 { redirect stderr to bad file } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/stderr.tcl 2> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.211 { redirect stderr append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stderr.tcl 2> testfile
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stderr
}}

test bgexec.212 { redirect stderr append to no file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>>
    } msg] $msg
} {1 {can't specify "2>>" as last word in command}}

test bgexec.213 { redirect stderr append to bad file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.214 { redirect stderr append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.215 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/stderr.tcl 2> testfile 2> testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.216 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/stderr.tcl 2> testfile 2>> testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.217 { redirect both stderr and stdout to files } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/both.tcl 2> testfile1 > testfile2
	set first [ReadAndDeleteFile testfile1]
	set second [ReadAndDeleteFile testfile2]
	list $first $second
    } msg] $msg
} {0 {{This is stderr
} {This is stdout
}}}

test bgexec.218 { redirect both stderr and stdout append to files } {
    list [catch {
	blt::bgexec myVar -session \
	    tclsh files/both.tcl 2>> testfile1 >> testfile2
	set first [ReadAndDeleteFile testfile1]
	set second [ReadAndDeleteFile testfile2]
	list $first $second
    } msg] $msg
} {0 {{This is stderr
} {This is stdout
}}}

test bgexec.219 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/both.tcl 2> testfile > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.220 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session \
	    tclsh files/both.tcl 2>> testfile >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}


test bgexec.221 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/both.tcl >& testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.222 { redirect both stderr and stdout to no file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl >& 
    } msg] $msg
} {1 {can't specify ">&" as last word in command}}

test bgexec.223 { redirect both stderr and stdout to bad file } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl >& badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.224 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -session tclsh files/both.tcl >>& testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.225 { redirect stderr to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.226 { redirect stderr to no channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>@ 
    } msg] $msg
} {1 {can't specify "2>@" as last word in command}}

test bgexec.227 { redirect stderr to bad channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/stderr.tcl 2>@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.228 { redirect stderr to file (verify empty string is result) } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -session -errorvariable myErr \
	    tclsh files/stderr.tcl 2> testfile 
	set err [ReadAndDeleteFile testfile]
	list $myErr $err
    } msg] $msg
} {0 {{} {This is stderr
}}}

test bgexec.229 { multiple stderr redirections /w two channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/stdout.tcl 2>@ $f 2>@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.230 { redirect both stderr and stdout to one channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/both.tcl >&@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.231 { redirect both stderr and stdout to no channel } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl >&@ 
    } msg] $msg
} {1 {can't specify ">&@" as last word in command}}

test bgexec.232 { multiple stderr/stdout redirections /w channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/both.tcl >&@ $f >&@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.233 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/both.tcl >&@ $f > testfile
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.234 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -session tclsh files/both.tcl >&@ $f 2> testfile
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.235 { redirect both stderr and stdout  } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl 2>@1
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.236 { redirect both stderr and stdout /w extraArg } {
    list [catch {
	blt::bgexec myVar -session tclsh files/both.tcl 2>@1 extraArg
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {1 {must specify "2>@1" as last word in command}}

test bgexec.237 { kill by setting status variable } {
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


test bgexec.238 {bgexec -tty kill with myVar } {
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

test bgexec.239 {bgexec -tty kill grandchildren with myVar } {
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

test bgexec.240 { redirect input from file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.241 { redirect input from literal } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl << "test me"
    } msg] $msg
} {0 {test me}}

test bgexec.242 { redirect input from channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -tty tclsh files/cat.tcl <@ $channel
    } msg] $msg
} {0 {exit 0}}


test bgexec.243 { bad redirect syntax is considered arg } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -tty tclsh files/cat.tcl @< $channel
    } msg] $msg
} {1 {child process exited abnormally}}

test bgexec.244 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar -tty \
	    -keepnewline yes  \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0
}}

test bgexec.245 { -keepnewline yes } {
    list [catch {
	blt::bgexec myVar -tty \
	    -keepnewline yes \
	    tclsh files/cat.tcl < files/null.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {exit 0\n}}

test bgexec.246 { collect stderr w/ -errorvariable } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr tclsh files/stderr.tcl
	set myErr
    } msg] $msg
} {0 {This is stderr}}

test bgexec.247 { collect stderr w/ -errorvariable -keepnewline } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr -keepnewline yes \
	    tclsh files/stderr.tcl
	set myErr
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\n}}

test bgexec.248 { collect stdout w/ -outputvariable } {
    list [catch {
	set myOut {}
	blt::bgexec myVar -tty -outputvariable myOut \
	    tclsh files/stdout.tcl
	set myOut
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stdout}}


test bgexec.249 { collect stdout /w -updatevariable } {
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

test bgexec.250 { collect stdout (command result) } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.251 { pipe output } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl | tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.252 { command returns exit code 0 } {
    set code [catch {
	blt::bgexec myVar -tty tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.253 { command returns exit code 1 } {
    set code [catch {
	blt::bgexec myVar -tty tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {1 1}

test bgexec.254 { command returns exit code 100 } {
    set code [catch {
	blt::bgexec myVar -tty tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.255 { command returns exit code -1 } {
    set code [catch {
	blt::bgexec myVar -tty tclsh files/exitcode.tcl -1
    } msg]
    list $code [lindex $myVar 2]
} {1 255}

test bgexec.256 { multiple pipes } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl | tclsh files/cat.tcl | tclsh files/cat.tcl 
    } msg] $msg
} {0 {This is stdout}}

test bgexec.257 { -ignoreexitcode yes (exit code 1) } {
    set code [catch {
	blt::bgexec myVar -tty -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 1
    } msg]
    list $code [lindex $myVar 2]
} {0 1}

test bgexec.258 { -ignoreexitcode yes (exit code 0) } {
    set code [catch {
	blt::bgexec myVar -tty -ignoreexitcode yes \
	    tclsh files/exitcode.tcl 0
    } msg]
    list $code [lindex $myVar 2]
} {0 0}

test bgexec.259 { -ignoreexitcode no (exit code 100) } {
    set code [catch {
	blt::bgexec myVar -tty -ignoreexitcode no \
	    tclsh files/exitcode.tcl 100
    } msg]
    list $code [lindex $myVar 2]
} {1 100}

test bgexec.260 { -environ "MYVAR myValue" } {
    list [catch {
	blt::bgexec myVar -tty -environ "MYVAR myValue" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {MYVAR myValue}}

test bgexec.261 { -environ "" } {
    list [catch {
	blt::bgexec myVar -tty -environ "" \
	    tclsh files/printenv.tcl MYVAR
    } msg] $msg
} {0 {}}

test bgexec.262 { -environ "PATH myPath" } {
    list [catch {
	blt::bgexec myVar -tty -environ "PATH myPath" \
	    tclsh files/printenv.tcl PATH
    } msg] $msg
} {0 {PATH myPath}}

test bgexec.263 { -echo error (see on screen) } {
    list [catch {
	blt::bgexec myVar -tty -echo error tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.264 { -echo output (see on screen) } {
    list [catch {
	blt::bgexec myVar -tty -echo output tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.265 { -echo both (see on screen) } {
    list [catch {
	blt::bgexec myVar -tty -echo both tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.266 { -echo none } {
    list [catch {
	blt::bgexec myVar -tty -echo none tclsh files/both.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.267 { collect stdout w/ -onoutput } {
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

test bgexec.268 { collect stdout w/ -onoutput -keepnewline } {
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

test bgexec.269 { collect stderr w/ -onerror } {
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

test bgexec.270 { collect stderr w/ -onerror -keepnewline } {
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

test bgexec.271 { pipe both stderr and stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl |& tclsh files/cat.tcl
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.272 { pipe just stderr w/ |&  } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stderr}}

test bgexec.273 { pipe just stdout w/ |&  } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl |& tclsh files/cat.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.274 { redirect input from file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl < files/null.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.275 { redirect input badFile } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl < badFile
    } msg] $msg
} {1 {can't read file "badFile": no such file or directory}}

test bgexec.276 { multiple input redirections w/ two files } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/cat.tcl < files/null.tcl < files/null.tcl
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.277 { redirect input w/ missing file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl <
    } msg] $msg
} {1 {can't specify "<" as last word in command}}

test bgexec.278 { redirect input w/ missing literal } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl <<
    } msg] $msg
} {1 {can't specify "<<" as last word in command}}

test bgexec.279 { redirect input w/ missing channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl <@
    } msg] $msg
} {1 {can't specify "<@" as last word in command}}

test bgexec.280 { multiple input redirections /w literal and file } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/cat.tcl < files/null.tcl << "hi"
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.281 { multiple input redirections w/ two literals. } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/cat.tcl << "hi" << "there"
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.282 { multiple input redirections w/ literal and channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -tty tclsh files/cat.tcl << "hi" <@ $channel
    } msg] $msg
} {1 {ambiguous input redirect.}}

test bgexec.283 { redirect input /w channel } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -tty tclsh files/cat.tcl <@ $channel
    } msg] $msg
} {0 {exit 0}}

test bgexec.284 { redirect input /w channel and pipe } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -tty \
	    tclsh files/cat.tcl <@ $channel | tclsh files/cat.tcl
    } msg] $msg
} {0 {exit 0}}

test bgexec.285 {  redirect input from file w/ no command } {
    list [catch {
	blt::bgexec myVar -tty < files/null.tcl
    } msg] $msg
} {1 {missing command for "<"}}

test bgexec.286 { redirect input from literal w/ no command } {
    list [catch {
	blt::bgexec myVar -tty << "hi"
    } msg] $msg
} {1 {missing command for "<<"}}

test bgexec.287 { redirect input from channel w/ no command } {
    list [catch {
	seek $channel 0
	blt::bgexec myVar -tty <@ $channel
    } msg] $msg
} {1 {missing command for "<@"}}

test bgexec.288 { pipe w/ no commands } {
    list [catch {
	blt::bgexec myVar -tty |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.289 { pipe w/ no output command } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl |
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.290 {  pipe w/ no input command } {
    list [catch {
	blt::bgexec myVar -tty | tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.291 { pipe both |& w/ no commands } {
    list [catch {
	blt::bgexec myVar -tty |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.292 { pipe both |& w/ no output command } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl |&
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.293 { pipe both |& w/ no input command } {
    list [catch {
	blt::bgexec myVar -tty |& tclsh files/stdout.tcl
    } msg] $msg
} {1 {illegal use of | or |& in command}}

test bgexec.294 { echo of switches -- } {
    list [catch {
	blt::bgexec myVar -tty -- tclsh files/stdout.tcl
    } msg] $msg
} {0 {This is stdout}}

test bgexec.295 { redirect output to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl > 
    } msg] $msg
} {1 {can't specify ">" as last word in command}}

test bgexec.296 { redirect output to bad file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl > badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.297 { redirect output to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.298 { redirect output and append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl > testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
This is stdout
}}

test bgexec.299 { redirect output append to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl >> 
    } msg] $msg
} {1 {can't specify ">>" as last word in command}}

test bgexec.300 { redirect output append to bad file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl >> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.301 { redirect output append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stdout.tcl >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.302 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl > testfile > testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.303 { multiple output redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/stdout.tcl > testfile >> testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.304 { redirect stdout to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/stdout.tcl >@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.305 { redirect stdout to no channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl >@ 
    } msg] $msg
} {1 {can't specify ">@" as last word in command}}

test bgexec.306 { redirect stdout to bad channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stdout.tcl >@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.307 { redirect stdout to file (verify empty string is result) } {
    list [catch {
	set out1 [blt::bgexec myVar -tty tclsh files/stdout.tcl > testfile]
	set out2 [ReadAndDeleteFile testfile]
	list $out1 $out2 
    } msg] $msg
} {0 {{} {This is stdout
}}}

test bgexec.308 { multiple output redirections /w two channels } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/stdout.tcl >@ $f >@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.309 { redirect stderr to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.310 { redirect input (literal) and output (file) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/cat.tcl << "testme" > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 testme}

test bgexec.311 { redirect stderr to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2> 
    } msg] $msg
} {1 {can't specify "2>" as last word in command}}

test bgexec.312 { redirect stderr to bad file } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl 2> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.313 { redirect stderr append to file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2> testfile
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stderr
}}

test bgexec.314 { redirect stderr append to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>>
    } msg] $msg
} {1 {can't specify "2>>" as last word in command}}

test bgexec.315 { redirect stderr append to bad file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>> badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.316 { redirect stderr append to file (file doesn't exist) } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.317 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl 2> testfile 2> testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.318 { multiple stderr redirections w/ two files } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/stderr.tcl 2> testfile 2>> testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.319 { redirect both stderr and stdout to files } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl 2> testfile1 > testfile2
	set first [ReadAndDeleteFile testfile1]
	set second [ReadAndDeleteFile testfile2]
	list $first $second
    } msg] $msg
} {0 {{This is stderr
} {This is stdout
}}}

test bgexec.320 { redirect both stderr and stdout append to files } {
    list [catch {
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl 2>> testfile1 >> testfile2
	set first [ReadAndDeleteFile testfile1]
	set second [ReadAndDeleteFile testfile2]
	list $first $second
    } msg] $msg
} {0 {{This is stderr
} {This is stdout
}}}

test bgexec.321 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl 2> testfile > testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stdout
}}

test bgexec.322 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty \
	    tclsh files/both.tcl 2>> testfile >> testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.323 { redirect both stderr and stdout to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/both.tcl >& testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.324 { redirect both stderr and stdout to no file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl >& 
    } msg] $msg
} {1 {can't specify ">&" as last word in command}}

test bgexec.325 { redirect both stderr and stdout to bad file } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl >& badDir/badFile
    } msg] $msg
} {1 {can't write file "badDir/badFile": no such file or directory}}

test bgexec.326 { redirect both stderr and stdout appending to one file } {
    list [catch {
	file delete -force testfile
	blt::bgexec myVar -tty tclsh files/both.tcl >>& testfile
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.327 { redirect stderr to channel } {
    list [catch {
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
}}

test bgexec.328 { redirect stderr to no channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>@ 
    } msg] $msg
} {1 {can't specify "2>@" as last word in command}}

test bgexec.329 { redirect stderr to bad channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/stderr.tcl 2>@ badChannel
    } msg] $msg
} {1 {can not find channel named "badChannel"}}

test bgexec.330 { redirect stderr to file (verify empty string is result) } {
    list [catch {
	set myErr {}
	blt::bgexec myVar -tty -errorvariable myErr \
	    tclsh files/stderr.tcl 2> testfile 
	set err [ReadAndDeleteFile testfile]
	list $myErr $err
    } msg] $msg
} {0 {{} {This is stderr
}}}

test bgexec.331 { multiple stderr redirections /w two channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/stdout.tcl 2>@ $f 2>@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.332 { redirect both stderr and stdout to one channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {0 {This is stderr
This is stdout
}}

test bgexec.333 { redirect both stderr and stdout to no channel } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ 
    } msg] $msg
} {1 {can't specify ">&@" as last word in command}}

test bgexec.334 { multiple stderr/stdout redirections /w channels } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ $f >&@ $f
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.335 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ $f > testfile
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous output redirect.}}

test bgexec.336 { multiple output redirections /w file and channel } {
    list [catch {
	catch { close $f }
	set f [open testfile "w"]
	blt::bgexec myVar -tty tclsh files/both.tcl >&@ $f 2> testfile
	close $f
	ReadAndDeleteFile testfile
    } msg] $msg
} {1 {ambiguous error redirect.}}

test bgexec.337 { redirect both stderr and stdout  } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl 2>@1
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {0 {This is stderr\nThis is stdout}}

test bgexec.338 { redirect both stderr and stdout /w extraArg } {
    list [catch {
	blt::bgexec myVar -tty tclsh files/both.tcl 2>@1 extraArg
    } msg] [string map { \0 {\0} \r {\r} \n {\n}} $msg]
} {1 {must specify "2>@1" as last word in command}}

test bgexec.339 { kill by setting status variable } {
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


test bgexec.340 {bgexec -tty kill with myVar } {
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

test bgexec.341 {bgexec -tty kill grandchildren with myVar } {
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

test bgexec.342 { kill with myVar } {
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

test bgexec.343 { kill grandchildren with myVar } {
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

test bgexec.344 { cleanup } {
    list [catch {
	file delete -force testfile testfile1 testfile2
	close $channel
    } msg] $msg
} {0 {}}


exit 0
