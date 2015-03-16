# -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- 
#
# paneset.tcl
#
#      Copyright 2009 George A Howlett.
#
#      Permission is hereby granted, free of charge, to any person
#      obtaining a copy of this software and associated documentation files
#      (the "Software"), to deal in the Software without restriction,
#      including without limitation the rights to use, copy, modify, merge,
#      publish, distribute, sublicense, and/or sell copies of the Software,
#      and to permit persons to whom the Software is furnished to do so,
#      subject to the following conditions:
#
#      The above copyright notice and this permission notice shall be
#      included in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#      EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#      NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
#      BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
#      ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#      CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#      SOFTWARE.
#

namespace eval blt {
    namespace eval Paneset {
	variable _private 
	array set _private {
	    buttonPressed 0
	}
	proc Initialize {} {
	}
    }
}

bind BltPanesetSash <Enter> { 
    if { !$blt::Paneset::_private(buttonPressed) } { 
	[winfo parent %W] handle activate %W
    } 
} 

bind BltPanesetSash <Leave> { 
    if { !$blt::Paneset::_private(buttonPressed) } { 
	[winfo parent %W] handle deactivate
    } 
}

bind BltPanesetSash <KeyPress-Left> { 
    [winfo parent %W] handle move %W -10 0 
}

bind BltPanesetSash <KeyPress-Right> { 
    [winfo parent %W] handle move %W 10  0 
}

bind BltPanesetSash <KeyPress-Up> { 
    [winfo parent %W] handle move %W 0 -10 
}

bind BltPanesetSash <KeyPress-Down> { 
    [winfo parent %W] handle move %W 0 10 
}

bind BltPanesetSash <ButtonPress-1> { 
    set blt::Paneset::_private(buttonPressed) 1
    [winfo parent %W] handle anchor %W %X %Y 
}

bind BltPanesetSash <B1-Motion> { 
    [winfo parent %W] handle mark %W %X %Y 
}

bind BltPanesetSash <ButtonRelease-1> { 
    set blt::Paneset::_private(buttonPressed) 0
    [winfo parent %W] handle set %W %X %Y 
}
