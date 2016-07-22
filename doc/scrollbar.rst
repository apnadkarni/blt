==================
blt::tk::scrollbar
==================

---------------------------------------
Create and manipulate scrollbar widgets
---------------------------------------

.. include:: man.rst
.. include:: toc.rst

SYNOPSIS
--------

**blt::tk::scrollbar** *pathName* ?\ *option value* ... ?

DESCRIPTION
-----------

The **blt::tk::scrollbar** command creates a new window (given by the
*pathName* argument) and makes it into a scrollbar widget.

Additional options, described above, may be specified on the command line
or in the option database to configure aspects of the scrollbar such as its
colors, orientation, and relief.  The **blt::tk::scrollbar** command
returns its *pathName* argument.  At the time this command is invoked,
there must not exist a window named *pathName*, but *pathName*\ 's parent
must exist.

A scrollbar is a widget that displays two arrows, one at each end of the
scrollbar, and a *slider* in the middle portion of the scrollbar.  It
provides information about what is visible in an *associated window* that
displays a document of some sort (such as a file being edited or a
drawing).  The position and size of the slider indicate which portion of
the document is visible in the associated window.  For example, if the
slider in a vertical scrollbar covers the top third of the area between the
two arrows, it means that the associated window displays the top third of
its document.

Scrollbars can be used to adjust the view in the associated window
by clicking or dragging with the mouse.  See the **BINDINGS** section
below for details.

ELEMENTS
--------

A scrollbar displays five elements, which are referred to in the
widget commands for the scrollbar:

**arrow1**
  The top or left arrow in the scrollbar.
**trough1**
  The region between the slider and **arrow1**.
**slider**
  The rectangle that indicates what is visible in the associated widget.
**trough2**
  The region between the slider and **arrow2**.
**arrow2**
  The bottom or right arrow in the scrollbar.

SYNTAX
------

  **blt::tk::scrollbar** *pathName* ?\ *option value* ... ?

The **blt::tk::scrollbar** command creates a new Tcl command whose name is
*pathName*. 

OPERATIONS
----------

All *scrollbar* operations are invoked by specifying the widget's pathname,
the operation, and any arguments that pertain to that operation.  The
general form is:

  *pathName* *operation* ?\ *arg* *arg* ... ?

*Operation* and the *arg*\ s determine the exact behavior of the command.
The following operations are available for scrollbar widgets:

*pathName* **activate** ?\ *element*\ ?
  Marks the element indicated by *element* as active, which causes it to be
  displayed as specified by the **-activebackground** and **-activerelief**
  options.  The only element values understood by this command are
  **arrow1**, **slider**, or **arrow2**.  If any other value is specified
  then no element of the scrollbar will be active.  If *element* is not
  specified, the command returns the name of the element that is currently
  active, or an empty string if no element is active.

*pathName* **cget** *option*
  Returns the current value of the configuration option given by *option*.
  *Option* may have any of the values accepted by the *scrollbar*
  widget (see the **configure** operation below).

