package require Tk
package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test background.1 {background no args} {
    list [catch {blt::background} msg] $msg
} {1 {wrong # args: should be one of...
  blt::background cget bgName option
  blt::background configure bgName ?option value ...?
  blt::background create type ?bgName? ?option value ...?
  blt::background delete ?bgName ...?
  blt::background exists bgName
  blt::background names ?pattern?
  blt::background type bgName}}

test background.2 {background badOp} {
    list [catch {blt::background badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  blt::background cget bgName option
  blt::background configure bgName ?option value ...?
  blt::background create type ?bgName? ?option value ...?
  blt::background delete ?bgName ...?
  blt::background exists bgName
  blt::background names ?pattern?
  blt::background type bgName}}

test background.3 {background create} {
    list [catch {blt::background create} msg] $msg
} {1 {wrong # args: should be "blt::background create type ?bgName? ?option value ...?"}}

test background.4 {background create badType} {
    list [catch {blt::background create badType} msg] $msg
} {1 {unknown background type "badType"}}

test background.5 {background create linear} {
    list [catch {blt::background create linear} msg] $msg
} {0 background1}

test background.6 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 background1}

test background.7 {background type background1} {
    list [catch {blt::background type background1} msg] $msg
} {0 linear}

test background.8 {background exists} {
    list [catch {blt::background exists background1} msg] $msg
} {0 1}

test background.9 {background configure background1} {
    list [catch {blt::background configure background1} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-from {} {} {top center} {0.5 0.0}} {-highcolor {} {} grey90 #e5e5e5} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-palette {} {} {} {}} {-repeat {} {} no no} {-to {} {} {} {0.5 1.0}} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.10 {background cget} {
    list [catch {blt::background cget} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.11 {background cget background1} {
    list [catch {blt::background cget background1} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.12 {background cget background1 -badOption} {
    list [catch {blt::background cget background1 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test background.13 {background cget background1 -background} {
    list [catch {blt::background cget background1 -background} msg] $msg
} {0 #d9d9d9}

test background.14 {background cget background1 -bg} {
    list [catch {blt::background cget background1 -bg} msg] $msg
} {0 #d9d9d9}

test background.15 {background cget background1 -border} {
    list [catch {blt::background cget background1 -border} msg] $msg
} {0 #d9d9d9}

test background.16 {background cget background1 -relativeto} {
    list [catch {blt::background cget background1 -relativeto} msg] $msg
} {0 toplevel}

test background.17 {background cget background1 -colorscale} {
    list [catch {blt::background cget background1 -colorscale} msg] $msg
} {0 linear}

test background.18 {background cget background1 -decreasing} {
    list [catch {blt::background cget background1 -decreasing} msg] $msg
} {0 0}

test background.19 {background cget background1 -from} {
    list [catch {blt::background cget background1 -from} msg] $msg
} {0 {0.5 0.0}}

test background.20 {background cget background1 -highcolor} {
    list [catch {blt::background cget background1 -highcolor} msg] $msg
} {0 #e5e5e5}

test background.21 {background cget background1 -jitter} {
    list [catch {blt::background cget background1 -jitter} msg] $msg
} {0 0.0}

test background.22 {background cget background1 -lowcolor} {
    list [catch {blt::background cget background1 -lowcolor} msg] $msg
} {0 #7f7f7f}

test background.23 {background cget background1 -palette} {
    list [catch {blt::background cget background1 -palette} msg] $msg
} {0 {}}

test background.24 {background cget background1 -repeat} {
    list [catch {blt::background cget background1 -repeat} msg] $msg
} {0 no}

test background.25 {background cget background1 -to} {
    list [catch {blt::background cget background1 -to} msg] $msg
} {0 {0.5 1.0}}

test background.26 {background cget background1 -xoffset} {
    list [catch {blt::background cget background1 -xoffset} msg] $msg
} {0 0}

test background.27 {background cget background1 -yoffset} {
    list [catch {blt::background cget background1 -yoffset} msg] $msg
} {0 0}

test background.28 {background cget background1} {
    list [catch {blt::background cget background1} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.29 {background delete} {
    list [catch {blt::background delete} msg] $msg
} {0 {}}

test background.30 {background delete badName} {
    list [catch {blt::background delete badName} msg] $msg
} {1 {can't find background "badName"}}

test background.31 {background delete background1} {
    list [catch {blt::background delete background1} msg] $msg
} {0 {}}

test background.32 {background exists background1} {
    list [catch {blt::background exists background1} msg] $msg
} {0 0}

test background.33 {background delete all backgrounds} {
    list [catch {eval blt::background delete [blt::background names]} msg] $msg
} {0 {}}

test background.34 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 {}}

test background.35 {background create linear} {
    list [catch {blt::background create linear myBackground} msg] $msg
} {0 myBackground}

test background.36 {background exists} {
    list [catch {blt::background exists myBackground} msg] $msg
} {0 1}

test background.37 {background type} {
    list [catch {blt::background type myBackground} msg] $msg
} {0 linear}

test background.38 {background configure myBackground} {
    list [catch {blt::background configure myBackground} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-from {} {} {top center} {0.5 0.0}} {-highcolor {} {} grey90 #e5e5e5} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-palette {} {} {} {}} {-repeat {} {} no no} {-to {} {} {} {0.5 1.0}} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}


test background.39 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.40 {background configure -background badColor} {
    list [catch {
	blt::background configure myBackground -background badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.41 {background configure -background green} {
    list [catch {
	blt::background configure myBackground -background green
    } msg] $msg
} {0 {}}

test background.42 {background configure -background myBackground} {
    list [catch {
	blt::background configure myBackground -background myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.43 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.44 {background configure -background #d9d9d9} {
    list [catch {
	blt::background configure myBackground -background #d9d9d9
    } msg] $msg
} {0 {}}

test background.45 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.46 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.47 {background configure -bg badColor} {
    list [catch {
	blt::background configure myBackground -bg badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.48 {background configure -bg green} {
    list [catch {
	blt::background configure myBackground -bg green
    } msg] $msg
} {0 {}}

test background.49 {background configure -bg myBackground} {
    list [catch {
	blt::background configure myBackground -bg myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.50 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.51 {background configure -bg #d9d9d9} {
    list [catch {
	blt::background configure myBackground -bg #d9d9d9
    } msg] $msg
} {0 {}}

test background.52 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.53 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.54 {background configure -border badColor} {
    list [catch {
	blt::background configure myBackground -border badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.55 {background configure -border green} {
    list [catch {
	blt::background configure myBackground -border green
    } msg] $msg
} {0 {}}

test background.56 {background configure -border myBackground} {
    list [catch {
	blt::background configure myBackground -border myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.57 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.58 {background configure -border #d9d9d9} {
    list [catch {
	blt::background configure myBackground -border #d9d9d9
    } msg] $msg
} {0 {}}

test background.59 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.60 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test background.61 {background configure -colorscale badScale} {
    list [catch {
	blt::background configure myBackground -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test background.62 {background configure -colorscale linear} {
    list [catch {
	blt::background configure myBackground -colorscale linear
    } msg] $msg
} {0 {}}

test background.63 {background configure -colorscale log} {
    list [catch {
	blt::background configure myBackground -colorscale log
    } msg] $msg
} {0 {}}

test background.64 {background configure -colorscale badScale} {
    list [catch {
	blt::background configure myBackground -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test background.65 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear logarithmic}}

test background.66 {background configure -colorscale linear} {
    list [catch {
	blt::background configure myBackground -colorscale linear
    } msg] $msg
} {0 {}}

test background.67 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test background.68 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.69 {background configure -decreasing badValue} {
    list [catch {
	blt::background configure myBackground -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test background.70 {background configure -decreasing 0} {
    list [catch {
	blt::background configure myBackground -decreasing 0
    } msg] $msg
} {0 {}}

test background.71 {background configure -decreasing 1} {
    list [catch {
	blt::background configure myBackground -decreasing 1
    } msg] $msg
} {0 {}}

test background.72 {background configure -decreasing no} {
    list [catch {
	blt::background configure myBackground -decreasing no
    } msg] $msg
} {0 {}}

test background.73 {background configure -decreasing yes} {
    list [catch {
	blt::background configure myBackground -decreasing yes
    } msg] $msg
} {0 {}}

test background.74 {background configure -decreasing on} {
    list [catch {
	blt::background configure myBackground -decreasing on
    } msg] $msg
} {0 {}}

test background.75 {background configure -decreasing off} {
    list [catch {
	blt::background configure myBackground -decreasing off
    } msg] $msg
} {0 {}}

test background.76 {background configure -decreasing true} {
    list [catch {
	blt::background configure myBackground -decreasing true
    } msg] $msg
} {0 {}}

test background.77 {background configure -decreasing false} {
    list [catch {
	blt::background configure myBackground -decreasing false
    } msg] $msg
} {0 {}}

test background.78 {background configure -decreasing badValue} {
    list [catch {
	blt::background configure myBackground -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test background.79 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.80 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.81 {background configure -from} {
    list [catch {
	blt::background configure myBackground -from
    } msg] $msg
} {0 {-from {} {} {top center} {0.5 0.0}}}

test background.82 {background configure -from badValue} {
    list [catch {
	blt::background configure myBackground -from badValue
    } msg] $msg
} {1 {unknown position "badValue": should be nw, n, ne, w, c, e, sw, s, or se.}}

test background.83 {background configure -from {badValue badValue}} {
    list [catch {
	blt::background configure myBackground -from {badValue badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test background.84 {background configure -from {badValue center}} {
    list [catch {
	blt::background configure myBackground -from {badValue center}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test background.85 {background configure -from {center badValue}} {
    list [catch {
	blt::background configure myBackground -from {center badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be left, right, or center.}}

test background.86 {background configure -from {0.0 1.0}} {
    list [catch {
	blt::background configure myBackground -from {0.0 1.0}
    } msg] $msg
} {0 {}}

test background.87 {background configure -from {2000.0 -1000.0}} {
    list [catch {
	blt::background configure myBackground -from {2000.0 -1000.0}
    } msg] $msg
} {0 {}}

test background.88 {background configure -from nw} {
    list [catch {
	blt::background configure myBackground -from nw
    } msg] $msg
} {0 {}}

test background.89 {background configure -from n} {
    list [catch {
	blt::background configure myBackground -from n
    } msg] $msg
} {0 {}}

test background.90 {background configure -from ne} {
    list [catch {
	blt::background configure myBackground -from ne
    } msg] $msg
} {0 {}}

test background.91 {background configure -from sw} {
    list [catch {
	blt::background configure myBackground -from s
    } msg] $msg
} {0 {}}

test background.92 {background configure -from s} {
    list [catch {
	blt::background configure myBackground -from s
    } msg] $msg
} {0 {}}

test background.93 {background configure -from se} {
    list [catch {
	blt::background configure myBackground -from se
    } msg] $msg
} {0 {}}

test background.94 {background configure -from c} {
    list [catch {
	blt::background configure myBackground -from c
    } msg] $msg
} {0 {}}

test background.95 {background configure -from {top left}} {
    list [catch {
	blt::background configure myBackground -from {top left}
    } msg] $msg
} {0 {}}

test background.96 {background configure -from {top right}} {
    list [catch {
	blt::background configure myBackground -from {top right}
    } msg] $msg
} {0 {}}

test background.97 {background configure -from {top center}} {
    list [catch {
	blt::background configure myBackground -from {top center}
    } msg] $msg
} {0 {}}

test background.98 {background configure -from {bottom left}} {
    list [catch {
	blt::background configure myBackground -from {bottom left}
    } msg] $msg
} {0 {}}

test background.99 {background configure -from {bottom right}} {
    list [catch {
	blt::background configure myBackground -from {bottom right}
    } msg] $msg
} {0 {}}

test background.100 {background configure -from {bottom center}} {
    list [catch {
	blt::background configure myBackground -from {bottom center}
    } msg] $msg
} {0 {}}

test background.101 {background configure -from {center left}} {
    list [catch {
	blt::background configure myBackground -from {center left}
    } msg] $msg
} {0 {}}

test background.102 {background configure -from {center right}} {
    list [catch {
	blt::background configure myBackground -from {center right}
    } msg] $msg
} {0 {}}

test background.103 {background configure -from {center center}} {
    list [catch {
	blt::background configure myBackground -from {center center}
    } msg] $msg
} {0 {}}

test background.104 {background configure -from {top center}} {
    list [catch {
	blt::background configure myBackground -from {top center}
    } msg] $msg
} {0 {}}

test background.105 {background configure -from {center center center}} {
    list [catch {
	blt::background configure myBackground -from {center center center}
    } msg] $msg
} {1 {unknown position "center center center": should be "top left" or "nw"}}


test background.106 {background configure -from} {
    list [catch {
	blt::background configure myBackground -from
    } msg] $msg
} {0 {-from {} {} {top center} {0.5 0.0}}}


test background.107 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #e5e5e5}}

test background.108 {background configure -highcolor badColor} {
    list [catch {
	blt::background configure myBackground -highcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.109 {background configure -highcolor green} {
    list [catch {
	blt::background configure myBackground -highcolor green
    } msg] $msg
} {0 {}}

test background.110 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test background.111 {background configure -highcolor myBackground} {
    list [catch {
	blt::background configure myBackground -highcolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.112 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test background.113 {background configure -highcolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.114 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.115 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test background.116 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.117 {background configure -jitter -1} {
    list [catch {
	blt::background configure myBackground -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test background.118 {background configure -jitter 101} {
    list [catch {
	blt::background configure myBackground -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test background.119 {background configure -jitter 1.1} {
    list [catch {
	blt::background configure myBackground -jitter 1.1
    } msg] $msg
} {0 {}}

test background.120 {background configure -jitter 99.99999999} {
    list [catch {
	blt::background configure myBackground -jitter 99.9999999999
    } msg] $msg
} {0 {}}


test background.121 {background configure -jitter 0} {
    list [catch {
	blt::background configure myBackground -jitter 0
    } msg] $msg
} {0 {}}

test background.122 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.123 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.124 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.125 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #7f7f7f}}

test background.126 {background configure -lowcolor badColor} {
    list [catch {
	blt::background configure myBackground -lowcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.127 {background configure -lowcolor green} {
    list [catch {
	blt::background configure myBackground -lowcolor green
    } msg] $msg
} {0 {}}

test background.128 {background configure -lowcolor myBackground} {
    list [catch {
	blt::background configure myBackground -lowcolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.129 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #00ff00}}

test background.130 {background configure -lowcolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -lowcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.131 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor 
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #d9d9d9}}

test background.132 {background configure -palette} {
    list [catch {
	blt::background configure myBackground -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test background.133 {background configure -palette badPalette} {
    list [catch {
	blt::background configure myBackground -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test background.134 {background configure -palette ""} {
    list [catch {
	blt::background configure myBackground -palette ""
    } msg] $msg
} {0 {}}

test background.135 {background configure -palette myPalette} {
    list [catch {
	blt::palette create myPalette -colordata {black white} -colorformat name
	blt::background configure myBackground -palette myPalette
	blt::palette delete myPalette
    } msg] $msg
} {0 {}}

test background.136 {background configure -palette ""} {
    list [catch {
	blt::background configure myBackground -palette ""
    } msg] $msg
} {0 {}}

test background.137 {background configure -palette badPalette} {
    list [catch {
	blt::background configure myBackground -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test background.138 {background configure -palette} {
    list [catch {
	blt::background configure myBackground -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test background.139 {background configure -to} {
    list [catch {
	blt::background configure myBackground -to
    } msg] $msg
} {0 {-to {} {} {} {0.5 1.0}}}

test background.140 {background configure -to badValue} {
    list [catch {
	blt::background configure myBackground -to badValue
    } msg] $msg
} {1 {unknown position "badValue": should be nw, n, ne, w, c, e, sw, s, or se.}}

test background.141 {background configure -to {badValue badValue}} {
    list [catch {
	blt::background configure myBackground -to {badValue badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test background.142 {background configure -to {badValue center}} {
    list [catch {
	blt::background configure myBackground -to {badValue center}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test background.143 {background configure -to {center badValue}} {
    list [catch {
	blt::background configure myBackground -to {center badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be left, right, or center.}}

test background.144 {background configure -to {0.0 1.0}} {
    list [catch {
	blt::background configure myBackground -to {0.0 1.0}
    } msg] $msg
} {0 {}}

test background.145 {background configure -to {2000.0 -1000.0}} {
    list [catch {
	blt::background configure myBackground -to {2000.0 -1000.0}
    } msg] $msg
} {0 {}}

test background.146 {background configure -to nw} {
    list [catch {
	blt::background configure myBackground -to nw
    } msg] $msg
} {0 {}}

test background.147 {background configure -to n} {
    list [catch {
	blt::background configure myBackground -to n
    } msg] $msg
} {0 {}}

test background.148 {background configure -to ne} {
    list [catch {
	blt::background configure myBackground -to ne
    } msg] $msg
} {0 {}}

test background.149 {background configure -to sw} {
    list [catch {
	blt::background configure myBackground -to s
    } msg] $msg
} {0 {}}

test background.150 {background configure -to s} {
    list [catch {
	blt::background configure myBackground -to s
    } msg] $msg
} {0 {}}

test background.151 {background configure -to se} {
    list [catch {
	blt::background configure myBackground -to se
    } msg] $msg
} {0 {}}

test background.152 {background configure -to c} {
    list [catch {
	blt::background configure myBackground -to c
    } msg] $msg
} {0 {}}

test background.153 {background configure -to {top left}} {
    list [catch {
	blt::background configure myBackground -to {top left}
    } msg] $msg
} {0 {}}

test background.154 {background configure -to {top right}} {
    list [catch {
	blt::background configure myBackground -to {top right}
    } msg] $msg
} {0 {}}

test background.155 {background configure -to {top center}} {
    list [catch {
	blt::background configure myBackground -to {top center}
    } msg] $msg
} {0 {}}

test background.156 {background configure -to {bottom left}} {
    list [catch {
	blt::background configure myBackground -to {bottom left}
    } msg] $msg
} {0 {}}

test background.157 {background configure -to {bottom right}} {
    list [catch {
	blt::background configure myBackground -to {bottom right}
    } msg] $msg
} {0 {}}

test background.158 {background configure -to {bottom center}} {
    list [catch {
	blt::background configure myBackground -to {bottom center}
    } msg] $msg
} {0 {}}

test background.159 {background configure -to {center left}} {
    list [catch {
	blt::background configure myBackground -to {center left}
    } msg] $msg
} {0 {}}

test background.160 {background configure -to {center right}} {
    list [catch {
	blt::background configure myBackground -to {center right}
    } msg] $msg
} {0 {}}

test background.161 {background configure -to {center center}} {
    list [catch {
	blt::background configure myBackground -to {center center}
    } msg] $msg
} {0 {}}

test background.162 {background configure -to {top center}} {
    list [catch {
	blt::background configure myBackground -to {top center}
    } msg] $msg
} {0 {}}

test background.163 {background configure -to {center center center}} {
    list [catch {
	blt::background configure myBackground -to {center center center}
    } msg] $msg
} {1 {unknown position "center center center": should be "top left" or "nw"}}

test background.164 {background configure -to} {
    list [catch {
	blt::background configure myBackground -to
    } msg] $msg
} {0 {-to {} {} {} {0.5 0.0}}}

test background.165 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.166 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.167 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.168 {background configure -xoffset 1} {
    list [catch {
	blt::background configure myBackground -xoffset 1
    } msg] $msg
} {0 {}}

test background.169 {background configure -xoffset 1i} {
    list [catch {
	blt::background configure myBackground -xoffset 1i
    } msg] $msg
} {0 {}}

test background.170 {background configure -xoffset -10} {
    list [catch {
	blt::background configure myBackground -xoffset -10
    } msg] $msg
} {0 {}}

test background.171 {background configure -xoffset 10c} {
    list [catch {
	blt::background configure myBackground -xoffset 10c
    } msg] $msg
} {0 {}}

test background.172 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.173 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.174 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.175 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.176 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.177 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.178 {background configure -yoffset 1} {
    list [catch {
	blt::background configure myBackground -yoffset 1
    } msg] $msg
} {0 {}}

test background.179 {background configure -yoffset 1i} {
    list [catch {
	blt::background configure myBackground -yoffset 1i
    } msg] $msg
} {0 {}}

test background.180 {background configure -yoffset -10} {
    list [catch {
	blt::background configure myBackground -yoffset -10
    } msg] $msg
} {0 {}}

test background.181 {background configure -yoffset 10c} {
    list [catch {
	blt::background configure myBackground -yoffset 10c
    } msg] $msg
} {0 {}}

test background.182 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.183 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.184 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.185 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.186 {background configure -relativeto badColor} {
    list [catch {
	blt::background configure myBackground -relativeto badRef
    } msg] $msg
} {1 {unknown reference type "badRef"}}

test background.187 {background configure -relativeto self} {
    list [catch {
	blt::background configure myBackground -relativeto self
    } msg] $msg
} {0 {}}

test background.188 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.189 {background configure -relativeto .} {
    list [catch {
	blt::background configure myBackground -relativeto .
    } msg] $msg
} {0 {}}

test background.190 {background configure -relativeto .badWindow} {
    list [catch {
	blt::background configure myBackground -relativeto .badWindow
    } msg] $msg
} {0 {}}

test background.191 {background configure -relativeto .badWindow} {
    list [catch {
	blt::tk::label .test -background myBackground
	proc tkerror { mesg } {
	    global testmsg
	    set testmsg $mesg
	}
	wm withdraw .
	update
	destroy .test
	set testmsg
    } msg] $msg
} {0 {bad window path name ".badWindow"}}

test background.192 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel .badWindow}}

test background.193 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.194 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.195 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 myBackground}

test background.196 {background delete myBackground} {
    list [catch {blt::background delete myBackground} msg] $msg
} {0 {}}


#####

test background.197 {background create radial} {
    list [catch {blt::background create radial} msg] $msg
} {0 background2}

test background.198 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 background2}

test background.199 {background type background2} {
    list [catch {blt::background type background2} msg] $msg
} {0 radial}

test background.200 {background exists} {
    list [catch {blt::background exists background2} msg] $msg
} {0 1}

test background.201 {background configure background2} {
    list [catch {blt::background configure background2} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-center {} {} c {0.5 0.5}} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-diameter {} {} 0.0 0.0} {-highcolor {} {} grey90 #e5e5e5} {-height {} {} 1.0 1.0} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-palette {} {} {} {}} {-repeat {} {} no no} {-width {} {} 1.0 1.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.202 {background cget} {
    list [catch {blt::background cget} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.203 {background cget background2} {
    list [catch {blt::background cget background2} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.204 {background cget background2 -badOption} {
    list [catch {blt::background cget background2 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test background.205 {background cget background2 -background} {
    list [catch {blt::background cget background2 -background} msg] $msg
} {0 #d9d9d9}

test background.206 {background cget background2 -bg} {
    list [catch {blt::background cget background2 -bg} msg] $msg
} {0 #d9d9d9}

test background.207 {background cget background2 -border} {
    list [catch {blt::background cget background2 -border} msg] $msg
} {0 #d9d9d9}

test background.208 {background cget background2 -relativeto} {
    list [catch {blt::background cget background2 -relativeto} msg] $msg
} {0 toplevel}

test background.209 {background cget background2 -colorscale} {
    list [catch {blt::background cget background2 -colorscale} msg] $msg
} {0 linear}

test background.210 {background cget background2 -decreasing} {
    list [catch {blt::background cget background2 -decreasing} msg] $msg
} {0 0}

test background.211 {background cget background2 -center} {
    list [catch {blt::background cget background2 -center} msg] $msg
} {0 {0.5 0.5}}

test background.212 {background cget background2 -highcolor} {
    list [catch {blt::background cget background2 -highcolor} msg] $msg
} {0 #e5e5e5}

test background.213 {background cget background2 -jitter} {
    list [catch {blt::background cget background2 -jitter} msg] $msg
} {0 0.0}

test background.214 {background cget background2 -lowcolor} {
    list [catch {blt::background cget background2 -lowcolor} msg] $msg
} {0 #7f7f7f}

test background.215 {background cget background2 -palette} {
    list [catch {blt::background cget background2 -palette} msg] $msg
} {0 {}}

test background.216 {background cget background2 -repeat} {
    list [catch {blt::background cget background2 -repeat} msg] $msg
} {0 no}

test background.217 {background cget background2 -xoffset} {
    list [catch {blt::background cget background2 -xoffset} msg] $msg
} {0 0}

test background.218 {background cget background2 -yoffset} {
    list [catch {blt::background cget background2 -yoffset} msg] $msg
} {0 0}

test background.219 {background cget background2} {
    list [catch {blt::background cget background2} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.220 {background delete} {
    list [catch {blt::background delete} msg] $msg
} {0 {}}

test background.221 {background delete badName} {
    list [catch {blt::background delete badName} msg] $msg
} {1 {can't find background "badName"}}

test background.222 {background delete background2} {
    list [catch {blt::background delete background2} msg] $msg
} {0 {}}

test background.223 {background exists background2} {
    list [catch {blt::background exists background2} msg] $msg
} {0 0}

test background.224 {background delete all backgrounds} {
    list [catch {eval blt::background delete [blt::background names]} msg] $msg
} {0 {}}

test background.225 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 {}}

test background.226 {background create radial} {
    list [catch {blt::background create radial myBackground} msg] $msg
} {0 myBackground}

test background.227 {background exists} {
    list [catch {blt::background exists myBackground} msg] $msg
} {0 1}

test background.228 {background type} {
    list [catch {blt::background type myBackground} msg] $msg
} {0 radial}

test background.229 {background configure myBackground} {
    list [catch {blt::background configure myBackground} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-center {} {} c {0.5 0.5}} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-diameter {} {} 0.0 0.0} {-highcolor {} {} grey90 #e5e5e5} {-height {} {} 1.0 1.0} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-palette {} {} {} {}} {-repeat {} {} no no} {-width {} {} 1.0 1.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.230 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.231 {background configure -background badColor} {
    list [catch {
	blt::background configure myBackground -background badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.232 {background configure -background green} {
    list [catch {
	blt::background configure myBackground -background green
    } msg] $msg
} {0 {}}

test background.233 {background configure -background myBackground} {
    list [catch {
	blt::background configure myBackground -background myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.234 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.235 {background configure -background #d9d9d9} {
    list [catch {
	blt::background configure myBackground -background #d9d9d9
    } msg] $msg
} {0 {}}

test background.236 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.237 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.238 {background configure -bg badColor} {
    list [catch {
	blt::background configure myBackground -bg badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.239 {background configure -bg green} {
    list [catch {
	blt::background configure myBackground -bg green
    } msg] $msg
} {0 {}}

test background.240 {background configure -bg myBackground} {
    list [catch {
	blt::background configure myBackground -bg myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.241 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.242 {background configure -bg #d9d9d9} {
    list [catch {
	blt::background configure myBackground -bg #d9d9d9
    } msg] $msg
} {0 {}}

test background.243 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.244 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.245 {background configure -border badColor} {
    list [catch {
	blt::background configure myBackground -border badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.246 {background configure -border green} {
    list [catch {
	blt::background configure myBackground -border green
    } msg] $msg
} {0 {}}

test background.247 {background configure -border myBackground} {
    list [catch {
	blt::background configure myBackground -border myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.248 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.249 {background configure -border #d9d9d9} {
    list [catch {
	blt::background configure myBackground -border #d9d9d9
    } msg] $msg
} {0 {}}

test background.250 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.251 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test background.252 {background configure -colorscale badScale} {
    list [catch {
	blt::background configure myBackground -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test background.253 {background configure -colorscale linear} {
    list [catch {
	blt::background configure myBackground -colorscale linear
    } msg] $msg
} {0 {}}

test background.254 {background configure -colorscale log} {
    list [catch {
	blt::background configure myBackground -colorscale log
    } msg] $msg
} {0 {}}

test background.255 {background configure -colorscale badScale} {
    list [catch {
	blt::background configure myBackground -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test background.256 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear logarithmic}}

test background.257 {background configure -colorscale linear} {
    list [catch {
	blt::background configure myBackground -colorscale linear
    } msg] $msg
} {0 {}}

test background.258 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test background.259 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.260 {background configure -decreasing badValue} {
    list [catch {
	blt::background configure myBackground -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test background.261 {background configure -decreasing 0} {
    list [catch {
	blt::background configure myBackground -decreasing 0
    } msg] $msg
} {0 {}}

test background.262 {background configure -decreasing 1} {
    list [catch {
	blt::background configure myBackground -decreasing 1
    } msg] $msg
} {0 {}}

test background.263 {background configure -decreasing no} {
    list [catch {
	blt::background configure myBackground -decreasing no
    } msg] $msg
} {0 {}}

test background.264 {background configure -decreasing yes} {
    list [catch {
	blt::background configure myBackground -decreasing yes
    } msg] $msg
} {0 {}}

test background.265 {background configure -decreasing on} {
    list [catch {
	blt::background configure myBackground -decreasing on
    } msg] $msg
} {0 {}}

test background.266 {background configure -decreasing off} {
    list [catch {
	blt::background configure myBackground -decreasing off
    } msg] $msg
} {0 {}}

test background.267 {background configure -decreasing true} {
    list [catch {
	blt::background configure myBackground -decreasing true
    } msg] $msg
} {0 {}}

test background.268 {background configure -decreasing false} {
    list [catch {
	blt::background configure myBackground -decreasing false
    } msg] $msg
} {0 {}}

test background.269 {background configure -decreasing badValue} {
    list [catch {
	blt::background configure myBackground -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test background.270 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.271 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.272 {background configure -center} {
    list [catch {
	blt::background configure myBackground -center
    } msg] $msg
} {0 {-center {} {} c {0.5 0.5}}}

test background.273 {background configure -center badValue} {
    list [catch {
	blt::background configure myBackground -center badValue
    } msg] $msg
} {1 {unknown position "badValue": should be nw, n, ne, w, c, e, sw, s, or se.}}

test background.274 {background configure -center {badValue badValue}} {
    list [catch {
	blt::background configure myBackground -center {badValue badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test background.275 {background configure -center {badValue center}} {
    list [catch {
	blt::background configure myBackground -center {badValue center}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test background.276 {background configure -center {center badValue}} {
    list [catch {
	blt::background configure myBackground -center {center badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be left, right, or center.}}

test background.277 {background configure -center {0.0 1.0}} {
    list [catch {
	blt::background configure myBackground -center {0.0 1.0}
    } msg] $msg
} {0 {}}

test background.278 {background configure -center {2000.0 -1000.0}} {
    list [catch {
	blt::background configure myBackground -center {2000.0 -1000.0}
    } msg] $msg
} {0 {}}

test background.279 {background configure -center nw} {
    list [catch {
	blt::background configure myBackground -center nw
    } msg] $msg
} {0 {}}

test background.280 {background configure -center n} {
    list [catch {
	blt::background configure myBackground -center n
    } msg] $msg
} {0 {}}

test background.281 {background configure -center ne} {
    list [catch {
	blt::background configure myBackground -center ne
    } msg] $msg
} {0 {}}

test background.282 {background configure -center sw} {
    list [catch {
	blt::background configure myBackground -center s
    } msg] $msg
} {0 {}}

test background.283 {background configure -center s} {
    list [catch {
	blt::background configure myBackground -center s
    } msg] $msg
} {0 {}}

test background.284 {background configure -center se} {
    list [catch {
	blt::background configure myBackground -center se
    } msg] $msg
} {0 {}}

test background.285 {background configure -center c} {
    list [catch {
	blt::background configure myBackground -center c
    } msg] $msg
} {0 {}}

test background.286 {background configure -center {top left}} {
    list [catch {
	blt::background configure myBackground -center {top left}
    } msg] $msg
} {0 {}}

test background.287 {background configure -center {top right}} {
    list [catch {
	blt::background configure myBackground -center {top right}
    } msg] $msg
} {0 {}}

test background.288 {background configure -center {top center}} {
    list [catch {
	blt::background configure myBackground -center {top center}
    } msg] $msg
} {0 {}}

test background.289 {background configure -center {bottom left}} {
    list [catch {
	blt::background configure myBackground -center {bottom left}
    } msg] $msg
} {0 {}}

test background.290 {background configure -center {bottom right}} {
    list [catch {
	blt::background configure myBackground -center {bottom right}
    } msg] $msg
} {0 {}}

test background.291 {background configure -center {bottom center}} {
    list [catch {
	blt::background configure myBackground -center {bottom center}
    } msg] $msg
} {0 {}}

test background.292 {background configure -center {center left}} {
    list [catch {
	blt::background configure myBackground -center {center left}
    } msg] $msg
} {0 {}}

test background.293 {background configure -center {center right}} {
    list [catch {
	blt::background configure myBackground -center {center right}
    } msg] $msg
} {0 {}}

test background.294 {background configure -center {center center}} {
    list [catch {
	blt::background configure myBackground -center {center center}
    } msg] $msg
} {0 {}}

test background.295 {background configure -center {top center}} {
    list [catch {
	blt::background configure myBackground -center {top center}
    } msg] $msg
} {0 {}}

test background.296 {background configure -center {center center center}} {
    list [catch {
	blt::background configure myBackground -center {center center center}
    } msg] $msg
} {1 {unknown position "center center center": should be "top left" or "nw"}}


test background.297 {background configure -center} {
    list [catch {
	blt::background configure myBackground -center
    } msg] $msg
} {0 {-center {} {} c {0.5 0.0}}}

test background.298 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #e5e5e5}}

test background.299 {background configure -highcolor badColor} {
    list [catch {
	blt::background configure myBackground -highcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.300 {background configure -highcolor green} {
    list [catch {
	blt::background configure myBackground -highcolor green
    } msg] $msg
} {0 {}}

test background.301 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test background.302 {background configure -highcolor myBackground} {
    list [catch {
	blt::background configure myBackground -highcolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.303 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test background.304 {background configure -highcolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.305 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.306 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test background.307 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.308 {background configure -jitter -1} {
    list [catch {
	blt::background configure myBackground -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test background.309 {background configure -jitter 101} {
    list [catch {
	blt::background configure myBackground -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test background.310 {background configure -jitter 1.1} {
    list [catch {
	blt::background configure myBackground -jitter 1.1
    } msg] $msg
} {0 {}}

test background.311 {background configure -jitter 99.99999999} {
    list [catch {
	blt::background configure myBackground -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test background.312 {background configure -jitter 0} {
    list [catch {
	blt::background configure myBackground -jitter 0
    } msg] $msg
} {0 {}}

test background.313 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.314 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.315 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.316 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #7f7f7f}}

test background.317 {background configure -lowcolor badColor} {
    list [catch {
	blt::background configure myBackground -lowcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.318 {background configure -lowcolor green} {
    list [catch {
	blt::background configure myBackground -lowcolor green
    } msg] $msg
} {0 {}}

test background.319 {background configure -lowcolor myBackground} {
    list [catch {
	blt::background configure myBackground -lowcolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.320 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #00ff00}}

test background.321 {background configure -lowcolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -lowcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.322 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor 
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #d9d9d9}}

test background.323 {background configure -palette} {
    list [catch {
	blt::background configure myBackground -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test background.324 {background configure -palette badPalette} {
    list [catch {
	blt::background configure myBackground -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test background.325 {background configure -palette ""} {
    list [catch {
	blt::background configure myBackground -palette ""
    } msg] $msg
} {0 {}}

test background.326 {background configure -palette myPalette} {
    list [catch {
	blt::palette create myPalette -colordata {black white} -colorformat name
	blt::background configure myBackground -palette myPalette
	blt::palette delete myPalette
    } msg] $msg
} {0 {}}

test background.327 {background configure -palette ""} {
    list [catch {
	blt::background configure myBackground -palette ""
    } msg] $msg
} {0 {}}

test background.328 {background configure -palette badPalette} {
    list [catch {
	blt::background configure myBackground -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test background.329 {background configure -palette} {
    list [catch {
	blt::background configure myBackground -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test background.330 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.331 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.332 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.333 {background configure -xoffset 1} {
    list [catch {
	blt::background configure myBackground -xoffset 1
    } msg] $msg
} {0 {}}

test background.334 {background configure -xoffset 1i} {
    list [catch {
	blt::background configure myBackground -xoffset 1i
    } msg] $msg
} {0 {}}

test background.335 {background configure -xoffset -10} {
    list [catch {
	blt::background configure myBackground -xoffset -10
    } msg] $msg
} {0 {}}

test background.336 {background configure -xoffset 10c} {
    list [catch {
	blt::background configure myBackground -xoffset 10c
    } msg] $msg
} {0 {}}

test background.337 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.338 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.339 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.340 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.341 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.342 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.343 {background configure -yoffset 1} {
    list [catch {
	blt::background configure myBackground -yoffset 1
    } msg] $msg
} {0 {}}

test background.344 {background configure -yoffset 1i} {
    list [catch {
	blt::background configure myBackground -yoffset 1i
    } msg] $msg
} {0 {}}

test background.345 {background configure -yoffset -10} {
    list [catch {
	blt::background configure myBackground -yoffset -10
    } msg] $msg
} {0 {}}

test background.346 {background configure -yoffset 10c} {
    list [catch {
	blt::background configure myBackground -yoffset 10c
    } msg] $msg
} {0 {}}

test background.347 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.348 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.349 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.350 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.351 {background configure -relativeto badColor} {
    list [catch {
	blt::background configure myBackground -relativeto badRef
    } msg] $msg
} {1 {unknown reference type "badRef"}}

test background.352 {background configure -relativeto self} {
    list [catch {
	blt::background configure myBackground -relativeto self
    } msg] $msg
} {0 {}}

test background.353 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.354 {background configure -relativeto .} {
    list [catch {
	blt::background configure myBackground -relativeto .
    } msg] $msg
} {0 {}}

test background.355 {background configure -relativeto .badWindow} {
    list [catch {
	blt::background configure myBackground -relativeto .badWindow
    } msg] $msg
} {0 {}}

test background.356 {background configure -relativeto .badWindow} {
    list [catch {
	blt::tk::label .test -background myBackground
	proc tkerror { mesg } {
	    global testmsg
	    set testmsg $mesg
	}
	wm withdraw .
	update
	destroy .test
	set testmsg
    } msg] $msg
} {0 {bad window path name ".badWindow"}}


test background.357 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel .badWindow}}

test background.358 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.359 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.360 {background delete myBackground} {
    list [catch {blt::background delete myBackground} msg] $msg
} {0 {}}


#####

test background.361 {background create conical} {
    list [catch {blt::background create conical} msg] $msg
} {0 background3}

test background.362 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 background3}

test background.363 {background type background3} {
    list [catch {blt::background type background3} msg] $msg
} {0 conical}

test background.364 {background exists} {
    list [catch {blt::background exists background3} msg] $msg
} {0 1}

test background.365 {background configure background3} {
    list [catch {blt::background configure background3} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-center {} {} c {0.5 0.5}} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-highcolor {} {} grey90 #e5e5e5} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-palette {} {} {} {}} {-rotate {} {} 45.0 45.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.366 {background cget} {
    list [catch {blt::background cget} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.367 {background cget background3} {
    list [catch {blt::background cget background3} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.368 {background cget background3 -badOption} {
    list [catch {blt::background cget background3 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test background.369 {background cget background3 -background} {
    list [catch {blt::background cget background3 -background} msg] $msg
} {0 #d9d9d9}

test background.370 {background cget background3 -bg} {
    list [catch {blt::background cget background3 -bg} msg] $msg
} {0 #d9d9d9}

test background.371 {background cget background3 -border} {
    list [catch {blt::background cget background3 -border} msg] $msg
} {0 #d9d9d9}

test background.372 {background cget background3 -relativeto} {
    list [catch {blt::background cget background3 -relativeto} msg] $msg
} {0 toplevel}

test background.373 {background cget background3 -colorscale} {
    list [catch {blt::background cget background3 -colorscale} msg] $msg
} {0 linear}

test background.374 {background cget background3 -decreasing} {
    list [catch {blt::background cget background3 -decreasing} msg] $msg
} {0 0}

test background.375 {background cget background3 -center} {
    list [catch {blt::background cget background3 -center} msg] $msg
} {0 {0.5 0.5}}

test background.376 {background cget background3 -highcolor} {
    list [catch {blt::background cget background3 -highcolor} msg] $msg
} {0 #e5e5e5}

test background.377 {background cget background3 -jitter} {
    list [catch {blt::background cget background3 -jitter} msg] $msg
} {0 0.0}

test background.378 {background cget background3 -lowcolor} {
    list [catch {blt::background cget background3 -lowcolor} msg] $msg
} {0 #7f7f7f}

test background.379 {background cget background3 -palette} {
    list [catch {blt::background cget background3 -palette} msg] $msg
} {0 {}}

test background.380 {background cget background3 -xoffset} {
    list [catch {blt::background cget background3 -xoffset} msg] $msg
} {0 0}

test background.381 {background cget background3 -yoffset} {
    list [catch {blt::background cget background3 -yoffset} msg] $msg
} {0 0}

test background.382 {background cget background3} {
    list [catch {blt::background cget background3} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.383 {background delete} {
    list [catch {blt::background delete} msg] $msg
} {0 {}}

test background.384 {background delete badName} {
    list [catch {blt::background delete badName} msg] $msg
} {1 {can't find background "badName"}}

test background.385 {background delete background3} {
    list [catch {blt::background delete background3} msg] $msg
} {0 {}}

test background.386 {background exists background3} {
    list [catch {blt::background exists background3} msg] $msg
} {0 0}

test background.387 {background delete all backgrounds} {
    list [catch {eval blt::background delete [blt::background names]} msg] $msg
} {0 {}}

test background.388 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 {}}

test background.389 {background create conical} {
    list [catch {blt::background create conical myBackground} msg] $msg
} {0 myBackground}

test background.390 {background exists} {
    list [catch {blt::background exists myBackground} msg] $msg
} {0 1}

test background.391 {background type} {
    list [catch {blt::background type myBackground} msg] $msg
} {0 conical}

test background.392 {background configure myBackground} {
    list [catch {blt::background configure myBackground} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-center {} {} c {0.5 0.5}} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-highcolor {} {} grey90 #e5e5e5} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-palette {} {} {} {}} {-rotate {} {} 45.0 45.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.393 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.394 {background configure -background badColor} {
    list [catch {
	blt::background configure myBackground -background badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.395 {background configure -background green} {
    list [catch {
	blt::background configure myBackground -background green
    } msg] $msg
} {0 {}}

test background.396 {background configure -background myBackground} {
    list [catch {
	blt::background configure myBackground -background myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.397 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.398 {background configure -background #d9d9d9} {
    list [catch {
	blt::background configure myBackground -background #d9d9d9
    } msg] $msg
} {0 {}}

test background.399 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.400 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.401 {background configure -bg badColor} {
    list [catch {
	blt::background configure myBackground -bg badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.402 {background configure -bg green} {
    list [catch {
	blt::background configure myBackground -bg green
    } msg] $msg
} {0 {}}

test background.403 {background configure -bg myBackground} {
    list [catch {
	blt::background configure myBackground -bg myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.404 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.405 {background configure -bg #d9d9d9} {
    list [catch {
	blt::background configure myBackground -bg #d9d9d9
    } msg] $msg
} {0 {}}

test background.406 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.407 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.408 {background configure -border badColor} {
    list [catch {
	blt::background configure myBackground -border badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.409 {background configure -border green} {
    list [catch {
	blt::background configure myBackground -border green
    } msg] $msg
} {0 {}}

test background.410 {background configure -border myBackground} {
    list [catch {
	blt::background configure myBackground -border myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.411 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.412 {background configure -border #d9d9d9} {
    list [catch {
	blt::background configure myBackground -border #d9d9d9
    } msg] $msg
} {0 {}}

test background.413 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.414 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test background.415 {background configure -colorscale badScale} {
    list [catch {
	blt::background configure myBackground -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test background.416 {background configure -colorscale linear} {
    list [catch {
	blt::background configure myBackground -colorscale linear
    } msg] $msg
} {0 {}}

test background.417 {background configure -colorscale log} {
    list [catch {
	blt::background configure myBackground -colorscale log
    } msg] $msg
} {0 {}}

test background.418 {background configure -colorscale badScale} {
    list [catch {
	blt::background configure myBackground -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test background.419 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear logarithmic}}

test background.420 {background configure -colorscale linear} {
    list [catch {
	blt::background configure myBackground -colorscale linear
    } msg] $msg
} {0 {}}

test background.421 {background configure -colorscale} {
    list [catch {
	blt::background configure myBackground -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test background.422 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.423 {background configure -decreasing badValue} {
    list [catch {
	blt::background configure myBackground -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test background.424 {background configure -decreasing 0} {
    list [catch {
	blt::background configure myBackground -decreasing 0
    } msg] $msg
} {0 {}}

test background.425 {background configure -decreasing 1} {
    list [catch {
	blt::background configure myBackground -decreasing 1
    } msg] $msg
} {0 {}}

test background.426 {background configure -decreasing no} {
    list [catch {
	blt::background configure myBackground -decreasing no
    } msg] $msg
} {0 {}}

test background.427 {background configure -decreasing yes} {
    list [catch {
	blt::background configure myBackground -decreasing yes
    } msg] $msg
} {0 {}}

test background.428 {background configure -decreasing on} {
    list [catch {
	blt::background configure myBackground -decreasing on
    } msg] $msg
} {0 {}}

test background.429 {background configure -decreasing off} {
    list [catch {
	blt::background configure myBackground -decreasing off
    } msg] $msg
} {0 {}}

test background.430 {background configure -decreasing true} {
    list [catch {
	blt::background configure myBackground -decreasing true
    } msg] $msg
} {0 {}}

test background.431 {background configure -decreasing false} {
    list [catch {
	blt::background configure myBackground -decreasing false
    } msg] $msg
} {0 {}}

test background.432 {background configure -decreasing badValue} {
    list [catch {
	blt::background configure myBackground -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test background.433 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.434 {background configure -decreasing} {
    list [catch {
	blt::background configure myBackground -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test background.435 {background configure -center} {
    list [catch {
	blt::background configure myBackground -center
    } msg] $msg
} {0 {-center {} {} c {0.5 0.5}}}

test background.436 {background configure -center badValue} {
    list [catch {
	blt::background configure myBackground -center badValue
    } msg] $msg
} {1 {unknown position "badValue": should be nw, n, ne, w, c, e, sw, s, or se.}}

test background.437 {background configure -center {badValue badValue}} {
    list [catch {
	blt::background configure myBackground -center {badValue badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test background.438 {background configure -center {badValue center}} {
    list [catch {
	blt::background configure myBackground -center {badValue center}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test background.439 {background configure -center {center badValue}} {
    list [catch {
	blt::background configure myBackground -center {center badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be left, right, or center.}}

test background.440 {background configure -center {0.0 1.0}} {
    list [catch {
	blt::background configure myBackground -center {0.0 1.0}
    } msg] $msg
} {0 {}}

test background.441 {background configure -center {2000.0 -1000.0}} {
    list [catch {
	blt::background configure myBackground -center {2000.0 -1000.0}
    } msg] $msg
} {0 {}}

test background.442 {background configure -center nw} {
    list [catch {
	blt::background configure myBackground -center nw
    } msg] $msg
} {0 {}}

test background.443 {background configure -center n} {
    list [catch {
	blt::background configure myBackground -center n
    } msg] $msg
} {0 {}}

test background.444 {background configure -center ne} {
    list [catch {
	blt::background configure myBackground -center ne
    } msg] $msg
} {0 {}}

test background.445 {background configure -center sw} {
    list [catch {
	blt::background configure myBackground -center s
    } msg] $msg
} {0 {}}

test background.446 {background configure -center s} {
    list [catch {
	blt::background configure myBackground -center s
    } msg] $msg
} {0 {}}

test background.447 {background configure -center se} {
    list [catch {
	blt::background configure myBackground -center se
    } msg] $msg
} {0 {}}

test background.448 {background configure -center c} {
    list [catch {
	blt::background configure myBackground -center c
    } msg] $msg
} {0 {}}

test background.449 {background configure -center {top left}} {
    list [catch {
	blt::background configure myBackground -center {top left}
    } msg] $msg
} {0 {}}

test background.450 {background configure -center {top right}} {
    list [catch {
	blt::background configure myBackground -center {top right}
    } msg] $msg
} {0 {}}

test background.451 {background configure -center {top center}} {
    list [catch {
	blt::background configure myBackground -center {top center}
    } msg] $msg
} {0 {}}

test background.452 {background configure -center {bottom left}} {
    list [catch {
	blt::background configure myBackground -center {bottom left}
    } msg] $msg
} {0 {}}

test background.453 {background configure -center {bottom right}} {
    list [catch {
	blt::background configure myBackground -center {bottom right}
    } msg] $msg
} {0 {}}

test background.454 {background configure -center {bottom center}} {
    list [catch {
	blt::background configure myBackground -center {bottom center}
    } msg] $msg
} {0 {}}

test background.455 {background configure -center {center left}} {
    list [catch {
	blt::background configure myBackground -center {center left}
    } msg] $msg
} {0 {}}

test background.456 {background configure -center {center right}} {
    list [catch {
	blt::background configure myBackground -center {center right}
    } msg] $msg
} {0 {}}

test background.457 {background configure -center {center center}} {
    list [catch {
	blt::background configure myBackground -center {center center}
    } msg] $msg
} {0 {}}

test background.458 {background configure -center {top center}} {
    list [catch {
	blt::background configure myBackground -center {top center}
    } msg] $msg
} {0 {}}

test background.459 {background configure -center {center center center}} {
    list [catch {
	blt::background configure myBackground -center {center center center}
    } msg] $msg
} {1 {unknown position "center center center": should be "top left" or "nw"}}


test background.460 {background configure -center} {
    list [catch {
	blt::background configure myBackground -center
    } msg] $msg
} {0 {-center {} {} c {0.5 0.0}}}

test background.461 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #e5e5e5}}

test background.462 {background configure -highcolor badColor} {
    list [catch {
	blt::background configure myBackground -highcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.463 {background configure -highcolor green} {
    list [catch {
	blt::background configure myBackground -highcolor green
    } msg] $msg
} {0 {}}

test background.464 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test background.465 {background configure -highcolor myBackground} {
    list [catch {
	blt::background configure myBackground -highcolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.466 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test background.467 {background configure -highcolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.468 {background configure -highcolor} {
    list [catch {
	blt::background configure myBackground -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.469 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test background.470 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.471 {background configure -jitter -1} {
    list [catch {
	blt::background configure myBackground -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test background.472 {background configure -jitter 101} {
    list [catch {
	blt::background configure myBackground -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test background.473 {background configure -jitter 1.1} {
    list [catch {
	blt::background configure myBackground -jitter 1.1
    } msg] $msg
} {0 {}}

test background.474 {background configure -jitter 99.99999999} {
    list [catch {
	blt::background configure myBackground -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test background.475 {background configure -jitter 0} {
    list [catch {
	blt::background configure myBackground -jitter 0
    } msg] $msg
} {0 {}}

test background.476 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.477 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.478 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.479 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #7f7f7f}}

test background.480 {background configure -lowcolor badColor} {
    list [catch {
	blt::background configure myBackground -lowcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.481 {background configure -lowcolor green} {
    list [catch {
	blt::background configure myBackground -lowcolor green
    } msg] $msg
} {0 {}}

test background.482 {background configure -lowcolor myBackground} {
    list [catch {
	blt::background configure myBackground -lowcolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.483 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #00ff00}}

test background.484 {background configure -lowcolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -lowcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.485 {background configure -lowcolor} {
    list [catch {
	blt::background configure myBackground -lowcolor 
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #d9d9d9}}

test background.486 {background configure -palette} {
    list [catch {
	blt::background configure myBackground -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test background.487 {background configure -palette badPalette} {
    list [catch {
	blt::background configure myBackground -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test background.488 {background configure -palette ""} {
    list [catch {
	blt::background configure myBackground -palette ""
    } msg] $msg
} {0 {}}

test background.489 {background configure -palette myPalette} {
    list [catch {
	blt::palette create myPalette -colordata {black white} -colorformat name
	blt::background configure myBackground -palette myPalette
	blt::palette delete myPalette
    } msg] $msg
} {0 {}}

test background.490 {background configure -palette ""} {
    list [catch {
	blt::background configure myBackground -palette ""
    } msg] $msg
} {0 {}}

test background.491 {background configure -palette badPalette} {
    list [catch {
	blt::background configure myBackground -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test background.492 {background configure -palette} {
    list [catch {
	blt::background configure myBackground -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test background.493 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.494 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.495 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.496 {background configure -xoffset 1} {
    list [catch {
	blt::background configure myBackground -xoffset 1
    } msg] $msg
} {0 {}}

test background.497 {background configure -xoffset 1i} {
    list [catch {
	blt::background configure myBackground -xoffset 1i
    } msg] $msg
} {0 {}}

test background.498 {background configure -xoffset -10} {
    list [catch {
	blt::background configure myBackground -xoffset -10
    } msg] $msg
} {0 {}}

test background.499 {background configure -xoffset 10c} {
    list [catch {
	blt::background configure myBackground -xoffset 10c
    } msg] $msg
} {0 {}}

test background.500 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.501 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.502 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.503 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.504 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.505 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.506 {background configure -yoffset 1} {
    list [catch {
	blt::background configure myBackground -yoffset 1
    } msg] $msg
} {0 {}}

test background.507 {background configure -yoffset 1i} {
    list [catch {
	blt::background configure myBackground -yoffset 1i
    } msg] $msg
} {0 {}}

test background.508 {background configure -yoffset -10} {
    list [catch {
	blt::background configure myBackground -yoffset -10
    } msg] $msg
} {0 {}}

test background.509 {background configure -yoffset 10c} {
    list [catch {
	blt::background configure myBackground -yoffset 10c
    } msg] $msg
} {0 {}}

test background.510 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.511 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.512 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.513 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.514 {background configure -relativeto badColor} {
    list [catch {
	blt::background configure myBackground -relativeto badRef
    } msg] $msg
} {1 {unknown reference type "badRef"}}

test background.515 {background configure -relativeto self} {
    list [catch {
	blt::background configure myBackground -relativeto self
    } msg] $msg
} {0 {}}

test background.516 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.517 {background configure -relativeto .} {
    list [catch {
	blt::background configure myBackground -relativeto .
    } msg] $msg
} {0 {}}

test background.518 {background configure -relativeto .badWindow} {
    list [catch {
	blt::background configure myBackground -relativeto .badWindow
    } msg] $msg
} {0 {}}

test background.519 {background configure -relativeto .badWindow} {
    list [catch {
	blt::tk::label .test -background myBackground
	proc tkerror { mesg } {
	    global testmsg
	    set testmsg $mesg
	}
	wm withdraw .
	update
	destroy .test
	set testmsg
    } msg] $msg
} {0 {bad window path name ".badWindow"}}


test background.520 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel .badWindow}}

test background.521 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.522 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.523 {background delete myBackground} {
    list [catch {blt::background delete myBackground} msg] $msg
} {0 {}}

#####

test background.524 {background create stripes} {
    list [catch {blt::background create stripes} msg] $msg
} {0 background4}

test background.525 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 background4}

test background.526 {background type background4} {
    list [catch {blt::background type background4} msg] $msg
} {0 stripes}

test background.527 {background exists} {
    list [catch {blt::background exists background4} msg] $msg
} {0 1}

test background.528 {background configure background4} {
    list [catch {blt::background configure background4} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-jitter {} {} 0 0.0} {-offcolor {} {} grey97 #f7f7f7} {-oncolor {} {} grey90 #e5e5e5} {-orient {} {} vertical vertical} {-stride {} {} 2 2} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.529 {background cget} {
    list [catch {blt::background cget} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.530 {background cget background4} {
    list [catch {blt::background cget background4} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.531 {background cget background4 -badOption} {
    list [catch {blt::background cget background4 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test background.532 {background cget background4 -background} {
    list [catch {blt::background cget background4 -background} msg] $msg
} {0 #d9d9d9}

test background.533 {background cget background4 -bg} {
    list [catch {blt::background cget background4 -bg} msg] $msg
} {0 #d9d9d9}

test background.534 {background cget background4 -border} {
    list [catch {blt::background cget background4 -border} msg] $msg
} {0 #d9d9d9}

test background.535 {background cget background4 -relativeto} {
    list [catch {blt::background cget background4 -relativeto} msg] $msg
} {0 toplevel}

test background.536 {background cget background4 -oncolor} {
    list [catch {blt::background cget background4 -oncolor} msg] $msg
} {0 #e5e5e5}

test background.537 {background cget background4 -jitter} {
    list [catch {blt::background cget background4 -jitter} msg] $msg
} {0 0.0}

test background.538 {background cget background4 -offcolor} {
    list [catch {blt::background cget background4 -offcolor} msg] $msg
} {0 #f7f7f7}

test background.539 {background cget background4 -xoffset} {
    list [catch {blt::background cget background4 -xoffset} msg] $msg
} {0 0}

test background.540 {background cget background4 -yoffset} {
    list [catch {blt::background cget background4 -yoffset} msg] $msg
} {0 0}

test background.541 {background cget background4} {
    list [catch {blt::background cget background4} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.542 {background delete} {
    list [catch {blt::background delete} msg] $msg
} {0 {}}

test background.543 {background delete badName} {
    list [catch {blt::background delete badName} msg] $msg
} {1 {can't find background "badName"}}

test background.544 {background delete background4} {
    list [catch {blt::background delete background4} msg] $msg
} {0 {}}

test background.545 {background exists background4} {
    list [catch {blt::background exists background4} msg] $msg
} {0 0}

test background.546 {background delete all backgrounds} {
    list [catch {eval blt::background delete [blt::background names]} msg] $msg
} {0 {}}

test background.547 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 {}}

test background.548 {background create stripes} {
    list [catch {blt::background create stripes myBackground} msg] $msg
} {0 myBackground}

test background.549 {background exists} {
    list [catch {blt::background exists myBackground} msg] $msg
} {0 1}

test background.550 {background type} {
    list [catch {blt::background type myBackground} msg] $msg
} {0 stripes}

test background.551 {background configure myBackground} {
    list [catch {blt::background configure myBackground} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-jitter {} {} 0 0.0} {-offcolor {} {} grey97 #f7f7f7} {-oncolor {} {} grey90 #e5e5e5} {-orient {} {} vertical vertical} {-stride {} {} 2 2} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.552 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.553 {background configure -background badColor} {
    list [catch {
	blt::background configure myBackground -background badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.554 {background configure -background green} {
    list [catch {
	blt::background configure myBackground -background green
    } msg] $msg
} {0 {}}

test background.555 {background configure -background myBackground} {
    list [catch {
	blt::background configure myBackground -background myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.556 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.557 {background configure -background #d9d9d9} {
    list [catch {
	blt::background configure myBackground -background #d9d9d9
    } msg] $msg
} {0 {}}

test background.558 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.559 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.560 {background configure -bg badColor} {
    list [catch {
	blt::background configure myBackground -bg badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.561 {background configure -bg green} {
    list [catch {
	blt::background configure myBackground -bg green
    } msg] $msg
} {0 {}}

test background.562 {background configure -bg myBackground} {
    list [catch {
	blt::background configure myBackground -bg myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.563 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.564 {background configure -bg #d9d9d9} {
    list [catch {
	blt::background configure myBackground -bg #d9d9d9
    } msg] $msg
} {0 {}}

test background.565 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.566 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.567 {background configure -border badColor} {
    list [catch {
	blt::background configure myBackground -border badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.568 {background configure -border green} {
    list [catch {
	blt::background configure myBackground -border green
    } msg] $msg
} {0 {}}

test background.569 {background configure -border myBackground} {
    list [catch {
	blt::background configure myBackground -border myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.570 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.571 {background configure -border #d9d9d9} {
    list [catch {
	blt::background configure myBackground -border #d9d9d9
    } msg] $msg
} {0 {}}

test background.572 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.573 {background configure -oncolor} {
    list [catch {
	blt::background configure myBackground -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #e5e5e5}}

test background.574 {background configure -oncolor badColor} {
    list [catch {
	blt::background configure myBackground -oncolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.575 {background configure -oncolor green} {
    list [catch {
	blt::background configure myBackground -oncolor green
    } msg] $msg
} {0 {}}

test background.576 {background configure -oncolor} {
    list [catch {
	blt::background configure myBackground -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #00ff00}}

test background.577 {background configure -oncolor myBackground} {
    list [catch {
	blt::background configure myBackground -oncolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.578 {background configure -oncolor} {
    list [catch {
	blt::background configure myBackground -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #00ff00}}

test background.579 {background configure -oncolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -oncolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.580 {background configure -oncolor} {
    list [catch {
	blt::background configure myBackground -oncolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.581 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test background.582 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.583 {background configure -jitter -1} {
    list [catch {
	blt::background configure myBackground -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test background.584 {background configure -jitter 101} {
    list [catch {
	blt::background configure myBackground -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test background.585 {background configure -jitter 1.1} {
    list [catch {
	blt::background configure myBackground -jitter 1.1
    } msg] $msg
} {0 {}}

test background.586 {background configure -jitter 99.99999999} {
    list [catch {
	blt::background configure myBackground -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test background.587 {background configure -jitter 0} {
    list [catch {
	blt::background configure myBackground -jitter 0
    } msg] $msg
} {0 {}}

test background.588 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.589 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.590 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.591 {background configure -offcolor} {
    list [catch {
	blt::background configure myBackground -offcolor
    } msg] $msg
} {0 {-offcolor {} {} grey97 #f7f7f7}}

test background.592 {background configure -offcolor badColor} {
    list [catch {
	blt::background configure myBackground -offcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.593 {background configure -offcolor green} {
    list [catch {
	blt::background configure myBackground -offcolor green
    } msg] $msg
} {0 {}}

test background.594 {background configure -offcolor myBackground} {
    list [catch {
	blt::background configure myBackground -offcolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.595 {background configure -offcolor} {
    list [catch {
	blt::background configure myBackground -offcolor
    } msg] $msg
} {0 {-offcolor {} {} grey97 #00ff00}}

test background.596 {background configure -offcolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -offcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.597 {background configure -offcolor} {
    list [catch {
	blt::background configure myBackground -offcolor 
    } msg] $msg
} {0 {-offcolor {} {} grey97 #d9d9d9}}


test background.598 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.599 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.600 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.601 {background configure -xoffset 1} {
    list [catch {
	blt::background configure myBackground -xoffset 1
    } msg] $msg
} {0 {}}

test background.602 {background configure -xoffset 1i} {
    list [catch {
	blt::background configure myBackground -xoffset 1i
    } msg] $msg
} {0 {}}

test background.603 {background configure -xoffset -10} {
    list [catch {
	blt::background configure myBackground -xoffset -10
    } msg] $msg
} {0 {}}

test background.604 {background configure -xoffset 10c} {
    list [catch {
	blt::background configure myBackground -xoffset 10c
    } msg] $msg
} {0 {}}

test background.605 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.606 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.607 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.608 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.609 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.610 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.611 {background configure -yoffset 1} {
    list [catch {
	blt::background configure myBackground -yoffset 1
    } msg] $msg
} {0 {}}

test background.612 {background configure -yoffset 1i} {
    list [catch {
	blt::background configure myBackground -yoffset 1i
    } msg] $msg
} {0 {}}

test background.613 {background configure -yoffset -10} {
    list [catch {
	blt::background configure myBackground -yoffset -10
    } msg] $msg
} {0 {}}

test background.614 {background configure -yoffset 10c} {
    list [catch {
	blt::background configure myBackground -yoffset 10c
    } msg] $msg
} {0 {}}

test background.615 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.616 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.617 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.618 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.619 {background configure -relativeto badColor} {
    list [catch {
	blt::background configure myBackground -relativeto badRef
    } msg] $msg
} {1 {unknown reference type "badRef"}}

test background.620 {background configure -relativeto self} {
    list [catch {
	blt::background configure myBackground -relativeto self
    } msg] $msg
} {0 {}}

test background.621 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.622 {background configure -relativeto .} {
    list [catch {
	blt::background configure myBackground -relativeto .
    } msg] $msg
} {0 {}}

test background.623 {background configure -relativeto .badWindow} {
    list [catch {
	blt::background configure myBackground -relativeto .badWindow
    } msg] $msg
} {0 {}}

test background.624 {background configure -relativeto .badWindow} {
    list [catch {
	blt::tk::label .test -background myBackground
	proc tkerror { mesg } {
	    global testmsg
	    set testmsg $mesg
	}
	wm withdraw .
	update
	destroy .test
	set testmsg
    } msg] $msg
} {0 {bad window path name ".badWindow"}}


test background.625 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel .badWindow}}

test background.626 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.627 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.628 {background configure -orient} {
    list [catch {
	blt::background configure myBackground -orient
    } msg] $msg
} {0 {-orient {} {} vertical vertical}}

test background.629 {background configure -orient badOrient} {
    list [catch {
	blt::background configure myBackground -orient badOrient
    } msg] $msg
} {1 {unknown orient value "badOrient": should be vertical or horizontal.}}

test background.630 {background configure -orient ""} {
    list [catch {
	blt::background configure myBackground -orient ""
    } msg] $msg
} {1 {unknown orient value "": should be vertical or horizontal.}}

test background.631 {background configure -orient horizontal} {
    list [catch {
	blt::background configure myBackground -orient horizontal
    } msg] $msg
} {0 {}}

test background.632 {background configure -orient vertical} {
    list [catch {
	blt::background configure myBackground -orient vertical
    } msg] $msg
} {0 {}}

test background.633 {background configure -orient} {
    list [catch {
	blt::background configure myBackground -orient
    } msg] $msg
} {0 {-orient {} {} vertical vertical}}

test background.634 {background configure -stride} {
    list [catch {
	blt::background configure myBackground -stride
    } msg] $msg
} {0 {-stride {} {} 2 2}}

test background.635 {background configure -stride badValue} {
    list [catch {
	blt::background configure myBackground -stride badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.636 {background configure -stride 0} {
    list [catch {
	blt::background configure myBackground -stride 0
    } msg] $msg
} {1 {bad distance "0": must be positive}}

test background.637 {background configure -stride 1} {
    list [catch {
	blt::background configure myBackground -stride 1
    } msg] $msg
} {0 {}}

test background.638 {background configure -stride 1i} {
    list [catch {
	blt::background configure myBackground -stride 1i
    } msg] $msg
} {0 {}}

test background.639 {background configure -stride -10} {
    list [catch {
	blt::background configure myBackground -stride -10
    } msg] $msg
} {1 {bad distance "-10": must be positive}}

test background.640 {background configure -stride 10c} {
    list [catch {
	blt::background configure myBackground -stride 10c
    } msg] $msg
} {0 {}}

test background.641 {background configure -stride 2} {
    list [catch {
	blt::background configure myBackground -stride 2
    } msg] $msg
} {0 {}}

test background.642 {background configure -stride badValue} {
    list [catch {
	blt::background configure myBackground -stride badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.643 {background configure -stride} {
    list [catch {
	blt::background configure myBackground -stride
    } msg] $msg
} {0 {-stride {} {} 2 2}}

test background.644 {background delete myBackground} {
    list [catch {blt::background delete myBackground} msg] $msg
} {0 {}}

#####

test background.645 {background create checkers} {
    list [catch {blt::background create checkers} msg] $msg
} {0 background5}

test background.646 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 background5}

test background.647 {background type background5} {
    list [catch {blt::background type background5} msg] $msg
} {0 checkers}

test background.648 {background exists} {
    list [catch {blt::background exists background5} msg] $msg
} {0 1}

test background.649 {background configure background5} {
    list [catch {blt::background configure background5} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-jitter {} {} 0 0.0} {-offcolor {} {} grey97 #f7f7f7} {-oncolor {} {} grey90 #e5e5e5} {-stride {} {} 10 10} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.650 {background cget} {
    list [catch {blt::background cget} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.651 {background cget background5} {
    list [catch {blt::background cget background5} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.652 {background cget background5 -badOption} {
    list [catch {blt::background cget background5 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test background.653 {background cget background5 -background} {
    list [catch {blt::background cget background5 -background} msg] $msg
} {0 #d9d9d9}

test background.654 {background cget background5 -bg} {
    list [catch {blt::background cget background5 -bg} msg] $msg
} {0 #d9d9d9}

test background.655 {background cget background5 -border} {
    list [catch {blt::background cget background5 -border} msg] $msg
} {0 #d9d9d9}

test background.656 {background cget background5 -relativeto} {
    list [catch {blt::background cget background5 -relativeto} msg] $msg
} {0 toplevel}

test background.657 {background cget background5 -oncolor} {
    list [catch {blt::background cget background5 -oncolor} msg] $msg
} {0 #e5e5e5}

test background.658 {background cget background5 -jitter} {
    list [catch {blt::background cget background5 -jitter} msg] $msg
} {0 0.0}

test background.659 {background cget background5 -offcolor} {
    list [catch {blt::background cget background5 -offcolor} msg] $msg
} {0 #f7f7f7}

test background.660 {background cget background5 -xoffset} {
    list [catch {blt::background cget background5 -xoffset} msg] $msg
} {0 0}

test background.661 {background cget background5 -yoffset} {
    list [catch {blt::background cget background5 -yoffset} msg] $msg
} {0 0}

test background.662 {background cget background5} {
    list [catch {blt::background cget background5} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.663 {background delete} {
    list [catch {blt::background delete} msg] $msg
} {0 {}}

test background.664 {background delete badName} {
    list [catch {blt::background delete badName} msg] $msg
} {1 {can't find background "badName"}}

test background.665 {background delete background5} {
    list [catch {blt::background delete background5} msg] $msg
} {0 {}}

test background.666 {background exists background5} {
    list [catch {blt::background exists background5} msg] $msg
} {0 0}

test background.667 {background delete all backgrounds} {
    list [catch {eval blt::background delete [blt::background names]} msg] $msg
} {0 {}}

test background.668 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 {}}

test background.669 {background create checkers} {
    list [catch {blt::background create checkers myBackground} msg] $msg
} {0 myBackground}

test background.670 {background exists} {
    list [catch {blt::background exists myBackground} msg] $msg
} {0 1}

test background.671 {background type} {
    list [catch {blt::background type myBackground} msg] $msg
} {0 checkers}

test background.672 {background configure myBackground} {
    list [catch {blt::background configure myBackground} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-jitter {} {} 0 0.0} {-offcolor {} {} grey97 #f7f7f7} {-oncolor {} {} grey90 #e5e5e5} {-stride {} {} 10 10} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}


test background.673 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.674 {background configure -background badColor} {
    list [catch {
	blt::background configure myBackground -background badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.675 {background configure -background green} {
    list [catch {
	blt::background configure myBackground -background green
    } msg] $msg
} {0 {}}

test background.676 {background configure -background myBackground} {
    list [catch {
	blt::background configure myBackground -background myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.677 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.678 {background configure -background #d9d9d9} {
    list [catch {
	blt::background configure myBackground -background #d9d9d9
    } msg] $msg
} {0 {}}

test background.679 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.680 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.681 {background configure -bg badColor} {
    list [catch {
	blt::background configure myBackground -bg badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.682 {background configure -bg green} {
    list [catch {
	blt::background configure myBackground -bg green
    } msg] $msg
} {0 {}}

test background.683 {background configure -bg myBackground} {
    list [catch {
	blt::background configure myBackground -bg myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.684 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.685 {background configure -bg #d9d9d9} {
    list [catch {
	blt::background configure myBackground -bg #d9d9d9
    } msg] $msg
} {0 {}}

test background.686 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.687 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.688 {background configure -border badColor} {
    list [catch {
	blt::background configure myBackground -border badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.689 {background configure -border green} {
    list [catch {
	blt::background configure myBackground -border green
    } msg] $msg
} {0 {}}

test background.690 {background configure -border myBackground} {
    list [catch {
	blt::background configure myBackground -border myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.691 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.692 {background configure -border #d9d9d9} {
    list [catch {
	blt::background configure myBackground -border #d9d9d9
    } msg] $msg
} {0 {}}

test background.693 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.694 {background configure -oncolor} {
    list [catch {
	blt::background configure myBackground -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #e5e5e5}}

test background.695 {background configure -oncolor badColor} {
    list [catch {
	blt::background configure myBackground -oncolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.696 {background configure -oncolor green} {
    list [catch {
	blt::background configure myBackground -oncolor green
    } msg] $msg
} {0 {}}

test background.697 {background configure -oncolor} {
    list [catch {
	blt::background configure myBackground -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #00ff00}}

test background.698 {background configure -oncolor myBackground} {
    list [catch {
	blt::background configure myBackground -oncolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.699 {background configure -oncolor} {
    list [catch {
	blt::background configure myBackground -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #00ff00}}

test background.700 {background configure -oncolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -oncolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.701 {background configure -oncolor} {
    list [catch {
	blt::background configure myBackground -oncolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.702 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test background.703 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.704 {background configure -jitter -1} {
    list [catch {
	blt::background configure myBackground -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test background.705 {background configure -jitter 101} {
    list [catch {
	blt::background configure myBackground -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test background.706 {background configure -jitter 1.1} {
    list [catch {
	blt::background configure myBackground -jitter 1.1
    } msg] $msg
} {0 {}}

test background.707 {background configure -jitter 99.99999999} {
    list [catch {
	blt::background configure myBackground -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test background.708 {background configure -jitter 0} {
    list [catch {
	blt::background configure myBackground -jitter 0
    } msg] $msg
} {0 {}}

test background.709 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.710 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.711 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.712 {background configure -offcolor} {
    list [catch {
	blt::background configure myBackground -offcolor
    } msg] $msg
} {0 {-offcolor {} {} grey97 #f7f7f7}}

test background.713 {background configure -offcolor badColor} {
    list [catch {
	blt::background configure myBackground -offcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test background.714 {background configure -offcolor green} {
    list [catch {
	blt::background configure myBackground -offcolor green
    } msg] $msg
} {0 {}}

test background.715 {background configure -offcolor myBackground} {
    list [catch {
	blt::background configure myBackground -offcolor myBackground
    } msg] $msg
} {1 {bad color specification "myBackground"}}

test background.716 {background configure -offcolor} {
    list [catch {
	blt::background configure myBackground -offcolor
    } msg] $msg
} {0 {-offcolor {} {} grey97 #00ff00}}

test background.717 {background configure -offcolor #d9d9d9} {
    list [catch {
	blt::background configure myBackground -offcolor #d9d9d9
    } msg] $msg
} {0 {}}

test background.718 {background configure -offcolor} {
    list [catch {
	blt::background configure myBackground -offcolor 
    } msg] $msg
} {0 {-offcolor {} {} grey97 #d9d9d9}}


test background.719 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.720 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.721 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.722 {background configure -xoffset 1} {
    list [catch {
	blt::background configure myBackground -xoffset 1
    } msg] $msg
} {0 {}}

test background.723 {background configure -xoffset 1i} {
    list [catch {
	blt::background configure myBackground -xoffset 1i
    } msg] $msg
} {0 {}}

test background.724 {background configure -xoffset -10} {
    list [catch {
	blt::background configure myBackground -xoffset -10
    } msg] $msg
} {0 {}}

test background.725 {background configure -xoffset 10c} {
    list [catch {
	blt::background configure myBackground -xoffset 10c
    } msg] $msg
} {0 {}}

test background.726 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.727 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.728 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.729 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.730 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.731 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.732 {background configure -yoffset 1} {
    list [catch {
	blt::background configure myBackground -yoffset 1
    } msg] $msg
} {0 {}}

test background.733 {background configure -yoffset 1i} {
    list [catch {
	blt::background configure myBackground -yoffset 1i
    } msg] $msg
} {0 {}}

test background.734 {background configure -yoffset -10} {
    list [catch {
	blt::background configure myBackground -yoffset -10
    } msg] $msg
} {0 {}}

test background.735 {background configure -yoffset 10c} {
    list [catch {
	blt::background configure myBackground -yoffset 10c
    } msg] $msg
} {0 {}}

test background.736 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.737 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.738 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.739 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.740 {background configure -relativeto badColor} {
    list [catch {
	blt::background configure myBackground -relativeto badRef
    } msg] $msg
} {1 {unknown reference type "badRef"}}

test background.741 {background configure -relativeto self} {
    list [catch {
	blt::background configure myBackground -relativeto self
    } msg] $msg
} {0 {}}

test background.742 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.743 {background configure -relativeto .} {
    list [catch {
	blt::background configure myBackground -relativeto .
    } msg] $msg
} {0 {}}

test background.744 {background configure -relativeto .badWindow} {
    list [catch {
	blt::background configure myBackground -relativeto .badWindow
    } msg] $msg
} {0 {}}

test background.745 {background configure -relativeto .badWindow} {
    list [catch {
	blt::tk::label .test -background myBackground
	proc tkerror { mesg } {
	    global testmsg
	    set testmsg $mesg
	}
	wm withdraw .
	update
	destroy .test
	set testmsg
    } msg] $msg
} {0 {bad window path name ".badWindow"}}


test background.746 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel .badWindow}}

test background.747 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.748 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}


test background.749 {background configure -stride} {
    list [catch {
	blt::background configure myBackground -stride
    } msg] $msg
} {0 {-stride {} {} 10 10}}


test background.750 {background configure -stride badValue} {
    list [catch {
	blt::background configure myBackground -stride badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.751 {background configure -stride 0} {
    list [catch {
	blt::background configure myBackground -stride 0
    } msg] $msg
} {1 {bad distance "0": must be positive}}

test background.752 {background configure -stride 1} {
    list [catch {
	blt::background configure myBackground -stride 1
    } msg] $msg
} {0 {}}

test background.753 {background configure -stride 1i} {
    list [catch {
	blt::background configure myBackground -stride 1i
    } msg] $msg
} {0 {}}

test background.754 {background configure -stride -10} {
    list [catch {
	blt::background configure myBackground -stride -10
    } msg] $msg
} {1 {bad distance "-10": must be positive}}

test background.755 {background configure -stride 10c} {
    list [catch {
	blt::background configure myBackground -stride 10c
    } msg] $msg
} {0 {}}

test background.756 {background configure -stride 10} {
    list [catch {
	blt::background configure myBackground -stride 10
    } msg] $msg
} {0 {}}

test background.757 {background configure -stride badValue} {
    list [catch {
	blt::background configure myBackground -stride badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.758 {background configure -stride} {
    list [catch {
	blt::background configure myBackground -stride
    } msg] $msg
} {0 {-stride {} {} 10 10}}

test background.759 {background delete myBackground} {
    list [catch {blt::background delete myBackground} msg] $msg
} {0 {}}

####

test background.760 {background create tile} {
    list [catch {blt::background create tile} msg] $msg
} {0 background6}

test background.761 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 background6}

test background.762 {background type background6} {
    list [catch {blt::background type background6} msg] $msg
} {0 tile}

test background.763 {background exists} {
    list [catch {blt::background exists background6} msg] $msg
} {0 1}

test background.764 {background configure background6} {
    list [catch {blt::background configure background6} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-image {} {} {} {}} {-jitter {} {} 0 0.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test background.765 {background cget} {
    list [catch {blt::background cget} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.766 {background cget background6} {
    list [catch {blt::background cget background6} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.767 {background cget background6 -badOption} {
    list [catch {blt::background cget background6 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test background.768 {background cget background6 -background} {
    list [catch {blt::background cget background6 -background} msg] $msg
} {0 #d9d9d9}

test background.769 {background cget background6 -bg} {
    list [catch {blt::background cget background6 -bg} msg] $msg
} {0 #d9d9d9}

test background.770 {background cget background6 -border} {
    list [catch {blt::background cget background6 -border} msg] $msg
} {0 #d9d9d9}

test background.771 {background cget background6 -relativeto} {
    list [catch {blt::background cget background6 -relativeto} msg] $msg
} {0 toplevel}

test background.772 {background cget background6 -image} {
    list [catch {blt::background cget background6 -image} msg] $msg
} {0 {}}

test background.773 {background cget background6 -jitter} {
    list [catch {blt::background cget background6 -jitter} msg] $msg
} {0 0.0}

test background.774 {background cget background6 -xoffset} {
    list [catch {blt::background cget background6 -xoffset} msg] $msg
} {0 0}

test background.775 {background cget background6 -yoffset} {
    list [catch {blt::background cget background6 -yoffset} msg] $msg
} {0 0}

test background.776 {background cget background6} {
    list [catch {blt::background cget background6} msg] $msg
} {1 {wrong # args: should be "blt::background cget bgName option"}}

test background.777 {background delete} {
    list [catch {blt::background delete} msg] $msg
} {0 {}}

test background.778 {background delete badName} {
    list [catch {blt::background delete badName} msg] $msg
} {1 {can't find background "badName"}}

test background.779 {background delete background6} {
    list [catch {blt::background delete background6} msg] $msg
} {0 {}}

test background.780 {background exists background6} {
    list [catch {blt::background exists background6} msg] $msg
} {0 0}

test background.781 {background delete all backgrounds} {
    list [catch {eval blt::background delete [blt::background names]} msg] $msg
} {0 {}}

test background.782 {background names} {
    list [catch {lsort [blt::background names]} msg] $msg
} {0 {}}

test background.783 {background create tile} {
    list [catch {blt::background create tile myBackground} msg] $msg
} {0 myBackground}

test background.784 {background exists} {
    list [catch {blt::background exists myBackground} msg] $msg
} {0 1}

test background.785 {background type} {
    list [catch {blt::background type myBackground} msg] $msg
} {0 tile}

test background.786 {background configure myBackground} {
    list [catch {blt::background configure myBackground} msg] $msg
} {0 {{-background color} {-bg color} {-border color Color #d9d9d9 #d9d9d9} {-relativeto {} {} toplevel toplevel} {-image {} {} {} {}} {-jitter {} {} 0 0.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}


test background.787 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.788 {background configure -background badColor} {
    list [catch {
	blt::background configure myBackground -background badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.789 {background configure -background green} {
    list [catch {
	blt::background configure myBackground -background green
    } msg] $msg
} {0 {}}

test background.790 {background configure -background myBackground} {
    list [catch {
	blt::background configure myBackground -background myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.791 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.792 {background configure -background #d9d9d9} {
    list [catch {
	blt::background configure myBackground -background #d9d9d9
    } msg] $msg
} {0 {}}

test background.793 {background configure -background} {
    list [catch {
	blt::background configure myBackground -background 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.794 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.795 {background configure -bg badColor} {
    list [catch {
	blt::background configure myBackground -bg badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.796 {background configure -bg green} {
    list [catch {
	blt::background configure myBackground -bg green
    } msg] $msg
} {0 {}}

test background.797 {background configure -bg myBackground} {
    list [catch {
	blt::background configure myBackground -bg myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.798 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.799 {background configure -bg #d9d9d9} {
    list [catch {
	blt::background configure myBackground -bg #d9d9d9
    } msg] $msg
} {0 {}}

test background.800 {background configure -bg} {
    list [catch {
	blt::background configure myBackground -bg 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.801 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}

test background.802 {background configure -border badColor} {
    list [catch {
	blt::background configure myBackground -border badColor
    } msg] $msg
} {1 {unknown color name "badColor"}}

test background.803 {background configure -border green} {
    list [catch {
	blt::background configure myBackground -border green
    } msg] $msg
} {0 {}}

test background.804 {background configure -border myBackground} {
    list [catch {
	blt::background configure myBackground -border myBackground
    } msg] $msg
} {1 {unknown color name "myBackground"}}

test background.805 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border
    } msg] $msg
} {0 {-border color Color #d9d9d9 green}}

test background.806 {background configure -border #d9d9d9} {
    list [catch {
	blt::background configure myBackground -border #d9d9d9
    } msg] $msg
} {0 {}}

test background.807 {background configure -border} {
    list [catch {
	blt::background configure myBackground -border 
    } msg] $msg
} {0 {-border color Color #d9d9d9 #d9d9d9}}


test background.808 {background configure -image} {
    list [catch {
	blt::background configure myBackground -image
    } msg] $msg
} {0 {-image {} {} {} {}}}

test background.809 {background configure -image badImage} {
    list [catch {
	blt::background configure myBackground -image badImage
    } msg] $msg
} {1 {image "badImage" doesn't exist}}

test background.810 {background configure -image myImage} {
    list [catch {
	image create picture myImage -file files/blt98.gif
	blt::background configure myBackground -image myImage
    } msg] $msg
} {0 {}}

test background.811 {background configure -image} {
    list [catch {
	blt::background configure myBackground -image
    } msg] $msg
} {0 {-image {} {} {} myImage}}

test background.812 {background configure -image} {
    list [catch {
	image delete myImage
	blt::background configure myBackground -image
    } msg] $msg
} {0 {-image {} {} {} {}}}

test background.813 {background configure -image badImage} {
    list [catch {
	blt::background configure myBackground -image bagImage
    } msg] $msg
} {1 {image "bagImage" doesn't exist}}

test background.814 {background configure -image} {
    list [catch {
	blt::background configure myBackground -image
    } msg] $msg
} {0 {-image {} {} {} {}}}


test background.815 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test background.816 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.817 {background configure -jitter -1} {
    list [catch {
	blt::background configure myBackground -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test background.818 {background configure -jitter 101} {
    list [catch {
	blt::background configure myBackground -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test background.819 {background configure -jitter 1.1} {
    list [catch {
	blt::background configure myBackground -jitter 1.1
    } msg] $msg
} {0 {}}

test background.820 {background configure -jitter 99.99999999} {
    list [catch {
	blt::background configure myBackground -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test background.821 {background configure -jitter 0} {
    list [catch {
	blt::background configure myBackground -jitter 0
    } msg] $msg
} {0 {}}

test background.822 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.823 {background configure -jitter badValue} {
    list [catch {
	blt::background configure myBackground -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test background.824 {background configure -jitter} {
    list [catch {
	blt::background configure myBackground -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test background.825 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.826 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.827 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.828 {background configure -xoffset 1} {
    list [catch {
	blt::background configure myBackground -xoffset 1
    } msg] $msg
} {0 {}}

test background.829 {background configure -xoffset 1i} {
    list [catch {
	blt::background configure myBackground -xoffset 1i
    } msg] $msg
} {0 {}}

test background.830 {background configure -xoffset -10} {
    list [catch {
	blt::background configure myBackground -xoffset -10
    } msg] $msg
} {0 {}}

test background.831 {background configure -xoffset 10c} {
    list [catch {
	blt::background configure myBackground -xoffset 10c
    } msg] $msg
} {0 {}}

test background.832 {background configure -xoffset 0} {
    list [catch {
	blt::background configure myBackground -xoffset 0
    } msg] $msg
} {0 {}}

test background.833 {background configure -xoffset badValue} {
    list [catch {
	blt::background configure myBackground -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.834 {background configure -xoffset} {
    list [catch {
	blt::background configure myBackground -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test background.835 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.836 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.837 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.838 {background configure -yoffset 1} {
    list [catch {
	blt::background configure myBackground -yoffset 1
    } msg] $msg
} {0 {}}

test background.839 {background configure -yoffset 1i} {
    list [catch {
	blt::background configure myBackground -yoffset 1i
    } msg] $msg
} {0 {}}

test background.840 {background configure -yoffset -10} {
    list [catch {
	blt::background configure myBackground -yoffset -10
    } msg] $msg
} {0 {}}

test background.841 {background configure -yoffset 10c} {
    list [catch {
	blt::background configure myBackground -yoffset 10c
    } msg] $msg
} {0 {}}

test background.842 {background configure -yoffset 0} {
    list [catch {
	blt::background configure myBackground -yoffset 0
    } msg] $msg
} {0 {}}

test background.843 {background configure -yoffset badValue} {
    list [catch {
	blt::background configure myBackground -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test background.844 {background configure -yoffset} {
    list [catch {
	blt::background configure myBackground -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test background.845 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.846 {background configure -relativeto badColor} {
    list [catch {
	blt::background configure myBackground -relativeto badRef
    } msg] $msg
} {1 {unknown reference type "badRef"}}

test background.847 {background configure -relativeto self} {
    list [catch {
	blt::background configure myBackground -relativeto self
    } msg] $msg
} {0 {}}

test background.848 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.849 {background configure -relativeto .} {
    list [catch {
	blt::background configure myBackground -relativeto .
    } msg] $msg
} {0 {}}

test background.850 {background configure -relativeto .badWindow} {
    list [catch {
	blt::background configure myBackground -relativeto .badWindow
    } msg] $msg
} {0 {}}

test background.851 {background configure -relativeto .badWindow} {
    list [catch {
	blt::tk::label .test -background myBackground
	proc tkerror { mesg } {
	    global testmsg
	    set testmsg $mesg
	}
	wm withdraw .
	update
	destroy .test
	set testmsg
    } msg] $msg
} {0 {bad window path name ".badWindow"}}


test background.852 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel .badWindow}}

test background.853 {background configure -relativeto toplevel} {
    list [catch {
	blt::background configure myBackground -relativeto toplevel
    } msg] $msg
} {0 {}}

test background.854 {background configure -relativeto} {
    list [catch {
	blt::background configure myBackground -relativeto
    } msg] $msg
} {0 {-relativeto {} {} toplevel toplevel}}

test background.855 {background delete myBackground} {
    list [catch {blt::background delete myBackground} msg] $msg
} {0 {}}

#####
exit 0








