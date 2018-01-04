
package require BLT

blt::scrollset .ss \
    -xscrollbar .ss.xsbar \
    -yscrollbar .ss.ysbar \
    -window .ss.t -width 300

blt::tk::scrollbar .ss.ysbar -orient vertical 
blt::tk::scrollbar .ss.xsbar -orient horizontal 
text .ss.t -wrap none -font "Arial 16" -width 40 -height 12

.ss.t insert end \
"Lorem ipsum dolor sit amet, consectetur 
adipiscing elit, sed do eiusmod tempor 
incididunt ut labore et dolore magna 
aliqua. Ut enim ad minim veniam, quis 
nostrud exercitation ullamco laboris 
nisi ut aliquip ex ea commodo consequat. 
Duis aute irure dolor in reprehenderit 
in voluptate velit esse cillum dolore 
eu fugiat nulla pariatur. Excepteur sint 
occaecat cupidatat non proident, sunt in 
culpa qui officia deserunt mollit anim 
id est laborum."

pack .ss -fill both -expand yes

