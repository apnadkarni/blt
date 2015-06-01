#!../src/bltwish

package require BLT
source scripts/demo.tcl

set filter sinc
set shadow 8
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
set height [image height $src]
set bg [image create picture \
	    -width [expr $width + $shadow] \
	    -height [expr $height + $shadow]]
$bg blank white
$bg draw rectangle 4 4 -width [expr $width - 4] -height [expr $height - 4] \
    -linewidth 0 -color grey60
#set src $bg
$bg blur $bg 8
$bg copy $src 
option add *Label.font *helvetica*10*
option add *Label.background white

option add *BltTkLabel.background white
option add *BltTkRadiobutton.background white
label .l0 -image $bg
label .header0 -text "$width x $height"
label .footer0 -text "100%"
. configure -bg white
set iw $width
set ih $height
set dest [image create picture -width $iw -height $ih]
$dest resample $src -filter $filter
blt::tk::label .header1 -text "Original Image" 
blt::tk::label .header2 -text "Filtered Image"
blt::tk::label .footer -text "$filter"
blt::tk::label .l1 -image $dest
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
   1,1 .l0 1,2 .l1 \
   2,1 .footer -cspan 2

blt::table configure . r* -resize none
blt::table configure . r1 -resize both
