
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
    [winfo parent %W] handle move %W -10 0 
}
bind BltDrawerHandle <KeyPress-Right> { 
    [winfo parent %W] handle move %W 10  0 
}
bind BltDrawerHandle <KeyPress-Up> { 
    [winfo parent %W] handle move %W 0 -10 
}
bind BltDrawerHandle <KeyPress-Down> { 
    [winfo parent %W] handle move %W 0 10 
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

