package require BLT

. configure -bg green

blt::filmstrip .fs -height 550 -animate yes -scrolldelay 30 -height 400 \
    -orient vertical -postscrollcommand { puts stderr "I'm done" }

blt::tk::frame .fs.inputs -height 500
blt::tk::frame .fs.results
blt::tk::frame .fs.outputs

.fs add inputs -window .fs.inputs 
.fs add results -window .fs.results 
.fs add outputs -window .fs.outputs 

scale .fs.inputs.scale
entry .fs.inputs.entry
blt::table .fs.inputs \
    0,0 .fs.inputs.scale -fill both \
    1,0 .fs.inputs.entry -fill both

text .fs.results.text -relief sunken -bg white
blt::table .fs.results \
    0,0 .fs.results.text -fill both -padx 20 -pady 20

blt::graph .fs.outputs.graph -width 500 -height 500
blt::table .fs.outputs \
    0,0 .fs.outputs.graph -fill both

blt::tk::frame .controls
blt::tk::button .controls.down -text "down" -command {
    if { [.fs index prev] != -1 } {
	puts stderr prev=[.fs index prev]
	puts stderr next=[.fs index next]
	.fs see prev
	if { [.fs index prev] != -1 } {
	    .controls.down configure -text [.fs index prev]
	}
	if { [.fs index next] != -1 } {
	    .controls.down configure -text [.fs index next]
	}
    }
}
blt::tk::button .controls.up -text "up" -command {
    if { [.fs index next] != -1 } {
	puts stderr prev=[.fs index prev]
	puts stderr next=[.fs index next]
	.fs see next
	if { [.fs index prev] != -1 } {
	    .controls.up configure -text [.fs index prev]
	}
	if { [.fs index next] != -1 } {
	    .controls.up configure -text [.fs index next]
	}
    }
}

blt::table .controls \
    0,0 .controls.down -anchor n \
    2,0 .controls.up -anchor s

blt::table configure .controls c* -resize none
blt::table configure .controls c1 -resize both

blt::table . \
    0,0 .fs		-fill both \
    0,1 .controls	-fill y

    


