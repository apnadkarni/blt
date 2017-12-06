package require BLT
set rgbNames {
    000000 black
    000000 gray0
    000000 grey0
    000080 NavyBlue
    000080 navy
    000080 {navy blue}
    00008B DarkBlue
    00008B blue4
    00008B {dark blue}
    0000CD MediumBlue
    0000CD blue3
    0000CD {medium blue}
    0000EE blue2
    0000FF blue
    0000FF blue1
    006400 DarkGreen
    006400 {dark green}
    00688B DeepSkyBlue4
    00868B turquoise4
    008B00 green4
    008B45 SpringGreen4
    008B8B DarkCyan
    008B8B cyan4
    008B8B {dark cyan}
    009ACD DeepSkyBlue3
    00B2EE DeepSkyBlue2
    00BFFF DeepSkyBlue
    00BFFF DeepSkyBlue1
    00BFFF {deep sky blue}
    00C5CD turquoise3
    00CD00 green3
    00CD66 SpringGreen3
    00CDCD cyan3
    00CED1 DarkTurquoise
    00CED1 {dark turquoise}
    00E5EE turquoise2
    00EE00 green2
    00EE76 SpringGreen2
    00EEEE cyan2
    00F5FF turquoise1
    00FA9A MediumSpringGreen
    00FA9A {medium spring green}
    00FF00 green
    00FF00 green1
    00FF7F SpringGreen
    00FF7F SpringGreen1
    00FF7F {spring green}
    00FFFF cyan
    00FFFF cyan1
    030303 gray1
    030303 grey1
    050505 gray2
    050505 grey2
    080808 gray3
    080808 grey3
    0A0A0A gray4
    0A0A0A grey4
    0D0D0D gray5
    0D0D0D grey5
    0F0F0F gray6
    0F0F0F grey6
    104E8B DodgerBlue4
    121212 gray7
    121212 grey7
    141414 gray8
    141414 grey8
    171717 gray9
    171717 grey9
    1874CD DodgerBlue3
    191970 MidnightBlue
    191970 {midnight blue}
    1A1A1A gray10
    1A1A1A grey10
    1C1C1C gray11
    1C1C1C grey11
    1C86EE DodgerBlue2
    1E90FF DodgerBlue
    1E90FF DodgerBlue1
    1E90FF {dodger blue}
    1F1F1F gray12
    1F1F1F grey12
    20B2AA LightSeaGreen
    20B2AA {light sea green}
    212121 gray13
    212121 grey13
    228B22 ForestGreen
    228B22 {forest green}
    242424 gray14
    242424 grey14
    262626 gray15
    262626 grey15
    27408B RoyalBlue4
    292929 gray16
    292929 grey16
    2B2B2B gray17
    2B2B2B grey17
    2E2E2E gray18
    2E2E2E grey18
    2E8B57 SeaGreen
    2E8B57 SeaGreen4
    2E8B57 {sea green}
    2F4F4F DarkSlateGray
    2F4F4F DarkSlateGrey
    2F4F4F {dark slate gray}
    2F4F4F {dark slate grey}
    303030 gray19
    303030 grey19
    32CD32 LimeGreen
    32CD32 {lime green}
    333333 gray20
    333333 grey20
    363636 gray21
    363636 grey21
    36648B SteelBlue4
    383838 gray22
    383838 grey22
    3A5FCD RoyalBlue3
    3B3B3B gray23
    3B3B3B grey23
    3CB371 MediumSeaGreen
    3CB371 {medium sea green}
    3D3D3D gray24
    3D3D3D grey24
    404040 gray25
    404040 grey25
    40E0D0 turquoise
    4169E1 RoyalBlue
    4169E1 {royal blue}
    424242 gray26
    424242 grey26
    436EEE RoyalBlue2
    43CD80 SeaGreen3
    454545 gray27
    454545 grey27
    458B00 chartreuse4
    458B74 aquamarine4
    4682B4 SteelBlue
    4682B4 {steel blue}
    473C8B SlateBlue4
    474747 gray28
    474747 grey28
    483D8B DarkSlateBlue
    483D8B {dark slate blue}
    4876FF RoyalBlue1
    48D1CC MediumTurquoise
    48D1CC {medium turquoise}
    4A4A4A gray29
    4A4A4A grey29
    4A708B SkyBlue4
    4D4D4D gray30
    4D4D4D grey30
    4EEE94 SeaGreen2
    4F4F4F gray31
    4F4F4F grey31
    4F94CD SteelBlue3
    525252 gray32
    525252 grey32
    528B8B DarkSlateGray4
    53868B CadetBlue4
    545454 gray33
    545454 grey33
    548B54 PaleGreen4
    54FF9F SeaGreen1
    551A8B purple4
    556B2F DarkOliveGreen
    556B2F {dark olive green}
    575757 gray34
    575757 grey34
    595959 gray35
    595959 grey35
    5C5C5C gray36
    5C5C5C grey36
    5CACEE SteelBlue2
    5D478B MediumPurple4
    5E5E5E gray37
    5E5E5E grey37
    5F9EA0 CadetBlue
    5F9EA0 {cadet blue}
    607B8B LightSkyBlue4
    616161 gray38
    616161 grey38
    636363 gray39
    636363 grey39
    63B8FF SteelBlue1
    6495ED CornflowerBlue
    6495ED {cornflower blue}
    666666 gray40
    666666 grey40
    668B8B PaleTurquoise4
    66CD00 chartreuse3
    66CDAA MediumAquamarine
    66CDAA aquamarine3
    66CDAA {medium aquamarine}
    68228B DarkOrchid4
    68838B LightBlue4
    6959CD SlateBlue3
    696969 DimGray
    696969 DimGrey
    696969 gray41
    696969 grey41
    696969 {dim gray}
    696969 {dim grey}
    698B22 OliveDrab4
    698B69 DarkSeaGreen4
    6A5ACD SlateBlue
    6A5ACD {slate blue}
    6B6B6B gray42
    6B6B6B grey42
    6B8E23 OliveDrab
    6B8E23 {olive drab}
    6C7B8B SlateGray4
    6CA6CD SkyBlue3
    6E6E6E gray43
    6E6E6E grey43
    6E7B8B LightSteelBlue4
    6E8B3D DarkOliveGreen4
    707070 gray44
    707070 grey44
    708090 SlateGray
    708090 SlateGrey
    708090 {slate gray}
    708090 {slate grey}
    737373 gray45
    737373 grey45
    757575 gray46
    757575 grey46
    76EE00 chartreuse2
    76EEC6 aquamarine2
    778899 LightSlateGray
    778899 LightSlateGrey
    778899 {light slate gray}
    778899 {light slate grey}
    787878 gray47
    787878 grey47
    79CDCD DarkSlateGray3
    7A378B MediumOrchid4
    7A67EE SlateBlue2
    7A7A7A gray48
    7A7A7A grey48
    7A8B8B LightCyan4
    7AC5CD CadetBlue3
    7B68EE MediumSlateBlue
    7B68EE {medium slate blue}
    7CCD7C PaleGreen3
    7CFC00 LawnGreen
    7CFC00 {lawn green}
    7D26CD purple3
    7D7D7D gray49
    7D7D7D grey49
    7EC0EE SkyBlue2
    7F7F7F gray50
    7F7F7F grey50
    7FFF00 chartreuse
    7FFF00 chartreuse1
    7FFFD4 aquamarine
    7FFFD4 aquamarine1
    828282 gray51
    828282 grey51
    836FFF SlateBlue1
    838B83 honeydew4
    838B8B azure4
    8470FF LightSlateBlue
    8470FF {light slate blue}
    858585 gray52
    858585 grey52
    878787 gray53
    878787 grey53
    87CEEB SkyBlue
    87CEEB {sky blue}
    87CEFA LightSkyBlue
    87CEFA {light sky blue}
    87CEFF SkyBlue1
    8968CD MediumPurple3
    8A2BE2 BlueViolet
    8A2BE2 {blue violet}
    8A8A8A gray54
    8A8A8A grey54
    8B0000 DarkRed
    8B0000 red4
    8B0000 {dark red}
    8B008B DarkMagenta
    8B008B magenta4
    8B008B {dark magenta}
    8B0A50 DeepPink4
    8B1A1A firebrick4
    8B1C62 maroon4
    8B2252 VioletRed4
    8B2323 brown4
    8B2500 OrangeRed4
    8B3626 tomato4
    8B3A3A IndianRed4
    8B3A62 HotPink4
    8B3E2F coral4
    8B4500 DarkOrange4
    8B4513 SaddleBrown
    8B4513 chocolate4
    8B4513 {saddle brown}
    8B4726 sienna4
    8B475D PaleVioletRed4
    8B4789 orchid4
    8B4C39 salmon4
    8B5742 LightSalmon4
    8B5A00 orange4
    8B5A2B tan4
    8B5F65 LightPink4
    8B636C pink4
    8B6508 DarkGoldenrod4
    8B668B plum4
    8B6914 goldenrod4
    8B6969 RosyBrown4
    8B7355 burlywood4
    8B7500 gold4
    8B7765 PeachPuff4
    8B795E NavajoWhite4
    8B7B8B thistle4
    8B7D6B bisque4
    8B7D7B MistyRose4
    8B7E66 wheat4
    8B814C LightGoldenrod4
    8B8378 AntiqueWhite4
    8B8386 LavenderBlush4
    8B864E khaki4
    8B8682 seashell4
    8B8878 cornsilk4
    8B8970 LemonChiffon4
    8B8989 snow4
    8B8B00 yellow4
    8B8B7A LightYellow4
    8B8B83 ivory4
    8C8C8C gray55
    8C8C8C grey55
    8DB6CD LightSkyBlue3
    8DEEEE DarkSlateGray2
    8EE5EE CadetBlue2
    8F8F8F gray56
    8F8F8F grey56
    8FBC8F DarkSeaGreen
    8FBC8F {dark sea green}
    90EE90 LightGreen
    90EE90 PaleGreen2
    90EE90 {light green}
    912CEE purple2
    919191 gray57
    919191 grey57
    9370DB MediumPurple
    9370DB {medium purple}
    9400D3 DarkViolet
    9400D3 {dark violet}
    949494 gray58
    949494 grey58
    969696 gray59
    969696 grey59
    96CDCD PaleTurquoise3
    97FFFF DarkSlateGray1
    98F5FF CadetBlue1
    98FB98 PaleGreen
    98FB98 {pale green}
    9932CC DarkOrchid
    9932CC {dark orchid}
    999999 gray60
    999999 grey60
    9A32CD DarkOrchid3
    9AC0CD LightBlue3
    9ACD32 OliveDrab3
    9ACD32 YellowGreen
    9ACD32 {yellow green}
    9AFF9A PaleGreen1
    9B30FF purple1
    9BCD9B DarkSeaGreen3
    9C9C9C gray61
    9C9C9C grey61
    9E9E9E gray62
    9E9E9E grey62
    9F79EE MediumPurple2
    9FB6CD SlateGray3
    A020F0 purple
    A0522D sienna
    A1A1A1 gray63
    A1A1A1 grey63
    A2B5CD LightSteelBlue3
    A2CD5A DarkOliveGreen3
    A3A3A3 gray64
    A3A3A3 grey64
    A4D3EE LightSkyBlue2
    A52A2A brown
    A6A6A6 gray65
    A6A6A6 grey65
    A8A8A8 gray66
    A8A8A8 grey66
    A9A9A9 DarkGray
    A9A9A9 DarkGrey
    A9A9A9 {dark gray}
    A9A9A9 {dark grey}
    AB82FF MediumPurple1
    ABABAB gray67
    ABABAB grey67
    ADADAD gray68
    ADADAD grey68
    ADD8E6 LightBlue
    ADD8E6 {light blue}
    ADFF2F GreenYellow
    ADFF2F {green yellow}
    AEEEEE PaleTurquoise2
    AFEEEE PaleTurquoise
    AFEEEE {pale turquoise}
    B03060 maroon
    B0B0B0 gray69
    B0B0B0 grey69
    B0C4DE LightSteelBlue
    B0C4DE {light steel blue}
    B0E0E6 PowderBlue
    B0E0E6 {powder blue}
    B0E2FF LightSkyBlue1
    B22222 firebrick
    B23AEE DarkOrchid2
    B2DFEE LightBlue2
    B3B3B3 gray70
    B3B3B3 grey70
    B3EE3A OliveDrab2
    B452CD MediumOrchid3
    B4CDCD LightCyan3
    B4EEB4 DarkSeaGreen2
    B5B5B5 gray71
    B5B5B5 grey71
    B8860B DarkGoldenrod
    B8860B {dark goldenrod}
    B8B8B8 gray72
    B8B8B8 grey72
    B9D3EE SlateGray2
    BA55D3 MediumOrchid
    BA55D3 {medium orchid}
    BABABA gray73
    BABABA grey73
    BBFFFF PaleTurquoise1
    BC8F8F RosyBrown
    BC8F8F {rosy brown}
    BCD2EE LightSteelBlue2
    BCEE68 DarkOliveGreen2
    BDB76B DarkKhaki
    BDB76B {dark khaki}
    BDBDBD gray74
    BDBDBD grey74
    BEBEBE gray
    BEBEBE grey
    BF3EFF DarkOrchid1
    BFBFBF gray75
    BFBFBF grey75
    BFEFFF LightBlue1
    C0FF3E OliveDrab1
    C1CDC1 honeydew3
    C1CDCD azure3
    C1FFC1 DarkSeaGreen1
    C2C2C2 gray76
    C2C2C2 grey76
    C4C4C4 gray77
    C4C4C4 grey77
    C6E2FF SlateGray1
    C71585 MediumVioletRed
    C71585 {medium violet red}
    C7C7C7 gray78
    C7C7C7 grey78
    C9C9C9 gray79
    C9C9C9 grey79
    CAE1FF LightSteelBlue1
    CAFF70 DarkOliveGreen1
    CCCCCC gray80
    CCCCCC grey80
    CD0000 red3
    CD00CD magenta3
    CD1076 DeepPink3
    CD2626 firebrick3
    CD2990 maroon3
    CD3278 VioletRed3
    CD3333 brown3
    CD3700 OrangeRed3
    CD4F39 tomato3
    CD5555 IndianRed3
    CD5B45 coral3
    CD5C5C IndianRed
    CD5C5C {indian red}
    CD6090 HotPink3
    CD6600 DarkOrange3
    CD661D chocolate3
    CD6839 sienna3
    CD6889 PaleVioletRed3
    CD69C9 orchid3
    CD7054 salmon3
    CD8162 LightSalmon3
    CD8500 orange3
    CD853F peru
    CD853F tan3
    CD8C95 LightPink3
    CD919E pink3
    CD950C DarkGoldenrod3
    CD96CD plum3
    CD9B1D goldenrod3
    CD9B9B RosyBrown3
    CDAA7D burlywood3
    CDAD00 gold3
    CDAF95 PeachPuff3
    CDB38B NavajoWhite3
    CDB5CD thistle3
    CDB79E bisque3
    CDB7B5 MistyRose3
    CDBA96 wheat3
    CDBE70 LightGoldenrod3
    CDC0B0 AntiqueWhite3
    CDC1C5 LavenderBlush3
    CDC5BF seashell3
    CDC673 khaki3
    CDC8B1 cornsilk3
    CDC9A5 LemonChiffon3
    CDC9C9 snow3
    CDCD00 yellow3
    CDCDB4 LightYellow3
    CDCDC1 ivory3
    CFCFCF gray81
    CFCFCF grey81
    D02090 VioletRed
    D02090 {violet red}
    D15FEE MediumOrchid2
    D1D1D1 gray82
    D1D1D1 grey82
    D1EEEE LightCyan2
    D2691E chocolate
    D2B48C tan
    D3D3D3 LightGray
    D3D3D3 LightGrey
    D3D3D3 {light gray}
    D3D3D3 {light grey}
    D4D4D4 gray83
    D4D4D4 grey83
    D6D6D6 gray84
    D6D6D6 grey84
    D8BFD8 thistle
    D9D9D9 gray85
    D9D9D9 grey85
    DA70D6 orchid
    DAA520 goldenrod
    DB7093 PaleVioletRed
    DB7093 {pale violet red}
    DBDBDB gray86
    DBDBDB grey86
    DCDCDC gainsboro
    DDA0DD plum
    DEB887 burlywood
    DEDEDE gray87
    DEDEDE grey87
    E066FF MediumOrchid1
    E0E0E0 gray88
    E0E0E0 grey88
    E0EEE0 honeydew2
    E0EEEE azure2
    E0FFFF LightCyan
    E0FFFF LightCyan1
    E0FFFF {light cyan}
    E3E3E3 gray89
    E3E3E3 grey89
    E5E5E5 gray90
    E5E5E5 grey90
    E6E6FA lavender
    E8E8E8 gray91
    E8E8E8 grey91
    E9967A DarkSalmon
    E9967A {dark salmon}
    EBEBEB gray92
    EBEBEB grey92
    EDEDED gray93
    EDEDED grey93
    EE0000 red2
    EE00EE magenta2
    EE1289 DeepPink2
    EE2C2C firebrick2
    EE30A7 maroon2
    EE3A8C VioletRed2
    EE3B3B brown2
    EE4000 OrangeRed2
    EE5C42 tomato2
    EE6363 IndianRed2
    EE6A50 coral2
    EE6AA7 HotPink2
    EE7600 DarkOrange2
    EE7621 chocolate2
    EE7942 sienna2
    EE799F PaleVioletRed2
    EE7AE9 orchid2
    EE8262 salmon2
    EE82EE violet
    EE9572 LightSalmon2
    EE9A00 orange2
    EE9A49 tan2
    EEA2AD LightPink2
    EEA9B8 pink2
    EEAD0E DarkGoldenrod2
    EEAEEE plum2
    EEB422 goldenrod2
    EEB4B4 RosyBrown2
    EEC591 burlywood2
    EEC900 gold2
    EECBAD PeachPuff2
    EECFA1 NavajoWhite2
    EED2EE thistle2
    EED5B7 bisque2
    EED5D2 MistyRose2
    EED8AE wheat2
    EEDC82 LightGoldenrod2
    EEDD82 LightGoldenrod
    EEDD82 {light goldenrod}
    EEDFCC AntiqueWhite2
    EEE0E5 LavenderBlush2
    EEE5DE seashell2
    EEE685 khaki2
    EEE8AA PaleGoldenrod
    EEE8AA {pale goldenrod}
    EEE8CD cornsilk2
    EEE9BF LemonChiffon2
    EEE9E9 snow2
    EEEE00 yellow2
    EEEED1 LightYellow2
    EEEEE0 ivory2
    F08080 LightCoral
    F08080 {light coral}
    F0E68C khaki
    F0F0F0 gray94
    F0F0F0 grey94
    F0F8FF AliceBlue
    F0F8FF {alice blue}
    F0FFF0 honeydew
    F0FFF0 honeydew1
    F0FFFF azure
    F0FFFF azure1
    F2F2F2 gray95
    F2F2F2 grey95
    F4A460 SandyBrown
    F4A460 {sandy brown}
    F5DEB3 wheat
    F5F5DC beige
    F5F5F5 WhiteSmoke
    F5F5F5 gray96
    F5F5F5 grey96
    F5F5F5 {white smoke}
    F5FFFA MintCream
    F5FFFA {mint cream}
    F7F7F7 gray97
    F7F7F7 grey97
    F8F8FF GhostWhite
    F8F8FF {ghost white}
    FA8072 salmon
    FAEBD7 AntiqueWhite
    FAEBD7 {antique white}
    FAF0E6 linen
    FAFAD2 LightGoldenrodYellow
    FAFAD2 {light goldenrod yellow}
    FAFAFA gray98
    FAFAFA grey98
    FCFCFC gray99
    FCFCFC grey99
    FDF5E6 OldLace
    FDF5E6 {old lace}
    FF0000 red
    FF0000 red1
    FF00FF magenta
    FF00FF magenta1
    FF1493 DeepPink
    FF1493 DeepPink1
    FF1493 {deep pink}
    FF3030 firebrick1
    FF34B3 maroon1
    FF3E96 VioletRed1
    FF4040 brown1
    FF4500 OrangeRed
    FF4500 OrangeRed1
    FF4500 {orange red}
    FF6347 tomato
    FF6347 tomato1
    FF69B4 HotPink
    FF69B4 {hot pink}
    FF6A6A IndianRed1
    FF6EB4 HotPink1
    FF7256 coral1
    FF7F00 DarkOrange1
    FF7F24 chocolate1
    FF7F50 coral
    FF8247 sienna1
    FF82AB PaleVioletRed1
    FF83FA orchid1
    FF8C00 DarkOrange
    FF8C00 {dark orange}
    FF8C69 salmon1
    FFA07A LightSalmon
    FFA07A LightSalmon1
    FFA07A {light salmon}
    FFA500 orange
    FFA500 orange1
    FFA54F tan1
    FFAEB9 LightPink1
    FFB5C5 pink1
    FFB6C1 LightPink
    FFB6C1 {light pink}
    FFB90F DarkGoldenrod1
    FFBBFF plum1
    FFC0CB pink
    FFC125 goldenrod1
    FFC1C1 RosyBrown1
    FFD39B burlywood1
    FFD700 gold
    FFD700 gold1
    FFDAB9 PeachPuff
    FFDAB9 PeachPuff1
    FFDAB9 {peach puff}
    FFDEAD NavajoWhite
    FFDEAD NavajoWhite1
    FFDEAD {navajo white}
    FFE1FF thistle1
    FFE4B5 moccasin
    FFE4C4 bisque
    FFE4C4 bisque1
    FFE4E1 MistyRose
    FFE4E1 MistyRose1
    FFE4E1 {misty rose}
    FFE7BA wheat1
    FFEBCD BlanchedAlmond
    FFEBCD {blanched almond}
    FFEC8B LightGoldenrod1
    FFEFD5 PapayaWhip
    FFEFD5 {papaya whip}
    FFEFDB AntiqueWhite1
    FFF0F5 LavenderBlush
    FFF0F5 LavenderBlush1
    FFF0F5 {lavender blush}
    FFF5EE seashell
    FFF5EE seashell1
    FFF68F khaki1
    FFF8DC cornsilk
    FFF8DC cornsilk1
    FFFACD LemonChiffon
    FFFACD LemonChiffon1
    FFFACD {lemon chiffon}
    FFFAF0 FloralWhite
    FFFAF0 {floral white}
    FFFAFA snow
    FFFAFA snow1
    FFFF00 yellow
    FFFF00 yellow1
    FFFFE0 LightYellow
    FFFFE0 LightYellow1
    FFFFE0 {light yellow}
    FFFFF0 ivory
    FFFFF0 ivory1
    FFFFFF gray100
    FFFFFF grey100
    FFFFFF white
}



