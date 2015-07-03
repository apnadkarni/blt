
package require BLT

blt::tk::label .layout_l \
    -text "Layout Mode"
set m .layout.menu
blt::comboentry .layout \
    -editable no \
    -textvariable layoutMode \
    -menu $m \
    -width 200
blt::combomenu $m \
    -textvariable layoutMode 
$m add -text "columns" \
    -command [list .ss.listview configure -layoutmode columns]
$m add -text "icons" \
    -command [list .ss.listview configure -layoutmode icons]
$m add -text "row" \
    -command [list .ss.listview configure -layoutmode row]
$m add -text "rows" \
    -command [list .ss.listview configure -layoutmode rows]

blt::scrollset .ss \
    -xscrollbar .ss.xs \
    -yscrollbar .ss.ys \
    -window .ss.listview

blt::listview .ss.listview \
    -width 5i \
    -height 2i  \
    -layoutmode columns \
    -selectmode multiple  

.ss.listview sort configure \
    -decreasing 0 \
    -auto 1 \
    -by type
    
image create picture smallIcon -data {
    AAEBAACJACAAAAAAEAAQAAgIcnd25uTp5/8AAAAA4+fl/+/y8P9mamj83ePg//P1
    9P+ssq+y8vPy/7Cxsf9uc3Dl9/j4/+Hm5P+psK1EZGln/Nrg3f+vtrJJ3uPh/9jf
    3P/j6OX/gJ+ACJmenLHm6ef/q7Kv/+fr6f+wt7Xi/f39/+vu7f/V3Nn/kpiW/+Dl
    4v9rcG789/n4/5CWlPnk6Ob/5ero/3F2dP+Ok5H/8PPx/9bY1/5obWv83uTh//T2
    9f+lrKn/dXp4/OLn5f/t8O7/foSB/Nfe2v9kaGb88fPy/4SKh/+kq6l5oaek/9vh
    3v/f5OL/ur+99v3+/f/M09D/1NvX/660sf/r7+3/7vDv/4CAgAifpqN15Onm/4yR
    j/zW2Nj/v8TC/+js6v+ss7D/YGBgCP7+/v+wtrP2Z2xp/Ozv7v/W3dr/7/Dw/+Hm
    4/+nrKn/5enn/+ns6/+CiIX8/P39/2VoZVFobGr87O3t//X39v96gHz84+jm/+Lm
    5P/Y2tn/ytDM/6uyr/b5+vr/mZ2dPAAAAAFggIAI7vHv/+Xn5v/c4t//Zmtp/PL0
    8/9pbWvn4OXj/2NoZvxpb2xOl52b/9nf3P+8wr//p6yq9XB1c/yssq//6Ovp/620
    sf/p7ev/bHBu6nZ7ef//////f4WC/+zu7f/t8O//nKKf//Dx8f/b4d//qqqqA/3+
    /v/m6uj/ZWpn/HuAff/q7ez/9vj3/4mPjP+HjIr8rLOw5N/k4f8CawspVktmBYEP
    MmpoVQICAgBXd3d3d3d3d3d3fHVhAgIlSWcHK1iEDCEhDHcgSAICdkkEBCczZ2cH
    B2d3cGICAoJJPhxML3o/Y2NOdy1AAgJ4f3IZGUZGUnR0g0lZQAICNDoXWhRaIyNC
    QlFJMEACAoU6AVtPHzg4ODhpSVNAAgImOlENaSplEBAQN0mGQAICHjpRDR8SfRMd
    HU1JQxUCAmx/gC5pBhAxPDtdXCJ+AgJ7SXSAAx9lbW5QCigWAgICNkl6dIAuiDdF
    d1RvYAICAixJCXp0JA1pRBs5QQICAgKHeUlJSUlJX2RKNQICAgICERo9c0dxGF4I
    DgICAgIC7wEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAYAHADfBwkAEwAt
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAANgMAAAAAAABUUlVFVklTSU9OLVhGSUxFLgA=
}

