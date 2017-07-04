# -*- mode: tcl; tcl-indent-level: 4; indent-tabs-mode: nil -*- 
#
# bltGraph.tcl
#
# Copyright 2015 George A. Howlett. All rights reserved.  
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#   1) Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#   2) Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the
#      distribution.
#   3) Neither the name of the authors nor the names of its contributors
#      may be used to endorse or promote products derived from this
#      software without specific prior written permission.
#   4) Products derived from this software may not be called "BLT" nor may
#      "BLT" appear in their names without specific prior written
#      permission from the author.
#
#   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
#   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
#   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

namespace eval blt {
    namespace eval Graph {
	variable _private 
	array set _private {
	    afterId ""
	    scroll 0
	    space off
	    drag 0
	    x 0
	    y 0
	}
    }
}

option add *zoomOutline.dashes		4	
option add *zoomOutline.lineWidth	2
option add *zoomOutline.xor		yes
option add *zoomTitle.anchor		nw
option add *zoomTitle.coords		"-Inf Inf"
option add *zoomTitle.font		"Arial 14"
option add *zoomTitle.foreground	yellow3
option add *zoomTitle.shadow		yellow4

# ----------------------------------------------------------------------
#
# InitLegend --
#
#	Initializes bindings for graph legend.
#
# ----------------------------------------------------------------------
proc blt::Graph::InitLegend { w } {
    if 0 {
    #
    # Active entry bindings
    #
    $w legend bind all <Enter> { 
	%W entry highlight current 
    }
    $w legend bind all <Leave> { 
	%W entry highlight "" 
    }
    }

    #
    # ButtonPress-1
    #
    #	Performs the following operations:
    #
    #	1. Clears the previous selection.
    #	2. Selects the current entry.
    #	3. Sets the focus to this entry.
    #	4. Scrolls the entry into view.
    #	5. Sets the selection anchor to this entry, just in case
    #	   this is "multiple" mode.
    #
    
    $w legend bind all <ButtonPress-1> { 	
	blt::Graph::SetSelectionAnchor %W current
	set blt::Graph::_private(scroll) 1
    }

    #
    # B1-Motion
    #
    #	For "multiple" mode only.  Saves the current location of the
    #	pointer for auto-scrolling.  Resets the selection mark.  
    #
    $w legend bind all <B1-Motion> { 
	set blt::Graph::_private(x) %x
	set blt::Graph::_private(y) %y
	set elem [%W legend get @%x,%y]
	if { $elem != "" } {
	    if { [%W legend cget -selectmode] == "multiple" } {
		%W legend selection mark $elem
	    } else {
		blt::Graph::SetSelectionAnchor %W $elem
	    }
	}
    }

    #
    # ButtonRelease-1
    #
    #	For "multiple" mode only.  
    #
    $w legend bind all <ButtonRelease-1> { 
	if { [%W legend cget -selectmode] == "multiple" } {
	    %W legend selection anchor current
	}
	after cancel $blt::Graph::_private(afterId)
	set blt::Graph::_private(scroll) 0
    }

    #
    # Shift-ButtonPress-1
    #
    #	For "multiple" mode only.
    #

    $w legend bind all <Shift-ButtonPress-1> { 
	if { [%W legend cget -selectmode] == "multiple" && 
	     [%W legend selection present] } {
	    if { [%W legend get anchor] == "" } {
		%W legend selection anchor current
	    }
	    set elem [%W legend get anchor]
	    %W legend selection clearall
	    %W legend selection set $elem current
	} else {
	    blt::Graph::SetSelectionAnchor %W current
	}
    }
    $w legend bind all <Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w legend bind all <Shift-B1-Motion> { 
	# do nothing
    }
    $w legend bind all <Shift-ButtonRelease-1> { 
	after cancel $blt::Graph::_private(afterId)
	set blt::Graph::_private(scroll) 0
    }

    #
    # Control-ButtonPress-1
    #
    #	For "multiple" mode only.  
    #
    $w legend bind all <Control-ButtonPress-1> { 
	if { [%W legend cget -selectmode] == "multiple" } {
	    set elem [%W legend get current]
	    %W legend selection toggle $elem
	    %W legend selection anchor $elem
	} else {
	    blt::Graph::SetSelectionAnchor %W current
	}
    }
    $w legend bind all <Control-Double-ButtonPress-1> {
	# do nothing
    }
    $w legend bind all <Control-B1-Motion> { 
	# do nothing
    }
    $w legend bind all <Control-ButtonRelease-1> { 
	after cancel $blt::Graph::_private(afterId)
	set blt::Graph::_private(scroll) 0
    }

    $w legend bind all <Control-Shift-ButtonPress-1> { 
	if { [%W legend cget -selectmode] == "multiple" && 
	     [%W legend selection present] } {
	    if { [%W legend get anchor] == "" } {
		%W selection anchor current
	    }
	    if { [%W legend selection includes anchor] } {
		%W legend selection set anchor current
	    } else {
		%W legend selection clear anchor current
		%W legend selection set current
	    }
	} else {
	    blt::Graph::SetSelectionAnchor %W current
	}
    }
    $w legend bind all <Control-Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w legend bind all <Control-Shift-B1-Motion> { 
	# do nothing
    }
    $w legend bind all <KeyPress-Up> {
	blt::Graph::MoveFocus %W previous.row
	if { $blt::Graph::_private(space) } {
	    %W legend selection toggle focus
	}
    }
    $w legend bind all <KeyPress-Down> {
	blt::Graph::MoveFocus %W next.row
	if { $blt::Graph::_private(space) } {
	    %W legend selection toggle focus
	}
    }
    $w legend bind all <KeyPress-Left> {
	blt::Graph::MoveFocus %W previous.column
	if { $blt::Graph::_private(space) } {
	    %W legend selection toggle focus
	}
    }
    $w legend bind all <KeyPress-Right> {
	blt::Graph::MoveFocus %W next.column
	if { $blt::Graph::_private(space) } {
	    %W legend selection toggle focus
	}
    }
    $w legend bind all <KeyPress-space> {
	if { [%W legend cget -selectmode] == "single" } {
	    if { [%W legend selection includes focus] } {
		%W legend selection clearall
	    } else {
		%W legend selection clearall
		%W legend selection set focus
	    }
	} else {
	    %W legend selection toggle focus
	}
	set blt::Graph::_private(space) on
    }

    $w legend bind all <KeyRelease-space> { 
	set blt::Graph::_private(space) off
    }
    $w legend bind all <KeyPress-Return> {
	blt::Graph::MoveFocus %W focus
	set blt::Graph::_private(space) on
    }
    $w legend bind all <KeyRelease-Return> { 
	set blt::Graph::_private(space) off
    }
    $w legend bind all <KeyPress-Home> {
	blt::Graph::MoveFocus %W first
    }
    $w legend bind all <KeyPress-End> {
	blt::Graph::MoveFocus %W last
    }
}

