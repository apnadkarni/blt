#bltdebug 100

array set cursors {
    w left_side
    e right_side
    n top_side
    s bottom_side
    sw bottom_left_corner
    ne top_right_corner
    se bottom_right_corner
    nw top_left_corner
}

set paperSizes {
    "Letter (8 1/2 x 11 in.)"	"i 8.5i 11i" 
    "A3 (29.7 x 42 cm.)"	"c 28.7c 41c" 
    "A4 (21 x 29.7 cm.)"	"c 21c 29.7c" 
    "A5 (14.85 x 21 cm.)"	"c 14.85c 21c" 
    "Legal (8 1/2 x 14 in.)"	"i 8.5i 14i" 
    "Large (11 x 17 in.)"	"i 11i 17i" 
}

array set paperSize $paperSizes

array set pageInfo {
    gripSize 8
    scale 0.25
    radioFont -*-helvetica-medium-r-*-*-11-120-*-*-*-*-*-*
    labelFont   "Arial 10 bold"
    printCmd	"nlp -d2a211"
    printFile   "out.ps"
    units	"i"
    uscale	1.0
}

proc ApplyWhenIdle { } {
    global pageInfo 

    set data [$pageInfo(graph) postscript output]
    puts stderr "landscape= [$pageInfo(graph) postscript cget -landscape]"
    $pageInfo(image) import ps -data $data -crop no
    puts stderr "writing out2.ps"
    $pageInfo(graph) postscript output -file out2.ps
    unset pageInfo(afterId)
}

proc SetUnits { units }  {
    global pageInfo
    switch -glob $units {
	"i*"    { set pageInfo(uscale) [winfo fpixels . 1i]  }
	"c*"    { set pageInfo(uscale) [winfo fpixels . 1c] }
	default { error "unknown unit \"$units\"" }
    }
    set pageInfo(units) [string index $units 0]
}


proc ConvertUnits { value } {
    global pageInfo
    set value [expr double($value) / $pageInfo(uscale)]
    return [format "%.1f%s" $value $pageInfo(units)]
}


proc SetPaperSize {} {
    global pageInfo 

    set extents $pageInfo(paperExtents)
    foreach { unit w h } $extents break
    SetUnits $unit
    set pageInfo(-paperwidth) $w
    set pageInfo(-paperheight) $h
    ApplyPs
}
    
proc SetCanvasSize { canvas width height } {
    global pageInfo

    set width [winfo pixels . $width]
    set height [winfo pixels . $height]
    $canvas configure -width $width -height $height
}

proc SetCanvasOrientation { canvas } {
    global pageInfo
    set width $pageInfo(paperWidth)
    set height $pageInfo(paperHeight)
    SetCanvasSize $canvas $width $height
}


proc GetPsOptions { graph } {
    global pageInfo

    foreach opt [$graph postscript configure] {
	set pageInfo([lindex $opt 0]) [lindex $opt 4]
    }
}

proc SetOutline { canvas } {
    global pageInfo
    foreach var { gripSize xMin yMin xMax yMax } {
	set $var $pageInfo($var)
    }
    set xMid [expr ($xMax + $xMin - $gripSize) * 0.5]
    set yMid [expr ($yMax + $yMin - $gripSize) * 0.5]
    $canvas coords image $xMin $yMin 
    $canvas itemconfigure image \
	-width [expr $xMax - $xMin] -height [expr $yMax - $yMin]
    $canvas coords nw \
	$xMin $yMin [expr $xMin + $gripSize] [expr $yMin + $gripSize] 
    $canvas coords se \
	[expr $xMax - $gripSize] [expr $yMax - $gripSize] $xMax $yMax 
    $canvas coords ne \
	[expr $xMax - $gripSize] [expr $yMin + $gripSize] $xMax $yMin 
    $canvas coords sw \
	$xMin $yMax [expr $xMin + $gripSize] [expr $yMax - $gripSize] 
    SetCanvasOrientation $canvas
    $canvas coords n \
	$xMid $yMin [expr $xMid + $gripSize] [expr $yMin + $gripSize] 
    $canvas coords s \
	$xMid [expr $yMax - $gripSize] [expr $xMid + $gripSize] $yMax
     $canvas coords e \
 	[expr $xMax - $gripSize] $yMid $xMax [expr $yMid + $gripSize] 
     $canvas coords w \
 	$xMin $yMid [expr $xMin + $gripSize] [expr $yMid + $gripSize] 
}

