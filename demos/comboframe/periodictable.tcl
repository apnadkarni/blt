
package require BLT

namespace eval periodicTable {
    variable _current ""
    variable _state
    variable _colors
    variable _table
    variable _families
    variable _tableData
    variable _chkImage
    variable _selected
    variable _isSelected
    variable _symToName
    
    array set _colors {
	actinoids-activebackground                       \#cd679a 
	actinoids-activeforeground                       white 
	actinoids-disabledbackground                     \#ff99cc  
	actinoids-disabledforeground                     \#D97DAB
	actinoids-background                             \#ff99cc 
	actinoids-foreground                             black 
	alkali-metals-activebackground                   \#cd3434 
	alkali-metals-activeforeground                   white 
	alkali-metals-disabledbackground                 \#ff6666 
	alkali-metals-disabledforeground                 \#D04747
	alkali-metals-background                         \#ff6666 
	alkali-metals-foreground                         black 
	alkaline-earth-metals-activebackground           \#cdac7b 
	alkaline-earth-metals-activeforeground           white 
	alkaline-earth-metals-disabledbackground         \#ffdead 
	alkaline-earth-metals-disabledforeground         \#C19A64
	alkaline-earth-metals-background                 \#ffdead 
	alkaline-earth-metals-foreground                 black 
	halogens-activebackground                        \#cdcd67 
	halogens-activeforeground                        white 
	halogens-disabledbackground                      \#ffff99 
	halogens-disabledforeground                      \#D5D562
	halogens-background                              \#ffff99 
	halogens-foreground                              black 
	lanthanoids-activebackground                     \#cd8dcd
	lanthanoids-activeforeground                     white
	lanthanoids-disabledbackground                   \#ffbfff
	lanthanoids-disabledforeground                   \#D884D8
	lanthanoids-background                           \#ffbfff 
	lanthanoids-foreground                           black
	metalloids-activebackground                      \#9a9a67 
	metalloids-activeforeground                      white 
	metalloids-disabledbackground                    \#cccc99 
	metalloids-disabledforeground                    \#92922C
	metalloids-background                            \#cccc99
	metalloids-foreground                            black 
	inert-gases-activebackground                      \#8ecdcd 
	inert-gases-activeforeground                      white 
	inert-gases-disabledbackground                    \#c0ffff 
	inert-gases-disabledforeground                    \#7FC1C1
	inert-gases-background                            \#c0ffff 
	inert-gases-foreground                            black 
	other-non-metals-activebackground                \#6ecd6e 
	other-non-metals-activeforeground                white 
	other-non-metals-disabledbackground              \#a0ffa0 
	other-non-metals-disabledforeground              \#6ACD6A
	other-non-metals-background                      \#a0ffa0 
	other-non-metals-foreground                      black 
	post-transition-metals-activebackground          \#9a9a9a        
	post-transition-metals-activeforeground          white 
	post-transition-metals-disabledbackground        \#cccccc 
	post-transition-metals-disabledforeground        \#999999
	post-transition-metals-background                \#cccccc 
	post-transition-metals-foreground                black 
	transition-metals-activebackground               \#cd8e8e 
	transition-metals-activeforeground               white 
	transition-metals-disabledbackground             \#ffc0c0 
	transition-metals-disabledforeground             \#C77E7E
	transition-metals-background                     \#ffc0c0 
	transition-metals-foreground                     black 
	unknown-activebackground                        \#cdcdcd 
	unknown-activeforeground                        white 
	unknown-disabledbackground                      \#ffffff 
	unknown-disabledforeground                      \#B9B9B9
	unknown-background                              \#ffffff 
	unknown-foreground                              black 
	pnictogens-activebackground                        \#cdcdcd 
	pnictogens-activeforeground                        white 
	pnictogens-disabledbackground                      \#ffffff 
	pnictogens-disabledforeground                      \#B9B9B9
	pnictogens-background                              \#ffffff 
	pnictogens-foreground                              black 
    }
    set _tableData {
	Hydrogen        1  H  1.0079    1 1     other-non-metals
	Helium          2  He 4.0026    1 18    inert-gases
	Lithium         3  Li 6.941(2)  2 1     alkali-metals
	Beryllium       4  Be 9.0122    2 2     alkaline-earth-metals
	Boron           5  B  10.811(7) 2 13    metalloids
	Carbon          6  C  12.011    2 14    other-non-metals
	Nitrogen        7  N  14.007    2 15    other-non-metals
	Oxygen          8  O  15.999    2 16    other-non-metals 
	Fluorine        9  F  18.998    2 17    halogens
	Neon            10 Ne 20.180    2 18    inert-gases
	
	Sodium          11 Na 22.990    3 1     alkali-metals
	Magnesium       12 Mg 24.305    3 2     alkaline-earth-metals
	Aluminium       13 Al 26.982    3 13    post-transition-metals
	Silicon         14 Si 28.086    3 14    metalloids
	Phosphorus      15 P  30.974    3 15    other-non-metals 
	Sulfur          16 S  32.066(6) 3 16    other-non-metals
	Chlorine        17 Cl 35.453    3 17    halogens
	Argon           18 Ar 39.948(1) 3 18    inert-gases
	
	Potassium       19 K  39.098    4 1     alkali-metals
	Calcium         20 Ca 40.078(4) 4 2     alkaline-earth-metals
	Scandium        21 Sc 44.956    4 3     transition-metals
	Titanium        22 Ti 47.867(1) 4 4     transition-metals
	Vanadium        23 V  50.942(1) 4 5     transition-metals
	Chromium        24 Cr 51.996    4 6     transition-metals
	Manganese       25 Mn 54.938    4 7     transition-metals
	Iron            26 Fe 55.845(2) 4 8     transition-metals
	Cobalt          27 Co 58.933    4 9     transition-metals
	Nickel          28 Ni 58.693    4 10    transition-metals
	Copper          29 Cu 63.546(3) 4 11    transition-metals
	Zinc            30 Zn 65.39(2)  4 12    transition-metals
	Gallium         31 Ga 69.723(1) 4 13    post-transition-metals
	Germanium       32 Ge 72.61(2)  4 14    metalloids
	Arsenic         33 As 74.922    4 15    metalloids
	Selenium        34 Se 78.96(3)  4 16    other-non-metals 
	Bromine         35 Br 79.904(1) 4 17    halogens
	Krypton         36 Kr 83.80(1)  4 18    inert-gases
	
	Rubidium        37 Rb 85.468    5 1     alkali-metals
	Strontium       38 Sr 87.62(1)  5 2     alkaline-earth-metals
	Yttrium         39 Y  88.906    5 3     transition-metals
	Zirconium       40 Zr 91.224(2) 5 4     transition-metals
	Niobium         41 Nb 92.906    5 5     transition-metals
	Molybdenum      42 Mo 95.94(1)  5 6     transition-metals
	Technetium      43 Tc [97.907]  5 7     transition-metals
	Ruthenium       44 Ru 101.07(2) 5 8     transition-metals
	Rhodium         45 Rh 102.906   5 9     transition-metals
	Palladium       46 Pd 106.42(1) 5 10    transition-metals
	Silver          47 Ag 107.868   5 11    transition-metals
	Cadmium         48 Cd 112.411(8) 5 12   transition-metals
	Indium          49 In 114.818(3) 5 13   post-transition-metals
	Tin             50 Sn 118.710(7) 5 14   post-transition-metals
	Antimony        51 Sb 121.760(1) 5 15   metalloids
	Tellurium       52 Te 127.60(3) 5 16    metalloids
	Iodine          53 I  126.904   5 17    halogens
	Xenon           54 Xe 131.29(2) 5 18    inert-gases
	
	Cesium          55 Cs 132.905   6 1     alkali-metals
	Barium          56 Ba 137.327(7) 6 2    alkaline-earth-metals

	Hafnium         72 Hf 178.49(2) 6 4     transition-metals
	Tantalum        73 Ta 180.948   6 5     transition-metals
	Tungsten        74 W  183.84(1) 6 6     transition-metals
	Rhenium         75 Re 186.207(1) 6 7    transition-metals
	Osmium          76 Os 190.23(3) 6 8     transition-metals
	Iridium         77 Ir 192.217(3) 6 9    transition-metals
	Platinum        78 Pt 195.084(9) 6 10   transition-metals
	Gold            79 Au 196.967 6 11      transition-metals
	Mercury         80 Hg 200.59(2) 6 12    transition-metals
	Thallium        81 Tl 204.383   6 13    post-transition-metals
	Lead            82 Pb 207.2(1) 6 14     post-transition-metals
	Bismuth         83 Bi 208.980   6 15    post-transition-metals
	Polonium        84 Po [208.982] 6 16    metalloids
	Astatine        85 At [209.987] 6 17    halogens
	Radon           86 Rn [222.018] 6 18    inert-gases
	
	Francium        87 Fr [223.020] 7 1     alkali-metals
	Radium          88 Ra [226.0254] 7 2    alkaline-earth-metals

	Rutherfordium   104 Rf [263.113] 7 4    transition-metals
	Dubnium         105 Db [262.114] 7 5    transition-metals
	Seaborgium      106 Sg [266.122] 7 6    transition-metals
	Bohrium         107 Bh [264.1247] 7 7   transition-metals
	Hassium         108 Hs [269.134] 7 8    transition-metals
	Meitnerium      109 Mt [268.139] 7 9    transition-metals
	Darmstadtium    110 Ds [272.146] 7 10   transition-metals
	Roentgenium     111 Rg [272.154] 7 11   transition-metals
	Copernicium     112 Cn [277]   7 12     transition-metals
	Nihonium        113 Nh  [284] 7 13      post-transition-metals
	Flerovium       114 Fl  [289] 7 14      post-transition-metals
	Moscovium       115 Mc  [288] 7 15      post-transition-metals
	Livermorium     116 Lv  [292]  7 16     post-transition-metals
	Tennessine      117 Ts ? 7 17           halogens
        Oganesson       118 Og [294] 7 18       inert-gases
	
	Lanthanum       57 La 138.905 8 3       lanthanoids
	Cerium          58 Ce 140.116(1) 8 4    lanthanoids
	Praseodymium    59 Pr 140.908 8 5       lanthanoids
	Neodymium       60 Nd 144.242(3) 8 6    lanthanoids
	Promethium      61 Pm [144.913] 8 7     lanthanoids
	Samarium        62 Sm 150.36(2) 8 8     lanthanoids
	Europium        63 Eu 151.964(1) 8 9    lanthanoids
	Gadolinium      64 Gd 157.25(3) 8 10    lanthanoids
	Terbium         65 Tb 158.925   8 11    lanthanoids
	Dysprosium      66 Dy 162.500(1) 8 12   lanthanoids
	Holmium         67 Ho 164.930 8 13      lanthanoids
	Erbium          68 Er 167.259(3) 8 14   lanthanoids
	Thulium         69 Tm 168.934   8 15    lanthanoids
	Ytterbium       70 Yb 173.04(3) 8 16    lanthanoids
	Lutetium        71 Lu 174.967(1) 8 17   lanthanoids
	
	Actinium        89 Ac [227.027] 9 3     actinoids
	Thorium         90 Th 232.038   9 4     actinoids
	Protactinium    91 Pa 231.036 9 5       actinoids
	Uranium         92 U 238.029 9 6        actinoids
	Neptunium       93 Np [237.048] 9 7     actinoids
	Plutonium       94 Pu [244.064] 9 8     actinoids
	Americium       95 Am [243.061] 9 9     actinoids
	Curium          96 Cm [247.070] 9 10    actinoids
	Berkelium       97 Bk [247.070] 9 11    actinoids
	Californium     98 Cf [251.080] 9 12    actinoids
	Einsteinium     99 Es [252.083] 9 13    actinoids
	Fermium         100 Fm [257.095] 9 14   actinoids
	Mendelevium     101 Md [258.098] 9 15   actinoids
	Nobelium        102 No [259.101] 9 16   actinoids
	Lawrencium      103 Lr [262.110] 9 17   actinoids
    }
    foreach { name number symbol weight row column family } $_tableData {
	set _table($name) \
	    [list name $name number $number symbol $symbol \
		 weight $weight row $row column $column family $family]
    }
    array set _families {
        actinoids {
            Actinium Americium Berkelium Californium Curium 
            Einsteinium Fermium Mendelevium Neptunium Plutonium Protactinium 
            Thorium Uranium Lawrencium Nobelium 
        }
        alkali-metals {
            Cesium Francium Lithium Potassium Rubidium Sodium           
        }
        alkaline-earth-metals {
            Barium Beryllium Calcium Magnesium Radium Strontium 
        }
        halogens {
            Astatine Bromine Chlorine Fluorine Iodine Tennessine
        }
        lanthanoids {
            Cerium Erbium Europium Gadolinium Holmium  Lanthanum     
            Lutetium Neodymium Praseodymium Promethium Samarium Terbium 
            Thulium Ytterbium Dysprosium        
        }
        metalloids {
            Arsenic Boron Germanium Polonium Silicon Tellurium Antimony 
        }
        inert-gases {
            Argon Helium Krypton Neon Radon Xenon 
            Oganesson
        }
        other-non-metals {
            Carbon Hydrogen Nitrogen Sulfur Oxygen Phosphorus Selenium  
        }
        post-transition-metals {
            Aluminium Bismuth Gallium Indium Lead Thallium Tin Livermorium
            Flerovium Nihonium
        }
        transition-metals {
            Chromium Cobalt Copper Dubnium Gold Hafnium Hassium Iridium         
            Iron Manganese Meitnerium Mercury Molybdenum Nickel Niobium         
            Osmium Palladium Rhenium Rhodium Roentgenium Ruthenium 
            Rutherfordium Scandium Seaborgium Silver Tantalum Technetium 
            Titanium Tungsten Copernicium Vanadium Yttrium Zinc Zirconium 
            Bohrium Cadmium Darmstadtium Platinum       
        }
	pnictogens {
	    Nitrogen Phosphorus Arsenic Antimony Bismuth Moscovium
	}
	chalcogens {
	    Oxygen Sulfur Selenium Tellurium Polonium Livermorium
	}
    }
}

