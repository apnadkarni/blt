
package require BLT

set text \
"Four score and seven years ago
our fathers brought forth on this
continent, a new nation, conceived
in Liberty, and dedicated to the
proposition that all men are
created equal."


blt::comboeditor .e -text $text -exportselection yes \
    -xscrollbar .e.xs \
    -yscrollbar .e.ys \
    -height 1i

blt::tk::scrollbar .e.xs
blt::tk::scrollbar .e.ys
text .t
pack .t
.e select range 1 3
.e insert 4 "\nextra\ncharacters\n"
focus .e
after 500 {
    set rootx [winfo rootx .]
    set rooty [winfo rooty .]
    .e post -popup [list $rootx $rooty]
    update
    grab .e 
    focus -force .e
}