proc CreateOutline { canvas } {
    global pageInfo
    foreach var { gripSize xMin yMin xMax yMax } {
	set $var $pageInfo($var)
    }
    if { ![blt::bitmap exists pattern8] } {
	blt::bitmap define pattern8 { {8 8} {ff 00 ff 00 ff 00 ff 00 } }
    }
    $canvas create eps $xMin $yMin \
	-tags "outline image" \
	-width [expr $xMax - $xMin] \
	-height [expr $yMax - $yMin]

    $canvas bind image <ButtonPress-1>   "StartMove $canvas %x %y"
    $canvas bind image <B1-Motion>       "MoveOutline $canvas %x %y"
    $canvas bind image <ButtonRelease-1> "EndMove $canvas"

    $canvas bind image <Shift-B1-Motion> "ConstrainMoveOutline $canvas %x %y"
    $canvas bind image <Enter> "EnterImage $canvas"
    $canvas bind image <Leave> "LeaveImage $canvas"
    focus $canvas
    $canvas create rectangle \
	$xMin $yMin [expr $xMin + $gripSize] [expr $yMin + $gripSize] \
	-tags "outline grip nw" 
    $canvas create rectangle \
	[expr $xMax - $gripSize] [expr $yMax - $gripSize] $xMax $yMax \
	-tags "outline grip se" 
    $canvas create rectangle \
	[expr $xMax - $gripSize] [expr $yMin + $gripSize] $xMax $yMin \
	-tags "outline grip ne" 
    $canvas create rectangle \
	$xMin $yMax [expr $xMin + $gripSize] [expr $yMax - $gripSize] \
	-tags "outline grip sw" 

    set xMid [expr ($xMax + $xMin - $gripSize) * 0.5]
    set yMid [expr ($yMax + $yMin - $gripSize) * 0.5]
    $canvas create rectangle \
	$xMid $yMin [expr $xMid + $gripSize] [expr $yMin + $gripSize] \
	-tags "outline grip n" 
    $canvas create rectangle \
	$xMid [expr $yMax - $gripSize] [expr $xMid + $gripSize] $yMax \
	-tags "outline grip s" 
     $canvas create rectangle \
 	[expr $xMax - $gripSize] $yMid $xMax [expr $yMid + $gripSize] \
 	-tags "outline grip e" 
     $canvas create rectangle \
 	$xMin $yMid [expr $xMin + $gripSize] [expr $yMid + $gripSize] \
 	-tags "outline grip w" 
    foreach grip { e w s n sw ne se nw } {
	$canvas bind $grip <ButtonPress-1>   "StartResize %W $grip %x %y"
	$canvas bind $grip <B1-Motion>	     "ResizeOutline %W %x %y"
	$canvas bind $grip <ButtonRelease-1> "EndResize %W $grip %x %y"
	$canvas bind $grip <Enter>	     "EnterGrip %W $grip %x %y"
	$canvas bind $grip <Leave>	     "LeaveGrip %W $grip"
    }
    $canvas raise grip
    $canvas itemconfigure grip -fill red -outline black

    set pageInfo(image) [image create picture]
    set data [$pageInfo(graph) postscript output]
    $pageInfo(image) import ps -data $data -crop no
    $canvas itemconfigure image -image $pageInfo(image)
    $pageInfo(graph) postscript output out2.ps
}


