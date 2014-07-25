namespace eval blt::Filmstrip {
    set buttonPressed 0
    proc Initialize {} {
    }
}

bind BltFilmstripGrip <Enter> { 
    if { !$blt::Filmstrip::buttonPressed } { 
	[winfo parent %W] handle activate %W
    } 
} 
bind BltFilmstripGrip <Leave> { 
    if { !$blt::Filmstrip::buttonPressed } { 
	[winfo parent %W] handle deactivate
    } 
}
bind BltFilmstripGrip <KeyPress-Left> { 
    [winfo parent %W] handle move %W -10 0 
}
bind BltFilmstripGrip <KeyPress-Right> { 
    [winfo parent %W] handle move %W 10  0 
}
bind BltFilmstripGrip <KeyPress-Up> { 
    [winfo parent %W] handle move %W 0 -10 
}
bind BltFilmstripGrip <KeyPress-Down> { 
    [winfo parent %W] handle move %W 0 10 
}
bind BltFilmstripGrip <Shift-KeyPress-Left> { 
    [winfo parent %W] handle move %W -100 0 
}
bind BltFilmstripGrip <Shift-KeyPress-Right> { 
    [winfo parent %W] handle move %W 100  0 
}
bind BltFilmstripGrip <Shift-KeyPress-Up> { 
    [winfo parent %W] handle move %W 0 -100 
}
bind BltFilmstripGrip <Shift-KeyPress-Down> { 
    [winfo parent %W] handle move %W 0 100
}
bind BltFilmstripGrip <ButtonPress-1> { 
    set blt::Filmstrip::buttonPressed 1
    [winfo parent %W] handle anchor %W %X %Y 
}
bind BltFilmstripGrip <B1-Motion> { 
    [winfo parent %W] handle mark %W %X %Y 
}
bind BltFilmstripGrip <ButtonRelease-1> { 
    set blt::Filmstrip::buttonPressed 0
    [winfo parent %W] handle set %W %X %Y 
}

