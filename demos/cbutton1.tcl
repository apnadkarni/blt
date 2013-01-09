
package require BLT

foreach {key file} {
save_as /usr/share/gtk-doc/html/pygtk/icons/stock_save_as_24.png
new_tab /usr/share/gtk-doc/html/pygtk/icons/stock_new_24.png
new_window /usr/share/gtk-doc/html/pygtk/icons/stock_network_24.png
open_file /usr/share/gtk-doc/html/pygtk/icons/stock_open_24.png
quit /usr/share/gtk-doc/html/pygtk/icons/stock_exit_24.png
print /usr/share/gtk-doc/html/pygtk/icons/stock_print_24.png
print_preview /usr/share/gtk-doc/html/pygtk/icons/stock_print_preview_24.png
undo /usr/share/gtk-doc/html/pygtk/icons/stock_undo_24.png
redo /usr/share/gtk-doc/html/pygtk/icons/stock_redo_24.png
cut /usr/share/gtk-doc/html/pygtk/icons/stock_cut_24.png
paste /usr/share/gtk-doc/html/pygtk/icons/stock_paste_24.png
copy /usr/share/gtk-doc/html/pygtk/icons/stock_copy_24.png
delete /usr/share/gtk-doc/html/pygtk/icons/stock_trash_24.png
select_all /usr/share/gtk-doc/html/pygtk/icons/stock_broken_image_24.png
find /usr/share/gtk-doc/html/pygtk/icons/stock_search_24.png
preferences /usr/share/gtk-doc/html/pygtk/icons/stock_preferences_24.png
stop /usr/share/gtk-doc/html/pygtk/icons/stock_stop_24.png
reload /usr/share/gtk-doc/html/pygtk/icons/stock_refresh_24.png
back /usr/share/gtk-doc/html/pygtk/icons/stock_left_arrow_24.png
forward /usr/share/gtk-doc/html/pygtk/icons/stock_right_arrow_24.png
home /usr/share/gtk-doc/html/pygtk/icons/stock_home_24.png
help /usr/share/gtk-doc/html/pygtk/icons/stock_help_24.png
about /usr/share/gtk-doc/html/pygtk/icons/stock_about_24.png
download /usr/share/icons/gnome/24x24/emblems/emblem-downloads.png
bookmark /usr/share/icons/Gant.Xfce/24x24/stock/stock_bookmark.png
} {
    set icon($key) [image create picture -file $file]
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
set bg [blt::background create gradient -high  grey70 -low grey95 \
	-jitter 10 -scale log -relativeto self]

set image ""

blt::tk::frame .mbar -bg $bg

set t "Hello, World"
blt::combobutton .mbar.file \
    -text "File" \
    -underline 0 \
    -image $image \
    -relief flat \
    -activerelief raised \
    -arrowon off \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -menuanchor sw \
    -menu .mbar.file.m



blt::combomenu .mbar.file.m \
    -width { 0 400 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.file.m add -text "New Window" -accelerator "Ctrl+N" -underline 0 \
    -icon $icon(new_window)
.mbar.file.m add -text "New Tab" -accelerator "Ctrl+T" -underline 4 \
    -icon $icon(new_tab)
.mbar.file.m add -text "Open Location..." -accelerator "Ctrl+L" -underline 5
.mbar.file.m add -text "Open File..." -accelerator "Ctrl+O" -underline 0 \
    -icon $icon(open_file)
.mbar.file.m add -text "Close Window" -accelerator "Ctrl+Shift+W" -underline 9
.mbar.file.m add -text "Close Tab" -accelerator "Ctrl+W" -underline 0
.mbar.file.m add -type separator
.mbar.file.m add -text "Save Page As..." -accelerator "Ctrl+O" -underline 10 \
    -icon $icon(save_as)
.mbar.file.m add -text "Save Page As PDF..." -accelerator "Ctrl+Shift+W" -underline 15
.mbar.file.m add -text "Send Link..." -accelerator "Ctrl+W" -underline 1
.mbar.file.m add -type separator
.mbar.file.m add -text "Page Setup..." -underline 8
.mbar.file.m add -text "Print Preview" -accelerator "Ctrl+Shift+W" -underline 9 \
    -icon $icon(print_preview)
.mbar.file.m add -text "Print..." -accelerator "Ctrl+P" -underline 0 \
    -icon $icon(print)
.mbar.file.m add -type separator
.mbar.file.m add -text "Import..." -underline 0
.mbar.file.m add -type separator
.mbar.file.m add -text "Work Offline" -underline 0
.mbar.file.m add -text "Quit" -accelerator "Ctrl+Q" -underline 0 \
    -icon $icon(quit) 

blt::combobutton .mbar.edit \
    -text "Edit" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menuanchor nw \
    -menu .mbar.edit.m

blt::combomenu .mbar.edit.m \
    -width { 0 400 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.edit.m add -text "Undo" -accelerator "Ctrl+Z"  \
    -icon $icon(undo)
.mbar.edit.m add -text "Redo" -accelerator "Ctrl+Shift+Z"  \
    -icon $icon(redo)
.mbar.edit.m add -type separator
.mbar.edit.m add -text "Cut" -accelerator "Ctrl+X" \
    -icon $icon(cut)
.mbar.edit.m add -text "Copy" -accelerator "Ctrl+C" \
    -icon $icon(copy)
.mbar.edit.m add -text "Paste" -accelerator "Ctrl+V" \
    -icon $icon(paste)
.mbar.edit.m add -text "Delete" -accelerator "Del" \
    -icon $icon(delete)
.mbar.edit.m add -type separator
.mbar.edit.m add -text "Select All" -accelerator "Ctrl+X" \
    -icon $icon(select_all)
.mbar.edit.m add -type separator
.mbar.edit.m add -text "Find" -accelerator "Ctrl+F"  \
    -icon $icon(find)
.mbar.edit.m add -text "Find Again" -accelerator "Ctrl+G"
.mbar.edit.m add -type separator
.mbar.edit.m add -text "Preferences" \
    -icon $icon(preferences)

blt::combomenu .mbar.edit.m.m
.mbar.edit.m.m add -type command -text "five" -accelerator "^A" -command "set t five"
.mbar.edit.m.m add -type command -text "six" -accelerator "^B" -command "set t six"
.mbar.edit.m.m add -type command -text "seven" -accelerator "^C" -command "set t seven"
.mbar.edit.m.m add -type command -text "eight" -accelerator "^D" -command "set t eight"
.mbar.edit.m.m add -type cascade -text "cascade" -accelerator "^E" 


blt::combobutton .mbar.view \
    -text "View" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menuanchor nw \
    -menu .mbar.view.m

blt::combomenu .mbar.view.m \
    -width { 0 600 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.view.m add -type cascade -text "Toolbars" -underline 0 
.mbar.view.m add -type checkbutton -text "Status Bar" \
    -underline 4 -variable statusbar
.mbar.view.m add -type checkbutton -text "Sidebar" \
    -underline 5 -variable sidebar 
.mbar.view.m add -type checkbutton -text "Adblock Plus: Blockable items" \
    -accelerator "Ctrl+Shift+V" -underline 0 -variable adblock 
.mbar.view.m add -type separator
.mbar.view.m add -text "Stop" -accelerator "Esc" -underline 9 \
    -icon $icon(stop)
.mbar.view.m add -text "Reload" -accelerator "Ctrl+R" -underline 0 \
    -icon $icon(reload)
.mbar.view.m add -type separator
.mbar.view.m add -type cascade -text "Zoom" -accelerator "Ctrl+O" -underline 10 
.mbar.view.m add -type cascade -text "Page Style" -accelerator "Ctrl+Shift+W" \
    -underline 15
.mbar.view.m add -type cascade -text "Character Encoding" -accelerator "Ctrl+W" \
    -underline 1
.mbar.view.m add -type separator
.mbar.view.m add -text "Page Source" -underline 8 -accelerator "Ctrl+U"
.mbar.view.m add -text "Full Screen" -accelerator "F11" -underline 9 


blt::combobutton .mbar.history \
    -text "History" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menuanchor nw \
    -menu .mbar.history.m

blt::combomenu .mbar.history.m \
    -width { 0 600 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.history.m add -text "Back" -accelerator "Alt+Left Arrow" \
    -underline 0 -icon $icon(back)
.mbar.history.m add -text "Forward" -accelerator "Alt+Right Arrow" \
    -underline 4 -icon $icon(forward)
.mbar.history.m add -text "Home" -accelerator "Alt+Home" \
    -underline 5 -icon $icon(home)
.mbar.history.m add -text "Show All History" -accelerator "Ctrl+Shift+H" \
    -underline 0 
.mbar.history.m add -type separator
.mbar.history.m add -type cascade -text "Recently Closed Tabs" \
    -accelerator "Ctrl+O" -underline 10 

blt::combobutton .mbar.bmarks \
    -text "Bookmarks" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menuanchor nw \
    -menu .mbar.bmarks.m

blt::combomenu .mbar.bmarks.m \
    -width { 0 600 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.bmarks.m add -text "Bookmark This Page" -accelerator "Ctrl+D" \
    -underline 0 -icon $icon(bookmark)
.mbar.bmarks.m add -text "Subscribe to This Page..." \
    -underline 4 -icon $icon(forward)
.mbar.bmarks.m add -text "Bookmark All Tabs"  \
    -underline 5 -icon $icon(home)
.mbar.bmarks.m add -text "Organize Bookmarks" \
    -underline 0 
.mbar.bmarks.m add -type separator
.mbar.bmarks.m add -type cascade -text "Bookmarks Toolbar" \
    -underline 10 
.mbar.bmarks.m add -type separator
.mbar.bmarks.m add -type cascade -text "Recently Bookmarked" \
    -underline 10 
.mbar.bmarks.m add -type cascade -text "Recent Tags" \
    -underline 10 
.mbar.bmarks.m add -type separator
.mbar.bmarks.m add -text "Page 1" \
    -underline 10 
.mbar.bmarks.m add -text "Page 2" \
    -underline 10 

blt::combobutton .mbar.tools \
    -text "Tools" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menuanchor nw \
    -menu .mbar.tools.m

blt::combomenu .mbar.tools.m \
    -width { 0 600 }  -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.tools.m add -text "Web Search" -accelerator "Ctrl+K" \
    -underline 0 
.mbar.tools.m add -type separator
.mbar.tools.m add -text "Downloads" -accelerator "Ctrl+Y" \
    -underline 4 -icon $icon(download)
.mbar.tools.m add -text "Add-ons" -underline 0 
.mbar.tools.m add -type separator
.mbar.tools.m add -text "PDF Download - Options" -underline 0 
.mbar.tools.m add -text "Save Images From Tabs" -underline 10 
.mbar.tools.m add -text "Error Console" \
    -accelerator "Ctrl+Shift+J" -underline 10 
.mbar.tools.m add -text "Adblock Plus Preferences..." \
    -accelerator "Ctrl+Shift+E" -underline 10 
.mbar.tools.m add -text "Page Info" \
    -accelerator "Ctrl+I" -underline 10 
.mbar.tools.m add -type separator
.mbar.tools.m add -text "Clear Private Data" \
    -accelerator "Ctrl+Shift+Del" -underline 10 
.mbar.tools.m add -text "Batch Download Settings" \
    -underline 10 

blt::combobutton .mbar.help \
    -text "Help" \
    -relief flat \
    -activerelief raised \
    -bg $bg \
    -font { Arial 9 } -justify left \
    -underline 0 \
    -arrowon no \
    -menuanchor nw \
    -menu .mbar.help.m

blt::combomenu .mbar.help.m \
    -width { 0 600 } -font "Arial 9" -acceleratorfont "Arial 9" \
    -bg grey85 -relief raised -bd 1
.mbar.help.m add -text "Help Contents" \
    -underline 0 -icon $icon(help)
.mbar.help.m add -text "Release Notes" -underline 0
.mbar.help.m add -text "Report Broken Website..." -underline 5
.mbar.help.m add -text "Report Web Forgery..." -underline 0 
.mbar.help.m add -type separator
.mbar.help.m add -text "Check For Updates..." -underline 0 
.mbar.help.m add -text "About..." -underline 0  -icon $icon(about)

canvas .c
blt::table .mbar \
    1,0 .mbar.file -fill both \
    1,1 .mbar.edit -fill both \
    1,2 .mbar.view -fill both \
    1,3 .mbar.history -fill both \
    1,4 .mbar.bmarks -fill both \
    1,5 .mbar.tools -fill both \
    1,6 .mbar.help -fill both \

blt::table configure .mbar c* -padx 2 -resize none
blt::table configure .mbar c7 -resize expand

blt::table . \
    0,0 .mbar -fill x \
    1,0 .c -fill both 

blt::table configure . r0 -resize none
blt::table configure . r1 -resize expand
