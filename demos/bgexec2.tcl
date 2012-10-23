#!../src/bltwish

package require BLT

proc ShowResult { name1 name2 how } {
    global var
    .l$name2 configure -text "$var($name2)"
    after 2000 "blt::table forget .l$name2"
}
    
for { set i 1 } { $i <= 20 } { incr i } {
    label .l$i 
    blt::table . .l$i $i,0
    set pid [blt::bgexec var($i) du /usr/include &]
    .l$i configure -text "Starting #$i pid=$pid"
    trace variable var($i) w ShowResult
    update
    after 500
}

