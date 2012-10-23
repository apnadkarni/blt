
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
	%W activate 
    } 
} 

bind BltPanesetSash <Leave> { 
    if { !$blt::Paneset::_private(buttonPressed) } { 
	%W deactivate
    } 
}

bind BltPanesetSash <KeyPress-Left> { 
    %W move -10 0 
}

bind BltPanesetSash <KeyPress-Right> { 
    %W move 10  0 
}

bind BltPanesetSash <KeyPress-Up> { 
    %W move 0 -10 
}

bind BltPanesetSash <KeyPress-Down> { 
    %W move 0 10 
}

bind BltPanesetSash <ButtonPress-1> { 
    set blt::Paneset::_private(buttonPressed) 1
    %W anchor %X %Y 
}

bind BltPanesetSash <B1-Motion> { 
    %W mark %X %Y 
}

bind BltPanesetSash <ButtonRelease-1> { 
    set blt::Paneset::_private(buttonPressed) 0
    %W set %X %Y 
}