proc EnterImage  { canvas } {
    global cursors
    global pageInfo
    bind $canvas <KeyPress-Left>  {
	MoveOutline %W [expr $pageInfo(lastX) - 1]  $pageInfo(lastY)
    }
    bind $canvas <KeyPress-Right> {
	MoveOutline %W [expr $pageInfo(lastX) + 1]  $pageInfo(lastY)
    }
    bind $canvas <KeyPress-Up>   {
	MoveOutline %W $pageInfo(lastX) [expr $pageInfo(lastY) - 1]
    }
    bind $canvas <KeyPress-Down> {
	MoveOutline %W $pageInfo(lastX) [expr $pageInfo(lastY) + 1]
    }
    focus $canvas
    $canvas configure -cursor fleur
    set pageInfo(lastX) 0
    set pageInfo(lastY) 0
}


proc LeaveImage { canvas } {    
    bind $canvas <KeyPress-Left> ""
    bind $canvas <KeyPress-Right> ""
    bind $canvas <KeyPress-Up> ""
    bind $canvas <KeyPress-Down> ""
    $canvas configure -cursor ""
}

proc EnterGrip  { canvas grip x y } {
    global pageInfo
    $canvas itemconfigure $grip -fill blue -outline black
    set pageInfo(grip) $grip
    global cursors
    bind $canvas <KeyPress-Left>  {
	ResizeOutline %W [expr $pageInfo(lastX) - 1] $pageInfo(lastY)
    }
    bind $canvas <KeyPress-Right> {
	ResizeOutline %W [expr $pageInfo(lastX) + 1] $pageInfo(lastY)
    }
    bind $canvas <KeyPress-Up> {
	ResizeOutline %W $pageInfo(lastX) [expr $pageInfo(lastY) - 1] 
    }
    bind $canvas <KeyPress-Down> {
	ResizeOutline %W $pageInfo(lastX) [expr $pageInfo(lastY) + 1] 
    }
    focus $canvas
    $canvas configure -cursor $cursors($grip)
    set pageInfo(lastX) $x
    set pageInfo(lastY) $y
}

proc LeaveGrip { canvas grip } {    
    $canvas itemconfigure $grip -fill red -outline black
    bind $canvas <KeyPress-Left> ""
    bind $canvas <KeyPress-Right> ""
    bind $canvas <KeyPress-Up> ""
    bind $canvas <KeyPress-Down> ""
    $canvas configure -cursor ""
}

proc StartMove { canvas x y } {
    global pageInfo 
    set pageInfo(lastX) $x
    set pageInfo(lastY) $y
    set pageInfo(direction) "undecided"
    $canvas configure -cursor fleur
}

proc MoveOutline { canvas x y } {
    global pageInfo
    $canvas move outline [expr $x - $pageInfo(lastX)] [expr $y - $pageInfo(lastY)]
    set pageInfo(lastX) $x
    set pageInfo(lastY) $y
}

proc ConstrainMoveOutline { canvas x y } {
    global pageInfo

    set dx [expr $x - $pageInfo(lastX)]
    set dy [expr $y - $pageInfo(lastY)]

    if { $pageInfo(direction) == "undecided" } {
	if { abs($dx) > abs($dy) } {
	    set pageInfo(direction) x
	    $canvas configure -cursor sb_h_double_arrow
	} else {
	    set pageInfo(direction) y
	    $canvas configure -cursor sb_v_double_arrow
	}
    }
    switch $pageInfo(direction) {
	x { set dy 0 ; set pageInfo(lastX) $x } 
	y { set dx 0 ; set pageInfo(lastY) $y }
    }
    $canvas move outline $dx $dy
}

