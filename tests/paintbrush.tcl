package require Tk
package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test paintbrush.1 {paintbrush no args} {
    list [catch {blt::paintbrush} msg] $msg
} {1 {wrong # args: should be one of...
  blt::paintbrush cget brushName option
  blt::paintbrush configure brushName ?option value ...?
  blt::paintbrush create type ?brushName? ?option value ...?
  blt::paintbrush delete ?brushName ...?
  blt::paintbrush exists brushName
  blt::paintbrush names brushName ?pattern ...?
  blt::paintbrush type brushName}}

test paintbrush.2 {paintbrush badOp} {
    list [catch {blt::paintbrush badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  blt::paintbrush cget brushName option
  blt::paintbrush configure brushName ?option value ...?
  blt::paintbrush create type ?brushName? ?option value ...?
  blt::paintbrush delete ?brushName ...?
  blt::paintbrush exists brushName
  blt::paintbrush names brushName ?pattern ...?
  blt::paintbrush type brushName}}

test paintbrush.3 {paintbrush create} {
    list [catch {blt::paintbrush create} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush create type ?brushName? ?option value ...?"}}

test paintbrush.4 {paintbrush create badType} {
    list [catch {blt::paintbrush create badType} msg] $msg
} {1 {unknown paintbrush type "badType"}}

test paintbrush.5 {paintbrush create linear} {
    list [catch {blt::paintbrush create linear} msg] $msg
} {0 paintbrush1}

test paintbrush.6 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 paintbrush1}

test paintbrush.7 {paintbrush type paintbrush1} {
    list [catch {blt::paintbrush type paintbrush1} msg] $msg
} {0 linear}

test paintbrush.8 {paintbrush exists} {
    list [catch {blt::paintbrush exists paintbrush1} msg] $msg
} {0 1}

test paintbrush.9 {paintbrush configure paintbrush1} {
    list [catch {blt::paintbrush configure paintbrush1} msg] $msg
} {0 {{-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-from {} {} {top center} {0.5 0.0}} {-highcolor {} {} grey90 #e5e5e5} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-opacity {} {} 100.0 100.0} {-palette {} {} {} {}} {-repeat {} {} no no} {-to {} {} {} {0.5 1.0}} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.10 {paintbrush cget} {
    list [catch {blt::paintbrush cget} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.11 {paintbrush cget paintbrush1} {
    list [catch {blt::paintbrush cget paintbrush1} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.12 {paintbrush cget paintbrush1 -badOption} {
    list [catch {blt::paintbrush cget paintbrush1 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test paintbrush.13 {paintbrush cget paintbrush1 -colorscale} {
    list [catch {blt::paintbrush cget paintbrush1 -colorscale} msg] $msg
} {0 linear}

test paintbrush.14 {paintbrush cget paintbrush1 -decreasing} {
    list [catch {blt::paintbrush cget paintbrush1 -decreasing} msg] $msg
} {0 0}

test paintbrush.15 {paintbrush cget paintbrush1 -from} {
    list [catch {blt::paintbrush cget paintbrush1 -from} msg] $msg
} {0 {0.5 0.0}}

test paintbrush.16 {paintbrush cget paintbrush1 -highcolor} {
    list [catch {blt::paintbrush cget paintbrush1 -highcolor} msg] $msg
} {0 #e5e5e5}

test paintbrush.17 {paintbrush cget paintbrush1 -jitter} {
    list [catch {blt::paintbrush cget paintbrush1 -jitter} msg] $msg
} {0 0.0}

test paintbrush.18 {paintbrush cget paintbrush1 -lowcolor} {
    list [catch {blt::paintbrush cget paintbrush1 -lowcolor} msg] $msg
} {0 #7f7f7f}

test paintbrush.19 {paintbrush cget paintbrush1 -palette} {
    list [catch {blt::paintbrush cget paintbrush1 -palette} msg] $msg
} {0 {}}

test paintbrush.20 {paintbrush cget paintbrush1 -repeat} {
    list [catch {blt::paintbrush cget paintbrush1 -repeat} msg] $msg
} {0 no}

test paintbrush.21 {paintbrush cget paintbrush1 -to} {
    list [catch {blt::paintbrush cget paintbrush1 -to} msg] $msg
} {0 {0.5 1.0}}

test paintbrush.22 {paintbrush cget paintbrush1 -xoffset} {
    list [catch {blt::paintbrush cget paintbrush1 -xoffset} msg] $msg
} {0 0}

test paintbrush.23 {paintbrush cget paintbrush1 -yoffset} {
    list [catch {blt::paintbrush cget paintbrush1 -yoffset} msg] $msg
} {0 0}

test paintbrush.24 {paintbrush cget paintbrush1} {
    list [catch {blt::paintbrush cget paintbrush1} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.25 {paintbrush delete} {
    list [catch {blt::paintbrush delete} msg] $msg
} {0 {}}

test paintbrush.26 {paintbrush delete badName} {
    list [catch {blt::paintbrush delete badName} msg] $msg
} {1 {can't find paintbrush "badName"}}

test paintbrush.27 {paintbrush delete paintbrush1} {
    list [catch {blt::paintbrush delete paintbrush1} msg] $msg
} {0 {}}

test paintbrush.28 {paintbrush exists paintbrush1} {
    list [catch {blt::paintbrush exists paintbrush1} msg] $msg
} {0 0}

test paintbrush.29 {paintbrush delete all paintbrushs} {
    list [catch {eval blt::paintbrush delete [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.30 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.31 {paintbrush create linear} {
    list [catch {blt::paintbrush create linear myPaintbrush} msg] $msg
} {0 myPaintbrush}

test paintbrush.32 {paintbrush exists} {
    list [catch {blt::paintbrush exists myPaintbrush} msg] $msg
} {0 1}

test paintbrush.33 {paintbrush type} {
    list [catch {blt::paintbrush type myPaintbrush} msg] $msg
} {0 linear}

test paintbrush.34 {paintbrush configure myPaintbrush} {
    list [catch {blt::paintbrush configure myPaintbrush} msg] $msg
} {0 {{-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-from {} {} {top center} {0.5 0.0}} {-highcolor {} {} grey90 #e5e5e5} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-opacity {} {} 100.0 100.0} {-palette {} {} {} {}} {-repeat {} {} no no} {-to {} {} {} {0.5 1.0}} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}


test paintbrush.35 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test paintbrush.36 {paintbrush configure -colorscale badScale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test paintbrush.37 {paintbrush configure -colorscale linear} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale linear
    } msg] $msg
} {0 {}}

test paintbrush.38 {paintbrush configure -colorscale log} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale log
    } msg] $msg
} {0 {}}

test paintbrush.39 {paintbrush configure -colorscale badScale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test paintbrush.40 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear logarithmic}}

test paintbrush.41 {paintbrush configure -colorscale linear} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale linear
    } msg] $msg
} {0 {}}

test paintbrush.42 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test paintbrush.43 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.44 {paintbrush configure -decreasing badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test paintbrush.45 {paintbrush configure -decreasing 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing 0
    } msg] $msg
} {0 {}}

test paintbrush.46 {paintbrush configure -decreasing 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing 1
    } msg] $msg
} {0 {}}

test paintbrush.47 {paintbrush configure -decreasing no} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing no
    } msg] $msg
} {0 {}}

test paintbrush.48 {paintbrush configure -decreasing yes} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing yes
    } msg] $msg
} {0 {}}

test paintbrush.49 {paintbrush configure -decreasing on} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing on
    } msg] $msg
} {0 {}}

test paintbrush.50 {paintbrush configure -decreasing off} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing off
    } msg] $msg
} {0 {}}

test paintbrush.51 {paintbrush configure -decreasing true} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing true
    } msg] $msg
} {0 {}}

test paintbrush.52 {paintbrush configure -decreasing false} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing false
    } msg] $msg
} {0 {}}

test paintbrush.53 {paintbrush configure -decreasing badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test paintbrush.54 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.55 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.56 {paintbrush configure -from} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from
    } msg] $msg
} {0 {-from {} {} {top center} {0.5 0.0}}}

test paintbrush.57 {paintbrush configure -from badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from badValue
    } msg] $msg
} {1 {unknown position "badValue": should be nw, n, ne, w, c, e, sw, s, or se.}}

