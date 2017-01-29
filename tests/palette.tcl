package require Tk
package require BLT

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
  blt::palette colors paletteName
  blt::palette create ?paletteName? ?option value ...?
  blt::palette delete ?paletteName?...
  blt::palette draw paletteName picture
  blt::palette exists paletteName
  blt::palette interpolate paletteName value
  blt::palette names ?pattern ...?
  blt::palette opacities paletteName}}

test palette.2 {palette badOp} {
    list [catch {blt::palette badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  blt::palette colors paletteName
  blt::palette create ?paletteName? ?option value ...?
  blt::palette delete ?paletteName?...
  blt::palette draw paletteName picture
  blt::palette exists paletteName
  blt::palette interpolate paletteName value
  blt::palette names ?pattern ...?
  blt::palette opacities paletteName}}

test palette.3 {palette create} {
    list [catch {blt::palette create} msg] $msg
} {1 {one of -colorfile and -colordata switches are required}}

test palette.4 {palette create myPalette} {
    list [catch {blt::palette create myPalette} msg] $msg
} {1 {one of -colorfile and -colordata switches are required}}

test palette.5 {palette names} {
    list [catch {lsort [blt::palette names]} msg] $msg
} {0 {3gauss.ncmap 3gauss.rgb 3saw.ncmap 3saw.rgb BCGYR BGYOR BkBlAqGrYeOrReViWh200.rgb BlAqGrYeOrRe.rgb BlAqGrYeOrReVi200.rgb BlGrYeOrReVi200.rgb BlRe.rgb BlWhRe.rgb BlueDarkOrange18.rgb BlueDarkRed18.rgb BlueGreen14.rgb BlueRed.rgb BlueRedGray.rgb BlueWhiteOrangeRed.rgb BlueYellowRed.rgb BrownBlue12.rgb CBR_coldhot.rgb CBR_drywet.rgb CBR_set3.rgb CBR_wet.rgb Cat12.rgb GHRSST_anomaly.rgb GMT_cool.rgb GMT_copper.rgb GMT_drywet.rgb GMT_gebco.rgb GMT_globe.rgb GMT_gray.rgb GMT_haxby.rgb GMT_hot.rgb GMT_jet.rgb GMT_nighttime.rgb GMT_no_green.rgb GMT_ocean.rgb GMT_paired.rgb GMT_panoply.rgb GMT_polar.rgb GMT_red2green.rgb GMT_relief.rgb GMT_relief_oceanonly.rgb GMT_seis.rgb GMT_split.rgb GMT_topo.rgb GMT_wysiwyg.rgb GMT_wysiwygcont.rgb GrayWhiteGray.rgb GreenMagenta16.rgb GreenYellow.rgb MPL_Accent.rgb MPL_Blues.rgb MPL_BrBG.rgb MPL_BuGn.rgb MPL_BuPu.rgb MPL_Dark2.rgb MPL_GnBu.rgb MPL_Greens.rgb MPL_Greys.rgb MPL_OrRd.rgb MPL_Oranges.rgb MPL_PRGn.rgb MPL_Paired.rgb MPL_Pastel1.rgb MPL_Pastel2.rgb MPL_PiYG.rgb MPL_PuBu.rgb MPL_PuBuGn.rgb MPL_PuOr.rgb MPL_PuRd.rgb MPL_Purples.rgb MPL_RdBu.rgb MPL_RdGy.rgb MPL_RdPu.rgb MPL_RdYlBu.rgb MPL_RdYlGn.rgb MPL_Reds.rgb MPL_Set1.rgb MPL_Set2.rgb MPL_Set3.rgb MPL_Spectral.rgb MPL_StepSeq.rgb MPL_YlGn.rgb MPL_YlGnBu.rgb MPL_YlOrBr.rgb MPL_YlOrRd.rgb MPL_afmhot.rgb MPL_autumn.rgb MPL_bone.rgb MPL_brg.rgb MPL_bwr.rgb MPL_cool.rgb MPL_coolwarm.rgb MPL_copper.rgb MPL_cubehelix.rgb MPL_flag.rgb MPL_gist_earth.rgb MPL_gist_gray.rgb MPL_gist_heat.rgb MPL_gist_ncar.rgb MPL_gist_rainbow.rgb MPL_gist_stern.rgb MPL_gist_yarg.rgb MPL_gnuplot.rgb MPL_gnuplot2.rgb MPL_hot.rgb MPL_hsv.rgb MPL_jet.rgb MPL_ocean.rgb MPL_pink.rgb MPL_prism.rgb MPL_rainbow.rgb MPL_s3pcpn.rgb MPL_s3pcpn_l.rgb MPL_seismic.rgb MPL_spring.rgb MPL_sstanom.rgb MPL_summer.rgb MPL_terrain.rgb MPL_winter.rgb NCV_banded.rgb NCV_blu_red.rgb NCV_blue_red.rgb NCV_bright.rgb NCV_gebco.rgb NCV_jaisnd.rgb NCV_jet.rgb NCV_manga.rgb NCV_rainbow2.rgb NCV_roullet.rgb OceanLakeLandSnow.rgb ROYGB RYGCB SVG_Gallet13.rgb SVG_Lindaa06.rgb SVG_Lindaa07.rgb SVG_bhw3_22.rgb SVG_es_landscape_79.rgb SVG_feb_sunrise.rgb SVG_foggy_sunrise.rgb SVG_fs2006.rgb StepSeq25.rgb ViBlGrWhYeOrRe.rgb WhBlGrYeRe.rgb WhBlReWh.rgb WhViBlGrYeOrRe.rgb WhViBlGrYeOrReWh.rgb WhiteBlue.rgb WhiteBlueGreenYellowRed.rgb WhiteGreen.rgb WhiteYellowOrangeRed.rgb amwg.rgb amwg256.rgb amwg_blueyellowred.rgb blue-to-brown.rgb blue-to-gray.rgb blue-to-green.rgb blue-to-grey.rgb blue.rgb brown-to-blue.rgb cb_9step.rgb cb_rainbow.rgb cb_rainbow_inv.rgb circular_0.rgb circular_1.rgb circular_2.rgb cmp_b2r.rgb cmp_flux.rgb cmp_haxby.rgb cosam.rgb cosam12.rgb cyclic.rgb default.rgb detail.ncmap detail.rgb example.rgb extrema.ncmap extrema.rgb grads_default.rgb grads_rainbow.rgb green-to-magenta.rgb greyscale gscyclic.rgb gsdtol.rgb gsltod.rgb gui_default.rgb helix.ncmap helix.rgb helix1.ncmap helix1.rgb hlu_default.rgb hotcold_18lev.rgb hotcolr_19lev.rgb hotres.ncmap hotres.rgb lithology.rgb matlab_hot.rgb matlab_hsv.rgb matlab_jet.rgb matlab_lines.rgb mch_default.rgb nanohub ncl_default.rgb ncview_default.ncmap ncview_default.rgb nice_gfdl.rgb nrl_sirkes.rgb nrl_sirkes_nowhite.rgb orange-to-blue.rgb perc2_9lev.rgb percent_11lev.rgb posneg_1.rgb posneg_2.rgb prcp_1.rgb prcp_2.rgb prcp_3.rgb precip2_15lev.rgb precip2_17lev.rgb precip3_16lev.rgb precip4_11lev.rgb precip4_diff_19lev.rgb precip_11lev.rgb precip_diff_12lev.rgb precip_diff_1lev.rgb psgcap.rgb radar.rgb radar_1.rgb rainbow rainbow+gray.rgb rainbow+white+gray.rgb rainbow+white.rgb rainbow.rgb rh_19lev.rgb seaice_1.rgb seaice_2.rgb so4_21.rgb so4_23.rgb spectral-scheme.rgb spectral.rgb spread_15lev.rgb sunshine_9lev.rgb sunshine_diff_12lev.rgb t2m_29lev.rgb tbrAvg1.rgb tbrStd1.rgb tbrVar1.rgb tbr_240-300.rgb tbr_stdev_0-30.rgb tbr_var_0-500.rgb temp1.rgb temp_19lev.rgb temp_diff_18lev.rgb temp_diff_1lev.rgb testcmap.rgb thelix.rgb topo_15lev.rgb uniform.rgb wgne15.rgb wh-bl-gr-ye-re.rgb wind_17lev.rgb wxpEnIR.rgb}}

test palette.6 {palette create -colordata {}} {
    list [catch {blt::palette create -colordata {}} msg] $msg
} {0 palette1}

test palette.7 {palette create -help} {
    list [catch {blt::palette create -help} msg] $msg
} {1 {following switches are available:
   -cdata dataString
   -cfile fileName
   -colordata dataString
   -colorfile fileName
   -colorformat formatName
   -colorspacing spacingName
   -fade percentFade
   -odata dataString
   -ofile fileName
   -opacityfile fileName
   -opacitydata dataString
   -opacityspacing spacingName}}

test palette.8 {palette delete} {
    list [catch {eval blt::palette delete} msg] $msg
} {0 {}}

test palette.9 {palette delete badName} {
    list [catch {eval blt::palette delete badName} msg] $msg
} {1 {can't find a palette "badName"}}

test palette.10 {palette delete all palettes} {
    list [catch {eval blt::palette delete [blt::palette names]} msg] $msg
} {0 {}}

test palette.11 {palette names} {
    list [catch {lsort [blt::palette names]} msg] $msg
} {0 {}}

test palette.12 {palette exists} {
    list [catch {eval blt::palette exists myPalette} msg] $msg
} {0 0}

test palette.13 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {black white} \
	    -colorformat name
    } msg] $msg
} {0 myPalette}