proc EndMove { canvas } {
    $canvas configure -cursor ""

    set coords [$canvas coords image]
    set x [lindex $coords 0]
    set y [lindex $coords 1]
    set w [$canvas itemcget image -width]
    set h [$canvas itemcget image -height]

    global pageInfo
    set pageInfo(xMin) $x
    set pageInfo(xMin) $y
    set pageInfo(xMax) [expr $x + $w]
    set pageInfo(yMax) [expr $y + $h]

    global pageInfo
    set pageInfo(-padx) [list $pageInfo(xMin) [expr $pageInfo(paperWidth) - $pageInfo(xMax)]]
    set pageInfo(-pady) [list $pageInfo(yMin) [expr $pageInfo(paperHeight) - $pageInfo(yMax)]]
}

proc StartResize { canvas grip x y } {
    global pageInfo
    $canvas itemconfigure image -quick yes
    set pageInfo(grip) $grip
    $canvas itemconfigure $grip -fill red -outline black 
    $canvas raise grip
    global cursors
    $canvas configure -cursor $cursors($grip)
    set pageInfo(lastX) $x
    set pageInfo(lastY) $y
}

proc EndResize { canvas grip x y } {
    $canvas itemconfigure image -quick no
    ResizeOutline $canvas $x $y
    $canvas itemconfigure $grip -fill "" -outline "" 
    $canvas configure -cursor ""
}

proc ResizeOutline { canvas x y } {
    global pageInfo

    foreach var { gripSize xMin yMin xMax yMax } {
	set $var $pageInfo($var)
    }
    switch $pageInfo(grip) {
	n {
	    set yMin $y
	}
	s {
	    set yMax $y
	}
	e {
	    set xMax $x
	}
	w {
	    set xMin $x
	}
	sw {
	    set xMin $x ; set yMax $y
	}
	ne {
	    set xMax $x ; set yMin $y
	}
	se {
	    set xMax $x ; set yMax $y
	}
	nw {
	    set xMin $x ; set yMin $y
	}
    }
    set width [expr $xMax - $xMin]
    set height [expr $yMax - $yMin]
    if { ($width < 1) || ($height < 1) } {
	return
    }
    SetOutline $canvas
    foreach var { xMin yMin xMax yMax } {
	set pageInfo($var) [set $var]
    }
}

proc ComputePlotGeometry { graph } {
    global pageInfo

    GetPsOptions $graph
    set width [winfo width $graph]
    set height [winfo height $graph]
    if { $pageInfo(-width) > 0 } {
	set width $pageInfo(-width)
    }
    if { $pageInfo(-height) > 0 } {
	set height $pageInfo(-height)
    }

    set left [lindex $pageInfo(-padx) 0]
    set right [lindex $pageInfo(-padx) 1]
    set top [lindex $pageInfo(-pady) 0]
    set bottom [lindex $pageInfo(-pady) 1]
    set padx [expr $left + $right]
    set pady [expr $top + $bottom]

    if { $pageInfo(-paperwidth) > 0 } {
	set paperWidth $pageInfo(-paperwidth)
    } else {
	set paperWidth [expr $width + $padx]
    }
    if { $pageInfo(-paperheight) > 0 } {
	set paperHeight $pageInfo(-paperheight)
    } else {
	set paperHeight [expr $height + $pady]
    }
    set scale 1.0
    if { [info exists $pageInfo(-maxpect)] && $pageInfo(-maxpect) } {
	set xScale [expr ($paperWidth - $padx) / double($width)]
	set yScale [expr ($paperHeight - $pady) / double($height)]
	set scale [expr min($xScale,$yScale)]
	set bboxWidth [expr round($width * $scale)]
	set bboxHeight [expr round($height * $scale)]
    } else {
	if { ($width + $padx) > $paperWidth } {
	    set width [expr $paperWidth - $padx]
	}
	if { ($height + $pady) > $paperHeight } {
	    set height [expr $paperHeight - $pady]
	}
	set bboxWidth $width
	set bboxHeight $height
    }
    set x $left
    set y $top
    if { $pageInfo(-center) } {
	if { $paperWidth > $bboxWidth }  {
	    set x [expr ($paperWidth - $bboxWidth) / 2]
	}
	if { $paperHeight > $bboxHeight } {
	    set y [expr ($paperHeight - $bboxHeight) / 2]
	}
    }
    set pageInfo(xMin) [expr $x * $pageInfo(scale)]
    set pageInfo(yMin) [expr $y * $pageInfo(scale)]
    set pageInfo(xMax) [expr ($x + $bboxWidth) * $pageInfo(scale)]
    set pageInfo(yMax) [expr ($y + $bboxHeight) * $pageInfo(scale)]
    set pageInfo(paperHeight) [expr $paperHeight * $pageInfo(scale)]
    set pageInfo(paperWidth) [expr $paperWidth * $pageInfo(scale)]
}

