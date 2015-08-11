# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# bltDrawerset.tcl
#
# Bindings for the BLT drawerset widget
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
    namespace eval Drawerset {
	variable _private
	array set _private {
	    buttonDown 0
	}
	proc Initialize {} {
	}
    }
}

bind BltDrawerHandle <Enter> { 
    if { !$blt::Drawerset::_private(buttonDown) } { 
	[winfo parent %W] handle activate %W
    } 
} 
bind BltDrawerHandle <Leave> { 
    if { !$blt::Drawerset::_private(buttonDown) } { 
	[winfo parent %W] handle deactivate 
    } 
}
bind BltDrawerHandle <KeyPress-Left> { 
    [winfo parent %W] resize %W 10 0
}
bind BltDrawerHandle <KeyPress-Right> { 
    [winfo parent %W] resize %W 10 0
}
bind BltDrawerHandle <KeyPress-Up> { 
    [winfo parent %W] resize %W 0 10 
}
bind BltDrawerHandle <KeyPress-Down> { 
    [winfo parent %W] resize %W 0 -10 
}
bind BltDrawerHandle <ButtonPress-1> { 
    set blt::Drawerset::_private(buttonDown) 1
    [winfo parent %W] handle anchor %W %X %Y 
}
bind BltDrawerHandle <B1-Motion> { 
    [winfo parent %W] handle mark %W %X %Y 
}
bind BltDrawerHandle <ButtonRelease-1> { 
    set blt::Drawerset::_private(buttonDown) 0
    [winfo parent %W] handle set %W %X %Y 
}

