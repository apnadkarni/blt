
blt::palette create rainbow -colorformat name -cdata {
    "red" "#FFA500" "yellow" "#008000" "blue" "#4B0082" "#EE82EE"
}
blt::palette create greyscale -colorformat name -cdata {
    "black" "white"
}
blt::palette create nanohub  -colorformat name -cdata {
    white yellow green cyan blue magenta
}

blt::palette create "BGYOR" -colorformat name -cdata {
    "blue" "#008000" "yellow" "#FFA500" "red" 
}

blt::palette create "ROYGB" -colorformat name -cdata {
    "red" "#FFA500" "yellow" "#008000" "blue" 
}

blt::palette create "RYGCB" -colorformat name -cdata {
    "red" "yellow" "green" "cyan" "blue"
}

blt::palette create "BCGYR" -colorformat name -cdata {
    "blue" "cyan" "green" "yellow" "red" 
}

if { [info exists blt_library] } {
    foreach file [glob -nocomplain $blt_library/palettes/*.ncmap \
		  $blt_library/palettes/*.rgb] {
	set tail [file tail $file]
	if { [catch {
	    blt::palette create $tail -colorfile $file
	} errs] != 0 } {
	    puts stderr "file=$file errs=$errs"
	}
    }
}