proc PsDialog { graph } {
    global pageInfo paperSizes 

    set pageInfo(graph) $graph
    set top $graph.top
    toplevel $top
    option add *graph.top*Radiobutton.font $pageInfo(radioFont)
    GetPsOptions $graph
    ComputePlotGeometry $graph
    set canvas $top.layout
    canvas $canvas -confine yes \
	-width $pageInfo(paperWidth) -height $pageInfo(paperHeight) -bg gray \
	-bd 2 -relief sunken 
    CreateOutline $canvas
    SetCanvasOrientation $canvas
    blt::tk::label $top.titleLabel -text "PostScript Options"
    blt::table $top \
	0,0 $top.titleLabel -cspan 7 \
	1,0 $canvas -cspan 7

    set row 2
    set col 0

    blt::tk::label $top.paperLabel -text "Paper" -font "Arial 10 bold"
    blt::combobutton $top.paper \
	-textvariable pageInfo(paperSize)  \
	-font { Arial 10 } \
	-menu $top.paper.m

    set menu [blt::combomenu $top.paper.m -textvariable pageInfo(paperSize)]
    foreach { key value } $paperSizes {
	$menu add -type command -text $key -value $value
    }
    $menu item configure command -variable pageInfo(paperExtents) \
	-command "SetPaperSize"
    set pageInfo(paperSize) [lindex $paperSizes 0]
    blt::table $top \
	$row,$col     $top.paperLabel -anchor e \
	$row,$col+1   $top.paper -anchor e -fill x
    incr row 

    blt::tk::label $top.orientLabel -text "Orientation" -font "Arial 10 bold"
    blt::combobutton $top.orient \
	-textvariable pageInfo(orient)  \
	-font { Arial 10 } \
	-menu $top.orient.m

    set menu [blt::combomenu $top.orient.m -textvariable pageInfo(orient)]
    $menu add -type command -text "Portrait" -value "0" \
	-variable pageInfo(-landscape) -command "ApplyPs"
    $menu add -type command -text "Landscape" -value "1" \
	-variable pageInfo(-landscape) -command "ApplyPs"
    set pageInfo(orient) "Landscape"
    set pageInfo(-landscape) 1
    blt::table $top \
	$row,$col     $top.orientLabel -anchor e \
	$row,$col+1   $top.orient -anchor e -fill x 
    incr row 

    set col 0
    incr row
    blt::tk::label $top.sizeLabel -text "Plot Size" -font "Arial 10 bold"
    blt::combobutton $top.plotsize \
	-textvariable pageInfo(plotsize)  \
	-font { Arial 10 } \
	-menu $top.plotsize.m

    set menu [blt::combomenu $top.plotsize.m -textvariable pageInfo(plotsize)]
    $menu add -type command -text "Default" -value "default" \
	-variable pageInfo(-plotsize) -command "SetPlotSize"
    $menu add -type command -text "Max Aspect" -value "maxpect" \
	-variable pageInfo(-plotsize) -command "SetPlotSize"
    $menu add -type command -text "Small (3.25 in x 3.25 in)" -value "small" \
	-variable pageInfo(-plotsize) -command "SetPlotSize"
    $menu add -type command -text "Large (6 in x 8 in)" -value "large" \
	-variable pageInfo(-plotsize) -command "SetPlotSize"
    $menu add -type command -text "Other" -value "other" \
	-variable pageInfo(-plotsize) \
	-command "SizeDialog $graph {Adjust Plot Size}"
    set pageInfo(plotsize) "Default"
    blt::table $top \
	$row,$col     $top.sizeLabel -anchor e \
	$row,$col+1   $top.plotsize -anchor e -fill x 
    incr row 

    blt::tk::label $top.modeLabel -text "Color mode" -font "Arial 10 bold"
    blt::combobutton $top.colormode \
	-textvariable pageInfo(colormode)  \
	-font { Arial 10 } \
	-menu $top.colormode.m

    set menu [blt::combomenu $top.colormode.m -textvariable pageInfo(colormode)]
    $menu add -type command -text "Full Color" -value "0" \
	-variable pageInfo(-greyscale) -command "ApplyPs"
    $menu add -type command -text "Greyscale" -value "1" \
	-variable pageInfo(-greyscale) -command "ApplyPs"
    set pageInfo(colormode) "Full Color"
    blt::table $top \
	$row,$col     $top.modeLabel -anchor e \
	$row,$col+1   $top.colormode -anchor e -fill x 
    incr row 
    
    set pageInfo(oldPadX) $pageInfo(-padx)
    set pageInfo(oldPadY) $pageInfo(-pady)

    blt::tk::label $top.posLabel -text "Position" -font "Arial 10 bold"
    blt::combobutton $top.position \
	-textvariable pageInfo(position)  \
	-font { Arial 10 } \
	-menu $top.position.m

    set menu [blt::combomenu $top.position.m -textvariable pageInfo(position)]

    $menu add -type command -text "Center" -value "1" \
	-variable pageInfo(-position) -command { CenterPlot }
    $menu add -type command -text "Origin" -value "0" \
	-variable pageInfo(-position) -command { ApplyPs }
    $menu add -type command -text "Move" -value "move" \
 	-variable pageInfo(-position) -command { MoveDialog }

    set pageInfo(position) "Center"
    blt::table $top \
	$row,$col     $top.posLabel -anchor e \
	$row,$col+1   $top.position -anchor e -fill x 
    incr row 

    blt::tk::label $top.previewLabel -text "Preview" -font "Arial 10 bold"
    blt::combobutton $top.preview \
	-textvariable pageInfo(preview)  \
	-font { Arial 10 } \
	-menu $top.preview.m

    set pageInfo(preview) "No"
    set menu [blt::combomenu $top.preview.m -textvariable pageInfo(preview)]
    $menu add -type command -text "Yes" -value "1" \
	-variable pageInfo(-preview) -command "ApplyPs"
    $menu add -type command -text "No" -value "0" \
	-variable pageInfo(-preview) -command "ApplyPs"

    blt::table $top \
	$row,$col     $top.previewLabel -anchor e \
	$row,$col+1   $top.preview -anchor e -fill x 
    incr row 

    blt::tk::label $top.printLabel -text "Print To" -font "Arial 10 bold"
    blt::combobutton $top.printer \
	-textvariable pageInfo(printer)  \
	-font { Arial 10 } -justify left \
	-menu $top.printer.m

    set menu [blt::combomenu $top.printer.m -textvariable pageInfo(printer)]
    $menu add -type command -text "File" -value "printFile" \
	-command "$top.fileEntry configure -textvariable pageInfo(printCmd)" \
	-variable pageInfo(printTo) 

    $menu add -type command -text "Command" -value "printCmd" \
	-command "$top.fileEntry configure -textvariable pageInfo(printCmd)" \
	-variable pageInfo(printTo) 
    
    entry $top.fileEntry -textvariable pageInfo(printTo)
    $menu invoke "File" 

    blt::table $top \
	$row,$col     $top.printLabel -anchor e \
	$row,$col+1   $top.printer -anchor e -fill x \
	$row+1,1 $top.fileEntry -anchor w -fill x -cspan 3 
    incr row 2


    #blt::table configure $top c4 -width .125i
    frame $top.frame 
    button $top.frame.cancel -text "Cancel" -command "destroy $top"
    button $top.frame.print -text "Ok" -command "PrintPs $graph"
    blt::table $top.frame \
	0,0 $top.frame.cancel -width 0.5i \
	0,1 $top.frame.print  -width 0.5i

    blt::table $top \
	$row,$col     $top.frame -fill x -columnspan 2

    blt::table configure $top r* -resize none -pady { 0 2 }
    blt::table configure $top r1 -resize both
}

