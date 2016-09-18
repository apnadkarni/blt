package require Tk
package require BLT

if {[info procs test] != "test"} {
    source defs
}

if [file exists ../library] {
    set blt_library ../library
}

#set VERBOSE 1

test picture.1 {image create picture} {
    list [catch {image create picture} msg] $msg
} {0 image1}

test picture.2 {image names} {
    list [catch {image names} msg] $msg
} {0 image1}

test picture.3 {image type image1} {
    list [catch {image type image1} msg] $msg
} {0 picture}

test picture.4 {image types} {
    list [catch {image types} msg] [lsort $msg]
} {0 {bitmap photo picture}}

test picture.5 {image inuse image1} {
    list [catch {image inuse image1} msg] $msg
} {0 0}

test picture.6 {image inuse image1} {
    list [catch {
	label .test -image image1
	set result [image inuse image1]
	destroy .test
	set result
    } msg] $msg
} {0 1}

test picture.7 {image width image1} {
    list [catch {image width image1} msg] $msg
} {0 16}

test picture.8 {image height image1} {
    list [catch {image height image1} msg] $msg
} {0 16}


test picture.9 {image delete image1} {
    list [catch {image delete image1} msg] $msg
} {0 {}}

test picture.10 {image create picture -width 200 -height 200} {
    list [catch {image create picture -width 200 -height 200} msg] $msg
} {0 image2}

test picture.11 {image names} {
    list [catch {image names} msg] $msg
} {0 image2}

test picture.12 {image width image2} {
    list [catch {image width image2} msg] $msg
} {0 200}

test picture.13 {image height image2} {
    list [catch {image height image2} msg] $msg
} {0 200}

test picture.14 {image delete image2} {
    list [catch {image delete image2} msg] $msg
} {0 {}}

test picture.15 {image create picture -badOption} {
    list [catch {image create picture -badOption} msg] $msg
} {1 {unknown option "-badOption"}}

test picture.16 {image names} {
    list [catch {image names} msg] $msg
} {0 {}}

test picture.17 {image create picture -data} {
    list [catch {image create picture -data} msg] $msg
} {1 {value for "-data" missing}}

test picture.18 {image create picture -dither} {
    list [catch {image create picture -dither} msg] $msg
} {1 {value for "-dither" missing}}

test picture.19 {image create picture -file} {
    list [catch {image create picture -file} msg] $msg
} {1 {value for "-file" missing}}

test picture.20 {image create picture -filter} {
    list [catch {image create picture -filter} msg] $msg
} {1 {value for "-filter" missing}}

test picture.21 {image create picture -gamma} {
    list [catch {image create picture -gamma} msg] $msg
} {1 {value for "-gamma" missing}}

test picture.22 {image create picture -height} {
    list [catch {image create picture -height} msg] $msg
} {1 {value for "-height" missing}}

test picture.23 {image create picture -image} {
    list [catch {image create picture -image} msg] $msg
} {1 {value for "-image" missing}}

test picture.24 {image create picture -maxpect} {
    list [catch {image create picture -maxpect} msg] $msg
} {1 {value for "-maxpect" missing}}

test picture.25 {image create picture -rotate} {
    list [catch {image create picture -rotate} msg] $msg
} {1 {value for "-rotate" missing}}

test picture.26 {image create picture -sharpen} {
    list [catch {image create picture -sharpen} msg] $msg
} {1 {value for "-sharpen" missing}}

test picture.27 {image create picture -width} {
    list [catch {image create picture -width} msg] $msg
} {1 {value for "-width" missing}}

test picture.28 {image create picture -window} {
    list [catch {image create picture -window} msg] $msg
} {1 {value for "-window" missing}}

test picture.29 {image create picture myPicture} {
    list [catch {image create picture myPicture} msg] $msg
} {0 myPicture}

test picture.30 {image names} {
    list [catch {image names} msg] $msg
} {0 myPicture}

test picture.31 {myPicture} {
    list [catch {myPicture} msg] $msg
} {1 {wrong # args: should be one of...
  myPicture add pictOrColor ?switches ...?
  myPicture and pictOrColor ?switches ...?
  myPicture blank ?colorSpec?
  myPicture blur srcName width
  myPicture cget option
  myPicture colorblend bgName fgName ?switches ...?
  myPicture composite bgName fgName ?switches ...?
  myPicture configure ?option value ...?
  myPicture convolve srcName ?switches ...?
  myPicture copy srcName ?switches ...?
  myPicture crop x1 y1 x2 y2
  myPicture crossfade fromName toName ?switches ...?
  myPicture dissolve fromName toName ?switches ...?
  myPicture draw ?args ...?
  myPicture dup ?switches ...?
  myPicture emboss srcName ?azimuth elevation?
  myPicture export formatName ?switches ...?
  myPicture fade srcName factor
  myPicture flip x|y ?switches ...?
  myPicture gamma value
  myPicture get x y
  myPicture greyscale srcName
  myPicture height ?newHeight?
  myPicture import formatName ?switches ...?
  myPicture info 
  myPicture max pictOrColor ?switches ...?
  myPicture min pictOrColor ?switches ...?
  myPicture multiply float
  myPicture nand pictOrColor ?switches ...?
  myPicture nor pictOrColor ?switches ...?
  myPicture or pictOrColor ?switches ...?
  myPicture project srcName coords coords ?switches ...?
  myPicture put x y color
  myPicture quantize srcName numColors
  myPicture reflect srcName ?switches ...?
  myPicture resample srcName ?switches ...?
  myPicture rotate srcName angle
  myPicture select srcName ?color ...?
  myPicture sequence ?args ...?
  myPicture sharpen 
  myPicture snap windowName ?switches ...?
  myPicture subtract pictOrColor ?switches ...?
  myPicture width ?newWidth?
  myPicture wipe fromName toName ?switches ...?
  myPicture xor pictOrColor ?switches ...?}}

test picture.32 {myPicture badOper} {
    list [catch {myPicture badOper} msg] $msg
} {1 {bad operation "badOper": should be one of...
  myPicture add pictOrColor ?switches ...?
  myPicture and pictOrColor ?switches ...?
  myPicture blank ?colorSpec?
  myPicture blur srcName width
  myPicture cget option
  myPicture colorblend bgName fgName ?switches ...?
  myPicture composite bgName fgName ?switches ...?
  myPicture configure ?option value ...?
  myPicture convolve srcName ?switches ...?
  myPicture copy srcName ?switches ...?
  myPicture crop x1 y1 x2 y2
  myPicture crossfade fromName toName ?switches ...?
  myPicture dissolve fromName toName ?switches ...?
  myPicture draw ?args ...?
  myPicture dup ?switches ...?
  myPicture emboss srcName ?azimuth elevation?
  myPicture export formatName ?switches ...?
  myPicture fade srcName factor
  myPicture flip x|y ?switches ...?
  myPicture gamma value
  myPicture get x y
  myPicture greyscale srcName
  myPicture height ?newHeight?
  myPicture import formatName ?switches ...?
  myPicture info 
  myPicture max pictOrColor ?switches ...?
  myPicture min pictOrColor ?switches ...?
  myPicture multiply float
  myPicture nand pictOrColor ?switches ...?
  myPicture nor pictOrColor ?switches ...?
  myPicture or pictOrColor ?switches ...?
  myPicture project srcName coords coords ?switches ...?
  myPicture put x y color
  myPicture quantize srcName numColors
  myPicture reflect srcName ?switches ...?
  myPicture resample srcName ?switches ...?
  myPicture rotate srcName angle
  myPicture select srcName ?color ...?
  myPicture sequence ?args ...?
  myPicture sharpen 
  myPicture snap windowName ?switches ...?
  myPicture subtract pictOrColor ?switches ...?
  myPicture width ?newWidth?
  myPicture wipe fromName toName ?switches ...?
  myPicture xor pictOrColor ?switches ...?}}

