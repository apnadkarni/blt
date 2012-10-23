
package require BLT
source scripts/demo.tcl

blt::scrollset .ss \
    -xscrollbar .ss.xsbar \
    -yscrollbar .ss.ysbar \
    -window .ss.t 

blt::tk::scrollbar .ss.ysbar -orient vertical 
blt::tk::scrollbar .ss.xsbar -orient horizontal 
text .ss.t -wrap none

.ss.t insert end "salsadkjlda s
adslkjda lskjd
asldkjda lskjd sa
aslkj dlsakj lkdsa
asdlkjdalskj ds
aslkdj aldskjd ls
asldkj dlskjd sl
asldkj dlskjd l
adlaldksjd ldkasj ldkjs ld"

pack .ss -fill both -expand yes

