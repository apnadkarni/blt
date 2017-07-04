
package require BLT

set table [blt::datatable create]
$table restore -file ./data/graph4a.tab

#option add *Row.titleJustify center
set view .ss.t

blt::scrollset .ss \
    -window $view \
    -xscrollbar .ss.x \
    -yscrollbar .ss.y 
blt::tk::scrollbar .ss.x
blt::tk::scrollbar .ss.y

blt::tableview $view \
    -table $table \
    -titles both \
    -selectmode multiple \
    -columnfilters yes 
blt::table . \
    0,0 .ss -fill both

$view style create textbox textbox \
    -editor $view.editor -edit yes
blt::comboeditor $view.editor  \
    -height { 0 2i }  \
    -yscrollbar $view.editor.ybar \
    -xscrollbar $view.editor.xbar
blt::tk::scrollbar $view.editor.xbar 
blt::tk::scrollbar $view.editor.ybar

$view column configure x -style textbox 