test paintbrush.58 {paintbrush configure -from {badValue badValue}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {badValue badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test paintbrush.59 {paintbrush configure -from {badValue center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {badValue center}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test paintbrush.60 {paintbrush configure -from {center badValue}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {center badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be left, right, or center.}}

test paintbrush.61 {paintbrush configure -from {0.0 1.0}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {0.0 1.0}
    } msg] $msg
} {0 {}}

test paintbrush.62 {paintbrush configure -from {2000.0 -1000.0}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {2000.0 -1000.0}
    } msg] $msg
} {0 {}}

test paintbrush.63 {paintbrush configure -from nw} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from nw
    } msg] $msg
} {0 {}}

test paintbrush.64 {paintbrush configure -from n} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from n
    } msg] $msg
} {0 {}}

test paintbrush.65 {paintbrush configure -from ne} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from ne
    } msg] $msg
} {0 {}}

test paintbrush.66 {paintbrush configure -from sw} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from s
    } msg] $msg
} {0 {}}

test paintbrush.67 {paintbrush configure -from s} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from s
    } msg] $msg
} {0 {}}

test paintbrush.68 {paintbrush configure -from se} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from se
    } msg] $msg
} {0 {}}

test paintbrush.69 {paintbrush configure -from c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from c
    } msg] $msg
} {0 {}}

test paintbrush.70 {paintbrush configure -from {top left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {top left}
    } msg] $msg
} {0 {}}

test paintbrush.71 {paintbrush configure -from {top right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {top right}
    } msg] $msg
} {0 {}}

test paintbrush.72 {paintbrush configure -from {top center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {top center}
    } msg] $msg
} {0 {}}

test paintbrush.73 {paintbrush configure -from {bottom left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {bottom left}
    } msg] $msg
} {0 {}}

test paintbrush.74 {paintbrush configure -from {bottom right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {bottom right}
    } msg] $msg
} {0 {}}

test paintbrush.75 {paintbrush configure -from {bottom center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {bottom center}
    } msg] $msg
} {0 {}}

test paintbrush.76 {paintbrush configure -from {center left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {center left}
    } msg] $msg
} {0 {}}

test paintbrush.77 {paintbrush configure -from {center right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {center right}
    } msg] $msg
} {0 {}}

test paintbrush.78 {paintbrush configure -from {center center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {center center}
    } msg] $msg
} {0 {}}

test paintbrush.79 {paintbrush configure -from {top center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {top center}
    } msg] $msg
} {0 {}}

test paintbrush.80 {paintbrush configure -from {center center center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from {center center center}
    } msg] $msg
} {1 {unknown position "center center center": should be "top left" or "nw"}}


test paintbrush.81 {paintbrush configure -from} {
    list [catch {
	blt::paintbrush configure myPaintbrush -from
    } msg] $msg
} {0 {-from {} {} {top center} {0.5 0.0}}}


test paintbrush.82 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #e5e5e5}}

test paintbrush.83 {paintbrush configure -highcolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.84 {paintbrush configure -highcolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor green
    } msg] $msg
} {0 {}}

test paintbrush.85 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test paintbrush.86 {paintbrush configure -highcolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.87 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test paintbrush.88 {paintbrush configure -highcolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.89 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.90 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test paintbrush.91 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.92 {paintbrush configure -jitter -1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test paintbrush.93 {paintbrush configure -jitter 101} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test paintbrush.94 {paintbrush configure -jitter 1.1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 1.1
    } msg] $msg
} {0 {}}

test paintbrush.95 {paintbrush configure -jitter 99.99999999} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 99.9999999999
    } msg] $msg
} {0 {}}


test paintbrush.96 {paintbrush configure -jitter 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 0
    } msg] $msg
} {0 {}}

test paintbrush.97 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.98 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.99 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.100 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #7f7f7f}}

test paintbrush.101 {paintbrush configure -lowcolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.102 {paintbrush configure -lowcolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor green
    } msg] $msg
} {0 {}}

test paintbrush.103 {paintbrush configure -lowcolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.104 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #00ff00}}

test paintbrush.105 {paintbrush configure -lowcolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.106 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor 
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #d9d9d9}}

test paintbrush.107 {paintbrush configure -palette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test paintbrush.108 {paintbrush configure -palette badPalette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test paintbrush.109 {paintbrush configure -palette ""} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette ""
    } msg] $msg
} {0 {}}

test paintbrush.110 {paintbrush configure -palette myPalette} {
    list [catch {
	blt::palette create myPalette -colordata {black white} -colorformat name
	blt::paintbrush configure myPaintbrush -palette myPalette
	blt::palette delete myPalette
    } msg] $msg
} {0 {}}

test paintbrush.111 {paintbrush configure -palette ""} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette ""
    } msg] $msg
} {0 {}}

test paintbrush.112 {paintbrush configure -palette badPalette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test paintbrush.113 {paintbrush configure -palette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test paintbrush.114 {paintbrush configure -to} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to
    } msg] $msg
} {0 {-to {} {} {} {0.5 1.0}}}

test paintbrush.115 {paintbrush configure -to badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to badValue
    } msg] $msg
} {1 {unknown position "badValue": should be nw, n, ne, w, c, e, sw, s, or se.}}

test paintbrush.116 {paintbrush configure -to {badValue badValue}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {badValue badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test paintbrush.117 {paintbrush configure -to {badValue center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {badValue center}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test paintbrush.118 {paintbrush configure -to {center badValue}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {center badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be left, right, or center.}}

test paintbrush.119 {paintbrush configure -to {0.0 1.0}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {0.0 1.0}
    } msg] $msg
} {0 {}}

test paintbrush.120 {paintbrush configure -to {2000.0 -1000.0}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {2000.0 -1000.0}
    } msg] $msg
} {0 {}}

test paintbrush.121 {paintbrush configure -to nw} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to nw
    } msg] $msg
} {0 {}}

test paintbrush.122 {paintbrush configure -to n} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to n
    } msg] $msg
} {0 {}}

test paintbrush.123 {paintbrush configure -to ne} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to ne
    } msg] $msg
} {0 {}}

test paintbrush.124 {paintbrush configure -to sw} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to s
    } msg] $msg
} {0 {}}

test paintbrush.125 {paintbrush configure -to s} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to s
    } msg] $msg
} {0 {}}

test paintbrush.126 {paintbrush configure -to se} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to se
    } msg] $msg
} {0 {}}

test paintbrush.127 {paintbrush configure -to c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to c
    } msg] $msg
} {0 {}}

test paintbrush.128 {paintbrush configure -to {top left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {top left}
    } msg] $msg
} {0 {}}

test paintbrush.129 {paintbrush configure -to {top right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {top right}
    } msg] $msg
} {0 {}}

test paintbrush.130 {paintbrush configure -to {top center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {top center}
    } msg] $msg
} {0 {}}

test paintbrush.131 {paintbrush configure -to {bottom left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {bottom left}
    } msg] $msg
} {0 {}}

test paintbrush.132 {paintbrush configure -to {bottom right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {bottom right}
    } msg] $msg
} {0 {}}

test paintbrush.133 {paintbrush configure -to {bottom center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {bottom center}
    } msg] $msg
} {0 {}}

test paintbrush.134 {paintbrush configure -to {center left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {center left}
    } msg] $msg
} {0 {}}

test paintbrush.135 {paintbrush configure -to {center right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {center right}
    } msg] $msg
} {0 {}}

test paintbrush.136 {paintbrush configure -to {center center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {center center}
    } msg] $msg
} {0 {}}

test paintbrush.137 {paintbrush configure -to {top center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {top center}
    } msg] $msg
} {0 {}}

test paintbrush.138 {paintbrush configure -to {center center center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to {center center center}
    } msg] $msg
} {1 {unknown position "center center center": should be "top left" or "nw"}}

test paintbrush.139 {paintbrush configure -to} {
    list [catch {
	blt::paintbrush configure myPaintbrush -to
    } msg] $msg
} {0 {-to {} {} {} {0.5 0.0}}}