proc periodicTable::MakeCheckImage { w h } {
    variable _chkImage

    set cw [expr $w - 4]
    set ch [expr $h - 4]
    set x 2
    set y 2
    lappend points \
	$x [expr $y + (0.4 * $ch)] \
	$x [expr $y + (0.6 * $ch)] \
	[expr $x + (0.4 * $cw)] [expr $y + $ch] \
	[expr $x + $cw] [expr $y + (0.2 * $ch)] \
	[expr $x + $cw] $y \
	[expr $x + (0.4 * $cw)] [expr $y + (0.7 * $ch)] \
	$x [expr $y + (0.4 * $ch)] 
    set img [image create picture -width $w -height $h]
    $img blank 0x0
    $img draw polygon -coords $points \
	-antialiased yes -color red2 \
	-shadow {-offset 2 -width 2 -color 0x5F000000}
    set _chkImage $img
}

proc periodicTable::NewTable { w } {
    variable _table
    variable _state
    variable _families 
    variable _isSelected
    variable _symToName
    
    frame $w -bg white
    foreach name [array names _table] {
	array set elemInfo $_table($name)
	set _symToName($elemInfo(symbol)) $elemInfo(name)
	set _state($name) "normal"
    }
    canvas $w.table \
	-highlightthickness 0

    MakeCheckImage 11 11

    blt::scrollset $w.selections \
	-yscrollbar $w.selections.ys \
	-window $w.selections.view 
    blt::tk::scrollbar $w.selections.ys
    blt::comboview $w.selections.view \
	-checkbuttonoutlinecolor grey80  \
	-height 1.5i -borderwidth 0 -font "Arial 9"

    $w.selections.view add -type checkbutton -text (all) \
	-command [list periodicTable::Select $w all]  \
	-variable ::periodicTable::_isSelected(all)

    foreach family [lsort -dictionary [array names _families]] {
	regsub -all -- {-} $family { } label
	set label [string totitle $label]
	$w.selections.view add -type checkbutton -text $label \
	    -command [list periodicTable::Select $w $family] \
	    -variable ::periodicTable::_isSelected($family)
    }
    $w.selections.view add -type separator
    foreach name [lsort -dictionary [array names _table]] {
        array set elem $_table($name)
	set label [string totitle $elem(name)]
	$w.selections.view add -type checkbutton -text $label \
	    -command [list periodicTable::Select $w $name] \
	    -variable ::periodicTable::_isSelected($name)
    }
    blt::tk::button $w.cancel -text "Cancel" -font "Arial 9" \
	-command [list ::periodicTable::Cancel $w] \
	-relief flat -padx 1 -pady 1 -highlightthickness 0
    blt::tk::button $w.ok -text "OK" -font "Arial 9" \
	-command [list ::periodicTable::Ok $w] \
	-relief flat -padx 1 -pady 1 -highlightthickness 0
    blt::table $w \
	0,0 $w.table -fill both -cspan 3 \
	1,0 $w.selections -fill x -cspan 3 \
	2,1 $w.cancel -padx 10 -pady 5 -width 0.8i \
	2,2 $w.ok -padx 10 -pady 5 -width 0.8i
    
    RedrawTable $w
}
    
