
package require BLT

blt::tk::pushbutton .p -text "Push" 
pack .p -side top

set onFile ~/r7/rappture/gui/scripts/images/zoom-in.png
set offFile ~/r7/rappture/gui/scripts/images/zoom-out.png
set on  [image create picture -file $onFile]
set off [image create picture -file $offFile]
blt::tk::pushbutton .p1 -variable "test" -onvalue 1 \
    -onimage $on -offimage $off
blt::tk::pushbutton .p2 -text "Push 2" -variable "test" -onvalue 2
blt::tk::pushbutton .p3 -text "Push 3" -variable "test" -onvalue 3 
blt::tk::pushbutton .p4 -text "Push 4" -variable "test" -onvalue 4 
blt::tk::pushbutton .p5 -text "Push 5" -variable "test" -onvalue 5
pack .p1 .p2 .p3 .p4 .p5 -side top
blt::tk::button .b -text "Query" -command {
   puts stderr variable=[set [.p cget -variable]]
   puts stderr test=$test
}
pack .b
puts stderr class=[winfo class .p1]