test paintbrush.140 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.141 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.142 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.143 {paintbrush configure -xoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.144 {paintbrush configure -xoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.145 {paintbrush configure -xoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.146 {paintbrush configure -xoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.147 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.148 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.149 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.150 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.151 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.152 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.153 {paintbrush configure -yoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.154 {paintbrush configure -yoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.155 {paintbrush configure -yoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.156 {paintbrush configure -yoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.157 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.158 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.159 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.160 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 myPaintbrush}

test paintbrush.161 {paintbrush delete myPaintbrush} {
    list [catch {blt::paintbrush delete myPaintbrush} msg] $msg
} {0 {}}


#####

test paintbrush.162 {paintbrush create radial} {
    list [catch {blt::paintbrush create radial} msg] $msg
} {0 paintbrush2}

test paintbrush.163 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 paintbrush2}

test paintbrush.164 {paintbrush type paintbrush2} {
    list [catch {blt::paintbrush type paintbrush2} msg] $msg
} {0 radial}

test paintbrush.165 {paintbrush exists} {
    list [catch {blt::paintbrush exists paintbrush2} msg] $msg
} {0 1}

test paintbrush.166 {paintbrush configure paintbrush2} {
    list [catch {blt::paintbrush configure paintbrush2} msg] $msg
} {0 {{-center {} {} c {0.5 0.5}} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-diameter {} {} 0.0 0.0} {-highcolor {} {} grey90 #e5e5e5} {-height {} {} 1.0 1.0} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-opacity {} {} 100.0 100.0} {-palette {} {} {} {}} {-repeat {} {} no no} {-width {} {} 1.0 1.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.167 {paintbrush cget} {
    list [catch {blt::paintbrush cget} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.168 {paintbrush cget paintbrush2} {
    list [catch {blt::paintbrush cget paintbrush2} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.169 {paintbrush cget paintbrush2 -badOption} {
    list [catch {blt::paintbrush cget paintbrush2 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test paintbrush.170 {paintbrush cget paintbrush2 -colorscale} {
    list [catch {blt::paintbrush cget paintbrush2 -colorscale} msg] $msg
} {0 linear}

test paintbrush.171 {paintbrush cget paintbrush2 -decreasing} {
    list [catch {blt::paintbrush cget paintbrush2 -decreasing} msg] $msg
} {0 0}

test paintbrush.172 {paintbrush cget paintbrush2 -center} {
    list [catch {blt::paintbrush cget paintbrush2 -center} msg] $msg
} {0 {0.5 0.5}}

test paintbrush.173 {paintbrush cget paintbrush2 -highcolor} {
    list [catch {blt::paintbrush cget paintbrush2 -highcolor} msg] $msg
} {0 #e5e5e5}

test paintbrush.174 {paintbrush cget paintbrush2 -jitter} {
    list [catch {blt::paintbrush cget paintbrush2 -jitter} msg] $msg
} {0 0.0}

test paintbrush.175 {paintbrush cget paintbrush2 -lowcolor} {
    list [catch {blt::paintbrush cget paintbrush2 -lowcolor} msg] $msg
} {0 #7f7f7f}

test paintbrush.176 {paintbrush cget paintbrush2 -palette} {
    list [catch {blt::paintbrush cget paintbrush2 -palette} msg] $msg
} {0 {}}

test paintbrush.177 {paintbrush cget paintbrush2 -repeat} {
    list [catch {blt::paintbrush cget paintbrush2 -repeat} msg] $msg
} {0 no}

test paintbrush.178 {paintbrush cget paintbrush2 -xoffset} {
    list [catch {blt::paintbrush cget paintbrush2 -xoffset} msg] $msg
} {0 0}

test paintbrush.179 {paintbrush cget paintbrush2 -yoffset} {
    list [catch {blt::paintbrush cget paintbrush2 -yoffset} msg] $msg
} {0 0}

test paintbrush.180 {paintbrush cget paintbrush2} {
    list [catch {blt::paintbrush cget paintbrush2} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.181 {paintbrush delete} {
    list [catch {blt::paintbrush delete} msg] $msg
} {0 {}}

test paintbrush.182 {paintbrush delete badName} {
    list [catch {blt::paintbrush delete badName} msg] $msg
} {1 {can't find paintbrush "badName"}}

test paintbrush.183 {paintbrush delete paintbrush2} {
    list [catch {blt::paintbrush delete paintbrush2} msg] $msg
} {0 {}}

test paintbrush.184 {paintbrush exists paintbrush2} {
    list [catch {blt::paintbrush exists paintbrush2} msg] $msg
} {0 0}

test paintbrush.185 {paintbrush delete all paintbrushs} {
    list [catch {eval blt::paintbrush delete [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.186 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.187 {paintbrush create radial} {
    list [catch {blt::paintbrush create radial myPaintbrush} msg] $msg
} {0 myPaintbrush}

test paintbrush.188 {paintbrush exists} {
    list [catch {blt::paintbrush exists myPaintbrush} msg] $msg
} {0 1}

test paintbrush.189 {paintbrush type} {
    list [catch {blt::paintbrush type myPaintbrush} msg] $msg
} {0 radial}

test paintbrush.190 {paintbrush configure myPaintbrush} {
    list [catch {blt::paintbrush configure myPaintbrush} msg] $msg
} {0 {{-center {} {} c {0.5 0.5}} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-diameter {} {} 0.0 0.0} {-highcolor {} {} grey90 #e5e5e5} {-height {} {} 1.0 1.0} {-jitter {} {} 0 0.0} {-lowcolor {} {} grey50 #7f7f7f} {-opacity {} {} 100.0 100.0} {-palette {} {} {} {}} {-repeat {} {} no no} {-width {} {} 1.0 1.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.191 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test paintbrush.192 {paintbrush configure -colorscale badScale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test paintbrush.193 {paintbrush configure -colorscale linear} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale linear
    } msg] $msg
} {0 {}}

test paintbrush.194 {paintbrush configure -colorscale log} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale log
    } msg] $msg
} {0 {}}

test paintbrush.195 {paintbrush configure -colorscale badScale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test paintbrush.196 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear logarithmic}}

test paintbrush.197 {paintbrush configure -colorscale linear} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale linear
    } msg] $msg
} {0 {}}

test paintbrush.198 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test paintbrush.199 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.200 {paintbrush configure -decreasing badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test paintbrush.201 {paintbrush configure -decreasing 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing 0
    } msg] $msg
} {0 {}}

test paintbrush.202 {paintbrush configure -decreasing 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing 1
    } msg] $msg
} {0 {}}

test paintbrush.203 {paintbrush configure -decreasing no} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing no
    } msg] $msg
} {0 {}}

test paintbrush.204 {paintbrush configure -decreasing yes} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing yes
    } msg] $msg
} {0 {}}

test paintbrush.205 {paintbrush configure -decreasing on} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing on
    } msg] $msg
} {0 {}}

test paintbrush.206 {paintbrush configure -decreasing off} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing off
    } msg] $msg
} {0 {}}

test paintbrush.207 {paintbrush configure -decreasing true} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing true
    } msg] $msg
} {0 {}}

test paintbrush.208 {paintbrush configure -decreasing false} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing false
    } msg] $msg
} {0 {}}

test paintbrush.209 {paintbrush configure -decreasing badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test paintbrush.210 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.211 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.212 {paintbrush configure -center} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center
    } msg] $msg
} {0 {-center {} {} c {0.5 0.5}}}

test paintbrush.213 {paintbrush configure -center badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center badValue
    } msg] $msg
} {1 {unknown position "badValue": should be nw, n, ne, w, c, e, sw, s, or se.}}

test paintbrush.214 {paintbrush configure -center {badValue badValue}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {badValue badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test paintbrush.215 {paintbrush configure -center {badValue center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {badValue center}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test paintbrush.216 {paintbrush configure -center {center badValue}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be left, right, or center.}}

test paintbrush.217 {paintbrush configure -center {0.0 1.0}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {0.0 1.0}
    } msg] $msg
} {0 {}}

test paintbrush.218 {paintbrush configure -center {2000.0 -1000.0}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {2000.0 -1000.0}
    } msg] $msg
} {0 {}}

test paintbrush.219 {paintbrush configure -center nw} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center nw
    } msg] $msg
} {0 {}}

test paintbrush.220 {paintbrush configure -center n} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center n
    } msg] $msg
} {0 {}}

test paintbrush.221 {paintbrush configure -center ne} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center ne
    } msg] $msg
} {0 {}}