set colors {
#DCFFD8
#FFD0D0
#D4D5FF
#FFFFC9
#D6FBFF
#EDD3FF
#E7FFCF
#FFEAC3
#D9E5FF
#C4FFE0
#FFD5ED
#FFDBA8
#B5CFFF
#FF7684
#85FF9B
white
#CEFFF8
#FFDFD0
#D0FFCF
#AFFFFF
#FFFFC2
#FFE0F4
#D8E9FF
#ECD5FF
#D2FFCA
#FFF9C4
#EFFFD8
	#a2b5cd
	#7ac5cd
	#66cdaa
	#a2cd5a
	#cd9b9b
	#cdba96
	#cd3333
	#cd6600
	#cd8c95
	#cd00cd
	#9a32cd
	#6ca6cd
	#9ac0cd
	#9bcd9b
	#00cd66
	#cdc673
	#cdad00
	#cd5555
	#cd853f
	#cd7054
	#cd5b45
	#cd6889
	#cd69c9
	#551a8b
}

if { [file exists ../library] } {
    set blt_library ../library
}

set bg [blt::background create linear -lowcolor  grey100 -highcolor grey90 \
	-jitter 20 -colorscale log]
set bg white
set myIcon ""
blt::comboentry .e \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -textbackground $bg \
    -menu .e.m \
    -exportselection yes 