*pathName* **configure** ?\ *option*\ ? ?\ *value option value*\  ...?
  Query or modify the configuration options of the widget.
  If no *option* is specified, returns a list describing all of
  the available options for *pathName* (see **Tk_ConfigureInfo** for
  information on the format of this list).  If *option* is specified
  with no *value*, then the command returns a list describing the
  one named option (this list will be identical to the corresponding
  sublist of the value returned if no *option* is specified).  If
  one or more \fIoption-value\fR pairs are specified, then the command
  modifies the given widget option(s) to have the given value(s);  in
  this case the command returns an empty string.
  *Option* may have any of the values accepted by the **scrollbar**
  command.

  Widget configuration options may be set either by the **configure** 
  operation or the Tk **option** command.  The resource class
  is **BltTkScrollbar**.  The resource name is the name of the widget::

    option add *BltTkScrollbar.background grey80
    option add *BltTkScrollbar.Background white

  The following widget options are available:

  **-activebackground** *bgName*
    Specifies the background color of *pathName* when it is active.
    *BgName* may be a color name or the name of a background object
    created by the **blt::background** command.  The default is "white".

  **-activerelief** *reliefName*
    Specifies the relief to use when displaying the element that is active,
    if any. Specifies the 3-D effect for *pathName*.  *ReliefName*
    indicates how the element appear relative to the *scrollbar*
    window. Acceptable values are **raised**, **sunken**, **flat**,
    **ridge**, **solid**, and **groove**. For example, "raised" means
    element should appear to protrude.  The default is "raised".  Elements
    other than the active element are always displayed with a raised
    relief.

  **-background** *bgName*
    Specifies the background color of *pathName*.  *BgName* may be a
    color name or the name of a background object created by the
    **blt::background** command.  The default is "white".

  **-borderwidth** *numPixels*
    Specifies the borderwidth of *pathName*.  *NumPixels* is a non-negative
    value indicating the width of the 3-D border drawn around the button.
    *NumPixels* may have any of the forms acceptable to **Tk_GetPixels**.
    The default is "1".

  **-command** *cmdPrefix*
    Specifies the prefix of a Tcl command to invoke to change the view
    in the widget associated with the scrollbar.  When a user requests
    a view change by manipulating the scrollbar, a Tcl command is
    invoked.  The actual command consists of this option followed by
    additional information as described later.  This option almost always has
    a value such as **.t xview** or **.t yview**, consisting of the
    name of a widget and either **xview** (if the scrollbar is for
    horizontal scrolling) or **yview** (for vertical scrolling).
    All scrollable widgets have **xview** and **yview** commands
    that take exactly the additional arguments appended by the scrollbar
    as described in SCROLLING COMMANDS below.

  **-cursor** *cursorName*
    Specifies the cursor to be used for the widget. *CursorName* may have
    any of the forms acceptable to **Tk_GetCursor**.  If *cursorName* is "",
    this indicates that the widget should defer to its parent for cursor
    specification.  The default is "".

  **-elementborderwidth** *numPixels*
    Specifies the width of borders drawn around the internal elements of
    the scrollbar (the two arrows and the slider).  The value may have any
    of the forms acceptable to **Tk_GetPixels**.  If this value is less
    than zero, the value of the **-borderWidth** option is used in its
    place.

  **-highlightbackground** *bgName*
    Specifies the color of the traversal highlight region when the
    scrollbar does not have the input focus.  *BgName* may be a color name
    or the name of a background object created by the **blt::background**
    command.  The default is "grey85".
    FIXME
    
  **-highlightcolor** *bgName*
    Specifies the color of the traversal highlight region when *pathName*
    has input focus.  *BgName* may be a color name or the name of a
    background object created by the **blt::background** command. The
    default is "black".

  **-highlightthickness** *numPixels*
    Specifies a non-negative value for the width of the highlight rectangle
    to drawn around the outside of the widget.  *NumPixels* may have any of
    the forms acceptable to **Tk_GetPixels**.  If *numPixels* is "0", no
    focus highlight is drawn around the widget.  The default is "2".

  **-jump** *boolean*
     For widgets with a slider that can be dragged to adjust a value, such
     as scrollbars, this option determines when notifications are made
     about changes in the value.  The option's value must be a boolean of
     the form accepted by Tcl_GetBoolean.  If the value is false, updates
     are made continuously as the slider is dragged.  If the value is true,
     updates are delayed until the mouse button is released to end the
     drag; at that point a single notification is made (the value "jumps"
     rather than changing smoothly).

  **-orient** *orientName*
    Specifies the orientation of the *scrollbar*.  *OrientName* may be
    "vertical" (scrollbar runs left to right) or "horizontal" (scrollbar
    runs top to bottom).  The default is "horizontal".
    
  **-relief** *reliefName*
    Specifies the 3-D effect for *pathName*.  *ReliefName* indicates how
    the menu should appear relative to the window it's packed
    into. Acceptable values are **raised**, **sunken**, **flat**,
    **ridge**, **solid**, and **groove**. For example, "raised" means
    *pathName* should appear to protrude.  The default is "raised".

  **-repeatdelay**

  **-repeatinterval** 

  **-takefocus** *boolean*
    Provides information used when moving the focus from window to window
    via keyboard traversal (e.g., Tab and Shift-Tab).  If *boolean* is "0",
    this means that this window should be skipped entirely during keyboard
    traversal.  "1" means that the this window should always receive the
    input focus.  An empty value means that the traversal scripts make the
    decision whether to focus on the window.  The default is "".

  **-troughcolor** *colorName*

  **-width** *numPixels*
    Specifies the desired narrow dimension of the scrollbar window,
    not including 3-D border, if any.  For vertical scrollbars this will be
    the width and for horizontal scrollbars this will be the height.  The
    value may have any of the forms acceptable to **Tk_GetPixels**.