test paintbrush.222 {paintbrush configure -center sw} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center s
    } msg] $msg
} {0 {}}

test paintbrush.223 {paintbrush configure -center s} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center s
    } msg] $msg
} {0 {}}

test paintbrush.224 {paintbrush configure -center se} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center se
    } msg] $msg
} {0 {}}

test paintbrush.225 {paintbrush configure -center c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center c
    } msg] $msg
} {0 {}}

test paintbrush.226 {paintbrush configure -center {top left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {top left}
    } msg] $msg
} {0 {}}

test paintbrush.227 {paintbrush configure -center {top right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {top right}
    } msg] $msg
} {0 {}}

test paintbrush.228 {paintbrush configure -center {top center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {top center}
    } msg] $msg
} {0 {}}

test paintbrush.229 {paintbrush configure -center {bottom left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {bottom left}
    } msg] $msg
} {0 {}}

test paintbrush.230 {paintbrush configure -center {bottom right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {bottom right}
    } msg] $msg
} {0 {}}

test paintbrush.231 {paintbrush configure -center {bottom center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {bottom center}
    } msg] $msg
} {0 {}}

test paintbrush.232 {paintbrush configure -center {center left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center left}
    } msg] $msg
} {0 {}}

test paintbrush.233 {paintbrush configure -center {center right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center right}
    } msg] $msg
} {0 {}}

test paintbrush.234 {paintbrush configure -center {center center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center center}
    } msg] $msg
} {0 {}}

test paintbrush.235 {paintbrush configure -center {top center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {top center}
    } msg] $msg
} {0 {}}

test paintbrush.236 {paintbrush configure -center {center center center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center center center}
    } msg] $msg
} {1 {unknown position "center center center": should be "top left" or "nw"}}


test paintbrush.237 {paintbrush configure -center} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center
    } msg] $msg
} {0 {-center {} {} c {0.5 0.0}}}

test paintbrush.238 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #e5e5e5}}

test paintbrush.239 {paintbrush configure -highcolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.240 {paintbrush configure -highcolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor green
    } msg] $msg
} {0 {}}

test paintbrush.241 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test paintbrush.242 {paintbrush configure -highcolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.243 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test paintbrush.244 {paintbrush configure -highcolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.245 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.246 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test paintbrush.247 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.248 {paintbrush configure -jitter -1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test paintbrush.249 {paintbrush configure -jitter 101} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test paintbrush.250 {paintbrush configure -jitter 1.1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 1.1
    } msg] $msg
} {0 {}}

test paintbrush.251 {paintbrush configure -jitter 99.99999999} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test paintbrush.252 {paintbrush configure -jitter 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 0
    } msg] $msg
} {0 {}}

test paintbrush.253 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.254 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.255 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.256 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #7f7f7f}}

test paintbrush.257 {paintbrush configure -lowcolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.258 {paintbrush configure -lowcolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor green
    } msg] $msg
} {0 {}}

test paintbrush.259 {paintbrush configure -lowcolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.260 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #00ff00}}

test paintbrush.261 {paintbrush configure -lowcolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.262 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor 
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #d9d9d9}}

test paintbrush.263 {paintbrush configure -palette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test paintbrush.264 {paintbrush configure -palette badPalette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test paintbrush.265 {paintbrush configure -palette ""} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette ""
    } msg] $msg
} {0 {}}

test paintbrush.266 {paintbrush configure -palette myPalette} {
    list [catch {
	blt::palette create myPalette -colordata {black white} -colorformat name
	blt::paintbrush configure myPaintbrush -palette myPalette
	blt::palette delete myPalette
    } msg] $msg
} {0 {}}

test paintbrush.267 {paintbrush configure -palette ""} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette ""
    } msg] $msg
} {0 {}}

test paintbrush.268 {paintbrush configure -palette badPalette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test paintbrush.269 {paintbrush configure -palette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test paintbrush.270 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.271 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.272 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.273 {paintbrush configure -xoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.274 {paintbrush configure -xoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.275 {paintbrush configure -xoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.276 {paintbrush configure -xoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.277 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.278 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.279 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.280 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.281 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.282 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.283 {paintbrush configure -yoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.284 {paintbrush configure -yoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.285 {paintbrush configure -yoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.286 {paintbrush configure -yoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.287 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.288 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.289 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.290 {paintbrush delete myPaintbrush} {
    list [catch {blt::paintbrush delete myPaintbrush} msg] $msg
} {0 {}}


#####

test paintbrush.291 {paintbrush create conical} {
    list [catch {blt::paintbrush create conical} msg] $msg
} {0 paintbrush3}

test paintbrush.292 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 paintbrush3}

test paintbrush.293 {paintbrush type paintbrush3} {
    list [catch {blt::paintbrush type paintbrush3} msg] $msg
} {0 conical}

test paintbrush.294 {paintbrush exists} {
    list [catch {blt::paintbrush exists paintbrush3} msg] $msg
} {0 1}

test paintbrush.295 {paintbrush configure paintbrush3} {
    list [catch {blt::paintbrush configure paintbrush3} msg] $msg
} {0 {{-center {} {} c {0.5 0.5}} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-highcolor {} {} grey90 #e5e5e5} {-jitter {} {} 0 0.0} {-opacity {} {} 100.0 100.0} {-palette {} {} {} {}} {-rotate {} {} 45.0 45.0} {-lowcolor {} {} grey50 #7f7f7f} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.296 {paintbrush cget} {
    list [catch {blt::paintbrush cget} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.297 {paintbrush cget paintbrush3} {
    list [catch {blt::paintbrush cget paintbrush3} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.298 {paintbrush cget paintbrush3 -badOption} {
    list [catch {blt::paintbrush cget paintbrush3 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test paintbrush.299 {paintbrush cget paintbrush3 -colorscale} {
    list [catch {blt::paintbrush cget paintbrush3 -colorscale} msg] $msg
} {0 linear}

test paintbrush.300 {paintbrush cget paintbrush3 -decreasing} {
    list [catch {blt::paintbrush cget paintbrush3 -decreasing} msg] $msg
} {0 0}

test paintbrush.301 {paintbrush cget paintbrush3 -center} {
    list [catch {blt::paintbrush cget paintbrush3 -center} msg] $msg
} {0 {0.5 0.5}}

test paintbrush.302 {paintbrush cget paintbrush3 -highcolor} {
    list [catch {blt::paintbrush cget paintbrush3 -highcolor} msg] $msg
} {0 #e5e5e5}

test paintbrush.303 {paintbrush cget paintbrush3 -jitter} {
    list [catch {blt::paintbrush cget paintbrush3 -jitter} msg] $msg
} {0 0.0}

test paintbrush.304 {paintbrush cget paintbrush3 -lowcolor} {
    list [catch {blt::paintbrush cget paintbrush3 -lowcolor} msg] $msg
} {0 #7f7f7f}

test paintbrush.305 {paintbrush cget paintbrush3 -palette} {
    list [catch {blt::paintbrush cget paintbrush3 -palette} msg] $msg
} {0 {}}

test paintbrush.306 {paintbrush cget paintbrush3 -xoffset} {
    list [catch {blt::paintbrush cget paintbrush3 -xoffset} msg] $msg
} {0 0}

test paintbrush.307 {paintbrush cget paintbrush3 -yoffset} {
    list [catch {blt::paintbrush cget paintbrush3 -yoffset} msg] $msg
} {0 0}

test paintbrush.308 {paintbrush cget paintbrush3} {
    list [catch {blt::paintbrush cget paintbrush3} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.309 {paintbrush delete} {
    list [catch {blt::paintbrush delete} msg] $msg
} {0 {}}

test paintbrush.310 {paintbrush delete badName} {
    list [catch {blt::paintbrush delete badName} msg] $msg
} {1 {can't find paintbrush "badName"}}

test paintbrush.311 {paintbrush delete paintbrush3} {
    list [catch {blt::paintbrush delete paintbrush3} msg] $msg
} {0 {}}

test paintbrush.312 {paintbrush exists paintbrush3} {
    list [catch {blt::paintbrush exists paintbrush3} msg] $msg
} {0 0}

test paintbrush.313 {paintbrush delete all paintbrushs} {
    list [catch {eval blt::paintbrush delete [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.314 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.315 {paintbrush create conical} {
    list [catch {blt::paintbrush create conical myPaintbrush} msg] $msg
} {0 myPaintbrush}

test paintbrush.316 {paintbrush exists} {
    list [catch {blt::paintbrush exists myPaintbrush} msg] $msg
} {0 1}

test paintbrush.317 {paintbrush type} {
    list [catch {blt::paintbrush type myPaintbrush} msg] $msg
} {0 conical}

test paintbrush.318 {paintbrush configure myPaintbrush} {
    list [catch {blt::paintbrush configure myPaintbrush} msg] $msg
} {0 {{-center {} {} c {0.5 0.5}} {-colorscale {} {} linear linear} {-decreasing {} {} 0 0} {-highcolor {} {} grey90 #e5e5e5} {-jitter {} {} 0 0.0} {-opacity {} {} 100.0 100.0} {-palette {} {} {} {}} {-rotate {} {} 45.0 45.0} {-lowcolor {} {} grey50 #7f7f7f} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.319 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test paintbrush.320 {paintbrush configure -colorscale badScale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test paintbrush.321 {paintbrush configure -colorscale linear} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale linear
    } msg] $msg
} {0 {}}

test paintbrush.322 {paintbrush configure -colorscale log} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale log
    } msg] $msg
} {0 {}}

test paintbrush.323 {paintbrush configure -colorscale badScale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale badScale
    } msg] $msg
} {1 {unknown color scale "badScale": should be linear or logarithmic.}}

test paintbrush.324 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear logarithmic}}

test paintbrush.325 {paintbrush configure -colorscale linear} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale linear
    } msg] $msg
} {0 {}}

test paintbrush.326 {paintbrush configure -colorscale} {
    list [catch {
	blt::paintbrush configure myPaintbrush -colorscale
    } msg] $msg
} {0 {-colorscale {} {} linear linear}}

test paintbrush.327 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.328 {paintbrush configure -decreasing badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test paintbrush.329 {paintbrush configure -decreasing 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing 0
    } msg] $msg
} {0 {}}

test paintbrush.330 {paintbrush configure -decreasing 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing 1
    } msg] $msg
} {0 {}}

test paintbrush.331 {paintbrush configure -decreasing no} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing no
    } msg] $msg
} {0 {}}

test paintbrush.332 {paintbrush configure -decreasing yes} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing yes
    } msg] $msg
} {0 {}}

test paintbrush.333 {paintbrush configure -decreasing on} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing on
    } msg] $msg
} {0 {}}

test paintbrush.334 {paintbrush configure -decreasing off} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing off
    } msg] $msg
} {0 {}}

test paintbrush.335 {paintbrush configure -decreasing true} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing true
    } msg] $msg
} {0 {}}

