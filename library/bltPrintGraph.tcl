
package require BLT
package require Itk

#option add *PrintGraph.height 3i widgetDefault
#option add *PrintGraph.width 3i widgetDefault
option add *PrintGraph*Entry*background "white" widgetDefault
option add *PrintGraph*Entry*width "6" widgetDefault
option add *PrintGraph*Font "Arial 9" widgetDefault
option add *PrintGraph.activeColor blue widgetDefault
option add *PrintGraph.controlBackground gray widgetDefault
option add *PrintGraph.dimColor gray widgetDefault
option add *PrintGraph.gridColor #d9d9d9 widgetDefault

itk::usual BltComboMenu {
    #empty
}
itk::usual BltTkFrame {
    #empty
}
itk::usual BltComboEntry {
    #empty
}

itcl::class blt::PrintGraph {
    inherit itk::Widget

    constructor {args} {}
    destructor {}

    private variable _graph "";         # Original graph. 
    private variable _clone "";         # Cloned graph.
    private variable _preview "";       # Preview image.
    private variable _outputFormatType "";

    public method print { graph }
    public method reset {}

    private method CopyOptions { cmd orig clone {exclude {}}} 
    private method CloneGraph { orig }

    private method BuildGeneralTab {}
    private method BuildLegendTab {}
    private method BuildAxisTab {}
    private method SetOption { opt }
    private method SetComponentOption { comp opt }
    private method SetNamedComponentOption { comp name opt }
    private method SetNamedComponentOptionValue { comp name opt new }
    private method GetAxis {}
    private method GetElement { args }
    private method RegeneratePreview {} 
    private method InitClone {} 
    private method Pixels2Inches { pixels } 
    private method Inches2Pixels { inches {defValue ""}} 
    private method Color2RGB { color } 

    private method ApplyGeneralSettings {} 
    private method ApplyLegendSettings {} 
    private method ApplyAxisSettings {} 
    private method ApplyElementSettings {} 
    private method ApplyLayoutSettings {} 
    private method InitializeSettings {} 
    private method DestroySettings {}
    private method GetOutput {}
    private method SetWaitVariable { state }
    private method SetLayoutOption { option } 
    private method GetAxisType { axis } 
    private method AddColorToMenu { menu name color } 
    private method EventuallyRedraw {}
    private method GetIcon {name}
    private variable _settings
    private variable _wait ""
    private variable _afterId  -1;	# For redraw events.
    private variable _iconpath "";
    private common _icons
    private variable _anchorText ""
    private variable _axisText ""
    private variable _colorIcon ""
    private variable _colorText ""
    private variable _dashesIcon ""
    private variable _dashesText ""
    private variable _formatIcon ""
    private variable _positionIcon ""
    private variable _symbolIcon ""
    private variable _symbolText ""
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body blt::PrintGraph::constructor { args } {
    set w [winfo pixels . 2.5i]
    set h [winfo pixels . 2i]
    set _preview [image create picture -width $w -height $h]
    set _iconpath [list [file join [pwd] images icons]]

    set bg [eval blt::background create linear \
		-low grey95 -high grey80 \
		-relativeto $itk_interior.tabs]
    #set bg grey92
    option add *BltTkLabel.background $bg
    option add *BltTkCheckbutton.background $bg
    option add *BltTkPushbutton.background $bg
    option add *BltTkFrame.background $bg
    option add *BltTabset.selectBackground $bg
    $itk_interior configure -background grey82
    itk_component add tabs {
        blt::tabset $itk_interior.tabs \
            -highlightthickness 0 -tearoff 0 -side top \
            -outerborderwidth 0 -gap 0 -borderwidth 1 \
            -outerpad 1 -troughcolor grey -dashes dot
    } {
        keep -cursor
        ignore -highlightthickness -borderwidth -background -dashes
    }
    set inner [frame $itk_interior.frame -bg grey]
    itk_component add preview {
        label $inner.preview \
            -highlightthickness 0 -bd 0 -image $_preview -width 2.5i \
	    -height 2.5i -background grey
    } {
        ignore -background
    }
    set bg grey90
    itk_component add ok {
        button $itk_interior.ok -text "Save" \
            -highlightthickness 0 -pady 2 -padx 0 \
            -command [itcl::code $this SetWaitVariable 1] \
            -compound left \
	    -background $bg \
	    -image [GetIcon download]
    } {
	ignore -background
    }
    itk_component add cancel {
	button $itk_interior.cancel \
	    -text "Cancel" \
	    -highlightthickness 0 -pady 2 -padx 0 \
	    -command [itcl::code $this SetWaitVariable 0] \
	    -compound left \
	    -background $bg \
	    -image [GetIcon cancel]
    } {
	ignore -background
    }
    blt::table $itk_interior \
        0,0 $inner -fill both \
        0,1 $itk_component(tabs) -fill both  \
        1,1 $itk_component(cancel) -padx 2 -pady 4 -width .7i -fill y \
        1,0 $itk_component(ok) -padx 2 -pady 4 -width .7i -fill y
    blt::table $inner \
        0,0 $itk_component(preview) -fill both -padx 10 -pady 10 

    #blt::table configure $itk_interior c1 c2 -resize none
    blt::table configure $itk_interior c0 -resize both
    if { [catch {
    BuildGeneralTab
    BuildAxisTab
    BuildLegendTab
    } errs] != 0 } {
	global errorInfo
	puts stderr "error: errorInfo=$errorInfo"
    }
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body blt::PrintGraph::destructor {} {
    destroy $_clone
    image delete $_preview
    array unset _settings 
    foreach name [array names _icons] {
	image delete $_icons($name)
    }
}

itcl::body blt::PrintGraph::DestroySettings {} {
    destroy $_clone
    set _clone ""
    set _graph ""
}

itcl::body blt::PrintGraph::reset {} {
    SetWaitVariable 0
}

itcl::body blt::PrintGraph::SetWaitVariable { state } {
    set _wait $state
}

itcl::body blt::PrintGraph::print { w } {
    set _graph $w
    CloneGraph $w
    InitClone
    InitializeSettings
    SetWaitVariable 0
    tkwait variable [itcl::scope _wait]
    set output ""
    if { $_wait } {
        set output [GetOutput]
    }
    DestroySettings 
    return $output
}

itcl::body blt::PrintGraph::GetOutput {} {
    # Get the selected format of the download.
    set page $itk_component(graph_page)
    set m $page.format.menu
    set format [$m item cget $_settings(general-format) -value]
    if { $format == "jpg" } {
        set img [image create picture]
        $_clone snap $img
        $img export jpg -quality 100 -data bytes
        image delete $img
        return [list .jpg $bytes]
    } elseif { $format == "png" } {
        set img [image create picture]
        $_clone snap $img
        $img export png -data bytes
        image delete $img
        return [list .png $bytes]
    } elseif { $format == "gif" } {
        set img [image create picture]
        $_clone snap $img
        $img export gif -data bytes
        image delete $img
        return [list .gif $bytes]
    }

    # Handle encapsulated postscript or portable document format.

    # Append an "i" for the graph postscript component.
    set w $_settings(layout-width)i
    set h $_settings(layout-height)i

    $_clone postscript configure \
        -decoration yes \
        -center yes \
        -width $w -height $h
    
    set psdata [$_clone postscript output]
    if { $format == "eps" } { 
        return [list .$format $psdata]
    }
    set f [open "junk.ps" "w"]
    puts $f $psdata
    close $f

    set cmd ""
    # | eps2eps << $psdata 
    lappend cmd "|" "/usr/bin/gs" \
        "-q" "-sDEVICE=eps2write" "-sstdout=%stderr" \
        "-sOutputFile=-" "-dNOPAUSE" "-dBATCH" "-dSAFER" \
        "-dDEVICEWIDTH=250000" "-dDEVICEHEIGHT=250000" "-" "<<" "$psdata"
    if { $format == "pdf" } {
        # | eps2eps << $psdata | ps2pdf 
        lappend cmd "|" "/usr/bin/gs" \
            "-q" "-sDEVICE=pdfwrite" "-sstdout=%stderr" \
            "-sOutputFile=-" "-dNOPAUSE" "-dBATCH" "-dSAFER" \
            "-dCompatibilityLevel=1.4" "-c" ".setpdfwrite" "-f" "-" 
    }
    if { [catch {
        set f [open $cmd "r"]
        fconfigure $f -translation binary -encoding binary
        set output [read $f]
        close $f
    } err ] != 0 } {
        global errorInfo 
        puts stderr "failed to generate file: $err\n$errorInfo"
        return ""
    }
    return [list .$format $output]
}

itcl::body blt::PrintGraph::CopyOptions { cmd orig clone {exclude {}} } {
    set all [eval $orig $cmd]
    set configLine $clone
    foreach name $exclude {
        set ignore($name) 1
    }
    foreach arg $cmd {
        lappend configLine $arg
    }
    foreach option $all {
        if { [llength $option] != 5 } {
            continue
        }
        set switch [lindex $option 0]
        set initial [lindex $option 3]
        set current [lindex $option 4]
        if { [info exists ignore($switch)] } {
            continue
        }
        if { [string compare $initial $current] == 0 } {
            continue
        }
        lappend configLine $switch $current
    }
    eval $configLine
}

itcl::body blt::PrintGraph::CloneGraph { orig } {
    set top $itk_interior
    if { [winfo exists $top.graph] } {
        destroy $top.graph
    }
    set _clone [blt::graph $top.graph]
    CopyOptions "configure" $orig $_clone
    # Axis component
    foreach axis [$orig axis names] {
        if { $axis == "z" } {
            continue
        }
        if { [$orig axis cget $axis -hide] } {
            continue
        }
        if { [$_clone axis name $axis] == "" } {
            $_clone axis create $axis
        }
        CopyOptions [list axis configure $axis] $orig $_clone
    }
    foreach axis { x y x2 y2 } {
        $_clone ${axis}axis use [$orig ${axis}axis use]
    }
    # Pen component
    foreach pen [$orig pen names] {
        if { [$_clone pen name $pen] == "" } {
            $_clone pen create $pen
        }
        CopyOptions [list pen configure $pen] $orig $_clone
    }
    # Marker component
    foreach marker [$orig marker names] {
        $_clone marker create [$orig marker type $marker] -name $marker
        CopyOptions [list marker configure $marker] $orig $_clone -name
    }
    # Element component
    foreach elem [$orig element names] {
        set oper [$orig element type $elem] 
        $_clone $oper create $elem
        CopyOptions [list $oper configure $elem] $orig $_clone -data 
        if { [$_clone $oper cget $elem -hide] } {
            $_clone $oper configure $elem -label "" 
        }
    }
    # Fix element display list
    $_clone element show [$orig element show]
    # Legend component
    CopyOptions {legend configure} $orig $_clone
    set m $itk_component(symbol_menu)
    $_clone element create dummy -linewidth 1 \
	-color black -fill grey90 -outline blue
    foreach name {
	"square"
	"circle"
	"diamond"
	"plus" 
	"cross" 
	"splus" 
	"scross" 
	"triangle" 
    } {
	$_clone element configure dummy -symbol $name
	set img [image create picture]
	$_clone legend icon dummy $img
	set _icons($name) $img
	$m item configure $name -icon $img
    }
    $_clone element configure dummy -linewidth 1 -symbol none 
    set m $itk_component(dashes_menu)
    foreach {value name} {
	""	   "solid"  
	"1"	   "dot" 
	"5 2"	   "dash" 
	"2 4 2"    "dash-dot" 
	"2 4 2 2"  "dash-dot-dot"
    } {
	$_clone element configure dummy -dashes $value
	set img [image create picture]
	$_clone legend icon dummy $img
	set _icons($name) $img
	$m item configure $name -icon $img
    }
    $_clone element delete dummy

    # Postscript component
    CopyOptions {postscript configure} $orig $_clone
    # Crosshairs component
    CopyOptions {crosshairs configure} $orig $_clone

    # Create markers representing lines at zero for the x and y axis.
    $_clone marker create line -name x-zero \
        -coords "0 -Inf 0 Inf" -dashes 1 -hide yes
    $_clone marker create line -name y-zero \
        -coords "-Inf 0 Inf 0" -dashes 1 -hide yes
}

itcl::body blt::PrintGraph::InitClone {} {
    
    $_clone configure -width 3.4i -height 3.4i -background white \
        -borderwidth 0 \
        -leftmargin 0 \
        -rightmargin 0 \
        -topmargin 0 \
        -bottommargin 0
    
    # Kill the title and create a border around the plot
    $_clone configure \
        -title "" \
        -plotborderwidth 1 -plotrelief solid  \
        -plotbackground white -plotpadx 0 -plotpady 0

    set _settings(layout-width) [Pixels2Inches [$_clone cget -width]]
    set _settings(layout-height) [Pixels2Inches [$_clone cget -height]]

    set _settings(legend-fontfamily) helvetica
    set _settings(legend-fontsize)   10
    set _settings(legend-fontweight) normal
    set _settings(legend-fontslant)  roman
    set font "helvetica 10 normal roman"
    $_clone legend configure \
        -position right \
        -font $font \
        -hide yes -borderwidth 0 -background white -relief solid \
        -anchor nw -activeborderwidth 0
    # 
    set _settings(axis-ticks-fontfamily) helvetica
    set _settings(axis-ticks-fontsize)   10
    set _settings(axis-ticks-fontweight) normal
    set _settings(axis-ticks-fontslant)  roman
    set _settings(axis-title-fontfamily) helvetica
    set _settings(axis-title-fontsize)   10
    set _settings(axis-title-fontweight) normal
    set _settings(axis-title-fontslant)  roman
    foreach axis [$_clone axis names] {
        if { $axis == "z" } {
            continue
        }
        if { [$_clone axis cget $axis -hide] } {
            continue
        }
        set _settings($axis-ticks-fontfamily) helvetica
        set _settings($axis-ticks-fontsize)   10
        set _settings($axis-ticks-fontweight) normal
        set _settings($axis-ticks-fontslant)  roman
        set _settings($axis-title-fontfamily) helvetica
        set _settings($axis-title-fontsize)   10
        set _settings($axis-title-fontweight) normal
        set _settings($axis-title-fontslant)  roman
        set tickfont "helvetica 10 normal roman"
        set titlefont "helvetica 10 normal roman"
        $_clone axis configure $axis -ticklength 5  \
            -majorticks {} -minorticks {}
        $_clone axis configure $axis \
            -tickfont $tickfont \
            -titlefont $titlefont
    }
    set count 0
    foreach elem [$_clone element names] {
        if { [$_clone element type $elem] == "bar" } {
            continue
        }
        incr count
        if { [$_clone element cget $elem -linewidth] > 1 } {
            $_clone element configure $elem -linewidth 1 -pixels 3 
        }
    }
    if { $count == 0 } {
        # There are no "line" elements in the graph. 
        # Remove the symbol and dashes controls.
        set page $itk_component(legend_page)
        # May have already been forgotten.
        catch { 
            blt::table forget $page.symbol_l
            blt::table forget $page.symbol
            blt::table forget $page.dashes_l
            blt::table forget $page.dashes 
        }
    }
}

itcl::body blt::PrintGraph::SetOption { opt } {
    set new $_settings($opt) 
    set old [$_clone cget $opt]
    set code [catch [list $_clone configure $opt $new] err]
    if { $code != 0 } {
        bell
        global errorInfo
        puts stderr "$err: $errorInfo"
        set _settings($opt) $old
        $_clone configure $opt $old
    }
}

itcl::body blt::PrintGraph::SetComponentOption { comp opt } {
    set new $_settings($comp$opt) 
    set old [$_clone $comp cget $opt]
    set code [catch [list $_clone $comp configure $opt $new] err]
    if { $code != 0 } {
        bell
        global errorInfo
        puts stderr "$err: $errorInfo"
        set _settings($comp$opt) $old
        $_clone $comp configure $opt $old
    }
}

itcl::body blt::PrintGraph::SetNamedComponentOption { comp name opt } {
    set new $_settings($comp$opt) 
    set old [$_clone $comp cget $name $opt]
    set code [catch [list $_clone $comp configure $name $opt $new] errs]
    if { $code != 0 } {
        bell
        global errorInfo
        puts stderr "$errs: $errorInfo"
        set _settings($comp$opt) $old
        $_clone $comp configure $name $opt $old
    }
}

itcl::body blt::PrintGraph::SetNamedComponentOptionValue { comp name opt new } {
    set old [$_clone $comp cget $name $opt]
    set code [catch [list $_clone $comp configure $name $opt $new] errs]
    if { $code != 0 } {
        bell
        global errorInfo
        puts stderr "$errs: $errorInfo"
        set _settings($comp$opt) $old
        $_clone $comp configure $name $opt $old
    }
}

itcl::body blt::PrintGraph::RegeneratePreview {} {
    update 
    set img [image create picture]
    set w [Inches2Pixels $_settings(layout-width) 3.4]
    set h [Inches2Pixels $_settings(layout-height) 3.4]
    $_clone snap $img -width $w -height $h

    set pixelsPerInch [winfo pixels . 1i]
    set cw [winfo width $itk_component(preview)]
    set ch [winfo height $itk_component(preview)]
    set rw [expr 2.5*$pixelsPerInch]
    set rh [expr 2.5*$pixelsPerInch]
    set maxwidth $rw
    set maxheight $rh
    if { $maxwidth > $cw } {
        set maxwidth $cw 
    }
    if { $maxheight > $ch } {
        set maxheight $ch 
    }
    set sx [expr double($maxwidth)/$w]
    set sy [expr double($maxheight)/$h]
    set s [expr min($sx,$sy)]

    set pw [expr int(round($s * $w))]
    set ph [expr int(round($s * $h))]
    $_preview configure -width $pw -height $ph
    #.labeltest.label configure -image $img
    $_preview resample $img -filter box
    image delete $img
}

itcl::body blt::PrintGraph::Pixels2Inches { pixels } {
    if { [llength $pixels] == 2 } {
        set pixels [lindex $pixels 0]
    }
    set pixelsPerInch [winfo pixels . 1i]
    set inches [expr { double($pixels) / $pixelsPerInch }]
    return [format %.3g ${inches}]
}

itcl::body blt::PrintGraph::Inches2Pixels { inches {defValue ""}} {
    set n [scan $inches %g dummy]
    if { $n != 1  && $defValue != "" } {
        set inches $defValue
    }
    return  [winfo pixels . ${inches}i]
}

itcl::body blt::PrintGraph::Color2RGB { color } {
    foreach { r g b } [winfo rgb $_clone $color] {
        set r [expr round($r / 257.0)]
        set g [expr round($g / 257.0)]
        set b [expr round($b / 257.0)]
    }
    return [format "\#%02x%02x%02x" $r $g $b]
}

itcl::body blt::PrintGraph::GetAxisType { axis } {
    foreach type { x y x2 y2 } {
        set axes [$_clone ${type}axis use]
        if { [lsearch $axes $axis] >= 0 } {
            return [string range $type 0 0]
        }
    }
    return ""
}

itcl::body blt::PrintGraph::GetAxis {} {
    set axis $_axisText
    foreach option { -min -max -loose -title -stepsize -subdivisions } {
        set _settings(axis$option) [$_clone axis cget $axis $option]
    }
    set font [$_clone axis cget $axis -titlefont]
    set _settings(axis-title-fontfamily) [lindex $font 0]
    set _settings(axis-title-fontsize) [lindex $font 1]
    set _settings(axis-title-fontweight) [lindex $font 2]
    set _settings(axis-title-fontslant) [lindex $font 3]
    set font [$_clone axis cget $axis -tickfont]
    set _settings(axis-ticks-fontfamily) [lindex $font 0]
    set _settings(axis-ticks-fontsize) [lindex $font 1]
    set _settings(axis-ticks-fontweight) [lindex $font 2]
    set _settings(axis-ticks-fontslant) [lindex $font 3]

    set type [GetAxisType $axis]
    set _settings(axis-plotpad${type}) \
        [Pixels2Inches [$_clone cget -plotpad${type}]]
    set _settings(axis-zero) [$_clone marker cget ${type}-zero -hide]
}

itcl::body blt::PrintGraph::GetElement { args } {
    set index 1
    set page $itk_component(legend_page)
    set elem $_settings(legend-element)
    set _settings(element-label) [$_clone element cget $elem -label]

    set color [$_clone element cget $elem -color]
    set found 0
    set m $itk_component(color_menu)
    foreach item [$m names] {
	set c [$m item cget $item -value] 
	if { $c == $color } {
	    set color $item
	    set found 1
	    break
	}
    }
    if { !$found } {
	AddColorToMenu $m $color $color
    }
    $m select $color

    if { [$_clone element type $elem] != "bar" } {
	foreach item { symbol dashes } {
	    set m $itk_component(${item}_menu)
	    set value [$_clone element cget $elem -$item]
	    set index [$m index -value $value]
	    if { $index == -1 } {
		set index [$m size]
		$m add -text $value -value $value
	    }
	    set _settings(element-$item) $value
	    set _${item}Text [$m item cget $index -text]
	    set _${item}Icon [$m item cget $index -icon]
	}
    }
}

itcl::body blt::PrintGraph::BuildGeneralTab {} {
    itk_component add graph_page {
        blt::tk::frame $itk_component(tabs).graph_page 
    }
    set page $itk_component(graph_page)
    $itk_component(tabs) insert end "graph" \
        -text "General" -padx 2 -pady 2 -window $page -fill both
    blt::tk::label $page.format_l -text "format"
    set m $page.format.menu
    blt::comboentry $page.format \
	-width 2i \
        -textvariable [itcl::scope _settings(general-format)] \
        -editable no -menu $m \
	-iconvariable [itcl::scope _formatIcon] \
        -command [itcl::code $this ApplyGeneralSettings]
    blt::combomenu $m \
        -xscrollbar $m.xs \
        -yscrollbar $m.ys  \
        -textvariable [itcl::scope _settings(general-format)] \
	-iconvariable [itcl::scope _formatIcon] \
        -height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    $m add -value "pdf" -text "PDF Portable Document Format"  \
	-icon [GetIcon pdf]
    $m add -value "ps"  -text "PS PostScript Format"  \
	-icon [GetIcon ps]
    $m add -value "eps" -text "EPS Encapsulated PostScript"  \
	-icon [GetIcon eps]
    $m add -value "jpg" -text "JPEG Joint Photographic Experts Group Format"  \
	-icon [GetIcon jpg]
    $m add -value "png" -text "PNG Portable Network Graphics Format"  \
	-icon [GetIcon png]
    $m add -value "gif" -text "GIF Graphics Interchange Format" \
	-icon [GetIcon gif]

    blt::tk::label $page.title_l -text "title" 
    entry $page.title \
	-textvariable [itcl::scope _settings(general-title)]
    bind  $page.title <KeyPress-Return> [itcl::code $this ApplyGeneralSettings]

    set f $page.layout
    blt::tk::frame $page.layout
    blt::tk::label $f.width_l -text "width (in)"  
    entry $f.width -width 6 \
        -textvariable [itcl::scope _settings(layout-width)]
    bind  $f.width <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    blt::tk::label $f.height_l -text "height (in)" 
    entry $f.height -width 6 \
        -textvariable [itcl::scope _settings(layout-height)]
    bind  $f.height <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    blt::tk::label $f.margin_l -text "Margins"  

    blt::tk::label $f.left_l -text "left (in)"
    entry $f.left -width 6 \
        -textvariable [itcl::scope _settings(layout-leftmargin)]
    bind  $f.left <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    blt::tk::label $f.right_l -text "right (in)" 
    entry $f.right -width 6 \
        -textvariable [itcl::scope _settings(layout-rightmargin)]
    bind  $f.right <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    blt::tk::label $f.top_l -text "top (in)"
    entry $f.top -width 6 \
        -textvariable [itcl::scope _settings(layout-topmargin)]
    bind  $f.top <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    blt::tk::label $f.bottom_l -text "bottom (in)"
    entry $f.bottom -width 6 \
        -textvariable [itcl::scope _settings(layout-bottommargin)]
    bind  $f.bottom <KeyPress-Return> [itcl::code $this ApplyLayoutSettings]

    blt::tk::label $f.map -image [GetIcon graphmargins]
    blt::table $f \
        0,2 $f.map -fill both -rspan 7 -padx 8 -pady 4 \
        0,0 $f.width_l -anchor e \
        0,1 $f.width -fill x  \
        1,0 $f.height_l -anchor e \
        1,1 $f.height -fill x \
        3,0 $f.left_l -anchor e \
        3,1 $f.left -fill x  \
        4,0 $f.right_l -anchor e \
        4,1 $f.right -fill x \
        5,0 $f.top_l -anchor e \
        5,1 $f.top -fill x \
        6,0 $f.bottom_l -anchor e \
        6,1 $f.bottom -fill x  

    blt::table configure $f c0 r* -resize none 
    blt::table configure $f c1 r7 -resize both

    blt::table $page \
        0,0 $page.title_l -anchor e -padx { 0 2 } \
        0,1 $page.title -fill x \
        2,0 $page.layout -fill both -cspan 2  \
        4,0 $page.format_l -anchor e -padx { 0 2 } \
        4,1 $page.format -fill x 

    blt::table configure $page r* -resize none  -pady { 0 2 }
    blt::table configure $page c* -resize none
    blt::table configure $page r1 r3 -height 0.125i
    blt::table configure $page c1 -resize both

}

itcl::body blt::PrintGraph::BuildLegendTab {} {
    itk_component add legend_page {
        blt::tk::frame $itk_component(tabs).legend_page 
    }
    set page $itk_component(legend_page)
    $itk_component(tabs) insert end "legend" \
        -text "Legend" -padx 2 -pady 2 -window $page -fill both

    blt::tk::checkbutton $page.show -text "show legend" \
        -offvalue 1 -onvalue 0 \
        -variable [itcl::scope _settings(legend-hide)]  \
        -command [itcl::code $this ApplyLegendSettings] 

    blt::tk::label $page.position_l -text "position"
    set m $page.position.menu
    blt::comboentry $page.position \
        -width 1i \
        -textvariable [itcl::scope _settings(legend-position)] \
        -editable no -menu $m \
	-iconvariable [itcl::scope _positionIcon] \
        -command [itcl::code $this ApplyGeneralSettings]
    blt::combomenu $m \
        -xscrollbar $m.xs \
        -yscrollbar $m.ys  \
        -textvariable [itcl::scope _settings(legend-position)] \
	-iconvariable [itcl::scope _positionIcon] \
        -height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    foreach { value name } {
        "leftmargin"	"left margin"  
        "rightmargin"	"right margin"  
        "bottommargin"	"bottom margin"  
        "topmargin"	"top margin"  
        "plotarea"	"inside plot"
    } {
	$m add -text $name -value $value
    }

    blt::tk::label $page.anchor_l -text "anchor"
    set m $page.anchor.menu
    blt::comboentry $page.anchor \
        -width 1i \
        -textvariable [itcl::scope _anchorText] \
        -editable no -menu $m \
        -command [itcl::code $this ApplyLegendSettings]
    blt::combomenu $m \
        -xscrollbar $m.xs \
        -yscrollbar $m.ys  \
        -textvariable [itcl::scope _anchorText] \
        -valuevariable [itcl::scope _settings(legend-anchor)] \
        -height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    foreach { value name } {
        "nw" "northwest"  
        "n" "north"  
        "ne" "northeast"  
        "sw" "southwest"  
        "s" "south"  
        "se" "southeast"  
        "c" "center"  
        "e" "east"  
        "w" "west"  
    } {
	$m add -text $name -value $value
    }
    set _anchorText "northwest"
    
    blt::tk::checkbutton $page.border -text "border" \
        -variable [itcl::scope _settings(legend-borderwidth)] \
        -onvalue 1 -offvalue 0 \
        -command [itcl::code $this ApplyLegendSettings]

    blt::tk::label $page.element_l -text "element" 
    set m $page.element.menu
    blt::comboentry $page.element \
	-width 2i \
        -textvariable [itcl::scope _settings(legend-element)] \
        -editable no -menu $m \
        -command [itcl::code $this GetElement]
    itk_component add element_menu {
	blt::combomenu $m \
	    -xscrollbar $m.xs \
	    -yscrollbar $m.ys  \
	    -textvariable [itcl::scope _settings(legend-element)] \
	    -height { 0 2i }
    }
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys

    blt::tk::label $page.font_l -text "font"
    set m $page.fontfamily.menu
    blt::comboentry $page.fontfamily \
	-width 1i \
	-textvariable [itcl::scope _settings(legend-fontfamily)] \
	-editable no \
	-menu $m \
	-command [itcl::code $this ApplyLegendSettings]
    blt::combomenu $m \
	-xscrollbar $m.xs \
	-yscrollbar $m.ys  \
	-textvariable [itcl::scope _settings(legend-fontfamily)] \
	-height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    $m style create "courier" -font "{courier new} 9"
    $m style create "helvetica" -font "{arial} 9" 
    $m style create "newcentury" -font "{new century schoolbook} 9"
    $m style create "times" -font "{times new roman} 9"
    $m add -text "courier" -value "courier" -style "courier"
    $m add -text "helvetica" -value "helvetica"  -style "helvetica"
    $m add -text "new century schoolbook" -value "new*century*schoolbook" \
	-style "newcentury"
    $m add -text "symbol" -value "symbol" 
    $m add -text "times" -value "times" -style "times"
    $m item configure all -icon [GetIcon font]
    $page.fontfamily configure -icon [GetIcon font]

    set m $page.fontsize.menu
    blt::comboentry $page.fontsize \
	-textvariable [itcl::scope _settings(legend-fontsize)] \
	-editable no -menu $m \
	-command [itcl::code $this ApplyLegendSettings]
    blt::combomenu $m \
	-xscrollbar $m.xs \
	-yscrollbar $m.ys  \
	-textvariable [itcl::scope _settings(legend-fontsize)] \
	-height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    foreach size { "8" "9" "10" "11" "12" "14" "17" "18" "20" } {
	$m add -text $size -value $size
    }

    blt::tk::pushbutton $page.fontweight \
	-relief flat \
	-width 18 -height 18 \
	-onimage [GetIcon font-bold] \
	-offimage [GetIcon font-bold] \
	-onvalue "bold" -offvalue "normal" \
	-command [itcl::code $this ApplyLegendSettings] \
	-variable [itcl::scope _settings(legend-fontweight)]

    blt::tk::pushbutton $page.fontslant \
	-relief flat \
	-width 18 -height 18 \
	-onimage [GetIcon font-italic] \
	-offimage [GetIcon font-italic] \
	-onvalue "italic" -offvalue "roman" \
	-command [itcl::code $this ApplyLegendSettings] \
	-variable [itcl::scope _settings(legend-fontslant)]

    set f $page.frame 

    set bg [eval blt::background create linear -relativeto $itk_component(tabs)]
    set bg grey95
    option add *frame.BltTkLabel.background $bg
    option add *frame.BltTkCheckbutton.background $bg
    option add *frame.BltTkPushbutton.background $bg

    blt::tk::frame $f -bg $bg
    blt::tk::label $f.label_l -text "label" 
    entry $f.label \
        -background white \
	-width 10 \
        -textvariable [itcl::scope _settings(element-label)]
    bind  $f.label <KeyPress-Return> [itcl::code $this ApplyElementSettings]

    blt::tk::label $f.color_l -text "color"
    set m $f.color.menu
    blt::comboentry $f.color \
        -width 1i \
        -textvariable [itcl::scope _colorText] \
        -iconvariable [itcl::scope _colorIcon] \
        -editable no -menu $m \
        -command [itcl::code $this ApplyElementSettings]
    itk_component add color_menu {
	blt::combomenu $m \
	    -xscrollbar $m.xs \
	    -yscrollbar $m.ys  \
	    -textvariable [itcl::scope _colorText] \
	    -iconvariable [itcl::scope _colorIcon] \
	    -valuevariable [itcl::scope _settings(element-color)] \
	    -height { 0 2i }
    }
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    foreach {rgb name} {
        "#000000" "black" 
        "#ffffff" "white" 
        "#0000cd" "blue" 
        "#cd0000" "red" 
        "#00cd00" "green" 
        "#3a5fcd" "royal blue" 
        "#cdcd00" "yellow" 
        "#cd1076" "deep pink" 
        "#009acd" "deep sky blue" 
        "#00c5cd" "turquoise" 
        "#a2b5cd" "light steel blue" 
        "#7ac5cd" "cadet blue" 
        "#66cdaa" "aquamarine" 
        "#a2cd5a" "dark olive green" 
        "#cd9b9b" "rosy brown" 
        "#0000ff" "blue1" 
        "#ff0000" "red1" 
        "#00ff00" "green1" 
    } {
	AddColorToMenu $m $name $rgb
    }

    blt::tk::label $f.symbol_l -text "symbol" 
    set m $f.symbol.menu
    blt::comboentry $f.symbol \
        -width 1i \
        -textvariable [itcl::scope _symbolText] \
        -iconvariable [itcl::scope _symbolIcon] \
        -editable no \
	-menu $m \
        -command [itcl::code $this ApplyElementSettings]
    itk_component add symbol_menu {
	blt::combomenu $m \
	    -xscrollbar $m.xs \
	    -yscrollbar $m.ys  \
	    -textvariable [itcl::scope _symbolText] \
	    -iconvariable [itcl::scope _symbolIcon] \
	    -valuevariable [itcl::scope _settings(element-symbol)] \
	    -height { 0 2i }
    }
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    foreach name {
	"none" 
	"square"
	"circle"
	"diamond"
	"plus" 
	"cross" 
	"splus" 
	"scross" 
	"triangle" 
    } {
	$m add -text $name -value $name
    }
    blt::tk::label $f.dashes_l -text "line style" 
    set m $f.dashes.menu
    blt::comboentry $f.dashes \
        -width 1i \
        -textvariable [itcl::scope _dashesText] \
        -iconvariable [itcl::scope _dashesIcon] \
        -editable no -menu $m \
        -command [itcl::code $this ApplyElementSettings]
    itk_component add dashes_menu {
	blt::combomenu $m \
	    -xscrollbar $m.xs \
	    -yscrollbar $m.ys  \
	    -textvariable [itcl::scope _dashesText] \
	    -iconvariable [itcl::scope _dashesIcon] \
	    -valuevariable [itcl::scope _settings(element-dashes)] \
	    -height { 0 2i }
    }
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    foreach {value name} {
	""	   "solid"  
	"1"	   "dot" 
	"5 2"	   "dash" 
	"2 4 2"    "dash-dot" 
	"2 4 2 2"  "dash-dot-dot"
    } {
	$m add -text $name -value $value 
    }

    blt::table $f \
	0,0 $f.label_l -anchor e \
	0,1 $f.label -fill x  \
	1,0 $f.symbol_l -anchor e \
	1,1 $f.symbol -fill x \
	2,0 $f.color_l -anchor e \
	2,1 $f.color -fill x \
	3,0 $f.dashes_l -anchor e \
	3,1 $f.dashes -fill x 
    blt::table configure $f r* c* -resize none
    blt::table configure $f c1 -resize both
    blt::table configure $f r* -pady 3

    blt::table $page \
	0,0 $page.show -cspan 4 -anchor w \
	1,0 $page.border -cspan 4 -anchor w \
	2,0 $page.position_l -anchor e \
	2,1 $page.position -fill x \
	3,0 $page.anchor_l -anchor e \
	3,1 $page.anchor -fill x   \
	4,0 $page.font_l -anchor e  \
	4,1 $page.fontfamily -fill x \
	4,2 $page.fontsize -fill x \
	4,3 $page.fontweight -anchor e \
	4,4 $page.fontslant -anchor e \
	6,0 $page.element_l -anchor e -cspan 1 \
	6,1 $page.element -fill x -cspan 2 \
	7,1 $f -cspan 2 -anchor w -fill x

    blt::table configure $page r* -resize none -pady { 0 2 }
    blt::table configure $page c* -resize none
    blt::table configure $page  r* -pady 1
    blt::table configure $page r5 -height 0.125i
    blt::table configure $page c1 r8 -resize both
    blt::table configure $page r0 -pady { 0.125i 0 }

}

itcl::body blt::PrintGraph::BuildAxisTab {} {
    itk_component add axis_page {
	blt::tk::frame $itk_component(tabs).axis_page 
    }
    set page $itk_component(axis_page)
    $itk_component(tabs) insert end "axis" \
	-text "Axis" -padx 2 -pady 2 -window $page -fill both
    
    blt::tk::label $page.axis_l -text "axis" 
    set m $page.axis.menu
    blt::comboentry $page.axis \
        -width 1i \
        -textvariable [itcl::scope _axisText] \
        -editable no -menu $m \
        -command [itcl::code $this GetAxis]
    itk_component add axis_menu {
	blt::combomenu $m \
	    -xscrollbar $m.xs \
	    -yscrollbar $m.ys  \
	    -textvariable [itcl::scope _axisText] \
	    -height { 0 2i }
    }
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys

    blt::tk::label $page.title_l -text "title" 
    entry $page.title \
	-textvariable [itcl::scope _settings(axis-title)]
    bind  $page.title <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    blt::tk::label $page.min_l -text "min"
    entry $page.min -width 10 \
	-textvariable [itcl::scope _settings(axis-min)]
    bind  $page.min <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    blt::tk::label $page.max_l -text "max"
    entry $page.max -width 10 \
	-textvariable [itcl::scope _settings(axis-max)]
    bind  $page.max <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    blt::tk::label $page.subdivisions_l -text "subdivisions"
    entry $page.subdivisions \
	-textvariable [itcl::scope _settings(axis-subdivisions)]
    bind  $page.subdivisions <KeyPress-Return> \
	[itcl::code $this ApplyAxisSettings]

    blt::tk::label $page.stepsize_l -text "step size"
    entry $page.stepsize \
	-textvariable [itcl::scope _settings(axis-stepsize)]
    bind  $page.stepsize <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    blt::tk::checkbutton $page.loose -text "loose limits" \
	-onvalue "always" -offvalue "0" \
	-variable [itcl::scope _settings(axis-loose)] \
	-command [itcl::code $this ApplyAxisSettings]

    blt::tk::label $page.plotpad_l -text "pad"  
    entry $page.plotpad -width 6 \
	-textvariable [itcl::scope _settings(axis-plotpad)]
    bind  $page.plotpad <KeyPress-Return> [itcl::code $this ApplyAxisSettings]

    blt::tk::checkbutton $page.grid -text "show grid lines" \
	-variable [itcl::scope _settings(axis-grid)] \
	-command [itcl::code $this ApplyAxisSettings]

    blt::tk::checkbutton $page.zero -text "mark zero" \
	-offvalue 1 -onvalue 0 \
	-variable [itcl::scope _settings(axis-zero)] \
	-command [itcl::code $this ApplyAxisSettings]

    blt::tk::label $page.tickfont_l -text "tick font"
    set m $page.tickfontfamily.menu
    blt::comboentry $page.tickfontfamily \
	-width 1i \
	-textvariable [itcl::scope _settings(axis-ticks-fontfamily)] \
	-editable no -menu $m \
	-command [itcl::code $this ApplyAxisSettings]
    blt::combomenu $m \
	-xscrollbar $m.xs \
	-yscrollbar $m.ys  \
	-textvariable [itcl::scope _settings(axis-ticks-fontfamily)] \
	-height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    $m style create "courier" -font "{courier new} 9"
    $m style create "helvetica" -font "{arial} 9"
    $m style create "newcentury" -font "{new century schoolbook} 9"
    $m style create "times" -font "{times new roman} 9"
    $m add -text "courier" -value "courier" -style "courier"
    $m add -text "helvetica" -value "helvetica"  -style "helvetica"
    $m add -text "new century schoolbook" -value "new*century*schoolbook" \
	-style "newcentury"
    $m add -text "symbol" -value "symbol" 
    $m add -text "times" -value "times" -style "times"
    $m item configure all -icon [GetIcon font]
    $page.tickfontfamily configure -icon [GetIcon font]

    set m $page.tickfontsize.menu
    blt::comboentry $page.tickfontsize \
	-textvariable [itcl::scope _settings(axis-ticks-fontsize)] \
	-editable no -menu $m \
	-command [itcl::code $this ApplyAxisSettings]
    blt::combomenu $m \
	-xscrollbar $m.xs \
	-yscrollbar $m.ys  \
	-textvariable [itcl::scope _settings(axis-ticks-fontsize)] \
	-height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    foreach size { "8" "9" "10" "11" "12" "14" "17" "18" "20" } {
	$m add -text $size -value $size
    }

    blt::tk::pushbutton $page.tickfontweight \
	-relief flat \
	-width 18 -height 18 \
	-onimage [GetIcon font-bold] \
	-offimage [GetIcon font-bold] \
	-onvalue "bold" -offvalue "normal" \
	-command [itcl::code $this ApplyAxisSettings] \
	-variable [itcl::scope _settings(axis-ticks-fontweight)]

    blt::tk::pushbutton $page.tickfontslant \
	-relief flat \
	-width 18 -height 18 \
	-onimage [GetIcon font-italic] \
	-offimage [GetIcon font-italic] \
	-onvalue "italic" -offvalue "roman" \
	-command [itcl::code $this ApplyAxisSettings] \
	-variable [itcl::scope _settings(axis-ticks-fontslant)]

    blt::tk::label $page.titlefont_l -text "title font"
    set m $page.titlefontfamily.menu
    blt::comboentry $page.titlefontfamily \
	-width 1i \
	-textvariable [itcl::scope _settings(axis-title-fontfamily)] \
	-editable no -menu $m \
	-command [itcl::code $this ApplyAxisSettings]
    blt::combomenu $m \
	-xscrollbar $m.xs \
	-yscrollbar $m.ys  \
	-textvariable [itcl::scope _settings(axis-title-fontfamily)] \
	-height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    $m style create "courier" -font "{courier new} 9"
    $m style create "helvetica" -font "{arial} 9"
    $m style create "newcentury" -font "{new century schoolbook} 9"
    $m style create "times" -font "{times new roman} 9"
    $m add -text "courier" -value "courier" -style "courier"
    $m add -text "helvetica" -value "helvetica"  -style "helvetica"
    $m add -text "new century schoolbook" -value "new*century*schoolbook" \
	-style "newcentury"
    $m add -text "symbol" -value "symbol" 
    $m add -text "times" -value "times" -style "times"
    $m item configure all -icon [GetIcon font]
    $page.titlefontfamily configure -icon [GetIcon font]

    set m $page.titlefontsize.menu
    blt::comboentry $page.titlefontsize \
	-textvariable [itcl::scope _settings(axis-title-fontsize)] \
	-editable no -menu $m \
	-command [itcl::code $this ApplyAxisSettings]
    blt::combomenu $m \
	-xscrollbar $m.xs \
	-yscrollbar $m.ys  \
	-textvariable [itcl::scope _settings(axis-title-fontsize)] \
	-height { 0 2i } 
    blt::tk::scrollbar $m.xs
    blt::tk::scrollbar $m.ys
    foreach size { "8" "9" "10" "11" "12" "14" "17" "18" "20" } {
	$m add -text $size -value $size
    }

    blt::tk::pushbutton $page.titlefontweight \
	-relief flat \
	-width 18 -height 18 \
	-onimage [GetIcon font-bold] \
	-offimage [GetIcon font-bold] \
	-onvalue "bold" -offvalue "normal" \
	-command [itcl::code $this ApplyAxisSettings] \
	-variable [itcl::scope _settings(axis-title-fontweight)]

    blt::tk::pushbutton $page.titlefontslant \
	-relief flat \
	-width 18 -height 18 \
	-onimage [GetIcon font-italic] \
	-offimage [GetIcon font-italic] \
	-onvalue "italic" -offvalue "roman" \
	-command [itcl::code $this ApplyAxisSettings] \
	-variable [itcl::scope _settings(axis-title-fontslant)]

    blt::table $page \
	0,0 $page.axis_l -anchor e  -pady 6 \
	0,1 $page.axis -anchor w -cspan 6 \
	2,1 $page.title_l -anchor e \
	2,2 $page.title -fill x -cspan 5 \
	3,1 $page.min_l -anchor e \
	3,2 $page.min -fill x \
	4,1 $page.max_l -anchor e \
	4,2 $page.max -fill x \
	5,1 $page.stepsize_l -anchor e \
	5,2 $page.stepsize -fill x \
	6,1 $page.subdivisions_l -anchor e \
	6,2 $page.subdivisions -fill x  \
	7,1 $page.titlefont_l -anchor e \
	7,2 $page.titlefontfamily -fill x -cspan 2 \
	7,4 $page.titlefontsize -fill x -padx { 4 0 }  \
	7,5 $page.titlefontweight -anchor e -padx 4 \
	7,6 $page.titlefontslant -anchor e -padx { 0 2 } \
	8,1 $page.tickfont_l -anchor e \
	8,2 $page.tickfontfamily -fill x -cspan 2 \
	8,4 $page.tickfontsize -fill x -padx { 4 0 }  \
	8,5 $page.tickfontweight -anchor e -padx 4 \
	8,6 $page.tickfontslant -anchor e -padx { 0 2 } \
	9,1 $page.plotpad_l -anchor e \
	9,2 $page.plotpad -fill both  \
	10,1 $page.loose -cspan 2 -anchor w \
	11,1 $page.grid -anchor w -cspan 2 \
	12,1 $page.zero -cspan 2 -anchor w 

    blt::table configure $page  r* c* -resize none
    blt::table configure $page  r* -pady 1
    blt::table configure $page  c1 -padx { 0 2 }
    blt::table configure $page r11 -resize both
}

itcl::body blt::PrintGraph::ApplyGeneralSettings {} {
    $_clone configure -title $_settings(general-title)
    RegeneratePreview
}

itcl::body blt::PrintGraph::ApplyLegendSettings {} {
    set page $itk_component(legend_page)

    foreach option { -hide -position -anchor -borderwidth } {
	SetComponentOption legend $option
    }
    lappend font $_settings(legend-fontfamily)
    lappend font $_settings(legend-fontsize)
    lappend font $_settings(legend-fontweight)
    lappend font $_settings(legend-fontslant)
    $_clone legend configure -font fixed -font $font

    # Set the font of the comboentry to the selected legend font.
    set m $page.fontfamily.menu
    set style [$m item cget $_settings(legend-fontfamily) -style]
    set font [$m style cget $style -font]
    $page.fontfamily configure -font $font
    ApplyElementSettings
}

itcl::body blt::PrintGraph::ApplyAxisSettings {} {
    set axis $_axisText
    set type [GetAxisType $axis]
    $_clone configure -plotpad${type} $_settings(axis-plotpad)
    foreach option { -grid -min -max -loose -title -stepsize -subdivisions } {
	SetNamedComponentOption axis $axis $option
    }
    set page $itk_component(axis_page)
    set font {}
    lappend font $_settings(axis-title-fontfamily)
    lappend font $_settings(axis-title-fontsize)
    lappend font $_settings(axis-title-fontweight)
    lappend font $_settings(axis-title-fontslant)
    $_clone axis configure $axis -titlefont fixed -titlefont $font
    set family $_settings(axis-title-fontfamily)
    if { $family == "symbol" } {
	set family helvetica
    }
    $page.titlefontfamily configure -font [list $family 9 normal roman]
    set font {}
    lappend font $_settings(axis-ticks-fontfamily)
    lappend font $_settings(axis-ticks-fontsize)
    lappend font $_settings(axis-ticks-fontweight)
    lappend font $_settings(axis-ticks-fontslant)
    $_clone axis configure $axis -tickfont fixed -tickfont $font
    set family $_settings(axis-ticks-fontfamily)
    if { $family == "symbol" } {
	set family helvetica
    }
    $page.tickfontfamily configure -font [list $family 9 normal roman]

    $_clone marker configure ${type}-zero -hide $_settings(axis-zero)

    RegeneratePreview
}

itcl::body blt::PrintGraph::ApplyElementSettings {} {
    set page $itk_component(legend_page)
    if { $_clone != "" } {
	set elem $_settings(legend-element)
	foreach option { -color -label } {
	    SetNamedComponentOption element $elem $option
	}
	$_clone element configure $elem \
	    -fill $_settings(element-color) \
	    -outline $_settings(element-color)
	
	if { [$_clone element type $elem] != "bar" } {
	    foreach option { -symbol -dashes } {
		SetNamedComponentOption element $elem $option 
	    }
	}
	RegeneratePreview
    }
}

itcl::body blt::PrintGraph::SetLayoutOption { opt } {
    set new [Inches2Pixels $_settings(layout$opt)]
    $_clone configure $opt $new
}

itcl::body blt::PrintGraph::ApplyLayoutSettings {} {
    foreach opt { -width -height -leftmargin -rightmargin -topmargin 
	-bottommargin } {
	set old [$_clone cget $opt]
	set code [catch { SetLayoutOption $opt } err]
	if { $code != 0 } {
	    bell
	    global errorInfo
	    puts stderr "$err: $errorInfo"
	    set _settings(layout$opt) [Pixels2Inches $old]
	    $_clone configure $opt [Pixels2Inches $old]
	}
    }
    RegeneratePreview
}


itcl::body blt::PrintGraph::InitializeSettings {} {
    # General settings
    set page $itk_component(graph_page)

    # Always set to "jpg" "ieee"
    set _settings(general-format)   \
	"JPEG Joint Photographic Experts Group Format" 
    $page.format.menu select "JPEG Joint Photographic Experts Group Format" 

    set _settings(general-title) [$_clone cget -title]

    # Layout settings
    set _settings(layout-width) [Pixels2Inches [$_clone cget -width]]
    set _settings(layout-height) [Pixels2Inches [$_clone cget -height]]
    set _settings(layout-leftmargin) \
	[Pixels2Inches [$_clone cget -leftmargin]]
    set _settings(layout-rightmargin) \
	[Pixels2Inches [$_clone cget -rightmargin]]
    set _settings(layout-topmargin) \
	[Pixels2Inches [$_clone cget -topmargin]]
    set _settings(layout-bottommargin) \
	[Pixels2Inches [$_clone cget -bottommargin]]

    # Legend settings
    set page $itk_component(legend_page)

    set names [$_clone element show]
    foreach name $names {
	$itk_component(element_menu) add -text $name
    }
    $itk_component(element_menu) select 0

    # Always set the borderwidth to be not displayed
    set _settings(legend-borderwidth) 0

    if { $_settings(legend-fontweight) == "bold" } {
	set _settings(legend-font-bold) 1
    }
    set _settings(legend-hide) [$_clone legend cget -hide]
    set _settings(legend-position) [$_clone legend cget -position]
    set _settings(legend-anchor) [$_clone legend cget -anchor]
    GetElement

    # Axis settings
    set page $itk_component(axis_page)
    set names [lsort [$_clone axis names]] 
    $itk_component(axis_menu) delete all
    foreach axis $names {
	if { $axis == "z" } {
	    continue
	}
	if { ![$_clone axis cget $axis -hide] } {
	    $itk_component(axis_menu) add -text $axis -value $axis
	}
	lappend axisnames $axis
    }
    set axis [lindex $names 0]

    # Always hide the zero line.
    set _settings(axis-zero) 1
    set _settings(axis-plotpad) [Pixels2Inches [$_clone cget -plotpadx]]
    # Pick the first axis to initially display
    $itk_component(axis_menu) select 0
    GetAxis 
    RegeneratePreview
}

itcl::body blt::PrintGraph::AddColorToMenu { m name color } {
    set icon [image create picture -width 29 -height 17]
    $icon blank 0x0
    $icon draw circle 7 7 8 -color black \
	-antialiased 1 -linewidth 0 -shadow { -width 1 -offset 1 }
    $icon draw circle 7 7 7 -color $color \
	-antialiased 1 -linewidth 0
    set _icons($name) $icon
    $m add -text $name -icon $icon -value $color 
}


itcl::body blt::PrintGraph::GetIcon {name} {
    # Already loaded? then return it directly
    if {[info exists _icons($name)]} {
        return $_icons($name)
    }

    # Search for the icon along the iconpath search path
    set file ""
    foreach dir $_iconpath {
        set path [file join $dir $name.*]
        set file [lindex [glob -nocomplain $path] 0]
        if {"" != $file} {
            break
        }
    }

    set img ""
    if { $file != "" } {
        switch -- [file extension $file] {
            .gif - .jpg - .png - .xpm - .tif - .xbm {
                set img [image create picture -file $file]
            }
        }
    }
    if { $img != "" } {
        set _icons($name) $img
    }
    return $img
}
