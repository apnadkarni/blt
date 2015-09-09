package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test bgexec.1 {bgexec no args} {
    list [catch {blt::bgexec} msg] $msg
} {1 {wrong # args: should be "blt::bgexec varName ?options? command ?arg...?"}}

test bgexec.2 {bgexec var no args} {
    list [catch {blt::bgexec myVar} msg] $msg
} {1 {wrong # args: should be "blt::bgexec varName ?options? command ?arg...?"}}

test bgexec.3 {bgexec var tclsh files/null.tcl} {
    list [catch {blt::bgexec myVar tclsh files/null.tcl} msg] $msg
} {0 {}}

test bgexec.4 {bgexec var -help } {
    list [catch {blt::bgexec myVar -help} msg] $msg
} {1 {following switches are available:
   -decodeerror encoding
   -decodeoutput encoding
   -detach bool
   -echo bool
   -environ list
   -error variable
   -ignoreexitcode bool
   -keepnewline bool
   -killsignal signal
   -lasterror variable
   -lastoutput variable
   -linebuffered bool
   -onerror command
   -onoutput command
   -output variable
   -poll interval
   -update variable}}

test bgexec.5 {bgexec myVar -badSwitch } {
    list [catch {blt::bgexec myMyVar -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -decodeerror encoding
   -decodeoutput encoding
   -detach bool
   -echo bool
   -environ list
   -error variable
   -ignoreexitcode bool
   -keepnewline bool
   -killsignal signal
   -lasterror variable
   -lastoutput variable
   -linebuffered bool
   -onerror command
   -onoutput command
   -output variable
   -poll interval
   -update variable}}

test bgexec.6 {bgexec myVar -decodeerror no arg } {
    list [catch {blt::bgexec myVar -decodeerror} msg] $msg
} {1 {value for "-decodeerror" missing}}

test bgexec.7 {bgexec myVar -decodeoutput no arg } {
    list [catch {blt::bgexec myVar -decodeoutput} msg] $msg
} {1 {value for "-decodeoutput" missing}}

test bgexec.8 {bgexec myVar -detach no arg } {
    list [catch {blt::bgexec myVar -detach} msg] $msg
} {1 {value for "-detach" missing}}

test bgexec.9 {bgexec myVar -echo no arg } {
    list [catch {blt::bgexec myVar -echo} msg] $msg
} {1 {value for "-echo" missing}}

test bgexec.10 {bgexec myVar -environ no arg } {
    list [catch {blt::bgexec myVar -environ} msg] $msg
} {1 {value for "-environ" missing}}

test bgexec.11 {bgexec myVar -error no arg } {
    list [catch {blt::bgexec myVar -error} msg] $msg
} {1 {value for "-error" missing}}

test bgexec.12 {bgexec myVar -ignoreexitcode no arg } {
    list [catch {blt::bgexec myVar -ignoreexitcode} msg] $msg
} {1 {value for "-ignoreexitcode" missing}}

test bgexec.13 {bgexec myVar -keepnewline no arg } {
    list [catch {blt::bgexec myVar -keepnewline} msg] $msg
} {1 {value for "-keepnewline" missing}}

test bgexec.14 {bgexec myVar -killsignal no arg } {
    list [catch {blt::bgexec myVar -killsignal} msg] $msg
} {1 {value for "-killsignal" missing}}

test bgexec.15 {bgexec myVar -lasterror no arg } {
    list [catch {blt::bgexec myVar -lasterror} msg] $msg
} {1 {value for "-lasterror" missing}}

test bgexec.16 {bgexec myVar -lastoutput no arg } {
    list [catch {blt::bgexec myVar -lastoutput} msg] $msg
} {1 {value for "-lastoutput" missing}}

test bgexec.17 {bgexec myVar -linebuffered no arg } {
    list [catch {blt::bgexec myVar -linebuffered} msg] $msg
} {1 {value for "-linebuffered" missing}}

test bgexec.18 {bgexec myVar -onerror no arg } {
    list [catch {blt::bgexec myVar -onerror} msg] $msg
} {1 {value for "-onerror" missing}}

test bgexec.19 {bgexec myVar -onoutput no arg } {
    list [catch {blt::bgexec myVar -onoutput} msg] $msg
} {1 {value for "-onoutput" missing}}

test bgexec.20 {bgexec myVar -output no arg } {
    list [catch {blt::bgexec myVar -output} msg] $msg
} {1 {value for "-output" missing}}

test bgexec.21 {bgexec myVar -poll no arg } {
    list [catch {blt::bgexec myVar -poll} msg] $msg
} {1 {value for "-poll" missing}}

test bgexec.22 {bgexec myVar -update no arg } {
    list [catch {blt::bgexec myVar -update} msg] $msg
} {1 {value for "-update" missing}}

test bgexec.23 {bgexec myVar badCmd } {
    list [catch {blt::bgexec myVar badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}

test bgexec.24 {bgexec myVar badCmd } {
    list [catch {blt::bgexec myVar badCmd} msg] $msg
} {1 {can't execute "badCmd": no such file or directory}}

test bgexec.25 {bgexec myVar redirect input } {
    list [catch {blt::bgexec myVar tclsh files/redirectInput.tcl < files/null.tcl} msg] $msg
} {0 {exit 0
}}


test bgexec.26 {bgexec myVar redirect input } {
    list [catch {blt::bgexec myVar tclsh files/redirectInput.tcl << "test me"} msg] $msg
} {0 {test me}}

test bgexec.27 {bgexec myVar redirect input } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar tclsh files/redirectInput.tcl <@ $file]
	close $file
	set out
    } msg] $msg
} {0 {exit 0
}}


test bgexec.28 {bgexec myVar bad redirect syntax } {
    list [catch {
	set file [open "files/null.tcl" "r"]
	set out [blt::bgexec myVar tclsh files/redirectInput.tcl @< $file]
	close $file
	set out
    } msg] $msg
} {1 {child process exited abnormally}}








