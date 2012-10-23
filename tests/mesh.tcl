
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

test mesh.1 {mesh no args} {
    list [catch {blt::mesh} msg] $msg
} {1 {wrong # args: should be one of...
  blt::mesh cget name option
  blt::mesh configure name ?option value?...
  blt::mesh create type ?name? ?option value?...
  blt::mesh delete ?name?...
  blt::mesh hide name ?indices...?
  blt::mesh hull name ?-vertices?
  blt::mesh names ?pattern?...
  blt::mesh triangles name
  blt::mesh type name
  blt::mesh vertices name}}

test mesh.2 {mesh badOp} {
    list [catch {blt::mesh badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  blt::mesh cget name option
  blt::mesh configure name ?option value?...
  blt::mesh create type ?name? ?option value?...
  blt::mesh delete ?name?...
  blt::mesh hide name ?indices...?
  blt::mesh hull name ?-vertices?
  blt::mesh names ?pattern?...
  blt::mesh triangles name
  blt::mesh type name
  blt::mesh vertices name}}

test mesh.3 {mesh create} {
    list [catch {blt::mesh create} msg] $msg
} {1 {wrong # args: should be "blt::mesh create type ?name? ?option value?..."}}

test mesh.4 {mesh create regular} {
    list [catch {blt::mesh create regular} msg] $msg
} {0 ::mesh0}

test mesh.5 {mesh type mesh0} {
    list [catch {blt::mesh type mesh0} msg] $msg
} {0 regular}

test mesh.6 {mesh create regular myRegular} {
    list [catch {blt::mesh create regular myRegular} msg] $msg
} {0 ::myRegular}

test mesh.7 {mesh create irregular} {
    list [catch {blt::mesh create irregular} msg] $msg
} {0 ::mesh1}

test mesh.8 {mesh type mesh1} {
    list [catch {blt::mesh type mesh1} msg] $msg
} {0 irregular}

test mesh.9 {mesh create irregular myIrregular} {
    list [catch {blt::mesh create irregular myIrregular} msg] $msg
} {0 ::myIrregular}

test mesh.10 {mesh create cloud} {
    list [catch {blt::mesh create cloud} msg] $msg
} {0 ::mesh2}

test mesh.11 {mesh type mesh2} {
    list [catch {blt::mesh type mesh2} msg] $msg
} {0 cloud}

test mesh.12 {mesh create cloud myCloud} {
    list [catch {blt::mesh create cloud myCloud} msg] $msg
} {0 ::myCloud}

test mesh.13 {mesh create triangle} {
    list [catch {blt::mesh create triangle} msg] $msg
} {0 ::mesh3}

test mesh.14 {mesh type mesh3} {
    list [catch {blt::mesh type mesh3} msg] $msg
} {0 triangle}

test mesh.15 {mesh create triangle myTriangular} {
    list [catch {blt::mesh create triangle myTriangular} msg] $msg
} {0 ::myTriangular}

test mesh.16 {mesh names} {
    list [catch {lsort [blt::mesh names]} msg] $msg
} {0 {::mesh0 ::mesh1 ::mesh2 ::mesh3 ::myCloud ::myIrregular ::myRegular ::myTriangular}}

test mesh.17 {mesh names mesh*} {
    list [catch {lsort [blt::mesh names ::mesh*]} msg] $msg
} {0 {::mesh0 ::mesh1 ::mesh2 ::mesh3}}

test mesh.18 {mesh names *my*} {
    list [catch {lsort [blt::mesh names *my*]} msg] $msg
} {0 {::myCloud ::myIrregular ::myRegular ::myTriangular}}

test mesh.19 {mesh delete [mesh names]} {
    list [catch {eval blt::mesh delete [blt::mesh names]} msg] $msg
} {0 {}}

test mesh.20 {mesh names} {
    list [catch {blt::mesh names} msg] $msg
} {0 {}}

test mesh.21 {mesh create cloud myMesh} {
    list [catch {blt::mesh create cloud myMesh} msg] $msg
} {0 ::myMesh}

test mesh.22 {mesh names} {
    list [catch {blt::mesh names} msg] $msg
} {0 ::myMesh}

test mesh.23 {mesh configure myMesh} {
    list [catch {blt::mesh configure myMesh} msg] $msg
} {0 {{-x {} {}} {-y {} {}}}}

test mesh.24 {mesh cget myMesh -x} {
    list [catch {blt::mesh cget myMesh -x} msg] $msg
} {0 {}}

test mesh.25 {mesh cget myMesh -y} {
    list [catch {blt::mesh cget myMesh -y} msg] $msg
} {0 {}}

test mesh.26 {mesh create regular myRegular -x { 0 5 5 } -y { 0 5 5 } } {
    list [catch {
	blt::mesh create regular myRegular -x { 0 5 5 } -y { 0 5 5 }
    } msg] $msg
} {0 ::myRegular}

test mesh.27 {mesh configure irregular myIrregular ...} {
    list [catch {
	blt::mesh create irregular myIrregular -x { 0 1 2 3 4 5 7 10 } \
	    -y { 0 1 2 3 4 5 7 10 } 
    } msg] $msg
} {0 ::myIrregular}

test mesh.28 {mesh configure myRegular} {
    list [catch {
	blt::mesh configure myRegular
    } msg] $msg
} {0 {{-x {} {0.0 5.0 5.0}} {-y {} {0.0 5.0 5.0}}}}

test mesh.29 {mesh vertices myRegular} {
    list [catch {
	blt::mesh vertices myRegular
    } msg] $msg
} {0 {{0 0.0 0.0} {1 1.25 0.0} {2 2.5 0.0} {3 3.75 0.0} {4 5.0 0.0} {5 0.0 1.25} {6 1.25 1.25} {7 2.5 1.25} {8 3.75 1.25} {9 5.0 1.25} {10 0.0 2.5} {11 1.25 2.5} {12 2.5 2.5} {13 3.75 2.5} {14 5.0 2.5} {15 0.0 3.75} {16 1.25 3.75} {17 2.5 3.75} {18 3.75 3.75} {19 5.0 3.75} {20 0.0 5.0} {21 1.25 5.0} {22 2.5 5.0} {23 3.75 5.0} {24 5.0 5.0}}}


test mesh.30 {mesh triangles myRegular} {
    list [catch {blt::mesh triangles myRegular} msg] $msg
} {0 {{0 6 1} {5 6 0} {1 7 2} {6 7 1} {2 8 3} {7 8 2} {3 9 4} {8 9 3} {5 11 6} {10 11 5} {6 12 7} {11 12 6} {7 13 8} {12 13 7} {8 14 9} {13 14 8} {10 16 11} {15 16 10} {11 17 12} {16 17 11} {12 18 13} {17 18 12} {13 19 14} {18 19 13} {15 21 16} {20 21 15} {16 22 17} {21 22 16} {17 23 18} {22 23 17} {18 24 19} {23 24 18}}}

test mesh.31 {mesh hull myRegular} {
    list [catch {
	blt::mesh hull myRegular 
    } msg] $msg
} {0 {0 4 24 20 0}}

test mesh.32 {mesh hull myRegular -vertices} {
    list [catch {
	blt::mesh hull myRegular -vertices
    } msg] $msg
} {0 {0.0 0.0 5.0 0.0 5.0 5.0 0.0 5.0 0.0 0.0}}

test mesh.33 {mesh configure myIrregular} {
    list [catch {
	blt::mesh configure myIrregular
    } msg] $msg
} {0 {{-x {} {0.0 1.0 2.0 3.0 4.0 5.0 7.0 10.0}} {-y {} {0.0 1.0 2.0 3.0 4.0 5.0 7.0 10.0}}}}

test mesh.34 {mesh vertices myIrregular} {
    list [catch {
	blt::mesh vertices myIrregular
    } msg] $msg
} {0 {{0 0.0 0.0} {1 1.0 0.0} {2 2.0 0.0} {3 3.0 0.0} {4 4.0 0.0} {5 5.0 0.0} {6 7.0 0.0} {7 10.0 0.0} {8 0.0 1.0} {9 1.0 1.0} {10 2.0 1.0} {11 3.0 1.0} {12 4.0 1.0} {13 5.0 1.0} {14 7.0 1.0} {15 10.0 1.0} {16 0.0 2.0} {17 1.0 2.0} {18 2.0 2.0} {19 3.0 2.0} {20 4.0 2.0} {21 5.0 2.0} {22 7.0 2.0} {23 10.0 2.0} {24 0.0 3.0} {25 1.0 3.0} {26 2.0 3.0} {27 3.0 3.0} {28 4.0 3.0} {29 5.0 3.0} {30 7.0 3.0} {31 10.0 3.0} {32 0.0 4.0} {33 1.0 4.0} {34 2.0 4.0} {35 3.0 4.0} {36 4.0 4.0} {37 5.0 4.0} {38 7.0 4.0} {39 10.0 4.0} {40 0.0 5.0} {41 1.0 5.0} {42 2.0 5.0} {43 3.0 5.0} {44 4.0 5.0} {45 5.0 5.0} {46 7.0 5.0} {47 10.0 5.0} {48 0.0 7.0} {49 1.0 7.0} {50 2.0 7.0} {51 3.0 7.0} {52 4.0 7.0} {53 5.0 7.0} {54 7.0 7.0} {55 10.0 7.0} {56 0.0 10.0} {57 1.0 10.0} {58 2.0 10.0} {59 3.0 10.0} {60 4.0 10.0} {61 5.0 10.0} {62 7.0 10.0} {63 10.0 10.0}}}

test mesh.35 {mesh hull myIrregular} {
    list [catch {
	blt::mesh hull myIrregular
    } msg] $msg
} {0 {0 7 63 56 0}}


test mesh.36 {mesh delete myIrregular} {
    list [catch {
	blt::mesh delete myIrregular
    } msg] $msg
} {0 {}}

test mesh.37 {mesh delete myRegular} {
    list [catch {
	blt::mesh delete myRegular
    } msg] $msg
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

test mesh.38 {mesh configure myMesh -x xVector -y yVector} {
    list [catch {blt::mesh configure myMesh -x xVector -y yVector} msg] $msg
} {0 {}}

test mesh.39 {mesh triangles myMesh} {
    list [catch {blt::mesh triangles myMesh} msg] $msg
} {0 {{4 3 1} {3 5 1} {1 2 0} {6 3 4} {1 7 2} {3 8 5} {1 10 7} {9 11 0} {6 13 3} {15 13 6} {14 5 8} {16 10 1} {20 16 1} {19 2 7} {5 20 1} {15 18 13} {2 9 0} {10 19 7} {14 20 5} {9 12 11} {13 8 3} {6 21 15} {2 22 9} {19 22 2} {9 17 12} {21 18 15} {12 23 11} {4 21 6} {26 23 12} {17 26 12} {24 22 19} {4 25 21} {8 28 14} {13 28 8} {18 28 13} {27 26 17} {27 29 26} {16 19 10} {30 14 28} {22 31 9} {32 28 18} {21 32 18} {30 34 14} {29 23 26} {34 20 14} {16 24 19} {25 32 21} {31 17 9} {31 27 17} {29 36 23} {16 35 24} {32 30 28} {33 32 25} {39 36 29} {31 38 27} {38 29 27} {41 24 35} {37 30 32} {40 41 35} {41 22 24} {38 39 29} {30 42 34} {44 41 40} {20 35 16} {4 33 25} {31 43 38} {31 45 43} {33 37 32} {37 42 30} {41 31 22} {34 35 20} {38 46 39} {42 47 34} {38 48 46} {47 35 34} {47 40 35} {31 49 45} {43 48 38} {39 50 36} {46 50 39} {51 47 42} {41 49 31} {48 52 46} {47 44 40} {52 50 46} {51 53 47} {42 56 51} {37 56 42} {41 54 49} {53 44 47} {45 48 43} {41 58 54} {44 58 41} {55 44 53} {57 44 55} {49 48 45} {44 59 58} {33 60 37} {60 56 37} {57 59 44} {50 23 36} {61 51 56} {61 53 51} {61 55 53} {62 54 58} {63 61 56} {61 57 55} {64 49 54} {64 48 49} {64 52 48} {62 64 54} {60 63 56} {66 64 62} {59 62 58} {63 65 61} {67 59 57} {61 67 57} {65 67 61} {68 64 66} {60 69 63} {65 71 67} {71 72 67} {68 70 64} {69 65 63} {74 64 70} {74 52 64} {73 69 60} {75 50 52} {74 75 52} {76 72 71} {78 77 74} {65 76 71} {70 78 74} {69 76 65} {81 74 77} {79 69 73} {80 70 68} {82 72 76} {80 78 70} {81 75 74} {66 80 68} {59 66 62} {67 66 59} {78 81 77} {67 80 66} {83 84 82} {72 80 67} {84 72 82} {84 80 72} {84 85 80} {88 87 86} {79 88 86} {86 69 79} {89 81 78} {86 76 69} {80 89 78} {90 85 84} {93 82 76} {94 93 76} {86 94 76} {93 83 82} {91 84 83} {85 89 80} {81 92 75} {95 93 94} {93 91 83} {93 96 91} {91 90 84} {97 85 90} {98 94 86} {86 99 98} {87 99 86} {89 92 81} {98 95 94} {88 99 87} {95 96 93} {33 73 60} {97 89 85} {91 97 90} {96 97 91} {98 96 95} {73 88 79} {97 92 89} {98 97 96} {92 50 75} {99 97 98}}}

test mesh.40 {mesh hull myMesh} {
    list [catch {blt::mesh hull myMesh} msg] $msg
} {0 {0 11 23 50 92 97 99 88 73 33 4 1 0}}

test mesh.41 {mesh create regular (bad namespace)} {
    list [catch {blt::mesh create regular badName::fred} msg] $msg
} {1 {unknown namespace "badName"}}

test mesh.42 {mesh create (wrong # args)} {
    list [catch {blt::mesh create} msg] $msg
} {1 {wrong # args: should be "blt::mesh create type ?name? ?option value?..."}}


test mesh.43 {mesh names} {
    list [catch {blt::mesh names} msg] [lsort $msg]
} {0 ::myMesh}

test mesh.44 {mesh names pattern)} {
    list [catch {blt::mesh names ::myMesh*} msg] [lsort $msg]
} {0 ::myMesh}

test mesh.45 {mesh names badPattern)} {
    list [catch {blt::mesh names badPattern*} msg] $msg
} {0 {}}

test mesh.46 {mesh names badPattern *} {
    list [catch {blt::mesh names badPattern *} msg] $msg
} {0 ::myMesh}

test mesh.47 {mesh delete (no args)} {
    list [catch {blt::mesh delete} msg] $msg
} {0 {}}

test mesh.48 {mesh delete badMesh} {
    list [catch {blt::mesh delete badMesh} msg] $msg
} {1 {can't find a mesh "badMesh"}}

exit 0

