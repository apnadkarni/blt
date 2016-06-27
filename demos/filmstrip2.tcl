package require BLT

. configure -bg green

blt::filmstrip .fs -width 550 -animate yes -scrolldelay 30 -height 400 -relwidth 1.0

blt::tk::frame .fs.page1 -width 500
blt::tk::frame .fs.page2 
blt::tk::frame .fs.page3 

.fs add page1 -window .fs.page1 -fill both 
.fs add page2 -window .fs.page2 -fill both
.fs add page3 -window .fs.page3 -fill both

# Page 1

scale .fs.page1.scale
entry .fs.page1.entry
blt::tk::button .fs.page1.right -text "Page 2 >>" -command [list .fs see page2]

blt::table .fs.page1 \
    0,0 .fs.page1.scale -fill both -cspan 2 \
    1,0 .fs.page1.entry -fill both -cspan 2 \
    2,1 .fs.page1.right -anchor se

blt::table configure .fs.page1 r* -resize none
blt::table configure .fs.page1 r0 -resize both


# Page 2 

text .fs.page2.text -relief sunken -bg white
blt::tk::button .fs.page2.left -text "<< Page 1"  -command [list .fs see page1]
blt::tk::button .fs.page2.right -text "Page 3 >>" -command [list .fs see page3]

blt::table .fs.page2 \
    0,0 .fs.page2.text -fill both -padx 20 -pady 20 -cspan 2 \
    1,0 .fs.page2.left -anchor w \
    1,1 .fs.page2.right -anchor e

# Page 3

blt::graph .fs.page3.graph -width 500 -height 500
blt::tk::button .fs.page3.left -text "<< Page 2"  -command [list .fs see page2]

blt::table .fs.page3 \
    0,0 .fs.page3.graph -fill both -cspan 2 \
    1,0 .fs.page3.left -anchor w 

blt::table . \
    0,0 .fs		-fill both 

    