proc periodicTable::RedrawTable { w } {
    variable _table
    variable _colors
    variable _state
    variable _chkImage

    set sqwidth [winfo pixels . 0.22i]
    set sqheight [winfo pixels . 0.22i]
    set xoffset 4
    set yoffset 4
    set last ""
    set c $w.table
    $c delete all
    foreach name [array names _table] {
        array set elem $_table($name)
        set x1 [expr ($elem(column)-1)*$sqwidth+$xoffset]
        set y1 [expr ($elem(row)-1)*$sqheight+$yoffset]
        set x2 [expr ($elem(column)*$sqwidth)-2+$xoffset]
        set y2 [expr ($elem(row)*$sqheight)-2+$yoffset]
        set family $elem(family)
        if { $_state($name) == "selected" } {
            set fg $_colors($family-foreground)
            set bg $_colors($family-background)
        } else { 
            set fg $_colors($family-foreground)
            set fg white
            set bg $_colors($family-disabledforeground)
        }
        $c create rectangle $x1 $y1 $x2 $y2 -outline $fg -fill $bg \
            -tags $elem(name)-rect
        if { $elem(symbol) != "*" } {
            $c create text [expr ($x2+$x1)/2+1] [expr ($y2+$y1)/2+4] \
                -anchor c -fill $fg \
                -text [string range $elem(symbol) 0 4] \
                -font "Arial 6" -tags $elem(name)-symbol
            $c create text [expr $x2-2] [expr $y1+2] -anchor ne \
                -text $elem(number) -fill $fg \
                -font "math1 4" -tags $elem(name)-number
        }
	if { $_state($elem(name)) == "selected" } {
	    $c create image $x1 $y1 -image $_chkImage -tags $elem(name) \
		-anchor nw
	}
        $c create rectangle $x1 $y1 $x2 $y2 -outline "" -fill "" \
            -tags $elem(name) 
        if { $_state($elem(name)) != "disabled" } {
	    $c bind $elem(name) <Enter> \
		[list periodicTable::ActivateElement %W $elem(name) %X %Y]
	    $c bind $elem(name) <Leave> \
		[list periodicTable::DeactivateElement %W $elem(name)]
	    $c bind $elem(name) <ButtonRelease-1> \
		[list periodicTable::ToggleSelection $w $elem(name)]
        }
    }

    set x [expr 2*$sqwidth+$xoffset+5]
    set y [expr $yoffset+3]
    
    set x [expr 3*$sqwidth+$xoffset+5]
    $c create text [expr $x - 5] $y -text "?" -tag "symbolName elemInfo" \
	-font "Arial 8 bold" -anchor ne
    set x [expr 3*$sqwidth+$xoffset+5]
    $c create text [expr $x + 5] $y -text "?" -tag "elementName elemInfo" \
	-font "Arial 8 italic" -anchor nw
    set x [expr 11*$sqwidth+$xoffset]
    $c create text [expr $x - 5] $y -text "?" \
	-tag "atomicNumber elemInfo" \
	-font "Arial 8" -anchor ne
    $c create text [expr $x + 5] $y -text "?" \
	-tag "elementFamily elemInfo" \
	-font "Arial 8 italic" -anchor nw
    $c itemconfigure elemInfo -state hidden
    update
    foreach { x1 y1 x2 y2 } [$c bbox all] break
    set width [expr $x2-$x1+$xoffset*2]
    set height [expr $y2-$y1+$yoffset*2]
    $c configure -height $height -width $width -background white
}


