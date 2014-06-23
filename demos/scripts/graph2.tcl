option add *HighlightThicknes	0
option add *Tile			bgTexture
option add *Button.Tile			""

image create picture bgTexture -file ./images/chalk.gif

set configOptions [subst {
    InvertXY			no
    Axis.TickFont		{ {Sans Serif} 14 bold }
    Axis.TitleFont		{ {Sans Serif} 12 bold }
    BorderWidth			2
    Element.Pixels		8
    Element.ScaleSymbols	true
    Element.Smooth		parametriccubic
    Element.Smooth		catrom
    Element.lineWidth		1
    Font			{ {Serif} 10 }
    Foreground			white
    Legend.ActiveBorderWidth	2
    Legend.ActiveRelief		raised
    Legend.Anchor		ne
    Legend.BorderWidth		0
    Legend.Font			{ Serif 14 }
    Legend.Foreground		orange
    #Legend.Position		plotarea
    Legend.Hide			yes
    Legend.Relief		flat
    Postscript.Preview		yes
    Relief			raised
    Shadow			{ navyblue 2 }
    Title			"Bitmap Symbols" 
    degrees.Command		[namespace current]::FormatAxisLabel
    degrees.LimitsFormat	"Deg=%g"
    degrees.Subdivisions	0 
    degrees.Title		"Degrees" 
    degrees.stepSize		90 
    temp.LimitsFormat		"Temp=%g"
    temp.Title			"Temperature"
    y.Color			purple2
    y.LimitsFormat		"Y=%g"
    y.Rotate			90 
    y.Title			"Y" 
    y.loose			yes
    y2.Color			magenta3
    y2.Hide			no
    xy2.Rotate			270
    y2.Rotate			0
    y2.Title			"Y2" 
    y2.LimitsFormat		"Y2=%g"
    x2.LimitsFormat		"x2=%g"
}]

set resource [string trimleft $graph .]
foreach { option value } $configOptions {
    option add *$resource.$option $value
}

proc FormatAxisLabel {graph x} {
     format "%d%c" [expr int($x)] 0xB0
}

set max -1.0
set step 0.2

set letters { A B C D E F G H I J K L }
set count 0
toplevel .top
for { set level 30 } { $level <= 100 } { incr level 10 } {
    set color [format "#FF0d%0.2x" [expr round($level*2.55)]]
    set pen "pen$count"
    set symbol "symbol$count"
    set img [image create picture -width 25 -height 35]
    $img blank 0x00FFFFFF
    $img draw text [lindex $letters $count] 12 12 -color $color \
    	-font "Arial 10" -anchor c 
    label .top.l$level -image $img 
    pack .top.l$level
    $graph pen create $pen -symbol @$img 
    set min $max
    set max [expr $max + $step]
    lappend styles "$pen $min $max"
    incr count
}

$graph axis create temp \
    -color lightgreen \
    -title Temp  
$graph axis create degrees 
$graph xaxis use degrees

set tcl_precision 15
set pi1_2 [expr 3.14159265358979323846/180.0]

blt::vector w x sinX cosX radians
x seq -360.0 360.0 60.0
#x seq -360.0 -180.0 30.0
radians expr { x * $pi1_2 }
sinX expr sin(radians)
cosX expr cos(radians)
cosX dup w
blt::vector destroy radians

blt::vector xh xl yh yl
set pct [expr (($cosX(max) - $cosX(min)) * 0.025)]
yh expr {cosX + $pct}
yl expr {cosX - $pct}
set pct [expr ($x(max) - $x(min)) * 0.025]
xh expr {x + $pct}
xl expr {x - $pct}

set img [image create picture -width 25 -height 25]
$img blank 0x00FFFFFF

$img draw circle 12 12 5 -shadow 0 -linewidth 0 \
	-color 0x8FFF0000
$img draw circle 12 12 5 -shadow 0 -linewidth 1.5 \
	-color 0xFFFF0000

$graph element create line3 \
    -color green4 \
    -fill green \
    -label "cos(x)" \
    -mapx degrees \
    -symbol @$img \
    -styles $styles \
    -weights w \
    -x x \
    -y cosX  \

#    -ylow yl -yhigh yh 
set img [image create picture -width 25 -height 25]
$img blank 0x00FFFFFF

$img draw circle 12 12 5 -shadow 0 -linewidth 0 \
	-color 0x8F00FF00
$img draw circle 12 12 5 -shadow 0 -linewidth 1.5 \
	-color 0xFF00FF00


$graph element create line1 \
    -color orange \
    -outline black \
    -fill orange \
    -fill yellow \
    -label "sin(x)" \
    -mapx degrees \
    -pixels 6m \
    -symbol @$img \
    -x x \
    -y sinX 

Blt_ZoomStack $graph
Blt_Crosshairs $graph
#Blt_ActiveLegend $graph
Blt_ClosestPoint $graph
Blt_PrintKey $graph