proc PrintPs { graph } {
    $graph postscript output "out.ps"
    puts stdout "wrote file \"out.ps\"."
    flush stdout
}

proc ApplyPs { } {
    global pageInfo

    set graph $pageInfo(graph)
    foreach option [$graph postscript configure] {
	set var [lindex $option 0]
	set old [lindex $option 4]
	if { [catch {$graph postscript configure $var $pageInfo($var)}] != 0 } {
	    $graph postscript configure $var $old
	    set pageInfo($var) $old
	}
    }
    ComputePlotGeometry $graph
    foreach var { -paperheight -paperwidth -width -height } {
	set pageInfo($var) [ConvertUnits $pageInfo($var)]
    }
    SetOutline $graph.top.layout
    if { ![info exists pageInfo(afterId)] } {
	set pageInfo(afterId) [after idle ApplyWhenIdle]
    }
}

proc StartChange { w delta } {
    ChangeSize $w $delta
    global pageInfo
    set pageInfo(afterId) [after 300 RepeatChange $w $delta]
}

proc RepeatChange { w delta } {
    ChangeSize $w $delta
    global pageInfo
    set pageInfo(afterId) [after 100 RepeatChange $w $delta]
}

proc EndChange { w } {
    global pageInfo
    after cancel $pageInfo(afterId)
}

