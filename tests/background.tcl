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
  blt::background create type ?bgName? ?args ...?
  blt::background delete ?bgName ...?
  blt::background exists bgName
  blt::background names ?pattern?
  blt::background type bgName}}

test background.2 {background badOp} {
    list [catch {blt::background badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  blt::background cget bgName option
  blt::background configure bgName ?option value ...?
  blt::background create type ?bgName? ?args ...?
  blt::background delete ?bgName ...?
  blt::background exists bgName
  blt::background names ?pattern?
  blt::background type bgName}}

test background.3 {background create} {
    list [catch {blt::background create} msg] $msg
} {1 {wrong # args: should be "blt::background create type ?bgName? ?args ...?"}}

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


#####
exit 0

