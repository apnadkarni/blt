#!../src/bltwish

package require BLT
source scripts/demo.tcl

set imgfile ./images/blt98.gif
if { [llength $argv] > 0 } {
    set imgfile [lindex $argv 0]
}
if { [ file exists $imgfile] } {
    set src [image create picture -file $imgfile]  
} else {
    puts stderr "no image file"
    exit 0
}
set name [file root [file tail $imgfile]]
set width [image width $src]
set width [expr [image width $src]+[image width $src]]
set height [image height $src]
set height [expr [image height $src]+[image height $src]]
set filter sinc

option add *BltTkLabel.background white
option add *BltTkRadiobutton.background white
set iw $width
set ih $height
set dest [image create picture -width $iw -height $ih]
$dest resample $src -filter $filter

blt::tk::label .header0 -text "$width x $height"
blt::tk::label .footer0 -text "100%"
blt::tk::label .header1 -text "Original Image" 
blt::tk::label .header2 -text "Filtered Image"
blt::tk::label .footer -text "$filter"
blt::tk::label .orig -image $src
blt::tk::label .filtered -image $dest
. configure -bg white
set filters {
    "bell"    
    "bessel"  
    "box"     
    "bspline" 
    "catrom"  
    "default" 
    "gauss8"  
    "gaussian"
    "gi"	
    "gi8"
    "lanczos3"
    "mitchell"
    "none"    
    "sinc"    
    "tent"	
    "triangle"
}

proc Doit { filter } {
    global dest src
    set time [time {$dest resample $src -filter $filter}]
    .footer configure -text $time
}

set i 0
frame .f -bg white
foreach f $filters {
    blt::tk::radiobutton .f.$f -variable filter -value $f -text $f \
	-command "Doit $f" -highlightbackground white
    blt::table .f $i,0 .f.$f -anchor w
    incr i
}
blt::table . \
    0,0 .f -rspan 3 \
    0,1 .header1 \
    0,2 .header2 \
    1,1 .orig \
    1,2 .filtered \
    2,1 .footer -cspan 2

blt::table configure . r* -resize none
blt::table configure . r1 -resize both
