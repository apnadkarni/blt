#!../src/bltwish

#package require BLT
source scripts/demo.tcl
option add *Scrollbar.relief	flat
set oldLabel "dummy"

proc RunDemo { program } {
    if { ![file executable $program] } {
	return
    }
    set cmd [list $program -name "demo:$program" -geom -4000-4000]
    global programInfo
    if { [info exists programInfo(lastProgram)] } {
	set programInfo($programInfo(lastProgram)) 0
    }
    eval bgexec programInfo($program) $cmd &
    set programInfo(lastProgram) $program
    puts stderr [.top.tab.f1 search -name demo:$program]
    .top.tab.f1 configure -name demo:$program
}

frame .top
set tree [blt::tree create]
blt::treeview .top.hier -separator "." -xscrollincrement 1 \
    -yscrollcommand { .top.yscroll set } -xscrollcommand { .top.xscroll set } \
    -separator . \
    -tree $tree \
    -selectcommand { 
	set index [.top.hier curselection]
	if { $index != "" } {
	    set label [.top.hier entry cget $index -label]
	    .top.title configure -text $label
	    .top.tab tab configure Example -window .top.tab.f1 
	    if { $label != $oldLabel }  {
		RunDemo $label
	    }
	}
    }
	

blt::tk::scrollbar .top.yscroll -command { .top.hier yview }
blt::tk::scrollbar .top.xscroll -command { .top.hier xview } -orient horizontal
blt::tk::label .top.mesg -relief groove -borderwidth 2 
blt::tk::label .top.title -text "Synopsis" -highlightthickness 0
blt::tabset .top.tab -side bottom -outerrelief flat -outerborderwidth 0 \
    -highlightthickness 0 -pageheight 4i

foreach tab { "Example" "See Code" "Manual" } {
    .top.tab insert end $tab
}
 
set pics /DOS/f/gah/Pics
set pics /home/gah/Pics
image create picture dummy -file images/blt98.gif
image create picture graph.img -width 50 -height 50
graph.img resample dummy

dummy configure -file images/blt98.gif
image create picture barchart.img -width 50 -height 50
barchart.img resample dummy

.top.hier entry configure root -label "BLT"
.top.hier insert end \
    "Plotting" \
    "Plotting.graph" \
    "Plotting.graph.graph" \
    "Plotting.graph.graph2" \
    "Plotting.graph.graph3" \
    "Plotting.graph.graph4" \
    "Plotting.graph.graph5" \
    "Plotting.graph.graph6" \
    "Plotting.barchart" \
    "Plotting.barchart.barchart1" \
    "Plotting.barchart.barchart2" \
    "Plotting.barchart.barchart3" \
    "Plotting.barchart.barchart4" \
    "Plotting.barchart.barchart5" \
    "Plotting.stripchart" \
    "Plotting.vector" \
    "Composition" \
    "Composition.htext" \
    "Composition.table" \
    "Composition.tabset" \
    "Composition.hierbox" \
    "Miscellaneous" \
    "Miscellaneous.busy" \
    "Miscellaneous.bgexec" \
    "Miscellaneous.watch" \
    "Miscellaneous.bltdebug" 
.top.hier open -r root
.top.hier entry configure root -font *-helvetica*-bold-r-*-18-* 
puts stderr [$tree dump root]
foreach item { "Plotting" "Composition" "Miscellaneous" } {
    set index [.top.hier index ".$item"]
    .top.hier entry configure $index -font *-helvetica*-bold-r-*-14-* 
}
.top.hier entry configure [.top.hier index ".Plotting.graph"] \
    -font *-helvetica*-bold-r-*-14-* -label "X-Y Graph"
.top.hier entry configure [.top.hier index ".Plotting.barchart"] \
    -font *-helvetica*-bold-r-*-14-* -label "Bar Chart"

.top.hier entry configure [.top.hier index ".Plotting.stripchart"] \
    -font *-helvetica*-bold-r-*-14-* -label "X-Y Graph"
.top.hier entry configure [.top.hier index ".Plotting.stripchart"] \
    -font *-helvetica*-bold-r-*-14-* -label "Strip Chart"

.top.hier entry configure [.top.hier index ".Plotting.graph"] -icon graph.img
.top.hier entry configure [.top.hier index ".Plotting.barchart"] \
    -icon barchart.img

blt::table .top \
    0,0 .top.hier -fill both -rspan 2 \
    0,1 .top.yscroll -fill y -rspan 2 \
    0,2 .top.mesg -padx 2 -pady { 8 2 } -fill both \
    0,2 .top.title -anchor nw -padx { 8 8 }  \
    1,2 .top.tab -fill both -rspan 2 \
    2,0 .top.xscroll -fill x 

blt::table configure .top c1 r2 -resize none
blt::table configure .top c0 -width { 3i {} }
blt::table configure .top c2 -width { 4i {} }
blt::table . \
    .top -fill both

proc DoExit { code } {
    global progStatus
    set progStatus 1
    exit $code
}

blt::container .top.tab.f1 -relief raised -bd 2 -takefocus 0
.top.tab tab configure Example -window .top.tab.f1 

if  1 {
    set cmd "xterm -fn fixed -geom +4000+4000"
    eval blt::bgexec programInfo(xterm) $cmd &
    set programInfo(lastProgram) xterm
    .top.tab.f1 configure -command $cmd 
} 
wm protocol . WM_DELETE_WINDOW { destroy . }