*pathName* **delta** *deltaX deltaY*
  Returns a real number indicating the fractional change in the scrollbar
  setting that corresponds to a given change in slider position.  For
  example, if the scrollbar is horizontal, the result indicates how much
  the scrollbar setting must change to move the slider *deltaX* pixels to
  the right (*deltaY* is ignored in this case).  If the scrollbar is
  vertical, the result indicates how much the scrollbar setting must change
  to move the slider *deltaY* pixels down.  The arguments and the result
  may be zero or negative.

*pathName* **fraction** *x y*
  Returns a real number between 0 and 1 indicating where the point given by
  *x* and *y* lies in the trough area of the scrollbar.  The value 0
  corresponds to the top or left of the trough, the value 1 corresponds to
  the bottom or right, 0.5 corresponds to the middle, and so on.  *X* and
  *y* must be pixel coordinates relative to the scrollbar widget.  If *x*
  and *y* refer to a point outside the trough, the closest point in the
  trough is used.

*pathName* **get**
  Returns the scrollbar settings in the form of a list whose
  elements are the arguments to the most recent **set** operation.

*pathName* **identify** *x y*
  Returns the name of the element under the point given by *x* and *y*
  (such as **arrow1**), or an empty string if the point does not lie in any
  element of the scrollbar.  *X* and *y* must be pixel coordinates relative
  to the scrollbar widget.

*pathName* **set** *first last*
  This command is invoked by the scrollbar's associated widget to tell the
  scrollbar about the current view in the widget.  The command takes two
  arguments, each of which is a real fraction between 0 and 1.  The
  fractions describe the range of the document that is visible in the
  associated widget.  For example, if *first* is 0.2 and *last* is 0.4, it
  means that the first part of the document visible in the window is 20% of
  the way through the document, and the last visible part is 40% of the way
  through.

SCROLLING COMMANDS
------------------

When the user interacts with the scrollbar, for example by dragging the
slider, the scrollbar notifies the associated widget that it must change
its view.  The scrollbar makes the notification by evaluating a Tcl command
generated from the scrollbar's **-command** option.  The command may take
any of the following forms.  In each case, *prefix* is the contents of the
**-command** option, which usually has a form like **.t yview**

*cmdPrefix* **moveto** *fraction*
  *Fraction* is a real number between 0 and 1.  The widget should adjust
  its view so that the point given by *fraction* appears at the beginning
  of the widget.  If *fraction* is 0 it refers to the beginning of the
  document.  1.0 refers to the end of the document, 0.333 refers to a point
  one-third of the way through the document, and so on.

*cmdPrefix* **scroll** *number* **units**
  The widget should adjust its view by *number* units.  The units are
  defined in whatever way makes sense for the widget, such as characters or
  lines in a text widget.  *Number* is either 1, which means one unit
  should scroll off the top or left of the window, or -1, which means that
  one unit should scroll off the bottom or right of the window.

*cmdPrefix* **scroll** *number* **pages**
  The widget should adjust its view by *number* pages.  It is up to the
  widget to define the meaning of a page; typically it is slightly less
  than what fits in the window, so that there is a slight overlap between
  the old and new views.  *Number* is either 1, which means the next page
  should become visible, or -1, which means that the previous page should
  become visible.

OLD COMMAND SYNTAX
------------------

In versions of Tk before 4.0, the **set** and **get** widget commands used
a different form.  This form is still supported for backward compatibility,
but it is deprecated.  In the old command syntax, the **set** widget
command has the following form:

*pathName* **set** *totalUnits windowUnits firstUnit lastUnit*
  In this form the arguments are all integers.  *TotalUnits* gives the
  total size of the object being displayed in the associated widget.  The
  meaning of one unit depends on the associated widget; for example, in a
  text editor widget units might correspond to lines of text.
  *WindowUnits* indicates the total number of units that can fit in the
  associated window at one time.  *FirstUnit* and *lastUnit* give the
  indices of the first and last units currently visible in the associated
  window (zero corresponds to the first unit of the object).

  Under the old syntax the **get** widget command returns a list
  of four integers, consisting of the *totalUnits*, *windowUnits*,
  *firstUnit*, and *lastUnit* values from the last **set**
  widget command.

  The commands generated by scrollbars also have a different form
  when the old syntax is being used:

