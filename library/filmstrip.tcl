namespace eval blt::Filmstrip {
    set buttonPressed 0
    proc Initialize {} {
    }
}

bind BltFilmstripGrip <Enter> { 
    if { !$blt::Filmstrip::buttonPressed } { 
	%W activate 
    } 
} 
bind BltFilmstripGrip <Leave> { 
    if { !$blt::Filmstrip::buttonPressed } { 
	%W deactivate
    } 
}
bind BltFilmstripGrip <KeyPress-Left> { 
    %W move -10 0 
}
bind BltFilmstripGrip <KeyPress-Right> { 
    %W move 10  0 
}
bind BltFilmstripGrip <KeyPress-Up> { 
    %W move 0 -10 
}
bind BltFilmstripGrip <KeyPress-Down> { 
    %W move 0 10 
}
bind BltFilmstripGrip <Shift-KeyPress-Left> { 
    %W move -100 0 
}
bind BltFilmstripGrip <Shift-KeyPress-Right> { 
    %W move 100  0 
}
bind BltFilmstripGrip <Shift-KeyPress-Up> { 
    %W move 0 -100 
}
bind BltFilmstripGrip <Shift-KeyPress-Down> { 
    %W move 0 100
}
bind BltFilmstripGrip <ButtonPress-1> { 
    set blt::Filmstrip::buttonPressed 1
    %W anchor %X %Y 
}
bind BltFilmstripGrip <B1-Motion> { 
    %W mark %X %Y 
}
bind BltFilmstripGrip <ButtonRelease-1> { 
    set blt::Filmstrip::buttonPressed 0
    %W set %X %Y 
}

