#!../src/bltwish

package require BLT

source scripts/stipples.tcl

if { ![string match "*gray*" [winfo screenvisual .]] } {
    option add *Button.Background	red
    option add *TextMarker.Foreground	black
    option add *TextMarker.Background	yellow
    option add *LineMarker.Foreground	black
    option add *LineMarker.Background	yellow
    option add *PolyMarker.Fill		yellow2
    option add *PolyMarker.Outline	""
    option add *PolyMarker.Stipple	bdiagonal1
    option add *activeLine.Color	red4
    option add *activeLine.Fill		red2
    option add *Element.Color		purple
}

set data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}
set data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM1WBrM+rAEMigJ8c3Kb3OSII6kGABhp1JnaK1VGwjwKwtvHqNzzd263M3H4n2OH1QBwGw6
    nQkAOw==
}
set image [image create picture -data $data]

set graph [blt::graph .g]
blt::table . \
    0,0 $graph -fill both 

source scripts/graph2.tcl

$graph postscript configure \
    -landscape yes 

$graph configure \
    -width 6i \
    -height 4i \
    -title "Graph" \
    -background white \
    -plotpady 0 \
    -plotpadx 0 \
    -plotborderwidth 0 \
    -plotpady 0

$graph axis configure y \
    -titlefont "arial 10" \
    -title "Ratio" \
    -colorbarthickness 20 \
    -loose yes

$graph legend configure \
    -relief flat \
    -borderwidth 0

if 1 {
    blt::palette create red -colorformat name -cdata {
	"white" "red" 
    } -fade 50
    blt::palette create green -colorformat name -cdata {
	"white" "green" 
    } -fade 50
    blt::palette create bluegreen -colorformat name -cdata {
	"navyblue"  "lightblue" 
    } 
    $graph axis configure degrees -palette red -colorbarthickness 20
    $graph axis configure y -palette green 
    $graph element configure line1 -mapx degrees -colormap y 
    $graph element configure line3 -colormap degrees 
    $graph element raise line1
}

bind $graph <Control-ButtonPress-3> { MakeSnapshot }
bind $graph <Shift-ButtonPress-3> { 
    %W postscript output demo2.ps 
    update
    global tcl_platform
    if { $tcl_platform(platform) == "windows" } {
	%W snap -format emf demo2.emf
    }
}

$graph configure -title "Sine and Cosine"
set unique 0
proc MakeSnapshot {} {
    update idletasks
    global unique graph
    set top ".snapshot[incr unique]"
    set im [image create picture]
    $graph snap $im 210 150

    toplevel $top
    wm title $top "Snapshot \#$unique of \"[$graph cget -title]\""
    label $top.lab -image $im 
    button $top.but -text "Dismiss" -command "DestroySnapshot $top"
    blt::table $top $top.lab
    blt::table $top $top.but -pady 4 
    focus $top.but
}

proc DestroySnapshot { win } {
    set im [$win.lab cget -image]
    image delete $im
    destroy $win
    exit
}

after 2000 {$graph postscript output demo2.ps }

if { $tcl_platform(platform) == "windows" } {
    if 0 {
        set name [lindex [blt::printer names] 0]
        set printer {Lexmark Optra E310}
	blt::printer open $printer
	blt::printer getattrs $printer attrs
	puts $attrs(Orientation)
	set attrs(Orientation) Landscape
	set attrs(DocumentName) "This is my print job"
	blt::printer setattrs $printer attrs
	blt::printer getattrs $printer attrs
	puts $attrs(Orientation)
	after 5000 {
	    $graph print2 $printer
	    blt::printer close $printer
	}
    } else {
	after 5000 {
	 #   $graph print2 
	}
    }	
    if 1 {
	after 2000 {$graph snap -format emf CLIPBOARD}
    }
}