proc blt::Graph::SetSelectionAnchor { w tagOrId } {
    set elem [$w legend get $tagOrId]
    # If the anchor hasn't changed, don't do anything
    if { $elem != [$w legend get anchor] } {
	$w legend selection clearall
	$w legend focus $elem
	$w legend selection set $elem
	$w legend selection anchor $elem
    }
}

# ----------------------------------------------------------------------
#
# MoveFocus --
#
#	Invoked by KeyPress bindings.  Moves the active selection to
#	the entry <where>, which is an index such as "up", "down",
#	"prevsibling", "nextsibling", etc.
#
# ----------------------------------------------------------------------
proc blt::Graph::MoveFocus { w elem } {
    catch {$w legend focus $elem} result
    if { [$w legend cget -selectmode] == "single" } {
        $w legend selection clearall
        $w legend selection set focus
	$w legend selection anchor focus
    }
}


proc Blt_ActiveLegend { g } {
    $g legend bind all <Enter> [list blt::Graph::ActivateLegend $g ]
    $g legend bind all <Leave> [list blt::Graph::DeactivateLegend $g]
}

proc Blt_Crosshairs { g } {
    blt::Graph::Crosshairs $g 
}

proc Blt_ResetCrosshairs { g state } {
    blt::Graph::Crosshairs $g "Any-Motion" $state
}

proc Blt_ZoomStack { g args } {
    array set params {
	-mode click
    }
    array set params $args
    if { $params(-mode) == "click" } {
	blt::Graph::ClickClick $g
    } else {
	blt::Graph::ClickRelease $g
    }	
}

proc Blt_ResetZoomStack { g } {
    while { ![blt::Graph::IsEmpty $g] } {
	blt::Graph::Pop $g
	after 140
    }
    blt::Graph::Pop $g
}

proc Blt_PrintKey { g } {
    blt::Graph::PrintKey $g
}

