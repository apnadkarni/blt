
package require BLT

# Create a vertical paneset.
blt::paneset .vert -orient vertical 
# Create a horizontal paneset (default orientation is horizontal).
blt::paneset .vert.horiz  

# Create windows for paneset. They must be children of the paneset.
option add *Divisions 4
blt::graph .vert.g -height 200 -width 250
blt::barchart .vert.horiz.b  -height 200 -width 250 
blt::barchart .vert.horiz.b2 -height 200 -width 250 

# Add 2 vertical panes
.vert add -window .vert.g -fill both  
.vert add -window .vert.horiz -fill both 
# And 2 horizontal panes
.vert.horiz add -window .vert.horiz.b -fill both 
.vert.horiz add -window .vert.horiz.b2 -fill both

focus .vert
blt::table . \
    0,0 .vert -fill both

