package require BLT

set count 0

proc LinearGradientBackground { args } {
    global count
    
    set ts .ts$count
    set bg [eval blt::background create linear $args -relativeto $ts]
    blt::tabset $ts -bg $bg -width 200 -height 100 -selectbackground $bg
    blt::tk::frame $ts.frame -bg $bg 
    blt::tk::button $ts.frame.l -text "Background $count" -bg $bg \
	-highlightbackground $bg -highlightthickness 5
    blt::table $ts.frame \
	$count,0 $ts.frame.l -padx 5  -pady 5 -fill both
    blt::table . \
	$count,0 $ts -fill both
    $ts add "Background $count" -window $ts.frame -fill both
    incr count
}

proc RadialGradientBackground { args } {
    global count
    
    set ts .ts$count
    set bg [eval blt::background create radial $args -relativeto $ts]
    blt::tabset $ts -bg $bg -width 200 -height 100 -selectbackground $bg
    blt::tk::frame $ts.frame -bg $bg 
    blt::tk::button $ts.frame.l -text "Background $count" -bg $bg \
	-highlightbackground $bg -highlightthickness 5
    blt::table $ts.frame \
	$count,0 $ts.frame.l -padx 5  -pady 5 -fill both
    blt::table . \
	$count,0 $ts -fill both
    $ts add "Background $count" -window $ts.frame -fill both
    incr count
}

set img [image create picture -file images/buckskin.gif]
proc TileBackground { args } {
    global count
    
    set ts .ts$count
    set bg [eval blt::background create tile $args -relativeto $ts]
    blt::tabset $ts -bg $bg -width 200 -height 100 -selectbackground $bg
    blt::tk::frame $ts.frame -bg $bg 
    blt::tk::button $ts.frame.l -text "Background $count" -bg $bg \
	-highlightbackground $bg -highlightthickness 5
    blt::table $ts.frame \
	$count,0 $ts.frame.l -padx 5  -pady 5 -fill both
    blt::table . \
	$count,0 $ts -fill both
    $ts add "Background $count" -window $ts.frame -fill both
    incr count
}

proc ConicalGradientBackground { args } {
    global count
    
    set ts .ts$count
    set bg [eval blt::background create conical $args -relativeto $ts]
    blt::tabset $ts -bg $bg -width 200 -height 100 -selectbackground $bg
    blt::tk::frame $ts.frame -bg $bg 
    blt::tk::button $ts.frame.l -text "Background $count" -bg $bg \
	-highlightbackground $bg -highlightthickness 5
    blt::table $ts.frame \
	$count,0 $ts.frame.l -padx 5  -pady 5 -fill both
    blt::table . \
	$count,0 $ts -fill both
    $ts add "Background $count" -window $ts.frame -fill both
    incr count
}

proc CheckerBackground { args } {
    global count
    
    set ts .ts$count
    set bg [eval blt::background create checker $args -relativeto $ts]
    blt::tabset $ts -bg $bg -width 200 -height 100 -selectbackground $bg
    blt::tk::frame $ts.frame -bg $bg 
    blt::tk::button $ts.frame.l -text "Background $count" -bg $bg \
	-highlightbackground $bg -highlightthickness 5
    blt::table $ts.frame \
	$count,0 $ts.frame.l -padx 5  -pady 5 -fill both
    blt::table . \
	$count,0 $ts -fill both
    $ts add "Background $count" -window $ts.frame -fill both
    incr count
}

proc StripeBackground { args } {
    global count
    
    set ts .ts$count
    set bg [eval blt::background create stripe $args -relativeto $ts]
    blt::tabset $ts -bg $bg -width 200 -height 100 -selectbackground $bg
    blt::tk::frame $ts.frame -bg $bg 
    blt::tk::button $ts.frame.l -text "Background $count" -bg $bg \
	-highlightbackground $bg -highlightthickness 5
    blt::table $ts.frame \
	$count,0 $ts.frame.l -padx 5  -pady 5 -fill both
    blt::table . \
	$count,0 $ts -fill both
    $ts add "Background $count" -window $ts.frame -fill both
    incr count
}

proc ColorBackground {} {
    global count
    
    set ts .ts$count
    set bg green
    blt::tabset $ts -bg $bg -width 200 -height 100 -selectbackground $bg
    blt::tk::frame $ts.frame -bg $bg 
    blt::tk::button $ts.frame.l -text "Background $count" -bg $bg \
	-highlightbackground $bg -highlightthickness 5
    blt::table $ts.frame \
	$count,0 $ts.frame.l -padx 5  -pady 5 -fill both
    blt::table . \
	$count,0 $ts -fill both
    $ts add "Background $count" -window $ts.frame -fill both
    incr count
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
    -border brown 

StripeBackground \
    -jitter 3 

CheckerBackground \
    -jitter 3 

ColorBackground 

