
#
# -interface bltTk
# -platform generic
# -hooks 
#
set interface blt
set platform generic
set hooks ""
set directory /tmp

for { set i 0 } { $i < $argc } { incr i 2 } {
    set option [lindex $argv $i]
    set value  [lindex $argv [expr $i+1]]
    switch -- "$option" {
	"-interface" {
	    set interface $value
	}
	"-platform" {
	    set platform $value
	}
	"-hooks" {
	    set hooks $value
	}
	"-directory" {
	    set directory $value
	}
	"--" {
	    set files [lrange $argv [expr $i+1] end]
	    break
	}
	default {
	    error "unknown option \"$option\""
	}
    }
}

set decls [open ${interface}.decls "w"]
set header [open /tmp/${interface}Procs.h "a"]

puts $decls "library blt"
puts $decls "interface $interface"
if { $hooks != "" } {
    puts $decls "hooks [list $hooks]"
}

set count 0
set pat { *BLT_EXTERN([^;]*);}
set pat2 { *BLT_.*\([^\)]*\)}
foreach file $files {
    set in [open $file "r"]
    set contents [read $in]
    close $in
    set tail [file tail $file]
    puts $header "\#include \"$tail\""
    set start 0
    while { 1 } {
	if { ![regexp -indices -start $start $pat $contents match submatch] } {
	    #puts stderr "can't match pattern \"$pat\" in \"$file\""
	    #puts stderr "start=$start"
	    break;
	} else {
	    set first [lindex $submatch 0]
	    set last [lindex $submatch 1]
	    set start [expr [lindex $match 1] + 1]
	    set string [string range $contents $first $last]
	    if { [regexp -nocase $pat2 $string match] } {
		puts $decls "declare [incr count] $platform {"
		puts $decls "  $string"
		puts $decls "}"
	    }
	}
    }
}    
close $decls
close $header