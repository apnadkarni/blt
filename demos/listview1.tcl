
package require BLT

blt::tk::radiobutton .rowmode -text "Row Layout" \
    -variable layoutMode -value row \
    -command [list .ss.l configure -layoutmode row]

blt::tk::radiobutton .colmode -text "Column Layout" \
    -variable layoutMode -value column \
    -command [list .ss.l configure -layoutmode column]

blt::tk::radiobutton .iconsmode -text "Icons Layout" \
    -variable layoutMode -value icons \
    -command [list .ss.l configure -layoutmode icons]

blt::scrollset .ss \
    -xscrollbar .ss.xs \
    -yscrollbar .ss.ys \
    -window .ss.l

blt::listview .ss.l \
    -width 3i \
    -height 2i  \
    -layoutmode column \
    -selectmode multiple  \
    -activebackground grey95

.ss.l sort configure \
    -decreasing 0 \
    -auto 1 \
    -by type
    
image create picture closeIcon -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM1WBrM+rAEMigJ8c3Kb3OSII6kGABhp1JnaK1VGwjwKwtvHqNzzd263M3H4n2OH1QBwGw6
    nQkAOw==
}

proc debugselect {} {
    puts stderr selected=[.ss.l curselection]
}

proc icon { name } {
    global icons
    if { [info exists icons($name)] } {
	return $icons($name)
    }
    set file ""
    foreach dir {
       ~/neeshub/indeed/src/icons/filetype
       ~/neeshub/indeed/src/icons/misc
    } {
       if { [file exists $dir/$name.png] } {
          set file $dir/$name.png
	  break
       }
    }
    if { $file == "" } {
    	return closeIcon
    }
    set icons($name) [image create picture -file $file]
    return $icons($name)
}

foreach f [lsort [glob -nocomplain ~/*]] {
    set name [file tail $f]
    set ext [file ext $name]
    set ext [string trimleft $ext .]
    if { [file isdir $f] } {
	set ext .dir
    }
    .ss.l add -text $name -icon [icon $ext] -type $ext \
    	-bigicon [icon folder-green]
}

blt::tk::scrollbar .ss.xs
blt::tk::scrollbar .ss.ys

blt::table . \
    0,0 .rowmode \
    0,1 .colmode \
    0,2 .iconsmode \
    1,0 .ss -fill both -cspan 3 

blt::table configure . r0 -resize none

.colmode select

