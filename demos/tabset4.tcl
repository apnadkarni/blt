#!../src/bltwish

package require BLT
source scripts/demo.tcl
#blt::bltdebug 100

source scripts/stipples.tcl

blt::tabset .t \
    -side left \
    -scrollincrement 10 \
    -scrollcommand { .s set } \
    -selectcommand {  MakePicture .t }  \
    -pagewidth 500 -pageheight 500 \
  -scrolltabs 1

scrollbar .s -command { .t view } -orient horizontal
 
option clear
option add *Tabset.Tab.font -*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*

set files [glob ./images/*.gif]
set files [lsort $files]
set vertFilter sinc
set horzFilter sinc
#set vertFilter none
#set horzFilter none


proc ResizePicture { file maxSize } {
    set src [image create picture -file $file]
    set maxSize [winfo fpixels . $maxSize]
    set w [image width $src]
    set h [image height $src]
    set sw [expr double($maxSize) / $w]
    set sh [expr double($maxSize) / $h]
    set s [expr min($sw, $sh)]
    set w [expr round($s * $w)]
    set h [expr round($s * $h)]
    set dst [image create picture -width $w -height $h]
    global horzFilter vertFilter
    $dst resample $src -filter $horzFilter 
    image delete $src
    return $dst
}

proc MakePicture { w index } {
    set file [$w id $index]
    set src [image create picture -file $file]
    set tail [file tail $file]
    set root [file root $tail]
    regsub -all {\.} $root {_} root
    set width [$w cget -pagewidth]
    set height [$w cget -pageheight]
    if { $width < $height } {
	set dst [ResizePicture $file $width]
    } else {
	set dst [ResizePicture $file $height]
    }
    set label ".t.${root}_l"
    if { [winfo exists $label] } {
	set old [$label cget -image]
	$label configure -image $dst
	image delete $old
    } else {
	label $label -image $dst 
	.t tab configure $index -window $label -padx 1i -pady 0m -fill both 
    }
    .t dockall
    image delete $src
}

blt::table . \
    .t 0,0 -fill both \
    .s 1,0 -fill x 

blt::table configure . r1 -resize none 
focus .t

foreach f [lrange $files 0 3] {
    set tail [file tail $f]
    set root [file root $tail]
    regsub -all {\.} $root {_} root
    set thumb [ResizePicture $f .25i]
    .t insert end $f -icon $thumb -fill both -text $root
}

.t focus 0
.t invoke 0