proc ChangeSize { w delta } {
    set f [winfo parent $w]
    set value [$f.entry get]
    set value [expr $value + $delta]
    if { $value < 0 } {
	set value 1
    }
    $f.entry delete 0 end
    $f.entry insert 0 $value
}

proc MakeSizeAdjustor { w label var } {
    frame $w
    label $w.label -text $label
    button $w.plus -text "+" -padx 1 -pady 0 -font \*symbol\* 
    entry $w.entry -width 6 -textvariable "pageInfo($var)"
    button $w.minus -text "-" -padx 1 -pady 0 -font \*symbol\*
    label $w.units -text "in"
    bind $w.plus <ButtonPress-1>  { StartChange %W 0.1}
    bind $w.plus <ButtonRelease-1> { EndChange %W }
    bind $w.minus <ButtonPress-1>  { StartChange %W -0.1}
    bind $w.minus <ButtonRelease-1> { EndChange %W }
    blt::table $w \
	0,1 $w.label \
	1,1 $w.entry -rspan 2 -fill y \
	1,0 $w.minus -padx 2 -pady 2 \
	2,0 $w.plus -padx 2 -pady { 0 2 } \
	1,2 $w.units -rspan 2 -fill y 
    
}


proc SizeDialog { graph title } {
    global pageInfo
    set top .plotSize
    if { [winfo exists $top] } {
	return
    }
    toplevel $top
    label $top.title -text $title
    button $top.cancel -text "Cancel" -command "destroy $top"
    button $top.ok -text "Ok" -command "ApplyPs; destroy $top"
    MakeSizeAdjustor $top.plotWidth "Width" -width
    MakeSizeAdjustor $top.plotHeight "Height" -height
    blt::table $top \
	0,0 $top.title -cspan 2 \
	1,0 $top.plotWidth \
	1,1 $top.plotHeight \
	2,0 $top.cancel -pady 4 -padx 4 -width 1i \
	2,1 $top.ok -pady 4 -padx 4 -width 1i
    set width [winfo fpixels . $pageInfo(-width)]
    set height [winfo fpixels . $pageInfo(-height)]
    if { $width == 0 } {
	set width [expr ($pageInfo(xMax) - $pageInfo(xMin)) / $pageInfo(scale)]
	set pageInfo(-width) [ConvertUnits $width]
    }
    if { $height == 0 } {
	set height [expr ($pageInfo(yMax) - $pageInfo(yMin)) / $pageInfo(scale)]
	set pageInfo(-height) [ConvertUnits $height]
    }
    set pageInfo(-maxpect) 0
}

