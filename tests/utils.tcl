
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
  blt::utils::string between str first last ?switches?
  blt::utils::string contains str pattern ?switches?
  blt::utils::string ends str pattern ?switches?
  blt::utils::string equals str pattern ?switches?
  blt::utils::string inlist str list ?switches?}}

test utils.2 {number no arg} {
    list [catch {blt::utils::number} msg] $msg
} {1 {wrong # args: should be one of...
  blt::utils::number between value first last
  blt::utils::number eq value1 value2
  blt::utils::number ge value1 value2
  blt::utils::number gt value1 value2
  blt::utils::number inlist value list ?switches?
  blt::utils::number le value1 value2
  blt::utils::number lt value1 value2}}

test utils.3 {number badArg} {
    list [catch {blt::utils::number badArg} msg] $msg
} {1 {bad operation "badArg": should be one of...
  blt::utils::number between value first last
  blt::utils::number eq value1 value2
  blt::utils::number ge value1 value2
  blt::utils::number gt value1 value2
  blt::utils::number inlist value list ?switches?
  blt::utils::number le value1 value2
  blt::utils::number lt value1 value2}}

test utils.4 {number between } {
    list [catch {blt::utils::number between} msg] $msg
} {1 {wrong # args: should be "blt::utils::number between value first last"}}

test utils.5 {number between 1} {
    list [catch {blt::utils::number between 1} msg] $msg
} {1 {wrong # args: should be "blt::utils::number between value first last"}}

test utils.6 {number between 1 2} {
    list [catch {blt::utils::number between 1 2} msg] $msg
} {1 {wrong # args: should be "blt::utils::number between value first last"}}

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

test utils.12 {number between 1 0 3 extraArg} {
    list [catch {blt::utils::number between 1 0 3 extraArg} msg] $msg
} {1 {wrong # args: should be "blt::utils::number between value first last"}}

test utils.13 {number between 1 0 3} {
    list [catch {blt::utils::number between 1 0 3} msg] $msg
} {0 1}

test utils.14 {number between 1 3 0} {
    list [catch {blt::utils::number between 1 3 0} msg] $msg
} {0 1}

test utils.15 {number between 1.00000001 1 2} {
    list [catch {blt::utils::number between 1.000000001 1 2} msg] $msg
} {0 1}

test utils.16 {number between 1.00000001 1 0} {
    list [catch {blt::utils::number between 1.000000001 1 0} msg] $msg
} {0 0}

test utils.17 {number eq 1.00000000000000000001 1 } {
    list [catch {blt::utils::number eq 1.00000000000000000001 1} msg] $msg
} {0 1}

test utils.18 {number eq} {
    list [catch {blt::utils::number eq} msg] $msg
} {1 {wrong # args: should be "blt::utils::number eq value1 value2"}}

test utils.19 {number eq badArg} {
    list [catch {blt::utils::number eq badArg} msg] $msg
} {1 {wrong # args: should be "blt::utils::number eq value1 value2"}}

test utils.20 {number eq badArg badArg} {
    list [catch {blt::utils::number eq badArg badArg} msg] $msg
} {1 {expected floating-point number but got "badArg"}}

test utils.21 {number eq 1 1 -badSwitch} {
    list [catch {blt::utils::number eq 1 1 extraArg} msg] $msg
} {1 {wrong # args: should be "blt::utils::number eq value1 value2"}}

test utils.22 {number inlist} {
    list [catch {blt::utils::number inlist} msg] $msg
} {1 {wrong # args: should be "blt::utils::number inlist value list ?switches?"}}

test utils.23 {number inlist arg} {
    list [catch {blt::utils::number inlist arg } msg] $msg
} {1 {wrong # args: should be "blt::utils::number inlist value list ?switches?"}}

test utils.24 {number inlist 1.0 ""} {
    list [catch {blt::utils::number inlist 1.0 "" } msg] $msg
} {0 0}

test utils.25 {number inlist 1.0 "1 2 3"} {
    list [catch {blt::utils::number inlist 1.0 "1 2 3" } msg] $msg
} {0 1}

test utils.26 {number inlist 1.0 "1e0 2 3"} {
    list [catch {blt::utils::number inlist 1.0 "1e0 2 3" } msg] $msg
} {0 1}

test utils.27 {number inlist 1.0 "3 2 1"} {
    list [catch {blt::utils::number inlist 1.0 "3 2 1" } msg] $msg
} {0 1}

test utils.28 {number inlist 1.0 "3 1 2 1"} {
    list [catch {blt::utils::number inlist 1.0 "3 1 2 1" } msg] $msg
} {0 1}

test utils.29 {number inlist 100 "3 1 2 1e2"} {
    list [catch {blt::utils::number inlist 100 "3 2 1e2" } msg] $msg
} {0 1}

test utils.30 {number inlist 1.0 "1 2 3" -sorted increasing} {
    list [catch {blt::utils::number inlist 1.0 "1 2 3" -sorted increasing} msg] $msg
} {0 1}

test utils.31 {number inlist 1.0 "1 2 3" -sorted decreasing} {
    list [catch {blt::utils::number inlist 1.0 "1 2 3" -sorted decreasing} msg] $msg
} {0 0}

test utils.32 {number inlist 1.0 "3 2 1" -sorted decreasing} {
    list [catch {blt::utils::number inlist 1.0 "3 2 1" -sorted decreasing} msg] $msg
} {0 1}

test utils.33 {number inlist 1.0 "1e0 2 3"} {
    list [catch {blt::utils::number inlist 1.0 "1e0 2 3" -sorted increasing} msg] $msg
} {0 1}

test utils.34 {number inlist 1.0 "3 2 1" -sorted increasing} {
    list [catch {blt::utils::number inlist 1.0 "3 2 1" -sorted increasing} msg] $msg
} {0 0}

test utils.35 {number inlist 1.0 "3 1 2 1" -sorted increasing} {
    list [catch {blt::utils::number inlist 1.0 "3 1 2 1" -sorted increasing} msg] $msg
} {0 1}

test utils.36 {number inlist 100 "3 1 2 1e2" -sorted increasing} {
    list [catch {blt::utils::number inlist 100 "3 2 1e2" -sorted increasing} msg] $msg
} {0 1}

test utils.37 {number inlist 100 "3 1 2 1e2" -sorted badValue} {
    list [catch {blt::utils::number inlist 100 "3 2 1e2" -sorted badValue} msg] $msg
} {1 {bad sorted value "badValue": should be decreasing, increasing, or none}}

test utils.38 {number inlist 100 "3 1 2 1e2" -badSwitch} {
    list [catch {blt::utils::number inlist 100 "3 2 1e2" -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -sorted decreasing|increasing}}