test palette.14 {palette exists} {
    list [catch {eval blt::palette exists myPalette} msg] $msg
} {0 1}

test palette.15 {palette interpolate myPalette (missing value)} {
    list [catch {blt::palette interpolate myPalette} msg] $msg
} {1 {wrong # args: should be "blt::palette interpolate paletteName value"}}

test palette.16 {palette interpolate myPalette badValue} {
    list [catch {blt::palette interpolate myPalette badValue} msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test palette.17 {palette interpolate myPalette 0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 #000000}

test palette.18 {palette interpolate myPalette 1} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 #ffffff}

test palette.19 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 #7f7f7f}

test palette.20 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.21 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.22 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 #7f7f7f}

test palette.23 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 #000000}

test palette.24 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 #ffffff}

test palette.25 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 #ffffff}

test palette.26 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.27 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {0 0 0 1 1 1} \
	    -colorformat rgb
    } msg] $msg
} {0 myPalette}


test palette.28 {palette interpolate myPalette 0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 {0 0 0}}

test palette.29 {palette interpolate myPalette 1} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 {255 255 255}}

test palette.30 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 {127 127 127}}

test palette.31 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.32 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.33 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 {127 127 127}}

test palette.34 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 {0 0 0}}

test palette.35 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 {255 255 255}}

