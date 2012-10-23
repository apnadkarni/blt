
namespace eval blt {
    namespace eval Drawerset {
	variable _private
	array set _private {
	    buttonDown 0
	}
    }
}

bind BltDrawerHandle <Enter> { 
    if { !$blt::Drawerset::_private(buttonDown) } { 
	%W activate 
    } 
} 
bind BltDrawerHandle <Leave> { 
    if { !$blt::Drawerset::_private(buttonDown) } { 
	%W deactivate
    } 
}
bind BltDrawerHandle <KeyPress-Left> { 
    %W move -10 0 
}
bind BltDrawerHandle <KeyPress-Right> { 
    %W move 10  0 
}
bind BltDrawerHandle <KeyPress-Up> { 
    %W move 0 -10 
}
bind BltDrawerHandle <KeyPress-Down> { 
    %W move 0 10 
}
bind BltDrawerHandle <ButtonPress-1> { 
    set blt::Drawerset::_private(buttonDown) 1
    %W anchor %X %Y 
}
bind BltDrawerHandle <B1-Motion> { 
    %W mark %X %Y 
}
bind BltDrawerHandle <ButtonRelease-1> { 
    set blt::Drawerset::_private(buttonDown) 0
    %W set %X %Y 
}

