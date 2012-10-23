
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
    list [catch {blt::vector create #auto} msg] $msg
} {0 ::vector1}

test vector.3 {vector create myVec} {
    list [catch {blt::vector create myVec} msg] $msg
} {0 ::myVec}

test vector.4 {vector create myVec.#auto} {
    list [catch {blt::vector create myVec.#auto} msg] $msg
} {0 ::myVec.vector2}

test vector.5 {vector create #auto.myVec} {
    list [catch {blt::vector create #auto.myVec} msg] $msg
} {0 ::vector3.myVec}

test vector.6 {vector create myVec.#auto.myVec} {
    list [catch {blt::vector create myVec.#auto.myVec} msg] $msg
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
   -size length}}


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
  myVec expr expression
  myVec fft vecName ?switches?
  myVec indices what
  myVec inversefft vecName vecName
  myVec length ?newSize?
  myVec maximum 
  myVec merge vecName ?vecName...?
  myVec minimum 
  myVec notify keyword
  myVec normalize ?vecName?
  myVec notify keyword
  myVec offset ?offset?
  myVec populate vecName density
  myVec random ?seed?
  myVec range first last
  myVec search ?-value? value ?value?
  myVec sequence begin end ?length?
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
  myVec expr expression
  myVec fft vecName ?switches?
  myVec indices what
  myVec inversefft vecName vecName
  myVec length ?newSize?
  myVec maximum 
  myVec merge vecName ?vecName...?
  myVec minimum 
  myVec notify keyword
  myVec normalize ?vecName?
  myVec notify keyword
  myVec offset ?offset?
  myVec populate vecName density
  myVec random ?seed?
  myVec range first last
  myVec search ?-value? value ?value?
  myVec sequence begin end ?length?
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
} {0 0}

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

test vector.55 {blt::vector create test.\#auto -size 20} {
    list [catch {blt::vector create test.\#auto -size 20 } msg] $msg
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

test vector.70 {blt::vector create myVec1 -size 20} {
    list [catch {blt::vector create myVec1 -size 20 } msg] $msg
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

test vector.128 {blt::vector create myVec7} {
    list [catch {blt::vector create myVec7} msg] $msg
} {0 ::myVec7}

test vector.128 {blt::vector expr {myVec2 > 3.0 && myVec2 < 13.0}} {
    list [catch { blt::vector expr {myVec2 > 3.0 && myVec2 < 13.0} } msg] $msg
} {0 {0.0 0.0 0.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0}}

test vector.128 {myVec7 expr {myVec2 > 3.0}} {
    list [catch {myVec7 expr {myVec2 > 3.0}} msg] $msg
} {0 {}}

test vector.128 {myVec7 values} {
    list [catch {myVec7 values} msg] $msg
} {0 {0.0 0.0 0.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0}}

test vector.128 {myVec7 indices zero} {
    list [catch {myVec7 indices zero} msg] $msg
} {0 {0 1 2}}

test vector.128 {myVec7 indices nonzero} {
    list [catch {myVec7 indices nonzero} msg] $msg
} {0 {3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19}}

test vector.128 {myVec7 indices nonempty} {
    list [catch {myVec7 indices nonempty} msg] $msg
} {0 {0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19}}

test vector.128 {myVec7 indices empty} {
    list [catch {myVec7 indices empty} msg] $msg
} {0 {}}

test vector.129 {blt::vector expr median(myVec1)} {
    list [catch {blt::vector expr median(myVec1)} msg] $msg
} {0 3.0}

test vector.130 {blt::vector expr q1(myVec1)} {
    list [catch {blt::vector expr q1(myVec1)} msg] $msg
} {0 1.0}

test vector.131 {blt::vector expr q2(myVec1)} {
    list [catch {blt::vector expr q2(myVec1)} msg] $msg
} {0 2.75}

test vector.132 {blt::vector expr q3(myVec1)} {
    list [catch {blt::vector expr q3(myVec1)} msg] $msg
} {0 4.0}

test vector.133 {blt::vector expr myVec1+0.111} {
    list [catch {blt::vector expr myVec1+0.111} msg] $msg
} {0 {NaN NaN 3.111 4.111 NaN NaN NaN NaN NaN NaN 1.111 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.134 {blt::vector expr ceil(myVec1+0.111)} {
    list [catch {blt::vector expr ceil(myVec1+0.111)} msg] $msg
} {0 {NaN NaN 4.0 5.0 NaN NaN NaN NaN NaN NaN 2.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.135 {blt::vector expr floor(myVec1+0.111)} {
    list [catch {blt::vector expr floor(myVec1+0.111)} msg] $msg
} {0 {NaN NaN 3.0 4.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.136 {blt::vector expr floor(myVec1)} {
    list [catch {blt::vector expr floor(myVec1)} msg] $msg
} {0 {NaN NaN 3.0 4.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.137 {blt::vector expr ceil(myVec1)} {
    list [catch {blt::vector expr ceil(myVec1)} msg] $msg
} {0 {NaN NaN 3.0 4.0 NaN NaN NaN NaN NaN NaN 1.0 NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.138 {blt::vector create myVec3 -size 10} {
    list [catch {blt::vector create myVec3 -size 10} msg] $msg
} {0 ::myVec3}

test vector.139 {myVec3 range 1 2} {
    list [catch {myVec3 range 1 2} msg] $msg
} {0 {NaN NaN}}

test vector.140 {myVec3 range 1 2 3} {
    list [catch {myVec3 range 1 2 3} msg] $msg
} {1 {wrong # args: should be "myVec3 range first last"}}

test vector.141 {myVec3 range 1 } {
    list [catch {myVec3 range 1 } msg] $msg
} {1 {wrong # args: should be "myVec3 range ?first last?}}

test vector.142 {myVec3 range} {
    list [catch {myVec3 range} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}


test vector.143 {myVec3 variable} {
    list [catch {myVec3 variable} msg] $msg
} {0 ::myVec3}

test vector.144 {myVec3 variable myVar badArg} {
    list [catch {myVec3 variable myVar badArg} msg] $msg
} {1 {wrong # args: should be "myVec3 variable ?varName?"}}

test vector.145 {myVec3 sequence 0 20} {
    list [catch {myVec3 sequence 0 20} msg] $msg
} {0 {}}

test vector.146 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 2.2222222222222223 4.444444444444445 6.666666666666667 8.88888888888889 11.11111111111111 13.333333333333334 15.555555555555557 17.77777777777778 20.0}}

test vector.147 {myVec3 random 10} {
    list [catch {myVec3 random 10} msg] $msg
} {0 {}}

test vector.148 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.8788511227621747 0.7958066229216172 0.4808272810570422 0.5256732582083217 0.45916247450539416 0.9454757943603767 0.6461477888718399 0.5854443700928513 0.6398346405389113 0.9568804954767529}}

test vector.149 {myVec3 value set all 4.0} {
    list [catch {myVec3 value set all 4.0} msg] $msg
} {0 4.0}

test vector.150 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {4.0 4.0 4.0 4.0 4.0 4.0 4.0 4.0 4.0 4.0}}

test vector.151 {myVec3 value set all 0.0} {
    list [catch {myVec3 value set all 0.0} msg] $msg
} {0 0.0}

test vector.152 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0}}

test vector.153 {myVec3 value set 1:end 1.0} {
    list [catch {myVec3 value set 1:end 1.0} msg] $msg
} {0 1.0}

test vector.154 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0}}

test vector.155 {myVec3 value set 0:end 7.0} {
    list [catch {myVec3 value set 0:end 7.0} msg] $msg
} {0 7.0}

test vector.156 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {7.0 7.0 7.0 7.0 7.0 7.0 7.0 7.0 7.0 7.0}}

test vector.157 {myVec3 value set 0:end++ 2.0} {
    list [catch {myVec3 value set 0:end++ 2.0} msg] $msg
} {1 {bad index "end++"}}

test vector.158 {myVec3 length} {
    list [catch {myVec3 length} msg] $msg
} {0 10}

test vector.159 {myVec3 value set 0:++end 2.0} {
    list [catch {myVec3 value set 0:++end 2.0} msg] $msg
} {0 2.0}

test vector.160 {myVec3 length} {
    list [catch {myVec3 length} msg] $msg
} {0 11}

test vector.161 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0}}

test vector.162 {myVec3 delete end} {
    list [catch {myVec3 delete end} msg] $msg
} {0 {}}

test vector.163 {myVec3 value set ++end 0.0} {
    list [catch {myVec3 value set ++end 100.0} msg] $msg
} {0 100.0}

test vector.164 {myVec3 length} {
    list [catch {myVec3 length} msg] $msg
} {0 11}

test vector.165 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 100.0}}

test vector.166 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 100.0}}

test vector.167 {myVec3 value set end 0.0} {
    list [catch {myVec3 value set end 0.0} msg] $msg
} {0 0.0}

test vector.168 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 0.0}}

test vector.169 {myVec3 value set 0 0.0} {
    list [catch {myVec3 value set 0 0.0} msg] $msg
} {0 0.0}

test vector.170 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 0.0}}

test vector.171 {myVec3 delete end} {
    list [catch {myVec3 delete end} msg] $msg
} {0 {}}

test vector.172 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0 2.0}}

test vector.173 {myVec3 sequence 0 0} {
    list [catch {myVec3 sequence 0 0} msg] $msg
} {0 {}}

test vector.174 {myVec3 values} {
    list [catch {myVec3 values} msg] $msg
} {0 {0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0}}


test vector.175 {myVec1 sort -indices} {
    list [catch {myVec1 sort -indices} msg] $msg
} {0 {10 2 3}}

test vector.176 {myVec1 sort -decreasing} {
    list [catch {myVec1 sort -decreasing} msg] $msg
} {0 {4.0 3.0 1.0}}

test vector.177 {myVec1 sort -uniq} {
    list [catch {myVec1 sort -uniq} msg] $msg
} {0 {1.0 3.0 4.0}}

test vector.178 {myVec1 sort myVec1} {
    list [catch {myVec1 sort myVec1} msg] $msg
} {0 {}}

test vector.179 {myVec1 values} {
    list [catch {myVec1 values} msg] $msg
} {0 {1.0 3.0 4.0}}

test vector.180 {myVec1 length} {
    list [catch {myVec1 length} msg] $msg
} {0 3}

test vector.181 {myVec1 count empty} {
    list [catch {myVec1 count empty} msg] $msg
} {0 0}

test vector.182 {myVec1 count zero} {
    list [catch {myVec1 count zero} msg] $msg
} {0 0}

test vector.183 {myVec3 duplicate} {
    list [catch {myVec3 duplicate} msg] $msg
} {0 ::vector7}

test vector.184 {myVec3 duplicate myVec4} {
    list [catch {myVec3 duplicate myVec4} msg] $msg
} {0 ::myVec4}

test vector.185 {blt::vector expr myVec4==myVec3} {
    list [catch {blt::vector expr myVec4==myVec3} msg] $msg
} {0 {1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0}}

test vector.186 {blt::vector create myVec5 -size 20} {
    list [catch {blt::vector create myVec5 -size 20} msg] $msg
} {0 ::myVec5}

test vector.187 {myVec5 length} {
    list [catch {myVec5 length} msg] $msg
} {0 20}

test vector.188 {myVec5 values} {
    list [catch {myVec5 values} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.189 {blt::vector expr myVec5==myVec5} {
    list [catch {blt::vector expr myVec5==myVec5} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.190 {blt::vector destroy myVec4 vector7} {
    list [catch {blt::vector destroy myVec4 vector7} msg] $msg
} {0 {}}

test vector.191 {myVec set (missing arg)} {
    list [catch {myVec set} msg] $msg
} {1 {wrong # args: should be "myVec set list"}}

test vector.192 {myVec3 value badOp} {
    list [catch {myVec3 value badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myVec3 value get index
  myVec3 value set index value
  myVec3 value unset ?index...?}}

test vector.193 {myVec3 value badOp extraArgs } {
    list [catch {myVec3 value badOp extraArgs} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myVec3 value get index
  myVec3 value set index value
  myVec3 value unset ?index...?}}

test vector.194 {myVec3 sequence 1 10} {
    list [catch {myVec3 sequence 1 10} msg] $msg
} {0 {}}

test vector.195 {myVec3 value get } {
    list [catch {myVec3 value get} msg] $msg
} {1 {wrong # args: should be "myVec3 value get index"}}

test vector.196 {myVec3 value get badIndex} {
    list [catch {myVec3 value get badIndex} msg] $msg
} {1 {bad index "badIndex"}}

test vector.197 {myVec3 value get -1} {
    list [catch {myVec3 value get -1} msg] $msg
} {1 {index "-1" is out of range}}

test vector.198 {myVec3 value get ++end} {
    list [catch {myVec3 value get ++end} msg] $msg
} {1 {can't get index "++end"}}

test vector.199 {myVec3 value get min} {
    list [catch {myVec3 value get min} msg] $msg
} {1 {bad index "min"}}

test vector.200 {myVec3 value get max} {
    list [catch {myVec3 value get max} msg] $msg
} {1 {bad index "max"}}

test vector.201 {myVec3 value get 0} {
    list [catch {myVec3 value get 0} msg] $msg
} {0 1.0}

test vector.202 {myVec3 value get 1} {
    list [catch {myVec3 value get 1} msg] $msg
} {0 2.0}

test vector.203 {myVec3 value get end} {
    list [catch {myVec3 value get end} msg] $msg
} {0 10.0}

test vector.204 {myVec3 value get 0 extraArg} {
    list [catch {myVec3 value get 0 extraArg} msg] $msg
} {1 {wrong # args: should be "myVec3 value get index"}}

test vector.205 {myVec3 value set ++end 1000.0} {
    list [catch {myVec3 value set ++end 1000.0} msg] $msg
} {0 1000.0}

test vector.206 {myVec3 value unset end} {
    list [catch {myVec3 value unset end} msg] $msg
} {0 {}}

test vector.207 {myVec3 value get end} {
    list [catch {myVec3 value get end} msg] $msg
} {0 NaN}

test vector.208 {myVec3 value set all 1.0} {
    list [catch {myVec3 value set all 1.0} msg] $msg
} {0 1.0}

test vector.209 {myVec3 value unset all} {
    list [catch {myVec3 value unset all} msg] $msg
} {0 {}}

test vector.210 {myVec3 value get end} {
    list [catch {myVec3 value get end} msg] $msg
} {0 NaN}

test vector.211 {myVec3 value set all 1.0} {
    list [catch {myVec3 value set all 1.0} msg] $msg
} {0 1.0}

test vector.212 {myVec3 value unset 0:end} {
    list [catch {myVec3 value unset 0:end} msg] $msg
} {0 {}}

test vector.213 {myVec3 value get end} {
    list [catch {myVec3 value get end} msg] $msg
} {0 NaN}

test vector.3 {vector create myVec6} {
    list [catch {blt::vector create myVec6} msg] $msg
} {0 ::myVec6}

test vector.214 {myVec6 populate} {
    list [catch {myVec6 populate} msg] $msg
} {1 {wrong # args: should be "myVec6 populate vecName density"}}

test vector.214 {myVec6 populate myVec4} {
    list [catch {myVec6 populate myVec4} msg] $msg
} {1 {wrong # args: should be "myVec6 populate vecName density"}}

test vector.214 {myVec6 populate myVec3 10} {
    list [catch {myVec6 populate myVec3 10} msg] $msg
} {0 {}}

test vector.41 {myVec6 values} {
    list [catch {myVec6 values} msg] $msg
} {0 {NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN}}

test vector.214 {myVec6 populate myVec2 2} {
    list [catch {myVec6 populate myVec2 2} msg] $msg
} {0 {}}

test vector.41 {myVec6 values} {
    list [catch {myVec6 values} msg] $msg
} {0 {1.0 1.3333333333333333 1.6666666666666665 2.0 2.3333333333333335 2.6666666666666665 3.0 3.3333333333333335 3.6666666666666665 4.0 4.333333333333333 4.666666666666667 5.0 5.333333333333333 5.666666666666667 6.0 6.333333333333333 6.666666666666667 7.0 7.333333333333333 7.666666666666667 8.0 8.333333333333334 8.666666666666666 9.0 9.333333333333334 9.666666666666666 10.0 10.333333333333334 10.666666666666666 11.0 11.333333333333334 11.666666666666666 12.0 12.333333333333334 12.666666666666666 13.0 13.333333333333334 13.666666666666666 14.0 14.333333333333334 14.666666666666666 15.0 15.333333333333334 15.666666666666666 16.0 16.333333333333332 16.666666666666668 17.0 17.333333333333332 17.666666666666668 18.0 18.333333333333332 18.666666666666668 19.0 19.333333333333332 19.666666666666668 20.0}}

test vector.215 {myVec insert 0 -tags myTag} {
    list [catch {myVec insert 0 -tags myTag} msg] $msg
} {0 5}

test vector.216 {myVec insert 0 -tags {myTag1 myTag2} } {
    list [catch {myVec insert 0 -tags {myTag1 myTag2}} msg] $msg
} {0 6}

test vector.217 {myVec insert 0 -tags root} {
    list [catch {myVec insert 0 -tags root} msg] $msg
} {1 {can't add reserved tag "root"}}

test vector.218 {myVec insert 0 -tags (missing arg)} {
    list [catch {myVec insert 0 -tags} msg] $msg
} {1 {value for "-tags" missing}}

test vector.219 {myVec insert 0 -label myLabel -tags thisTag} {
    list [catch {myVec insert 0 -label myLabel -tags thisTag} msg] $msg
} {0 8}

test vector.220 {myVec insert 0 -label (missing arg)} {
    list [catch {myVec insert 0 -label} msg] $msg
} {1 {value for "-label" missing}}

test vector.221 {myVec insert 1 -tags thisTag} {
    list [catch {myVec insert 1 -tags thisTag} msg] $msg
} {0 9}

test vector.222 {myVec insert 1 -data key (missing value)} {
    list [catch {myVec insert 1 -data key} msg] $msg
} {1 {missing value for "key"}}

test vector.223 {myVec insert 1 -data {key value}} {
    list [catch {myVec insert 1 -data {key value}} msg] $msg
} {0 11}

test vector.224 {myVec insert 1 -data {key1 value1 key2 value2}} {
    list [catch {myVec insert 1 -data {key1 value1 key2 value2}} msg] $msg
} {0 12}

test vector.225 {get} {
    list [catch {
	myVec get 12
    } msg] $msg
} {0 {key1 value1 key2 value2}}

test vector.226 {myVec children} {
    list [catch {myVec children} msg] $msg
} {1 {wrong # args: should be "myVec children node ?first? ?last?"}}

test vector.227 {myVec children 0} {
    list [catch {myVec children 0} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test vector.228 {myVec children root} {
    list [catch {myVec children root} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test vector.229 {myVec children 1} {
    list [catch {myVec children 1} msg] $msg
} {0 {9 11 12}}

test vector.230 {myVec insert myTag} {
    list [catch {myVec insert myTag} msg] $msg
} {0 13}

test vector.231 {myVec children myTag} {
    list [catch {myVec children myTag} msg] $msg
} {0 13}

test vector.232 {myVec children root 0 end} {
    list [catch {myVec children root 0 end} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test vector.233 {myVec children root 2} {
    list [catch {myVec children root 2} msg] $msg
} {0 3}

test vector.234 {myVec children root 2 end} {
    list [catch {myVec children root 2 end} msg] $msg
} {0 {3 4 5 6 8}}

test vector.235 {myVec children root end end} {
    list [catch {myVec children root end end} msg] $msg
} {0 8}

test vector.236 {myVec children root 0 2} {
    list [catch {myVec children root 0 2} msg] $msg
} {0 {1 2 3}}

test vector.237 {myVec children root -1 -20} {
    list [catch {myVec children root -1 -20} msg] $msg
} {0 {}}

test vector.238 {myVec firstchild (missing arg)} {
    list [catch {myVec firstchild} msg] $msg
} {1 {wrong # args: should be "myVec firstchild node"}}

test vector.239 {myVec firstchild root} {
    list [catch {myVec firstchild root} msg] $msg
} {0 1}

test vector.240 {myVec lastchild (missing arg)} {
    list [catch {myVec lastchild} msg] $msg
} {1 {wrong # args: should be "myVec lastchild node"}}

test vector.241 {myVec lastchild root} {
    list [catch {myVec lastchild root} msg] $msg
} {0 8}

test vector.242 {myVec nextsibling (missing arg)} {
    list [catch {myVec nextsibling} msg] $msg
} {1 {wrong # args: should be "myVec nextsibling node"}}

test vector.243 {myVec nextsibling 1)} {
    list [catch {myVec nextsibling 1} msg] $msg
} {0 2}

test vector.244 {myVec nextsibling 2)} {
    list [catch {myVec nextsibling 2} msg] $msg
} {0 3}

test vector.245 {myVec nextsibling 3)} {
    list [catch {myVec nextsibling 3} msg] $msg
} {0 4}

test vector.246 {myVec nextsibling 4)} {
    list [catch {myVec nextsibling 4} msg] $msg
} {0 5}

test vector.247 {myVec nextsibling 5)} {
    list [catch {myVec nextsibling 5} msg] $msg
} {0 6}

test vector.248 {myVec nextsibling 6)} {
    list [catch {myVec nextsibling 6} msg] $msg
} {0 8}

test vector.249 {myVec nextsibling 8)} {
    list [catch {myVec nextsibling 8} msg] $msg
} {0 -1}

test vector.250 {myVec nextsibling all)} {
    list [catch {myVec nextsibling all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.251 {myVec nextsibling badTag)} {
    list [catch {myVec nextsibling badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::myVec}}

test vector.252 {myVec nextsibling -1)} {
    list [catch {myVec nextsibling -1} msg] $msg
} {1 {can't find tag or id "-1" in ::myVec}}

test vector.253 {myVec prevsibling 2)} {
    list [catch {myVec prevsibling 2} msg] $msg
} {0 1}

test vector.254 {myVec prevsibling 1)} {
    list [catch {myVec prevsibling 1} msg] $msg
} {0 -1}

test vector.255 {myVec prevsibling -1)} {
    list [catch {myVec prevsibling -1} msg] $msg
} {1 {can't find tag or id "-1" in ::myVec}}

test vector.256 {myVec root)} {
    list [catch {myVec root} msg] $msg
} {0 0}

test vector.257 {myVec root badArg)} {
    list [catch {myVec root badArgs} msg] $msg
} {1 {wrong # args: should be "myVec root "}}

test vector.258 {myVec parent (missing arg))} {
    list [catch {myVec parent} msg] $msg
} {1 {wrong # args: should be "myVec parent node"}}

test vector.259 {myVec parent root)} {
    list [catch {myVec parent root} msg] $msg
} {0 -1}

test vector.260 {myVec parent 1)} {
    list [catch {myVec parent 1} msg] $msg
} {0 0}

test vector.261 {myVec parent myTag)} {
    list [catch {myVec parent myTag} msg] $msg
} {0 0}

test vector.262 {myVec next (missing arg))} {
    list [catch {myVec next} msg] $msg
} {1 {wrong # args: should be "myVec next node"}}


test vector.263 {myVec next (extra arg))} {
    list [catch {myVec next root root} msg] $msg
} {1 {wrong # args: should be "myVec next node"}}

test vector.264 {myVec next root} {
    list [catch {myVec next root} msg] $msg
} {0 1}

test vector.265 {myVec next 1)} {
    list [catch {myVec next 1} msg] $msg
} {0 9}

test vector.266 {myVec next 2)} {
    list [catch {myVec next 2} msg] $msg
} {0 3}

test vector.267 {myVec next 3)} {
    list [catch {myVec next 3} msg] $msg
} {0 4}

test vector.268 {myVec next 4)} {
    list [catch {myVec next 4} msg] $msg
} {0 5}

test vector.269 {myVec next 5)} {
    list [catch {myVec next 5} msg] $msg
} {0 13}

test vector.270 {myVec next 6)} {
    list [catch {myVec next 6} msg] $msg
} {0 8}

test vector.271 {myVec next 8)} {
    list [catch {myVec next 8} msg] $msg
} {0 -1}

test vector.272 {myVec previous 1)} {
    list [catch {myVec previous 1} msg] $msg
} {0 0}

test vector.273 {myVec previous 0)} {
    list [catch {myVec previous 0} msg] $msg
} {0 -1}

test vector.274 {myVec previous 8)} {
    list [catch {myVec previous 8} msg] $msg
} {0 6}

test vector.275 {myVec depth (no arg))} {
    list [catch {myVec depth} msg] $msg
} {1 {wrong # args: should be "myVec depth node"}}

test vector.276 {myVec depth root))} {
    list [catch {myVec depth root} msg] $msg
} {0 0}

test vector.277 {myVec depth myTag))} {
    list [catch {myVec depth myTag} msg] $msg
} {0 1}

test vector.278 {myVec depth myTag))} {
    list [catch {myVec depth myTag} msg] $msg
} {0 1}

test vector.279 {myVec dump (missing arg)))} {
    list [catch {myVec dump} msg] $msg
} {1 {wrong # args: should be "myVec dump node"}}

test vector.280 {myVec dump root} {
    list [catch {myVec dump root} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test vector.281 {myVec dump 1} {
    list [catch {myVec dump 1} msg] $msg
} {0 {-1 1 {node1} {} {}
1 9 {node1 node9} {} {thisTag}
1 11 {node1 node11} {key value} {}
1 12 {node1 node12} {key1 value1 key2 value2} {}
}}

test vector.282 {myVec dump this} {
    list [catch {myVec dump myTag} msg] $msg
} {0 {-1 5 {node5} {} {myTag}
5 13 {node5 node13} {} {}
}}

test vector.283 {myVec dump 1 badArg (too many args)} {
    list [catch {myVec dump 1 badArg} msg] $msg
} {1 {wrong # args: should be "myVec dump node"}}

test vector.284 {myVec dump 11} {
    list [catch {myVec dump 11} msg] $msg
} {0 {-1 11 {node11} {key value} {}
}}

test vector.285 {myVec dump all} {
    list [catch {myVec dump all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.286 {myVec dump all} {
    list [catch {myVec dump all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.287 {myVec dumpfile 0 test.dump} {
    list [catch {myVec dumpfile 0 test.dump} msg] $msg
} {0 {}}

test vector.288 {myVec get 9} {
    list [catch {myVec get 9} msg] $msg
} {0 {}}

test vector.289 {myVec get all} {
    list [catch {myVec get all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.290 {myVec get root} {
    list [catch {myVec get root} msg] $msg
} {0 {}}

test vector.291 {myVec get 9 key} {
    list [catch {myVec get root} msg] $msg
} {0 {}}

test vector.292 {myVec get 12} {
    list [catch {myVec get 12} msg] $msg
} {0 {key1 value1 key2 value2}}

test vector.293 {myVec get 12 key1} {
    list [catch {myVec get 12 key1} msg] $msg
} {0 value1}

test vector.294 {myVec get 12 key2} {
    list [catch {myVec get 12 key2} msg] $msg
} {0 value2}

test vector.295 {myVec get 12 key1 defValue } {
    list [catch {myVec get 12 key1 defValue} msg] $msg
} {0 value1}

test vector.296 {myVec get 12 key100 defValue } {
    list [catch {myVec get 12 key100 defValue} msg] $msg
} {0 defValue}

test vector.297 {myVec value (missing arg) } {
    list [catch {myVec value} msg] $msg
} {1 {wrong # args: should be "myVec value label|list"}}

test vector.298 {myVec value 0 10 (extra arg) } {
    list [catch {myVec value 0 10} msg] $msg
} {1 {wrong # args: should be "myVec index label|list"}}

test vector.299 {myVec index 0} {
    list [catch {myVec index 0} msg] $msg
} {0 0}

test vector.300 {myVec index root} {
    list [catch {myVec index root} msg] $msg
} {0 0}

test vector.301 {myVec index all} {
    list [catch {myVec index all} msg] $msg
} {0 -1}

test vector.302 {myVec index myTag} {
    list [catch {myVec index myTag} msg] $msg
} {0 5}

test vector.303 {myVec index thisTag} {
    list [catch {myVec index thisTag} msg] $msg
} {0 -1}

test vector.304 {myVec is (no args)} {
    list [catch {myVec is} msg] $msg
} {1 {wrong # args: should be one of...
  myVec is ancestor node1 node2
  myVec is before node1 node2
  myVec is leaf node
  myVec is root node}}

test vector.305 {myVec is badOp} {
    list [catch {myVec is badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myVec is ancestor node1 node2
  myVec is before node1 node2
  myVec is leaf node
  myVec is root node}}

test vector.306 {myVec is before} {
    list [catch {myVec is before} msg] $msg
} {1 {wrong # args: should be "myVec is before node1 node2"}}

test vector.307 {myVec is before 0 10 20} {
    list [catch {myVec is before 0 10 20} msg] $msg
} {1 {wrong # args: should be "myVec is before node1 node2"}}

test vector.308 {myVec is before 0 12} {
    list [catch {myVec is before 0 12} msg] $msg
} {0 1}

test vector.309 {myVec is before 12 0} {
    list [catch {myVec is before 12 0} msg] $msg
} {0 0}

test vector.310 {myVec is before 0 0} {
    list [catch {myVec is before 0 0} msg] $msg
} {0 0}

test vector.311 {myVec is before root 0} {
    list [catch {myVec is before root 0} msg] $msg
} {0 0}

test vector.312 {myVec is before 0 all} {
    list [catch {myVec is before 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.313 {myVec is ancestor} {
    list [catch {myVec is ancestor} msg] $msg
} {1 {wrong # args: should be "myVec is ancestor node1 node2"}}

test vector.314 {myVec is ancestor 0 12 20} {
    list [catch {myVec is ancestor 0 12 20} msg] $msg
} {1 {wrong # args: should be "myVec is ancestor node1 node2"}}

test vector.315 {myVec is ancestor 0 12} {
    list [catch {myVec is ancestor 0 12} msg] $msg
} {0 1}

test vector.316 {myVec is ancestor 12 0} {
    list [catch {myVec is ancestor 12 0} msg] $msg
} {0 0}

test vector.317 {myVec is ancestor 1 2} {
    list [catch {myVec is ancestor 1 2} msg] $msg
} {0 0}

test vector.318 {myVec is ancestor root 0} {
    list [catch {myVec is ancestor root 0} msg] $msg
} {0 0}

test vector.319 {myVec is ancestor 0 all} {
    list [catch {myVec is ancestor 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.320 {myVec is root (missing arg)} {
    list [catch {myVec is root} msg] $msg
} {1 {wrong # args: should be "myVec is root node"}}

test vector.321 {myVec is root 0 20 (extra arg)} {
    list [catch {myVec is root 0 20} msg] $msg
} {1 {wrong # args: should be "myVec is root node"}}

test vector.322 {myVec is root 0} {
    list [catch {myVec is root 0} msg] $msg
} {0 1}

test vector.323 {myVec is root 12} {
    list [catch {myVec is root 12} msg] $msg
} {0 0}

test vector.324 {myVec is root 1} {
    list [catch {myVec is root 1} msg] $msg
} {0 0}

test vector.325 {myVec is root root} {
    list [catch {myVec is root root} msg] $msg
} {0 1}

test vector.326 {myVec is root all} {
    list [catch {myVec is root all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.327 {myVec is leaf (missing arg)} {
    list [catch {myVec is leaf} msg] $msg
} {1 {wrong # args: should be "myVec is leaf node"}}

test vector.328 {myVec is leaf 0 20 (extra arg)} {
    list [catch {myVec is leaf 0 20} msg] $msg
} {1 {wrong # args: should be "myVec is leaf node"}}

test vector.329 {myVec is leaf 0} {
    list [catch {myVec is leaf 0} msg] $msg
} {0 0}

test vector.330 {myVec is leaf 12} {
    list [catch {myVec is leaf 12} msg] $msg
} {0 1}

test vector.331 {myVec is leaf 1} {
    list [catch {myVec is leaf 1} msg] $msg
} {0 0}

test vector.332 {myVec is leaf root} {
    list [catch {myVec is leaf root} msg] $msg
} {0 0}

test vector.333 {myVec is leaf all} {
    list [catch {myVec is leaf all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.334 {myVec is leaf 1000} {
    list [catch {myVec is leaf 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::myVec}}

test vector.335 {myVec is leaf badTag} {
    list [catch {myVec is leaf badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::myVec}}

test vector.336 {myVec set (missing arg)} {
    list [catch {myVec set} msg] $msg
} {1 {wrong # args: should be "myVec set node ?key value...?"}}

test vector.337 {myVec set 0 (missing arg)} {
    list [catch {myVec set 0} msg] $msg
} {0 {}}

test vector.338 {myVec set 0 key (missing arg)} {
    list [catch {myVec set 0 key} msg] $msg
} {1 {missing value for field "key"}}

test vector.339 {myVec set 0 key value} {
    list [catch {myVec set 0 key value} msg] $msg
} {0 {}}

test vector.340 {myVec set 0 key1 value1 key2 value2 key3 value3} {
    list [catch {myVec set 0 key1 value1 key2 value2 key3 value3} msg] $msg
} {0 {}}

test vector.341 {myVec set 0 key1 value1 key2 (missing arg)} {
    list [catch {myVec set 0 key1 value1 key2} msg] $msg
} {1 {missing value for field "key2"}}

test vector.342 {myVec set 0 key value} {
    list [catch {myVec set 0 key value} msg] $msg
} {0 {}}

test vector.343 {myVec set 0 key1 value1 key2 (missing arg)} {
    list [catch {myVec set 0 key1 value1 key2} msg] $msg
} {1 {missing value for field "key2"}}

test vector.344 {myVec set all} {
    list [catch {myVec set all} msg] $msg
} {0 {}}

test vector.345 {myVec set all abc 123} {
    list [catch {myVec set all abc 123} msg] $msg
} {0 {}}

test vector.346 {myVec set root} {
    list [catch {myVec set root} msg] $msg
} {0 {}}

test vector.347 {myVec restore stuff} {
    list [catch {
	set data [myVec dump root]
	blt::vector create
	vector1 restore root $data
	set data [vector1 dump root]
	blt::vector destroy vector1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {key value key1 value1 key2 value2 key3 value3 abc 123} {}
0 1 {{} node1} {abc 123} {}
1 9 {{} node1 node9} {abc 123} {thisTag}
1 11 {{} node1 node11} {key value abc 123} {}
1 12 {{} node1 node12} {key1 value1 key2 value2 abc 123} {}
0 2 {{} node2} {abc 123} {}
0 3 {{} node3} {abc 123} {}
0 4 {{} node4} {abc 123} {}
0 5 {{} node5} {abc 123} {myTag}
5 13 {{} node5 node13} {abc 123} {}
0 6 {{} node6} {abc 123} {myTag2 myTag1}
0 8 {{} myLabel} {abc 123} {thisTag}
}}

test vector.348 {myVec restorefile 0 test.dump} {
    list [catch {
	blt::vector create
	vector1 restorefile root test.dump
	set data [vector1 dump root]
	blt::vector destroy vector1
	file delete test.dump
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}


test vector.349 {myVec unset 0 key1} {
    list [catch {myVec unset 0 key1} msg] $msg
} {0 {}}

test vector.350 {myVec get 0} {
    list [catch {myVec get 0} msg] $msg
} {0 {key value key2 value2 key3 value3 abc 123}}

test vector.351 {myVec unset 0 key2 key3} {
    list [catch {myVec unset 0 key2 key3} msg] $msg
} {0 {}}

test vector.352 {myVec get 0} {
    list [catch {myVec get 0} msg] $msg
} {0 {key value abc 123}}

test vector.353 {myVec unset 0} {
    list [catch {myVec unset 0} msg] $msg
} {0 {}}

test vector.354 {myVec get 0} {
    list [catch {myVec get 0} msg] $msg
} {0 {}}

test vector.355 {myVec unset all abc} {
    list [catch {myVec unset all abc} msg] $msg
} {0 {}}

test vector.356 {myVec restore stuff} {
    list [catch {
	set data [myVec dump root]
	blt::vector create vector1
	vector1 restore root $data
	set data [vector1 dump root]
	blt::vector destroy vector1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test vector.357 {myVec restore (missing arg)} {
    list [catch {myVec restore} msg] $msg
} {1 {wrong # args: should be "myVec restore node data ?switches?"}}

test vector.358 {myVec restore 0 badString} {
    list [catch {myVec restore 0 badString} msg] $msg
} {1 {line #1: wrong # elements in restore entry}}

test vector.359 {myVec restore 0 {} arg (extra arg)} {
    list [catch {myVec restore 0 {} arg} msg] $msg
} {1 {unknown switch "arg"
following switches are available:
   -notags 
   -overwrite }}


test vector.360 {myVec size (missing arg)} {
    list [catch {myVec size} msg] $msg
} {1 {wrong # args: should be "myVec size node"}}

test vector.361 {myVec size 0} {
    list [catch {myVec size 0} msg] $msg
} {0 12}

test vector.362 {myVec size all} {
    list [catch {myVec size all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.363 {myVec size 0 10 (extra arg)} {
    list [catch {myVec size 0 10} msg] $msg
} {1 {wrong # args: should be "myVec size node"}}

test vector.364 {myVec delete (no args)} {
    list [catch {myVec delete} msg] $msg
} {0 {}}

test vector.365 {myVec delete 11} {
    list [catch {myVec delete 11} msg] $msg
} {0 {}}

test vector.366 {myVec delete 11} {
    list [catch {myVec delete 11} msg] $msg
} {1 {can't find tag or id "11" in ::myVec}}

test vector.367 {myVec delete 9 12} {
    list [catch {myVec delete 9 12} msg] $msg
} {0 {}}

test vector.368 {myVec dump 0} {
    list [catch {myVec dump 0} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test vector.369 {delete all} {
    list [catch {
	set data [myVec dump root]
	blt::vector create
	vector1 restore root $data
	vector1 delete all
	set data [vector1 dump root]
	blt::vector destroy vector1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
}}

test vector.370 {delete all all} {
    list [catch {
	set data [myVec dump root]
	blt::vector create
	vector1 restore root $data
	vector1 delete all all
	set data [vector1 dump root]
	blt::vector destroy vector1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
}}

test vector.371 {myVec apply (missing arg)} {
    list [catch {myVec apply} msg] $msg
} {1 {wrong # args: should be "myVec apply node ?switches?"}}

test vector.372 {myVec apply 0} {
    list [catch {myVec apply 0} msg] $msg
} {0 {}}

test vector.373 {myVec apply 0 -badSwitch} {
    list [catch {myVec apply 0 -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -precommand command
   -postcommand command
   -depth number
   -exact string
   -glob pattern
   -invert 
   -key pattern
   -keyexact string
   -keyglob pattern
   -keyregexp pattern
   -leafonly 
   -nocase 
   -path 
   -regexp pattern
   -tag {?tag?...}}}


test vector.374 {myVec apply badTag} {
    list [catch {myVec apply badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::myVec}}

test vector.375 {myVec apply all} {
    list [catch {myVec apply all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.376 {myVec apply myTag -precommand lappend} {
    list [catch {
	set mylist {}
	myVec apply myTag -precommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {5 13}}

test vector.377 {myVec apply root -precommand lappend} {
    list [catch {
	set mylist {}
	myVec apply root -precommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {0 1 2 3 4 5 13 6 8}}

test vector.378 {myVec apply -precommand -postcommand} {
    list [catch {
	set mylist {}
	myVec apply root -precommand {lappend mylist} \
		-postcommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {0 1 1 2 2 3 3 4 4 5 13 13 5 6 6 8 8 0}}

test vector.379 {myVec apply root -precommand lappend -depth 1} {
    list [catch {
	set mylist {}
	myVec apply root -precommand {lappend mylist} -depth 1
	set mylist
    } msg] $msg
} {0 {0 1 2 3 4 5 6 8}}


test vector.380 {myVec apply root -precommand -depth 0} {
    list [catch {
	set mylist {}
	myVec apply root -precommand {lappend mylist} -depth 0
	set mylist
    } msg] $msg
} {0 0}

test vector.381 {myVec apply root -precommand -tag myTag} {
    list [catch {
	set mylist {}
	myVec apply root -precommand {lappend mylist} -tag myTag
	set mylist
    } msg] $msg
} {0 5}


test vector.382 {myVec apply root -precommand -key key1} {
    list [catch {
	set mylist {}
	myVec set myTag key1 0.0
	myVec apply root -precommand {lappend mylist} -key key1
	myVec unset myTag key1
	set mylist
    } msg] $msg
} {0 5}

test vector.383 {myVec apply root -postcommand -regexp node.*} {
    list [catch {
	set mylist {}
	myVec set myTag key1 0.0
	myVec apply root -precommand {lappend mylist} -regexp {node5} 
	myVec unset myTag key1
	set mylist
    } msg] $msg
} {0 5}

test vector.384 {myVec find (missing arg)} {
    list [catch {myVec find} msg] $msg
} {1 {wrong # args: should be "myVec find node ?switches?"}}

test vector.385 {myVec find 0} {
    list [catch {myVec find 0} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test vector.386 {myVec find root} {
    list [catch {myVec find root} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test vector.387 {myVec find 0 -glob node*} {
    list [catch {myVec find root -glob node*} msg] $msg
} {0 {1 2 3 4 13 5 6}}

test vector.388 {myVec find 0 -glob nobody} {
    list [catch {myVec find root -glob nobody} msg] $msg
} {0 {}}

test vector.389 {myVec find 0 -regexp {node[0-3]}} {
    list [catch {myVec find root -regexp {node[0-3]}} msg] $msg
} {0 {1 2 3 13}}

test vector.390 {myVec find 0 -regexp {.*[A-Z].*}} {
    list [catch {myVec find root -regexp {.*[A-Z].*}} msg] $msg
} {0 8}

test vector.391 {myVec find 0 -exact myLabel} {
    list [catch {myVec find root -exact myLabel} msg] $msg
} {0 8}

test vector.392 {myVec find 0 -exact myLabel -invert} {
    list [catch {myVec find root -exact myLabel -invert} msg] $msg
} {0 {1 2 3 4 13 5 6 0}}


test vector.393 {myVec find 3 -exact node3} {
    list [catch {myVec find 3 -exact node3} msg] $msg
} {0 3}

test vector.394 {myVec find 0 -nocase -exact mylabel} {
    list [catch {myVec find 0 -nocase -exact mylabel} msg] $msg
} {0 8}

test vector.395 {myVec find 0 -nocase} {
    list [catch {myVec find 0 -nocase} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test vector.396 {myVec find 0 -path -nocase -glob *node1* } {
    list [catch {myVec find 0 -path -nocase -glob *node1*} msg] $msg
} {0 {1 13}}

test vector.397 {myVec find 0 -count 5 } {
    list [catch {myVec find 0 -count 5} msg] $msg
} {0 {1 2 3 4 13}}

test vector.398 {myVec find 0 -count -5 } {
    list [catch {myVec find 0 -count -5} msg] $msg
} {1 {bad value "-5": can't be negative}}

test vector.399 {myVec find 0 -count badValue } {
    list [catch {myVec find 0 -count badValue} msg] $msg
} {1 {expected integer but got "badValue"}}

test vector.400 {myVec find 0 -count badValue } {
    list [catch {myVec find 0 -count badValue} msg] $msg
} {1 {expected integer but got "badValue"}}

test vector.401 {myVec find 0 -leafonly} {
    list [catch {myVec find 0 -leafonly} msg] $msg
} {0 {1 2 3 4 13 6 8}}

test vector.402 {myVec find 0 -leafonly -glob {node[18]}} {
    list [catch {myVec find 0 -glob {node[18]} -leafonly} msg] $msg
} {0 1}

test vector.403 {myVec find 0 -depth 0} {
    list [catch {myVec find 0 -depth 0} msg] $msg
} {0 0}

test vector.404 {myVec find 0 -depth 1} {
    list [catch {myVec find 0 -depth 1} msg] $msg
} {0 {1 2 3 4 5 6 8 0}}

test vector.405 {myVec find 0 -depth 2} {
    list [catch {myVec find 0 -depth 2} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test vector.406 {myVec find 0 -depth 20} {
    list [catch {myVec find 0 -depth 20} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test vector.407 {myVec find 1 -depth 0} {
    list [catch {myVec find 1 -depth 0} msg] $msg
} {0 1}

test vector.408 {myVec find 1 -depth 1} {
    list [catch {myVec find 1 -depth 1} msg] $msg
} {0 1}

test vector.409 {myVec find 1 -depth 2} {
    list [catch {myVec find 1 -depth 2} msg] $msg
} {0 1}

test vector.410 {myVec find all} {
    list [catch {myVec find all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.411 {myVec find badTag} {
    list [catch {myVec find badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::myVec}}

test vector.412 {myVec find 0 -addtag hi} {
    list [catch {myVec find 0 -addtag hi} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test vector.413 {myVec find 0 -addtag all} {
    list [catch {myVec find 0 -addtag all} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test vector.414 {myVec find 0 -addtag root} {
    list [catch {myVec find 0 -addtag root} msg] $msg
} {1 {can't add reserved tag "root"}}

test vector.415 {myVec find 0 -exec {lappend list} -leafonly} {
    list [catch {
	set list {}
	myVec find 0 -exec {lappend list} -leafonly
	set list
	} msg] $msg
} {0 {1 2 3 4 13 6 8}}

test vector.416 {myVec find 0 -tag root} {
    list [catch {myVec find 0 -tag root} msg] $msg
} {0 0}

test vector.417 {myVec find 0 -tag myTag} {
    list [catch {myVec find 0 -tag myTag} msg] $msg
} {0 5}

test vector.418 {myVec find 0 -tag badTag} {
    list [catch {myVec find 0 -tag badTag} msg] $msg
} {0 {}}

test vector.419 {myVec tag (missing args)} {
    list [catch {myVec tag} msg] $msg
} {1 {wrong # args: should be "myVec tag args..."}}

test vector.420 {myVec tag badOp} {
    list [catch {myVec tag badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myVec tag add tag ?node...?
  myVec tag delete tag node...
  myVec tag dump tag...
  myVec tag exists tag ?node?
  myVec tag forget tag...
  myVec tag get node ?pattern...?
  myVec tag names ?node...?
  myVec tag nodes tag ?tag...?
  myVec tag set node tag...
  myVec tag unset node tag...}}

test vector.421 {myVec tag add} {
    list [catch {myVec tag add} msg] $msg
} {1 {wrong # args: should be "myVec tag add tag ?node...?"}}

test vector.422 {myVec tag add newTag} {
    list [catch {myVec tag add newTag} msg] $msg
} {0 {}}

test vector.423 {myVec tag add tag badNode} {
    list [catch {myVec tag add tag badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::myVec}}

test vector.424 {myVec tag add newTag root} {
    list [catch {myVec tag add newTag root} msg] $msg
} {0 {}}

test vector.425 {myVec tag add newTag all} {
    list [catch {myVec tag add newTag all} msg] $msg
} {0 {}}

test vector.426 {myVec tag add tag2 0 1 2 3 4} {
    list [catch {myVec tag add tag2 0 1 2 3 4} msg] $msg
} {0 {}}

test vector.427 {myVec tag exists tag2} {
    list [catch {myVec tag exists tag2} msg] $msg
} {0 1}

test vector.428 {myVec tag exists tag2 0} {
    list [catch {myVec tag exists tag2 0} msg] $msg
} {0 1}

test vector.429 {myVec tag exists tag2 5} {
    list [catch {myVec tag exists tag2 5} msg] $msg
} {0 0}

test vector.430 {myVec tag exists badTag} {
    list [catch {myVec tag exists badTag} msg] $msg
} {0 0}

test vector.431 {myVec tag exists badTag 1000} {
    list [catch {myVec tag exists badTag 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::myVec}}

test vector.432 {myVec tag add tag2 0 1 2 3 4 1000} {
    list [catch {myVec tag add tag2 0 1 2 3 4 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::myVec}}

test vector.433 {myVec tag names} {
    list [catch {myVec tag names} msg] [lsort $msg]
} {0 {all hi myTag myTag1 myTag2 newTag root tag2 thisTag}}

test vector.434 {myVec tag names badNode} {
    list [catch {myVec tag names badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::myVec}}

test vector.435 {myVec tag names all} {
    list [catch {myVec tag names all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.436 {myVec tag names root} {
    list [catch {myVec tag names root} msg] [lsort $msg]
} {0 {all hi newTag root tag2}}

test vector.437 {myVec tag names 0 1} {
    list [catch {myVec tag names 0 1} msg] [lsort $msg]
} {0 {all hi newTag root tag2}}

test vector.438 {myVec tag nodes (missing arg)} {
    list [catch {myVec tag nodes} msg] $msg
} {1 {wrong # args: should be "myVec tag nodes tag ?tag...?"}}

test vector.439 {myVec tag nodes root badTag} {
    # It's not an error to use bad tag.
    list [catch {myVec tag nodes root badTag} msg] $msg
} {0 {}}

test vector.440 {myVec tag nodes root tag2} {
    list [catch {myVec tag nodes root tag2} msg] [lsort $msg]
} {0 {0 1 2 3 4}}

test vector.441 {myVec ancestor (missing arg)} {
    list [catch {myVec ancestor} msg] $msg
} {1 {wrong # args: should be "myVec ancestor node1 node2"}}

test vector.442 {myVec ancestor 0 (missing arg)} {
    list [catch {myVec ancestor 0} msg] $msg
} {1 {wrong # args: should be "myVec ancestor node1 node2"}}

test vector.443 {myVec ancestor 0 10} {
    list [catch {myVec ancestor 0 10} msg] $msg
} {1 {can't find tag or id "10" in ::myVec}}

test vector.444 {myVec ancestor 0 4} {
    list [catch {myVec ancestor 0 4} msg] $msg
} {0 0}

test vector.445 {myVec ancestor 1 8} {
    list [catch {myVec ancestor 1 8} msg] $msg
} {0 0}

test vector.446 {myVec ancestor root 0} {
    list [catch {myVec ancestor root 0} msg] $msg
} {0 0}

test vector.447 {myVec ancestor 8 8} {
    list [catch {myVec ancestor 8 8} msg] $msg
} {0 8}

test vector.448 {myVec ancestor 0 all} {
    list [catch {myVec ancestor 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.449 {myVec ancestor 7 9} {
    list [catch {
	set n1 1; set n2 1;
	for { set i 0 } { $i < 4 } { incr i } {
	    set n1 [myVec insert $n1]
	    set n2 [myVec insert $n2]
	}
	myVec ancestor $n1 $n2
	} msg] $msg
} {0 1}

test vector.450 {myVec path (missing arg)} {
    list [catch {myVec path} msg] $msg
} {1 {wrong # args: should be "myVec path node"}}

test vector.451 {myVec path root} {
    list [catch {myVec path root} msg] $msg
} {0 {}}

test vector.452 {myVec path 0} {
    list [catch {myVec path 0} msg] $msg
} {0 {}}

test vector.453 {myVec path 15} {
    list [catch {myVec path 15} msg] $msg
} {0 {node1 node15}}

test vector.454 {myVec path all} {
    list [catch {myVec path all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.455 {myVec path 0 1 2 4 (extra args)} {
    list [catch {myVec path 0 1 2 4} msg] $msg
} {1 {wrong # args: should be "myVec path node"}}

test vector.456 {myVec tag forget} {
    list [catch {myVec tag forget} msg] $msg
} {1 {wrong # args: should be "myVec tag forget tag..."}}

test vector.457 {myVec tag forget badTag} {
    list [catch {
	myVec tag forget badTag
	lsort [myVec tag names]
    } msg] $msg
} {0 {all hi myTag myTag1 myTag2 newTag root tag2 thisTag}}

test vector.458 {myVec tag forget hi} {
    list [catch {
	myVec tag forget hi
	lsort [myVec tag names]
    } msg] $msg
} {0 {all myTag myTag1 myTag2 newTag root tag2 thisTag}}

test vector.459 {myVec tag forget tag1 tag2} {
    list [catch {
	myVec tag forget myTag1 myTag2
	lsort [myVec tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test vector.460 {myVec tag forget all} {
    list [catch {
	myVec tag forget all
	lsort [myVec tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test vector.461 {myVec tag forget root} {
    list [catch {
	myVec tag forget root
	lsort [myVec tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test vector.462 {myVec tag delete} {
    list [catch {myVec tag delete} msg] $msg
} {1 {wrong # args: should be "myVec tag delete tag node..."}}

test vector.463 {myVec tag delete tag} {
    list [catch {myVec tag delete tag} msg] $msg
} {1 {wrong # args: should be "myVec tag delete tag node..."}}

test vector.464 {myVec tag delete tag 0} {
    list [catch {myVec tag delete tag 0} msg] $msg
} {0 {}}

test vector.465 {myVec tag delete root 0} {
    list [catch {myVec tag delete root 0} msg] $msg
} {1 {can't delete reserved tag "root"}}

test vector.466 {myVec move} {
    list [catch {myVec move} msg] $msg
} {1 {wrong # args: should be "myVec move node newParent ?switches?"}}

test vector.467 {myVec move 0} {
    list [catch {myVec move 0} msg] $msg
} {1 {wrong # args: should be "myVec move node newParent ?switches?"}}

test vector.468 {myVec move 0 0} {
    list [catch {myVec move 0 0} msg] $msg
} {1 {can't move root node}}

test vector.469 {myVec move 0 badNode} {
    list [catch {myVec move 0 badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::myVec}}

test vector.470 {myVec move 0 all} {
    list [catch {myVec move 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.471 {myVec move 1 0 -before 2} {
    list [catch {
	myVec move 1 0 -before 2
	myVec children 0
    } msg] $msg
} {0 {1 2 3 4 5 6 8}}

test vector.472 {myVec move 1 0 -after 2} {
    list [catch {
	myVec move 1 0 -after 2
	myVec children 0
    } msg] $msg
} {0 {2 1 3 4 5 6 8}}

test vector.473 {myVec move 1 2} {
    list [catch {
	myVec move 1 2
	myVec children 0
    } msg] $msg
} {0 {2 3 4 5 6 8}}

test vector.474 {myVec move 0 2} {
    list [catch {myVec move 0 2} msg] $msg
} {1 {can't move root node}}

test vector.475 {myVec move 1 17} {
    list [catch {myVec move 1 17} msg] $msg
} {1 {can't move node: "1" is an ancestor of "17"}}

test vector.476 {myVec attach} {
    list [catch {myVec attach} msg] $msg
} {1 {wrong # args: should be "myVec attach vector ?switches?"}}

test vector.477 {myVec attach vector2 badArg} {
    list [catch {myVec attach vector2 badArg} msg] $msg
} {1 {unknown switch "badArg"
following switches are available:
   -newtags }}


test vector.478 {vector1 attach myVec -newtags} {
    list [catch {
	blt::vector create
	vector1 attach myVec -newtags
	vector1 dump 0
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 2 {{} node2} {} {}
2 1 {{} node2 node1} {} {}
1 14 {{} node2 node1 node14} {} {}
14 16 {{} node2 node1 node14 node16} {} {}
16 18 {{} node2 node1 node14 node16 node18} {} {}
18 20 {{} node2 node1 node14 node16 node18 node20} {} {}
1 15 {{} node2 node1 node15} {} {}
15 17 {{} node2 node1 node15 node17} {} {}
17 19 {{} node2 node1 node15 node17 node19} {} {}
19 21 {{} node2 node1 node15 node17 node19 node21} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {}
0 8 {{} myLabel} {} {}
}}

test vector.479 {vector1 attach myVec} {
    list [catch {
	blt::vector create
	vector1 attach myVec
	vector1 dump 0
	} msg] $msg
} {0 {-1 0 {{}} {} {tag2 newTag}
0 2 {{} node2} {} {tag2 newTag}
2 1 {{} node2 node1} {} {tag2 newTag}
1 14 {{} node2 node1 node14} {} {}
14 16 {{} node2 node1 node14 node16} {} {}
16 18 {{} node2 node1 node14 node16 node18} {} {}
18 20 {{} node2 node1 node14 node16 node18 node20} {} {}
1 15 {{} node2 node1 node15} {} {}
15 17 {{} node2 node1 node15 node17} {} {}
17 19 {{} node2 node1 node15 node17 node19} {} {}
19 21 {{} node2 node1 node15 node17 node19 node21} {} {}
0 3 {{} node3} {} {tag2 newTag}
0 4 {{} node4} {} {tag2 newTag}
0 5 {{} node5} {} {newTag myTag}
5 13 {{} node5 node13} {} {newTag}
0 6 {{} node6} {} {newTag}
0 8 {{} myLabel} {} {thisTag newTag}
}}

test vector.480 {vector1 attach ""} {
    list [catch {vector1 attach ""} msg] $msg
} {0 {}}


test vector.481 {blt::vector destroy vector1} {
    list [catch {blt::vector destroy vector1} msg] $msg
} {0 {}}

test vector.482 {myVec find root -badSwitch} {
    list [catch {myVec find root -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -addtag tagName
   -count number
   -depth number
   -exact string
   -excludes nodes
   -exec command
   -glob pattern
   -invert 
   -key string
   -keyexact string
   -keyglob pattern
   -keyregexp pattern
   -leafonly 
   -nocase 
   -order order
   -path 
   -regexp pattern
   -tag {?tag?...}}}


test vector.483 {myVec find root -order} {
    list [catch {myVec find root -order} msg] $msg
} {1 {value for "-order" missing}}

test vector.484 {myVec find root ...} {
    list [catch {myVec find root -order preorder -order postorder -order inorder} msg] $msg
} {0 {20 18 16 14 1 21 19 17 15 2 0 3 4 13 5 6 8}}

test vector.485 {myVec find root -order preorder} {
    list [catch {myVec find root -order preorder} msg] $msg
} {0 {0 2 1 14 16 18 20 15 17 19 21 3 4 5 13 6 8}}

test vector.486 {myVec find root -order postorder} {
    list [catch {myVec find root -order postorder} msg] $msg
} {0 {20 18 16 14 21 19 17 15 1 2 3 4 13 5 6 8 0}}

test vector.487 {myVec find root -order inorder} {
    list [catch {myVec find root -order inorder} msg] $msg
} {0 {20 18 16 14 1 21 19 17 15 2 0 3 4 13 5 6 8}}

test vector.488 {myVec find root -order breadthfirst} {
    list [catch {myVec find root -order breadthfirst} msg] $msg
} {0 {0 2 3 4 5 6 8 1 13 14 15 16 17 18 19 20 21}}

test vector.489 {myVec set all key1 myValue} {
    list [catch {myVec set all key1 myValue} msg] $msg
} {0 {}}

test vector.490 {myVec set 15 key1 123} {
    list [catch {myVec set 15 key1 123} msg] $msg
} {0 {}}

test vector.491 {myVec set 16 key1 1234 key2 abc} {
    list [catch {myVec set 16 key1 123 key2 abc} msg] $msg
} {0 {}}

test vector.492 {myVec find root -key } {
    list [catch {myVec find root -key} msg] $msg
} {1 {value for "-key" missing}}

test vector.493 {myVec find root -key noKey} {
    list [catch {myVec find root -key noKey} msg] $msg
} {0 {}}

test vector.494 {myVec find root -key key1} {
    list [catch {myVec find root -key key1} msg] $msg
} {0 {20 18 16 14 21 19 17 15 1 2 3 4 13 5 6 8 0}}

test vector.495 {myVec find root -key key2} {
    list [catch {myVec find root -key key2} msg] $msg
} {0 16}

test vector.496 {myVec find root -key key2 -exact notThere } {
    list [catch {myVec find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test vector.497 {myVec find root -key key1 -glob notThere } {
    list [catch {myVec find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test vector.498 {myVec find root -key badKey -regexp notThere } {
    list [catch {myVec find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test vector.499 {myVec find root -key key1 -glob 12*} {
    list [catch {myVec find root -key key1 -glob 12*} msg] $msg
} {0 {16 15}}

test vector.500 {myVec sort} {
    list [catch {myVec sort} msg] $msg
} {1 {wrong # args: should be "myVec sort node ?flags...?"}}

test vector.501 {myVec sort all} {
    list [catch {myVec sort all} msg] $msg
} {1 {more than one node tagged as "all"}}

test vector.502 {myVec sort -recurse} {
    list [catch {myVec sort -recurse} msg] $msg
} {1 {can't find tag or id "-recurse" in ::myVec}}

test vector.503 {myVec sort 0} {
    list [catch {myVec sort 0} msg] $msg
} {0 {8 2 3 4 5 6}}

test vector.504 {myVec sort 0 -recurse} {
    list [catch {myVec sort 0 -recurse} msg] $msg
} {0 {0 8 1 2 3 4 5 6 13 14 15 16 17 18 19 20 21}}

test vector.505 {myVec sort 0 -decreasing -key} {
    list [catch {myVec sort 0 -decreasing -key} msg] $msg
} {1 {value for "-key" missing}}

test vector.506 {myVec sort 0 -re} {
    list [catch {myVec sort 0 -re} msg] $msg
} {1 {ambiguous switch "-re"
following switches are available:
   -ascii 
   -command command
   -decreasing 
   -dictionary 
   -integer 
   -key string
   -path 
   -real 
   -recurse 
   -reorder }}


test vector.507 {myVec sort 0 -decreasing} {
    list [catch {myVec sort 0 -decreasing} msg] $msg
} {0 {6 5 4 3 2 8}}

test vector.508 {myVec sort 0} {
    list [catch {
	set list {}
	foreach n [myVec sort 0] {
	    lappend list [myVec label $n]
	}	
	set list
    } msg] $msg
} {0 {myLabel node2 node3 node4 node5 node6}}

test vector.509 {myVec sort 0 -decreasing} {
    list [catch {myVec sort 0 -decreasing} msg] $msg
} {0 {6 5 4 3 2 8}}


test vector.510 {myVec sort 0 -decreasing -key} {
    list [catch {myVec sort 0 -decreasing -key} msg] $msg
} {1 {value for "-key" missing}}

test vector.511 {myVec sort 0 -decreasing -key key1} {
    list [catch {myVec sort 0 -decreasing -key key1} msg] $msg
} {0 {8 6 5 4 3 2}}

test vector.512 {myVec sort 0 -decreasing -recurse -key key1} {
    list [catch {myVec sort 0 -decreasing -recurse -key key1} msg] $msg
} {0 {15 16 0 1 2 3 4 5 6 8 13 14 17 18 19 20 21}}

test vector.513 {myVec sort 0 -decreasing -key key1} {
    list [catch {
	set list {}
	foreach n [myVec sort 0 -decreasing -key key1] {
	    lappend list [myVec get $n key1]
	}
	set list
    } msg] $msg
} {0 {myValue myValue myValue myValue myValue myValue}}


test vector.514 {myVec index 1->firstchild} {
    list [catch {myVec index 1->firstchild} msg] $msg
} {0 14}

test vector.515 {myVec index root->firstchild} {
    list [catch {myVec index root->firstchild} msg] $msg
} {0 2}

test vector.516 {myVec label root->parent} {
    list [catch {myVec label root->parent} msg] $msg
} {1 {can't find tag or id "root->parent" in ::myVec}}

test vector.517 {myVec index root->parent} {
    list [catch {myVec index root->parent} msg] $msg
} {0 -1}

test vector.518 {myVec index root->lastchild} {
    list [catch {myVec index root->lastchild} msg] $msg
} {0 8}

test vector.519 {myVec index root->next} {
    list [catch {myVec index root->next} msg] $msg
} {0 2}

test vector.520 {myVec index root->previous} {
    list [catch {myVec index root->previous} msg] $msg
} {0 -1}

test vector.521 {myVec label root->previous} {
    list [catch {myVec label root->previous} msg] $msg
} {1 {can't find tag or id "root->previous" in ::myVec}}

test vector.522 {myVec index 1->previous} {
    list [catch {myVec index 1->previous} msg] $msg
} {0 2}

test vector.523 {myVec label root->badModifier} {
    list [catch {myVec label root->badModifier} msg] $msg
} {1 {can't find tag or id "root->badModifier" in ::myVec}}

test vector.524 {myVec index root->badModifier} {
    list [catch {myVec index root->badModifier} msg] $msg
} {0 -1}

test vector.525 {myVec index root->firstchild->parent} {
    list [catch {myVec index root->firstchild->parent} msg] $msg
} {0 0}

test vector.526 {myVec trace} {
    list [catch {myVec trace} msg] $msg
} {1 {wrong # args: should be one of...
  myVec trace create node key how command ?-whenidle?
  myVec trace delete id...
  myVec trace info id
  myVec trace names }}


test vector.527 {myVec trace create} {
    list [catch {myVec trace create} msg] $msg
} {1 {wrong # args: should be "myVec trace create node key how command ?-whenidle?"}}

test vector.528 {myVec trace create root} {
    list [catch {myVec trace create root} msg] $msg
} {1 {wrong # args: should be "myVec trace create node key how command ?-whenidle?"}}

test vector.529 {myVec trace create root * } {
    list [catch {myVec trace create root * } msg] $msg
} {1 {wrong # args: should be "myVec trace create node key how command ?-whenidle?"}}

test vector.530 {myVec trace create root * rwuc} {
    list [catch {myVec trace create root * rwuc} msg] $msg
} {1 {wrong # args: should be "myVec trace create node key how command ?-whenidle?"}}

proc Doit args { global mylist; lappend mylist $args }

test vector.531 {myVec trace create all newKey rwuc Doit} {
    list [catch {myVec trace create all newKey rwuc Doit} msg] $msg
} {0 trace0}

test vector.532 {myVec trace info trace0} {
    list [catch {myVec trace info trace0} msg] $msg
} {0 {all newKey rwuc Doit}}

test vector.533 {test create trace} {
    list [catch {
	set mylist {}
	myVec set all newKey 20
	set mylist
	} msg] $msg
} {0 {{::myVec 0 newKey wc} {::myVec 2 newKey wc} {::myVec 1 newKey wc} {::myVec 14 newKey wc} {::myVec 16 newKey wc} {::myVec 18 newKey wc} {::myVec 20 newKey wc} {::myVec 15 newKey wc} {::myVec 17 newKey wc} {::myVec 19 newKey wc} {::myVec 21 newKey wc} {::myVec 3 newKey wc} {::myVec 4 newKey wc} {::myVec 5 newKey wc} {::myVec 13 newKey wc} {::myVec 6 newKey wc} {::myVec 8 newKey wc}}}

test vector.534 {test read trace} {
    list [catch {
	set mylist {}
	myVec get root newKey
	set mylist
	} msg] $msg
} {0 {{::myVec 0 newKey r}}}

test vector.535 {test write trace} {
    list [catch {
	set mylist {}
	myVec set all newKey 21
	set mylist
	} msg] $msg
} {0 {{::myVec 0 newKey w} {::myVec 2 newKey w} {::myVec 1 newKey w} {::myVec 14 newKey w} {::myVec 16 newKey w} {::myVec 18 newKey w} {::myVec 20 newKey w} {::myVec 15 newKey w} {::myVec 17 newKey w} {::myVec 19 newKey w} {::myVec 21 newKey w} {::myVec 3 newKey w} {::myVec 4 newKey w} {::myVec 5 newKey w} {::myVec 13 newKey w} {::myVec 6 newKey w} {::myVec 8 newKey w}}}

test vector.536 {test unset trace} {
    list [catch {
	set mylist {}
	myVec set all newKey 21
	set mylist
	} msg] $msg
} {0 {{::myVec 0 newKey w} {::myVec 2 newKey w} {::myVec 1 newKey w} {::myVec 14 newKey w} {::myVec 16 newKey w} {::myVec 18 newKey w} {::myVec 20 newKey w} {::myVec 15 newKey w} {::myVec 17 newKey w} {::myVec 19 newKey w} {::myVec 21 newKey w} {::myVec 3 newKey w} {::myVec 4 newKey w} {::myVec 5 newKey w} {::myVec 13 newKey w} {::myVec 6 newKey w} {::myVec 8 newKey w}}}

test vector.537 {myVec trace delete} {
    list [catch {myVec trace delete} msg] $msg
} {0 {}}

test vector.538 {myVec trace delete badId} {
    list [catch {myVec trace delete badId} msg] $msg
} {1 {unknown trace "badId"}}

test vector.539 {myVec trace delete trace0} {
    list [catch {myVec trace delete trace0} msg] $msg
} {0 {}}

test vector.540 {test create trace} {
    list [catch {
	set mylist {}
	myVec set all newKey 20
	set mylist
	} msg] $msg
} {0 {}}

test vector.541 {test unset trace} {
    list [catch {
	set mylist {}
	myVec unset all newKey
	set mylist
	} msg] $msg
} {0 {}}


test vector.542 {myVec notify} {
    list [catch {myVec notify} msg] $msg
} {1 {wrong # args: should be one of...
  myVec notify create ?flags? command
  myVec notify delete notifyId...
  myVec notify info notifyId
  myVec notify names }}


test vector.543 {myVec notify create} {
    list [catch {myVec notify create} msg] $msg
} {1 {wrong # args: should be "myVec notify create ?flags? command"}}

test vector.544 {myVec notify create -allevents} {
    list [catch {myVec notify create -allevents Doit} msg] $msg
} {0 notify0}

test vector.545 {myVec notify info notify0} {
    list [catch {myVec notify info notify0} msg] $msg
} {0 {notify0 {-create -delete -move -sort -relabel} {Doit}}}

test vector.546 {myVec notify info badId} {
    list [catch {myVec notify info badId} msg] $msg
} {1 {unknown notify name "badId"}}

test vector.547 {myVec notify info} {
    list [catch {myVec notify info} msg] $msg
} {1 {wrong # args: should be "myVec notify info notifyId"}}

test vector.548 {myVec notify names} {
    list [catch {myVec notify names} msg] $msg
} {0 notify0}


test vector.549 {test create notify} {
    list [catch {
	set mylist {}
	myVec insert 1 -tags test
	set mylist
	} msg] $msg
} {0 {{-create 22}}}

test vector.550 {test move notify} {
    list [catch {
	set mylist {}
	myVec move 8 test
	set mylist
	} msg] $msg
} {0 {{-move 8}}}

test vector.551 {test sort notify} {
    list [catch {
	set mylist {}
	myVec sort 0 -reorder 
	set mylist
	} msg] $msg
} {0 {{-sort 0}}}

test vector.552 {test relabel notify} {
    list [catch {
	set mylist {}
	myVec label test "newLabel"
	set mylist
	} msg] $msg
} {0 {{-relabel 22}}}

test vector.553 {test delete notify} {
    list [catch {
	set mylist {}
	myVec delete test
	set mylist
	} msg] $msg
} {0 {{-delete 8} {-delete 22}}}


test vector.554 {myVec notify delete badId} {
    list [catch {myVec notify delete badId} msg] $msg
} {1 {unknown notify name "badId"}}


test vector.555 {test create notify} {
    list [catch {
	set mylist {}
	myVec set all newKey 20
	set mylist
	} msg] $msg
} {0 {}}

test vector.556 {test delete notify} {
    list [catch {
	set mylist {}
	myVec unset all newKey
	set mylist
	} msg] $msg
} {0 {}}

test vector.557 {test delete notify} {
    list [catch {
	set mylist {}
	myVec unset all newKey
	set mylist
	} msg] $msg
} {0 {}}

test vector.558 {myVec copy} {
    list [catch {myVec copy} msg] $msg
} {1 {wrong # args: should be "myVec copy parent ?vector? node ?switches?"}}

test vector.559 {myVec copy root} {
    list [catch {myVec copy root} msg] $msg
} {1 {wrong # args: should be "myVec copy parent ?vector? node ?switches?"}}

test vector.560 {myVec copy root 14} {
    list [catch {myVec copy root 14} msg] $msg
} {0 23}

test vector.561 {myVec copy 14 root} {
    list [catch {myVec copy 14 root} msg] $msg
} {0 24}

test vector.562 {myVec copy 14 root -recurse} {
    list [catch {myVec copy 14 root -recurse} msg] $msg
} {1 {can't make cyclic copy: source node is an ancestor of the destination}}

test vector.563 {myVec copy 3 2 -recurse -tags} {
    list [catch {myVec copy 3 2 -recurse -tags} msg] $msg
} {0 25}

test vector.564 {copy vector to vector -recurse} {
    list [catch {
	blt::vector create vector1
	foreach node [myVec children root] {
	    vector1 copy root myVec $node -recurse 
	}
	foreach node [myVec children root] {
	    vector1 copy root myVec $node -recurse 
	}
	vector1 dump root
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node2} {key1 myValue} {}
1 2 {{} node2 node1} {key1 myValue} {}
2 3 {{} node2 node1 node14} {key1 myValue} {}
3 4 {{} node2 node1 node14 node16} {key1 123 key2 abc} {}
4 5 {{} node2 node1 node14 node16 node18} {key1 myValue} {}
5 6 {{} node2 node1 node14 node16 node18 node20} {key1 myValue} {}
3 7 {{} node2 node1 node14 {}} {key1 myValue} {}
2 8 {{} node2 node1 node15} {key1 123} {}
8 9 {{} node2 node1 node15 node17} {key1 myValue} {}
9 10 {{} node2 node1 node15 node17 node19} {key1 myValue} {}
10 11 {{} node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 12 {{} node3} {key1 myValue} {}
12 13 {{} node3 node2} {key1 myValue} {}
13 14 {{} node3 node2 node1} {key1 myValue} {}
14 15 {{} node3 node2 node1 node14} {key1 myValue} {}
15 16 {{} node3 node2 node1 node14 node16} {key1 123 key2 abc} {}
16 17 {{} node3 node2 node1 node14 node16 node18} {key1 myValue} {}
17 18 {{} node3 node2 node1 node14 node16 node18 node20} {key1 myValue} {}
15 19 {{} node3 node2 node1 node14 {}} {key1 myValue} {}
14 20 {{} node3 node2 node1 node15} {key1 123} {}
20 21 {{} node3 node2 node1 node15 node17} {key1 myValue} {}
21 22 {{} node3 node2 node1 node15 node17 node19} {key1 myValue} {}
22 23 {{} node3 node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 24 {{} node4} {key1 myValue} {}
0 25 {{} node5} {key1 myValue} {}
25 26 {{} node5 node13} {key1 myValue} {}
0 27 {{} node6} {key1 myValue} {}
0 28 {{} node14} {key1 myValue} {}
0 29 {{} node2} {key1 myValue} {}
29 30 {{} node2 node1} {key1 myValue} {}
30 31 {{} node2 node1 node14} {key1 myValue} {}
31 32 {{} node2 node1 node14 node16} {key1 123 key2 abc} {}
32 33 {{} node2 node1 node14 node16 node18} {key1 myValue} {}
33 34 {{} node2 node1 node14 node16 node18 node20} {key1 myValue} {}
31 35 {{} node2 node1 node14 {}} {key1 myValue} {}
30 36 {{} node2 node1 node15} {key1 123} {}
36 37 {{} node2 node1 node15 node17} {key1 myValue} {}
37 38 {{} node2 node1 node15 node17 node19} {key1 myValue} {}
38 39 {{} node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 40 {{} node3} {key1 myValue} {}
40 41 {{} node3 node2} {key1 myValue} {}
41 42 {{} node3 node2 node1} {key1 myValue} {}
42 43 {{} node3 node2 node1 node14} {key1 myValue} {}
43 44 {{} node3 node2 node1 node14 node16} {key1 123 key2 abc} {}
44 45 {{} node3 node2 node1 node14 node16 node18} {key1 myValue} {}
45 46 {{} node3 node2 node1 node14 node16 node18 node20} {key1 myValue} {}
43 47 {{} node3 node2 node1 node14 {}} {key1 myValue} {}
42 48 {{} node3 node2 node1 node15} {key1 123} {}
48 49 {{} node3 node2 node1 node15 node17} {key1 myValue} {}
49 50 {{} node3 node2 node1 node15 node17 node19} {key1 myValue} {}
50 51 {{} node3 node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 52 {{} node4} {key1 myValue} {}
0 53 {{} node5} {key1 myValue} {}
53 54 {{} node5 node13} {key1 myValue} {}
0 55 {{} node6} {key1 myValue} {}
0 56 {{} node14} {key1 myValue} {}
}}

puts stderr "done testing vectorcmd.tcl"

exit 0