*prefix* *unit*
  *Unit* is an integer that indicates what should appear at
  the top or left of the associated widget's window.
  It has the same meaning as the *firstUnit* and *lastUnit*
  arguments to the **set** widget command.

The most recent **set** widget command determines whether or not to use the
old syntax.  If it is given two real arguments then the new syntax will be
used in the future, and if it is given four integer arguments then the old
syntax will be used.

BINDINGS
--------

Tk automatically creates class bindings for scrollbars that give them
the following default behavior.
If the behavior is different for vertical and horizontal scrollbars,
the horizontal behavior is described in parentheses.

**<ButtonPress-1>**
  Pressing button 1 over **arrow1** causes the view in the
  associated widget to shift up (left) by one unit so that the
  document appears to move down (right) one unit.
  If the button is held down, the action auto-repeats.

  Pressing button 1 over **trough1** causes the view in the
  associated widget to shift up (left) by one screenful so that the
  document appears to move down (right) one screenful.
  If the button is held down, the action auto-repeats.

  Pressing button 1 over the slider and dragging causes the view to drag
  with the slider.  If the **jump** option is true, then the view doesn't
  drag along with the slider; it changes only when the mouse button is
  released.

  Pressing button 1 over **trough2** causes the view in the
  associated widget to shift down (right) by one screenful so that the
  document appears to move up (left) one screenful.
  If the button is held down, the action auto-repeats.

  Pressing button 1 over **arrow2** causes the view in the
  associated widget to shift down (right) by one unit so that the
  document appears to move up (left) one unit.
  If the button is held down, the action auto-repeats.

**<ButtonPress-2>**
  If button 2 is pressed over the trough or the slider, it sets
  the view to correspond to the mouse position;  dragging the
  mouse with button 2 down causes the view to drag with the mouse.
  If button 2 is pressed over one of the arrows, it causes the
  same behavior as pressing button 1.

**<Control-ButtonPress-1>**
  If button 1 is pressed with the Control key down, then if the
  mouse is over **arrow1** or **trough1** the view changes
  to the very top (left) of the document;  if the mouse is over
  **arrow2** or **trough2** the view changes
  to the very bottom (right) of the document;  if the mouse is
  anywhere else then the button press has no effect.

**<KeyPress-Up>**

**<KeyPress-Down>**
  In vertical scrollbars the Up and Down keys have the same behavior
  as mouse clicks over **arrow1** and **arrow2**, respectively.
  In horizontal scrollbars these keys have no effect.

**<Control-KeyPress-Up>**

**<Control-KeyPress-Down>**
  In vertical scrollbars Control-Up and Control-Down have the same
  behavior as mouse clicks over **trough1** and **trough2**, respectively.
  In horizontal scrollbars these keys have no effect.

  In horizontal scrollbars the Up and Down keys have the same behavior
  as mouse clicks over **arrow1** and **arrow2**, respectively.
  In vertical scrollbars these keys have no effect.

  In horizontal scrollbars Control-Up and Control-Down have the same
  behavior as mouse clicks over **trough1** and **trough2**, respectively.
  In vertical scrollbars these keys have no effect.

**<KeyPress-Prior>**

**<KeyPress-Next>**
  The Prior and Next keys have the same behavior
  as mouse clicks over **trough1** and **trough2**, respectively.

**<KeyPress-Home>**
  The Home key adjusts the view to the top (left edge) of the document.

**<KeyPress-End>**
  The End key adjusts the view to the bottom (right edge) of the document.

EXAMPLE
-------

Create a window with a scrollable **text** widget:

 ::

   package require BLT
   
   toplevel .tl
   text .tl.t -yscrollcommand {.tl.s set}
   blt::tk::scrollbar .tl.s -command {.tl.t yview}
   grid .tl.t .tl.s -sticky nsew
   grid columnconfigure .tl 0 -weight 1
   grid rowconfigure .tl 0 -weight 1

KEYWORDS
--------

scrollbar, widget
