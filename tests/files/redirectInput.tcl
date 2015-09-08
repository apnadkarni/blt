if { ![fblocked stdin] } {
    set in [read stdin]
    puts stdout $in
}
exit 0
