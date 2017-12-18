
package require BLT

option add *Font "Arial 9"

if { [blt::winop xdpi] > 150 } {
    set offFile $blt_library/icons/32x32/folder.png 
    set onFile $blt_library/icons/32x32/folder-open.png
    set filterFile $blt_library/icons/32x32/filter.png
} else {
    set offFile $blt_library/icons/16x16/folder.png 
    set onFile $blt_library/icons/16x16/folder-open.png
    set filterFile $blt_library/icons/16x16/filter.png
}    

blt::tk::frame .f0
blt::tk::label .label0 -text "Text or Image"
blt::tk::pushbutton .f0.p1 -text "Filter" 
set img  [image create picture -file $filterFile]
blt::tk::pushbutton .f0.p2 -image $img
blt::table .f0 \
    0,0 .f0.p1 -padx 2 \
    0,1 .f0.p2 -padx 2 

blt::tk::label .label1 -text "On/Off Image"
set on  [image create picture -file $onFile]
set off [image create picture -file $offFile]
blt::tk::pushbutton .t1 -variable "test" -onvalue 1 \
    -onimage $on -offimage $off

set color brown
proc SetColor { w } {
    global color 
    .text tag configure color -foreground $color
}

blt::tk::frame .f1
blt::tk::label .label2 -text "As Radiobuttons"
blt::tk::pushbutton .f1.r1 \
    -text "Red" \
    -variable "color" \
    -command [list SetColor .f1.r1] \
    -onvalue red2 -offvalue black
blt::tk::pushbutton .f1.r2 \
    -text "Green" \
    -variable "color" \
    -command [list SetColor .f1.r2] \
    -onvalue green3 -offvalue black 
blt::tk::pushbutton .f1.r3 \
    -text "Blue" \
    -variable "color" \
    -command [list SetColor .f1.r3] \
    -onvalue blue -offvalue black
blt::tk::pushbutton .f1.r4 \
    -text "Brown" \
    -variable "color" \
    -command [list SetColor .f1.r4] \
    -onvalue brown -offvalue black
blt::table .f1 \
    0,0 .f1.r1 -padx 2 \
    0,1 .f1.r2 -padx 2 \
    0,2 .f1.r3 -padx 2 \
    0,3 .f1.r4 -padx 2 

proc SetFont {} {
    global italic bold large underline
    set string "Arial "
    if { $large } {
	append string "10 "
    } else {
	append string "8 "
    }
    if { $bold } {
	append string "bold "
    }
    if { $italic } {
	append string "italic "
    }
    .text tag configure special -font $string -underline $underline
}

blt::tk::label .label3 -text "As Checkbuttons"
set large 0
set bold 0
set underline 0
set italic 0
blt::tk::frame .f2
blt::tk::pushbutton .f2.c1 \
    -text "Italic" \
    -variable "italic" \
    -command [list SetFont] 
blt::tk::pushbutton .f2.c2 \
    -text "Bold" \
    -variable "bold" \
    -command [list SetFont] 
blt::tk::pushbutton .f2.c3 \
    -text "Underline" \
    -variable "underline" \
    -command [list SetFont] 
blt::tk::pushbutton .f2.c4 \
    -text "Large" \
    -variable "large" \
    -command [list SetFont] 

blt::table .f2 \
    0,0 .f2.c1 -padx 2 \
    0,1 .f2.c2 -padx 2 \
    0,2 .f2.c3 -padx 2 \
    0,3 .f2.c4 -padx 2 

text .text -height 3 -width 35
.text tag configure normal -font "Arial 8"
.text tag configure special -font "Arial 8" -underline 0
.text tag configure color  -font "Arial 8" -foreground brown
.text insert end "\n  The quick " normal "brown" color " fox " normal \
    "jumps" special " over the lazy dog." normal 

blt::tk::button .q -text "Quit" -command exit

blt::table . \
    0,0 .label0 -anchor w \
    0,1 .f0 -anchor w  \
    1,0 .label1 -anchor w \
    1,1 .t1 -anchor w -cspan 4 \
    2,0 .label2 -anchor w  \
    2,1 .f1  -anchor w \
    3,0 .label3 -anchor w  \
    3,1 .f2 -anchor w \
    4,0 .text -cspan 4 -padx 2 \
    5,1 .q -padx 5 -anchor e

blt::table configure . r* c* -resize none
blt::table configure . r4 c2 -resize both
blt::table configure . r* -pady { 3 0 }
blt::table configure . c0 -padx { 0 3 }

