package require BLT

# Test switches
#
# -before, -after, -node for "insert" operation
#

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test palette.1 {palette no args} {
    list [catch {blt::palette} msg] $msg
} {1 {wrong # args: should be one of...
  blt::palette cget name option
  blt::palette configure name ?option value?...
  blt::palette create ?name? ?option value?...
  blt::palette delete ?name?...
  blt::palette draw name picture
  blt::palette exists name
  blt::palette interpolate name value ?switches?
  blt::palette names ?pattern?...}}

test palette.2 {palette badOp} {
    list [catch {blt::palette badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  blt::palette cget name option
  blt::palette configure name ?option value?...
  blt::palette create ?name? ?option value?...
  blt::palette delete ?name?...
  blt::palette draw name picture
  blt::palette exists name
  blt::palette interpolate name value ?switches?
  blt::palette names ?pattern?...}}

test palette.3 {palette create} {
    list [catch {blt::palette create} msg] $msg
} {0 ::palette0}

test palette.4 {palette create myPalette} {
    list [catch {blt::palette create myPalette} msg] $msg
} {0 ::myPalette}

test palette.5 {palette names} {
    list [catch {lsort [blt::palette names]} msg] $msg
} {0 {::BCGYR ::BGYOR ::ROYGB ::RYGCB ::blue ::blue-to-brown ::blue-to-gray ::blue-to-green ::blue-to-grey ::blue-to-orange ::brown-to-blue ::green-to-magenta ::grey-to-blue ::greyscale ::myPalette ::nanohub ::orange-to-blue ::palette0 ::rainbow ::spectral ::spectral-scheme}}

test palette.6 {palette create -colors {} -opacities {}} {
    list [catch {blt::palette create -colors {} -opacities {}} msg] $msg
} {0 ::palette1}

test palette.7 {palette delete names} {
    list [catch {eval blt::palette delete [blt::palette names]} msg] $msg
} {0 {}}

test palette.8 {palette names} {
    list [catch {lsort [blt::palette name]} msg] $msg
} {0 {}}

test palette.9 {palette create myPalette} {
    list [catch {blt::palette create myPalette} msg] $msg
} {0 ::myPalette}

test palette.10 {palette configure myPalette} {
    list [catch {blt::palette configure myPalette} msg] $msg
} {0 {{-colors {} {}} {-opacities {} {}} {-rgbcolors {} {}} {-baseopacity {} 100.0}}}

test palette.11 {palette configure ::myPalette} {
    list [catch {blt::palette configure ::myPalette} msg] $msg
} {0 {{-colors {} {}} {-opacities {} {}} {-rgbcolors {} {}} {-baseopacity {} 100.0}}}

test palette.12 {palette configure badPalette} {
    list [catch {blt::palette configure badPalette} msg] $msg
} {1 {can't find a palette "badPalette"}}

test palette.13 {palette configure ::badNs::myPalette} {
    list [catch {blt::palette configure ::badNs::myPalette} msg] $msg
} {1 {unknown namespace "::badNs"}}

test palette.14 {palette configure ::myPalette -colors} {
    list [catch {blt::palette configure ::myPalette -colors} msg] $msg
} {0 {-colors {} {}}}

test palette.15 {palette configure ::myPalette -opacities} {
    list [catch {blt::palette configure ::myPalette -opacities} msg] $msg
} {0 {-opacities {} {}}}

test palette.16 {palette configure ::myPalette -colors { blue red }} {
    list [catch {
	blt::palette configure ::myPalette -colors { blue red }
    } msg] $msg
} {0 {}}

test palette.17 {palette configure ::myPalette} {
    list [catch {blt::palette configure ::myPalette} msg] $msg
} {0 {{-colors {} {{0.0% 0xff0000ff 100.0% 0xffff0000}}} {-opacities {} {}} {-rgbcolors {} {{0.0% 0xff0000ff 100.0% 0xffff0000}}} {-baseopacity {} 100.0}}}

test palette.18 {palette interpolate myPalette (missing value)} {
    list [catch {blt::palette interpolate myPalette} msg] $msg
} {1 {wrong # args: should be "blt::palette interpolate name value ?switches?"}}

test palette.19 {palette interpolate myPalette badValue} {
    list [catch {blt::palette interpolate myPalette badValue} msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test palette.20 {palette interpolate myPalette 50.0% badSwitch} {
    list [catch {blt::palette interpolate myPalette 50.0% badSwitch} msg] $msg
} {1 {unknown switch "badSwitch"
following switches are available:
   -min 
   -max }}

test palette.21 {palette interpolate myPalette 50.0% -min badValue} {
    list [catch {
	blt::palette interpolate myPalette 50.0% -min badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test palette.22 {palette interpolate myPalette 50.0% -max badValue} {
    list [catch {
	blt::palette interpolate myPalette 50.0% -max badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test palette.23 {palette interpolate myPalette 50.0% -min -1.0} {
    list [catch {
	blt::palette interpolate myPalette 50.0% -min -1.0
    } msg] $msg
} {0 {255 127 0 128}}

test palette.24 {palette interpolate myPalette 50.0% -max -1.0} {
    list [catch {
	blt::palette interpolate myPalette 50.0% -max -1.0
    } msg] $msg
} {1 {value "50.0%" not in any range}}


test palette.25 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 {255 127 0 128}}

test palette.26 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 {255 127 0 128}}

test palette.27 {palette interpolate myPalette 1.0} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 {255 255 0 0}}

test palette.28 {palette interpolate myPalette 0.0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 {255 0 0 255}}

test palette.29 {palette configure ::myPalette -opacities { 0.0 0.25 }} {
    list [catch {
	blt::palette configure ::myPalette -opacities { 0.0% 25% }
    } msg] $msg
} {0 {}}

test palette.30 {palette configure myPalette -opacities {}} {
    list [catch {blt::palette configure myPalette -opacities {}} msg] $msg
} {0 {}}

test palette.31 {palette configure myPalette -color {blue green red white}} {
    list [catch {
	blt::palette configure myPalette -color {blue green red white}
    } msg] $msg
} {0 {}}

test palette.32 {palette cget myPalette -color} {
    list [catch {
	blt::palette cget myPalette -color
    } msg] $msg
} {0 {{0.0% 0xff0000ff 33.333333333333336% 0xff00ff00} {33.333333333333336% 0xff00ff00 66.66666666666667% 0xffff0000} {66.66666666666667% 0xffff0000 100.0% 0xffffffff}}}

test palette.33 {configure myPalette -color {{0.0% white} {100% black}}} {
    list [catch {
	blt::palette configure myPalette -color {{0.0% white} {100% black}}
    } msg] $msg
} {0 {}}

test palette.34 {palette cget myPalette -color} {
    list [catch {
	blt::palette cget myPalette -color
    } msg] $msg
} {0 {{0.0% 0xffffffff 100.0% 0xff000000}}}

test palette.35 {configure myPalette -color ....} {
    list [catch {
	blt::palette configure myPalette \
	    -color {{0.0% white 100% black} {25.0% red 50.0% green}}
    } msg] $msg
} {0 {}}

test palette.36 {palette cget myPalette -color} {
    list [catch {
	blt::palette cget myPalette -color
    } msg] $msg
} {0 {{0.0% 0xffffffff 100.0% 0xff000000} {25.0% 0xffff0000 50.0% 0xff00ff00}}}

test palette.37 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 {255 128 128 128}}

test palette.38 {palette interpolate myPalette 25.0%} {
    list [catch {blt::palette interpolate myPalette 25.0%} msg] $msg
} {0 {255 192 192 192}}

test palette.39 {palette interpolate myPalette 75.0%} {
    list [catch {blt::palette interpolate myPalette 75.0%} msg] $msg
} {0 {255 64 64 64}}

test palette.40 {palette interpolate myPalette 0.0%} {
    list [catch {blt::palette interpolate myPalette 0.0%} msg] $msg
} {0 {255 255 255 255}}

test palette.41 {palette interpolate myPalette 100.0%} {
    list [catch {blt::palette interpolate myPalette 100.0%} msg] $msg
} {0 {255 0 0 0}}

test palette.42 {palette create myIrregular} {
    list [catch {blt::palette create irregular myIrregular} msg] $msg
} {0 ::myIrregular}

test palette.43 {palette create} {
    list [catch {blt::palette create cloud} msg] $msg
} {0 ::palette2}

test palette.44 {palette type palette2} {
    list [catch {blt::palette type palette2} msg] $msg
} {0 cloud}

test palette.45 {palette create cloud myCloud} {
    list [catch {blt::palette create cloud myCloud} msg] $msg
} {0 ::myCloud}

test palette.46 {palette create triangular} {
    list [catch {blt::palette create triangular} msg] $msg
} {0 ::palette3}

test palette.47 {palette type palette3} {
    list [catch {blt::palette type palette3} msg] $msg
} {0 triangular}

test palette.48 {palette create triangular myTriangular} {
    list [catch {blt::palette create triangular myTriangular} msg] $msg
} {0 ::myTriangular}

test palette.49 {palette names} {
    list [catch {lsort [blt::palette names]} msg] $msg
} {0 {::palette0 ::palette1 ::palette2 ::palette3 ::myCloud ::myIrregular ::myRegular ::myTriangular}}

test palette.50 {palette names palette*} {
    list [catch {lsort [blt::palette names ::palette*]} msg] $msg
} {0 {::palette0 ::palette1 ::palette2 ::palette3}}

test palette.51 {palette names *my*} {
    list [catch {lsort [blt::palette names *my*]} msg] $msg
} {0 {::myCloud ::myIrregular ::myRegular ::myTriangular}}

test palette.52 {palette delete [palette names]} {
    list [catch {eval blt::palette delete [blt::palette names]} msg] $msg
} {0 {}}

test palette.53 {palette names} {
    list [catch {blt::palette names} msg] $msg
} {0 {}}

test palette.54 {palette create cloud myPalette} {
    list [catch {blt::palette create cloud myPalette} msg] $msg
} {0 ::myPalette}

test palette.55 {palette names} {
    list [catch {blt::palette names} msg] $msg
} {0 ::myPalette}

test palette.56 {palette configure myPalette} {
    list [catch {blt::palette configure myPalette} msg] $msg
} {0 {{-x xData {} {} {}} {-y yData {} {} {}}}}

test palette.57 {palette cget myPalette -x} {
    list [catch {blt::palette cget myPalette -x} msg] $msg
} {0 {}}

test palette.58 {palette cget myPalette -y} {
    list [catch {blt::palette cget myPalette -y} msg] $msg
} {0 {}}

blt::vector create xVector
blt::vector create yVector
set x { 
    0.8599351407956028
    0.01299275167490066
    0.10006096150956623
    0.3063476742276734
    0.8472018222067028
    0.2493283888162523
    0.7822191093907058
    0.32795977960800116
    0.4562450792199009
    0.13385382981245186
    0.6434438630534451
    0.8759254910132697
    0.350641541040833
    0.6534163864493223
    0.11300301987314398
    0.4321670824077799
    0.002391220327599086
    0.8047882219288311
    0.46874983272894255
    0.37010831271847877
    0.07943128950055112
    0.7598073233418177
    0.4968837505893866
    0.029975440653210228
    0.9404316164333757
    0.8765663673072623
    0.5263499102472302
    0.6054835283535382
    0.7606121127023506
    0.8961463297455055
    0.10511909558830368
    0.10101586202240043
    0.9877560601194837
    0.34725683357508785
    0.11749982340628762
    0.4540099390047416
    0.36759068210531964
    0.07012609360140232
    0.23390792693343698
    0.051269971043417684
    0.6871491280685298
    0.8983064685001096
    0.2498518024467522
    0.1841222950830499
    0.49662455385197646
    0.20057862808928917
    0.07510367302979759
    0.2601305285511515
    0.2957099219787729
    0.9983247525315591
    0.04606411932232035
    0.7335308180087381
    0.24874432438693717
    0.3157005612109174
    0.47625904237302663
    0.03829817093177468
    0.24154086007756703
    0.8853938852888206
    0.2569304710253242
    0.2531016926602021
    0.6570601931190403
    0.1820655118600314
    0.15011551811154433
    0.6332636367329272
    0.350750685248574
    0.36470270170103447
    0.6618966386081091
    0.48874357941505053
    0.0011582543925534594
    0.21967867933233265
    0.9782214613123124
    0.5372888107177474
    0.9271996682883703
    0.7655290760007034
    0.02752716604083716
    0.8270142571116068
    0.05829965353308708
    0.23118033572851004
    0.8941829733208344
    0.5022139779408761
    0.5535477973080738
    0.5900720342744279
    0.33843011554634117
    0.12020057075894641
    0.4553935542338401
    0.4274064732034617
    0.3291219801060201
    0.3460827429245583
    0.1745495567956148
    0.35636135877389563
    0.21534326855265462
    0.7279139203235943
    0.8062226770478027
    0.4668664205148332
    0.35523631398053723
    0.8484901315129676
    0.6221717123542767
    0.8884490296902499
    0.7915304539038424
    0.5647825894210072
}

set y {
    0.12490359608000645
    0.04514052006149427
    0.11398898557866843
    0.9624231984807423
    0.1837378371393079
    0.38504318286139494
    0.7459345207519981
    0.13507378780658286
    0.24823291366047684
    0.9858802650712732
    0.43864597754308576
    0.32744724861320407
    0.6679520511834092
    0.7527283114266226
    0.22794089194257694
    0.08735701729903411
    0.6708519924148426
    0.26826928614084977
    0.9237067292619479
    0.8263475248395551
    0.8801524872632349
    0.6510347247135968
    0.28388870208405237
    0.17121814096403654
    0.2555430060458299
    0.10695133992550154
    0.01592734209416946
    0.5576725251935741
    0.9067335442303381
    0.088845913211415
    0.04395001165886114
    0.12921351146467686
    0.3600048550912369
    0.7790471915201067
    0.9881878078656534
    0.2833076142442863
    0.12417959045952287
    0.8892790588407564
    0.06986253561350608
    0.8925465216300026
    0.353077461824828
    0.17127402330504538
    0.11473791975224046
    0.18919959742343195
    0.16691746691013876
    0.2081756176584264
    0.11935030170854688
    0.04597051206496161
    0.733296272981935
    0.9336942154351675
    0.05644703600689738
    0.3133567241249082
    0.5554368200823951
    0.3416095798690293
    0.5243109790956417
    0.5248017022588769
    0.9356229175753903
    0.20667674992495222
    0.23550715942176836
    0.5854905746543082
    0.2174968683843197
    0.5590161903468953
    0.6443482462622647
    0.5904593815205814
    0.9327057795674172
    0.6230060527829835
    0.6323591079157147
    0.13458034860668278
    0.23387101792498655
    0.9495204753713864
    0.7136728799847241
    0.9834896585683381
    0.16378205766310217
    0.30367465114886727
    0.7471818641379677
    0.7638041347506501
    0.14496355624963897
    0.30127148377870583
    0.412091559430106
    0.03681813110267029
    0.15734069699107422
    0.863747584885715
    0.43410550447852003
    0.26669580499697787
    0.0657654650323316
    0.8470647132333369
    0.6597476920477696
    0.44701589919022666
    0.4529233899370162
    0.023788074581318597
    0.93837842037588
    0.5640502375297629
    0.6739992052417172
    0.3042617548222353
    0.46047388039101733
    0.34765240878680714
    0.07294865589563315
    0.28268551041114165
    0.7450398573128147
    0.47801363667833385
}
xVector set $x
yVector set $y

test palette.59 {palette configure myPalette -x xVector -y yVector} {
    list [catch {blt::palette configure myPalette -x xVector -y yVector} msg] $msg
} {0 {}}

test palette.60 {palette triangles myPalette} {
    list [catch {blt::palette triangles myPalette} msg] $msg
} {1 {a command "::if" already exists}}

test palette.61 {palette create (bad namespace)} {
    list [catch {blt::palette create badName::fred} msg] $msg
} {1 {unknown namespace "badName"}}

test palette.62 {palette create (wrong # args)} {
    list [catch {blt::palette create a b} msg] $msg
} {1 {wrong # args: should be "blt::palette create ?name?"}}

test palette.63 {palette names} {
    list [catch {blt::palette names} msg] [lsort $msg]
} {0 {::fred ::palette0 ::palette1}}

test palette.64 {palette names pattern)} {
    list [catch {blt::palette names ::palette*} msg] [lsort $msg]
} {0 {::palette0 ::palette1}}

test palette.65 {palette names badPattern)} {
    list [catch {blt::palette names badPattern*} msg] $msg
} {0 {}}

test palette.66 {palette names pattern arg (wrong # args)} {
    list [catch {blt::palette names pattern arg} msg] $msg
} {1 {wrong # args: should be "blt::palette names ?pattern?..."}}

test palette.67 {palette destroy (wrong # args)} {
    list [catch {blt::palette destroy} msg] $msg
} {1 {wrong # args: should be "blt::palette destroy name..."}}

test palette.68 {palette destroy badPalette} {
    list [catch {blt::palette destroy badPalette} msg] $msg
} {1 {can't find a palette named "badPalette"}}

test palette.69 {palette destroy fred} {
    list [catch {blt::palette destroy fred} msg] $msg
} {0 {}}

test palette.70 {palette destroy palette0 palette1} {
    list [catch {blt::palette destroy palette0 palette1} msg] $msg
} {0 {}}

test palette.71 {create} {
    list [catch {blt::palette create} msg] $msg
} {0 ::palette0}

exit 0
test palette.72 {palette0} {
    list [catch {palette0} msg] $msg
} {1 {wrong # args: should be one of...
  palette0 ancestor node1 node2
  palette0 apply node ?switches?
  palette0 attach palette ?switches?
  palette0 children node ?first? ?last?
  palette0 copy parent ?palette? node ?switches?
  palette0 degree node
  palette0 delete node ?node...?
  palette0 depth node
  palette0 dump node
  palette0 dumpfile node fileName
  palette0 exists node ?key?
  palette0 export format ?switches?
  palette0 find node ?switches?
  palette0 findchild node label
  palette0 firstchild node
  palette0 get node ?key? ?defaultValue?
  palette0 import format ?switches?
  palette0 index label|list
  palette0 insert parent ?switches?
  palette0 is oper args...
  palette0 keys node ?node...?
  palette0 label node ?newLabel?
  palette0 lastchild node
  palette0 move node newParent ?switches?
  palette0 next node
  palette0 nextsibling node
  palette0 notify args...
  palette0 parent node
  palette0 parsepath node string ?separator?
  palette0 path node
  palette0 position ?switches? node...
  palette0 previous node
  palette0 prevsibling node
  palette0 restore node data ?switches?
  palette0 restorefile node fileName ?switches?
  palette0 root 
  palette0 set node ?key value...?
  palette0 size node
  palette0 sort node ?flags...?
  palette0 tag args...
  palette0 trace args...
  palette0 type node key
  palette0 unset node ?key...?
  palette0 values node ?key?}}

test palette.73 {palette0 badOp} {
    list [catch {palette0 badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  palette0 ancestor node1 node2
  palette0 apply node ?switches?
  palette0 attach palette ?switches?
  palette0 children node ?first? ?last?
  palette0 copy parent ?palette? node ?switches?
  palette0 degree node
  palette0 delete node ?node...?
  palette0 depth node
  palette0 dump node
  palette0 dumpfile node fileName
  palette0 exists node ?key?
  palette0 export format ?switches?
  palette0 find node ?switches?
  palette0 findchild node label
  palette0 firstchild node
  palette0 get node ?key? ?defaultValue?
  palette0 import format ?switches?
  palette0 index label|list
  palette0 insert parent ?switches?
  palette0 is oper args...
  palette0 keys node ?node...?
  palette0 label node ?newLabel?
  palette0 lastchild node
  palette0 move node newParent ?switches?
  palette0 next node
  palette0 nextsibling node
  palette0 notify args...
  palette0 parent node
  palette0 parsepath node string ?separator?
  palette0 path node
  palette0 position ?switches? node...
  palette0 previous node
  palette0 prevsibling node
  palette0 restore node data ?switches?
  palette0 restorefile node fileName ?switches?
  palette0 root 
  palette0 set node ?key value...?
  palette0 size node
  palette0 sort node ?flags...?
  palette0 tag args...
  palette0 trace args...
  palette0 type node key
  palette0 unset node ?key...?
  palette0 values node ?key?}}

test palette.74 {palette0 insert (wrong # args)} {
    list [catch {palette0 insert} msg] $msg
} {1 {wrong # args: should be "palette0 insert parent ?switches?"}}

test palette.75 {palette0 insert badParent} {
    list [catch {palette0 insert badParent} msg] $msg
} {1 {can't find tag or id "badParent" in ::palette0}}

test palette.76 {palette0 insert 1000} {
    list [catch {palette0 insert 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::palette0}}

test palette.77 {palette0 insert 0} {
    list [catch {palette0 insert 0} msg] $msg
} {0 1}

test palette.78 {palette0 insert 0} {
    list [catch {palette0 insert 0} msg] $msg
} {0 2}

test palette.79 {palette0 insert root} {
    list [catch {palette0 insert root} msg] $msg
} {0 3}

test palette.80 {palette0 insert all} {
    list [catch {palette0 insert all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.81 {palette0 insert 0 -at badPosition} {
    list [catch {palette0 insert 0 -at badPosition} msg] $msg
} {1 {expected integer but got "badPosition"}}

test palette.82 {palette0 insert 0 -at -1} {
    list [catch {palette0 insert 0 -at -1} msg] $msg
} {1 {bad value "-1": can't be negative}}

test palette.83 {palette0 insert 0 -at 1000} {
    list [catch {palette0 insert 0 -at 1000} msg] $msg
} {0 4}

test palette.84 {palette0 insert 0 -at (no arg)} {
    list [catch {palette0 insert 0 -at} msg] $msg
} {1 {value for "-at" missing}}

test palette.85 {palette0 insert 0 -tags myTag} {
    list [catch {palette0 insert 0 -tags myTag} msg] $msg
} {0 5}

test palette.86 {palette0 insert 0 -tags {myTag1 myTag2} } {
    list [catch {palette0 insert 0 -tags {myTag1 myTag2}} msg] $msg
} {0 6}

test palette.87 {palette0 insert 0 -tags root} {
    list [catch {palette0 insert 0 -tags root} msg] $msg
} {1 {can't add reserved tag "root"}}

test palette.88 {palette0 insert 0 -tags (missing arg)} {
    list [catch {palette0 insert 0 -tags} msg] $msg
} {1 {value for "-tags" missing}}

test palette.89 {palette0 insert 0 -label myLabel -tags thisTag} {
    list [catch {palette0 insert 0 -label myLabel -tags thisTag} msg] $msg
} {0 8}

test palette.90 {palette0 insert 0 -label (missing arg)} {
    list [catch {palette0 insert 0 -label} msg] $msg
} {1 {value for "-label" missing}}

test palette.91 {palette0 insert 1 -tags thisTag} {
    list [catch {palette0 insert 1 -tags thisTag} msg] $msg
} {0 9}

test palette.92 {palette0 insert 1 -data key (missing value)} {
    list [catch {palette0 insert 1 -data key} msg] $msg
} {1 {missing value for "key"}}

test palette.93 {palette0 insert 1 -data {key value}} {
    list [catch {palette0 insert 1 -data {key value}} msg] $msg
} {0 11}

test palette.94 {palette0 insert 1 -data {key1 value1 key2 value2}} {
    list [catch {palette0 insert 1 -data {key1 value1 key2 value2}} msg] $msg
} {0 12}

test palette.95 {get} {
    list [catch {
	palette0 get 12
    } msg] $msg
} {0 {key1 value1 key2 value2}}

test palette.96 {palette0 children} {
    list [catch {palette0 children} msg] $msg
} {1 {wrong # args: should be "palette0 children node ?first? ?last?"}}

test palette.97 {palette0 children 0} {
    list [catch {palette0 children 0} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test palette.98 {palette0 children root} {
    list [catch {palette0 children root} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test palette.99 {palette0 children 1} {
    list [catch {palette0 children 1} msg] $msg
} {0 {9 11 12}}

test palette.100 {palette0 insert myTag} {
    list [catch {palette0 insert myTag} msg] $msg
} {0 13}

test palette.101 {palette0 children myTag} {
    list [catch {palette0 children myTag} msg] $msg
} {0 13}

test palette.102 {palette0 children root 0 end} {
    list [catch {palette0 children root 0 end} msg] $msg
} {0 {1 2 3 4 5 6 8}}

test palette.103 {palette0 children root 2} {
    list [catch {palette0 children root 2} msg] $msg
} {0 3}

test palette.104 {palette0 children root 2 end} {
    list [catch {palette0 children root 2 end} msg] $msg
} {0 {3 4 5 6 8}}

test palette.105 {palette0 children root end end} {
    list [catch {palette0 children root end end} msg] $msg
} {0 8}

test palette.106 {palette0 children root 0 2} {
    list [catch {palette0 children root 0 2} msg] $msg
} {0 {1 2 3}}

test palette.107 {palette0 children root -1 -20} {
    list [catch {palette0 children root -1 -20} msg] $msg
} {0 {}}

test palette.108 {palette0 firstchild (missing arg)} {
    list [catch {palette0 firstchild} msg] $msg
} {1 {wrong # args: should be "palette0 firstchild node"}}

test palette.109 {palette0 firstchild root} {
    list [catch {palette0 firstchild root} msg] $msg
} {0 1}

test palette.110 {palette0 lastchild (missing arg)} {
    list [catch {palette0 lastchild} msg] $msg
} {1 {wrong # args: should be "palette0 lastchild node"}}

test palette.111 {palette0 lastchild root} {
    list [catch {palette0 lastchild root} msg] $msg
} {0 8}

test palette.112 {palette0 nextsibling (missing arg)} {
    list [catch {palette0 nextsibling} msg] $msg
} {1 {wrong # args: should be "palette0 nextsibling node"}}

test palette.113 {palette0 nextsibling 1)} {
    list [catch {palette0 nextsibling 1} msg] $msg
} {0 2}

test palette.114 {palette0 nextsibling 2)} {
    list [catch {palette0 nextsibling 2} msg] $msg
} {0 3}

test palette.115 {palette0 nextsibling 3)} {
    list [catch {palette0 nextsibling 3} msg] $msg
} {0 4}

test palette.116 {palette0 nextsibling 4)} {
    list [catch {palette0 nextsibling 4} msg] $msg
} {0 5}

test palette.117 {palette0 nextsibling 5)} {
    list [catch {palette0 nextsibling 5} msg] $msg
} {0 6}

test palette.118 {palette0 nextsibling 6)} {
    list [catch {palette0 nextsibling 6} msg] $msg
} {0 8}

test palette.119 {palette0 nextsibling 8)} {
    list [catch {palette0 nextsibling 8} msg] $msg
} {0 -1}

test palette.120 {palette0 nextsibling all)} {
    list [catch {palette0 nextsibling all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.121 {palette0 nextsibling badTag)} {
    list [catch {palette0 nextsibling badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::palette0}}

test palette.122 {palette0 nextsibling -1)} {
    list [catch {palette0 nextsibling -1} msg] $msg
} {1 {can't find tag or id "-1" in ::palette0}}

test palette.123 {palette0 prevsibling 2)} {
    list [catch {palette0 prevsibling 2} msg] $msg
} {0 1}

test palette.124 {palette0 prevsibling 1)} {
    list [catch {palette0 prevsibling 1} msg] $msg
} {0 -1}

test palette.125 {palette0 prevsibling -1)} {
    list [catch {palette0 prevsibling -1} msg] $msg
} {1 {can't find tag or id "-1" in ::palette0}}

test palette.126 {palette0 root)} {
    list [catch {palette0 root} msg] $msg
} {0 0}

test palette.127 {palette0 root badArg)} {
    list [catch {palette0 root badArgs} msg] $msg
} {1 {wrong # args: should be "palette0 root "}}

test palette.128 {palette0 parent (missing arg))} {
    list [catch {palette0 parent} msg] $msg
} {1 {wrong # args: should be "palette0 parent node"}}

test palette.129 {palette0 parent root)} {
    list [catch {palette0 parent root} msg] $msg
} {0 -1}

test palette.130 {palette0 parent 1)} {
    list [catch {palette0 parent 1} msg] $msg
} {0 0}

test palette.131 {palette0 parent myTag)} {
    list [catch {palette0 parent myTag} msg] $msg
} {0 0}

test palette.132 {palette0 next (missing arg))} {
    list [catch {palette0 next} msg] $msg
} {1 {wrong # args: should be "palette0 next node"}}


test palette.133 {palette0 next (extra arg))} {
    list [catch {palette0 next root root} msg] $msg
} {1 {wrong # args: should be "palette0 next node"}}

test palette.134 {palette0 next root} {
    list [catch {palette0 next root} msg] $msg
} {0 1}

test palette.135 {palette0 next 1)} {
    list [catch {palette0 next 1} msg] $msg
} {0 9}

test palette.136 {palette0 next 2)} {
    list [catch {palette0 next 2} msg] $msg
} {0 3}

test palette.137 {palette0 next 3)} {
    list [catch {palette0 next 3} msg] $msg
} {0 4}

test palette.138 {palette0 next 4)} {
    list [catch {palette0 next 4} msg] $msg
} {0 5}

test palette.139 {palette0 next 5)} {
    list [catch {palette0 next 5} msg] $msg
} {0 13}

test palette.140 {palette0 next 6)} {
    list [catch {palette0 next 6} msg] $msg
} {0 8}

test palette.141 {palette0 next 8)} {
    list [catch {palette0 next 8} msg] $msg
} {0 -1}

test palette.142 {palette0 previous 1)} {
    list [catch {palette0 previous 1} msg] $msg
} {0 0}

test palette.143 {palette0 previous 0)} {
    list [catch {palette0 previous 0} msg] $msg
} {0 -1}

test palette.144 {palette0 previous 8)} {
    list [catch {palette0 previous 8} msg] $msg
} {0 6}

test palette.145 {palette0 depth (no arg))} {
    list [catch {palette0 depth} msg] $msg
} {1 {wrong # args: should be "palette0 depth node"}}

test palette.146 {palette0 depth root))} {
    list [catch {palette0 depth root} msg] $msg
} {0 0}

test palette.147 {palette0 depth myTag))} {
    list [catch {palette0 depth myTag} msg] $msg
} {0 1}

test palette.148 {palette0 depth myTag))} {
    list [catch {palette0 depth myTag} msg] $msg
} {0 1}

test palette.149 {palette0 dump (missing arg)))} {
    list [catch {palette0 dump} msg] $msg
} {1 {wrong # args: should be "palette0 dump node"}}

test palette.150 {palette0 dump root} {
    list [catch {palette0 dump root} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test palette.151 {palette0 dump 1} {
    list [catch {palette0 dump 1} msg] $msg
} {0 {-1 1 {node1} {} {}
1 9 {node1 node9} {} {thisTag}
1 11 {node1 node11} {key value} {}
1 12 {node1 node12} {key1 value1 key2 value2} {}
}}

test palette.152 {palette0 dump this} {
    list [catch {palette0 dump myTag} msg] $msg
} {0 {-1 5 {node5} {} {myTag}
5 13 {node5 node13} {} {}
}}

test palette.153 {palette0 dump 1 badArg (too many args)} {
    list [catch {palette0 dump 1 badArg} msg] $msg
} {1 {wrong # args: should be "palette0 dump node"}}

test palette.154 {palette0 dump 11} {
    list [catch {palette0 dump 11} msg] $msg
} {0 {-1 11 {node11} {key value} {}
}}

test palette.155 {palette0 dump all} {
    list [catch {palette0 dump all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.156 {palette0 dump all} {
    list [catch {palette0 dump all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.157 {palette0 dumpfile 0 test.dump} {
    list [catch {palette0 dumpfile 0 test.dump} msg] $msg
} {0 {}}

test palette.158 {palette0 get 9} {
    list [catch {palette0 get 9} msg] $msg
} {0 {}}

test palette.159 {palette0 get all} {
    list [catch {palette0 get all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.160 {palette0 get root} {
    list [catch {palette0 get root} msg] $msg
} {0 {}}

test palette.161 {palette0 get 9 key} {
    list [catch {palette0 get root} msg] $msg
} {0 {}}

test palette.162 {palette0 get 12} {
    list [catch {palette0 get 12} msg] $msg
} {0 {key1 value1 key2 value2}}

test palette.163 {palette0 get 12 key1} {
    list [catch {palette0 get 12 key1} msg] $msg
} {0 value1}

test palette.164 {palette0 get 12 key2} {
    list [catch {palette0 get 12 key2} msg] $msg
} {0 value2}

test palette.165 {palette0 get 12 key1 defValue } {
    list [catch {palette0 get 12 key1 defValue} msg] $msg
} {0 value1}

test palette.166 {palette0 get 12 key100 defValue } {
    list [catch {palette0 get 12 key100 defValue} msg] $msg
} {0 defValue}

test palette.167 {palette0 index (missing arg) } {
    list [catch {palette0 index} msg] $msg
} {1 {wrong # args: should be "palette0 index label|list"}}

test palette.168 {palette0 index 0 10 (extra arg) } {
    list [catch {palette0 index 0 10} msg] $msg
} {1 {wrong # args: should be "palette0 index label|list"}}

test palette.169 {palette0 index 0} {
    list [catch {palette0 index 0} msg] $msg
} {0 0}

test palette.170 {palette0 index root} {
    list [catch {palette0 index root} msg] $msg
} {0 0}

test palette.171 {palette0 index all} {
    list [catch {palette0 index all} msg] $msg
} {0 -1}

test palette.172 {palette0 index myTag} {
    list [catch {palette0 index myTag} msg] $msg
} {0 5}

test palette.173 {palette0 index thisTag} {
    list [catch {palette0 index thisTag} msg] $msg
} {0 -1}

test palette.174 {palette0 is (no args)} {
    list [catch {palette0 is} msg] $msg
} {1 {wrong # args: should be one of...
  palette0 is ancestor node1 node2
  palette0 is before node1 node2
  palette0 is leaf node
  palette0 is root node}}

test palette.175 {palette0 is badOp} {
    list [catch {palette0 is badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  palette0 is ancestor node1 node2
  palette0 is before node1 node2
  palette0 is leaf node
  palette0 is root node}}

test palette.176 {palette0 is before} {
    list [catch {palette0 is before} msg] $msg
} {1 {wrong # args: should be "palette0 is before node1 node2"}}

test palette.177 {palette0 is before 0 10 20} {
    list [catch {palette0 is before 0 10 20} msg] $msg
} {1 {wrong # args: should be "palette0 is before node1 node2"}}

test palette.178 {palette0 is before 0 12} {
    list [catch {palette0 is before 0 12} msg] $msg
} {0 1}

test palette.179 {palette0 is before 12 0} {
    list [catch {palette0 is before 12 0} msg] $msg
} {0 0}

test palette.180 {palette0 is before 0 0} {
    list [catch {palette0 is before 0 0} msg] $msg
} {0 0}

test palette.181 {palette0 is before root 0} {
    list [catch {palette0 is before root 0} msg] $msg
} {0 0}

test palette.182 {palette0 is before 0 all} {
    list [catch {palette0 is before 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.183 {palette0 is ancestor} {
    list [catch {palette0 is ancestor} msg] $msg
} {1 {wrong # args: should be "palette0 is ancestor node1 node2"}}

test palette.184 {palette0 is ancestor 0 12 20} {
    list [catch {palette0 is ancestor 0 12 20} msg] $msg
} {1 {wrong # args: should be "palette0 is ancestor node1 node2"}}

test palette.185 {palette0 is ancestor 0 12} {
    list [catch {palette0 is ancestor 0 12} msg] $msg
} {0 1}

test palette.186 {palette0 is ancestor 12 0} {
    list [catch {palette0 is ancestor 12 0} msg] $msg
} {0 0}

test palette.187 {palette0 is ancestor 1 2} {
    list [catch {palette0 is ancestor 1 2} msg] $msg
} {0 0}

test palette.188 {palette0 is ancestor root 0} {
    list [catch {palette0 is ancestor root 0} msg] $msg
} {0 0}

test palette.189 {palette0 is ancestor 0 all} {
    list [catch {palette0 is ancestor 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.190 {palette0 is root (missing arg)} {
    list [catch {palette0 is root} msg] $msg
} {1 {wrong # args: should be "palette0 is root node"}}

test palette.191 {palette0 is root 0 20 (extra arg)} {
    list [catch {palette0 is root 0 20} msg] $msg
} {1 {wrong # args: should be "palette0 is root node"}}

test palette.192 {palette0 is root 0} {
    list [catch {palette0 is root 0} msg] $msg
} {0 1}

test palette.193 {palette0 is root 12} {
    list [catch {palette0 is root 12} msg] $msg
} {0 0}

test palette.194 {palette0 is root 1} {
    list [catch {palette0 is root 1} msg] $msg
} {0 0}

test palette.195 {palette0 is root root} {
    list [catch {palette0 is root root} msg] $msg
} {0 1}

test palette.196 {palette0 is root all} {
    list [catch {palette0 is root all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.197 {palette0 is leaf (missing arg)} {
    list [catch {palette0 is leaf} msg] $msg
} {1 {wrong # args: should be "palette0 is leaf node"}}

test palette.198 {palette0 is leaf 0 20 (extra arg)} {
    list [catch {palette0 is leaf 0 20} msg] $msg
} {1 {wrong # args: should be "palette0 is leaf node"}}

test palette.199 {palette0 is leaf 0} {
    list [catch {palette0 is leaf 0} msg] $msg
} {0 0}

test palette.200 {palette0 is leaf 12} {
    list [catch {palette0 is leaf 12} msg] $msg
} {0 1}

test palette.201 {palette0 is leaf 1} {
    list [catch {palette0 is leaf 1} msg] $msg
} {0 0}

test palette.202 {palette0 is leaf root} {
    list [catch {palette0 is leaf root} msg] $msg
} {0 0}

test palette.203 {palette0 is leaf all} {
    list [catch {palette0 is leaf all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.204 {palette0 is leaf 1000} {
    list [catch {palette0 is leaf 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::palette0}}

test palette.205 {palette0 is leaf badTag} {
    list [catch {palette0 is leaf badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::palette0}}

test palette.206 {palette0 set (missing arg)} {
    list [catch {palette0 set} msg] $msg
} {1 {wrong # args: should be "palette0 set node ?key value...?"}}

test palette.207 {palette0 set 0 (missing arg)} {
    list [catch {palette0 set 0} msg] $msg
} {0 {}}

test palette.208 {palette0 set 0 key (missing arg)} {
    list [catch {palette0 set 0 key} msg] $msg
} {1 {missing value for field "key"}}

test palette.209 {palette0 set 0 key value} {
    list [catch {palette0 set 0 key value} msg] $msg
} {0 {}}

test palette.210 {palette0 set 0 key1 value1 key2 value2 key3 value3} {
    list [catch {palette0 set 0 key1 value1 key2 value2 key3 value3} msg] $msg
} {0 {}}

test palette.211 {palette0 set 0 key1 value1 key2 (missing arg)} {
    list [catch {palette0 set 0 key1 value1 key2} msg] $msg
} {1 {missing value for field "key2"}}

test palette.212 {palette0 set 0 key value} {
    list [catch {palette0 set 0 key value} msg] $msg
} {0 {}}

test palette.213 {palette0 set 0 key1 value1 key2 (missing arg)} {
    list [catch {palette0 set 0 key1 value1 key2} msg] $msg
} {1 {missing value for field "key2"}}

test palette.214 {palette0 set all} {
    list [catch {palette0 set all} msg] $msg
} {0 {}}

test palette.215 {palette0 set all abc 123} {
    list [catch {palette0 set all abc 123} msg] $msg
} {0 {}}

test palette.216 {palette0 set root} {
    list [catch {palette0 set root} msg] $msg
} {0 {}}

test palette.217 {palette0 restore stuff} {
    list [catch {
	set data [palette0 dump root]
	blt::palette create
	palette1 restore root $data
	set data [palette1 dump root]
	blt::palette destroy palette1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {key value key1 value1 key2 value2 key3 value3 abc 123} {}
0 1 {{} node1} {abc 123} {}
1 9 {{} node1 node9} {abc 123} {thisTag}
1 11 {{} node1 node11} {key value abc 123} {}
1 12 {{} node1 node12} {key1 value1 key2 value2 abc 123} {}
0 2 {{} node2} {abc 123} {}
0 3 {{} node3} {abc 123} {}
0 4 {{} node4} {abc 123} {}
0 5 {{} node5} {abc 123} {myTag}
5 13 {{} node5 node13} {abc 123} {}
0 6 {{} node6} {abc 123} {myTag2 myTag1}
0 8 {{} myLabel} {abc 123} {thisTag}
}}

test palette.218 {palette0 restorefile 0 test.dump} {
    list [catch {
	blt::palette create
	palette1 restorefile root test.dump
	set data [palette1 dump root]
	blt::palette destroy palette1
	file delete test.dump
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}


test palette.219 {palette0 unset 0 key1} {
    list [catch {palette0 unset 0 key1} msg] $msg
} {0 {}}

test palette.220 {palette0 get 0} {
    list [catch {palette0 get 0} msg] $msg
} {0 {key value key2 value2 key3 value3 abc 123}}

test palette.221 {palette0 unset 0 key2 key3} {
    list [catch {palette0 unset 0 key2 key3} msg] $msg
} {0 {}}

test palette.222 {palette0 get 0} {
    list [catch {palette0 get 0} msg] $msg
} {0 {key value abc 123}}

test palette.223 {palette0 unset 0} {
    list [catch {palette0 unset 0} msg] $msg
} {0 {}}

test palette.224 {palette0 get 0} {
    list [catch {palette0 get 0} msg] $msg
} {0 {}}

test palette.225 {palette0 unset all abc} {
    list [catch {palette0 unset all abc} msg] $msg
} {0 {}}

test palette.226 {palette0 restore stuff} {
    list [catch {
	set data [palette0 dump root]
	blt::palette create palette1
	palette1 restore root $data
	set data [palette1 dump root]
	blt::palette destroy palette1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
1 9 {{} node1 node9} {} {thisTag}
1 11 {{} node1 node11} {key value} {}
1 12 {{} node1 node12} {key1 value1 key2 value2} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test palette.227 {palette0 restore (missing arg)} {
    list [catch {palette0 restore} msg] $msg
} {1 {wrong # args: should be "palette0 restore node data ?switches?"}}

test palette.228 {palette0 restore 0 badString} {
    list [catch {palette0 restore 0 badString} msg] $msg
} {1 {line #1: wrong # elements in restore entry}}

test palette.229 {palette0 restore 0 {} arg (extra arg)} {
    list [catch {palette0 restore 0 {} arg} msg] $msg
} {1 {unknown switch "arg"
following switches are available:
   -notags 
   -overwrite }}


test palette.230 {palette0 size (missing arg)} {
    list [catch {palette0 size} msg] $msg
} {1 {wrong # args: should be "palette0 size node"}}

test palette.231 {palette0 size 0} {
    list [catch {palette0 size 0} msg] $msg
} {0 12}

test palette.232 {palette0 size all} {
    list [catch {palette0 size all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.233 {palette0 size 0 10 (extra arg)} {
    list [catch {palette0 size 0 10} msg] $msg
} {1 {wrong # args: should be "palette0 size node"}}

test palette.234 {palette0 delete (no args)} {
    list [catch {palette0 delete} msg] $msg
} {0 {}}

test palette.235 {palette0 delete 11} {
    list [catch {palette0 delete 11} msg] $msg
} {0 {}}

test palette.236 {palette0 delete 11} {
    list [catch {palette0 delete 11} msg] $msg
} {1 {can't find tag or id "11" in ::palette0}}

test palette.237 {palette0 delete 9 12} {
    list [catch {palette0 delete 9 12} msg] $msg
} {0 {}}

test palette.238 {palette0 dump 0} {
    list [catch {palette0 dump 0} msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node1} {} {}
0 2 {{} node2} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {myTag}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {myTag2 myTag1}
0 8 {{} myLabel} {} {thisTag}
}}

test palette.239 {delete all} {
    list [catch {
	set data [palette0 dump root]
	blt::palette create
	palette1 restore root $data
	palette1 delete all
	set data [palette1 dump root]
	blt::palette destroy palette1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
}}

test palette.240 {delete all all} {
    list [catch {
	set data [palette0 dump root]
	blt::palette create
	palette1 restore root $data
	palette1 delete all all
	set data [palette1 dump root]
	blt::palette destroy palette1
	set data
	} msg] $msg
} {0 {-1 0 {{}} {} {}
}}

test palette.241 {palette0 apply (missing arg)} {
    list [catch {palette0 apply} msg] $msg
} {1 {wrong # args: should be "palette0 apply node ?switches?"}}

test palette.242 {palette0 apply 0} {
    list [catch {palette0 apply 0} msg] $msg
} {0 {}}

test palette.243 {palette0 apply 0 -badSwitch} {
    list [catch {palette0 apply 0 -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -precommand command
   -postcommand command
   -depth number
   -exact string
   -glob pattern
   -invert 
   -key pattern
   -keyexact string
   -keyglob pattern
   -keyregexp pattern
   -leafonly 
   -nocase 
   -path 
   -regexp pattern
   -tag {?tag?...}}}


test palette.244 {palette0 apply badTag} {
    list [catch {palette0 apply badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::palette0}}

test palette.245 {palette0 apply all} {
    list [catch {palette0 apply all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.246 {palette0 apply myTag -precommand lappend} {
    list [catch {
	set mylist {}
	palette0 apply myTag -precommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {5 13}}

test palette.247 {palette0 apply root -precommand lappend} {
    list [catch {
	set mylist {}
	palette0 apply root -precommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {0 1 2 3 4 5 13 6 8}}

test palette.248 {palette0 apply -precommand -postcommand} {
    list [catch {
	set mylist {}
	palette0 apply root -precommand {lappend mylist} \
		-postcommand {lappend mylist}
	set mylist
    } msg] $msg
} {0 {0 1 1 2 2 3 3 4 4 5 13 13 5 6 6 8 8 0}}

test palette.249 {palette0 apply root -precommand lappend -depth 1} {
    list [catch {
	set mylist {}
	palette0 apply root -precommand {lappend mylist} -depth 1
	set mylist
    } msg] $msg
} {0 {0 1 2 3 4 5 6 8}}


test palette.250 {palette0 apply root -precommand -depth 0} {
    list [catch {
	set mylist {}
	palette0 apply root -precommand {lappend mylist} -depth 0
	set mylist
    } msg] $msg
} {0 0}

test palette.251 {palette0 apply root -precommand -tag myTag} {
    list [catch {
	set mylist {}
	palette0 apply root -precommand {lappend mylist} -tag myTag
	set mylist
    } msg] $msg
} {0 5}


test palette.252 {palette0 apply root -precommand -key key1} {
    list [catch {
	set mylist {}
	palette0 set myTag key1 0.0
	palette0 apply root -precommand {lappend mylist} -key key1
	palette0 unset myTag key1
	set mylist
    } msg] $msg
} {0 5}

test palette.253 {palette0 apply root -postcommand -regexp node.*} {
    list [catch {
	set mylist {}
	palette0 set myTag key1 0.0
	palette0 apply root -precommand {lappend mylist} -regexp {node5} 
	palette0 unset myTag key1
	set mylist
    } msg] $msg
} {0 5}

test palette.254 {palette0 find (missing arg)} {
    list [catch {palette0 find} msg] $msg
} {1 {wrong # args: should be "palette0 find node ?switches?"}}

test palette.255 {palette0 find 0} {
    list [catch {palette0 find 0} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test palette.256 {palette0 find root} {
    list [catch {palette0 find root} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test palette.257 {palette0 find 0 -glob node*} {
    list [catch {palette0 find root -glob node*} msg] $msg
} {0 {1 2 3 4 13 5 6}}

test palette.258 {palette0 find 0 -glob nobody} {
    list [catch {palette0 find root -glob nobody} msg] $msg
} {0 {}}

test palette.259 {palette0 find 0 -regexp {node[0-3]}} {
    list [catch {palette0 find root -regexp {node[0-3]}} msg] $msg
} {0 {1 2 3 13}}

test palette.260 {palette0 find 0 -regexp {.*[A-Z].*}} {
    list [catch {palette0 find root -regexp {.*[A-Z].*}} msg] $msg
} {0 8}

test palette.261 {palette0 find 0 -exact myLabel} {
    list [catch {palette0 find root -exact myLabel} msg] $msg
} {0 8}

test palette.262 {palette0 find 0 -exact myLabel -invert} {
    list [catch {palette0 find root -exact myLabel -invert} msg] $msg
} {0 {1 2 3 4 13 5 6 0}}


test palette.263 {palette0 find 3 -exact node3} {
    list [catch {palette0 find 3 -exact node3} msg] $msg
} {0 3}

test palette.264 {palette0 find 0 -nocase -exact mylabel} {
    list [catch {palette0 find 0 -nocase -exact mylabel} msg] $msg
} {0 8}

test palette.265 {palette0 find 0 -nocase} {
    list [catch {palette0 find 0 -nocase} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test palette.266 {palette0 find 0 -path -nocase -glob *node1* } {
    list [catch {palette0 find 0 -path -nocase -glob *node1*} msg] $msg
} {0 {1 13}}

test palette.267 {palette0 find 0 -count 5 } {
    list [catch {palette0 find 0 -count 5} msg] $msg
} {0 {1 2 3 4 13}}

test palette.268 {palette0 find 0 -count -5 } {
    list [catch {palette0 find 0 -count -5} msg] $msg
} {1 {bad value "-5": can't be negative}}

test palette.269 {palette0 find 0 -count badValue } {
    list [catch {palette0 find 0 -count badValue} msg] $msg
} {1 {expected integer but got "badValue"}}

test palette.270 {palette0 find 0 -count badValue } {
    list [catch {palette0 find 0 -count badValue} msg] $msg
} {1 {expected integer but got "badValue"}}

test palette.271 {palette0 find 0 -leafonly} {
    list [catch {palette0 find 0 -leafonly} msg] $msg
} {0 {1 2 3 4 13 6 8}}

test palette.272 {palette0 find 0 -leafonly -glob {node[18]}} {
    list [catch {palette0 find 0 -glob {node[18]} -leafonly} msg] $msg
} {0 1}

test palette.273 {palette0 find 0 -depth 0} {
    list [catch {palette0 find 0 -depth 0} msg] $msg
} {0 0}

test palette.274 {palette0 find 0 -depth 1} {
    list [catch {palette0 find 0 -depth 1} msg] $msg
} {0 {1 2 3 4 5 6 8 0}}

test palette.275 {palette0 find 0 -depth 2} {
    list [catch {palette0 find 0 -depth 2} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test palette.276 {palette0 find 0 -depth 20} {
    list [catch {palette0 find 0 -depth 20} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test palette.277 {palette0 find 1 -depth 0} {
    list [catch {palette0 find 1 -depth 0} msg] $msg
} {0 1}

test palette.278 {palette0 find 1 -depth 1} {
    list [catch {palette0 find 1 -depth 1} msg] $msg
} {0 1}

test palette.279 {palette0 find 1 -depth 2} {
    list [catch {palette0 find 1 -depth 2} msg] $msg
} {0 1}

test palette.280 {palette0 find all} {
    list [catch {palette0 find all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.281 {palette0 find badTag} {
    list [catch {palette0 find badTag} msg] $msg
} {1 {can't find tag or id "badTag" in ::palette0}}

test palette.282 {palette0 find 0 -addtag hi} {
    list [catch {palette0 find 0 -addtag hi} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test palette.283 {palette0 find 0 -addtag all} {
    list [catch {palette0 find 0 -addtag all} msg] $msg
} {0 {1 2 3 4 13 5 6 8 0}}

test palette.284 {palette0 find 0 -addtag root} {
    list [catch {palette0 find 0 -addtag root} msg] $msg
} {1 {can't add reserved tag "root"}}

test palette.285 {palette0 find 0 -exec {lappend list} -leafonly} {
    list [catch {
	set list {}
	palette0 find 0 -exec {lappend list} -leafonly
	set list
	} msg] $msg
} {0 {1 2 3 4 13 6 8}}

test palette.286 {palette0 find 0 -tag root} {
    list [catch {palette0 find 0 -tag root} msg] $msg
} {0 0}

test palette.287 {palette0 find 0 -tag myTag} {
    list [catch {palette0 find 0 -tag myTag} msg] $msg
} {0 5}

test palette.288 {palette0 find 0 -tag badTag} {
    list [catch {palette0 find 0 -tag badTag} msg] $msg
} {0 {}}

test palette.289 {palette0 tag (missing args)} {
    list [catch {palette0 tag} msg] $msg
} {1 {wrong # args: should be "palette0 tag args..."}}

test palette.290 {palette0 tag badOp} {
    list [catch {palette0 tag badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  palette0 tag add tag ?node...?
  palette0 tag delete tag node...
  palette0 tag dump tag...
  palette0 tag exists tag ?node?
  palette0 tag forget tag...
  palette0 tag get node ?pattern...?
  palette0 tag names ?node...?
  palette0 tag nodes tag ?tag...?
  palette0 tag set node tag...
  palette0 tag unset node tag...}}

test palette.291 {palette0 tag add} {
    list [catch {palette0 tag add} msg] $msg
} {1 {wrong # args: should be "palette0 tag add tag ?node...?"}}

test palette.292 {palette0 tag add newTag} {
    list [catch {palette0 tag add newTag} msg] $msg
} {0 {}}

test palette.293 {palette0 tag add tag badNode} {
    list [catch {palette0 tag add tag badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::palette0}}

test palette.294 {palette0 tag add newTag root} {
    list [catch {palette0 tag add newTag root} msg] $msg
} {0 {}}

test palette.295 {palette0 tag add newTag all} {
    list [catch {palette0 tag add newTag all} msg] $msg
} {0 {}}

test palette.296 {palette0 tag add tag2 0 1 2 3 4} {
    list [catch {palette0 tag add tag2 0 1 2 3 4} msg] $msg
} {0 {}}

test palette.297 {palette0 tag exists tag2} {
    list [catch {palette0 tag exists tag2} msg] $msg
} {0 1}

test palette.298 {palette0 tag exists tag2 0} {
    list [catch {palette0 tag exists tag2 0} msg] $msg
} {0 1}

test palette.299 {palette0 tag exists tag2 5} {
    list [catch {palette0 tag exists tag2 5} msg] $msg
} {0 0}

test palette.300 {palette0 tag exists badTag} {
    list [catch {palette0 tag exists badTag} msg] $msg
} {0 0}

test palette.301 {palette0 tag exists badTag 1000} {
    list [catch {palette0 tag exists badTag 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::palette0}}

test palette.302 {palette0 tag add tag2 0 1 2 3 4 1000} {
    list [catch {palette0 tag add tag2 0 1 2 3 4 1000} msg] $msg
} {1 {can't find tag or id "1000" in ::palette0}}

test palette.303 {palette0 tag names} {
    list [catch {palette0 tag names} msg] [lsort $msg]
} {0 {all hi myTag myTag1 myTag2 newTag root tag2 thisTag}}

test palette.304 {palette0 tag names badNode} {
    list [catch {palette0 tag names badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::palette0}}

test palette.305 {palette0 tag names all} {
    list [catch {palette0 tag names all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.306 {palette0 tag names root} {
    list [catch {palette0 tag names root} msg] [lsort $msg]
} {0 {all hi newTag root tag2}}

test palette.307 {palette0 tag names 0 1} {
    list [catch {palette0 tag names 0 1} msg] [lsort $msg]
} {0 {all hi newTag root tag2}}

test palette.308 {palette0 tag nodes (missing arg)} {
    list [catch {palette0 tag nodes} msg] $msg
} {1 {wrong # args: should be "palette0 tag nodes tag ?tag...?"}}

test palette.309 {palette0 tag nodes root badTag} {
    # It's not an error to use bad tag.
    list [catch {palette0 tag nodes root badTag} msg] $msg
} {0 {}}

test palette.310 {palette0 tag nodes root tag2} {
    list [catch {palette0 tag nodes root tag2} msg] [lsort $msg]
} {0 {0 1 2 3 4}}

test palette.311 {palette0 ancestor (missing arg)} {
    list [catch {palette0 ancestor} msg] $msg
} {1 {wrong # args: should be "palette0 ancestor node1 node2"}}

test palette.312 {palette0 ancestor 0 (missing arg)} {
    list [catch {palette0 ancestor 0} msg] $msg
} {1 {wrong # args: should be "palette0 ancestor node1 node2"}}

test palette.313 {palette0 ancestor 0 10} {
    list [catch {palette0 ancestor 0 10} msg] $msg
} {1 {can't find tag or id "10" in ::palette0}}

test palette.314 {palette0 ancestor 0 4} {
    list [catch {palette0 ancestor 0 4} msg] $msg
} {0 0}

test palette.315 {palette0 ancestor 1 8} {
    list [catch {palette0 ancestor 1 8} msg] $msg
} {0 0}

test palette.316 {palette0 ancestor root 0} {
    list [catch {palette0 ancestor root 0} msg] $msg
} {0 0}

test palette.317 {palette0 ancestor 8 8} {
    list [catch {palette0 ancestor 8 8} msg] $msg
} {0 8}

test palette.318 {palette0 ancestor 0 all} {
    list [catch {palette0 ancestor 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.319 {palette0 ancestor 7 9} {
    list [catch {
	set n1 1; set n2 1;
	for { set i 0 } { $i < 4 } { incr i } {
	    set n1 [palette0 insert $n1]
	    set n2 [palette0 insert $n2]
	}
	palette0 ancestor $n1 $n2
	} msg] $msg
} {0 1}

test palette.320 {palette0 path (missing arg)} {
    list [catch {palette0 path} msg] $msg
} {1 {wrong # args: should be "palette0 path node"}}

test palette.321 {palette0 path root} {
    list [catch {palette0 path root} msg] $msg
} {0 {}}

test palette.322 {palette0 path 0} {
    list [catch {palette0 path 0} msg] $msg
} {0 {}}

test palette.323 {palette0 path 15} {
    list [catch {palette0 path 15} msg] $msg
} {0 {node1 node15}}

test palette.324 {palette0 path all} {
    list [catch {palette0 path all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.325 {palette0 path 0 1 2 4 (extra args)} {
    list [catch {palette0 path 0 1 2 4} msg] $msg
} {1 {wrong # args: should be "palette0 path node"}}

test palette.326 {palette0 tag forget} {
    list [catch {palette0 tag forget} msg] $msg
} {1 {wrong # args: should be "palette0 tag forget tag..."}}

test palette.327 {palette0 tag forget badTag} {
    list [catch {
	palette0 tag forget badTag
	lsort [palette0 tag names]
    } msg] $msg
} {0 {all hi myTag myTag1 myTag2 newTag root tag2 thisTag}}

test palette.328 {palette0 tag forget hi} {
    list [catch {
	palette0 tag forget hi
	lsort [palette0 tag names]
    } msg] $msg
} {0 {all myTag myTag1 myTag2 newTag root tag2 thisTag}}

test palette.329 {palette0 tag forget tag1 tag2} {
    list [catch {
	palette0 tag forget myTag1 myTag2
	lsort [palette0 tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test palette.330 {palette0 tag forget all} {
    list [catch {
	palette0 tag forget all
	lsort [palette0 tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test palette.331 {palette0 tag forget root} {
    list [catch {
	palette0 tag forget root
	lsort [palette0 tag names]
    } msg] $msg
} {0 {all myTag newTag root tag2 thisTag}}

test palette.332 {palette0 tag delete} {
    list [catch {palette0 tag delete} msg] $msg
} {1 {wrong # args: should be "palette0 tag delete tag node..."}}

test palette.333 {palette0 tag delete tag} {
    list [catch {palette0 tag delete tag} msg] $msg
} {1 {wrong # args: should be "palette0 tag delete tag node..."}}

test palette.334 {palette0 tag delete tag 0} {
    list [catch {palette0 tag delete tag 0} msg] $msg
} {0 {}}

test palette.335 {palette0 tag delete root 0} {
    list [catch {palette0 tag delete root 0} msg] $msg
} {1 {can't delete reserved tag "root"}}

test palette.336 {palette0 move} {
    list [catch {palette0 move} msg] $msg
} {1 {wrong # args: should be "palette0 move node newParent ?switches?"}}

test palette.337 {palette0 move 0} {
    list [catch {palette0 move 0} msg] $msg
} {1 {wrong # args: should be "palette0 move node newParent ?switches?"}}

test palette.338 {palette0 move 0 0} {
    list [catch {palette0 move 0 0} msg] $msg
} {1 {can't move root node}}

test palette.339 {palette0 move 0 badNode} {
    list [catch {palette0 move 0 badNode} msg] $msg
} {1 {can't find tag or id "badNode" in ::palette0}}

test palette.340 {palette0 move 0 all} {
    list [catch {palette0 move 0 all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.341 {palette0 move 1 0 -before 2} {
    list [catch {
	palette0 move 1 0 -before 2
	palette0 children 0
    } msg] $msg
} {0 {1 2 3 4 5 6 8}}

test palette.342 {palette0 move 1 0 -after 2} {
    list [catch {
	palette0 move 1 0 -after 2
	palette0 children 0
    } msg] $msg
} {0 {2 1 3 4 5 6 8}}

test palette.343 {palette0 move 1 2} {
    list [catch {
	palette0 move 1 2
	palette0 children 0
    } msg] $msg
} {0 {2 3 4 5 6 8}}

test palette.344 {palette0 move 0 2} {
    list [catch {palette0 move 0 2} msg] $msg
} {1 {can't move root node}}

test palette.345 {palette0 move 1 17} {
    list [catch {palette0 move 1 17} msg] $msg
} {1 {can't move node: "1" is an ancestor of "17"}}

test palette.346 {palette0 attach} {
    list [catch {palette0 attach} msg] $msg
} {1 {wrong # args: should be "palette0 attach palette ?switches?"}}

test palette.347 {palette0 attach palette2 badArg} {
    list [catch {palette0 attach palette2 badArg} msg] $msg
} {1 {unknown switch "badArg"
following switches are available:
   -newtags }}


test palette.348 {palette1 attach palette0 -newtags} {
    list [catch {
	blt::palette create
	palette1 attach palette0 -newtags
	palette1 dump 0
	} msg] $msg
} {0 {-1 0 {{}} {} {}
0 2 {{} node2} {} {}
2 1 {{} node2 node1} {} {}
1 14 {{} node2 node1 node14} {} {}
14 16 {{} node2 node1 node14 node16} {} {}
16 18 {{} node2 node1 node14 node16 node18} {} {}
18 20 {{} node2 node1 node14 node16 node18 node20} {} {}
1 15 {{} node2 node1 node15} {} {}
15 17 {{} node2 node1 node15 node17} {} {}
17 19 {{} node2 node1 node15 node17 node19} {} {}
19 21 {{} node2 node1 node15 node17 node19 node21} {} {}
0 3 {{} node3} {} {}
0 4 {{} node4} {} {}
0 5 {{} node5} {} {}
5 13 {{} node5 node13} {} {}
0 6 {{} node6} {} {}
0 8 {{} myLabel} {} {}
}}

test palette.349 {palette1 attach palette0} {
    list [catch {
	blt::palette create
	palette1 attach palette0
	palette1 dump 0
	} msg] $msg
} {0 {-1 0 {{}} {} {tag2 newTag}
0 2 {{} node2} {} {tag2 newTag}
2 1 {{} node2 node1} {} {tag2 newTag}
1 14 {{} node2 node1 node14} {} {}
14 16 {{} node2 node1 node14 node16} {} {}
16 18 {{} node2 node1 node14 node16 node18} {} {}
18 20 {{} node2 node1 node14 node16 node18 node20} {} {}
1 15 {{} node2 node1 node15} {} {}
15 17 {{} node2 node1 node15 node17} {} {}
17 19 {{} node2 node1 node15 node17 node19} {} {}
19 21 {{} node2 node1 node15 node17 node19 node21} {} {}
0 3 {{} node3} {} {tag2 newTag}
0 4 {{} node4} {} {tag2 newTag}
0 5 {{} node5} {} {newTag myTag}
5 13 {{} node5 node13} {} {newTag}
0 6 {{} node6} {} {newTag}
0 8 {{} myLabel} {} {thisTag newTag}
}}

test palette.350 {palette1 attach ""} {
    list [catch {palette1 attach ""} msg] $msg
} {0 {}}


test palette.351 {blt::palette destroy palette1} {
    list [catch {blt::palette destroy palette1} msg] $msg
} {0 {}}

test palette.352 {palette0 find root -badSwitch} {
    list [catch {palette0 find root -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -addtag tagName
   -count number
   -depth number
   -exact string
   -excludes nodes
   -exec command
   -glob pattern
   -invert 
   -key string
   -keyexact string
   -keyglob pattern
   -keyregexp pattern
   -leafonly 
   -nocase 
   -order order
   -path 
   -regexp pattern
   -tag {?tag?...}}}


test palette.353 {palette0 find root -order} {
    list [catch {palette0 find root -order} msg] $msg
} {1 {value for "-order" missing}}

test palette.354 {palette0 find root ...} {
    list [catch {palette0 find root -order preorder -order postorder -order inorder} msg] $msg
} {0 {20 18 16 14 1 21 19 17 15 2 0 3 4 13 5 6 8}}

test palette.355 {palette0 find root -order preorder} {
    list [catch {palette0 find root -order preorder} msg] $msg
} {0 {0 2 1 14 16 18 20 15 17 19 21 3 4 5 13 6 8}}

test palette.356 {palette0 find root -order postorder} {
    list [catch {palette0 find root -order postorder} msg] $msg
} {0 {20 18 16 14 21 19 17 15 1 2 3 4 13 5 6 8 0}}

test palette.357 {palette0 find root -order inorder} {
    list [catch {palette0 find root -order inorder} msg] $msg
} {0 {20 18 16 14 1 21 19 17 15 2 0 3 4 13 5 6 8}}

test palette.358 {palette0 find root -order breadthfirst} {
    list [catch {palette0 find root -order breadthfirst} msg] $msg
} {0 {0 2 3 4 5 6 8 1 13 14 15 16 17 18 19 20 21}}

test palette.359 {palette0 set all key1 myValue} {
    list [catch {palette0 set all key1 myValue} msg] $msg
} {0 {}}

test palette.360 {palette0 set 15 key1 123} {
    list [catch {palette0 set 15 key1 123} msg] $msg
} {0 {}}

test palette.361 {palette0 set 16 key1 1234 key2 abc} {
    list [catch {palette0 set 16 key1 123 key2 abc} msg] $msg
} {0 {}}

test palette.362 {palette0 find root -key } {
    list [catch {palette0 find root -key} msg] $msg
} {1 {value for "-key" missing}}

test palette.363 {palette0 find root -key noKey} {
    list [catch {palette0 find root -key noKey} msg] $msg
} {0 {}}

test palette.364 {palette0 find root -key key1} {
    list [catch {palette0 find root -key key1} msg] $msg
} {0 {20 18 16 14 21 19 17 15 1 2 3 4 13 5 6 8 0}}

test palette.365 {palette0 find root -key key2} {
    list [catch {palette0 find root -key key2} msg] $msg
} {0 16}

test palette.366 {palette0 find root -key key2 -exact notThere } {
    list [catch {palette0 find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test palette.367 {palette0 find root -key key1 -glob notThere } {
    list [catch {palette0 find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test palette.368 {palette0 find root -key badKey -regexp notThere } {
    list [catch {palette0 find root -key key2 -exact notThere } msg] $msg
} {0 {}}

test palette.369 {palette0 find root -key key1 -glob 12*} {
    list [catch {palette0 find root -key key1 -glob 12*} msg] $msg
} {0 {16 15}}

test palette.370 {palette0 sort} {
    list [catch {palette0 sort} msg] $msg
} {1 {wrong # args: should be "palette0 sort node ?flags...?"}}

test palette.371 {palette0 sort all} {
    list [catch {palette0 sort all} msg] $msg
} {1 {more than one node tagged as "all"}}

test palette.372 {palette0 sort -recurse} {
    list [catch {palette0 sort -recurse} msg] $msg
} {1 {can't find tag or id "-recurse" in ::palette0}}

test palette.373 {palette0 sort 0} {
    list [catch {palette0 sort 0} msg] $msg
} {0 {8 2 3 4 5 6}}

test palette.374 {palette0 sort 0 -recurse} {
    list [catch {palette0 sort 0 -recurse} msg] $msg
} {0 {0 8 1 2 3 4 5 6 13 14 15 16 17 18 19 20 21}}

test palette.375 {palette0 sort 0 -decreasing -key} {
    list [catch {palette0 sort 0 -decreasing -key} msg] $msg
} {1 {value for "-key" missing}}

test palette.376 {palette0 sort 0 -re} {
    list [catch {palette0 sort 0 -re} msg] $msg
} {1 {ambiguous switch "-re"
following switches are available:
   -ascii 
   -command command
   -decreasing 
   -dictionary 
   -integer 
   -key string
   -path 
   -real 
   -recurse 
   -reorder }}


test palette.377 {palette0 sort 0 -decreasing} {
    list [catch {palette0 sort 0 -decreasing} msg] $msg
} {0 {6 5 4 3 2 8}}

test palette.378 {palette0 sort 0} {
    list [catch {
	set list {}
	foreach n [palette0 sort 0] {
	    lappend list [palette0 label $n]
	}	
	set list
    } msg] $msg
} {0 {myLabel node2 node3 node4 node5 node6}}

test palette.379 {palette0 sort 0 -decreasing} {
    list [catch {palette0 sort 0 -decreasing} msg] $msg
} {0 {6 5 4 3 2 8}}


test palette.380 {palette0 sort 0 -decreasing -key} {
    list [catch {palette0 sort 0 -decreasing -key} msg] $msg
} {1 {value for "-key" missing}}

test palette.381 {palette0 sort 0 -decreasing -key key1} {
    list [catch {palette0 sort 0 -decreasing -key key1} msg] $msg
} {0 {8 6 5 4 3 2}}

test palette.382 {palette0 sort 0 -decreasing -recurse -key key1} {
    list [catch {palette0 sort 0 -decreasing -recurse -key key1} msg] $msg
} {0 {15 16 0 1 2 3 4 5 6 8 13 14 17 18 19 20 21}}

test palette.383 {palette0 sort 0 -decreasing -key key1} {
    list [catch {
	set list {}
	foreach n [palette0 sort 0 -decreasing -key key1] {
	    lappend list [palette0 get $n key1]
	}
	set list
    } msg] $msg
} {0 {myValue myValue myValue myValue myValue myValue}}


test palette.384 {palette0 index 1->firstchild} {
    list [catch {palette0 index 1->firstchild} msg] $msg
} {0 14}

test palette.385 {palette0 index root->firstchild} {
    list [catch {palette0 index root->firstchild} msg] $msg
} {0 2}

test palette.386 {palette0 label root->parent} {
    list [catch {palette0 label root->parent} msg] $msg
} {1 {can't find tag or id "root->parent" in ::palette0}}

test palette.387 {palette0 index root->parent} {
    list [catch {palette0 index root->parent} msg] $msg
} {0 -1}

test palette.388 {palette0 index root->lastchild} {
    list [catch {palette0 index root->lastchild} msg] $msg
} {0 8}

test palette.389 {palette0 index root->next} {
    list [catch {palette0 index root->next} msg] $msg
} {0 2}

test palette.390 {palette0 index root->previous} {
    list [catch {palette0 index root->previous} msg] $msg
} {0 -1}

test palette.391 {palette0 label root->previous} {
    list [catch {palette0 label root->previous} msg] $msg
} {1 {can't find tag or id "root->previous" in ::palette0}}

test palette.392 {palette0 index 1->previous} {
    list [catch {palette0 index 1->previous} msg] $msg
} {0 2}

test palette.393 {palette0 label root->badModifier} {
    list [catch {palette0 label root->badModifier} msg] $msg
} {1 {can't find tag or id "root->badModifier" in ::palette0}}

test palette.394 {palette0 index root->badModifier} {
    list [catch {palette0 index root->badModifier} msg] $msg
} {0 -1}

test palette.395 {palette0 index root->firstchild->parent} {
    list [catch {palette0 index root->firstchild->parent} msg] $msg
} {0 0}

test palette.396 {palette0 trace} {
    list [catch {palette0 trace} msg] $msg
} {1 {wrong # args: should be one of...
  palette0 trace create node key how command ?-whenidle?
  palette0 trace delete id...
  palette0 trace info id
  palette0 trace names }}


test palette.397 {palette0 trace create} {
    list [catch {palette0 trace create} msg] $msg
} {1 {wrong # args: should be "palette0 trace create node key how command ?-whenidle?"}}

test palette.398 {palette0 trace create root} {
    list [catch {palette0 trace create root} msg] $msg
} {1 {wrong # args: should be "palette0 trace create node key how command ?-whenidle?"}}

test palette.399 {palette0 trace create root * } {
    list [catch {palette0 trace create root * } msg] $msg
} {1 {wrong # args: should be "palette0 trace create node key how command ?-whenidle?"}}

test palette.400 {palette0 trace create root * rwuc} {
    list [catch {palette0 trace create root * rwuc} msg] $msg
} {1 {wrong # args: should be "palette0 trace create node key how command ?-whenidle?"}}

proc Doit args { global mylist; lappend mylist $args }

test palette.401 {palette0 trace create all newKey rwuc Doit} {
    list [catch {palette0 trace create all newKey rwuc Doit} msg] $msg
} {0 trace0}

test palette.402 {palette0 trace info trace0} {
    list [catch {palette0 trace info trace0} msg] $msg
} {0 {all newKey rwuc Doit}}

test palette.403 {test create trace} {
    list [catch {
	set mylist {}
	palette0 set all newKey 20
	set mylist
	} msg] $msg
} {0 {{::palette0 0 newKey wc} {::palette0 2 newKey wc} {::palette0 1 newKey wc} {::palette0 14 newKey wc} {::palette0 16 newKey wc} {::palette0 18 newKey wc} {::palette0 20 newKey wc} {::palette0 15 newKey wc} {::palette0 17 newKey wc} {::palette0 19 newKey wc} {::palette0 21 newKey wc} {::palette0 3 newKey wc} {::palette0 4 newKey wc} {::palette0 5 newKey wc} {::palette0 13 newKey wc} {::palette0 6 newKey wc} {::palette0 8 newKey wc}}}

test palette.404 {test read trace} {
    list [catch {
	set mylist {}
	palette0 get root newKey
	set mylist
	} msg] $msg
} {0 {{::palette0 0 newKey r}}}

test palette.405 {test write trace} {
    list [catch {
	set mylist {}
	palette0 set all newKey 21
	set mylist
	} msg] $msg
} {0 {{::palette0 0 newKey w} {::palette0 2 newKey w} {::palette0 1 newKey w} {::palette0 14 newKey w} {::palette0 16 newKey w} {::palette0 18 newKey w} {::palette0 20 newKey w} {::palette0 15 newKey w} {::palette0 17 newKey w} {::palette0 19 newKey w} {::palette0 21 newKey w} {::palette0 3 newKey w} {::palette0 4 newKey w} {::palette0 5 newKey w} {::palette0 13 newKey w} {::palette0 6 newKey w} {::palette0 8 newKey w}}}

test palette.406 {test unset trace} {
    list [catch {
	set mylist {}
	palette0 set all newKey 21
	set mylist
	} msg] $msg
} {0 {{::palette0 0 newKey w} {::palette0 2 newKey w} {::palette0 1 newKey w} {::palette0 14 newKey w} {::palette0 16 newKey w} {::palette0 18 newKey w} {::palette0 20 newKey w} {::palette0 15 newKey w} {::palette0 17 newKey w} {::palette0 19 newKey w} {::palette0 21 newKey w} {::palette0 3 newKey w} {::palette0 4 newKey w} {::palette0 5 newKey w} {::palette0 13 newKey w} {::palette0 6 newKey w} {::palette0 8 newKey w}}}

test palette.407 {palette0 trace delete} {
    list [catch {palette0 trace delete} msg] $msg
} {0 {}}

test palette.408 {palette0 trace delete badId} {
    list [catch {palette0 trace delete badId} msg] $msg
} {1 {unknown trace "badId"}}

test palette.409 {palette0 trace delete trace0} {
    list [catch {palette0 trace delete trace0} msg] $msg
} {0 {}}

test palette.410 {test create trace} {
    list [catch {
	set mylist {}
	palette0 set all newKey 20
	set mylist
	} msg] $msg
} {0 {}}

test palette.411 {test unset trace} {
    list [catch {
	set mylist {}
	palette0 unset all newKey
	set mylist
	} msg] $msg
} {0 {}}


test palette.412 {palette0 notify} {
    list [catch {palette0 notify} msg] $msg
} {1 {wrong # args: should be one of...
  palette0 notify create ?flags? command
  palette0 notify delete notifyId...
  palette0 notify info notifyId
  palette0 notify names }}


test palette.413 {palette0 notify create} {
    list [catch {palette0 notify create} msg] $msg
} {1 {wrong # args: should be "palette0 notify create ?flags? command"}}

test palette.414 {palette0 notify create -allevents} {
    list [catch {palette0 notify create -allevents Doit} msg] $msg
} {0 notify0}

test palette.415 {palette0 notify info notify0} {
    list [catch {palette0 notify info notify0} msg] $msg
} {0 {notify0 {-create -delete -move -sort -relabel} {Doit}}}

test palette.416 {palette0 notify info badId} {
    list [catch {palette0 notify info badId} msg] $msg
} {1 {unknown notify name "badId"}}

test palette.417 {palette0 notify info} {
    list [catch {palette0 notify info} msg] $msg
} {1 {wrong # args: should be "palette0 notify info notifyId"}}

test palette.418 {palette0 notify names} {
    list [catch {palette0 notify names} msg] $msg
} {0 notify0}


test palette.419 {test create notify} {
    list [catch {
	set mylist {}
	palette0 insert 1 -tags test
	set mylist
	} msg] $msg
} {0 {{-create 22}}}

test palette.420 {test move notify} {
    list [catch {
	set mylist {}
	palette0 move 8 test
	set mylist
	} msg] $msg
} {0 {{-move 8}}}

test palette.421 {test sort notify} {
    list [catch {
	set mylist {}
	palette0 sort 0 -reorder 
	set mylist
	} msg] $msg
} {0 {{-sort 0}}}

test palette.422 {test relabel notify} {
    list [catch {
	set mylist {}
	palette0 label test "newLabel"
	set mylist
	} msg] $msg
} {0 {{-relabel 22}}}

test palette.423 {test delete notify} {
    list [catch {
	set mylist {}
	palette0 delete test
	set mylist
	} msg] $msg
} {0 {{-delete 8} {-delete 22}}}


test palette.424 {palette0 notify delete badId} {
    list [catch {palette0 notify delete badId} msg] $msg
} {1 {unknown notify name "badId"}}


test palette.425 {test create notify} {
    list [catch {
	set mylist {}
	palette0 set all newKey 20
	set mylist
	} msg] $msg
} {0 {}}

test palette.426 {test delete notify} {
    list [catch {
	set mylist {}
	palette0 unset all newKey
	set mylist
	} msg] $msg
} {0 {}}

test palette.427 {test delete notify} {
    list [catch {
	set mylist {}
	palette0 unset all newKey
	set mylist
	} msg] $msg
} {0 {}}

test palette.428 {palette0 copy} {
    list [catch {palette0 copy} msg] $msg
} {1 {wrong # args: should be "palette0 copy parent ?palette? node ?switches?"}}

test palette.429 {palette0 copy root} {
    list [catch {palette0 copy root} msg] $msg
} {1 {wrong # args: should be "palette0 copy parent ?palette? node ?switches?"}}

test palette.430 {palette0 copy root 14} {
    list [catch {palette0 copy root 14} msg] $msg
} {0 23}

test palette.431 {palette0 copy 14 root} {
    list [catch {palette0 copy 14 root} msg] $msg
} {0 24}

test palette.432 {palette0 copy 14 root -recurse} {
    list [catch {palette0 copy 14 root -recurse} msg] $msg
} {1 {can't make cyclic copy: source node is an ancestor of the destination}}

test palette.433 {palette0 copy 3 2 -recurse -tags} {
    list [catch {palette0 copy 3 2 -recurse -tags} msg] $msg
} {0 25}

test palette.434 {copy palette to palette -recurse} {
    list [catch {
	blt::palette create palette1
	foreach node [palette0 children root] {
	    palette1 copy root palette0 $node -recurse 
	}
	foreach node [palette0 children root] {
	    palette1 copy root palette0 $node -recurse 
	}
	palette1 dump root
    } msg] $msg
} {0 {-1 0 {{}} {} {}
0 1 {{} node2} {key1 myValue} {}
1 2 {{} node2 node1} {key1 myValue} {}
2 3 {{} node2 node1 node14} {key1 myValue} {}
3 4 {{} node2 node1 node14 node16} {key1 123 key2 abc} {}
4 5 {{} node2 node1 node14 node16 node18} {key1 myValue} {}
5 6 {{} node2 node1 node14 node16 node18 node20} {key1 myValue} {}
3 7 {{} node2 node1 node14 {}} {key1 myValue} {}
2 8 {{} node2 node1 node15} {key1 123} {}
8 9 {{} node2 node1 node15 node17} {key1 myValue} {}
9 10 {{} node2 node1 node15 node17 node19} {key1 myValue} {}
10 11 {{} node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 12 {{} node3} {key1 myValue} {}
12 13 {{} node3 node2} {key1 myValue} {}
13 14 {{} node3 node2 node1} {key1 myValue} {}
14 15 {{} node3 node2 node1 node14} {key1 myValue} {}
15 16 {{} node3 node2 node1 node14 node16} {key1 123 key2 abc} {}
16 17 {{} node3 node2 node1 node14 node16 node18} {key1 myValue} {}
17 18 {{} node3 node2 node1 node14 node16 node18 node20} {key1 myValue} {}
15 19 {{} node3 node2 node1 node14 {}} {key1 myValue} {}
14 20 {{} node3 node2 node1 node15} {key1 123} {}
20 21 {{} node3 node2 node1 node15 node17} {key1 myValue} {}
21 22 {{} node3 node2 node1 node15 node17 node19} {key1 myValue} {}
22 23 {{} node3 node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 24 {{} node4} {key1 myValue} {}
0 25 {{} node5} {key1 myValue} {}
25 26 {{} node5 node13} {key1 myValue} {}
0 27 {{} node6} {key1 myValue} {}
0 28 {{} node14} {key1 myValue} {}
0 29 {{} node2} {key1 myValue} {}
29 30 {{} node2 node1} {key1 myValue} {}
30 31 {{} node2 node1 node14} {key1 myValue} {}
31 32 {{} node2 node1 node14 node16} {key1 123 key2 abc} {}
32 33 {{} node2 node1 node14 node16 node18} {key1 myValue} {}
33 34 {{} node2 node1 node14 node16 node18 node20} {key1 myValue} {}
31 35 {{} node2 node1 node14 {}} {key1 myValue} {}
30 36 {{} node2 node1 node15} {key1 123} {}
36 37 {{} node2 node1 node15 node17} {key1 myValue} {}
37 38 {{} node2 node1 node15 node17 node19} {key1 myValue} {}
38 39 {{} node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 40 {{} node3} {key1 myValue} {}
40 41 {{} node3 node2} {key1 myValue} {}
41 42 {{} node3 node2 node1} {key1 myValue} {}
42 43 {{} node3 node2 node1 node14} {key1 myValue} {}
43 44 {{} node3 node2 node1 node14 node16} {key1 123 key2 abc} {}
44 45 {{} node3 node2 node1 node14 node16 node18} {key1 myValue} {}
45 46 {{} node3 node2 node1 node14 node16 node18 node20} {key1 myValue} {}
43 47 {{} node3 node2 node1 node14 {}} {key1 myValue} {}
42 48 {{} node3 node2 node1 node15} {key1 123} {}
48 49 {{} node3 node2 node1 node15 node17} {key1 myValue} {}
49 50 {{} node3 node2 node1 node15 node17 node19} {key1 myValue} {}
50 51 {{} node3 node2 node1 node15 node17 node19 node21} {key1 myValue} {}
0 52 {{} node4} {key1 myValue} {}
0 53 {{} node5} {key1 myValue} {}
53 54 {{} node5 node13} {key1 myValue} {}
0 55 {{} node6} {key1 myValue} {}
0 56 {{} node14} {key1 myValue} {}
}}

puts stderr "done testing palettecmd.tcl"

exit 0







