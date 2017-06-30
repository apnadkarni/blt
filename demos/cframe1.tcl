
package require BLT

namespace eval periodicTable {
    variable _current ""
    variable _state
    variable _colors
    variable _table
    variable _types
    variable _tableData
    variable _chkImage
    variable _selected

    array set _colors {
	actinoid-activebackground                       \#cd679a 
	actinoid-activeforeground                       white 
	actinoid-disabledbackground                     \#ff99cc  
	actinoid-disabledforeground                     \#D97DAB
	actinoid-background                             \#ff99cc 
	actinoid-foreground                             black 
	alkali-metal-activebackground                   \#cd3434 
	alkali-metal-activeforeground                   white 
	alkali-metal-disabledbackground                 \#ff6666 
	alkali-metal-disabledforeground                 \#D04747
	alkali-metal-background                         \#ff6666 
	alkali-metal-foreground                         black 
	alkaline-earth-metal-activebackground           \#cdac7b 
	alkaline-earth-metal-activeforeground           white 
	alkaline-earth-metal-disabledbackground         \#ffdead 
	alkaline-earth-metal-disabledforeground         \#C19A64
	alkaline-earth-metal-background                 \#ffdead 
	alkaline-earth-metal-foreground                 black 
	halogen-activebackground                        \#cdcd67 
	halogen-activeforeground                        white 
	halogen-disabledbackground                      \#ffff99 
	halogen-disabledforeground                      \#D5D562
	halogen-background                              \#ffff99 
	halogen-foreground                              black 
	lanthanoid-activebackground                     \#cd8dcd
	lanthanoid-activeforeground                     white
	lanthanoid-disabledbackground                   \#ffbfff
	lanthanoid-disabledforeground                   \#D884D8
	lanthanoid-background                           \#ffbfff 
	lanthanoid-foreground                           black
	metalloid-activebackground                      \#9a9a67 
	metalloid-activeforeground                      white 
	metalloid-disabledbackground                    \#cccc99 
	metalloid-disabledforeground                    \#92922C
	metalloid-background                            \#cccc99
	metalloid-foreground                            black 
	noble-gas-activebackground                      \#8ecdcd 
	noble-gas-activeforeground                      white 
	noble-gas-disabledbackground                    \#c0ffff 
	noble-gas-disabledforeground                    \#7FC1C1
	noble-gas-background                            \#c0ffff 
	noble-gas-foreground                            black 
	other-non-metal-activebackground                \#6ecd6e 
	other-non-metal-activeforeground                white 
	other-non-metal-disabledbackground              \#a0ffa0 
	other-non-metal-disabledforeground              \#6ACD6A
	other-non-metal-background                      \#a0ffa0 
	other-non-metal-foreground                      black 
	post-transition-metal-activebackground          \#9a9a9a        
	post-transition-metal-activeforeground          white 
	post-transition-metal-disabledbackground        \#cccccc 
	post-transition-metal-disabledforeground        \#999999
	post-transition-metal-background                \#cccccc 
	post-transition-metal-foreground                black 
	transition-metal-activebackground               \#cd8e8e 
	transition-metal-activeforeground               white 
	transition-metal-disabledbackground             \#ffc0c0 
	transition-metal-disabledforeground             \#C77E7E
	transition-metal-background                     \#ffc0c0 
	transition-metal-foreground                     black 
	unknown-activebackground                        \#cdcdcd 
	unknown-activeforeground                        white 
	unknown-disabledbackground                      \#ffffff 
	unknown-disabledforeground                      \#B9B9B9
	unknown-background                              \#ffffff 
	unknown-foreground                              black 
    }
    set _tableData {
	Hydrogen        1  H  1.0079    1 1     other-non-metal
	Helium          2  He 4.0026    1 18    noble-gas
	Lithium         3  Li 6.941(2)  2 1     alkali-metal
	Beryllium       4  Be 9.0122    2 2     alkaline-earth-metal
	Boron           5  B  10.811(7) 2 13    metalloid
	Carbon          6  C  12.011    2 14    other-non-metal
	Nitrogen        7  N  14.007    2 15    other-non-metal
	Oxygen          8  O  15.999    2 16    other-non-metal 
	Fluorine        9  F  18.998    2 17    halogen
	Neon            10 Ne 20.180    2 18    noble-gas
	
	Sodium          11 Na 22.990    3 1     alkali-metal
	Magnesium       12 Mg 24.305    3 2     alkaline-earth-metal
	Aluminium       13 Al 26.982    3 13    post-transition-metal
	Silicon         14 Si 28.086    3 14    metalloid
	Phosphorus      15 P  30.974    3 15    other-non-metal 
	Sulfur          16 S  32.066(6) 3 16    other-non-metal
	Chlorine        17 Cl 35.453    3 17    halogen
	Argon           18 Ar 39.948(1) 3 18    noble-gas
	
	Potassium       19 K  39.098    4 1     alkali-metal
	Calcium         20 Ca 40.078(4) 4 2     alkaline-earth-metal
	Scandium        21 Sc 44.956    4 3     transition-metal
	Titanium        22 Ti 47.867(1) 4 4     transition-metal
	Vanadium        23 V  50.942(1) 4 5     transition-metal
	Chromium        24 Cr 51.996    4 6     transition-metal
	Manganese       25 Mn 54.938    4 7     transition-metal
	Iron            26 Fe 55.845(2) 4 8     transition-metal
	Cobalt          27 Co 58.933    4 9     transition-metal
	Nickel          28 Ni 58.693    4 10    transition-metal
	Copper          29 Cu 63.546(3) 4 11    transition-metal
	Zinc            30 Zn 65.39(2)  4 12    transition-metal
	Gallium         31 Ga 69.723(1) 4 13    post-transition-metal
	Germanium       32 Ge 72.61(2)  4 14    metalloid
	Arsenic         33 As 74.922    4 15    metalloid
	Selenium        34 Se 78.96(3)  4 16    other-non-metal 
	Bromine         35 Br 79.904(1) 4 17    halogen
	Krypton         36 Kr 83.80(1)  4 18    noble-gas
	
	Rubidium        37 Rb 85.468    5 1     alkali-metal
	Strontium       38 Sr 87.62(1)  5 2     alkaline-earth-metal
	Yttrium         39 Y  88.906    5 3     transition-metal
	Zirconium       40 Zr 91.224(2) 5 4     transition-metal
	Niobium         41 Nb 92.906    5 5     transition-metal
	Molybdenum      42 Mo 95.94(1)  5 6     transition-metal
	Technetium      43 Tc [97.907]  5 7     transition-metal
	Ruthenium       44 Ru 101.07(2) 5 8     transition-metal
	Rhodium         45 Rh 102.906   5 9     transition-metal
	Palladium       46 Pd 106.42(1) 5 10    transition-metal
	Silver          47 Ag 107.868   5 11    transition-metal
	Cadmium         48 Cd 112.411(8) 5 12   transition-metal
	Indium          49 In 114.818(3) 5 13   post-transition-metal
	Tin             50 Sn 118.710(7) 5 14   post-transition-metal
	Antimony        51 Sb 121.760(1) 5 15   metalloid
	Tellurium       52 Te 127.60(3) 5 16    metalloid
	Iodine          53 I  126.904   5 17    halogen
	Xenon           54 Xe 131.29(2) 5 18    noble-gas
	
	Cesium          55 Cs 132.905   6 1     alkali-metal
	Barium          56 Ba 137.327(7) 6 2    alkaline-earth-metal

	Hafnium         72 Hf 178.49(2) 6 4     transition-metal
	Tantalum        73 Ta 180.948   6 5     transition-metal
	Tungsten        74 W  183.84(1) 6 6     transition-metal
	Rhenium         75 Re 186.207(1) 6 7    transition-metal
	Osmium          76 Os 190.23(3) 6 8     transition-metal
	Iridium         77 Ir 192.217(3) 6 9    transition-metal
	Platinum        78 Pt 195.084(9) 6 10   transition-metal
	Gold            79 Au 196.967 6 11      transition-metal
	Mercury         80 Hg 200.59(2) 6 12    transition-metal
	Thallium        81 Tl 204.383   6 13    post-transition-metal
	Lead            82 Pb 207.2(1) 6 14     post-transition-metal
	Bismuth         83 Bi 208.980   6 15    post-transition-metal
	Polonium        84 Po [208.982] 6 16    metalloid
	Astatine        85 At [209.987] 6 17    halogen
	Radon           86 Rn [222.018] 6 18    noble-gas
	
	Francium        87 Fr [223.020] 7 1     alkali-metal
	Radium          88 Ra [226.0254] 7 2    alkaline-earth-metal

	Rutherfordium   104 Rf [263.113] 7 4    transition-metal
	Dubnium         105 Db [262.114] 7 5    transition-metal
	Seaborgium      106 Sg [266.122] 7 6    transition-metal
	Bohrium         107 Bh [264.1247] 7 7   transition-metal
	Hassium         108 Hs [269.134] 7 8    transition-metal
	Meitnerium      109 Mt [268.139] 7 9    transition-metal
	Darmstadtium    110 Ds [272.146] 7 10   transition-metal
	Roentgenium     111 Rg [272.154] 7 11   transition-metal
	Ununbium        112 Uub [277]   7 12    transition-metal
	Ununtrium       113 Uut [284] 7 13      post-transition-metal
	Ununquadium     114 Uuq [289] 7 14      post-transition-metal
	Ununpentium     115 Uup [288] 7 15      post-transition-metal
	Ununhexium      116 Uuh [292]  7 16     post-transition-metal
	Ununseptium     117 Uus ? 7 17          unknown
	Ununoctium      118 Uuo [294] 7 18      unknown
	
	Lanthanum       57 La 138.905 8 3       lanthanoid
	Cerium          58 Ce 140.116(1) 8 4    lanthanoid
	Praseodymium    59 Pr 140.908 8 5       lanthanoid
	Neodymium       60 Nd 144.242(3) 8 6    lanthanoid
	Promethium      61 Pm [144.913] 8 7     lanthanoid
	Samarium        62 Sm 150.36(2) 8 8     lanthanoid
	Europium        63 Eu 151.964(1) 8 9    lanthanoid
	Gadolinium      64 Gd 157.25(3) 8 10    lanthanoid
	Terbium         65 Tb 158.925   8 11    lanthanoid
	Dysprosium      66 Dy 162.500(1) 8 12   lanthanoid
	Holmium         67 Ho 164.930 8 13      lanthanoid
	Erbium          68 Er 167.259(3) 8 14   lanthanoid
	Thulium         69 Tm 168.934   8 15    lanthanoid
	Ytterbium       70 Yb 173.04(3) 8 16    lanthanoid
	Lutetium        71 Lu 174.967(1) 8 17   lanthanoid
	
	Actinium        89 Ac [227.027] 9 3     actinoid
	Thorium         90 Th 232.038   9 4     actinoid
	Protactinium    91 Pa 231.036 9 5       actinoid
	Uranium         92 U 238.029 9 6        actinoid
	Neptunium       93 Np [237.048] 9 7     actinoid
	Plutonium       94 Pu [244.064] 9 8     actinoid
	Americium       95 Am [243.061] 9 9     actinoid
	Curium          96 Cm [247.070] 9 10    actinoid
	Berkelium       97 Bk [247.070] 9 11    actinoid
	Californium     98 Cf [251.080] 9 12    actinoid
	Einsteinium     99 Es [252.083] 9 13    actinoid
	Fermium         100 Fm [257.095] 9 14   actinoid
	Mendelevium     101 Md [258.098] 9 15   actinoid
	Nobelium        102 No [259.101] 9 16   actinoid
	Lawrencium      103 Lr [262.110] 9 17   actinoid
    }
    foreach { name number symbol weight row column type } $_tableData {
	set _table($symbol) \
	    [list name $name number $number symbol $symbol \
		 weight $weight row $row column $column type $type]
    }

