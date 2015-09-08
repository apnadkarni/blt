#!../src/bltwish

package require BLT
set myVar "empty"
blt::bgexec var -output myVar -environ "MYVAR myvalue" tclsh printenv.tcl 
array set newEnv $myVar
puts stderr "myVar=$newEnv(MYVAR)"