proc Blt_ClosestPoint { g } {
    blt::Graph::ClosestPoint $g
}

#
# The following procedures that reside in the blt namespace are private.
#

proc blt::Graph::ActivateLegend { g } {
    $g legend activate current
}
proc blt::Graph::DeactivateLegend { g } {
    $g legend deactivate
}

proc blt::Graph::Crosshairs { g {event "Any-Motion"} {state "on"}} {
    $g crosshairs $state
    bind crosshairs-$g <$event>   {
	%W crosshairs configure -position @%x,%y 
    }
    bind crosshairs-$g <Leave>   {
	%W crosshairs off
    }
    bind crosshairs-$g <Enter>   {
	%W crosshairs on
    }
    $g crosshairs configure -color red
    if { $state == "on" } {
	AddBindTag $g crosshairs-$g
    } elseif { $state == "off" } {
	RemoveBindTag $g crosshairs-$g
    }
}

proc blt::Graph::PrintKey { g {event "Shift-ButtonRelease-3"} } {
    bind print-$g <$event>  { Blt_PostScriptDialog %W }
    AddBindTag $g print-$g
}

proc blt::Graph::ClosestPoint { g {event "Control-ButtonPress-2"} } {
    bind closest-point-$g <$event>  {
	blt::Graph::FindElement %W %x %y
    }
    AddBindTag $g closest-point-$g
}

proc blt::Graph::AddBindTag { widget tag } {
    set oldTagList [bindtags $widget]
    if { [lsearch $oldTagList $tag] < 0 } {
	bindtags $widget [linsert $oldTagList 0  $tag]
    }
}

proc blt::Graph::RemoveBindTag { widget tag } {
    set oldTagList [bindtags $widget]
    set index [lsearch $oldTagList $tag]
    if { $index >= 0 } {
	bindtags $widget [lreplace $oldTagList $index $index]
    }
}

proc blt::Graph::FindElement { g x y } {
    if { ![$g element closest $x $y info -interpolate yes] } {
	blt::beep
	return
    }
    # --------------------------------------------------------------
    # find(name)		- element Id
    # find(index)		- index of closest point
    # find(x) find(y)		- coordinates of closest point
    #				  or closest point on line segment.
    # find(dist)		- distance from sample coordinate
    # --------------------------------------------------------------
    set markerName "bltClosest_$info(name)"
    catch { $g marker delete $markerName }
    $g marker create text -coords [list $info(x) $info(y)] \
	-name $markerName \
	-text "$info(name): $info(dist)\nindex $info(index)" \
	-font "Arial 6" \
	-anchor center -justify left \
	-yoffset 0 -bg {} 

    set coords [$g invtransform $x $y]
    set nx [lindex $coords 0]
    set ny [lindex $coords 1]

    $g marker create line -coords [list $nx $ny $info(x) $info(y)] \
	-name line.$markerName 

    FlashPoint $g $info(name) $info(index) 10
    FlashPoint $g $info(name) [expr $info(index) + 1] 10
}

proc blt::Graph::FlashPoint { g name index count } {
    if { ![winfo exists $g] } {
	return;				# Graph may no longer exist.
    }
    if { $count & 1 } {
        $g element deactivate $name 
    } else {
        $g element activate $name $index
    }
    incr count -1
    if { $count > 0 } {
	after 200 blt::Graph::FlashPoint $g $name $index $count
	update
    } else {
	eval $g marker delete [$g marker names "bltClosest_*"]
    }
}


proc blt::Graph::InitZoomStack { g } {
    variable _private
    set _private($g,interval) 100
    set _private($g,afterId) 0
    set _private($g,A,x) {}
    set _private($g,A,y) {}
    set _private($g,B,x) {}
    set _private($g,B,y) {}
    set _private($g,stack) {}
    set _private($g,corner) A
}

proc blt::Graph::ClickClick { g {start "ButtonPress-1"} {reset "ButtonPress-3"} } {
    variable _private
    
    InitZoomStack $g
    
    bind zoom-$g <Enter> "focus %W"
    bind zoom-$g <KeyPress-Escape> { blt::Graph::ResetZoomStack %W }
    bind zoom-$g <${start}> { blt::Graph::SetPoint %W %x %y }
    bind zoom-$g <${reset}> { 
	if { [%W inside %x %y] } { 
	    blt::Graph::ResetZoomStack %W 
	}
    }
    AddBindTag $g zoom-$g
}