    array set _types {
	actinoid {
	    Ac Am Bk Cf Cm Es Fm Md Np Pu Pa Th U Lr No 
	}
	alkali-metal {
	    Cs Fr Li K Rb Na           
	}
	alkaline-earth-metal {
	    Ba Be Ca Mg Ra Sr 
	}
	halogen {
	    At Br Cl F I           
	}
	lanthanoid {
	    Ce Er Eu Gd Ho La Lu Nd Pr Pm Sm Tb Tm Yb Dy        
	}
	metalloid {
	    As B Ge Po Si Te Sb 
	}
	noble-gas {
	    Ar He Kr Ne Rn Xe
	}
	other-non-metal {
	    C H N S O P Se  
	}
	post-transition-metal {
	    Al Bi Ga In Pb Tl Tin Uuh Uup Uuq Uut   
	}
	transition-metal {
	    Cr Co Cu Db Au Hf Hs Ir Fe Mn Mt Hg Mo Ni Nb         
	    Os Pd Re Rh Rg Ru Rf Sc Sg Ag Ta Tc Ti W Uub 
	    V Y Zn Zr Bh Cd Ds Pt       
	}
	unknown {
	    Uus Uuo
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
    variable _types 
    variable _typeSelected

    frame $w -bg white
    foreach sym [array names _table] {
	set _state($sym) "normal"
    }
    canvas $w.table \
	-highlightthickness 0

    MakeCheckImage 11 11

    blt::scrollset $w.types \
	-yscrollbar $w.types.ys \
	-window $w.types.view 
    blt::tk::scrollbar $w.types.ys
    blt::comboview $w.types.view \
	-checkbuttonoutlinecolor grey80  \
	-height 1.5i -borderwidth 0

    $w.types.view add -type checkbutton -text (all) \
	-command [list periodicTable::SelectType $w all]  \
	-variable ::periodicTable::_typeSelected(all)
	
    foreach type [lsort -dictionary [array names _types]] {
	regsub -all -- {-} $type { } name
	set name [string totitle $name]
	$w.types.view add -type checkbutton -text $name \
	    -command [list periodicTable::SelectType $w $type] \
	    -variable ::periodicTable::_typeSelected($type)
    }
    blt::tk::button $w.cancel -text "Cancel" -font "Arial 9" \
	-command [list ::periodicTable::Cancel $w] \
	-relief flat -padx 1 -pady 1 -highlightthickness 0
    blt::tk::button $w.ok -text "OK" -font "Arial 9" \
	-command [list ::periodicTable::Ok $w] \
	-relief flat -padx 1 -pady 1 -highlightthickness 0
    blt::table $w \
	0,0 $w.table -fill both -cspan 3 \
	1,0 $w.types -fill x -cspan 3 \
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
    foreach sym [array names _table] {
        array set info $_table($sym)
        set x1 [expr ($info(column)-1)*$sqwidth+$xoffset]
        set y1 [expr ($info(row)-1)*$sqheight+$yoffset]
        set x2 [expr ($info(column)*$sqwidth)-2+$xoffset]
        set y2 [expr ($info(row)*$sqheight)-2+$yoffset]
        set type $info(type)
        if { $_state($sym) == "selected" } {
            set fg $_colors($type-disabledforeground)
            set bg $_colors($type-disabledbackground)
        } else { 
            set fg $_colors($type-foreground)
            set bg $_colors($type-background)
        }
        $c create rectangle $x1 $y1 $x2 $y2 -outline $fg -fill $bg \
            -tags $info(name)-rect
        if { $info(symbol) != "*" } {
            $c create text [expr ($x2+$x1)/2+1] [expr ($y2+$y1)/2+4] \
                -anchor c -fill $fg \
                -text [string range $info(symbol) 0 4] \
                -font "Arial 6" -tags $info(name)-symbol
            $c create text [expr $x2-2] [expr $y1+2] -anchor ne \
                -text $info(number) -fill $fg \
                -font "math1 4" -tags $info(name)-number
        }
	if { $_state($info(symbol)) == "selected" } {
	    $c create image $x1 $y1 -image $_chkImage -tags $info(name) -anchor nw
	}
        $c create rectangle $x1 $y1 $x2 $y2 -outline "" -fill "" \
            -tags $info(name) 
        if { $_state($info(symbol)) != "disabled" } {
	    $c bind $info(name) <Enter> \
		[list periodicTable::ActivateElement %W $info(symbol) %X %Y]
	    $c bind $info(name) <Leave> \
		[list periodicTable::DeactivateElement %W $info(symbol)]
	    $c bind $info(name) <ButtonRelease-1> \
		[list periodicTable::ToggleSelection $w $info(symbol)]
        }
    }

    set x [expr 2*$sqwidth+$xoffset+5]
    set y [expr $yoffset+3]
    
    set x [expr 3*$sqwidth+$xoffset+5]
    $c create text [expr $x - 5] $y -text "?" -tag "symbolName info" \
	-font "Arial 8 bold" -anchor ne
    set x [expr 3*$sqwidth+$xoffset+5]
    $c create text [expr $x + 5] $y -text "?" -tag "elementName info" \
	-font "Arial 8 italic" -anchor nw
    set x [expr 11*$sqwidth+$xoffset]
    $c create text [expr $x - 5] $y -text "?" \
	-tag "atomicNumber info" \
	-font "Arial 8" -anchor ne
    $c create text [expr $x + 5] $y -text "?" \
	-tag "elementType info" \
	-font "Arial 8 italic" -anchor nw
    $c itemconfigure info -state hidden
    update
    foreach { x1 y1 x2 y2 } [$c bbox all] break
    set width [expr $x2-$x1+$xoffset*2]
    set height [expr $y2-$y1+$yoffset*2]
    $c configure -height $height -width $width -background white
}


proc periodicTable::Cancel { w } {
    set menu [winfo parent $w]
    set mb [winfo parent $menu]
    $mb configure -text "Unselected"
    $menu unpost
}

proc periodicTable::Ok { w } {
    variable _state
    set menu [winfo parent $w]
    set mb [winfo parent $menu]
    set selected {}
    foreach sym [array names _state] {
	if { $_state($sym) == "selected" } {
	    lappend selected $sym
	}
    }
    set text [lsort -dictionary $selected]
    $mb configure -text [join $text {, }]
    $menu unpost
}

# ----------------------------------------------------------------------
# USAGE: select <name> 
#
# Used to manipulate the selection in the table.
#
# ----------------------------------------------------------------------
proc periodicTable::SelectType { w type } {
    variable _types
    variable _state
    variable _typeSelected
    variable _table

    if { $type == "all" } {
	if { $_typeSelected($type) } {
	    foreach sym [array names _table] {
		set _state($sym) "selected"
	    }
	} else {
	    foreach sym [array names _table] {
		set _state($sym) "normal"
	    }
	    foreach type [array names _typeSelected] {
		set _typeSelected($type) 0
	    }
	}
    } else {
	if { $_typeSelected($type) } {
	    foreach sym $_types($type) {
		set _state($sym) "selected"
	    }
	} else {
	    foreach sym $_types($type) {
		set _state($sym) "normal"
	    }
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
proc periodicTable::ToggleSelection { w sym } {
    variable _state

    if { ![info exists _state($sym)] } {
	set _state($sym) "selected"
    } else {
	set state $_state($sym)
	if { $state == "selected" } {
	    set _state($sym) "normal"
	} else {
	    set _state($sym) "selected"
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
proc periodicTable::selectElement { what } {
    set elem [FindElement $what]
    if { $elem == "" } {
        set elem "Hydrogen"
    }
    set _current $elem
    after idle [list periodicTable::RedrawTable $w]
}

# ----------------------------------------------------------------------
# USAGE: FindElement <what>
#
# Checks to see if the given string <what> is recognized as an element
# name, symbol, or number.  Returns the corresponding element name
# or "" if the element is not recognized.
# ----------------------------------------------------------------------
proc periodicTable::FindElement { what } {
    variable _table
    foreach name [array names _table] { 
        array unset info
        array set info $_table($name)
        if { $what eq $info(name) || $what eq $info(number) || 
             $what eq $info(symbol) } {
	    return $info(name)
        }
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: ElementIsType <name> <type> <type>...
#
# Checks to see if the given element <name> is one of the specified
# <type> values.  Returns true if the element is one of the types
# (logical or), or false if it is not.
# ----------------------------------------------------------------------
proc periodicTable::ElementIsType { name args } {
    variable _types
    
    foreach type $args {
        if {[info exists _types($type)]} {
            if {[lsearch -exact $_types($type) $name] >= 0} {
                return 1
            }
        }
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: _react
#
# Invoked automatically when the user has selected something from
# the listbox.  Unposts the drop-down and generates an event letting
# the client know that the selection has changed.
# ----------------------------------------------------------------------
proc  periodicTable::React { w } {
    set menu [winfo parent $w]
    $menu unpost
    event generate $w <<PeriodicTableSelect>>
}

proc periodicTable::ActivateElement { c id x y } {
    variable _colors
    variable _table
    
    array set info $_table($id)
    set type $info(type)
    set fg $_colors($type-activeforeground)
    set bg $_colors($type-activebackground)
    $c itemconfigure $id-rect -outline black -width 1 -fill $bg
    $c itemconfigure $id-number -fill white
    $c itemconfigure $id-symbol -fill white

    $c itemconfigure elementName -text $info(name)
    $c itemconfigure symbolName -text $info(symbol)
    $c itemconfigure atomicNumber -text "#$info(number)"
    $c itemconfigure atomicWeight -text $info(weight)
    regsub -all -- {-} $info(type) { } type
    $c itemconfigure elementType -text [string totitle $type]
    $c itemconfigure info -state normal
}

proc periodicTable::DeactivateElement { c id } {
    variable _table
    variable _colors
    
    array set info $_table($id)
    set type $info(type)
    set fg $_colors($type-foreground)
    set bg $_colors($type-background)
    $c itemconfigure $id-rect -outline $fg -width 1 -fill $bg
    $c itemconfigure $id-number -fill $fg
    $c itemconfigure $id-symbol -fill $fg
    $c itemconfigure info -state hidden
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
    -highlightthickness 0

periodicTable::NewTable .e.m.ptable

blt::table . \
    0,0 .e -fill x -anchor n 

