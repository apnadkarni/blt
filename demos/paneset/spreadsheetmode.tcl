
package require BLT

# Create a horizontal paneset (default orientation is horizontal).
# Mode is "spreadsheet".
blt::paneset .ps \
    -width 800 \
    -mode spreadsheet 

# Create windows for paneset. They must be children of the paneset.
option add *Divisions 4
blt::graph .ps.g -height 300 
blt::barchart .ps.b -height 300
blt::barchart .ps.b2 -height 300 

# Add 3 horizontal panes
.ps add -window .ps.g -fill both 
.ps add -window .ps.b -fill both 
.ps add -window .ps.b2 -fill both 

focus .ps

blt::table . \
    0,0 .ps -fill both 

blt::table configure . r1 -resize none