proc blt::Graph::ClickRelease { g } {
    variable _private
    
    InitZoomStack $g
    bind zoom-$g <Enter> "focus %W"
    bind zoom-$g <KeyPress-Escape> { 
	blt::Graph::ResetZoomStack %W 
    }
    bind zoom-$g <ButtonPress-1> { 
	blt::Graph::DragStart %W %x %y 
    }
    bind zoom-$g <B1-Motion> { 
	blt::Graph::DragMotion %W %x %y 
    }
    bind zoom-$g <ButtonRelease-1> { 
	blt::Graph::DragFinish %W %x %y 
    }
    bind zoom-$g <ButtonPress-3> { 
	if { [%W inside %x %y] } { 
	    blt::Graph::ResetZoomStack %W 
	}
    }
    AddBindTag $g zoom-$g
}

proc blt::Graph::GetCoords { g x y index } {
    variable _private
    if { [$g cget -invertxy] } {
	set _private($g,$index,x) $y
	set _private($g,$index,y) $x
    } else {
	set _private($g,$index,x) $x
	set _private($g,$index,y) $y
    }
}

proc blt::Graph::MarkPoint { g index } {
    variable _private

    if { [llength [$g xaxis use]] > 0 } {
	set x [$g xaxis invtransform $_private($g,$index,x)]
    } else if { [llength [$g x2axis use]] > 0 } {
	set x [$g x2axis invtransform $_private($g,$index,x)]
    }
    if { [llength [$g yaxis use]] > 0 } {
	set y [$g yaxis invtransform $_private($g,$index,y)]
    } else if { [llength [$g y2axis use]] > 0 } {
	set y [$g y2axis invtransform $_private($g,$index,y)]
    }
    set marker "zoomText_$index"
    set text [format "x=%.4g\ny=%.4g" $x $y] 

    if [$g marker exists $marker] {
     	$g marker configure $marker -coords { $x $y } -text $text 
    } else {
    	$g marker create text -coords { $x $y } -name $marker \
   	    -font "mathmatica1 10" \
	    -text $text -anchor center -bg {} -justify left
    }
}

proc blt::Graph::DestroyTitle { g } {
    variable _private

    if { ![winfo exists $g] } {
	return;				# Graph may no longer exist.
    }
    if { $_private($g,corner) == "A" } {
	catch { $g marker delete "zoomTitle" }
    }
}

proc blt::Graph::IsEmpty { g } {
    variable _private

    set zoomStack $_private($g,stack)
    return [expr {[llength $zoomStack] == 0}]
}

proc blt::Graph::Pop { g } {
    variable _private

    set zoomStack $_private($g,stack)
    if { [llength $zoomStack] > 0 } {
	set cmd [lindex $zoomStack 0]
	set _private($g,stack) [lrange $zoomStack 1 end]
	eval $cmd
	TitleLast $g
	blt::busy hold $g
	update
	blt::busy release $g
	after 2000 [list blt::Graph::DestroyTitle $g]
    } else {
	catch { $g marker delete "zoomTitle" }
    }
}

# Push the old axis limits on the stack and set the new ones

proc blt::Graph::Push { g } {
    variable _private

    eval $g marker delete [$g marker names "zoom*"]
    if { [info exists _private($g,afterId)] } {
	after cancel $_private($g,afterId)
    }
    set x1 $_private($g,A,x)
    set y1 $_private($g,A,y)
    set x2 $_private($g,B,x)
    set y2 $_private($g,B,y)

    if { ($x1 == $x2) || ($y1 == $y2) } { 
	# No delta, revert to start
	return
    }
    set cmd {}
    foreach axis [$g axis names -zoom] {
	set min [$g axis cget $axis -min] 
	set max [$g axis cget $axis -max]
	set scale  [$g axis cget $axis -scale]
	# Save the current scale (log or linear) so that we can restore it.
	# This is for the case where the user changes to logscale while
	# zooming.  A previously pushed axis limit could be negative.  It
	# seems better for popping the zoom stack to restore a previous
	# view (not convert the ranges).
	set c [list $g axis configure $axis]
	lappend c -min $min -max $max -scale $scale
	append cmd "$c\n"
    }

    # This effectively pushes the command to reset the graph to the current
    # zoom level onto the stack.  This is useful if the new axis ranges are
    # bad and we need to reset the zoom stack.
    set _private($g,stack) [linsert $_private($g,stack) 0 $cmd]
    foreach axis [$g axis names -zoom] {
	set type [$g axis type $axis]
	if { $type  == "x" } {
	    set min [$g axis invtransform $axis $x1]
	    set max [$g axis invtransform $axis $x2]
	} elseif { $type == "y" } {
	    set min [$g axis invtransform $axis $y1]
	    set max [$g axis invtransform $axis $y2]
	} else {
	    error "unknown type $type of zoomable axis $axis"
	}
        #puts stderr "axis=$axis min=$min max=$max"
	if { ![SetAxisRanges $g $axis $min $max] } {
	    Pop $g
	    bell
	    return
	}
    }
    blt::busy hold $g 
    update;				# This "update" redraws the graph
    blt::busy release $g
}

