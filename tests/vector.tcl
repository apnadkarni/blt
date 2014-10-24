
# Test switches
#
# -before, -after, -node for "insert" operation
#

package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test vector.1 {vector no args} {
    list [catch {blt::vector} msg] $msg
} {1 {wrong # args: should be one of...
  blt::vector create ?vecName? ?switches...?
  blt::vector destroy ?vecName...?
  blt::vector expr expression
  blt::vector names ?pattern?...}}

test vector.2 {vector create} {
    list [catch {blt::vector create \#auto} msg] $msg
} {0 ::vector1}

test vector.3 {vector create myVec} {
    list [catch {blt::vector create myVec} msg] $msg
} {0 ::myVec}

test vector.4 {vector create myVec.\#auto} {
    list [catch {blt::vector create myVec.\#auto} msg] $msg
} {0 ::myVec.vector2}

test vector.5 {vector create \#auto.myVec} {
    list [catch {blt::vector create \#auto.myVec} msg] $msg
} {0 ::vector3.myVec}

test vector.6 {vector create myVec.\#auto.myVec} {
    list [catch {blt::vector create myVec.\#auto.myVec} msg] $msg
} {0 ::myVec.vector4.myVec}

test vector.7 {vector destroy vector1} {
    list [catch {eval blt::vector destroy vector1} msg] $msg
} {0 {}}

test vector.8 {vector destroy [vector names]} {
    list [catch {eval blt::vector destroy [blt::vector names]} msg] $msg
} {0 {}}

test vector.9 {create myVec} {
    list [catch {blt::vector create myVec} msg] $msg
} {0 ::myVec}

test vector.10 {vector names} {
    list [catch {blt::vector names} msg] [lsort $msg]
} {0 ::myVec}

test vector.11 {create myVec} {
    list [catch {blt::vector create myVec} msg] $msg
} {1 {a vector "::myVec" already exists}}

test vector.12 {create if} {
    list [catch {blt::vector create if} msg] $msg
} {1 {a command "::if" already exists}}

test vector.13 {vector create (bad namespace)} {
    list [catch {blt::vector create badNamespace::myVec} msg] $msg
} {1 {unknown namespace "badNamespace"}}

test vector.14 {vector create (bad switch)} {
    list [catch {blt::vector create a badSwitch} msg] $msg
} {1 {unknown switch "badSwitch"
following switches are available:
   -variable varName
   -command command
   -watchunset bool
   -flush bool
   -length length}}


test vector.15 {vector names} {
    list [catch {blt::vector names} msg] [lsort $msg]
} {0 ::myVec}

test vector.16 {vector create myVec2} {
    list [catch {blt::vector create myVec2} msg] [lsort $msg]
} {0 ::myVec2}

test vector.17 {vector names pattern)} {
    list [catch {blt::vector names ::myVec*} msg] [lsort $msg]
} {0 {::myVec ::myVec2}}

test vector.18 {vector names badPattern)} {
    list [catch {blt::vector names badPattern*} msg] $msg
} {0 {}}

test vector.19 {vector names pattern arg (wrong # args)} {
    list [catch {blt::vector names pattern arg} msg] $msg
} {1 {wrong # args: should be "blt::vector names ?pattern?..."}}

test vector.20 {vector destroy (wrong # args)} {
    list [catch {blt::vector destroy} msg] $msg
} {0 {}}

test vector.21 {vector destroy badVector} {
    list [catch {blt::vector destroy badVector} msg] $msg
} {1 {can't find a vector named "badVector"}}

test vector.22 {vector destroy myVec2} {
    list [catch {blt::vector destroy myVec2} msg] $msg
} {0 {}}

test vector.23 {vector destroy myVec2 myVec} {
    list [catch {blt::vector destroy myVec2 myVec} msg] $msg
} {1 {can't find a vector named "myVec2"}}

test vector.24 {vector names)} {
    list [catch {blt::vector names} msg] [lsort $msg]
} {0 ::myVec}

test vector.25 {vector destroy myVec} {
    list [catch {blt::vector destroy myVec} msg] $msg
} {0 {}}

test vector.26 {create} {
    list [catch {blt::vector create myVec} msg] $msg
} {0 ::myVec}

test vector.27 {myVec} {
    list [catch {myVec} msg] $msg
} {1 {wrong # args: should be one of...
  myVec * item
  myVec + item
  myVec - item
  myVec / item
  myVec append item ?item...?
  myVec binread channel ?numValues? ?flags?
  myVec clear 
  myVec count what
  myVec delete index ?index...?
  myVec duplicate ?vecName?
  myVec export format ?switches?
  myVec expr expression
  myVec fft vecName ?switches?
  myVec frequency vecName numBins
  myVec indices what
  myVec inversefft vecName vecName
  myVec length ?newSize?
  myVec limits 
  myVec linspace first last ?numSteps?
  myVec maximum 
  myVec merge vecName ?vecName...?
  myVec minimum 
  myVec normalize ?vecName?
  myVec notify keyword
  myVec offset ?offset?
  myVec populate vecName density
  myVec print format ?switches?
  myVec random ?seed?
  myVec range first last
  myVec search ?-value? value ?value?
  myVec sequence start stop ?step?
  myVec set list
  myVec simplify 
  myVec sort ?switches? ?vecName...?
  myVec split ?vecName...?
  myVec value oper
  myVec values ?switches?
  myVec variable ?varName?}}

test vector.28 {myVec badOp} {
    list [catch {myVec badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myVec * item
  myVec + item
  myVec - item
  myVec / item
  myVec append item ?item...?
  myVec binread channel ?numValues? ?flags?
  myVec clear 
  myVec count what
  myVec delete index ?index...?
  myVec duplicate ?vecName?
  myVec export format ?switches?
  myVec expr expression
  myVec fft vecName ?switches?
  myVec frequency vecName numBins
  myVec indices what
  myVec inversefft vecName vecName
  myVec length ?newSize?
  myVec limits 
  myVec linspace first last ?numSteps?
  myVec maximum 
  myVec merge vecName ?vecName...?
  myVec minimum 
  myVec normalize ?vecName?
  myVec notify keyword
  myVec offset ?offset?
  myVec populate vecName density
  myVec print format ?switches?
  myVec random ?seed?
  myVec range first last
  myVec search ?-value? value ?value?
  myVec sequence start stop ?step?
  myVec set list
  myVec simplify 
  myVec sort ?switches? ?vecName...?
  myVec split ?vecName...?
  myVec value oper
  myVec values ?switches?
  myVec variable ?varName?}}

test vector.29 {myVec length} {
    list [catch {myVec length} msg] $msg
} {0 0}

test vector.30 {myVec l} {
    list [catch {myVec l} msg] $msg
} {1 {ambiguous operation "l" matches:  length limits linspace}}

test vector.31 {myVec le} {
    list [catch {myVec le} msg] $msg
} {0 0}

test vector.32 {myVec len} {
    list [catch {myVec len} msg] $msg
} {0 0}

test vector.33 {myVec leng} {
    list [catch {myVec leng} msg] $msg
} {0 0}

test vector.34 {myVec lengt} {
    list [catch {myVec lengt} msg] $msg
} {0 0}

test vector.35 {myVec length 10} {
    list [catch {myVec length 10} msg] $msg
} {0 10}

test vector.36 {myVec length 10} {
    list [catch {myVec length 10} msg] $msg
} {0 10}


test vector.37 {myVec length -20} {
    list [catch {myVec length -20} msg] $msg
} {1 {bad vector size "-20"}}

test vector.38 {myVec length 0} {
    list [catch {myVec length 0} msg] $msg
} {0 0}

test vector.39 {myVec length 0 badArg} {
    list [catch {myVec length 0 badArg} msg] $msg
} {1 {wrong # args: should be "myVec length ?newSize?"}}

test vector.40 {myVec length 10} {
    list [catch {myVec length 10} msg] $msg
} {0 10}

test vector.41 {myVec values} {
    list [catch {myVec values} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.42 {myVec maximum} {
    list [catch {myVec maximum} msg] $msg
} {0 NaN}

test vector.43 {myVec minimum} {
    list [catch {myVec minimum} msg] $msg
} {0 NaN}

test vector.44 {myVec set "1 2 3 ..." } {
    list [catch {myVec set { 1 2 3 4 5 6 7 8 9 10 }} msg] $msg
} {0 {}}

test vector.45 {myVec values} {
    list [catch {myVec values} msg] $msg
} {0 {1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 10.0}}

test vector.46 {myVec values -format "%E"} {
    list [catch {myVec values -format "%E"} msg] $msg
} {0 {1.000000E+00 2.000000E+00 3.000000E+00 4.000000E+00 5.000000E+00 6.000000E+00 7.000000E+00 8.000000E+00 9.000000E+00 1.000000E+01}}


test vector.47 {blt::vector create test.\#auto} {
    list [catch {blt::vector create test.\#auto} msg] $msg
} {0 ::test.vector5}

test vector.48 {test.vector5 length} {
    list [catch {test.vector5 length} msg] $msg
} {0 0}

test vector.49 {test.vector5 values} {
    list [catch {test.vector5 values} msg] $msg
} {0 {}}

test vector.50 {test.vector5 values -empty no} {
    list [catch {test.vector5 values -empty no} msg] $msg
} {0 {}}

test vector.51 {test.vector5 values -empty yes} {
    list [catch {test.vector5 values -empty yes} msg] $msg
} {0 {}}

test vector.52 {test.vector5 values -badSwitch} {
    list [catch {test.vector5 values -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -format string
   -from index
   -to index
   -empty bool}}

test vector.53 {test.vector5 values -from 0 -to 20} {
    list [catch {test.vector5 values -from 0 -to 20} msg] $msg
} {1 {index "0" is out of range}}

test vector.54 {test.vector5 values -from 1 -to 20} {
    list [catch {test.vector5 values -from 1 -to 20} msg] $msg
} {1 {index "1" is out of range}}

test vector.55 {blt::vector create test.\#auto -length 20} {
    list [catch {blt::vector create test.\#auto -length 20 } msg] $msg
} {0 ::test.vector6}

test vector.56 {test.vector6 length} {
    list [catch {test.vector6 length} msg] $msg
} {0 20}

test vector.57 {test.vector6 count empty} {
    list [catch {test.vector6 count empty} msg] $msg
} {0 20}

test vector.58 {test.vector6 values} {
    list [catch {test.vector6 values} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}


test vector.59 {info exists test.vector5} {
    list [catch {info exists test.vector5} msg] $msg
} {0 1}

test vector.60 {info exists test.vector6} {
    list [catch {info exists test.vector6} msg] $msg
} {0 1}

test vector.61 {blt::vector create test.\#auto -variable myVar} {
    list [catch {blt::vector create test.\#auto -variable myVar} msg] $msg
} {0 ::test.vector7}

test vector.62 {info exists myVar} {
    list [catch {info exists myVar} msg] $msg
} {0 1}

test vector.63 {info commands ::test.*} {
    list [catch {info command ::test.*} msg] [lsort $msg]
} {0 {::test.vector5 ::test.vector6 ::test.vector7}}

test vector.64 {blt::vector names ::test.*} {
    list [catch {blt::vector names ::test.*} msg] [lsort $msg]
} {0 {::test.vector5 ::test.vector6 ::test.vector7}}

test vector.65 {blt::vector destroy names ::test.*} {
    list [catch {
	eval blt::vector destroy [blt::vector names ::test.*]
    } msg] $msg
} {0 {}}

test vector.66 {info exists test.vector5} {
    list [catch {info exists test.vector5} msg] $msg
} {0 0}

test vector.67 {info exists test.vector6} {
    list [catch {info exists test.vector6} msg] $msg
} {0 0}

test vector.68 {info exists myVar} {
    list [catch {info exists myVar} msg] $msg
} {0 0}

test vector.69 {info commands ::test.*} {
    list [catch {info command ::test.*} msg] [lsort $msg]
} {0 {}}

test vector.70 {blt::vector create myVec1 -length 20} {
    list [catch {blt::vector create myVec1 -length 20 } msg] $msg
} {0 ::myVec1}

test vector.71 {blt::vector create myVec2} {
    list [catch {blt::vector create myVec2} msg] $msg
} {0 ::myVec2}

test vector.72 {myVec2 set { 1 2 3 ... }} {
    list [catch {myVec2 set { 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20}} msg] $msg
} {0 {}}

test vector.73 {blt::vector expr myVec1+myVec2} {
    list [catch {blt::vector expr myVec1+myVec2} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.74 {blt::vector expr myVec1-myVec2} {
    list [catch {blt::vector expr myVec1-myVec2} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.75 {blt::vector expr myVec1/myVec2} {
    list [catch {blt::vector expr myVec1/myVec2} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.76 {blt::vector expr myVec1*myVec2} {
    list [catch {blt::vector expr myVec1*myVec2} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.77 {set myVec1(2) 3.0} {
    list [catch {set myVec1(2) 3.0} msg] $msg
} {0 3.0}

test vector.78 {myVec1 value set 3 4.0} {
    list [catch {myVec1 value set 3 4.0} msg] $msg
} {0 4.0}

test vector.79 {myVec1 values} {
    list [catch {myVec1 values} msg] $msg
} {0 {NaN NaN 3.0 4.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.80 {myVec1 maximum} {
    list [catch {myVec1 maximum} msg] $msg
} {0 4.0}

test vector.81 {myVec1 minimum} {
    list [catch {myVec1 minimum} msg] $msg
} {0 3.0}

test vector.82 {blt::vector expr myVec1+myVec2} {
    list [catch {blt::vector expr myVec1+myVec2} msg] $msg
} {0 {NaN NaN 6.0 8.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.83 {blt::vector expr myVec1-myVec2} {
    list [catch {blt::vector expr myVec1-myVec2} msg] $msg
} {0 {NaN NaN 0.0 0.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.84 {blt::vector expr myVec1/myVec2} {
    list [catch {blt::vector expr myVec1/myVec2} msg] $msg
} {0 {NaN NaN 1.0 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.85 {blt::vector expr myVec1*myVec2} {
    list [catch {blt::vector expr myVec1*myVec2} msg] $msg
} {0 {NaN NaN 9.0 16.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.86 {myVec1 count empty} {
    list [catch {myVec1 count empty} msg] $msg
} {0 18}

test vector.87 {myVec1 count zero} {
    list [catch {myVec1 count zero} msg] $msg
} {0 0}

test vector.88 {myVec1 count nonzero} {
    list [catch {myVec1 count nonzero} msg] $msg
} {0 2}

test vector.89 {myVec1 count nonempty} {
    list [catch {myVec1 count nonempty} msg] $msg
} {0 2}

test vector.90 {myVec1 value set 10 0.0} {
    list [catch {myVec1 value set 10 0.0} msg] $msg
} {0 0.0}

test vector.91 {myVec1 count nonzero} {
    list [catch {myVec1 count nonzero} msg] $msg
} {0 2}

test vector.92 {myVec1 count zero} {
    list [catch {myVec1 count zero} msg] $msg
} {0 1}

test vector.93 {myVec1 value set 10 1.0} {
    list [catch {myVec1 value set 10 1.0} msg] $msg
} {0 1.0}

test vector.94 {blt::vector expr myVec1+1.0} {
    list [catch {blt::vector expr myVec1+1.0} msg] $msg
} {0 {NaN NaN 4.0 5.0 NaN NaN NaN NaN NaN NaN 2.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.95 {blt::vector expr myVec1-1.0} {
    list [catch {blt::vector expr myVec1-1.0} msg] $msg
} {0 {NaN NaN 2.0 3.0 NaN NaN NaN NaN NaN NaN 0.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.96 {blt::vector expr myVec1/2.0} {
    list [catch {blt::vector expr myVec1/2.0} msg] $msg
} {0 {NaN NaN 1.5 2.0 NaN NaN NaN NaN NaN NaN 0.5 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.97 {blt::vector expr myVec1*2.0} {
    list [catch {blt::vector expr myVec1*2.0} msg] $msg
} {0 {NaN NaN 6.0 8.0 NaN NaN NaN NaN NaN NaN 2.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}


test vector.98 {blt::vector expr 1.0+myVec} {
    list [catch {blt::vector expr 1.0+myVec1} msg] $msg
} {0 {NaN NaN 4.0 5.0 NaN NaN NaN NaN NaN NaN 2.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.99 {blt::vector expr 1.0-myVec1} {
    list [catch {blt::vector expr 1.0-myVec1} msg] $msg
} {0 {NaN NaN -2.0 -3.0 NaN NaN NaN NaN NaN NaN 0.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.100 {blt::vector expr 2.0/myVec1} {
    list [catch {blt::vector expr 2.0/myVec1} msg] $msg
} {0 {NaN NaN 0.6666666666666666 0.5 NaN NaN NaN NaN NaN NaN 2.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.101 {blt::vector expr 2.0*myVec1} {
    list [catch {blt::vector expr 2.0*myVec1} msg] $msg
} {0 {NaN NaN 6.0 8.0 NaN NaN NaN NaN NaN NaN 2.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.102 {blt::vector expr 1.0+myVec1*myVec2} {
    list [catch {blt::vector expr 1.0+myVec1*myVec2} msg] $msg
} {0 {NaN NaN 10.0 17.0 NaN NaN NaN NaN NaN NaN 12.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.103 {blt::vector expr sin(myVec1)} {
    list [catch {blt::vector expr sin(myVec1)} msg] $msg
} {0 {NaN NaN 0.1411200080598672 -0.7568024953079282 NaN NaN NaN NaN NaN NaN 0.8414709848078965 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.104 {blt::vector expr -myVec1} {
    list [catch {blt::vector expr -myVec1} msg] $msg
} {0 {NaN NaN -3.0 -4.0 NaN NaN NaN NaN NaN NaN -1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.105 {blt::vector expr +myVec1} {
    list [catch {blt::vector expr +myVec1} msg] $msg
} {1 {missing operand}}

test vector.106 {blt::vector expr myVec1} {
    list [catch {blt::vector expr myVec1} msg] $msg
} {0 {NaN NaN 3.0 4.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.107 {blt::vector expr pow(myVec1,3.0)} {
    list [catch {blt::vector expr -myVec1} msg] $msg
} {0 {NaN NaN -3.0 -4.0 NaN NaN NaN NaN NaN NaN -1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.108 {blt::vector expr sqrt(myVec1)} {
    list [catch {blt::vector expr sqrt(myVec1)} msg] $msg
} {0 {NaN NaN 1.7320508075688772 2.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.109 {blt::vector expr atan(myVec1)} {
    list [catch {blt::vector expr -myVec1} msg] $msg
} {0 {NaN NaN -3.0 -4.0 NaN NaN NaN NaN NaN NaN -1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.110 {blt::vector expr myVec1==myVec1} {
    list [catch {blt::vector expr myVec1==myVec1} msg] $msg
} {0 {NaN NaN 1.0 1.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.111 {blt::vector expr myVec1!=myVec1} {
    list [catch {blt::vector expr myVec1!=myVec1} msg] $msg
} {0 {NaN NaN 0.0 0.0 NaN NaN NaN NaN NaN NaN 0.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.112 {blt::vector expr myVec1^3} {
    list [catch {blt::vector expr myVec1^3} msg] $msg
} {0 {NaN NaN 27.0 64.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.113 {blt::vector expr sum(myVec1)} {
    list [catch {blt::vector expr sum(myVec1)} msg] $msg
} {0 11.0}

test vector.114 {blt::vector expr skew(myVec1)} {
    list [catch {blt::vector expr skew(myVec1)} msg] $msg
} {0 0.6807767048090729}

test vector.115 {blt::vector expr var(myVec1)} {
    list [catch {blt::vector expr var(myVec1)} msg] $msg
} {0 2.34375}

test vector.116 {blt::vector expr sdev(myVec1)} {
    list [catch {blt::vector expr sdev(myVec1)} msg] $msg
} {0 1.5309310892394863}

test vector.117 {blt::vector expr adev(myVec1)} {
    list [catch {blt::vector expr adev(myVec1)} msg] $msg
} {0 1.0833333333333333}

test vector.118 {blt::vector expr prod(myVec1)} {
    list [catch {blt::vector expr prod(myVec1)} msg] $msg
} {0 12.0}

test vector.119 {blt::vector expr mean(myVec1)} {
    list [catch {blt::vector expr mean(myVec1)} msg] $msg
} {0 2.75}

test vector.120 {blt::vector expr skew(myVec1)} {
    list [catch {blt::vector expr skew(myVec1)} msg] $msg
} {0 0.6807767048090729}

test vector.121 {blt::vector expr tanh(myVec1)} {
    list [catch {blt::vector expr tanh(myVec1)} msg] $msg
} {0 {NaN NaN 0.9950547536867305 0.999329299739067 NaN NaN NaN NaN NaN NaN 0.7615941559557649 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.122 {blt::vector expr sort(myVec1)} {
    list [catch {blt::vector expr sort(myVec1)} msg] $msg
} {0 {1.0 3.0 4.0}}

test vector.123 {blt::vector expr median(myVec1)} {
    list [catch {blt::vector expr median(myVec1)} msg] $msg
} {0 3.0}

test vector.124 {blt::vector expr exp(myVec1)} {
    list [catch {blt::vector expr exp(myVec1)} msg] $msg
} {0 {NaN NaN 20.085536923187668 54.598150033144236 NaN NaN NaN NaN NaN NaN 2.718281828459045 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.125 {blt::vector expr log10(myVec1)} {
    list [catch {blt::vector expr log10(myVec1)} msg] $msg
} {0 {NaN NaN 0.47712125471966244 0.6020599913279624 NaN NaN NaN NaN NaN NaN 0.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.126 {blt::vector expr log(myVec1)} {
    list [catch {blt::vector expr log(myVec1)} msg] $msg
} {0 {NaN NaN 1.0986122886681098 1.3862943611198906 NaN NaN NaN NaN NaN NaN 0.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.127 {blt::vector expr badFunc(myVec1)} {
    list [catch {blt::vector expr badFunc(myVec1)} msg] $msg
} {1 {can't find a vector named "badFunc"}}

test vector.128 {blt::vector expr myVec2)} {
    list [catch {blt::vector expr myVec2} msg] $msg
} {0 {1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 10.0 11.0 12.0 13.0 14.0 15.0 16.0 17.0 18.0 19.0 20.0}}

test vector.129 {blt::vector create myVec7} {
    list [catch {blt::vector create myVec7} msg] $msg
} {0 ::myVec7}

test vector.130 {blt::vector expr {myVec2 > 3.0 && myVec2 < 13.0}} {
    list [catch { blt::vector expr {myVec2 > 3.0 && myVec2 < 13.0} } msg] $msg
} {0 {0.0 0.0 0.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0}}

test vector.131 {myVec7 expr {myVec2 > 3.0}} {
    list [catch {myVec7 expr {myVec2 > 3.0}} msg] $msg
} {0 {}}

test vector.132 {myVec7 values} {
    list [catch {myVec7 values} msg] $msg
} {0 {0.0 0.0 0.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0}}

test vector.133 {myVec7 indices zero} {
    list [catch {myVec7 indices zero} msg] $msg
} {0 {0 1 2}}

test vector.134 {myVec7 indices nonzero} {
    list [catch {myVec7 indices nonzero} msg] $msg
} {0 {3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19}}

test vector.135 {myVec7 indices nonempty} {
    list [catch {myVec7 indices nonempty} msg] $msg
} {0 {0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19}}

test vector.136 {myVec7 indices empty} {
    list [catch {myVec7 indices empty} msg] $msg
} {0 {}}

test vector.137 {blt::vector expr median(myVec1)} {
    list [catch {blt::vector expr median(myVec1)} msg] $msg
} {0 3.0}

test vector.138 {blt::vector expr q1(myVec1)} {
    list [catch {blt::vector expr q1(myVec1)} msg] $msg
} {0 1.0}

test vector.139 {blt::vector expr q2(myVec1)} {
    list [catch {blt::vector expr q2(myVec1)} msg] $msg
} {0 2.75}

test vector.140 {blt::vector expr q3(myVec1)} {
    list [catch {blt::vector expr q3(myVec1)} msg] $msg
} {0 4.0}

test vector.141 {blt::vector expr myVec1+0.111} {
    list [catch {blt::vector expr myVec1+0.111} msg] $msg
} {0 {NaN NaN 3.111 4.111 NaN NaN NaN NaN NaN NaN 1.111 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.142 {blt::vector expr ceil(myVec1+0.111)} {
    list [catch {blt::vector expr ceil(myVec1+0.111)} msg] $msg
} {0 {NaN NaN 4.0 5.0 NaN NaN NaN NaN NaN NaN 2.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.143 {blt::vector expr floor(myVec1+0.111)} {
    list [catch {blt::vector expr floor(myVec1+0.111)} msg] $msg
} {0 {NaN NaN 3.0 4.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.144 {blt::vector expr floor(myVec1)} {
    list [catch {blt::vector expr floor(myVec1)} msg] $msg
} {0 {NaN NaN 3.0 4.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.145 {blt::vector expr ceil(myVec1)} {
    list [catch {blt::vector expr ceil(myVec1)} msg] $msg
} {0 {NaN NaN 3.0 4.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.146 {blt::vector create myVec3 -length 10} {
    list [catch {blt::vector create myVec3 -length 10} msg] $msg
} {0 ::myVec3}

# Is there a graceful way of handling the range when the vector is empty?
# Should it return an empty string rather than an error? 
test vector.147 {myVec3 range 1 2} {
    list [catch {myVec3 range 1 2} msg] $msg
} {0 {NaN NaN}}

test vector.148 {myVec3 range 1 2 3} {
    list [catch {myVec3 range 1 2 3} msg] $msg
} {1 {wrong # args: should be "myVec3 range first last"}}

test vector.149 {myVec3 range 1 } {
    list [catch {myVec3 range 1 } msg] $msg
} {1 {wrong # args: should be "myVec3 range ?first last?"}}

test vector.150 {myVec3 range} {
    list [catch {myVec3 range} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.151 {myVec3 variable} {
    list [catch {myVec3 variable} msg] $msg
} {0 ::myVec3}

test vector.152 {myVec3 variable myVar badArg} {
    list [catch {myVec3 variable myVar badArg} msg] $msg
} {1 {wrong # args: should be "myVec3 variable ?varName?"}}

# linspace uses the length of the vector if no step is given.
test vector.153 {myVec3 linspace 0 20} {
    list [catch {myVec3 linspace 0 20} msg] $msg
} {0 {}}

# sequence will resize the vector. The default number of steps is 1.
test vector.154 {myVec3 sequence 0 20 20/9} {
    list [catch {myVec3 sequence 0 20 [expr 20.0/9]} msg] $msg
} {0 {}}

test vector.155 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 2.2222222222222223 4.444444444444445 6.666666666666667 8.88888888888889 11.11111111111111 13.333333333333334 15.555555555555557 17.77777777777778 20.0}}

test vector.156 {myVec3 random 10} {
    list [catch {myVec3 random 10} msg] $msg
} {0 {}}

test vector.157 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.8788511227621747 0.7958066229216172 0.4808272810570422 0.5256732582083217 0.45916247450539416 0.9454757943603767 0.6461477888718399 0.5854443700928513 0.6398346405389113 0.9568804954767529}}

test vector.158 {myVec3 value set all 4.0} {
    list [catch {myVec3 value set all 4.0} msg] $msg
} {0 4.0}

test vector.159 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {4.0 4.0 4.0 4.0 4.0 4.0 4.0 4.0 4.0 4.0}}

test vector.160 {myVec3 value set all 0.0} {
    list [catch {myVec3 value set all 0.0} msg] $msg
} {0 0.0}

test vector.161 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0}}

test vector.162 {myVec3 value set 1:end 1.0} {
    list [catch {myVec3 value set 1:end 1.0} msg] $msg
} {0 1.0}

test vector.163 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0}}

test vector.164 {myVec3 value set 1:3 14.0} {
    list [catch {
	myVec3 value set 1:3 14.0
	myVec3 values
    } msg] $msg
} {0 {0.0 14.0 14.0 14.0 1.0 1.0 1.0 1.0 1.0 1.0}}

test vector.165 {myVec3 value set 0:end 7.0} {
    list [catch {myVec3 value set 0:end 7.0} msg] $msg
} {0 7.0}

test vector.166 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {7.0 7.0 7.0 7.0 7.0 7.0 7.0 7.0 7.0 7.0}}

test vector.167 {myVec3 value set 0:end++ 2.0} {
    list [catch {myVec3 value set 0:end++ 2.0} msg] $msg
} {1 {bad index "end++"}}

test vector.168 {myVec3 length} {
    list [catch {myVec3 length} msg] $msg
} {0 10}

test vector.169 {myVec3 value set 0:++end 2.0} {
    list [catch {myVec3 value set 0:++end 2.0} msg] $msg
} {0 2.0}

test vector.170 {myVec3 length} {
    list [catch {myVec3 length} msg] $msg
} {0 11}

test vector.171 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0}}

test vector.172 {myVec3 delete end} {
    list [catch {
	myVec3 delete end
    } msg] $msg
} {0 {}}

test vector.173 {myVec3 value set ++end 0.0} {
    list [catch {
	myVec3 value set ++end 100.0
    } msg] $msg
} {0 100.0}

test vector.174 {myVec3 length} {
    list [catch {myVec3 length} msg] $msg
} {0 11}

test vector.175 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 100.0}}

test vector.176 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 100.0}}

test vector.177 {myVec3 value set end 0.0} {
    list [catch {myVec3 value set end 0.0} msg] $msg
} {0 0.0}

test vector.178 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 0.0}}

test vector.179 {myVec3 value set 0 0.0} {
    list [catch {myVec3 value set 0 0.0} msg] $msg
} {0 0.0}

test vector.180 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 0.0}}

test vector.181 {myVec3 delete end} {
    list [catch {myVec3 delete end} msg] $msg
} {0 {}}

test vector.182 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0}}

test vector.183 {myVec3 linspace 0 0} {
    list [catch {myVec3 linspace 0 0} msg] $msg
} {0 {}}

test vector.184 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0}}


test vector.185 {myVec1 sort -indices} {
    list [catch {myVec1 sort -indices} msg] $msg
} {0 {10 2 3}}

test vector.186 {myVec1 sort -decreasing} {
    list [catch {myVec1 sort -decreasing} msg] $msg
} {0 {4.0 3.0 1.0}}

test vector.187 {myVec1 sort -uniq} {
    list [catch {myVec1 sort -uniq} msg] $msg
} {0 {1.0 3.0 4.0}}

test vector.188 {myVec1 sort myVec1} {
    list [catch {myVec1 sort myVec1} msg] $msg
} {0 {}}

test vector.189 {myVec1 values} {
    list [catch {myVec1 values} msg] $msg
} {0 {1.0 3.0 4.0}}

test vector.190 {myVec1 length} {
    list [catch {myVec1 length} msg] $msg
} {0 3}

test vector.191 {myVec1 count empty} {
    list [catch {myVec1 count empty} msg] $msg
} {0 0}

test vector.192 {myVec1 count zero} {
    list [catch {myVec1 count zero} msg] $msg
} {0 0}

test vector.193 {myVec3 duplicate} {
    list [catch {myVec3 duplicate} msg] $msg
} {0 ::vector7}

test vector.194 {myVec3 duplicate myVec4} {
    list [catch {myVec3 duplicate myVec4} msg] $msg
} {0 ::myVec4}

test vector.195 {blt::vector expr myVec4==myVec3} {
    list [catch {blt::vector expr myVec4==myVec3} msg] $msg
} {0 {1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0}}

test vector.196 {blt::vector create myVec5 -length 20} {
    list [catch {blt::vector create myVec5 -length 20} msg] $msg
} {0 ::myVec5}

test vector.197 {myVec5 length} {
    list [catch {myVec5 length} msg] $msg
} {0 20}

test vector.198 {myVec5 values} {
    list [catch {myVec5 values} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.199 {blt::vector expr myVec5==myVec5} {
    list [catch {blt::vector expr myVec5==myVec5} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.200 {blt::vector destroy myVec4 vector7} {
    list [catch {blt::vector destroy myVec4 vector7} msg] $msg
} {0 {}}

test vector.201 {myVec set (missing arg)} {
    list [catch {myVec set} msg] $msg
} {1 {wrong # args: should be "myVec set list"}}

test vector.202 {myVec3 value badOp} {
    list [catch {myVec3 value badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myVec3 value get index
  myVec3 value set index value
  myVec3 value unset ?index...?}}

test vector.203 {myVec3 value badOp extraArgs } {
    list [catch {myVec3 value badOp extraArgs} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myVec3 value get index
  myVec3 value set index value
  myVec3 value unset ?index...?}}

test vector.204 {myVec3 linspace 1 10} {
    list [catch {myVec3 linspace 1 10} msg] $msg
} {0 {}}

test vector.205 {myVec3 value get } {
    list [catch {myVec3 value get} msg] $msg
} {1 {wrong # args: should be "myVec3 value get index"}}

test vector.206 {myVec3 value get badIndex} {
    list [catch {myVec3 value get badIndex} msg] $msg
} {1 {bad index "badIndex"}}

test vector.207 {myVec3 value get -1} {
    list [catch {myVec3 value get -1} msg] $msg
} {1 {index "-1" is out of range}}

test vector.208 {myVec3 value get ++end} {
    list [catch {myVec3 value get ++end} msg] $msg
} {1 {can't get index "++end"}}

test vector.209 {myVec3 value get min} {
    list [catch {myVec3 value get min} msg] $msg
} {1 {bad index "min"}}

test vector.210 {myVec3 value get max} {
    list [catch {myVec3 value get max} msg] $msg
} {1 {bad index "max"}}

test vector.211 {myVec3 value get 0} {
    list [catch {myVec3 value get 0} msg] $msg
} {0 1.0}

test vector.212 {myVec3 value get 1} {
    list [catch {myVec3 value get 1} msg] $msg
} {0 2.0}

test vector.213 {myVec3 value get end} {
    list [catch {myVec3 value get end} msg] $msg
} {0 10.0}

test vector.214 {myVec3 value get 0 extraArg} {
    list [catch {myVec3 value get 0 extraArg} msg] $msg
} {1 {wrong # args: should be "myVec3 value get index"}}

test vector.215 {myVec3 value set ++end 1000.0} {
    list [catch {myVec3 value set ++end 1000.0} msg] $msg
} {0 1000.0}

test vector.216 {myVec3 value unset end} {
    list [catch {myVec3 value unset end} msg] $msg
} {0 {}}

test vector.217 {myVec3 value get end} {
    list [catch {myVec3 value get end} msg] $msg
} {0 NaN}

test vector.218 {myVec3 value set all 1.0} {
    list [catch {myVec3 value set all 1.0} msg] $msg
} {0 1.0}

test vector.219 {myVec3 value unset all} {
    list [catch {myVec3 value unset all} msg] $msg
} {0 {}}

test vector.220 {myVec3 value get end} {
    list [catch {myVec3 value get end} msg] $msg
} {0 NaN}

test vector.221 {myVec3 value set all 1.0} {
    list [catch {myVec3 value set all 1.0} msg] $msg
} {0 1.0}

test vector.222 {myVec3 value unset 0:end} {
    list [catch {myVec3 value unset 0:end} msg] $msg
} {0 {}}

test vector.223 {myVec3 value get end} {
    list [catch {myVec3 value get end} msg] $msg
} {0 NaN}

test vector.224 {vector create myVec6} {
    list [catch {blt::vector create myVec6} msg] $msg
} {0 ::myVec6}

test vector.225 {myVec6 populate} {
    list [catch {myVec6 populate} msg] $msg
} {1 {wrong # args: should be "myVec6 populate vecName density"}}

test vector.226 {myVec6 populate myVec4} {
    list [catch {myVec6 populate myVec4} msg] $msg
} {1 {wrong # args: should be "myVec6 populate vecName density"}}

test vector.227 {myVec6 populate myVec3 10} {
    list [catch {myVec6 populate myVec3 10} msg] $msg
} {0 {}}

test vector.228 {myVec6 values} {
    list [catch {myVec6 values} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.229 {myVec6 populate myVec2 2} {
    list [catch {myVec6 populate myVec2 2} msg] $msg
} {0 {}}

test vector.230 {myVec6 values} {
    list [catch {myVec6 values} msg] $msg
} {0 {1.0 1.3333333333333333 1.6666666666666665 2.0 2.3333333333333335 2.6666666666666665 3.0 3.3333333333333335 3.6666666666666665 4.0 4.333333333333333 4.666666666666667 5.0 5.333333333333333 5.666666666666667 6.0 6.333333333333333 6.666666666666667 7.0 7.333333333333333 7.666666666666667 8.0 8.333333333333334 8.666666666666666 9.0 9.333333333333334 9.666666666666666 10.0 10.333333333333334 10.666666666666666 11.0 11.333333333333334 11.666666666666666 12.0 12.333333333333334 12.666666666666666 13.0 13.333333333333334 13.666666666666666 14.0 14.333333333333334 14.666666666666666 15.0 15.333333333333334 15.666666666666666 16.0 16.333333333333332 16.666666666666668 17.0 17.333333333333332 17.666666666666668 18.0 18.333333333333332 18.666666666666668 19.0 19.333333333333332 19.666666666666668 20.0}}

exit 0