image create picture bigIcon -data {
    AAACAAAAAAAAAAAAMAAwACAIAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAgAA
    AAIAAAACAAAAAwAAAAMAAAADAAAABAAAAAQAAAAEAAAABAAAAAQAAAAEAAAABAAA
    AAQAAAAEAAAABAAAAAQAAAAEAAAABAAAAAQAAAAEAAAABAAAAAQAAAAEAAAABAAA
    AAQAAAAEAAAABAAAAAQAAAAEAAAAAwAAAAMAAAADAAAAAwAAAAIAAAACAAAAAQAA
    AAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAIAAAAEAAAABgAA
    AAcAAAAIAAAACQAAAAoAAAALAAAADAAAAAwAAAAMAAAADAAAAAwAAAAMAAAADAAA
    AAwAAAAMAAAADAAAAAwAAAAMAAAADAAAAAwAAAAMAAAADAAAAAwAAAAMAAAADAAA
    AAwAAAAMAAAADAAAAAwAAAAMAAAACwAAAAsAAAAKAAAACQAAAAgAAAAHAAAABQAA
    AAMAAAACAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAwAAAAYAAAAKAAAAEAAA
    ABQAAAAYAAAAGgAAAB0AAAAeAAAAIAAAACAAAAAhAAAAIQAAACEAAAAhAAAAIQAA
    ACEAAAAhAAAAIQAAACEAAAAhAAAAIQAAACEAAAAhAAAAIQAAACEAAAAhAAAAIQAA
    ACEAAAAhAAAAIQAAACEAAAAgAAAAHwAAAB4AAAAbAAAAGQAAABcAAAATAAAADgAA
    AAkAAAAEAAAAAgAAAAEAAAAAAAAAAAAAAAAAAAACAAAABQAAAAsAAAAVAAAAJAAA
    ADUAAABBAAAASQAAAE0AAABQAAAAVAAAAFUAAABVAAAAVgAAAFYAAABWAAAAVgAA
    AFYAAABWAAAAVgAAAFYAAABWAAAAVgAAAFYAAABWAAAAVgAAAFYAAABWAAAAVgAA
    AFYAAABWAAAAVgAAAFUAAABVAAAAUwAAAFAAAABLAAAARgAAAD4AAAAwAAAAHQAA
    ABEAAAAIAAAAAwAAAAEAAAAAAAAAAAAAAAAAAAACAAAABwAAABAAAAAfKCwoUmtw
    bsh8goD8e4B+/3uAfv97gH7/eoB9/3p/ff96f33/en98/3l/fP95fnz/eX58/3l+
    e/94fXv/eH17/3h9e/94fXr/d3x6/3d8ev93fHr/d3x5/3Z7ef92e3n/dnt4/3V6
    eP91enj/dXp4/3V6d/90eXf/dHl3/3R5dv90eXb/dHl3/mZqadQnJyduAAAAMAAA
    ABgAAAALAAAABAAAAAEAAAAAAAAAAAAAAAAAAAACAAAABgAAAA8AAAAedHh1ss/R
    0P7+/v7/////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////9fZ2P9scXDNAAAALgAA
    ABcAAAAKAAAABAAAAAEAAAAAAAAAAAAAAAAAAAABAAAABAAAAAkAAAASgoeF9f39
    /f/w8/L/7/Lw//Dy8f/w8/L/8fPy//H08//y9PP/8/X0//P19P/09vX/9Pb1//X2
    9v/19/b/9vf2//b39//2+Pf/9/j3//f4+P/3+Pj/9/n4//f5+P/4+fj/9/n4//f5
    +P/3+fj/9/j4//f4+P/3+Pf/9vj3//b39//19/b/9vj3//7//v97gH37ERERHgAA
    AA4AAAAHAAAAAwAAAAEAAAAAAAAAAAAAAAAAAAABAAAAAgAAAAQAAAAIgoeE+/7/
    ///w8vD/8PLx//Dz8v/x8/L/8vTz//L08//z9fT/8/X0//T29f/19vX/9ff2//b3
    9v/2+Pf/9/j3//f4+P/4+fj/+Pn5//j5+f/5+vn/+fr5//n6+f/5+vn/+fr5//n6
    +f/5+vn/+Pr5//j5+f/4+fj/9/n4//f4+P/2+Pf/9vf3//////98gYD8NjY2EwAA
    AAYAAAADAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAEAAAADhIiG+/7/
    ///w8vH/8PPx//Hz8v/y9PP/8vTz//P19P/z9fT/9Pb1//X29v/19/b/9vf3//f4
    9//3+Pj/+Pn4//j5+f/5+vn/+fr6//r6+v/6+/r/+vv7//r7+//6+/v/+vv7//r7
    +//6+/r/+vv6//n6+v/5+vn/+Pr5//j5+f/3+fj/9/j4//////99hID8XV1dCwAA
    AAIAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABhIqH+/7/
    ///w8vH/8fPy//H08v/y9PP/8/X0//P19P/09vX/9fb1//X39v/29/f/9/j3//f5
    +P/4+fj/+Pr5//n6+v/6+/r/+vv7//v7+//7/Pv/+/z8//z8/P/8/Pz//Pz8//z8
    /P/7/Pz/+/z7//r7+//6+/r/+fr6//n6+f/4+fn/+Pn4//////+AhYP8cXFxCQAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAhouJ+/7/
    ///w8/H/8fPy//L08//y9PP/8/X0//T19f/09vX/9ff2//b39v/2+Pf/9/j4//j5
    +P/4+fn/+fr5//r7+v/6+/v/+/z7//z8/P/8/fz//f39//39/f/9/v3//f39//39
    /f/8/f3//Pz8//v8/P/7+/v/+vv6//n6+v/5+vn/+Pn5//////+Bh4T8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAh42K+///
    ///w8/H/8fPy//L08//y9PP/8/X0//T29f/09vX/9ff2//b39//3+Pf/9/j4//j5
    +P/5+vn/+fr6//r7+v/7+/v/+/z8//z8/P/9/f3//f79//7+/v///////v7+//7+
    /v/9/f3//P39//z8/P/7/Pv/+vv7//r7+v/5+vn/+Pn5//////+DiIb8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAiY6M+///
    ///w8/H/8fPy//L08//y9PP/8/X0//T29f/09vX/9ff2//b39//2+Pf/9/j4//j5
    +P/4+vn/+fr6//r7+v/6+/v/+/z8//z8/P/8/f3//f39//7+/v/+/v7//v7+//3+
    /v/9/f3//P38//z8/P/7/Pv/+vv7//r6+v/5+vn/+Pn5//////+EiYf8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAipCN+/7/
    ///w8/H/8fPy//H08//y9PP/8/X0//P19P/09vX/9fb2//X39v/2+Pf/9/j3//f5
    +P/4+fn/+fr5//n6+v/6+/r/+/v7//v8+//8/Pz//Pz8//z9/f/8/f3//P39//z9
    /P/8/Pz/+/z8//v8+//6+/v/+vv6//n6+v/4+vn/+Pn4//////+Gi4n8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAi5GP+/7/
    ///w8vH/8PPy//Hz8v/y9PP/8vTz//P19P/09vX/9Pb1//X39v/29/b/9vj3//f4
    +P/3+fj/+Pn5//n6+f/5+vr/+vv6//r7+v/6+/v/+/v7//v8+//7/Pv/+/z7//v8
    +//7+/v/+vv7//r7+v/5+vr/+fr5//j5+f/4+fj/9/n4//////+HjIr8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAjZOQ+/7/
    ///w8vH/8PLx//Hz8v/x9PL/8vTz//P19P/z9fT/9Pb1//T29f/19/b/9vf2//b4
    9//3+Pf/9/j4//j5+P/4+fn/+Pr5//n6+f/5+vr/+fr6//r6+v/6+/r/+vv6//r6
    +v/5+vr/+fr6//n6+f/4+fn/+Pn5//f5+P/3+Pj/9vj3//////+Jjov8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAjpSR+/7/
    ///w8fD/7/Lx//Dz8f/x8/L/8fTy//L08//y9fP/8/X0//T19f/09vX/9fb1//X3
    9v/29/b/9vj3//f49//3+Pj/9/n4//j5+P/4+fj/+Pn5//j5+f/4+fn/+Pn5//j5
    +f/4+fn/+Pn4//f5+P/3+Pj/9/j3//b49//29/f/9ff2//////+Kj438gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAkJaT+/7/
    /v/v8fD/7/Hw/+/y8f/w8vH/8PPy//Hz8v/y9PP/8vTz//P19P/z9fT/9Pb1//T2
    9f/19vX/9ff2//X39v/29/b/9vf3//b49//2+Pf/9/j3//f49//3+Pf/9/j3//f4
    9//3+Pf/9vj3//b49//29/f/9vf2//X39v/19/b/9Pb1//////+LkY78gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAkZeU+/7/
    /v/u8PD/7vHv/+/x8P/v8vD/8PLx//Dz8f/x8/L/8fPy//L08//y9PP/8/X0//P1
    9P/z9fT/9Pb1//T29f/09vX/9fb2//X39v/19/b/9ff2//X39v/19/b/9ff2//X3
    9v/19/b/9ff2//X39v/19vX/9Pb1//T29f/09fX/8/X0//////+NkpD8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAk5iW+/7+
    /v/u8O//7fDv/+7x7//u8fD/7/Hw/+/y8f/w8vH/8PPx//Hz8v/x8/L/8fTz//L0
    8//y9PP/8/X0//P19P/z9fT/8/X0//T19P/09vX/9Pb1//T29f/09vX/9Pb1//T2
    9f/09vX/9PX1//P19P/z9fT/8/X0//P19P/y9PP/8vTz//////+OlJH8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAlJqX+/7+
    /v/t8O7/7O/u/+3w7v/t8O//7vHv/+7x8P/v8fD/7/Lw/+/y8f/w8vH/8PPx//Hz
    8v/x8/L/8fTy//H08//y9PP/8vTz//L08//y9PP/8vTz//L18//y9fT/8vX0//L1
    8//y9PP/8vTz//L08//y9PP/8vTz//H08//x8/L/8fPy//////+QlZP8gICACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAlZuZ+/7+
    /v/s7+3/6+7t/+zv7f/s7+7/7fDu/+3w7//t8O//7vHv/+7x8P/v8fD/7/Lw/+/y
    8f/w8vH/8PLx//Dz8f/w8/L/8fPy//Hz8v/x8/L/8fPy//Hz8v/x8/L/8fPy//Hz
    8v/x8/L/8fPy//Hz8v/x8/L/8PPy//Dz8f/w8vH/8PLx//////+RlpT8gJ+ACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAl52a+/7+
    /v/s7+3/6+/t/+zv7f/s7+7/7fDu/+3w7//t8O//7vDv/+3w7//t8O//7vHv/+7x
    7//u8fD/7/Hw/+/y8P/v8vD/7/Lx/+/y8f/v8vH/8PLx//Dy8f/w8vH/8PLx//Dy
    8f/w8vH/7/Lx/+/y8f/v8vD/7/Lw/+/x8P/u8fD/7vHw//////+Sl5X8gJ+ACAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAmJ6c+/7+
    /v/s7+7/6+/t/+zv7f/s7u7/7O/u/+3v7f/s7+7/7PDu/+3w7v/t8O//7fHv/+7x
    7//u8e//7vHw/+7w8P/u8e//7vHv/+7x7//u8e//7vHw/+7x8P/u8fD/7vHw/+7x
    8P/u8fD/7vHv/+7x7//u8e//7vDv/+3w7//t8O//7fDu//////+UmJb8gJ+fCAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAmqGd+/7+
    /v/s7+3/6+7t/+vu7f/s7+3/7O/t/+zv7f/s7+7/7PDu/+3w7v/t8O//7fDv/+3w
    7//u8O7/7fDu/+3w7//t8O//7fHv/+7x7//u8e//7vDv/+3w7v/t8O7/7fDu/+3w
    7v/t8O7/7fDu/+zw7v/s7+7/7O/u/+zv7v/s7+3/7O/t//7///+VmZf8gJ+fCAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAm6Kg+/7+
    /v/r7+3/6+7t/+vu7f/s7+3/7O/t/+zv7f/s7+7/7PDu/+zw7v/t7+7/7e/u/+3w
    7v/s8O7/7PDu/+3w7v/t8O//7fDv/+3v7v/s7+7/7PDu/+zw7v/s8O7/7PDu/+zw
    7v/r7+3/6+7t/+vu7f/r7u3/6+7s/+vu7P/q7uz/6u7s//7//v+Vm5j8n5+fCAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnaSh+/7+
    /v/r7+3/6+7t/+vu7f/r7+3/7O/t/+zv7f/s7+3/7PDu/+zv7v/s7+7/7e/u/+3v
    7v/s8O7/7PDu/+zw7v/s7+7/7e/u/+zv7f/s7+7/7O/u/+zv7v/s7u7/7O7t/+vu
    7f/r7u3/6+7t/+vu7P/q7ev/6e3r/+nt6//p7ev/6e3r//7+/v+WnJn8n5+fCAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnqWj+/7+
    /v/s7+3/6+7t/+vu7f/r7u3/6+/t/+zv7f/s7+3/6+/t/+zv7v/s7+7/7O/t/+zv
    7f/s7+7/7PDu/+zv7v/s7+7/7O/t/+zv7f/r7+3/6+/t/+vu7f/r7uz/6u7s/+ru
    7P/q7uz/6u3s/+rt6//p7ev/6e3r/+nt6//o7Or/5+vp//7+/v+Ynpv8n5+fCAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAoKak+/7+
    /v/s7+3/6+7t/+zu7f/r7u3/6+/t/+zv7f/s7+3/7O/t/+zv7f/s7+7/7O/u/+zv
    7f/s7+3/6+/t/+zv7v/s7+3/7O/t/+zv7f/r7+3/6+7t/+vu7f/r7uz/6u7s/+ru
    7P/q7ez/6u3r/+nt6//p7ev/6ezr/+js6v/n6+n/5+vp//7+/v+Zn5z8n5+fCAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAoqek+/7+
    /v/s8O7/6+/t/+vv7f/s7+3/7O/t/+zv7v/s7+3/7O/t/+zv7f/s7+3/6+/u/+zv
    7f/s7+3/7O/t/+vu7f/r7+3/6+/t/+vv7f/r7u3/6+7t/+vu7P/r7uz/6u7s/+rt
    7P/q7ev/6e3r/+jt6//o7Or/6Ovp/+fr6f/n6+n/5+rp//7+/v+boJ78n5+fCAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAoqmm+/7/
    /v/s8O7/7O/t/+zv7v/r7+3/7O/t/+zv7f/s7+7/7O/u/+zv7f/s7+3/7O/t/+zv
    7v/r7+3/7O/t/+zu7f/r7u3/6+7t/+vv7P/r7uz/6u7s/+ru7P/q7uv/6u3r/+nt
    6//p7ev/6e3q/+js6v/o6+r/6Ovp/+fr6f/n6un/5uro//n6+v+boZ78n5+fCAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAApKqn+/7/
    /v/t8O7/7O/t/+zv7f/s7+7/7O/u/+zv7v/s7+3/7O/u/+zv7v/s7+7/7O/t/+zv
    7f/s7+3/6+/t/+vv7f/r7u3/6+7t/+vu7P/r7uz/6u3s/+rt7P/q7ev/6e3r/+ns
    6//p7Or/6Ozq/+jr6v/n6+n/5+vp/+br6f/m6uj/5urn//P19P+boZ/8mZmZBQAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAApayp+/7/
    /v/t8O//7PDu/+zw7v/t7+7/7O/u/+zw7v/t8O7/7fDu/+zv7v/s7+7/7O/u/+zv
    7f/s7+3/7O/t/+vv7f/r7u3/6+7t/+vu7f/q7uz/6u3s/+rt7P/p7ev/6e3r/+ns
    6//o7Or/6Ozq/+fr6v/n6+n/5+vo/+bq6P/m6uj/4OXi/9/i4f+fpaPgAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAp62q+/7/
    ///u8O//7fDv/+3w7v/t8O7/7fDu/+3w7//t8O7/7PDu/+3w7v/t7+7/7O/u/+zv
    7v/s7+7/6+/t/+zv7f/r7+3/6+7s/+vu7P/q7uz/6u7s/+rt6//p7ev/6e3q/+jr
    6f/i5uT/3ODd/9XZ1//P09H/yc3K/7zAvv+nq6j/o6Wk/83Rz/+epaOhAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAqK+s+/7/
    ///u8PD/7vDv/+7x7//t8O//7fDv/+7w7//t8O//7fDv/+3w7v/t8O7/7fDu/+3v
    7v/s7+7/7O/u/+zv7f/r7+3/6+/t/+vu7P/q7uz/6u7s/+nt6//p7ev/6O3r/+To
    5v/Z3Nr/y8/N/7/Cwf+ztrX/pqmn/5qdnP+anJv/29/e/7G3tfKdpKFRAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAqbCt+///
    ///v8fD/7vHw/+7x8P/u8fD/7vHv/+7x7//u8e//7vHv/+7x7//t8O//7fDv/+3w
    7v/s8O7/7O/u/+zv7v/s7+3/6+/t/+vu7f/q7uz/6u7s/+rt7P/p7ev/6e3r/+Pn
    5f/X2tn/yc3L/73Bv/+ws7L/pKem/62wr//a3tv/19zZ/6Srp+P///8BAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAq7Kv+///
    ///w8vH/7/Lw/+/y8P/v8vD/7/Lw/+/x8P/u8fD/7vHw/+7x7//u8e//7vHv/+3w
    7//t8O7/7fDu/+zv7v/s7+3/6+/t/+vu7P/r7uz/6u3s/+ru7P/p7ev/6e3r/+Lm
    5P/V2Nf/z9LQ/8zPzv/Mzs3/1trX/+Xp5//i5+X/tbq59aCmo1kAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAArLOw+///
    ///w8/H/8PLx//Dy8f/w8/H/8PLx/+/y8f/v8vH/7/Lw/+/x8P/u8fD/7vHw/+7x
    7//t8O//7fDu/+3w7v/s7+7/7O/t/+vu7f/r7uz/6u7s/+rt6//p7ev/6ezq/9/j
    4f/S1tT/8/T0///////09vX/6Ovp/+Xp5//Dycb4p66rqQAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAArrSy+///
    ///x9PL/8fPy//Hz8v/x8/L/8fPy//Dz8f/w8/H/7/Lx//Dy8f/v8vD/7/Hw/+7x
    7//u8e//7fDv/+3w7//t8O7/7O/u/+zv7f/r7u3/6+7s/+rt7P/p7ev/6ezr/97i
    4P/R1NP/9vf3//r7+v/u8fD/5enn/87T0fuor6zVn5+fCAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAr7az+///
    ///y9PP/8vTz//L08//y9PP/8fPy//H08v/x8/L/8PPy//Dy8f/w8vH/7/Lw/+/x
    8P/u8fD/7vHv/+3w7//t8O7/7O/u/+zv7f/r7u3/6u7t/+ru7P/p7ev/6ezr/9zg
    3v/Q1NP/+/z8//P19f/n7On/19zZ/6qwru2nsacaAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAsbe0+///
    ///z9vT/8/X0//P19P/z9fT/8vT0//L08//y9PP/8fTy//Hz8v/w8/L/8PLx/+/y
    8f/v8fD/7vHw/+7x7//t8O//7fDu/+zv7f/r7+3/6+7s/+ru7P/p7ev/6Ozp/9nd
    2//m6Of/+Pn4/+zv7v/Z3dv/rLKv9aitrTgAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAsrm2+///
    ///19/b/9Pb1//T29f/09vX/8/X0//P19P/y9PT/8vTz//L08//x8/L/8PPy//Dy
    8f/v8vH/7/Hw/+7x8P/t8O//7fDu/+zv7v/r7u3/6+7s/+ru7P/p7ev/5Onn/9/i
    4f/4+fj/7/Lx/9bb2f2stLDwqq6uOQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAtLq3+///
    ///2+Pf/9vf3//X39v/19/b/9Pb1//T29f/z9fT/8/X0//L08//x9PP/8fPy//Dz
    8f/w8vH/7/Lw/+7x8P/u8O//7fDv/+zv7v/s7+3/6+7t/+rt7P/p7ev/7e/u//j5
    +f/x8/L/09jW+q+2suKqsrIhAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAtry58/7+
    /v/4+vn/9/j4//f49//2+Pf/9ff2//X29v/09vX/9PX0//P19P/y9PP/8fTz//Hz
    8v/w8vH/7/Lx/+7x8P/u8e//7fDu/+zv7v/s7+3/6+7s/+7w7//29/b/+fr5/+vu
    7f/K0c33sLe0xqq7qg8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAtr67lujq
    6f3/////////////////////////////////////////////////////////////
    ////////////////////////////////////////+/z7//T29f/m6en/zdLO97S4
    tvKvuLR9v7+/BAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAtb+/GLm/
    vJy3vrv3tr26/7a9uv+1vLn/tby5/7W8uf+1vLn/tLu4/7S7uP+0u7j/tLu4/7O6
    t/+zurf/s7q3/7O5tv+yubb/srm2/7K5tv+xuLX/sbi1/LO7uOGzura2sLezbrOz
    swoAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAA7wEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAYAHADfBwkAFQAXAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    AAAAAAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEiQAAAAAAABUUlVFVklT
    SU9OLVhGSUxFLgA=
}

set t [blt::datatable create]

foreach f [lsort [glob -nocomplain ~/*]] {
    set name [file tail $f]
    set ext [file ext $name]
    set ext [string trimleft $ext .]
    if { [file isdir $f] } {
	set ext dir
    }
    set row [$t row extend 1]
    $t row set $row "name" $name "icon" "smallIcon" "type" $ext \
	"bigicon" "bigIcon"
    $t row label $row "$name"
}

.ss.listview table attach $t \
    -text "name" \
    -icon "icon" \
    -bigicon "bigicon" \
    -type "type"

blt::tk::scrollbar .ss.xs
blt::tk::scrollbar .ss.ys

blt::table . \
    0,0 .layout_l -anchor e \
    0,1 .layout -anchor w \
    1,0 .ss -fill both -cspan 2 

blt::table configure . r0 c0 -resize none

$m select "columns"
