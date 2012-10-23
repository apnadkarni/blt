set infile [lindex $argv 0]
set outfile [lindex $argv 1]
set f [open $infile "r"]
set data [read $f]
close $f

set test [file root [file tail $infile]]
set f [open $outfile "w"]
set count 1
set pattern [subst -nocommands {^test ${test}\.[a-z0-9]+ }]
foreach line [split $data \n] {
    if { [regsub $pattern $line "test ${test}.${count} " line] } {
	incr count
    }
    puts $f $line
}
close $f