proc blt::Graph::SetAxisRanges { g axis min max } {
    if { $min > $max } { 
	set tmp $max; set max $min; set min $tmp
    }
    set oldmin [$g axis cget $axis -min]
    set oldmax [$g axis cget $axis -max]
    if { [catch { $g axis configure $axis -min $min -max $max } errs] != 0 } {
	$g axis configure $axis -min $oldmin -max $oldmax
	puts stderr $errs
	return 0
    }
    return 1
}

#
# This routine terminates either an existing zoom, or pops back to
# the previous zoom level (if no zoom is in progress).
#
proc blt::Graph::ResetZoomStack { g } {
    variable _private

    if { ![info exists _private($g,corner)] } {
	Init $g 
    }
    eval $g marker delete [$g marker names "zoom*"]

    if { $_private($g,corner) == "A" } {
	# Reset the whole axis
	Pop $g
    } else {
	set _private($g,corner) A
	RemoveBindTag $g select-region-$g
    }
}

proc blt::Graph::TitleNext { g } {
    variable _private

    set level [expr [llength $_private($g,stack)] + 1]
    if { [$g cget -invertxy] } {
	set coords "Inf -Inf"
    } else {
	set coords "-Inf Inf"
    }
    $g marker create text -name "zoomTitle" -text "Zoom #$level" \
	-coords $coords -bindtags "" -anchor nw
}

proc blt::Graph::TitleLast { g } {
    variable _private

    set level [llength $_private($g,stack)]
    if { $level > 0 } {
     	$g marker create text -name "zoomTitle" -anchor nw \
	    -text "Zoom #$level" 
    }
}


proc blt::Graph::SetPoint { g x y } {
    variable _private

    if { ![info exists _private($g,corner)] } {
	Init $g
    }
    GetCoords $g $x $y $_private($g,corner)
    bind select-region-$g <Motion> { 
	blt::Graph::GetCoords %W %x %y B
	#blt::Graph::MarkPoint $g B
	blt::Graph::Box %W
    }
    if { $_private($g,corner) == "A" } {
	if { ![$g inside $x $y] } {
	    return
	}
	# First corner selected, start watching motion events

	#MarkPoint $g A
	TitleNext $g 

	AddBindTag $g select-region-$g
	set _private($g,corner) B
    } else {
	# Delete the modal binding
	RemoveBindTag $g select-region-$g
	Push $g 
	set _private($g,corner) A
    }
}

proc blt::Graph::DragStart { g x y } {
    variable _private

    if { ![info exists _private($g,corner)] } {
	Init $g
    }
    GetCoords $g $x $y A
    if { ![$g inside $x $y] } {
	return
    }
    set _private(drag) 1
    TitleNext $g 
}

proc blt::Graph::DragMotion { g x y } {
    variable _private 

    if { $_private(drag) } {
	GetCoords $g $x $y B
	set dx [expr abs($_private($g,B,x) - $_private($g,A,x))]
	set dy [expr abs($_private($g,B,y) - $_private($g,A,y))]
	Box $g
	if { $dy > 10 && $dx > 10 } {
	    return 1
	}	
    }
    return 0
}

proc blt::Graph::DragFinish { g x y } {
    variable _private 
    if { [DragMotion $g $x $y] } {
	Push $g 
    } else {
	eval $g marker delete [$g marker names "zoom*"]
	if { [info exists _private($g,afterId)] } {
	    after cancel $_private($g,afterId)
	}
    }
    set _private(drag) 0
}


proc blt::Graph::MarchingAnts { g offset } {
    variable _private

    if { ![winfo exists $g] } {
	return;				# Graph may no longer exist.
    }
    incr offset
    # wrap the counter after 2^16
    set offset [expr $offset & 0xFFFF]
    if { [$g marker exists zoomOutline] } {
	$g marker configure zoomOutline -dashoffset $offset 
	set interval $_private($g,interval)
	set id [after $interval [list blt::Graph::MarchingAnts $g $offset]]
	set _private($g,afterId) $id
    }
}

