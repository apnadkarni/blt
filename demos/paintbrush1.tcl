package require BLT

proc RadialGradientBrush  { args } {
    global img 

    set brush [blt::paintbrush create radial $args]
    $img blank white
    $img draw rectangle 25 25 -width 175 -height 175 -color $brush
    blt::paintbrush delete $brush
}

proc LinearGradientBrush  { args } {
    global img

    set brush [eval blt::paintbrush create linear $args]
    $img blank white
    $img draw rectangle 25 25 -width 175 -height 175 -color $brush
    blt::paintbrush delete $brush
}

proc RadialGradientBrush  { args } {
    global img

    set brush [eval blt::paintbrush create radial $args]
    $img blank white
    $img draw rectangle 25 25 -width 175 -height 175 -color $brush
    blt::paintbrush delete $brush
}

proc ConicalGradientBrush  { args } {
    global img

    set brush [eval blt::paintbrush create conical $args]
    $img blank white
    $img draw rectangle 25 25 -width 175 -height 175 -color $brush
    blt::paintbrush delete $brush
}

proc ColorBrush  { args } {
    global img

    set brush [eval blt::paintbrush create color $args]
    $img blank white
    $img draw rectangle 25 25 -width 175 -height 175 -color $brush
    blt::paintbrush delete $brush
}

proc TileBrush  { args } {
    global img

    set brush [eval blt::paintbrush create tile $args]
    $img blank white
    $img draw rectangle 25 25 -width 175 -height 175 -color $brush
    blt::paintbrush delete $brush
}

proc StripeBrush  { args } {
    global img

    set brush [eval blt::paintbrush create stripe $args]
    $img blank white
    $img draw rectangle 25 25 -width 175 -height 175 -color $brush
    blt::paintbrush delete $brush
}

proc CheckerBrush  { args } {
    global img

    set brush [eval blt::paintbrush create checker $args]
    $img blank white
    $img draw rectangle 25 25 -width 175 -height 175 -color $brush
    blt::paintbrush delete $brush
}

blt::comboentry .entry -menu .entry.menu -textvariable textVar

set tile [image create picture -file images/buckskin.gif]
set m .entry.menu
blt::combomenu .entry.menu -textvariable textVar

set img [image create picture -width 200 -height 200]
$img blank white

label .l -image $img
blt::table . \
    0,0 .entry -fill x \
    1,0 .l -fill both

$m add -text "Color Brush" -command {
    ColorBrush \
	-jitter 10 \
	-color "blue"\
	-opacity 100
}

$m add -text "Tile Brush" -command {
    TileBrush \
	-image $tile \
	-jitter 2 
}

$m add -text "Stripe Brush" -command {
    StripeBrush
}

$m add -text "Checker Brush" -command {
    CheckerBrush
}

$m add -text "Radial Brush" -command {
    RadialGradientBrush \
	-jitter 3 \
	-colorscale linear \
	-palette blue-to-green.rgb \
	-repeat reversing
}

$m add -text "Conical Brush" -command {
    ConicalGradientBrush 
}

$m add -text "Linear Brush" -command {
    LinearGradientBrush \
	-jitter 3 \
	-colorscale linear \
	-from n \
	-to s \
	-palette spectral.rgb \
	-repeat reversing
}

$m invoke "Color Brush"