proc periodicTable::Cancel { w } {
    set menu [winfo parent $w]
    set mb [winfo parent $menu]
    $menu unpost
}

proc periodicTable::Ok { w } {
    variable _state
    variable _table
    
    set selected {}
    foreach name [array names _state] {
	if { $_state($name) == "selected" } {
	    array set elem $_table($name)
	    lappend selected $elem(symbol)
	}
    }
    set text [lsort -dictionary $selected]
    set menu [winfo parent $w]
    set mb [winfo parent $menu]
    $mb configure -text [join $text {, }]
    $menu unpost
}

proc periodicTable::ResetMenu { w string } {
    variable _symToName
    variable _state
    regsub -all {,} $string { } string
    foreach name [array names _state] {
	set _state($name) "normal"
    }
    foreach sym $string {
	set name $_symToName($sym)
	set _state($name) selected
    }
    after idle [list periodicTable::RedrawTable $w]
}

# ----------------------------------------------------------------------
# USAGE: select <name> 
#
# Used to manipulate the selection in the table.
#
# ----------------------------------------------------------------------
proc periodicTable::Select { w family } {
    variable _families
    variable _state
    variable _isSelected
    variable _table

    if { $family == "all" } {
	if { $_isSelected($family) } {
	    foreach name [array names _table] {
		set _state($name) "selected"
	    }
	} else {
	    foreach name [array names _table] {
		set _state($name) "normal"
	    }
	    foreach family [array names _isSelected] {
		set _isSelected($family) 0
	    }
	}
    } elseif { [info exists _families($family)] } {
	if { $_isSelected($family) } {
	    foreach name $_families($family) {
		set _state($name) "selected"
	    }
	} else {
	    foreach name $_families($family) {
		set _state($name) "normal"
	    }
	}
    } elseif { [info exists _state($family)] } {
	if { $_isSelected($family) } {
	    set _state($family) "selected"
	} else {
	    set _state($family) "normal"
	}
    }	
    after idle [list periodicTable::RedrawTable $w]
}