test picture.33 {myPicture info} {
    list [catch {myPicture info} msg] $msg
} {0 {colors 1 premultipled 0 greyscale 1 opaque 0 width 16 height 16 count 1 index 0 format none}}

test picture.34 {myPicture configure} {
    list [catch {myPicture configure} msg] $msg
} {0 {{-data {} {} {} {}} {-dither {} {} 0 0} {-file {} {} {} {}} {-filter {} {} {} {}} {-gamma {} {} 1.0 1.0} {-height {} {} 0 0} {-image {} {} {} {}} {-maxpect {} {} 0 0} {-rotate {} {} 0.0 0.0} {-sharpen {} {} 0 0} {-width {} {} 0 0} {-window {} {} {} {}}}}

test picture.35 {myPicture configure -data } {
    list [catch {myPicture configure -data} msg] $msg
} {0 {-data {} {} {} {}}}

test picture.36 {myPicture configure -data badData} {
    list [catch {myPicture configure -data badData} msg] $msg
} {1 {premature end of base64 data}}

test picture.37 {myPicture configure -data data} {
    list [catch {
	blt::fencode base64 ./files/blt98.gif -data data
	myPicture configure -data $data
    } msg] $msg
} {0 {}}

test picture.38 {myPicture configure -dither} {
    list [catch {
	myPicture configure -dither
    } msg] $msg
} {0 {-dither {} {} 0 0}}