test palette.36 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 {255 255 255}}

test palette.37 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.38 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {0 0 0 0 0 100} \
	    -colorformat hsv
    } msg] $msg
} {0 myPalette}

test palette.39 {palette interpolate myPalette 0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 {0 0 0}}

test palette.40 {palette interpolate myPalette 1} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 {255 255 255}}

test palette.41 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 {127 127 127}}

test palette.42 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.43 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.44 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 {127 127 127}}

test palette.45 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 {0 0 0}}

test palette.46 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 {255 255 255}}

test palette.47 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 {255 255 255}}

test palette.48 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.49 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {0 0 0 0 1 0 0 100} \
	    -colorformat hsv -colorspacing irregular 
    } msg] $msg
} {0 myPalette}

test palette.50 {palette interpolate myPalette 0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 {0 0 0}}

test palette.51 {palette interpolate myPalette 1} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 {255 255 255}}

test palette.52 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 {127 127 127}}

test palette.53 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.54 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.55 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 {127 127 127}}

test palette.56 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 {0 0 0}}

test palette.57 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 {255 255 255}}

test palette.58 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 {255 255 255}}

test palette.59 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.60 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {0 0 0 0 1 1 1 1} \
	    -colorformat rgb -colorspacing irregular
    } msg] $msg
} {0 myPalette}


test palette.61 {palette interpolate myPalette 0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 {0 0 0}}

test palette.62 {palette interpolate myPalette 1} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 {255 255 255}}

test palette.63 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 {127 127 127}}

test palette.64 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.65 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.66 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 {127 127 127}}

test palette.67 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 {0 0 0}}

test palette.68 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 {255 255 255}}

test palette.69 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 {255 255 255}}

test palette.70 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.71 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {0 black 1 white} \
	    -colorformat name -colorspacing irregular
    } msg] $msg
} {0 myPalette}

test palette.72 {palette interpolate myPalette 0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 #000000}

test palette.73 {palette interpolate myPalette 1} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 #ffffff}

test palette.74 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 #7f7f7f}

test palette.75 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.76 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.77 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 #7f7f7f}

test palette.78 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 #000000}

test palette.79 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 #ffffff}

test palette.80 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 #ffffff}

test palette.81 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.82 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {black white} \
	    -colorformat name -opacitydata { 1 1 } 
    } msg] $msg
} {0 myPalette}

test palette.83 {palette interpolate myPalette 0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 0xff000000}

test palette.84 {palette interpolate myPalette 1} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 0xffffffff}

test palette.85 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 0xff7f7f7f}

test palette.86 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.87 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.88 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 0xff7f7f7f}

test palette.89 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 0xff000000}