# ----------------------------------------------------------------------
# USAGE: select <name> 
#
# Used to manipulate the selection in the table.
#
# ----------------------------------------------------------------------
proc periodicTable::ToggleSelection { w name } {
    variable _state

    if { ![info exists _state($name)] } {
	set _state($name) "selected"
    } else {
	set state $_state($name)
	if { $state == "selected" } {
	    set _state($name) "normal"
	} else {
	    set _state($name) "selected"
	}
    }
    after idle [list periodicTable::RedrawTable $w]
}

proc periodicTable::ActivateElement { c id x y } {
    variable _colors
    variable _table
    
    array set elem $_table($id)
    set family $elem(family)
    set fg $_colors($family-activeforeground)
    set bg $_colors($family-activebackground)
    $c itemconfigure $id-rect -outline black -width 1 -fill $bg
    $c itemconfigure $id-number -fill white
    $c itemconfigure $id-symbol -fill white

    $c itemconfigure elementName -text $elem(name)
    $c itemconfigure symbolName -text $elem(symbol)
    $c itemconfigure atomicNumber -text "#$elem(number)"
    $c itemconfigure atomicWeight -text $elem(weight)
    regsub -all -- {-} $elem(family) { } family
    $c itemconfigure elementFamily -text [string totitle $family]
    $c itemconfigure elemInfo -state normal
}