test paintbrush.336 {paintbrush configure -decreasing false} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing false
    } msg] $msg
} {0 {}}

test paintbrush.337 {paintbrush configure -decreasing badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test paintbrush.338 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.339 {paintbrush configure -decreasing} {
    list [catch {
	blt::paintbrush configure myPaintbrush -decreasing
    } msg] $msg
} {0 {-decreasing {} {} 0 0}}

test paintbrush.340 {paintbrush configure -center} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center
    } msg] $msg
} {0 {-center {} {} c {0.5 0.5}}}

test paintbrush.341 {paintbrush configure -center badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center badValue
    } msg] $msg
} {1 {unknown position "badValue": should be nw, n, ne, w, c, e, sw, s, or se.}}

test paintbrush.342 {paintbrush configure -center {badValue badValue}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {badValue badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test paintbrush.343 {paintbrush configure -center {badValue center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {badValue center}
    } msg] $msg
} {1 {unknown position "badValue": should be top, bottom, or center.}}

test paintbrush.344 {paintbrush configure -center {center badValue}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center badValue}
    } msg] $msg
} {1 {unknown position "badValue": should be left, right, or center.}}

test paintbrush.345 {paintbrush configure -center {0.0 1.0}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {0.0 1.0}
    } msg] $msg
} {0 {}}

test paintbrush.346 {paintbrush configure -center {2000.0 -1000.0}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {2000.0 -1000.0}
    } msg] $msg
} {0 {}}

test paintbrush.347 {paintbrush configure -center nw} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center nw
    } msg] $msg
} {0 {}}

test paintbrush.348 {paintbrush configure -center n} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center n
    } msg] $msg
} {0 {}}

test paintbrush.349 {paintbrush configure -center ne} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center ne
    } msg] $msg
} {0 {}}

test paintbrush.350 {paintbrush configure -center sw} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center s
    } msg] $msg
} {0 {}}

test paintbrush.351 {paintbrush configure -center s} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center s
    } msg] $msg
} {0 {}}

test paintbrush.352 {paintbrush configure -center se} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center se
    } msg] $msg
} {0 {}}

test paintbrush.353 {paintbrush configure -center c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center c
    } msg] $msg
} {0 {}}

test paintbrush.354 {paintbrush configure -center {top left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {top left}
    } msg] $msg
} {0 {}}

test paintbrush.355 {paintbrush configure -center {top right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {top right}
    } msg] $msg
} {0 {}}

test paintbrush.356 {paintbrush configure -center {top center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {top center}
    } msg] $msg
} {0 {}}

test paintbrush.357 {paintbrush configure -center {bottom left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {bottom left}
    } msg] $msg
} {0 {}}

test paintbrush.358 {paintbrush configure -center {bottom right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {bottom right}
    } msg] $msg
} {0 {}}

test paintbrush.359 {paintbrush configure -center {bottom center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {bottom center}
    } msg] $msg
} {0 {}}

test paintbrush.360 {paintbrush configure -center {center left}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center left}
    } msg] $msg
} {0 {}}

test paintbrush.361 {paintbrush configure -center {center right}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center right}
    } msg] $msg
} {0 {}}

test paintbrush.362 {paintbrush configure -center {center center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center center}
    } msg] $msg
} {0 {}}

test paintbrush.363 {paintbrush configure -center {top center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {top center}
    } msg] $msg
} {0 {}}

test paintbrush.364 {paintbrush configure -center {center center center}} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center {center center center}
    } msg] $msg
} {1 {unknown position "center center center": should be "top left" or "nw"}}


test paintbrush.365 {paintbrush configure -center} {
    list [catch {
	blt::paintbrush configure myPaintbrush -center
    } msg] $msg
} {0 {-center {} {} c {0.5 0.0}}}

test paintbrush.366 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #e5e5e5}}

test paintbrush.367 {paintbrush configure -highcolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.368 {paintbrush configure -highcolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor green
    } msg] $msg
} {0 {}}

test paintbrush.369 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test paintbrush.370 {paintbrush configure -highcolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.371 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor
    } msg] $msg
} {0 {-highcolor {} {} grey90 #00ff00}}

test paintbrush.372 {paintbrush configure -highcolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.373 {paintbrush configure -highcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -highcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.374 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test paintbrush.375 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.376 {paintbrush configure -jitter -1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test paintbrush.377 {paintbrush configure -jitter 101} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test paintbrush.378 {paintbrush configure -jitter 1.1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 1.1
    } msg] $msg
} {0 {}}

test paintbrush.379 {paintbrush configure -jitter 99.99999999} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test paintbrush.380 {paintbrush configure -jitter 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 0
    } msg] $msg
} {0 {}}

test paintbrush.381 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.382 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.383 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.384 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #7f7f7f}}

test paintbrush.385 {paintbrush configure -lowcolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.386 {paintbrush configure -lowcolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor green
    } msg] $msg
} {0 {}}

test paintbrush.387 {paintbrush configure -lowcolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.388 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #00ff00}}

test paintbrush.389 {paintbrush configure -lowcolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.390 {paintbrush configure -lowcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -lowcolor 
    } msg] $msg
} {0 {-lowcolor {} {} grey50 #d9d9d9}}

test paintbrush.391 {paintbrush configure -palette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test paintbrush.392 {paintbrush configure -palette badPalette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test paintbrush.393 {paintbrush configure -palette ""} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette ""
    } msg] $msg
} {0 {}}

test paintbrush.394 {paintbrush configure -palette myPalette} {
    list [catch {
	blt::palette create myPalette -colordata {black white} -colorformat name
	blt::paintbrush configure myPaintbrush -palette myPalette
	blt::palette delete myPalette
    } msg] $msg
} {0 {}}

test paintbrush.395 {paintbrush configure -palette ""} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette ""
    } msg] $msg
} {0 {}}