proc blt::Graph::Box { g } {
    variable _private

    if { $_private($g,A,x) > $_private($g,B,x) } { 
	set x1 [$g xaxis invtransform $_private($g,B,x)]
	set y1 [$g yaxis invtransform $_private($g,B,y)]
	set x2 [$g xaxis invtransform $_private($g,A,x)]
	set y2 [$g yaxis invtransform $_private($g,A,y)]
    } else {
	set x1 [$g xaxis invtransform $_private($g,A,x)]
	set y1 [$g yaxis invtransform $_private($g,A,y)]
	set x2 [$g xaxis invtransform $_private($g,B,x)]
	set y2 [$g yaxis invtransform $_private($g,B,y)]
    }
    set coords [list $x1 $y1 $x2 $y2]
    if { [$g marker exists "zoomOutline"] } {
	$g marker configure "zoomOutline" -coords $coords 
    } else {
	set X [lindex [$g xaxis use] 0]
	set Y [lindex [$g yaxis use] 0]
            $g marker create rectangle -coords $coords -name "zoomOutline"   \
		-mapx $X -mapy $Y -fill "" 
	
	set interval $_private($g,interval)
	set id [after $interval [list blt::Graph::MarchingAnts $g 0]]
	set _private($g,afterId) $id
    }
}


proc Blt_PostScriptDialog { g } {
    set top $g.top
    toplevel $top

    foreach var { center landscape decorations padx 
	pady paperwidth paperheight width height } {
	global $g.$var
	set $g.$var [$g postscript cget -$var]
    }
    set row 1
    set col 0
    label $top.title -text "PostScript Options"
    blt::table $top $top.title -cspan 7
    foreach bool { center landscape decorations } {
	set w $top.$bool-label
	label $w -text "-$bool" -font "courier 12"
	blt::table $top $row,$col $w -anchor e -pady { 2 0 } -padx { 0 4 }
	set w $top.$bool-yes
	global $g.$bool
	radiobutton $w -text "yes" -variable $g.$bool -value 1
	blt::table $top $row,$col+1 $w -anchor w
	set w $top.$bool-no
	radiobutton $w -text "no" -variable $g.$bool -value 0
	blt::table $top $row,$col+2 $w -anchor w
	incr row
    }
    label $top.modes -text "-colormode" -font "courier 12"
    blt::table $top $row,0 $top.modes -anchor e  -pady { 2 0 } -padx { 0 4 }
    set col 1
    foreach m { color greyscale } {
	set w $top.$m
	radiobutton $w -text $m -variable $g.colormode -value $m
	blt::table $top $row,$col $w -anchor w
	incr col
    }
    set row 1
    frame $top.sep -width 2 -bd 1 -relief sunken
    blt::table $top $row,3 $top.sep -fill y -rspan 6
    set col 4
    foreach value { padx pady paperwidth paperheight width height } {
	set w $top.$value-label
	label $w -text "-$value" -font "courier 12"
	blt::table $top $row,$col $w -anchor e  -pady { 2 0 } -padx { 0 4 }
	set w $top.$value-entry
	global $g.$value
	entry $w -textvariable $g.$value -width 8
	blt::table $top $row,$col+1 $w -cspan 2 -anchor w -padx 8
	incr row
    }
    blt::table configure $top c3 -width .125i
    button $top.cancel -text "Cancel" -command "destroy $top"
    blt::table $top $row,0 $top.cancel  -width 1i -pady 2 -cspan 3
    button $top.reset -text "Reset" -command "destroy $top"
    #blt::table $top $row,1 $top.reset  -width 1i
    button $top.print -text "Print" -command "blt::Graph::ResetPostScript $g"
    blt::table $top $row,4 $top.print  -width 1i -pady 2 -cspan 2
}

proc blt::Graph::ResetPostScript { g } {
    foreach var { center landscape decorations padx 
	pady paperwidth paperheight width height } {
	global $g.$var
	set old [$g postscript cget -$var]

	if { [catch {$g postscript configure -$var [set $g.$var]}] != 0 } {
	    $g postscript configure -$var $old
	    set $g.$var $old
	}
    }
    $g postscript output "out.ps"
    flush stdout
}
