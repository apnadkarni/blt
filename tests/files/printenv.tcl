if { [llength $argv] == 0 } {
    puts stdout [array get env]
} else {
    foreach varName $argv {
	if { [info exists env($varName)] } {
	    puts stdout "$varName $env($varName)"
	}
    }
}
exit 0
