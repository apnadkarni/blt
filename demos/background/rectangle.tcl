package require BLT

set count 0
set img [image create picture -file images/buckskin.gif]

proc BuildControls { bg } {
    global count
    
    blt::tk::frame .f$count -bg $bg -width 200 -height 200
    blt::tk::button .f$count.l -text "Background $count" -bg $bg \
	-highlightbackground $bg -highlightthickness 10
    blt::table .f$count \
	$count,0 .f$count.l -padx 20  -pady 20 -fill both
    blt::table . \
	$count,0 .f$count -fill both
    incr count
}

proc LinearGradientBackground { args } {
    global count
    
    set bg [eval blt::background create linear $args -relativeto .f$count]
    BuildControls $bg
}

proc RadialGradientBackground { args } {
    global count
    
    set bg [eval blt::background create radial $args -relativeto .f$count]
    BuildControls $bg
}

proc TileBackground { args } {
    global count
    
    set bg [eval blt::background create tile $args -relativeto .f$count]
    BuildControls $bg
}

proc ConicalGradientBackground { args } {
    global count
    
    set bg [eval blt::background create conical $args -relativeto .f$count]
    BuildControls $bg
}

proc CheckerBackground { args } {
    global count
    
    set bg [eval blt::background create checker $args -relativeto .f$count]
    BuildControls $bg
}

proc StripeBackground { args } {
    global count
    
    set bg [eval blt::background create stripe $args -relativeto .f$count]
    BuildControls $bg
}

proc ColorBackground {} {
    global count
    
    set bg lightgreen
    BuildControls $bg
}


LinearGradientBackground \
    -jitter 3 \
    -from n \
    -to s \
    -colorscale linear \
    -lowcolor "grey97"\
    -highcolor "grey80"\
    -palette spectral.rgb \
    -repeat reversing 

RadialGradientBackground \
    -colorscale log \
    -center { 0.2 0.8} \
    -width 3.0 \
    -height 3.0 \
    -lowcolor "grey97"\
    -highcolor "grey80"\
    -palette spectral.rgb \
    -repeat reversing \

ConicalGradientBackground \
    -lowcolor "grey97"\
    -highcolor "grey80"


TileBackground \
    -image $img \
    -jitter 2 \
    -xoffset 0 \
    -border brown 

StripeBackground \
    -jitter 3 

CheckerBackground \
    -jitter 3 

ColorBackground 

