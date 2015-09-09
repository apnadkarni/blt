if { [llength $argv] > 0 } {
    exit 1
}
set in [read stdin]
puts stdout $in
exit 0
