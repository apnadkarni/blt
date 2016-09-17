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
  myPicture crop bbox
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
  myPicture crop bbox
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
} {0 {{-data {} {} {} {}} {-dither {} {} 0 0} {-file {} {} {} {}} {-filter {} {} {} {}} {-gamma {} {} 1.0 1.0} {-height {} {} 0 0} {-image {} {} {} {}} {-maxpect {} {} 0 0} {-rotate {} {} 0.0 0.0} {-sharpen {} {} no 0} {-width {} {} 0 0} {-window {} {} {} {}}}}

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

test picture.67 {myPicture configure -gamma } {
    list [catch {
	myPicture configure -gamma
    } msg] $msg
} {0 {-gamma {} {} 1.0 1.0}}

test picture.68 {myPicture configure -gamma badValue} {
    list [catch {
	myPicture configure -gamma badValue
    } msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test picture.69 {myPicture configure -gamma neg Value} {
    list [catch {
	myPicture configure -gamma -1.0
    } msg] $msg
} {1 {gamma value can't be negative}}

test picture.70 {myPicture configure -gamma 0.0} {
    list [catch {
	myPicture configure -gamma 0.0
    } msg] $msg
} {1 {gamma value can't be zero}}

test picture.71 {myPicture configure -gamma 20.0} {
    list [catch {
	myPicture configure -gamma 26.0
	myPicture cget -gamma
    } msg] $msg
} {0 20.0}

test picture.72 {myPicture configure -gamma 2.0} {
    list [catch {
	myPicture configure -gamma 2.0
	myPicture cget -gamma
    } msg] $msg
} {0 2.0}

test picture.73 {myPicture configure -gamma 1.0} {
    list [catch {
	myPicture configure -gamma 1.0
	myPicture cget -gamma
    } msg] $msg
} {0 1.0}

test picture.74 {myPicture configure -height} {
    list [catch {
	myPicture configure -height
    } msg] $msg
} {0 {-height {} {} 0 0}}

test picture.75 {myPicture configure -height badValue} {
    list [catch {
	myPicture configure -height badValue
    } msg] $msg
} {1 {bad screen distance "badValue"}}

test picture.76 {myPicture configure -height 0} {
    list [catch {
	myPicture configure -height 1
    } msg] $msg
} {0 {}}

test picture.77 {myPicture configure -height -1} {
    list [catch {
	myPicture configure -height -1
    } msg] $msg
} {1 {bad distance "-1": can't be negative}}

test picture.78 {myPicture configure -height 1i} {
    list [catch {
	myPicture configure -height 1i
    } msg] $msg
} {0 {}}

test picture.79 {myPicture configure -height 500.01} {
    list [catch {
	myPicture configure -height 500.01
    } msg] $msg
} {0 {}}

test picture.80 {myPicture configure -image} {
    list [catch {
	myPicture configure -image
    } msg] $msg
} {0 {-image {} {} {} {}}}

test picture.81 {myPicture configure -image badImage} {
    list [catch {
	myPicture configure -image badImage
    } msg] $msg
} {1 {image "badImage" doesn't exist}}

test picture.82 {myPicture configure -image tkPhoto} {
    list [catch {
	set img [image create photo -file ./files/blt98.gif]
	myPicture configure -image $img
	image delete $img
    } msg] $msg
} {0 {}}

test picture.83 {myPicture configure -image tkBitmap} {
    list [catch {
	set img [image create bitmap \
		     -file ./files/hobbes.xbm \
		     -maskfile ./files/hobbes_mask.xbm]
	myPicture configure -image $img
	image delete $img
    } msg] $msg
} {0 {}}

test picture.84 {myPicture configure -image bltPicture} {
    list [catch {
	set img [image create picture -file ./files/blt98.gif]
	myPicture configure -image $img
	image delete $img
    } msg] $msg
} {0 {}}

test picture.85 {myPicture configure -image ""} {
    list [catch {
	myPicture configure -image ""
    } msg] $msg
} {1 {image "" doesn't exist}}

test picture.86 {myPicture configure -maxpect} {
    list [catch {
	myPicture configure -maxpect
    } msg] $msg
} {0 {-maxpect {} {} 0 0}}

test picture.87 {myPicture configure -maxpect badValue} {
    list [catch {
	myPicture configure -maxpect badValue
    } msg] $msg
} {1 {expected boolean value but got "badValue"}}

test picture.88 {myPicture configure -maxpect true} {
    list [catch {
	myPicture configure -maxpect true
    } msg] $msg
} {0 {}}

test picture.89 {myPicture configure -maxpect false} {
    list [catch {
	myPicture configure -maxpect false
    } msg] $msg
} {0 {}}

test picture.90 {myPicture configure -maxpect on} {
    list [catch {
	myPicture configure -maxpect on
    } msg] $msg
} {0 {}}

test picture.91 {myPicture configure -maxpect off} {
    list [catch {
	myPicture configure -maxpect off
    } msg] $msg
} {0 {}}

test picture.92 {myPicture configure -maxpect yes} {
    list [catch {
	myPicture configure -maxpect yes
    } msg] $msg
} {0 {}}

test picture.93 {myPicture configure -maxpect no } {
    list [catch {
	myPicture configure -maxpect no
    } msg] $msg
} {0 {}}

test picture.94 {myPicture configure -maxpect 1} {
    list [catch {
	myPicture configure -maxpect 1
    } msg] $msg
} {0 {}}

test picture.95 {myPicture configure -maxpect 0 } {
    list [catch {
	myPicture configure -maxpect 0
    } msg] $msg
} {0 {}}

exit 0

