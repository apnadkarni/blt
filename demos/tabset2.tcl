#!../src/bltwish

package require BLT

image create picture label2 -data {
    AAEBAABZACAAAAAAFgAWAAgIAAAAAACPrf8Aj5j/AOn+/wDC//8AJy3kADpE+gAo
    LOMA5///AN7//wAFCLEANkT6AMD8/wDj//8AAAAKAJCt/wDa//8AkJj/ACgt5AAS
    FN4AHynzAAAAEQAAAFYAyOz/AJWt/wDO+P8AAAAGAISP/wAAAF0AN0T6ANb//wA7
    QfgAJC3kAAAAUgDa+f8AAABvAMz3/wAAAGQAka3/AOv+/wDD9/8AKDb6ACUx7wA8
    RPoAKS3kAAAAWQDS//8Awfj/AMj3/wA6Q/oAm6//AML+/wCNmP8AJS3kADhE+gAY
    H9sAzff/AM7//wBha/8Akq3/AJKY/wDz//8APUT6AAAADAB7mP8AKi3kANf7/wAa
    I+4ANET6AJet/wDK//8ANEL6AC1B+gDv//8Ajpj/ACYt5AA5RPoABwm5AAAAfADO
    9/8Ak63/AMH9/wA+RPoAKy3kAOv//wB9i/8AAABQACIt5ADK9/8AAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAjFCkpKSkpKSkpKSkp
    KSkpKSkpQxwAKjMEBAQEBAQEBAQEBAQEBAQEBAw3AERRUVFRUVFRUVFRUVFRUVFR
    UVFRVwALRkZGRkZGRkZGRkZGRkZGRkZGRiAAHTk5OTk5OTk5OTk5OTk5OTk5OTk1
    ADYuLi4uLi4uLi4uLi4uLi4uLi4uNQBMHh4eHh4eHh4eHh4eHh4eHh4eHksABhAQ
    EBAQEBAQEBAQEBAQEBAQEBAFACsJCQkJCQkJCQkJCQkJCQkJCQkJEgA+DQ0NDQ0N
    DQ0NDQ0NDQ0NDQ0NDRIAUggICAgICAgICAgICAgICAgICAgsAFJUVFRUVFRUVFRU
    VFRUVFRUVFRUQQBIKDBYJE9POCQZJ0lJSUlJSUlJSVMARy8vLy8vLy8vLyI9PT09
    PT09PT0HADFCQkJCQkJCQkIXMkUYUDsmDwFACgAfAwMDAwMDAwMDOk4lJSUlJSUl
    LQ4ATVU0ETw8EQJKGxMaAAAAAAAAAAAAAD9WFhYWFhYWFiEVAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA7wEAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUAHgDbBwsACgAvAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAWgMAAAAAAABUUlVFVklTSU9OLVhGSUxFLgA=
}

image create picture bgTile -file ./images/smblue_rock.gif
image create picture label1 -file ./images/mini-book1.gif
image create picture testImage -file ./images/txtrflag.gif
blt::tk::scrollbar .s -command { .t view } -orient horizontal
blt::tabset .t \
    -outerrelief flat \
    -font "Arial 8" \
    -outerborderwidth 2 \
    -tearoff yes \
    -iconposition left \
    -tabwidth same \
    -tiers 2 \
    -scrollcommand { .s set } \
    -scrollincrement 1 

blt::tk::label .t.l -image testImage

set attributes {
    graph1 "Graph \#1" red	.t.graph1  
    graph2 "Graph \#2" green	.t.graph2  
    graph3 "Graph \#3" cyan	.t.graph3  
    graph5 "Graph \#5" yellow	.t.graph5  
    graph6 one		orange	.t.l       
}

foreach { entry label color window } $attributes {
    .t insert end $entry -text $label -fill both -ipady 4
}

foreach page { there bunky another test of a widget } {
    .t insert end $page -image label2  -ipady 4
}

blt::table . \
    .t 0,0 -fill both \
    .s 1,0 -fill x 

blt::table configure . r1 -resize none

foreach file { graph1 graph2 graph3 graph5 } {
    namespace eval $file {
	set graph [blt::graph .t.$file]
	source scripts/$file.tcl
	.t tab configure $file -window $graph 
    }
}