proc SetPlotSize { } {
    global pageInfo
    set graph $pageInfo(graph)
    switch $pageInfo(-plotsize) {
	default { 
	    set pageInfo(-width) 0
	    set pageInfo(-height) 0 
	    set pageInfo(-maxpect) 0
	    set pageInfo(-padx) $pageInfo(oldPadX)
	    set pageInfo(-pady) $pageInfo(oldPadY)
	} maxpect { 
	    set pageInfo(-width) 0
	    set pageInfo(-height) 0
	    set pageInfo(-maxpect) 1
	    set pageInfo(-padx) $pageInfo(oldPadX)
	    set pageInfo(-pady) $pageInfo(oldPadY)
	} resize {
	    set pageInfo(-maxpect) 0
	}
    }
    ApplyPs
}


proc PaperSizeDialog { title } {
    set top .paperSize
    if { [winfo exists $top] } {
	return
    }
    toplevel $top
    label $top.title -text $title
    MakeSizeAdjustor $top.width "Width" -paperwidth
    MakeSizeAdjustor $top.height "Height" -paperheight
    button $top.cancel -text "Cancel" -command "destroy $top"
    button $top.ok -text "Ok" -command "ApplyPs; destroy $top"
    blt::table $top \
	0,0 $top.title -cspan 2 \
	1,0 $top.width \
	1,1 $top.height \
	2,0 $top.cancel -pady 4 -padx 4 -width 1i \
	2,1 $top.ok -pady 4 -padx 4 -width 1i
}

proc MarginDialog { graph } {
    set top $graph.top.options
    if { [winfo exists $top] } {
	return
    }
    toplevel $top
    set row 0
    set col 0
    label $top.modeLabel -text "Printer"
    radiobutton $top.color -text "Color" -value "color" \
	-variable pageInfo(-colormode) -command "ApplyPs"
    radiobutton $top.greyscale -text "Greyscale" -value "greyscale" \
	-variable pageInfo(-colormode) -command "ApplyPs"
    blt::table $top \
	$row,$col   $top.modeLabel -anchor e \
	$row,$col+1 $top.color -anchor w \
	$row+1,$col+1 $top.greyscale -anchor w 

    blt::table configure $top r$row -pady { 4 0 }

    label $top.previewLabel -text "Preview"
    radiobutton $top.previewYes -text "Yes" -value "1" \
	-variable pageInfo(-preview) -command "ApplyPs"
    radiobutton $top.previewNo -text "No" -value "0" \
	-variable pageInfo(-preview) -command "ApplyPs"
    set col 2
    blt::table $top \
	$row,$col   $top.previewLabel -anchor e \
	$row,$col+1 $top.previewYes -anchor w \
	$row+1,$col+1 $top.previewNo -anchor w 
    incr row 2

    button $top.cancel -text "Cancel" -command "destroy $top"
    button $top.ok -text "Done" -command "PrintPs $graph"
    blt::table $top \
	$row,0 $top.cancel -pady 4 -padx 4 -width 1i \
	$row,1 $top.ok -pady 4 -padx 4 -width 1i
	
}    

proc CenterPlot { } {
    global pageInfo

    set pageInfo(-padx) $pageInfo(oldPadX)
    set pageInfo(-pady) $pageInfo(oldPadY)
    ApplyPs
}
