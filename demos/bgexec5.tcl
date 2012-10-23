#!../src/bltwish

package require BLT

foreach demo [glob barchart?.tcl] {
    blt::bgexec var wish $demo &
}

button .kill -text "Kill All" -command { set var 0 }
blt::table . .kill -fill both 