test picture.39 {myPicture configure -dither badValue} {
    list [catch {
	myPicture configure -dither badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test picture.40 {myPicture configure -dither true} {
    list [catch {
	myPicture configure -dither true
    } msg] $msg
} {0 {}}

test picture.41 {myPicture configure -dither false} {
    list [catch {
	myPicture configure -dither false
    } msg] $msg
} {0 {}}

test picture.42 {myPicture configure -dither on} {
    list [catch {
	myPicture configure -dither on
    } msg] $msg
} {0 {}}

test picture.43 {myPicture configure -dither off} {
    list [catch {
	myPicture configure -dither off
    } msg] $msg
} {0 {}}

test picture.44 {myPicture configure -dither yes} {
    list [catch {
	myPicture configure -dither yes
    } msg] $msg
} {0 {}}

test picture.45 {myPicture configure -dither no } {
    list [catch {
	myPicture configure -dither no
    } msg] $msg
} {0 {}}

test picture.46 {myPicture configure -dither 1} {
    list [catch {
	myPicture configure -dither 1
    } msg] $msg
} {0 {}}

test picture.47 {myPicture configure -dither 0 } {
    list [catch {
	myPicture configure -dither 0
    } msg] $msg
} {0 {}}

test picture.48 {myPicture configure -file } {
    list [catch {
	myPicture configure -file
    } msg] $msg
} {0 {-file {} {} {} {}}}

test picture.49 {myPicture configure -file badFile} {
    list [catch {
	myPicture configure -file badFile
    } msg] $msg
} {1 {couldn't open "badFile": no such file or directory}}

test picture.50 {myPicture cget -data } {
    list [catch {
	set data [myPicture cget -data]
	string length $data
    } msg] $msg
} {0 48960}

test picture.51 {myPicture configure -file files/blt98.gif} {
    list [catch {
	myPicture configure -file files/blt98.gif
    } msg] $msg
} {0 {}}

test picture.52 {myPicture configure -data } {
    list [catch {myPicture configure -data} msg] $msg
} {0 {-data {} {} {} {}}}

test picture.53 {myPicture configure -file ""} {
    list [catch {
	myPicture configure -file ""
    } msg] $msg
} {0 {}}

test picture.54 {myPicture configure -filter } {
    list [catch {
	myPicture configure -filter
    } msg] $msg
} {0 {-filter {} {} {} {}}}

test picture.55 {myPicture configure -filter badFilter } {
    list [catch {
	myPicture configure -filter badFilter
    } msg] $msg
} {1 {can't find filter "badFilter"}}

test picture.56 {myPicture configure -filter bell } {
    list [catch {
	myPicture configure -filter bell
    } msg] $msg
} {0 {}}

test picture.57 {myPicture configure -filter bessel } {
    list [catch {
	myPicture configure -filter bessel
    } msg] $msg
} {0 {}}

test picture.58 {myPicture configure -filter box } {
    list [catch {
	myPicture configure -filter box
    } msg] $msg
} {0 {}}

test picture.59 {myPicture configure -filter bspline } {
    list [catch {
	myPicture configure -filter bspline
    } msg] $msg
} {0 {}}

test picture.60 {myPicture configure -filter catrom } {
    list [catch {
	myPicture configure -filter catrom
    } msg] $msg
} {0 {}}

test picture.61 {myPicture configure -filter gaussian } {
    list [catch {
	myPicture configure -filter gaussian
    } msg] $msg
} {0 {}}

test picture.62 {myPicture configure -filter lanczos3 } {
    list [catch {
	myPicture configure -filter lanczos3
    } msg] $msg
} {0 {}}

test picture.63 {myPicture configure -filter mitchell } {
    list [catch {
	myPicture configure -filter mitchell
    } msg] $msg
} {0 {}}

test picture.64 {myPicture configure -filter sinc } {
    list [catch {
	myPicture configure -filter sinc
    } msg] $msg
} {0 {}}

test picture.65 {myPicture configure -filter tent } {
    list [catch {
	myPicture configure -filter tent
    } msg] $msg
} {0 {}}

test picture.66 {myPicture configure -filter triangle } {
    list [catch {
	myPicture configure -filter triangle
    } msg] $msg
} {0 {}}

test picture.67 {myPicture configure -filter "" } {
    list [catch {
	myPicture configure -filter ""
    } msg] $msg
} {0 {}}

test picture.68 {myPicture configure -gamma } {
    list [catch {
	myPicture configure -gamma
    } msg] $msg
} {0 {-gamma {} {} 1.0 1.0}}

test picture.69 {myPicture configure -gamma badValue} {
    list [catch {
	myPicture configure -gamma badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test picture.70 {myPicture configure -gamma neg Value} {
    list [catch {
	myPicture configure -gamma -1.0
    } msg] $msg
} {1 {gamma value can't be negative}}

test picture.71 {myPicture configure -gamma 0.0} {
    list [catch {
	myPicture configure -gamma 0.0
    } msg] $msg
} {1 {gamma value can't be zero}}

test picture.72 {myPicture configure -gamma 20.0} {
    list [catch {
	myPicture configure -gamma 26.0
	myPicture cget -gamma
    } msg] $msg
} {0 20.0}

test picture.73 {myPicture configure -gamma 2.0} {
    list [catch {
	myPicture configure -gamma 2.0
	myPicture cget -gamma
    } msg] $msg
} {0 2.0}

test picture.74 {myPicture configure -gamma 1.0} {
    list [catch {
	myPicture configure -gamma 1.0
	myPicture cget -gamma
    } msg] $msg
} {0 1.0}

test picture.75 {myPicture configure -height} {
    list [catch {
	myPicture configure -height
    } msg] $msg
} {0 {-height {} {} 0 0}}

test picture.76 {myPicture configure -height badValue} {
    list [catch {
	myPicture configure -height badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test picture.77 {myPicture configure -height 0} {
    list [catch {
	myPicture configure -height 1
    } msg] $msg
} {0 {}}

test picture.78 {myPicture configure -height -1} {
    list [catch {
	myPicture configure -height -1
    } msg] $msg
} {1 {bad distance "-1": can't be negative}}

test picture.79 {myPicture configure -height 1i} {
    list [catch {
	myPicture configure -height 1i
    } msg] $msg
} {0 {}}

test picture.80 {myPicture configure -height 500.01} {
    list [catch {
	myPicture configure -height 500.01
    } msg] $msg
} {0 {}}

test picture.81 {myPicture configure -image} {
    list [catch {
	myPicture configure -image
    } msg] $msg
} {0 {-image {} {} {} {}}}

test picture.82 {myPicture configure -image badImage} {
    list [catch {
	myPicture configure -image badImage
    } msg] $msg
} {1 {image "badImage" doesn't exist}}

test picture.83 {myPicture configure -image tkPhoto} {
    list [catch {
	set img [image create photo -file ./files/blt98.gif]
	myPicture configure -image $img
	image delete $img
    } msg] $msg
} {0 {}}

test picture.84 {myPicture configure -image tkBitmap} {
    list [catch {
	set img [image create bitmap \
		     -file ./files/hobbes.xbm \
		     -maskfile ./files/hobbes_mask.xbm]
	myPicture configure -image $img
	image delete $img
    } msg] $msg
} {0 {}}

test picture.85 {myPicture configure -image bltPicture} {
    list [catch {
	set img [image create picture -file ./files/blt98.gif]
	myPicture configure -image $img
	image delete $img
    } msg] $msg
} {0 {}}

test picture.86 {myPicture configure -image ""} {
    list [catch {
	myPicture configure -image ""
    } msg] $msg
} {1 {image "" doesn't exist}}

test picture.87 {myPicture configure -maxpect} {
    list [catch {
	myPicture configure -maxpect
    } msg] $msg
} {0 {-maxpect {} {} 0 0}}

test picture.88 {myPicture configure -maxpect badValue} {
    list [catch {
	myPicture configure -maxpect badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test picture.89 {myPicture configure -maxpect true} {
    list [catch {
	myPicture configure -maxpect true
    } msg] $msg
} {0 {}}

test picture.90 {myPicture configure -maxpect false} {
    list [catch {
	myPicture configure -maxpect false
    } msg] $msg
} {0 {}}

test picture.91 {myPicture configure -maxpect on} {
    list [catch {
	myPicture configure -maxpect on
    } msg] $msg
} {0 {}}

test picture.92 {myPicture configure -maxpect off} {
    list [catch {
	myPicture configure -maxpect off
    } msg] $msg
} {0 {}}

test picture.93 {myPicture configure -maxpect yes} {
    list [catch {
	myPicture configure -maxpect yes
    } msg] $msg
} {0 {}}

test picture.94 {myPicture configure -maxpect no } {
    list [catch {
	myPicture configure -maxpect no
    } msg] $msg
} {0 {}}

test picture.95 {myPicture configure -maxpect 1} {
    list [catch {
	myPicture configure -maxpect 1
    } msg] $msg
} {0 {}}

test picture.96 {myPicture configure -maxpect 0 } {
    list [catch {
	myPicture configure -maxpect 0
    } msg] $msg
} {0 {}}

test picture.97 {myPicture configure -rotate} {
    list [catch {
	myPicture configure -maxpect
    } msg] $msg
} {0 {-maxpect {} {} 0 0}}

test picture.98 {myPicture configure -rotate badValue} {
    list [catch {
	myPicture configure -rotate badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test picture.99 {myPicture configure -rotate 0} {
    list [catch {
	myPicture configure -rotate 0
    } msg] $msg
} {0 {}}

test picture.100 {myPicture configure -rotate 90.0} {
    list [catch {
	myPicture configure -rotate 90.0
    } msg] $msg
} {0 {}}

test picture.101 {myPicture configure -rotate 10000.0} {
    list [catch {
	myPicture configure -rotate 10000.0
    } msg] $msg
} {0 {}}

test picture.102 {myPicture configure -rotate 360} {
    list [catch {
	myPicture configure -rotate 360
    } msg] $msg
} {0 {}}

test picture.103 {myPicture configure -rotate 0} {
    list [catch {
	myPicture configure -rotate 0
    } msg] $msg
} {0 {}}

test picture.104 {myPicture configure -sharpen} {
    list [catch {
	myPicture configure -sharpen
    } msg] $msg
} {0 {-sharpen {} {} 0 0}}

test picture.105 {myPicture configure -sharpen badValue} {
    list [catch {
	myPicture configure -sharpen badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test picture.106 {myPicture configure -sharpen true} {
    list [catch {
	myPicture configure -sharpen true
    } msg] $msg
} {0 {}}

test picture.107 {myPicture configure -sharpen false} {
    list [catch {
	myPicture configure -sharpen false
    } msg] $msg
} {0 {}}

test picture.108 {myPicture configure -sharpen on} {
    list [catch {
	myPicture configure -sharpen on
    } msg] $msg
} {0 {}}

test picture.109 {myPicture configure -sharpen off} {
    list [catch {
	myPicture configure -sharpen off
    } msg] $msg
} {0 {}}

test picture.110 {myPicture configure -sharpen yes} {
    list [catch {
	myPicture configure -sharpen yes
    } msg] $msg
} {0 {}}

test picture.111 {myPicture configure -sharpen no } {
    list [catch {
	myPicture configure -sharpen no
    } msg] $msg
} {0 {}}

test picture.112 {myPicture configure -sharpen 1} {
    list [catch {
	myPicture configure -sharpen 1
    } msg] $msg
} {0 {}}

test picture.113 {myPicture configure -sharpen 0 } {
    list [catch {
	myPicture configure -sharpen 0
    } msg] $msg
} {0 {}}


test picture.114 {myPicture configure -width} {
    list [catch {
	myPicture configure -width
    } msg] $msg
} {0 {-width {} {} 0 0}}

test picture.115 {myPicture configure -width badValue} {
    list [catch {
	myPicture configure -width badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test picture.116 {myPicture configure -width 0} {
    list [catch {
	myPicture configure -width 1
    } msg] $msg
} {0 {}}

test picture.117 {myPicture configure -width -1} {
    list [catch {
	myPicture configure -width -1
    } msg] $msg
} {1 {bad distance "-1": can't be negative}}

test picture.118 {myPicture configure -width 1i} {
    list [catch {
	myPicture configure -width 1i
    } msg] $msg
} {0 {}}

test picture.119 {myPicture configure -width 500.01} {
    list [catch {
	myPicture configure -width 500.01
    } msg] $msg
} {0 {}}

test picture.120 {myPicture configure -window} {
    list [catch {
	myPicture configure -window
    } msg] $msg
} {0 {-window {} {} {} {}}}

test picture.121 {myPicture configure -window badWindow} {
    list [catch {
	myPicture configure -window badWindow
    } msg] $msg
} {1 {can't find window "badWindow"}}

test picture.122 {myPicture configure -window .} {
    list [catch {
	myPicture configure -window .
    } msg] $msg
} {0 {}}

test picture.123 {myPicture configure -window ""} {
    list [catch {
	myPicture configure -window ""
    } msg] $msg
} {0 {}}

test picture.124 {myPicture add (no args)} {
    list [catch {
	myPicture add
    } msg] $msg
} {1 {wrong # args: should be "myPicture add pictOrColor ?switches ...?"}}

test picture.125 {myPicture add badPicture} {
    list [catch {
	myPicture add badPicture
    } msg] $msg
} {1 {can't find picture "badPicture"}}

test picture.126 {myPicture add 0xbadValue} {
    list [catch {
	myPicture add 0xbadPicture
    } msg] $msg
} {1 {expected color value but got "0xbadPicture"}}

test picture.127 {myPicture add 0x0} {
    list [catch {
	myPicture add 0x0
    } msg] $msg
} {0 {}}

test picture.128 {myPicture add 0x0 -badSwitch } {
    list [catch {
	myPicture add 0x0 -badSwitch
    } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -invert 
   -mask mask}}

test picture.129 {myPicture add 0x0 -invert -badSwitch} {
    list [catch {
	myPicture add 0x0 -invert -badSwitch
    } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -invert 
   -mask mask}}

test picture.130 {myPicture add 0x0 -invert} {
    list [catch {
	myPicture add 0x0 -invert
    } msg] $msg
} {0 {}}

test picture.131 {myPicture add 0x0 -mask} {
    list [catch {
	myPicture add 0x0 -mask
    } msg] $msg
} {1 {value for "-mask" missing}}

test picture.132 {myPicture add 0x0 -mask badPicture} {
    list [catch {
	myPicture add 0x0 -mask badPicture
    } msg] $msg
} {1 {can't find picture "badPicture"}}

#### FIXME: add positive -mask test.

test picture.133 {myPicture and (no args)} {
    list [catch {
	myPicture and
    } msg] $msg
} {1 {wrong # args: should be "myPicture and pictOrColor ?switches ...?"}}

test picture.134 {myPicture and badPicture} {
    list [catch {
	myPicture and badPicture
    } msg] $msg
} {1 {can't find picture "badPicture"}}

test picture.135 {myPicture and 0xbadValue} {
    list [catch {
	myPicture and 0xbadPicture
    } msg] $msg
} {1 {expected color value but got "0xbadPicture"}}

test picture.136 {myPicture and 0x0} {
    list [catch {
	myPicture and 0x0
    } msg] $msg
} {0 {}}

test picture.137 {myPicture and 0x0 -badSwitch } {
    list [catch {
	myPicture and 0x0 -badSwitch
    } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -invert 
   -mask mask}}

test picture.138 {myPicture and 0x0 -invert -badSwitch} {
    list [catch {
	myPicture and 0x0 -invert -badSwitch
    } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -invert 
   -mask mask}}

test picture.139 {myPicture and 0x0 -invert} {
    list [catch {
	myPicture and 0x0 -invert
    } msg] $msg
} {0 {}}

test picture.140 {myPicture and 0x0 -mask} {
    list [catch {
	myPicture and 0x0 -mask
    } msg] $msg
} {1 {value for "-mask" missing}}

test picture.141 {myPicture and 0x0 -mask badPicture} {
    list [catch {
	myPicture and 0x0 -mask badPicture
    } msg] $msg
} {1 {can't find picture "badPicture"}}

#### FIXME: add positive -mask test.

test picture.142 {myPicture blank no args} {
    list [catch { myPicture blank } msg] $msg
} {0 {}}

test picture.143 {myPicture blank badColor} {
    list [catch { myPicture blank badColor } msg] $msg
} {1 {bad color specification "badColor"}}

test picture.144 {myPicture blank white} {
    list [catch { myPicture blank white } msg] $msg
} {0 {}}

test picture.145 {myPicture blank paintbrush} {
    list [catch {
	set brush [blt::paintbrush create linear]
	myPicture blank $brush
    } msg] $msg
} {1 {bad color specification "paintbrush1"}}

test picture.146 {myPicture blank backgroun} {
    list [catch {
	set bg [blt::background create linear]
	myPicture blank $bg
    } msg] $msg
} {1 {bad color specification "background1"}}

test picture.147 {myPicture blank 0x00000000} {
    list [catch { myPicture blank 0x00000000 } msg] $msg
} {0 {}}

test picture.148 {myPicture blur} {
    list [catch { myPicture blur} msg] $msg
} {1 {wrong # args: should be "myPicture blur srcName width"}}

test picture.149 {myPicture blur myPicture} {
    list [catch { myPicture blur myPicture} msg] $msg
} {1 {wrong # args: should be "myPicture blur srcName width"}}

test picture.150 {myPicture blur myPicture badArg} {
    list [catch { myPicture blur myPicture badArg} msg] $msg
} {1 {expected integer but got "badArg"}}

test picture.151 {myPicture blur myPicture 1.2} {
    list [catch { myPicture blur myPicture 1.2} msg] $msg
} {1 {expected integer but got "1.2"}}

test picture.152 {myPicture blur myPicture 8} {
    list [catch { myPicture blur myPicture 8} msg] $msg
} {0 {}}

test picture.153 {myPicture blur myPicture 80} {
    list [catch { myPicture blur myPicture 80} msg] $msg
} {0 {}}

test picture.154 {myPicture blur myPicture -8} {
    list [catch { myPicture blur myPicture -8} msg] $msg
} {1 {blur radius can't be negative}}

test picture.155 {myPicture blur myPicture 0} {
    list [catch { myPicture blur myPicture 0} msg] $msg
} {1 {radius of blur must be > 1 pixel wide}}

test picture.156 {myPicture blur myPicture 1} {
    list [catch { myPicture blur myPicture 1} msg] $msg
} {1 {radius of blur must be > 1 pixel wide}}

test picture.157 {myPicture blur myPicture 2} {
    list [catch { myPicture blur myPicture 2} msg] $msg
} {0 {}}

test picture.158 {myPicture blur myPicture 8000} {
    list [catch { myPicture blur myPicture 8000} msg] $msg
} {0 {}}

test picture.159 {myPicture cget no args} {
    list [catch { myPicture cget} msg] $msg
} {1 {wrong # args: should be "myPicture cget option"}}

test picture.160 {myPicture cget too many args} {
    list [catch { myPicture cget arg arg arg} msg] $msg
} {1 {wrong # args: should be "myPicture cget option"}}

test picture.161 {myPicture cget badOption} {
    list [catch { myPicture cget badOption} msg] $msg
} {1 {unknown option "badOption"}}

test picture.162 {myPicture cget -data} {
    list [catch { myPicture cget -data} msg] $msg
} {0 {}}

test picture.163 {myPicture cget -dither} {
    list [catch { myPicture cget -dither} msg] $msg
} {0 0}

test picture.164 {myPicture cget -file} {
    list [catch { myPicture cget -file} msg] $msg
} {0 {}}

test picture.165 {myPicture cget -filter} {
    list [catch { myPicture cget -filter} msg] $msg
} {0 {}}

test picture.166 {myPicture cget -gamma} {
    list [catch { myPicture cget -gamma} msg] $msg
} {0 1.0}

test picture.167 {myPicture cget -height} {
    list [catch { myPicture cget -height} msg] $msg
} {0 500}

test picture.168 {myPicture cget -image} {
    list [catch { myPicture cget -image} msg] $msg
} {0 {}}

test picture.169 {myPicture cget -maxpect} {
    list [catch { myPicture cget -maxpect} msg] $msg
} {0 0}

test picture.170 {myPicture cget -rotate} {
    list [catch { myPicture cget -rotate} msg] $msg
} {0 0.0}

test picture.171 {myPicture cget -sharpen} {
    list [catch { myPicture cget -sharpen} msg] $msg
} {0 0}

test picture.172 {myPicture cget -width} {
    list [catch { myPicture cget -width} msg] $msg
} {0 500}

test picture.173 {myPicture cget -window} {
    list [catch { myPicture cget -window} msg] $msg
} {0 {}}

test picture.174 {myPicture colorblend no args} {
    list [catch { myPicture colorblend} msg] $msg
} {1 {wrong # args: should be "myPicture colorblend bgName fgName ?switches ...?"}}

test picture.175 {myPicture colorblend one args} {
    list [catch { myPicture colorblend myPicture} msg] $msg
} {1 {wrong # args: should be "myPicture colorblend bgName fgName ?switches ...?"}}

test picture.176 {myPicture colorblend} {
    list [catch { myPicture colorblend myPicture myPicture} msg] $msg
} {0 {}}

test picture.177 {myPicture colorblend different size image} {
    list [catch {
	image create picture mySrc -width 20 -height 20
	myPicture colorblend myPicture mySrc
	image delete mySrc
    } msg] $msg
} {0 {}}

test picture.178 {myPicture colorblend badSwitch} {
    list [catch {
	myPicture colorblend myPicture myPicture -badSwitch
    } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -mode blendingMode
   -from bbox
   -to bbox}}

test picture.179 {myPicture colorblend missing value} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode badMode
    } msg] $msg
} {1 {can't find blend mode "badMode"}}

test picture.180 {myPicture colorblend -mode colorburn} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode colorburn
    } msg] $msg
} {0 {}}

test picture.181 {myPicture colorblend -mode colordodge} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode colordodge
    } msg] $msg
} {0 {}}

test picture.182 {myPicture colorblend -mode darken} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode darken
    } msg] $msg
} {0 {}}

test picture.183 {myPicture colorblend -mode difference} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode difference
    } msg] $msg
} {0 {}}

test picture.184 {myPicture colorblend -mode exclusion} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode exclusion
    } msg] $msg
} {0 {}}

test picture.185 {myPicture colorblend -mode hardlight} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode hardlight
    } msg] $msg
} {0 {}}

test picture.186 {myPicture colorblend -mode hardmix} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode hardmix
    } msg] $msg
} {0 {}}

test picture.187 {myPicture colorblend -mode lighten} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode lighten
    } msg] $msg
} {0 {}}

test picture.188 {myPicture colorblend -mode linearburn} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode linearburn
    } msg] $msg
} {0 {}}

test picture.189 {myPicture colorblend -mode lineardodge} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode lineardodge
    } msg] $msg
} {0 {}}

test picture.190 {myPicture colorblend -mode linearlight} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode linearlight
    } msg] $msg
} {0 {}}

test picture.191 {myPicture colorblend -mode normal} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode normal
    } msg] $msg
} {0 {}}

test picture.192 {myPicture colorblend -mode multiply} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode multiply
    } msg] $msg
} {0 {}}

test picture.193 {myPicture colorblend -mode screen} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode screen
    } msg] $msg
} {0 {}}

test picture.194 {myPicture colorblend -mode softlight} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode softlight
    } msg] $msg
} {0 {}}

test picture.195 {myPicture colorblend -mode subtract} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode subtract
    } msg] $msg
} {0 {}}

test picture.196 {myPicture colorblend -mode overlay} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode overlay
    } msg] $msg
} {0 {}}

test picture.197 {myPicture colorblend -mode pinlight} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode pinlight
    } msg] $msg
} {0 {}}

test picture.198 {myPicture colorblend -mode vividlight} {
    list [catch {
	myPicture colorblend myPicture myPicture -mode vividlight
    } msg] $msg
} {0 {}}

test picture.199 {myPicture colorblend -from} {
    list [catch {
	myPicture colorblend myPicture myPicture -from
    } msg] $msg
} {1 {value for "-from" missing}}

test picture.200 {myPicture colorblend -from badBBox} {
    list [catch {
	myPicture colorblend myPicture myPicture -from badBBox
    } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.201 {myPicture colorblend -from "badX 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "badX 0"
    } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.202 {myPicture colorblend -from "0 badY"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "0 badY"
    } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.203 {myPicture colorblend -from "0 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "0 0"
    } msg] $msg
} {0 {}}

test picture.204 {myPicture colorblend -from "0 0 badArg"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "0 0 badArg"
    } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.205 {myPicture colorblend -from "0 0 badX 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "0 0 badX 0"
    } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.206 {myPicture colorblend -from "0 0 0 badY"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "0 0 0 badY"
    } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.207 {myPicture colorblend -from "0 0 0 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "0 0 0 0"
    } msg] $msg
} {0 {}}

test picture.208 {myPicture colorblend -from "100 100 0 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "100 100 0 0"
    } msg] $msg
} {0 {}}

test picture.209 {myPicture colorblend -from "1000 1000 0 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "1000 1000 0 0"
    } msg] $msg
} {0 {}}

test picture.210 {myPicture colorblend -from "1000 1000 10000 10000"} {
    list [catch {
	myPicture colorblend myPicture myPicture -from "1000 1000 10000 10000"
    } msg] $msg
} {0 {}}

test picture.211 {myPicture colorblend -to} {
    list [catch {
	myPicture colorblend myPicture myPicture -to
    } msg] $msg
} {1 {value for "-to" missing}}

test picture.212 {myPicture colorblend -to badBBox} {
    list [catch {
	myPicture colorblend myPicture myPicture -to badBBox
    } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.213 {myPicture colorblend -to "badX 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "badX 0"
    } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.214 {myPicture colorblend -to "0 badY"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "0 badY"
    } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.215 {myPicture colorblend -to "0 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "0 0"
    } msg] $msg
} {0 {}}

test picture.216 {myPicture colorblend -to "0 0 badArg"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "0 0 badArg"
    } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.217 {myPicture colorblend -to "0 0 badX 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "0 0 badX 0"
    } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.218 {myPicture colorblend -to "0 0 0 badY"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "0 0 0 badY"
    } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.219 {myPicture colorblend -to "0 0 0 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "0 0 0 0"
    } msg] $msg
} {0 {}}

test picture.220 {myPicture colorblend -to "100 100 0 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "100 100 0 0"
    } msg] $msg
} {0 {}}

