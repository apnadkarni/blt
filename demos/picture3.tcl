package require BLT

set dest [image create picture -width 900 -height 600]

source ./data/usmap.tcl

set count 0
array set colors {
    0 red 
    1 green2 
    2 dodgerblue 
    3 cyan 
    4 orange
    5 purple
    6 brown
    7 violet
    8 limegreen
    9 lightblue
    10 yellow
    11 pink
    12 khaki
    13 grey
    14 navyblue
}

blt::vector all
blt::vector x
blt::vector y
set subset *
label .l -image $dest 
$dest blank white
set bg [image create picture -file images/blt98.gif]
$dest copy $bg -to [list 0 0 [image width $dest] [image height $dest]]
pack .l

proc CompareRegions { name1 name2 } {
    global us_regions

    foreach { x1 y1 } $us_regions($name1) break
    foreach { x2 y2 } $us_regions($name2) break
    if { $x1 < $x2 } {
	return 1
    }
    if { $x1 > $x2 } {
	return -1
    }
    if { $y1 < $y2 } {
	return 1
    }
    if { $y1 > $y2 } {
	return -1
    }
    return 0
}

set cnum -1
set regions [array names us_regions $subset]
foreach region [lsort -command CompareRegions $regions] {
    set coords $us_regions($region)
    incr cnum
    if { $cnum == 14 } {
	set cnum 0
    }
    all set $coords
#     all split x y
#     set min [blt::vector expr min(x)]
#     x expr { (x-$min)*3.0 + 10 }
#     set min [blt::vector expr min(y)]
#     y expr { (y-$min)*3.0 + 10 }
#     all merge x y 
    all expr { all * 3.0 }
    set coords [all values]
    $dest draw polygon -coords $coords -color $colors($cnum) \
	-antialiased 1 -shadow 1 
        #$dest draw line -coords $coords -color black 
#    foreach {rx ry} $coords {
#        $dest draw rectangle [expr int($rx-2)] [expr int($ry-2)] -width [expr int($rx+2)] -height [expr int($ry+2)] -color red 
#    }
    incr count
}

#$dest draw rectangle 200 200 -width 300 -height 300 -color green -shadow 0 \
    -radius 10 -alpha 100
#$dest draw rectangle 200 200 -width 400 -height 300 -color blue -linewidth 29 \
    -radius 19 -shadow 0 -antialiased 0 -alpha 180 

$dest draw circle 200 200 100 -color yellow -shadow 0 \
    -antialiased 1 -linewidth 10 

#$dest draw line -coords "200 200 500 200" -color black