test paintbrush.396 {paintbrush configure -palette badPalette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette badPalette
    } msg] $msg
} {1 {can't find a palette "badPalette"}}

test paintbrush.397 {paintbrush configure -palette} {
    list [catch {
	blt::paintbrush configure myPaintbrush -palette
    } msg] $msg
} {0 {-palette {} {} {} {}}}

test paintbrush.398 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.399 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.400 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.401 {paintbrush configure -xoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.402 {paintbrush configure -xoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.403 {paintbrush configure -xoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.404 {paintbrush configure -xoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.405 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.406 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.407 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.408 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.409 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.410 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.411 {paintbrush configure -yoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.412 {paintbrush configure -yoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.413 {paintbrush configure -yoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.414 {paintbrush configure -yoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.415 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.416 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.417 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.418 {paintbrush delete myPaintbrush} {
    list [catch {blt::paintbrush delete myPaintbrush} msg] $msg
} {0 {}}

#####

test paintbrush.419 {paintbrush create stripes} {
    list [catch {blt::paintbrush create stripes} msg] $msg
} {0 paintbrush4}

test paintbrush.420 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 paintbrush4}

test paintbrush.421 {paintbrush type paintbrush4} {
    list [catch {blt::paintbrush type paintbrush4} msg] $msg
} {0 stripes}

test paintbrush.422 {paintbrush exists} {
    list [catch {blt::paintbrush exists paintbrush4} msg] $msg
} {0 1}

test paintbrush.423 {paintbrush configure paintbrush4} {
    list [catch {blt::paintbrush configure paintbrush4} msg] $msg
} {0 {{-jitter {} {} 0 0.0} {-offcolor {} {} grey97 #f7f7f7} {-oncolor {} {} grey90 #e5e5e5} {-opacity {} {} 100.0 100.0} {-orient {} {} vertical vertical} {-stride {} {} 2 2} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.424 {paintbrush cget} {
    list [catch {blt::paintbrush cget} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.425 {paintbrush cget paintbrush4} {
    list [catch {blt::paintbrush cget paintbrush4} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.426 {paintbrush cget paintbrush4 -badOption} {
    list [catch {blt::paintbrush cget paintbrush4 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test paintbrush.427 {paintbrush cget paintbrush4 -oncolor} {
    list [catch {blt::paintbrush cget paintbrush4 -oncolor} msg] $msg
} {0 #e5e5e5}

test paintbrush.428 {paintbrush cget paintbrush4 -jitter} {
    list [catch {blt::paintbrush cget paintbrush4 -jitter} msg] $msg
} {0 0.0}

test paintbrush.429 {paintbrush cget paintbrush4 -offcolor} {
    list [catch {blt::paintbrush cget paintbrush4 -offcolor} msg] $msg
} {0 #f7f7f7}

test paintbrush.430 {paintbrush cget paintbrush4 -xoffset} {
    list [catch {blt::paintbrush cget paintbrush4 -xoffset} msg] $msg
} {0 0}

test paintbrush.431 {paintbrush cget paintbrush4 -yoffset} {
    list [catch {blt::paintbrush cget paintbrush4 -yoffset} msg] $msg
} {0 0}

test paintbrush.432 {paintbrush cget paintbrush4} {
    list [catch {blt::paintbrush cget paintbrush4} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.433 {paintbrush delete} {
    list [catch {blt::paintbrush delete} msg] $msg
} {0 {}}

test paintbrush.434 {paintbrush delete badName} {
    list [catch {blt::paintbrush delete badName} msg] $msg
} {1 {can't find paintbrush "badName"}}

test paintbrush.435 {paintbrush delete paintbrush4} {
    list [catch {blt::paintbrush delete paintbrush4} msg] $msg
} {0 {}}

test paintbrush.436 {paintbrush exists paintbrush4} {
    list [catch {blt::paintbrush exists paintbrush4} msg] $msg
} {0 0}

test paintbrush.437 {paintbrush delete all paintbrushs} {
    list [catch {eval blt::paintbrush delete [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.438 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.439 {paintbrush create stripes} {
    list [catch {blt::paintbrush create stripes myPaintbrush} msg] $msg
} {0 myPaintbrush}

test paintbrush.440 {paintbrush exists} {
    list [catch {blt::paintbrush exists myPaintbrush} msg] $msg
} {0 1}

test paintbrush.441 {paintbrush type} {
    list [catch {blt::paintbrush type myPaintbrush} msg] $msg
} {0 stripes}

test paintbrush.442 {paintbrush configure myPaintbrush} {
    list [catch {blt::paintbrush configure myPaintbrush} msg] $msg
} {0 {{-jitter {} {} 0 0.0} {-offcolor {} {} grey97 #f7f7f7} {-oncolor {} {} grey90 #e5e5e5} {-opacity {} {} 100.0 100.0} {-orient {} {} vertical vertical} {-stride {} {} 2 2} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.443 {paintbrush configure -oncolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #e5e5e5}}

test paintbrush.444 {paintbrush configure -oncolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.445 {paintbrush configure -oncolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor green
    } msg] $msg
} {0 {}}

test paintbrush.446 {paintbrush configure -oncolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #00ff00}}

test paintbrush.447 {paintbrush configure -oncolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.448 {paintbrush configure -oncolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #00ff00}}

test paintbrush.449 {paintbrush configure -oncolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.450 {paintbrush configure -oncolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.451 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test paintbrush.452 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.453 {paintbrush configure -jitter -1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test paintbrush.454 {paintbrush configure -jitter 101} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test paintbrush.455 {paintbrush configure -jitter 1.1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 1.1
    } msg] $msg
} {0 {}}

test paintbrush.456 {paintbrush configure -jitter 99.99999999} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test paintbrush.457 {paintbrush configure -jitter 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 0
    } msg] $msg
} {0 {}}

test paintbrush.458 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.459 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.460 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.461 {paintbrush configure -offcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor
    } msg] $msg
} {0 {-offcolor {} {} grey97 #f7f7f7}}

test paintbrush.462 {paintbrush configure -offcolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.463 {paintbrush configure -offcolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor green
    } msg] $msg
} {0 {}}

test paintbrush.464 {paintbrush configure -offcolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.465 {paintbrush configure -offcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor
    } msg] $msg
} {0 {-offcolor {} {} grey97 #00ff00}}

test paintbrush.466 {paintbrush configure -offcolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.467 {paintbrush configure -offcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor 
    } msg] $msg
} {0 {-offcolor {} {} grey97 #d9d9d9}}


test paintbrush.468 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.469 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.470 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.471 {paintbrush configure -xoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.472 {paintbrush configure -xoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.473 {paintbrush configure -xoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.474 {paintbrush configure -xoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.475 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.476 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.477 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.478 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.479 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.480 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.481 {paintbrush configure -yoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.482 {paintbrush configure -yoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.483 {paintbrush configure -yoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.484 {paintbrush configure -yoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.485 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.486 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.487 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.488 {paintbrush configure -orient} {
    list [catch {
	blt::paintbrush configure myPaintbrush -orient
    } msg] $msg
} {0 {-orient {} {} vertical vertical}}

test paintbrush.489 {paintbrush configure -orient badOrient} {
    list [catch {
	blt::paintbrush configure myPaintbrush -orient badOrient
    } msg] $msg
} {1 {unknown orient value "badOrient": should be vertical or horizontal.}}

test paintbrush.490 {paintbrush configure -orient ""} {
    list [catch {
	blt::paintbrush configure myPaintbrush -orient ""
    } msg] $msg
} {1 {unknown orient value "": should be vertical or horizontal.}}

test paintbrush.491 {paintbrush configure -orient horizontal} {
    list [catch {
	blt::paintbrush configure myPaintbrush -orient horizontal
    } msg] $msg
} {0 {}}

test paintbrush.492 {paintbrush configure -orient vertical} {
    list [catch {
	blt::paintbrush configure myPaintbrush -orient vertical
    } msg] $msg
} {0 {}}

test paintbrush.493 {paintbrush configure -orient} {
    list [catch {
	blt::paintbrush configure myPaintbrush -orient
    } msg] $msg
} {0 {-orient {} {} vertical vertical}}

test paintbrush.494 {paintbrush configure -stride} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride
    } msg] $msg
} {0 {-stride {} {} 2 2}}

test paintbrush.495 {paintbrush configure -stride badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.496 {paintbrush configure -stride 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 0
    } msg] $msg
} {1 {bad distance "0": must be positive}}

test paintbrush.497 {paintbrush configure -stride 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 1
    } msg] $msg
} {0 {}}

test paintbrush.498 {paintbrush configure -stride 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 1i
    } msg] $msg
} {0 {}}

test paintbrush.499 {paintbrush configure -stride -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride -10
    } msg] $msg
} {1 {bad distance "-10": must be positive}}

test paintbrush.500 {paintbrush configure -stride 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 10c
    } msg] $msg
} {0 {}}

test paintbrush.501 {paintbrush configure -stride 2} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 2
    } msg] $msg
} {0 {}}

test paintbrush.502 {paintbrush configure -stride badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.503 {paintbrush configure -stride} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride
    } msg] $msg
} {0 {-stride {} {} 2 2}}

test paintbrush.504 {paintbrush delete myPaintbrush} {
    list [catch {blt::paintbrush delete myPaintbrush} msg] $msg
} {0 {}}

#####

test paintbrush.505 {paintbrush create checkers} {
    list [catch {blt::paintbrush create checkers} msg] $msg
} {0 paintbrush5}

test paintbrush.506 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 paintbrush5}

test paintbrush.507 {paintbrush type paintbrush5} {
    list [catch {blt::paintbrush type paintbrush5} msg] $msg
} {0 checkers}

test paintbrush.508 {paintbrush exists} {
    list [catch {blt::paintbrush exists paintbrush5} msg] $msg
} {0 1}

test paintbrush.509 {paintbrush configure paintbrush5} {
    list [catch {blt::paintbrush configure paintbrush5} msg] $msg
} {0 {{-jitter {} {} 0 0.0} {-offcolor {} {} grey97 #f7f7f7} {-oncolor {} {} grey90 #e5e5e5} {-opacity {} {} 100.0 100.0} {-stride {} {} 10 10} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.510 {paintbrush cget} {
    list [catch {blt::paintbrush cget} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.511 {paintbrush cget paintbrush5} {
    list [catch {blt::paintbrush cget paintbrush5} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.512 {paintbrush cget paintbrush5 -badOption} {
    list [catch {blt::paintbrush cget paintbrush5 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test paintbrush.513 {paintbrush cget paintbrush5 -oncolor} {
    list [catch {blt::paintbrush cget paintbrush5 -oncolor} msg] $msg
} {0 #e5e5e5}

test paintbrush.514 {paintbrush cget paintbrush5 -jitter} {
    list [catch {blt::paintbrush cget paintbrush5 -jitter} msg] $msg
} {0 0.0}

test paintbrush.515 {paintbrush cget paintbrush5 -offcolor} {
    list [catch {blt::paintbrush cget paintbrush5 -offcolor} msg] $msg
} {0 #f7f7f7}

test paintbrush.516 {paintbrush cget paintbrush5 -xoffset} {
    list [catch {blt::paintbrush cget paintbrush5 -xoffset} msg] $msg
} {0 0}

test paintbrush.517 {paintbrush cget paintbrush5 -yoffset} {
    list [catch {blt::paintbrush cget paintbrush5 -yoffset} msg] $msg
} {0 0}

test paintbrush.518 {paintbrush cget paintbrush5} {
    list [catch {blt::paintbrush cget paintbrush5} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.519 {paintbrush delete} {
    list [catch {blt::paintbrush delete} msg] $msg
} {0 {}}

test paintbrush.520 {paintbrush delete badName} {
    list [catch {blt::paintbrush delete badName} msg] $msg
} {1 {can't find paintbrush "badName"}}

test paintbrush.521 {paintbrush delete paintbrush5} {
    list [catch {blt::paintbrush delete paintbrush5} msg] $msg
} {0 {}}

test paintbrush.522 {paintbrush exists paintbrush5} {
    list [catch {blt::paintbrush exists paintbrush5} msg] $msg
} {0 0}

test paintbrush.523 {paintbrush delete all paintbrushs} {
    list [catch {eval blt::paintbrush delete [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.524 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.525 {paintbrush create checkers} {
    list [catch {blt::paintbrush create checkers myPaintbrush} msg] $msg
} {0 myPaintbrush}

test paintbrush.526 {paintbrush exists} {
    list [catch {blt::paintbrush exists myPaintbrush} msg] $msg
} {0 1}

test paintbrush.527 {paintbrush type} {
    list [catch {blt::paintbrush type myPaintbrush} msg] $msg
} {0 checkers}

test paintbrush.528 {paintbrush configure myPaintbrush} {
    list [catch {blt::paintbrush configure myPaintbrush} msg] $msg
} {0 {{-jitter {} {} 0 0.0} {-offcolor {} {} grey97 #f7f7f7} {-oncolor {} {} grey90 #e5e5e5} {-opacity {} {} 100.0 100.0} {-stride {} {} 10 10} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.529 {paintbrush configure -oncolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #e5e5e5}}

test paintbrush.530 {paintbrush configure -oncolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.531 {paintbrush configure -oncolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor green
    } msg] $msg
} {0 {}}

test paintbrush.532 {paintbrush configure -oncolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #00ff00}}

test paintbrush.533 {paintbrush configure -oncolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.534 {paintbrush configure -oncolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor
    } msg] $msg
} {0 {-oncolor {} {} grey90 #00ff00}}

test paintbrush.535 {paintbrush configure -oncolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.536 {paintbrush configure -oncolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -oncolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.537 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test paintbrush.538 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.539 {paintbrush configure -jitter -1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test paintbrush.540 {paintbrush configure -jitter 101} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test paintbrush.541 {paintbrush configure -jitter 1.1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 1.1
    } msg] $msg
} {0 {}}

test paintbrush.542 {paintbrush configure -jitter 99.99999999} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test paintbrush.543 {paintbrush configure -jitter 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 0
    } msg] $msg
} {0 {}}

test paintbrush.544 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.545 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.546 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.547 {paintbrush configure -offcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor
    } msg] $msg
} {0 {-offcolor {} {} grey97 #f7f7f7}}

test paintbrush.548 {paintbrush configure -offcolor badColor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor badColor
    } msg] $msg
} {1 {bad color specification "badColor"}}

test paintbrush.549 {paintbrush configure -offcolor green} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor green
    } msg] $msg
} {0 {}}

test paintbrush.550 {paintbrush configure -offcolor myPaintbrush} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor myPaintbrush
    } msg] $msg
} {1 {bad color specification "myPaintbrush"}}

test paintbrush.551 {paintbrush configure -offcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor
    } msg] $msg
} {0 {-offcolor {} {} grey97 #00ff00}}

test paintbrush.552 {paintbrush configure -offcolor #d9d9d9} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor #d9d9d9
    } msg] $msg
} {0 {}}

test paintbrush.553 {paintbrush configure -offcolor} {
    list [catch {
	blt::paintbrush configure myPaintbrush -offcolor 
    } msg] $msg
} {0 {-offcolor {} {} grey97 #d9d9d9}}


test paintbrush.554 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.555 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.556 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.557 {paintbrush configure -xoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.558 {paintbrush configure -xoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.559 {paintbrush configure -xoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.560 {paintbrush configure -xoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.561 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.562 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.563 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.564 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.565 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.566 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.567 {paintbrush configure -yoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.568 {paintbrush configure -yoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.569 {paintbrush configure -yoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.570 {paintbrush configure -yoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.571 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.572 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.573 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.574 {paintbrush configure -stride} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride
    } msg] $msg
} {0 {-stride {} {} 10 10}}


test paintbrush.575 {paintbrush configure -stride badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.576 {paintbrush configure -stride 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 0
    } msg] $msg
} {1 {bad distance "0": must be positive}}

test paintbrush.577 {paintbrush configure -stride 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 1
    } msg] $msg
} {0 {}}

test paintbrush.578 {paintbrush configure -stride 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 1i
    } msg] $msg
} {0 {}}

test paintbrush.579 {paintbrush configure -stride -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride -10
    } msg] $msg
} {1 {bad distance "-10": must be positive}}

test paintbrush.580 {paintbrush configure -stride 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 10c
    } msg] $msg
} {0 {}}

test paintbrush.581 {paintbrush configure -stride 10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride 10
    } msg] $msg
} {0 {}}

test paintbrush.582 {paintbrush configure -stride badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.583 {paintbrush configure -stride} {
    list [catch {
	blt::paintbrush configure myPaintbrush -stride
    } msg] $msg
} {0 {-stride {} {} 10 10}}

test paintbrush.584 {paintbrush delete myPaintbrush} {
    list [catch {blt::paintbrush delete myPaintbrush} msg] $msg
} {0 {}}

####

test paintbrush.585 {paintbrush create tile} {
    list [catch {blt::paintbrush create tile} msg] $msg
} {0 paintbrush6}

test paintbrush.586 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 paintbrush6}

test paintbrush.587 {paintbrush type paintbrush6} {
    list [catch {blt::paintbrush type paintbrush6} msg] $msg
} {0 tile}

test paintbrush.588 {paintbrush exists} {
    list [catch {blt::paintbrush exists paintbrush6} msg] $msg
} {0 1}

test paintbrush.589 {paintbrush configure paintbrush6} {
    list [catch {blt::paintbrush configure paintbrush6} msg] $msg
} {0 {{-image {} {} {} {}} {-jitter {} {} 0 0.0} {-opacity {} {} 100.0 100.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.590 {paintbrush cget} {
    list [catch {blt::paintbrush cget} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.591 {paintbrush cget paintbrush6} {
    list [catch {blt::paintbrush cget paintbrush6} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.592 {paintbrush cget paintbrush6 -badOption} {
    list [catch {blt::paintbrush cget paintbrush6 -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test paintbrush.593 {paintbrush cget paintbrush6 -image} {
    list [catch {blt::paintbrush cget paintbrush6 -image} msg] $msg
} {0 {}}

test paintbrush.594 {paintbrush cget paintbrush6 -jitter} {
    list [catch {blt::paintbrush cget paintbrush6 -jitter} msg] $msg
} {0 0.0}

test paintbrush.595 {paintbrush cget paintbrush6 -xoffset} {
    list [catch {blt::paintbrush cget paintbrush6 -xoffset} msg] $msg
} {0 0}

test paintbrush.596 {paintbrush cget paintbrush6 -yoffset} {
    list [catch {blt::paintbrush cget paintbrush6 -yoffset} msg] $msg
} {0 0}

test paintbrush.597 {paintbrush cget paintbrush6} {
    list [catch {blt::paintbrush cget paintbrush6} msg] $msg
} {1 {wrong # args: should be "blt::paintbrush cget brushName option"}}

test paintbrush.598 {paintbrush delete} {
    list [catch {blt::paintbrush delete} msg] $msg
} {0 {}}

test paintbrush.599 {paintbrush delete badName} {
    list [catch {blt::paintbrush delete badName} msg] $msg
} {1 {can't find paintbrush "badName"}}

test paintbrush.600 {paintbrush delete paintbrush6} {
    list [catch {blt::paintbrush delete paintbrush6} msg] $msg
} {0 {}}

test paintbrush.601 {paintbrush exists paintbrush6} {
    list [catch {blt::paintbrush exists paintbrush6} msg] $msg
} {0 0}

test paintbrush.602 {paintbrush delete all paintbrushs} {
    list [catch {eval blt::paintbrush delete [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.603 {paintbrush names} {
    list [catch {lsort [blt::paintbrush names]} msg] $msg
} {0 {}}

test paintbrush.604 {paintbrush create tile} {
    list [catch {blt::paintbrush create tile myPaintbrush} msg] $msg
} {0 myPaintbrush}

test paintbrush.605 {paintbrush exists} {
    list [catch {blt::paintbrush exists myPaintbrush} msg] $msg
} {0 1}

test paintbrush.606 {paintbrush type} {
    list [catch {blt::paintbrush type myPaintbrush} msg] $msg
} {0 tile}

test paintbrush.607 {paintbrush configure myPaintbrush} {
    list [catch {blt::paintbrush configure myPaintbrush} msg] $msg
} {0 {{-image {} {} {} {}} {-jitter {} {} 0 0.0} {-opacity {} {} 100.0 100.0} {-xoffset {} {} 0 0} {-yoffset {} {} 0 0}}}

test paintbrush.608 {paintbrush configure -image} {
    list [catch {
	blt::paintbrush configure myPaintbrush -image
    } msg] $msg
} {0 {-image {} {} {} {}}}

test paintbrush.609 {paintbrush configure -image badImage} {
    list [catch {
	blt::paintbrush configure myPaintbrush -image badImage
    } msg] $msg
} {1 {image "badImage" doesn't exist}}

test paintbrush.610 {paintbrush configure -image myImage} {
    list [catch {
	image create picture myImage -file files/blt98.gif
	blt::paintbrush configure myPaintbrush -image myImage
    } msg] $msg
} {0 {}}

test paintbrush.611 {paintbrush configure -image} {
    list [catch {
	blt::paintbrush configure myPaintbrush -image
    } msg] $msg
} {0 {-image {} {} {} myImage}}

test paintbrush.612 {paintbrush configure -image} {
    list [catch {
	image delete myImage
	blt::paintbrush configure myPaintbrush -image
    } msg] $msg
} {0 {-image {} {} {} {}}}

test paintbrush.613 {paintbrush configure -image badImage} {
    list [catch {
	blt::paintbrush configure myPaintbrush -image bagImage
    } msg] $msg
} {1 {image "bagImage" doesn't exist}}

test paintbrush.614 {paintbrush configure -image} {
    list [catch {
	blt::paintbrush configure myPaintbrush -image
    } msg] $msg
} {0 {-image {} {} {} {}}}


test paintbrush.615 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}


test paintbrush.616 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.617 {paintbrush configure -jitter -1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter -1
    } msg] $msg
} {1 {invalid percent jitter "-1" number should be between 0 and 100}}

test paintbrush.618 {paintbrush configure -jitter 101} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 101
    } msg] $msg
} {1 {invalid percent jitter "101" number should be between 0 and 100}}

test paintbrush.619 {paintbrush configure -jitter 1.1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 1.1
    } msg] $msg
} {0 {}}

test paintbrush.620 {paintbrush configure -jitter 99.99999999} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 99.9999999999
    } msg] $msg
} {0 {}}

test paintbrush.621 {paintbrush configure -jitter 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter 0
    } msg] $msg
} {0 {}}

test paintbrush.622 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.623 {paintbrush configure -jitter badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test paintbrush.624 {paintbrush configure -jitter} {
    list [catch {
	blt::paintbrush configure myPaintbrush -jitter
    } msg] $msg
} {0 {-jitter {} {} 0 0.0}}

test paintbrush.625 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.626 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.627 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.628 {paintbrush configure -xoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.629 {paintbrush configure -xoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.630 {paintbrush configure -xoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.631 {paintbrush configure -xoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.632 {paintbrush configure -xoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.633 {paintbrush configure -xoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.634 {paintbrush configure -xoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -xoffset
    } msg] $msg
} {0 {-xoffset {} {} 0 0}}

test paintbrush.635 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.636 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.637 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.638 {paintbrush configure -yoffset 1} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1
    } msg] $msg
} {0 {}}

test paintbrush.639 {paintbrush configure -yoffset 1i} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 1i
    } msg] $msg
} {0 {}}

test paintbrush.640 {paintbrush configure -yoffset -10} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset -10
    } msg] $msg
} {0 {}}

test paintbrush.641 {paintbrush configure -yoffset 10c} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 10c
    } msg] $msg
} {0 {}}

test paintbrush.642 {paintbrush configure -yoffset 0} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset 0
    } msg] $msg
} {0 {}}

test paintbrush.643 {paintbrush configure -yoffset badValue} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test paintbrush.644 {paintbrush configure -yoffset} {
    list [catch {
	blt::paintbrush configure myPaintbrush -yoffset
    } msg] $msg
} {0 {-yoffset {} {} 0 0}}

test paintbrush.645 {paintbrush delete myPaintbrush} {
    list [catch {blt::paintbrush delete myPaintbrush} msg] $msg
} {0 {}}

#####
exit 0









