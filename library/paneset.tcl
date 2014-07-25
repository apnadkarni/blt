
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
