
package require BLT

set text \
"Four score and seven years ago our fathers brought forth on this
continent, a new nation, conceived in Liberty, and dedicated to the
proposition that all men are created equal.

Now we are engaged in a great civil war, testing whether that nation, or
any nation so conceived and so dedicated, can long endure. We are met on a
great battle-field of that war. We have come to dedicate a portion of that
field, as a final resting place for those who here gave their lives that
that nation might live. It is altogether fitting and proper that we should
do this. 
"


blt::comboeditor .e -text $text -exportselection yes \
    -xscrollbar .e.xs \
    -yscrollbar .e.ys \
    -height 1i \
    -width 2i 

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
