#!../src/bltwish

package require BLT
source scripts/demo.tcl
blt::debug watch ResizeEpsItem

proc StartMove { canvas tagName x y } {
    SaveImageCoords $canvas $tagName $x $y
    $canvas itemconfigure $tagName-image -quick yes 
}

proc EndMove { canvas tagName } {
    $canvas configure -cursor {}
    $canvas itemconfigure $tagName-image -quick no 
}

proc MoveEpsItem { canvas tagName x y } {
    global lastX lastY
    $canvas move $tagName \
	[expr $x - $lastX($tagName)] [expr $y - $lastY($tagName)]
    set lastX($tagName) $x
    set lastY($tagName) $y
}

proc GetEpsBBox { canvas tagName } {
    global left top right bottom
    set anchor [$canvas coords $tagName-image]
    set left [lindex $anchor 0]
    set top [lindex $anchor 1]
    if { [$canvas type $tagName-image] == "image" } {
	set image [$canvas itemcget $tagName-image -image]
	set width [image width $image]
	set height [image height $image]
    } else {
	set width [$canvas itemcget $tagName-image -width]
	set height [$canvas itemcget $tagName-image -height]
    }
    set right [expr $left + $width]
    set bottom [expr $top + $height]
}
    
proc SaveImageCoords { canvas tagName x y } {
    global lastX lastY 
    set lastX($tagName) $x
    set lastY($tagName) $y
    $canvas configure -cursor sb_h_double_arrow
    $canvas itemconfigure $tagName-image -quick yes 
}

array set cursors {
    sw bottom_left_corner
    ne top_right_corner
    se bottom_right_corner
    nw top_left_corner
}

proc StartResize { canvas tagName x y anchor } {
    global left top right bottom image

    GetEpsBBox $canvas $tagName
    $canvas itemconfigure $tagName-image -quick yes 
    $canvas itemconfigure $tagName-grip -fill red
    $canvas create line $left $top $right $bottom  \
	-tags "$tagName $tagName-cross $tagName-l1" \
	-fill red -width 2

    $canvas create line $left $bottom $right $top \
	-tags "$tagName $tagName-cross $tagName-l2" \
	-fill red  -width 2
    $canvas raise $tagName-grip
    global cursors
    $canvas configure -cursor $cursors($anchor)
    global lastX lastY 
    set lastX($tagName) $x
    set lastY($tagName) $y
}

proc EndResize { canvas tagName x y anchor } {
    $canvas itemconfigure $tagName-image -quick no \
        -showimage yes
    ResizeEpsItem $canvas $anchor $tagName $x $y
    $canvas itemconfigure $tagName-grip -fill green
    $canvas delete $tagName-cross
    $canvas configure -cursor ""
}

proc ResetGrips { canvas tagName } {
    global gripSize
    global left top right bottom

    GetEpsBBox $canvas $tagName
    $canvas coords $tagName-nw \
	$left $top [expr $left + $gripSize] [expr $top + $gripSize] 
    $canvas coords $tagName-se \
	[expr $right - $gripSize] [expr $bottom - $gripSize] $right $bottom 
    $canvas coords $tagName-ne \
	[expr $right - $gripSize] [expr $top + $gripSize] $right $top 
    $canvas coords $tagName-sw \
	$left $bottom [expr $left + $gripSize] [expr $bottom - $gripSize] 
    $canvas coords $tagName-l1 $left $top $right $bottom  
    $canvas coords $tagName-l2 $left $bottom $right $top 
}

proc ResizeEpsItem { canvas anchor tagName x y } {
    global left top right bottom 

    GetEpsBBox $canvas $tagName
    switch $anchor {
	sw {
	    set left $x ; set bottom $y
	    set cursor bottom_left_corner
	}
	ne {
	    set right $x ; set top $y
	    set cursor top_right_corner
	}
	se {
	    set right $x ; set bottom $y
	    set cursor bottom_right_corner
	}
	nw {
	    set left $x ; set top $y
	    set cursor top_left_corner
	}
	default {
	    error "anchor can't be $anchor"
	}
    }
    set w [expr $right - $left]
    set h [expr $bottom - $top]
    set options ""
    if { $w > 1 } {
	append options "-width $w "
    }
    if { $h > 1 } {
	append options "-height $h "
    }
    $canvas coords $tagName-image $left $top
    eval $canvas itemconfigure $tagName-image $options
    GetEpsBBox $canvas $tagName
    ResetGrips $canvas $tagName
}

set numGroups 0
set id 0