test picture.221 {myPicture colorblend -to "1000 1000 0 0"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "1000 1000 0 0"
    } msg] $msg
} {0 {}}

test picture.222 {myPicture colorblend -to "1000 1000 10000 10000"} {
    list [catch {
	myPicture colorblend myPicture myPicture -to "1000 1000 10000 10000"
    } msg] $msg
} {0 {}}

test picture.223 {myPicture composite no args} {
    list [catch { myPicture composite} msg] $msg
} {1 {wrong # args: should be "myPicture composite bgName fgName ?switches ...?"}}

test picture.224 {myPicture composite one args} {
    list [catch { myPicture composite myPicture} msg] $msg
} {1 {wrong # args: should be "myPicture composite bgName fgName ?switches ...?"}}

test picture.225 {myPicture composite} {
    list [catch { myPicture composite myPicture myPicture} msg] $msg
} {0 {}}

test picture.226 {myPicture composite different size image} {
    list [catch {
	image create picture mySrc -width 20 -height 20
	myPicture composite myPicture mySrc
	image delete mySrc
    } msg] $msg
} {0 {}}

test picture.227 {myPicture composite badSwitch} {
    list [catch {
	myPicture composite myPicture myPicture -badSwitch
    } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -from bbox
   -to bbox}}

test picture.228 {myPicture composite -from} {
    list [catch {
	myPicture composite myPicture myPicture -from
    } msg] $msg
} {1 {value for "-from" missing}}

test picture.229 {myPicture composite -from badBBox} {
    list [catch {
	myPicture composite myPicture myPicture -from badBBox
    } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.230 {myPicture composite -from "badX 0"} {
    list [catch {
	myPicture composite myPicture myPicture -from "badX 0"
    } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.231 {myPicture composite -from "0 badY"} {
    list [catch {
	myPicture composite myPicture myPicture -from "0 badY"
    } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.232 {myPicture composite -from "0 0"} {
    list [catch {
	myPicture composite myPicture myPicture -from "0 0"
    } msg] $msg
} {0 {}}

test picture.233 {myPicture composite -from "0 0 badArg"} {
    list [catch {
	myPicture composite myPicture myPicture -from "0 0 badArg"
    } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.234 {myPicture composite -from "0 0 badX 0"} {
    list [catch {
	myPicture composite myPicture myPicture -from "0 0 badX 0"
    } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.235 {myPicture composite -from "0 0 0 badY"} {
    list [catch {
	myPicture composite myPicture myPicture -from "0 0 0 badY"
    } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.236 {myPicture composite -from "0 0 0 0"} {
    list [catch {
	myPicture composite myPicture myPicture -from "0 0 0 0"
    } msg] $msg
} {0 {}}

test picture.237 {myPicture composite -from "100 100 0 0"} {
    list [catch {
	myPicture composite myPicture myPicture -from "100 100 0 0"
    } msg] $msg
} {0 {}}

test picture.238 {myPicture composite -from "1000 1000 0 0"} {
    list [catch {
	myPicture composite myPicture myPicture -from "1000 1000 0 0"
    } msg] $msg
} {0 {}}

test picture.239 {myPicture composite -from "1000 1000 10000 10000"} {
    list [catch {
	myPicture composite myPicture myPicture -from "1000 1000 10000 10000"
    } msg] $msg
} {1 {source bounding box lies outside of picture}}

test picture.240 {myPicture composite -to} {
    list [catch {
	myPicture composite myPicture myPicture -to
    } msg] $msg
} {1 {value for "-to" missing}}

test picture.241 {myPicture composite -to badBBox} {
    list [catch {
	myPicture composite myPicture myPicture -to badBBox
    } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.242 {myPicture composite -to "badX 0"} {
    list [catch {
	myPicture composite myPicture myPicture -to "badX 0"
    } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.243 {myPicture composite -to "0 badY"} {
    list [catch {
	myPicture composite myPicture myPicture -to "0 badY"
    } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.244 {myPicture composite -to "0 0"} {
    list [catch {
	myPicture composite myPicture myPicture -to "0 0"
    } msg] $msg
} {0 {}}

test picture.245 {myPicture composite -to "0 0 badArg"} {
    list [catch {
	myPicture composite myPicture myPicture -to "0 0 badArg"
    } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.246 {myPicture composite -to "0 0 badX 0"} {
    list [catch {
	myPicture composite myPicture myPicture -to "0 0 badX 0"
    } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.247 {myPicture composite -to "0 0 0 badY"} {
    list [catch {
	myPicture composite myPicture myPicture -to "0 0 0 badY"
    } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.248 {myPicture composite -to "0 0 0 0"} {
    list [catch {
	myPicture composite myPicture myPicture -to "0 0 0 0"
    } msg] $msg
} {0 {}}

test picture.249 {myPicture composite -to "100 100 0 0"} {
    list [catch {
	myPicture composite myPicture myPicture -to "100 100 0 0"
    } msg] $msg
} {0 {}}

test picture.250 {myPicture composite -to "1000 1000 0 0"} {
    list [catch {
	myPicture composite myPicture myPicture -to "1000 1000 0 0"
    } msg] $msg
} {0 {}}

test picture.251 {myPicture composite -to "1000 1000 10000 10000"} {
    list [catch {
	myPicture composite myPicture myPicture -to "1000 1000 10000 10000"
    } msg] $msg
} {1 {destination bounding box lies outside of picture}}

test picture.252 {myPicture convolve no args} {
    list [catch { myPicture convolve} msg] $msg
} {1 {wrong # args: should be "myPicture convolve srcName ?switches ...?"}}

test picture.253 {myPicture convolve one args} {
    list [catch { myPicture convolve myPicture} msg] $msg
} {0 {}}

test picture.254 {myPicture convolve} {
    list [catch { myPicture convolve myPicture myPicture} msg] $msg
} {1 {unknown switch "myPicture"
following switches are available:
   -filter filter
   -hfilter filter
   -vfilter filter}}

test picture.255 {myPicture convolve different size image} {
    list [catch {
	image create picture mySrc -width 20 -height 20
	myPicture convolve mySrc 
	image delete mySrc
    } msg] $msg
} {0 {}}

test picture.256 {myPicture convolve badSwitch} {
    list [catch {
	myPicture convolve myPicture -badSwitch
    } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -filter filter
   -hfilter filter
   -vfilter filter}}

test picture.257 {myPicture convolve -filter} {
    list [catch {
	myPicture convolve myPicture -filter
    } msg] $msg
} {1 {value for "-filter" missing}}

test picture.258 {myPicture convolve -filter badFilter} {
    list [catch {
	myPicture convolve myPicture -filter badFilter
    } msg] $msg
} {1 {can't find filter "badFilter"}}

test picture.259 {myPicture convolve myPicture -filter bessel } {
    list [catch {
	myPicture convolve myPicture -filter bessel
    } msg] $msg
} {0 {}}

test picture.260 {myPicture convolve myPicture -filter box } {
    list [catch {
	myPicture convolve myPicture -filter box
    } msg] $msg
} {0 {}}

test picture.261 {myPicture convolve myPicture -filter bspline } {
    list [catch {
	myPicture convolve myPicture -filter bspline
    } msg] $msg
} {0 {}}

test picture.262 {myPicture convolve myPicture -filter catrom } {
    list [catch {
	myPicture convolve myPicture -filter catrom
    } msg] $msg
} {0 {}}

test picture.263 {myPicture convolve myPicture -filter gaussian } {
    list [catch {
	myPicture convolve myPicture -filter gaussian
    } msg] $msg
} {0 {}}

test picture.264 {myPicture convolve myPicture -filter lanczos3 } {
    list [catch {
	myPicture convolve myPicture -filter lanczos3
    } msg] $msg
} {0 {}}

test picture.265 {myPicture convolve myPicture -filter mitchell } {
    list [catch {
	myPicture convolve myPicture -filter mitchell
    } msg] $msg
} {0 {}}

test picture.266 {myPicture convolve myPicture -filter sinc } {
    list [catch {
	myPicture convolve myPicture -filter sinc
    } msg] $msg
} {0 {}}

test picture.267 {myPicture convolve myPicture -filter tent } {
    list [catch {
	myPicture convolve myPicture -filter tent
    } msg] $msg
} {0 {}}

test picture.268 {myPicture convolve myPicture -filter triangle } {
    list [catch {
	myPicture convolve myPicture -filter triangle
    } msg] $msg
} {0 {}}

test picture.269 {myPicture convolve myPicture -filter "" } {
    list [catch {
	myPicture convolve myPicture -filter ""
    } msg] $msg
} {1 {can't find filter ""}}

test picture.270 {myPicture convolve -hfilter} {
    list [catch {
	myPicture convolve myPicture -hfilter
    } msg] $msg
} {1 {value for "-hfilter" missing}}

test picture.271 {myPicture convolve -hfilter badFilter} {
    list [catch {
	myPicture convolve myPicture -hfilter badFilter
    } msg] $msg
} {1 {can't find filter "badFilter"}}

test picture.272 {myPicture convolve myPicture -hfilter bessel } {
    list [catch {
	myPicture convolve myPicture -hfilter bessel
    } msg] $msg
} {0 {}}

test picture.273 {myPicture convolve myPicture -hfilter box } {
    list [catch {
	myPicture convolve myPicture -hfilter box
    } msg] $msg
} {0 {}}

test picture.274 {myPicture convolve myPicture -hfilter bspline } {
    list [catch {
	myPicture convolve myPicture -hfilter bspline
    } msg] $msg
} {0 {}}

test picture.275 {myPicture convolve myPicture -hfilter catrom } {
    list [catch {
	myPicture convolve myPicture -hfilter catrom
    } msg] $msg
} {0 {}}

test picture.276 {myPicture convolve myPicture -hfilter gaussian } {
    list [catch {
	myPicture convolve myPicture -hfilter gaussian
    } msg] $msg
} {0 {}}

test picture.277 {myPicture convolve myPicture -hfilter lanczos3 } {
    list [catch {
	myPicture convolve myPicture -hfilter lanczos3
    } msg] $msg
} {0 {}}

test picture.278 {myPicture convolve myPicture -hfilter mitchell } {
    list [catch {
	myPicture convolve myPicture -hfilter mitchell
    } msg] $msg
} {0 {}}

test picture.279 {myPicture convolve myPicture -hfilter sinc } {
    list [catch {
	myPicture convolve myPicture -hfilter sinc
    } msg] $msg
} {0 {}}

test picture.280 {myPicture convolve myPicture -hfilter tent } {
    list [catch {
	myPicture convolve myPicture -hfilter tent
    } msg] $msg
} {0 {}}

test picture.281 {myPicture convolve myPicture -hfilter triangle } {
    list [catch {
	myPicture convolve myPicture -hfilter triangle
    } msg] $msg
} {0 {}}

test picture.282 {myPicture convolve myPicture -hfilter "" } {
    list [catch {
	myPicture convolve myPicture -hfilter ""
    } msg] $msg
} {1 {can't find filter ""}}


test picture.283 {myPicture convolve -vfilter} {
    list [catch {
	myPicture convolve myPicture -vfilter
    } msg] $msg
} {1 {value for "-vfilter" missing}}

test picture.284 {myPicture convolve -vfilter badFilter} {
    list [catch {
	myPicture convolve myPicture -vfilter badFilter
    } msg] $msg
} {1 {can't find filter "badFilter"}}

test picture.285 {myPicture convolve myPicture -vfilter bessel } {
    list [catch {
	myPicture convolve myPicture -vfilter bessel
    } msg] $msg
} {0 {}}

test picture.286 {myPicture convolve myPicture -vfilter box } {
    list [catch {
	myPicture convolve myPicture -vfilter box
    } msg] $msg
} {0 {}}

test picture.287 {myPicture convolve myPicture -vfilter bspline } {
    list [catch {
	myPicture convolve myPicture -vfilter bspline
    } msg] $msg
} {0 {}}

test picture.288 {myPicture convolve myPicture -vfilter catrom } {
    list [catch {
	myPicture convolve myPicture -vfilter catrom
    } msg] $msg
} {0 {}}

test picture.289 {myPicture convolve myPicture -vfilter gaussian } {
    list [catch {
	myPicture convolve myPicture -vfilter gaussian
    } msg] $msg
} {0 {}}

test picture.290 {myPicture convolve myPicture -vfilter lanczos3 } {
    list [catch {
	myPicture convolve myPicture -vfilter lanczos3
    } msg] $msg
} {0 {}}

test picture.291 {myPicture convolve myPicture -vfilter mitchell } {
    list [catch {
	myPicture convolve myPicture -vfilter mitchell
    } msg] $msg
} {0 {}}

test picture.292 {myPicture convolve myPicture -vfilter sinc } {
    list [catch {
	myPicture convolve myPicture -vfilter sinc
    } msg] $msg
} {0 {}}

test picture.293 {myPicture convolve myPicture -vfilter tent } {
    list [catch {
	myPicture convolve myPicture -vfilter tent
    } msg] $msg
} {0 {}}

test picture.294 {myPicture convolve myPicture -vfilter triangle } {
    list [catch {
	myPicture convolve myPicture -vfilter triangle
    } msg] $msg
} {0 {}}

test picture.295 {myPicture convolve myPicture -vfilter "" } {
    list [catch {
	myPicture convolve myPicture -vfilter ""
    } msg] $msg
} {1 {can't find filter ""}}

test picture.296 {myPicture copy} {
    list [catch { myPicture copy } msg] $msg
} {1 {wrong # args: should be "myPicture copy srcName ?switches ...?"}}

test picture.297 {myPicture copy badPicture} {
    list [catch { myPicture copy badPicture} msg] $msg
} {1 {can't find picture "badPicture"}}

test picture.298 {myPicture copy myPicture} {
    list [catch { myPicture copy myPicture} msg] $msg
} {0 {}}

test picture.299 {myPicture copy myPicture -badSwitch } {
    list [catch { myPicture copy myPicture -badSwitch } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -composite 
   -from bbox
   -to bbox}}

test picture.300 {myPicture copy myPicture } {
    list [catch {
	image create picture mySrc -width 20 -height 20
	myPicture copy mySrc
	image delete mySrc
    } msg] $msg
} {0 {}}

test picture.301 {myPicture copy -from} {
    list [catch { myPicture copy myPicture -from } msg] $msg
} {1 {value for "-from" missing}}

test picture.302 {myPicture copy -from badBBox} {
    list [catch { myPicture copy myPicture -from badBBox } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.303 {myPicture copy -from "badX 0"} {
    list [catch { myPicture copy myPicture -from "badX 0" } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.304 {myPicture copy -from "0 badY"} {
    list [catch { myPicture copy myPicture -from "0 badY" } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.305 {myPicture copy -from "0 0"} {
    list [catch { myPicture copy myPicture -from "0 0" } msg] $msg
} {0 {}}

test picture.306 {myPicture copy -from "0 0 badArg"} {
    list [catch { myPicture copy myPicture -from "0 0 badArg" } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.307 {myPicture copy -from "0 0 badX 0"} {
    list [catch { myPicture copy myPicture -from "0 0 badX 0" } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.308 {myPicture copy -from "0 0 0 badY"} {
    list [catch { myPicture copy myPicture -from "0 0 0 badY" } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.309 {myPicture copy -from "0 0 0 0"} {
    list [catch { myPicture copy myPicture -from "0 0 0 0" } msg] $msg
} {0 {}}

test picture.310 {myPicture copy -from "100 100 0 0"} {
    list [catch { myPicture copy myPicture -from "100 100 0 0" } msg] $msg
} {0 {}}

test picture.311 {myPicture copy -from "1000 1000 0 0"} {
    list [catch { myPicture copy myPicture -from "1000 1000 0 0" } msg] $msg
} {0 {}}

test picture.312 {myPicture copy -from "1000 1000 10000 10000"} {
    list [catch {
	myPicture copy myPicture -from "1000 1000 10000 10000"
    } msg] $msg
} {0 {}}

test picture.313 {myPicture copy -to} {
    list [catch { myPicture copy myPicture -to } msg] $msg
} {1 {value for "-to" missing}}

test picture.314 {myPicture copy -to badBBox} {
    list [catch { myPicture copy myPicture -to badBBox } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.315 {myPicture copy -to "badX 0"} {
    list [catch { myPicture copy myPicture -to "badX 0" } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.316 {myPicture copy -to "0 badY"} {
    list [catch { myPicture copy myPicture -to "0 badY" } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.317 {myPicture copy -to "0 0"} {
    list [catch { myPicture copy myPicture -to "0 0" } msg] $msg
} {0 {}}

test picture.318 {myPicture copy -to "0 0 badArg"} {
    list [catch { myPicture copy myPicture -to "0 0 badArg" } msg] $msg
} {1 {wrong # elements in bounding box}}

test picture.319 {myPicture copy -to "0 0 badX 0"} {
    list [catch { myPicture copy myPicture -to "0 0 badX 0" } msg] $msg
} {1 {expected floating-point number but got "badX"}}

test picture.320 {myPicture copy -to "0 0 0 badY"} {
    list [catch { myPicture copy myPicture -to "0 0 0 badY" } msg] $msg
} {1 {expected floating-point number but got "badY"}}

test picture.321 {myPicture copy -to "0 0 0 0"} {
    list [catch { myPicture copy myPicture -to "0 0 0 0" } msg] $msg
} {0 {}}

test picture.322 {myPicture copy -to "100 100 0 0"} {
    list [catch { myPicture copy myPicture -to "100 100 0 0" } msg] $msg
} {0 {}}

test picture.323 {myPicture copy -to "1000 1000 0 0"} {
    list [catch { myPicture copy myPicture -to "1000 1000 0 0" } msg] $msg
} {0 {}}

test picture.324 {myPicture copy -to "1000 1000 10000 10000"} {
    list [catch {
	myPicture copy myPicture -to "1000 1000 10000 10000"
    } msg] $msg
} {0 {}}

test picture.325 {myPicture copy myPicture -composite badValue} {
    list [catch { myPicture copy myPicture -composite badValue } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test picture.326 {myPicture copy myPicture -composite true} {
    list [catch { myPicture copy myPicture -composite true } msg] $msg
} {0 {}}

test picture.327 {myPicture copy myPicture -composite false} {
    list [catch { myPicture copy myPicture -composite false } msg] $msg
} {0 {}}

test picture.328 {myPicture copy myPicture -composite on} {
    list [catch { myPicture copy myPicture -composite on } msg] $msg
} {0 {}}

test picture.329 {myPicture copy myPicture -composite off} {
    list [catch { myPicture copy myPicture -composite off } msg] $msg
} {0 {}}

test picture.330 {myPicture copy myPicture -composite yes} {
    list [catch { myPicture copy myPicture -composite yes } msg] $msg
} {0 {}}

test picture.331 {myPicture copy myPicture -composite no } {
    list [catch { myPicture copy myPicture -composite no } msg] $msg
} {0 {}}

test picture.332 {myPicture copy myPicture -composite 1} {
    list [catch { myPicture copy myPicture -composite 1 } msg] $msg
} {0 {}}

test picture.333 {myPicture copy myPicture -composite 0 } {
    list [catch { myPicture copy myPicture -composite 0 } msg] $msg
} {0 {}}

test picture.333 {myPicture crop } {
    list [catch { myPicture crop } msg] $msg
} {1 {wrong # args: should be "myPicture crop x1 y1 x2 y2"}}

test picture.333 {myPicture crop } {
    list [catch { myPicture crop } msg] $msg
} {1 {wrong # args: should be "myPicture crop x1 y1 x2 y2"}}

test picture.333 {myPicture crop badValue 0 0 0 } {
    list [catch { myPicture crop badValue 0 0 0 } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test picture.333 {myPicture crop 0 badValue 0 0 } {
    list [catch { myPicture crop 0 badValue 0 0 } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test picture.333 {myPicture crop 0 0 badValue 0 } {
    list [catch { myPicture crop 0 0 badValue 0 } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test picture.333 {myPicture crop 0 0 0 badValue } {
    list [catch { myPicture crop 0 0 0 badValue } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test picture.333 {myPicture crop 0 0 0 0 } {
    list [catch { myPicture crop 0 0 0 0 } msg] $msg
} {0 {}}

test picture.333 {myPicture crop 0 0 1000 1000 } {
    list [catch { myPicture crop 0 0 1000 1000 } msg] $msg
} {0 {}}

test picture.333 {myPicture crop 1000 1000 0 0 } {
    list [catch { myPicture crop 1000 1000 0 0 } msg] $msg
} {0 {}}

test picture.210 {myPicture crop 1000 1000 10000 10000} {
    list [catch { myPicture crop 1000 1000 10000 10000 } msg] $msg
} {1 {impossible coordinates for region}}

test picture.210 {myPicture crossfade} {
    list [catch { myPicture crossfade} msg] $msg
} {1 {wrong # args: should be "myPicture crossfade fromName toName ?switches ...?"}}

test picture.210 {myPicture crossfade myPicture } {
    list [catch { myPicture crossfade myPicture} msg] $msg
} {1 {wrong # args: should be "myPicture crossfade fromName toName ?switches ...?"}}

test picture.210 {myPicture crossfade myPicture myPicture } {
    list [catch { myPicture crossfade myPicture myPicture} msg] $msg
} {1 {"from" picture can not be "myPicture"}}

test picture.210 {myPicture crossfade myPicture myPicture } {
    list [catch {
	image create picture mySrc -width 20 -height 20
	myPicture crossfade mySrc myPicture
	image delete mySrc
    } msg] $msg
} {1 {"to" picture can not be "myPicture"}}

test picture.210 {myPicture crossfade myPicture myPicture } {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2
	image delete mySrc1 mySrc2
    } msg] $msg
} {0 {}}

test picture.210 {myPicture crossfade myPicture myPicture -badSwitch} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -badSwitch
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -goto step
   -delay milliseconds
   -scale step
   -steps numSteps
   -variable varName}}

test picture.210 {myPicture crossfade myPicture myPicture -goto} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -goto
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {value for "-goto" missing}}

test picture.210 {myPicture crossfade myPicture myPicture -goto badValue} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -goto badValue
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {expected integer but got "badValue"}}

test picture.210 {myPicture crossfade myPicture myPicture -goto 1.0} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -goto 1.0
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {expected integer but got "1.0"}}

test picture.210 {myPicture crossfade myPicture myPicture -goto -1} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -goto -1
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {bad value "-1": can't be negative}}

test picture.210 {myPicture crossfade myPicture myPicture -goto 0} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -goto 0
	image delete mySrc1 mySrc2
    } msg] $msg
} {0 {}}

test picture.210 {myPicture crossfade myPicture myPicture -goto 1} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -goto 1
	image delete mySrc1 mySrc2
    } msg] $msg
} {0 {}}

test picture.210 {myPicture crossfade myPicture myPicture -goto 10} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -goto 10
	image delete mySrc1 mySrc2
    } msg] $msg
} {0 {}}

test picture.210 {myPicture crossfade myPicture myPicture -goto 1000} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -goto 1000
	image delete mySrc1 mySrc2
    } msg] $msg
} {0 {}}

test picture.210 {myPicture crossfade myPicture myPicture -delay} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -delay
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {value for "-delay" missing}}

test picture.210 {myPicture crossfade myPicture myPicture -delay badValue} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -delay badValue
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {expected integer but got "badValue"}}

test picture.210 {myPicture crossfade myPicture myPicture -delay -1} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -delay -1
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {bad value "-1": can't be negative}}

test picture.210 {myPicture crossfade myPicture myPicture -delay 0} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -delay 0
	image delete mySrc1 mySrc2
    } msg] $msg
} {0 {}}

test picture.210 {myPicture crossfade myPicture myPicture -delay 1.0} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -delay 1.0
	image delete mySrc1 mySrc2
    } msg] $msg
} {1 {expected integer but got "1.0"}}

test picture.210 {myPicture crossfade myPicture myPicture -delay 100000} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -delay 100000
	image delete mySrc1 mySrc2
    } msg] $msg
} {0 {}}

test picture.210 {myPicture crossfade myPicture myPicture -delay 0} {
    list [catch {
	image create picture mySrc1 -width 20 -height 20
	image create picture mySrc2 -width 20 -height 20
	myPicture crossfade mySrc1 mySrc2 -delay 0
	image delete mySrc1 mySrc2
    } msg] $msg
} {0 {}}


exit 0








