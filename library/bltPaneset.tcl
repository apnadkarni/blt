# -*- mode: tcl; tcl-indent-level: 4; indent-tabs-mode: nil -*- 
#
# bltPaneset.tcl
#
# Copyright 2015 George A. Howlett. All rights reserved.  
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#   1) Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#   2) Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the
#      distribution.
#   3) Neither the name of the authors nor the names of its contributors
#      may be used to endorse or promote products derived from this
#      software without specific prior written permission.
#   4) Products derived from this software may not be called "BLT" nor may
#      "BLT" appear in their names without specific prior written
#      permission from the author.
#
#   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
#   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
#   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
	[winfo parent %W] sash activate %W
    } 
} 

bind BltPanesetSash <Leave> { 
    if { !$blt::Paneset::_private(buttonPressed) } { 
	[winfo parent %W] sash deactivate
    } 
}

bind BltPanesetSash <KeyPress-Left> { 
    [winfo parent %W] sash move %W -10 0 
}

bind BltPanesetSash <KeyPress-Right> { 
    [winfo parent %W] sash move %W 10  0 
}

bind BltPanesetSash <KeyPress-Up> { 
    [winfo parent %W] sash move %W 0 -10 
}

bind BltPanesetSash <KeyPress-Down> { 
    [winfo parent %W] sash move %W 0 10 
}

bind BltPanesetSash <ButtonPress-1> { 
    set blt::Paneset::_private(buttonPressed) 1
    [winfo parent %W] sash anchor %W %X %Y 
}

bind BltPanesetSash <B1-Motion> { 
    [winfo parent %W] sash mark %W %X %Y 
}

bind BltPanesetSash <ButtonRelease-1> { 
    set blt::Paneset::_private(buttonPressed) 0
    [winfo parent %W] sash set %W %X %Y 
}