proc MakeEps { canvas {epsFile ""} {imageFile ""} } {
    global numGroups id gripSize image

    set image ""
    if { $imageFile != "" } {
        set image [image create picture -file $imageFile]
    }
    set tagName "epsGroup[incr numGroups]"
    $canvas create eps 20 20 \
	-anchor nw \
	-tags "$tagName $tagName-image" \
	-titlecolor white \
	-titlerotate 0 \
	-titleanchor nw \
	-font { Courier 24 } \
	-stipple BLT \
	-outline orange4 \
	-fill orange \
	-file $epsFile \
	-showimage yes \
	-image $image 
    
    set gripSize 8
    GetEpsBBox $canvas $tagName
    global left top right bottom
    $canvas create rectangle \
	$left $top [expr $left + $gripSize] [expr $top + $gripSize] \
	-tags "$tagName $tagName-grip $tagName-nw" \
	-fill red -outline ""
    $canvas create rectangle \
	[expr $right - $gripSize] [expr $bottom - $gripSize] $right $bottom \
	-tags "$tagName $tagName-grip $tagName-se" \
	-fill red -outline ""
    $canvas create rectangle \
	[expr $right - $gripSize] [expr $top + $gripSize] $right $top \
	-tags "$tagName $tagName-grip $tagName-ne" \
	-fill red -outline ""
    $canvas create rectangle \
	$left $bottom [expr $left + $gripSize] [expr $bottom - $gripSize] \
	-tags "$tagName $tagName-grip $tagName-sw" \
	-fill red -outline ""

    $canvas bind $tagName-image <ButtonPress-1> \
	 "StartMove $canvas $tagName %x %y"
    $canvas bind $tagName-image <B1-Motion> \
	"MoveEpsItem $canvas $tagName %x %y"
    $canvas bind $tagName <ButtonRelease-1> \
	"EndMove $canvas $tagName"

    foreach grip { sw ne se nw } {
	$canvas bind $tagName-$grip <ButtonPress-1> \
	    "StartResize $canvas $tagName %x %y $grip"
	$canvas bind $tagName-$grip <B1-Motion> \
	    "ResizeEpsItem $canvas $grip $tagName %x %y"
	$canvas bind $tagName-$grip <ButtonRelease-1> \
	    "EndResize $canvas $tagName %x %y $grip"
	$canvas raise $tagName-$grip
    }
}

proc MakeImage { canvas fileName } {
    global numGroups id gripSize image

    set image ""
    set image [image create picture -file $fileName]

    set tagName "epsGroup[incr numGroups]"
    $canvas create image 20 20 \
	-anchor nw \
	-tags "$tagName $tagName-image" \
	-image $image 
    
    set gripSize 8
    GetEpsBBox $canvas $tagName
    global left top right bottom
    $canvas create rectangle \
	$left $top [expr $left + $gripSize] [expr $top + $gripSize] \
	-tags "$tagName $tagName-grip $tagName-nw" \
	-fill red -outline ""
    $canvas create rectangle \
	[expr $right - $gripSize] [expr $bottom - $gripSize] $right $bottom \
	-tags "$tagName $tagName-grip $tagName-se" \
	-fill red -outline ""
    $canvas create rectangle \
	[expr $right - $gripSize] [expr $top + $gripSize] $right $top \
	-tags "$tagName $tagName-grip $tagName-ne" \
	-fill red -outline ""
    $canvas create rectangle \
	$left $bottom [expr $left + $gripSize] [expr $bottom - $gripSize] \
	-tags "$tagName $tagName-grip $tagName-sw" \
	-fill red -outline ""

    $canvas bind $tagName <ButtonRelease-1> \
	"$canvas configure -cursor {}"
    $canvas bind $tagName-image <ButtonPress-1> \
	"SaveImageCoords $canvas $tagName %x %y"
    $canvas bind $tagName-image <B1-Motion> \
	"MoveEpsItem $canvas $tagName %x %y"

    foreach grip { sw ne se nw } {
	$canvas bind $tagName-$grip <ButtonPress-1> \
	    "StartResize $canvas $tagName %x %y $grip"
	$canvas bind $tagName-$grip <B1-Motion> \
	    "ResizeEpsItem $canvas $grip $tagName %x %y"
	$canvas bind $tagName-$grip <ButtonRelease-1> \
	    "EndResize $canvas $tagName %x %y $grip"
	$canvas raise $tagName-$grip
    }
}

source scripts/stipples.tcl

#
# Script to test the BLT "eps" canvas item.
# 

canvas .layout -bg white

button .print -text "Print" -command {
    wm iconify .
    update
    .layout postscript -file eps.ps 
    wm deiconify .
    update
}
button .quit -text "Quit" -command {
    exit 0
}

blt::table . \
    0,0 .layout -fill both -cspan 2 \
    1,0 .print \
    1,1 .quit \

blt::table configure . r1 -resize none

MakeImage .layout test2.gif

foreach file { ./images/out.ps xy.ps test.ps } {
    if { [file exists $file] } {
        MakeEps .layout $file
    }
}

if 0 {
set image [image create picture -file testImg.jpg]
.layout create eps 20 20 \
	-anchor nw \
        -outline blue \
        -fill yellow \
	-showimage yes \
	-image $image 
}


.layout create rectangle 10 10 50 50 -fill blue -outline white

.layout create text 200 200 \
    -text "This is a text item" \
    -fill yellow \
    -anchor w \
    -font { Times 24 }


.layout create rectangle 50 50 150 150 -fill green -outline red

wm colormapwindows . .layout

.layout configure -scrollregion [.layout bbox all]
