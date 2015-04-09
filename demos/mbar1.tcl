
foreach {key file} {
save_as /usr/share/gtk-doc/html/pygtk/icons/stock_save_as
new_tab /usr/share/gtk-doc/html/pygtk/icons/stock_new
new_window /usr/share/gtk-doc/html/pygtk/icons/stock_network
open_file /usr/share/gtk-doc/html/pygtk/icons/stock_open
quit /usr/share/gtk-doc/html/pygtk/icons/stock_exit
print /usr/share/gtk-doc/html/pygtk/icons/stock_print
print_preview /usr/share/gtk-doc/html/pygtk/icons/stock_print_preview
undo /usr/share/gtk-doc/html/pygtk/icons/stock_undo
redo /usr/share/gtk-doc/html/pygtk/icons/stock_redo
cut /usr/share/gtk-doc/html/pygtk/icons/stock_cut
paste /usr/share/gtk-doc/html/pygtk/icons/stock_paste
copy /usr/share/gtk-doc/html/pygtk/icons/stock_copy
delete /usr/share/gtk-doc/html/pygtk/icons/stock_trash
select_all /usr/share/gtk-doc/html/pygtk/icons/stock_broken_image
find /usr/share/gtk-doc/html/pygtk/icons/stock_search
preferences /usr/share/gtk-doc/html/pygtk/icons/stock_preferences
stop /usr/share/gtk-doc/html/pygtk/icons/stock_stop
reload /usr/share/gtk-doc/html/pygtk/icons/stock_refresh
back /usr/share/gtk-doc/html/pygtk/icons/stock_left_arrow
forward /usr/share/gtk-doc/html/pygtk/icons/stock_right_arrow
home /usr/share/gtk-doc/html/pygtk/icons/stock_home
} {
    set icon($key) [image create picture -file ${file}_24.png]
}

if { [file exists ../library] } {
    set blt_library ../library
}

set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

#set image [image create picture -file ~/images.jpeg]
set bg [blt::background create linear -highcolor  grey70 -lowcolor grey95 \
	-jitter 10 -colorscale log -relativeto self]

set image ""

blt::menubar .mbar \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 10 bold } -justify left \

.mbar add \
    -text "File" \
    -underline 0 \
    -image $image \
    -menuanchor sw \
    -menu .mbar.file

blt::combomenu .mbar.file \
    -width -400  -font "Arial 10 bold" -acceleratorfont "Arial 10 bold" \
    -bg grey85 -relief raised -bd 1
.mbar.file.m add -text "New Window" -accelerator "Ctrl+N" -underline 0 \
    -icon $icon(new_window)
.mbar.file add -text "New Tab" -accelerator "Ctrl+T" -underline 4 \
    -icon $icon(new_tab)
.mbar.file add -text "Open Location..." -accelerator "Ctrl+L" -underline 5
.mbar.file add -text "Open File..." -accelerator "Ctrl+O" -underline 0 \
    -icon $icon(open_file)
.mbar.file add -text "Close Window" -accelerator "Ctrl+Shift+W" -underline 9
.mbar.file add -text "Close Tab" -accelerator "Ctrl+W" -underline 0
.mbar.file add -type separator
.mbar.file add -text "Save Page As..." -accelerator "Ctrl+O" -underline 10 \
    -icon $icon(save_as)
.mbar.file add -text "Save Page As PDF..." -accelerator "Ctrl+Shift+W" -underline 15
.mbar.file add -text "Send Link..." -accelerator "Ctrl+W" -underline 1
.mbar.file add -type separator
.mbar.file add -text "Page Setup..." -underline 8
.mbar.file add -text "Print Preview" -accelerator "Ctrl+Shift+W" -underline 9 \
    -icon $icon(print_preview)
.mbar.file add -text "Print..." -accelerator "Ctrl+P" -underline 0 \
    -icon $icon(print)
.mbar.file add -type separator
.mbar.file add -text "Import..." -underline 0
.mbar.file add -type separator
.mbar.file add -text "Work Offline" -underline 0
.mbar.file add -text "Quit" -accelerator "Ctrl+Q" -underline 0 \
    -icon $icon(quit) 