proc periodicTable::DeactivateElement { c id } {
    variable _table
    variable _colors
    variable _state
    
    array set elem $_table($id)
    set family $elem(family)
    set fg $_colors($family-foreground)
    set bg $_colors($family-background)
    if { $_state($id) == "selected" } {
	set fg $_colors($family-foreground)
	set bg $_colors($family-background)
    } else { 
	set fg $_colors($family-foreground)
	set fg white
	set bg $_colors($family-disabledforeground)
    }
    $c itemconfigure $id-rect -outline $fg -width 1 -fill $bg
    $c itemconfigure $id-number -fill $fg
    $c itemconfigure $id-symbol -fill $fg
    $c itemconfigure elemInfo -state hidden
}

proc periodicTable::value {{value "" }} {
    variable _current
    
    if { $value != "" } {
        set _current $value
    }
}


set imgData {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}

set icon2 [image create picture -file images/blt98.gif]
set icon [image create picture -data $imgData]
set bg white

set image ""
option add *ComboEntry.takeFocus 1

if { [file exists ../library] } {
    set blt_library ../library
}

#    -postcommand {.e.m configure -width [winfo width .e] ; update} \
    set myIcon ""
blt::comboentry .e \
    -image $image \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -arrowrelief flat \
    -textwidth 0 \
    -menu .e.m \
    -exportselection yes \
    -clearbutton yes 

#    -bg $bg 

blt::comboframe .e.m  \
    -restrictwidth min \
    -window .e.m.ptable \
    -highlightthickness 0 \
    -resetcommand [list periodicTable::ResetMenu .e.m.ptable]

periodicTable::NewTable .e.m.ptable

blt::table . \
    0,0 .e -fill x -anchor n 

