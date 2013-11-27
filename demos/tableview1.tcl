
package require BLT

set table [blt::datatable create]
$table restore -file graph4a.tab

#option add *Row.titleJustify center
set view .ss.t

blt::scrollset .ss \
    -window $view \
    -xscrollbar .ss.x \
    -yscrollbar .ss.y 
blt::tk::scrollbar .ss.x
blt::tk::scrollbar .ss.y


blt::tableview $view -table $table -titles both -selectmode multiple \
    -autofilters yes 
blt::table . \
    0,0 .ss -fill both
$view filter configure -menu $view.filter
blt::combomenu $view.filter  \
    -restrictwidth min \
    -height { 0 2i }  \
    -yscrollbar $view.filter.ybar \
    -xscrollbar $view.filter.xbar
blt::tk::scrollbar $view.filter.xbar 
blt::tk::scrollbar $view.filter.ybar