.mbar add \
    -text "Edit" \
    -underline 0 \
    -menuanchor nw \
    -menu .mbar.edit

blt::combomenu .mbar.edit \
    -width -400  -font "Arial 10 bold" -acceleratorfont "Arial 10 bold" \
    -bg grey85 -relief raised -bd 1
.mbar.edit add -text "Undo" -accelerator "Ctrl+Z"  \
    -icon $icon(undo)
.mbar.edit add -text "Redo" -accelerator "Ctrl+Shift+Z"  \
    -icon $icon(redo)
.mbar.edit add -type separator
.mbar.edit add -text "Cut" -accelerator "Ctrl+X" \
    -icon $icon(cut)
.mbar.edit add -text "Copy" -accelerator "Ctrl+C" \
    -icon $icon(copy)
.mbar.edit add -text "Paste" -accelerator "Ctrl+V" \
    -icon $icon(paste)
.mbar.edit add -text "Delete" -accelerator "Del" \
    -icon $icon(delete)
.mbar.edit add -type separator
.mbar.edit add -text "Select All" -accelerator "Ctrl+X" \
    -icon $icon(select_all)
.mbar.edit add -type separator
.mbar.edit add -text "Find" -accelerator "Ctrl+F"  \
    -icon $icon(find)
.mbar.edit add -text "Find Again" -accelerator "Ctrl+G"
.mbar.edit add -type separator
.mbar.edit add -text "Preferences" \
    -icon $icon(preferences)

blt::combomenu .mbar.edit.m
.mbar.edit.m add -type command -text "five" -accelerator "^A" -command "set t five"
.mbar.edit.m add -type command -text "six" -accelerator "^B" -command "set t six"
.mbar.edit.m add -type command -text "seven" -accelerator "^C" -command "set t seven"
.mbar.edit.m add -type command -text "eight" -accelerator "^D" -command "set t eight"
.mbar.edit.m add -type cascade -text "cascade" -accelerator "^E" 


.mbar add \
    -text "View" \
    -underline 0 \
    -menuanchor nw \
    -menu .mbar.view

blt::combomenu .mbar.view \
    -width -600  -font "Arial 10 bold" -acceleratorfont "Arial 10 bold" \
    -bg grey85 -relief raised -bd 1
.mbar.view add -type cascade -text "Toolbars" -underline 0 
.mbar.view add -type checkbutton -text "Status Bar" \
    -underline 4 
.mbar.view add -type checkbutton -text "Sidebar" \
    -underline 5 -variable sidebar 
.mbar.view add -type checkbutton -text "Adblock Plus: Blockable items" \
    -accelerator "Ctrl+Shift+V" -underline 0 -variable adblock 
.mbar.view add -type separator
.mbar.view add -text "Stop" -accelerator "Esc" -underline 9 \
    -icon $icon(stop)
.mbar.view add -text "Reload" -accelerator "Ctrl+R" -underline 0 \
    -icon $icon(reload)
.mbar.view add -type separator
.mbar.view add -type cascade -text "Zoom" -accelerator "Ctrl+O" -underline 10 
.mbar.view add -type cascade -text "Page Style" -accelerator "Ctrl+Shift+W" \
    -underline 15
.mbar.view add -type cascade -text "Character Encoding" -accelerator "Ctrl+W" \
    -underline 1
.mbar.view add -type separator
.mbar.view add -text "Page Source" -underline 8 -accelerator "Ctrl+U"
.mbar.view add -text "Full Screen" -accelerator "F11" -underline 9 


.mbar add \
    -text "History" \
    -underline 0 \
    -menuanchor nw \
    -menu .mbar.history

blt::combomenu .mbar.history \
    -width -600  -font "Arial 10 bold" -acceleratorfont "Arial 10 bold" \
    -bg grey85 -relief raised -bd 1
.mbar.history add -text "Back" -accelerator "Alt+Left Arrow" \
    -underline 0 -icon $icon(back)
.mbar.history add -text "Forward" -accelerator "Alt+Right Arrow" \
    -underline 4 -icon $icon(forward)
