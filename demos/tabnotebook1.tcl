#!../src/bltwish

package require BLT
source scripts/demo.tcl

# Create a tabset widget.  

blt::tabset .ts -bg blue -outerpad 0 \
    -highlightthickness 0 -bd 0 -gap 2 -justify left -tabwidth same

# The tabset is initially empty.  Insert tabs (pages) into the tabset.  

foreach label { First Second Third Fourth } {
    .ts insert end -text $label
}

# Tabs are referred to by their index.  Tab indices can be one of the 
# following:
#
#	 number		Position of tab the tabset's list of tabs.
# 	 @x,y		Tab closest to the specified X-Y screen coordinates.
# 	 "active"	Tab currently under the mouse pointer.
# 	 "focus"	Tab that has focus.  
# 	 "select"	The currently selected tab.
# 	 "right"	Next tab from "focus".
# 	 "left"		Previous tab from "focus".
# 	 "up"		Next tab from "focus".
# 	 "down"		Previous tab from "focus".
# 	 "end"		Last tab in list.
#	 string		Tab identifier.  The "insert" operation returns 
#			a unique identifier for the new tab (e.g. "tab0").  
#			This ID is valid for the life of the tab, even if
#			the tabs are moved or reordered.  

# Each tab has a text label and an optional Tk image.

set image [image create picture -file ./images/mini-book1.gif]
.ts tab configure 0 -image $image

#
# How to embed a widget into a page.  
#

# 1. The widget must be a child of the tabset.

set image [image create picture -file ./images/blt98.gif]
label .ts.label -image $image -relief sunken -bd 2

# 2. Use the -window option to embed the widget.

#.ts tab configure 0 -window .ts.label

# The tearoff perforation, displayed on the selected tab, is
# controlled by the tabset's -tearoff option.  
#
# If you don't want tearoff pages, configure -tearoff to "no".

.ts configure -tearoff yes

blt::table . \
    0,0 .ts -fill both 

focus .ts

