# -*- mode: tcl; indent-tabs-mode: nil -*- 
#
# bltFilmstrip.tcl
#
# Bindings for the BLT filmstrip widget.
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

namespace eval blt::Filmstrip {
    proc Initialize {} {
        # Dummy procedure to determine if this file has been loaded.
    }
}

bind BltFilmstripGrip <Enter> { 
    focus %W
    [winfo parent %W] grip activate %W
} 
bind BltFilmstripGrip <Leave> { 
    [winfo parent %W] grip deactivate
}
bind BltFilmstripGrip <KeyPress-Left> { 
    [winfo parent %W] grip move %W -10 0 
}
bind BltFilmstripGrip <KeyPress-Right> { 
    [winfo parent %W] grip move %W 10  0 
}
bind BltFilmstripGrip <KeyPress-Up> { 
    [winfo parent %W] grip move %W 0 -10 
}
bind BltFilmstripGrip <KeyPress-Down> { 
    [winfo parent %W] grip move %W 0 10 
}
bind BltFilmstripGrip <Shift-KeyPress-Left> { 
    [winfo parent %W] grip move %W -100 0 
}
bind BltFilmstripGrip <Shift-KeyPress-Right> { 
    [winfo parent %W] grip move %W 100  0 
}
bind BltFilmstripGrip <Shift-KeyPress-Up> { 
    [winfo parent %W] grip move %W 0 -100 
}
bind BltFilmstripGrip <Shift-KeyPress-Down> { 
    [winfo parent %W] grip move %W 0 100
}
bind BltFilmstripGrip <ButtonPress-1> { 
    [winfo parent %W] grip anchor %W %X %Y 
}
bind BltFilmstripGrip <B1-Motion> { 
    [winfo parent %W] grip mark %W %X %Y 
}
bind BltFilmstripGrip <ButtonRelease-1> { 
    [winfo parent %W] grip set %W %X %Y 
}