.mbar.history add -text "Home" -accelerator "Alt+Home" \
    -underline 5 -icon $icon(home)
.mbar.history add -text "Show All History" -accelerator "Ctrl+Shift+H" \
    -underline 0 
.mbar.history add -type separator
.mbar.history add -type cascade -text "Recently Closed Tabs" \
    -accelerator "Ctrl+O" -underline 10 

.mbar add \
    -text "Bookmarks" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 10 bold } -justify left \
    -underline 0 \
    -menuanchor nw \
    -menu .mbar.bmarks

blt::combomenu .mbar.bmarks \
    -width -600  -font "Arial 10 bold" -acceleratorfont "Arial 10 bold" \
    -bg grey85 -relief raised -bd 1
.mbar.bmarks add -text "Bookmark This Page" -accelerator "Ctrl+D" \
    -underline 0 -icon $icon(back)
.mbar.bmarks add -text "Subscribe to This Page..." \
    -underline 4 -icon $icon(forward)
.mbar.bmarks add -text "Bookmark All Tabs"  \
    -underline 5 -icon $icon(home)
.mbar.bmarks add -text "Organize Bookmarks" \
    -underline 0 
.mbar.bmarks add -type separator
.mbar.bmarks add -type cascade -text "Bookmarks Toolbar" \
    -underline 10 
.mbar.bmarks add -type separator
.mbar.bmarks add -type cascade -text "Recently Bookmarked" \
    -underline 10 
.mbar.bmarks add -type cascade -text "Recent Tags" \
    -underline 10 
.mbar.bmarks add -type separator
.mbar.bmarks add -text "Page 1" \
    -underline 10 
.mbar.bmarks add -text "Page 2" \
    -underline 10 

.mbar add  \
    -text "Tools" \
    -underline 0 \
    -menuanchor nw \
    -menu .mbar.tools

blt::combomenu .mbar.tools \
    -width -600  -font "Arial 10 bold" -acceleratorfont "Arial 10 bold" \
    -bg grey85 -relief raised -bd 1
.mbar.tools add -text "Web Search" -accelerator "Ctrl+K" \
    -underline 0 
.mbar.tools add -type separator
.mbar.tools add -text "Downloads" -accelerator "Ctrl+Y" \
    -underline 4 -icon $icon(forward)
.mbar.tools add -text "Add-ons" -underline 0 
.mbar.tools add -type separator
.mbar.tools add -text "PDF Download - Options" -underline 0 
.mbar.tools add -text "Save Images From Tabs" -underline 10 
.mbar.tools add -text "Error Console" \
    -accelerator "Ctrl+Shift+J" -underline 10 
.mbar.tools add -text "Adblock Plus Preferences..." \
    -accelerator "Ctrl+Shift+E" -underline 10 
.mbar.tools add -text "Page Info" \
    -accelerator "Ctrl+I" -underline 10 
.mbar.tools add -type separator
.mbar.tools add -text "Clear Private Data" \
    -accelerator "Ctrl+Shift+Del" -underline 10 
.mbar.tools add -text "Batch Download Settings" \
    -underline 10 

.mbar add \
    -text "Help" \
    -underline 0 \
    -menuanchor nw \
    -menu .mbar.help

blt::combomenu .mbar.help \
    -width -600  -font "Arial 10 bold" -acceleratorfont "Arial 10 bold" \
    -bg grey85 -relief raised -bd 1
.mbar.help add -text "Help Contents" \
    -underline 0 -icon $icon(back)
.mbar.help add -text "Release Notes" \
    -underline 4 -icon $icon(forward)
.mbar.help add -text "Report Broken Website..." \
    -underline 5 -icon $icon(home)
.mbar.help add -text "Report Web Forgery..." \
    -underline 0 
.mbar.help add -type separator
.mbar.help add -text "Check For Updates..." \
    -underline 0 
.mbar.help add -text "About..." \
    -underline 0 

canvas .c

blt::table . \
    0,0 .mbar -fill x \
    1,0 .c -fill both 

blt::table configure . r0 -resize none
blt::table configure . r1 -resize expand