test palette.90 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 0xffffffff}

test palette.91 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 0xffffffff}

test palette.92 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.93 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {black white} \
	    -colorformat name -opacitydata { 0 1 }
    } msg] $msg
} {0 myPalette}

test palette.94 {palette colors myPalette} {
    list [catch {blt::palette colors myPalette} msg] $msg
} {0 {0.0 1.0 #000000 #ffffff}}

test palette.95 {palette opacities myPalette} {
    list [catch {blt::palette opacities myPalette} msg] $msg
} {0 {0.0 1.0 0.0 1.0}}

test palette.96 {palette interpolate myPalette 0.0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 0x00000000}

test palette.97 {palette interpolate myPalette 0.0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 0x00000000}

test palette.98 {palette interpolate myPalette 1.0} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 0xffffffff}

test palette.99 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 0x7f7f7f7f}

test palette.100 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.101 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.102 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 0x7f7f7f7f}

test palette.103 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 0x00000000}

test palette.104 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 0xffffffff}

test palette.105 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 0xffffffff}

test palette.106 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.107 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {black white} \
	    -colorformat name -opacitydata { 0 0 1 1 } \
	    -opacityspacing irregular 
    } msg] $msg
} {0 myPalette}

test palette.108 {palette colors myPalette} {
    list [catch {blt::palette colors myPalette} msg] $msg
} {0 {0.0 1.0 #000000 #ffffff}}

test palette.109 {palette opacities myPalette} {
    list [catch {blt::palette opacities myPalette} msg] $msg
} {0 {0.0 1.0 0.0 1.0}}

test palette.110 {palette interpolate myPalette 0.0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 0x00000000}

test palette.111 {palette interpolate myPalette 0.0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 0x00000000}

test palette.112 {palette interpolate myPalette 1.0} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 0xffffffff}

test palette.113 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 0x7f7f7f7f}

test palette.114 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.115 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.116 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 0x7f7f7f7f}

test palette.117 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 0x00000000}

test palette.118 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 0xffffffff}

test palette.119 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 0xffffffff}

test palette.120 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

test palette.121 {palette create myPalette -colordata -colorformat} {
    list [catch {
	blt::palette create myPalette -colordata {black white} \
	    -colorformat name -opacitydata { 0 0 1 1 } \
	    -opacityspacing interval 
    } msg] $msg
} {0 myPalette}

test palette.122 {palette colors myPalette} {
    list [catch {blt::palette colors myPalette} msg] $msg
} {0 {0.0 1.0 #000000 #ffffff}}

test palette.123 {palette opacities myPalette} {
    list [catch {blt::palette opacities myPalette} msg] $msg
} {0 {0.0 1.0 0.0 1.0}}

test palette.124 {palette interpolate myPalette 0.0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 0x00000000}

test palette.125 {palette interpolate myPalette 0.0} {
    list [catch {blt::palette interpolate myPalette 0.0} msg] $msg
} {0 0x00000000}

test palette.126 {palette interpolate myPalette 1.0} {
    list [catch {blt::palette interpolate myPalette 1.0} msg] $msg
} {0 0xffffffff}

test palette.127 {palette interpolate myPalette 0.5} {
    list [catch {blt::palette interpolate myPalette 0.5} msg] $msg
} {0 0x7f7f7f7f}

test palette.128 {palette interpolate myPalette 1e+500} {
    list [catch {blt::palette interpolate myPalette 1e+500} msg] $msg
} {1 {value "1e+500" can't be represented: Numerical result out of range}}

test palette.129 {palette interpolate myPalette 1e-500} {
    list [catch {blt::palette interpolate myPalette 1e-500} msg] $msg
} {1 {value "1e-500" can't be represented: Numerical result out of range}}

test palette.130 {palette interpolate myPalette 50.0%} {
    list [catch {blt::palette interpolate myPalette 50.0%} msg] $msg
} {0 0x7f7f7f7f}

test palette.131 {palette interpolate myPalette 0.0%} {
    list [catch { blt::palette interpolate myPalette 0.0% } msg] $msg
} {0 0x00000000}

test palette.132 {palette interpolate myPalette 100.0%} {
    list [catch { blt::palette interpolate myPalette 100.0% } msg] $msg
} {0 0xffffffff}

test palette.133 {palette interpolate myPalette 100%} {
    list [catch { blt::palette interpolate myPalette 100% } msg] $msg
} {0 0xffffffff}

test palette.134 {palette delete myPalette} {
    list [catch {eval blt::palette delete myPalette} msg] $msg
} {0 {}}

exit 0
