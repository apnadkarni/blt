package require BLT

. configure -bg green

blt::filmstrip .fs -width 550 -animate yes -scrolldelay 30 -height 400

blt::tk::frame .fs.inputs -width 500
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
blt::tk::button .controls.left -text "<-" -command {
    if { [.fs index prev] != -1 } {
	puts stderr prev=[.fs index prev]
	puts stderr next=[.fs index next]
	.fs see prev
	if { [.fs index prev] != -1 } {
	    .controls.left configure -text [.fs index prev]
	}
	if { [.fs index next] != -1 } {
	    .controls.right configure -text [.fs index next]
	}
    }
}
blt::tk::button .controls.right -text "->" -command {
    if { [.fs index next] != -1 } {
	puts stderr prev=[.fs index prev]
	puts stderr next=[.fs index next]
	.fs see next
	if { [.fs index prev] != -1 } {
	    .controls.left configure -text [.fs index prev]
	}
	if { [.fs index next] != -1 } {
	    .controls.right configure -text [.fs index next]
	}
    }
}

blt::table .controls \
    0,0 .controls.left -anchor w \
    0,2 .controls.right -anchor e

blt::table configure .controls c* -resize none
blt::table configure .controls c1 -resize both

blt::table . \
    0,0 .fs		-fill both \
    1,0 .controls	-fill x

    


