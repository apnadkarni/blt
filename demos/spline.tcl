#!../src/bltwish

package require BLT
source scripts/demo.tcl

option add *graph.Element.ScaleSymbols true

# test to show spline over-shooting

set tcl_precision 15

# Make and fill small vectors
blt::vector x y
x seq 10 0 -0.5 
y expr sin(x^3)
x expr x*x
x sort y
blt::vector x2 y1 y2 y3

# make and fill (x only) large vectors
x2 populate x 10

# natural spline interpolation
blt::spline natural x y x2 y1

# quadratic spline interpolation
blt::spline quadratic x y x2 y2 

# make plot
blt::graph .graph 
.graph xaxis configure -title "x^2" -grid yes 
.graph yaxis configure -title "sin(y^3)" -grid yes 

.graph pen configure activeLine -pixels 5
.graph element create Original \
    -x x -y y \
    -color red4 \
    -fill red \
    -pixels 5 \
    -symbol circle

.graph element create Natural -x x2 -y y1 \
    -color green4 \
    -fill green \
    -pixels 5 \
    -symbol triangle

.graph element create Quadratic -x x2 -y y2 \
    -color blue4 \
    -fill blue2 \
    -pixels 5 \
    -symbol arrow

blt::table . .graph -fill both

Blt_ZoomStack .graph
Blt_Crosshairs .graph
Blt_ActiveLegend .graph
Blt_ClosestPoint .graph
Blt_PrintKey .graph