blt::combomenu .e.m  \
    -textvariable myText1 \
    -iconvariable myIcon1 \
    -height { 0 400 } \
    -width  { 0 400 } \
    -background $bg \
    -yscrollbar .e.m.ybar \
    -xscrollbar .e.m.xbar

blt::tk::scrollbar .e.m.xbar 
blt::tk::scrollbar .e.m.ybar 

set bg [image create picture -width 30 -height 20]
$bg blank 0x0000000
$bg draw rectangle 5 5 -width 13 -height 25 -color 0xFF00000
$bg blur $bg 4

if 1 {
foreach {rgb name} $rgbNames {
    set icon [image create picture -width 27 -height 27]
    $icon blank 0x0
    $icon draw circle 12 12 11 -color black \
    	-antialiased 1 -linewidth 0 -shadow { -width 1 -offset 2 }
    $icon draw circle 12 12 10 -color \#$rgb \
    	-antialiased 1 -linewidth 0
    .e.m add -text $name -icon $icon
}
} else {
foreach {rgb} $colors {
    set icon [image create picture -width 27 -height 27]
    $icon blank 0x0
    $icon draw circle 12 12 11 -color black \
    	-antialiased 1 -linewidth 0 -shadow { -width 1 -offset 2 }
    $icon draw circle 12 12 10 -color $rgb \
    	-antialiased 1 -linewidth 0
    .e.m add -text $rgb -icon $icon
}
}
button .quit -text "Exit" -command exit
blt::table . \
    0,0 .e -fill both -padx 2 -pady 2  \
    1,0 .quit 
