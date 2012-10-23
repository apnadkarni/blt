#!../src/bltwish

package require BLT
source scripts/demo.tcl

set filter sinc
set shadow 8
#set imgfile ./images/sample.gif
set imgfile ./images/blt98.gif
#set imgfile ~/.icons/cd-player.xpm
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
$bg draw rectangle 4 4 [expr $width - 4] [expr $height - 4] -linewidth 0 -color grey60
puts stderr "blur=[time {$bg blur $bg 8}]"
#puts stderr "resample=[time {$bg resample $bg -filter gi8}]"
#$bg copy $src 
set src $bg
#$src blur $src 8
option add *Label.font *helvetica*10*
option add *Label.background white

label .l0 -image $src
label .header0 -text "$width x $height"
label .footer0 -text "100%"
. configure -bg white
set iw $width
set ih $height
set dest [image create picture -width $iw -height $ih]
$dest resample $src -filter $filter
label .header -text "$iw x $ih"
label .footer -text "$filter"
label .l1 -image $dest
set filters {
    "bell"    
    "bessel"  
    "box"     
    "bspline" 
    "catrom"  
    "default" 
    "dummy"   
    "gauss8"  
    "gaussian"
    "gi"	
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
frame .f 
foreach f $filters {
    radiobutton .f.$f -variable filter -value $f -text $f \
	-command "Doit $f"
    blt::table .f $i,0 .f.$f -anchor w
    incr i
}
blt::table . \
   0,0 .f -rspan 3 \
   0,1 .header \
   1,1 .l0 1,2 .l1 \
   2,1 .footer


